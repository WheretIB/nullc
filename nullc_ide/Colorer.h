#include "stdafx.h"
#pragma once

enum COLOR_STYLE
{
	COLOR_CODE,
	COLOR_RWORD,
	COLOR_VAR,
	COLOR_VARDEF,
	COLOR_FUNC,
	COLOR_TEXT,
	COLOR_BOLD,
	COLOR_CHAR,
	COLOR_REAL,
	COLOR_INT,
	COLOR_ERR,
	COLOR_COMMENT,
};

class ColorCodeCallback;

namespace ColorerGrammar
{
	class Grammar;
};

class Colorer
{
public:
	Colorer();
	~Colorer();

	bool	ColorText(HWND wnd, char *text, void (*)(HWND, unsigned int, unsigned int, int));
	std::string		GetError();
private:
	ColorerGrammar::Grammar	*syntax;

	friend class ColorCodeCallback;

	void	ColorCode(int style, const char* start, const char* end);

	bool	errUnderline;

	std::string		lastError;
	//////////////////////////////////////////////////////////////////////////
	HWND			richEdit;
	char*			strBuf;
};