#include "BinaryCache.h"

namespace BinaryCache
{
	FastVector<CodeDescriptor>	cache;
}

void BinaryCache::Initialize()
{
}

void BinaryCache::Terminate()
{
	for(unsigned int i = 0; i < cache.size(); i++)
	{
		delete[] cache[i].name;
		delete[] cache[i].binary;
	}
	cache.clear();
}

char* BinaryCache::GetBytecode(const char* path)
{
	unsigned int hash = GetStringHash(path);
	unsigned int i = 0;
	for(; i < cache.size(); i++)
	{
		if(hash == cache[i].nameHash)
			break;
	}
	if(i != cache.size())
		return cache[i].binary;

	BinaryCache::CodeDescriptor *desc = cache.push_back();
	unsigned int pathLen = (unsigned int)strlen(path);
	desc->name = strcpy((char*)NULLC::alloc(pathLen + 1), path);
	desc->nameHash = hash;

	if(FILE *module = fopen(path, "rb"))
	{
		fseek(module, 0, SEEK_END);
		unsigned int bcSize = ftell(module);
		fseek(module, 0, SEEK_SET);
		desc->binary = new char[bcSize];
		fread(desc->binary, 1, bcSize, module);
		fclose(module);

		return desc->binary;
	}

	return NULL;
}