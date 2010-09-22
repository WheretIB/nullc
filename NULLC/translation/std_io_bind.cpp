#include "runtime.h"
#include <stdio.h>

#if defined(_WIN32)
	#include <windows.h>
#endif

void Print(NULLCArray<char > text, void* unused)
{
	printf("%s", text.ptr);
}

long long abs(long long x){ return x < 0 ? -x : x; }

void Print(double num, void* unused)
{
	printf("%.12f", num);
}

void Print(long long number, int base, void* unused)
{
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

void Print(int num, int base, void* unused)
{
	Print((long long)num, base, NULL);
}

void Print(char ch, void* unused)
{
	printf("%c", ch);
}

int Input(NULLCArray<char > data, void* unused)
{
	char buffer[2048];

	if(fgets(buffer, 2048, stdin))
	{
		char *pos = strchr(buffer, '\n');
		if(pos)
			*pos = '\0'; 
	}
	unsigned int len = (unsigned int)strlen(buffer) + 1;
	char *target = data.ptr;
	for(unsigned int i = 0; i < (data.size < len ? data.size : len); i++)
		target[i] = buffer[i];
	buffer[data.size-1] = 0;
	
	return ((unsigned int)len < data.size ? len : data.size);
}

void Input(int * num, void* unused)
{
	scanf("%d", num);
}

void Write(NULLCArray<char > buf, void* unused)
{
	fwrite(buf.ptr, 1, buf.size, stdout);
}

void SetConsoleCursorPos(int x, int y, void* unused)
{
#if !defined(_WIN32)
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

void GetKeyboardState(NULLCArray<char > state, void* unused)
{
#if !defined(_WIN32)
	nullcThrowError("GetKeyboardState: supported only under Windows");
#else
	if(state.size < 256)
		nullcThrowError("GetKeyboardState requires array with 256 or more elements");
	::GetKeyboardState((unsigned char*)state.ptr);
#endif
}

void GetMouseState(int * x, int * y, void* unused)
{
#if !defined(_WIN32)
	nullcThrowError("GetMouseState: supported only under Windows");
#else
	POINT pos;
	GetCursorPos(&pos);
	*x = pos.x;
	*y = pos.y;
#endif
}

