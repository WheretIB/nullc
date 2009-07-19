#pragma once
#include "stdafx.h"

#include "ParseCommand.h"

class NodeZeroOP;

class TypeInfo;
class FunctionInfo;

class FunctionType
{
public:
	FunctionType():paramType(16)
	{
		retType = NULL;
	}

	TypeInfo	*retType;
	FastVector<TypeInfo*>	paramType;
};

//Information about type
class TypeInfo
{
public:
	static const unsigned int UNSIZED_ARRAY = (unsigned int)-1;
	static const unsigned int UNSPECIFIED_ALIGNMENT = (unsigned int)-1;
	
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR, };

	TypeInfo(const char *typeName, unsigned int referenceLevel, unsigned int arrayLevel, unsigned int arraySize, TypeInfo *childType, FunctionType *functionType = NULL):memberData(4),memberFunctions(4)
	{
		assert(typeName != NULL || functionType != NULL || referenceLevel != 0 || arrayLevel != 0);
		name = typeName;
		nameHash = name ? GetStringHash(name) : (unsigned int)(~0);

		size = 0;
		type = TYPE_VOID;

		refLevel = referenceLevel;
		arrLevel = arrayLevel;
		arrSize = arraySize;
		subType = childType;

		alignBytes = 0;
		paddingBytes = 0;

		funcType = functionType;

		fullName = NULL;
		fullNameLength = (unsigned int)(~0);
		fullNameHash = (unsigned int)(~0);
	}

	const char		*name;	// base type name
	unsigned int	nameHash;

	const char		*fullName;	// full type name
	unsigned int	fullNameLength;
	unsigned int	fullNameHash;

	unsigned int	size;	// sizeof(type)
	TypeCategory	type;	// type id

	unsigned int	refLevel;	// reference to a type depth
	unsigned int	arrLevel;	// array to a type depth

	unsigned int	arrSize;	// element count for an array

	unsigned int	alignBytes;
	unsigned int	paddingBytes;

	TypeInfo	*subType;

	const char*		GetFullTypeName()
	{
		if(fullName)
			return fullName;
		if(arrLevel && arrSize != TypeInfo::UNSIZED_ARRAY)
		{
			fullName = new char[subType->GetFullNameLength() + 8 + 3]; // 8 for the digits of arrSize, and 3 for '[',']' and \0
			sprintf((char*)fullName, "%s[%d]", subType->GetFullTypeName(), arrSize);
		}else if(arrLevel && arrSize == TypeInfo::UNSIZED_ARRAY){
			fullName = new char[subType->GetFullNameLength() + 3]; // 3 for '[',']' and \0
			sprintf((char*)fullName, "%s[]", subType->GetFullTypeName());
		}else if(refLevel){
			fullName = new char[subType->GetFullNameLength() + 5]; // 5 for " ref" and \0
			sprintf((char*)fullName, "%s ref", subType->GetFullTypeName());
		}else{
			if(funcType)
			{
				// 7 is the length of " ref(", ")" and \0
				unsigned int bufferSize = 7 + funcType->retType->GetFullNameLength() + funcType->paramType.size();
				for(unsigned int i = 0; i < funcType->paramType.size(); i++)
					bufferSize += funcType->paramType[i]->GetFullNameLength() + (i != funcType->paramType.size()-1 ? 2 : 0);
				char *curr = new char[bufferSize+1];
				fullName = curr;
				curr += sprintf(curr, "%s ref(", funcType->retType->GetFullTypeName());
				for(unsigned int i = 0; i < funcType->paramType.size(); i++)
				{
					curr += sprintf(curr, "%s", funcType->paramType[i]->GetFullTypeName());
					if(i != funcType->paramType.size()-1)
						curr += sprintf(curr, ", ");
				}
				sprintf(curr, ")");
			}else{
				fullName = new char[(int)strlen(name)+ 1]; // 1 for \0
				sprintf((char*)fullName, "%s", name);
			}
		}
		fullNameLength = (int)strlen(fullName);
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
	//TYPE_COMPLEX are structures
	void	AddMember(const char *name, TypeInfo* type)
	{
		memberData.push_back(MemberInfo());
		memberData.back().name = name;
		memberData.back().nameHash = GetStringHash(name);
		memberData.back().type = type;
		memberData.back().offset = size;
		size += type->size;
	}
	struct MemberInfo
	{
		const char		*name;
		unsigned int	nameHash;
		TypeInfo*		type;
		unsigned int	offset;
	};
	FastVector<MemberInfo>	memberData;

	struct MemberFunction
	{
		FunctionInfo	*func;
		NodeZeroOP		*defNode;
	};
	FastVector<MemberFunction>	memberFunctions;

	FunctionType		*funcType;
};

