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

shared_ptr<NodeZeroOP>	TakeLastNode()
{
	shared_ptr<NodeZeroOP> last = nodeList.back();
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
void DrawLine(ostringstream& ostr)
{
	ostr << preStr;
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

//returns new value type
asmStackType	ConvertToReal(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	(void)op;
	if(st == STYPE_DOUBLE)
		return st;
	cmdList->AddData(cmdITOR);
	cmdList->AddData((unsigned short)(st | DTYPE_DOUBLE));
	return STYPE_DOUBLE;
}
asmStackType	ConvertToInteger(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	(void)op;
	if(st == STYPE_INT || st == STYPE_LONG)
		return st;
	cmdList->AddData(cmdRTOI);
	cmdList->AddData((unsigned short)(st | DTYPE_INT));
	return STYPE_INT;
}

//This function converts a type according to result type of binary operation between types 'first' and 'second'
//For example,  int * double = double, so first operand will be transformed to double
//				double * int = double, no transformations
asmStackType	ConvertFirstForSecond(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
	{
		//getLog() << "Converting from integer to float\r\n";
		cmdList->AddData(cmdITOR);
		cmdList->AddData((unsigned short)(first | dataTypeForStackType[second]));
		return second;
	}
	if(first == STYPE_INT && second == STYPE_LONG)
	{
		cmdList->AddData(cmdITOL);
		return second;
	}
	return first;
}

//This functions transforms first type to second one
void	ConvertFirstToSecond(asmStackType first, asmStackType second)
{
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT || first == STYPE_LONG)
		{
			cmdList->AddData(cmdITOR);
			cmdList->AddData((unsigned short)(first | DTYPE_DOUBLE));
		}
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
		{
			cmdList->AddData(cmdITOL);
		}else if(first == STYPE_DOUBLE){
			cmdList->AddData(cmdRTOI);
			cmdList->AddData((unsigned short)(STYPE_DOUBLE | DTYPE_LONG));
		}
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
		{
			cmdList->AddData(cmdRTOI);
			cmdList->AddData((unsigned short)(STYPE_DOUBLE | DTYPE_INT));
		}else if(first == STYPE_LONG){
			cmdList->AddData(cmdLTOI);
		}
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

unsigned int	ConvertToRealSize(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	(void)op;
	if(st == STYPE_DOUBLE)
		return 0;
	return sizeof(CmdID) + sizeof(unsigned short);
}
unsigned int	ConvertToIntegerSize(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	(void)op;
	if(st == STYPE_INT || st == STYPE_LONG)
		return 0;
	return sizeof(CmdID) + sizeof(unsigned short);
}

std::pair<unsigned int, asmStackType>	ConvertFirstForSecondSize(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
		return std::pair<unsigned int, asmStackType>(sizeof(CmdID) + sizeof(unsigned short), STYPE_DOUBLE);
	if(first == STYPE_INT && second == STYPE_LONG)
		return std::pair<unsigned int, asmStackType>(sizeof(CmdID), STYPE_LONG);
	return std::pair<unsigned int, asmStackType>(0, first);
}

unsigned int	ConvertFirstToSecondSize(asmStackType first, asmStackType second)
{
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT || first == STYPE_LONG)
			return sizeof(CmdID) + sizeof(unsigned short);
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			return sizeof(CmdID);
		else if(first == STYPE_DOUBLE)
			return sizeof(CmdID) + sizeof(unsigned short);
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			return sizeof(CmdID) + sizeof(unsigned short);
		else if(first == STYPE_LONG)
			return sizeof(CmdID);
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
void NodeZeroOP::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "ZeroOp\r\n";
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
	strBegin = start;
	strEnd = end;
}
//////////////////////////////////////////////////////////////////////////
// ����, ������� ���� �������� ����
NodeOneOP::NodeOneOP()
{
}
NodeOneOP::~NodeOneOP()
{
}

void NodeOneOP::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();

	first->Compile();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeOneOP::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "OneOP :\r\n";
	GoDown();
	first->LogToStream(ostr);
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
}
NodeTwoOP::~NodeTwoOP()
{
}

void NodeTwoOP::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();

	NodeOneOP::Compile();
	second->Compile();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeTwoOP::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "TwoOp :\r\n"; GoDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
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
}
NodeThreeOP::~NodeThreeOP()
{
}

void NodeThreeOP::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();

	NodeTwoOP::Compile();
	third->Compile();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeThreeOP::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "ThreeOp :\r\n";
	GoDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	third->LogToStream(ostr);
	GoUp();
}
unsigned int NodeThreeOP::GetSize()
{
	return NodeTwoOP::GetSize() + third->GetSize();
}

