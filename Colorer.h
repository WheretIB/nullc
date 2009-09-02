#include "stdafx.h"
#pragma once

class ColorCodeCallback;

namespace ColorerGrammar
{
	class Grammar;

	void AddVar(char const* s, char const* e);
	void SetVar(char const* s, char const* e);
	void GetVar(char const* s, char const* e);

	void FuncAdd(char const* s, char const* e);
	void FuncEnd(char const* s, char const* e);
	void FuncCall(char const* s, char const* e);

	void StartType(char const* s, char const* e);
	void AddType(char const* s, char const* e);

	void OnError(char const* s, char const* e);

	void BlockBegin(char const* s, char const* e);
	void BlockEnd(char const* s, char const* e);

	void LogStrAndInfo(char const* s, char const* e);
	void LogStr(char const* s, char const* e);
};

class Colorer
{
public:
	Colorer(HWND rich);
	~Colorer();

	bool	ColorText(char *text, void (*)(unsigned int, unsigned int, int));
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