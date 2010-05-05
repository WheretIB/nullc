#include "io.h"
#include "../../nullc/nullc.h"
#include <windows.h>

#include <stdio.h>

#if defined(_MSC_VER)
	#pragma warning(disable: 4996)
#endif

namespace NULLCIO
{
	HANDLE	conStdIn;
	HANDLE	conStdOut;

	void InitConsole()
	{
		AllocConsole();

		conStdIn = GetStdHandle(STD_INPUT_HANDLE);
		conStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

		DWORD fdwMode = ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT; 
		SetConsoleMode(conStdIn, fdwMode);
	}

	void DeInitConsole()
	{
		FreeConsole();
	}

	void WriteToConsole(NullCArray data)
	{
		InitConsole();
		DWORD written;
		WriteFile(conStdOut, data.ptr, (DWORD)strlen(data.ptr), &written, NULL); 
	}

	void WriteIntConsole(int num)
	{
		InitConsole();
		char buf[128];
		sprintf(buf, "%d", num);
		DWORD written;
		WriteFile(conStdOut, buf, (int)strlen(buf), &written, NULL); 
	}

	void WriteDoubleConsole(double num)
	{
		InitConsole();
		char buf[128];
		sprintf(buf, "%.12f", num);
		DWORD written;
		WriteFile(conStdOut, buf, (int)strlen(buf), &written, NULL); 
	}

	void WriteLongConsole(long long num)
	{
		InitConsole();
		char buf[128];
#ifdef _MSC_VER
		sprintf(buf, "%I64dL", num);
#else
		sprintf(buf, "%lld", num);
#endif
		DWORD written;
		WriteFile(conStdOut, buf, (int)strlen(buf), &written, NULL); 
	}

	void WriteCharConsole(char ch)
	{
		DWORD written;
		WriteFile(conStdOut, &ch, 1, &written, NULL); 
	}

	void ReadIntFromConsole(int* val)
	{
		InitConsole();
		char temp[128];
		DWORD read;
		ReadFile(conStdIn, temp, 128, &read, NULL);
		*val = atoi(temp);

		DWORD written;
		WriteFile(conStdOut, "\r\n", 2, &written, NULL); 
	}

	int ReadTextFromConsole(NullCArray data)
	{
		char buffer[2048];

		InitConsole();
		DWORD read;
		ReadFile(conStdIn, buffer, 2048, &read, NULL);
		buffer[read-1] = 0;
		char *target = data.ptr;
		int c = 0;
		for(unsigned int i = 0; i < read; i++)
		{
			buffer[c++] = buffer[i];
			if(buffer[i] == '\b')
				c -= 2;
			if(c < 0)
				c = 0;
		}
		if((unsigned int)c < data.len)
			buffer[c-1] = 0;
		else
			buffer[data.len-1] = 0;
		memcpy(target, buffer, data.len);

		DWORD written;
		WriteFile(conStdOut, "\r\n", 2, &written, NULL);
		return ((unsigned int)c < data.len ? c : data.len);
	}

	void SetConsoleCursorPos(int x, int y)
	{
		if(x < 0 || y < 0)
		{
			nullcThrowError("SetConsoleCursorPos: Negative values are not allowed");
			return;
		}
		COORD coords;
		coords.X = (short)x;
		coords.Y = (short)y;
		SetConsoleCursorPosition(conStdOut, coords);
	}

	void GetKeyboardState(NullCArray arr)
	{
		if(arr.len < 256)
			nullcThrowError("GetKeyboardState requires array with 256 or more elements");
		::GetKeyboardState((unsigned char*)arr.ptr);
	}

	void GetMouseState(int* x, int* y)
	{
		POINT pos;
		GetCursorPos(&pos);
		*x = pos.x;
		*y = pos.y;
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
	REGISTER_FUNC(SetConsoleCursorPos, "SetConsoleCursorPos", 0);

	REGISTER_FUNC(GetKeyboardState, "GetKeyboardState", 0);
	REGISTER_FUNC(GetMouseState, "GetMouseState", 0);

	return true;
}
