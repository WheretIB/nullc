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
	AreaLine(){}

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
unsigned int charWidth, charHeight;
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
HBRUSH	areaBrushWhite, areaBrushBlack;

AreaLine	*firstLine = NULL;
// Current line is the line, where cursor is placed
AreaLine	*currLine = NULL;

// For text selection, here are starting and ending positions (in symbols)
int dragStartX, dragStartY;
int dragEndX, dragEndY;
// And a flag that shows if the selection is active
bool selectionOn = false;

// Is symbol overwrite on
bool insertionMode = false;

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
	for(unsigned int i = 0, n = 0; i < maximumEnd;)
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
		// Append line ending
		*currChar++ = '\r';
		*currChar++ = '\n';
		// Move to the next line
		curr = curr->next;
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
		if(*text >= 0x20)
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


void ReDraw()
{
	if(!areaCreated)
		return;

	// Windows will tell, what part we have to update
	RECT updateRect;
	if(!GetUpdateRect(areaWnd, &updateRect, false))
		return;

	// Find the first line for current vertical scroll position
	AreaLine *startLine = firstLine;
	for(int i = 0; i < shiftCharY; i++)
		startLine = startLine->next;

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
	curr = firstLine;
	longestLine = 0;
	while(curr)
	{
		longestLine = longestLine < (int)curr->length ? (int)curr->length : longestLine;
		curr = curr->next;
	}
	// Reset horizontal scroll position, if longest line can fit to window
	if(longestLine < int(areaWidth / charWidth) - 1)
		shiftCharX = 0;
	UpdateScrollBar();

	// Setup the box of the first symbol
	charRect.left = padLeft;
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
			for(unsigned int i = shiftCharX; i < curr->length; i++)
			{
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
					SetBkColor(hdc, RGB(0,0,0));
					SetTextColor(hdc, RGB(255-style.color[0], 255-style.color[1], 255-style.color[2]));
				}else{
					SetBkColor(hdc, RGB(255,255,255));
					SetTextColor(hdc, RGB(style.color[0], style.color[1], style.color[2]));
				}
				// If symbol has different font, change it
				if(style.font != currFont)
				{
					currFont = (FontStyle)style.font;
					SelectFont(hdc, areaFont[currFont]);
				}
				// Draw character
				DrawText(hdc, &curr->data[i].ch, 1, &charRect, 0);
				// Shift the box to the next position
				charRect.left += charWidth;
				charRect.right += charWidth;
				// Break if out of view
				if(charRect.left > areaWidth - int(charWidth))
					break;
			}
			// Fill the end of the line with white color
			charRect.right = areaWidth - charWidth;
			FillRect(hdc, &charRect, areaBrushWhite);
		}
		// Shift to the beginning of the next line
		charRect.left = padLeft;
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

	// While selecting pen, we check if our window is active. Cursor shouldn't blink if focus is on some other window
	SelectPen(hdc, GetFocus() == hwnd ? currPen : areaPenWhite1px);

	MoveToEx(hdc, padLeft + (cursorCharX - shiftCharX) * charWidth, (cursorCharY - shiftCharY) * charHeight, NULL);
	LineTo(hdc, padLeft + (cursorCharX - shiftCharX) * charWidth, (cursorCharY - shiftCharY) * charHeight + charHeight);

	if(insertionMode)
	{
		// Draw full box
		LineTo(hdc, padLeft + (cursorCharX - shiftCharX) * charWidth + charWidth, (cursorCharY - shiftCharY) * charHeight + charHeight);
		LineTo(hdc, padLeft + (cursorCharX - shiftCharX) * charWidth+ charWidth, (cursorCharY - shiftCharY) * charHeight);
		LineTo(hdc, padLeft + (cursorCharX - shiftCharX) * charWidth, (cursorCharY - shiftCharY) * charHeight);
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
	// Create new line
	AreaLine *nLine = new AreaLine;
	memset(nLine, 0, sizeof(AreaLine));
	// Link new line with others
	if(currLine->next)
	{
		currLine->next->prev = nLine;
		nLine->next = currLine->next;
	}else{
		nLine->next = NULL;
	}
	currLine->next = nLine;
	nLine->prev = currLine;
	currLine = currLine->next;

	// Default state
	currLine->length = 0;
	currLine->maxLength = DEFAULT_STRING_LENGTH;
	currLine->data = currLine->startBuf;

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

	// For single-line selection
	if(startY == endY)
	{
		// Find selected line
		AreaLine *curr = firstLine;
		for(unsigned int i = 0; i < startY; i++)
			curr = curr->next;
		// Clamp selection points in Z axis
		endX = endX > curr->length ? curr->length : endX;
		startX = startX > curr->length ? curr->length : startX;
		// Move text after selection to the start of selection
		memmove(&curr->data[startX], &curr->data[endX], (curr->length - endX) * sizeof(AreaChar));
		// Shrink length
		curr->length -= endX - startX;
		// Move cursor and disable selection mode
		cursorCharX = startX;
		selectionOn = false;
	}else{	// For multi-line selection
		// Find first selection line
		AreaLine *first = firstLine;
		for(unsigned int i = 0; i < startY; i++)
			first = first->next;
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

// Textarea message handler
LRESULT CALLBACK TextareaProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;

	unsigned int startX, startY, endX, endY;

	if(!areaWnd)
		areaWnd = hWnd;
	switch(message)
	{
	case WM_CREATE:
		if(!AreaLine::pool)
			AreaLine::pool = new ObjectBlockPool<AreaLine, 32>();

		hdc = BeginPaint(areaWnd, &areaPS);
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

		EndPaint(areaWnd, &areaPS);
		
		// Init linear text buffer
		areaTextSize = 16 * 1024;
		areaText = new char[areaTextSize];
		areaTextEx = new char[areaTextSize];

		// Create first line of text
		firstLine = new AreaLine;
		memset(firstLine, 0, sizeof(AreaLine));
		firstLine->data = firstLine->startBuf;
		firstLine->next = firstLine->prev = NULL;
		firstLine->length = 0;
		firstLine->maxLength = DEFAULT_STRING_LENGTH;
		
		currLine = firstLine;
		currLine->data[0].style = 0;

		lineCount = 1;

		SetTimer(areaWnd, 0, 62, AreaCursorUpdate);
		areaCreated = true;
		break;
	case WM_DESTROY:
		ClearAreaText();
		DeleteLine(firstLine);
		delete[] areaText;
		delete[] areaTextEx;
		delete AreaLine::pool;
		areaText = NULL;
		areaTextEx = NULL;
		AreaLine::pool = NULL;
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
		if((wParam & 0xFF) >= 0x20)	// If it isn't special symbol
		{
			// If insert mode, create selection
			if(insertionMode)
			{
				selectionOn = true;
				dragStartX = cursorCharX;
				dragEndY = dragStartY = cursorCharY;
				dragEndX = cursorCharX + 1;
				if(dragEndX > currLine->length)
				{
					dragEndY = cursorCharY + 1;
					dragEndX = 0;
				}
			}
			// Remove selection
			if(selectionOn)
				DeleteSelection();
			// Insert symbol
			InputChar((char)(wParam & 0xFF));
		}else if((wParam & 0xFF) == '\r'){	// Line break
			// Remove selection
			if(selectionOn)
				DeleteSelection();
			// Insert symbol
			InputEnter();
		}else if((wParam & 0xFF) == '\b'){	// Backspace
			// Remove selection
			if(selectionOn)
			{
				DeleteSelection();
			}else{
				// If cursor is not at the beginning of a line
				if(cursorCharX)
				{
					// If cursor is in the middle, move characters to the vacant position
					if(cursorCharX != currLine->length)
						memmove(&currLine->data[cursorCharX-1], &currLine->data[cursorCharX], (currLine->length - cursorCharX) * sizeof(AreaChar));
					// Shrink line size and move cursor
					currLine->length--;
					cursorCharX--;
					// Force redraw on the updated region
					RECT invalid = { 0, (cursorCharY - shiftCharY) * charHeight, areaWidth, (cursorCharY - shiftCharY + 1) * charHeight };
					InvalidateRect(areaWnd, &invalid, false);
				}else{
					// If there is a line before current
					if(currLine->prev)
					{
						// Add current line to the previous, as if are removing the line break
						currLine = currLine->prev;
						// Check if there is enough space
						unsigned int sum = currLine->length + currLine->next->length;
						ExtendLine(currLine, sum);
						// Append one line to the other
						memcpy(&currLine->data[currLine->length], currLine->next->data, currLine->next->length * sizeof(AreaChar));
						currLine->length = sum;

						// Remove line that was current before event
						DeleteLine(currLine->next);
						lineCount--;

						// Place cursor at the end of line
						cursorCharX = sum;
						cursorCharY--;

						// Force redraw on the updated region
						RECT invalid = { 0, (cursorCharY - shiftCharY - 1) * charHeight, areaWidth, areaHeight };
						InvalidateRect(areaWnd, &invalid, false);
					}
				}
			}
		}else if((wParam & 0xFF) == 22){	// Ctrl+V
			// Remove selection
			if(selectionOn)
				DeleteSelection();
			// Get pasted text
			OpenClipboard(areaWnd);
			HANDLE clipData = GetClipboardData(CF_TEXT);
			if(clipData)
			{
				// Simulate as if the text was written
				char *str = (char*)clipData;
				while(*str)
				{
					if(*str >= 0x20)
						InputChar(*str);
					else if(*str == '\r')
						InputEnter();
					str++;
				}
			}
			CloseClipboard();
			// Force update on whole window
			InvalidateRect(areaWnd, NULL, false);
		}else if((wParam & 0xFF) == 1){	// Ctrl+A
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
		}else if(((wParam & 0xFF) == 3 || (wParam & 0xFF) == 24)){	// Ctrl+C and Ctrl+X
			// Get linear text
			const char *start = GetAreaText();
			AreaLine *curr = firstLine;

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
			if((wParam & 0xFF) == 24)
				DeleteSelection();
			if(!genuineSelection)
				selectionOn = false;
		}
		ScrollToCursor();
		needUpdate = true;
		break;
	case WM_KEYDOWN:
		if(wParam == VK_CONTROL)
			break;
		// If key is pressed, remove I-bar
		AreaCursorUpdate(NULL, 0, NULL, 0);
		// Reset I-bar tick count, so it will be visible
		ibarState = 0;
		// Selection using Shift+Arrows
		if(GetAsyncKeyState(VK_SHIFT) && !selectionOn && (wParam == VK_DOWN || wParam == VK_UP || wParam == VK_LEFT || wParam == VK_RIGHT))
		{
			dragStartX = cursorCharX;
			dragStartY = cursorCharY;
			selectionOn = true;
			InvalidateRect(areaWnd, NULL, false);
		}
		// First four to move cursor
		if(wParam == VK_DOWN)
		{
			if(GetAsyncKeyState(VK_CONTROL))
			{
				shiftCharY++;
			}else{
				if(currLine->next)
				{
					currLine = currLine->next;
					cursorCharY++;
					if(cursorCharX > currLine->length)
						cursorCharX = currLine->length;
				}
			}
		}else if(wParam == VK_UP){
			if(GetAsyncKeyState(VK_CONTROL))
			{
				shiftCharY--;
			}else{
				if(currLine->prev)
				{
					currLine = currLine->prev;
					cursorCharY--;
					if(cursorCharX > currLine->length)
						cursorCharX = currLine->length;
				}
			}
		}else if(wParam == VK_LEFT){
			if(cursorCharX > 0)
			{
				cursorCharX--;
				if(GetAsyncKeyState(VK_CONTROL))
				{
					if(isspace(currLine->data[cursorCharX - 1].ch))
						while(cursorCharX >= 1 && isspace(currLine->data[cursorCharX - 1].ch))
							cursorCharX--;
					else
						while(cursorCharX >= 1 && (isalnum(currLine->data[cursorCharX - 1].ch) || currLine->data[cursorCharX - 1].ch == '_' || currLine->data[cursorCharX - 1].ch == '.'))
							cursorCharX--;
				}
			}else{
				if(currLine->prev)
				{
					currLine = currLine->prev;
					cursorCharX = currLine->length;
					cursorCharY--;
				}
			}
		}else if(wParam == VK_RIGHT){
			if(cursorCharX < currLine->length)
			{
				cursorCharX++;
				if(GetAsyncKeyState(VK_CONTROL))
				{
					if(isspace(currLine->data[cursorCharX].ch))
						while((cursorCharX < currLine->length) && isspace(currLine->data[cursorCharX].ch))
							cursorCharX++;
					else
						while((cursorCharX < currLine->length) && (isalnum(currLine->data[cursorCharX].ch) || currLine->data[cursorCharX].ch == '_' || currLine->data[cursorCharX].ch == '.'))
							cursorCharX++;
				}
			}else{
				if(currLine->next)
				{
					currLine = currLine->next;
					cursorCharY++;
					cursorCharX = 0;
				}
			}
		}else if(wParam == VK_DELETE){	// Delete
			// Remove selection, if active
			if(selectionOn)
			{
				DeleteSelection();
				return 0;
			}else{
				// If the cursor is not at the end of the line
				if(cursorCharX != currLine->length)
				{
					// Move characters to vacant space
					memmove(&currLine->data[cursorCharX], &currLine->data[cursorCharX+1], (currLine->length - cursorCharX) * sizeof(AreaChar));
					// Shrink line size
					currLine->length--;
					// Force redraw on updated region
					RECT invalid = { 0, (cursorCharY - shiftCharY) * charHeight, areaWidth, (cursorCharY - shiftCharY + 1) * charHeight };
					InvalidateRect(areaWnd, &invalid, false);
					return 0;
				}else if(currLine->next){	// If delete was pressed on the end of the line and there is a line afterwards
					// Merge next line with current
					unsigned int sum = currLine->length + currLine->next->length;
					// Find out, if we have needed space
					ExtendLine(currLine, sum);
					// Append next line to current
					memcpy(&currLine->data[currLine->length], currLine->next->data, currLine->next->length * sizeof(AreaChar));
					// Extend length
					currLine->length = sum;

					// Remove next line
					DeleteLine(currLine->next);
					lineCount--;
					// Force redraw on the updated region
					RECT invalid = { 0, (cursorCharY - shiftCharY - 1) * charHeight, areaWidth, areaHeight };
					InvalidateRect(areaWnd, &invalid, false);
					return 0;
				}
			}
			ScrollToCursor();
		}else if(wParam == VK_PRIOR){	// Page up
			if(GetAsyncKeyState(VK_CONTROL))
			{
				cursorCharY = shiftCharY;
				ClampCursorBounds();
			}else{
				shiftCharY -= charHeight ? areaHeight / charHeight : 1;
			}
			ClampShift();
			UpdateScrollBar();
			InvalidateRect(areaWnd, NULL, false);
		}else if(wParam == VK_NEXT){	// Page down
			if(GetAsyncKeyState(VK_CONTROL))
			{
				cursorCharY = shiftCharY + (charHeight ? areaHeight / charHeight : 1);
				ClampCursorBounds();
			}else{
				shiftCharY += charHeight ? areaHeight / charHeight : 1;
			}
			ClampShift();
			UpdateScrollBar();
			InvalidateRect(areaWnd, NULL, false);
		}else if(wParam == VK_HOME){
			if(GetAsyncKeyState(VK_CONTROL))
			{
				shiftCharY = 0;
				cursorCharX = 0;
				cursorCharY = 0;
			}else{
				if(cursorCharX == 0)
				{
					while(cursorCharX < currLine->length && (currLine->data[cursorCharX].ch == ' ' || currLine->data[cursorCharX].ch == '\t'))
						cursorCharX++;
				}else{
					cursorCharX = 0;
				}
			}
			ClampShift();
			UpdateScrollBar();
			InvalidateRect(areaWnd, NULL, false);
		}else if(wParam == VK_END){
			if(GetAsyncKeyState(VK_CONTROL))
			{
				shiftCharY = lineCount;
				cursorCharX = ~0u;
				cursorCharY = lineCount;
				ClampCursorBounds();
			}else{
				cursorCharX = currLine->length;
			}
			ClampShift();
			UpdateScrollBar();
			InvalidateRect(areaWnd, NULL, false);
		}else if(wParam == VK_INSERT){
			insertionMode = !insertionMode;
		}
		if(wParam == VK_DOWN || wParam == VK_UP || wParam == VK_LEFT || wParam == VK_RIGHT)
		{
			if(!GetAsyncKeyState(VK_CONTROL))
				ScrollToCursor();
			if(GetAsyncKeyState(VK_SHIFT))
			{
				dragEndX = cursorCharX;
				dragEndY = cursorCharY;
				InvalidateRect(areaWnd, NULL, false);
			}else{
				selectionOn = false;
			}
			// Draw cursor
			AreaCursorUpdate(areaWnd, 0, NULL, 0);
			// Ctrl+Arrows
			if(GetAsyncKeyState(VK_CONTROL))
			{
				ClampShift();
				UpdateScrollBar();
				InvalidateRect(areaWnd, NULL, false);
			}
		}
		break;
	case WM_LBUTTONDBLCLK:
		// Find line number, where double-click happened
		dragEndY = dragStartY = HIWORD(lParam) / charHeight + shiftCharY;
		// Clamp it to the line count
		if(dragStartY > (int)lineCount)
			break;
		
		{
			// Find selected line
			AreaLine *curr = firstLine;
			for(int i = 0; i < dragStartY; i++)
				curr = curr->next;

			// Find horizontal position
			dragStartX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
			// Clamp horizontal position to line length
			if(dragStartX > (int)curr->length && curr->length != 0)
				dragStartX = curr->length - 1;
			dragEndX = dragStartX;

			// Find, what character is at the cursor position
			char symb = curr->data[dragStartX].ch;
			// If it is a digit
			if(isdigit(symb) || symb == '.')
			{
				// Extend region to the left, to include all digits and '.'
				while(dragStartX != 0 && (isdigit(curr->data[dragStartX - 1].ch) || curr->data[dragStartX - 1].ch == '.'))
					dragStartX--;
				// Extend region to the right, to include all digits and '.'
				while(dragEndX < (int)curr->length - 1 && (isdigit(curr->data[dragEndX + 1].ch) || curr->data[dragEndX + 1].ch == '.'))
					dragEndX++;
			}else if(isalpha(symb) || symb == '_'){	// If it isn't a digit, but a alpha letter or '_'
				// Extend region to the left, to include all alpha-numerical letters and '_'
				while(dragStartX != 0 && (isalnum(curr->data[dragStartX - 1].ch) || curr->data[dragStartX - 1].ch == '_'))
					dragStartX--;
				// Extend region to the right, to include all alpha-numerical letters and '_'
				while(dragEndX < (int)curr->length - 1 && (isalnum(curr->data[dragEndX + 1].ch) || curr->data[dragEndX + 1].ch == '_'))
					dragEndX++;
			}
			// Additional shift needed
			dragEndX++;

			// Selection is active is the length of selected string is not 0
			selectionOn = curr->length != 0;
		}
		if(selectionOn)
		{
			// Force line redraw
			RECT invalid = { (dragStartX - shiftCharX) * charWidth, (dragStartY - shiftCharY) * charHeight, (dragEndX - shiftCharX + 1) * charWidth, (dragEndY - shiftCharY + 1) * charHeight };
			InvalidateRect(areaWnd, &invalid, false);
		}
		break;
	case WM_LBUTTONDOWN:
		// When left mouse button is pressed, disable selection mode and save position as selection start
		if(selectionOn)
		{
			selectionOn = false;
			// Sort selection range
			SortSelPoints(startX, endX, startY, endY);
			// Force selected part redraw
			RECT invalid = { 0, (startY - shiftCharY) * charHeight, areaWidth, (endY - shiftCharY + 1) * charHeight };
			InvalidateRect(areaWnd, &invalid, false);
		}
		if(GetAsyncKeyState(VK_SHIFT))
		{
			if(!selectionOn)
			{
				dragStartX = cursorCharX;
				dragStartY = cursorCharY;
			}
			dragEndX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
			dragEndY = HIWORD(lParam) / charHeight + shiftCharY;
			selectionOn = true;
			InvalidateRect(areaWnd, NULL, false);
		}else{
			dragStartX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
			dragStartY = HIWORD(lParam) / charHeight + shiftCharY;
		}
		break;
	case WM_MOUSEMOVE:
		// If mouse if moving with the left mouse down
		if(wParam != MK_LBUTTON)
			break;

		// Sort old selection range
		SortSelPoints(startX, endX, startY, endY);

		// Track the cursor position which is the selection end
		dragEndX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
		dragEndY = HIWORD(lParam) / charHeight + shiftCharY;
		// If current position differs from starting position, enable selection mode
		if(dragStartX != dragEndX || dragStartY != dragEndY)
			selectionOn = true;
		// No break!
	case WM_LBUTTONUP:
		if(GetAsyncKeyState(VK_SHIFT))
			break;
		// If left mouse button is released, or we are continuing previous case
		// Remove I-bar
		AreaCursorUpdate(NULL, 0, NULL, 0);
		// If we are continuing previous case, force window redraw
		if(message == WM_MOUSEMOVE)
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
		// Find cursor position
		cursorCharX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
		cursorCharY = HIWORD(lParam) / charHeight + shiftCharY;

		// Clamp cursor position (notice, that we never clamped selection cursor positions)
		ClampCursorBounds();
		// Reset I-bar tick count
		ibarState = 0;
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
