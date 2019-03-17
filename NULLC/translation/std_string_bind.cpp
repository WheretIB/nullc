#include "runtime.h"

#include <stdlib.h>

int strstr_int_ref_char___char___(NULLCArray< char > string, NULLCArray< char > substring, void* __context)
{
	if(!string.ptr || !substring.ptr)
		return -1;
	const char *pos = strstr(string.ptr, substring.ptr);

	return pos ? int(pos - string.ptr) : -1;
}

int strchr_int_ref_char___char_(NULLCArray< char > string, char ch, void* __context)
{
	if(!string.ptr)
		return -1;
	const char *pos = ::strchr(string.ptr, ch);
	return pos ? int(pos - string.ptr) : -1;
}

int strcmp_int_ref_char___char___(NULLCArray< char > a, NULLCArray< char > b, void* __context)
{
	if(!a.ptr || !b.ptr)
	{
		nullcThrowError("strcmp: one of input strings is null");
		return -1;
	}
	return ::strcmp(a.ptr, b.ptr);
}
