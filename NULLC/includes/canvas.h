#pragma once
#include "../../NULLC/nullc.h"

namespace NULLCCanvas
{
	struct Canvas
	{
		int width, height;
		
		float color[4];
		NULLCArray data;
		NULLCArray dataI;

		bool aaEnabled;
	};
	void CanvasCommit(Canvas* ptr);
}

bool	nullcInitCanvasModule();
