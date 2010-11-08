#pragma once
#include "../../NULLC/nullc.h"

namespace NULLCCanvas
{
	struct Canvas
	{
		int width, height;
		
		int color;
		NULLCArray data;

		bool lineAA;
	};
}

bool	nullcInitCanvasModule();
