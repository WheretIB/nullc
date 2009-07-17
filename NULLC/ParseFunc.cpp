#include "stdafx.h"
#include "ParseFunc.h"

#include "CodeInfo.h"
using namespace CodeInfo;

unsigned int GetFuncIndexByPtr(FunctionInfo* funcInfo)
{
	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		if(CodeInfo::funcInfo[i] == funcInfo)
			return i;

	return ~0u;
}

NodeZeroOP*	TakeLastNode()
{
	NodeZeroOP* last = nodeList.back();
	nodeList.pop_back();
	return last;
}

std::vector<unsigned int>	breakAddr;
std::vector<unsigned int>	continueAddr;

static char* binCommandToText[] = { "+", "-", "*", "/", "^", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "bin.and", "bin.or", "bin.xor", "log.and", "log.or", "log.xor"};

//////////////////////////////////////////////////////////////////////////

int	level = 0;
std::string preStr = "--";
bool preNeedChange = false;
void GoDown()
{
	level++;
	preStr = preStr.substr(0, preStr.length()-2);
	preStr += "  |__";
}
void GoDownB()
{
	GoDown();
	preNeedChange = true;
}
void GoUp()
{
	level--;
	preStr = preStr.substr(0, preStr.length()-5);
	preStr += "__";
}
void DrawLine(FILE *fGraph)
{
	fprintf(fGraph, "%s", preStr.c_str());
	if(preNeedChange)
	{
		preNeedChange = false;
		GoUp();
		level++;
		preStr = preStr.substr(0, preStr.length()-2);
		preStr += "   __"; 
	}
}

//Functions for work with types

//This function converts a type according to result type of binary operation between types 'first' and 'second'
//For example,  int * double = double, so first operand will be transformed to double
//				double * int = double, no transformations
asmStackType	ConvertFirstForSecond(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT)
			cmdList.push_back(VMCmd(cmdItoD));
		else
			cmdList.push_back(VMCmd(cmdLtoD));
		return second;
	}
	if(first == STYPE_INT && second == STYPE_LONG)
	{
		cmdList.push_back(VMCmd(cmdItoL));
		return second;
	}
	return first;
}

//This functions transforms first type to second one
void	ConvertFirstToSecond(asmStackType first, asmStackType second)
{
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT)
			cmdList.push_back(VMCmd(cmdItoD));
		else if(first == STYPE_LONG)
			cmdList.push_back(VMCmd(cmdLtoD));
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			cmdList.push_back(VMCmd(cmdItoL));
		else if(first == STYPE_DOUBLE)
			cmdList.push_back(VMCmd(cmdDtoL));
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			cmdList.push_back(VMCmd(cmdDtoI));
		else if(first == STYPE_LONG)
			cmdList.push_back(VMCmd(cmdLtoI));
	}
}

TypeInfo*	ChooseBinaryOpResultType(TypeInfo* a, TypeInfo* b)
{
	if(a->type == TypeInfo::TYPE_DOUBLE)
		return a;
	if(b->type == TypeInfo::TYPE_DOUBLE)
		return b;
	if(a->type == TypeInfo::TYPE_FLOAT)
		return a;
	if(b->type == TypeInfo::TYPE_FLOAT)
		return b;
	if(a->type == TypeInfo::TYPE_LONG)
		return a;
	if(b->type == TypeInfo::TYPE_LONG)
		return b;
	if(a->type == TypeInfo::TYPE_INT)
		return a;
	if(b->type == TypeInfo::TYPE_INT)
		return b;
	if(a->type == TypeInfo::TYPE_SHORT)
		return a;
	if(b->type == TypeInfo::TYPE_SHORT)
		return b;
	if(a->type == TypeInfo::TYPE_CHAR)
		return a;
	if(b->type == TypeInfo::TYPE_CHAR)
		return b;
	assert(false);
	return NULL;
}

std::pair<unsigned int, asmStackType>	ConvertFirstForSecondSize(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
		return std::pair<unsigned int, asmStackType>(1, STYPE_DOUBLE);
	if(first == STYPE_INT && second == STYPE_LONG)
		return std::pair<unsigned int, asmStackType>(1, STYPE_LONG);
	return std::pair<unsigned int, asmStackType>(0, first);
}

unsigned int	ConvertFirstToSecondSize(asmStackType first, asmStackType second)
{
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT || first == STYPE_LONG)
			return 1;
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			return 1;
		else if(first == STYPE_DOUBLE)
			return 1;
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			return 1;
		else if(first == STYPE_LONG)
			return 1;
	}
	return 0;
}

// class implementation

//////////////////////////////////////////////////////////////////////////
// ���� �� ������� �������� �����
NodeZeroOP::NodeZeroOP()
{
	typeInfo = typeVoid;
	strBegin = NULL;
	strEnd = NULL;
}
NodeZeroOP::NodeZeroOP(TypeInfo* tinfo)
{
	typeInfo = tinfo;
}
NodeZeroOP::~NodeZeroOP()
{
}

void NodeZeroOP::Compile()
{
}
void NodeZeroOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ZeroOp\r\n", typeInfo->GetTypeName().c_str());
}
unsigned int NodeZeroOP::GetSize()
{
	return 0;
}
TypeInfo* NodeZeroOP::GetTypeInfo()
{
	return typeInfo;
}

void NodeZeroOP::SetCodeInfo(const char* start, const char* end)
{
	assert(end > start);
	strBegin = start;
	strEnd = end;
}
//////////////////////////////////////////////////////////////////////////
// ����, ������� ���� �������� ����
NodeOneOP::NodeOneOP()
{
	first = NULL;
}
NodeOneOP::~NodeOneOP()
{
	delete first;
}

