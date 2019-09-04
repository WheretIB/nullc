#include "file.h"
#include "../../NULLC/nullc.h"
#include "../../NULLC/nullbind.h"

#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
#pragma warning(disable: 4996)
#endif

namespace NULLCFile
{
	const int FILE_OPENED = 1;

#pragma pack(push, 4)
	struct File
	{
		int		flag;
		FILE	*handle;
	};
#pragma pack(pop)

	File FileCreateEmpty()
	{
		File ret;
		ret.flag = 0;
		ret.handle = NULL;
		return ret;
	}

	File FileCreate(NULLCArray name, NULLCArray access)
	{
		File ret = { 0, 0 };

		ret.flag = FILE_OPENED;
		ret.handle = fopen(name.ptr ? name.ptr : "", access.ptr ? access.ptr : "");

		if(!ret.handle)
			ret.flag = 0;

		return ret;
	}

	void FileOpen(NULLCArray name, NULLCArray access, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return;
		}

		if(file->flag & FILE_OPENED)
			fclose(file->handle);

		file->flag = FILE_OPENED;
		file->handle = fopen(name.ptr ? name.ptr : "", access.ptr ? access.ptr : "");

		if(!file->handle)
			file->flag = 0;
	}

	void FileClose(File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return;
		}

		if(file->flag & FILE_OPENED)
			fclose(file->handle);

		file->flag = 0;
		file->handle = NULL;
	}

	int FileOpened(File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		return !!(file->flag & FILE_OPENED);
	}

	int FileEof(File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		return !file->handle || feof(file->handle);
	}

	template<typename T>
	int FileWriteType(T data, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot write to a closed file.");
			return 0;
		}

		return 1 == fwrite(&data, sizeof(T), 1, file->handle);
	}

	int FileWriteC(char data, File* file)
	{
		return FileWriteType<char>(data, file);
	}

	int FileWriteS(short data, File* file)
	{
		return FileWriteType<short>(data, file);
	}

	int FileWriteI(int data, File* file)
	{
		return FileWriteType<int>(data, file);
	}

	int FileWriteL(long long data, File* file)
	{
		return FileWriteType<long long>(data, file);
	}

	int FileWriteF(float data, File* file)
	{
		return FileWriteType<float>(data, file);
	}

	int FileWriteD(double data, File* file)
	{
		return FileWriteType<double>(data, file);
	}

	template<typename T>
	int FileReadType(T* data, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot read from a closed file.");
			return 0;
		}

		return 1 == fread(data, sizeof(T), 1, file->handle);
	}

	int FileReadC(char* data, File* file)
	{
		return FileReadType<char>(data, file);
	}

	int FileReadS(short* data, File* file)
	{
		return FileReadType<short>(data, file);
	}

	int FileReadI(int* data, File* file)
	{
		return FileReadType<int>(data, file);
	}

	int FileReadL(long long* data, File* file)
	{
		return FileReadType<long long>(data, file);
	}

	int FileReadF(float* data, File* file)
	{
		return FileReadType<float>(data, file);
	}

	int FileReadD(double* data, File* file)
	{
		return FileReadType<double>(data, file);
	}

	int FileRead(NULLCArray arr, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot read from a closed file.");
			return 0;
		}

		return arr.ptr ? int(fread(arr.ptr, 1, arr.len, file->handle)) : 0;
	}

	int FileWrite(NULLCArray arr, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot write to a closed file.");
			return 0;
		}

		return arr.ptr ? int(fwrite(arr.ptr, 1, arr.len, file->handle)) : 0;
	}

	int FilePrint(NULLCArray arr, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot write to a closed file.");
			return 0;
		}

		unsigned length = 0;
		for(; length < arr.len; length++)
		{
			if(!arr.ptr[length])
				break;
		}

		return arr.ptr ? length == fwrite(arr.ptr, 1, length, file->handle) : 0;
	}

	void FileSeek(int dir, int shift, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot seek in a closed file.");
			return;
		}

		fseek(file->handle, shift, dir);
	}

	long long FileTell(File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot tell cursor position in a closed file.");
			return 0;
		}

		return ftell(file->handle);
	}

	long long FileSize(File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot get size of a closed file.");
			return 0;
		}

		long pos = ftell(file->handle);
		fseek(file->handle, 0, SEEK_END);
		long res = ftell(file->handle);
		fseek(file->handle, pos, SEEK_SET);

		return res;
	}

	int FileReadArr(NULLCArray arr, unsigned offset, unsigned count, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot read from a closed file.");
			return 0;
		}

		if((long long)count + offset > arr.len)
		{
			nullcThrowError("Array can't hold %d bytes from the specified offset.", count);
			return 0;
		}

		return arr.ptr ? int(fread(arr.ptr + offset, 1, count, file->handle)) : 0;
	}

	int FileWriteArr(NULLCArray arr, unsigned offset, unsigned count, File* file)
	{
		if(!file)
		{
			nullcThrowError("ERROR: null pointer access");
			return 0;
		}

		if(!file->handle)
		{
			nullcThrowError("Cannot write to a closed file.");
			return 0;
		}

		if((long long)count + offset > arr.len)
		{
			nullcThrowError("Array doesn't contain %d bytes from the specified offset.", count);
			return 0;
		}

		return arr.ptr ? int(fwrite(arr.ptr + offset, 1, count, file->handle)) : 0;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.file", NULLCFile::funcPtr, name, index)) return false;

bool nullcInitFileModule()
{
	REGISTER_FUNC(FileCreateEmpty, "File", 0);
	REGISTER_FUNC(FileCreate, "File", 1);

	REGISTER_FUNC(FileOpen, "File::Open", 0);
	REGISTER_FUNC(FileClose, "File::Close", 0);

	REGISTER_FUNC(FileOpened, "File::Opened", 0);
	REGISTER_FUNC(FileEof, "File::Eof", 0);

	REGISTER_FUNC(FileSeek, "File::Seek", 0);

	REGISTER_FUNC(FileWriteC, "File::Write", 0);
	REGISTER_FUNC(FileWriteS, "File::Write", 1);
	REGISTER_FUNC(FileWriteI, "File::Write", 2);
	REGISTER_FUNC(FileWriteL, "File::Write", 3);
	REGISTER_FUNC(FileWriteF, "File::Write", 4);
	REGISTER_FUNC(FileWriteD, "File::Write", 5);

	REGISTER_FUNC(FileReadC, "File::Read", 0);
	REGISTER_FUNC(FileReadS, "File::Read", 1);
	REGISTER_FUNC(FileReadI, "File::Read", 2);
	REGISTER_FUNC(FileReadL, "File::Read", 3);
	REGISTER_FUNC(FileReadF, "File::Read", 4);
	REGISTER_FUNC(FileReadD, "File::Read", 5);

	REGISTER_FUNC(FileRead, "File::Read", 6);
	REGISTER_FUNC(FileWrite, "File::Write", 6);
	REGISTER_FUNC(FilePrint, "File::Print", 0);

	REGISTER_FUNC(FileTell, "File::Tell", 0);
	REGISTER_FUNC(FileSize, "File::Size", 0);
	REGISTER_FUNC(FileReadArr, "File::Read", 7);
	REGISTER_FUNC(FileWriteArr, "File::Write", 7);
	return true;
}
