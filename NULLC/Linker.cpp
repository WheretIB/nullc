#include "Linker.h"

Linker::Linker(): exTypes(50), exVariables(50), exFunctions(50)
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
	exVariables.clear();
	exFunctions.clear();
	exCode.clear();

	globalVarSize = 0;
	offsetToGlobalCode = 0;
}

FastVector<unsigned int>	typeRemap(50);
FastVector<unsigned int>	funcRemap(50);

bool Linker::LinkCode(const char *code, int redefinitions)
{
	linkError[0] = 0;

	ByteCode *bCode = (ByteCode*)code;

	typeRemap.clear();

	unsigned int oldTypeCount = exTypes.size();

	// Add all types from bytecode to the list
	ExternTypeInfo *tInfo = FindFirstType(bCode);
	for(unsigned int i = 0; i < bCode->typeCount; i++)
	{
		const unsigned int index_none = ~0u;

		unsigned int index = index_none;
		for(unsigned int n = 0; n < oldTypeCount && index == index_none; n++)
			if(exTypes[n].nameHash == tInfo->nameHash)
				index = n;

		if(index != index_none && exTypes[index].size != tInfo->size)
		{
			sprintf(linkError, "Link Error: type #%d is redefined with a different size", i);
			return false;
		}
		if(index == index_none)
		{
			typeRemap.push_back(exTypes.size());
			exTypes.push_back(*tInfo);
		}else{
			typeRemap.push_back(index);
		}

		tInfo++;
	}

	// Add all global variables
	ExternVarInfo *vInfo = FindFirstVar(bCode);
	for(unsigned int i = 0; i < bCode->variableCount; i++)
	{
		exVariables.push_back(*vInfo);
		// Type index have to be updated
		exVariables.back().type = typeRemap[vInfo->type];

		vInfo++;
	}

	unsigned int oldGlobalSize = globalVarSize;
	globalVarSize += bCode->globalVarSize;

	funcRemap.clear();
	unsigned int oldFunctionCount = exFunctions.size();

	// Add new code
	unsigned int oldCodeSize = exCode.size();
	exCode.resize(oldCodeSize + bCode->codeSize);
	memcpy(&exCode[oldCodeSize], FindCode(bCode), bCode->codeSize * sizeof(VMCmd));

	// Add new functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	for(unsigned int i = 0; i < bCode->functionCount; i++, fInfo++)
	{
		const unsigned int index_none = ~0u;

		unsigned int index = index_none;
		for(unsigned int n = 0; n < oldFunctionCount && index == index_none; n++)
			if(fInfo->isVisible && exFunctions[n].nameHash == fInfo->nameHash && exFunctions[n].funcType == fInfo->funcType)
				index = n;

		// If the function exists and is build-in or external, skip
		if(index != index_none && exFunctions[index].address == -1)
		{
			funcRemap.push_back(index);
			continue;
		}
		// If the function exists and is internal, check if redefinition is allowed
		if(index != index_none)
		{
			if(redefinitions)
			{
				sprintf(linkError, "Warning: function #%d is redefined", i);
			}else{
				sprintf(linkError, "Link Error: function #%d is redefined", i);
				return false;
			}
		}
		if(index == index_none)
		{
			funcRemap.push_back(exFunctions.size());
			exFunctions.push_back(*fInfo);

			if(exFunctions.back().funcPtr != NULL && exFunctions.back().retType == ExternFuncInfo::RETURN_UNKNOWN)
			{
				strcpy(linkError, "ERROR: user functions with return type size larger than 8 bytes are not supported");
				return false;
			}
#if defined(__CELLOS_LV2__)
			if(!exFunctions.back().ps3Callable)
			{
				sprintf(linkError, "Link Error: External function #%d is not callable on PS3", i);
				return false;
			}
#endif
			if(exFunctions.back().address == 0)
			{
				sprintf(linkError, "Link Error: External function #%d doesn't have implementation", i);
				return false;
			}
			// Update internal function address
			if(exFunctions.back().address != -1)
				exFunctions.back().address = oldCodeSize + fInfo->address;
		}else{
			funcRemap.push_back(index);
			assert(!"No function rewrite at the moment");
		}
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
			if(cmd.argument != CALL_BY_POINTER)
				cmd.argument = exFunctions[funcRemap[cmd.argument]].address;
			break;
		case cmdCallStd:
			cmd.argument = funcRemap[cmd.argument];
			break;
		case cmdFuncAddr:
			cmd.argument = funcRemap[cmd.argument];
			break;
		}
	}

#ifdef NULLC_LOG_FILES
	FILE *linkAsm = fopen("link.txt", "wb");
	char instBuf[128];
	for(unsigned int i = 0; i < exCode.size(); i++)
	{
		exCode[i].Decode(instBuf);
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
