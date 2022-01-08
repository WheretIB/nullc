#include "runtime.h"

#include <stdlib.h>

int strlen_int_ref_char___(NULLCArray< char > string, void* __context)
{
	if(!string.ptr)
	{
		nullcThrowError("string is null");
		return -1;
	}

	for(unsigned i = 0; i < string.size; i++)
	{
		if(!string.ptr[i])
			return i;
	}

	nullcThrowError("string is not null-terminated");
	return -1;
}

int strstr_int_ref_char___char___(NULLCArray< char > string, NULLCArray< char > substring, void* __context)
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

	if(string.size == 0 || string.ptr[string.size - 1])
	{
		nullcThrowError("string is not null-terminated");
		return -1;
	}

	if(substring.size == 0 || substring.ptr[substring.size - 1])
	{
		nullcThrowError("substring is not null-terminated");
		return -1;
	}

	if(const char* pos = ::strstr(string.ptr, substring.ptr))
		return int(pos - string.ptr);

	return -1;
}

int strchr_int_ref_char___char_(NULLCArray< char > string, char ch, void* __context)
{
	if(!string.ptr)
	{
		nullcThrowError("string is null");
		return -1;
	}

	if(string.size == 0 || string.ptr[string.size - 1])
	{
		nullcThrowError("string is not null-terminated");
		return -1;
	}

	if(const char* pos = ::strchr(string.ptr, ch))
		return int(pos - string.ptr);

	return -1;
}

int strcmp_int_ref_char___char___(NULLCArray< char > a, NULLCArray< char > b, void* __context)
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

	if(a.size == 0 || a.ptr[a.size - 1])
	{
		nullcThrowError("first string is not null-terminated");
		return -1;
	}

	if(b.size == 0 || b.ptr[b.size - 1])
	{
		nullcThrowError("second string is not null-terminated");
		return -1;
	}

	int result = ::strcmp(a.ptr, b.ptr);

	return result < 0 ? -1 : result == 0 ? 0 : 1;
}

int strcpy_int_ref_char___char___(NULLCArray< char > dst, NULLCArray< char > src, void* __context)
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

	for(unsigned i = 0; i < src.size; i++)
	{
		if(i >= dst.size)
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