void NodeOneOP::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	first->Compile();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeOneOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s OneOP :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeOneOP::GetSize()
{
	return first->GetSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ������� ��� �������� ����
NodeTwoOP::NodeTwoOP()
{
	second = NULL;
}
NodeTwoOP::~NodeTwoOP()
{
	delete second;
}

void NodeTwoOP::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	NodeOneOP::Compile();
	second->Compile();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeTwoOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s TwoOp :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeTwoOP::GetSize()
{
	return NodeOneOP::GetSize() + second->GetSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ������� ��� �������� ����
NodeThreeOP::NodeThreeOP()
{
	third = NULL;
}
NodeThreeOP::~NodeThreeOP()
{
	delete third;
}

void NodeThreeOP::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	NodeTwoOP::Compile();
	third->Compile();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeThreeOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ThreeOp :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	third->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeThreeOP::GetSize()
{
	return NodeTwoOP::GetSize() + third->GetSize();
}

//////////////////////////////////////////////////////////////////////////
// ��������������� ������� ��� NodeNumber<T>
void NodeNumberPushCommand(asmDataType dt, char* data)
{
	if(dt == DTYPE_CHAR)
		cmdList.push_back(VMCmd(cmdPushImmt, (int)*data));
	else if(dt == DTYPE_SHORT)
		cmdList.push_back(VMCmd(cmdPushImmt, (int)*(short*)data));
	else if(dt == DTYPE_INT)
		cmdList.push_back(VMCmd(cmdPushImmt, *(int*)data));
	else if(dt == DTYPE_FLOAT){
		double val = (double)*(float*)(data);
		cmdList.push_back(VMCmd(cmdPushImmt, ((int*)(&val))[1]));
		cmdList.push_back(VMCmd(cmdPushImmt, ((int*)(&val))[0]));
	}else if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG){
		cmdList.push_back(VMCmd(cmdPushImmt, *(int*)(data + 4)));
		cmdList.push_back(VMCmd(cmdPushImmt, *(int*)(data)));
	}else{
		assert(!"complex type cannot be pushed immediately");
	}
}
//////////////////////////////////////////////////////////////////////////
// ����, ��������� � ������� ����� ��������, ����������� �������� �����
NodePopOp::NodePopOp()
{
	first = TakeLastNode();
}
NodePopOp::~NodePopOp()
{
}

void NodePopOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	// ��� ��������� ���� ��������� ��������
	first->Compile();
	if(first->GetTypeInfo() != typeVoid)
	{
		// ������� ��� � ������� �����
		cmdList.push_back(VMCmd(cmdPop, first->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX ? first->GetTypeInfo()->size : stackTypeSize[podTypeToStackType[first->GetTypeInfo()->type]]));
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodePopOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s PopOp :\r\n", typeInfo->GetTypeName().c_str());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}
unsigned int NodePopOp::GetSize()
{
	unsigned int size = NodeOneOP::GetSize();
	if(first->GetTypeInfo() != typeVoid)
		size += 1;

	return size;
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� ������� �������� ��� ��������� �� ������� �����
NodeUnaryOp::NodeUnaryOp(CmdID cmd)
{
	// ������� ��������
	cmdID = cmd;

	first = TakeLastNode();
	// ��� ���������� ����� ��, ��� ��������
	typeInfo = first->GetTypeInfo();
}
NodeUnaryOp::~NodeUnaryOp()
{
}

void NodeUnaryOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	// ��� ��������� ���� ��������� ��������
	first->Compile();
	// �������� �������
	if(aOT == OTYPE_INT)
		cmdList.push_back(VMCmd((InstructionCode)cmdID));
	else if(aOT == OTYPE_DOUBLE)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID + 6)));
	else
		cmdList.push_back(VMCmd((InstructionCode)(cmdID + 3)));

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeUnaryOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s UnaryOp :\r\n", typeInfo->GetTypeName().c_str());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeUnaryOp::GetSize()
{
	return NodeOneOP::GetSize() + 1;
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ������� �� ������� ��� �� ���������
NodeReturnOp::NodeReturnOp(unsigned int c, TypeInfo* tinfo)
{
	// ������� �������� ����� ������ �� ����� ������ ����� ���������� (�_�)
	popCnt = c;
	// ��� ���������� ������������ �����
	typeInfo = tinfo;

	first = TakeLastNode();
}
NodeReturnOp::~NodeReturnOp()
{
}

void NodeReturnOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	// ����� ��������, ������� ����� ����������
	first->Compile();
	// ����������� ��� � ��� ����������� �������� �������
	if(typeInfo)
		ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]);

	// ������ �� ������� ��� ���������
	TypeInfo *retType = typeInfo ? typeInfo : first->GetTypeInfo();
	asmOperType operType = operTypeForStackType[podTypeToStackType[retType->type]];

	if(retType->type == TypeInfo::TYPE_COMPLEX || retType->type == TypeInfo::TYPE_VOID)
		cmdList.push_back(VMCmd(cmdReturn, 0, (unsigned short)retType->size, popCnt));
	else
		cmdList.push_back(VMCmd(cmdReturn, 0, (unsigned short)(bitRetSimple | operType), popCnt));

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeReturnOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	if(typeInfo)
		fprintf(fGraph, "%s ReturnOp :\r\n", typeInfo->GetTypeName().c_str());
	else
		fprintf(fGraph, "%s ReturnOp :\r\n", first->GetTypeInfo()->GetTypeName().c_str());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeReturnOp::GetSize()
{
	return NodeOneOP::GetSize() + 1 + (typeInfo ? ConvertFirstToSecondSize(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]) : 0);
}

//////////////////////////////////////////////////////////////////////////
// ����, ���������� ���������. �������� ��� NodeOneOP �� ����������� ������ � ���.
// � �������� ����� ���� ��������������, � �� ��������� �� ����� ������ (���������� void)
// �� ������ ����� ��������� �� ���, ������� ��� ���������
// (����� �� ������� ������ �������, �������� ����� ���� �������������� ��������)
NodeExpression::NodeExpression(TypeInfo* realRetType)
{
	typeInfo = realRetType;

	first = TakeLastNode();
}
NodeExpression::~NodeExpression()
{
}

void NodeExpression::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	NodeOneOP::Compile();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeExpression::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Expression :\r\n", typeInfo->GetTypeName().c_str());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeExpression::GetSize()
{
	return NodeOneOP::GetSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ��������� ����� ��� ����� ����������
NodeVarDef::NodeVarDef(std::string nm)
{
	// ��� ����������
	name = nm;
}
NodeVarDef::~NodeVarDef()
{
}

void NodeVarDef::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeVarDef::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s VarDef '%s'\r\n", typeInfo->GetTypeName().c_str(), name.c_str());
}
unsigned int NodeVarDef::GetSize()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// ���� c ���������� ����� {}
NodeBlock::NodeBlock(unsigned int varShift, bool postPop)
{
	first = TakeLastNode();

	shift = varShift;
	popAfter = postPop;
}
NodeBlock::~NodeBlock()
{
}

void NodeBlock::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// �������� �������� ������� ����� ����������
	cmdList.push_back(VMCmd(cmdPushVTop));
	if(shift)
		cmdList.push_back(VMCmd(cmdPushV, shift));
	// �������� ���������� ����� (�� �� ��� first->Compile())
	first->Compile();
	// ���������� �������� ������� ����� ����������
	if(popAfter)
		cmdList.push_back(VMCmd(cmdPopVTop));

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeBlock::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Block (%d)\r\n", typeInfo->GetTypeName().c_str(), shift);
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeBlock::GetSize()
{
	return first->GetSize() + (popAfter ? 2 : 1) + (shift ? 1 : 0);
}

NodeFuncDef::NodeFuncDef(FunctionInfo *info)
{
	// ��������� �������� �������
	funcInfo = info;

	disabled = false;

	first = TakeLastNode();
}
NodeFuncDef::~NodeFuncDef()
{
}

void NodeFuncDef::Disable()
{
	disabled = true;
}

