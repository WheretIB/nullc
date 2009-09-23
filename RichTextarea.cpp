#include "RichTextarea.h"

#include "ObjectPool.h"

#include <windowsx.h>

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
	void*		operator new(unsigned int size)
	{
		(void)size;
		return pool->Create();
	}
	void		operator delete(void *ptr, unsigned int size)
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

// Buffer for linear text
char	*areaText = NULL;
// Buffer for linear text style information
char	*areaTextEx = NULL;
// Reserved size of linear buffer
unsigned int areaTextSize = 0;
// Current size of linear buffer
unsigned int overallLength = 0;

bool	areaCreated = false;
HWND	areaWnd = 0;

HFONT	areaFont[4];

PAINTSTRUCT areaPS;
int		areaWidth, areaHeight;
// Single character width and height (using monospaced font)
int charWidth, charHeight;
// Cursor position in text, in symbols
unsigned int cursorCharX = 0, cursorCharY = 0;
// Horizontal and vertical scroll positions, in symbols
int shiftCharX = 0, shiftCharY = 0;
// Padding to the left of the first symbol
unsigned int padLeft = 5;

unsigned int lineCount = 0;
// Longest line in characters for horizontal scrolling
int longestLine = 0;

// Flag shows, if colorer should be issued to update text
bool needUpdate = false;

// A few pens and brushes for rendering
HPEN	areaPenWhite1px, areaPenBlack1px;
HBRUSH	areaBrushWhite, areaBrushBlack, areaBrushSelected;

AreaLine	*firstLine = NULL;
// Current line is the line, where cursor is placed
AreaLine	*currLine = NULL;

// For text selection, here are starting and ending positions (in symbols)
unsigned int dragStartX, dragStartY;
unsigned int dragEndX, dragEndY;
// And a flag that shows if the selection is active
bool selectionOn = false;

// Is symbol overwrite on
bool insertionMode = false;

// Class that tracks text change for Ctrl+Z\Ctrl+Y
class HistoryManager
{
public:
	HistoryManager()
	{
		firstShot = lastShot = NULL;
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
		lastShot->cursorX = cursorCharX;
		lastShot->cursorY = cursorCharY;
		// Save selection state
		lastShot->selectStartX = dragStartX;
		lastShot->selectStartY = dragStartY;
		lastShot->selectEndX = dragEndX;
		lastShot->selectEndY = dragEndY;
		lastShot->selectionOn = selectionOn;

		lastShot->first = NULL;
		// Find the number of a line, where edit starts
		lastShot->startLine = 0;
		AreaLine *src = firstLine;
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
		selectionOn = false;

		// Restore cursor position
		cursorCharX = lastShot->cursorX;
		cursorCharY = lastShot->cursorY;
		// Restore selection state
		dragStartX = lastShot->selectStartX;
		dragStartY = lastShot->selectStartY;
		dragEndX = lastShot->selectEndX;
		dragEndY = lastShot->selectEndY;
		selectionOn = lastShot->selectionOn;

		// Set currLine to the old one
		currLine = firstLine;
		for(unsigned int i = 0; i < lastShot->startLine; i++)
			currLine = currLine->next;
		// Copy previous contents of current line
		ExtendLine(currLine, lastShot->first->length);
		memcpy(currLine->data, lastShot->first->data, lastShot->first->length * sizeof(AreaChar));
		currLine->length = lastShot->first->length;
		// If lines were added by edit, remove them
		if(lastShot->type == LINES_ADDED)
		{
			lineCount -= lastShot->lines;
			for(unsigned int i = 0; i < lastShot->lines; i++)
				DeleteLine(currLine->next);
		}else if(lastShot->type == LINES_DELETED){	// If lines were deleted by edit, restore them
			AreaLine *curr = lastShot->first->next;
			lineCount += lastShot->lines - 1;
			for(unsigned int i = 0; i < lastShot->lines-1; i++)
			{
				currLine = InsertLineAfter(currLine);
				ExtendLine(currLine, curr->length);
				memcpy(currLine->data, curr->data, curr->length * sizeof(AreaChar));
				currLine->length = curr->length;
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
		InvalidateRect(areaWnd, NULL, false);
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
};
HistoryManager	*history = NULL;

// Set style parameters. bold\italics\underline flags don't work together
bool SetTextStyle(unsigned int id, unsigned char red, unsigned char green, unsigned char blue, bool bold, bool italics, bool underline)
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
void ExtendLinearTextBuffer()
{
	// Size that is needed is calculated by summing lengths of all lines
	overallLength = 0;
	AreaLine *curr = firstLine;
	while(curr)
	{
		// AreaLine doesn't have the line endings (or any other special symbols)
		// But GetAreaText() returns text with \r\n at the end, so we have to add
		overallLength += curr->length + 2;	// additional 2 bytes for \r\n
		curr = curr->next;
	}
	if(overallLength >= areaTextSize)
	{
		// Reserve more than is needed
		areaTextSize = overallLength + overallLength / 2;
		delete[] areaText;
		delete[] areaTextEx;
		areaText = new char[areaTextSize];
		areaTextEx = new char[areaTextSize];
	}
}

// Last edited position
unsigned int maximumEnd;

// Function called before setting style to text parts. It reserves needed space in linear buffer
void BeginStyleUpdate()
{
	ExtendLinearTextBuffer();
	maximumEnd = 0;
}

// Style is changed for linear buffer
void SetStyleToSelection(unsigned int start, unsigned int end, int style)
{
	// Simply set style to the specified range of symbols
	for(unsigned int i = start; i <= end; i++)
		areaTextEx[i] = (char)style;
	maximumEnd = maximumEnd < end ? end : maximumEnd;
}

// Function is called after setting style to text parts. It copies style information from linear buffer to AreaLine list
void EndStyleUpdate()
{
	AreaLine *curr = firstLine;
	// i has the position inside linear buffer
	// n has the position inside a line
	// Update only to the last edited position
	for(unsigned int i = 0, n = 0; i < maximumEnd && curr;)
	{
		if(n < curr->length)	// Until n reaches line size, add style information to it
		{
			curr->data[n].style = areaTextEx[i];
			i++;
			n++;
		}else{	// After that,
			i += 2;		// skip the \r\n symbols, present after each line in linear buffer
			n = 0;		// Returning to the first symbol
			curr = curr->next;	// At the next line.
		}
	}
}

// This function register RichTextarea window class, so it can be created with CreateWindow call in client application
// Because there is global state for this control, only one instance can be created.
void RegisterTextarea(const char *className, HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize			= sizeof(WNDCLASSEX); 
	wcex.style			= CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)TextareaProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_IBEAM);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= className;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);
}

