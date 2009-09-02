#include "RichTextarea.h"

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
	AreaChar		startBuf[32];
	AreaChar		*data;
	unsigned int	length;
	unsigned int	maxLength;

	AreaLine		*prev, *next;
};

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

// Set style parameters. bold\italics\underline flags don't work together
bool SetTextStyle(unsigned int id, char red, char green, char blue, bool bold, bool italics, bool underline)
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

// Function called before setting style to text parts. It reserves needed space in linear buffer
void BeginStyleUpdate()
{
	ExtendLinearTextBuffer();
}

// Style is changed for linear buffer
void SetStyleToSelection(unsigned int start, unsigned int end, int style)
{
	// Simply set style to the specified range of symbols
	for(unsigned int i = start; i <= end; i++)
		areaTextEx[i] = (char)style;
}

// Function is called after setting style to text parts. It copies style information from linear buffer to AreaLine list
void EndStyleUpdate()
{
	AreaLine *curr = firstLine;
	// i has the position inside linear buffer
	// n has the position inside a line
	for(unsigned int i = 0, n = 0; i < overallLength;)
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
	sbInfo.nPage = charWidth ? (areaWidth - 17) / charWidth : 1;
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
			}
			// Fill the end of the line with white color (17 is the length of a scrollbar)
			charRect.right = areaWidth - 17;
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

HPEN currPen = 0;
int	ibarState = 0;

VOID CALLBACK AreaCursorUpdate(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	(void)dwTime;
	(void)uMsg;
	if(idEvent != 0)
		return;
	currPen = ibarState % 16 < 8 ? areaPenBlack1px : areaPenWhite1px;
	InvalidateRect(areaWnd, NULL, false);
	HDC hdc = BeginPaint(areaWnd, &areaPS);

	SelectPen(hdc, GetFocus() == hwnd ? currPen : areaPenWhite1px);
	currPen = currPen == areaPenWhite1px ? areaPenBlack1px : areaPenWhite1px;

	MoveToEx(hdc, padLeft + (cursorCharX - shiftCharX) * charWidth, (cursorCharY - shiftCharY) * charHeight, NULL);
	LineTo(hdc, padLeft + (cursorCharX - shiftCharX) * charWidth, (cursorCharY - shiftCharY) * charHeight + charHeight);
	EndPaint(areaWnd, &areaPS);

	ibarState++;
}

void InputChar(char ch)
{
	if(currLine->length == currLine->maxLength)
	{
		AreaChar	*newData = new AreaChar[currLine->maxLength + (currLine->maxLength >> 1)];
		memcpy(newData, currLine->data, currLine->maxLength * sizeof(AreaChar));
		if(currLine->data != currLine->startBuf)
			delete[] currLine->data;
		currLine->data = newData;
		currLine->maxLength += currLine->maxLength >> 1;
	}
	if(cursorCharX != currLine->length)
		memmove(&currLine->data[cursorCharX+1], &currLine->data[cursorCharX], (currLine->length - cursorCharX) * sizeof(AreaChar));
	currLine->data[cursorCharX].ch = ch;
	currLine->length++;
	cursorCharX++;
	InvalidateRect(areaWnd, NULL, false);
}

void InputEnter()
{
	lineCount++;
	AreaLine *nLine = new AreaLine;
	memset(nLine, 0, sizeof(AreaLine));
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

	currLine->length = 0;
	currLine->maxLength = 32;
	currLine->data = currLine->startBuf;

	currLine->data[0].style = currLine->prev->length ? currLine->prev->data[currLine->prev->length-1].style : 0;

	if(cursorCharX != currLine->prev->length)
	{
		unsigned int diff = currLine->prev->length - cursorCharX;
		if(diff >= currLine->maxLength)
		{
			currLine->data = new AreaChar[diff + diff / 2];
			memset(currLine->data, 0, sizeof(AreaChar) * diff + diff / 2);
			currLine->maxLength = diff + diff / 2;
		}
		memcpy(currLine->data, &currLine->prev->data[cursorCharX], diff * sizeof(AreaChar));
		currLine->prev->length = cursorCharX;
		currLine->length = diff;
	}
	cursorCharY++;
	cursorCharX = 0;
	InvalidateRect(areaWnd, NULL, true);
}

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

void	CheckCursorBounds()
{
	if(charHeight && int(cursorCharY) > (areaHeight-32) / charHeight + shiftCharY)
	{
		shiftCharY = cursorCharY - (areaHeight-32) / charHeight;
		ClampShift();
		InvalidateRect(areaWnd, NULL, true);
	}
	if(int(cursorCharY) - shiftCharY < 0)
	{
		shiftCharY = cursorCharY - 2;
		ClampShift();
		InvalidateRect(areaWnd, NULL, true);
	}
	UpdateScrollBar();
}

