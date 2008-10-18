#include "stdafx.h"
#include "ParseFunc.h"

#include "CodeInfo.h"
using namespace CodeInfo;

shared_ptr<NodeZeroOP>	TakeLastNode()
{
	shared_ptr<NodeZeroOP> last = nodeList.back();
	nodeList.pop_back();
	return last;
}

std::vector<UINT>			indTemp;

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
	if(st == STYPE_DOUBLE)
		return st;
	cmdList->AddData(cmdITOR);
	cmdList->AddData((USHORT)(st | DTYPE_DOUBLE));
	return STYPE_DOUBLE;
}
asmStackType	ConvertToInteger(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	if(st == STYPE_INT || st == STYPE_LONG)
		return st;
	cmdList->AddData(cmdRTOI);
	cmdList->AddData((USHORT)(st | DTYPE_INT));
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
		cmdList->AddData((USHORT)(first | dataTypeForStackType[second]));
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
			cmdList->AddData((USHORT)(first | DTYPE_DOUBLE));
		}
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
		{
			cmdList->AddData(cmdITOL);
		}else if(first == STYPE_DOUBLE){
			cmdList->AddData(cmdRTOI);
			cmdList->AddData((USHORT)(STYPE_DOUBLE | DTYPE_LONG));
		}
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
		{
			cmdList->AddData(cmdRTOI);
			cmdList->AddData((USHORT)(STYPE_DOUBLE | DTYPE_INT));
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
	__asm int 3;
	return NULL;
}

UINT	ConvertToRealSize(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	if(st == STYPE_DOUBLE)
		return 0;
	return sizeof(CmdID) + sizeof(USHORT);
}
UINT	ConvertToIntegerSize(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	if(st == STYPE_INT || st == STYPE_LONG)
		return 0;
	return sizeof(CmdID) + sizeof(USHORT);
}

std::pair<UINT, asmStackType>	ConvertFirstForSecondSize(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
		return std::pair<UINT, asmStackType>(sizeof(CmdID) + sizeof(USHORT), STYPE_DOUBLE);
	if(first == STYPE_INT && second == STYPE_LONG)
		return std::pair<UINT, asmStackType>(sizeof(CmdID), STYPE_LONG);
	return std::pair<UINT, asmStackType>(0, first);
}

UINT	ConvertFirstToSecondSize(asmStackType first, asmStackType second)
{
	if(second == STYPE_DOUBLE)
	{
		if(first == STYPE_INT || first == STYPE_LONG)
			return sizeof(CmdID) + sizeof(USHORT);
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
			return sizeof(CmdID);
		else if(first == STYPE_DOUBLE)
			return sizeof(CmdID) + sizeof(USHORT);
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
			return sizeof(CmdID) + sizeof(USHORT);
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
UINT NodeZeroOP::GetSize()
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
	first->Compile();
}
void NodeOneOP::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "OneOP :\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
}
UINT NodeOneOP::GetSize()
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
	NodeOneOP::Compile();
	second->Compile();
}
void NodeTwoOP::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "TwoOp :\r\n"; GoDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	GoUp();
}
UINT NodeTwoOP::GetSize()
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
	NodeTwoOP::Compile();
	third->Compile();
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
UINT NodeThreeOP::GetSize()
{
	return NodeTwoOP::GetSize() + third->GetSize();
}

//////////////////////////////////////////////////////////////////////////
// ��������������� ������� ��� NodeNumber<T>
void NodeNumberPushCommand(USHORT cmdFlag, char* data, UINT dataSize)
{
	cmdList->AddData(cmdPush);
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
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ��� ��������� ���� ��������� ��������
	first->Compile();
	if(first->GetTypeInfo() != typeVoid)
	{
		// ������� ��� � ������� �����
		cmdList->AddData(cmdPop);
		cmdList->AddData((USHORT)(podTypeToStackType[first->GetTypeInfo()->type]));
		if(first->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX)
			cmdList->AddData(first->GetTypeInfo()->size);
	}
}
void NodePopOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "PopOp :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
UINT NodePopOp::GetSize()
{
	UINT size = NodeOneOP::GetSize();
	if(first->GetTypeInfo() != typeVoid)
	{
		size += sizeof(CmdID) + sizeof(USHORT);
		if(first->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX)
			size += sizeof(UINT);
	}
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
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	// ��� ��������� ���� ��������� ��������
	first->Compile();
	// �������� �������
	cmdList->AddData(cmdID);
	cmdList->AddData((UCHAR)(aOT));
}
void NodeUnaryOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "UnaryOp :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
UINT NodeUnaryOp::GetSize()
{
	return NodeOneOP::GetSize() + sizeof(CmdID) + sizeof(UCHAR);
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ������� �� ������� ��� �� ���������
NodeReturnOp::NodeReturnOp(UINT c, TypeInfo* tinfo)
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
	cmdList->AddData((USHORT)(retType->type == TypeInfo::TYPE_COMPLEX ? retType->size : (bitRetSimple | operType)));
	cmdList->AddData((USHORT)(popCnt));
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
UINT NodeReturnOp::GetSize()
{
	return NodeOneOP::GetSize() + sizeof(CmdID) + 2 * sizeof(USHORT) + (typeInfo ? ConvertFirstToSecondSize(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]) : 0);
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
	NodeOneOP::Compile();
}
void NodeExpression::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "Expression :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
UINT NodeExpression::GetSize()
{
	return NodeOneOP::GetSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ��������� ����� ��� ����� ����������
NodeVarDef::NodeVarDef(UINT sh, std::string nm)
{
	// ����� ������� ����� ����������
	shift = sh;
	// ��� ����������
	name = nm;
}
NodeVarDef::~NodeVarDef()
{
}

void NodeVarDef::Compile()
{
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ���� ����� �� ����� ����
	if(shift)
	{
		// ������� ������� ����� ����������
		cmdList->AddData(cmdPushV);
		cmdList->AddData(shift);
	}
}
void NodeVarDef::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "VarDef '" << name << "' " << shift << "\r\n";
}
UINT NodeVarDef::GetSize()
{
	return shift ? (sizeof(CmdID) + sizeof(UINT)) : 0;
}

//////////////////////////////////////////////////////////////////////////
// ���� c ���������� ����� {}
NodeBlock::NodeBlock()
{
	first = TakeLastNode();
}
NodeBlock::~NodeBlock()
{
}

