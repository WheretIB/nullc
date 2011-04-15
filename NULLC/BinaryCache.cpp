#include "BinaryCache.h"
#include "Lexer.h"

namespace BinaryCache
{
	FastVector<CodeDescriptor>	cache;
	const char*	importPath = NULL;

	unsigned int	lastReserved = 0;
	char*			lastBytecode = NULL;
	const unsigned int	lastHash = GetStringHash("__last.nc");
}

void BinaryCache::SetImportPath(const char* path)
{
	importPath = path;
}

const char* BinaryCache::GetImportPath()
{
	return importPath;
}

void BinaryCache::Initialize()
{
	importPath = NULL;
	lastReserved = 0;
	lastBytecode = NULL;
}

void BinaryCache::Terminate()
{
	for(unsigned int i = 0; i < cache.size(); i++)
	{
		NULLC::dealloc((void*)cache[i].name);
		delete[] cache[i].binary;
		delete[] cache[i].lexemes;
	}
	cache.clear();
	cache.reset();

	delete[] lastBytecode;
	lastBytecode = NULL;
}

void BinaryCache::PutBytecode(const char* path, const char* bytecode, Lexeme* lexStart, unsigned lexCount)
{
	unsigned int hash = GetStringHash(path);
	unsigned int i = 0;
	for(; i < cache.size(); i++)
	{
		if(hash == cache[i].nameHash)
			break;
	}
	assert(i == cache.size());

	BinaryCache::CodeDescriptor *desc = cache.push_back();
	unsigned int pathLen = (unsigned int)strlen(path);
	desc->name = strcpy((char*)NULLC::alloc(pathLen + 1), path);
	desc->nameHash = hash;
	desc->binary = bytecode;
	if(lexStart)
	{
		desc->lexemes = new Lexeme[lexCount];
		memcpy(desc->lexemes, lexStart, lexCount * sizeof(Lexeme));
		desc->lexemeCount = lexCount;
	}else{
		desc->lexemes = NULL;
		desc->lexemeCount = 0;
	}
}

const char* BinaryCache::GetBytecode(const char* path)
{
	unsigned int hash = GetStringHash(path);
	if(hash == lastHash)
		return lastBytecode;

	unsigned int i = 0;
	for(; i < cache.size(); i++)
	{
		if(hash == cache[i].nameHash)
			break;
	}
	if(i != cache.size())
		return cache[i].binary;

	return NULL;
}

Lexeme* BinaryCache::GetLexems(const char* path, unsigned& count)
{
	unsigned int hash = GetStringHash(path);
	for(unsigned int i = 0; i < cache.size(); i++)
	{
		if(hash == cache[i].nameHash)
		{
			count = cache[i].lexemeCount;
			return cache[i].lexemes;
		}
	}
	return NULL;
}

void BinaryCache::RemoveBytecode(const char* path)
{
	unsigned int hash = GetStringHash(path);
	unsigned int i = 0;
	for(; i < cache.size(); i++)
	{
		if(hash == cache[i].nameHash)
			break;
	}
	if(i == cache.size())
		return;

	NULLC::dealloc((void*)cache[i].name);
	delete[] cache[i].binary;
	delete[] cache[i].lexemes;

	cache[i] = cache.back();
	cache.pop_back();
}

const char* BinaryCache::EnumerateModules(unsigned id)
{
	if(id >= cache.size())
		return NULL;
	return cache[id].name;
}

void BinaryCache::LastBytecode(const char* bytecode)
{
	unsigned int size = *(unsigned int*)bytecode;
	if(size > lastReserved)
	{
		delete[] lastBytecode;
		lastReserved = size + (size >> 1);
		lastBytecode = new char[lastReserved];
	}
	memcpy(lastBytecode, bytecode, size);
}
