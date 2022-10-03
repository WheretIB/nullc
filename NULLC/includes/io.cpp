#include "io.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullbind.h"

#if defined(_MSC_VER)
	#include <windows.h>
#elif defined(EMSCRIPTEN)
	#include <SDL/SDL.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if defined(_MSC_VER)
	#pragma warning(disable: 4996)
#endif

namespace NULLCIO
{
	void *contextFunc = NULL;
	unsigned (*writeFunc)(void *context, char *data, unsigned length) = NULL;
	unsigned (*readFunc)(void *context, char *target, unsigned length) = NULL;

	int	SafeSprintf(char* dst, size_t size, const char* src, ...)
	{
		if(size == 0)
			return 0;

		va_list args;
		va_start(args, src);

		int result = vsnprintf(dst, size, src, args);
		dst[size-1] = '\0';

		va_end(args);

		return (result == -1 || (size_t)result >= size) ? (int)size : result;
	}

	void DefaultInitialize()
	{
#if defined(_MSC_VER)
		static bool initialized = false;

		if(!initialized)
		{
			AllocConsole();

			freopen("CONOUT$", "w", stdout);
			freopen("CONIN$", "r", stdin);

			initialized = true;
		}
#endif
	}

	unsigned DefaultWrite(void *context, char *data, unsigned length)
	{
		(void)context;

		DefaultInitialize();

		return (unsigned)fwrite(data, 1, length, stdout);
	}

	unsigned DefaultRead(void *context, char *target, unsigned length)
	{
		(void)context;

		DefaultInitialize();

		return (unsigned)fread(target, 1, length, stdin);
	}

	void ReadString(char *buf, unsigned length)
	{
		if(!length)
			return;

		*buf = 0;

		char *pos = buf;
		char ch = 0;

		while(readFunc(contextFunc, &ch, 1))
		{
			if(ch == '\n')
				break;

			*pos++ = ch;

			if(pos + 1 == buf + length)
				break;
		}

		*pos = 0;
	}

	int abs(int x){ return x < 0 ? -x : x; }

	void WriteToConsole(NULLCArray data)
	{
		if(!writeFunc)
			return;

		// Empty arrays are silently ignored
		if(!data.ptr)
			return;
		
		for(unsigned i = 0; i < data.len; i++)
		{
			if(!data.ptr[i])
				return;

			writeFunc(contextFunc, data.ptr + i, 1);
		}
	}

	void WriteLongConsole(long long number, int base)
	{
		if(!(base > 1 && base <= 16))
		{
			nullcThrowError("ERROR: incorrect base %d", base);
			return;
		}

		if(!writeFunc)
			return;

		static char symb[] = "0123456789abcdef";
		bool sign = 0;
		char buf[128];
		char *curr = buf;
		if(number < 0)
			sign = 1;

		*curr++ = *(abs(number % base) + symb);
		while(number /= base)
			*curr++ = *(abs(number % base) + symb);
		if(sign)
			*curr++ = '-';
		*curr = 0;
		int size = int(curr - buf), halfsize = size >> 1;
		for(int i = 0; i < halfsize; i++)
		{
			char tmp = buf[i];
			buf[i] = buf[size-i-1];
			buf[size-i-1] = tmp;
		}

		writeFunc(contextFunc, buf, (unsigned)strlen(buf));
	}

	void WriteIntConsole(int number, int base)
	{
		WriteLongConsole(number, base);
	}

	void WriteDoubleConsole(double num, int precision)
	{
		if(!writeFunc)
			return;

		char buf[128];
		SafeSprintf(buf, 128, "%.*f", precision, num);

		writeFunc(contextFunc, buf, (unsigned)strlen(buf));
	}

	void WriteCharConsole(char ch)
	{
		if(!writeFunc)
			return;

		char buf[128];
		sprintf(buf, "%c", ch);

		writeFunc(contextFunc, buf, (unsigned)strlen(buf));
	}

	void ReadIntFromConsole(int* val)
	{
		if(!val)
		{
			nullcThrowError("ERROR: argument should not be a nullptr");
			return;
		}

		if(!readFunc)
		{
			nullcThrowError("ERROR: read stream is not avaiable");
			return;
		}

		char buf[128];
		ReadString(buf, 128);

		int result = sscanf(buf, "%d", val);

		if(result != 1)
		{
			nullcThrowError("ERROR: failed to read an int");
			return;
		}
	}

	int ReadTextFromConsole(NULLCArray data)
	{
		if(!data.ptr)
		{
			nullcThrowError("ERROR: argument should not be a nullptr");
			return 0;
		}

		if(!readFunc)
			return 0;

		char buf[2048];
		ReadString(buf, 2048);

		if(!data.len)
			return 0;

		unsigned int len = (unsigned int)strlen(buf) + 1;
		char *target = data.ptr;
		for(unsigned int i = 0; i < (data.len < len ? data.len : len); i++)
			target[i] = buf[i];
		target[data.len - 1] = 0;
		
		return ((unsigned int)len < data.len - 1 ? len : data.len - 1);
	}