void NodeBlock::Compile()
{
	// �������� �������� ������� ����� ����������
	cmdList->AddData(cmdPushVTop);
	// �������� ���������� ����� (�� �� ��� first->Compile())
	NodeOneOP::Compile();
	// ���������� �������� ������� ����� ����������
	cmdList->AddData(cmdPopVTop);
}
void NodeBlock::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "Block :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
UINT NodeBlock::GetSize()
{
	return NodeOneOP::GetSize() + 2 * sizeof(CmdID);
}

NodeFuncDef::NodeFuncDef(FunctionInfo *info)
{
	// ��������� �������� �������
	funcInfo = info;

	first = TakeLastNode();
}
NodeFuncDef::~NodeFuncDef()
{
}

void NodeFuncDef::Compile()
{
	// ����� ���������� ������� ������� ������� �� � �����
	// ��� ������� ����� ���� ������ � ����� � ���������� ������� ���������, � ��� ���� ����������
	cmdList->AddData(cmdJmp);
	cmdList->AddData(cmdList->GetCurrPos() + sizeof(CmdID) + sizeof(UINT) + 2*sizeof(USHORT) + first->GetSize());
	funcInfo->address = cmdList->GetCurrPos();
	// ����������� ��� �������
	first->Compile();

	cmdList->AddData(cmdReturn);
	if(funcInfo->retType == typeVoid)
	{
		// ���� ������� �� ���������� ��������, �� ��� ������ ret
		cmdList->AddData((USHORT)(0));	// ���������� �������� �������� 0 ����
		cmdList->AddData((USHORT)(1));
	}else{
		// ��������� ��������� � �������
		cmdList->AddData((USHORT)(bitRetError));
		cmdList->AddData((USHORT)(1));
	}
}
void NodeFuncDef::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "FuncDef :\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
UINT NodeFuncDef::GetSize()
{
	return first->GetSize() + 2*sizeof(CmdID) + sizeof(UINT) + 2*sizeof(USHORT);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ ����� �������
NodeFuncCall::NodeFuncCall(FunctionInfo *info)
{
	// ��������� �������� �������
	funcInfo = info;
	// ��� ���������� - ��� ����������� �������� �������
	typeInfo = funcInfo->retType;

	// ������ ���� ������� ���������
	for(UINT i = 0; i < funcInfo->params.size(); i++)
		paramList.push_back(TakeLastNode());
}
NodeFuncCall::~NodeFuncCall()
{
}

void NodeFuncCall::Compile()
{
	// ���� ������� ���������, ����� �� ��������
	UINT currParam = 0;
	for(paramPtr s = paramList.rbegin(), e = paramList.rend(); s != e; s++)
	{
		// ��������� �������� ���������
		(*s)->Compile();
		// ����������� ��� � ��� �������� ��������� �������
		if(funcInfo)
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcInfo->params[currParam].varType->type]);
		else
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[typeDouble->type]);
		currParam++;
	}
	if(funcInfo->address == -1)		// ���� ������� ����������
	{
		// ������� �� �����
		cmdList->AddData(cmdCallStd);
		cmdList->AddData((UINT)funcInfo->name.length());
		cmdList->AddData(funcInfo->name.c_str(), funcInfo->name.length());
	}else{					// ���� ������� ���������� �������������
		// �������� � ��������� ��������� ����� ���, �����
		UINT addr = 0;
		for(int i = int(funcInfo->params.size())-1; i >= 0; i--)
		{
			asmStackType newST = podTypeToStackType[funcInfo->params[i].varType->type];
			asmDataType newDT = podTypeToDataType[funcInfo->params[i].varType->type];
			cmdList->AddData(cmdMov);
			cmdList->AddData((USHORT)(newST | newDT | bitAddrRelTop));
			// ����� ������ �������
			cmdList->AddData(addr);
			addr += funcInfo->params[i].varType->size;
			if(newST == STYPE_COMPLEX_TYPE)
				cmdList->AddData(funcInfo->params[i].varType->size);

			cmdList->AddData(cmdPop);
			cmdList->AddData((USHORT)(newST));
			if(newST == STYPE_COMPLEX_TYPE)
				cmdList->AddData(funcInfo->params[i].varType->size);
		}

		cmdList->AddData(cmdPushVTop);

		// ����, ������� �������� ��� ����������
		UINT allSize=0;
		for(UINT i = 0; i < funcInfo->params.size(); i++)
			allSize += funcInfo->params[i].varType->size;

		// �������� ���� ���������� �� ��� ��������
		cmdList->AddData(cmdPushV);
		cmdList->AddData(allSize);

		// ������� �� ������
		cmdList->AddData(cmdCall);
		cmdList->AddData(funcInfo->address);
		cmdList->AddData((USHORT)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[podTypeToStackType[typeInfo->type]])));
	}
}
void NodeFuncCall::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "FuncCall '" << funcInfo->name << "' :\r\n";
	GoDown();
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
UINT NodeFuncCall::GetSize()
{
	UINT size = 0;
	UINT currParam = 0;
	for(paramPtr s = paramList.rbegin(), e = paramList.rend(); s != e; s++)
	{
		size += (*s)->GetSize();
		if(funcInfo)
			size += ConvertFirstToSecondSize(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcInfo->params[currParam].varType->type]);
		else
			size += ConvertFirstToSecondSize(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[typeDouble->type]);
		currParam++;
	}
	
	if(funcInfo->address == -1)
	{
		size += sizeof(CmdID) + sizeof(UINT) + (UINT)funcInfo->name.length();
	}else{
		size += 3*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(USHORT) + (UINT)(funcInfo->params.size()) * (2*sizeof(CmdID)+2+4+2);
		for(int i = int(funcInfo->params.size())-1; i >= 0; i--)
		{
			if(funcInfo->params[i].varType->type == TypeInfo::TYPE_COMPLEX)
				size += 2*sizeof(UINT);
		}
	}

	return size;
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� �������� ������ �� ������ �������
NodePushShift::NodePushShift(int varSizeOf)
{
	// ���������� int
	typeInfo = typeInt;
	// �������� ������ ����
	sizeOfType = varSizeOf;

	first = TakeLastNode();
}
NodePushShift::~NodePushShift()
{
}

