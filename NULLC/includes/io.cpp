#include "io.h"
#include "../../NULLC/nullc.h"
#if defined(_MSC_VER)
	#include <windows.h>
#endif

#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
	#pragma warning(disable: 4996)
#endif

namespace NULLCIO
{
	void InitConsole()
	{
#if defined(_MSC_VER)
		AllocConsole();

		freopen("CONOUT$", "w", stdout);
		freopen("CONIN$", "r", stdin);
#endif
	}

	void DeInitConsole()
	{
#if defined(_MSC_VER)
		FreeConsole();
#endif
	}

	int abs(int x){ return x < 0 ? -x : x; }

	void WriteToConsole(NULLCArray data)
	{
		InitConsole();
		printf("%s", data.ptr);
	}

	void WriteLongConsole(long long number, int base)
	{
		InitConsole();
		if(!(base > 1 && base <= 16))
		{
			nullcThrowError("incorrect base %d", base);
			return;
		}

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
		printf("%s", buf);
	}

	void WriteIntConsole(int number, int base)
	{
		WriteLongConsole(number, base);
	}

	void WriteDoubleConsole(double num)
	{
		InitConsole();
		printf("%.12f", num);
	}

	void WriteCharConsole(char ch)
	{
		InitConsole();
		printf("%c", ch);
	}

	void ReadIntFromConsole(int* val)
	{
		InitConsole();
		scanf("%d", val);
	}

	int ReadTextFromConsole(NULLCArray data)
	{
		char buffer[2048];

		InitConsole();
		if(fgets(buffer, 2048, stdin))
		{
			char *pos = strchr(buffer, '\n');
			if(pos)
				*pos = '\0'; 
		}
		unsigned int len = (unsigned int)strlen(buffer) + 1;
		char *target = data.ptr;
		for(unsigned int i = 0; i < (data.len < len ? data.len : len); i++)
			target[i] = buffer[i];
		buffer[data.len-1] = 0;
		
		return ((unsigned int)len < data.len ? len : data.len);
	}

	void WriteToConsoleExact(NULLCArray data)
	{
		InitConsole();
		fwrite(data.ptr, 1, data.len, stdout);
	}

	void SetConsoleCursorPos(int x, int y)
	{
#if !defined(_MSC_VER)
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
#if !defined(_MSC_VER)
		nullcThrowError("GetKeyboardState: supported only under Windows");
#else
		if(arr.len < 256)
			nullcThrowError("GetKeyboardState requires array with 256 or more elements");
		::GetKeyboardState((unsigned char*)arr.ptr);
#endif
	}

	void GetMouseState(int* x, int* y)
	{
#if !defined(_MSC_VER)
		nullcThrowError("GetMouseState: supported only under Windows");
#else
		POINT pos;
		GetCursorPos(&pos);
		*x = pos.x;
		*y = pos.y;
#endif
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.io", (void(*)())NULLCIO::funcPtr, name, index)) return false;
bool	nullcInitIOModule()
{
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

	return true;
}
