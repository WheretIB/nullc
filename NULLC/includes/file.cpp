#include "file.h"
#include "../../NULLC/nullc.h"

#include <stdio.h>
#include <string.h>

#pragma warning(disable: 4996)

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
		File ret;
		ret.flag = FILE_OPENED;
		ret.handle = fopen(name.ptr, access.ptr);
		if(!ret.handle)
			nullcThrowError("Cannot open file.");
		return ret;
	}
	void FileOpen(NULLCArray name, NULLCArray access, File* file)
	{
		if(file->flag & FILE_OPENED)
			fclose(file->handle);
		file->flag = FILE_OPENED;
		file->handle = fopen(name.ptr, access.ptr);
		if(!file->handle)
			nullcThrowError("Cannot open file.");
	}
	void FileClose(File* file)
	{
		fclose(file->handle);
		file->flag = 0;
		file->handle = NULL;
	}

	int FileOpened(File* file)
	{
		return !!(file->flag & FILE_OPENED);
	}

	template<typename T>
	void FileWriteType(T data, File* file)
	{
		if(!file->handle)
		{
			nullcThrowError("Cannot write to a closed file.");
			return;
		}
		if(1 != fwrite(&data, sizeof(T), 1, file->handle))
			nullcThrowError("Failed to write to a file.");
	}
	void FileWriteC(char data, File* file)
	{
		FileWriteType<char>(data, file);
	}
	void FileWriteS(short data, File* file)
	{
		FileWriteType<short>(data, file);
	}
	void FileWriteI(int data, File* file)
	{
		FileWriteType<int>(data, file);
	}
	void FileWriteL(long long data, File* file)
	{
		FileWriteType<long long>(data, file);
	}
	void FileWriteF(float data, File* file)
	{
		FileWriteType<float>(data, file);
	}
	void FileWriteD(double data, File* file)
	{
		FileWriteType<double>(data, file);
	}

	template<typename T>
	void FileReadType(T* data, File* file)
	{
		if(!file->handle)
		{
			nullcThrowError("Cannot read from a closed file.");
			return;
		}
		if(1 != fread(data, sizeof(T), 1, file->handle))
			nullcThrowError("Failed to read from a file.");
	}
	void FileReadC(char* data, File* file)
	{
		FileReadType<char>(data, file);
	}
	void FileReadS(short* data, File* file)
	{
		FileReadType<short>(data, file);
	}
	void FileReadI(int* data, File* file)
	{
		FileReadType<int>(data, file);
	}
	void FileReadL(long long* data, File* file)
	{
		FileReadType<long long>(data, file);
	}
	void FileReadF(float* data, File* file)
	{
		FileReadType<float>(data, file);
	}
	void FileReadD(double* data, File* file)
	{
		FileReadType<double>(data, file);
	}

	void FileRead(NULLCArray arr, File* file)
	{
		if(!file->handle)
		{
			nullcThrowError("Cannot read from a closed file.");
			return;
		}
		if(arr.len != fread(arr.ptr, 1, arr.len, file->handle))
			nullcThrowError("Failed to read from a file.");
	}
	void FileWrite(NULLCArray arr, File* file)
	{
		if(!file->handle)
		{
			nullcThrowError("Cannot write to a closed file.");
			return;
		}
		if(arr.len != fwrite(arr.ptr, 1, arr.len, file->handle))
			nullcThrowError("Failed to write to a file.");
	}
	void FilePrint(NULLCArray arr, File* file)
	{
		if(!file->handle)
		{
			nullcThrowError("Cannot write to a closed file.");
			return;
		}
		unsigned int length = (unsigned int)strlen(arr.ptr);
		if(length != fwrite(arr.ptr, 1, length, file->handle))
			nullcThrowError("Failed to write to a file.");
	}
	void FileSeek(int dir, int shift, File* file)
	{
		if(!file->handle)
		{
			nullcThrowError("Cannot seek in a closed file.");
			return;
		}
		fseek(file->handle, shift, dir);
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunction("std.file", (void(*)())NULLCFile::funcPtr, name, index)) return false;

bool nullcInitFileModule()
{
	REGISTER_FUNC(FileCreateEmpty, "File", 0);
	REGISTER_FUNC(FileCreate, "File", 1);

	REGISTER_FUNC(FileOpen, "File::Open", 0);
	REGISTER_FUNC(FileClose, "File::Close", 0);

	REGISTER_FUNC(FileOpened, "File::Opened", 0);

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
	return true;
}
