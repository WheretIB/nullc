#include "Bytecode.h"

unsigned int GetStringHash(const char *str)
{
	unsigned int hash = 5381;
	int c;
	while((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c;
	return hash;
}

void	BytecodeFixup(ByteCode *code)
{
	char *bytecode = (char*)code;

	ExternVarInfo *varInfo = (ExternVarInfo*)(bytecode + sizeof(ByteCode));
	code->firstVar = varInfo;
	for(unsigned int i = 0; i < code->variableCount; i++)
	{
		char *namePtr = (char*)(&varInfo->name) + sizeof(varInfo->name);
		varInfo->name = namePtr;

		if(i+1 == code->variableCount)
			varInfo->next = 0;
		else
			varInfo->next = (ExternVarInfo*)((char*)(varInfo + 1) + varInfo->nameLength + 1);

		varInfo = varInfo->next;
	}

	ExternFuncInfo *fInfo = (ExternFuncInfo*)(bytecode + code->offsetToFirstFunc);
	code->firstFunc = fInfo;
	for(unsigned int i = 0; i < code->functionCount; i++)
	{
		char *namePtr = (char*)(&fInfo->name) + sizeof(fInfo->name);
		fInfo->name = namePtr;

		if(i+1 == code->functionCount)
			fInfo->next = 0;
		else
			fInfo->next = (ExternFuncInfo*)((char*)(fInfo + 1) + fInfo->nameLength + 1);

		fInfo = fInfo->next;
	}

	code->code = (bytecode + code->offsetToCode);
}

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