static asmStackType podTypeToStackType[] = { STYPE_COMPLEX_TYPE, (asmStackType)0, STYPE_INT, STYPE_DOUBLE, STYPE_LONG, STYPE_DOUBLE, STYPE_INT, STYPE_INT };
static asmDataType podTypeToDataType[] = { DTYPE_COMPLEX_TYPE, (asmDataType)0, DTYPE_INT, DTYPE_FLOAT, DTYPE_LONG, DTYPE_DOUBLE, DTYPE_SHORT, DTYPE_CHAR };

extern TypeInfo*	typeVoid;
extern TypeInfo*	typeInt;
extern TypeInfo*	typeFloat;
extern TypeInfo*	typeLong;
extern TypeInfo*	typeDouble;

class VariableInfo
{
public:
	VariableInfo(){}
	VariableInfo(const char *newname, unsigned int newpos, TypeInfo* newtype, bool newisConst=true):
		name(newname), pos(newpos), isConst(newisConst), dataReserved(false), varType(newtype)
	{
		nameHash = GetStringHash(name);	
	}

	const char		*name;		// Variable name
	unsigned int	nameHash;	// Variable name hash

	unsigned int	pos;		// Variable position in value stack
	bool			isConst;	// Constant flag

	bool			dataReserved;	// Tells if cmdPushV was used for this variable

	TypeInfo*		varType;	// Pointer to the variable type info

	void*		operator new(unsigned int size)
	{
		return variablePool.Allocate(size);
	}
	void		operator delete(void *ptr, unsigned int size)
	{
		(void)ptr; (void)size;
		assert(!"Cannot delete VariableInfo");
	}

	static	ChunkedStackPool<1024>	variablePool;
	static void	DeleteVariableInformation(){ variablePool.Clear(); }
};

class FunctionInfo
{
public:
	FunctionInfo(): params(8), external(8)
	{
		address = 0;
		codeSize = 0;
		funcPtr = NULL;
		retType = NULL;
		visible = true;
		type = NORMAL;
		funcType = NULL;
		allParamSize = 0;
	}
	int			address;				// Address of the beginning of function inside bytecode
	int			codeSize;				// Size of a function bytecode
	void		*funcPtr;				// Address of the function in memory

	const char		*name;				// Function name
	unsigned int	nameHash;

	FastVector<VariableInfo> params;	// Parameter list
	unsigned int	allParamSize;
	unsigned int	vTopSize;				// For "return" operator, we need to know,
										// how many variables we need to remove from variable stack
	TypeInfo*	retType;				// Function return type

	bool		visible;				// true until function goes out of scope

	enum FunctionType{ NORMAL, LOCAL, THISCALL };
	FunctionType	type;

	struct ExternalName
	{
		ExternalName()
		{
			name = NULL;
			nameHash = 0;
		}
		explicit ExternalName(const char *external)
		{
			name = external;
			nameHash = GetStringHash(external);
		}
		const char		*name;
		unsigned int	nameHash;
	};
	FastVector<ExternalName> external;	// External variable names

	TypeInfo	*funcType;				// Function type
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