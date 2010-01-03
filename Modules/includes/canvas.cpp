#include "canvas.h"

namespace NULLCCanvas
{
	void CanvasClearRGB(unsigned char red, unsigned char green, unsigned char blue, Canvas* ptr)
	{
		int color = (red << 16) | (green << 8) | (blue) | (255 << 24);
		for(int i = 0; i < ptr->width * ptr->height; i++)
			((int*)ptr->data.ptr)[i] = color;
	}
	void CanvasClearRGBA(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, Canvas* ptr)
	{
		int color = (red << 16) | (green << 8) | (blue) | (alpha << 24);
		for(int i = 0; i < ptr->width * ptr->height; i++)
			((int*)ptr->data.ptr)[i] = color;
	}

	void CanvasSetColor(unsigned char red, unsigned char green, unsigned char blue, Canvas* ptr)
	{
		ptr->color = (red << 16) | (green << 8) | (blue) | (255 << 24);
	}

	void CanvasDrawLine(int x1, int y1, int x2, int y2, Canvas* ptr)
	{
		(void)x1; (void)x2; (void)y1; (void)y2; (void)ptr;
		nullcThrowError("Unimplemented");
	}
	void CanvasDrawRect(int x1, int y1, int x2, int y2, Canvas* ptr)
	{
		for(int x = x1 < 0 ? 0 : x1, xe = x2 > ptr->width ? ptr->width : x2; x < xe; x++)
			for(int y = y1 < 0 ? 0 : y1, ye = y2 > ptr->height ? ptr->height : y2; y < ye; y++)
				((int*)ptr->data.ptr)[y*ptr->width + x] = ptr->color;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("img.canvas", (void(*)())NULLCCanvas::funcPtr, name, index)) return false;
bool	nullcInitCanvasModule()
{
	REGISTER_FUNC(CanvasClearRGB, "Canvas::Clear", 0);
	REGISTER_FUNC(CanvasClearRGBA, "Canvas::Clear", 1);
	REGISTER_FUNC(CanvasSetColor, "Canvas::SetColor", 0);
	REGISTER_FUNC(CanvasDrawLine, "Canvas::DrawLine", 0);
	REGISTER_FUNC(CanvasDrawRect, "Canvas::DrawRect", 0);

	return true;
}

void	nullcDeinitCanvasModule()
{

}
