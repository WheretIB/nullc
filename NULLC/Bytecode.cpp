#include "Bytecode.h"

ExternTypeInfo* FindFirstType(ByteCode *code)
{
	return (ExternTypeInfo*)((char*)(code) + sizeof(ByteCode));
}

ExternMemberInfo* FindFirstMember(ByteCode *code)
{
	return (ExternMemberInfo*)(FindFirstType(code) + code->typeCount);
}

ExternConstantInfo* FindFirstConstant(ByteCode *code)
{
	return (ExternConstantInfo*)((char*)code + code->offsetToConstants);
}

ExternModuleInfo* FindFirstModule(ByteCode *code)
{
	return (ExternModuleInfo*)((char*)(code) + code->offsetToFirstModule);
}

ExternVarInfo* FindFirstVar(ByteCode *code)
{
	return (ExternVarInfo*)((char*)(code) + code->offsetToFirstVar);
}

ExternFuncInfo* FindFirstFunc(ByteCode *code)
{
	return (ExternFuncInfo*)((char*)(code) + code->offsetToFirstFunc);
}

ExternLocalInfo* FindFirstLocal(ByteCode *code)
{
	return (ExternLocalInfo*)((char*)(code) + code->offsetToLocals);
}

ExternTypedefInfo* FindFirstTypedef(ByteCode *code)
{
	return (ExternTypedefInfo*)((char*)(code) + code->offsetToTypedef);
}

ExternNamespaceInfo* FindFirstNamespace(ByteCode *code)
{
	return (ExternNamespaceInfo*)((char*)code + code->offsetToNamespaces);
}

char* FindCode(ByteCode *code)
{
	return ((char*)(code) + code->offsetToCode);
}

unsigned int* FindSourceInfo(ByteCode *code)
{
	return (unsigned int*)((char*)code + code->offsetToInfo);
}

char* FindSymbols(ByteCode *code)
{
	return (char*)(code) + code->offsetToSymbols;
}

char* FindSource(ByteCode *code)
{
	return (char*)(code) + code->offsetToSource;
}
