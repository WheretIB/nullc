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
TextStyle	tStyle[16];

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

char	*areaText = NULL;
AreaChar	*areaTextEx = NULL;

unsigned int areaTextSize = 0;

bool	areaCreated = false;
HFONT	areaFont[4];
HWND	areaWnd = 0;

PAINTSTRUCT areaPS;
int		areaWidth, areaHeight;
unsigned int charWidth, charHeight;
unsigned int cursorCharX = 0, cursorCharY = 0;
int shiftCharX = 0, shiftCharY = 0;
unsigned int padLeft = 5;
unsigned int overallLength = 0;

unsigned int lineCount = 0;

bool needUpdate = false;

HPEN	areaPenWhite1px, areaPenBlack1px;
HBRUSH	areaBrushWhite;

AreaLine	*firstLine = NULL;
AreaLine	*currLine = NULL;

LRESULT CALLBACK TextareaProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam);

bool SetTextStyle(unsigned int id, char red, char green, char blue, bool bold, bool italics, bool underline)
{
	if(id >= 16)
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

void ExtendSolidTextBuf(unsigned int size)
{
	if(size >= areaTextSize)
	{
		areaTextSize = size + size / 2;
		delete[] areaText;
		delete[] areaTextEx;
		areaText = new char[areaTextSize];
		areaTextEx = new AreaChar[areaTextSize];
	}
}

void BeginStyleUpdate()
{
	overallLength = 0;
	AreaLine *curr = firstLine;
	while(curr)
	{
		overallLength += curr->length + 2;	// additional 2 for \r\n
		curr = curr->next;
	}

	ExtendSolidTextBuf(overallLength);
}

void SetStyleToSelection(unsigned int start, unsigned int end, int style)
{
	for(int i = start; i <= end; i++)
		areaTextEx[i].style = (char)style;
}

void EndStyleUpdate()
{
	AreaLine *curr = firstLine;
	for(int i = 0, n = 0; i < overallLength; i++, n++)
	{
		if(n < curr->length)
		{
			curr->data[n].style = areaTextEx[i].style;
		}else{
			i += 1;
			n = -1;
			curr = curr->next;
		}
	}
}

void RegisterTextarea(const char *className, HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize			= sizeof(WNDCLASSEX); 
	wcex.style			= 0;
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

const char* GetAreaText()
{
	AreaLine *curr = NULL;
	// Calculate length of all text
	overallLength = 0;
	curr = firstLine;
	while(curr)
	{
		overallLength += curr->length + 2;	// additional 2 for \r\n
		curr = curr->next;
	}

	ExtendSolidTextBuf(overallLength);

	char	*currChar = areaText;
	curr = firstLine;
	while(curr)
	{
		for(unsigned int i = 0; i < curr->length; i++, currChar++)
			*currChar = curr->data[i].ch;
		*currChar++ = '\r';
		*currChar++ = '\n';
		curr = curr->next;
	}
	*currChar = 0;
	return areaText;
};

void SetAreaText(const char *text)
{
	while(*text)
	{
		if(*text >= 0x20)
			InputChar(*text);
		else if(*text == '\r')
			InputEnter();
		*text++;
	}
	needUpdate = true;
}

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

void ReDraw()
{
	if(!areaCreated)
		return;

	RECT updateRect;
	if(!GetUpdateRect(areaWnd, &updateRect, false))
		return;

	AreaLine *startLine = firstLine;
	for(int i = 0; i < shiftCharY; i++)
		startLine = startLine->next;

	AreaLine *curr = NULL;
	// Calculate length of all text
	overallLength = 0;
	curr = startLine;
	while(curr)
	{
		overallLength += curr->length;
		curr = curr->next;
	}

	ExtendSolidTextBuf(overallLength);

	char	*currChar = areaText;
	curr = startLine;
	while(curr)
	{
		for(unsigned int i = 0; i < curr->length; i++, currChar++)
			*currChar = curr->data[i].ch;
		curr = curr->next;
	}

	HDC hdc = BeginPaint(areaWnd, &areaPS);
	FontStyle currFont = FONT_REGULAR;
	SelectFont(hdc, areaFont[currFont]);

	RECT charRect = { 0, 0, 0, 0 };
	DrawText(hdc, "W", 1, &charRect, DT_CALCRECT);
	charWidth = charRect.right;
	charHeight = charRect.bottom;

	charRect.left = padLeft;
	charRect.right = charRect.left + charWidth;

	currChar = areaText;
	curr = startLine;
	while(curr && charRect.top < updateRect.bottom)
	{
		if(charRect.bottom > updateRect.top)
		{
			for(unsigned int i = 0; i < curr->length; i++, currChar++)
			{
				TextStyle &style = tStyle[curr->data[i].style];
				if(style.font != currFont)
				{
					currFont = (FontStyle)style.font;
					SelectFont(hdc, areaFont[currFont]);
				}
				SetTextColor(hdc, RGB(style.color[0], style.color[1], style.color[2]));
				DrawText(hdc, currChar, 1, &charRect, 0);
				charRect.left += charWidth;
				charRect.right += charWidth;
			}
			charRect.right = areaWidth - 17;
			FillRect(hdc, &charRect, areaBrushWhite);
		}else{
			currChar += curr->length;
		}
		charRect.left = padLeft;
		charRect.right = charRect.left + charWidth;
		charRect.top += charHeight;
		charRect.bottom += charHeight;
		curr = curr->next;
	}
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
VOID CALLBACK AreaCursorUpdate(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	(void)dwTime;
	(void)uMsg;
	if(idEvent != 0)
		return;
	if(!currPen)
		currPen = areaPenBlack1px;
	InvalidateRect(areaWnd, NULL, false);
	HDC hdc = BeginPaint(areaWnd, &areaPS);

	SelectPen(hdc, GetFocus() == hwnd ? currPen : areaPenWhite1px);
	currPen = currPen == areaPenWhite1px ? areaPenBlack1px : areaPenWhite1px;

	MoveToEx(hdc, padLeft + cursorCharX * charWidth, (cursorCharY - shiftCharY) * charHeight, NULL);
	LineTo(hdc, padLeft + cursorCharX * charWidth, (cursorCharY - shiftCharY) * charHeight + charHeight);
	EndPaint(areaWnd, &areaPS);
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

void	UpdateScrollBar()
{
	SCROLLINFO sbInfo;
	sbInfo.cbSize = sizeof(SCROLLINFO);
	sbInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	sbInfo.nMin = 0;
	sbInfo.nMax = lineCount;
	sbInfo.nPage = charHeight ? areaHeight / charHeight : 1;
	sbInfo.nPos = shiftCharY;
	SetScrollInfo(areaWnd, SB_VERT, &sbInfo, true);
}

void	ClampShift()
{
	if(shiftCharY < 0)
		shiftCharY = 0;
	if(shiftCharY >= int(lineCount) - 1)
		shiftCharY = lineCount - 2;
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

		EndPaint(areaWnd, &areaPS);
		
		areaTextSize = 16 * 1024;
		areaText = new char[areaTextSize];
		areaTextEx = new AreaChar[areaTextSize];

		firstLine = new AreaLine;
		memset(firstLine, 0, sizeof(AreaLine));
		firstLine->data = firstLine->startBuf;
		firstLine->next = firstLine->prev = NULL;
		firstLine->length = 0;
		firstLine->maxLength = 32;
		
		currLine = firstLine;
		currLine->data[0].style = 0;

		lineCount = 1;

		SetTimer(areaWnd, 0, 500, AreaCursorUpdate);
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
			InputChar((char)(wParam & 0xFF));
		}else if((wParam & 0xFF) == '\r'){
			InputEnter();
		}else if((wParam & 0xFF) == '\b'){
			if(cursorCharX)
			{
				if(cursorCharX != currLine->length)
					memmove(&currLine->data[cursorCharX-1], &currLine->data[cursorCharX], (currLine->length - cursorCharX) * sizeof(AreaChar));
				currLine->length--;
				cursorCharX--;
				RECT invalid = { cursorCharX * charWidth, (cursorCharY - shiftCharY) * charHeight, areaWidth, (cursorCharY - shiftCharY + 1) * charHeight };
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
		}else if((wParam & 0xFF) == 22){
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
			if(cursorCharX != currLine->length)
			{
				memmove(&currLine->data[cursorCharX], &currLine->data[cursorCharX+1], (currLine->length - cursorCharX) * sizeof(AreaChar));
				currLine->length--;
				RECT invalid = { cursorCharX * charWidth, (cursorCharY - shiftCharY) * charHeight, areaWidth, (cursorCharY - shiftCharY + 1) * charHeight };
				InvalidateRect(areaWnd, &invalid, false);
				return 0;
			}else{
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
		AreaCursorUpdate(areaWnd, 0, NULL, 0);
		CheckCursorBounds();
		
		break;
	case WM_LBUTTONDOWN:
		AreaCursorUpdate(NULL, 0, NULL, 0);
		cursorCharX = (LOWORD(lParam) - padLeft + charWidth / 2) / charWidth;
		cursorCharY = HIWORD(lParam) / charHeight + shiftCharY;

		if(cursorCharY >= lineCount)
			cursorCharY = lineCount - 1;
		currLine = firstLine;
		for(unsigned int i = 0; i < cursorCharY; i++)
			currLine = currLine->next;
		if(cursorCharX > currLine->length)
			cursorCharX = currLine->length;
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
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
