#include "TabbedFiles.h"

#include <windowsx.h>

#include <algorithm>

#ifdef _WIN64
	#define WND_PTR_TYPE LONG_PTR
#else
	#define WND_PTR_TYPE LONG
#endif

const unsigned int TAB_HEIGHT = 19;
const unsigned int TAB_MAX_COUNT = 32;

struct TabbedFilesData
{
	unsigned int	width;

	unsigned int	selectedTab;
	bool			leftMouseDown;

	unsigned int	tabCount;
	const char		*tabNames[TAB_MAX_COUNT];
	unsigned int	tabWidths[TAB_MAX_COUNT];

	HFONT			mainFont;
	HPEN			brWhite, brGray, brLightGray, brRed, brGradient[15];
};

namespace TabbedFiles
{
	TabbedFilesData*	GetData(HWND wnd)
	{
		return (TabbedFilesData*)(intptr_t)GetWindowLongPtr(wnd, 0);
	}

	unsigned int		CursorToIndex(HWND wnd, unsigned int x, unsigned int *leftover = NULL)
	{
		TabbedFilesData	*data = GetData(wnd);

		// Remove left padding
		x -= 15;

		unsigned int retValue = ~0u;
		for(unsigned int i = 0; i < data->tabCount; i++)
		{
			if(x < data->tabWidths[i])
			{
				retValue = i;
				break;
			}
			x -= data->tabWidths[i];
		}
		if(leftover)
			*leftover = x;
		return retValue;
	}

	void OnCreate(HWND wnd)
	{
		SetWindowLongPtr(wnd, 0, (WND_PTR_TYPE)(intptr_t)new TabbedFilesData);

		TabbedFilesData	*data = GetData(wnd);
		memset(data, 0, sizeof(TabbedFilesData));

		data->selectedTab = 1;

		PAINTSTRUCT paintData;
		HDC hdc = BeginPaint(wnd, &paintData);

		data->mainFont = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, FW_REGULAR, false, false, false,
			RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");

		data->brWhite = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
		data->brGray = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
		data->brLightGray = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
		data->brRed = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));

		for(int i = 0; i < 15; i++)
			data->brGradient[i] = CreatePen(PS_SOLID, 1, RGB(186 + 4*i, 186 + 4*i, 186 + 4*i));

		EndPaint(wnd, &paintData);
	}

	void OnDestroy(HWND wnd)
	{
		delete (TabbedFilesData*)(intptr_t)GetWindowLongPtr(wnd, 0);
	}

	void OnPaint(HWND wnd)
	{
		TabbedFilesData	*data = GetData(wnd);

		PAINTSTRUCT paintData;
		HDC hdc = BeginPaint(wnd, &paintData);

		SelectFont(hdc, data->mainFont);

		unsigned int leftPos = 5;

		SelectPen(hdc, data->brGray);
		MoveToEx(hdc, 0, TAB_HEIGHT, NULL);
		LineTo(hdc, data->width, TAB_HEIGHT);

		for(unsigned int i = 0; i < data->tabCount; i++)
		{
			RECT textRect = { 0, 0, 0, 0 };
			DrawText(hdc, data->tabNames[i], (int)strlen(data->tabNames[i]), &textRect, DT_CALCRECT);

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
			RECT textRectMod = { textRect.left + leftPos + 16, 3, textRect.right + leftPos, TAB_HEIGHT };
			SetBkColor(hdc, i == data->selectedTab ? RGB(255, 255, 255) : RGB(192, 192, 192));
			SetBkMode(hdc, TRANSPARENT);
			ExtTextOut(hdc, textRectMod.left, textRectMod.top, 0, &textRectMod, data->tabNames[i], (int)strlen(data->tabNames[i]), NULL);
			SetBkMode(hdc, OPAQUE);

			// Advance
			leftPos += textRect.right - textRect.left + 12;

			data->tabWidths[i] = textRect.right - textRect.left + 12;
		}

		EndPaint(wnd, &paintData);
	}

	void OnSize(HWND wnd, unsigned int width, unsigned int height)
	{
		TabbedFilesData	*data = GetData(wnd);
		data->width = width;
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
		}
		(void)y;
	}

	void OnMouseMove(HWND wnd, unsigned int x, unsigned int y)
	{
		TabbedFilesData	*data = GetData(wnd);
		if(data->leftMouseDown)
		{
			unsigned int newTab = CursorToIndex(wnd, x);
			if(newTab != -1 && newTab != data->selectedTab)
			{
				std::swap(data->tabWidths[data->selectedTab], data->tabWidths[newTab]);
				if(CursorToIndex(wnd, x) != newTab)
				{
					std::swap(data->tabWidths[data->selectedTab], data->tabWidths[newTab]);
					return;
				}
				std::swap(data->tabNames[data->selectedTab], data->tabNames[newTab]);
				data->selectedTab = newTab;
				InvalidateRect(wnd, NULL, true);
			}
		}
		(void)y;
	}
};

void TabbedFiles::AddTab(HWND wnd, const char* filename)
{
	TabbedFilesData	*data = GetData(wnd);
	if(data->tabCount >= TAB_MAX_COUNT)
	{
		MessageBox(wnd, "Maximum limit of opened tabs exceeded", "ERROR", MB_OK);
		return;
	}
	data->tabNames[data->tabCount++] = filename;
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
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= className;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);
}

LRESULT CALLBACK TabbedFiles::TabbedFilesProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		TabbedFiles::OnCreate(hWnd);
		break;
	case WM_DESTROY:
		TabbedFiles::OnDestroy(hWnd);
		break;
	case WM_PAINT:
		TabbedFiles::OnPaint(hWnd);
		break;
	case WM_SIZE:
		TabbedFiles::OnSize(hWnd, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDOWN:
		TabbedFiles::OnMouseLeft(hWnd, true, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONUP:
		TabbedFiles::OnMouseLeft(hWnd, false, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_MOUSEMOVE:
		TabbedFiles::OnMouseMove(hWnd, LOWORD(lParam), HIWORD(lParam));
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
