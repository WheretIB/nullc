#include "RichTextarea.h"

#include "ObjectPool.h"

#include <windowsx.h>

#ifdef _WIN64
	#define WND_PTR_TYPE LONG_PTR
#else
	#define WND_PTR_TYPE LONG
#endif

enum FontStyle
{
	FONT_REGULAR,
	FONT_BOLD,
	FONT_ITALIC,
	FONT_UNDERLINED,
};

struct TextStyle
{
	char	color[3];
	char	font;
};
TextStyle	tStyle[FONT_STYLE_COUNT];

struct AreaChar
{
	char	ch;
	char	style;
};

struct AreaLine
{
	AreaLine()
	{
		length = 0;
		maxLength = DEFAULT_STRING_LENGTH;
		data = startBuf;
		prev = next = NULL;
	}

	AreaChar		startBuf[DEFAULT_STRING_LENGTH];
	AreaChar		*data;
	unsigned int	length;
	unsigned int	maxLength;

	AreaLine		*prev, *next;

	static ObjectBlockPool<AreaLine, 32>	*pool;
	void*		operator new(size_t size)
	{
		(void)size;
		return pool->Create();
	}
	void		operator delete(void *ptr, size_t size)
	{
		(void)size;
		pool->Destroy((AreaLine*)ptr);
	}
};

ObjectBlockPool<AreaLine, 32>*	AreaLine::pool = NULL;

void ExtendLine(AreaLine *line, unsigned int size)
{
	// If there isn't enough space
	if(size > line->maxLength)
	{
		AreaChar	*nChars = new AreaChar[size + size / 2];
		line->maxLength = size + size / 2;
		// Clear memory
		memset(nChars, 0, sizeof(AreaChar) * line->maxLength);
		// Copy old data
		memcpy(nChars, line->data, line->length * sizeof(AreaChar));
		// Delete old data if it's not an internal buffer
		if(line->data != line->startBuf)
			delete[] line->data;
		line->data = nChars;
	}
}

AreaLine* InsertLineAfter(AreaLine *curr)
{
	AreaLine *nLine = new AreaLine();
	// Link new line with others
	if(curr && curr->next)
	{
		curr->next->prev = nLine;
		nLine->next = curr->next;
	}else{
		nLine->next = NULL;
	}
	if(curr)
		curr->next = nLine;
	nLine->prev = curr;
	return nLine;
}

void DeleteLine(AreaLine *dead)
{
	if(dead->prev)
		dead->prev->next = dead->next;
	if(dead->next)
		dead->next->prev = dead->prev;
	if(dead->data != dead->startBuf)
		delete[] dead->data;
	delete dead;
}

// Get shift after symbol ch in characters from current position to the next
int GetCharShift(char ch, int currPos)
{
	return ch == '\t' ? TAB_SIZE - currPos % TAB_SIZE : 1;
}

bool IsPressed(int key)
{
	return !!(GetKeyState(key) & 0x80000000);
}

int AdvanceCursor(AreaLine *line, int cursorX, bool left)
{
	// Advance direction
	int dir = left ? -1 : 1;

	// If out of bounds, return as it is
	if(line->length == 0 || (left && cursorX == 0) || cursorX > (int)(line->length)-dir)
		return cursorX;

	// Find, what character is at the cursor position
	char symb = line->data[cursorX + (left ? -1 : 0)].ch;

	int minX = left ? 1 : 0;
	int maxX = left ? line->length : line->length - 2;

	// If it's a digit, move to the left, skipping all digits and '.'
	if(isdigit(symb) || symb == '.')
	{
		while((cursorX >= minX && cursorX <= maxX) && (isdigit(line->data[cursorX + dir].ch) || line->data[cursorX + dir].ch == '.'))
			cursorX += dir;
	}else if(isalpha(symb) || symb == '_'){	// If it's an alphanumerical or '_', move to the left, skipping all of them
		while((cursorX >= minX && cursorX <= maxX) && (isalnum(line->data[cursorX + dir].ch) || line->data[cursorX + dir].ch == '_'))
			cursorX += dir;
	}else{
		if(cursorX != 0 && left)
			cursorX--;
	}

	return cursorX;
}

int CALLBACK InitFont(const LOGFONTA *lpelfe, const TEXTMETRICA *lpntme, DWORD FontType, LPARAM lParam)
{
	(void)lpelfe; (void)lpntme; (void)FontType;

	int &found = *(int*)lParam;
	found = true;
	return 0;
}

namespace RichTextarea
{
	bool	sharedCreated = false;

	// Global shared data
	HWND	areaStatus = 0;
	//HWND	activeWnd = 0;
	HFONT	areaFont[4];

	PAINTSTRUCT areaPS;

	// Padding to the left of the first symbol
	unsigned int padLeft = 5;
	// Single character width and height (using monospaced font)
	int charWidth, charHeight;

	// A few pens and brushes for rendering
	HPEN	areaPenWhite1px, areaPenBlack1px;
	HBRUSH	areaBrushWhite, areaBrushBlack, areaBrushSelected;

	// Is symbol overwrite on
	bool insertionMode = false;
}

class HistoryManager;

struct TextareaData
{
	HWND	areaWnd;

	// Buffer for linear text
	char	*areaText;
	// Buffer for linear text style information
	char	*areaTextEx;

	// Reserved size of linear buffer
	unsigned int areaTextSize;
	// Current size of linear buffer
	unsigned int overallLength;

	int		areaWidth, areaHeight;
	// Cursor position in text, in symbols
	unsigned int cursorCharX, cursorCharY;
	// Horizontal and vertical scroll positions, in symbols
	int shiftCharX, shiftCharY;

	unsigned int lineCount;
	// Longest line in characters for horizontal scrolling
	int longestLine;

	// Flag shows, if colorer should be issued to update text
	bool needUpdate;

	AreaLine	*firstLine;
	// Current line is the line, where cursor is placed
	AreaLine	*currLine;

	// For text selection, here are starting and ending positions (in symbols)
	unsigned int dragStartX, dragStartY;
	unsigned int dragEndX, dragEndY;
	// And a flag that shows if the selection is active
	bool selectionOn;

	HistoryManager	*history;
};

// Class that tracks text change for Ctrl+Z\Ctrl+Y
class HistoryManager
{
public:
	HistoryManager(TextareaData *areaData)
	{
		firstShot = lastShot = NULL;
		data = areaData;
	}
	~HistoryManager()
	{
	}

	enum ChangeFlag
	{
		LINES_CHANGED = 1 << 0,
		LINES_ADDED = 1 << 1,
		LINES_DELETED = 1 << 2,
	};

	void	ResetHistory()
	{
		// While we have snapshots, delete them
		while(firstShot)
		{
			Snapshot *next = firstShot->nextShot;
			delete firstShot;
			firstShot = next;
		}
		firstShot = lastShot = NULL;
	}
	void	TakeSnapshot(AreaLine *start, ChangeFlag changeFlag, unsigned int linesAffected)
	{
		// Create linked list
		if(!firstShot)
		{
			firstShot = lastShot = new Snapshot;
			firstShot->prevShot = lastShot->nextShot = NULL;
		}else{	// Or add element to it
			lastShot->nextShot = new Snapshot;
			lastShot->nextShot->prevShot = lastShot;
			lastShot->nextShot->nextShot = NULL;
			lastShot = lastShot->nextShot;
		}
		// Type of edit and lines affected by it
		lastShot->type = changeFlag;
		lastShot->lines = linesAffected;
		// Save cursor position
		lastShot->cursorX = data->cursorCharX;
		lastShot->cursorY = data->cursorCharY;
		// Save selection state
		lastShot->selectStartX = data->dragStartX;
		lastShot->selectStartY = data->dragStartY;
		lastShot->selectEndX = data->dragEndX;
		lastShot->selectEndY = data->dragEndY;
		lastShot->selectionOn = data->selectionOn;

		lastShot->first = NULL;
		// Find the number of a line, where edit starts
		lastShot->startLine = 0;
		AreaLine *src = data->firstLine;
		while(src != start)
		{
			src = src->next;
			lastShot->startLine++;
		}
		// Copy affected lines to snapshot line list.
		// (unless the edit creates a number of new lines, in which case we have to save only one)
		AreaLine *curr = NULL;
		for(unsigned int i = 0; i < (changeFlag == LINES_ADDED ? 1 : linesAffected); i++)
		{
			// Copy line
			curr = InsertLineAfter(curr);
			ExtendLine(curr, src->length);
			memcpy(curr->data, src->data, src->length * sizeof(AreaChar));
			curr->length = src->length;
			
			// Set the first line, if not set
			if(!lastShot->first)
				lastShot->first = curr;
			// Move to the next
			src = src->next;
		}
	}