void ClearAreaText()
{
	firstLine->length = 0;
	lineCount = 1;
	longestLine = 0;
	currLine = firstLine;
	while(currLine->next)
		DeleteLine(currLine->next);
	currLine = firstLine;
	firstLine->next = NULL;
}

// Function that returns text with line ending in linear buffer
const char* GetAreaText()
{
	// Reserve size of all text
	ExtendLinearTextBuffer();

	char	*currChar = areaText;
	AreaLine *curr = firstLine;
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
	return areaText;
};

// Function that sets the text in text area
void SetAreaText(const char *text)
{
	// Remove current text
	ClearAreaText();
	// Because we are not holding text as a simple string,
	// we simulate how this text is written symbol by symbol
	while(*text)
	{
		if(*text >= 0x20 || *text == '\t')
			InputChar(*text);
		else if(*text == '\r')
			InputEnter();
		*text++;
	}
	// Colorer should update text
	needUpdate = true;
	// Reset cursor and selection
	cursorCharX = dragStartX = dragEndX = 0;
	cursorCharY = dragStartY = dragEndY = 0;
	selectionOn = false;

	history->ResetHistory();
}

// Force redraw
void UpdateArea()
{
	InvalidateRect(areaWnd, NULL, false);
}

bool NeedUpdate()
{
	bool ret = needUpdate;
	return ret;
}
void ResetUpdate()
{
	needUpdate = false;
}

// Selection made by user can have ending point _before_ the starting point
// This function is called to sort them out, but it doesn't change the global state (function is called to redraw selection, while user is still dragging his mouse)
void SortSelPoints(unsigned int &startX, unsigned int &endX, unsigned int &startY, unsigned int &endY)
{
	// At first, save points as they are.
	startX = dragStartX;
	endX = dragEndX;
	startY = dragStartY;
	endY = dragEndY;
	// If this is a single line selection and end is before start at X axis, swap them
	if(dragEndX < dragStartX && dragStartY == dragEndY)
	{
		startX = dragEndX;
		endX = dragStartX;
	}
	// If it a multiple line selection, and points are placed in a wrong order, swap X and Y components
	if(dragEndY < dragStartY)
	{
		startY = dragEndY;
		endY = dragStartY;

		startX = dragEndX;
		endX = dragStartX;
	}
}

// Update scrollbars, according to the scrolled text
void	UpdateScrollBar()
{
	SCROLLINFO sbInfo;
	sbInfo.cbSize = sizeof(SCROLLINFO);
	sbInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	sbInfo.nMin = 0;
	sbInfo.nMax = lineCount;
	sbInfo.nPage = charHeight ? (areaHeight) / charHeight : 1;
	sbInfo.nPos = shiftCharY;
	SetScrollInfo(areaWnd, SB_VERT, &sbInfo, true);

	sbInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	sbInfo.nMin = 0;
	sbInfo.nMax = longestLine;
	sbInfo.nPage = charWidth ? (areaWidth - charWidth) / charWidth : 1;
	sbInfo.nPos = shiftCharX;
	SetScrollInfo(areaWnd, SB_HORZ, &sbInfo, true);
}

// Function puts longest line size to a global variable
// Size is in characters, but the Tab can be represented with more that one
void	FindLongestLine()
{
	AreaLine *curr = firstLine;
	longestLine = 0;
	while(curr)
	{
		int length = 0;
		for(unsigned int i = 0; i < curr->length; i++, length++)
			if(curr->data[i].ch == '\t')
				length += 3;
		longestLine = longestLine < length ? length : longestLine;
		curr = curr->next;
	}
}

// Get shift after symbol ch in characters from current position to the next
int GetCharShift(char ch, int currPos)
{
	return ch == '\t' ? TAB_SIZE - currPos % TAB_SIZE : 1;
}

void ReDraw()
{
	if(!areaCreated)
		return;

	// Windows will tell, what part we have to update
	RECT updateRect;
	if(!GetUpdateRect(areaWnd, &updateRect, false))
		return;

	AreaLine *curr = NULL;

	// Start drawing
	HDC hdc = BeginPaint(areaWnd, &areaPS);
	FontStyle currFont = FONT_REGULAR;
	SelectFont(hdc, areaFont[currFont]);

	// Find out the size of a single symbol
	RECT charRect = { 0, 0, 0, 0 };
	DrawText(hdc, "W", 1, &charRect, DT_CALCRECT);
	charWidth = charRect.right;
	charHeight = charRect.bottom;

	// Find the length of the longest line in text (should move to someplace that is more appropriate)
	FindLongestLine();

	// Reset horizontal scroll position, if longest line can fit to window
	if(longestLine < areaWidth / charWidth - 1)
		shiftCharX = 0;
	if(int(lineCount) < areaHeight / charHeight)
		shiftCharY = 0;
	UpdateScrollBar();

	// Find the first line for current vertical scroll position
	AreaLine *startLine = firstLine;
	for(int i = 0; i < shiftCharY; i++)
		startLine = startLine->next;

	// Setup the box of the first symbol
	charRect.left = padLeft - shiftCharX * charWidth;
	charRect.right = charRect.left + charWidth;

	// Find the selection range
	unsigned int startX, startY, endX, endY;
	SortSelPoints(startX, endX, startY, endY);

	curr = startLine;
	unsigned int currLine = shiftCharY;
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
				if(charRect.right < -TAB_SIZE * charWidth)
				{
					charRect.left += shift * charWidth;
					charRect.right += shift * charWidth;
				}else{
					TextStyle &style = tStyle[curr->data[i].style];
					// Find out, if the symbol is in the selected range
					bool selected = false;
					if(startY == endY)
						selected = i >= startX && i < endX && currLine == startY;
					else
						selected = (currLine == startY && i >= startX) || (currLine == endY && i < endX) || (currLine > startY && currLine < endY);
					// If character is in the selection range and selection is active, invert colors
					if(selected && selectionOn)
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
						FillRect(hdc, &charRect, selected && selectionOn ? areaBrushSelected : areaBrushWhite);
					}else{	// Draw character
						ExtTextOut(hdc, charRect.left, charRect.top, ETO_CLIPPED, &charRect, &curr->data[i].ch, 1, NULL);
					}
				}
				posInChars += shift;

				// Shift the box to the next position
				charRect.left += shift * charWidth;
				charRect.right = charRect.left + charWidth;

				// Break if out of view
				if(charRect.left > areaWidth - int(charWidth))
					break;
			}
			// Fill the end of the line with white color
			charRect.right = areaWidth - charWidth;
			FillRect(hdc, &charRect, areaBrushWhite);
			charRect.left = 0;
			charRect.right = padLeft;
			FillRect(hdc, &charRect, areaBrushWhite);//*/curr == ::currLine ? areaBrushBlack : areaBrushWhite);
		}
		// Shift to the beginning of the next line
		charRect.left = padLeft - shiftCharX * charWidth;
		charRect.right = charRect.left + charWidth;
		charRect.top += charHeight;
		charRect.bottom += charHeight;
		curr = curr->next;
		currLine++;
	}
	// Fill the empty space after text with white color
	if(charRect.top < areaHeight)
	{
		charRect.left = padLeft;
		charRect.right = areaWidth;
		charRect.bottom = areaHeight;
		FillRect(hdc, &charRect, areaBrushWhite);
	}

	EndPaint(areaWnd, &areaPS);
}

