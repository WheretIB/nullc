#include "stdafx.h"
#pragma once

struct ColorerGrammar;

class Colorer
{
public:
	Colorer(HWND rich);
	~Colorer();

	void	InitParser();
	void	ColorText();
private:
	void	ColorCode(int red, int green, int blue, int bold, int ital, int under, const char* start, const char* end);
	void	LogTempStr(int str, char const* s, char const* e);
	bool	m_errUnderline;
	//////////////////////////////////////////////////////////////////////////
	HWND			m_richEdit;
	//char*			m_strStart;
	ColorerGrammar*	m_data;
	char*			m_buf;
};