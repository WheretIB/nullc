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
		debugFullMessages = false;

		nullcInitialized = false;

		workspaceConfiguration = false;
		textDocumentDefinitionLinkSupport = false;
		textDocumentHierarchicalDocumentSymbolSupport = false;
	}

	bool infoMode;
	bool debugMode;
	bool debugFullMessages;

	bool nullcInitialized;

	bool workspaceConfiguration;
	bool textDocumentDefinitionLinkSupport;
	bool textDocumentHierarchicalDocumentSymbolSupport;

	std::string defaultModulePath;

	std::string rootPath;
	std::string modulePath;

	std::map<std::string, Document> documents;
};
