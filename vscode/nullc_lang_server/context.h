#pragma once

struct Context
{
	Context()
	{
		debugMode = false;
		nullcInitialized = false;
	}

	bool debugMode;

	bool nullcInitialized;
};
