#include "Executor_Common.h"

#include "CodeInfo.h"
#include "StdLib.h"
#include "nullc.h"
#include "Executor.h"
#include "Executor_X86.h"

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
	//printf("searching after %d, fixing after %d\r\n", paramBase, paramBase + argument);
	while(curr && (char*)curr->ptr >= paramBase)
	{
		ExternFuncInfo::Upvalue *next = curr->next;
		unsigned int size = curr->size;
		
		// Close only in part of scope
		if((char*)curr->ptr >= (paramBase + argument))
		{
			//printf("fix\r\n");
			// delete from list
			if(prev)
				prev->next = curr->next;
			else
				func.externalList = curr->next;

			memcpy(&curr->next, curr->ptr, size);
			curr->ptr = (unsigned int*)&curr->next;
		}else{
			//printf("skip\r\n");
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

namespace GC
{
	char	*unmanagableBase = 0;
	char	*unmanagableTop = NULL;
	unsigned int	markCalls = 0, canMark = 0, markedCount = 0;

	void CheckArray(char* ptr, const ExternTypeInfo& type);
	void CheckClass(char* ptr, const ExternTypeInfo& type);
	void CheckVariable(char* ptr, const ExternTypeInfo& type);

	void MarkPointer(char* ptr, const ExternTypeInfo& type)
	{
		markCalls++;
		char **rPtr = (char**)ptr;
		if(*rPtr > (char*)0x00010000)
		{
			if(*rPtr < unmanagableBase || *rPtr > unmanagableTop)
			{
				canMark++;
				ExternTypeInfo &subType = NULLC::commonLinker->exTypes[type.subType];
				//printf("\tGlobal pointer %s %p (at %p)\r\n", NULLC::commonLinker->exSymbols.data + type.offsetToName, *rPtr, ptr);
				unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(*rPtr);
				if(!basePtr)
					return;
				//printf("\tPointer base is %p\r\n", basePtr);
				unsigned int *marker = (unsigned int*)(basePtr)-1;
				//printf("\tMarker is %d\r\n", *marker);
				if(*marker == 0)// != checkedMarker)
				{
					markedCount++;
					*marker = *marker + 1;// = checkedMarker;
					if(type.subCat != ExternTypeInfo::CAT_NONE)
						CheckVariable(*rPtr, subType);
				}
			}
		}
	}

	void CheckArray(char* ptr, const ExternTypeInfo& type)
	{
		ExternTypeInfo &subType = NULLC::commonLinker->exTypes[type.subType];
		if(type.arrSize == -1)
		{
			MarkPointer(ptr, subType);
			return;
		}
		switch(subType.subCat)
		{
		case ExternTypeInfo::CAT_ARRAY:
			for(unsigned int i = 0; i < type.arrSize; i++, ptr += subType.size)
				CheckArray(ptr, subType);
			break;
		case ExternTypeInfo::CAT_POINTER:
			for(unsigned int i = 0; i < type.arrSize; i++, ptr += subType.size)
				MarkPointer(ptr, subType);
			break;
		case ExternTypeInfo::CAT_CLASS:
			for(unsigned int i = 0; i < type.arrSize; i++, ptr += subType.size)
				CheckClass(ptr, subType);
			break;
		}
	}

	void CheckClass(char* ptr, const ExternTypeInfo& type)
	{
		unsigned int *memberList = &NULLC::commonLinker->exTypeExtra[0];
		for(unsigned int n = 0; n < type.memberCount; n++)
		{
			ExternTypeInfo &subType = NULLC::commonLinker->exTypes[memberList[type.memberOffset + n]];
			CheckVariable(ptr, subType);
			ptr += subType.size;
		}
	}

	void CheckVariable(char* ptr, const ExternTypeInfo& type)
	{
		switch(type.subCat)
		{
		case ExternTypeInfo::CAT_ARRAY:
			CheckArray(ptr, type);
			break;
		case ExternTypeInfo::CAT_POINTER:
			MarkPointer(ptr, type);
			break;
		case ExternTypeInfo::CAT_CLASS:
			CheckVariable(ptr, type);
			break;
		}
	}
}

void SetUnmanagableRange(char* base, unsigned int size)
{
	GC::unmanagableBase = base;
	GC::unmanagableTop = base + size;
}

void MarkUsedBlocks()
{
	//printf("Unmanageable range: %p-%p\r\n", GC::unmanagableBase, GC::unmanagableTop);

	ExternFuncInfo	*functions = &NULLC::commonLinker->exFunctions[0];
	ExternVarInfo	*vars = &NULLC::commonLinker->exVariables[0];
	ExternTypeInfo	*types = &NULLC::commonLinker->exTypes[0];
	//char			*symbols = &NULLC::commonLinker->exSymbols[0];

	// Mark global variables
	for(unsigned int i = 0; i < NULLC::commonLinker->exVariables.size(); i++)
	{
		//printf("Global %s %s (with offset of %d)\r\n", symbols + types[vars[i].type].offsetToName, symbols + vars[i].offsetToName, vars[i].offset);
		GC::CheckVariable(GC::unmanagableBase + vars[i].offset, types[vars[i].type]);
	}
	//printf("Calls: %d, GetBase: %d, Marked: %d\r\n", GC::markCalls, GC::canMark, GC::markedCount);

	void *unknownExec = NULL;
	unsigned int execID = nullcGetCurrentExecutor(&unknownExec);
	int offset = NULLC::commonLinker->globalVarSize;
	
	if(execID == NULLC_VM)
	{
		Executor *exec = (Executor*)unknownExec;
		exec->BeginCallStack();
	}else{
		ExecutorX86 *exec = (ExecutorX86*)unknownExec;
		exec->BeginCallStack();
	}
	// Mark local variables
	while(true)
	{
		int address = -1;
		if(execID == NULLC_VM)
		{
			Executor *exec = (Executor*)unknownExec;
			address = exec->GetNextAddress();
		}else{
			ExecutorX86 *exec = (ExecutorX86*)unknownExec;
			address = exec->GetNextAddress();
		}
		if(address == -1)
			break;
		int funcID = -1;

		int debugMatch = 0;
		for(unsigned int i = 0; i < NULLC::commonLinker->exFunctions.size(); i++)
		{
			if(address >= functions[i].address && address < (functions[i].address + functions[i].codeSize))
			{
				funcID = i;
				debugMatch++;
			}
		}
		assert(debugMatch < 2);

		if(funcID != -1)
		{
			int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;
			//printf("In function %s (with offset of %d)\r\n", symbols + functions[funcID].offsetToName, alignOffset);
			offset += alignOffset;
			for(unsigned int i = 0; i < functions[funcID].localCount; i++)
			{
				ExternLocalInfo &lInfo = NULLC::commonLinker->exLocals[functions[funcID].offsetToFirstLocal + i];
				//printf("Local %s %s (with offset of %d)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset + lInfo.offset);
				GC::CheckVariable(GC::unmanagableBase + offset + lInfo.offset, types[lInfo.type]);
			}
			ExternLocalInfo &lInfo = NULLC::commonLinker->exLocals[functions[funcID].offsetToFirstLocal + functions[funcID].localCount - 1];
			offset += lInfo.offset + lInfo.size;
		}
	}
	//printf("Calls: %d, GetBase: %d, Marked: %d\r\n", GC::markCalls, GC::canMark, GC::markedCount);

	const unsigned int intHash = GetStringHash("int");
	// Mark closure lists
	unsigned int upvalCount = 0;
	for(unsigned int i = 0; i < NULLC::commonLinker->exFunctions.size(); i++)
	{
		ExternFuncInfo::Upvalue *curr = functions[i].externalList;
		while(curr)
		{
			upvalCount++;
			assert(intHash == types[4].nameHash);
			char *rRef1 = (char*)&curr->ptr;
			GC::MarkPointer(rRef1, types[4]);
			char *rRef2 = (char*)&curr->next;
			GC::MarkPointer(rRef2, types[4]);
			curr = curr->next;
		}
	}
	//printf("Upvalues: %d\r\n", upvalCount);
	//printf("Calls: %d, GetBase: %d, Marked: %d\r\n", GC::markCalls, GC::canMark, GC::markedCount);
	GC::markCalls = 0;
	GC::canMark = 0;
	GC::markedCount = 0;
}