//////////////////////////////////////////////////////////////////////////
// ��������������� ������� ��� NodeNumber<T>
void NodeNumberPushCommand(unsigned short cmdFlag, char* data, unsigned int dataSize)
{
	cmdList->AddData(cmdPushImmt);
	cmdList->AddData(cmdFlag);
	cmdList->AddData(data, dataSize);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ��� ��������� ���� ��������� ��������
	first->Compile();
	if(first->GetTypeInfo() != typeVoid)
	{
		// ������� ��� � ������� �����
		cmdList->AddData(cmdPop);
		if(first->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX)
			cmdList->AddData(first->GetTypeInfo()->size);
		else
			cmdList->AddData(stackTypeSize[podTypeToStackType[first->GetTypeInfo()->type]]);
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodePopOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "PopOp :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
unsigned int NodePopOp::GetSize()
{
	unsigned int size = NodeOneOP::GetSize();
	if(first->GetTypeInfo() != typeVoid)
		size += sizeof(CmdID) + sizeof(unsigned int);

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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	// ��� ��������� ���� ��������� ��������
	first->Compile();
	// �������� �������
	cmdList->AddData(cmdID);
	cmdList->AddData((unsigned char)(aOT));

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeUnaryOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "UnaryOp :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
unsigned int NodeUnaryOp::GetSize()
{
	return NodeOneOP::GetSize() + sizeof(CmdID) + sizeof(unsigned char);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ����� ��������, ������� ����� ����������
	first->Compile();
	// ����������� ��� � ��� ����������� �������� �������
	if(typeInfo)
		ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]);

	// ������ �� ������� ��� ���������
	TypeInfo *retType = typeInfo ? typeInfo : first->GetTypeInfo();
	asmOperType operType = operTypeForStackType[podTypeToStackType[retType->type]];
	cmdList->AddData(cmdReturn);
	cmdList->AddData((unsigned short)(retType->type == TypeInfo::TYPE_COMPLEX ? retType->size : (bitRetSimple | operType)));
	cmdList->AddData((unsigned short)(popCnt));

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeReturnOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	if(typeInfo)
		ostr << *typeInfo << "ReturnOp :\r\n";
	else
		ostr << *first->GetTypeInfo() << "ReturnOp :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
unsigned int NodeReturnOp::GetSize()
{
	return NodeOneOP::GetSize() + sizeof(CmdID) + 2 * sizeof(unsigned short) + (typeInfo ? ConvertFirstToSecondSize(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]) : 0);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	NodeOneOP::Compile();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeExpression::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "Expression :\r\n";
	GoDownB();
	first->LogToStream(ostr);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeVarDef::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "VarDef '" << name << "'\r\n";
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	// �������� �������� ������� ����� ����������
	cmdList->AddData(cmdPushVTop);
	if(shift)
	{
		cmdList->AddData(cmdPushV);
		cmdList->AddData(shift);
	}
	// �������� ���������� ����� (�� �� ��� first->Compile())
	first->Compile();
	// ���������� �������� ������� ����� ����������
	if(popAfter)
		cmdList->AddData(cmdPopVTop);

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeBlock::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "Block (" << shift << ") :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
unsigned int NodeBlock::GetSize()
{
	return first->GetSize() + (popAfter ? 2 : 1) * sizeof(CmdID) + (shift ? sizeof(CmdID) + 4 : 0);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	funcInfo->address = cmdList->GetCurrPos();
	// ����������� ��� �������
	first->Compile();

	cmdList->AddData(cmdReturn);
	if(funcInfo->retType == typeVoid)
	{
		// ���� ������� �� ���������� ��������, �� ��� ������ ret
		cmdList->AddData((unsigned short)(0));	// ���������� �������� �������� 0 ����
		cmdList->AddData((unsigned short)(1));
	}else{
		// ��������� ��������� � �������
		cmdList->AddData((unsigned short)(bitRetError));
		cmdList->AddData((unsigned short)(1));
	}

	funcInfo->codeSize = cmdList->GetCurrPos() - funcInfo->address;

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeFuncDef::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "FuncDef " << funcInfo->name << (disabled ? " disabled" : "") << "\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
unsigned int NodeFuncDef::GetSize()
{
	if(disabled)
		return 0;
	return first->GetSize() + sizeof(CmdID) + 2*sizeof(unsigned short);
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
}

void NodeFuncCall::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();

	// ���� ������� ���������, ����� �� ��������
	unsigned int currParam = 0;
	
	if(funcInfo && funcInfo->address == -1 && funcInfo->funcPtr != NULL)
	{
		std::list<shared_ptr<NodeZeroOP> >::iterator s, e;
		s = paramList.begin();
		e = paramList.end();
		for(; s != e; s++)
		{
			// ��������� �������� ���������
			(*s)->Compile();
			// ����������� ��� � ��� �������� ��������� �������
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[paramList.size()-currParam-1]->type]);
			if(funcType->paramType[paramList.size()-currParam-1] == typeFloat)
				cmdList->AddData(cmdDTOF);
			currParam++;
		}
	}else{
		std::list<shared_ptr<NodeZeroOP> >::reverse_iterator s, e;
		s = paramList.rbegin();
		e = paramList.rend();
		for(; s != e; s++)
		{
			// ��������� �������� ���������
			(*s)->Compile();
			// ����������� ��� � ��� �������� ��������� �������
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[currParam]->type]);
			currParam++;
		}
	}
	if(funcInfo && funcInfo->address == -1)		// ���� ������� ����������
	{
		// ������� �� �����
		cmdList->AddData(cmdCallStd);
		unsigned int ID = GetFuncIndexByPtr(funcInfo);
		cmdList->AddData(ID);
	}else{					// ���� ������� ���������� �������������
		// �������� � ��������� ��������� ����� ���, �����
		unsigned int addr = 0;
		for(int i = int(funcType->paramType.size())-1; i >= 0; i--)
		{
			asmStackType newST = podTypeToStackType[funcType->paramType[i]->type];
			asmDataType newDT = podTypeToDataType[funcType->paramType[i]->type];
			if(CodeInfo::activeExecutor == EXEC_VM)
			{
				cmdList->AddData(cmdMovRTaP);
				cmdList->AddData((unsigned short)(newDT));
				// ����� ������ �������
				cmdList->AddData(addr);
				addr += funcType->paramType[i]->size;
				if(newST == STYPE_COMPLEX_TYPE)
					cmdList->AddData(funcType->paramType[i]->size);
			}else{
				cmdList->AddData(cmdMov);
				cmdList->AddData((unsigned short)(newST | newDT | bitAddrRelTop));
				// ����� ������ �������
				cmdList->AddData(addr);
				addr += funcType->paramType[i]->size;
				if(newST == STYPE_COMPLEX_TYPE)
					cmdList->AddData(funcType->paramType[i]->size);

				cmdList->AddData(cmdPop);
				if(newST == STYPE_COMPLEX_TYPE)
					cmdList->AddData(funcType->paramType[i]->size);
				else
					cmdList->AddData(stackTypeSize[newST]);
			}
		}

		if(!funcInfo || second)
		{
			if(second)
				second->Compile();
			else
				first->Compile();
			if(CodeInfo::activeExecutor == EXEC_VM)
			{
				cmdList->AddData(cmdMovRTaP);
				cmdList->AddData((unsigned short)(DTYPE_INT));
				cmdList->AddData(addr);
			}else{
				cmdList->AddData(cmdMov);
				cmdList->AddData((unsigned short)(STYPE_INT | DTYPE_INT | bitAddrRelTop));
				cmdList->AddData(addr);

				cmdList->AddData(cmdPop);
				cmdList->AddData(4);
			}
		}

		// ������� �� ������
		cmdList->AddData(cmdCall);
		unsigned int ID = GetFuncIndexByPtr(funcInfo);
		cmdList->AddData(funcInfo ? ID : -1);
		cmdList->AddData((unsigned short)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[podTypeToStackType[typeInfo->type]])));
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeFuncCall::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "FuncCall '" << (funcInfo ? funcInfo->name : "$ptr") << "' " << paramList.size() << ":\r\n";
	GoDown();
	if(first)
		first->LogToStream(ostr);
	if(second)
		second->LogToStream(ostr);
	for(paramPtr s = paramList.rbegin(), e = paramList.rend(); s != e; s++)
	{
		if(s == --paramList.rend())
		{
			GoUp();
			GoDownB();
		}
		(*s)->LogToStream(ostr);
	}
	GoUp();
}
unsigned int NodeFuncCall::GetSize()
{
	unsigned int size = 0;
	if(!funcInfo || second)
	{
		if(second)
			size += second->GetSize();
		else
			size += first->GetSize();
		size += sizeof(CmdID) + sizeof(unsigned int) + sizeof(unsigned short);
		if(CodeInfo::activeExecutor == EXEC_X86)
			size += sizeof(CmdID) + sizeof(unsigned int);
	}

	unsigned int currParam = 0;
	for(paramPtr s = paramList.rbegin(), e = paramList.rend(); s != e; s++)
	{
		size += (*s)->GetSize();
		size += ConvertFirstToSecondSize(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[currParam]->type]);
		if(funcInfo && funcInfo->address == -1 && funcInfo->funcPtr != NULL && funcType->paramType[paramList.size()-currParam-1] == typeFloat)
			size += sizeof(CmdID);
		currParam++;
	}
	
	if(funcInfo && funcInfo->address == -1)
	{
		size += sizeof(CmdID) + sizeof(funcInfo);
	}else{
		size += sizeof(CmdID) + sizeof(unsigned int) + sizeof(unsigned short) + (unsigned int)(funcType->paramType.size()) * (sizeof(CmdID)+2+4);
		if(CodeInfo::activeExecutor == EXEC_X86)
			size += (unsigned int)(funcType->paramType.size()) * (sizeof(CmdID)+4);
		for(int i = int(funcType->paramType.size())-1; i >= 0; i--)
		{
			if(funcType->paramType[i]->type == TypeInfo::TYPE_COMPLEX)
				size += sizeof(unsigned int);
		}
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
	unsigned int startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	if(absAddress)
	{
		cmdList->AddData(cmdPushImmt);
		cmdList->AddData((unsigned short)(STYPE_INT | DTYPE_INT));
		// ����� ������ �������
		cmdList->AddData(varAddress);
	}else{
		cmdList->AddData(cmdGetAddr);
		// ������������� ����� ����������
		cmdList->AddData(varAddress);
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}

void NodeGetAddress::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *GetReferenceType(typeInfo) << "GetAddress ";
	if(varInfo)
		ostr << *varInfo;
	else
		ostr << "$$$";
	ostr << " (" << (int)varAddress << (absAddress ? " absolute" : " relative") << ")\r\n";
}

