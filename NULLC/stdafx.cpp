#include "stdafx.h"

char*		DuplicateString(const char *str)
{
	if(!str)
		return NULL;
	char* strDuplicate = new char[strlen(str) + 1];
	strcpy(strDuplicate, str);
	return strDuplicate;
}

unsigned int GetStringHash(const char *str)
{
	unsigned int hash = 5381;
	int c;
	while((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c;
	return hash;
}

unsigned int StringHashContinue(unsigned int hash, const char *str)
{
	int c;
	while((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c;
	return hash;
}
