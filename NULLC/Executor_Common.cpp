#include "Executor_Common.h"

#include "CodeInfo.h"

namespace NULLC
{
	Linker *commonLinker = NULL;
}

void CommonSetLinker(Linker* linker)
{
	NULLC::commonLinker = linker;
}

void ClosureCreate(char* paramBase, unsigned int helper, unsigned int argument, ExternFuncInfo::Upvalue* closure)
{
	/*ExternFuncInfo &func = NULLC::commonLinker->exFunctions[argument];
	ExternLocalInfo *externals = &NULLC::commonLinker->exLocals[func.offsetToFirstLocal + func.localCount];
	for(unsigned int i = 0; i < func.externCount; i++)
	{
		ExternFuncInfo *varParent = &NULLC::commonLinker->exFunctions[externals[i].closeFuncList & ~0x80000000];
		if(externals[i].closeFuncList & 0x80000000)
		{
			closure[0] = (unsigned int)(intptr_t)(externals[i].target + paramBase + NULLC::parameterHead);
		}else{
			unsigned int **prevClosure = (unsigned int**)(NULLC::parameterHead + helper + paramBase);

			closure[0] = (*prevClosure)[externals[i].target >> 2];
		}
		closure[1] = (unsigned int)(intptr_t)varParent->externalList;
		closure[2] = externals[i].size;
		varParent->externalList = (ExternFuncInfo::Upvalue*)closure;
		closure += (externals[i].size >> 2) < 3 ? 3 : (externals[i].size >> 2);
	}*/
	ExternFuncInfo &func = NULLC::commonLinker->exFunctions[argument];
	ExternLocalInfo *externals = &NULLC::commonLinker->exLocals[func.offsetToFirstLocal + func.localCount];
	for(unsigned int i = 0; i < func.externCount; i++)
	{
		ExternFuncInfo *varParent = &NULLC::commonLinker->exFunctions[externals[i].closeFuncList & ~0x80000000];
		if(externals[i].closeFuncList & 0x80000000)
		{
			closure->ptr = (unsigned int*)&paramBase[externals[i].target];
		}else{
			unsigned int *prevClosure = (unsigned int*)(intptr_t)*(int*)(&paramBase[helper]);
			closure->ptr = (unsigned int*)(intptr_t)prevClosure[externals[i].target >> 2];
		}
		closure->next = varParent->externalList;
		closure->size = externals[i].size;
		varParent->externalList = closure;
		closure = (ExternFuncInfo::Upvalue*)((int*)closure + ((externals[i].size >> 2) < 3 ? 3 : (externals[i].size >> 2)));
	}
}

void CloseUpvalues(char* paramBase, unsigned int helper, unsigned int argument)
{
	ExternFuncInfo &func = NULLC::commonLinker->exFunctions[helper];
	ExternFuncInfo::Upvalue *curr = func.externalList, *prev = NULL;
	while(curr && (char*)curr->ptr >= paramBase)
	{
		ExternFuncInfo::Upvalue *next = curr->next;
		unsigned int size = curr->size;

		// Close only in part of scope
		if((char*)curr->ptr >= (paramBase + argument))
		{
			// delete from list
			if(prev)
				prev->next = curr->next;
			else
				func.externalList = curr->next;

			memcpy(&curr->next, curr->ptr, size);
			curr->ptr = (unsigned int*)&curr->next;
		}else{
			prev = curr;
		}
		curr = next;
	}
}


unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize)
{
	const char *start = current;

	FastVector<ExternFuncInfo> &exFunctions = NULLC::commonLinker->exFunctions;
	FastVector<ExternLocalInfo> &exLocals = NULLC::commonLinker->exLocals;
	FastVector<ExternTypeInfo> &exTypes = NULLC::commonLinker->exTypes;
	FastVector<char> &exSymbols = NULLC::commonLinker->exSymbols;

	int funcID = -1;
	for(unsigned int i = 0; i < exFunctions.size(); i++)
		if(address >= exFunctions[i].address && address <= (exFunctions[i].address + exFunctions[i].codeSize))
			funcID = i;
	if(funcID != -1)
		current += SafeSprintf(current, bufSize - int(current - start), "%s", &exSymbols[exFunctions[funcID].offsetToName]);
	else
		current += SafeSprintf(current, bufSize - int(current - start), "%s", address == -1 ? "external" : "global scope");
	if(address != -1)
	{
		unsigned int line = 0;
		unsigned int i = address - 1;
		while((line < CodeInfo::cmdInfoList.sourceInfo.size() - 1) && (i >= CodeInfo::cmdInfoList.sourceInfo[line + 1].byteCodePos))
				line++;
		const char *codeStart = CodeInfo::cmdInfoList.sourceInfo[line].sourcePos;
		while(*codeStart == ' ' || *codeStart == '\t')
			codeStart++;
		int codeLength = (int)(CodeInfo::cmdInfoList.sourceInfo[line].sourceEnd - codeStart) - 1;
		current += SafeSprintf(current, bufSize - int(current - start), " (at %.*s)\r\n", codeLength, codeStart);
	}
#ifdef NULLC_STACK_TRACE_WITH_LOCALS
	if(funcID != -1)
	{
		for(unsigned int i = 0; i < exFunctions[funcID].localCount + exFunctions[funcID].externCount; i++)
		{
			ExternLocalInfo &lInfo = exLocals[exFunctions[funcID].offsetToFirstLocal + i];
			const char *typeName = &exSymbols[exTypes[lInfo.type].offsetToName];
			const char *localName = &exSymbols[lInfo.offsetToName];
			const char *localType = lInfo.paramType == ExternLocalInfo::PARAMETER ? "param" : (lInfo.paramType == ExternLocalInfo::EXTERNAL ? "extern" : "local");
			const char *offsetType = (lInfo.paramType == ExternLocalInfo::PARAMETER || lInfo.paramType == ExternLocalInfo::LOCAL) ? "base" :
				(lInfo.closeFuncList & 0x80000000 ? "local" : "closure");
			current += SafeSprintf(current, bufSize - int(current - start), " %s %d: %s %s (at %s+%d size %d)\r\n",
				localType, i, typeName, localName, offsetType, lInfo.offset, exTypes[lInfo.type].size);
		}
	}
#endif
	return (unsigned int)(current - start);
}