	void	Undo()
	{
		// If there are no snapshots, exit
		if(!lastShot)
			return;

		// Disable selection
		data->selectionOn = false;

		// Restore cursor position
		data->cursorCharX = lastShot->cursorX;
		data->cursorCharY = lastShot->cursorY;
		// Restore selection state
		data->dragStartX = lastShot->selectStartX;
		data->dragStartY = lastShot->selectStartY;
		data->dragEndX = lastShot->selectEndX;
		data->dragEndY = lastShot->selectEndY;
		data->selectionOn = lastShot->selectionOn;

		// Set currLine to the old one
		data->currLine = data->firstLine;
		for(unsigned int i = 0; i < lastShot->startLine; i++)
			data->currLine = data->currLine->next;
		// Copy previous contents of current line
		ExtendLine(data->currLine, lastShot->first->length);
		memcpy(data->currLine->data, lastShot->first->data, lastShot->first->length * sizeof(AreaChar));
		data->currLine->length = lastShot->first->length;
		// If lines were added by edit, remove them
		if(lastShot->type == LINES_ADDED)
		{
			data->lineCount -= lastShot->lines;
			for(unsigned int i = 0; i < lastShot->lines; i++)
				DeleteLine(data->currLine->next);
		}else if(lastShot->type == LINES_DELETED){	// If lines were deleted by edit, restore them
			AreaLine *curr = lastShot->first->next;
			data->lineCount += lastShot->lines - 1;
			for(unsigned int i = 0; i < lastShot->lines-1; i++)
			{
				data->currLine = InsertLineAfter(data->currLine);
				ExtendLine(data->currLine, curr->length);
				memcpy(data->currLine->data, curr->data, curr->length * sizeof(AreaChar));
				data->currLine->length = curr->length;
				curr = curr->next;
			}
		}

		// Remove snapshot
		if(lastShot->prevShot)
		{
			lastShot = lastShot->prevShot;
			delete lastShot->nextShot;
			lastShot->nextShot = NULL;
		}else{
			delete lastShot;
			firstShot = lastShot = NULL;
		}
		// Update window
		InvalidateRect(data->areaWnd, NULL, false);
	}

	struct Snapshot
	{
		~Snapshot()
		{
			AreaLine *line = first;
			// Delete all the lines in snapshot
			while(line)
			{
				AreaLine *next = line->next;
				DeleteLine(line);
				line = next;
			}
		}

		ChangeFlag		type;
		unsigned int	lines, startLine;
		AreaLine		*first;

		unsigned int	cursorX, cursorY;
		unsigned int	selectStartX, selectStartY, selectEndX, selectEndY;
		bool			selectionOn;

		Snapshot		*nextShot, *prevShot;
	};

	Snapshot	*firstShot, *lastShot;

	TextareaData	*data;
};

namespace RichTextarea
{
	TextareaData*	GetData(HWND wnd)
	{
		return (TextareaData*)(intptr_t)GetWindowLongPtr(wnd, 0);
	}

	void	CreateShared(HWND wnd)
	{
		if(sharedCreated)
			return;

		if(!AreaLine::pool)
			AreaLine::pool = new ObjectBlockPool<AreaLine, 32>();

		HDC hdc = BeginPaint(wnd, &areaPS);

		int found = 0;
		EnumFontFamilies(hdc, "Consolas", InitFont, (LPARAM)&found);
		const char *fontFam = "Courier New";
		bool italic = false;
		if(found)
		{
			fontFam = "Consolas";
			italic = true;
		}
			
		// Create font for every FONT_STYLE
		areaFont[FONT_REGULAR] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFam);
		areaFont[FONT_BOLD] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_BOLD, false, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFam);
		areaFont[FONT_ITALIC] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, italic, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFam);
		areaFont[FONT_UNDERLINED] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, true, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFam);

		// Create pens and brushes
		areaPenWhite1px = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
		areaPenBlack1px = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

		areaBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
		areaBrushBlack = CreateSolidBrush(RGB(0, 0, 0));
		areaBrushSelected = CreateSolidBrush(RGB(51, 153, 255));

		EndPaint(wnd, &areaPS);

		sharedCreated = true;
	}

	// Function is used to reserve space in linear text buffer
	void ExtendLinearTextBuffer(TextareaData *data);

	// Selection made by user can have ending point _before_ the starting point
	// This function is called to sort them out, but it doesn't change the global state (function is called to redraw selection, while user is still dragging his mouse)
	void SortSelPoints(TextareaData *data, unsigned int &startX, unsigned int &endX, unsigned int &startY, unsigned int &endY);

	// Update scrollbars, according to the scrolled text
	void	UpdateScrollBar(TextareaData *data);

	// Function puts longest line size to a global variable
	// Size is in characters, but the Tab can be represented with more that one
	void	FindLongestLine(TextareaData *data);
	
	void OnPaint(HWND wnd);

	// Windows scrollbar can scroll beyond valid positions
	void	ClampShift(TextareaData *data);

	// Check that the cursor is visible (called, when editing text)
	// If it's not, shift scroll positions, so it will be visible once again
	void	ScrollToCursor(TextareaData *data);

	// Convert cursor position to local pixel coordinates
	void	CursorToClient(TextareaData *data, unsigned int xCursor, unsigned int yCursor, int &xPos, int &yPos);

	// Convert local pixel coordinates to cursor position
	// Position X coordinate is clamped when clampX is set
	AreaLine* ClientToCursor(TextareaData *data, int xPos, int yPos, unsigned int &cursorX, unsigned int &cursorY, bool clampX);

	// Function checks if the cursor is placed on a valid position, and if not, moves it
	void	ClampCursorBounds(TextareaData *data);

	// Remove characters in active selection
	void DeleteSelection(TextareaData *data);

	void DeletePreviousChar(TextareaData *data);

	AreaLine* ExtendSelectionFromPoint(TextareaData *data, unsigned int xPos, unsigned int yPos);

	void InputChar(TextareaData *data, char ch);
	void InputEnter(TextareaData *data);

	void OnCopyOrCut(HWND wnd, bool cut);
	void OnPaste(HWND wnd);
	void OnCreate(HWND wnd);
	void OnDestroy(HWND wnd);
	void OnCharacter(HWND wnd, char ch);
	void OnKeyEvent(HWND wnd, int key);
}

// Set style parameters. bold\italics\underline flags don't work together
bool RichTextarea::SetTextStyle(unsigned int id, unsigned char red, unsigned char green, unsigned char blue, bool bold, bool italics, bool underline)
{
	// There are FONT_STYLE_COUNT styles available
	if(id >= FONT_STYLE_COUNT)
		return false;
	tStyle[id].color[0] = red;
	tStyle[id].color[1] = green;
	tStyle[id].color[2] = blue;
	tStyle[id].font = FONT_REGULAR;
	if(bold)
		tStyle[id].font = FONT_BOLD;
	if(italics)
		tStyle[id].font = FONT_ITALIC;
	if(underline)
		tStyle[id].font = FONT_UNDERLINED;
	return true;
}

// Function is used to reserve space in linear text buffer
void RichTextarea::ExtendLinearTextBuffer(TextareaData *data)
{
	// Size that is needed is calculated by summing lengths of all lines
	data->overallLength = 0;
	AreaLine *curr = data->firstLine;
	while(curr)
	{
		// AreaLine doesn't have the line endings (or any other special symbols)
		// But GetAreaText() returns text with \r\n at the end, so we have to add
		data->overallLength += curr->length + 2;	// additional 2 bytes for \r\n
		curr = curr->next;
	}
	if(data->overallLength >= data->areaTextSize)
	{
		// Reserve more than is needed
		data->areaTextSize = data->overallLength + data->overallLength / 2;
		delete[] data->areaText;
		delete[] data->areaTextEx;
		data->areaText = new char[data->areaTextSize];
		data->areaTextEx = new char[data->areaTextSize];
	}
}

// Last edited position
unsigned int maximumEnd;

// Function called before setting style to text parts. It reserves needed space in linear buffer
void RichTextarea::BeginStyleUpdate(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	ExtendLinearTextBuffer(data);
	maximumEnd = 0;
}

// Style is changed for linear buffer
void RichTextarea::SetStyleToSelection(HWND wnd, unsigned int start, unsigned int end, int style)
{
	TextareaData *data = GetData(wnd);

	// Simply set style to the specified range of symbols
	for(unsigned int i = start; i <= end; i++)
		data->areaTextEx[i] = (char)style;
	maximumEnd = maximumEnd < end ? end : maximumEnd;
}

