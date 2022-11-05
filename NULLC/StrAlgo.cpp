#include "StrAlgo.h"

unsigned int NULLC::GetStringHash(const char *str)
{
	unsigned int hash = 5381;
	int c;
	while((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c;
	return hash;
}

unsigned int NULLC::GetStringHash(const char *str, const char *end)
{
	unsigned int hash = 5381;
	while(str != end)
		hash = ((hash << 5) + hash) + (*str++);
	return hash;
}

unsigned int NULLC::StringHashContinue(unsigned int hash, const char *str)
{
	int c;
	while((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c;
	return hash;
}

unsigned int NULLC::StringHashContinue(unsigned int hash, const char *str, const char *end)
{
	while(str != end)
		hash = ((hash << 5) + hash) + (*str++);
	return hash;
}

int	NULLC::SafeSprintf(char* dst, size_t size, const char* src, ...)
{
	if(size == 0)
		return 0;

	va_list args;
	va_start(args, src);

	int result = vsnprintf(dst, size, src, args);
	dst[size - 1] = '\0';

	va_end(args);

	return (result == -1 || (size_t)result >= size) ? (int)size : result;
}
