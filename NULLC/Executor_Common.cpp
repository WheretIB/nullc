#include "Executor_Common.h"

#include "CodeInfo.h"
#include "StdLib.h"
#include "nullc_debug.h"
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

void ClosureCreate(char* paramBase, unsigned int helper, unsigned int argument, ExternFuncInfo::Upvalue* upvalue)
{
	// Function with a list of external variables to capture
	ExternFuncInfo &func = NULLC::commonLinker->exFunctions[argument];
	// Array of upvalue lists
	ExternFuncInfo::Upvalue **externalList = &NULLC::commonLinker->exCloseLists[0];
	// Function external list
	ExternLocalInfo *externals = &NULLC::commonLinker->exLocals[func.offsetToFirstLocal + func.localCount];
	// For every function external
	for(unsigned int i = 0; i < func.externCount; i++)
	{
		if(externals[i].closeListID & 0x80000000)	// If external variable can be found in current scope
		{
			// Take a pointer to it
			upvalue->ptr = (unsigned int*)&paramBase[externals[i].target];
		}else{	// Otherwise, we have to get pointer from functions' existing closure
			// Pointer to previous closure is the last function parameter (offset of cmd.helper from stack frame base)
			unsigned int *prevClosure = (unsigned int*)(intptr_t)*(int*)(&paramBase[helper]);
			// Take pointer from inside the closure (externals[i].target is in bytes, but array is of unsigned int elements)
			upvalue->ptr = (unsigned int*)(intptr_t)prevClosure[externals[i].target >> 2];
		}
		// Next upvalue will be current list head
		upvalue->next = externalList[externals[i].closeListID];
		// Save variable size
		upvalue->size = externals[i].size;
		// Change list head to a new upvalue
		externalList[externals[i].closeListID] = upvalue;
		// Move to the next upvalue (upvalue size is max(sizeof(ExternFuncInfo::Upvalue), externals[i].size)
		upvalue = (ExternFuncInfo::Upvalue*)((int*)upvalue + ((externals[i].size >> 2) < 3 ? 3 : 1 + (externals[i].size >> 2)));
	}
}

void CloseUpvalues(char* paramBase, unsigned int argument)
{
	// Array of upvalue lists
	ExternFuncInfo::Upvalue **externalList = &NULLC::commonLinker->exCloseLists[0];
	// Current upvalue and previous
	ExternFuncInfo::Upvalue *curr = externalList[argument];
	// While we have an upvalue that points to address larger than base (so that in recursive function call only last functions upvalues will be closed)
	while(curr && (char*)curr->ptr >= paramBase)
	{
		// Save pointer to next upvalue
		ExternFuncInfo::Upvalue *next = curr->next;
		// And save the size of target variable
		unsigned int size = curr->size;
		
		// Delete upvalue from list (move global list head to the next element)
		externalList[argument] = curr->next;

		// Copy target variable data to upvalue
		memcpy(&curr->next, curr->ptr, size);
		curr->ptr = (unsigned int*)&curr->next;

		// Proceed to the next upvalue
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
				(lInfo.closeListID & 0x80000000 ? "local" : "closure");
			current += SafeSprintf(current, bufSize - int(current - start), " %s %d: %s %s (at %s+%d size %d)\r\n",
				localType, i, typeName, localName, offsetType, lInfo.offset, exTypes[lInfo.type].size);
		}
	}
#endif
	return (unsigned int)(current - start);
}

#define GC_DEBUG_PRINT(...)
//#define GC_DEBUG_PRINT printf

namespace GC
{
	// Range of memory that is not checked. Used to exclude pointers to stack from marking and GC
	char	*unmanageableBase = NULL;
	char	*unmanageableTop = NULL;
	unsigned int	objectName = GetStringHash("auto ref");

	void CheckArray(char* ptr, const ExternTypeInfo& type);
	void CheckClass(char* ptr, const ExternTypeInfo& type);
	void CheckVariable(char* ptr, const ExternTypeInfo& type);

