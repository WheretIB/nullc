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

ExternTypeInfo*	GetTypeList();

unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize, bool withVariables);
void DumpStackFrames();

// Garbage collector

void	SetUnmanagableRange(char* base, unsigned int size);
int		IsPointerUnmanaged(NULLCRef ptr);
void	MarkUsedBlocks();
void	ResetGC();

#if !defined(NULLC_NO_RAW_EXTERNAL_CALL)
typedef struct DCCallVM_ DCCallVM;

void RunRawExternalFunction(DCCallVM *dcCallVM, ExternFuncInfo &func, ExternLocalInfo *exLocals, ExternTypeInfo *exTypes, ExternMemberInfo *exTypeExtra, unsigned *callStorage);
#endif

int VmIntPow(int power, int number);
long long VmLongPow(long long power, long long number);
