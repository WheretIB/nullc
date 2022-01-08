#include "runtime.h"

static NULLCArray<int> allocateArrayTyped(unsigned typeID, unsigned count)
{
	NULLCArray<int> arr;

	return NULLC::AllocArray(__nullcGetTypeInfo(typeID)->size, count, typeID);
}

static bool check_access(NULLCArray< char > buffer, int offset, unsigned readSizeLog2, int elements)
{
	if(!buffer.ptr)
	{
		nullcThrowError("buffer is empty");
		return false;
	}

	if(offset < 0)
	{
		nullcThrowError("offset is negative");
		return false;
	}

	if(elements < 0)
	{
		nullcThrowError("element count is negative");
		return false;
	}

	if(unsigned(offset) >= buffer.size)
	{
		nullcThrowError("buffer overflow");
		return false;
	}

	unsigned availableElements = ((buffer.size - offset) >> readSizeLog2);

	if(unsigned(elements) > availableElements)
	{
		nullcThrowError("buffer overflow");
		return false;
	}

	return true;
}

bool memory_read_bool_bool_ref_char___int_(NULLCArray< char > buffer, int offset, void* __context)
{
	bool result = false;

	if(!check_access(buffer, offset, 0, 1))
		return result;

	memcpy(&result, buffer.ptr + offset, sizeof(result));
	return result;
}

char memory_read_char_char_ref_char___int_(NULLCArray< char > buffer, int offset, void* __context)
{
	char result = 0;

	if(!check_access(buffer, offset, 0, 1))
		return result;

	memcpy(&result, buffer.ptr + offset, sizeof(result));
	return result;
}

short memory_read_short_short_ref_char___int_(NULLCArray< char > buffer, int offset, void* __context)
{
	short result = 0;

	if(!check_access(buffer, offset, 1, 1))
		return result;

	memcpy(&result, buffer.ptr + offset, sizeof(result));
	return result;
}

int memory_read_int_int_ref_char___int_(NULLCArray< char > buffer, int offset, void* __context)
{
	int result = 0;

	if(!check_access(buffer, offset, 2, 1))
		return result;

	memcpy(&result, buffer.ptr + offset, sizeof(result));
	return result;
}

long long memory_read_long_long_ref_char___int_(NULLCArray< char > buffer, int offset, void* __context)
{
	long long result = 0;

	if(!check_access(buffer, offset, 3, 1))
		return result;

	memcpy(&result, buffer.ptr + offset, sizeof(result));
	return result;
}

float memory_read_float_float_ref_char___int_(NULLCArray< char > buffer, int offset, void* __context)
{
	float result = 0;

	if(!check_access(buffer, offset, 2, 1))
		return result;

	memcpy(&result, buffer.ptr + offset, sizeof(result));
	return result;
}

double memory_read_double_double_ref_char___int_(NULLCArray< char > buffer, int offset, void* __context)
{
	double result = 0;

	if(!check_access(buffer, offset, 3, 1))
		return result;

	memcpy(&result, buffer.ptr + offset, sizeof(result));
	return result;
}

NULLCArray< bool > memory_read_bool_array_bool___ref_char___int_int_(NULLCArray< char > buffer, int offset, int elements, void* __context)
{
	NULLCArray<char> result;

	if(!check_access(buffer, offset, 0, elements))
		return result;

	result = allocateArrayTyped(NULLC_BASETYPE_BOOL, elements);

	memcpy(result.ptr, buffer.ptr + offset, sizeof(bool) * elements);
	return result;
}

NULLCArray< char > memory_read_char_array_char___ref_char___int_int_(NULLCArray< char > buffer, int offset, int elements, void* __context)
{
	NULLCArray<char> result;

	if(!check_access(buffer, offset, 0, elements))
		return result;

	result = allocateArrayTyped(NULLC_BASETYPE_CHAR, elements);

	memcpy(result.ptr, buffer.ptr + offset, sizeof(char) * elements);
	return result;
}

