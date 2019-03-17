#include "runtime.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
	#pragma warning(disable:4996)
#endif

// Array classes
#pragma pack(push, 4)
struct File
{
	int flag;
	void* id;
};
struct Seek
{
	Seek(): value(0){}
	Seek(int v): value(v){}
	int value;
};
#pragma pack(pop)

File File_File_ref__(void* unused)
{
	File ret;
	ret.flag = 0;
	ret.id = NULL;
	return ret;
}

#define FILE_OPENED 1

File File_File_ref_char___char___(NULLCArray< char > name, NULLCArray< char > access, void* __context)
{
	File ret;
	ret.flag = FILE_OPENED;
	ret.id = fopen(name.ptr, access.ptr);
	if(!ret.id)
		nullcThrowError("Cannot open file.");
	return ret;
}

void File__Open_void_ref_char___char___(NULLCArray< char > name, NULLCArray< char > access, File* __context)
{
	if(__context->flag & FILE_OPENED)
		fclose((FILE*)__context->id);
	__context->flag = FILE_OPENED;
	__context->id = fopen(name.ptr, access.ptr);
	if(!__context->id)
		nullcThrowError("Cannot open file.");
}

void File__Close_void_ref__(File* __context)
{
	fclose((FILE*)__context->id);
	__context->flag = 0;
	__context->id = NULL;
}

bool File__Opened_bool_ref__(File* __context)
{
	return !!(__context->flag & FILE_OPENED);
}

bool File__Eof_bool_ref__(File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	return !__context->id || feof((FILE*)__context->id);
}

void File__Seek_void_ref_Seek_int_(Seek origin, int shift, File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot seek in a closed file.");
		return;
	}

	fseek((FILE*)__context->id, shift, origin.value);
}

long long File__Tell_long_ref__(File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot tell cursor position in a closed file.");
		return 0;
	}

	return ftell((FILE*)__context->id);
}

long long File__Size_long_ref__(File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot get size of a closed file.");
		return 0;
	}

	long pos = ftell((FILE*)__context->id);
	fseek((FILE*)__context->id, 0, SEEK_END);
	long res = ftell((FILE*)__context->id);
	fseek((FILE*)__context->id, pos, SEEK_SET);

	return res;
}

template<typename T>
bool FileWriteType(T data, File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return false;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot write to a closed file.");
		return false;
	}

	return 1 == fwrite(&data, sizeof(T), 1, (FILE*)__context->id);
}

bool File__Write_bool_ref_char_(char data, File* __context)
{
	return FileWriteType<char>(data, __context);
}

bool File__Write_bool_ref_short_(short data, File* __context)
{
	return FileWriteType<short>(data, __context);
}

bool File__Write_bool_ref_int_(int data, File* __context)
{
	return FileWriteType<int>(data, __context);
}

bool File__Write_bool_ref_long_(long long data, File* __context)
{
	return FileWriteType<long long>(data, __context);
}

bool File__Write_bool_ref_float_(float data, File* __context)
{
	return FileWriteType<float>(data, __context);
}

bool File__Write_bool_ref_double_(double data, File* __context)
{
	return FileWriteType<double>(data, __context);
}

template<typename T>
bool FileReadType(T* data, File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return false;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot read from a closed file.");
		return false;
	}

	return 1 == fread(data, sizeof(T), 1, (FILE*)__context->id);
}

bool File__Read_bool_ref_char_ref_(char* data, File* __context)
{
	return FileReadType<char>(data, __context);
}

bool File__Read_bool_ref_short_ref_(short* data, File* __context)
{
	return FileReadType<short>(data, __context);
}

bool File__Read_bool_ref_int_ref_(int* data, File* __context)
{
	return FileReadType<int>(data, __context);
}

bool File__Read_bool_ref_long_ref_(long long* data, File* __context)
{
	return FileReadType<long long>(data, __context);
}

bool File__Read_bool_ref_float_ref_(float* data, File* __context)
{
	return FileReadType<float>(data, __context);
}

bool File__Read_bool_ref_double_ref_(double* data, File* __context)
{
	return FileReadType<double>(data, __context);
}

int File__Read_int_ref_char___(NULLCArray< char > arr, File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot read from a closed file.");
		return 0;
	}

	return arr.ptr ? (int)fread(arr.ptr, 1, arr.size, (FILE*)__context->id) : 0;
}

int File__Write_int_ref_char___(NULLCArray< char > arr, File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot write to a closed file.");
		return 0;
	}

	return arr.ptr ? int(fwrite(arr.ptr, 1, arr.size, (FILE*)__context->id)) : 0;
}

bool File__Print_bool_ref_char___(NULLCArray< char > arr, File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return false;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot write to a closed file.");
		return false;
	}

	unsigned length = 0;
	for(; length < arr.size; length++)
	{
		if(!arr.ptr[length])
			break;
	}

	return arr.ptr ? length == fwrite(arr.ptr, 1, length, (FILE*)__context->id) : 0;
}

int File__Read_int_ref_char___int_int_(NULLCArray< char > arr, int offset, int bytes, File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot read from a closed file.");
		return 0;
	}

	if((long long)bytes + offset > arr.size)
	{
		nullcThrowError("Array can't hold %d bytes from the specified offset.", bytes);
		return 0;
	}

	return arr.ptr ? int(fread(arr.ptr + offset, 1, bytes, (FILE*)__context->id)) : 0;
}

int File__Write_int_ref_char___int_int_(NULLCArray< char > arr, int offset, int bytes, File* __context)
{
	if(!__context)
	{
		nullcThrowError("ERROR: null pointer access");
		return 0;
	}

	if(!__context->id)
	{
		nullcThrowError("Cannot write to a closed file.");
		return 0;
	}

	if((long long)bytes + offset > arr.size)
	{
		nullcThrowError("Array doesn't contain %d bytes from the specified offset.", bytes);
		return 0;
	}

	return arr.ptr ? int(fwrite(arr.ptr + offset, 1, bytes, (FILE*)__context->id)) : 0;
}
