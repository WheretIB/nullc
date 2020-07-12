#include "string.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullbind.h"

#include <string.h>

namespace NULLCString
{
	int strstr(NULLCArray string, NULLCArray substring)
	{
		if(!string.ptr || !substring.ptr)
			return -1;
		const char *pos = ::strstr(string.ptr, substring.ptr);
		return pos ? int(pos - string.ptr) : -1;
	}
	int strchr(NULLCArray string, char ch)
	{
		if(!string.ptr)
			return -1;
		const char *pos = ::strchr(string.ptr, ch);
		return pos ? int(pos - string.ptr) : -1;
	}
	int strcmp(NULLCArray a, NULLCArray b)
	{
		if(!a.ptr || !b.ptr)
		{
			nullcThrowError("strcmp: one of input strings is null");
			return -1;
		}
		return ::strcmp(a.ptr, b.ptr);
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.string", NULLCString::funcPtr, name, index)) return false;
bool	nullcInitStringModule()
{
	REGISTER_FUNC(strstr, "strstr", 0);
	REGISTER_FUNC(strchr, "strchr", 0);
	REGISTER_FUNC(strcmp, "strcmp", 0);

	return true;
}
