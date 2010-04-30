#pragma once
#ifndef NULLC_STRALGO_H
#define NULLC_STRALGO_H

#include "stdafx.h"

unsigned int GetStringHash(const char *str);
unsigned int GetStringHash(const char *str, const char *end);
unsigned int StringHashContinue(unsigned int hash, const char *str);
unsigned int StringHashContinue(unsigned int hash, const char *str, const char *end);

char*	PrintInteger(char* str, int number);

int		SafeSprintf(char* dst, size_t size, const char* src, ...);

// A string that doesn't terminate with a \0 character
class InplaceStr
{
public:
	InplaceStr(){ begin = NULL; end = NULL; }
	// It is possible to construct it from \0-terminated string
	explicit InplaceStr(const char *strBegin){ begin = strBegin; end = begin + strlen(begin); }
	// And from non-terminating strings
	InplaceStr(const char *strBegin, unsigned int length){ begin = strBegin; end = begin + length; }
	InplaceStr(const char *strBegin, const char *strEnd){ begin = strBegin; end = strEnd; }

	const char *begin, *end;
};

#endif