NULLCArray< short > memory_read_short_array_short___ref_char___int_int_(NULLCArray< char > buffer, int offset, int elements, void* __context)
{
	NULLCArray<char> result;

	if(!check_access(buffer, offset, 1, elements))
		return result;

	result = allocateArrayTyped(NULLC_BASETYPE_SHORT, elements);

	memcpy(result.ptr, buffer.ptr + offset, sizeof(short) * elements);
	return result;
}

NULLCArray< int > memory_read_int_array_int___ref_char___int_int_(NULLCArray< char > buffer, int offset, int elements, void* __context)
{
	NULLCArray<char> result;

	if(!check_access(buffer, offset, 2, elements))
		return result;

	result = allocateArrayTyped(NULLC_BASETYPE_INT, elements);

	memcpy(result.ptr, buffer.ptr + offset, sizeof(int) * elements);
	return result;
}

NULLCArray< long long > memory_read_long_array_long___ref_char___int_int_(NULLCArray< char > buffer, int offset, int elements, void* __context)
{
	NULLCArray<char> result;

	if(!check_access(buffer, offset, 3, elements))
		return result;

	result = allocateArrayTyped(NULLC_BASETYPE_LONG, elements);

	memcpy(result.ptr, buffer.ptr + offset, sizeof(long long) * elements);
	return result;
}

NULLCArray< float > memory_read_float_array_float___ref_char___int_int_(NULLCArray< char > buffer, int offset, int elements, void* __context)
{
	NULLCArray<char> result;

	if(!check_access(buffer, offset, 2, elements))
		return result;

	result = allocateArrayTyped(NULLC_BASETYPE_FLOAT, elements);

	memcpy(result.ptr, buffer.ptr + offset, sizeof(float) * elements);
	return result;
}

NULLCArray< double > memory_read_double_array_double___ref_char___int_int_(NULLCArray< char > buffer, int offset, int elements, void* __context)
{
	NULLCArray<char> result;

	if(!check_access(buffer, offset, 3, elements))
		return result;

	result = allocateArrayTyped(NULLC_BASETYPE_DOUBLE, elements);

	memcpy(result.ptr, buffer.ptr + offset, sizeof(double) * elements);
	return result;
}

void memory_read_void_ref_char___int_bool_ref_(NULLCArray< char > buffer, int offset, bool* value, void* __context)
{
	if(!check_access(buffer, offset, 0, 1))
		return;

	memcpy(value, buffer.ptr + offset, sizeof(*value));
}

void memory_read_void_ref_char___int_char_ref_(NULLCArray< char > buffer, int offset, char* value, void* __context)
{
	if(!check_access(buffer, offset, 0, 1))
		return;

	memcpy(value, buffer.ptr + offset, sizeof(*value));
}

void memory_read_void_ref_char___int_short_ref_(NULLCArray< char > buffer, int offset, short* value, void* __context)
{
	if(!check_access(buffer, offset, 1, 1))
		return;

	memcpy(value, buffer.ptr + offset, sizeof(*value));
}

void memory_read_void_ref_char___int_int_ref_(NULLCArray< char > buffer, int offset, int* value, void* __context)
{
	if(!check_access(buffer, offset, 2, 1))
		return;

	memcpy(value, buffer.ptr + offset, sizeof(*value));
}

void memory_read_void_ref_char___int_long_ref_(NULLCArray< char > buffer, int offset, long long* value, void* __context)
{
	if(!check_access(buffer, offset, 3, 1))
		return;

	memcpy(value, buffer.ptr + offset, sizeof(*value));
}

void memory_read_void_ref_char___int_float_ref_(NULLCArray< char > buffer, int offset, float* value, void* __context)
{
	if(!check_access(buffer, offset, 2, 1))
		return;

	memcpy(value, buffer.ptr + offset, sizeof(*value));
}

void memory_read_void_ref_char___int_double_ref_(NULLCArray< char > buffer, int offset, double* value, void* __context)
{
	if(!check_access(buffer, offset, 3, 1))
		return;

	memcpy(value, buffer.ptr + offset, sizeof(*value));
}