// Function is called after setting style to text parts. It copies style information from linear buffer to AreaLine list
void RichTextarea::EndStyleUpdate(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	AreaLine *curr = data->firstLine;
	// i has the position inside linear buffer
	// n has the position inside a line
	// Update only to the last edited position
	for(unsigned int i = 0, n = 0; i < maximumEnd && curr;)
	{
		if(n < curr->length)	// Until n reaches line size, add style information to it
		{
			curr->data[n].style = data->areaTextEx[i];
			i++;
			n++;
		}else{	// After that,
			i += 2;		// skip the \r\n symbols, present after each line in linear buffer
			n = 0;		// Returning to the first symbol
			curr = curr->next;	// At the next line.
		}
	}
}

void RichTextarea::ClearAreaText(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	data->firstLine->length = 0;
	data->lineCount = 1;
	data->longestLine = 0;
	data->currLine = data->firstLine;
	while(data->currLine->next)
		DeleteLine(data->currLine->next);
	data->currLine = data->firstLine;
	data->firstLine->next = NULL;
}

// Function that returns text with line ending in linear buffer
const char* RichTextarea::GetAreaText(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	// Reserve size of all text
	ExtendLinearTextBuffer(data);

	char	*currChar = data->areaText;
	AreaLine *curr = data->firstLine;
	// For every line
	while(curr)
	{
		// Append line contents to the buffer
		for(unsigned int i = 0; i < curr->length; i++, currChar++)
			*currChar = curr->data[i].ch;
		// Move to the next line
		curr = curr->next;
		if(curr)
		{
			// Append line ending
			*currChar++ = '\r';
			*currChar++ = '\n';
		}
	}
	// Terminate buffer at the end
	*currChar = 0;
	return data->areaText;
};

// Function that sets the text in text area
void RichTextarea::SetAreaText(HWND wnd, const char *text)
{
	TextareaData *data = GetData(wnd);

	// Remove current text
	ClearAreaText(wnd);
	// Because we are not holding text as a simple string,
	// we simulate how this text is written symbol by symbol
	while(*text)
	{
		if(*text >= 0x20 || *text == '\t')
			InputChar(data, *text);
		else if(*text == '\r')
			InputEnter(data);
		*text++;
	}
	// Colorer should update text
	data->needUpdate = true;
	// Reset cursor and selection
	data->cursorCharX = data->dragStartX = data->dragEndX = 0;
	data->cursorCharY = data->dragStartY = data->dragEndY = 0;
	data->selectionOn = false;

	data->history->ResetHistory();
}

void RichTextarea::SetStatusBar(HWND status, unsigned int barWidth)
{
	const int SB_SETPARTS = (WM_USER+4);
	const int parts = 5;
	int	miniPart = 64;
	int widths[parts] = { barWidth - 4 * miniPart, barWidth - 3 * miniPart, barWidth - 2 * miniPart, barWidth - miniPart, -1 };
	areaStatus = status;
	SendMessage(areaStatus, SB_SETPARTS, parts, (LPARAM)widths);
}

// Force redraw
void RichTextarea::UpdateArea(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	InvalidateRect(data->areaWnd, NULL, false);
}

bool RichTextarea::NeedUpdate(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	bool ret = data->needUpdate;
	return ret;
}
void RichTextarea::ResetUpdate(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	data->needUpdate = false;
}

// Selection made by user can have ending point _before_ the starting point
// This function is called to sort them out, but it doesn't change the global state (function is called to redraw selection, while user is still dragging his mouse)
void RichTextarea::SortSelPoints(TextareaData *data, unsigned int &startX, unsigned int &endX, unsigned int &startY, unsigned int &endY)
{
	// At first, save points as they are.
	startX = data->dragStartX;
	endX = data->dragEndX;
	startY = data->dragStartY;
	endY = data->dragEndY;
	// If this is a single line selection and end is before start at X axis, swap them
	if(data->dragEndX < data->dragStartX && data->dragStartY == data->dragEndY)
	{
		startX = data->dragEndX;
		endX = data->dragStartX;
	}
	// If it a multiple line selection, and points are placed in a wrong order, swap X and Y components
	if(data->dragEndY < data->dragStartY)
	{
		startY = data->dragEndY;
		endY = data->dragStartY;

		startX = data->dragEndX;
		endX = data->dragStartX;
	}
}

// Update scrollbars, according to the scrolled text
void	RichTextarea::UpdateScrollBar(TextareaData *data)
{
	SCROLLINFO sbInfo;
	sbInfo.cbSize = sizeof(SCROLLINFO);
	sbInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	sbInfo.nMin = 0;
	sbInfo.nMax = data->lineCount;
	sbInfo.nPage = charHeight ? (data->areaHeight) / charHeight : 1;
	sbInfo.nPos = data->shiftCharY;
	SetScrollInfo(data->areaWnd, SB_VERT, &sbInfo, true);

	sbInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	sbInfo.nMin = 0;
	sbInfo.nMax = data->longestLine + 1;
	sbInfo.nPage = charWidth ? (data->areaWidth - charWidth) / charWidth : 1;
	sbInfo.nPos = data->shiftCharX;
	SetScrollInfo(data->areaWnd, SB_HORZ, &sbInfo, true);
}

// Function puts longest line size to a global variable
// Size is in characters, but the Tab can be represented with more that one
void	RichTextarea::FindLongestLine(TextareaData *data)
{
	AreaLine *curr = data->firstLine;
	data->longestLine = 0;
	while(curr)
	{
		int length = 0;
		for(unsigned int i = 0; i < curr->length; i++, length++)
			if(curr->data[i].ch == '\t')
				length += 3;
		data->longestLine = data->longestLine < length ? length : data->longestLine;
		curr = curr->next;
	}
}

void RichTextarea::OnPaint(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	// Windows will tell, what part we have to update
	RECT updateRect;
	if(!GetUpdateRect(wnd, &updateRect, false))
		return;

	AreaLine *curr = NULL;

	// Start drawing
	HDC hdc = BeginPaint(wnd, &areaPS);
	FontStyle currFont = FONT_REGULAR;
	SelectFont(hdc, areaFont[currFont]);

	// Find out the size of a single symbol
	RECT charRect = { 0, 0, 0, 0 };
	DrawText(hdc, "W", 1, &charRect, DT_CALCRECT);
	charWidth = charRect.right;
	charHeight = charRect.bottom;

	// Find the length of the longest line in text (should move to someplace that is more appropriate)
	FindLongestLine(data);

	// Reset horizontal scroll position, if longest line can fit to window
	if(data->longestLine < data->areaWidth / charWidth - 1)
		data->shiftCharX = 0;
	if(int(data->lineCount) < data->areaHeight / charHeight)
		data->shiftCharY = 0;
	UpdateScrollBar(data);

	// Find the first line for current vertical scroll position
	AreaLine *startLine = data->firstLine;
	for(int i = 0; i < data->shiftCharY; i++)
		startLine = startLine->next;

	// Setup the box of the first symbol
	charRect.left = padLeft - data->shiftCharX * charWidth;
	charRect.right = charRect.left + charWidth;

	// Find the selection range
	unsigned int startX, startY, endX, endY;
	SortSelPoints(data, startX, endX, startY, endY);

	curr = startLine;
	unsigned int currLine = data->shiftCharY;
	// While they are lines and they didn't go out of view
	while(curr && charRect.top < updateRect.bottom)
	{
		// If line box is inside update rect
		if(charRect.bottom > updateRect.top)
		{
			// Draw line symbols
			for(unsigned int i = 0, posInChars = 0; i < curr->length; i++)
			{
				int shift = GetCharShift(curr->data[i].ch, posInChars);
				if(charRect.right > -TAB_SIZE * charWidth)
				{
					TextStyle &style = tStyle[curr->data[i].style];
					// Find out, if the symbol is in the selected range
					bool selected = false;
					if(startY == endY)
						selected = i >= startX && i < endX && currLine == startY;
					else
						selected = (currLine == startY && i >= startX) || (currLine == endY && i < endX) || (currLine > startY && currLine < endY);
					// If character is in the selection range and selection is active, invert colors
					if(selected && data->selectionOn)
					{
						SetBkColor(hdc, RGB(51, 153, 255));
						SetTextColor(hdc, RGB(255, 255, 255));
					}else{
						SetBkColor(hdc, RGB(255, 255, 255));
						SetTextColor(hdc, RGB(style.color[0], style.color[1], style.color[2]));
					}
					// If symbol has different font, change it
					if(style.font != currFont)
					{
						currFont = (FontStyle)style.font;
						SelectFont(hdc, areaFont[currFont]);
					}
					// If this is a tab, draw rectangle of appropriate size
					if(curr->data[i].ch =='\t')
					{
						charRect.right = charRect.left + shift * charWidth;
						FillRect(hdc, &charRect, selected && data->selectionOn ? areaBrushSelected : areaBrushWhite);
					}else{	// Draw character
						ExtTextOut(hdc, charRect.left, charRect.top, ETO_CLIPPED, &charRect, &curr->data[i].ch, 1, NULL);
					}
				}
				posInChars += shift;

				// Shift the box to the next position
				charRect.left += shift * charWidth;
				charRect.right = charRect.left + charWidth;

				// Break if out of view
				if(charRect.left > data->areaWidth - int(charWidth))
					break;
			}
			// Fill the end of the line with white color
			charRect.right = data->areaWidth - charWidth;
			FillRect(hdc, &charRect, areaBrushWhite);
			charRect.left = 0;
			charRect.right = padLeft;
			FillRect(hdc, &charRect, areaBrushWhite);//*/curr == ::currLine ? areaBrushBlack : areaBrushWhite);
		}
		// Shift to the beginning of the next line
		charRect.left = padLeft - data->shiftCharX * charWidth;
		charRect.right = charRect.left + charWidth;
		charRect.top += charHeight;
		charRect.bottom += charHeight;
		curr = curr->next;
		currLine++;
	}
	// Fill the empty space after text with white color
	if(charRect.top < data->areaHeight)
	{
		charRect.left = padLeft;
		charRect.right = data->areaWidth;
		charRect.bottom = data->areaHeight;
		FillRect(hdc, &charRect, areaBrushWhite);
	}

	EndPaint(data->areaWnd, &areaPS);
}

