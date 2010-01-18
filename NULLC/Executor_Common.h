#pragma once
#include "Bytecode.h"
#include "Linker.h"

void CommonSetLinker(Linker* linker);

void ClosureCreate(char* paramBase, unsigned int helper, unsigned int argument, ExternFuncInfo::Upvalue* upvalue);
void CloseUpvalues(char* paramBase, unsigned int argument);

unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize);

// Garbage collector

void SetUnmanagableRange(char* base, unsigned int size);
void MarkUsedBlocks();