// Global variable that holds how many ticks have passed after last I-bar cursor reset
// Reset is used so that cursor will always be visible after key or mouse events
int	ibarState = 0;
VOID CALLBACK AreaCursorUpdate(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	(void)dwTime;
	(void)uMsg;
	// Check that it is our timer
	if(idEvent != 0)
		return;
	// Ticks go more frequently than I-bar cursor switches its state
	HPEN currPen = ibarState % 16 < 8 ? areaPenBlack1px : areaPenWhite1px;

	// Must be used so that painting could be done
	InvalidateRect(areaWnd, NULL, false);
	HDC hdc = BeginPaint(areaWnd, &areaPS);

	// Find cursor position in characters
	int	posInChars = 0;
	for(unsigned int i = 0; i < cursorCharX; i++)
		posInChars += GetCharShift(currLine->data[i].ch, posInChars);
	// Calculate cursor position in pixels
	int	xPos = padLeft - shiftCharX * charWidth + posInChars * charWidth;
	int chWidth = GetCharShift(currLine->data[cursorCharX].ch, posInChars) * charWidth;

	// While selecting pen, we check if our window is active. Cursor shouldn't blink if focus is on some other window
	SelectPen(hdc, GetFocus() == hwnd ? currPen : areaPenWhite1px);

	MoveToEx(hdc, xPos, (cursorCharY - shiftCharY) * charHeight, NULL);
	LineTo(hdc, xPos, (cursorCharY - shiftCharY) * charHeight + charHeight);

	if(insertionMode)
	{
		// Draw full box
		LineTo(hdc, xPos + chWidth, (cursorCharY - shiftCharY) * charHeight + charHeight);
		LineTo(hdc, xPos + chWidth, (cursorCharY - shiftCharY) * charHeight);
		LineTo(hdc, xPos, (cursorCharY - shiftCharY) * charHeight);
	}
	EndPaint(areaWnd, &areaPS);

	ibarState++;
}

// Function for single char insertion at the cursor position
void InputChar(char ch)
{
	// We need to reallocate line buffer if it is full
	ExtendLine(currLine, currLine->length + 1);
	// If cursor is in the middle of the line, we need to move characters after it to the right
	if(cursorCharX != currLine->length)
		memmove(&currLine->data[cursorCharX+1], &currLine->data[cursorCharX], (currLine->length - cursorCharX) * sizeof(AreaChar));
	currLine->data[cursorCharX].ch = ch;
	if(cursorCharX != 0)
		currLine->data[cursorCharX].style = currLine->data[cursorCharX-1].style;
	currLine->length++;
	// Move cursor forward
	cursorCharX++;
	// Force redraw on the modified line
	RECT invalid = { 0, (cursorCharY - shiftCharY) * charHeight, areaWidth, (cursorCharY - shiftCharY + 1) * charHeight };
	InvalidateRect(areaWnd, &invalid, false);
}

// Function that adds line break at the cursor position
void InputEnter()
{
	// Increment line count
	lineCount++;
	
	// Insert new line after current and switch to it
	currLine = InsertLineAfter(currLine);

	currLine->data[0].style = currLine->prev->length ? currLine->prev->data[currLine->prev->length-1].style : 0;

	// If line break is in the middle of the line
	if(cursorCharX != currLine->prev->length)
	{
		// Find, how much symbols will move to the next line
		unsigned int diff = currLine->prev->length - cursorCharX;
		// If it is more than the line can handle, extend character buffer
		ExtendLine(currLine, diff);
		// Copy symbols to the new line
		memcpy(currLine->data, &currLine->prev->data[cursorCharX], diff * sizeof(AreaChar));
		// Shrink old
		currLine->prev->length = cursorCharX;
		// Extend new
		currLine->length = diff;
	}
	// Move cursor to the next line
	cursorCharY++;
	cursorCharX = 0;

	// Force redraw on the modified line and all that goes after it
	RECT invalid = { 0, (cursorCharY - shiftCharY - 1) * charHeight, areaWidth, areaHeight };
	InvalidateRect(areaWnd, &invalid, false);
}

// Windows scrollbar can scroll beyond valid positions
void	ClampShift()
{
	if(shiftCharY < 0)
		shiftCharY = 0;
	if(shiftCharY >= int(lineCount) - 1)
		shiftCharY = lineCount - 1;
	if(shiftCharX < 0)
		shiftCharX = 0;
	if(shiftCharX > longestLine)
		shiftCharX = longestLine;
}