unsigned int NodeGetAddress::GetSize()
{
	if(absAddress)
		return sizeof(CmdID) + sizeof(unsigned short) + sizeof(unsigned int);
	// else
	return sizeof(CmdID) + sizeof(unsigned int);
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

	bytesToPush = pushVar;

	if(swapNodes)
		second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->GetTypeInfo()->refLevel != 0);

	if(!swapNodes)
		second = TakeLastNode();

	// ���� ��� ������ ����������� ���������� � ������� ������������� ������� ���
	arrSetAll = (bytesToPush && typeInfo->arrLevel != 0 && second->GetTypeInfo()->arrLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && second->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX);

	if(second->GetTypeInfo() == typeVoid)
		throw std::string("ERROR: cannot convert from void to " + typeInfo->GetTypeName());
	if(typeInfo == typeVoid)
		throw std::string("ERROR: cannot convert from " + second->GetTypeInfo()->GetTypeName() + " to void");

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
				throw std::string("ERROR: Cannot convert '" + second->GetTypeInfo()->GetTypeName() + "' to '" + typeInfo->GetTypeName() + "'");
		}
	}

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->GetNodeType() == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first.get())->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first.get())->varAddress;
		knownAddress = true;
	}
	if(first->GetNodeType() == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first.get())->memberShift;
		first = static_cast<NodeShiftAddress*>(first.get())->first;
	}
	if(first->GetNodeType() == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first.get())->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first.get())->shiftValue;
		first = static_cast<NodeArrayIndex*>(first.get())->first;
	}
}

