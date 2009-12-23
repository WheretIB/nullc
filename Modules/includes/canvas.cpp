#include "canvas.h"

namespace NULLCCanvas
{
	void CanvasCreate(Canvas* canvas, int width, int height)
	{
		canvas->color = 0;
		canvas->data.ptr = (char*)new int[width*height];
		canvas->data.len = width*height;
		canvas->width = width;
		canvas->height = height;
	}

	void CanvasClearRGB(char red, char green, char blue, Canvas* ptr)
	{
		int color = (red << 24) | (green << 16) | (blue << 8) | 255;
		for(int i = 0; i < ptr->width * ptr->height; i++)
			((int*)ptr->data.ptr)[i] = color;
	}
	void CanvasClearRGBA(char red, char green, char blue, char alpha, Canvas* ptr)
	{
		int color = (red << 24) | (green << 16) | (blue << 8) | alpha;
		for(int i = 0; i < ptr->width * ptr->height; i++)
			((int*)ptr->data.ptr)[i] = color;
	}

	void CanvasSetColor(char red, char green, char blue, Canvas* ptr)
	{
		ptr->color = (red << 24) | (green << 16) | (blue << 8) | 255;
	}

	void CanvasDrawLine(int x1, char y1, char x2, char y2, Canvas* ptr)
	{
		(void)x1; (void)x2; (void)y1; (void)y2; (void)ptr;
		nullcThrowError("Unimplemented");
	}
	void CanvasDrawRect(int x1, char y1, char x2, char y2, Canvas* ptr)
	{
		for(int x = x1; x < x2; x++)
			for(int y = y1; y < y2; y++)
				((int*)ptr->data.ptr)[y*ptr->width + x] = ptr->color;
	}
	void CanvasDestroy(Canvas* ptr)
	{
		delete[] ptr->data.ptr;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("img.canvas", (void(*)())NULLCCanvas::funcPtr, name, index)) return false;
bool	nullcInitCanvasModule()
{
	REGISTER_FUNC(CanvasCreate, "Canvas", 0);

	REGISTER_FUNC(CanvasClearRGB, "Canvas::Clear", 0);
	REGISTER_FUNC(CanvasClearRGBA, "Canvas::Clear", 1);
	REGISTER_FUNC(CanvasSetColor, "Canvas::SetColor", 0);
	REGISTER_FUNC(CanvasDrawLine, "Canvas::DrawLine", 0);
	REGISTER_FUNC(CanvasDrawRect, "Canvas::DrawRect", 0);

	REGISTER_FUNC(CanvasDestroy, "Canvas::Destroy", 0);

	return true;
}

void	nullcDeinitCanvasModule()
{

}
