#include "stdafx.h"

unsigned int GetStringHash(const char *str)
{
	unsigned int hash = 5381;
	int c;
	while((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c;
	return hash;
}

unsigned int GetStringHash(const char *str, const char *end)
{
	unsigned int hash = 5381;
	int c;
	while((c = *str++) != 0 && str != end)
		hash = ((hash << 5) + hash) + c;
	return hash;
}