NodeVariableSet::~NodeVariableSet()
{
}


void NodeVariableSet::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	asmStackType asmST = podTypeToStackType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)];
	asmDataType asmDT = podTypeToDataType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)];

	second->Compile();
	ConvertFirstToSecond(podTypeToStackType[second->GetTypeInfo()->type], asmST);

	if(!knownAddress)
		first->Compile();
	if(arrSetAll)
	{
		assert(knownAddress);
		cmdList->AddData(cmdSetRange);
		cmdList->AddData((unsigned short)(podTypeToDataType[typeInfo->subType->type]));
		cmdList->AddData(addrShift);
		cmdList->AddData(typeInfo->size / typeInfo->subType->size);
	}else{
		cmdList->AddData(cmdMov);
		cmdList->AddData((unsigned short)(asmST | asmDT | (absAddress ? bitAddrAbs : bitAddrRel) | (knownAddress ? 0 : bitShiftStk)));
		// ����� ������ �������
		cmdList->AddData(addrShift);
		if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
			cmdList->AddData(typeInfo->size);
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}

void NodeVariableSet::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "VariableSet " << (arrSetAll ? "set all elements" : "") << "\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
	GoDownB();
	second->LogToStream(ostr);
	GoUp();
}

unsigned int NodeVariableSet::GetSize()
{
	unsigned int size = second->GetSize();
	if(!knownAddress)
		size += first->GetSize();
	size += ConvertFirstToSecondSize(podTypeToStackType[second->GetTypeInfo()->type], podTypeToStackType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)]);
	if(arrSetAll)
		size += sizeof(CmdID) + sizeof(unsigned short) + 2*sizeof(unsigned int);
	else
		size += sizeof(CmdID) + sizeof(unsigned short) + sizeof(unsigned int) + (typeInfo->type == TypeInfo::TYPE_COMPLEX ? sizeof(unsigned int) : 0);
	return size;
}

TypeInfo* NodeVariableSet::GetTypeInfo()
{
	return (arrSetAll ? typeInfo->subType : typeInfo);
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
		NodeZeroOP* zOP = second.get();
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
			throw CompilerError("NodeArrayIndex() ERROR: unknown type " + aType->name, lastKnownStartPos);
		}
		knownShift = true;
	}
}

NodeArrayIndex::~NodeArrayIndex()
{
}

