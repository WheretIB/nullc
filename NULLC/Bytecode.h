#pragma once

struct ExternTypeInfo
{
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR, };

	unsigned int	structSize;
	ExternTypeInfo	*next;	// needs fix up after load

	unsigned int	size;	// sizeof(type)
	TypeCategory	type;

	unsigned int	nameLength;
	char			*name;	// needs fix up after load
//	char			name[nameLength];
};

struct ExternVarInfo
{
	unsigned int	structSize;
	ExternVarInfo	*next;	// needs fix up after load

	unsigned int	type;	// index in type array

	unsigned int	size;
	unsigned int	nameLength;
	char			*name;	// needs fix up after load
//	char			name[nameLength];
};

struct ExternFuncInfo
{
	unsigned int	structSize;
	ExternFuncInfo	*next;	// needs fix up after load

	int				oldAddress;
	int				address;
	int				codeSize;
	void			*funcPtr;
	int				isVisible;
	enum FunctionType{ NORMAL, LOCAL, THISCALL };
	FunctionType	funcType;

	unsigned int	retType;	// index in type array
	unsigned int	paramCount;
	unsigned int	*paramList;	// needs fix up after load

	unsigned int	nameHash;
	unsigned int	nameLength;
	char			*name;	// needs fix up after load
//	char			name[nameLength];

//	unsigned int	paramType[paramCount];
};

struct ByteCode
{
	unsigned int	size;	// Overall size

	unsigned int	typeCount;
	ExternTypeInfo	*firstType;
	
	unsigned int	globalVarSize;	// size of all global variables, in bytes
	unsigned int	variableCount;	//
	unsigned int	offsetToFirstVar;
	ExternVarInfo	*firstVar;

	unsigned int	functionCount;	//
	unsigned int	offsetToFirstFunc;	// Offset from the beginning of a structure to the first ExternFuncInfo data
	ExternFuncInfo	*firstFunc;

	unsigned int	codeSize;
	unsigned int	offsetToCode;
	unsigned int	globalCodeStart;
	char			*code;	// needs fix up after load

//	ExternTypeInfo	types[typeCount];	// data about variables

//	ExternVarInfo	variables[variableCount];	// data about variables

//	ExternFuncInfo	functions[functionCount];	// info about first function

//	char			code[codeSize];
};

unsigned int GetStringHash(const char *str);

void	BytecodeFixup(ByteCode *code);

ExternTypeInfo*	FindFirstType(ByteCode *code);
ExternVarInfo*	FindFirstVar(ByteCode *code);
ExternFuncInfo*	FindFirstFunc(ByteCode *code);
char*			FindCode(ByteCode *code);

ExternTypeInfo*	FindNextType(ExternTypeInfo *type);
ExternVarInfo*	FindNextVar(ExternVarInfo *var);
ExternFuncInfo*	FindNextFunc(ExternFuncInfo *func);