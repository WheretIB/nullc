#include "TabbedFiles.h"

#include <windowsx.h>

#include <algorithm>

#pragma warning(disable: 4996)

#ifdef _WIN64
	#define WND_PTR_TYPE LONG_PTR
#else
	#define WND_PTR_TYPE LONG
#endif

const unsigned int TAB_HEIGHT = 19;
const unsigned int TAB_MAX_COUNT = 64;

struct TabbedFilesData
{
	unsigned int	width;

	unsigned int	selectedTab;
	bool			leftMouseDown;

	unsigned int	tabCount;
	TabbedFiles::TabInfo	tabInfo[TAB_MAX_COUNT + 1];

	void	(*closeHandler)(TabbedFiles::TabInfo &tab);

	HFONT	mainFont, miniFont;
	HPEN	brWhite, brGray, brRed, brGradient[15];

	HWND	newTab, closeButton;
};

namespace TabbedFiles
{
	TabbedFilesData*	GetData(HWND wnd)
	{
		return (TabbedFilesData*)(intptr_t)GetWindowLongPtr(wnd, 0);
	}

	unsigned int		CursorToIndex(HWND wnd, unsigned int x)
	{
		TabbedFilesData	*data = GetData(wnd);

		// Remove left padding
		x -= 15;

		unsigned int retValue = ~0u;
		for(unsigned int i = 0; i < data->tabCount + 1; i++)
		{
			if(x < data->tabInfo[i].width)
			{
				retValue = i;
				break;
			}
			x -= data->tabInfo[i].width;
		}
		return retValue;
	}

