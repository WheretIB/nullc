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
		outputBuf = 0;
		outputBufSize = 0;
		outputBufPos = 0;

		tempBuf = 0;
		tempBufSize = 0;

		stream = 0;

		openStream = 0;
		writeStream = 0;
		closeStream = 0;
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
			else
			{
				break;
			}
			
		}

		if(!*pos)
			return;

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

	char *outputBuf;
	unsigned outputBufSize;
	unsigned outputBufPos;

	char *tempBuf;
	unsigned tempBufSize;

	void *stream;

	void* (*openStream)(const char* name);
	void (*writeStream)(void *stream, const char *data, unsigned size);
	void (*closeStream)(void* stream);
};