// Check that the cursor is visible (called, when editing text)
// If it's not, shift scroll positions, so it will be visible once again
void	ScrollToCursor()
{
	bool updated = false;
	if(charHeight && int(cursorCharY) > (areaHeight-32) / charHeight + shiftCharY)
	{
		shiftCharY = cursorCharY - (areaHeight-32) / charHeight;
		updated = true;
	}
	if(int(cursorCharY) < shiftCharY)
	{
		shiftCharY = cursorCharY - 1;
		updated = true;
	}
	if(charWidth && int(cursorCharX) > (areaWidth-32) / charWidth + shiftCharX)
	{
		shiftCharX = cursorCharX - (areaWidth-32) / charWidth;
		updated = true;
	}
	if(int(cursorCharX) < shiftCharX)
	{
		shiftCharX = cursorCharX - 1;
		updated = true;
	}
	if(!updated)
		return;
	ClampShift();
	InvalidateRect(areaWnd, NULL, false);
	UpdateScrollBar();
}

// Convert cursor position to local pixel coordinates
void	CursorToClient(unsigned int xCursor, unsigned int yCursor, int &xPos, int &yPos)
{
	// Find the line where the cursor is placed
	AreaLine *curr = firstLine;
	for(unsigned int i = 0; i < yCursor; i++)
		curr = curr->next;

	int posInChars = 0;
	// Find cursor position in characters
	for(unsigned int i = 0; i < xCursor; i++)
		posInChars += GetCharShift(curr->data[i].ch, posInChars);

	// Start with padding, add position in pixels and subtract horizontal scroll value
	xPos = padLeft + posInChars * charWidth - shiftCharX * charWidth;
	// Find Y local coordinate
	yPos = (yCursor - shiftCharY) * charHeight + charHeight / 2;
}

// Convert local pixel coordinates to cursor position
// Position X coordinate is clamped when clampX is set
AreaLine* ClientToCursor(int xPos, int yPos, unsigned int &cursorX, unsigned int &cursorY, bool clampX)
{
	if(yPos > 32768)
		yPos = 0;
	if(xPos > 32768)
		xPos = 0;
	// Find vertical cursor position
	cursorY = yPos / charHeight + shiftCharY - (yPos < 0 ? 1 : 0);

	// Clamp vertical position
	if(cursorY >= lineCount)
		cursorY = lineCount - 1;
	// Change current line
	AreaLine *curr = firstLine;
	for(unsigned int i = 0; i < cursorY; i++)
		curr = curr->next;

	// Convert local X coordinate to virtual X coordinate (no padding and scroll shifts)
	int vMouseX = xPos - padLeft + shiftCharX * charWidth;
	// Starting X  position in pixels
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
void	ClampCursorBounds()
{
	if(cursorCharY >= lineCount)
		cursorCharY = lineCount - 1;
	// Find selected line
	currLine = firstLine;
	for(unsigned int i = 0; i < cursorCharY; i++)
		currLine = currLine->next;
	// To clamp horizontal position
	if(cursorCharX > currLine->length)
		cursorCharX = currLine->length;
}

// Remove characters in active selection
void DeleteSelection()
{
	// Selection must be active
	if(!selectionOn)
		return;

	// Sort selection points
	unsigned int startX, startY, endX, endY;
	SortSelPoints(startX, endX, startY, endY);

	// If both points are outside the text, exit
	if(startY > lineCount && endY > lineCount)
	{
		selectionOn = false;
		return;
	}
	// Clamp selection points in Y axis
	startY = startY > lineCount ? lineCount - 1 : startY;
	endY = endY > lineCount ? lineCount-1 : endY;

	// Find first selection line
	AreaLine *first = firstLine;
	for(unsigned int i = 0; i < startY; i++)
		first = first->next;

	history->TakeSnapshot(first, HistoryManager::LINES_DELETED, (endY - startY)+1);

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
		cursorCharX = startX;
		selectionOn = false;
	}else{	// For multi-line selection
		// Find last selection line
		AreaLine *last = first;
		for(unsigned int i = startY; i < endY; i++)
			last = last->next;

		// Shrink first line
		first->length = startX > first->length ? first->length : startX;

		// Move cursor to starting position
		cursorCharX = first->length;
		cursorCharY = startY;

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
			lineCount--;
		}
		// Disable selection mode
		selectionOn = false;
		// Change current line
		currLine = first;
		if(lineCount == 0)
			lineCount = 1;
	}
	UpdateScrollBar();
	ClampShift();
	InvalidateRect(areaWnd, NULL, false);
}

bool IsPressed(int key)
{
	return !!(GetKeyState(key) & 0x80000000);
}


void DeletePreviousChar()
{
	// If cursor is not at the beginning of a line
	if(cursorCharX)
	{
		history->TakeSnapshot(currLine, HistoryManager::LINES_CHANGED, 1);
		// If cursor is in the middle, move characters to the vacant position
		if(cursorCharX != currLine->length)
			memmove(&currLine->data[cursorCharX-1], &currLine->data[cursorCharX], (currLine->length - cursorCharX) * sizeof(AreaChar));
		// Shrink line size and move cursor
		currLine->length--;
		cursorCharX--;
		// Force redraw on the updated region
		RECT invalid = { 0, (cursorCharY - shiftCharY) * charHeight, areaWidth, (cursorCharY - shiftCharY + 1) * charHeight };
		InvalidateRect(areaWnd, &invalid, false);
	}else if(currLine->prev){	// If it's at the beginning of a line and there is a line before current
		// Add current line to the previous, as if are removing the line break
		currLine = currLine->prev;

		history->TakeSnapshot(currLine, HistoryManager::LINES_DELETED, 2);
		// Check if there is enough space
		unsigned int sum = currLine->length + currLine->next->length;
		ExtendLine(currLine, sum);
		// Append one line to the other
		memcpy(&currLine->data[currLine->length], currLine->next->data, currLine->next->length * sizeof(AreaChar));

		// Update cursor position
		cursorCharX = currLine->length;
		cursorCharY--;

		// Update line length
		currLine->length = sum;

		// Remove line that was current before event
		DeleteLine(currLine->next);
		lineCount--;

		// Force redraw on the updated region
		RECT invalid = { 0, (cursorCharY - shiftCharY - 1) * charHeight, areaWidth, areaHeight };
		InvalidateRect(areaWnd, &invalid, false);
	}
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

void OnCopyOrCut(bool cut)
{
	// Get linear text
	const char *start = GetAreaText();
	AreaLine *curr = firstLine;

	// If there is no selection, remember this fact, and select current line
	bool genuineSelection = selectionOn;
	if(!selectionOn)
	{
		dragStartX = 0;
		dragEndX = currLine->length;
		dragStartY = cursorCharY;
		dragEndY = cursorCharY;
		selectionOn = true;
	}
	// Sort selection range
	unsigned int startX, startY, endX, endY;
	SortSelPoints(startX, endX, startY, endY);

	// Clamp selection points in Y axis
	startY = startY > lineCount ? lineCount - 1 : startY;
	endY = endY > lineCount ? lineCount-1 : endY;

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
	OpenClipboard(areaWnd);
	EmptyClipboard();
	HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, end - start + 1);
	char *pchData = (char*)GlobalLock(hClipboardData);
	memcpy(pchData, start, end - start);
	GlobalUnlock(hClipboardData);
	SetClipboardData(CF_TEXT, hClipboardData);
	CloseClipboard();
	// If it is a cut operation, remove selection
	if(cut)
		DeleteSelection();
	// If we had to make an artificial selection, disable it
	if(!genuineSelection)
		selectionOn = false;
}

