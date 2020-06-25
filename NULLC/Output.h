#pragma once

#if defined(_MSC_VER)
#pragma warning(disable: 4996)
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "nullcdef.h"

struct OutputContext
{
	OutputContext()
	{
		outputBuf = outputBufDef;
		outputBufSize = 256;
		outputBufPos = 0;

		tempBuf = tempBufDef;
		tempBufSize = 256;

		stream = 0;

		openStream = FileOpen;
		writeStream = FileWrite;
		closeStream = FileClose;
	}

	~OutputContext()
	{
		assert(!stream);
		assert(outputBufPos == 0);
	}

	void Print(char ch)
	{
		outputBuf[outputBufPos++] = ch;

		if(outputBufPos == outputBufSize)
		{
			writeStream(stream, outputBuf, outputBufPos);
			outputBufPos = 0;
		}
	}

	void Print(const char *str, unsigned length)
	{
		if(outputBufPos + length < outputBufSize)
		{
			memcpy(outputBuf + outputBufPos, str, length);
			outputBufPos += length;
		}
		else
		{
			unsigned remainder = outputBufSize - outputBufPos;

			memcpy(outputBuf + outputBufPos, str, remainder);
			outputBufPos += remainder;

			writeStream(stream, outputBuf, outputBufPos);
			outputBufPos = 0;

			str += remainder;
			length -= remainder;

			if(!length)
				return;

			if(length < outputBufSize)
			{
				memcpy(outputBuf + outputBufPos, str, length);
				outputBufPos += length;
			}
			else
			{
				writeStream(stream, str, length);
			}
		}
	}

	void Print(const char *str)
	{
		Print(str, unsigned(strlen(str)));
	}

	void Print(const char *format, va_list args)
	{
		const char *pos = format;

		for(;;)
		{
			const char *start = pos;

			while(*pos && *pos != '%')
				pos++;

			if(pos != start)
				Print(start, unsigned(pos - start));

			if(pos[0] == '%' && pos[1] == 's')
			{
				pos += 2;

				char *str = va_arg(args, char*);

				Print(str);
			}
			else if(pos[0] == '%' && pos[1] == '.' && pos[2] == '*' && pos[3] == 's')
			{
				pos += 4;

				unsigned length = va_arg(args, unsigned);
				char *str = va_arg(args, char*);

				Print(str, length);
			}
			else if(pos[0] == '%' && pos[1] == '%')
			{
				pos += 2;

				Print('%');
			}
			else if(pos[0] == '%')
			{
				const char *tmpPos = pos + 1;

				bool leadingZeroes = false;

				if(*tmpPos == '0')
				{
					leadingZeroes = true;

					tmpPos++;
				}

				unsigned width = 0;

				if(unsigned(*tmpPos - '0') < 10)
				{
					width = unsigned(*tmpPos - '0');

					tmpPos++;
				}

				if(*tmpPos == 'd' || *tmpPos == 'x')
				{
					pos = tmpPos + 1;

					int value = va_arg(args, int);

					if(*tmpPos == 'd' && value < 0)
						Print('-');

					unsigned uvalue;

					if(*tmpPos == 'd' && value < 0)
						uvalue = -value;
					else
						uvalue = value;

					char reverse[16];

					char *curr = reverse;

					if(*tmpPos == 'd')
					{
						*curr++ = (char)((uvalue % 10) + '0');

						while(uvalue /= 10)
							*curr++ = (char)((uvalue % 10) + '0');
					}
					else
					{
						const char *symbols = "0123456789abcdef";

						*curr++ = symbols[uvalue % 16];

						while(uvalue /= 16)
							*curr++ = symbols[uvalue % 16];
					}

					while(unsigned(curr - reverse) < width)
						*curr++ = leadingZeroes ? '0' : ' ';

					char forward[16];

					char *result = forward;

					do
					{
						--curr;
						*result++ = *curr;
					}
					while(curr != reverse);

					*result = 0;

					Print(forward);
				}
				else
				{
					break;
				}
			}
			else if(pos[0] == 0)
			{
				return;
			}
			else
			{
				break;
			}
		}

		int length = vsnprintf(tempBuf, tempBufSize - 1, pos, args);

		if(length < 0 || length > int(tempBufSize) - 1)
		{
			writeStream(stream, outputBuf, outputBufPos);
			outputBufPos = 0;

			writeStream(stream, tempBuf, tempBufSize - 1);

			assert(!"temporary buffer is too small");
		}
		else
		{
			Print(tempBuf);
		}
	}

	NULLC_PRINT_FORMAT_CHECK(2, 3) void Printf(const char *format, ...)
	{
		va_list args;
		va_start(args, format);

		Print(format, args);

		va_end(args);
	}

	void Flush()
	{
		if(outputBufPos)
			writeStream(stream, outputBuf, outputBufPos);
		outputBufPos = 0;
	}

	static void* FileOpen(const char *name)
	{
		return fopen(name, "w");
	}

	static void FileWrite(void *stream, const char *data, unsigned size)
	{
		fwrite(data, 1, size, (FILE*)stream);
	}

	static void FileClose(void *stream)
	{
		fclose((FILE*)stream);
	}

	char outputBufDef[256];
	char *outputBuf;
	unsigned outputBufSize;
	unsigned outputBufPos;

	char tempBufDef[256];
	char *tempBuf;
	unsigned tempBufSize;

	void *stream;

	void* (*openStream)(const char* name);
	void (*writeStream)(void *stream, const char *data, unsigned size);
	void (*closeStream)(void* stream);
};
