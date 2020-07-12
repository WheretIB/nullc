#include "canvas.h"

#include "../nullc.h"
#include "../nullbind.h"

#include <math.h>

namespace NULLCCanvas
{
	void CanvasClearRGB(float red, float green, float blue, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		float fRed = red / 255.0f;
		float fGreen = green / 255.0f;
		float fBlue = blue / 255.0f;
		for(int i = 0; i < ptr->width * ptr->height; i++)
		{
			((float*)ptr->data.ptr)[i * 4 + 0] = fRed;
			((float*)ptr->data.ptr)[i * 4 + 1] = fGreen;
			((float*)ptr->data.ptr)[i * 4 + 2] = fBlue;
			((float*)ptr->data.ptr)[i * 4 + 3] = 1.0f;
		}
	}
	void CanvasClearRGBA(float red, float green, float blue, float alpha, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		float fRed = red / 255.0f;
		float fGreen = green / 255.0f;
		float fBlue = blue / 255.0f;
		float fAlpha = alpha / 255.0f;
		for(int i = 0; i < ptr->width * ptr->height; i++)
		{
			((float*)ptr->data.ptr)[i * 4 + 0] = fRed;
			((float*)ptr->data.ptr)[i * 4 + 1] = fGreen;
			((float*)ptr->data.ptr)[i * 4 + 2] = fBlue;
			((float*)ptr->data.ptr)[i * 4 + 3] = fAlpha;
		}
	}

	void CanvasSetColor(unsigned char red, unsigned char green, unsigned char blue, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		ptr->color[0] = red / 255.0f;
		ptr->color[1] = green / 255.0f;
		ptr->color[2] = blue / 255.0f;
		ptr->color[3] = 1.0f;
	}
	void CanvasSetAA(bool set, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		ptr->aaEnabled = set;
	}

	int abs(int x){ return x < 0 ? -x : x; }
	void swap(int &a, int &b){ int tmp = a; a = b; b = tmp; }

	int round(double x){ return int(x + 0.5); }
	void swap(double &a, double &b){ double tmp = a; a = b; b = tmp; }
	double abs(double x){ return x < 0.0 ? -x : x; }
	double fpart(double x){ return x - floor(x); }
	double rfpart(double x){ return 1.0 - fpart(x); }

	void CanvasDrawPointInternal(unsigned x, unsigned y, double alpha, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		if(x < (unsigned)ptr->width && y < (unsigned)ptr->height)
		{
			float *pixel = &((float*)ptr->data.ptr)[y * ptr->width * 4 + x * 4];

			pixel[0] = float(ptr->color[0] * alpha + pixel[0] * (1.0 - alpha));
			pixel[1] = float(ptr->color[1] * alpha + pixel[1] * (1.0 - alpha));
			pixel[2] = float(ptr->color[2] * alpha + pixel[2] * (1.0 - alpha));
			pixel[3] = 1.0f;
		}
	}

	void CanvasDrawPointA(double x, double y, double alpha, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		unsigned xx = unsigned(x);
		unsigned yy = unsigned(y);
		if(ptr->aaEnabled)
		{
			CanvasDrawPointInternal(xx, yy, alpha * rfpart(x) * rfpart(y), ptr);
			CanvasDrawPointInternal(xx + 1, yy, alpha * fpart(x) * rfpart(y), ptr);
			CanvasDrawPointInternal(xx, yy + 1, alpha * rfpart(x) * fpart(y), ptr);
			CanvasDrawPointInternal(xx + 1, yy + 1, alpha * fpart(x) * fpart(y), ptr);
		}else{
			CanvasDrawPointInternal(xx, yy, alpha, ptr);
		}
	}

	void CanvasDrawPoint(double x, double y, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		unsigned xx = unsigned(x);
		unsigned yy = unsigned(y);
		if(ptr->aaEnabled)
		{
			CanvasDrawPointInternal(xx, yy, rfpart(x) * rfpart(y), ptr);
			CanvasDrawPointInternal(xx + 1, yy, fpart(x) * rfpart(y), ptr);
			CanvasDrawPointInternal(xx, yy + 1, rfpart(x) * fpart(y), ptr);
			CanvasDrawPointInternal(xx + 1, yy + 1, fpart(x) * fpart(y), ptr);
		}else{
			CanvasDrawPointInternal(xx, yy, 1.0, ptr);
		}
	}

	void CanvasDrawLineNoAA(int x0, int y0, int x1, int y1, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
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
					CanvasDrawPointInternal(y, x, 1.0, ptr);
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
					CanvasDrawPointInternal(x, y, 1.0, ptr);
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
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
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
			if(x2 < 0.0)
				return;
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
			CanvasDrawPointInternal(ypxl1, xpxl1, rfpart(yend) * xgap, ptr);
			CanvasDrawPointInternal(ypxl1 + 1, xpxl1, fpart(yend) * xgap, ptr);
		}else{
			CanvasDrawPointInternal(xpxl1, ypxl1, rfpart(yend) * xgap, ptr);
			CanvasDrawPointInternal(xpxl1, ypxl1 + 1, fpart(yend) * xgap, ptr);
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
			CanvasDrawPointInternal(ypxl2, xpxl2, rfpart(yend) * xgap, ptr);
			CanvasDrawPointInternal(ypxl2 + 1, xpxl2, fpart(yend) * xgap, ptr);
		}else{
			CanvasDrawPointInternal(xpxl2, ypxl2, rfpart(yend) * xgap, ptr);
			CanvasDrawPointInternal(xpxl2, ypxl2 + 1, fpart(yend) * xgap, ptr);
		}

