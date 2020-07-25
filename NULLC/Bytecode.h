#pragma once
#ifndef NULLC_BYTECODE_H
#define NULLC_BYTECODE_H

#include "nullcdef.h"

#pragma pack(push, 4)

struct ExternTypeInfo
{
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR };

	unsigned int	offsetToName;

	unsigned int	size;	// sizeof(type)
	unsigned int	padding;
	TypeCategory	type;

	enum SubCategory{ CAT_NONE, CAT_ARRAY, CAT_POINTER, CAT_FUNCTION, CAT_CLASS };
	SubCategory		subCat;

	enum TypeFlags
	{
		TYPE_HAS_FINALIZER = 1 << 0,
		TYPE_DEPENDS_ON_GENERIC = 1 << 1,
		TYPE_IS_EXTENDABLE = 1 << 2,
		TYPE_IS_INTERNAL = 1 << 3,
		TYPE_IS_COMPLETED = 1 << 4
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
	unsigned int	constantOffset;

	union
	{
		unsigned int	subType;
		unsigned int	memberOffset;
	};

	unsigned int	nameHash;

	unsigned int	definitionModule; // Index of the module containing the definition

	unsigned int	definitionLocationStart;
	unsigned int	definitionLocationEnd;
	unsigned int	definitionLocationName;

	// For generic types
	unsigned int	definitionOffsetStart; // Offset in a lexeme stream to the point of class definition start
	unsigned int	definitionOffset; // Offset in a lexeme stream to the point of type argument list
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
	unsigned int	offset;
	unsigned int	closeListID;
	unsigned char	alignmentLog2; // 1 << value

	unsigned int	offsetToName;
};

struct ExternFuncInfo
{
	unsigned int	offsetToName;

	int				regVmAddress;
	int				regVmCodeSize;
	int				regVmRegisters;

	int				builtinIndex;

	int				isVisible;

	void			(*funcPtrRaw)();
#ifndef _M_X64
	unsigned int	ptrRawPad;
#endif

	void			*funcPtrWrapTarget;
#ifndef _M_X64
	unsigned int	ptrWrapTargetPad;
#endif

	void			(*funcPtrWrap)(void*, char*, char*);
#ifndef _M_X64
	unsigned int	ptrWrapPad;
#endif

	enum ReturnType
	{
		RETURN_UNKNOWN,
		RETURN_VOID,
		RETURN_INT,
		RETURN_DOUBLE,
		RETURN_LONG
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
	unsigned char	isOperator;
	unsigned char	returnShift; // Amount of dwords to remove for result type after raw external function call
	unsigned int	returnSize; // Return size of the wrapped external function call
	unsigned int	funcType;	// index to the type array

	unsigned int	startInByteCode;
	unsigned int	parentType; // Type inside which the function is defined
	unsigned int	contextType; // Type of the function context

	unsigned int	offsetToFirstLocal;
	unsigned int	paramCount;
	unsigned int	localCount;
	unsigned int	externCount;

	// Object copy storage is splaced after the upvalue
	struct Upvalue
	{
		unsigned int	*ptr;
		Upvalue			*next;
	};

	unsigned int	namespaceHash;

	unsigned int	bytesToPop; // Arguments size
	unsigned int	stackSize; // Including arguments

	// For generic functions
	unsigned int	genericOffsetStart; // Position in the lexeme stream of the definition
	unsigned int	genericOffset;
	unsigned int	genericReturnType;

	// Size of the explicit type list for generic functions and generic function instances
	unsigned int	explicitTypeCount;

	unsigned int	nameHash;

	unsigned int	definitionModule; // Index of the module containing the definition

	unsigned int	definitionLocationModule;
	unsigned int	definitionLocationStart;
	unsigned int	definitionLocationEnd;
	unsigned int	definitionLocationName;
};

struct ExternTypedefInfo
{
	unsigned		offsetToName;
	unsigned		targetType;
	unsigned		parentType;
};

struct ExternModuleInfo
{
	unsigned int	nameHash;
	unsigned int	nameOffset;

	unsigned int	funcStart;
	unsigned int	funcCount;

	unsigned int	variableOffset;

	unsigned int	sourceOffset;
	unsigned int	sourceSize;

	unsigned int	dependencyStart;
	unsigned int	dependencyCount;
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
	unsigned int	offsetToName;
	unsigned int	parentHash;
};

struct ExternSourceInfo
{
	unsigned int	instruction;
	unsigned int	definitionModule; // Index of the module containing the definition
	unsigned int	sourceOffset;
};

struct ByteCode
{
	unsigned int	size;	// Overall size

	unsigned int	typeCount;

	unsigned int	dependsCount;
	unsigned int	offsetToFirstModule;
	
	unsigned int	globalVarSize;	// size of all global variables, in bytes
	unsigned int	variableCount;	// variable info count
	unsigned int	variableExportCount;	// exported variable count
	unsigned int	offsetToFirstVar;

	unsigned int	functionCount;
	unsigned int	moduleFunctionCount;
	unsigned int	offsetToFirstFunc;	// Offset from the beginning of a structure to the first ExternFuncInfo data

	unsigned int	localCount;
	unsigned int	offsetToLocals;

	unsigned int	closureListCount;

	unsigned int	regVmInfoSize;
	unsigned int	regVmOffsetToInfo;

	unsigned int	regVmCodeSize;
	unsigned int	regVmOffsetToCode;
	unsigned int	regVmGlobalCodeStart;

	unsigned int	regVmConstantCount;
	unsigned int	regVmOffsetToConstants;

	unsigned int	regVmRegKillInfoCount;
	unsigned int	regVmOffsetToRegKillInfo;

	unsigned int	symbolLength;
	unsigned int	offsetToSymbols;

	unsigned int	sourceSize;
	unsigned int	offsetToSource;

	unsigned int	llvmSize;
	unsigned int	llvmOffset;

	unsigned int	typedefCount;
	unsigned int	offsetToTypedef;

	unsigned int	constantCount;
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

//	ExternTypedefInfo	typedefs[typedefCount];

//	ExternNamespaceInfo	namespaces[namespaceCount];

//	char			regVmCode[regVmCodeSize];

//	ExternSourceInfo	regVmSourceInfo[regVmInfoSize];

//	char			debugSymbols[symbolLength];

//	char			source[sourceSize]

//	char			llvmCode[llvmSize];

//	unsigned		regVmConstants[regVmConstantCount];
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
char*				FindRegVmCode(ByteCode *code);
ExternSourceInfo*	FindRegVmSourceInfo(ByteCode *code);
char*				FindSymbols(ByteCode *code);
char*				FindSource(ByteCode *code);
unsigned*			FindRegVmConstants(ByteCode *code);
unsigned char*		FindRegVmRegKillInfo(ByteCode *code);

#endif