void DeleteSelection()
{
	if(!selectionOn)
		return;

	unsigned int startX, startY, endX, endY;
	SortSelPoints(startX, endX, startY, endY);

	startY = startY > lineCount ? lineCount - 1 : startY;
	endY = endY > lineCount ? lineCount-1 : endY;

	if(startY == endY)
	{
		AreaLine *curr = firstLine;
		for(unsigned int i = 0; i < startY; i++)
			curr = curr->next;
		endX = endX > curr->length ? curr->length : endX;
		startX = startX > curr->length ? curr->length : startX;
		memmove(&curr->data[startX], &curr->data[endX], (curr->length - endX) * sizeof(AreaChar));
		curr->length -= endX - startX;
		selectionOn = false;
		cursorCharX = startX;
	}else{
		AreaLine *first = firstLine;
		for(unsigned int i = 0; i < startY; i++)
			first = first->next;
		AreaLine *last = first;
		for(unsigned int i = startY; i < endY; i++)
			last = last->next;

		first->length = startX > first->length ? first->length : startX;

		endX = endX > last->length ? last->length : endX;

		memmove(&last->data[0], &last->data[endX], (last->length - endX) * sizeof(AreaChar));
		last->length -= endX;
		if(last->length < 0)
			last->length = 0;

		unsigned int sum = first->length + last->length;
		if(sum >= first->maxLength)
		{
			AreaChar *nChars = new AreaChar[sum + sum / 2];
			first->maxLength = sum + sum / 2;
			memset(nChars, 0, sizeof(AreaChar) * first->maxLength);
			memcpy(nChars, first->data, first->length * sizeof(AreaChar));
			first->data = nChars;
		}
		memcpy(&first->data[first->length], last->data, last->length * sizeof(AreaChar));
		first->length = sum;

		for(unsigned int i = startY; i < endY; i++)
		{
			AreaLine *oldLine = first->next;
			if(first->next->next)
				first->next->next->prev = first;
			first->next = first->next->next;
			delete oldLine;

			lineCount--;
		}
		selectionOn = false;
		cursorCharX = startX;
		cursorCharY = startY;
		currLine = first;
	}
	UpdateScrollBar();
	ClampShift();
	InvalidateRect(areaWnd, NULL, false);
}

