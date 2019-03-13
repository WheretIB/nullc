#pragma once

#include <map>
#include <string>

#include "document.h"

struct Context
{
	Context()
	{
		infoMode = false;
		debugMode = false;

		nullcInitialized = false;
	}

	bool infoMode;
	bool debugMode;

	bool nullcInitialized;

	std::string rootPath;
	std::string modulePath;

	std::map<std::string, Document> documents;
};
