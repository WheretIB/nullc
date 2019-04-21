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

		workspaceConfiguration = false;
		textDocumentDefinitionLinkSupport = false;
	}

	bool infoMode;
	bool debugMode;

	bool nullcInitialized;

	bool workspaceConfiguration;
	bool textDocumentDefinitionLinkSupport;

	std::string defaultModulePath;

	std::string rootPath;
	std::string modulePath;

	std::map<std::string, Document> documents;
};
