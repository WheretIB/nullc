#include "runtime.h"
#include <stdio.h>

#if defined(_WIN32)
	#pragma warning(disable:4996)
	#include <windows.h>
#endif

long long std_io_absl(long long x)
{
	return x < 0 ? -x : x;
}

void Print_void_ref_long_int_(long long number, int base, void* unused);

void Print_void_ref_char___(NULLCArray<char > text, void* unused)
{
	printf("%s", text.ptr);
}

void Print_void_ref_int_int_(int num, int base, void* unused)
{
	Print_void_ref_long_int_((long long)num, base, NULL);
}

void Print_void_ref_double_int_(double num, int precision, void* unused)
{
	printf("%.*f", precision, num);
}

void Print_void_ref_long_int_(long long number, int base, void* unused)
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

	*curr++ = *(std_io_absl(number % base) + symb);
	while(number /= base)
		*curr++ = *(std_io_absl(number % base) + symb);
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

void Print_void_ref_char_(char ch, void* unused)
{
	printf("%c", ch);
}

int Input_int_ref_char___(NULLCArray<char > data, void* unused)
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

void Input_void_ref_int_ref_(int * num, void* unused)
{
	scanf("%d", num);
}

void Write_void_ref_char___(NULLCArray<char > buf, void* unused)
{
	fwrite(buf.ptr, 1, buf.size, stdout);
}

void SetConsoleCursorPos_void_ref_int_int_(int x, int y, void* unused)
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

void GetKeyboardState_void_ref_char___(NULLCArray<char > state, void* unused)
{
#if !defined(_WIN32)
	nullcThrowError("GetKeyboardState: supported only under Windows");
#else
	if(state.size < 256)
		nullcThrowError("GetKeyboardState requires array with 256 or more elements");
	::GetKeyboardState((unsigned char*)state.ptr);
#endif
}

void GetMouseState_void_ref_int_ref_int_ref_(int * x, int * y, void* unused)
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

struct VK
{
	int value;
};

bool IsPressed_bool_ref_VK_(VK key, void* __context)
{
#if !defined(_MSC_VER)
	(void)key;

	nullcThrowError("IsPressed: supported only under Windows");
	return false;
#else
	unsigned char arr[256];
	::GetKeyboardState(arr);
	return !!(arr[unsigned(key.value) & 0xff] & 0x80);
#endif
}

bool IsPressed_bool_ref_char_(char key, void* __context)
{
#if !defined(_MSC_VER)
	(void)key;

	nullcThrowError("IsPressed: supported only under Windows");
	return false;
#else
	unsigned char arr[256];
	::GetKeyboardState(arr);
	if(arr[key & 0xff] > 1)
		key = key;
	return !!(arr[unsigned(key) & 0xff] & 0x80);
#endif
}

bool IsToggled_bool_ref_VK_(VK key, void* __context)
{
#if !defined(_MSC_VER)
	(void)key;

	nullcThrowError("IsToggled: supported only under Windows");
	return false;
#else
	unsigned char arr[256];
	::GetKeyboardState(arr);
	return !!(arr[unsigned(key.value) & 0xff] & 0x1);
#endif
}

bool IsToggled_bool_ref_char_(char key, void* __context)
{
#if !defined(_MSC_VER)
	(void)key;

	nullcThrowError("IsToggled: supported only under Windows");
	return false;
#else
	unsigned char arr[256];
	::GetKeyboardState(arr);
	return !!(arr[unsigned(key) & 0xff] & 0x1);
#endif
}