void OnPaste()
{
	// Remove selection
	if(selectionOn)
		DeleteSelection();
	
	// Get pasted text
	OpenClipboard(areaWnd);
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
		history->TakeSnapshot(currLine, HistoryManager::LINES_ADDED, linesAdded);

		str = (char*)clipData;
		// Simulate as if the text was written
		while(*str)
		{
			if(*str >= 0x20 || *str == '\t')
				InputChar(*str);
			else if(*str == '\r')
				InputEnter();
			str++;
		}
	}
	CloseClipboard();
	// Force update on whole window
	InvalidateRect(areaWnd, NULL, false);
}

void OnCreate()
{
	if(!AreaLine::pool)
		AreaLine::pool = new ObjectBlockPool<AreaLine, 32>();

	HDC hdc = BeginPaint(areaWnd, &areaPS);
	// Create font for every FONT_STYLE
	areaFont[FONT_REGULAR] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, false, false,
		RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");
	areaFont[FONT_BOLD] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_BOLD, false, false, false,
		RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");
	areaFont[FONT_ITALIC] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, true, false, false,
		RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");
	areaFont[FONT_UNDERLINED] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, true, false,
		RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");

	// Create pens and brushes
	areaPenWhite1px = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	areaPenBlack1px = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

	areaBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
	areaBrushBlack = CreateSolidBrush(RGB(0, 0, 0));
	areaBrushSelected = CreateSolidBrush(RGB(51, 153, 255));

	EndPaint(areaWnd, &areaPS);
	
	// Init linear text buffer
	areaTextSize = 16 * 1024;
	areaText = new char[areaTextSize];
	areaTextEx = new char[areaTextSize];

	// Create first line of text
	currLine = firstLine = InsertLineAfter(NULL);
	currLine->data[0].style = 0;

	lineCount = 1;

	history = new HistoryManager();

	SetTimer(areaWnd, 0, 62, AreaCursorUpdate);
	areaCreated = true;
}

void OnDestroy()
{
	ClearAreaText();
	DeleteLine(firstLine);
	delete[] areaText;
	delete[] areaTextEx;
	delete AreaLine::pool;
	delete history;
	areaText = NULL;
	areaTextEx = NULL;
	AreaLine::pool = NULL;
	history = NULL;
}

void OnCharacter(char ch)
{
	unsigned int startX, startY, endX, endY;
	if(ch >= 0x20 || ch == '\t')	// If it isn't special symbol or it is a Tab
	{
		// We add a history snapshot only if cursor position changed since last character input
		static unsigned int lastCursorX = ~0u, lastCursorY = ~0u;
		if(cursorCharX != lastCursorX || cursorCharY != lastCursorY)
		{
			history->TakeSnapshot(currLine, HistoryManager::LINES_CHANGED, 1);
			lastCursorX = cursorCharX;
			lastCursorY = cursorCharY;
		}
		// Compensate the caret movement
		lastCursorX++;
		// If insert mode, create selection
		if(insertionMode)
		{
			selectionOn = true;
			dragStartX = cursorCharX;
			dragEndY = dragStartY = cursorCharY;
			dragEndX = cursorCharX + 1;
			if(dragEndX > int(currLine->length))
				selectionOn = false;
		}
		// Remove selection
		if(selectionOn)
		{
			SortSelPoints(startX, endX, startY, endY);
			// If Tab key was pressed, and we have a multiple-line selection,
			// depending on the state of Shift key,
			// we either need to add Tab character at the beginning of each line
			// or we have to remove whitespace at the beginning of the each line
			if(ch == '\t' && startY != endY)
			{
				cursorCharY = startY;
				// Clamp cursor position and select first line
				ClampCursorBounds();
				// For every selected line
				for(unsigned int i = startY; i <= endY; i++)
				{
					// Inserting in front
					cursorCharX = 0;
					// If it's a Shift+Tab
					if(IsPressed(VK_SHIFT))
					{
						// We have to remove a number of whitespaces so the length of removed whitespaces will be equal to tab size
						int toRemove = 0;
						// So we select a maximum of TAB_SIZE spaces
						while(toRemove < min(TAB_SIZE, int(currLine->length)) && currLine->data[toRemove].ch == ' ')
							toRemove++;
						// And if we haven't reached our goal, and there is a Tab symbol
						if(toRemove < min(TAB_SIZE, int(currLine->length)) && currLine->data[toRemove].ch == '\t')
							toRemove++;	// Select it too
						// Remove characters
						memmove(&currLine->data[0], &currLine->data[toRemove], (currLine->length - toRemove) * sizeof(AreaChar));
						// Shrink line length
						currLine->length -= toRemove;
					}else{	// Simply Tab, insert symbol
						InputChar(ch);
					}
					currLine = currLine->next;
					cursorCharY++;
				}
				// Restore cursor position
				cursorCharX = endX;
				cursorCharY--;
				// Clamp cursor position and update current line
				ClampCursorBounds();
				InvalidateRect(areaWnd, NULL, false);
				return;
			}else if(ch == '\t' && IsPressed(VK_SHIFT)){
				cursorCharX = startX;
				cursorCharY = startY;
				// Clamp cursor position and select first line
				ClampCursorBounds();
				if(startX && isspace(currLine->data[startX-1].ch))
				{
					DeletePreviousChar();
					if(startY == endY)
					{
						cursorCharX = endX - 1;
						if(dragStartX == startX)
							dragStartX = --startX;
						else
							dragEndX = --startX;
					}
				}
				return;
			}else{
				// Otherwise, just remove selection
				DeleteSelection();
			}
		}else if(ch == '\t' && IsPressed(VK_SHIFT)){
			if(cursorCharX && isspace(currLine->data[cursorCharX-1].ch))
				DeletePreviousChar();
			return;
		}
		// Insert symbol
		InputChar(ch);
	}else if(ch == '\r'){	// Line break
		history->TakeSnapshot(currLine, HistoryManager::LINES_ADDED, 1);
		// Remove selection
		if(selectionOn)
			DeleteSelection();
		// Find current indentation
		int characterIdent = 0;
		int effectiveIdent = 0;
		while(characterIdent < int(currLine->length) && isspace(currLine->data[characterIdent].ch))
		{
			effectiveIdent += GetCharShift(currLine->data[characterIdent].ch, effectiveIdent);
			characterIdent++;
		}
		// Insert line break
		InputEnter();
		// Add indentation
		while(effectiveIdent > 0)
		{
			InputChar('\t');
			effectiveIdent -= TAB_SIZE;
		}
	}else if(ch == '\b'){	// Backspace
		// Remove selection
		if(selectionOn)
		{
			DeleteSelection();
		}else{
			DeletePreviousChar();
		}
	}else if(ch == 22){	// Ctrl+V
		OnPaste();
	}else if(ch == 1){	// Ctrl+A
		// Select all
		selectionOn = true;
		dragStartX = 0;
		dragStartY = 0;
		// Find last line to know end cursor position
		AreaLine *curr = firstLine;
		int line = 0;
		while(curr->next)
			curr = curr->next, line++;
		dragEndX = curr->length;
		dragEndY = line;
		// Force update on whole window
		InvalidateRect(areaWnd, NULL, false);
	}else if(ch == 3 || ch == 24){	// Ctrl+C and Ctrl+X
		OnCopyOrCut(ch == 24);
	}else if(ch == 26){	// Ctrl+Z
		history->Undo();
	}
	ScrollToCursor();
	needUpdate = true;
}