void NodePushShift::Compile()
{
	asmOperType oAsmType = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// �������� ������
	first->Compile();
	// �������� ��� � ����� �����
	cmdList->AddData(cmdCTI);
	// ��������� ��� ��������
	cmdList->AddData((UCHAR)(oAsmType));
	// ������� �� ������ �������� (����� ������ ���� � ������)
	cmdList->AddData(sizeOfType);
}
void NodePushShift::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "PushShift " << sizeOfType << "\r\n";
	GoDownB();
	first->LogToStream(ostr);
	GoUp();
}
UINT NodePushShift::GetSize()
{
	return first->GetSize() + sizeof(CmdID) + sizeof(UCHAR) + sizeof(UINT);
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ��������� ������ ���������� ������������ ���� �����
NodeGetAddress::NodeGetAddress(VariableInfo vInfo, UINT varAddr)
{
	// ���������� � ����������
	varInfo = vInfo;
	// � � �����
	varAddress = varAddr;
	// ���������� "���������" - ����� �����
	typeInfo = typeInt;
}
NodeGetAddress::~NodeGetAddress()
{
}

void NodeGetAddress::Compile()
{
	// ������� � ���� ����� ����������, ������������ ���� �����
	cmdList->AddData(cmdGetAddr);
	cmdList->AddData(varAddress);
}
void NodeGetAddress::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "GetAddress " << varInfo << " " << varAddress << "\r\n";
}
UINT NodeGetAddress::GetSize()
{
	return sizeof(CmdID) + sizeof(UINT);
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ���������� �������� ����������
NodeVarSet::NodeVarSet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool absAddr, UINT pushBytes)
{
	// ���������� � ����������
	varInfo = vInfo;
	// � � �����
	varAddress = varAddr;
	// ������������ ���������� ��������� (��� ���������� ����������)
	absAddress = absAddr;
	// ��� ����������� �������� ����� ���� ������, ���� ���������� ���������
	typeInfo = targetType;
	// ��������� ����������� ������������� ����� � ������ ����������
	shiftAddress = shiftAddr;
	// �� ������� ���� ������� ��������� ���� ����������
	bytesToPush = pushBytes;

	// ����� ��� ��������� � ������
	bakedShift = false;

	// �������� ����, ������������� ��������
	first = TakeLastNode();

	// ���� ��� ������ ����������� ���������� � ������� ������������� ������� ���
	arrSetAll = (bytesToPush && typeInfo->arrLevel != 0 && first->GetTypeInfo()->arrLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && first->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX);//arraySetAll;
	if(arrSetAll)
		shiftAddress = false;

	if(first->GetTypeInfo() == typeVoid)
		throw std::string("ERROR: cannot convert from void to " + typeInfo->GetTypeName());

	// ���� ���� �� �����
	if(first->GetTypeInfo() != typeInfo)
	{
		// ���� ��� �� ���������� ������� ����, ���
		// ���� ����������� ����������� ��������, � ��� ���� �� ���������� ������ ����������� ����������, ���
		// ���� ����������� ������� ����������, ���
		// ���� ��� ���������, ������� ���������� �����, �� ��� ���� ���, �� ������� ��������� ��������� ����������, ��
		// ������� �� ������ �������������� �����
		if(!(typeInfo->type != TypeInfo::TYPE_COMPLEX && first->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX) ||
			(typeInfo->arrLevel != first->GetTypeInfo()->arrLevel && !arrSetAll) ||
			(typeInfo->refLevel != first->GetTypeInfo()->refLevel) ||
			(typeInfo->refLevel && typeInfo->refLevel == first->GetTypeInfo()->refLevel && typeInfo->subType != first->GetTypeInfo()->subType))
		{
			if(!(typeInfo->arrLevel != 0 && first->GetTypeInfo()->arrLevel == 0 && arrSetAll))
				throw std::string("ERROR: Cannot convert '" + first->GetTypeInfo()->GetTypeName() + "' to '" + typeInfo->GetTypeName() + "'");
		}
	}

	// ���� ���������� - ������ � ����������� ���� ������
	// ��� ���� ���������� - ���� ���������� ���� � ����� ����� ������
	if(shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		second = TakeLastNode();

		// ����� ������ ������ ���� ����� ������
		if(second->GetTypeInfo()->type != TypeInfo::TYPE_INT)
			throw std::string("ERROR: NodeVarSet() address shift must be an integer number");

		if(second->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize != -1)
		{
			if(varInfo.varType->arrLevel != 0 && static_cast<NodeNumber<UINT>* >(second.get())->GetVal() > varInfo.varType->size)
				throw std::string("ERROR: array index out of range");
			varAddress += static_cast<NodeNumber<int>* >(second.get())->GetVal();
			bakedShift = true;
		}
		if(varInfo.varType->arrLevel == 0 && second->GetNodeType() == typeNodeTwoAndCmdOp)
		{
			NodeTwoAndCmdOp* nodeInside = static_cast<NodeTwoAndCmdOp*>(second.get());
			if(nodeInside->cmdID == cmdAdd && nodeInside->first->GetNodeType() == typeNodeVarGet && nodeInside->second->GetNodeType() == typeNodeNumber)
			{
				varAddress += static_cast<NodeNumber<int>* >(nodeInside->second.get())->GetVal();
				second = nodeInside->first;
			}
		}
	}
}

NodeVarSet::~NodeVarSet()
{
}

