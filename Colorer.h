#include "stdafx.h"
#pragma once

class ColorCodeCallback;

namespace ColorerGrammar
{
	void AddVar(char const* s, char const* e);
	void SetVar(char const* s, char const* e);
	void GetVar(char const* s, char const* e);

	void FuncAdd(char const* s, char const* e);
	void FuncEnd(char const* s, char const* e);
	void FuncCall(char const* s, char const* e);

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

	void	InitParser();
	void	ColorText();
private:
	friend class ColorCodeCallback;

	void	ColorCode(int red, int green, int blue, int bold, int ital, int under, const char* start, const char* end);

	bool	errUnderline;
	//////////////////////////////////////////////////////////////////////////
	HWND			richEdit;
	char*			strBuf;
};