	void WriteToConsoleExact(NULLCArray data)
	{
		if(!data.ptr)
		{
			nullcThrowError("ERROR: argument should not be a nullptr");
			return;
		}

		if(!writeFunc)
			return;

		writeFunc(contextFunc, data.ptr, data.len);
	}

	void SetConsoleCursorPos(int x, int y)
	{
#if !defined(_MSC_VER)
		(void)x;
		(void)y;

		nullcThrowError("SetConsoleCursorPos: supported only under Windows");
#else	
		if(x < 0 || y < 0)
		{
			nullcThrowError("SetConsoleCursorPos: Negative values are not allowed");
			return;
		}
		COORD coords;
		coords.X = (short)x;
		coords.Y = (short)y;
		HANDLE conStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleCursorPosition(conStdOut, coords);
#endif
	}

	void GetKeyboardState(NULLCArray arr)
	{
#if defined(_MSC_VER)
		if(arr.len < 256)
			nullcThrowError("GetKeyboardState requires array with 256 or more elements");

		::GetKeyboardState((unsigned char*)arr.ptr);
#elif defined(EMSCRIPTEN)
		if(arr.len < 256)
			nullcThrowError("GetKeyboardState requires array with 256 or more elements");

		int numKeys = 0;
		const Uint8* keys = SDL_GetKeyboardState(&numKeys);

		for(int i = 0; i < numKeys && i < arr.len; i++)
			((unsigned char*)arr.ptr)[i] = keys[i];
#else
		(void)arr;

		nullcThrowError("GetKeyboardState: supported only under Windows");
#endif
	}

	void GetMouseState(int* x, int* y)
	{
#if defined(_MSC_VER)
		if(!x)
		{
			nullcThrowError("ERROR: 'x' argument should not be a nullptr");
			return;
		}
		if(!y)
		{
			nullcThrowError("ERROR: 'y' argument should not be a nullptr");
			return;
		}

		POINT pos;
		GetCursorPos(&pos);
		*x = pos.x;
		*y = pos.y;
#elif defined(EMSCRIPTEN)
		SDL_GetMouseState(x, y);
#else
		(void)x;
		(void)y;

		nullcThrowError("GetMouseState: supported only under Windows");
#endif
	}

	bool IsPressed(int key)
	{
#if defined(_MSC_VER)
		unsigned char arr[256];
		::GetKeyboardState(arr);
		return !!(arr[key & 0xff] & 0x80);
#elif defined(EMSCRIPTEN)
		int numKeys = 0;
		const Uint8* keys = SDL_GetKeyboardState(&numKeys);

		return key < numKeys ? keys[key] : false;
#else
		(void)key;

		nullcThrowError("IsPressed: supported only under Windows");
		return false;
#endif
	}

	bool IsToggled(int key)
	{
#if defined(_MSC_VER)
		unsigned char arr[256];
		::GetKeyboardState(arr);
		return !!(arr[key & 0xff] & 0x1);
#elif defined(EMSCRIPTEN)
		static Uint8 prevState[256];

		int numKeys = 0;
		const Uint8* keys = SDL_GetKeyboardState(&numKeys);

		if(key < 256 && key < numKeys)
		{
			bool changed = prevState[key] != keys[key];

			prevState[key] = keys[key];

			return changed;
		}

		return false;
#else
		(void)key;

		nullcThrowError("IsToggled: supported only under Windows");
		return false;
#endif
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.io", NULLCIO::funcPtr, name, index)) return false;

bool nullcInitIOModule(void *context, unsigned (*writeFunc)(void *context, char *data, unsigned length), unsigned (*readFunc)(void *context, char *target, unsigned length))
{
	NULLCIO::contextFunc = context;
	NULLCIO::writeFunc = writeFunc;
	NULLCIO::readFunc = readFunc;

	REGISTER_FUNC(WriteToConsole, "Print", 0);
	REGISTER_FUNC(WriteIntConsole, "Print", 1);
	REGISTER_FUNC(WriteDoubleConsole, "Print", 2);
	REGISTER_FUNC(WriteLongConsole, "Print", 3);
	REGISTER_FUNC(WriteCharConsole, "Print", 4);
	REGISTER_FUNC(ReadTextFromConsole, "Input", 0);
	REGISTER_FUNC(ReadIntFromConsole, "Input", 1);
	REGISTER_FUNC(WriteToConsoleExact, "Write", 0);
	REGISTER_FUNC(SetConsoleCursorPos, "SetConsoleCursorPos", 0);

	REGISTER_FUNC(GetKeyboardState, "GetKeyboardState", 0);
	REGISTER_FUNC(GetMouseState, "GetMouseState", 0);
	REGISTER_FUNC(IsPressed, "IsPressed", 0);
	REGISTER_FUNC(IsToggled, "IsToggled", 0);
	REGISTER_FUNC(IsPressed, "IsPressed", 1);
	REGISTER_FUNC(IsToggled, "IsToggled", 1);

	return true;
}

bool nullcInitIOModule()
{
	return nullcInitIOModule(NULL, NULLCIO::DefaultWrite, NULLCIO::DefaultRead);
}
