#include "memory.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullbind.h"

namespace NULLCMemory
{
	bool check_access(NULLCArray buffer, int offset, unsigned readSizeLog2, int elements)
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

		if(unsigned(offset) >= buffer.len)
		{
			nullcThrowError("buffer overflow");
			return false;
		}

		unsigned availableElements = ((buffer.len - offset) >> readSizeLog2);

		if(unsigned(elements) > availableElements)
		{
			nullcThrowError("buffer overflow");
			return false;
		}

		return true;
	}

	bool read_bool(NULLCArray buffer, int offset)
	{
		bool result = false;

		if(!check_access(buffer, offset, 0, 1))
			return result;

		memcpy(&result, buffer.ptr + offset, sizeof(result));
		return result;
	}

	char read_char(NULLCArray buffer, int offset)
	{
		char result = 0;

		if(!check_access(buffer, offset, 0, 1))
			return result;

		memcpy(&result, buffer.ptr + offset, sizeof(result));
		return result;
	}

	short read_short(NULLCArray buffer, int offset)
	{
		short result = 0;

		if(!check_access(buffer, offset, 1, 1))
			return result;

		memcpy(&result, buffer.ptr + offset, sizeof(result));
		return result;
	}

	int read_int(NULLCArray buffer, int offset)
	{
		int result = 0;

		if(!check_access(buffer, offset, 2, 1))
			return result;

		memcpy(&result, buffer.ptr + offset, sizeof(result));
		return result;
	}

	long long read_long(NULLCArray buffer, int offset)
	{
		long long result = 0;

		if(!check_access(buffer, offset, 3, 1))
			return result;

		memcpy(&result, buffer.ptr + offset, sizeof(result));
		return result;
	}

	float read_float(NULLCArray buffer, int offset)
	{
		float result = 0;

		if(!check_access(buffer, offset, 2, 1))
			return result;

		memcpy(&result, buffer.ptr + offset, sizeof(result));
		return result;
	}

	double read_double(NULLCArray buffer, int offset)
	{
		double result = 0;

		if(!check_access(buffer, offset, 3, 1))
			return result;

		memcpy(&result, buffer.ptr + offset, sizeof(result));
		return result;
	}

	NULLCArray read_bool_array(NULLCArray buffer, int offset, int elements)
	{
		NULLCArray result = { 0, 0 };

		if(!check_access(buffer, offset, 0, elements))
			return result;

		result = nullcAllocateArrayTyped(NULLC_TYPE_BOOL, elements);

		memcpy(result.ptr, buffer.ptr + offset, sizeof(bool) * elements);
		return result;
	}

	NULLCArray read_char_array(NULLCArray buffer, int offset, int elements)
	{
		NULLCArray result = { 0, 0 };

		if(!check_access(buffer, offset, 0, elements))
			return result;

		result = nullcAllocateArrayTyped(NULLC_TYPE_CHAR, elements);

		memcpy(result.ptr, buffer.ptr + offset, sizeof(char) * elements);
		return result;
	}

	NULLCArray read_short_array(NULLCArray buffer, int offset, int elements)
	{
		NULLCArray result = { 0, 0 };

		if(!check_access(buffer, offset, 1, elements))
			return result;

		result = nullcAllocateArrayTyped(NULLC_TYPE_SHORT, elements);

		memcpy(result.ptr, buffer.ptr + offset, sizeof(short) * elements);
		return result;
	}

	NULLCArray read_int_array(NULLCArray buffer, int offset, int elements)
	{
		NULLCArray result = { 0, 0 };

		if(!check_access(buffer, offset, 2, elements))
			return result;

		result = nullcAllocateArrayTyped(NULLC_TYPE_INT, elements);

		memcpy(result.ptr, buffer.ptr + offset, sizeof(int) * elements);
		return result;
	}

	NULLCArray read_long_array(NULLCArray buffer, int offset, int elements)
	{
		NULLCArray result = { 0, 0 };

		if(!check_access(buffer, offset, 3, elements))
			return result;

		result = nullcAllocateArrayTyped(NULLC_TYPE_LONG, elements);

		memcpy(result.ptr, buffer.ptr + offset, sizeof(long long) * elements);
		return result;
	}

	NULLCArray read_float_array(NULLCArray buffer, int offset, int elements)
	{
		NULLCArray result = { 0, 0 };

		if(!check_access(buffer, offset, 2, elements))
			return result;

		result = nullcAllocateArrayTyped(NULLC_TYPE_FLOAT, elements);

		memcpy(result.ptr, buffer.ptr + offset, sizeof(float) * elements);
		return result;
	}

	NULLCArray read_double_array(NULLCArray buffer, int offset, int elements)
	{
		NULLCArray result = { 0, 0 };

		if(!check_access(buffer, offset, 3, elements))
			return result;

		result = nullcAllocateArrayTyped(NULLC_TYPE_DOUBLE, elements);

		memcpy(result.ptr, buffer.ptr + offset, sizeof(double) * elements);
		return result;
	}

	void read_into_bool(NULLCArray buffer, int offset, bool* result)
	{
		if(!check_access(buffer, offset, 0, 1))
			return;

		memcpy(result, buffer.ptr + offset, sizeof(*result));
	}

	void read_into_char(NULLCArray buffer, int offset, char* result)
	{
		if(!check_access(buffer, offset, 0, 1))
			return;

		memcpy(result, buffer.ptr + offset, sizeof(*result));
	}

	void read_into_short(NULLCArray buffer, int offset, short* result)
	{
		if(!check_access(buffer, offset, 1, 1))
			return;

		memcpy(result, buffer.ptr + offset, sizeof(*result));
	}

	void read_into_int(NULLCArray buffer, int offset, int* result)
	{
		if(!check_access(buffer, offset, 2, 1))
			return;

		memcpy(result, buffer.ptr + offset, sizeof(*result));
	}

	void read_into_long(NULLCArray buffer, int offset, long long* result)
	{
		if(!check_access(buffer, offset, 3, 1))
			return;

		memcpy(result, buffer.ptr + offset, sizeof(*result));
	}

	void read_into_float(NULLCArray buffer, int offset, float* result)
	{
		if(!check_access(buffer, offset, 2, 1))
			return;

		memcpy(result, buffer.ptr + offset, sizeof(*result));
	}

	void read_into_double(NULLCArray buffer, int offset, double* result)
	{
		if(!check_access(buffer, offset, 3, 1))
			return;

		memcpy(result, buffer.ptr + offset, sizeof(*result));
	}

	void read_into_bool_array(NULLCArray buffer, int offset, NULLCArray result)
	{
		if(!result.ptr)
		{
			nullcThrowError("buffer is empty");
			return;
		}

		if(!check_access(buffer, offset, 0, result.len))
			return;

		memcpy(result.ptr, buffer.ptr + offset, sizeof(bool) * result.len);
	}

	void read_into_char_array(NULLCArray buffer, int offset, NULLCArray result)
	{
		if(!result.ptr)
		{
			nullcThrowError("buffer is empty");
			return;
		}

		if(!check_access(buffer, offset, 0, result.len))
			return;

		memcpy(result.ptr, buffer.ptr + offset, sizeof(char) * result.len);
	}

	void read_into_short_array(NULLCArray buffer, int offset, NULLCArray result)
	{
		if(!result.ptr)
		{
			nullcThrowError("buffer is empty");
			return;
		}

		if(!check_access(buffer, offset, 1, result.len))
			return;

		memcpy(result.ptr, buffer.ptr + offset, sizeof(short) * result.len);
	}

	void read_into_int_array(NULLCArray buffer, int offset, NULLCArray result)
	{
		if(!result.ptr)
		{
			nullcThrowError("buffer is empty");
			return;
		}

		if(!check_access(buffer, offset, 2, result.len))
			return;

		memcpy(result.ptr, buffer.ptr + offset, sizeof(int) * result.len);
	}

	void read_into_long_array(NULLCArray buffer, int offset, NULLCArray result)
	{
		if(!result.ptr)
		{
			nullcThrowError("buffer is empty");
			return;
		}

		if(!check_access(buffer, offset, 3, result.len))
			return;

		memcpy(result.ptr, buffer.ptr + offset, sizeof(long long) * result.len);
	}

	void read_into_float_array(NULLCArray buffer, int offset, NULLCArray result)
	{
		if(!result.ptr)
		{
			nullcThrowError("buffer is empty");
			return;
		}

		if(!check_access(buffer, offset, 2, result.len))
			return;

		memcpy(result.ptr, buffer.ptr + offset, sizeof(float) * result.len);
	}

	void read_into_double_array(NULLCArray buffer, int offset, NULLCArray result)
	{
		if(!result.ptr)
		{
			nullcThrowError("buffer is empty");
			return;
		}

		if(!check_access(buffer, offset, 3, result.len))
			return;

		memcpy(result.ptr, buffer.ptr + offset, sizeof(double) * result.len);
	}

	void write_bool(NULLCArray buffer, int offset, bool value)
	{
		if(!check_access(buffer, offset, 0, 1))
			return;

		memcpy(buffer.ptr + offset, &value, sizeof(value));
	}

	void write_char(NULLCArray buffer, int offset, char value)
	{
		if(!check_access(buffer, offset, 0, 1))
			return;

		memcpy(buffer.ptr + offset, &value, sizeof(value));
	}

	void write_short(NULLCArray buffer, int offset, short value)
	{
		if(!check_access(buffer, offset, 1, 1))
			return;

		memcpy(buffer.ptr + offset, &value, sizeof(value));
	}

	void write_int(NULLCArray buffer, int offset, int value)
	{
		if(!check_access(buffer, offset, 2, 1))
			return;

		memcpy(buffer.ptr + offset, &value, sizeof(value));
	}

	void write_long(NULLCArray buffer, int offset, long long value)
	{
		if(!check_access(buffer, offset, 3, 1))
			return;

		memcpy(buffer.ptr + offset, &value, sizeof(value));
	}

	void write_float(NULLCArray buffer, int offset, float value)
	{
		if(!check_access(buffer, offset, 2, 1))
			return;

		memcpy(buffer.ptr + offset, &value, sizeof(value));
	}

	void write_double(NULLCArray buffer, int offset, double value)
	{
		if(!check_access(buffer, offset, 3, 1))
			return;

		memcpy(buffer.ptr + offset, &value, sizeof(value));
	}

	void write_bool_array(NULLCArray buffer, int offset, NULLCArray value)
	{
		if(!check_access(buffer, offset, 0, value.len))
			return;

		memcpy(buffer.ptr + offset, value.ptr, sizeof(bool) * value.len);
	}

	void write_char_array(NULLCArray buffer, int offset, NULLCArray value)
	{
		if(!check_access(buffer, offset, 0, value.len))
			return;

		memcpy(buffer.ptr + offset, value.ptr, sizeof(char) * value.len);
	}

	void write_short_array(NULLCArray buffer, int offset, NULLCArray value)
	{
		if(!check_access(buffer, offset, 1, value.len))
			return;

		memcpy(buffer.ptr + offset, value.ptr, sizeof(short) * value.len);
	}

	void write_int_array(NULLCArray buffer, int offset, NULLCArray value)
	{
		if(!check_access(buffer, offset, 2, value.len))
			return;

		memcpy(buffer.ptr + offset, value.ptr, sizeof(int) * value.len);
	}

	void write_long_array(NULLCArray buffer, int offset, NULLCArray value)
	{
		if(!check_access(buffer, offset, 3, value.len))
			return;

		memcpy(buffer.ptr + offset, value.ptr, sizeof(long long) * value.len);
	}

	void write_float_array(NULLCArray buffer, int offset, NULLCArray value)
	{
		if(!check_access(buffer, offset, 2, value.len))
			return;

		memcpy(buffer.ptr + offset, value.ptr, sizeof(float) * value.len);
	}

	void write_double_array(NULLCArray buffer, int offset, NULLCArray value)
	{
		if(!check_access(buffer, offset, 3, value.len))
			return;

		memcpy(buffer.ptr + offset, value.ptr, sizeof(double) * value.len);
	}

	void copy(NULLCArray dst, int dstOffset, NULLCArray src, int srcOffset, int size)
	{
		if(!check_access(dst, dstOffset, 0, size))
			return;

		if(!check_access(src, srcOffset, 0, size))
			return;

		memcpy(dst.ptr + dstOffset, src.ptr + srcOffset, size);
	}

	void set(NULLCArray dst, int dstOffset, char value, int size)
	{
		if(!check_access(dst, dstOffset, 0, size))
			return;

		memset(dst.ptr + dstOffset, value, size);
	}

	int compare(NULLCArray lhs, int lhsOffset, NULLCArray rhs, int rhsOffset, int size)
	{
		if(!check_access(lhs, lhsOffset, 0, size))
			return 0;

		if(!check_access(rhs, rhsOffset, 0, size))
			return 0;

		return memcmp(lhs.ptr + lhsOffset, rhs.ptr + rhsOffset, size);
	}

	float as_float(int value)
	{
		float result;
		memcpy(&result, &value, sizeof(value));
		return result;
	}

	double as_double(long long value)
	{
		double result;
		memcpy(&result, &value, sizeof(value));
		return result;
	}

	int as_int(float value)
	{
		int result;
		memcpy(&result, &value, sizeof(value));
		return result;
	}

	long long as_long(double value)
	{
		long long result;
		memcpy(&result, &value, sizeof(value));
		return result;
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("std.memory", NULLCMemory::funcPtr, name, index)) return false;