void NodeVarSet::Compile()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	if(bytesToPush)
	{
		cmdList->AddData(cmdPushV);
		cmdList->AddData(bytesToPush);
	}

	// ����������� �������� ��� ���������� ����������
	first->Compile();

	if(varInfo.varType->arrSize == -1 && shiftAddress)
	{
		// ����������� � ��� ������ �������
		ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);

		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT | (absAddress ? bitAddrAbs : bitAddrRel)));
		cmdList->AddData(varAddress);
		if(!bakedShift)
		{
			second->Compile();
			cmdList->AddData(cmdAdd);
			cmdList->AddData((UCHAR)(OTYPE_INT));
		}

		// ��������� ������� ����������
		cmdList->AddData(cmdMov);
		cmdList->AddData((USHORT)(newST | newDT | bitAddrAbs | (bakedShift ? bitShiftStk : (bitShiftStk/* | bitSizeOn*/))));
		// ����� ������ �������
		cmdList->AddData(0);
		// ����� ������ ������� (� ������) � �������, ��� �������������� ������ �� ��� �������
		/*if(!bakedShift)
			cmdList->AddData(varInfo.varType->size);
		if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
			cmdList->AddData(typeInfo->size);*/
	}else{
		if((shiftAddress || arrSetAll) && varInfo.varType->arrLevel != 0) 	// ���� ��� ������ � �������� ��� ����������� �������
		{
			if(arrSetAll)			// ���� ������� ��������� �������� ���� ������� �������
			{
				// ����������� � ��� ������ �������
				ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);
				// ��������� �������� �������
				cmdList->AddData(cmdSetRange);
				cmdList->AddData((USHORT)(newDT));
				cmdList->AddData(varAddress);
				cmdList->AddData(varInfo.varType->size/first->GetTypeInfo()->size);
			}else{					// ���� ������� ��������� �������� ����� ������ �������
				// ����������� � ��� ������ �������
				ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);

				// ����� ����� �� ������ ������� (� ������)
				if(!bakedShift)
					second->Compile();

				// ��������� ������� ����������
				cmdList->AddData(cmdMov);
				cmdList->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (bakedShift ? 0 : (bitShiftStk | bitSizeOn))));
				// ����� ������ �������
				cmdList->AddData(varAddress);
				// ����� ������ ������� (� ������) � �������, ��� �������������� ������ �� ��� �������
				if(!bakedShift)
					cmdList->AddData(varInfo.varType->size);
				if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
					cmdList->AddData(typeInfo->size);
			}
		}else{	// ���� ��� �� ������ ��� ������ ��� �������
			// ����������� � ��� ����������
			if(varInfo.varType->arrLevel == 0)
				ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);

			if(shiftAddress && !bakedShift)		// ���� ���������� - ���� ���������� ���� � ����� ����� ������
			{
				// ����� ����� � ���� (� ������)
				second->Compile();
			}

			// ��������� ������� ����������
			cmdList->AddData(cmdMov);
			cmdList->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | ((shiftAddress && !bakedShift) ? bitShiftStk : 0)));
			cmdList->AddData(varAddress);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				cmdList->AddData(typeInfo->size);
		}
	}
}
void NodeVarSet::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << (*typeInfo) << "VarSet " << varInfo << " (" << varAddress << (absAddress?" absolute":"") << ")";
	ostr << (bakedShift ? " baked" : "") << (shiftAddress ? " shift" : "") << (arrSetAll ? " set whole array" : "") << "\r\n";
	if(second)
		GoDown();
	else
		GoDownB();
	first->LogToStream(ostr);
	if(second)
	{
		GoUp();
		GoDownB();
		second->LogToStream(ostr);
	}
	GoUp();
}
UINT NodeVarSet::GetSize()
{
	asmStackType fST =	podTypeToStackType[typeInfo->type];
	asmStackType sST = first ? podTypeToStackType[first->GetTypeInfo()->type] : STYPE_INT;
	asmStackType tST = second ? podTypeToStackType[second->GetTypeInfo()->type] : STYPE_INT;

	UINT size = 0;
	if(bytesToPush)
		size += sizeof(CmdID) + sizeof(UINT);
	size += first->GetSize();

	if(varInfo.varType->arrSize == -1 && shiftAddress)
	{
		size += ConvertFirstToSecondSize(sST, fST);
		size += 2 * sizeof(CmdID) + 2 * sizeof(USHORT) + 2 * sizeof(UINT);
		if(!bakedShift)
			size += second->GetSize() + sizeof(CmdID) + 1;
	}else{
		if((shiftAddress || arrSetAll) && varInfo.varType->arrLevel != 0) 	// ���� ��� ������ � �������� ��� ����������� �������
		{
			if(arrSetAll)
			{
				size += ConvertFirstToSecondSize(sST, fST);
				size += sizeof(CmdID) + sizeof(USHORT) + 2 * sizeof(UINT);
			}else{
				if(!bakedShift)
					size += second->GetSize();
				size += ConvertFirstToSecondSize(sST, fST);
				size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
				if(!bakedShift)
					size += sizeof(UINT);
				if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
					size += sizeof(UINT);
			}
		}else{
			if(shiftAddress && !bakedShift)
				size += second->GetSize();

			if(varInfo.varType->arrLevel == 0)
				size += ConvertFirstToSecondSize(sST, fST);
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				size += sizeof(UINT);
		}
	}
	return size;
}
TypeInfo* NodeVarSet::GetTypeInfo()
{
	if(arrSetAll)
		return first->GetTypeInfo();
	return typeInfo;
}
//////////////////////////////////////////////////////////////////////////
// ���� ��� ��������� �������� ����������
NodeVarGet::NodeVarGet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool absAddr)
{
	// ���������� � ����������
	varInfo = vInfo;
	// � � �����
	varAddress = varAddr;
	// ������������ ���������� ��������� (��� ���������� ����������)
	absAddress = absAddr;
	// ��� ����������� �������� ����� ���� ������, ���� ���������� ���������
	typeInfo = targetType;
	// ��������� ����������� ������������� ����� � ������ ����������
	shiftAddress = shiftAddr;

	// ����� ��� ��������� � ������
	bakedShift = false;

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		first = TakeLastNode();

		// ����� ������ ������ ����  ����� ������
		if(first->GetTypeInfo()->type != TypeInfo::TYPE_INT)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");

		if(first->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize != -1)
		{
			if(varInfo.varType->arrLevel != 0 && static_cast<NodeNumber<UINT>* >(first.get())->GetVal() > varInfo.varType->size)
				throw std::string("ERROR: array index out of range (overflow)");
			varAddress += static_cast<NodeNumber<int>* >(first.get())->GetVal();
			bakedShift = true;
		}
		if(varInfo.varType->arrLevel == 0 && first->GetNodeType() == typeNodeTwoAndCmdOp)
		{
			NodeTwoAndCmdOp* nodeInside = static_cast<NodeTwoAndCmdOp*>(first.get());
			if(nodeInside->cmdID == cmdAdd && nodeInside->first->GetNodeType() == typeNodeVarGet && nodeInside->second->GetNodeType() == typeNodeNumber)
			{
				varAddress += static_cast<NodeNumber<int>* >(nodeInside->second.get())->GetVal();
				first = nodeInside->first;
			}
		}
	}
}

NodeVarGet::~NodeVarGet()
{
}