void OnKeyEvent(int key)
{
	unsigned int startX, startY, endX, endY;

	// If key is pressed, remove I-bar
	AreaCursorUpdate(NULL, 0, NULL, 0);
	// Reset I-bar tick count, so it will be visible
	ibarState = 0;
	// Start selection if Shift+Arrows are in use, and selection is disabled
	if(IsPressed(VK_SHIFT) && !selectionOn && (key == VK_DOWN || key == VK_UP || key == VK_LEFT || key == VK_RIGHT ||
												key == VK_PRIOR || key == VK_NEXT || key == VK_HOME || key == VK_END))
	{
		dragStartX = cursorCharX;
		dragStartY = cursorCharY;
	}
	// First four to move cursor
	if(key == VK_DOWN)
	{
		// If Ctrl is pressed, scroll vertically
		if(IsPressed(VK_CONTROL))
		{
			shiftCharY++;
			ClampShift();
		}
		// If Ctrl is not pressed or if it is and cursor is out of sight
		if(!IsPressed(VK_CONTROL) || (IsPressed(VK_CONTROL) && int(cursorCharY) < shiftCharY))
		{
			// If there is a next line, move to it
			if(currLine->next)
			{
				int oldPosX, oldPosY;
				CursorToClient(cursorCharX, cursorCharY, oldPosX, oldPosY);
				currLine = currLine->next;
				ClientToCursor(oldPosX, oldPosY + charHeight, cursorCharX, cursorCharY, true);
			}
		}
	}else if(key == VK_UP){
		// If Ctrl is pressed, scroll vertically
		if(IsPressed(VK_CONTROL))
		{
			shiftCharY--;
			ClampShift();
		}
		// If Ctrl is not pressed or if it is and cursor is out of sight
		if(!IsPressed(VK_CONTROL) || (IsPressed(VK_CONTROL) && int(cursorCharY) > (shiftCharY + areaHeight / charHeight - 1)))
		{
			// If there is a previous line, move to it
			if(currLine->prev)
			{
				int oldPosX, oldPosY;
				CursorToClient(cursorCharX, cursorCharY, oldPosX, oldPosY);
				currLine = currLine->prev;
				ClientToCursor(oldPosX, oldPosY - charHeight, cursorCharX, cursorCharY, true);
			}
		}
	}else if(key == VK_LEFT){
		// If Shift is not pressed and there is an active selection
		if(!IsPressed(VK_SHIFT) && selectionOn)
		{
			// Sort selection range
			SortSelPoints(startX, endX, startY, endY);
			// Set cursor position to the start of selection
			cursorCharX = startX;
			cursorCharY = startY;
			ClampCursorBounds();
		}else{
			// If the cursor is not at the beginning of the line
			if(cursorCharX > 0)
			{
				if(IsPressed(VK_CONTROL))
				{
					// Skip spaces
					while(cursorCharX > 1 && isspace(currLine->data[cursorCharX - 1].ch))
						cursorCharX--;
					cursorCharX = AdvanceCursor(currLine, cursorCharX, true);
				}else{
					cursorCharX--;
				}
			}else{
				// Otherwise, move to the end of the previous line
				if(currLine->prev)
				{
					currLine = currLine->prev;
					cursorCharX = currLine->length;
					cursorCharY--;
				}
			}
		}
	}else if(key == VK_RIGHT){
		// If Shift is not pressed and there is an active selection
		if(!IsPressed(VK_SHIFT) && selectionOn)
		{
			// Sort selection range
			SortSelPoints(startX, endX, startY, endY);
			// Set cursor position to the end of selection
			cursorCharX = endX;
			cursorCharY = endY;
			ClampCursorBounds();
		}else{
			// If the cursor is not at the end of the line
			if(cursorCharX < currLine->length)
			{
				if(IsPressed(VK_CONTROL))
				{
					cursorCharX = AdvanceCursor(currLine, cursorCharX, false);
					// Skip spaces
					while(int(cursorCharX) < int(currLine->length)-1 && isspace(currLine->data[cursorCharX + 1].ch))
						cursorCharX++;
					cursorCharX++;
				}else{
					cursorCharX++;
				}
			}else{
				// Otherwise, move to the start of the next line
				if(currLine->next)
				{
					currLine = currLine->next;
					cursorCharY++;
					cursorCharX = 0;
				}
			}
		}
	}else if(key == VK_DELETE){	// Delete
		// Shift+Delete is a cut operation
		if(IsPressed(VK_SHIFT))
		{
			OnCopyOrCut(true);
			return;
		}
		// Remove selection, if active
		if(selectionOn)
		{
			DeleteSelection();
			return;
		}else{
			// Move to the next character
			if(cursorCharX < currLine->length)
			{
				cursorCharX++;
			}else if(currLine->next){
				currLine = currLine->next;
				cursorCharY++;
				cursorCharX = 0;
			}else{
				return;
			}
			DeletePreviousChar();
		}
		ScrollToCursor();
	}else if(key == VK_PRIOR){	// Page up
		// If Ctrl is pressed
		if(IsPressed(VK_CONTROL))
		{
			// Move to the start of view
			cursorCharY = shiftCharY;
			ClampCursorBounds();
		}else{
			// Scroll view
			cursorCharY -= charHeight ? areaHeight / charHeight : 1;
			if(int(cursorCharY) < 0)
				cursorCharY = 0;
			ClampCursorBounds();
			ScrollToCursor();
		}
		ClampShift();
		UpdateScrollBar();
		InvalidateRect(areaWnd, NULL, false);
	}else if(key == VK_NEXT){	// Page down
		// If Ctrl is pressed
		if(IsPressed(VK_CONTROL))
		{
			// Move to the end of view
			cursorCharY = shiftCharY + (charHeight ? areaHeight / charHeight : 1);
			ClampCursorBounds();
		}else{
			// Scroll view
			cursorCharY += charHeight ? areaHeight / charHeight : 1;
			if(int(cursorCharY) < 0)
				cursorCharY = 0;
			ClampCursorBounds();
			ScrollToCursor();
		}
		ClampShift();
		UpdateScrollBar();
		InvalidateRect(areaWnd, NULL, false);
	}else if(key == VK_HOME){
		// If Ctrl is pressed
		if(IsPressed(VK_CONTROL))
		{
			// Move view and cursor to the beginning of the text
			shiftCharY = 0;
			cursorCharX = 0;
			cursorCharY = 0;
			ClampCursorBounds();
		}else{
			int identWidth = 0;
			while(identWidth < int(currLine->length) && (currLine->data[identWidth].ch == ' ' || currLine->data[identWidth].ch == '\t'))
				identWidth++;
			// If we are at the beginning of a line, move through all the spaces
			if(cursorCharX == 0 || int(cursorCharX) != identWidth)
				cursorCharX = identWidth;
			else	// Move cursor to the beginning of the line
				cursorCharX = 0;
		}
		ClampShift();
		UpdateScrollBar();
		InvalidateRect(areaWnd, NULL, false);
	}else if(key == VK_END){
		// If Ctrl is pressed
		if(IsPressed(VK_CONTROL))
		{
			// Move view and cursor to the end of the text
			shiftCharY = lineCount;
			cursorCharX = ~0u;
			cursorCharY = lineCount;
			ClampCursorBounds();
		}else{
			// Move cursor to the end of the line
			cursorCharX = currLine->length;
		}
		ClampShift();
		UpdateScrollBar();
		InvalidateRect(areaWnd, NULL, false);
	}else if(key == VK_INSERT){
		// Shift+Insert is a paste operation
		if(IsPressed(VK_SHIFT))
		{
			OnPaste();
		}else if(IsPressed(VK_CONTROL)){	// Ctrl+Insert is a copy operation
			OnCopyOrCut(false);
		}else{
			// Toggle input mode between insert\overwrite
			insertionMode = !insertionMode;
		}
	}else if(key == VK_ESCAPE){
		// Disable selection
		selectionOn = false;
		InvalidateRect(areaWnd, NULL, false);
	}
	if(key == VK_DOWN || key == VK_UP || key == VK_LEFT || key == VK_RIGHT ||
		key == VK_PRIOR || key == VK_NEXT || key == VK_HOME || key == VK_END)
	{
		// If Ctrl is not pressed, center view around cursor
		if(!IsPressed(VK_CONTROL))
			ScrollToCursor();
		// If Shift is pressed, set end of selection, redraw window and return
		if(IsPressed(VK_SHIFT))
		{
			dragEndX = cursorCharX;
			dragEndY = cursorCharY;
			if(dragStartX != dragEndX || dragStartY != dragEndY)
				selectionOn = true;
			InvalidateRect(areaWnd, NULL, false);
			return;
		}else{
			// Or disable selection, and if control is not pressed, update window and return
			selectionOn = false;
			if(!IsPressed(VK_CONTROL))
			{
				InvalidateRect(areaWnd, NULL, false);
				return;
			}
		}
		// Draw cursor
		AreaCursorUpdate(areaWnd, 0, NULL, 0);
		// Handle Ctrl+Arrows
		if(IsPressed(VK_CONTROL))
		{
			ClampShift();
			UpdateScrollBar();
			InvalidateRect(areaWnd, NULL, false);
		}
	}
}

