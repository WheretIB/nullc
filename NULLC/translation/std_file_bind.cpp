#include "runtime.h"
#include <stdio.h>
#include <string.h>
// Array classes
struct File
{
	int flag;
	void * id;
};
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
void File__Open_void_ref_char___char___(NULLCArray<char> name, NULLCArray<char> access, File * file)
{
	if(file->flag & FILE_OPENED)
		fclose((FILE*)file->id);
	file->flag = FILE_OPENED;
	file->id = fopen(name.ptr, access.ptr);
	if(!file->id)
		nullcThrowError("Cannot open file.");
}
void File__Close_void_ref__(File * file)
{
	fclose((FILE*)file->id);
	file->flag = 0;
	file->id = NULL;
}
int File__Opened_int_ref__(File * file)
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
void File__Write_void_ref_char_(char data, File * file)
{
	FileWriteType<char>(data, file);
}
void File__Write_void_ref_short_(short data, File * file)
{
	FileWriteType<short>(data, file);
}
void File__Write_void_ref_int_(int data, File * file)
{
	FileWriteType<int>(data, file);
}
void File__Write_void_ref_long_(long long data, File * file)
{
	FileWriteType<long long>(data, file);
}
void File__Write_void_ref_float_(float data, File * file)
{
	FileWriteType<float>(data, file);
}
void File__Write_void_ref_double_(double data, File * file)
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
void File__Read_void_ref_char_ref_(char * data, File * file)
{
	FileReadType<char>(data, file);
}
void File__Read_void_ref_short_ref_(short * data, File * file)
{
	FileReadType<short>(data, file);
}
void File__Read_void_ref_int_ref_(int * data, File * file)
{
	FileReadType<int>(data, file);
}
void File__Read_void_ref_long_ref_(long long * data, File * file)
{
	FileReadType<long long>(data, file);
}
void File__Read_void_ref_float_ref_(float * data, File * file)
{
	FileReadType<float>(data, file);
}
void File__Read_void_ref_double_ref_(double * data, File * file)
{
	FileReadType<double>(data, file);
}
void File__Read_void_ref_char___(NULLCArray<char> arr, File * file)
{
	if(!file->id)
	{
		nullcThrowError("Cannot read from a closed file.");
		return;
	}
	if(arr.size != fread(arr.ptr, 1, arr.size, (FILE*)file->id))
		nullcThrowError("Failed to read from a file.");
}
void File__Write_void_ref_char___(NULLCArray<char> arr, File * file)
{
	if(!file->id)
	{
		nullcThrowError("Cannot write to a closed file.");
		return;
	}
	if(arr.size != fwrite(arr.ptr, 1, arr.size, (FILE*)file->id))
		nullcThrowError("Failed to write to a file.");
}
void File__Print_void_ref_char___(NULLCArray<char> arr, File * file)
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