void NodeVarGet::Compile()
{
	asmStackType asmST = podTypeToStackType[typeInfo->type];
	asmDataType asmDT = podTypeToDataType[typeInfo->type];
	if(varInfo.varType->arrSize == -1 && typeInfo != typeVoid)
	{
		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT | (absAddress ? bitAddrAbs : bitAddrRel)));
		cmdList->AddData(varAddress);
		if(shiftAddress && !bakedShift)
		{
			first->Compile();
			cmdList->AddData(cmdAdd);
			cmdList->AddData((UCHAR)(OTYPE_INT));
		}

		// ��������� ������� ����������
		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(asmST | asmDT | bitAddrAbs | (bakedShift ? bitShiftStk : (bitShiftStk/* | bitSizeOn*/))));
		// ����� ������ �������
		cmdList->AddData(0);
	}else{
		if(shiftAddress && varInfo.varType->arrLevel != 0 && varInfo.varType->arrSize != -1) 	// ���� ��� ������ � ��������
		{
			// ����� ����� �� ������ ������� (� ������)
			if(!bakedShift)
				first->Compile();

			// �������� �������� ���������� �� ������
			cmdList->AddData(cmdPush);
			cmdList->AddData((USHORT)(asmST | asmDT | (absAddress ? bitAddrAbs : bitAddrRel) | (bakedShift ? 0 : (bitShiftStk | bitSizeOn))));
			// ����� ������ �������
			cmdList->AddData(varAddress);
			// ����� ������ ������� (� ������) � ����, ��� �������������� ������ �� ��� �������
			if(!bakedShift)
				cmdList->AddData(varInfo.varType->size);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				cmdList->AddData(typeInfo->size);
		}else{						// ���� �� ��� ������
			if(shiftAddress && !bakedShift)		// ���� ���������� - ���� ���������� ���� � ����� ����� ������
				first->Compile();		// ����� ��� � ���� (� ������)

			// �������� �������� ���������� �� ������
			cmdList->AddData(cmdPush);
			cmdList->AddData((USHORT)(asmST | asmDT | (absAddress ? bitAddrAbs : bitAddrRel) | ((shiftAddress && !bakedShift) ? bitShiftStk : 0)));
			// ����� ����������
			cmdList->AddData(varAddress);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				cmdList->AddData(typeInfo->size);
		}
	}
}
void NodeVarGet::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "VarGet " << varInfo << " (" << varAddress << (absAddress?" absolute":"") << ")";
	ostr << (bakedShift ? " baked" : "") << (shiftAddress ? " shift" : "") << "\r\n";
	if(first)
	{
		GoDownB();
		first->LogToStream(ostr);
		GoUp();
	}
}
UINT NodeVarGet::GetSize()
{
	UINT size = 0;
	if(varInfo.varType->arrSize == -1 && typeInfo != typeVoid)
	{
		size += 2 * sizeof(CmdID) + 2 * sizeof(USHORT) + 2 * sizeof(UINT);
		if(shiftAddress && !bakedShift)
			size += first->GetSize() + sizeof(CmdID) + 1;
	}else{
		if(shiftAddress && varInfo.varType->arrLevel != 0 && varInfo.varType->arrSize != -1)
		{
			if(!bakedShift)
				size += first->GetSize();
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if(!bakedShift)
				size += sizeof(UINT);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				size += 4;
		}else{
			if(shiftAddress && !bakedShift)
				size += first->GetSize();
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				size += 4;
		}
	}
	return size;
}

TypeInfo* NodeVarGet::GetTypeInfo()
{
	if(varInfo.varType->arrSize == -1 && typeInfo == typeVoid)
		return typeInt;
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ��������� �������� ���������� (a += b, � �.�.)
NodeVarSetAndOp::NodeVarSetAndOp(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool absAddr, CmdID cmd)
{
	// ���������� � ����������
	varInfo = vInfo;
	// � � �����
	varAddress = varAddr;
	// ������������ ���������� ��������� (��� ���������� ����������)
	absAddress = absAddr;
	// ��� ����������� �������� ����� ���� ������, ���� ���������� ���������
	typeInfo = targetType;
	// ��������� ����������� ������������� ����� � ������ ����������
	shiftAddress = shiftAddr;
	// �������, ����������� � ����� ����������
	cmdID = cmd;

	// ����� ��� ��������� � ������
	bakedShift = false;

	first = TakeLastNode();

	// �� ������ ������ �������� � ������������ ������ �����������
	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0 || first->GetTypeInfo()->refLevel != 0)
		throw std::string("ERROR: Operation " + std::string(binCommandToText[cmdID - cmdAdd]) + "= is not supported on '" + typeInfo->GetTypeName() + "' and '" + first->GetTypeInfo()->GetTypeName() + "'");

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(varInfo.varType->arrLevel != 0 || shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		second = TakeLastNode();

		// ����� ������ ������ ����  ����� ������
		if(second->GetTypeInfo()->type != TypeInfo::TYPE_INT)
			throw std::string("ERROR: NodeVarSetAndOp() address shift must be an integer number");

		if(second->GetNodeType() == typeNodeNumber)
		{
			if(varInfo.varType->arrLevel != 0 && static_cast<NodeNumber<UINT>* >(second.get())->GetVal() > varInfo.varType->size)
				throw std::string("ERROR: array index out of range");
			varAddress += static_cast<NodeNumber<int>* >(second.get())->GetVal();
			bakedShift = true;
		}
		if(varInfo.varType->arrLevel == 0 && second->GetNodeType() == typeNodeTwoAndCmdOp)
		{
			NodeTwoAndCmdOp* nodeInside = static_cast<NodeTwoAndCmdOp*>(second.get());
			if(nodeInside->cmdID == cmdAdd && nodeInside->first->GetNodeType() == typeNodeVarGet && nodeInside->second->GetNodeType() == typeNodeNumber)
			{
				varAddress += static_cast<NodeNumber<int>* >(nodeInside->second.get())->GetVal();
				second = nodeInside->first;
			}
		}
	}
}
NodeVarSetAndOp::~NodeVarSetAndOp()
{
}