void NodeFuncDef::Compile()
{
	if(disabled)
		return;
	unsigned int startCmdSize = cmdList.size();

	funcInfo->address = cmdList.size();
	// ����������� ��� �������
	first->Compile();

	if(funcInfo->retType == typeVoid)
	{
		cmdList.push_back(VMCmd(cmdReturn, 0, 0, 1));
		// ���� ������� �� ���������� ��������, �� ��� ������ ret
	}else{
		// ��������� ��������� � �������
		cmdList.push_back(VMCmd(cmdReturn, bitRetError, 0, 1));
	}

	funcInfo->codeSize = cmdList.size() - funcInfo->address;

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeFuncDef::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncDef %s %s\r\n", typeInfo->GetTypeName().c_str(), funcInfo->name.c_str(), (disabled ? " disabled" : ""));
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeFuncDef::GetSize()
{
	if(disabled)
		return 0;
	return first->GetSize() + 1;
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ ����� �������
NodeFuncCall::NodeFuncCall(FunctionInfo *info, FunctionType *type)
{
	// ��������� �������� �������
	funcInfo = info;

	// ��������� �������� ���� �������
	funcType = type;

	// ��� ���������� - ��� ����������� �������� �������
	typeInfo = type->retType;

	if(funcInfo && funcInfo->type == FunctionInfo::LOCAL)
		second = TakeLastNode();

	if(!funcInfo)
		first = TakeLastNode();

	// ������ ���� ������� ���������
	for(unsigned int i = 0; i < type->paramType.size(); i++)
		paramList.push_back(TakeLastNode());

	if(funcInfo && funcInfo->type == FunctionInfo::THISCALL)
		second = TakeLastNode();
}
NodeFuncCall::~NodeFuncCall()
{
	for(paramPtr s = paramList.rbegin(), e = paramList.rend(); s != e; s++)
		delete *s;
}

void NodeFuncCall::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// ���� ������� ���������, ����� �� ��������
	unsigned int currParam = 0;

	bool onlyStackTypes = true;
	if(funcInfo && funcInfo->address == -1 && funcInfo->funcPtr != NULL)
	{
		std::vector<NodeZeroOP*>::iterator s, e;
		s = paramList.begin();
		e = paramList.end();
		for(; s != e; s++)
		{
			// ��������� �������� ���������
			(*s)->Compile();
			// ����������� ��� � ��� �������� ��������� �������
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[paramList.size()-currParam-1]->type]);
			if(funcType->paramType[paramList.size()-currParam-1] == typeFloat)
				cmdList.push_back(VMCmd(cmdDtoF));
			currParam++;
		}
	}else{
		if(!funcInfo || second)
		{
			if(second)
				second->Compile();
			else
				first->Compile();
		}
		std::vector<NodeZeroOP*>::reverse_iterator s, e;
		s = paramList.rbegin();
		e = paramList.rend();
		for(; s != e; s++)
		{
			// ��������� �������� ���������
			(*s)->Compile();
			// ����������� ��� � ��� �������� ��������� �������
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[currParam]->type]);
			if(funcType->paramType[currParam]->type == TypeInfo::TYPE_CHAR ||
				funcType->paramType[currParam]->type == TypeInfo::TYPE_SHORT ||
				funcType->paramType[currParam]->type == TypeInfo::TYPE_FLOAT)
					onlyStackTypes = false;
			currParam++;
		}
	}
	if(funcInfo && funcInfo->address == -1)		// ���� ������� ����������
	{
		// ������� �� �����
		unsigned int ID = GetFuncIndexByPtr(funcInfo);
		cmdList.push_back(VMCmd(cmdCallStd, ID));
	}else{					// ���� ������� ���������� �������������
		// �������� � ��������� ��������� ����� ���, �����
		unsigned int paramSize = 0;
		for(int i = int(funcType->paramType.size())-1; i >= 0; i--)
			paramSize += funcType->paramType[i]->size;
		paramSize += ((!funcInfo || second) ? 4 : 0);
		if(paramSize)
			cmdList.push_back(VMCmd(cmdReserveV, paramSize));

		unsigned int addr = 0;
		if(!onlyStackTypes)
		{
			for(int i = int(funcType->paramType.size())-1; i >= 0; i--)
			{
				asmDataType newDT = podTypeToDataType[funcType->paramType[i]->type];
				cmdList.push_back(VMCmd(cmdPopTypeTop[newDT>>2], newDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)funcType->paramType[i]->size, addr));
				addr += funcType->paramType[i]->size;
			}
		}

		if(!funcInfo || second)
		{
			/*if(second)
				second->Compile();
			else
				first->Compile();*/
			if(!onlyStackTypes)
				cmdList.push_back(VMCmd(cmdPopIntTop, 4, addr));
		}
		if(onlyStackTypes && paramSize != 0)
		{
			if(paramSize == 4)
				cmdList.push_back(VMCmd(cmdPopTypeTop[DTYPE_INT>>2], 0, (unsigned short)paramSize, addr));
			else if(paramSize == 8)
				cmdList.push_back(VMCmd(cmdPopTypeTop[DTYPE_LONG>>2], 0, (unsigned short)paramSize, addr));
			else
				cmdList.push_back(VMCmd(cmdPopTypeTop[DTYPE_COMPLEX_TYPE>>2], 0, (unsigned short)paramSize, addr));
		}


		// ������� �� ������
		unsigned int ID = GetFuncIndexByPtr(funcInfo);
		unsigned short helper = (unsigned short)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[podTypeToStackType[typeInfo->type]]));
		cmdList.push_back(VMCmd(cmdCall, helper, funcInfo ? ID : -1));
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeFuncCall::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncCall '%s' %d\r\n", typeInfo->GetTypeName().c_str(), (funcInfo ? funcInfo->name.c_str() : "$ptr"), paramList.size());
	GoDown();
	if(first)
		first->LogToStream(fGraph);
	if(second)
		second->LogToStream(fGraph);
	for(paramPtr s = paramList.rbegin(), e = paramList.rend(); s != e; s++)
	{
		if(s == --paramList.rend())
		{
			GoUp();
			GoDownB();
		}
		(*s)->LogToStream(fGraph);
	}
	GoUp();
}
unsigned int NodeFuncCall::GetSize()
{
	unsigned int size = 0;
	unsigned int paramSize = ((!funcInfo || second) ? 4 : 0);

	unsigned int currParam = 0;
	bool onlyStackTypes = true;
	for(paramPtr s = paramList.rbegin(), e = paramList.rend(); s != e; s++)
	{
		paramSize += funcType->paramType[currParam]->size;

		size += (*s)->GetSize();
		size += ConvertFirstToSecondSize(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[currParam]->type]);
		if(funcInfo && funcInfo->address == -1 && funcInfo->funcPtr != NULL && funcType->paramType[paramList.size()-currParam-1] == typeFloat)
			size += 1;
		if(funcType->paramType[paramList.size()-currParam-1]->type == TypeInfo::TYPE_CHAR ||
			funcType->paramType[paramList.size()-currParam-1]->type == TypeInfo::TYPE_SHORT ||
			funcType->paramType[paramList.size()-currParam-1]->type == TypeInfo::TYPE_FLOAT)
				onlyStackTypes = false;
		currParam++;
	}
	if(!funcInfo || second)
	{
		if(second)
			size += second->GetSize();
		else
			size += first->GetSize();
		if(!onlyStackTypes)
			size += 1;
	}
	
	if(funcInfo && funcInfo->address == -1)
	{
		size += 1;
	}else{
		if(onlyStackTypes)
			size += (paramSize ? 3 : 1);
		else
			size += (paramSize ? 2 : 1) + (unsigned int)(funcType->paramType.size());
	}

	return size;
}

