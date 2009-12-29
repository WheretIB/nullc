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

	struct SourceInfo
	{
		unsigned int byteCodePos;
		unsigned int sourceOffset;
	};

	SourceInfo *exInfo = (SourceInfo*)&NULLC::commonLinker->exCodeInfo[0];
	const char *source = &NULLC::commonLinker->exSource[0];
	unsigned int infoSize = NULLC::commonLinker->exCodeInfo.size() / 2;

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
		while((line < infoSize - 1) && (i >= exInfo[line + 1].byteCodePos))
				line++;
		const char *codeStart = source + exInfo[line].sourceOffset;
		// Find beginning of the line
		while(codeStart != source && *(codeStart-1) != '\n')
			codeStart--;
		// Skip whitespace
		while(*codeStart == ' ' || *codeStart == '\t')
			codeStart++;
		const char *codeEnd = codeStart;
		// Find ending of the line
		while(*codeEnd != '\0' && *codeEnd != '\r' && *codeEnd != '\n')
			codeEnd++;
		int codeLength = (int)(codeEnd - codeStart);
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

NullCArray NULLCTypeInfo::Typename(NULLCRef r)
{
	NullCArray ret;
	FastVector<ExternTypeInfo> &exTypes = NULLC::commonLinker->exTypes;
	char *symbols = &NULLC::commonLinker->exSymbols[0];

	ret.ptr = exTypes[r.typeID].offsetToName + symbols;
	ret.len = (unsigned int)strlen(ret.ptr) + 1;
	return ret;
}
