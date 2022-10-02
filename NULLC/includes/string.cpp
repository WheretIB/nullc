#include "string.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullbind.h"

#include <string.h>

#include "lua-regex.h"

static int calc_new_size_by_max_len(int start_pos, int max_len, int curr_size)
{
    int new_size;
    if(start_pos < 0)
    {
        new_size = curr_size + start_pos;
        start_pos = new_size < 0 ? 0 : new_size;
    }
    if(max_len > 0) new_size = start_pos + max_len;
    else new_size = curr_size + max_len;
    if( (new_size < curr_size) && (new_size > start_pos) )
    {
        return new_size;
    }
    return curr_size;
}

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

		return ::strcmp(a.ptr, b.ptr);
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

	int luamatch(NULLCArray a, NULLCArray b, int offset, int max_size)
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

                LuaMatchState ms;
                memset(&ms, 0, sizeof(ms));
                int src_size = a.len;
                if(max_size)
                {
                    src_size = calc_new_size_by_max_len(offset, max_size, src_size);
                }
                ptrdiff_t rc = lua_str_match (&ms, a.ptr, max_size ? offset + max_size : src_size, b.ptr, b.len, offset, 0, NULL, NULL);
		return rc;
	}

}

#define REGISTER_FUNC(name, index) if(!nullcBindModuleFunctionHelper("std.string", NULLCString::name, #name, index)) return false;
bool	nullcInitStringModule()
{
	REGISTER_FUNC(strlen, 0);
	REGISTER_FUNC(strstr, 0);
	REGISTER_FUNC(strchr, 0);
	REGISTER_FUNC(strcmp, 0);
	REGISTER_FUNC(strcpy, 0);
	REGISTER_FUNC(luamatch, 0);

	return true;
}