//////////////////////////////////////////////////////////////////////////
// ����� ���� ��� ��������� �������� ����������
NodeGetAddress::NodeGetAddress(VariableInfo* vInfo, int vAddress, bool absAddr, TypeInfo *retInfo)
{
	//assert(vInfo);
	varInfo = vInfo;
	varAddress = vAddress;
	absAddress = absAddr;

	if(vInfo)
		typeInfo = vInfo->varType;
	else
		typeInfo = retInfo;
}

NodeGetAddress::~NodeGetAddress()
{
}

bool NodeGetAddress::IsAbsoluteAddress()
{
	return absAddress;
}

void NodeGetAddress::IndexArray(int shift)
{
	assert(typeInfo->arrLevel != 0);
	varAddress += typeInfo->subType->size * shift;
	typeInfo = typeInfo->subType;
}

void NodeGetAddress::ShiftToMember(int member)
{
	assert(member < (int)typeInfo->memberData.size());
	varAddress += typeInfo->memberData[member].offset;
	typeInfo = typeInfo->memberData[member].type;
}

void NodeGetAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	if(absAddress)
		cmdList.push_back(VMCmd(cmdPushImmt, varAddress));
	else
		cmdList.push_back(VMCmd(cmdGetAddr, varAddress));

	assert((cmdList.size()-startCmdSize) == GetSize());
}

void NodeGetAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s GetAddress ", GetReferenceType(typeInfo)->GetTypeName().c_str());
	if(varInfo)
		fprintf(fGraph, "%s%s '%s'", (varInfo->isConst ? "const " : ""), varInfo->varType->GetTypeName().c_str(), varInfo->name.c_str());
	else
		fprintf(fGraph, "$$$");
	fprintf(fGraph, " (%d %s)\r\n", (int)varAddress, (absAddress ? " absolute" : " relative"));
}

unsigned int NodeGetAddress::GetSize()
{
	return 1;
}

TypeInfo* NodeGetAddress::GetTypeInfo()
{
	return GetReferenceType(typeInfo);
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ���������� �������� ����������

NodeVariableSet::NodeVariableSet(TypeInfo* targetType, unsigned int pushVar, bool swapNodes)
{
	assert(targetType);
	typeInfo = targetType;

	if(swapNodes)
		second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->GetTypeInfo()->refLevel != 0);

	if(!swapNodes)
		second = TakeLastNode();

	// ���� ��� ������ ����������� ���������� � ������� ������������� ������� ���
	arrSetAll = (pushVar && typeInfo->arrLevel != 0 && second->GetTypeInfo()->arrLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && second->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX);

	if(second->GetTypeInfo() == typeVoid)
	{
		lastError = CompilerError("ERROR: cannot convert from void to " + typeInfo->GetTypeName(), lastKnownStartPos);
		return;
	}
	if(typeInfo == typeVoid)
	{
		lastError = CompilerError("ERROR: cannot convert from " + second->GetTypeInfo()->GetTypeName() + " to void", lastKnownStartPos);
		return;
	}

	// ���� ���� �� �����
	if(second->GetTypeInfo() != typeInfo)
	{
		// ���� ��� �� ���������� ������� ����, ���
		// ���� ����������� ����������� ��������, � ��� ���� �� ���������� ������ ����������� ����������, ���
		// ���� ����������� ������� ����������, ���
		// ���� ��� ���������, ������� ���������� �����, �� ��� ���� ���, �� ������� ��������� ��������� ����������, ��
		// ������� �� ������ �������������� �����
		if(!(typeInfo->type != TypeInfo::TYPE_COMPLEX && second->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX) ||
			(typeInfo->arrLevel != second->GetTypeInfo()->arrLevel && !arrSetAll) ||
			(typeInfo->refLevel != second->GetTypeInfo()->refLevel) ||
			(typeInfo->refLevel && typeInfo->refLevel == second->GetTypeInfo()->refLevel && typeInfo->subType != second->GetTypeInfo()->subType))
		{
			if(!(typeInfo->arrLevel != 0 && second->GetTypeInfo()->arrLevel == 0 && arrSetAll))
			{
				lastError = CompilerError("ERROR: Cannot convert '" + second->GetTypeInfo()->GetTypeName() + "' to '" + typeInfo->GetTypeName() + "'", lastKnownStartPos);
				return;
			}
		}
	}

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->GetNodeType() == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->GetNodeType() == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
		delete oldFirst;
	}
	if(first->GetNodeType() == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
		delete oldFirst;
	}
}

NodeVariableSet::~NodeVariableSet()
{
}


