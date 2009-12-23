#include "window.h"
#include "../../nullc/nullc.h"

#include <Windows.h>
#include "canvas.h"

namespace NULLCWindow
{
	struct Window
	{
		NullCArray title;

		int x, y;
		int width, height;
		
		HWND handle;
	};

	void WindowCreate(Window* wnd, NullCArray title, int x, int y, int width, int height)
	{
		wnd->handle = CreateWindow("NCWND", title.ptr, WS_OVERLAPPED, x, y, width, height, NULL, NULL, NULL, 0);
	}

	void WindowSetTitle(NullCArray title, Window* wnd)
	{
		SetWindowText(wnd->handle, title.ptr);
	}
	void WindowSetPosition(int x, int y, Window* wnd)
	{
	}
	void WindowSetSize(int width, int height, Window* wnd)
	{
	}

	void WindowDrawCanvas(NULLCCanvas::Canvas* c, int x, int y, Window* wnd)
	{
	}

	void WindowClose(Window* wnd)
	{
		DestroyWindow(wnd->handle);
	}

	void OnPaint(HWND wnd)
	{
	}

	LRESULT CALLBACK WindowProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
	{
		switch(message)
		{
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

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("win.window", (void(*)())NULLCWindow::funcPtr, name, index)) return false;
bool	nullcInitWindowModule()
{
	NULLCWindow::RegisterClass("NCWND", NULL);

	REGISTER_FUNC(WindowCreate, "Window", 0);

	REGISTER_FUNC(WindowSetTitle, "Window::SetTitle", 0);
	REGISTER_FUNC(WindowSetPosition, "Window::SetPosition", 0);
	REGISTER_FUNC(WindowSetSize, "Window::SetSize", 0);

	REGISTER_FUNC(WindowDrawCanvas, "Window::DrawCanvas", 0);

	REGISTER_FUNC(WindowClose, "Window::Close", 0);

	return true;
}

void	nullcDeinitWindowModule()
{

}
