#include "window.h"
#include "../../NULLC/nullc.h"
#if !defined(_MSC_VER)
	#error "Only for Windows"
#else

#include <Windows.h>
#include "canvas.h"

namespace NULLCWindow
{
#pragma pack(push, 4)
	struct Window
	{
		NULLCArray title;

		int x, y;
		int width, height;
		
		HWND handle;
	};
#pragma pack(pop)

	void WindowCreate(Window* wnd, NULLCArray title, int x, int y, int width, int height)
	{
		wnd->handle = CreateWindow("NCWND", title.ptr, WS_OVERLAPPED | WS_VISIBLE, x, y, width, height, NULL, NULL, NULL, 0);
		wnd->x = x;
		wnd->y = y;
		wnd->width = width;
		wnd->height = height;
	}

	void WindowSetTitle(NULLCArray title, Window* wnd)
	{
		SetWindowText(wnd->handle, title.ptr);
	}
	void WindowSetPosition(int x, int y, Window* wnd)
	{
		(void)x; (void)y; (void)wnd;
		nullcThrowError("Unimplemented");
	}
	void WindowSetSize(int width, int height, Window* wnd)
	{
		(void)width; (void)height; (void)wnd;
		nullcThrowError("Unimplemented");
	}

	void WindowDrawCanvas(NULLCCanvas::Canvas* c, int x, int y, Window* wnd)
	{
		NULLCCanvas::CanvasCommit(c);
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

		SetDIBitsToDevice(dc, x, y, c->width, c->height, 0, 0, 0, c->height, c->dataI.ptr, &bi, DIB_RGB_COLORS);

		ReleaseDC(wnd->handle, dc);
		EndPaint(wnd->handle, &ps);
	}

	void WindowUpdate(Window* wnd)
	{
		UpdateWindow(wnd->handle);

		MSG msg;
		while(PeekMessage(&msg, wnd->handle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void WindowClose(Window* wnd)
	{
		DestroyWindow(wnd->handle);
	}

	void OnPaint(HWND wnd)
	{
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
	
	void RegisterClass(const char *className, HINSTANCE hInstance)
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
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("win.window", (void(*)())NULLCWindow::funcPtr, name, index)) return false;
bool	nullcInitWindowModule()
{
	NULLCWindow::RegisterClass("NCWND", NULL);

	if(!nullcBindModuleFunction("win.window_ex", (void(*)())NULLCWindow::WindowCreate, "Window", 0)) return false;

	REGISTER_FUNC(WindowSetTitle, "Window::SetTitle", 0);
	REGISTER_FUNC(WindowSetPosition, "Window::SetPosition", 0);
	REGISTER_FUNC(WindowSetSize, "Window::SetSize", 0);

	REGISTER_FUNC(WindowUpdate, "Window::Update", 0);

	REGISTER_FUNC(WindowDrawCanvas, "Window::DrawCanvas", 0);

	REGISTER_FUNC(WindowClose, "Window::Close", 0);

	return true;
}
#endif

