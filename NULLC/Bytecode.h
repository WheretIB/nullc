#pragma once

struct ExternTypeInfo
{
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR, };

	unsigned int	offsetToName;

	unsigned int	size;	// sizeof(type)
	TypeCategory	type;

	unsigned int	nameHash;
};

struct ExternVarInfo
{
	unsigned int	type;	// index in type array

	unsigned int	size;
	unsigned int	nameHash;
};

struct ExternLocalInfo
{
	bool			parameter;

	unsigned int	type;
	unsigned int	offset;

	unsigned int	offsetToName;
};

struct ExternFuncInfo
{
	unsigned int	offsetToName;

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
		RETURN_DOUBLE,
		RETURN_LONG,
	};
	unsigned int	retType;	// one of the ReturnType enumeration values
	unsigned int	funcType;	// index to the type array

	unsigned int	startInByteCode;

	unsigned int	offsetToFirstLocal;
	unsigned int	localCount;

// For x86 function call
	unsigned int	bytesToPop;
// For PS3 function call
	unsigned int	rOffsets[8];
	unsigned int	fOffsets[8];
	unsigned int	ps3Callable;

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

	unsigned int	localCount;
	unsigned int	offsetToLocals;
	ExternLocalInfo	*firstLocal;

	unsigned int	codeSize;
	unsigned int	offsetToCode;
	unsigned int	globalCodeStart;
	char			*code;	// needs fix up after load

	unsigned int	symbolLength;
	unsigned int	offsetToSymbols;
	char			*debugSymbols;

//	ExternTypeInfo	types[typeCount];	// data about variables

//	ExternVarInfo	variables[variableCount];	// data about variables

//	ExternFuncInfo	functions[functionCount];	// info about first function

//	ExternLocalInfo	locals[localCount];	// Function locals (including parameters)

//	char			code[codeSize];

//	char			debugSymbols[symbolLength];
};

ExternTypeInfo*	FindFirstType(ByteCode *code);
ExternVarInfo*	FindFirstVar(ByteCode *code);
ExternFuncInfo*	FindFirstFunc(ByteCode *code);
char*			FindCode(ByteCode *code);
