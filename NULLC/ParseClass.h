#pragma once
#include "stdafx.h"

#include "InstructionSet.h"

class NodeZeroOP;

class TypeInfo;
class FunctionInfo;

class FunctionType
{
public:
	FunctionType()
	{
		retType = NULL;
		paramType = NULL;
		paramCount = 0;
	}

	TypeInfo		*retType;
	TypeInfo		**paramType;	// Array of pointers to type information
	unsigned int	paramCount;
};

static asmStackType podTypeToStackType[] = { STYPE_COMPLEX_TYPE, (asmStackType)0, STYPE_INT, STYPE_DOUBLE, STYPE_LONG, STYPE_DOUBLE, STYPE_INT, STYPE_INT };
static asmDataType podTypeToDataType[] = { DTYPE_COMPLEX_TYPE, (asmDataType)0, DTYPE_INT, DTYPE_FLOAT, DTYPE_LONG, DTYPE_DOUBLE, DTYPE_SHORT, DTYPE_CHAR };

//Information about type
class TypeInfo
{
public:
	static const unsigned int UNSIZED_ARRAY = (unsigned int)-1;
	static const unsigned int UNSPECIFIED_ALIGNMENT = (unsigned int)-1;
	
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR, };

	TypeInfo(unsigned int index, const char *typeName, unsigned int referenceLevel, unsigned int arrayLevel, unsigned int arraySize, TypeInfo *childType, TypeCategory cat)
	{
		name = typeName;
		nameHash = name ? GetStringHash(name) : ~0u;

		size = 0;

		type = cat;
		stackType = podTypeToStackType[type];
		dataType = podTypeToDataType[type];

		refLevel = referenceLevel;
		arrLevel = arrayLevel;
		arrSize = arraySize;
		subType = childType;

		alignBytes = 0;
		paddingBytes = 0;

		funcType = NULL;

		refType = NULL;

		fullName = NULL;
		fullNameLength = ~0u;
		fullNameHash = ~0u;

		firstVariable = lastVariable = NULL;
		firstFunction = lastFunction = NULL;

		typeIndex = index;
	}

	const char		*name;	// base type name
	unsigned int	nameHash;

	char			*fullName;	// full type name
	unsigned int	fullNameLength;
	unsigned int	fullNameHash;

	unsigned int	size;	// sizeof(type)

	TypeCategory	type;	// type id
	asmStackType	stackType;
	asmDataType		dataType;

	unsigned int	refLevel;	// reference to a type depth
	unsigned int	arrLevel;	// array to a type depth

	unsigned int	arrSize;	// element count for an array

	unsigned int	alignBytes;
	unsigned int	paddingBytes;

	TypeInfo		*subType;

	unsigned int	typeIndex;

	TypeInfo		*refType;

	const char*		GetFullTypeName()
	{
		if(fullName)
			return fullName;
		if(arrLevel && arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			unsigned int subNameLength = subType->GetFullNameLength();
			fullName = (char*)typeInfoPool.Allocate(subNameLength + 8 + 3); // 8 for the digits of arrSize, and 3 for '[', ']' and \0
			memcpy((char*)fullName, subType->GetFullTypeName(), subNameLength);
			fullName[subNameLength] = '[';
			char *curr = PrintInteger(fullName + subNameLength + 1, arrSize);
			curr[0] = ']';
			curr[1] = 0;
			fullNameLength = (int)(curr - fullName + 1);
		}else if(arrLevel && arrSize == TypeInfo::UNSIZED_ARRAY){
			unsigned int subNameLength = subType->GetFullNameLength();
			fullName = (char*)typeInfoPool.Allocate(subNameLength + 3); // 3 for '[', ']' and \0
			memcpy((char*)fullName, subType->GetFullTypeName(), subNameLength);
			fullName[subNameLength] = '[';
			fullName[subNameLength + 1] = ']';
			fullName[subNameLength + 2] = 0;
			fullNameLength = subNameLength + 2;
		}else if(refLevel){
			unsigned int subNameLength = subType->GetFullNameLength();
			fullName = (char*)typeInfoPool.Allocate(subNameLength + 5); // 5 for " ref" and \0
			memcpy((char*)fullName, subType->GetFullTypeName(), subNameLength);
			memcpy((char*)fullName + subNameLength, " ref", 5);
			fullNameLength = subNameLength + 4;
		}else{
			if(funcType)
			{
				unsigned int retNameLength = funcType->retType->GetFullNameLength();
				// 7 is the length of " ref(", ")" and \0
				unsigned int bufferSize = 7 + retNameLength;
				for(unsigned int i = 0; i < funcType->paramCount; i++)
					bufferSize += funcType->paramType[i]->GetFullNameLength() + (i != funcType->paramCount-1 ? 1 : 0);
				char *curr = (char*)typeInfoPool.Allocate(bufferSize+1);
				fullName = curr;
				memcpy(curr, funcType->retType->GetFullTypeName(), retNameLength);
				memcpy(curr + retNameLength, " ref(", 5);
				curr += retNameLength + 5;
				for(unsigned int i = 0; i < funcType->paramCount; i++)
				{
					memcpy(curr, funcType->paramType[i]->GetFullTypeName(), funcType->paramType[i]->GetFullNameLength());
					curr += funcType->paramType[i]->GetFullNameLength();
					if(i != funcType->paramCount-1)
						*curr++ = ',';
				}
				*curr++ = ')';
				*curr++ = 0;
				fullNameLength = bufferSize - 1;
			}else{
				fullName = (char*)name;
				fullNameLength = (int)strlen(name);
				fullNameHash = nameHash;
				return name;
			}
		}
		fullNameHash = GetStringHash(fullName);
		return fullName;
	}
	unsigned int	GetFullNameLength()
	{
		if(fullName)
			return fullNameLength;
		GetFullTypeName();
		return fullNameLength;
	}
	unsigned int	GetFullNameHash()
	{
		if(fullName)
			return fullNameHash;
		GetFullTypeName();
		return fullNameHash;
	}

	void	AddMemberVariable(const char *name, TypeInfo* type)
	{
		if(!lastVariable)
		{
			firstVariable = lastVariable = (MemberVariable*)typeInfoPool.Allocate(sizeof(MemberVariable));
		}else{
			lastVariable->next = (MemberVariable*)typeInfoPool.Allocate(sizeof(MemberVariable));
			lastVariable = lastVariable->next;
		}
		lastVariable->next = NULL;
		lastVariable->name = name;
		lastVariable->nameHash = GetStringHash(name);
		lastVariable->type = type;
		lastVariable->offset = size;
		size += type->size;
	}
	void	AddMemberFunction()
	{
		if(!lastFunction)
		{
			firstFunction = lastFunction = (MemberFunction*)typeInfoPool.Allocate(sizeof(MemberFunction));
		}else{
			lastFunction->next = (MemberFunction*)typeInfoPool.Allocate(sizeof(MemberFunction));
			lastFunction = lastFunction->next;
		}
		lastFunction->next = NULL;
	}
	struct MemberVariable
	{
		const char		*name;
		unsigned int	nameHash;
		TypeInfo		*type;
		unsigned int	offset;

		MemberVariable	*next;
	};
	struct MemberFunction
	{
		FunctionInfo	*func;

		MemberFunction	*next;
	};
	MemberVariable	*firstVariable, *lastVariable;
	MemberFunction	*firstFunction, *lastFunction;

	FunctionType*	CreateFunctionType(TypeInfo *retType, unsigned int paramCount)
	{
		funcType = new (typeInfoPool.Allocate(sizeof(FunctionType))) FunctionType();
		funcType->paramType = (TypeInfo**)typeInfoPool.Allocate(paramCount * sizeof(TypeInfo*));
		funcType->paramCount = paramCount;
		funcType->retType = retType;
		return funcType;
	}

	FunctionType		*funcType;
