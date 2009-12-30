#include "Linker.h"
#include "StdLib.h"
#include "BinaryCache.h"

Linker::Linker(): exTypes(64), exVariables(64), exFunctions(64), exSymbols(4096), exLocals(64)
{
	globalVarSize = 0;
	offsetToGlobalCode = 0;
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

	globalVarSize = 0;
	offsetToGlobalCode = 0;

	typeRemap.clear();
	funcRemap.clear();

	NULLC::ClearMemory();
}

bool Linker::LinkCode(const char *code, int redefinitions)
{
	linkError[0] = 0;

	ByteCode *bCode = (ByteCode*)code;

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

			char *bytecode = BinaryCache::GetBytecode(fullPath);
			if(!bytecode && BinaryCache::GetImportPath())
				bytecode = BinaryCache::GetBytecode(path);

			if(bytecode)
			{
#ifdef VERBOSE_DEBUG_OUTPUT
				printf("Linking %s.\r\n", path);
#endif
				if(!LinkCode(bytecode, false))
				{
					SafeSprintf(linkError + strlen(linkError), LINK_ERROR_BUFFER_SIZE - strlen(linkError), "\r\nLink Error: failed to load module %s (ports %d-%d)", path, mInfo->funcStart, mInfo->funcStart + mInfo->funcCount - 1);
					return false;
				}
			}else{
				SafeSprintf(linkError + strlen(linkError), LINK_ERROR_BUFFER_SIZE - strlen(linkError), "\r\nFailed to load module %s", path);
				return false;
			}
			exModules.push_back(*mInfo);
			exModules.back().name = NULL;
			exModules.back().nameHash = GetStringHash(path);
			exModules.back().funcStart = exFunctions.size() - mInfo->funcCount;
			loadedId = exModules.size() - 1;
		}
		moduleFuncCount += mInfo->funcCount;
		mInfo++;
	}

#ifdef LINK_VERBOSE_DEBUG_OUTPUT
		printf("Function remap table is extended to %d functions (%d base, %d modules, %d new)\r\n", bCode->functionCount, bCode->externalFunctionCount, moduleFuncCount, bCode->functionCount-(bCode->externalFunctionCount + moduleFuncCount));