		if(steep)
		{
			for(int x = xpxl1 + 1; x < xpxl2; x++)
			{
				CanvasDrawPointInternal(int(intery), x, rfpart(intery), ptr);
				CanvasDrawPointInternal(int(intery) + 1, x, fpart(intery), ptr);
				intery = intery + gradient;
			}
		}else{
			for(int x = xpxl1 + 1; x < xpxl2; x++)
			{
				CanvasDrawPointInternal(x, int(intery), rfpart(intery), ptr);
				CanvasDrawPointInternal(x, int(intery) + 1, fpart(intery), ptr);
				intery = intery + gradient;
			}
		}
	}
	void CanvasDrawLine(double x0, double y0, double x1, double y1, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		if(ptr->aaEnabled)
			CanvasDrawLineAA(x0, y0, x1, y1, ptr);
		else
			CanvasDrawLineNoAA(int(x0), int(y0), int(x1), int(y1), ptr);
	}
	void CanvasDrawRect(int x1, int y1, int x2, int y2, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		for(int x = x1 < 0 ? 0 : x1, xe = x2 > ptr->width ? ptr->width : x2; x < xe; x++)
		{
			for(int y = y1 < 0 ? 0 : y1, ye = y2 > ptr->height ? ptr->height : y2; y < ye; y++)
			{
				((float*)ptr->data.ptr)[(y*ptr->width + x) * 4 + 0] = ptr->color[0];
				((float*)ptr->data.ptr)[(y*ptr->width + x) * 4 + 1] = ptr->color[1];
				((float*)ptr->data.ptr)[(y*ptr->width + x) * 4 + 2] = ptr->color[2];
				((float*)ptr->data.ptr)[(y*ptr->width + x) * 4 + 3] = ptr->color[3];
			}
		}
	}
	void CanvasDrawRectA(int x1, int y1, int x2, int y2, double alpha, Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		for(int x = x1 < 0 ? 0 : x1, xe = x2 > ptr->width ? ptr->width : x2; x < xe; x++)
		{
			for(int y = y1 < 0 ? 0 : y1, ye = y2 > ptr->height ? ptr->height : y2; y < ye; y++)
			{
				float *pixel = &((float*)ptr->data.ptr)[y * ptr->width * 4 + x * 4];

				pixel[0] = float(ptr->color[0] * alpha + pixel[0] * (1.0 - alpha));
				pixel[1] = float(ptr->color[1] * alpha + pixel[1] * (1.0 - alpha));
				pixel[2] = float(ptr->color[2] * alpha + pixel[2] * (1.0 - alpha));
				pixel[3] = 1.0f;
			}
		}
	}

	float saturate(float x){ if(x < 0.0) return 0.0; if(x > 255.0) return 255.0; return x; }
	
	void CanvasCommit(Canvas* ptr)
	{
		if(!ptr)
		{
			nullcThrowError("ERROR: Canvas object is nullptr");
			return;
		}
		int *pStart = (int*)ptr->dataI.ptr, *pEnd = (int*)ptr->dataI.ptr + ptr->width * ptr->height;
		float *fStart = (float*)ptr->data.ptr;
		for(int *pix = pStart; pix != pEnd; pix++)
		{
			int red = int(saturate(*fStart++ * 255.0f));
			int green = int(saturate(*fStart++ * 255.0f));
			int blue = int(saturate(*fStart++ * 255.0f));
			fStart++;

			*pix = (red << 16) | (green << 8) | (blue) | (255 << 24);
		}
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("img.canvas", NULLCCanvas::funcPtr, name, index)) return false;
bool	nullcInitCanvasModule()
{
	REGISTER_FUNC(CanvasClearRGB, "Canvas::Clear", 0);
	REGISTER_FUNC(CanvasClearRGBA, "Canvas::Clear", 1);
	REGISTER_FUNC(CanvasSetColor, "Canvas::SetColor", 0);
	REGISTER_FUNC(CanvasSetAA, "Canvas::SetAA", 0);
	REGISTER_FUNC(CanvasDrawPoint, "Canvas::DrawPoint", 0);
	REGISTER_FUNC(CanvasDrawPointA, "Canvas::DrawPoint", 1);
	REGISTER_FUNC(CanvasDrawLine, "Canvas::DrawLine", 0);
	REGISTER_FUNC(CanvasDrawRect, "Canvas::DrawRect", 0);
	REGISTER_FUNC(CanvasDrawRectA, "Canvas::DrawRect", 1);

	return true;
}