void NodeVariableSet::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	asmStackType asmST = podTypeToStackType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)];
	asmDataType asmDT = podTypeToDataType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)];

	second->Compile();
	ConvertFirstToSecond(podTypeToStackType[second->GetTypeInfo()->type], asmST);

	if(!knownAddress)
		first->Compile();
	if(arrSetAll)
	{
		assert(knownAddress);
		cmdList.push_back(VMCmd(cmdPushImmt, typeInfo->size / typeInfo->subType->size));
		cmdList.push_back(VMCmd(cmdSetRange, (unsigned short)(asmDT), addrShift));
	}else{
		if(knownAddress)
		{
			if(absAddress)
				cmdList.push_back(VMCmd(cmdMovTypeAbs[asmDT>>2], asmST == STYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
			else
				cmdList.push_back(VMCmd(cmdMovTypeRel[asmDT>>2], asmST == STYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		}else{
			cmdList.push_back(VMCmd(cmdMovTypeStk[asmDT>>2], asmST == STYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		}
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}

void NodeVariableSet::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s VariableSet %s\r\n", typeInfo->GetTypeName().c_str(), (arrSetAll ? "set all elements" : ""));
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

unsigned int NodeVariableSet::GetSize()
{
	unsigned int size = second->GetSize();
	if(!knownAddress)
		size += first->GetSize();
	size += ConvertFirstToSecondSize(podTypeToStackType[second->GetTypeInfo()->type], podTypeToStackType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)]);
	if(arrSetAll)
		size += 2;
	else
		size += 1;
	return size;
}

TypeInfo* NodeVariableSet::GetTypeInfo()
{
	return (arrSetAll ? typeInfo->subType : typeInfo);
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ��������� �������� ���������� (�������� += -= *= /= � �.�.)

NodeVariableModify::NodeVariableModify(TypeInfo* targetType, CmdID cmd)
{
	assert(targetType);
	typeInfo = targetType;

	cmdID = cmd;

	second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->GetTypeInfo()->refLevel != 0);

	if(second->GetTypeInfo() == typeVoid)
	{
		lastError = CompilerError("ERROR: cannot convert from void to " + typeInfo->GetTypeName(), lastKnownStartPos);
		return;
	}
	if(typeInfo == typeVoid)
	{
		lastError = CompilerError("ERROR: cannot convert from " + second->GetTypeInfo()->GetTypeName() + " to void", lastKnownStartPos);
		return;
	}

	// ���� ���� �� �����
	if(second->GetTypeInfo() != typeInfo)
	{
		// ���� ��� �� ���������� ������� ����, ���
		// ���� ����������� ������� ����������, ���
		// ���� ��� ���������, ������� ���������� �����, �� ��� ���� ���, �� ������� ��������� ��������� ����������, ��
		// ������� �� ������ �������������� �����
		if(!(typeInfo->type != TypeInfo::TYPE_COMPLEX && second->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX) ||
			(typeInfo->arrLevel != second->GetTypeInfo()->arrLevel) ||
			(typeInfo->refLevel != second->GetTypeInfo()->refLevel) ||
			(typeInfo->refLevel && typeInfo->refLevel == second->GetTypeInfo()->refLevel && typeInfo->subType != second->GetTypeInfo()->subType))
		{
			lastError = CompilerError("ERROR: Cannot convert '" + second->GetTypeInfo()->GetTypeName() + "' to '" + typeInfo->GetTypeName() + "'", lastKnownStartPos);
			return;
		}
	}

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->GetNodeType() == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->GetNodeType() == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
		delete oldFirst;
	}
	if(first->GetNodeType() == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
		delete oldFirst;
	}
}

NodeVariableModify::~NodeVariableModify()
{
}

void NodeVariableModify::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	asmStackType asmSTfirst = podTypeToStackType[typeInfo->type];
	asmDataType asmDT = podTypeToDataType[typeInfo->type];

	asmStackType asmSTsecond = podTypeToStackType[second->GetTypeInfo()->type];

	// ���� ����, ��������� ����� ������� ��������
	if(!knownAddress)
		first->Compile();

	// � ������� ��� � ����
	if(knownAddress)
	{
		if(absAddress)
			cmdList.push_back(VMCmd(cmdPushTypeAbs[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		else
			cmdList.push_back(VMCmd(cmdPushTypeRel[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}

	// �����������, ���� ����, � ���, ������� ���������� ����� ���������� ��������� ��������
	asmStackType asmSTresult = ConvertFirstForSecond(asmSTfirst, asmSTsecond);

	// ��������� ������ �������
	second->Compile();

	// �����������, ���� ����, � ���, ������� ���������� ����� ���������� ��������� ��������
	ConvertFirstForSecond(asmSTsecond, asmSTresult);

	// ��������� �������� �� ����������
	if(asmSTresult == STYPE_INT)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID)));
	else if(asmSTresult == STYPE_LONG)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddL)));
	else if(asmSTresult == STYPE_DOUBLE)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddD)));
	else
		assert(!"unknown operator type in NodeVariableModify");

	// ����������� ��������� � ��� ������� ��������
	ConvertFirstToSecond(asmSTresult, asmSTfirst);

	// ���� ����, ��������� ����� ������� ��������
	if(!knownAddress)
		first->Compile();

	// � ������� ����� �������� ����������
	if(knownAddress)
	{
		if(absAddress)
			cmdList.push_back(VMCmd(cmdMovTypeAbs[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		else
			cmdList.push_back(VMCmd(cmdMovTypeRel[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdMovTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}

void NodeVariableModify::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s VariableModify\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

unsigned int NodeVariableModify::GetSize()
{
	asmStackType asmSTfirst = podTypeToStackType[typeInfo->type];
	asmStackType asmSTsecond = podTypeToStackType[second->GetTypeInfo()->type];

	unsigned int size = second->GetSize();
	if(!knownAddress)
		size += 2 * first->GetSize();
	size += ConvertFirstForSecondSize(asmSTfirst, asmSTsecond).first;
	asmStackType asmSTresult = ConvertFirstForSecondSize(asmSTfirst, asmSTsecond).second;
	size += ConvertFirstForSecondSize(asmSTsecond, asmSTresult).first;
	size += ConvertFirstToSecondSize(asmSTresult, asmSTfirst);
	size += 3;
	return size;
}

TypeInfo* NodeVariableModify::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ��������� �������� �������
NodeArrayIndex::NodeArrayIndex(TypeInfo* parentType)
{
	assert(parentType);
	typeParent = parentType;
	typeInfo = GetReferenceType(parentType->subType);

	// �������� ����, ������������� ������
	second = TakeLastNode();

	// �������� ����, ������������� ����� ������ �������
	first = TakeLastNode();

	shiftValue = 0;
	knownShift = false;

	if(second->GetNodeType() == typeNodeNumber)
	{
		TypeInfo *aType = second->GetTypeInfo();
		NodeZeroOP* zOP = second;
		if(aType == typeDouble)
		{
			shiftValue = typeParent->subType->size * (int)static_cast<NodeNumber<double>* >(zOP)->GetVal();
		}else if(aType == typeFloat){
			shiftValue = typeParent->subType->size * (int)static_cast<NodeNumber<float>* >(zOP)->GetVal();
		}else if(aType == typeLong){
			shiftValue = typeParent->subType->size * (int)static_cast<NodeNumber<long long>* >(zOP)->GetVal();
		}else if(aType == typeInt){
			shiftValue = typeParent->subType->size * static_cast<NodeNumber<int>* >(zOP)->GetVal();
		}else{
			lastError = CompilerError("NodeArrayIndex() ERROR: unknown type " + aType->name, lastKnownStartPos);
			return;
		}
		knownShift = true;
	}
}

NodeArrayIndex::~NodeArrayIndex()
{
}

void NodeArrayIndex::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	asmOperType oAsmType = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// ������ ��������� �� ������ �������
	first->Compile();

	if(knownShift)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, shiftValue));
	}else{
		// �������� ������
		second->Compile();
		// �������� ��� � ����� �����.  ������� �� ������ ��������
		if(typeParent->subType->size != 1)
		{
			cmdList.push_back(VMCmd(cmdImmtMulType[oAsmType], typeParent->subType->size));
		}else{
			if(oAsmType != OTYPE_INT)
				cmdList.push_back(VMCmd(oAsmType == OTYPE_DOUBLE ? cmdDtoI : cmdLtoI));
		}

	}
	// ������ � �������, ������� ��� �� �������
	cmdList.push_back(VMCmd(cmdAdd));

	assert((cmdList.size()-startCmdSize) == GetSize());
}

void NodeArrayIndex::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ArrayIndex %s known: %d shiftval: %d\r\n", typeInfo->GetTypeName().c_str(), typeParent->GetTypeName().c_str(), knownShift, shiftValue);
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

unsigned int NodeArrayIndex::GetSize()
{
	if(knownShift)
		return first->GetSize() + 2;
	// else
	return first->GetSize() + second->GetSize() + 1 + (typeParent->subType->size == 1 && podTypeToStackType[second->GetTypeInfo()->type] == STYPE_INT ? 0 : 1);
}

TypeInfo* NodeArrayIndex::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ������ �������� �� ���������

NodeDereference::NodeDereference(TypeInfo* type)
{
	assert(type);
	typeInfo = type;

	first = TakeLastNode();
	assert(first->GetTypeInfo()->refLevel != 0);

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->GetNodeType() == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->GetNodeType() == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
		delete oldFirst;
	}
	if(first->GetNodeType() == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
		delete oldFirst;
	}
}

NodeDereference::~NodeDereference()
{
}


void NodeDereference::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	asmDataType asmDT = podTypeToDataType[typeInfo->type];
	
	if(!knownAddress)
		first->Compile();

	if(knownAddress)
	{
		if(absAddress)
			cmdList.push_back(VMCmd(cmdPushTypeAbs[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		else
			cmdList.push_back(VMCmd(cmdPushTypeRel[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}

void NodeDereference::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Dereference\r\n", typeInfo->GetTypeName().c_str());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

unsigned int NodeDereference::GetSize()
{
	return (!knownAddress ? first->GetSize() : 0) + 1;
}

TypeInfo* NodeDereference::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// ���� ���������� ����� �� ����� ������
NodeShiftAddress::NodeShiftAddress(unsigned int shift, TypeInfo* resType)
{
	memberShift = shift;
	typeInfo = GetReferenceType(resType);

	first = TakeLastNode();
}

NodeShiftAddress::~NodeShiftAddress()
{
}


void NodeShiftAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	first->Compile();

	if(memberShift)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, memberShift));
		// ������ � �������, ������� ��� �� �������
		cmdList.push_back(VMCmd(cmdAdd));
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}

void NodeShiftAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ShiftAddress\r\n", typeInfo->GetTypeName().c_str());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

unsigned int NodeShiftAddress::GetSize()
{
	unsigned int retSize = first->GetSize();
	if(memberShift)
		retSize += 2;
	return retSize;
}

TypeInfo* NodeShiftAddress::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� �������������� ��� ��������������
NodePreOrPostOp::NodePreOrPostOp(TypeInfo* resType, bool isInc, bool preOp)
{
	assert(resType);
	typeInfo = resType;

	first = TakeLastNode();
	assert(first->GetTypeInfo()->refLevel != 0);

	incOp = isInc;

	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0)
	{
		lastError = CompilerError(std::string("ERROR: ") + (isInc ? "Increment" : "Decrement") + std::string(" is not supported on '") + typeInfo->GetTypeName() + "'", lastKnownStartPos);
		return;
	}

	prefixOp = preOp;

	optimised = false;

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->GetNodeType() == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->GetNodeType() == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
		delete oldFirst;
	}
	if(first->GetNodeType() == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
		delete oldFirst;
	}
}

NodePreOrPostOp::~NodePreOrPostOp()
{
}


void NodePreOrPostOp::SetOptimised(bool doOptimisation)
{
	optimised = doOptimisation;
}


void NodePreOrPostOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	asmStackType asmST = podTypeToStackType[typeInfo->type];
	asmDataType asmDT = podTypeToDataType[typeInfo->type];
	asmOperType aOT = operTypeForStackType[podTypeToStackType[typeInfo->type]];
	
	if(!knownAddress)
		first->Compile();

	if(knownAddress)
	{
		if(absAddress)
		{
			cmdList.push_back(VMCmd(cmdPushTypeAbs[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
			cmdList.push_back(VMCmd(incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
			cmdList.push_back(VMCmd(cmdMovTypeAbs[asmDT>>2], asmST == STYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
			if(!prefixOp && !optimised)
				cmdList.push_back(VMCmd(!incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
			if(optimised)
				cmdList.push_back(VMCmd(cmdPop, stackTypeSize[asmST]));
		}else{
			cmdList.push_back(VMCmd(cmdPushTypeRel[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
			cmdList.push_back(VMCmd(incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
			cmdList.push_back(VMCmd(cmdMovTypeRel[asmDT>>2], asmST == STYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
			if(!prefixOp && !optimised)
				cmdList.push_back(VMCmd(!incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
			if(optimised)
				cmdList.push_back(VMCmd(cmdPop, stackTypeSize[asmST]));
		}
	}else{
		cmdList.push_back(VMCmd(cmdAddAtTypeStk[asmDT>>2], optimised ? 0 : (prefixOp ? bitPushAfter : bitPushBefore), incOp ? 1 : -1, addrShift));
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}

void NodePreOrPostOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s PreOrPostOp %s\r\n", typeInfo->GetTypeName().c_str(), (prefixOp ? "prefix" : "postfix"));
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

unsigned int NodePreOrPostOp::GetSize()
{
	unsigned int size = (!knownAddress ? first->GetSize() : 0);
	if(knownAddress)
	{
		size += 3;
		if(!prefixOp && !optimised)
			size++;
		if(optimised)
			size++;
	}else{
		size++;
	}
	return size;
}

TypeInfo* NodePreOrPostOp::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// ����, ���������� ����� �������

NodeFunctionAddress::NodeFunctionAddress(FunctionInfo* functionInfo)
{
	funcInfo = functionInfo;
	typeInfo = funcInfo->funcType;

	if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL)
		first = TakeLastNode();
}

NodeFunctionAddress::~NodeFunctionAddress()
{
}


void NodeFunctionAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	unsigned int ID = GetFuncIndexByPtr(funcInfo);
	cmdList.push_back(VMCmd(cmdFuncAddr, ID));

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, 0));
	}else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL){
		first->Compile();
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}

void NodeFunctionAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FunctionAddress %s %s\r\n", typeInfo->GetTypeName().c_str(), funcInfo->name.c_str(), (funcInfo->funcPtr ? " external" : ""));
	if(first)
	{
		GoDownB();
		first->LogToStream(fGraph);
		GoUp();
	}
}

unsigned int NodeFunctionAddress::GetSize()
{
	unsigned int size = 1;
	if(funcInfo->type == FunctionInfo::NORMAL)
		size += 1;
	else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL)
		size += first->GetSize();
	return size;
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� �������� � ����� ����������
NodeTwoAndCmdOp::NodeTwoAndCmdOp(CmdID cmd)
{
	// �������� ��������
	cmdID = cmd;

	second = TakeLastNode();
	first = TakeLastNode();

	// �� ������ ������ �������� � ������������ ������ �����������
	if(first->GetTypeInfo()->refLevel == 0)
	{
		if(first->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX || second->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX)
		{
			lastError = CompilerError("ERROR: Operation " + std::string(binCommandToText[cmdID - cmdAdd]) + " is not supported on '" + first->GetTypeInfo()->GetTypeName() + "' and '" + second->GetTypeInfo()->GetTypeName() + "'", lastKnownStartPos);
			return;
		}
	}
	if(first->GetTypeInfo() == typeVoid)
	{
		lastError = CompilerError("ERROR: first operator returns void", lastKnownStartPos);
		return;
	}
	if(second->GetTypeInfo() == typeVoid)
	{
		lastError = CompilerError("ERROR: second operator returns void", lastKnownStartPos);
		return;
	}

	// ����� �������������� ���, ����� ���������� ��������
	typeInfo = ChooseBinaryOpResultType(first->GetTypeInfo(), second->GetTypeInfo());
}
NodeTwoAndCmdOp::~NodeTwoAndCmdOp()
{
}

void NodeTwoAndCmdOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	asmStackType fST = podTypeToStackType[first->GetTypeInfo()->type], sST = podTypeToStackType[second->GetTypeInfo()->type];
	
	// ����� ������ ��������
	first->Compile();
	// �����������, ���� ����, � ���, ������� ���������� ����� ���������� ��������� ��������
	fST = ConvertFirstForSecond(fST, sST);
	// ����� ������ ��������
	second->Compile();
	// �����������, ���� ����, � ���, ������� ���������� ����� ���������� ��������� ��������
	sST = ConvertFirstForSecond(sST, fST);
	// ��������� �������� �� ����������
	if(fST == STYPE_INT)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID)));
	else if(fST == STYPE_LONG)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddL)));
	else if(fST == STYPE_DOUBLE)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddD)));
	else
		assert(!"unknown operator type in NodeTwoAndCmdOp");

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeTwoAndCmdOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s TwoAndCmd<%s> :\r\n", typeInfo->GetTypeName().c_str(), binCommandToText[cmdID-cmdAdd]);
	assert(cmdID >= cmdAdd);
	assert(cmdID <= cmdNEqualD);
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeTwoAndCmdOp::GetSize()
{
	asmStackType fST = podTypeToStackType[first->GetTypeInfo()->type], sST = podTypeToStackType[second->GetTypeInfo()->type];
	unsigned int resSize = 0;
	resSize += ConvertFirstForSecondSize(fST, sST).first;
	fST = ConvertFirstForSecondSize(fST, sST).second;
	resSize += ConvertFirstForSecondSize(sST, fST).first;
	return NodeTwoOP::GetSize() + 1 + resSize;
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ���� if(){}else{} ��� �������� �������� ?:
NodeIfElseExpr::NodeIfElseExpr(bool haveElse, bool isTerm)
{
	// ���� ������� ���� else{}
	if(haveElse)
	{
		third = TakeLastNode();
	}
	second = TakeLastNode();
	first = TakeLastNode();
	// ���� ��� �������� ��������, �� ������� ��� ���������� �������� �� void
	// ������������� ������ �������, ����� ������ �������������� �������� ����� ������ ����.
	// ������� ���������! BUG 0003
	if(isTerm)
		typeInfo = second->GetTypeInfo();
}
NodeIfElseExpr::~NodeIfElseExpr()
{
}

void NodeIfElseExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	// ��������� �������� ���������: if(first) second; else third;
	// ������ �������: first ? second : third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// �������� �������
	first->Compile();

	// ���� false, ������� � ���� else ��� ������ �� ���������, ���� ������ ����� �� �������
	cmdList.push_back(VMCmd(cmdJmpZType[aOT], 1 + cmdList.size() + second->GetSize() + (third ? 1 : 0)));

	// �������� ���� ��� ��������� ����������� ������� (true)
	second->Compile();
	// ���� ���� ���� else, �������� ���
	if(third)
	{
		// ������ �������� ����� �� ��������� ����� ��� �����, ����� �� ��������� ��� �����
		cmdList.push_back(VMCmd(cmdJmp, 1 + cmdList.size() + third->GetSize()));

		// �������� ���� else (false)
		third->Compile();
	}

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeIfElseExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s IfExpression :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	if(!third)
	{
		GoUp();
		GoDownB();
	}
	second->LogToStream(fGraph);
	if(third)
	{
		GoUp();
		GoDownB();
		third->LogToStream(fGraph);
	}
	GoUp();
}
unsigned int NodeIfElseExpr::GetSize()
{
	unsigned int size = first->GetSize() + second->GetSize() + 1;
	if(third)
		size += third->GetSize() + 1;
	return size;
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ���� for(){}
NodeForExpr::NodeForExpr()
{
	fourth = TakeLastNode();
	third = TakeLastNode();
	second = TakeLastNode();
	first = TakeLastNode();
}
NodeForExpr::~NodeForExpr()
{
	delete fourth;
}

void NodeForExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList->AddDescription(cmdList.size(), strBegin, strEnd);

	// ��������� �������� ���������: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// �������� �������������
	first->Compile();
	unsigned int posTestExpr = cmdList.size();

	// ����� ��������� �������
	second->Compile();

	// �������� ����� ��� ������ �� ����� ���������� break;
	breakAddr.push_back(cmdList.size() + 1 + third->GetSize() + fourth->GetSize() + 1);

	// ���� �����, ������ �� �����
	cmdList.push_back(VMCmd(cmdJmpZType[aOT], breakAddr.back()));

	// �������� ����� ��� �������� � ��������� �������� ���������� continue;
	continueAddr.push_back(cmdList.size()+fourth->GetSize());

	// �������� ���������� �����
	fourth->Compile();
	// �������� ��������, ���������� ����� ������ ��������
	third->Compile();
	// ������� �� �������� �������
	cmdList.push_back(VMCmd(cmdJmp, posTestExpr));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeForExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ForExpression :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	third->LogToStream(fGraph);
	GoUp();
	GoDownB(); 
	fourth->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeForExpr::GetSize()
{
	return NodeThreeOP::GetSize() + fourth->GetSize() + 2;
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ���� while(){}
NodeWhileExpr::NodeWhileExpr()
{
	second = TakeLastNode();
	first = TakeLastNode();
}
NodeWhileExpr::~NodeWhileExpr()
{
}

void NodeWhileExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// ��������� �������� ���������: while(first) second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	unsigned int posStart = cmdList.size();
	// �������� �������
	first->Compile();

	// �������� ����� ��� ������ �� ����� ���������� break;
	breakAddr.push_back(cmdList.size() + 1 + second->GetSize() + 1);

	// ���� ��� �����, ������ �� �����
	cmdList.push_back(VMCmd(cmdJmpZType[aOT], breakAddr.back()));

	// �������� ����� ��� �������� � ��������� �������� ���������� continue;
	continueAddr.push_back(cmdList.size() + second->GetSize());

	// �������� ���������� �����
	second->Compile();
	// ������� �� �������� �������
	cmdList.push_back(VMCmd(cmdJmp, posStart));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeWhileExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s WhileExpression :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB(); 
	second->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeWhileExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + 2;
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ���� do{}while()
NodeDoWhileExpr::NodeDoWhileExpr()
{
	second = TakeLastNode();
	first = TakeLastNode();
}
NodeDoWhileExpr::~NodeDoWhileExpr()
{
}

void NodeDoWhileExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// ��������� �������� ���������: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	unsigned int posStart = cmdList.size();
	// �������� ����� ��� ������ �� ����� ���������� break;
	breakAddr.push_back(cmdList.size()+first->GetSize()+second->GetSize()+1);

	// �������� ����� ��� �������� � ��������� �������� ���������� continue;
	continueAddr.push_back(cmdList.size()+first->GetSize());

	// �������� ���������� �����
	first->Compile();
	// �������� �������
	second->Compile();
	// ���� ������� �����, ������� � ���������� ��������� �������� �����
	cmdList.push_back(VMCmd(cmdJmpNZType[aOT], posStart));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeDoWhileExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s DoWhileExpression :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}
unsigned int NodeDoWhileExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + 1;
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� break;
NodeBreakOp::NodeBreakOp(unsigned int c)
{
	// ������� �������� ����� ������ �� ����� ������ ����� ���������� (�_�)
	popCnt = c;
}
NodeBreakOp::~NodeBreakOp()
{
}

void NodeBreakOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// ����� �������� �� ����� ������ ����� ����������
	for(unsigned int i = 0; i < popCnt; i++)
		cmdList.push_back(VMCmd(cmdPopVTop));
	// ������ �� �����
	cmdList.push_back(VMCmd(cmdJmp, breakAddr.back()));

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeBreakOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s BreakExpression\r\n", typeInfo->GetTypeName().c_str());
}
unsigned int NodeBreakOp::GetSize()
{
	return (1+popCnt);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ ��������� ������� � ��������� �������� �����

NodeContinueOp::NodeContinueOp(unsigned int c)
{
	// ������� �������� ����� ������ �� ����� ������ ����� ���������� (�_�)
	popCnt = c;
}
NodeContinueOp::~NodeContinueOp()
{
}

void NodeContinueOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// ����� �������� �� ����� ������ ����� ����������
	for(unsigned int i = 0; i < popCnt; i++)
		cmdList.push_back(VMCmd(cmdPopVTop));

	// ������ �� �����
	cmdList.push_back(VMCmd(cmdJmp, continueAddr.back()));

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeContinueOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ContinueOp\r\n", typeInfo->GetTypeName().c_str());
}
unsigned int NodeContinueOp::GetSize()
{
	return (1+popCnt);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ ��� ��� switch
NodeSwitchExpr::NodeSwitchExpr()
{
	// ������ ���� � ��������
	first = TakeLastNode();
}
NodeSwitchExpr::~NodeSwitchExpr()
{
	casePtr cond = caseCondList.begin(), econd = caseCondList.end();
	casePtr block = caseBlockList.begin(), eblocl = caseBlockList.end();
	for(; cond != econd; cond++, block++)
	{
		delete *cond;
		delete *block;
	}
}

void NodeSwitchExpr::AddCase()
{
	// ������ � �������� ����
	caseBlockList.push_back(TakeLastNode());
	// ������ ������� ��� �����
	caseCondList.push_back(TakeLastNode());
}

void NodeSwitchExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	asmStackType aST = podTypeToStackType[first->GetTypeInfo()->type];
	asmOperType aOT = operTypeForStackType[aST];
	// �������� ������� ����� ����������
	cmdList.push_back(VMCmd(cmdPushVTop));
	// ����� �������� �� �������� ����� �������� ������� ����
	first->Compile();

	// ����� ����� ������
	unsigned int switchEnd = cmdList.size() + 2 + caseCondList.size() * 3;
	for(casePtr s = caseCondList.begin(), e = caseCondList.end(); s != e; s++)
		switchEnd += (*s)->GetSize();
	unsigned int condEnd = switchEnd;
	unsigned int blockNum = 0;
	for(casePtr s = caseBlockList.begin(), e = caseBlockList.end(); s != e; s++, blockNum++)
		switchEnd += (*s)->GetSize() + 1 + (blockNum != caseBlockList.size()-1 ? 1 : 0);

	// �������� ����� ��� ��������� break;
	breakAddr.push_back(switchEnd+1);

	// ����������� ��� ��� ���� case'��
	casePtr cond = caseCondList.begin(), econd = caseCondList.end();
	casePtr block = caseBlockList.begin(), eblocl = caseBlockList.end();
	unsigned int caseAddr = condEnd;
	for(; cond != econd; cond++, block++)
	{
		if(aOT == OTYPE_INT)
			cmdList.push_back(VMCmd(cmdCopyI));
		else
			cmdList.push_back(VMCmd(cmdCopyDorL));

		(*cond)->Compile();
		// ������� �� ���������
		if(aOT == OTYPE_INT)
			cmdList.push_back(VMCmd(cmdEqual));
		else if(aOT == OTYPE_DOUBLE)
			cmdList.push_back(VMCmd(cmdEqualD));
		else
			cmdList.push_back(VMCmd(cmdEqualL));
		// ���� �����, ������� �� ������ ����
		cmdList.push_back(VMCmd(cmdJmpNZType[aOT], caseAddr));
		caseAddr += (*block)->GetSize() + 2;
	}
	// ����� � ������� ����� �������� �� �������� ��������� ������� ����
	cmdList.push_back(VMCmd(cmdPop, stackTypeSize[aST]));

	cmdList.push_back(VMCmd(cmdJmp, switchEnd));
	blockNum = 0;
	for(block = caseBlockList.begin(), eblocl = caseBlockList.end(); block != eblocl; block++, blockNum++)
	{
		// ����� � ������� ����� �������� �� �������� ��������� ������� ����
		cmdList.push_back(VMCmd(cmdPop, stackTypeSize[aST]));
		(*block)->Compile();
		if(blockNum != caseBlockList.size()-1)
		{
			cmdList.push_back(VMCmd(cmdJmp, cmdList.size() + 2));
		}
	}

	// ���������� ������� ����� ��������
	cmdList.push_back(VMCmd(cmdPopVTop));

	breakAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeSwitchExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s SwitchExpression :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	first->LogToStream(fGraph);
	casePtr cond = caseCondList.begin(), econd = caseCondList.end();
	casePtr block = caseBlockList.begin(), eblocl = caseBlockList.end();
	for(; cond != econd; cond++, block++)
	{
		(*cond)->LogToStream(fGraph);
		if(block == --caseBlockList.end())
		{
			GoUp();
			GoDownB();
		}
		(*block)->LogToStream(fGraph);
	}
	GoUp();
}
unsigned int NodeSwitchExpr::GetSize()
{
	unsigned int size = 0;
	size += first->GetSize();
	for(casePtr s = caseCondList.begin(), e = caseCondList.end(); s != e; s++)
		size += (*s)->GetSize();
	unsigned int blockNum = 0;
	for(casePtr s = caseBlockList.begin(), e = caseBlockList.end(); s != e; s++, blockNum++)
		size += (*s)->GetSize() + 1 + (blockNum != caseBlockList.size()-1 ? 1 : 0);
	size += 4;
	size += (unsigned int)caseCondList.size() * 3;
	return size;
}

//////////////////////////////////////////////////////////////////////////
// ����, ���������� ������ ���������.
NodeExpressionList::NodeExpressionList(TypeInfo *returnType)
{
	typeInfo = returnType;
	exprList.push_back(TakeLastNode());
}
NodeExpressionList::~NodeExpressionList()
{
	for(listPtr s = exprList.begin(), e = exprList.end(); s != e; s++)
		delete *s;
}

void NodeExpressionList::AddNode(bool reverse)
{
	exprList.insert(reverse ? exprList.begin() : exprList.end(), TakeLastNode());
}

NodeZeroOP* NodeExpressionList::GetFirstNode()
{
	assert(!exprList.empty());
	return *(exprList.begin());
}

void NodeExpressionList::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	for(listPtr s = exprList.begin(), e = exprList.end(); s != e; s++)
		(*s)->Compile();

	assert((cmdList.size()-startCmdSize) == GetSize());
}
void NodeExpressionList::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeExpressionList :\r\n", typeInfo->GetTypeName().c_str());
	GoDown();
	listPtr s, e;
	for(s = exprList.begin(), e = exprList.end(); s != e; s++)
	{
		if(s == --exprList.end())
		{
			GoUp();
			GoDownB();
		}
		(*s)->LogToStream(fGraph);
	}
	GoUp();
}
unsigned int NodeExpressionList::GetSize()
{
	unsigned int size = 0;
	for(listPtr s = exprList.begin(), e = exprList.end(); s != e; s++)
		size += (*s)->GetSize();
	return size;
}
