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

ExternTypeInfo*	FindNextType(ExternTypeInfo *type)
{
	return (ExternTypeInfo*)((char*)(type) + type->structSize);
}

ExternVarInfo*	FindNextVar(ExternVarInfo *var)
{
	return (ExternVarInfo*)((char*)(var) + var->structSize);
}

ExternFuncInfo*	FindNextFunc(ExternFuncInfo *func)
{
	return (ExternFuncInfo*)((char*)(func) + func->structSize);
}
