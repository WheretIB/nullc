#include "runtime.h"

#include <Windows.h>

struct Window 
{
	NULLCArray<char > title;
	int x;
	int y;
	int width;
	int height;
	void * handle;
};
void Window__(Window * wnd, NULLCArray<char > title, int x, int y, int width, int height, void* unused)
{
	wnd->handle = CreateWindow("NCWND", title.ptr, WS_OVERLAPPED | WS_VISIBLE, x, y, width, height, NULL, NULL, NULL, 0);
	wnd->x = x;
	wnd->y = y;
	wnd->width = width;
	wnd->height = height;
}
