#include "string.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullbind.h"

#include <string.h>
#include <stdio.h>

namespace NULLCString
{
	int strlen(NULLCArray string)
	{
		if(!string.ptr)
		{
			nullcThrowError("string is null");
			return -1;
		}

		for(unsigned i = 0; i < string.len; i++)
		{
			if(!string.ptr[i])
				return i;
		}

		nullcThrowError("string is not null-terminated");
		return -1;
	}

	int strstr(NULLCArray string, NULLCArray substring)
	{
		if(!string.ptr)
		{
			nullcThrowError("string is null");
			return -1;
		}

		if(!substring.ptr)
		{
			nullcThrowError("substring is null");
			return -1;
		}

		if(string.len == 0 || string.ptr[string.len - 1])
		{
			nullcThrowError("string is not null-terminated");
			return -1;
		}

		if(substring.len == 0 || substring.ptr[substring.len - 1])
		{
			nullcThrowError("substring is not null-terminated");
			return -1;
		}

		if(const char *pos = ::strstr(string.ptr, substring.ptr))
			return int(pos - string.ptr);

		return -1;
	}

	int strchr(NULLCArray string, char ch)
	{
		if(!string.ptr)
		{
			nullcThrowError("string is null");
			return -1;
		}

		if(string.len == 0 || string.ptr[string.len - 1])
		{
			nullcThrowError("string is not null-terminated");
			return -1;
		}

		if(const char *pos = ::strchr(string.ptr, ch))
			return int(pos - string.ptr);

		return -1;
	}

	int strcmp(NULLCArray a, NULLCArray b)
	{
		if(!a.ptr)
		{
			nullcThrowError("first string is null");
			return -1;
		}

		if(!b.ptr)
		{
			nullcThrowError("second string is null");
			return -1;
		}

		if(a.len == 0 || a.ptr[a.len - 1])
		{
			nullcThrowError("first string is not null-terminated");
			return -1;
		}

		if(b.len == 0 || b.ptr[b.len - 1])
		{
			nullcThrowError("second string is not null-terminated");
			return -1;
		}

		int result = ::strcmp(a.ptr, b.ptr);

		return result < 0 ? -1 : result == 0 ? 0 : 1;
	}

	int strcpy(NULLCArray dst, NULLCArray src)
	{
		if(!dst.ptr)
		{
			nullcThrowError("destination string is null");
			return -1;
		}

		if(!src.ptr)
		{
			nullcThrowError("source string is null");
			return -1;
		}

		for(unsigned i = 0; i < src.len; i++)
		{
			if(i >= dst.len)
			{
				nullcThrowError("buffer overflow");
				return -1;
			}

			if(!src.ptr[i])
			{
				dst.ptr[i] = 0;

				return i;
			}
			else
			{
				dst.ptr[i] = src.ptr[i];
			}
		}

		nullcThrowError("string is not null-terminated");
		return -1;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.string", NULLCString::funcPtr, name, index)) return false;
bool	nullcInitStringModule()
{
	REGISTER_FUNC(strlen, "strlen", 0);
	REGISTER_FUNC(strstr, "strstr", 0);
	REGISTER_FUNC(strchr, "strchr", 0);
	REGISTER_FUNC(strcmp, "strcmp", 0);
	REGISTER_FUNC(strcpy, "strcpy", 0);

	return true;
}