// Specialized allocation
	void*		operator new(size_t size)
	{
		return typeInfoPool.Allocate((unsigned int)size);
	}
	void		operator delete(void *ptr, size_t size)
	{
		(void)ptr; (void)size;
		assert(!"Cannot delete TypeInfo");
	}

	static void		SaveBuildinTop()
	{
		buildInSize = typeInfoPool.GetSize();
	}

	static	unsigned int	buildInSize;
	static	ChunkedStackPool<4092>	typeInfoPool;
	static	void	DeleteTypeInformation(){ typeInfoPool.ClearTo(buildInSize); }
};

extern TypeInfo*	typeVoid;
extern TypeInfo*	typeInt;
extern TypeInfo*	typeFloat;
extern TypeInfo*	typeLong;
extern TypeInfo*	typeDouble;

class VariableInfo
{
public:
	VariableInfo(){}
	VariableInfo(InplaceStr varName, unsigned int varHash, unsigned int newpos, TypeInfo* newtype, bool newisConst, bool global):
		name(varName), nameHash(varHash), pos(newpos), isConst(newisConst), dataReserved(false), varType(newtype), isGlobal(global)
	{
	}

	InplaceStr		name;		// Variable name
	unsigned int	nameHash;	// Variable name hash

	unsigned int	pos;		// Variable position in value stack
	bool			isConst;	// Constant flag
	bool			isGlobal;

	bool			dataReserved;	// Tells if cmdPushV was used for this variable

	TypeInfo		*varType;	// Pointer to the variable type info

