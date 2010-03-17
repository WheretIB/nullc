#pragma once
#include "../../nullc/nullc.h"

namespace NULLCCanvas
{
	struct Canvas
	{
		int width, height;
		
		int color;
		NullCArray data;
	};
}

bool	nullcInitCanvasModule();