// Global variable that holds how many ticks have passed after last I-bar cursor reset
// Reset is used so that cursor will always be visible after key or mouse events
int	ibarState = 0;
VOID CALLBACK RichTextarea::AreaCursorUpdate(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if(!hwnd || GetFocus() != hwnd)
		return;

	TextareaData *data = GetData(hwnd);

	(void)dwTime;
	(void)uMsg;
	// Ticks go more frequently than I-bar cursor switches its state
	HPEN currPen = ibarState % 16 < 8 ? areaPenBlack1px : areaPenWhite1px;

	// Must be used so that painting could be done
	InvalidateRect(hwnd, NULL, false);
	HDC hdc = BeginPaint(hwnd, &areaPS);

	// Find cursor position in characters
	int	posInChars = 0;
	for(unsigned int i = 0; i < data->cursorCharX; i++)
		posInChars += GetCharShift(data->currLine->data[i].ch, posInChars);
	// Calculate cursor position in pixels
	int	xPos = padLeft - data->shiftCharX * charWidth + posInChars * charWidth;
	int chWidth = GetCharShift(data->currLine->data[data->cursorCharX].ch, posInChars) * charWidth;

	// While selecting pen, we check if our window is active. Cursor shouldn't blink if focus is on some other window
	SelectPen(hdc, idEvent ? currPen : areaPenWhite1px);

	MoveToEx(hdc, xPos, (data->cursorCharY - data->shiftCharY) * charHeight, NULL);
	LineTo(hdc, xPos, (data->cursorCharY - data->shiftCharY) * charHeight + charHeight);

	if(insertionMode)
	{
		// Draw full box
		LineTo(hdc, xPos + chWidth, (data->cursorCharY - data->shiftCharY) * charHeight + charHeight);
		LineTo(hdc, xPos + chWidth, (data->cursorCharY - data->shiftCharY) * charHeight);
		LineTo(hdc, xPos, (data->cursorCharY - data->shiftCharY) * charHeight);
	}
	EndPaint(hwnd, &areaPS);

	if(areaStatus)
	{
		char buf[256];
		sprintf(buf, "Ln %d", data->cursorCharY + 1);
		SendMessage(areaStatus, (WM_USER+1), 1, (LPARAM)buf);
		sprintf(buf, "Col %d", posInChars + 1);
		SendMessage(areaStatus, (WM_USER+1), 2, (LPARAM)buf);
		sprintf(buf, "Ch %d", data->cursorCharX + 1);
		SendMessage(areaStatus, (WM_USER+1), 3, (LPARAM)buf);
		sprintf(buf, "%s", insertionMode ? "OVR" : "INS");
		SendMessage(areaStatus, (WM_USER+1), 4, (LPARAM)buf);
	}

	ibarState++;
}

// Function for single char insertion at the cursor position
void RichTextarea::InputChar(TextareaData* data, char ch)
{
	// We need to reallocate line buffer if it is full
	ExtendLine(data->currLine, data->currLine->length + 1);
	// If cursor is in the middle of the line, we need to move characters after it to the right
	if(data->cursorCharX != data->currLine->length)
		memmove(&data->currLine->data[data->cursorCharX+1], &data->currLine->data[data->cursorCharX], (data->currLine->length - data->cursorCharX) * sizeof(AreaChar));
	data->currLine->data[data->cursorCharX].ch = ch;
	if(data->cursorCharX != 0)
		data->currLine->data[data->cursorCharX].style = data->currLine->data[data->cursorCharX-1].style;
	data->currLine->length++;
	// Move cursor forward
	data->cursorCharX++;
	ScrollToCursor(data);
	// Force redraw on the modified line
	RECT invalid = { 0, (data->cursorCharY - data->shiftCharY) * charHeight, data->areaWidth, (data->cursorCharY - data->shiftCharY + 1) * charHeight };
	InvalidateRect(data->areaWnd, &invalid, false);
}

// Function that adds line break at the cursor position
void RichTextarea::InputEnter(TextareaData* data)
{
	// Increment line count
	data->lineCount++;
	
	// Insert new line after current and switch to it
	data->currLine = InsertLineAfter(data->currLine);

	data->currLine->data[0].style = data->currLine->prev->length ? data->currLine->prev->data[data->currLine->prev->length-1].style : 0;

	// If line break is in the middle of the line
	if(data->cursorCharX != data->currLine->prev->length)
	{
		// Find, how much symbols will move to the next line
		unsigned int diff = data->currLine->prev->length - data->cursorCharX;
		// If it is more than the line can handle, extend character buffer
		ExtendLine(data->currLine, diff);
		// Copy symbols to the new line
		memcpy(data->currLine->data, &data->currLine->prev->data[data->cursorCharX], diff * sizeof(AreaChar));
		// Shrink old
		data->currLine->prev->length = data->cursorCharX;
		// Extend new
		data->currLine->length = diff;
	}
	// Move cursor to the next line
	data->cursorCharY++;
	data->cursorCharX = 0;

	// Force redraw on the modified line and all that goes after it
	RECT invalid = { 0, (data->cursorCharY - data->shiftCharY - 1) * charHeight, data->areaWidth, data->areaHeight };
	InvalidateRect(data->areaWnd, &invalid, false);
}

// Windows scrollbar can scroll beyond valid positions
void	RichTextarea::ClampShift(TextareaData *data)
{
	if(data->shiftCharY < 0)
		data->shiftCharY = 0;
	if(data->shiftCharY >= int(data->lineCount) - 1)
		data->shiftCharY = data->lineCount - 1;
	if(data->shiftCharX < 0)
		data->shiftCharX = 0;
	if(data->shiftCharX > data->longestLine)
		data->shiftCharX = data->longestLine;
}

// Check that the cursor is visible (called, when editing text)
// If it's not, shift scroll positions, so it will be visible once again
void	RichTextarea::ScrollToCursor(TextareaData *data)
{
	bool updated = false;
	if(charHeight && int(data->cursorCharY) > (data->areaHeight-32) / charHeight + data->shiftCharY)
	{
		data->shiftCharY = data->cursorCharY - (data->areaHeight-32) / charHeight;
		updated = true;
	}
	if(int(data->cursorCharY) < data->shiftCharY)
	{
		data->shiftCharY = data->cursorCharY - 1;
		updated = true;
	}
	if(charWidth && int(data->cursorCharX) > (data->areaWidth-32) / charWidth + data->shiftCharX)
	{
		data->shiftCharX = data->cursorCharX - (data->areaWidth-32) / charWidth;
		updated = true;
	}
	if(int(data->cursorCharX) < data->shiftCharX)
	{
		data->shiftCharX = data->cursorCharX - 1;
		updated = true;
	}
	if(!updated)
		return;
	ClampShift(data);
	InvalidateRect(data->areaWnd, NULL, false);
	UpdateScrollBar(data);
}

// Convert cursor position to local pixel coordinates
void	RichTextarea::CursorToClient(TextareaData *data, unsigned int xCursor, unsigned int yCursor, int &xPos, int &yPos)
{
	// Find the line where the cursor is placed
	AreaLine *curr = data->firstLine;
	for(unsigned int i = 0; i < yCursor; i++)
		curr = curr->next;

	int posInChars = 0;
	// Find cursor position in characters
	for(unsigned int i = 0; i < xCursor; i++)
		posInChars += GetCharShift(curr->data[i].ch, posInChars);

	// Start with padding, add position in pixels and subtract horizontal scroll value
	xPos = padLeft + posInChars * charWidth - data->shiftCharX * charWidth;
	// Find Y local coordinate
	yPos = (yCursor - data->shiftCharY) * charHeight + charHeight / 2;
}