void NodeVarSetAndOp::Compile()
{
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ��� ����� ���������� � �������� �����
	asmStackType thisST = podTypeToStackType[typeInfo->type];
	// ��� ������� �������� � �������� �����
	asmStackType firstST = podTypeToStackType[first->GetTypeInfo()->type];
	// C������� ���, ������� ��������� ����� ���������� �������� ��������
	asmStackType resultST;// = ConvertFirstForSecond(thisST, firstST);

	// ��� ����� ���������� � ����� ����������
	asmDataType thisDT = podTypeToDataType[typeInfo->type];

	// ���, ��� ������� �������� ��������
	asmOperType aOT;// = operTypeForStackType[resultST];

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		second->Compile();

	UINT shiftInStack = 0, sizeOn = 0;
	// ���� ��� ������ ��� ���� ���������� ����, �������� ���� ��� ����� � �����
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		shiftInStack = bitShiftStk;
	// ���� ��� ������, �������� ����, ��� ������� ����������� �� ������� ������
	if((varInfo.varType->arrLevel != 0) && !bakedShift)
		sizeOn = bitSizeOn;

	// ����� ����� ��� ������ ��������� ���������
	UINT addrType = absAddress ? bitAddrAbs : bitAddrRel;

	// �������� �������� ���������� � ����
	cmdList->AddData(cmdPush);
	cmdList->AddData((USHORT)(thisST | thisDT | addrType | shiftInStack | sizeOn));
	cmdList->AddData(varAddress);
	if((varInfo.varType->arrLevel != 0) && !bakedShift)
		cmdList->AddData(varInfo.varType->size);

	// ����������� � ��� ���������� �������� ��������
	resultST = ConvertFirstForSecond(thisST, firstST);

	// ������ �������� ��� ��������
	aOT = operTypeForStackType[resultST];

	// ����� �������� ������� ��������
	first->Compile();
	// ����������� � ��� ���������� �������� ��������
	firstST = ConvertFirstForSecond(firstST, thisST);

	// �������� �������� ��������
	cmdList->AddData(cmdID);
	cmdList->AddData((UCHAR)(aOT));

	// ����������� �������� � ����� � ��� ����������
	ConvertFirstToSecond(resultST, thisST);

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		second->Compile();

	// �������� ����� �������� � ���������
	cmdList->AddData(cmdMov);
	cmdList->AddData((USHORT)(thisST | thisDT | addrType | shiftInStack | sizeOn));
	cmdList->AddData(varAddress);
	if((varInfo.varType->arrLevel != 0) && !bakedShift)
		cmdList->AddData(varInfo.varType->size);
}
void NodeVarSetAndOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "VarSetAndOp (array:" << (varInfo.varType->arrLevel != 0) << ") '" << varInfo.name << "' " << (int)(varAddress) << "\r\n";
	if(varInfo.varType->arrLevel != 0 || shiftAddress)
		GoDown();
	else
		GoDownB();
	first->LogToStream(ostr);
	if(varInfo.varType->arrLevel != 0 || shiftAddress)
	{
		GoUp();
		GoDownB();
		second->LogToStream(ostr);
	}
	GoUp();
}
UINT NodeVarSetAndOp::GetSize()
{
	asmStackType thisST = podTypeToStackType[typeInfo->type];
	asmStackType firstST = podTypeToStackType[first->GetTypeInfo()->type];
	asmStackType resultST = ConvertFirstForSecondSize(thisST, firstST).second;
	asmDataType thisDT = podTypeToDataType[typeInfo->type];

	UINT size = 0;
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		size += 2*second->GetSize();
	if((varInfo.varType->arrLevel != 0) && !bakedShift)
		size += 2 * sizeof(UINT);

	size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	size += ConvertFirstForSecondSize(thisST, firstST).first;

	size += first->GetSize();
	size += ConvertFirstForSecondSize(firstST, thisST).first;

	size += sizeof(CmdID) + sizeof(UCHAR);

	size += ConvertFirstToSecondSize(resultST, thisST);

	size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	return size;
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ���������� ��� ���������� �������� ����������
NodePreValOp::NodePreValOp(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool absAddr, CmdID cmd, bool preOp)
{
	// ���������� � ����������
	varInfo = vInfo;
	// � � �����
	varAddress = varAddr;
	// ��� ����������� �������� ����� ���� ������, ���� ���������� ���������
	typeInfo = targetType;
	// ��������� ����������� ������������� ����� � ������ ����������
	shiftAddress = shiftAddr;
	// ������������ ���������� ��������� (��� ���������� ����������)
	absAddress = absAddr;
	// �������, ������� ��������� � �������� (DEC ��� INC)
	cmdID = cmd;
	// ���������� ��� ����������� ��������
	prefixOperator = preOp;
	// ���� ��������� �������� �� ������������, ����� ��������� �����������.
	optimised = false;	// �� ��������� ���������

	// ����� ��� ��������� � ������
	bakedShift = false;

	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0)
		throw std::string("ERROR: ") + (cmdID == cmdIncAt ? "increment" : "decrement") + std::string(" is not supported on '") + typeInfo->GetTypeName() + "'";

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(varInfo.varType->arrLevel != 0 || shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		first = TakeLastNode();

		// ����� ������ ������ ����  ����� ������
		if(first->GetTypeInfo()->type != TypeInfo::TYPE_INT)
			throw std::string("ERROR: NodePreValOp() address shift must be an integer number");

		if(first->GetNodeType() == typeNodeNumber)
		{
			if(varInfo.varType->arrLevel != 0 && static_cast<NodeNumber<UINT>* >(first.get())->GetVal() > varInfo.varType->size)
				throw std::string("ERROR: array index out of range (overflow)");
			varAddress += static_cast<NodeNumber<int>* >(first.get())->GetVal();
			bakedShift = true;
		}
	}
}
NodePreValOp::~NodePreValOp()
{
}

// ����� � ����������� ������������� ����������� ������ ���������� (Compiler.cpp)
// ��� ��������� ����������� ��� ��������������� �������.
void NodePreValOp::SetOptimised(bool doOptimisation)
{
	optimised = doOptimisation;
}

