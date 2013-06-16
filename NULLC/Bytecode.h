#pragma once
#ifndef NULLC_BYTECODE_H
#define NULLC_BYTECODE_H

#include "nullcdef.h"

#pragma pack(push, 4)

struct ExternTypeInfo
{
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR, };

	unsigned int	offsetToName;

	unsigned int	size;	// sizeof(type)
	unsigned int	padding;
	TypeCategory	type;

	enum SubCategory{ CAT_NONE, CAT_ARRAY, CAT_POINTER, CAT_FUNCTION, CAT_CLASS, };
	SubCategory		subCat;

	enum TypeFlags
	{
		TYPE_HAS_FINALIZER = 1 << 0,
		TYPE_DEPENDS_ON_GENERIC = 1 << 1,
		TYPE_IS_EXTENDABLE = 1 << 2
	};

	unsigned char	defaultAlign;
	unsigned char	typeFlags; // TypeFlags
	unsigned short	pointerCount;

	union
	{
		unsigned int	arrSize;
		unsigned int	memberCount;
	};
	unsigned int	constantCount;
	union
	{
		unsigned int	subType;
		unsigned int	memberOffset;
	};

	unsigned int	nameHash;
	unsigned int	definitionOffset; // For generic types, an offset in a lexeme stream to the point of type argument list
	unsigned int	genericTypeCount;
	unsigned int	namespaceHash;
	unsigned int	baseType;
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
	enum LocalType
	{
		PARAMETER,
		LOCAL,
		EXTERNAL
	};
	enum LocalFlags
	{
		IS_EXPLICIT = 1 << 0
	};

	unsigned char	paramType;
	unsigned char	paramFlags;
	unsigned short	defaultFuncId;

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
	int				isVisible;

	void			*funcPtr;
#ifndef _M_X64
	unsigned int	ptrPad;
#endif

	enum ReturnType
	{
		RETURN_UNKNOWN,
		RETURN_VOID,
		RETURN_INT,
		RETURN_DOUBLE,
		RETURN_LONG,
	};
	unsigned char	retType;	// one of the ReturnType enumeration values
	enum FunctionCategory
	{
		NORMAL,
		LOCAL,
		THISCALL,
		COROUTINE
	};
	unsigned char	funcCat;
	unsigned char	isGenericInstance;
	unsigned char	returnShift;
	unsigned int	funcType;	// index to the type array

	unsigned int	startInByteCode;
	unsigned int	parentType; // Type inside which the function is defined
	unsigned int	contextType; // Type of the function context

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

	unsigned int	namespaceHash;

// For x86 function call
	unsigned int	bytesToPop;
// For PS3 function call
	unsigned int	rOffsets[8];
	unsigned int	fOffsets[8];
	unsigned int	ps3Callable;

	unsigned int	genericOffset;
	unsigned int	genericReturnType;

	// Size of the explicit type list for generic functions and generic function instances
	unsigned int	explicitTypeCount;

	unsigned int	nameHash;
};

struct ExternTypedefInfo
{
	unsigned		offsetToName;
	unsigned		targetType;
	unsigned		parentType;
};

struct ExternModuleInfo
{
	const char		*name;
#ifndef _M_X64
	unsigned int	ptrPad;
#endif

	unsigned int	nameHash;
	unsigned int	nameOffset;

	unsigned int	funcStart;
	unsigned int	funcCount;

	unsigned int	variableOffset;

	unsigned int	sourceOffset;
	unsigned int	sourceSize;
};

struct ExternMemberInfo
{
	unsigned int	type;
	unsigned int	offset;
};

struct ExternConstantInfo
{
	unsigned int	type;
	long long		value;
};

struct ExternNamespaceInfo
{
	unsigned offsetToName;
	unsigned parentHash;
};

struct ByteCode
{
	unsigned int	size;	// Overall size

