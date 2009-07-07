#include "Linker.h"

#if defined(_MSC_VER)
bool Linker::CreateExternalInfo(ExternFuncInfo *fInfo, ExternalFunctionInfo& externalInfo)
{
	externalInfo.bytesToPop = 0;
	for(unsigned int i = 0; i < fInfo->paramCount; i++)
	{
		unsigned int paramSize = exTypes[fInfo->paramList[i]]->size > 4 ? exTypes[fInfo->paramList[i]]->size : 4;
		externalInfo.bytesToPop += paramSize;
	}
	
	return true;
}
#elif defined(__CELLOS_LV2__)
bool Linker::CreateExternalInfo(ExternFuncInfo *fInfo, ExternalFunctionInfo& externalInfo)
{
    unsigned int rCount = 0, fCount = 0;
    unsigned int rMaxCount = sizeof(externalInfo.rOffsets) / sizeof(externalInfo.rOffsets[0]);
    unsigned int fMaxCount = sizeof(externalInfo.fOffsets) / sizeof(externalInfo.fOffsets[0]);
    
    // parse all parameters, fill offsets
    unsigned int offset = 0;
    
	for (unsigned int i = 0; i < fInfo->paramCount; i++)
	{
	    const ExternTypeInfo& type = *exTypes[fInfo->paramList[i]];
	    
	    switch (type.type)
	    {
	    case ExternTypeInfo::TYPE_CHAR:
	    case ExternTypeInfo::TYPE_SHORT:
	    case ExternTypeInfo::TYPE_INT:
	        if (rCount >= rMaxCount) return false; // too many r parameters
	        externalInfo.rOffsets[rCount++] = offset;
	        offset++;
	        break;
	    
	    case ExternTypeInfo::TYPE_FLOAT:
	    case ExternTypeInfo::TYPE_DOUBLE:
	        if (fCount >= fMaxCount || rCount >= rMaxCount) return false; // too many f/r parameters
	        externalInfo.rOffsets[rCount++] = offset;
	        externalInfo.fOffsets[fCount++] = offset;
	        offset += 2;
	        break;
	        
	    default:
	        return false; // unsupported type
	    }
    }
    
    // clear remaining offsets
    for (unsigned int i = rCount; i < rMaxCount; ++i) externalInfo.rOffsets[i] = 0;
    for (unsigned int i = fCount; i < fMaxCount; ++i) externalInfo.fOffsets[i] = 0;
    
    return true;
}
#endif

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
	for(unsigned int i = 0; i < exTypes.size(); i++)
		delete[] (char*)exTypes[i];
	exTypes.clear();
	for(unsigned int i = 0; i < exVariables.size(); i++)
		delete[] (char*)exVariables[i];
	exVariables.clear();
	for(unsigned int i = 0; i < exFunctions.size(); i++)
		delete[] (char*)exFunctions[i];
	exFunctions.clear();
	exCode.clear();

	globalVarSize = 0;
	offsetToGlobalCode = 0;
}

