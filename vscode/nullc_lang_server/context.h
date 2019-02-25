#pragma once

#include <map>

#include "document.h"

struct Context
{
	Context()
	{
		debugMode = false;
		nullcInitialized = false;
	}

	bool debugMode;

	bool nullcInitialized;

	std::map<std::string, Document> documents;
};
