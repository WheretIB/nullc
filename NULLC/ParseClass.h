#pragma once
#ifndef NULLC_PARSECLASS_H
#define NULLC_PARSECLASS_H

#include "stdafx.h"

#include "InstructionSet.h"
#include "Compiler.h"

class NodeZeroOP;
class NodeExpressionList;

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
		paramSize = NULLC_PTR_SIZE;
	}

	TypeInfo		*retType;
	TypeInfo		**paramType;	// Array of pointers to type information
	unsigned int	paramCount;
	unsigned int	paramSize;
};

class GenericContext
{
public:
	GenericContext()
	{
		start = 0;
		globalVarTop = 0;
		blockDepth = 0;
		parent = NULL;
		instancedType = NULL;
		aliasCount = 0;
	}

	unsigned		start;
	unsigned		globalVarTop;
	unsigned		blockDepth;
	FunctionInfo	*parent;
	TypeInfo		*instancedType;
	unsigned		aliasCount; // for generic type
};

static asmStackType podTypeToStackType[] = { STYPE_COMPLEX_TYPE, (asmStackType)0, STYPE_INT, STYPE_DOUBLE, STYPE_LONG, STYPE_DOUBLE, STYPE_INT, STYPE_INT };
static asmDataType podTypeToDataType[] = { DTYPE_COMPLEX_TYPE, (asmDataType)0, DTYPE_INT, DTYPE_FLOAT, DTYPE_LONG, DTYPE_DOUBLE, DTYPE_SHORT, DTYPE_CHAR };

struct AliasInfo
{
	InplaceStr		name;
	unsigned int	nameHash;
	TypeInfo		*type;
	AliasInfo		*next;
};

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
		memberCount = 0;
		subType = childType;
		hasPointers = (!!referenceLevel) || (arraySize == UNSIZED_ARRAY) || (subType && subType->hasPointers);
		dependsOnGeneric = false;
		hasFinalizer = false;

		alignBytes = 0;
		paddingBytes = 0;

		funcType = NULL;

		unsizedType = refType = NULL;
		arrayType = nextArrayType = NULL;

		fullName = NULL;
		fullNameLength = ~0u;
		fullNameHash = ~0u;

		firstVariable = lastVariable = NULL;

		originalIndex = typeIndex = index;
		childAlias = NULL;

		definitionDepth = 1;

		definitionList = NULL;
		genericInfo = NULL;
		genericBase = NULL;

		hasFinished = true;

#ifdef NULLC_LLVM_SUPPORT
		llvmType = NULL;
#endif
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
	bool			hasPointers;
	bool			dependsOnGeneric;
	bool			hasFinalizer;
	bool			hasFinished;

	unsigned int	arrSize;	// element count for an array
	unsigned int	memberCount;

	unsigned int	alignBytes;
	unsigned int	paddingBytes;

	unsigned int	definitionDepth;

	TypeInfo		*subType;

	unsigned int	typeIndex, originalIndex;

	TypeInfo		*refType, *unsizedType, *arrayType, *nextArrayType;

	AliasInfo		*childAlias;

	NodeZeroOP		*definitionList;
	GenericContext	*genericInfo;
	TypeInfo		*genericBase;

#ifdef NULLC_LLVM_SUPPORT
	const void		*llvmType;
#endif