// Convert local pixel coordinates to cursor position
// Position X coordinate is clamped when clampX is set
AreaLine* RichTextarea::ClientToCursor(TextareaData *data, int xPos, int yPos, unsigned int &cursorX, unsigned int &cursorY, bool clampX)
{
	if(yPos > 32768)
	{
		if(data->shiftCharY > 0)
			data->shiftCharY--;
		yPos = 0;
	}
	if(yPos > (data->areaHeight + charHeight) && data->shiftCharY < int(data->lineCount) - (data->areaHeight / charHeight) - 1)
		data->shiftCharY++;
	if(xPos > 32768)
		xPos = 0;
	// Find vertical cursor position
	cursorY = yPos / charHeight + data->shiftCharY - (yPos < 0 ? 1 : 0);

	// Clamp vertical position
	if(cursorY >= data->lineCount)
		cursorY = data->lineCount - 1;
	// Change current line
	AreaLine *curr = data->firstLine;
	for(unsigned int i = 0; i < cursorY; i++)
		curr = curr->next;

	// Convert local X coordinate to virtual X coordinate (no padding and scroll shifts)
	int vMouseX = xPos - padLeft + data->shiftCharX * charWidth;
	// Starting X position in pixels
	int vCurrX = 0;

	// Starting cursor position
	cursorX = 0;
	// Starting X position in characters
	int posInChars = 0;
	// Until we reach virtual X coordinate or line ends
	while(cursorX < curr->length)
	{
		// Find the length of a symbol
		int shift = GetCharShift(curr->data[cursorX].ch, posInChars);
		// Exit if current position with half of a character if bigger than mouse position
		if(vCurrX + shift * charWidth / 2 > vMouseX)
			break;
		// Advance cursor, position in pixels and position in characters
		cursorX++;
		vCurrX += shift * charWidth;
		posInChars += shift;
	}
	if(clampX)
	{
		// Clamp horizontal position
		if(cursorX > curr->length)
			cursorX = curr->length;
	}
	// Return pointer to a line
	return curr;
}

// Function checks if the cursor is placed on a valid position, and if not, moves it
void	RichTextarea::ClampCursorBounds(TextareaData *data)
{
	if(data->cursorCharY >= data->lineCount)
		data->cursorCharY = data->lineCount - 1;
	// Find selected line
	data->currLine = data->firstLine;
	for(unsigned int i = 0; i < data->cursorCharY; i++)
		data->currLine = data->currLine->next;
	// To clamp horizontal position
	if(data->cursorCharX > data->currLine->length)
		data->cursorCharX = data->currLine->length;
}

// Remove characters in active selection
void RichTextarea::DeleteSelection(TextareaData *data)
{
	// Selection must be active
	if(!data->selectionOn)
		return;

	// Sort selection points
	unsigned int startX, startY, endX, endY;
	SortSelPoints(data, startX, endX, startY, endY);

	// If both points are outside the text, exit
	if(startY > data->lineCount && endY > data->lineCount)
	{
		data->selectionOn = false;
		return;
	}
	// Clamp selection points in Y axis
	startY = startY > data->lineCount ? data->lineCount - 1 : startY;
	endY = endY > data->lineCount ? data->lineCount-1 : endY;

	// Find first selection line
	AreaLine *first = data->firstLine;
	for(unsigned int i = 0; i < startY; i++)
		first = first->next;

	data->history->TakeSnapshot(first, HistoryManager::LINES_DELETED, (endY - startY)+1);

	// For single-line selection
	if(startY == endY)
	{
		// Clamp selection points in Z axis
		endX = endX > first->length ? first->length : endX;
		startX = startX > first->length ? first->length : startX;
		// Move text after selection to the start of selection
		memmove(&first->data[startX], &first->data[endX], (first->length - endX) * sizeof(AreaChar));
		// Shrink length
		first->length -= endX - startX;
		// Move cursor and disable selection mode
		data->cursorCharX = startX;
		data->selectionOn = false;
	}else{	// For multi-line selection
		// Find last selection line
		AreaLine *last = first;
		for(unsigned int i = startY; i < endY; i++)
			last = last->next;

		// Shrink first line
		first->length = startX > first->length ? first->length : startX;

		// Move cursor to starting position
		data->cursorCharX = first->length;
		data->cursorCharY = startY;

		// Clamp ending X position
		endX = endX > last->length ? last->length : endX;

		// Move unselected part of last line to the beginning if the line
		memmove(&last->data[0], &last->data[endX], (last->length - endX) * sizeof(AreaChar));
		// Shrink last line length
		last->length -= endX;
		if(last->length < 0)
			last->length = 0;

		// Append last line contents to the first line
		unsigned int sum = first->length + last->length;
		// Check if there is enough room
		ExtendLine(first, sum);
		// Copy
		memcpy(&first->data[first->length], last->data, last->length * sizeof(AreaChar));
		// Extend length
		first->length = sum;

		// Remove lines after the first line, including the last
		for(unsigned int i = startY; i < endY; i++)
		{
			DeleteLine(first->next);
			data->lineCount--;
		}
		// Disable selection mode
		data->selectionOn = false;
		// Change current line
		data->currLine = first;
		if(data->lineCount == 0)
			data->lineCount = 1;
	}
	UpdateScrollBar(data);
	ClampShift(data);
	InvalidateRect(data->areaWnd, NULL, false);
}

void RichTextarea::DeletePreviousChar(TextareaData *data)
{
	// If cursor is not at the beginning of a line
	if(data->cursorCharX)
	{
		data->history->TakeSnapshot(data->currLine, HistoryManager::LINES_CHANGED, 1);
		// If cursor is in the middle, move characters to the vacant position
		if(data->cursorCharX != data->currLine->length)
			memmove(&data->currLine->data[data->cursorCharX-1], &data->currLine->data[data->cursorCharX], (data->currLine->length - data->cursorCharX) * sizeof(AreaChar));
		// Shrink line size and move cursor
		data->currLine->length--;
		data->cursorCharX--;
		// Force redraw on the updated region
		RECT invalid = { 0, (data->cursorCharY - data->shiftCharY) * charHeight, data->areaWidth, (data->cursorCharY - data->shiftCharY + 1) * charHeight };
		InvalidateRect(data->areaWnd, &invalid, false);
	}else if(data->currLine->prev){	// If it's at the beginning of a line and there is a line before current
		// Add current line to the previous, as if are removing the line break
		data->currLine = data->currLine->prev;

		data->history->TakeSnapshot(data->currLine, HistoryManager::LINES_DELETED, 2);
		// Check if there is enough space
		unsigned int sum = data->currLine->length + data->currLine->next->length;
		ExtendLine(data->currLine, sum);
		// Append one line to the other
		memcpy(&data->currLine->data[data->currLine->length], data->currLine->next->data, data->currLine->next->length * sizeof(AreaChar));

		// Update cursor position
		data->cursorCharX = data->currLine->length;
		data->cursorCharY--;

		// Update line length
		data->currLine->length = sum;

		// Remove line that was current before event
		DeleteLine(data->currLine->next);
		data->lineCount--;

		// Force redraw on the updated region
		RECT invalid = { 0, (data->cursorCharY - data->shiftCharY - 1) * charHeight, data->areaWidth, data->areaHeight };
		InvalidateRect(data->areaWnd, &invalid, false);
	}
}

void RichTextarea::OnCopyOrCut(HWND wnd, bool cut)
{
	TextareaData *data = GetData(wnd);

	// Get linear text
	const char *start = GetAreaText(wnd);
	AreaLine *curr = data->firstLine;

	// If there is no selection, remember this fact, and select current line
	bool genuineSelection = data->selectionOn;
	if(!data->selectionOn)
	{
		data->dragStartX = 0;
		data->dragEndX = data->currLine->length;
		data->dragStartY = data->cursorCharY;
		data->dragEndY = data->cursorCharY;
		data->selectionOn = true;
	}
	// Sort selection range
	unsigned int startX, startY, endX, endY;
	SortSelPoints(data, startX, endX, startY, endY);

	// Clamp selection points in Y axis
	startY = startY > data->lineCount ? data->lineCount - 1 : startY;
	endY = endY > data->lineCount ? data->lineCount-1 : endY;

	// Find first line start in linear buffer
	for(unsigned int i = 0; i < startY; i++)
	{
		start += curr->length + 2;
		curr = curr->next;
	}
	const char *end = start;
	// Move start to the first selected symbol
	start += (startX < curr->length ? startX : curr->length);
	// Find last line start in linear buffer
	for(unsigned int i = startY; i < endY; i++)
	{
		end += curr->length + 2;
		curr = curr->next;
	}
	// Move end to the last selected symbol
	end += (endX < curr->length ? endX : curr->length);

	// Copy to clipboard
	OpenClipboard(wnd);
	EmptyClipboard();
	HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, end - start + 1);
	char *pchData = (char*)GlobalLock(hClipboardData);
	memcpy(pchData, start, end - start);
	GlobalUnlock(hClipboardData);
	SetClipboardData(CF_TEXT, hClipboardData);
	CloseClipboard();
	// If it is a cut operation, remove selection
	if(cut)
		DeleteSelection(data);
	// If we had to make an artificial selection, disable it
	if(!genuineSelection)
		data->selectionOn = false;
}