void NodePreValOp::Compile()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	// ������� (cmdID+10): ��������� 10, �� ������ ���������� � INC � DEC �� INC_AT � DEC_AT

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		first->Compile();

	UINT shiftInStack = 0, sizeOn = 0;
	// ���� ��� ������ ��� ���� ���������� ����, �������� ���� ��� ����� � �����
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		shiftInStack = bitShiftStk;
	// ���� ��� ������, �������� ����, ��� ������� ����������� �� ������� ������
	if((varInfo.varType->arrLevel != 0) && !bakedShift)
		sizeOn = bitSizeOn;

	// ����� ����� ��� ������ ��������� ���������
	UINT addrType = absAddress ? bitAddrAbs : bitAddrRel;

	// ���� �������� ����� �������� �� ������������, ����� �������� �����������:
	// �������� �������� ����� � ����� ����������
	if(optimised)
	{
		// ������ �������� ���������� ����� �� ������
		cmdList->AddData(cmdID);
		cmdList->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
		// ����� ������ �������
		cmdList->AddData(varAddress);
		// ���� ��� ������, ����� ������ ������� (� ������) � ����, ��� �������������� ������ �� ��� �������
		if((varInfo.varType->arrLevel != 0) && !bakedShift)
			cmdList->AddData(varInfo.varType->size);
	}else{
		// ���� ���������� - ������ ��� ���� ���������� ����
		if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		{
			// ��������� ��� ��������� ����� �������
			// ������ ��� �� ��������� ����� ��������, � �� � ��� ���
			cmdList->AddData(cmdCopy);
			cmdList->AddData((UCHAR)(OTYPE_INT));
		}
		if(prefixOperator)			// ��� ����������� ��������� ++val/--val
		{
			// ������� �������� ����������, ����� ����� � ���� � ����� ��������

			// ������ �������� ���������� ����� �� ������
			cmdList->AddData(cmdID);
			cmdList->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
			cmdList->AddData(varAddress);
			if((varInfo.varType->arrLevel != 0) && !bakedShift)
				cmdList->AddData(varInfo.varType->size);

			// �������� ����� �������� ����������
			cmdList->AddData(cmdPush);
			cmdList->AddData((USHORT)(newST | newDT | addrType | shiftInStack | sizeOn));
			cmdList->AddData(varAddress);
			if((varInfo.varType->arrLevel != 0) && !bakedShift)
				cmdList->AddData(varInfo.varType->size);
		}else{						// ���  ������������ ��������� val++/val--
			// �� �������� ����������, �� � ���� �������� ������ ��������
			
			// �������� �� ��������� �������� ����������
			cmdList->AddData(cmdPush);
			cmdList->AddData((USHORT)(newST | newDT | addrType | shiftInStack | sizeOn));
			cmdList->AddData(varAddress);
			if((varInfo.varType->arrLevel != 0) && !bakedShift)
				cmdList->AddData(varInfo.varType->size);
			
			// ���� ���������� - ������ ��� ���� ���������� ����
			// ������ � ����� ���������� ����� ����� ������, � ����� �������� ����������
			// ������� �������� �� �������, ��� ��� ��� ��������� ���������� ����� ����� ������ ������ �������
			if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
			{
				cmdList->AddData(cmdSwap);
				cmdList->AddData((USHORT)(STYPE_INT | (newDT == DTYPE_FLOAT ? DTYPE_DOUBLE : newDT)));
			}
			
			// ������ �������� ���������� ����� �� ������
			cmdList->AddData(cmdID);
			cmdList->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
			cmdList->AddData(varAddress);
			if((varInfo.varType->arrLevel != 0) && !bakedShift)
				cmdList->AddData(varInfo.varType->size);
		}
	}
}
void NodePreValOp::LogToStream(ostringstream& ostr)
{
	static char* strs[] = { "++", "--" };
	if(cmdID != cmdIncAt &&  cmdID != cmdDecAt)
		throw std::string("ERROR: PreValOp error");
	DrawLine(ostr);
	ostr << *typeInfo << "PreValOp<" << strs[cmdID-cmdIncAt] << "> :\r\n";
	if(first)
	{
		GoDown();
		first->LogToStream(ostr);
		GoUp();
	}
}

UINT NodePreValOp::GetSize()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	UINT size = 0;
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		size += first->GetSize();
	if(optimised)
	{
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
		if((varInfo.varType->arrLevel != 0) && !bakedShift)
			size += sizeof(UINT);
	}else{
		if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
			size += sizeof(CmdID) + sizeof(UCHAR);
		if(prefixOperator)
		{
			size += 2 * (sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT));
			if((varInfo.varType->arrLevel != 0) && !bakedShift)
				size += 2 * sizeof(UINT);
		}else{
			if((varInfo.varType->arrLevel != 0) && !bakedShift)
				size += 2 * sizeof(UINT);
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
				size += sizeof(CmdID) + sizeof(USHORT);
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
		}
	}
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
	cmdList->AddData((UCHAR)(operTypeForStackType[fST]));
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
UINT NodeTwoAndCmdOp::GetSize()
{
	asmStackType fST = podTypeToStackType[first->GetTypeInfo()->type], sST = podTypeToStackType[second->GetTypeInfo()->type];
	UINT resSize = 0;
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
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ��������� �������� ���������: if(first) second; else third;
	// ������ �������: first ? second : third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// �������� �������
	first->Compile();

	// ���� false, ������� � ���� else ��� ������ �� ���������, ���� ������ ����� �� �������
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((UCHAR)(aOT));
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
}
void NodeIfElseExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "IfExpression :\r\n";
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
UINT NodeIfElseExpr::GetSize()
{
	UINT size = first->GetSize() + second->GetSize() + sizeof(CmdID) + sizeof(UINT) + sizeof(UCHAR);
	if(third)
		size += third->GetSize() + sizeof(CmdID) + sizeof(UINT);
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
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// ��������� �������� ���������: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// �������� �������������
	first->Compile();
	UINT posTestExpr = cmdList->GetCurrPos();

	// ����� ��������� �������
	second->Compile();

	// ���� �����, ������ �� �����
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((UCHAR)(aOT));
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmdList->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);
	cmdList->AddData(cmdList->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);

	// �������� ���������� �����
	fourth->Compile();
	// �������� ��������, ���������� ����� ������ ��������
	third->Compile();
	// ������� �� �������� �������
	cmdList->AddData(cmdJmp);
	cmdList->AddData(posTestExpr);
	indTemp.pop_back();
}
void NodeForExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "ForExpression :\r\n";
	GoDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	third->LogToStream(ostr);
	GoUp();
	GoDownB(); 
	fourth->LogToStream(ostr);
	GoUp();
}
UINT NodeForExpr::GetSize()
{
	return NodeThreeOP::GetSize() + fourth->GetSize() + 2*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(UCHAR);
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
	// ��������� �������� ���������: while(first) second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	UINT posStart = cmdList->GetCurrPos();
	// �������� �������
	first->Compile();
	// ���� ��� �����, ������ �� �����
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((UCHAR)(aOT));
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmdList->GetCurrPos()+4+second->GetSize()+2+4);
	cmdList->AddData(cmdList->GetCurrPos()+4+second->GetSize()+2+4);
	// �������� ���������� �����
	second->Compile();
	// ������� �� �������� �������
	cmdList->AddData(cmdJmp);
	cmdList->AddData(posStart);
}
void NodeWhileExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "WhileExpression :\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
	GoDownB(); 
	second->LogToStream(ostr);
	GoUp();
}
UINT NodeWhileExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + 2*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(UCHAR);
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
	// ��������� �������� ���������: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	UINT posStart = cmdList->GetCurrPos();
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmdList->GetCurrPos()+first->GetSize()+second->GetSize()+2+4);
	// �������� ���������� �����
	first->Compile();
	// �������� �������
	second->Compile();
	// ���� ������� �����, ������� � ���������� ��������� �������� �����
	cmdList->AddData(cmdJmpNZ);
	cmdList->AddData((UCHAR)(aOT));
	cmdList->AddData(posStart);
	indTemp.pop_back();
}
void NodeDoWhileExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "DoWhileExpression :\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
	GoDownB();
	second->LogToStream(ostr);
	GoUp();
}
UINT NodeDoWhileExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + sizeof(CmdID) + sizeof(UINT) + sizeof(UCHAR);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� break;
NodeBreakOp::NodeBreakOp(UINT c)
{
	// ������� �������� ����� ������ �� ����� ������ ����� ���������� (�_�)
	popCnt = c;
}
NodeBreakOp::~NodeBreakOp()
{
}

