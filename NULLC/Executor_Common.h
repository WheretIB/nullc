#pragma once
#include "Bytecode.h"
#include "Linker.h"

void CommonSetLinker(Linker* linker);

void ClosureCreate(char* paramBase, unsigned int helper, unsigned int argument, ExternFuncInfo::Upvalue* closure);
void CloseUpvalues(char* paramBase, unsigned int helper, unsigned int argument);

unsigned int PrintStackFrame(int address, char* current, unsigned int bufSize);
