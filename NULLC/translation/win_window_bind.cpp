#include "runtime.h"
#include <stdio.h>
#include <Windows.h>

struct Window 
{
	NULLCArray<char > title;
	int x;
	int y;
	int width;
	int height;
	HWND handle;
};
struct Canvas 
{
	int width;
	int height;
	int color;
	NULLCArray<int > data;
};
extern void Window__(Window * wnd, NULLCArray<char > title, int x, int y, int width, int height, void* unused);
extern Canvas Canvas__(int width, int height, void* unused);
extern void Canvas__Clear_void_ref_char_char_char_(char red, char green, char blue, Canvas * __context);
extern void Canvas__Clear_void_ref_char_char_char_char_(char red, char green, char blue, char alpha, Canvas * __context);
extern void Canvas__SetColor_void_ref_char_char_char_(char red, char green, char blue, Canvas * __context);
extern void Canvas__DrawLine_void_ref_int_int_int_int_(int x1, int y1, int x2, int y2, Canvas * __context);
extern void Canvas__DrawRect_void_ref_int_int_int_int_(int x1, int y1, int x2, int y2, Canvas * __context);
extern void Canvas__DrawPoint_void_ref_int_int_(int x, int y, Canvas * __context);
extern NULLCArray<int > Canvas__GetData_int___ref__(Canvas * __context);

void Window__SetTitle_void_ref_char___(NULLCArray<char > title, Window * wnd)
{
	SetWindowText(wnd->handle, title.ptr);
}
void Window__SetPosition_void_ref_int_int_(int x, int y, Window * wnd)
{
}
void Window__SetSize_void_ref_int_int_(int width, int height, Window * wnd)
{
}

void Window__DrawCanvas_void_ref_Canvas_ref_int_int_(Canvas * c, int x, int y, Window * wnd)
{
	PAINTSTRUCT ps;
	BeginPaint(wnd->handle, &ps);
	HDC dc = GetDC(wnd->handle);

	BITMAPINFO bi;
	memset(&bi, 0, sizeof(bi));
	bi.bmiHeader.biSize = sizeof(bi);
	bi.bmiHeader.biWidth = c->width;
	bi.bmiHeader.biHeight = -c->height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage;
	bi.bmiHeader.biXPelsPerMeter;
	bi.bmiHeader.biYPelsPerMeter;
	bi.bmiHeader.biClrUsed;
	bi.bmiHeader.biClrImportant;

	SetDIBitsToDevice(dc, x, y, c->width, c->height, 0, 0, 0, c->height, c->data.ptr, &bi, DIB_RGB_COLORS);

	ReleaseDC(wnd->handle, dc);
	EndPaint(wnd->handle, &ps);
}
void Window__Update_void_ref__(Window * wnd)
{
	UpdateWindow(wnd->handle);

	MSG msg;
	while(PeekMessage(&msg, wnd->handle, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
void Window__Close_void_ref__(Window * wnd)
{
	DestroyWindow(wnd->handle);
}


void OnPaint(HWND wnd)
{
	printf("OnPaint\n");
	PAINTSTRUCT ps;
	BeginPaint(wnd, &ps);
	EndPaint(wnd, &ps);
}

LRESULT CALLBACK WindowProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		break;
	case WM_PAINT:
		OnPaint(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void MyRegisterClass(const char *className, HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize			= sizeof(WNDCLASSEX); 
	wcex.style			= CS_DBLCLKS;
	wcex.lpfnWndProc	= (WNDPROC)WindowProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= className;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);
}

void __init_win_window_special()
{
	MyRegisterClass("NCWND", NULL);
}
