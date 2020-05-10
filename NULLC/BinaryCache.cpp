#include "BinaryCache.h"

#include "Lexer.h"

namespace BinaryCache
{
	FastVector<CodeDescriptor> cache;
	FastVector<char*> importPaths;

	unsigned int	lastReserved = 0;
	char*			lastBytecode = NULL;
	const unsigned int	lastHash = NULLC::GetStringHash("__last.nc");
}

void BinaryCache::ClearImportPaths()
{
	for(unsigned int i = 0; i < importPaths.size(); i++)
		NULLC::dealloc(importPaths[i]);
	importPaths.clear();
}

void BinaryCache::AddImportPath(const char* path)
{
	for(unsigned int i = 0; i < importPaths.size(); i++)
	{
		if(strcmp(importPaths[i], path) == 0)
			return;
	}

	char *importPath = (char*)NULLC::alloc(int(strlen(path)) + 1);
	strcpy(importPath, path);
	importPaths.push_back(importPath);
}

void BinaryCache::RemoveImportPath(const char* path)
{
	for(unsigned int i = 0; i < importPaths.size(); i++)
	{
		if(strcmp(importPaths[i], path) == 0)
		{
			importPaths[i] = importPaths.back();
			importPaths.pop_back();
			return;
		}
	}
}

bool BinaryCache::HasImportPath(const char* path)
{
	for(unsigned int i = 0; i < importPaths.size(); i++)
	{
		if(strcmp(importPaths[i], path) == 0)
			return true;
	}

	return false;
}

const char* BinaryCache::EnumImportPath(unsigned pos)
{
	return pos < importPaths.size() ? importPaths[pos] : NULL;
}

void BinaryCache::Initialize()
{
	lastReserved = 0;
	lastBytecode = NULL;
}

void BinaryCache::Terminate()
{
	ClearImportPaths();

	importPaths.reset();

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
	unsigned int hash = NULLC::GetStringHash(path);

	if(hash == lastHash)
		return;

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

void BinaryCache::PutLexemes(const char* path, Lexeme* lexStart, unsigned lexCount)
{
	unsigned int hash = NULLC::GetStringHash(path);

	if(hash == lastHash)
		return;

	for(unsigned i = 0; i < cache.size(); i++)
	{
		BinaryCache::CodeDescriptor &desc = cache[i];

		if(hash == cache[i].nameHash)
		{
			assert(!cache[i].lexemes);

			desc.lexemes = new Lexeme[lexCount];
			memcpy(desc.lexemes, lexStart, lexCount * sizeof(Lexeme));
			desc.lexemeCount = lexCount;

			return;
		}
	}

	assert(!"module not found");
}

const char* BinaryCache::GetBytecode(const char* path)
{
	unsigned int hash = NULLC::GetStringHash(path);
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

const char*	BinaryCache::FindBytecode(const char* moduleName, bool addExtension)
{
	if(!addExtension)
	{
		unsigned int hash = NULLC::GetStringHash(moduleName);

		if(hash == lastHash)
			return lastBytecode;
	}

	if(addExtension)
		assert(strstr(moduleName, ".nc") == 0);
	else
		assert(strstr(moduleName, ".nc") != 0);

	const unsigned pathLength = 1024;
	char path[pathLength];

	unsigned modulePathPos = 0;
	while(const char *modulePath = BinaryCache::EnumImportPath(modulePathPos++))
	{
		char *pathEnd = path + NULLC::SafeSprintf(path, pathLength, "%s%s", modulePath, moduleName);

		if(addExtension)
		{
			char *pathNoImport = path + strlen(modulePath);

			for(unsigned i = 0, e = (unsigned)strlen(pathNoImport); i != e; i++)
			{
				if(pathNoImport[i] == '.')
					pathNoImport[i] = '/';
			}

			NULLC::SafeSprintf(pathEnd, pathLength - int(pathEnd - path), ".nc");
		}

		if(const char *bytecode = BinaryCache::GetBytecode(path))
			return bytecode;
	}

	return NULL;
}

Lexeme* BinaryCache::GetLexems(const char* path, unsigned& count)
{
	unsigned int hash = NULLC::GetStringHash(path);
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

Lexeme* BinaryCache::FindLexems(const char* moduleName, bool addExtension, unsigned& count)
{
	if(addExtension)
		assert(strstr(moduleName, ".nc") == 0);
	else
		assert(strstr(moduleName, ".nc") != 0);

	const unsigned pathLength = 1024;
	char path[pathLength];

	unsigned modulePathPos = 0;
	while(const char *modulePath = BinaryCache::EnumImportPath(modulePathPos++))
	{
		char *pathEnd = path + NULLC::SafeSprintf(path, pathLength, "%s%s", modulePath, moduleName);

		if(addExtension)
		{
			char *pathNoImport = path + strlen(modulePath);

			for(unsigned i = 0, e = (unsigned)strlen(pathNoImport); i != e; i++)
			{
				if(pathNoImport[i] == '.')
					pathNoImport[i] = '/';
			}

			NULLC::SafeSprintf(pathEnd, pathLength - int(pathEnd - path), ".nc");
		}

		if(Lexeme *lexems = BinaryCache::GetLexems(path, count))
			return lexems;
	}

	return NULL;
}

void BinaryCache::RemoveBytecode(const char* path)
{
	unsigned int hash = NULLC::GetStringHash(path);
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
