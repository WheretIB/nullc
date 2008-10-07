#include "stdafx.h"
#pragma once

#include "ParseCommand.h"

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
	}

	std::string		name;	// base type name
	UINT			size;	// sizeof(type)
	TypeCategory	type;	// type id

	UINT		refLevel;	// reference to a type depth
	UINT		arrLevel;	// array to a type depth

	UINT		arrSize;	// element count for an array

	TypeInfo	*subType;

	std::string GetTypeName()
	{
		static char buf[256];
		if(arrLevel)
			sprintf(buf, "%s[%d]", subType->GetTypeName().c_str(), arrSize);
		if(refLevel)
			sprintf(buf, "%s ref", subType->GetTypeName().c_str());
		if(arrLevel == 0 && refLevel == 0)
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
};

template<class Ch, class Tr>
basic_ostream<Ch, Tr>& operator<< (basic_ostream<Ch, Tr>& str, TypeInfo info)
{
	str << info.GetTypeName();
	return str;
}
static asmStackType podTypeToStackType[] = { (asmStackType)0, (asmStackType)0, STYPE_INT, STYPE_DOUBLE, STYPE_LONG, STYPE_DOUBLE, STYPE_INT, STYPE_INT };
static asmDataType podTypeToDataType[] = { (asmDataType)0, (asmDataType)0, DTYPE_INT, DTYPE_FLOAT, DTYPE_LONG, DTYPE_DOUBLE, DTYPE_SHORT, DTYPE_CHAR };

extern TypeInfo*	typeVoid;
extern TypeInfo*	typeInt;
extern TypeInfo*	typeFloat;
extern TypeInfo*	typeLong;
extern TypeInfo*	typeDouble;

class VariableInfo
{
public:
	VariableInfo(){}
	VariableInfo(std::string newname, UINT newpos, TypeInfo* newtype, UINT newcount=1, bool newisConst=true):
	  name(newname), pos(newpos), varType(newtype)/*, count(newcount)*/, isConst(newisConst){}
	std::string	name;		//Variable name
	UINT		pos;		//Variable position in value stack
	bool		isConst;	//Constant flag

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
	//shared_ptr<NodeZeroOP>	defNode;	//A node that defines a function
	UINT		address;				//Address of the beginning of function inside bytecode
	std::string	name;					//Function name
	std::vector<VariableInfo> params;	//Parameter list
	UINT		vTopSize;				//For "return" operator, we need to know,
										//how many variables we need to remove from variable stack
	TypeInfo*	retType;				//Function return type
};

class CallStackInfo
{
public:
	CallStackInfo(){}
	CallStackInfo(UINT ncmd, UINT nnums, UINT nfunc): cmd(ncmd), func(nfunc), nums(nnums){}
	UINT		cmd;	//Next command position (RET operation will jump there)
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