	void OnCreate(HWND wnd)
	{
		SetWindowLongPtr(wnd, 0, (WND_PTR_TYPE)(intptr_t)new TabbedFilesData);

		TabbedFilesData	*data = GetData(wnd);
		memset(data, 0, sizeof(TabbedFilesData));

		data->selectedTab = 0;

		PAINTSTRUCT paintData;
		HDC hdc = BeginPaint(wnd, &paintData);

		data->mainFont = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
		data->miniFont = CreateFont(-8 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");

		data->brWhite =	CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
		data->brGray =	CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
		data->brRed =	CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

		for(int i = 0; i < 15; i++)
			data->brGradient[i] = CreatePen(PS_SOLID, 1, RGB(186 + 4*i, 186 + 4*i, 186 + 4*i));

		EndPaint(wnd, &paintData);

		data->closeButton = CreateWindow("BUTTON", "X", WS_VISIBLE | WS_CHILD | BS_FLAT, 200, 0, 10, 10, wnd, NULL, NULL, 0);

		data->newTab = NULL;
		data->closeHandler = NULL;
	}

	void OnDestroy(HWND wnd)
	{
		delete (TabbedFilesData*)(intptr_t)GetWindowLongPtr(wnd, 0);
	}

	void OnPaint(HWND wnd)
	{
		TabbedFilesData	*data = GetData(wnd);

		if(!data->closeHandler)
			ShowWindow(data->closeButton, SW_HIDE);

		PAINTSTRUCT paintData;
		HDC hdc = BeginPaint(wnd, &paintData);

		SelectFont(hdc, data->mainFont);

		unsigned int leftPos = 5;

		SelectPen(hdc, data->brGray);
		MoveToEx(hdc, 0, TAB_HEIGHT, NULL);
		LineTo(hdc, data->width, TAB_HEIGHT);

		for(unsigned int i = 0; i < data->tabCount + (data->newTab ? 1 : 0); i++)
		{
			RECT textRect = { 0, 0, 0, 0 };
			if(data->tabInfo[i].last)
				DrawText(hdc, data->tabInfo[i].last, (int)strlen(data->tabInfo[i].last), &textRect, DT_CALCRECT);

			// Graphics
			if(i == data->selectedTab)
			{
				SelectPen(hdc, data->brWhite);
				MoveToEx(hdc, leftPos + 2, TAB_HEIGHT - 1, NULL);
				LineTo(hdc, leftPos + 14, TAB_HEIGHT - 1);

				SelectPen(hdc, data->brWhite);
				for(int k = -1; k < 15; k++)
				{
					MoveToEx(hdc, leftPos + 2 + k, TAB_HEIGHT - 1 - k, NULL);
					LineTo(hdc, leftPos + 21 + textRect.right, TAB_HEIGHT - 1 - k);
				}
			}else{
				for(int k = 0; k < 15; k++)
				{
					SelectPen(hdc, data->brGradient[k]);
					MoveToEx(hdc, i == 0 ? leftPos + 3 + k : max(leftPos + 3 + k, leftPos + 1 + 10), TAB_HEIGHT - 1 - k, NULL);
					LineTo(hdc, leftPos + 21 + textRect.right, TAB_HEIGHT - 1 - k);
				}

				SelectPen(hdc, data->brWhite);
				MoveToEx(hdc, leftPos + 11, TAB_HEIGHT - 10, NULL);
				LineTo(hdc, leftPos + 14, TAB_HEIGHT - 13);
			}

			SelectPen(hdc, data->brGray);
			if(i == data->selectedTab || i == 0)
				MoveToEx(hdc, leftPos, TAB_HEIGHT, NULL);
			else
				MoveToEx(hdc, leftPos + 10, TAB_HEIGHT - 10, NULL);
			LineTo(hdc, leftPos + 13, TAB_HEIGHT - 13);
			LineTo(hdc, leftPos + 19, TAB_HEIGHT - 16);
			LineTo(hdc, leftPos + 19 + textRect.right, TAB_HEIGHT - 16);
			LineTo(hdc, leftPos + 21 + textRect.right, TAB_HEIGHT - 14);
			LineTo(hdc, leftPos + 21 + textRect.right, TAB_HEIGHT);
			LineTo(hdc, leftPos + 22 + textRect.right, TAB_HEIGHT);
			LineTo(hdc, leftPos + 22 + textRect.right, TAB_HEIGHT - 14);
			LineTo(hdc, leftPos + 19 + textRect.right, TAB_HEIGHT - 17);

			// Text
			RECT textRectMod = { textRect.left + int(leftPos) + 16, 3, textRect.right + int(leftPos), TAB_HEIGHT };
			SetBkMode(hdc, TRANSPARENT);
			if(data->tabInfo[i].last)
				ExtTextOut(hdc, textRectMod.left, textRectMod.top, 0, &textRectMod, data->tabInfo[i].last, (int)strlen(data->tabInfo[i].last), NULL);

			if(data->tabInfo[i].dirty)
			{
				SelectFont(hdc, data->mainFont);
				textRectMod.left += textRect.right - textRect.left;
				ExtTextOut(hdc, textRectMod.left, textRectMod.top, 0, &textRectMod, "*", 1, NULL);
				SelectFont(hdc, data->mainFont);
			}
			SetBkMode(hdc, OPAQUE);

			// Advance
			leftPos += textRect.right - textRect.left + 12;

			data->tabInfo[i].width = textRect.right - textRect.left + 12;
		}

		EndPaint(wnd, &paintData);
	}

	void OnSize(HWND wnd, unsigned int width, unsigned int height)
	{
		TabbedFilesData	*data = GetData(wnd);
		data->width = width;
		SetWindowPos(data->closeButton, NULL, width - 15, 0, 15, 15, NULL);
		(void)height;
	}

	void OnMouseLeft(HWND wnd, bool down, unsigned int x, unsigned int y)
	{
		TabbedFilesData	*data = GetData(wnd);
		if(down)
		{
			unsigned int newTab = CursorToIndex(wnd, x);
			if(newTab != -1)
			{
				data->leftMouseDown = true;
				SetCapture(wnd);
				data->selectedTab = newTab;
				InvalidateRect(wnd, NULL, false);
			}
		}else{
			data->leftMouseDown = false;
			ReleaseCapture();
			for(unsigned int i = 0; i < data->tabCount + 1; i++)
				ShowWindow(data->tabInfo[i].window, SW_HIDE);
			ShowWindow(data->tabInfo[data->selectedTab].window, SW_SHOW);
			InvalidateRect(wnd, NULL, true);
		}
		(void)y;
	}

	void OnMouseMove(HWND wnd, unsigned int x, unsigned int y)
	{
		TabbedFilesData	*data = GetData(wnd);
		if(data->leftMouseDown)
		{
			unsigned int newTab = CursorToIndex(wnd, x);
			if(data->selectedTab == data->tabCount || newTab == data->tabCount)
				return;
			if(newTab != -1 && newTab != data->selectedTab)
			{
				std::swap(data->tabInfo[data->selectedTab].width, data->tabInfo[newTab].width);
				if(CursorToIndex(wnd, x) != newTab)
					return;
				std::swap(data->tabInfo[data->selectedTab].width, data->tabInfo[newTab].width);
				std::swap(data->tabInfo[data->selectedTab], data->tabInfo[newTab]);
				data->selectedTab = newTab;
				InvalidateRect(wnd, NULL, true);
			}
		}
		(void)y;
	}

	void OnCommand(HWND wnd, HWND child)
	{
		TabbedFilesData	*data = GetData(wnd);
		if(child == data->closeButton && data->selectedTab != data->tabCount)
		{
			if(data->closeHandler)
				data->closeHandler(data->tabInfo[data->selectedTab]);
			ShowWindow(data->tabInfo[data->selectedTab].window, SW_HIDE);
			if(data->tabCount < 2)
			{
				data->tabCount = 0;
				data->selectedTab = 0;
				data->tabInfo[0].name = data->tabInfo[0].last = "+";
				data->tabInfo[0].window = data->newTab;
				data->tabInfo[0].dirty = false;
			}else{
				for(unsigned int i = data->selectedTab; i < data->tabCount + 1; i++)
					data->tabInfo[i] = data->tabInfo[i+1];
				data->tabCount--;
				data->selectedTab = data->selectedTab ? data->selectedTab - 1 : 0;
			}
			ShowWindow(data->tabInfo[data->selectedTab].window, SW_SHOW);
			InvalidateRect(wnd, NULL, true);
			SetFocus(wnd);
		}
	}
};

void TabbedFiles::AddTab(HWND wnd, const char* filename, HWND childWindow)
{
	TabbedFilesData	*data = GetData(wnd);
	if(data->tabCount >= TAB_MAX_COUNT)
	{
		MessageBox(wnd, "Maximum limit of opened tabs exceeded", "ERROR", MB_OK);
		return;
	}
	data->tabInfo[data->tabCount].name = strdup(filename);
	const char *filePart = strrchr(data->tabInfo[data->tabCount].name, '\\');
	data->tabInfo[data->tabCount].last = filePart ? filePart + 1 : data->tabInfo[data->tabCount].name;
	data->tabInfo[data->tabCount].window = childWindow;
	data->tabInfo[data->tabCount].dirty = false;
	data->tabCount++;

	data->tabInfo[data->tabCount].name = data->tabInfo[data->tabCount].last = "+";
	data->tabInfo[data->tabCount].window = data->newTab;
	InvalidateRect(wnd, NULL, true);
}

void TabbedFiles::SetNewTabWindow(HWND wnd, HWND newTab)
{
	TabbedFilesData	*data = GetData(wnd);

	data->newTab = newTab;
	data->tabInfo[data->tabCount].name = data->tabInfo[data->tabCount].last = "+";
	data->tabInfo[data->tabCount].window = newTab;
}

void TabbedFiles::RemoveTab(HWND wnd, unsigned int tab)
{
	TabbedFilesData	*data = GetData(wnd);

	data->selectedTab = tab;
	OnCommand(wnd, data->closeButton);
}

unsigned int TabbedFiles::GetCurrentTab(HWND wnd)
{
	TabbedFilesData	*data = GetData(wnd);

	return data->selectedTab;
}

void TabbedFiles::SetCurrentTab(HWND wnd, unsigned int id)
{
	TabbedFilesData	*data = GetData(wnd);
	if(data->selectedTab == id)
		return;

	data->selectedTab = id;
	for(unsigned int i = 0; i < data->tabCount + 1; i++)
		ShowWindow(data->tabInfo[i].window, SW_HIDE);
	ShowWindow(data->tabInfo[data->selectedTab].window, SW_SHOW);
	InvalidateRect(wnd, NULL, true);
}

TabbedFiles::TabInfo& TabbedFiles::GetTabInfo(HWND wnd, unsigned int id)
{
	TabbedFilesData	*data = GetData(wnd);

	return data->tabInfo[id];
}

void TabbedFiles::SetOnCloseTab(HWND wnd, void (*OnClose)(TabInfo &tab))
{
	TabbedFilesData	*data = GetData(wnd);

	data->closeHandler = OnClose;
}

void TabbedFiles::RegisterTabbedFiles(const char *className, HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize			= sizeof(WNDCLASSEX); 
	wcex.style			= CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)TabbedFilesProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= sizeof(TabbedFilesData*);
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= className;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);
}

LRESULT CALLBACK TabbedFiles::TabbedFilesProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	__try
	{
		switch(message)
		{
		case WM_CREATE:
			OnCreate(hWnd);
			break;
		case WM_DESTROY:
			OnDestroy(hWnd);
			break;
		case WM_PAINT:
			OnPaint(hWnd);
			break;
		case WM_SIZE:
			OnSize(hWnd, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_LBUTTONDOWN:
			OnMouseLeft(hWnd, true, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_LBUTTONUP:
			OnMouseLeft(hWnd, false, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_MOUSEMOVE:
			OnMouseMove(hWnd, LOWORD(lParam), HIWORD(lParam));
			break;
		case WM_COMMAND:
			OnCommand(hWnd, (HWND)lParam);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}__except(EXCEPTION_EXECUTE_HANDLER){
		assert(!"Exception in window procedure handler");
	}
	return 0;
}