void NodeBreakOp::Compile()
{
	// ����� �������� �� ����� ������ ����� ����������
	for(UINT i = 0; i < popCnt; i++)
		cmdList->AddData(cmdPopVTop);
	// ������ �� �����
	cmdList->AddData(cmdJmp);
	cmdList->AddData(indTemp.back());
}
void NodeBreakOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "BreakExpression\r\n";
}
UINT NodeBreakOp::GetSize()
{
	return (1+popCnt)*sizeof(CmdID) + sizeof(UINT);
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
	asmStackType aST = podTypeToStackType[first->GetTypeInfo()->type];
	asmOperType aOT = operTypeForStackType[aST];
	// �������� ������� ����� ����������
	cmdList->AddData(cmdPushVTop);
	// ����� �������� �� �������� ����� �������� ������� ����
	first->Compile();

	// ����� ����� ������
	UINT switchEnd = cmdList->GetCurrPos() + 2*sizeof(CmdID) + sizeof(UINT) + sizeof(USHORT) + caseCondList.size() * (3*sizeof(CmdID) + 3 + sizeof(UINT));
	for(casePtr s = caseCondList.begin(), e = caseCondList.end(); s != e; s++)
		switchEnd += (*s)->GetSize();
	UINT condEnd = switchEnd;
	int blockNum = 0;
	for(casePtr s = caseBlockList.begin(), e = caseBlockList.end(); s != e; s++, blockNum++)
		switchEnd += (*s)->GetSize() + sizeof(CmdID) + sizeof(USHORT) + (blockNum != caseBlockList.size()-1 ? sizeof(CmdID) + sizeof(UINT) : 0);

	// �������� ����� ��� ��������� break;
	indTemp.push_back(switchEnd+2);

	// ����������� ��� ��� ���� case'��
	casePtr cond = caseCondList.begin(), econd = caseCondList.end();
	casePtr block = caseBlockList.begin(), eblocl = caseBlockList.end();
	UINT caseAddr = condEnd;
	for(; cond != econd; cond++, block++)
	{
		cmdList->AddData(cmdCopy);
		cmdList->AddData((UCHAR)(aOT));

		(*cond)->Compile();
		// ������� �� ���������
		cmdList->AddData(cmdEqual);
		cmdList->AddData((UCHAR)(aOT));
		// ���� �����, ������� �� ������ ����
		cmdList->AddData(cmdJmpNZ);
		cmdList->AddData((UCHAR)(aOT));
		cmdList->AddData(caseAddr);
		caseAddr += (*block)->GetSize() + 2*sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	}
	// ����� � ������� ����� �������� �� �������� ��������� ������� ����
	cmdList->AddData(cmdPop);
	cmdList->AddData((USHORT)(aST));

	cmdList->AddData(cmdJmp);
	cmdList->AddData(switchEnd);
	blockNum = 0;
	for(block = caseBlockList.begin(), eblocl = caseBlockList.end(); block != eblocl; block++, blockNum++)
	{
		// ����� � ������� ����� �������� �� �������� ��������� ������� ����
		cmdList->AddData(cmdPop);
		cmdList->AddData((USHORT)(aST));
		(*block)->Compile();
		if(blockNum != caseBlockList.size()-1)
		{
			cmdList->AddData(cmdJmp);
			cmdList->AddData(cmdList->GetCurrPos() + sizeof(UINT) + sizeof(CmdID) + sizeof(USHORT));
		}
	}

	// ���������� ������� ����� ��������
	cmdList->AddData(cmdPopVTop);

	indTemp.pop_back();
}
void NodeSwitchExpr::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << "SwitchExpression :\r\n";
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
UINT NodeSwitchExpr::GetSize()
{
	UINT size = 0;
	size += first->GetSize();
	for(casePtr s = caseCondList.begin(), e = caseCondList.end(); s != e; s++)
		size += (*s)->GetSize();
	int blockNum = 0;
	for(casePtr s = caseBlockList.begin(), e = caseBlockList.end(); s != e; s++, blockNum++)
		size += (*s)->GetSize() + sizeof(CmdID) + sizeof(USHORT) + (blockNum != caseBlockList.size()-1 ? sizeof(CmdID) + sizeof(UINT) : 0);
	size += 4*sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	size += (UINT)caseCondList.size() * (3 * sizeof(CmdID) + 3 + sizeof(UINT));
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

void NodeExpressionList::Compile()
{
	for(listPtr s = exprList.begin(), e = exprList.end(); s != e; s++)
		(*s)->Compile();
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
UINT NodeExpressionList::GetSize()
{
	UINT size = 0;
	for(listPtr s = exprList.begin(), e = exprList.end(); s != e; s++)
		size += (*s)->GetSize();
	return size;
}
