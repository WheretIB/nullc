#include "runtime.h"
struct Canvas 
{
	int width;
	int height;
	int color;
	NULLCArray<int > data;
};

int abs(int x){ return x < 0 ? -x : x; }
void swap(int &a, int &b){ int tmp = a; a = b; b = tmp; }

void Canvas__Clear_void_ref_char_char_char_(char red, char green, char blue, Canvas * ptr)
{
	int color = (red << 16) | (green << 8) | (blue) | (255 << 24);
	for(int i = 0; i < ptr->width * ptr->height; i++)
		((int*)ptr->data.ptr)[i] = color;
}
void Canvas__Clear_void_ref_char_char_char_char_(char red, char green, char blue, char alpha, Canvas * ptr)
{
	int color = (red << 16) | (green << 8) | (blue) | (alpha << 24);
	for(int i = 0; i < ptr->width * ptr->height; i++)
		((int*)ptr->data.ptr)[i] = color;
}
void Canvas__SetColor_void_ref_char_char_char_(char red, char green, char blue, Canvas * ptr)
{
	ptr->color = (red << 16) | (green << 8) | (blue) | (255 << 24);
}
void Canvas__DrawLine_void_ref_int_int_int_int_(int x0, int y0, int x1, int y1, Canvas * ptr)
{
	bool steep = abs(y1 - y0) > abs(x1 - x0);
	if(steep)
	{
		swap(x0, y0);
		swap(x1, y1);
	}
	if(x0 > x1)
	{
		swap(x0, x1);
		swap(y0, y1);
	}
	int deltax = x1 - x0;
	int deltay = abs(y1 - y0);
	int error = deltax / 2;
	int ystep = y0 < y1 ? 1 : -1;
	int y = y0;
	if(x0 < 0)
		return;
	for(int x = x0; x <= x1; x++)
	{
		if(steep)
		{
			if(x >= 0 && y < ptr->width && y >= 0 && x < ptr->height)
				((int*)ptr->data.ptr)[x*ptr->width + y] = ptr->color;
		}else{
			if(x >= 0 && x < ptr->width && y >= 0 && y < ptr->height)
				((int*)ptr->data.ptr)[y*ptr->width + x] = ptr->color;
		}
		error -= deltay;
		if(error < 0)
		{
			y += ystep;
			error += deltax;
		}
	}
}
void Canvas__DrawRect_void_ref_int_int_int_int_(int x1, int y1, int x2, int y2, Canvas * ptr)
{
	for(int x = x1 < 0 ? 0 : x1, xe = x2 > ptr->width ? ptr->width : x2; x < xe; x++)
		for(int y = y1 < 0 ? 0 : y1, ye = y2 > ptr->height ? ptr->height : y2; y < ye; y++)
			((int*)ptr->data.ptr)[y*ptr->width + x] = ptr->color;
}
void Canvas__DrawPoint_void_ref_int_int_(int x, int y, Canvas * ptr)
{
	((int*)ptr->data.ptr)[y*ptr->width + x] = ptr->color;
}
