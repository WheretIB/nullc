#pragma once

class Linker;

struct ExternTypeInfo;
struct ExternMemberInfo;
struct ExternFuncInfo;
struct ExternLocalInfo;

struct NULLCRef;

void CommonSetLinker(Linker* linker);

unsigned ConvertFromAutoRef(unsigned int source, unsigned int target);

bool AreMembersAligned(ExternTypeInfo *lType, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra);

bool HasIntegerMembersInRange(ExternTypeInfo &type, unsigned fromOffset, unsigned toOffset, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra);

unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize, bool withVariables);
void DumpStackFrames();

// Garbage collector

namespace GC
{
	void CheckPointer(char* ptr);
	void CheckBasePointer(char* basePtr);
	void CheckArrayElements(char* ptr, unsigned size, const ExternTypeInfo& elementType);

	void CheckArray(char* ptr, const ExternTypeInfo& type);
	void CheckClass(char* ptr, const ExternTypeInfo& type);
	void CheckFunction(char* ptr);
	void CheckVariable(char* ptr, const ExternTypeInfo& type);

	void SetUnmanagableRange(char* base, unsigned int size);
	int IsPointerUnmanaged(NULLCRef ptr);
	void MarkUsedBlocks();
	void MarkPendingRoots();
	void ResetGC();
}

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
typedef struct DCCallVM_ DCCallVM;

void RunRawExternalFunction(DCCallVM *dcCallVM, ExternFuncInfo &func, ExternLocalInfo *exLocals, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra, unsigned *argumentStorage, unsigned *resultStorage);
#endif

unsigned GetFunctionVmReturnType(ExternFuncInfo &function, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra);

NULLCRef GetExecutorResultObject(unsigned tempStackType, unsigned *tempStackArrayBase);
const char* GetExecutorResult(char *execResult, unsigned execResultSize, unsigned tempStackType, unsigned *tempStackArrayBase, char *exSymbols, ExternTypeInfo *exTypes);
int GetExecutorResultInt(unsigned tempStackType, unsigned *tempStackArrayBase);
double GetExecutorResultDouble(unsigned tempStackType, unsigned *tempStackArrayBase);
long long GetExecutorResultLong(unsigned tempStackType, unsigned *tempStackArrayBase);

int VmIntPow(int power, int number);
long long VmLongPow(long long power, long long number);
