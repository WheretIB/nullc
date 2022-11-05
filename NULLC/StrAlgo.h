#pragma once

#include "stdafx.h"

namespace NULLC
{
	unsigned int GetStringHash(const char *str);
	unsigned int GetStringHash(const char *str, const char *end);
	unsigned int StringHashContinue(unsigned int hash, const char *str);
	unsigned int StringHashContinue(unsigned int hash, const char *str, const char *end);

	int SafeSprintf(char* dst, size_t size, const char* src, ...) NULLC_PRINT_FORMAT_CHECK(3, 4);
}

// A string that doesn't terminate with a \0 character
class InplaceStr
{
public:
	InplaceStr()
	{
		begin = NULL;
		end = NULL;
	}

	// It is possible to construct it from \0-terminated string
	explicit InplaceStr(const char *strBegin)
	{
		begin = strBegin;
		end = begin + strlen(begin);
	}

	// And from non-terminating strings
	InplaceStr(const char *strBegin, unsigned int length)
	{
		begin = strBegin;
		end = begin + length;
	}
	InplaceStr(const char *strBegin, const char *strEnd)
	{
		assert(strEnd >= strBegin);

		begin = strBegin;
		end = strEnd;
	}

	bool empty() const
	{
		return begin == end;
	}

	unsigned length() const
	{
		return unsigned(end - begin);
	}

	unsigned hash() const
	{
		return NULLC::GetStringHash(begin, end);
	}

	bool operator==(const InplaceStr& rhs) const
	{
		if(begin == rhs.begin && end == rhs.end)
			return true;

		return unsigned(end - begin) == unsigned(rhs.end - rhs.begin) && memcmp(begin, rhs.begin, unsigned(end - begin)) == 0;
	}
		
	bool operator!=(const InplaceStr& rhs) const
	{
		return !(*this == rhs);
	}

	const char* find(char ch)
	{
		if(empty())
			return NULL;

		for(const char *pos = begin; pos < end; pos++)
		{
			if(*pos == ch)
				return pos;
		}

		return NULL;
	}

	const char* rfind(char ch)
	{
		if(empty())
			return NULL;

		for(const char *pos = end - 1; pos >= begin; pos--)
		{
			if(*pos == ch)
				return pos;
		}

		return NULL;
	}

	const char *begin, *end;
};

struct InplaceStrHasher
{
	unsigned operator()(InplaceStr key)
	{
		return key.hash();
	}
};