bool nullcInitMemoryModule()
{
	REGISTER_FUNC(read_bool, "memory.read_bool", 0);
	REGISTER_FUNC(read_char, "memory.read_char", 0);
	REGISTER_FUNC(read_short, "memory.read_short", 0);
	REGISTER_FUNC(read_int, "memory.read_int", 0);
	REGISTER_FUNC(read_long, "memory.read_long", 0);
	REGISTER_FUNC(read_float, "memory.read_float", 0);
	REGISTER_FUNC(read_double, "memory.read_double", 0);

	REGISTER_FUNC(read_bool_array, "memory.read_bool_array", 0);
	REGISTER_FUNC(read_char_array, "memory.read_char_array", 0);
	REGISTER_FUNC(read_short_array, "memory.read_short_array", 0);
	REGISTER_FUNC(read_int_array, "memory.read_int_array", 0);
	REGISTER_FUNC(read_long_array, "memory.read_long_array", 0);
	REGISTER_FUNC(read_float_array, "memory.read_float_array", 0);
	REGISTER_FUNC(read_double_array, "memory.read_double_array", 0);

	REGISTER_FUNC(read_into_bool, "memory.read", 0);
	REGISTER_FUNC(read_into_char, "memory.read", 1);
	REGISTER_FUNC(read_into_short, "memory.read", 2);
	REGISTER_FUNC(read_into_int, "memory.read", 3);
	REGISTER_FUNC(read_into_long, "memory.read", 4);
	REGISTER_FUNC(read_into_float, "memory.read", 5);
	REGISTER_FUNC(read_into_double, "memory.read", 6);

	REGISTER_FUNC(read_into_bool_array, "memory.read", 7);
	REGISTER_FUNC(read_into_char_array, "memory.read", 8);
	REGISTER_FUNC(read_into_short_array, "memory.read", 9);
	REGISTER_FUNC(read_into_int_array, "memory.read", 10);
	REGISTER_FUNC(read_into_long_array, "memory.read", 11);
	REGISTER_FUNC(read_into_float_array, "memory.read", 12);
	REGISTER_FUNC(read_into_double_array, "memory.read", 13);

	REGISTER_FUNC(write_bool, "memory.write", 0);
	REGISTER_FUNC(write_char, "memory.write", 1);
	REGISTER_FUNC(write_short, "memory.write", 2);
	REGISTER_FUNC(write_int, "memory.write", 3);
	REGISTER_FUNC(write_long, "memory.write", 4);
	REGISTER_FUNC(write_float, "memory.write", 5);
	REGISTER_FUNC(write_double, "memory.write", 6);

	REGISTER_FUNC(write_bool_array, "memory.write", 7);
	REGISTER_FUNC(write_char_array, "memory.write", 8);
	REGISTER_FUNC(write_short_array, "memory.write", 9);
	REGISTER_FUNC(write_int_array, "memory.write", 10);
	REGISTER_FUNC(write_long_array, "memory.write", 11);
	REGISTER_FUNC(write_float_array, "memory.write", 12);
	REGISTER_FUNC(write_double_array, "memory.write", 13);

	REGISTER_FUNC(copy, "memory.copy", 0);
	REGISTER_FUNC(set, "memory.set", 0);
	REGISTER_FUNC(compare, "memory.compare", 0);

	REGISTER_FUNC(as_float, "memory.as_float", 0);
	REGISTER_FUNC(as_double, "memory.as_double", 0);
	REGISTER_FUNC(as_int, "memory.as_int", 0);
	REGISTER_FUNC(as_long, "memory.as_long", 0);

	return true;
}
