#include "runtime.h"
#include <stdio.h>
#include <string.h>
// Array classes
typedef struct
{
	int flag;
	void * id;
} File;
File File__(void* unused)
{
	File ret;
	ret.flag = 0;
	ret.id = NULL;
	return ret;
}
#define FILE_OPENED 1
File File__(NULLCArray<char> name, NULLCArray<char> access, void* unused)
{
	File ret;
	ret.flag = FILE_OPENED;
	ret.id = fopen(name.ptr, access.ptr);
	if(!ret.id)
		nullcThrowError("Cannot open file.");
	return ret;
}
void File__Open(NULLCArray<char> name, NULLCArray<char> access, File * file)
{
	if(file->flag & FILE_OPENED)
		fclose((FILE*)file->id);
	file->flag = FILE_OPENED;
	file->id = fopen(name.ptr, access.ptr);
	if(!file->id)
		nullcThrowError("Cannot open file.");
}
void File__Close(File * file)
{
	fclose((FILE*)file->id);
	file->flag = 0;
	file->id = NULL;
}
int File__Opened(File * file)
{
	return !!(file->flag & FILE_OPENED);
}
template<typename T>
void FileWriteType(T data, File* file)
{
	if(!file->id)
	{
		nullcThrowError("Cannot write to a closed file.");
		return;
	}
	if(1 != fwrite(&data, sizeof(T), 1, (FILE*)file->id))
		nullcThrowError("Failed to write to a file.");
}
void File__Write(char data, File * file)
{
	FileWriteType<char>(data, file);
}
void File__Write(short data, File * file)
{
	FileWriteType<short>(data, file);
}
void File__Write(int data, File * file)
{
	FileWriteType<int>(data, file);
}
void File__Write(long long data, File * file)
{
	FileWriteType<long long>(data, file);
}
void File__Write(float data, File * file)
{
	FileWriteType<float>(data, file);
}
void File__Write(double data, File * file)
{
	FileWriteType<double>(data, file);
}

template<typename T>
void FileReadType(T* data, File* file)
{
	if(!file->id)
	{
		nullcThrowError("Cannot read from a closed file.");
		return;
	}
	if(1 != fread(data, sizeof(T), 1, (FILE*)file->id))
		nullcThrowError("Failed to read from a file.");
}
void File__Read(char * data, File * file)
{
	FileReadType<char>(data, file);
}
void File__Read(short * data, File * file)
{
	FileReadType<short>(data, file);
}
void File__Read(int * data, File * file)
{
	FileReadType<int>(data, file);
}
void File__Read(long long * data, File * file)
{
	FileReadType<long long>(data, file);
}
void File__Read(float * data, File * file)
{
	FileReadType<float>(data, file);
}
void File__Read(double * data, File * file)
{
	FileReadType<double>(data, file);
}
void File__Read(NULLCArray<char> arr, File * file)
{
	if(!file->id)
	{
		nullcThrowError("Cannot read from a closed file.");
		return;
	}
	if(arr.size != fread(arr.ptr, 1, arr.size, (FILE*)file->id))
		nullcThrowError("Failed to read from a file.");
}
void File__Write(NULLCArray<char> arr, File * file)
{
	if(!file->id)
	{
		nullcThrowError("Cannot write to a closed file.");
		return;
	}
	if(arr.size != fwrite(arr.ptr, 1, arr.size, (FILE*)file->id))
		nullcThrowError("Failed to write to a file.");
}
void File__Print(NULLCArray<char> arr, File * file)
{
	if(!file->id)
	{
		nullcThrowError("Cannot write to a closed file.");
		return;
	}
	unsigned int length = (unsigned int)strlen(arr.ptr);
	if(length != fwrite(arr.ptr, 1, length, (FILE*)file->id))
		nullcThrowError("Failed to write to a file.");
}