	// Function that marks memory blocks belonging to GC
	void MarkPointer(char* ptr, const ExternTypeInfo& type, bool takeSubtype)
	{
		// We have pointer to stack that has a pointer inside, so 'ptr' is really a pointer to pointer
		char **rPtr = (char**)ptr;
		// Check for unmanageable ranges. Range of 0x00000000-0x00010000 is unmanageable by default due to upvalues with offsets inside closures.
		if(*rPtr > (char*)0x00010000 || *rPtr < unmanageableBase || *rPtr > unmanageableTop)
		{
			// Get type that pointer points to
			GC_DEBUG_PRINT("\tGlobal pointer %s %p (at %p)\r\n", NULLC::commonLinker->exSymbols.data + type.offsetToName, *rPtr, ptr);

			// Get pointer to the start of memory block. Some pointers may point to the middle of memory blocks
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(*rPtr);
			// If there is no base, this pointer points to memory that is not GCs memory
			if(!basePtr)
				return;
			GC_DEBUG_PRINT("\tPointer base is %p\r\n", basePtr);

			// Marker is 4 bytes before the block
			unsigned int *marker = (unsigned int*)(basePtr)-1;
			GC_DEBUG_PRINT("\tMarker is %d\r\n", *marker);

			// If block is unmarked
			if(*marker == 0)
			{
				// Mark block as used
				*marker = 1;
				// And if type is not simple, check memory to which pointer points to
				if(type.subCat != ExternTypeInfo::CAT_NONE)
					CheckVariable(*rPtr, takeSubtype ? NULLC::commonLinker->exTypes[type.subType] : type);
			}
		}
	}

	// Function that checks arrays for pointers
	void CheckArray(char* ptr, const ExternTypeInfo& type)
	{
		// Get array element type
		ExternTypeInfo &subType = NULLC::commonLinker->exTypes[type.subType];
		// Real array size (changed for unsized arrays)
		unsigned int size = type.arrSize;
		// If array type is an unsized array, check pointer that points to actual array contents
		if(type.arrSize == TypeInfo::UNSIZED_ARRAY)
		{
			// Get real array size
			size = *(int*)(ptr + 4);
			// Mark target data
			MarkPointer(ptr, subType, false);
			// Switch pointer to array data
			char **rPtr = (char**)ptr;
			ptr = *rPtr;
			// If initialized, return
			if(!ptr)
				return;
		}
		// Otherwise, check every array element is it's either array, pointer of class
		switch(subType.subCat)
		{
		case ExternTypeInfo::CAT_ARRAY:
			for(unsigned int i = 0; i < size; i++, ptr += subType.size)
				CheckArray(ptr, subType);
			break;
		case ExternTypeInfo::CAT_POINTER:
			for(unsigned int i = 0; i < size; i++, ptr += subType.size)
				MarkPointer(ptr, subType, true);
			break;
		case ExternTypeInfo::CAT_CLASS:
			for(unsigned int i = 0; i < size; i++, ptr += subType.size)
				CheckClass(ptr, subType);
			break;
		}
	}

	// Function that checks classes for pointers
	void CheckClass(char* ptr, const ExternTypeInfo& type)
	{
		const ExternTypeInfo *realType = &type;
		if(type.nameHash == objectName)
		{
			// Get real variable type
			realType = &NULLC::commonLinker->exTypes[*(int*)ptr];
			// Mark target data
			MarkPointer(ptr + 4, *realType, false);
			// Switch pointer to target
			char **rPtr = (char**)(ptr + 4);
			// Fixup target
			CheckVariable(*rPtr, *realType);
			// Exit
			return;
		}
		// Get class member type list
		unsigned int *memberList = &NULLC::commonLinker->exTypeExtra[0];
		// Check every member
		for(unsigned int n = 0; n < realType->memberCount; n++)
		{
			// Get member type
			ExternTypeInfo &subType = NULLC::commonLinker->exTypes[memberList[realType->memberOffset + n]];
			// Check member
			CheckVariable(ptr, subType);
			// Move pointer to the next member
			ptr += subType.size;	// $$$ and what about alignment?
		}
	}

	// Function that decides, how variable of type 'type' should be checked for pointers
	void CheckVariable(char* ptr, const ExternTypeInfo& type)
	{
		switch(type.subCat)
		{
		case ExternTypeInfo::CAT_ARRAY:
			CheckArray(ptr, type);
			break;
		case ExternTypeInfo::CAT_POINTER:
			MarkPointer(ptr, type, true);
			break;
		case ExternTypeInfo::CAT_CLASS:
			CheckClass(ptr, type);
			break;
		}
	}
}

// Set range of memory that is not checked. Used to exclude pointers to stack from marking and GC
void SetUnmanagableRange(char* base, unsigned int size)
{
	GC::unmanageableBase = base;
	GC::unmanageableTop = base + size;
}

