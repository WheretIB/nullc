#pragma once

#include "Bytecode.h"
#include "Linker.h"

void CommonSetLinker(Linker* linker);

unsigned ConvertFromAutoRef(unsigned int source, unsigned int target);

bool AreMembersAligned(ExternTypeInfo *lType, Linker *exLinker);

bool HasIntegerMembersInRange(ExternTypeInfo &type, unsigned fromOffset, unsigned toOffset, Linker *linker);

ExternTypeInfo*	GetTypeList();

unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize, bool withVariables);
void DumpStackFrames();

// Garbage collector

void	SetUnmanagableRange(char* base, unsigned int size);
int		IsPointerUnmanaged(NULLCRef ptr);
void	MarkUsedBlocks();
void	ResetGC();
