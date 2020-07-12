#pragma once

#include <string>

struct Document
{
	std::string uri;
	std::string code;
	bool temporary = false;
};