	unsigned int	typeCount;
	ExternTypeInfo	*firstType_; // deprecated, use FindFirstType
#ifndef _M_X64
	unsigned int	ptrPad0;
#endif

	unsigned int		dependsCount;
	unsigned int		offsetToFirstModule;
	ExternModuleInfo	*firstModule_; // deprecated, use FindFirstModule
#ifndef _M_X64
	unsigned int	ptrPad1;
#endif
	
	unsigned int	globalVarSize;	// size of all global variables, in bytes
	unsigned int	variableCount;	// variable info count
	unsigned int	variableExportCount;	// exported variable count
	ExternVarInfo	*firstVar_; // deprecated, use FindFirstVar
#ifndef _M_X64
	unsigned int	ptrPad2;
#endif
	unsigned int	offsetToFirstVar;

	unsigned int	functionCount;
	unsigned int	moduleFunctionCount;
	unsigned int	offsetToFirstFunc;	// Offset from the beginning of a structure to the first ExternFuncInfo data
	ExternFuncInfo	*firstFunc_; // deprecated, use FindFirstFunc
#ifndef _M_X64
	unsigned int	ptrPad3;
#endif

	unsigned int	localCount;
	unsigned int	offsetToLocals;
	ExternLocalInfo	*firstLocal_; // deprecated, use FindFirstLocal
#ifndef _M_X64
	unsigned int	ptrPad4;
#endif

	unsigned int	closureListCount;

	unsigned int	infoSize;
	unsigned int	offsetToInfo;

	unsigned int	codeSize;
	unsigned int	offsetToCode;
	unsigned int	globalCodeStart;
	char			*code_;	// deprecated, use FindCode
#ifndef _M_X64
	unsigned int	ptrPad5;
#endif

	unsigned int	symbolLength;
	unsigned int	offsetToSymbols;
	char			*debugSymbols_; // deprecated, used FindSymbols
#ifndef _M_X64
	unsigned int	ptrPad6;
#endif

	unsigned int	sourceSize;
	unsigned int	offsetToSource;

	unsigned int	llvmSize;
	unsigned int	llvmOffset;

	unsigned int	typedefCount;
	unsigned int	offsetToTypedef;

	unsigned int	offsetToConstants;

	unsigned int	namespaceCount;
	unsigned int	offsetToNamespaces;

//	ExternTypeInfo	types[typeCount];

//	ExternMemberInfo	typeMembers[];

//	ExternConstantInfo	typeConstants[];

//	ExternModuleInfo	modules[dependsCount];

//	ExternVarInfo	variables[variableCount];	// data about variables

//	ExternFuncInfo	functions[functionCount];	// info about first function

//	ExternLocalInfo	locals[localCount];	// Function locals (including parameters)

//	ExternFuncInfo::Upvalue	closureLists[closureListCount];

//	ExternTypedefInfo	typedefs[typedefCount]

//	ExternNamespaceInfo	namespaces[namespaceCount]

//	char			code[codeSize];

//	unsigned int	sourceInfo[infoLength * 2]

//	char			debugSymbols[symbolLength];

//	char			source[sourceSize]
};

#pragma pack(pop)

ExternTypeInfo*		FindFirstType(ByteCode *code);
ExternMemberInfo*	FindFirstMember(ByteCode *code);
ExternConstantInfo*	FindFirstConstant(ByteCode *code);
ExternModuleInfo*	FindFirstModule(ByteCode *code);
ExternVarInfo*		FindFirstVar(ByteCode *code);
ExternFuncInfo*		FindFirstFunc(ByteCode *code);
ExternLocalInfo*	FindFirstLocal(ByteCode *code);
ExternTypedefInfo*	FindFirstTypedef(ByteCode *code);
ExternNamespaceInfo*FindFirstNamespace(ByteCode *code);
char*				FindCode(ByteCode *code);
unsigned int*		FindSourceInfo(ByteCode *code);
char*				FindSymbols(ByteCode *code);
char*				FindSource(ByteCode *code);

#endif