void memory_read_void_ref_char___int_bool___(NULLCArray< char > buffer, int offset, NULLCArray< bool > value, void* __context)
{
	if(!value.ptr)
	{
		nullcThrowError("buffer is empty");
		return;
	}

	if(!check_access(buffer, offset, 0, value.size))
		return;

	memcpy(value.ptr, buffer.ptr + offset, sizeof(bool) * value.size);
}

void memory_read_void_ref_char___int_char___(NULLCArray< char > buffer, int offset, NULLCArray< char > value, void* __context)
{
	if(!value.ptr)
	{
		nullcThrowError("buffer is empty");
		return;
	}

	if(!check_access(buffer, offset, 0, value.size))
		return;

	memcpy(value.ptr, buffer.ptr + offset, sizeof(char) * value.size);
}

void memory_read_void_ref_char___int_short___(NULLCArray< char > buffer, int offset, NULLCArray< short > value, void* __context)
{
	if(!value.ptr)
	{
		nullcThrowError("buffer is empty");
		return;
	}

	if(!check_access(buffer, offset, 1, value.size))
		return;

	memcpy(value.ptr, buffer.ptr + offset, sizeof(short) * value.size);
}

void memory_read_void_ref_char___int_int___(NULLCArray< char > buffer, int offset, NULLCArray< int > value, void* __context)
{
	if(!value.ptr)
	{
		nullcThrowError("buffer is empty");
		return;
	}

	if(!check_access(buffer, offset, 2, value.size))
		return;

	memcpy(value.ptr, buffer.ptr + offset, sizeof(int) * value.size);
}

void memory_read_void_ref_char___int_long___(NULLCArray< char > buffer, int offset, NULLCArray< long long > value, void* __context)
{
	if(!value.ptr)
	{
		nullcThrowError("buffer is empty");
		return;
	}

	if(!check_access(buffer, offset, 3, value.size))
		return;

	memcpy(value.ptr, buffer.ptr + offset, sizeof(long long) * value.size);
}

void memory_read_void_ref_char___int_float___(NULLCArray< char > buffer, int offset, NULLCArray< float > value, void* __context)
{
	if(!value.ptr)
	{
		nullcThrowError("buffer is empty");
		return;
	}

	if(!check_access(buffer, offset, 2, value.size))
		return;

	memcpy(value.ptr, buffer.ptr + offset, sizeof(float) * value.size);
}

void memory_read_void_ref_char___int_double___(NULLCArray< char > buffer, int offset, NULLCArray< double > value, void* __context)
{
	if(!value.ptr)
	{
		nullcThrowError("buffer is empty");
		return;
	}

	if(!check_access(buffer, offset, 3, value.size))
		return;

	memcpy(value.ptr, buffer.ptr + offset, sizeof(double) * value.size);
}

void memory_write_void_ref_char___int_bool_(NULLCArray< char > buffer, int offset, bool value, void* __context)
{
	if(!check_access(buffer, offset, 0, 1))
		return;

	memcpy(buffer.ptr + offset, &value, sizeof(value));
}

void memory_write_void_ref_char___int_char_(NULLCArray< char > buffer, int offset, char value, void* __context)
{
	if(!check_access(buffer, offset, 0, 1))
		return;

	memcpy(buffer.ptr + offset, &value, sizeof(value));
}

void memory_write_void_ref_char___int_short_(NULLCArray< char > buffer, int offset, short value, void* __context)
{
	if(!check_access(buffer, offset, 1, 1))
		return;

	memcpy(buffer.ptr + offset, &value, sizeof(value));
}

void memory_write_void_ref_char___int_int_(NULLCArray< char > buffer, int offset, int value, void* __context)
{
	if(!check_access(buffer, offset, 2, 1))
		return;

	memcpy(buffer.ptr + offset, &value, sizeof(value));
}

void memory_write_void_ref_char___int_long_(NULLCArray< char > buffer, int offset, long long value, void* __context)
{
	if(!check_access(buffer, offset, 3, 1))
		return;

	memcpy(buffer.ptr + offset, &value, sizeof(value));
}

void memory_write_void_ref_char___int_float_(NULLCArray< char > buffer, int offset, float value, void* __context)
{
	if(!check_access(buffer, offset, 2, 1))
		return;

	memcpy(buffer.ptr + offset, &value, sizeof(value));
}