bool Linker::LinkCode(const char *code, int redefinitions)
{
	linkError[0] = 0;

	FastVector<unsigned int>	typeRemap(50);
	FastVector<unsigned int>	funcRemap(50);

	ByteCode *bCode = (ByteCode*)code;

	typeRemap.clear();
	// Add all types from bytecode to the list
	ExternTypeInfo *tInfo = FindFirstType(bCode);
	for(unsigned int i = 0; i < bCode->typeCount; i++)
	{
	    const unsigned int index_none = ~0u;
	    
		unsigned int index = index_none;
		for(unsigned int n = 0; n < exTypes.size() && index == index_none; n++)
			if(strcmp(exTypes[n]->name, tInfo->name) == 0)
				index = n;

		if(index != index_none && exTypes[index]->size != tInfo->size)
		{
			sprintf(linkError, "Link Error: type '%s' is redefined with a different size", tInfo->name);
			return false;
		}
		if(index == index_none)
		{
			typeRemap.push_back(exTypes.size());
			exTypes.push_back((ExternTypeInfo*)(new char[tInfo->structSize/* + 16*/]));
			//memset(exTypes.back(), 0, tInfo->structSize+16);
			memcpy(exTypes.back(), tInfo, tInfo->structSize);
			exTypes.back()->name = (char*)(&exTypes.back()->name) + sizeof(exTypes.back()->name);
			exTypes.back()->next = NULL;	// no one cares
		}else{
			typeRemap.push_back(index);
		}

		tInfo = FindNextType(tInfo);
	}

	// Add all global variables
	ExternVarInfo *vInfo = FindFirstVar(bCode);
	for(unsigned int i = 0; i < bCode->variableCount; i++)
	{
		exVariables.push_back((ExternVarInfo*)(new char[vInfo->structSize]));
		memcpy(exVariables.back(), vInfo, vInfo->structSize);

		exVariables.back()->name = (char*)(&exVariables.back()->name) + sizeof(exVariables.back()->name);
		exVariables.back()->next = NULL;	// no one cares
		// Type index have to be updated
		exVariables.back()->type = typeRemap[vInfo->type];

		vInfo = FindNextVar(vInfo);
	}

	unsigned int oldGlobalSize = globalVarSize;
	globalVarSize += bCode->globalVarSize;

	unsigned int	oldExFuncSize = exFunctions.size();
	unsigned int	oldOffsetToGlobalCode = offsetToGlobalCode;
	funcRemap.clear();
	unsigned int	allFunctionSize = 0;
	// Add new functions
	ExternFuncInfo *fInfo = FindFirstFunc(bCode);
	for(unsigned int i = 0; i < bCode->functionCount; i++, fInfo = FindNextFunc(fInfo))
	{
		allFunctionSize += fInfo->codeSize;
		
        const unsigned int index_none = ~0u;
        
		unsigned int index = index_none;
		for(unsigned int n = 0; n < exFunctions.size() && index == index_none; n++)
			if(strcmp(exFunctions[n]->name, fInfo->name) == 0)
				index = n;

		// Suppose, this is an overload function
		bool isOverload = true;
		if(index != index_none)
		{
			for(unsigned int n = 0; n < exFunctions.size() && isOverload == true; n++)
			{
				if(strcmp(exFunctions[n]->name, fInfo->name) == 0)
				{
					if(fInfo->paramCount == exFunctions[n]->paramCount)
					{
						// If the parameter count is equal, test parameters
						unsigned int *paramList = (unsigned int*)((char*)(&fInfo->name) + sizeof(fInfo->name) + fInfo->nameLength + 1);
						unsigned int k;
						for(k = 0; k < fInfo->paramCount; k++)
							if(typeRemap[paramList[k]] != exFunctions[n]->paramList[k])
								break;
						if(k == fInfo->paramCount)
						{
							isOverload = false;
							index = n;
						}
					}
				}
			}
			if(!exFunctions[index]->isVisible)
				isOverload = true;
		}
		// If the function exists and is build-in or external, skip
		if(index != index_none && !isOverload && exFunctions[index]->address == -1)
		{
			funcRemap.push_back(index);
			continue;
		}
		// If the function exists and is internal, check if redefinition is allowed
		if(index != index_none && !isOverload)
		{
			if(redefinitions)
			{
				sprintf(linkError, "Warning: function '%s' is redefined", fInfo->name);
			}else{
				sprintf(linkError, "Link Error: function '%s' is redefined", fInfo->name);
				return false;
			}
		}
		if(index == index_none || isOverload)
		{
			funcRemap.push_back(exFunctions.size());

			exFunctions.push_back((ExternFuncInfo*)(new char[fInfo->structSize]));
			memcpy(exFunctions.back(), fInfo, fInfo->structSize);

			exFunctions.back()->name = (char*)(&exFunctions.back()->name) + sizeof(exFunctions.back()->name);
			exFunctions.back()->next = NULL;	// no one cares
			exFunctions.back()->paramList = (unsigned int*)((char*)(&exFunctions.back()->name) + sizeof(fInfo->name) + fInfo->nameLength + 1);

			// Type index have to be updated
			exFunctions.back()->retType = typeRemap[exFunctions.back()->retType];
			if(exFunctions.back()->funcPtr != NULL && exTypes[exFunctions.back()->retType]->size > 8 && exTypes[exFunctions.back()->retType]->type != ExternTypeInfo::TYPE_DOUBLE)
			{
				strcpy(linkError, "ERROR: user functions with return type size larger than 4 bytes are not supported");
				return false;
			}
			for(unsigned int n = 0; n < exFunctions.back()->paramCount; n++)
				exFunctions.back()->paramList[n] = typeRemap[((unsigned int*)((char*)(&fInfo->name) + sizeof(fInfo->name) + fInfo->nameLength + 1))[n]];

			// Add function code to the bytecode,
			// shifting global code forward,
			// fixing all jump addresses in global code

			if(exFunctions.back()->address != -1)
			{
				unsigned int shift = fInfo->codeSize;
				unsigned int oldCodeSize = exCode.size();
				// Expand bytecode
				exCode.resize(exCode.size() + shift);
				// Move global code
				if(oldCodeSize-offsetToGlobalCode > 0)
					memmove(&exCode[offsetToGlobalCode] + shift, &exCode[offsetToGlobalCode], (oldCodeSize-offsetToGlobalCode) * sizeof(VMCmd));
				// Insert function code
				memcpy(&exCode[offsetToGlobalCode], FindCode(bCode) + fInfo->address * sizeof(VMCmd), shift * sizeof(VMCmd));
				// Update function position
			
				exFunctions.back()->address = offsetToGlobalCode;
				// Update global code start
				offsetToGlobalCode += shift;
			}
		}else{
			funcRemap.push_back(index);
			assert(!"No function rewrite at the moment");
		}
	}

	for(unsigned int i = oldExFuncSize; i < exFunctions.size(); i++)
	{
		if(exFunctions[i]->address != -1)
		{
			// Fix cmdJmp*, cmdCall, cmdCallStd and commands with absolute addressing in function code
			int pos = exFunctions[i]->address;
			while(pos < exFunctions[i]->address + exFunctions[i]->codeSize)
			{
				VMCmd &cmd = exCode[pos];
				pos++;
				switch(cmd.cmd)
				{
				case cmdPushCharAbs:
				case cmdPushShortAbs:
				case cmdPushIntAbs:
				case cmdPushFloatAbs:
				case cmdPushDorLAbs:
				case cmdPushCmplxAbs:
				case cmdMovCharAbs:
				case cmdMovShortAbs:
				case cmdMovIntAbs:
				case cmdMovFloatAbs:
				case cmdMovDorLAbs:
				case cmdMovCmplxAbs:
					cmd.argument += oldGlobalSize;
					break;
				case cmdJmp:
				case cmdJmpZI:
				case cmdJmpZD:
				case cmdJmpZL:
				case cmdJmpNZI:
				case cmdJmpNZD:
				case cmdJmpNZL:
					cmd.argument += exFunctions[i]->address - exFunctions[i]->oldAddress;
					break;
				case cmdCall:
					if(cmd.argument != CALL_BY_POINTER)
						cmd.argument = exFunctions[funcRemap[cmd.argument]]->address;
					break;
				case cmdCallStd:
					cmd.argument = funcRemap[cmd.argument];
					break;
				case cmdFuncAddr:
					cmd.argument = funcRemap[cmd.argument];
					break;
				}
			}
		}
	}

	unsigned int oldCodeSize = exCode.size();
	// Fix cmdJmp* in old code
	unsigned int pos = offsetToGlobalCode;
	while(pos < oldCodeSize)
	{
		VMCmd &cmd = exCode[pos];
		pos++;
		switch(cmd.cmd)
		{
		case cmdJmp:
		case cmdJmpZI:
		case cmdJmpZD:
		case cmdJmpZL:
		case cmdJmpNZI:
		case cmdJmpNZD:
		case cmdJmpNZL:
			cmd.argument += offsetToGlobalCode - oldOffsetToGlobalCode;
		}
	}

	// Add new global code

	// Expand bytecode
	exCode.resize(exCode.size() + bCode->codeSize - allFunctionSize);
	// Insert function code
	memcpy(&exCode[oldCodeSize], FindCode(bCode) + allFunctionSize * sizeof(VMCmd), (bCode->codeSize - allFunctionSize) * sizeof(VMCmd));

//	exCode.resize(bCode->codeSize);
//	memcpy(&exCode[0], FindCode(bCode), bCode->codeSize * sizeof(VMCmd));

	// Fix cmdJmp*, cmdCall, cmdCallStd and commands with absolute addressing in new code
	pos = oldCodeSize;
//	unsigned int pos = 0;
	while(pos < exCode.size())
	{
		VMCmd &cmd = exCode[pos];
		pos++;
		switch(cmd.cmd)
		{
		case cmdPushCharAbs:
		case cmdPushShortAbs:
		case cmdPushIntAbs:
		case cmdPushFloatAbs:
		case cmdPushDorLAbs:
		case cmdPushCmplxAbs:
		case cmdMovCharAbs:
		case cmdMovShortAbs:
		case cmdMovIntAbs:
		case cmdMovFloatAbs:
		case cmdMovDorLAbs:
		case cmdMovCmplxAbs:
			cmd.argument += oldGlobalSize;
			break;
		case cmdJmp:
		case cmdJmpZI:
		case cmdJmpZD:
		case cmdJmpZL:
		case cmdJmpNZI:
		case cmdJmpNZD:
		case cmdJmpNZL:
			cmd.argument += oldCodeSize - allFunctionSize;
			break;
		case cmdCall:
			if(cmd.argument != CALL_BY_POINTER)
				cmd.argument = exFunctions[funcRemap[cmd.argument]]->address;
			break;
		case cmdCallStd:
			cmd.argument = funcRemap[cmd.argument];
			break;
		case cmdFuncAddr:
			cmd.argument = funcRemap[cmd.argument];
			break;
		}
	}

	exFuncInfo.resize(exFunctions.size());
	for(unsigned int n = 0; n < exFunctions.size(); n++)
	{
		ExternFuncInfo* funcInfoPtr = exFunctions[n];

		if(funcInfoPtr->funcPtr && !CreateExternalInfo(funcInfoPtr, exFuncInfo[n]))
		{
			sprintf(linkError, "Link Error: External function info failed for '%s'", funcInfoPtr->name);
			return false;
		}
	}

#ifdef NULLC_LOG_FILES
	FILE *linkAsm = fopen("link.txt", "wb");
	char instBuf[128];
	for(unsigned int i = 0; i < exCode.size(); i++)
	{
		exCode[i].Decode(instBuf);
		fprintf(linkAsm, "%s\r\n", instBuf);
	}
	fclose(linkAsm);
#endif

	return true;
}

const char*	Linker::GetLinkError()
{
	return linkError;
}