// Main function for marking all pointers in a program
void MarkUsedBlocks()
{
	GC_DEBUG_PRINT("Unmanageable range: %p-%p\r\n", GC::unmanageableBase, GC::unmanageableTop);

	// Get information about programs' functions, variables, types and symbols (for debug output)
	ExternFuncInfo	*functions = &NULLC::commonLinker->exFunctions[0];
	ExternVarInfo	*vars = &NULLC::commonLinker->exVariables[0];
	ExternTypeInfo	*types = &NULLC::commonLinker->exTypes[0];
	char			*symbols = &NULLC::commonLinker->exSymbols[0];
	(void)symbols;

	// Mark global variables
	for(unsigned int i = 0; i < NULLC::commonLinker->exVariables.size(); i++)
	{
		GC_DEBUG_PRINT("Global %s %s (with offset of %d)\r\n", symbols + types[vars[i].type].offsetToName, symbols + vars[i].offsetToName, vars[i].offset);
		GC::CheckVariable(GC::unmanageableBase + vars[i].offset, types[vars[i].type]);
	}

	// To check every stack frame, we have to get it first. But we have two different executors, so flow alternates depending on which executor we are running
	void *unknownExec = NULL;
	unsigned int execID = nullcGetCurrentExecutor(&unknownExec);

	// Starting stack offset is equal to global variable size
	int offset = NULLC::commonLinker->globalVarSize;
	
	// Init stack trace
	if(execID == NULLC_VM)
	{
		Executor *exec = (Executor*)unknownExec;
		exec->BeginCallStack();
	}else{
#ifdef NULLC_BUILD_X86_JIT
		ExecutorX86 *exec = (ExecutorX86*)unknownExec;
		exec->BeginCallStack();
#endif
	}
	// Mark local variables
	while(true)
	{
		int address = 0;
		// Get next address from call stack
		if(execID == NULLC_VM)
		{
			Executor *exec = (Executor*)unknownExec;
			address = exec->GetNextAddress();
		}else{
#ifdef NULLC_BUILD_X86_JIT
			ExecutorX86 *exec = (ExecutorX86*)unknownExec;
			address = exec->GetNextAddress();
#endif
		}
		// If failed, exit
		if(address == 0)
			break;

		// Find corresponding function
		int funcID = -1;
		for(unsigned int i = 0; i < NULLC::commonLinker->exFunctions.size(); i++)
		{
			if(address >= functions[i].address && address < (functions[i].address + functions[i].codeSize))
			{
				funcID = i;
			}
		}

		// If we are not in global scope
		if(funcID != -1)
		{
			// Align offset to the first variable (by 16 byte boundary)
			int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;
			offset += alignOffset;
			GC_DEBUG_PRINT("In function %s (with offset of %d)\r\n", symbols + functions[funcID].offsetToName, alignOffset);

			unsigned int offsetToNextFrame = 4;
			// Check every function local
			for(unsigned int i = 0; i < functions[funcID].localCount; i++)
			{
				// Get information about local
				ExternLocalInfo &lInfo = NULLC::commonLinker->exLocals[functions[funcID].offsetToFirstLocal + i];
				GC_DEBUG_PRINT("Local %s %s (with offset of %d)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset + lInfo.offset);
				// Check it
				GC::CheckVariable(GC::unmanageableBase + offset + lInfo.offset, types[lInfo.type]);
				if(lInfo.offset + lInfo.size > offsetToNextFrame)
					offsetToNextFrame = lInfo.offset + lInfo.size;
			}
			offset += offsetToNextFrame;
			GC_DEBUG_PRINT("Moving offset to next frame by %d bytes\r\n", offsetToNextFrame);
		}
	}

	// For debug check that type #4 is indeed, int
	const unsigned int intHash = GetStringHash("int");
	assert(intHash == types[4].nameHash);

	// Check pointers inside all unclosed upvalue lists
	for(unsigned int i = 0; i < NULLC::commonLinker->exCloseLists.size(); i++)
	{
		// List head
		ExternFuncInfo::Upvalue *curr = NULLC::commonLinker->exCloseLists[i];
		// Move list head while it points to unused upvalue
		while(curr)
		{
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(curr);
			if(basePtr && basePtr[-1] == 0)
			{
				curr = curr->next;
			}else{
				break;
			}
		}
		// Change list head in global data
		NULLC::commonLinker->exCloseLists[i] = curr;
		// Delete remaining unused upvalues from list
		while(curr && curr->next)
		{
			unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(curr->next);
			if(basePtr && basePtr[-1] == 1)
			{
				curr = curr->next;
			}else{
				curr->next = curr->next->next;
			}
		}
		
	}

	// Check for pointers in stack
	char *tempStackBase = NULL, *tempStackTop = NULL;
	if(execID == NULLC_VM)
	{
		Executor *exec = (Executor*)unknownExec;
		tempStackBase = (char*)exec->GetStackStart();
		tempStackTop = (char*)exec->GetStackEnd();
	}else{
#ifdef NULLC_BUILD_X86_JIT
		ExecutorX86 *exec = (ExecutorX86*)unknownExec;
		tempStackBase = (char*)exec->GetStackStart();
		tempStackTop = (char*)exec->GetStackEnd();
#endif
	}
	while(tempStackBase < tempStackTop)
	{
		GC::MarkPointer(tempStackBase, types[4], false);
		tempStackBase += 4;
	}
}