void memory_write_void_ref_char___int_double_(NULLCArray< char > buffer, int offset, double value, void* __context)
{
	if(!check_access(buffer, offset, 3, 1))
		return;

	memcpy(buffer.ptr + offset, &value, sizeof(value));
}

void memory_write_void_ref_char___int_bool___(NULLCArray< char > buffer, int offset, NULLCArray< bool > value, void* __context)
{
	if(!check_access(buffer, offset, 0, value.size))
		return;

	memcpy(buffer.ptr + offset, value.ptr, sizeof(bool) * value.size);
}

void memory_write_void_ref_char___int_char___(NULLCArray< char > buffer, int offset, NULLCArray< char > value, void* __context)
{
	if(!check_access(buffer, offset, 0, value.size))
		return;

	memcpy(buffer.ptr + offset, value.ptr, sizeof(char) * value.size);
}

void memory_write_void_ref_char___int_short___(NULLCArray< char > buffer, int offset, NULLCArray< short > value, void* __context)
{
	if(!check_access(buffer, offset, 1, value.size))
		return;

	memcpy(buffer.ptr + offset, value.ptr, sizeof(short) * value.size);
}

void memory_write_void_ref_char___int_int___(NULLCArray< char > buffer, int offset, NULLCArray< int > value, void* __context)
{
	if(!check_access(buffer, offset, 2, value.size))
		return;

	memcpy(buffer.ptr + offset, value.ptr, sizeof(int) * value.size);
}

void memory_write_void_ref_char___int_long___(NULLCArray< char > buffer, int offset, NULLCArray< long long > value, void* __context)
{
	if(!check_access(buffer, offset, 3, value.size))
		return;

	memcpy(buffer.ptr + offset, value.ptr, sizeof(long long) * value.size);
}

void memory_write_void_ref_char___int_float___(NULLCArray< char > buffer, int offset, NULLCArray< float > value, void* __context)
{
	if(!check_access(buffer, offset, 2, value.size))
		return;

	memcpy(buffer.ptr + offset, value.ptr, sizeof(float) * value.size);
}

void memory_write_void_ref_char___int_double___(NULLCArray< char > buffer, int offset, NULLCArray< double > value, void* __context)
{
	if(!check_access(buffer, offset, 3, value.size))
		return;

	memcpy(buffer.ptr + offset, value.ptr, sizeof(double) * value.size);
}

void memory_copy_void_ref_char___int_char___int_int_(NULLCArray< char > dst, int dstOffset, NULLCArray< char > src, int srcOffset, int size, void* __context)
{
	if(!check_access(dst, dstOffset, 0, size))
		return;

	if(!check_access(src, srcOffset, 0, size))
		return;

	memcpy(dst.ptr + dstOffset, src.ptr + srcOffset, size);
}

void memory_set_void_ref_char___int_char_int_(NULLCArray< char > dst, int dstOffset, char value, int size, void* __context)
{
	if(!check_access(dst, dstOffset, 0, size))
		return;

	memset(dst.ptr + dstOffset, value, size);
}

int memory_compare_int_ref_char___int_char___int_int_(NULLCArray< char > lhs, int lhsOffset, NULLCArray< char > rhs, int rhsOffset, int size, void* __context)
{
	if(!check_access(lhs, lhsOffset, 0, size))
		return 0;

	if(!check_access(rhs, rhsOffset, 0, size))
		return 0;

	return memcmp(lhs.ptr + lhsOffset, rhs.ptr + rhsOffset, size);
}

float memory_as_float_float_ref_int_(int value, void* __context)
{
	float result;
	memcpy(&result, &value, sizeof(value));
	return result;
}

double memory_as_double_double_ref_long_(long long value, void* __context)
{
	double result;
	memcpy(&result, &value, sizeof(value));
	return result;
}

int memory_as_int_int_ref_float_(float value, void* __context)
{
	int result;
	memcpy(&result, &value, sizeof(value));
	return result;
}

long long memory_as_long_long_ref_double_(double value, void* __context)
{
	long long result;
	memcpy(&result, &value, sizeof(value));
	return result;
}

