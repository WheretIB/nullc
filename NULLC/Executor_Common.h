#pragma once
#include "Bytecode.h"
#include "Linker.h"

void CommonSetLinker(Linker* linker);

void ClosureCreate(char* paramBase, unsigned int helper, unsigned int argument, ExternFuncInfo::Upvalue* upvalue);
void CloseUpvalues(char* paramBase, unsigned int depth, unsigned int argument);
unsigned ConvertFromAutoRef(unsigned int source, unsigned int target);

ExternTypeInfo*	GetTypeList();

unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize);

// Garbage collector

void	SetUnmanagableRange(char* base, unsigned int size);
int		IsPointerUnmanaged(NULLCRef ptr);
void	MarkUsedBlocks();
void	ResetGC();
