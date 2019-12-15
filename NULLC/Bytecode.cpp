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

char* FindVmCode(ByteCode *code)
{
	return ((char*)(code) + code->vmOffsetToCode);
}

ExternSourceInfo* FindVmSourceInfo(ByteCode *code)
{
	return (ExternSourceInfo*)((char*)code + code->vmOffsetToInfo);
}

char* FindRegVmCode(ByteCode *code)
{
	return ((char*)(code)+code->regVmOffsetToCode);
}

ExternSourceInfo* FindRegVmSourceInfo(ByteCode *code)
{
	return (ExternSourceInfo*)((char*)code + code->regVmOffsetToInfo);
}

char* FindSymbols(ByteCode *code)
{
	return (char*)(code) + code->offsetToSymbols;
}

char* FindSource(ByteCode *code)
{
	return (char*)(code) + code->offsetToSource;
}

unsigned* FindRegVmConstants(ByteCode *code)
{
	return (unsigned*)((char*)(code) + code->regVmOffsetToConstants);
}

unsigned char* FindRegVmRegKillInfo(ByteCode *code)
{
	return (unsigned char*)((char*)(code) + code->regVmOffsetToRegKillInfo);
}