void NodeArrayIndex::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	asmOperType oAsmType = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// ������ ��������� �� ������ �������
	first->Compile();

	if(knownShift)
	{
		cmdList->AddData(cmdPushImmt);
		cmdList->AddData((unsigned short)(STYPE_INT | DTYPE_INT));
		// ����� ������ �������
		cmdList->AddData(shiftValue);
	}else{
		// �������� ������
		second->Compile();
		// �������� ��� � ����� �����
		cmdList->AddData(cmdCTI);
		// ��������� ��� ��������
		cmdList->AddData((unsigned char)(oAsmType));
		// ������� �� ������ �������� (����� ������ ���� � ������)
		cmdList->AddData(typeParent->subType->size);
	}
	// ������ � �������, ������� ��� �� �������
	cmdList->AddData(cmdAdd);
	// ��������� ��� ��������
	cmdList->AddData((unsigned char)(OTYPE_INT));

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}

void NodeArrayIndex::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "ArrayIndex " << *typeParent;
	ostr << " known: " << knownShift << " shiftval: " << shiftValue;
	ostr << "\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
	GoDownB();
	second->LogToStream(ostr);
	GoUp();
}

unsigned int NodeArrayIndex::GetSize()
{
	if(knownShift)
		return first->GetSize() + 2*sizeof(CmdID) + sizeof(unsigned short) + sizeof(unsigned char) + sizeof(unsigned int);
	// else
	return first->GetSize() + second->GetSize() + 2*sizeof(CmdID) + 2*sizeof(unsigned char) + sizeof(unsigned int);
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
		absAddress = static_cast<NodeGetAddress*>(first.get())->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first.get())->varAddress;
		knownAddress = true;
	}
	if(first->GetNodeType() == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first.get())->memberShift;
		first = static_cast<NodeShiftAddress*>(first.get())->first;
	}
	if(first->GetNodeType() == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first.get())->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first.get())->shiftValue;
		first = static_cast<NodeArrayIndex*>(first.get())->first;
	}
}

NodeDereference::~NodeDereference()
{
}


void NodeDereference::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	asmStackType asmST = podTypeToStackType[typeInfo->type];
	asmDataType asmDT = podTypeToDataType[typeInfo->type];
	
	if(!knownAddress)
		first->Compile();

	cmdList->AddData(cmdPush);
	cmdList->AddData((unsigned short)(asmST | asmDT | (absAddress ? bitAddrAbs : bitAddrRel) | (knownAddress ? 0 : bitShiftStk)));
	// ����� ������ �������
	cmdList->AddData(addrShift);
	if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
		cmdList->AddData(typeInfo->size);

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}

void NodeDereference::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "Dereference " << "\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}

unsigned int NodeDereference::GetSize()
{
	return (!knownAddress ? first->GetSize() : 0) + sizeof(CmdID) + sizeof(unsigned short) + sizeof(unsigned int) + (typeInfo->type == TypeInfo::TYPE_COMPLEX ? sizeof(unsigned int) : 0);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	first->Compile();

	if(memberShift)
	{
		cmdList->AddData(cmdPushImmt);
		cmdList->AddData((unsigned short)(STYPE_INT | DTYPE_INT));
		// ����� �� ����� ����
		cmdList->AddData(memberShift);

		// ������ � �������, ������� ��� �� �������
		cmdList->AddData(cmdAdd);
		cmdList->AddData((unsigned char)(OTYPE_INT));
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}

void NodeShiftAddress::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "ShiftAddress" << "\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}

unsigned int NodeShiftAddress::GetSize()
{
	unsigned int retSize = first->GetSize();
	if(memberShift)
		retSize += 2*sizeof(CmdID) + sizeof(unsigned short) + sizeof(unsigned int) + sizeof(unsigned char);
	return retSize;
}

TypeInfo* NodeShiftAddress::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� �������������� ��� ��������������
NodePreOrPostOp::NodePreOrPostOp(TypeInfo* resType, CmdID cmd, bool preOp)
{
	assert(resType);
	typeInfo = resType;

	first = TakeLastNode();
	assert(first->GetTypeInfo()->refLevel != 0);

	assert(cmd == cmdIncAt || cmd == cmdDecAt);
	cmdID = cmd;

	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0)
		throw std::string("ERROR: ") + (cmdID == cmdIncAt ? "Increment" : "Decrement") + std::string(" is not supported on '") + typeInfo->GetTypeName() + "'";

	prefixOp = preOp;

	optimised = false;

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->GetNodeType() == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first.get())->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first.get())->varAddress;
		knownAddress = true;
	}
	if(first->GetNodeType() == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first.get())->memberShift;
		first = static_cast<NodeShiftAddress*>(first.get())->first;
	}
	if(first->GetNodeType() == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first.get())->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first.get())->shiftValue;
		first = static_cast<NodeArrayIndex*>(first.get())->first;
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
	unsigned int startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	asmDataType asmDT = podTypeToDataType[typeInfo->type];
	
	if(!knownAddress)
		first->Compile();

	// ������ �������� ���������� ����� �� ������
	cmdList->AddData(cmdID);
	cmdList->AddData((unsigned short)(asmDT | (optimised ? 0 : (prefixOp ? bitPushAfter : bitPushBefore)) | (absAddress ? bitAddrAbs : bitAddrRel) | (knownAddress ? 0 : bitShiftStk)));
	cmdList->AddData(addrShift);

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}

void NodePreOrPostOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "PreOrPostOp " << (prefixOp ? "prefix" : "postfix") << "\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}

unsigned int NodePreOrPostOp::GetSize()
{
	return (!knownAddress ? first->GetSize() : 0) + sizeof(CmdID) + sizeof(unsigned short) + sizeof(unsigned int);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	cmdList->AddData(cmdFuncAddr);
    unsigned int ID = GetFuncIndexByPtr(funcInfo);
	cmdList->AddData(ID);

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		cmdList->AddData(cmdPushImmt);
		cmdList->AddData((unsigned short)(STYPE_INT | DTYPE_INT));
		cmdList->AddData(0);
	}else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL){
		first->Compile();
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}

void NodeFunctionAddress::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "FunctionAddress " << funcInfo->name << (funcInfo->funcPtr ? " external" : "") << "\r\n";
	if(first)
	{
		GoDownB();
		first->LogToStream(ostr);
		GoUp();
	}
}

unsigned int NodeFunctionAddress::GetSize()
{
	unsigned int size = sizeof(CmdID) + sizeof(FunctionInfo*);
	if(funcInfo->type == FunctionInfo::NORMAL)
		size += sizeof(CmdID) + sizeof(unsigned short) + sizeof(unsigned int);
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
		if(first->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX || second->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX)
			throw std::string("ERROR: Operation " + std::string(binCommandToText[cmdID - cmdAdd]) + " is not supported on '" + first->GetTypeInfo()->GetTypeName() + "' and '" + second->GetTypeInfo()->GetTypeName() + "'");
	if(first->GetTypeInfo() == typeVoid)
		throw std::string("ERROR: first operator returns void");
	if(second->GetTypeInfo() == typeVoid)
		throw std::string("ERROR: second operator returns void");

	// ����� �������������� ���, ����� ���������� ��������
	typeInfo = ChooseBinaryOpResultType(first->GetTypeInfo(), second->GetTypeInfo());
}
NodeTwoAndCmdOp::~NodeTwoAndCmdOp()
{
}