LRESULT CALLBACK TextareaProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;

	if(!areaWnd)
		areaWnd = hWnd;
	switch(message)
	{
	case WM_CREATE:
		hdc = BeginPaint(areaWnd, &areaPS);
		areaFont[FONT_REGULAR] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");
		areaFont[FONT_BOLD] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_BOLD, false, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");
		areaFont[FONT_ITALIC] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, true, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");
		areaFont[FONT_UNDERLINED] = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, true, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Consolas");

		areaPenWhite1px = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
		areaPenBlack1px = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

		areaBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
		areaBrushBlack = CreateSolidBrush(RGB(0, 0, 0));

		EndPaint(areaWnd, &areaPS);
		
		areaTextSize = 16 * 1024;
		areaText = new char[areaTextSize];
		areaTextEx = new char[areaTextSize];

		firstLine = new AreaLine;
		memset(firstLine, 0, sizeof(AreaLine));
		firstLine->data = firstLine->startBuf;
		firstLine->next = firstLine->prev = NULL;
		firstLine->length = 0;
		firstLine->maxLength = 32;
		
		currLine = firstLine;
		currLine->data[0].style = 0;

		lineCount = 1;

		SetTimer(areaWnd, 0, 62, AreaCursorUpdate);
		areaCreated = true;
		break;
	case WM_PAINT:
		ReDraw();
		break;
	case WM_SIZE:
		areaWidth = LOWORD(lParam);
		areaHeight = HIWORD(lParam);

		UpdateScrollBar();

		InvalidateRect(areaWnd, NULL, true);
		break;
	case WM_MOUSEACTIVATE:
		SetFocus(areaWnd);
		EnableWindow(areaWnd, true);
		break;
	case WM_CHAR:
		if((wParam & 0xFF) >= 0x20)
		{
			if(selectionOn)
				DeleteSelection();
			InputChar((char)(wParam & 0xFF));
		}else if((wParam & 0xFF) == '\r'){
			if(selectionOn)
				DeleteSelection();
			InputEnter();
		}else if((wParam & 0xFF) == '\b'){
			if(selectionOn)
			{
				DeleteSelection();
			}else{
				if(cursorCharX)
				{
					if(cursorCharX != currLine->length)
						memmove(&currLine->data[cursorCharX-1], &currLine->data[cursorCharX], (currLine->length - cursorCharX) * sizeof(AreaChar));
					currLine->length--;
					cursorCharX--;
					RECT invalid = { 0, (cursorCharY - shiftCharY) * charHeight, areaWidth, (cursorCharY - shiftCharY + 1) * charHeight };
					InvalidateRect(areaWnd, &invalid, false);
				}else{
					if(currLine->prev)
					{
						currLine = currLine->prev;
						unsigned int sum = currLine->length + currLine->next->length;
						if(sum >= currLine->maxLength)
						{
							AreaChar *nChars = new AreaChar[sum + sum / 2];
							currLine->maxLength = sum + sum / 2;
							memset(nChars, 0, sizeof(AreaChar) * currLine->maxLength);
							memcpy(nChars, currLine->data, currLine->length * sizeof(AreaChar));
							currLine->data = nChars;
						}
						memcpy(&currLine->data[currLine->length], currLine->next->data, currLine->next->length * sizeof(AreaChar));
						currLine->length = sum;

						AreaLine *oldLine = currLine->next;
						if(currLine->next->next)
							currLine->next->next->prev = currLine;
						currLine->next = currLine->next->next;
						delete oldLine;

						lineCount--;

						cursorCharX = sum;
						cursorCharY--;

						InvalidateRect(areaWnd, NULL, true);
					}
				}
			}
		}else if((wParam & 0xFF) == 22){
			if(selectionOn)
				DeleteSelection();
			OpenClipboard(areaWnd);
			HANDLE clipData = GetClipboardData(CF_TEXT);
			if(clipData)
			{
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
			InvalidateRect(areaWnd, NULL, false);
		}else if((wParam & 0xFF) == 1){
			selectionOn = true;
			dragStartX = 0;
			dragStartY = 0;
			AreaLine *curr = firstLine;
			int line = 0;
			while(curr->next)
				curr = curr->next, line++;
			dragEndX = curr->length;
			dragEndY = line;
			InvalidateRect(areaWnd, NULL, false);
		}else if(selectionOn && ((wParam & 0xFF) == 3 || (wParam & 0xFF) == 24)){
			const char *start = GetAreaText();
			AreaLine *curr = firstLine;

			unsigned int startX, startY, endX, endY;
			SortSelPoints(startX, endX, startY, endY);

			for(unsigned int i = 0; i < startY; i++)
			{
				start += curr->length + 2;
				curr = curr->next;
			}
			const char *end = start;
			start += (startX < curr->length ? startX : curr->length);
			for(unsigned int i = startY; i < endY; i++)
			{
				end += curr->length + 2;
				curr = curr->next;
			}
			end += (endX < curr->length ? endX : curr->length);

			OpenClipboard(areaWnd);
			EmptyClipboard();
			HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, end - start + 1);
			char *pchData = (char*)GlobalLock(hClipboardData);
			memcpy(pchData, start, end - start);
			GlobalUnlock(hClipboardData);
			SetClipboardData(CF_TEXT, hClipboardData);
			CloseClipboard();
			if((wParam & 0xFF) == 24)
				DeleteSelection();
		}
		CheckCursorBounds();
		needUpdate = true;
		break;
	case WM_KEYDOWN:
		AreaCursorUpdate(NULL, 0, NULL, 0);
		if(wParam == VK_DOWN)
		{
			if(currLine->next)
			{
				currLine = currLine->next;
				cursorCharY++;
				if(cursorCharX > currLine->length)
					cursorCharX = currLine->length;
			}
		}else if(wParam == VK_UP){
			if(currLine->prev)
			{
				currLine = currLine->prev;
				cursorCharY--;
				if(cursorCharX > currLine->length)
					cursorCharX = currLine->length;
			}
		}else if(wParam == VK_LEFT){
			if(cursorCharX > 0)
			{
				cursorCharX--;
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
			}else{
				if(currLine->next)
				{
					currLine = currLine->next;
					cursorCharY++;
					if(cursorCharX > currLine->length)
						cursorCharX = currLine->length;
				}
			}
		}else if(wParam == VK_DELETE){
			if(selectionOn)
			{
				DeleteSelection();
				return 0;
			}else{
				if(cursorCharX != currLine->length)
				{
					memmove(&currLine->data[cursorCharX], &currLine->data[cursorCharX+1], (currLine->length - cursorCharX) * sizeof(AreaChar));
					currLine->length--;
					RECT invalid = { 0, (cursorCharY - shiftCharY) * charHeight, areaWidth, (cursorCharY - shiftCharY + 1) * charHeight };
					InvalidateRect(areaWnd, &invalid, false);
					return 0;
				}else if(currLine->next){
					unsigned int sum = currLine->length + currLine->next->length;
					if(sum >= currLine->maxLength)
					{
						AreaChar *nChars = new AreaChar[sum + sum / 2];
						currLine->maxLength = sum + sum / 2;
						memset(nChars, 0, sizeof(AreaChar) * currLine->maxLength);
						memcpy(nChars, currLine->data, currLine->length * sizeof(AreaChar));
						currLine->data = nChars;
					}
					memcpy(&currLine->data[currLine->length], currLine->next->data, currLine->next->length * sizeof(AreaChar));
					currLine->length = sum;

					AreaLine *oldLine = currLine->next;
					if(currLine->next->next)
						currLine->next->next->prev = currLine;
					currLine->next = currLine->next->next;
					delete oldLine;

					lineCount--;
					InvalidateRect(areaWnd, NULL, true);
					return 0;
				}
			}
		}
		ibarState = 0;
		AreaCursorUpdate(areaWnd, 0, NULL, 0);
		CheckCursorBounds();
		
		break;
	case WM_LBUTTONDBLCLK:
		dragEndY = dragStartY = HIWORD(lParam) / charHeight + shiftCharY;
		if(dragStartY > (int)lineCount)
			break;
		dragStartX = 1;
		dragEndX = 5;

		{
			AreaLine *curr = firstLine;
			for(int i = 0; i < dragStartY; i++)
				curr = curr->next;

			dragStartX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
			if(dragStartX > (int)curr->length && curr->length != 0)
				dragStartX = curr->length - 1;
			dragEndX = dragStartX;

			char symb = curr->data[dragStartX].ch;
			if(isdigit(symb))
			{
				while(dragStartX != 0 && (isdigit(curr->data[dragStartX - 1].ch) || curr->data[dragStartX - 1].ch == '.'))
					dragStartX--;
				while(dragEndX < (int)curr->length && (isdigit(curr->data[dragEndX + 1].ch) || curr->data[dragEndX + 1].ch == '.'))
					dragEndX++;
				dragEndX++;
			}else if(isalnum(symb) || symb == '_')
			{
				while(dragStartX != 0 && (isalnum(curr->data[dragStartX - 1].ch) || curr->data[dragStartX - 1].ch == '_'))
					dragStartX--;
				while(dragEndX < (int)curr->length && (isalnum(curr->data[dragEndX + 1].ch) || curr->data[dragEndX + 1].ch == '_'))
					dragEndX++;
				dragEndX++;
			}else{
				dragEndX++;
			}

			selectionOn = curr->length != 0;
		}
		InvalidateRect(areaWnd, NULL, false);
		break;
	case WM_LBUTTONDOWN:
		dragStartX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
		dragStartY = HIWORD(lParam) / charHeight + shiftCharY;
		selectionOn = false;
		InvalidateRect(areaWnd, NULL, false);
		break;
	case WM_MOUSEMOVE:
		if(wParam != MK_LBUTTON)
			break;
		dragEndX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
		dragEndY = HIWORD(lParam) / charHeight + shiftCharY;
		if(dragStartX != dragEndX || dragStartY != dragEndY)
			selectionOn = true;
	case WM_LBUTTONUP:
		AreaCursorUpdate(NULL, 0, NULL, 0);
		if(message == WM_MOUSEMOVE)
			InvalidateRect(areaWnd, NULL, false);
		cursorCharX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth + shiftCharX;
		cursorCharY = HIWORD(lParam) / charHeight + shiftCharY;

		if(cursorCharY >= lineCount)
			cursorCharY = lineCount - 1;
		currLine = firstLine;
		for(unsigned int i = 0; i < cursorCharY; i++)
			currLine = currLine->next;
		if(cursorCharX > currLine->length)
			cursorCharX = currLine->length;
		ibarState = 0;
		break;
	case WM_MOUSEWHEEL:
		shiftCharY -= (GET_WHEEL_DELTA_WPARAM(wParam) / 120) * 3;
		ClampShift();
		InvalidateRect(areaWnd, NULL, false);

		UpdateScrollBar();
		break;
	case WM_VSCROLL:
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