void RichTextarea::OnPaste(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	// Remove selection
	if(data->selectionOn)
		DeleteSelection(data);
	
	// Get pasted text
	OpenClipboard(wnd);
	HANDLE clipData = GetClipboardData(CF_TEXT);
	if(clipData)
	{
		// Find line count
		char *str = (char*)clipData;
		unsigned int linesAdded = 0;
		do
		{
			if(*str == '\r')
				linesAdded++;
		}while(*str++);
		data->history->TakeSnapshot(data->currLine, HistoryManager::LINES_ADDED, linesAdded);

		str = (char*)clipData;
		// Simulate as if the text was written
		while(*str)
		{
			if(*str >= 0x20 || *str == '\t')
				InputChar(data, *str);
			else if(*str == '\r')
				InputEnter(data);
			str++;
		}
	}
	CloseClipboard();
	// Force update on whole window
	InvalidateRect(wnd, NULL, false);
}

void RichTextarea::OnCreate(HWND wnd)
{
	SetWindowLongPtr(wnd, 0, (WND_PTR_TYPE)(intptr_t)new TextareaData);

	TextareaData	*data = GetData(wnd);
	memset(data, 0, sizeof(TextareaData));

	CreateShared(wnd);

	// Init linear text buffer
	data->areaTextSize = 16 * 1024;
	data->areaText = new char[data->areaTextSize];
	data->areaTextEx = new char[data->areaTextSize];

	// Create first line of text
	data->currLine = data->firstLine = InsertLineAfter(NULL);
	data->currLine->data[0].style = 0;

	data->lineCount = 1;

	data->history = new HistoryManager(data);

	data->areaWnd = wnd;

	static int timerID = 1;
	SetTimer(wnd, timerID++, 62, AreaCursorUpdate);
}

void RichTextarea::OnDestroy(HWND wnd)
{
	TextareaData *data = GetData(wnd);

	ClearAreaText(wnd);
	DeleteLine(data->firstLine);
	data->history->ResetHistory();

	delete[] data->areaText;
	delete[] data->areaTextEx;
	delete data->history;
	data->areaText = NULL;
	data->areaTextEx = NULL;
	data->history = NULL;

	delete data;
}

void RichTextarea::OnCharacter(HWND wnd, char ch)
{
	TextareaData *data = GetData(wnd);

	unsigned int startX, startY, endX, endY;
	if(ch >= 0x20 || ch == '\t')	// If it isn't special symbol or it is a Tab
	{
		// We add a history snapshot only if cursor position changed since last character input
		static unsigned int lastCursorX = ~0u, lastCursorY = ~0u;
		if(data->cursorCharX != lastCursorX || data->cursorCharY != lastCursorY)
		{
			data->history->TakeSnapshot(data->currLine, HistoryManager::LINES_CHANGED, 1);
			lastCursorX = data->cursorCharX;
			lastCursorY = data->cursorCharY;
		}
		// Compensate the caret movement
		lastCursorX++;
		// If insert mode, create selection
		if(insertionMode)
		{
			data->selectionOn = true;
			data->dragStartX = data->cursorCharX;
			data->dragEndY = data->dragStartY = data->cursorCharY;
			data->dragEndX = data->cursorCharX + 1;
			if(data->dragEndX > int(data->currLine->length))
				data->selectionOn = false;
		}
		// Remove selection
		if(data->selectionOn)
		{
			SortSelPoints(data, startX, endX, startY, endY);
			// If Tab key was pressed, and we have a multiple-line selection,
			// depending on the state of Shift key,
			// we either need to add Tab character at the beginning of each line
			// or we have to remove whitespace at the beginning of the each line
			if(ch == '\t' && startY != endY)
			{
				data->cursorCharY = startY;
				// Clamp cursor position and select first line
				ClampCursorBounds(data);
				// For every selected line
				for(unsigned int i = startY; i <= endY; i++)
				{
					// Inserting in front
					data->cursorCharX = 0;
					// If it's a Shift+Tab
					if(IsPressed(VK_SHIFT))
					{
						// We have to remove a number of whitespaces so the length of removed whitespaces will be equal to tab size
						int toRemove = 0;
						// So we select a maximum of TAB_SIZE spaces
						while(toRemove < min(TAB_SIZE, int(data->currLine->length)) && data->currLine->data[toRemove].ch == ' ')
							toRemove++;
						// And if we haven't reached our goal, and there is a Tab symbol
						if(toRemove < min(TAB_SIZE, int(data->currLine->length)) && data->currLine->data[toRemove].ch == '\t')
							toRemove++;	// Select it too
						// Remove characters
						memmove(&data->currLine->data[0], &data->currLine->data[toRemove], (data->currLine->length - toRemove) * sizeof(AreaChar));
						// Shrink line length
						data->currLine->length -= toRemove;
					}else{	// Simply Tab, insert symbol
						InputChar(data, ch);
					}
					data->currLine = data->currLine->next;
					data->cursorCharY++;
				}
				// Restore cursor position
				data->cursorCharX = endX;
				data->cursorCharY--;
				// Clamp cursor position and update current line
				ClampCursorBounds(data);
				InvalidateRect(data->areaWnd, NULL, false);
				return;
			}else if(ch == '\t' && IsPressed(VK_SHIFT)){
				data->cursorCharX = startX;
				data->cursorCharY = startY;
				// Clamp cursor position and select first line
				ClampCursorBounds(data);
				if(startX && isspace(data->currLine->data[startX-1].ch))
				{
					DeletePreviousChar(data);
					if(startY == endY)
					{
						data->cursorCharX = endX - 1;
						if(data->dragStartX == startX)
							data->dragStartX = --startX;
						else
							data->dragEndX = --startX;
					}
				}
				return;
			}else{
				// Otherwise, just remove selection
				DeleteSelection(data);
			}
		}else if(ch == '\t' && IsPressed(VK_SHIFT)){
			if(data->cursorCharX && isspace(data->currLine->data[data->cursorCharX-1].ch))
				DeletePreviousChar(data);
			return;
		}
		// Insert symbol
		InputChar(data, ch);
	}else if(ch == '\r'){	// Line break
		data->history->TakeSnapshot(data->currLine, HistoryManager::LINES_ADDED, 1);
		// Remove selection
		if(data->selectionOn)
			DeleteSelection(data);
		// Find current indentation
		int characterIdent = 0;
		int effectiveIdent = 0;
		while(characterIdent < int(data->currLine->length) && isspace(data->currLine->data[characterIdent].ch))
		{
			effectiveIdent += GetCharShift(data->currLine->data[characterIdent].ch, effectiveIdent);
			characterIdent++;
		}
		// Insert line break
		InputEnter(data);
		// Add indentation
		while(effectiveIdent > 0)
		{
			InputChar(data, '\t');
			effectiveIdent -= TAB_SIZE;
		}
	}else if(ch == '\b'){	// Backspace
		// Remove selection
		if(data->selectionOn)
		{
			DeleteSelection(data);
		}else{
			DeletePreviousChar(data);
		}
		ScrollToCursor(data);
	}else if(ch == 22){	// Ctrl+V
		OnPaste(wnd);
	}else if(ch == 1){	// Ctrl+A
		// Select all
		data->selectionOn = true;
		data->dragStartX = 0;
		data->dragStartY = 0;
		// Find last line to know end cursor position
		AreaLine *curr = data->firstLine;
		int line = 0;
		while(curr->next)
			curr = curr->next, line++;
		data->dragEndX = curr->length;
		data->dragEndY = line;
		// Force update on whole window
		InvalidateRect(wnd, NULL, false);
	}else if(ch == 3 || ch == 24){	// Ctrl+C and Ctrl+X
		OnCopyOrCut(wnd, ch == 24);
	}else if(ch == 26){	// Ctrl+Z
		data->history->Undo();
	}
	ScrollToCursor(data);
	data->needUpdate = true;
}

