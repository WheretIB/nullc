#include "Linker.h"
#include "StdLib.h"
#include "BinaryCache.h"

Linker::Linker(): exTypes(128), exTypeExtra(256), exVariables(128), exFunctions(256), exSymbols(8192), exLocals(1024), jumpTargets(1024)
{
	globalVarSize = 0;
	offsetToGlobalCode = 0;

	typeMap.init();
	funcMap.init();

	NULLC::SetLinker(this);
}

Linker::~Linker()
{
	CleanCode();
}

void Linker::CleanCode()
{
	exTypes.clear();
	exTypeExtra.clear();
	exVariables.clear();
	exFunctions.clear();
	exCode.clear();
	exSymbols.clear();
	exLocals.clear();
	exModules.clear();
	exCodeInfo.clear();
	exSource.clear();
	exCloseLists.clear();

	jumpTargets.clear();

	functionAddress.clear();

	globalVarSize = 0;
	offsetToGlobalCode = 0;

	typeRemap.clear();
	funcRemap.clear();
	moduleRemap.clear();

	funcMap.clear();

	NULLC::ClearMemory();
}

bool Linker::LinkCode(const char *code)
{
	linkError[0] = 0;

	ByteCode *bCode = (ByteCode*)code;

	ExternTypeInfo *tInfo = FindFirstType(bCode), *tStart = tInfo;
	unsigned int *memberList = (unsigned int*)(tInfo + bCode->typeCount);

	unsigned int moduleFuncCount = 0;

	ExternModuleInfo *mInfo = (ExternModuleInfo*)((char*)(bCode) + bCode->offsetToFirstModule);
	for(unsigned int i = 0; i < bCode->dependsCount; i++)
	{
		const char *path = (char*)(bCode) + bCode->offsetToSymbols + mInfo->nameOffset;

		//Search for it in loaded modules
		int loadedId = -1;
		for(unsigned int n = 0; n < exModules.size(); n++)
		{
			if(exModules[n].nameHash == GetStringHash(path))
			{
				loadedId = n;
				break;
			}
		}
		if(loadedId == -1)
		{
			char fullPath[256];
			SafeSprintf(fullPath, 256, "%s%s", BinaryCache::GetImportPath() ? BinaryCache::GetImportPath() : "", path);

			const char *bytecode = BinaryCache::GetBytecode(fullPath);
			if(!bytecode && BinaryCache::GetImportPath())
				bytecode = BinaryCache::GetBytecode(path);

			// last module is not imported
			if(strcmp(path, "__last.nc") != 0)
			{
				if(bytecode)
				{
#ifdef VERBOSE_DEBUG_OUTPUT
					printf("Linking %s.\r\n", path);
#endif
					if(!LinkCode(bytecode))
					{
						SafeSprintf(linkError + strlen(linkError), LINK_ERROR_BUFFER_SIZE - strlen(linkError), "\r\nLink Error: failed to load module %s (ports %d-%d)", path, mInfo->funcStart, mInfo->funcStart + mInfo->funcCount - 1);
						return false;
					}
				}else{
					SafeSprintf(linkError + strlen(linkError), LINK_ERROR_BUFFER_SIZE - strlen(linkError), "\r\nFailed to load module %s", path);
					return false;
				}
			}
			exModules.push_back(*mInfo);
			exModules.back().name = path;
			exModules.back().nameOffset = 0;
			exModules.back().nameHash = GetStringHash(path);
			exModules.back().funcStart = exFunctions.size() - mInfo->funcCount;
			exModules.back().variableOffset = globalVarSize - ((ByteCode*)bytecode)->globalVarSize;
			exModules.back().sourceOffset = exSource.size() - ((ByteCode*)bytecode)->sourceSize;
			exModules.back().sourceSize = ((ByteCode*)bytecode)->sourceSize;
#ifdef VERBOSE_DEBUG_OUTPUT
			printf("Module %s variables are found at %d (size is %d).\r\n", path, exModules.back().variableOffset, ((ByteCode*)bytecode)->globalVarSize);
#endif
			loadedId = exModules.size() - 1;
		}
		moduleFuncCount += mInfo->funcCount;
		mInfo++;
	}

#ifdef LINK_VERBOSE_DEBUG_OUTPUT
		printf("Function remap table is extended to %d functions (%d modules, %d new)\r\n", bCode->functionCount, moduleFuncCount, bCode->functionCount - moduleFuncCount);
#endif
	funcRemap.resize(bCode->functionCount);
	for(unsigned int i = moduleFuncCount; i < bCode->functionCount; i++)
		funcRemap[i] = (exFunctions.size() ? exFunctions.size() - moduleFuncCount : 0) + i;

	moduleRemap.resize(bCode->dependsCount);

	unsigned int oldFunctionCount = exFunctions.size();
	unsigned int oldSymbolSize = exSymbols.size();
	unsigned int oldTypeCount = exTypes.size();
	unsigned int oldMemberSize = exTypeExtra.size();

	mInfo = (ExternModuleInfo*)((char*)(bCode) + bCode->offsetToFirstModule);
	// Fixup function table
	for(unsigned int i = 0; i < bCode->dependsCount; i++)
	{
		const char *path = (char*)(bCode) + bCode->offsetToSymbols + mInfo->nameOffset;
		//Search for it in loaded modules
		int loadedId = -1;
		for(unsigned int n = 0; n < exModules.size(); n++)
		{
			if(exModules[n].nameHash == GetStringHash(path))
			{
				loadedId = n;
				break;
			}
		}
		ExternModuleInfo *rInfo = &exModules[loadedId];
		for(unsigned int n = mInfo->funcStart; n < mInfo->funcStart + mInfo->funcCount; n++)
			funcRemap[n] = rInfo->funcStart + n - mInfo->funcStart;
		if(!rInfo->nameOffset)
			rInfo->nameOffset = mInfo->nameOffset + oldSymbolSize;

		moduleRemap[i] = loadedId;
#ifdef VERBOSE_DEBUG_OUTPUT
		printf("Module %d (%s) is found at index %d.\r\n", i, path, loadedId);
#endif
		mInfo++;
	}

	typeRemap.clear();

	// Add new symbols
	exSymbols.resize(oldSymbolSize + bCode->symbolLength);
	memcpy(&exSymbols[oldSymbolSize], (char*)(bCode) + bCode->offsetToSymbols, bCode->symbolLength);
	const char *symbolInfo = (char*)(bCode) + bCode->offsetToSymbols;

	// Create type map for fast searches
	typeMap.clear();
	for(unsigned int i = 0; i < oldTypeCount; i++)
		typeMap.insert(exTypes[i].nameHash, i);

	// Add all types from bytecode to the list
	tInfo = tStart;
	for(unsigned int i = 0; i < bCode->typeCount; i++)
	{
		unsigned int *lastType = typeMap.find(tInfo->nameHash);

		if(lastType && exTypes[*lastType].size != tInfo->size)
		{
			SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: type %s is redefined (%s) with a different size (%d != %d)", exTypes[*lastType].offsetToName + &exSymbols[0], tInfo->offsetToName + symbolInfo, exTypes[*lastType].size, tInfo->size);
			return false;
		}
		if(!lastType)
		{
			typeRemap.push_back(exTypes.size());
			exTypes.push_back(*tInfo);
			exTypes.back().offsetToName += oldSymbolSize;
			
			if(exTypes.back().subCat == ExternTypeInfo::CAT_ARRAY || exTypes.back().subCat == ExternTypeInfo::CAT_POINTER)
				exTypes.back().subType = typeRemap[exTypes.back().subType];
			if(tInfo->subCat == ExternTypeInfo::CAT_FUNCTION || tInfo->subCat == ExternTypeInfo::CAT_CLASS)
			{
				exTypes.back().memberOffset = exTypeExtra.size();
				exTypeExtra.push_back(memberList + tInfo->memberOffset, tInfo->memberCount + (tInfo->subCat == ExternTypeInfo::CAT_FUNCTION ? 1 : 0));
				// If class has some members with pointers
				if(tInfo->subCat == ExternTypeInfo::CAT_CLASS && tInfo->pointerCount)
				{
					// Then, after member types there's a list of members with pointers in a form of struct{ member type, member offset }, which we have to save
					unsigned int count = exTypeExtra.size();
					exTypeExtra.push_back(memberList + tInfo->memberOffset + tInfo->memberCount, tInfo->pointerCount * 2);
					// Mark member offsets by setting a bit, so that offsets won't be remapped using typeRemap array (during type ID fixup)
					for(unsigned int k = count + 1; k < exTypeExtra.size(); k += 2)
						exTypeExtra[k] |= 0x80000000;
				}
			}
		}else{
			typeRemap.push_back(*lastType);
		}

		tInfo++;
	}

	// Remap new member types (while skipping member offsets)
	for(unsigned int i = oldMemberSize; i < exTypeExtra.size(); i++)
		exTypeExtra[i] = (exTypeExtra[i] & 0x80000000) ? (exTypeExtra[i] & ~0x80000000) : typeRemap[exTypeExtra[i]];

#ifdef VERBOSE_DEBUG_OUTPUT
	printf("Global variable size is %d, starting from %d.\r\n", bCode->globalVarSize, globalVarSize);
#endif

	unsigned int oldGlobalSize = globalVarSize;
	globalVarSize += bCode->globalVarSize;

	// Add all global variables
	ExternVarInfo *vInfo = FindFirstVar(bCode);
	for(unsigned int i = 0; i < bCode->variableCount; i++)
	{
		exVariables.push_back(*vInfo);
		// Type index have to be updated
		exVariables.back().type = typeRemap[vInfo->type];
		exVariables.back().offsetToName += oldSymbolSize;
		exVariables.back().offset += oldGlobalSize;
#ifdef VERBOSE_DEBUG_OUTPUT
		printf("Variable %s %s at %d\r\n", &exSymbols[0] + exTypes[exVariables.back().type].offsetToName, &exSymbols[0] + exVariables.back().offsetToName, exVariables.back().offset);
#endif
		vInfo++;
	}

	// Add new locals
	unsigned int oldLocalsSize = exLocals.size();
	exLocals.resize(oldLocalsSize + bCode->localCount);
	memcpy(exLocals.data + oldLocalsSize, (char*)(bCode) + bCode->offsetToLocals, bCode->localCount * sizeof(ExternLocalInfo));

	// Add new code information
	unsigned int oldCodeInfoSize = exCodeInfo.size();
	exCodeInfo.resize(oldCodeInfoSize + bCode->infoSize * 2);
	memcpy(exCodeInfo.data + oldCodeInfoSize, (char*)(bCode) + bCode->offsetToInfo, bCode->infoSize * sizeof(unsigned int) * 2);

	// Add new source code
	unsigned int oldSourceSize = exSource.size();
	exSource.resize(oldSourceSize + bCode->sourceSize);
	memcpy(exSource.data + oldSourceSize, (char*)(bCode) + bCode->offsetToSource, bCode->sourceSize);

	// Add new code
	unsigned int oldCodeSize = exCode.size();
	exCode.reserve(oldCodeSize + bCode->codeSize + 1);
	exCode.resize(oldCodeSize + bCode->codeSize);
	memcpy(exCode.data + oldCodeSize, FindCode(bCode), bCode->codeSize * sizeof(VMCmd));

	for(unsigned int i = oldCodeInfoSize / 2; i < exCodeInfo.size() / 2; i++)
	{
		exCodeInfo[i*2+0] += oldCodeSize;
		exCodeInfo[i*2+1] += oldSourceSize;
	}

	unsigned int oldListCount = exCloseLists.size();
	exCloseLists.resize(oldListCount + bCode->closureListCount);
	memset(exCloseLists.data + oldListCount, 0, bCode->closureListCount * sizeof(ExternFuncInfo::Upvalue*));

	// Add new functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	unsigned int end = bCode->functionCount - bCode->moduleFunctionCount;
	for(unsigned int i = 0; i < end; i++, fInfo++)
	{
		const unsigned int index_none = ~0u;

		unsigned int index = index_none;
		if(fInfo->isVisible)
		{
			unsigned int remappedType = typeRemap[fInfo->funcType];
			HashMap<unsigned int>::Node *curr = funcMap.first(fInfo->nameHash);
			while(curr)
			{
				if(curr->value < oldFunctionCount && exFunctions[curr->value].funcType == remappedType)
				{
					index = curr->value;
					break;
				}
				curr = funcMap.next(curr);
			}
		}

		// If the function exists and is build-in or external, skip
		if(index != index_none && exFunctions[index].address == -1)
			continue;
		// If the function exists and is internal, check if redefinition is allowed
		if(index != index_none)
		{
			if(*(symbolInfo + fInfo->offsetToName) == '$')
			{
				exFunctions.push_back(exFunctions[index]);
				funcMap.insert(exFunctions.back().nameHash, exFunctions.size()-1);
				continue;
			}else{
				SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: function '%s' is redefined", symbolInfo + fInfo->offsetToName);
				return false;
			}
		}
		if(index == index_none)
		{
			exFunctions.push_back(*fInfo);
			funcMap.insert(exFunctions.back().nameHash, exFunctions.size()-1);

#if !defined(_M_X64) && !defined(NULLC_COMPLEX_RETURN)
			if(exFunctions.back().funcPtr != NULL && exFunctions.back().retType == ExternFuncInfo::RETURN_UNKNOWN)
			{
				strcpy(linkError, "ERROR: user functions with return type size larger than 8 bytes are not supported");
				return false;
			}
#endif
#if defined(__CELLOS_LV2__)
			if(exFunctions.back().funcPtr != NULL && !exFunctions.back().ps3Callable)
			{
				SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: External function '%s' is not callable on PS3", (char*)(bCode) + bCode->offsetToSymbols + exFunctions.back().offsetToName);
				return false;
			}
#endif
			if(exFunctions.back().address == 0)
			{
				SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: External function '%s' '%s' doesn't have implementation", (char*)(bCode) + bCode->offsetToSymbols + exFunctions.back().offsetToName, &exSymbols[0] + exTypes[exFunctions.back().funcType].offsetToName);
				return false;
			}
			// Move based pointer to the new section of symbol information
			exFunctions.back().offsetToName += oldSymbolSize;
			exFunctions.back().offsetToFirstLocal += oldLocalsSize;
			exFunctions.back().closeListStart += oldListCount;
			exFunctions.back().funcType = typeRemap[exFunctions.back().funcType];

			if(exFunctions.back().parentType != ~0u)
				exFunctions.back().parentType = typeRemap[exFunctions.back().parentType];

			// Update internal function address
			if(exFunctions.back().address != -1)
			{
				exFunctions.back().address = oldCodeSize + fInfo->address;
				jumpTargets.push_back(exFunctions.back().address);
			}

#ifdef LINK_VERBOSE_DEBUG_OUTPUT
			printf("Adding function %-16s (at address %4d [external %p])\r\n", &exSymbols[0] + exFunctions.back().offsetToName, exFunctions.back().address, exFunctions.back().funcPtr);
			printf("Closure list start: %d\r\n", exFunctions.back().closeListStart);
#endif
		}
	}

	for(unsigned int i = oldLocalsSize; i < oldLocalsSize + bCode->localCount; i++)
	{
		exLocals[i].type = typeRemap[exLocals[i].type];
		exLocals[i].offsetToName += oldSymbolSize;
		if(exLocals[i].paramType == ExternLocalInfo::EXTERNAL)
			exLocals[i].closeListID += oldListCount;
	}

	// Fix cmdJmp*, cmdCall, cmdCallStd and commands with absolute addressing in new code
	unsigned int pos = oldCodeSize;
	while(pos < exCode.size())
	{
		VMCmd &cmd = exCode[pos];
		pos++;
		switch(cmd.cmd)
		{
		case cmdPushChar:
		case cmdPushShort:
		case cmdPushInt:
		case cmdPushFloat:
		case cmdPushDorL:
		case cmdPushCmplx:
		case cmdMovChar:
		case cmdMovShort:
		case cmdMovInt:
		case cmdMovFloat:
		case cmdMovDorL:
		case cmdMovCmplx:
			if(cmd.flag == ADDRESS_ABOLUTE)
			{
				if(cmd.argument >> 24)
					cmd.argument = (cmd.argument & 0x00ffffff) + exModules[moduleRemap[(cmd.argument >> 24) - 1]].variableOffset;
				else
					cmd.argument += oldGlobalSize;
			}
			break;
		case cmdGetAddr:
			if(cmd.helper == ADDRESS_ABOLUTE)
			{
				if(cmd.argument >> 24)
					cmd.argument = (cmd.argument & 0x00ffffff) + exModules[moduleRemap[(cmd.argument >> 24) - 1]].variableOffset;
				else
					cmd.argument += oldGlobalSize;
			}
			break;
		case cmdSetRange:
			if(cmd.flag == ADDRESS_ABOLUTE)
				cmd.argument += oldGlobalSize;
			break;
		case cmdJmp:
		case cmdJmpZ:
		case cmdJmpNZ:
			cmd.argument += oldCodeSize;
			jumpTargets.push_back(cmd.argument);
			break;
		case cmdFuncAddr:
			cmd.cmd = cmdPushImmt;
			cmd.argument = funcRemap[cmd.argument];
			break;
		case cmdCall:
		case cmdCreateClosure:
			cmd.argument = funcRemap[cmd.argument];
			break;
		case cmdCloseUpvals:
			cmd.argument += exFunctions[funcRemap[cmd.helper]].closeListStart;
			break;
		case cmdPushTypeID:
			cmd.cmd = cmdPushImmt;
			cmd.argument = typeRemap[cmd.argument];
			break;
		case cmdConvertPtr:
			cmd.argument = typeRemap[cmd.argument];
			break;
#ifdef _M_X64
		case cmdPushPtr:
			cmd.cmd = cmdPushDorL;
			break;
		case cmdPushPtrStk:
			cmd.cmd = cmdPushDorLStk;
			break;
#else
		case cmdPushPtr:
			cmd.cmd = cmdPushInt;
			break;
		case cmdPushPtrStk:
			cmd.cmd = cmdPushIntStk;
			break;
		case cmdPushPtrImmt:
			cmd.cmd = cmdPushImmt;
			break;
#endif
		}
	}

#ifdef VERBOSE_DEBUG_OUTPUT
	unsigned int size = 0;
	printf("Data managed by linker.\r\n");
	printf("Types: %db, ", exTypes.size() * sizeof(ExternTypeInfo));
	size += exTypes.size() * sizeof(ExternTypeInfo);
	printf("Variables: %db, ", exVariables.size() * sizeof(ExternVarInfo));
	size += exVariables.size() * sizeof(ExternVarInfo);
	printf("Functions: %db, ", exFunctions.size() * sizeof(ExternFuncInfo));
	size += exFunctions.size() * sizeof(ExternFuncInfo);
	printf("Code: %db\r\n", exCode.size() * sizeof(VMCmd));
	size += exCode.size() * sizeof(VMCmd);
	printf("Symbols: %db, ", exSymbols.size() * sizeof(char));
	size += exSymbols.size() * sizeof(char);
	printf("Locals: %db, ", exLocals.size() * sizeof(ExternLocalInfo));
	size += exLocals.size() * sizeof(ExternLocalInfo);
	printf("Modules: %db, ", exModules.size() * sizeof(ExternModuleInfo));
	size += exModules.size() * sizeof(ExternModuleInfo);
	printf("Code info: %db, ", exCodeInfo.size() * sizeof(unsigned int) * 2);
	size += exCodeInfo.size() * sizeof(unsigned int) * 2;
	printf("Source: %db\r\n", exSource.size() * sizeof(char));
	size += exSource.size() * sizeof(char);
	printf("Overall: %d bytes\r\n\r\n", size);
#endif

#ifdef NULLC_LOG_FILES
	FILE *linkAsm = fopen("link.txt", "wb");
	char instBuf[128];
	unsigned int line = 0, lastLine = ~0u;

	struct SourceInfo
	{
		unsigned int byteCodePos;
		unsigned int sourceOffset;
	};

	SourceInfo *info = (SourceInfo*)&exCodeInfo[0];
	unsigned int infoSize = exCodeInfo.size() / 2;

	const char *lastSourcePos = &exSource[0];
	for(unsigned int i = 0; infoSize && i < exCode.size(); i++)
	{
		while((line < infoSize - 1) && (i >= info[line + 1].byteCodePos))
			line++;
		if(line != lastLine)
		{
			lastLine = line;
			const char *codeStart = &exSource[0] + info[line].sourceOffset;
			// Find beginning of the line
			while(codeStart != &exSource[0] && *(codeStart-1) != '\n')
				codeStart--;
			// Skip whitespace
			while(*codeStart == ' ' || *codeStart == '\t')
				codeStart++;
			const char *codeEnd = codeStart;
			while(*codeEnd != '\0' && *codeEnd != '\r' && *codeEnd != '\n')
				codeEnd++;
			if(codeEnd > lastSourcePos)
			{
				fprintf(linkAsm, "%.*s\r\n", codeEnd - lastSourcePos, lastSourcePos);
				lastSourcePos = codeEnd;
			}else{
				fprintf(linkAsm, "%.*s\r\n", codeEnd - codeStart, codeStart);
			}
		}
		exCode[i].Decode(instBuf);
		if(exCode[i].cmd == cmdCall)
			fprintf(linkAsm, "// %d %s (%s)\r\n", i, instBuf, &exSymbols[exFunctions[exCode[i].argument].offsetToName]);
		else
			fprintf(linkAsm, "// %d %s\r\n", i, instBuf);
	}
	fclose(linkAsm);
#endif

	return true;
}

const char*	Linker::GetLinkError()
{
	return linkError;
}