	VariableInfo	*next, *prev;		// For self-organizing lists

// Specialized allocation
	void*		operator new(size_t size)
	{
		return variablePool.Allocate((unsigned int)size);
	}
	void		operator delete(void *ptr, size_t size)
	{
		(void)ptr; (void)size;
		assert(!"Cannot delete VariableInfo");
	}

	static void		SaveBuildinTop()
	{
		buildInSize = variablePool.GetSize();
	}

	static	unsigned int	buildInSize;
	static	ChunkedStackPool<4092>	variablePool;
	static void	DeleteVariableInformation(){ variablePool.ClearTo(buildInSize); }
};

class FunctionInfo
{
public:
	explicit FunctionInfo(const char *funcName)
	{
		name = funcName;
		nameLength = (int)strlen(name);
		nameHash = GetStringHash(name);

		address = 0;
		codeSize = 0;
		funcPtr = NULL;
		retType = NULL;
		visible = true;
		implemented = false;
		type = NORMAL;
		funcType = NULL;
		allParamSize = 0;
		parentClass = NULL;

		firstParam = lastParam = NULL;
		paramCount = 0;
		firstExternal = lastExternal = NULL;
		externalCount = 0;
		externalSize = 0;
		closeUpvals = false;
		firstLocal = lastLocal = NULL;
		localCount = 0;
	}

	void	AddParameter(VariableInfo *variable)
	{
		if(!lastParam)
		{
			firstParam = lastParam = variable;
			firstParam->prev = NULL;
		}else{
			lastParam->next = variable;
			lastParam->next->prev = lastParam;
			lastParam = lastParam->next;
		}
		lastParam->next = NULL;
		paramCount++;
	}
	void	AddExternal(VariableInfo *var)
	{
		if(!lastExternal)
		{
			firstExternal = lastExternal = (ExternalInfo*)functionPool.Allocate(sizeof(ExternalInfo));
		}else{
			lastExternal->next = (ExternalInfo*)functionPool.Allocate(sizeof(ExternalInfo));
			lastExternal = lastExternal->next;
		}
		lastExternal->next = NULL;
		lastExternal->variable = var;
		lastExternal->closurePos = externalSize;
		externalSize += 4 + (var->varType->size < 8 ? 8 : var->varType->size);	// Pointer and a place for the union{variable, {pointer, size}}
		externalCount++;
	}
	int			address;				// Address of the beginning of function inside bytecode
	int			codeSize;				// Size of a function bytecode
	void		*funcPtr;				// Address of the function in memory

	const char		*name;				// Function name
	unsigned int	nameLength;
	unsigned int	nameHash;

	VariableInfo	*firstParam, *lastParam;	// Parameter list
	unsigned int	paramCount;

	unsigned int	allParamSize;
	unsigned int	vTopSize;			// For "return" operator, we need to know,
										// how many variables we need to remove from variable stack
	TypeInfo	*retType;				// Function return type
	TypeInfo	*parentClass;

	bool		visible;				// true until function goes out of scope
	bool		implemented;			// false if only function prototype has been found.

	enum FunctionCategory{ NORMAL, LOCAL, THISCALL };
	FunctionCategory	type;

	struct ExternalInfo
	{
		VariableInfo	*variable;

		bool			targetLocal;	// Target in local scope
		unsigned int	targetPos;		// Target address
		unsigned int	targetFunc;		// Target function ID
		unsigned int	closurePos;		// Position in closure

		ExternalInfo	*next;
	};
	ExternalInfo	*firstExternal, *lastExternal;	// External variable names
	unsigned int	externalCount;
	unsigned int	externalSize;
	bool			closeUpvals;

	VariableInfo	*firstLocal, *lastLocal;	// Local variable list. Filled in when function comes to an end.
	unsigned int	localCount;

	TypeInfo	*funcType;				// Function type

// Specialized allocation
	void*		operator new(size_t size)
	{
		return functionPool.Allocate((unsigned int)size);
	}
	void		operator delete(void *ptr, size_t size)
	{
		(void)ptr; (void)size;
		assert(!"Cannot delete FunctionInfo");
	}

	static void		SaveBuildinTop()
	{
		buildInSize = functionPool.GetSize();
	}

	static	unsigned int	buildInSize;
	static	ChunkedStackPool<4092>	functionPool;
	static void	DeleteFunctionInformation(){ functionPool.ClearTo(buildInSize); }
};

//VarTopInfo holds information about variable stack state
//It is used to destroy variables when then go in and out of scope
class VarTopInfo
{
public:
	VarTopInfo(){}
	VarTopInfo(unsigned int activeVariableCount, unsigned int variableStackSize)
	{
		activeVarCnt = activeVariableCount;
		varStackSize = variableStackSize;
	}
	unsigned int activeVarCnt;	//Active variable count
	unsigned int varStackSize;	//Variable stack size in bytes
};