void RichTextarea::OnKeyEvent(HWND wnd, int key)
{
	TextareaData *data = GetData(wnd);

	unsigned int startX, startY, endX, endY;

	// If key is pressed, remove I-bar
	AreaCursorUpdate(wnd, 0, NULL, 0);
	// Reset I-bar tick count, so it will be visible
	ibarState = 0;
	// Start selection if Shift+Arrows are in use, and selection is disabled
	if(IsPressed(VK_SHIFT) && !data->selectionOn && (key == VK_DOWN || key == VK_UP || key == VK_LEFT || key == VK_RIGHT ||
												key == VK_PRIOR || key == VK_NEXT || key == VK_HOME || key == VK_END))
	{
		data->dragStartX = data->cursorCharX;
		data->dragStartY = data->cursorCharY;
	}
	// First four to move cursor
	if(key == VK_DOWN)
	{
		// If Ctrl is pressed, scroll vertically
		if(IsPressed(VK_CONTROL))
		{
			data->shiftCharY++;
			ClampShift(data);
		}
		// If Ctrl is not pressed or if it is and cursor is out of sight
		if(!IsPressed(VK_CONTROL) || (IsPressed(VK_CONTROL) && int(data->cursorCharY) < data->shiftCharY))
		{
			// If there is a next line, move to it
			if(data->currLine->next)
			{
				int oldPosX, oldPosY;
				CursorToClient(data, data->cursorCharX, data->cursorCharY, oldPosX, oldPosY);
				data->currLine = data->currLine->next;
				ClientToCursor(data, oldPosX, oldPosY + charHeight, data->cursorCharX, data->cursorCharY, true);
			}
		}
	}else if(key == VK_UP){
		// If Ctrl is pressed, scroll vertically
		if(IsPressed(VK_CONTROL))
		{
			data->shiftCharY--;
			ClampShift(data);
		}
		// If Ctrl is not pressed or if it is and cursor is out of sight
		if(!IsPressed(VK_CONTROL) || (IsPressed(VK_CONTROL) && int(data->cursorCharY) > (data->shiftCharY + data->areaHeight / charHeight - 1)))
		{
			// If there is a previous line, move to it
			if(data->currLine->prev)
			{
				int oldPosX, oldPosY;
				CursorToClient(data, data->cursorCharX, data->cursorCharY, oldPosX, oldPosY);
				data->currLine = data->currLine->prev;
				ClientToCursor(data, oldPosX, oldPosY - charHeight, data->cursorCharX, data->cursorCharY, true);
			}
		}
	}else if(key == VK_LEFT){
		// If Shift is not pressed and there is an active selection
		if(!IsPressed(VK_SHIFT) && data->selectionOn)
		{
			// Sort selection range
			SortSelPoints(data, startX, endX, startY, endY);
			// Set cursor position to the start of selection
			data->cursorCharX = startX;
			data->cursorCharY = startY;
			ClampCursorBounds(data);
		}else{
			// If the cursor is not at the beginning of the line
			if(data->cursorCharX > 0)
			{
				if(IsPressed(VK_CONTROL))
				{
					// Skip spaces
					while(data->cursorCharX > 1 && isspace(data->currLine->data[data->cursorCharX - 1].ch))
						data->cursorCharX--;
					data->cursorCharX = AdvanceCursor(data->currLine, data->cursorCharX, true);
				}else{
					data->cursorCharX--;
				}
			}else{
				// Otherwise, move to the end of the previous line
				if(data->currLine->prev)
				{
					data->currLine = data->currLine->prev;
					data->cursorCharX = data->currLine->length;
					data->cursorCharY--;
				}
			}
		}
	}else if(key == VK_RIGHT){
		// If Shift is not pressed and there is an active selection
		if(!IsPressed(VK_SHIFT) && data->selectionOn)
		{
			// Sort selection range
			SortSelPoints(data, startX, endX, startY, endY);
			// Set cursor position to the end of selection
			data->cursorCharX = endX;
			data->cursorCharY = endY;
			ClampCursorBounds(data);
		}else{
			// If the cursor is not at the end of the line
			if(data->cursorCharX < data->currLine->length)
			{
				if(IsPressed(VK_CONTROL))
				{
					data->cursorCharX = AdvanceCursor(data->currLine, data->cursorCharX, false);
					// Skip spaces
					while(int(data->cursorCharX) < int(data->currLine->length)-1 && isspace(data->currLine->data[data->cursorCharX + 1].ch))
						data->cursorCharX++;
					data->cursorCharX++;
				}else{
					data->cursorCharX++;
				}
			}else{
				// Otherwise, move to the start of the next line
				if(data->currLine->next)
				{
					data->currLine = data->currLine->next;
					data->cursorCharY++;
					data->cursorCharX = 0;
				}
			}
		}
	}else if(key == VK_DELETE){	// Delete
		// Shift+Delete is a cut operation
		if(IsPressed(VK_SHIFT))
		{
			OnCopyOrCut(wnd, true);
			return;
		}
		// Remove selection, if active
		if(data->selectionOn)
		{
			DeleteSelection(data);
			return;
		}else{
			// Move to the next character
			if(data->cursorCharX < data->currLine->length)
			{
				data->cursorCharX++;
			}else if(data->currLine->next){
				data->currLine = data->currLine->next;
				data->cursorCharY++;
				data->cursorCharX = 0;
			}else{
				return;
			}
			DeletePreviousChar(data);
		}
		ScrollToCursor(data);
	}else if(key == VK_PRIOR){	// Page up
		// If Ctrl is pressed
		if(IsPressed(VK_CONTROL))
		{
			// Move to the start of view
			data->cursorCharY = data->shiftCharY;
			ClampCursorBounds(data);
		}else{
			// Scroll view
			data->cursorCharY -= charHeight ? data->areaHeight / charHeight : 1;
			if(int(data->cursorCharY) < 0)
				data->cursorCharY = 0;
			ClampCursorBounds(data);
			ScrollToCursor(data);
		}
		ClampShift(data);
		UpdateScrollBar(data);
		InvalidateRect(data->areaWnd, NULL, false);
	}else if(key == VK_NEXT){	// Page down
		// If Ctrl is pressed
		if(IsPressed(VK_CONTROL))
		{
			// Move to the end of view
			data->cursorCharY = data->shiftCharY + (charHeight ? data->areaHeight / charHeight : 1);
			ClampCursorBounds(data);
		}else{
			// Scroll view
			data->cursorCharY += charHeight ? data->areaHeight / charHeight : 1;
			if(int(data->cursorCharY) < 0)
				data->cursorCharY = 0;
			ClampCursorBounds(data);
			ScrollToCursor(data);
		}
		ClampShift(data);
		UpdateScrollBar(data);
		InvalidateRect(data->areaWnd, NULL, false);
	}else if(key == VK_HOME){
		// If Ctrl is pressed
		if(IsPressed(VK_CONTROL))
		{
			// Move view and cursor to the beginning of the text
			data->shiftCharY = 0;
			data->cursorCharX = 0;
			data->cursorCharY = 0;
			ClampCursorBounds(data);
		}else{
			int identWidth = 0;
			while(identWidth < int(data->currLine->length) && (data->currLine->data[identWidth].ch == ' ' || data->currLine->data[identWidth].ch == '\t'))
				identWidth++;
			// If we are at the beginning of a line, move through all the spaces
			if(data->cursorCharX == 0 || int(data->cursorCharX) != identWidth)
				data->cursorCharX = identWidth;
			else	// Move cursor to the beginning of the line
				data->cursorCharX = 0;
		}
		ClampShift(data);
		UpdateScrollBar(data);
		InvalidateRect(data->areaWnd, NULL, false);
	}else if(key == VK_END){
		// If Ctrl is pressed
		if(IsPressed(VK_CONTROL))
		{
			// Move view and cursor to the end of the text
			data->shiftCharY = data->lineCount;
			data->cursorCharX = ~0u;
			data->cursorCharY = data->lineCount;
			ClampCursorBounds(data);
		}else{
			// Move cursor to the end of the line
			data->cursorCharX = data->currLine->length;
		}
		ClampShift(data);
		UpdateScrollBar(data);
		InvalidateRect(data->areaWnd, NULL, false);
	}else if(key == VK_INSERT){
		// Shift+Insert is a paste operation
		if(IsPressed(VK_SHIFT))
		{
			OnPaste(wnd);
		}else if(IsPressed(VK_CONTROL)){	// Ctrl+Insert is a copy operation
			OnCopyOrCut(wnd, false);
		}else{
			// Toggle input mode between insert\overwrite
			insertionMode = !insertionMode;
		}
	}else if(key == VK_ESCAPE){
		// Disable selection
		data->selectionOn = false;
		InvalidateRect(data->areaWnd, NULL, false);
	}
	if(key == VK_DOWN || key == VK_UP || key == VK_LEFT || key == VK_RIGHT ||
		key == VK_PRIOR || key == VK_NEXT || key == VK_HOME || key == VK_END)
	{
		// If Ctrl is not pressed, center view around cursor
		if(!IsPressed(VK_CONTROL))
			ScrollToCursor(data);
		// If Shift is pressed, set end of selection, redraw window and return
		if(IsPressed(VK_SHIFT))
		{
			data->dragEndX = data->cursorCharX;
			data->dragEndY = data->cursorCharY;
			if(data->dragStartX != data->dragEndX || data->dragStartY != data->dragEndY)
				data->selectionOn = true;
			InvalidateRect(data->areaWnd, NULL, false);
			return;
		}else{
			// Or disable selection, and if control is not pressed, update window and return
			data->selectionOn = false;
			if(!IsPressed(VK_CONTROL))
			{
				InvalidateRect(data->areaWnd, NULL, false);
				return;
			}
		}
		// Draw cursor
		AreaCursorUpdate(data->areaWnd, 0, NULL, 0);
		// Handle Ctrl+Arrows
		if(IsPressed(VK_CONTROL))
		{
			ClampShift(data);
			UpdateScrollBar(data);
			InvalidateRect(data->areaWnd, NULL, false);
		}
	}
}