#define LENGTH_CHECK(x) if(x >= 65532) ThrowError(NULL, "ERROR: internal compiler error; type name length is over the limit (65532)");

	const char*		GetFullTypeName()
	{
		if(fullName)
			return fullName;
		if(arrLevel && arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			unsigned int subNameLength = subType->GetFullNameLength();
			LENGTH_CHECK(subNameLength + 8 + 3);
			fullName = (char*)typeInfoPool.Allocate(subNameLength + 8 + 3); // 8 for the digits of arrSize, and 3 for '[', ']' and \0
			memcpy((char*)fullName, subType->GetFullTypeName(), subNameLength);
			fullName[subNameLength] = '[';
			char *curr = PrintInteger(fullName + subNameLength + 1, arrSize);
			curr[0] = ']';
			curr[1] = 0;
			fullNameLength = (int)(curr - fullName + 1);
		}else if(arrLevel && arrSize == TypeInfo::UNSIZED_ARRAY){
			unsigned int subNameLength = subType->GetFullNameLength();
			LENGTH_CHECK(subNameLength + 3);
			fullName = (char*)typeInfoPool.Allocate(subNameLength + 3); // 3 for '[', ']' and \0
			memcpy((char*)fullName, subType->GetFullTypeName(), subNameLength);
			fullName[subNameLength] = '[';
			fullName[subNameLength + 1] = ']';
			fullName[subNameLength + 2] = 0;
			fullNameLength = subNameLength + 2;
		}else if(refLevel){
			unsigned int subNameLength = subType->GetFullNameLength();
			LENGTH_CHECK(subNameLength + 5);
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
				LENGTH_CHECK(bufferSize + 1);
				char *curr = (char*)typeInfoPool.Allocate(bufferSize + 1);
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
	void WriteCNameEscaped(FILE *fOut, const char *realName)
	{
		while(*realName)
		{
			if(*realName == ' ' || *realName == '[' || *realName == ']' || *realName == '(' || *realName == ')' || *realName == ',')
				fwrite("_", 1, 1, fOut);
			else
				fwrite(realName, 1, 1, fOut);
			realName++;
		}
	}
	void OutputCType(FILE *fOut, const char *variable)
	{
		if(arrLevel && arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			WriteCNameEscaped(fOut, GetFullTypeName());
			fprintf(fOut, " %s", variable);
		}else if(arrLevel && arrSize == TypeInfo::UNSIZED_ARRAY){
			fprintf(fOut, "NULLCArray<");
			subType->OutputCType(fOut, "");
			fprintf(fOut, "> %s", variable);
		}else if(refLevel){
			subType->OutputCType(fOut, "");
			fprintf(fOut, "* %s", variable);
		}else if(funcType){
			fprintf(fOut, "NULLCFuncPtr<__typeProxy_");
			WriteCNameEscaped(fOut, GetFullTypeName());
			fprintf(fOut, "> %s", variable);
		}else{
			if(strcmp(name, "auto ref") == 0)
				fprintf(fOut, "NULLCRef %s", variable);
			else if(strcmp(name, "long") == 0)
				fprintf(fOut, "long long %s", variable);
			else if(strcmp(name, "typeid") == 0)
				fprintf(fOut, "unsigned int %s", variable);
			else if(strcmp(name, "auto[]") == 0)
				fprintf(fOut, "NULLCAutoArray %s", variable);
			else{
				const char* realName = name;
				while(*realName)
				{
					if(*realName == ':' || *realName == '$')
						fwrite("_", 1, 1, fOut);
					else
						fwrite(realName, 1, 1, fOut);
					realName++;
				}
				fprintf(fOut, " %s", variable);
			}
		}
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
		memberCount++;
		if(type->hasPointers)
			hasPointers = true;
	}
	struct MemberVariable
	{
		const char		*name;
		unsigned int	nameHash;
		TypeInfo		*type;
		unsigned int	offset;

		MemberVariable	*next;
	};
	MemberVariable	*firstVariable, *lastVariable;

	FunctionType*	CreateFunctionType(TypeInfo *retType, unsigned int paramCount)
	{
		funcType = new (typeInfoPool.Allocate(sizeof(FunctionType))) FunctionType();
		funcType->paramType = (TypeInfo**)typeInfoPool.Allocate(paramCount * sizeof(TypeInfo*));
		funcType->paramCount = paramCount;
		funcType->retType = retType;
		return funcType;
	}
	static AliasInfo*	CreateAlias(InplaceStr name, TypeInfo* type)
	{
		AliasInfo *info = new (typeInfoPool.Allocate(sizeof(AliasInfo))) AliasInfo;
		info->name = name;
		info->nameHash = GetStringHash(name.begin, name.end);
		info->type = type;
		info->next = NULL;
		return info;
	}
	GenericContext*	CreateGenericContext(unsigned genericStart)
	{
		genericInfo = new (typeInfoPool.Allocate(sizeof(GenericContext))) GenericContext();
		genericInfo->start = genericStart;
		return genericInfo;
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

	static unsigned int	GetPoolTop()
	{
		return typeInfoPool.GetSize();
	}
	static void		SetPoolTop(unsigned int top)
	{
		typeInfoPool.ClearTo(top);
	}
	static	ChunkedStackPool<65532>	typeInfoPool;
};

extern TypeInfo*	typeVoid;
extern TypeInfo*	typeInt;
extern TypeInfo*	typeFloat;
extern TypeInfo*	typeLong;
extern TypeInfo*	typeDouble;

class VariableInfo
{
public:
	VariableInfo(FunctionInfo *parent, InplaceStr varName, unsigned int varHash, unsigned int newpos, TypeInfo* newtype, bool global)
	{
		name = varName;
		nameHash = varHash;
		pos = newpos;
		varType = newtype;
		isGlobal = global;
		usedAsExternal = false;
		parentFunction = parent;
		parentType = NULL;
		
		parentModule = 0;
		blockDepth = 0;

		autoDeref = false;

		defaultValue = NULL;
		defaultValueFuncID = ~0u;
		next = prev = NULL;

#ifdef NULLC_LLVM_SUPPORT
		llvmValue = NULL;
#endif
	}

	InplaceStr		name;		// Variable name
	unsigned int	nameHash;	// Variable name hash

	unsigned int	pos;		// Variable position in value stack
	bool			isGlobal, usedAsExternal;
	bool			autoDeref;
	FunctionInfo	*parentFunction;
	TypeInfo		*parentType;

	unsigned int	parentModule;
	unsigned int	blockDepth;

	TypeInfo		*varType;	// Pointer to the variable type info
	NodeZeroOP		*defaultValue;	// Default value code for function parameters
	unsigned int	defaultValueFuncID;

	VariableInfo	*next, *prev;		// For self-organizing lists

#ifdef NULLC_LLVM_SUPPORT
	void	*llvmValue;
#endif

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

	static unsigned int	GetPoolTop()
	{
		return variablePool.GetSize();
	}
	static void		SetPoolTop(unsigned int top)
	{
		variablePool.ClearTo(top);
	}
	static	ChunkedStackPool<65532>	variablePool;
};

class FunctionInfo
{
public:
	explicit FunctionInfo(const char *funcName, unsigned int hash, unsigned int originalHash)
	{
		name = funcName;
		nameLength = (int)strlen(name);
		nameHash = hash;
		nameHashOrig = originalHash;

		address = 0;
		codeSize = 0;
		funcPtr = NULL;
		indexInArr = 0;
		retType = NULL;
		visible = true;
		implemented = false;
		pure = false;
		explicitlyReturned = false;
		genericInstance = false;
		generic = NULL;
		genericBase = NULL;
		functionNode = NULL;
		type = NORMAL;
		funcType = NULL;
		allParamSize = 0;
		parentClass = NULL;
		parentFunc = NULL;

		extraParam = firstParam = lastParam = NULL;
		funcContext = NULL;
		afterNode = NULL;
		paramCount = 0;
		firstExternal = lastExternal = NULL;
		externalCount = 0;
		externalSize = 0;
		closeUpvals = false;
		firstLocal = lastLocal = NULL;
		localCount = 0;

		maxBlockDepth = 0;
		closeListStart = 0;

		childAlias = NULL;
#ifdef NULLC_ENABLE_C_TRANSLATION
		yieldCount = 0;
#endif
#ifdef NULLC_LLVM_SUPPORT
		llvmFunction = NULL;
		llvmImplemented = false;
#endif
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
		memset(lastExternal, 0, sizeof(FunctionInfo::ExternalInfo));
		lastExternal->next = NULL;
		lastExternal->variable = var;
		lastExternal->closurePos = externalSize;
#ifdef _M_X64
		externalSize += 8 + (var->varType->size < 12 ? 12 : var->varType->size);	// Pointer and a place for the union{variable, {pointer, size}}
#else
		externalSize += 4 + (var->varType->size < 8 ? 8 : var->varType->size);	// Pointer and a place for the union{variable, {pointer, size}}
#endif
		externalCount++;
	}
	int			address;				// Address of the beginning of function inside bytecode
	int			codeSize;				// Size of a function bytecode
	void		*funcPtr;				// Address of the function in memory
	int			indexInArr;

	const char		*name;				// Function name
	unsigned int	nameLength;
	unsigned int	nameHash;			// Full function name
	unsigned int	nameHashOrig;		// Hash of a function name without class name
#ifdef NULLC_ENABLE_C_TRANSLATION
	unsigned int	nameInCHash;
#endif

	VariableInfo	*firstParam, *lastParam;	// Parameter list
	VariableInfo	*extraParam;	// closure/this pointer
	VariableInfo	*funcContext;	// context variable that placed outside the local/coroutine function
	NodeExpressionList	*afterNode;	// node with closure initialization
	unsigned int	paramCount;

	unsigned int	allParamSize;
	unsigned int	maxBlockDepth;
	unsigned int	vTopSize;			// For "return" operator, we need to know,
										// how many variables we need to remove from variable stack
	TypeInfo	*retType;				// Function return type
	TypeInfo	*parentClass;
	FunctionInfo	*parentFunc;

	bool		visible;				// true until function goes out of scope
	bool		implemented;			// false if only function prototype has been found.
	bool		pure;					// function is pure and can possibly be evaluated at compile time
	bool		explicitlyReturned;		// an explicit return from function was compiled
	bool		genericInstance;		// function is a generic function instance
	GenericContext	*generic;				// function is a template that will be resolved at the time of calling
	GenericContext	*genericBase;			// pointer to a generic function base context
	void		*functionNode;

	enum FunctionCategory{ NORMAL, LOCAL, THISCALL, COROUTINE };
	FunctionCategory	type;

	struct ExternalInfo
	{
		VariableInfo	*variable;

		bool			targetLocal;	// Target in local scope
		unsigned int	targetPos;		// Target address
		VariableInfo	*targetVar;		// Target variable
		unsigned int	targetFunc;		// Target function ID
		unsigned int	targetDepth;
		unsigned int	closurePos;		// Position in closure

		ExternalInfo	*next;
	};
	ExternalInfo	*firstExternal, *lastExternal;	// External variable names
	unsigned int	externalCount;
	unsigned int	externalSize;
	bool			closeUpvals;
	unsigned int	closeListStart;
#ifdef NULLC_ENABLE_C_TRANSLATION
	unsigned int	yieldCount;
#endif
	VariableInfo	*firstLocal, *lastLocal;	// Local variable list. Filled in when function comes to an end.
	unsigned int	localCount;

	TypeInfo	*funcType;				// Function type

	AliasInfo	*childAlias;

#ifdef NULLC_LLVM_SUPPORT
	void		*llvmFunction;
	bool		llvmImplemented;
#endif

	const char*	GetOperatorName()
	{
		unsigned int offset = 0;
		if(parentClass)
		{
			offset = parentClass->GetFullNameLength();
			offset += 2;
		}
		if(nameLength - offset <= 3)
		{
			if(strcmp(name + offset, "+") == 0)
				return "__operatorAdd";
			if(strcmp(name + offset, "-") == 0)
				return "__operatorSub";
			if(strcmp(name + offset, "*") == 0)
				return "__operatorMul";
			if(strcmp(name + offset, "/") == 0)
				return "__operatorDiv";
			if(strcmp(name + offset, "%") == 0)
				return "__operatorMod";
			if(strcmp(name + offset, "**") == 0)
				return "__operatorPow";
			if(strcmp(name + offset, "<") == 0)
				return "__operatorLess";
			if(strcmp(name + offset, ">") == 0)
				return "__operatorGreater";
			if(strcmp(name + offset, "<=") == 0)
				return "__operatorLEqual";
			if(strcmp(name + offset, ">=") == 0)
				return "__operatorGEqual";
			if(strcmp(name + offset, "==") == 0)
				return "__operatorEqual";
			if(strcmp(name + offset, "!=") == 0)
				return "__operatorNEqual";
			if(strcmp(name + offset, "<<") == 0)
				return "__operatorShiftLeft";
			if(strcmp(name + offset, ">>") == 0)
				return "__operatorShiftRight";
			if(strcmp(name + offset, "=") == 0)
				return "__operatorSet";
			if(strcmp(name + offset, "+=") == 0)
				return "__operatorAddSet";
			if(strcmp(name + offset, "-=") == 0)
				return "__operatorSubSet";
			if(strcmp(name + offset, "*=") == 0)
				return "__operatorMulSet";
			if(strcmp(name + offset, "/=") == 0)
				return "__operatorDivSet";
			if(strcmp(name + offset, "**=") == 0)
				return "__operatorPowSet";
			if(strcmp(name + offset, "%=") == 0)
				return "__operatorModSet";
			if(strcmp(name + offset, "<<=") == 0)
				return "__operatorShlSet";
			if(strcmp(name + offset, ">>=") == 0)
				return "__operatorShrSet";
			if(strcmp(name + offset, "&=") == 0)
				return "__operatorAndSet";
			if(strcmp(name + offset, "|=") == 0)
				return "__operatorOrSet";
			if(strcmp(name + offset, "^=") == 0)
				return "__operatorXorSet";
			if(strcmp(name + offset, "[]") == 0)
				return "__operatorIndex";
			if(strcmp(name + offset, "!") == 0)
				return "__operatorLogNot";
			if(strcmp(name + offset, "~") == 0)
				return "__operatorBitNot";
			if(strcmp(name + offset, "()") == 0)
				return "__operatorFuncCall";
		}
		return NULL;
	}
	GenericContext*	CreateGenericContext(unsigned genericStart)
	{
		generic = new (functionPool.Allocate(sizeof(GenericContext))) GenericContext();
		generic->start = genericStart;
		return generic;
	}
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

	static unsigned int	GetPoolTop()
	{
		return functionPool.GetSize();
	}
	static void		SetPoolTop(unsigned int top)
	{
		functionPool.ClearTo(top);
	}
	static	ChunkedStackPool<65532>	functionPool;
};

//VarTopInfo holds information about variable stack state
//It is used to destroy variables when then go in and out of scope
class VarTopInfo
{
public:
	VarTopInfo()
	{
		activeVarCnt = 0;
		varStackSize = 0;
	}
	VarTopInfo(unsigned int activeVariableCount, unsigned int variableStackSize)
	{
		activeVarCnt = activeVariableCount;
		varStackSize = variableStackSize;
	}
	unsigned int activeVarCnt;	//Active variable count
	unsigned int varStackSize;	//Variable stack size in bytes
};

#endif
