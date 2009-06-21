#pragma once
#include "stdafx.h"

#include "ParseCommand.h"

class NodeZeroOP;

class TypeInfo;
class FunctionInfo;

class FunctionType
{
public:
	FunctionType()
	{
		retType = NULL;
	}

	TypeInfo	*retType;
	vector<TypeInfo*>	paramType;
};

//Information about type
class TypeInfo
{
public:
	enum TypeCategory{ TYPE_COMPLEX, TYPE_VOID, TYPE_INT, TYPE_FLOAT, TYPE_LONG, TYPE_DOUBLE, TYPE_SHORT, TYPE_CHAR, };

	TypeInfo()
	{
		size = 0;
		type = TYPE_VOID;
		refLevel = 0;
		arrLevel = 0;
		arrSize = 1;
		subType = NULL;
		alignBytes = 0;
		paddingBytes = 0;
		funcType = NULL;
	}

	std::string		name;	// base type name
	UINT			size;	// sizeof(type)
	TypeCategory	type;	// type id

	UINT		refLevel;	// reference to a type depth
	UINT		arrLevel;	// array to a type depth

	UINT		arrSize;	// element count for an array

	UINT		alignBytes;
	UINT		paddingBytes;

	TypeInfo	*subType;

	std::string GetTypeName()
	{
		char buf[512];
		if(funcType)
		{
			char *curr = buf + sprintf(buf, "%s ref(", funcType->retType->GetTypeName().c_str());
			for(UINT i = 0; i < funcType->paramType.size(); i++)
			{
				curr += sprintf(curr, "%s", funcType->paramType[i]->GetTypeName().c_str());
				if(i != funcType->paramType.size()-1)
					curr += sprintf(curr, ", ");
			}
			sprintf(curr, ")");
		}
		if(arrLevel && arrSize != -1)
			sprintf(buf, "%s[%d]", subType->GetTypeName().c_str(), arrSize);
		if(arrLevel && arrSize == -1)
			sprintf(buf, "%s[]", subType->GetTypeName().c_str());
		if(refLevel)
			sprintf(buf, "%s ref", subType->GetTypeName().c_str());
		if(arrLevel == 0 && refLevel == 0 && !funcType)
			sprintf(buf, "%s", name.c_str());
		return std::string(buf);
	}
	//TYPE_COMPLEX are structures
	void	AddMember(const std::string& name, TypeInfo* type)
	{
		memberData.push_back(MemberInfo());
		memberData.back().name = name;
		memberData.back().type = type;
		memberData.back().offset = size;
		size += type->size;
	}
	struct MemberInfo
	{
		std::string name;
		TypeInfo*	type;
		UINT		offset;
	};
	vector<MemberInfo>	memberData;

	struct MemberFunction
	{
		std::string		name;
		FunctionInfo	*func;
		NodeZeroOP		*defNode;
	};
	vector<MemberFunction>	memberFunctions;

	FunctionType		*funcType;
};

template<class Ch, class Tr>
basic_ostream<Ch, Tr>& operator<< (basic_ostream<Ch, Tr>& str, TypeInfo info)
{
	str << info.GetTypeName() << " ";
	return str;
}

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
	VariableInfo(std::string newname, UINT newpos, TypeInfo* newtype, bool newisConst=true):
	  name(newname), pos(newpos), varType(newtype), isConst(newisConst), dataReserved(false){}
	std::string	name;		//Variable name
	UINT		pos;		//Variable position in value stack
	bool		isConst;	//Constant flag

	bool		dataReserved;	// Tells if cmdPushV was used for this variable

	TypeInfo*	varType;	//Pointer to the variable type info
};
template<class Ch, class Tr>
basic_ostream<Ch, Tr>& operator<< (basic_ostream<Ch, Tr>& str, VariableInfo var)
{
	if(var.isConst)
		str << "const ";
	str << (*var.varType);
	str << '\'' << var.name << '\'';
	return str;
}

class FunctionInfo
{
public:
	FunctionInfo()
	{
		address = 0;
		funcPtr = NULL;
		retType = NULL;
		visible = true;
		type = NORMAL;
		funcType = NULL;
		allParamSize = 0;
	}
	//shared_ptr<NodeZeroOP>	defNode;	// A node that defines a function
	int			address;				// Address of the beginning of function inside bytecode
	void		*funcPtr;				// Address of the function in memory

	std::string	name;					// Function name
	std::vector<VariableInfo> params;	// Parameter list
	UINT		allParamSize;
	UINT		vTopSize;				// For "return" operator, we need to know,
										// how many variables we need to remove from variable stack
	TypeInfo*	retType;				// Function return type

	bool		visible;				// true until function goes out of scope

	enum FunctionType{ NORMAL, LOCAL, THISCALL };
	FunctionType	type;
	//bool		local;					// false for functions, declared in global scope

	std::vector<std::string> external;	// External variable names

	TypeInfo	*funcType;				// Function type
};

class CallStackInfo
{
public:
	CallStackInfo(){}
	CallStackInfo(char* ncmd, UINT nnums, UINT nfunc): cmd(ncmd), func(nfunc), nums(nnums){}
	char*		cmd;	//Next command position (RET operation will jump there)
	UINT		func;	//Address of beginning of function
	UINT		nums;	//General variable stack size to check if function really returned a value (This will be removed soon)
};

//VarTopInfo holds information about variable stack state
//It is used to destroy variables when then go in and out of scope
class VarTopInfo
{
public:
	VarTopInfo(UINT activeVariableCount, UINT variableStackSize)
	{
		activeVarCnt = activeVariableCount;
		varStackSize = variableStackSize;
	}
	UINT activeVarCnt;	//Active variable count
	UINT varStackSize;	//Variable stack size in bytes
};