#endif
	funcRemap.resize(bCode->functionCount);
	for(unsigned int i = 0; i < bCode->externalFunctionCount; i++)
		funcRemap[i] = i;
	for(unsigned int i = bCode->externalFunctionCount + moduleFuncCount; i < bCode->functionCount; i++)
		funcRemap[i] = (exFunctions.size() ? exFunctions.size() - (bCode->externalFunctionCount + moduleFuncCount) : 0) + i;

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
		mInfo++;
	}

	typeRemap.clear();

	unsigned int oldFunctionCount = exFunctions.size();
	unsigned int oldSymbolSize = exSymbols.size();
	unsigned int oldTypeCount = exTypes.size();
	unsigned int oldMemberSize = exTypeExtra.size();

	// Add all types from bytecode to the list
	ExternTypeInfo *tInfo = FindFirstType(bCode);
	unsigned int *memberList = (unsigned int*)(tInfo + bCode->typeCount);
	for(unsigned int i = 0; i < bCode->typeCount; i++)
	{
		const unsigned int index_none = ~0u;

		unsigned int index = index_none;
		for(unsigned int n = 0; n < oldTypeCount && index == index_none; n++)
			if(exTypes[n].nameHash == tInfo->nameHash)
				index = n;

		if(index != index_none && exTypes[index].size != tInfo->size)
		{
			SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: type #%d is redefined with a different size", i);
			return false;
		}
		if(index == index_none)
		{
			typeRemap.push_back(exTypes.size());
			exTypes.push_back(*tInfo);
			exTypes.back().offsetToName += oldSymbolSize;
			if(tInfo->subCat == ExternTypeInfo::CAT_FUNCTION || tInfo->subCat == ExternTypeInfo::CAT_CLASS)
			{
				exTypes.back().memberOffset = exTypeExtra.size();
				exTypeExtra.push_back(memberList + tInfo->memberOffset, tInfo->memberCount);
			}
		}else{
			typeRemap.push_back(index);
		}

		tInfo++;
	}

	// Remap new member types
	for(unsigned int i = oldMemberSize; i < exTypeExtra.size(); i++)
		exTypeExtra[i] = typeRemap[exTypeExtra[i]];

	// Add all global variables
	ExternVarInfo *vInfo = FindFirstVar(bCode);
	for(unsigned int i = 0; i < bCode->variableCount; i++)
	{
		exVariables.push_back(*vInfo);
		// Type index have to be updated
		exVariables.back().type = typeRemap[vInfo->type];
		exVariables.back().offsetToName += oldSymbolSize;

		vInfo++;
	}

	unsigned int oldGlobalSize = globalVarSize;
	globalVarSize += bCode->globalVarSize;

	// Add new symbols
	exSymbols.resize(oldSymbolSize + bCode->symbolLength);
	memcpy(&exSymbols[oldSymbolSize], (char*)(bCode) + bCode->offsetToSymbols, bCode->symbolLength);
	const char *symbolInfo = (char*)(bCode) + bCode->offsetToSymbols;

	// Add new locals
	unsigned int oldLocalsSize = exLocals.size();
	exLocals.resize(oldLocalsSize + bCode->localCount);
	memcpy(&exLocals[oldLocalsSize], (char*)(bCode) + bCode->offsetToLocals, bCode->localCount * sizeof(ExternLocalInfo));

	// Add new code information
	unsigned int oldCodeInfoSize = exCodeInfo.size();
	exCodeInfo.resize(oldCodeInfoSize + bCode->infoSize * 2);
	memcpy(&exCodeInfo[oldCodeInfoSize], (char*)(bCode) + bCode->offsetToInfo, bCode->infoSize * sizeof(unsigned int) * 2);

	// Add new source code
	unsigned int oldSourceSize = exSource.size();
	exSource.resize(oldSourceSize + bCode->sourceSize);
	memcpy(&exSource[oldSourceSize], (char*)(bCode) + bCode->offsetToSource, bCode->sourceSize);

	// Add new code
	unsigned int oldCodeSize = exCode.size();
	exCode.resize(oldCodeSize + bCode->codeSize);
	memcpy(&exCode[oldCodeSize], FindCode(bCode), bCode->codeSize * sizeof(VMCmd));

	for(unsigned int i = oldCodeInfoSize / 2; i < exCodeInfo.size() / 2; i++)
	{
		exCodeInfo[i*2+0] += oldCodeSize;
		exCodeInfo[i*2+1] += oldSourceSize;
	}

	// Add new functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	unsigned int end = bCode->moduleFunctionCount ? bCode->functionCount - (bCode->moduleFunctionCount + bCode->externalFunctionCount) : bCode->functionCount;
	for(unsigned int i = 0; i < end; i++, fInfo++)
	{
		const unsigned int index_none = ~0u;

		unsigned int index = index_none;
		for(unsigned int n = 0; n < oldFunctionCount && index == index_none; n++)
			if(fInfo->isVisible && exFunctions[n].nameHash == fInfo->nameHash && exFunctions[n].funcType == typeRemap[fInfo->funcType])
				index = n;

		// If the function exists and is build-in or external, skip
		if(index != index_none && exFunctions[index].address == -1)
			continue;
		// If the function exists and is internal, check if redefinition is allowed
		if(index != index_none)
		{
			if(redefinitions)
			{
				SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Warning: function '%s' is redefined", symbolInfo + fInfo->offsetToName);
			}else{
				SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: function '%s' is redefined", symbolInfo + fInfo->offsetToName);
				return false;
			}
		}
		if(index == index_none)
		{
			exFunctions.push_back(*fInfo);

			if(exFunctions.back().funcPtr != NULL && exFunctions.back().retType == ExternFuncInfo::RETURN_UNKNOWN)
			{
				strcpy(linkError, "ERROR: user functions with return type size larger than 8 bytes are not supported");
				return false;
			}
#if defined(__CELLOS_LV2__)
			if(!exFunctions.back().ps3Callable)
			{
				SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: External function '%s' is not callable on PS3", (char*)(bCode) + bCode->offsetToSymbols + exFunctions.back().offsetToName);
				return false;
			}
#endif
			if(exFunctions.back().address == 0)
			{
				SafeSprintf(linkError, LINK_ERROR_BUFFER_SIZE, "Link Error: External function '%s' doesn't have implementation", (char*)(bCode) + bCode->offsetToSymbols + exFunctions.back().offsetToName);
				return false;
			}
			// Move based pointer to the new section of symbol information
			exFunctions.back().offsetToName += oldSymbolSize;
			exFunctions.back().offsetToFirstLocal += oldLocalsSize;
			exFunctions.back().externalList = NULL;

			// Update internal function address
			if(exFunctions.back().address != -1)
				exFunctions.back().address = oldCodeSize + fInfo->address;

#ifdef LINK_VERBOSE_DEBUG_OUTPUT
			printf("Adding function %-16s (at address %4d [external %p])\r\n", &exSymbols[0] + exFunctions.back().offsetToName, exFunctions.back().address, exFunctions.back().funcPtr);
#endif
		}else{
			assert(!"No function rewrite at the moment");
		}
	}

	for(unsigned int i = oldLocalsSize; i < oldLocalsSize + bCode->localCount; i++)
	{
		exLocals[i].type = typeRemap[exLocals[i].type];
		exLocals[i].offsetToName += oldSymbolSize;
		if(exLocals[i].paramType == ExternLocalInfo::EXTERNAL)
			exLocals[i].closeFuncList = funcRemap[exLocals[i].closeFuncList & ~0x80000000] | (exLocals[i].closeFuncList & 0x80000000);
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
				cmd.argument += oldGlobalSize;
			break;
		case cmdGetAddr:
			if(cmd.helper == ADDRESS_ABOLUTE)
				cmd.argument += oldGlobalSize;
			break;
		case cmdJmp:
		case cmdJmpZ:
		case cmdJmpNZ:
			cmd.argument += oldCodeSize;
			break;
		case cmdCall:
		case cmdFuncAddr:
		case cmdCreateClosure:
			cmd.argument = funcRemap[cmd.argument];
			break;
		case cmdCloseUpvals:
			cmd.helper = (unsigned short)funcRemap[cmd.helper];
			break;
		case cmdPushTypeID:
			cmd.cmd = cmdPushImmt;
			cmd.argument = typeRemap[cmd.argument];
			break;
		case cmdConvertPtr:
			cmd.argument = typeRemap[cmd.argument];
			break;
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
	for(unsigned int i = 0; i < exCode.size(); i++)
	{
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