void ExtendSelectionFromPoint(unsigned int xPos, unsigned int yPos)
{
	// Find cursor position
	AreaLine *curr = ClientToCursor(xPos, yPos, dragStartX, dragStartY, true);

	if(curr->length == 0)
		return;
	// Clamp horizontal position to line length
	if(dragStartX >= (int)curr->length && curr->length != 0)
		dragStartX = curr->length - 1;
	dragEndX = dragStartX;
	dragEndY = dragStartY;

	dragStartX = AdvanceCursor(curr, dragStartX, true);
	dragEndX = AdvanceCursor(curr, dragEndX, false) + 1;

	// Selection is active is the length of selected string is not 0
	selectionOn = curr->length != 0;
}

// Textarea message handler
LRESULT CALLBACK TextareaProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	unsigned int startX, startY, endX, endY;

	if(!areaWnd)
		areaWnd = hWnd;
	switch(message)
	{
	case WM_CREATE:
		OnCreate();
		break;
	case WM_DESTROY:
		OnDestroy();
		break;
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		ReDraw();
		break;
	case WM_SIZE:
		areaWidth = LOWORD(lParam);
		areaHeight = HIWORD(lParam);

		UpdateScrollBar();

		InvalidateRect(areaWnd, NULL, false);
		break;
	case WM_MOUSEACTIVATE:
		SetFocus(areaWnd);
		EnableWindow(areaWnd, true);
		break;
	case WM_CHAR:
		OnCharacter((char)(wParam & 0xFF));
		break;
	case WM_KEYDOWN:
		if(wParam == VK_CONTROL || wParam == VK_SHIFT)
			break;
		OnKeyEvent((int)wParam);
		break;
	case WM_LBUTTONDBLCLK:
		ExtendSelectionFromPoint(LOWORD(lParam), HIWORD(lParam));
		if(selectionOn)
		{
			cursorCharX = dragEndX;
			cursorCharY = dragEndY;
			// Force line redraw
			RECT invalid = { 0, (dragStartY - shiftCharY) * charHeight, areaWidth, (dragEndY - shiftCharY + 1) * charHeight };
			InvalidateRect(areaWnd, &invalid, false);
		}
		break;
	case WM_LBUTTONDOWN:
		// Capture mouse
		SetCapture(areaWnd);
		// Remove cursor
		AreaCursorUpdate(NULL, 0, NULL, 0);
		// Reset I-bar tick count
		ibarState = 0;
		// When left mouse button is pressed, disable selection mode and save position as selection start
		if(selectionOn && !IsPressed(VK_SHIFT))
		{
			selectionOn = false;
			// Sort selection range
			SortSelPoints(startX, endX, startY, endY);
			// Force selected part redraw
			RECT invalid = { 0, (startY - shiftCharY) * charHeight, areaWidth, (endY - shiftCharY + 1) * charHeight };
			InvalidateRect(areaWnd, &invalid, false);
		}
		if(IsPressed(VK_SHIFT))
		{
			// If there is no selection, start it
			if(!selectionOn)
			{
				dragStartX = cursorCharX;
				dragStartY = cursorCharY;
			}
			// Update end of selection
			currLine = ClientToCursor(LOWORD(lParam), HIWORD(lParam), dragEndX, dragEndY, true);
			cursorCharX = dragEndX;
			cursorCharY = dragEndY;
			selectionOn = true;
			InvalidateRect(areaWnd, NULL, false);
		}else if(IsPressed(VK_CONTROL)){
			ExtendSelectionFromPoint(LOWORD(lParam), HIWORD(lParam));
			cursorCharX = dragEndX;
			cursorCharY = dragEndY;
			InvalidateRect(areaWnd, NULL, false);
		}else{
			// Set drag start and cursor position to where the user have clicked
			currLine = ClientToCursor(LOWORD(lParam), HIWORD(lParam), dragStartX, dragStartY, true);
			cursorCharX = dragStartX;
			cursorCharY = dragStartY;
		}
		break;
	case WM_MOUSEMOVE:
		// If mouse if moving with the left mouse down
		if(!(wParam & MK_LBUTTON))
			break;

		// Sort old selection range
		SortSelPoints(startX, endX, startY, endY);

		// Track the cursor position which is the selection end
		currLine = ClientToCursor(LOWORD(lParam), HIWORD(lParam), dragEndX, dragEndY, false);

		if(IsPressed(VK_CONTROL))
		{
			if(dragEndY > dragStartY || dragEndX > dragStartX)
				dragEndX = AdvanceCursor(currLine, dragEndX, false) + 1;
			else
				dragEndX = AdvanceCursor(currLine, dragEndX, true);
			dragEndX = dragEndX > currLine->length ? currLine->length : dragEndX;
			cursorCharX = dragEndX;
			cursorCharY = dragEndY;
		}else{
			// Find cursor position
			currLine = ClientToCursor(LOWORD(lParam), HIWORD(lParam), cursorCharX, cursorCharY, true);
		}

		// If current position differs from starting position, enable selection mode
		if(dragStartX != dragEndX || dragStartY != dragEndY)
			selectionOn = true;
		// Redraw selection
		{
			// Sort selection range
			unsigned int newStartY, newEndY;
			SortSelPoints(startX, endX, newStartY, newEndY);
			newStartY = min(newStartY, startY);
			newEndY = max(newEndY, endY);
			// Force selected part redraw
			RECT invalid = { 0, (newStartY - shiftCharY) * charHeight, areaWidth, (newEndY - shiftCharY + 1) * charHeight };
			InvalidateRect(areaWnd, &invalid, false);
		}
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		break;
	case WM_MOUSEWHEEL:
		// Mouse wheel scroll text vertically
		shiftCharY -= (GET_WHEEL_DELTA_WPARAM(wParam) / 120) * 3;
		ClampShift();
		InvalidateRect(areaWnd, NULL, false);

		UpdateScrollBar();
		break;
	case WM_VSCROLL:
		// Vertical scroll events
		switch(LOWORD(wParam))
		{
		case SB_LINEDOWN:
			shiftCharY++;
			break;
		case SB_LINEUP:
			shiftCharY--;
			break;
		case SB_PAGEDOWN:
			shiftCharY += charHeight ? areaHeight / charHeight : 1;
			break;
		case SB_PAGEUP:
			shiftCharY -= charHeight ? areaHeight / charHeight : 1;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			shiftCharY = HIWORD(wParam);
			break;
		}
		ClampShift();
		UpdateScrollBar();
		InvalidateRect(areaWnd, NULL, false);
		break;
	case WM_HSCROLL:
		// Horizontal scroll events
		switch(LOWORD(wParam))
		{
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			shiftCharX++;
			break;
		case SB_LINEUP:
		case SB_PAGEUP:
			shiftCharX--;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			shiftCharX = HIWORD(wParam);
			break;
		}
		ClampShift();
		UpdateScrollBar();
		InvalidateRect(areaWnd, NULL, false);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
