#include "canvas.h"
#include <math.h>

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
	void CanvasSetLineAA(bool set, Canvas* ptr)
	{
		ptr->lineAA = set;
	}

	int abs(int x){ return x < 0 ? -x : x; }
	void swap(int &a, int &b){ int tmp = a; a = b; b = tmp; }

	int round(double x){ return int(x + 0.5); }
	void swap(double &a, double &b){ double tmp = a; a = b; b = tmp; }
	double abs(double x){ return x < 0.0 ? -x : x; }
	double fpart(double x){ return x - floor(x); }
	double rfpart(double x){ return 1.0 - fpart(x); }

	void CanvasDrawPoint(unsigned x, unsigned y, Canvas* ptr)
	{
		if(x < (unsigned)ptr->width && y < (unsigned)ptr->height)
			((int*)ptr->data.ptr)[y * ptr->width + x] = ptr->color;
	}
	void CanvasDrawPointA(unsigned x, unsigned y, double alpha, Canvas* ptr)
	{
		if(x < (unsigned)ptr->width && y < (unsigned)ptr->height)
		{
			int *pixel = &((int*)ptr->data.ptr)[y * ptr->width + x];

			int red = int(((ptr->color >> 16) & 0xff) * alpha + ((*pixel >> 16) & 0xff) * (1.0 - alpha));
			int green = int(((ptr->color >> 8) & 0xff) * alpha + ((*pixel >> 8) & 0xff) * (1.0 - alpha));
			int blue = int(((ptr->color) & 0xff) * alpha + ((*pixel) & 0xff) * (1.0 - alpha));

			*pixel = (red << 16) | (green << 8) | (blue);
		}
	}

	void CanvasDrawLineNoAA(int x0, int y0, int x1, int y1, Canvas* ptr)
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
		{
			double l = double(-x0) / (x1 - x0);
			x0 = 0;
			y = int(y0 * (1.0 - l) + y1 * l);
		}
		if(x0 > (steep ? ptr->height : ptr->width))
			return;
		if(x1 > (steep ? ptr->height : ptr->width))
			x1 = steep ? ptr->height : ptr->width;
		if(steep)
		{
			for(int x = x0; x <= x1; x++)
			{
				if((unsigned)y < (unsigned)ptr->width && (unsigned)x < (unsigned)ptr->height)
					((int*)ptr->data.ptr)[x * ptr->width + y] = ptr->color;
				error -= deltay;
				if(error < 0)
				{
					y += ystep;
					error += deltax;
				}
			}
		}else{
			for(int x = x0; x <= x1; x++)
			{
				if((unsigned)x < (unsigned)ptr->width && (unsigned)y < (unsigned)ptr->height)
					((int*)ptr->data.ptr)[y * ptr->width + x] = ptr->color;
				error -= deltay;
				if(error < 0)
				{
					y += ystep;
					error += deltax;
				}
			}
		}
	}
	void CanvasDrawLineAA(double x1, double y1, double x2, double y2, Canvas* ptr)
	{
		double dx = x2 - x1;
		double dy = y2 - y1;
		bool steep = false;
		if(abs(dx) < abs(dy))
		{			
			swap(x1, y1);
			swap(x2, y2);
			swap(dx, dy);
			steep = true;
		}
		if(x2 < x1)
		{
			swap(x1, x2);
			swap(y1, y2);
		}
		if(x1 < 0.0)
		{
			double l = -x1 / (x2 - x1);
			x1 = 0.0;
			y1 = y1 * (1.0 - l) + y2 * l;
		}
		if(x1 > (steep ? ptr->height : ptr->width))
			return;
		if(x2 > (steep ? ptr->height : ptr->width))
			x2 = steep ? ptr->height : ptr->width;
		double gradient = dy / dx;

		// handle first endpoint
		int xend = round(x1);
		double yend = y1 + gradient * (xend - x1);
		double xgap = rfpart(x1 + 0.5);
		int xpxl1 = xend;
		int ypxl1 = int(yend);
		if(steep)
		{
			CanvasDrawPointA(ypxl1, xpxl1, rfpart(yend) * xgap, ptr);
			CanvasDrawPointA(ypxl1 + 1, xpxl1, fpart(yend) * xgap, ptr);
		}else{
			CanvasDrawPointA(xpxl1, ypxl1, rfpart(yend) * xgap, ptr);
			CanvasDrawPointA(xpxl1, ypxl1 + 1, fpart(yend) * xgap, ptr);
		}
		double intery = yend + gradient; // first y-intersection for the main loop

		// handle second endpoint
		xend = round(x2);
		yend = y2 + gradient * (xend - x2);
		xgap = fpart(x2 + 0.5);
		int xpxl2 = xend ; // this will be used in the main loop
		int ypxl2 = int(yend);
		if(steep)
		{
			CanvasDrawPointA(ypxl2, xpxl2, rfpart(yend) * xgap, ptr);
			CanvasDrawPointA(ypxl2 + 1, xpxl2, fpart(yend) * xgap, ptr);
		}else{
			CanvasDrawPointA(xpxl2, ypxl2, rfpart(yend) * xgap, ptr);
			CanvasDrawPointA(xpxl2, ypxl2 + 1, fpart(yend) * xgap, ptr);
		}

		if(steep)
		{
			for(int x = xpxl1 + 1; x < xpxl2; x++)
			{
				CanvasDrawPointA(int(intery), x, rfpart(intery), ptr);
				CanvasDrawPointA(int(intery) + 1, x, fpart(intery), ptr);
				intery = intery + gradient;
			}
		}else{
			for(int x = xpxl1 + 1; x < xpxl2; x++)
			{
				CanvasDrawPointA(x, int(intery), rfpart(intery), ptr);
				CanvasDrawPointA(x, int(intery) + 1, fpart(intery), ptr);
				intery = intery + gradient;
			}
		}
	}
	void CanvasDrawLine(double x0, double y0, double x1, double y1, Canvas* ptr)
	{
		if(ptr->lineAA)
			CanvasDrawLineAA(x0, y0, x1, y1, ptr);
		else
			CanvasDrawLineNoAA(int(x0), int(y0), int(x1), int(y1), ptr);
	}
	void CanvasDrawRect(int x1, int y1, int x2, int y2, Canvas* ptr)
	{
		for(int x = x1 < 0 ? 0 : x1, xe = x2 > ptr->width ? ptr->width : x2; x < xe; x++)
			for(int y = y1 < 0 ? 0 : y1, ye = y2 > ptr->height ? ptr->height : y2; y < ye; y++)
				((int*)ptr->data.ptr)[y*ptr->width + x] = ptr->color;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("img.canvas", (void(*)())NULLCCanvas::funcPtr, name, index)) return false;
bool	nullcInitCanvasModule()
{
	REGISTER_FUNC(CanvasClearRGB, "Canvas::Clear", 0);
	REGISTER_FUNC(CanvasClearRGBA, "Canvas::Clear", 1);
	REGISTER_FUNC(CanvasSetColor, "Canvas::SetColor", 0);
	REGISTER_FUNC(CanvasSetLineAA, "Canvas::SetLineAA", 0);
	REGISTER_FUNC(CanvasDrawPoint, "Canvas::DrawPoint", 0);
	REGISTER_FUNC(CanvasDrawPointA, "Canvas::DrawPoint", 1);
	REGISTER_FUNC(CanvasDrawLine, "Canvas::DrawLine", 0);
	REGISTER_FUNC(CanvasDrawRect, "Canvas::DrawRect", 0);

	return true;
}
