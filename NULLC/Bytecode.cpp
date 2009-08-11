#include "Bytecode.h"

ExternTypeInfo*	FindFirstType(ByteCode *code)
{
	return (ExternTypeInfo*)((char*)(code) + sizeof(ByteCode));
}

ExternVarInfo*	FindFirstVar(ByteCode *code)
{
	return (ExternVarInfo*)((char*)(code) + code->offsetToFirstVar);
}

ExternFuncInfo*	FindFirstFunc(ByteCode *code)
{
	return (ExternFuncInfo*)((char*)(code) + code->offsetToFirstFunc);
}

char*			FindCode(ByteCode *code)
{
	return ((char*)(code) + code->offsetToCode);
}