AreaLine* RichTextarea::ExtendSelectionFromPoint(TextareaData *data, unsigned int xPos, unsigned int yPos)
{
	// Find cursor position
	AreaLine *curr = ClientToCursor(data, xPos, yPos, data->dragStartX, data->dragStartY, true);

	if(curr->length == 0)
		return curr;
	// Clamp horizontal position to line length
	if(data->dragStartX >= (int)curr->length && curr->length != 0)
		data->dragStartX = curr->length - 1;
	data->dragEndX = data->dragStartX;
	data->dragEndY = data->dragStartY;

	data->dragStartX = AdvanceCursor(curr, data->dragStartX, true);
	data->dragEndX = AdvanceCursor(curr, data->dragEndX, false) + 1;

	// Selection is active is the length of selected string is not 0
	data->selectionOn = curr->length != 0;

	return curr;
}
/*
void RichTextarea::SetActiveWindow(HWND wnd)
{
	activeWnd = wnd;
}*/

// This function register RichTextarea window class, so it can be created with CreateWindow call in client application
// Because there is global state for this control, only one instance can be created.
void RichTextarea::RegisterTextarea(const char *className, HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize			= sizeof(WNDCLASSEX); 
	wcex.style			= CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)TextareaProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(TextareaData*);
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_IBEAM);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= className;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);
}

void RichTextarea::UnregisterTextarea()
{
	delete AreaLine::pool;
	AreaLine::pool = NULL;
}

// Textarea message handler
LRESULT CALLBACK RichTextarea::TextareaProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	unsigned int startX, startY, endX, endY;
	static int lastX, lastY;

	TextareaData *data = GetData(hWnd);

	switch(message)
	{
	case WM_CREATE:
		OnCreate(hWnd);
		break;
	case WM_DESTROY:
		OnDestroy(hWnd);
		break;
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		OnPaint(hWnd);
		break;
	case WM_SIZE:
		data->areaWidth = LOWORD(lParam);
		data->areaHeight = HIWORD(lParam);

		if(areaStatus)
			SetStatusBar(areaStatus, data->areaWidth);

		UpdateScrollBar(data);

		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_MOUSEACTIVATE:
		SetFocus(hWnd);
		EnableWindow(hWnd, true);
		break;
	case WM_CHAR:
		OnCharacter(hWnd, (char)(wParam & 0xFF));
		break;
	case WM_KEYDOWN:
		if(wParam == VK_CONTROL || wParam == VK_SHIFT)
			break;
		OnKeyEvent(hWnd, (int)wParam);
		break;
	case WM_LBUTTONDBLCLK:
		data->currLine = ExtendSelectionFromPoint(data, LOWORD(lParam), HIWORD(lParam));
		if(data->selectionOn)
		{
			data->cursorCharX = data->dragEndX;
			data->cursorCharY = data->dragEndY;
			// Force line redraw
			RECT invalid = { 0, (data->dragStartY - data->shiftCharY) * charHeight, data->areaWidth, (data->dragEndY - data->shiftCharY + 1) * charHeight };
			InvalidateRect(hWnd, &invalid, false);
		}
		break;
	case WM_LBUTTONDOWN:
		lastX = LOWORD(lParam);
		lastY = HIWORD(lParam);
		// Capture mouse
		SetCapture(hWnd);
		// Remove cursor
		AreaCursorUpdate(hWnd, 0, NULL, 0);
		// Reset I-bar tick count
		ibarState = 0;
		// When left mouse button is pressed, disable selection mode and save position as selection start
		if(data->selectionOn && !IsPressed(VK_SHIFT))
		{
			data->selectionOn = false;
			// Sort selection range
			SortSelPoints(data, startX, endX, startY, endY);
			// Force selected part redraw
			RECT invalid = { 0, (startY - data->shiftCharY) * charHeight, data->areaWidth, (endY - data->shiftCharY + 1) * charHeight };
			InvalidateRect(hWnd, &invalid, false);
		}
		if(IsPressed(VK_SHIFT))
		{
			// If there is no selection, start it
			if(!data->selectionOn)
			{
				data->dragStartX = data->cursorCharX;
				data->dragStartY = data->cursorCharY;
			}
			// Update end of selection
			data->currLine = ClientToCursor(data, LOWORD(lParam), HIWORD(lParam), data->dragEndX, data->dragEndY, true);
			data->cursorCharX = data->dragEndX;
			data->cursorCharY = data->dragEndY;
			data->selectionOn = true;
			InvalidateRect(hWnd, NULL, false);
		}else if(IsPressed(VK_CONTROL)){
			data->currLine = ExtendSelectionFromPoint(data, LOWORD(lParam), HIWORD(lParam));
			data->cursorCharX = data->dragEndX;
			data->cursorCharY = data->dragEndY;
			InvalidateRect(hWnd, NULL, false);
		}else{
			// Set drag start and cursor position to where the user have clicked
			data->currLine = ClientToCursor(data, LOWORD(lParam), HIWORD(lParam), data->dragStartX, data->dragStartY, true);
			data->cursorCharX = data->dragStartX;
			data->cursorCharY = data->dragStartY;
		}
		break;
	case WM_MOUSEMOVE:
		// If mouse if moving with the left mouse down
		if(!(wParam & MK_LBUTTON) || (lastX == LOWORD(lParam) && lastY == HIWORD(lParam)))
			break;

		lastX = LOWORD(lParam);
		lastY = HIWORD(lParam);

		// Sort old selection range
		SortSelPoints(data, startX, endX, startY, endY);

		// Track the cursor position which is the selection end
		data->currLine = ClientToCursor(data, LOWORD(lParam), HIWORD(lParam), data->dragEndX, data->dragEndY, false);

		if(IsPressed(VK_CONTROL))
		{
			if(data->dragEndY > data->dragStartY || data->dragEndX > data->dragStartX)
				data->dragEndX = AdvanceCursor(data->currLine, data->dragEndX, false) + 1;
			else
				data->dragEndX = AdvanceCursor(data->currLine, data->dragEndX, true);
			data->dragEndX = data->dragEndX > data->currLine->length ? data->currLine->length : data->dragEndX;
			data->cursorCharX = data->dragEndX;
			data->cursorCharY = data->dragEndY;
		}else{
			// Find cursor position
			data->currLine = ClientToCursor(data, LOWORD(lParam), HIWORD(lParam), data->cursorCharX, data->cursorCharY, true);
		}

		// If current position differs from starting position, enable selection mode
		if(data->dragStartX != data->dragEndX || data->dragStartY != data->dragEndY)
			data->selectionOn = true;
		// Redraw selection
		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		break;
	case WM_MOUSEWHEEL:
		// Mouse wheel scroll text vertically
		data->shiftCharY -= (GET_WHEEL_DELTA_WPARAM(wParam) / 120) * 3;
		ClampShift(data);
		InvalidateRect(hWnd, NULL, false);

		UpdateScrollBar(data);
		break;
	case WM_VSCROLL:
		// Vertical scroll events
		switch(LOWORD(wParam))
		{
		case SB_LINEDOWN:
			data->shiftCharY++;
			break;
		case SB_LINEUP:
			data->shiftCharY--;
			break;
		case SB_PAGEDOWN:
			data->shiftCharY += charHeight ? data->areaHeight / charHeight : 1;
			break;
		case SB_PAGEUP:
			data->shiftCharY -= charHeight ? data->areaHeight / charHeight : 1;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			data->shiftCharY = HIWORD(wParam);
			break;
		}
		ClampShift(data);
		UpdateScrollBar(data);
		InvalidateRect(hWnd, NULL, false);
		break;
	case WM_HSCROLL:
		// Horizontal scroll events
		switch(LOWORD(wParam))
		{
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			data->shiftCharX++;
			break;
		case SB_LINEUP:
		case SB_PAGEUP:
			data->shiftCharX--;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			data->shiftCharX = HIWORD(wParam);
			break;
		}
		ClampShift(data);
		UpdateScrollBar(data);
		InvalidateRect(hWnd, NULL, false);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