void NodeTwoAndCmdOp::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();

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
	cmdList->AddData(cmdID);
	cmdList->AddData((unsigned char)(operTypeForStackType[fST]));

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeTwoAndCmdOp::LogToStream(ostringstream& ostr)
{
	if((cmdID < cmdAdd) || (cmdID > cmdLogXor))
		throw std::string("ERROR: TwoAndCmd error");
	DrawLine(ostr);
	ostr << *typeInfo << "TwoAndCmd<" << binCommandToText[cmdID-cmdAdd] << "> :\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
	GoDownB();
	second->LogToStream(ostr);
	GoUp();
}
unsigned int NodeTwoAndCmdOp::GetSize()
{
	asmStackType fST = podTypeToStackType[first->GetTypeInfo()->type], sST = podTypeToStackType[second->GetTypeInfo()->type];
	unsigned int resSize = 0;
	resSize += ConvertFirstForSecondSize(fST, sST).first;
	fST = ConvertFirstForSecondSize(fST, sST).second;
	resSize += ConvertFirstForSecondSize(sST, fST).first;
	return NodeTwoOP::GetSize() + sizeof(CmdID) + 1 + resSize;
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ��������� �������� ���������: if(first) second; else third;
	// ������ �������: first ? second : third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// �������� �������
	first->Compile();

	// ���� false, ������� � ���� else ��� ������ �� ���������, ���� ������ ����� �� �������
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((unsigned char)(aOT));
	cmdList->AddData(4 + cmdList->GetCurrPos() + second->GetSize() + (third ? 6 : 0));

	// �������� ���� ��� ��������� ����������� ������� (true)
	second->Compile();
	// ���� ���� ���� else, �������� ���
	if(third)
	{
		// ������ �������� ����� �� ��������� ����� ��� �����, ����� �� ��������� ��� �����
		cmdList->AddData(cmdJmp);
		cmdList->AddData(4 + cmdList->GetCurrPos() + third->GetSize());

		// �������� ���� else (false)
		third->Compile();
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeIfElseExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "IfExpression :\r\n";
	GoDown();
	first->LogToStream(ostr);
	if(!third)
	{
		GoUp();
		GoDownB();
	}
	second->LogToStream(ostr);
	if(third)
	{
		GoUp();
		GoDownB();
		third->LogToStream(ostr);
	}
	GoUp();
}
unsigned int NodeIfElseExpr::GetSize()
{
	unsigned int size = first->GetSize() + second->GetSize() + sizeof(CmdID) + sizeof(unsigned int) + sizeof(unsigned char);
	if(third)
		size += third->GetSize() + sizeof(CmdID) + sizeof(unsigned int);
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
}

void NodeForExpr::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ��������� �������� ���������: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// �������� �������������
	first->Compile();
	unsigned int posTestExpr = cmdList->GetCurrPos();

	// ����� ��������� �������
	second->Compile();

	// ���� �����, ������ �� �����
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((unsigned char)(aOT));

	// �������� ����� ��� ������ �� ����� ���������� break;
	breakAddr.push_back(cmdList->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);
	cmdList->AddData(breakAddr.back());

	// �������� ����� ��� �������� � ��������� �������� ���������� continue;
	continueAddr.push_back(cmdList->GetCurrPos()+fourth->GetSize());

	// �������� ���������� �����
	fourth->Compile();
	// �������� ��������, ���������� ����� ������ ��������
	third->Compile();
	// ������� �� �������� �������
	cmdList->AddData(cmdJmp);
	cmdList->AddData(posTestExpr);

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeForExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "ForExpression :\r\n";
	GoDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	third->LogToStream(ostr);
	GoUp();
	GoDownB(); 
	fourth->LogToStream(ostr);
	GoUp();
}
unsigned int NodeForExpr::GetSize()
{
	return NodeThreeOP::GetSize() + fourth->GetSize() + 2*sizeof(CmdID) + 2*sizeof(unsigned int) + sizeof(unsigned char);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	// ��������� �������� ���������: while(first) second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	unsigned int posStart = cmdList->GetCurrPos();
	// �������� �������
	first->Compile();
	// ���� ��� �����, ������ �� �����
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((unsigned char)(aOT));

	// �������� ����� ��� ������ �� ����� ���������� break;
	breakAddr.push_back(cmdList->GetCurrPos()+4+second->GetSize()+2+4);
	cmdList->AddData(breakAddr.back());

	// �������� ����� ��� �������� � ��������� �������� ���������� continue;
	continueAddr.push_back(cmdList->GetCurrPos()+second->GetSize());

	// �������� ���������� �����
	second->Compile();
	// ������� �� �������� �������
	cmdList->AddData(cmdJmp);
	cmdList->AddData(posStart);

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeWhileExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "WhileExpression :\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
	GoDownB(); 
	second->LogToStream(ostr);
	GoUp();
}
unsigned int NodeWhileExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + 2*sizeof(CmdID) + 2*sizeof(unsigned int) + sizeof(unsigned char);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	// ��������� �������� ���������: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	unsigned int posStart = cmdList->GetCurrPos();
	// �������� ����� ��� ������ �� ����� ���������� break;
	breakAddr.push_back(cmdList->GetCurrPos()+first->GetSize()+second->GetSize()+2+4);

	// �������� ����� ��� �������� � ��������� �������� ���������� continue;
	continueAddr.push_back(cmdList->GetCurrPos()+first->GetSize());

	// �������� ���������� �����
	first->Compile();
	// �������� �������
	second->Compile();
	// ���� ������� �����, ������� � ���������� ��������� �������� �����
	cmdList->AddData(cmdJmpNZ);
	cmdList->AddData((unsigned char)(aOT));
	cmdList->AddData(posStart);

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeDoWhileExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "DoWhileExpression :\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
	GoDownB();
	second->LogToStream(ostr);
	GoUp();
}
unsigned int NodeDoWhileExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + sizeof(CmdID) + sizeof(unsigned int) + sizeof(unsigned char);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	// ����� �������� �� ����� ������ ����� ����������
	for(unsigned int i = 0; i < popCnt; i++)
		cmdList->AddData(cmdPopVTop);
	// ������ �� �����
	cmdList->AddData(cmdJmp);
	cmdList->AddData(breakAddr.back());

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeBreakOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "BreakExpression\r\n";
}
unsigned int NodeBreakOp::GetSize()
{
	return (1+popCnt)*sizeof(CmdID) + sizeof(unsigned int);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	// ����� �������� �� ����� ������ ����� ����������
	for(unsigned int i = 0; i < popCnt; i++)
		cmdList->AddData(cmdPopVTop);

	// ������ �� �����
	cmdList->AddData(cmdJmp);
	cmdList->AddData(continueAddr.back());

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeContinueOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "ContinueOp\r\n";
}
unsigned int NodeContinueOp::GetSize()
{
	return (1+popCnt)*sizeof(CmdID) + sizeof(unsigned int);
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
	unsigned int startCmdSize = cmdList->GetCurrPos();

	asmStackType aST = podTypeToStackType[first->GetTypeInfo()->type];
	asmOperType aOT = operTypeForStackType[aST];
	// �������� ������� ����� ����������
	cmdList->AddData(cmdPushVTop);
	// ����� �������� �� �������� ����� �������� ������� ����
	first->Compile();

	// ����� ����� ������
	unsigned int switchEnd = cmdList->GetCurrPos() + 2*sizeof(CmdID) + sizeof(unsigned int) + sizeof(unsigned int) + caseCondList.size() * (3*sizeof(CmdID) + 3 + sizeof(unsigned int));
	for(casePtr s = caseCondList.begin(), e = caseCondList.end(); s != e; s++)
		switchEnd += (*s)->GetSize();
	unsigned int condEnd = switchEnd;
	unsigned int blockNum = 0;
	for(casePtr s = caseBlockList.begin(), e = caseBlockList.end(); s != e; s++, blockNum++)
		switchEnd += (*s)->GetSize() + sizeof(CmdID) + sizeof(unsigned int) + (blockNum != caseBlockList.size()-1 ? sizeof(CmdID) + sizeof(unsigned int) : 0);

	// �������� ����� ��� ��������� break;
	breakAddr.push_back(switchEnd+2);

	// ����������� ��� ��� ���� case'��
	casePtr cond = caseCondList.begin(), econd = caseCondList.end();
	casePtr block = caseBlockList.begin(), eblocl = caseBlockList.end();
	unsigned int caseAddr = condEnd;
	for(; cond != econd; cond++, block++)
	{
		cmdList->AddData(cmdCopy);
		cmdList->AddData((unsigned char)(aOT));

		(*cond)->Compile();
		// ������� �� ���������
		cmdList->AddData(cmdEqual);
		cmdList->AddData((unsigned char)(aOT));
		// ���� �����, ������� �� ������ ����
		cmdList->AddData(cmdJmpNZ);
		cmdList->AddData((unsigned char)(aOT));
		cmdList->AddData(caseAddr);
		caseAddr += (*block)->GetSize() + 2*sizeof(CmdID) + sizeof(unsigned int) + sizeof(unsigned int);
	}
	// ����� � ������� ����� �������� �� �������� ��������� ������� ����
	cmdList->AddData(cmdPop);
	cmdList->AddData(stackTypeSize[aST]);

	cmdList->AddData(cmdJmp);
	cmdList->AddData(switchEnd);
	blockNum = 0;
	for(block = caseBlockList.begin(), eblocl = caseBlockList.end(); block != eblocl; block++, blockNum++)
	{
		// ����� � ������� ����� �������� �� �������� ��������� ������� ����
		cmdList->AddData(cmdPop);
		cmdList->AddData(stackTypeSize[aST]);
		(*block)->Compile();
		if(blockNum != caseBlockList.size()-1)
		{
			cmdList->AddData(cmdJmp);
			cmdList->AddData(cmdList->GetCurrPos() + sizeof(unsigned int) + sizeof(CmdID) + sizeof(unsigned int));
		}
	}

	// ���������� ������� ����� ��������
	cmdList->AddData(cmdPopVTop);

	breakAddr.pop_back();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeSwitchExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "SwitchExpression :\r\n";
	GoDown();
	first->LogToStream(ostr);
	casePtr cond = caseCondList.begin(), econd = caseCondList.end();
	casePtr block = caseBlockList.begin(), eblocl = caseBlockList.end();
	for(; cond != econd; cond++, block++)
	{
		(*cond)->LogToStream(ostr);
		if(block == --caseBlockList.end())
		{
			GoUp();
			GoDownB();
		}
		(*block)->LogToStream(ostr);
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
		size += (*s)->GetSize() + sizeof(CmdID) + sizeof(unsigned int) + (blockNum != caseBlockList.size()-1 ? sizeof(CmdID) + sizeof(unsigned int) : 0);
	size += 4*sizeof(CmdID) + sizeof(unsigned int) + sizeof(unsigned int);
	size += (unsigned int)caseCondList.size() * (3 * sizeof(CmdID) + 3 + sizeof(unsigned int));
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
}

void NodeExpressionList::AddNode(bool reverse)
{
	exprList.insert(reverse ? exprList.begin() : exprList.end(), TakeLastNode());
}

shared_ptr<NodeZeroOP> NodeExpressionList::GetFirstNode()
{
	if(exprList.empty())
		throw std::string("NodeExpressionList::GetLastNode() List is empty");
	return *(exprList.begin());
}

void NodeExpressionList::Compile()
{
	unsigned int startCmdSize = cmdList->GetCurrPos();

	for(listPtr s = exprList.begin(), e = exprList.end(); s != e; s++)
		(*s)->Compile();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeExpressionList::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "NodeExpressionList :\r\n";
	GoDown();
	listPtr s, e;
	for(s = exprList.begin(), e = exprList.end(); s != e; s++)
	{
		if(s == --exprList.end())
		{
			GoUp();
			GoDownB();
		}
		(*s)->LogToStream(ostr);
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
