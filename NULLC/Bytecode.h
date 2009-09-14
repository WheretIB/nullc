#pragma once

struct ExternTypeInfo
{
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR, };

	unsigned int	structSize;

	unsigned int	size;	// sizeof(type)
	TypeCategory	type;

	unsigned int	nameHash;
};

struct ExternVarInfo
{
	unsigned int	structSize;

	unsigned int	type;	// index in type array

	unsigned int	size;
	unsigned int	nameHash;
};

struct ExternFuncInfo
{
	unsigned int	structSize;

	int				oldAddress;
	int				address;
	int				codeSize;
	void			*funcPtr;
	int				isVisible;

	enum ReturnType
	{
		RETURN_UNKNOWN,
		RETURN_VOID,
		RETURN_INT,
		RETURN_DOUBLE
	};
	unsigned int	retType;	// one of the ReturnType enumeration values
	unsigned int	funcType;	// index to the type array

	unsigned int startInByteCode;

// For x86 function call
	unsigned int bytesToPop;
// For PS3 function call
	unsigned int rOffsets[8];
	unsigned int fOffsets[8];
	unsigned int ps3Callable;

	unsigned int	nameHash;
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

	unsigned int	parameterCount;
	unsigned int	offsetToFirstParameter;
	unsigned int	*firstParameter;

	unsigned int	codeSize;
	unsigned int	offsetToCode;
	unsigned int	globalCodeStart;
	char			*code;	// needs fix up after load

//	ExternTypeInfo	types[typeCount];	// data about variables

//	ExternVarInfo	variables[variableCount];	// data about variables

//	ExternFuncInfo	functions[functionCount];	// info about first function

//	unsigned int	parameters[paramCount];	// function parameter types

//	char			code[codeSize];
};

ExternTypeInfo*	FindFirstType(ByteCode *code);
ExternVarInfo*	FindFirstVar(ByteCode *code);
ExternFuncInfo*	FindFirstFunc(ByteCode *code);
char*			FindCode(ByteCode *code);
