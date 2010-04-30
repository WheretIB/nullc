#pragma once

struct ExternTypeInfo
{
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR, };

	unsigned int	offsetToName;

	unsigned int	size;	// sizeof(type)
	TypeCategory	type;

	enum SubCategory{ CAT_NONE, CAT_ARRAY, CAT_POINTER, CAT_FUNCTION, CAT_CLASS, };
	SubCategory		subCat;

	unsigned int	defaultAlign;

	union
	{
		unsigned int	arrSize;
		unsigned int	memberCount;
	};
	union
	{
		unsigned int	subType;
		unsigned int	memberOffset;
	};

	unsigned int	nameHash;
};

struct ExternVarInfo
{
	unsigned int	offsetToName;
	unsigned int	nameHash;

	unsigned int	type;	// index in type array
	unsigned int	offset;
};

struct ExternLocalInfo
{
	enum LocalType{ PARAMETER, LOCAL, EXTERNAL };
	LocalType		paramType;

	unsigned int	type, size;
	union
	{
		unsigned int	offset;
		unsigned int	target;
	};
	unsigned int	closeListID;

	unsigned int	offsetToName;
};

struct ExternFuncInfo
{
	unsigned int	offsetToName;

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

	union
	{
		unsigned int	startInByteCode;
		unsigned int	parentType;
	};

	unsigned int	offsetToFirstLocal;
	unsigned int	paramCount;
	unsigned int	localCount;
	unsigned int	externCount;

	struct Upvalue
	{
		unsigned int	*ptr;
		Upvalue			*next;
		unsigned int	size;
	};
	unsigned int	closeListStart;

// For x86 function call
	unsigned int	bytesToPop;
// For PS3 function call
	unsigned int	rOffsets[8];
	unsigned int	fOffsets[8];
	unsigned int	ps3Callable;

	unsigned int	nameHash;
};

struct ExternModuleInfo
{
	const char		*name;
	unsigned int	nameHash;
	unsigned int	nameOffset;

	unsigned int	funcStart;
	unsigned int	funcCount;

	unsigned int	variableOffset;

	unsigned int	sourceOffset;
	unsigned int	sourceSize;
};

struct ByteCode
{
	unsigned int	size;	// Overall size

	unsigned int	typeCount;
	ExternTypeInfo	*firstType;

	unsigned int		dependsCount;
	unsigned int		offsetToFirstModule;
	ExternModuleInfo	*firstModule;
	
	unsigned int	globalVarSize;	// size of all global variables, in bytes
	unsigned int	variableCount;	//
	unsigned int	offsetToFirstVar;
	ExternVarInfo	*firstVar;

	unsigned int	functionCount;	//
	unsigned int	moduleFunctionCount;
	unsigned int	offsetToFirstFunc;	// Offset from the beginning of a structure to the first ExternFuncInfo data
	ExternFuncInfo	*firstFunc;

	unsigned int	localCount;
	unsigned int	offsetToLocals;
	ExternLocalInfo	*firstLocal;

	unsigned int	closureListCount;

	unsigned int	infoSize;
	unsigned int	offsetToInfo;

	unsigned int	codeSize;
	unsigned int	offsetToCode;
	unsigned int	globalCodeStart;
	char			*code;	// needs fix up after load

	unsigned int	symbolLength;
	unsigned int	offsetToSymbols;
	char			*debugSymbols;

	unsigned int	sourceSize;
	unsigned int	offsetToSource;

//	ExternTypeInfo	types[typeCount];

//	ExternModuleInfo	modules[dependsCount];

//	unsigned int	complexTypeMemberTypes[];

//	ExternVarInfo	variables[variableCount];	// data about variables

//	ExternFuncInfo	functions[functionCount];	// info about first function

//	ExternLocalInfo	locals[localCount];	// Function locals (including parameters)

//	char			code[codeSize];

//	unsigned int	sourceInfo[infoLength * 2]

//	char			debugSymbols[symbolLength];
};

ExternTypeInfo*	FindFirstType(ByteCode *code);
ExternVarInfo*	FindFirstVar(ByteCode *code);
ExternFuncInfo*	FindFirstFunc(ByteCode *code);
char*			FindCode(ByteCode *code);
