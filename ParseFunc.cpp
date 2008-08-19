#include "stdafx.h"
#include "ParseFunc.h"

CommandList*	cmds;
std::vector<FunctionInfo*>* funcs;
std::vector<shared_ptr<NodeZeroOP> >*	nodeList;
ostringstream*	logStream;

void	SetCommandList(CommandList* list){ cmds = list; }
void	SetFunctionList(std::vector<FunctionInfo*>* list){ funcs = list; }
void	SetLogStream(ostringstream* stream){ logStream = stream; }
void	SetNodeList(std::vector<shared_ptr<NodeZeroOP> >* list){ nodeList = list; }
std::vector<shared_ptr<NodeZeroOP> >*	getList(){ return nodeList; }
ostringstream&	getLog(){ return *logStream; }

CommandList*	GetCommandList(){ return cmds; }

std::vector<UINT>			indTemp;

//////////////////////////////////////////////////////////////////////////

int	level = 0;
std::string preStr = "--";
bool preNeedChange=false;
void	goDown(){ level++; preStr = preStr.substr(0, preStr.length()-2); preStr += "  |--"; }
void	goDownB(){ goDown(); preNeedChange = true; }
void	goUp(){ level--; preStr = preStr.substr(0, preStr.length()-5); preStr += "--"; }
void	drawLn(ostringstream& ostr)
{
	ostr << preStr;
	if(preNeedChange)
	{
		preNeedChange = false;
		goUp();
		level++; preStr = preStr.substr(0, preStr.length()-2); preStr += "   --"; 
	}
}

//Functions for work with types

//returns new value type
asmStackType	ConvertToReal(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	if(st == STYPE_DOUBLE)
		return st;
	cmds->AddData(cmdITOR);
	cmds->AddData((USHORT)(st | DTYPE_DOUBLE));
	return STYPE_DOUBLE;
}
asmStackType	ConvertToInteger(shared_ptr<NodeZeroOP> op, asmStackType st)
{
	if(st == STYPE_INT || st == STYPE_LONG)
		return st;
	cmds->AddData(cmdRTOI);
	cmds->AddData((USHORT)(st | DTYPE_INT));
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
		cmds->AddData(cmdITOR);
		cmds->AddData((USHORT)(first | dataTypeForStackType[second]));
		return second;
	}
	if(first == STYPE_INT && second == STYPE_LONG)
	{
		cmds->AddData(cmdITOL);
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
			cmds->AddData(cmdITOR);
			cmds->AddData((USHORT)(first | DTYPE_DOUBLE));
		}
	}else if(second == STYPE_LONG){
		if(first == STYPE_INT)
		{
			cmds->AddData(cmdITOL);
		}else if(first == STYPE_DOUBLE){
			cmds->AddData(cmdRTOI);
			cmds->AddData((USHORT)(STYPE_DOUBLE | DTYPE_LONG));
		}
	}else if(second == STYPE_INT){
		if(first == STYPE_DOUBLE)
		{
			cmds->AddData(cmdRTOI);
			cmds->AddData((USHORT)(STYPE_DOUBLE | DTYPE_INT));
		}else if(first == STYPE_LONG){
			cmds->AddData(cmdLTOI);
		}
	}
}

TypeInfo*	ChooseBinaryOpResultType(TypeInfo* a, TypeInfo* b)
{
	if(a->type == TypeInfo::POD_DOUBLE)
		return a;
	if(b->type == TypeInfo::POD_DOUBLE)
		return b;
	if(a->type == TypeInfo::POD_FLOAT)
		return a;
	if(b->type == TypeInfo::POD_FLOAT)
		return b;
	if(a->type == TypeInfo::POD_LONG)
		return a;
	if(b->type == TypeInfo::POD_LONG)
		return b;
	if(a->type == TypeInfo::POD_INT)
		return a;
	if(b->type == TypeInfo::POD_INT)
		return b;
	if(a->type == TypeInfo::POD_SHORT)
		return a;
	if(b->type == TypeInfo::POD_SHORT)
		return b;
	if(a->type == TypeInfo::POD_CHAR)
		return a;
	if(b->type == TypeInfo::POD_CHAR)
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
}
NodeZeroOP::NodeZeroOP(TypeInfo* tinfo)
{
	typeInfo = tinfo;
}
NodeZeroOP::~NodeZeroOP()
{
}

void NodeZeroOP::doAct()
{
}
void NodeZeroOP::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "ZeroOp\r\n";
}
UINT NodeZeroOP::getSize()
{
	return 0;
}
TypeInfo* NodeZeroOP::getTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// ����, ������� ���� �������� ����
NodeOneOP::NodeOneOP()
{
}
NodeOneOP::~NodeOneOP()
{
}

void NodeOneOP::doAct()
{
	first->doAct();
}
void NodeOneOP::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "OneOP :\r\n";
	goDown();
	first->doLog(ostr);
	goUp();
}
UINT NodeOneOP::getSize()
{
	return first->getSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ������� ��� �������� ����
NodeTwoOP::NodeTwoOP()
{
}
NodeTwoOP::~NodeTwoOP()
{
}

void NodeTwoOP::doAct()
{
	NodeOneOP::doAct();
	second->doAct();
}
void NodeTwoOP::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "TwoOp :\r\n"; goDown();
	first->doLog(ostr);
	second->doLog(ostr);
	goUp();
}
UINT NodeTwoOP::getSize()
{
	return NodeOneOP::getSize() + second->getSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ������� ��� �������� ����
NodeThreeOP::NodeThreeOP()
{
}
NodeThreeOP::~NodeThreeOP()
{
}

void NodeThreeOP::doAct()
{
	NodeTwoOP::doAct();
	third->doAct();
}
void NodeThreeOP::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "ThreeOp :\r\n";
	goDown();
	first->doLog(ostr);
	second->doLog(ostr);
	third->doLog(ostr);
	goUp();
}
UINT NodeThreeOP::getSize()
{
	return NodeTwoOP::getSize() + third->getSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ��������� � ������� ����� ��������, ����������� �������� �����
NodePopOp::NodePopOp()
{
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodePopOp::~NodePopOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodePopOp::doAct()
{
	// ��� ��������� ���� ��������� ��������
	first->doAct();
	// ������� ��� � ������� �����
	cmds->AddData(cmdPop);
	cmds->AddData((USHORT)(podTypeToStackType[first->getTypeInfo()->type]));
}
void NodePopOp::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "PopOp :\r\n";
	goDownB();
	first->doLog(ostr);
	goUp();
}
UINT NodePopOp::getSize()
{
	return NodeOneOP::getSize() + sizeof(CmdID) + sizeof(USHORT);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� ������� �������� ��� ��������� �� ������� �����
NodeUnaryOp::NodeUnaryOp(CmdID cmd)
{
	// ������� ��������
	cmdID = cmd;

	first = getList()->back(); getList()->pop_back();
	// ��� ���������� ����� ��, ��� ��������
	typeInfo = first->getTypeInfo();

	getLog() << __FUNCTION__ << "\r\n";
}
NodeUnaryOp::~NodeUnaryOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeUnaryOp::doAct()
{
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->getTypeInfo()->type]];

	// ��� ��������� ���� ��������� ��������
	first->doAct();
	// �������� �������
	cmds->AddData(cmdID);
	cmds->AddData((UCHAR)(aOT));
}
void NodeUnaryOp::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "UnaryOp :\r\n";
	goDown();
	first->doLog(ostr);
	goUp();
}
UINT NodeUnaryOp::getSize()
{
	return NodeOneOP::getSize() + sizeof(CmdID) + sizeof(UCHAR);
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ������� �� ������� ��� �� ���������
NodeReturnOp::NodeReturnOp(UINT c, TypeInfo* tinfo)
{
	// ������� �������� ����� ������ �� ����� ������ ����� ���������� (�_�)
	popCnt = c;
	// ��� ���������� ������������ �����
	typeInfo = tinfo;

	first = getList()->back(); getList()->pop_back();
	
	getLog() << __FUNCTION__ << "\r\n";
}
NodeReturnOp::~NodeReturnOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeReturnOp::doAct()
{
	// ����� ��������, ������� ����� ����������
	first->doAct();
	// ����������� ��� � ��� ����������� �������� �������
	if(typeInfo)
		ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], podTypeToStackType[typeInfo->type]);
	// ����� �������� �� ����� ������ ����� ����������
	for(UINT i = 0; i < popCnt; i++)
		cmds->AddData(cmdPopVTop);
	// ������ �� ������� ��� ���������
	cmds->AddData(cmdReturn);
}
void NodeReturnOp::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	if(typeInfo)
		ostr << *typeInfo << "ReturnOp :\r\n";
	else
		ostr << *first->getTypeInfo() << "ReturnOp :\r\n";
	goDownB(); first->doLog(ostr); goUp();
}
UINT NodeReturnOp::getSize()
{
	return NodeOneOP::getSize() + sizeof(CmdID) * (1+popCnt) + (typeInfo ? ConvertFirstToSecondSize(podTypeToStackType[first->getTypeInfo()->type], podTypeToStackType[typeInfo->type]) : 0);
}

//////////////////////////////////////////////////////////////////////////
// ����, ���������� ���������. �������� ��� NodeOneOP �� ����������� ������ � ���.
// ����� ������? BUG 0001
NodeExpression::NodeExpression()
{
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeExpression::~NodeExpression()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeExpression::doAct()
{
	NodeOneOP::doAct();
}
void NodeExpression::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "Expression :\r\n";
	goDownB();
	first->doLog(ostr);
	goUp();
}
UINT NodeExpression::getSize()
{
	return NodeOneOP::getSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ��������� ����� ��� ����� ����������
NodeVarDef::NodeVarDef(UINT sh, std::string nm)
{
	// ����� ������� ����� ����������
	shift = sh;
	// ��� ����������
	name = nm;
	getLog() << __FUNCTION__ << "\r\n";
}
NodeVarDef::~NodeVarDef()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarDef::doAct()
{
	// ���� ����� �� ����� ����
	if(shift)
	{
		// ������� ������� ����� ����������
		cmds->AddData(cmdPushV);
		cmds->AddData(shift);
	}
}
void NodeVarDef::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "VarDef '" << name << "' " << shift << "\r\n";
}
UINT NodeVarDef::getSize()
{
	return shift ? (sizeof(CmdID) + sizeof(UINT)) : 0;
}

//////////////////////////////////////////////////////////////////////////
// ���� c ���������� ����� {}
NodeBlock::NodeBlock()
{
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeBlock::~NodeBlock()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeBlock::doAct()
{
	// �������� �������� ������� ����� ����������
	cmds->AddData(cmdPushVTop);
	// �������� ���������� ����� (�� �� ��� first->doAct())
	NodeOneOP::doAct();
	// ���������� �������� ������� ����� ����������
	cmds->AddData(cmdPopVTop);
}
void NodeBlock::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "Block :\r\n";
	goDownB();
	first->doLog(ostr);
	goUp();
}
UINT NodeBlock::getSize()
{
	return NodeOneOP::getSize() + 2 * sizeof(CmdID);
}

NodeFuncDef::NodeFuncDef(UINT id)
{
	// ����� �������
	funcID = id;
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeFuncDef::~NodeFuncDef()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeFuncDef::doAct()
{
	// ����� ���������� ������� ������� ������� �� � �����
	// ��� ������� ����� ���� ������ � ����� � ���������� ������� ���������, � ��� ���� ����������
	cmds->AddData(cmdJmp);
	cmds->AddData(cmds->GetCurrPos() + sizeof(CmdID) + sizeof(UINT) + first->getSize());
	(*funcs)[funcID]->address = cmds->GetCurrPos();
	// ����������� ��� �������
	first->doAct();
	// ������� ������� �� �������, ���� ������������ ����� (�� ������ ��� �� ��� �����, ������� �� ������)
	cmds->AddData(cmdReturn);
}
void NodeFuncDef::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "FuncDef :\r\n";
	goDownB(); first->doLog(ostr); goUp();
}
UINT NodeFuncDef::getSize()
{
	return first->getSize() + 2*sizeof(CmdID) + sizeof(UINT);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� ������� ���������� ����� ������� �������
NodeFuncParam::NodeFuncParam(TypeInfo* tinfo)
{
	// ���, ������� ������� �������
	typeInfo = tinfo;

	first = getList()->back(); getList()->pop_back();
}
NodeFuncParam::~NodeFuncParam()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeFuncParam::doAct()
{
	// ��������� ��������
	first->doAct();
	// ����������� ��� � ��� �������� ��������� �������
	ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], podTypeToStackType[typeInfo->type]);
}
void NodeFuncParam::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "FuncParam :\r\n";
	goDown();
	first->doLog(ostr);
	goUp();
}
UINT NodeFuncParam::getSize()
{
	return first->getSize() + ConvertFirstToSecondSize(podTypeToStackType[first->getTypeInfo()->type], podTypeToStackType[typeInfo->type]);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ ����� �������
NodeFuncCall::NodeFuncCall(std::string name, UINT id, UINT argCnt, TypeInfo* retType)
{
	// ��� ���������� - ��� ����������� �������� �������
	typeInfo = retType;
	// ��� �������
	funcName = name;
	// ������������� �������
	funcID = id;

	// ���� ������� ��������� ���������, ������� ����� ����, ������� ���������� �� ��������
	if(argCnt)
	{
		first = getList()->back(); getList()->pop_back();
	}
	// ���� ������� �� ����� ��������������, ������ ��� ����������
	if(id == -1)
	{
		// clock() - ������������ ������� ������������ int, � �� double
		if(funcName == "clock")
			typeInfo = typeInt;
		else
			typeInfo = typeDouble;
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodeFuncCall::~NodeFuncCall()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeFuncCall::doAct()
{
	// ���� ������� ���������, ����� �� ��������
	if(first)
		first->doAct();
	if(funcID == -1)		// ���� ������� ����������
	{
		// ������� �� �����
		cmds->AddData(cmdCallStd);
		cmds->AddData((UINT)funcName.length());
		cmds->AddData(funcName.c_str(), funcName.length());
	}else{					// ���� ������� ���������� �������������
		// ������� �� ������
		cmds->AddData(cmdCall);
		cmds->AddData((*funcs)[funcID]->address);
	}
}
void NodeFuncCall::doLog(ostringstream& ostr)
{
	drawLn(ostr); ostr << "FuncCall '" << funcName << "' :\r\n"; goDownB(); if(first) first->doLog(ostr); goUp();
}
UINT NodeFuncCall::getSize()
{
	if(first)
		if(funcID == -1)
			return first->getSize() + sizeof(CmdID) + sizeof(UINT) + funcName.length();// + ConvertToRealSize(first, podTypeToStackType[first->typeInfo()->type]);
		else
			return first->getSize() + sizeof(CmdID) + sizeof(UINT);// + ConvertToRealSize(first, podTypeToStackType[first->typeInfo()->type]);
	else
		if(funcID == -1)
			return sizeof(CmdID) + sizeof(UINT) + (UINT)funcName.length();
		else
			return sizeof(CmdID) + sizeof(UINT);
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� �������� ������ �� ������ �������
NodePushShift::NodePushShift(int varSizeOf)
{
	// ���������� int
	typeInfo = typeInt;
	// �������� ������ ����
	sizeOfType = varSizeOf;

	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodePushShift::~NodePushShift()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodePushShift::doAct()
{
	// �������� ������
	first->doAct();
	// �������� ��� � ����� �����
	cmds->AddData(cmdCTI);
	// ������� �� ������ �������� (����� ������ ���� � ������)
	cmds->AddData(sizeOfType);
}
void NodePushShift::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "PushShift " << sizeOfType << "\r\n";
	goDown();
	first->doLog(ostr);
	goUp();
}
UINT NodePushShift::getSize()
{
	return first->getSize() + sizeof(CmdID) + sizeof(UINT);
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ���������� �������� ����������
NodeVarSet::NodeVarSet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool arraySetAll, bool absAddr)
{
	// ���������� � ����������
	varInfo = vInfo;
	// � � �����
	varAddress = varAddr;
	// ��������� �������� ���� ��������� �������
	arrSetAll = arraySetAll;
	// ������������ ���������� ��������� (��� ���������� ����������)
	absAddress = absAddr;
	// ��� ����������� �������� ����� ���� ������, ���� ���������� ���������
	typeInfo = targetType;
	// ��������� ����������� ������������� ����� � ������ ����������
	shiftAddress = shiftAddr;

	// �������� ����, ������������� ��������
	first = getList()->back(); getList()->pop_back();

	// ���� ���������� - ������ � ����������� ���� ������
	// ��� ���� ���������� - ���� ���������� ���� � ����� ����� ������
	if((varInfo.count > 1 && !arrSetAll) || shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		second = getList()->back(); getList()->pop_back();

		// ����� ������ ������ ����  ����� ������
		if(second->getTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarSet() address shift must be an integer number");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
};

NodeVarSet::~NodeVarSet()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarSet::doAct()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	if(varInfo.count == 1)	// ���� ��� �� ������
	{
		if(shiftAddress)		// ���� ���������� - ���� ���������� ���� � ����� ����� ������
		{
			// ����� ����� � ���� (� ������)
			second->doAct();
		}

		// ����������� �������� ��� ���������� ����������
		first->doAct();
		// ����������� � ��� ����������
		ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], newST);

		// ��������� ������� ����������
		cmds->AddData(cmdMov);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (shiftAddress ? bitShiftStk : 0)));
		cmds->AddData(varAddress);
	}else{						// ���� ��� ������
		if(arrSetAll)			// ���� ������� ��������� �������� ���� ������� �������
		{
			// ����������� �������� ��� ���������� ����������
			first->doAct();
			// ����������� � ��� ������ �������
			ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], newST);
			// ��� ������� ��������
			for(UINT n = 0; n < varInfo.count; n++)
			{
				// ������� � ���� ����� �� ������ ������� (� ������)
				cmds->AddData(cmdPush);
				cmds->AddData((USHORT)(STYPE_INT | DTYPE_INT));
				cmds->AddData(n * typeInfo->size);
				
				// �������� ������� �������� �� �������� �����
				// (��� ���������� MOV �������� ������ ��������� �� �������)
				cmds->AddData(cmdSwap);
				cmds->AddData((USHORT)(newST | DTYPE_INT));
				
				// ��������� ������� ����������
				cmds->AddData(cmdMov);
				cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
				cmds->AddData(varAddress);
				cmds->AddData(varInfo.count * typeInfo->size);
			}
		}else{					// ���� ������� ��������� �������� ����� ������ �������
			// ����� ����� �� ������ ������� (� ������)
			second->doAct();
			
			// ����������� �������� ��� ���������� ������ �������
			first->doAct();
			// ����������� � ��� ������ �������
			ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], newST);
			
			// ��������� ������� ����������
			cmds->AddData(cmdMov);
			cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
			// ����� ������ �������
			cmds->AddData(varAddress);
			// ����� ������ ������� (� ������) � ����, ��� �������������� ������ �� ��� �������
			cmds->AddData(varInfo.count * varInfo.varType->size);
		}
	}
}
void NodeVarSet::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "VarSet " << varInfo << " " << varAddress << "\r\n";
	goDown();
	if(first)
		first->doLog(ostr);
	goUp();
	goDownB();
	if(second)
		second->doLog(ostr);
	goUp();
}
UINT NodeVarSet::getSize()
{
	asmStackType fST =	podTypeToStackType[typeInfo->type],
						sST = first ? podTypeToStackType[first->getTypeInfo()->type] : STYPE_INT,
						tST = second ? podTypeToStackType[second->getTypeInfo()->type] : STYPE_INT;
	if(varInfo.count == 1)
	{
		if(first)
			return (shiftAddress ? second->getSize() : 0) + first->getSize() + ConvertFirstToSecondSize(fST, sST) + sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
		else
			return sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	}else{
		if(arrSetAll)
		{
			return first->getSize() + ConvertFirstToSecondSize(fST, sST) + varInfo.count * (3*sizeof(CmdID)+3*sizeof(USHORT)+3*sizeof(UINT));
		}else{
			return first->getSize() + ConvertFirstToSecondSize(fST, sST) + second->getSize() + sizeof(CmdID)+sizeof(USHORT)+2*sizeof(UINT);
		}
	}
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

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(varInfo.count > 1 || shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		first = getList()->back(); getList()->pop_back();

		// ����� ������ ������ ����  ����� ������
		if(first->getTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}

NodeVarGet::~NodeVarGet()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarGet::doAct()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];
	if(varInfo.count > 1) 	// ���� ��� ������
	{
		// ����� ����� �� ������ ������� (� ������)
		first->doAct();

		// �������� �������� ���������� �� ������
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
		// ����� ������ �������
		cmds->AddData(varAddress);
		// ����� ������ ������� (� ������) � ����, ��� �������������� ������ �� ��� �������
		cmds->AddData(varInfo.count * varInfo.varType->size);
	}else{						// ���� �� ��� ������
		if(shiftAddress)		// ���� ���������� - ���� ���������� ���� � ����� ����� ������
			first->doAct();		// ����� ��� � ���� (� ������)

		// �������� �������� ���������� �� ������
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (shiftAddress ? bitShiftStk : 0)));
		// ����� ����������
		cmds->AddData(varAddress);
		
	}
}
void NodeVarGet::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "VarGet (array:" << (varInfo.count > 1) << ") '" << varInfo.name << "' " << (int)(varAddress) << "\r\n";
	goDown();
	if(first)
		first->doLog(ostr);
	goUp();
}
UINT NodeVarGet::getSize()
{
	if(varInfo.count > 1)
		return first->getSize() + sizeof(CmdID) + sizeof(USHORT) + 2*sizeof(UINT);
	else
		return sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT) + (shiftAddress ? first->getSize() : 0);
}

//////////////////////////////////////////////////////////////////////////
// ���� ��� ��������� �������� ����������
NodeVarSetAndOp::NodeVarSetAndOp(TypeInfo* tinfo, UINT vpos, std::string name, bool arr, UINT size, CmdID cmd)
{
	typeInfo = tinfo; m_vpos = vpos; m_name = name; m_arr = arr; m_size = size; m_cmd = cmd;
	first = getList()->back(); getList()->pop_back();
	if(arr){
		second = getList()->back(); getList()->pop_back();
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodeVarSetAndOp::~NodeVarSetAndOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarSetAndOp::doAct()
{
	asmOperType aOT;
	asmStackType meST, firstST, newST;
	asmDataType meDT = podTypeToDataType[typeInfo->type];
	meST = podTypeToStackType[typeInfo->type];
	firstST = podTypeToStackType[first->getTypeInfo()->type];

	UINT additFlag = bitAddrRel;
	if(m_arr)
	{
		additFlag = bitAddrRel | bitShiftStk | bitSizeOn;
		second->doAct();
		ConvertToInteger(second, podTypeToStackType[second->getTypeInfo()->type]);
		cmds->AddData(cmdCopy);
		cmds->AddData((UCHAR)(OTYPE_INT));
	}

	cmds->AddData(cmdPush);
	cmds->AddData((USHORT)(meST | meDT | additFlag));
	cmds->AddData(m_vpos);
	if(m_arr)
		cmds->AddData(m_size);
	newST = ConvertFirstForSecond(meST, firstST);

	first->doAct();
	firstST = ConvertFirstForSecond(firstST, newST);
	aOT = operTypeForStackType[newST];

	cmds->AddData(m_cmd);
	cmds->AddData((UCHAR)(aOT));

	ConvertFirstToSecond(newST, meST);
	
	cmds->AddData(cmdMov);
	cmds->AddData((USHORT)(meST | meDT | additFlag));
	cmds->AddData(m_vpos);
	if(m_arr)
		cmds->AddData(m_size);
}
void NodeVarSetAndOp::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "VarSet (array:" << m_arr << ") '" << m_name << "' " << (int)(m_vpos) << "\r\n";
	goDown();
	first->doLog(ostr);
	goUp();
	goDownB();
	if(m_arr)
		second->doLog(ostr);
	goUp();
}
UINT NodeVarSetAndOp::getSize()
{
	asmStackType meST, firstST, newST;
	asmDataType meDT = podTypeToDataType[typeInfo->type];
	meST = podTypeToStackType[typeInfo->type];
	firstST = podTypeToStackType[first->getTypeInfo()->type];

	UINT resSize = 0;
	if(m_arr)
		resSize += ConvertToIntegerSize(second, podTypeToStackType[second->getTypeInfo()->type]);
	resSize += ConvertFirstForSecondSize(meST, firstST).first;
	newST = ConvertFirstForSecondSize(meST, firstST).second;
	resSize += ConvertFirstForSecondSize(firstST, newST).first;
	resSize += ConvertFirstToSecondSize(newST, meST);

	if(m_arr)
		return second->getSize() + first->getSize() + 4*sizeof(CmdID)+2*sizeof(USHORT)+2*sizeof(UCHAR)+4*sizeof(UINT) + resSize;
	else
		return first->getSize() + 3*sizeof(CmdID)+2*sizeof(USHORT)+sizeof(UCHAR)+2*sizeof(UINT) + resSize;
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

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(varInfo.count > 1 || shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		first = getList()->back(); getList()->pop_back();

		// ����� ������ ������ ����  ����� ������
		if(first->getTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodePreValOp::~NodePreValOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

// ����� � ����������� ������������� ����������� ������ ���������� (Compiler.cpp)
// ��� ��������� ����������� ��� ��������������� �������.
void NodePreValOp::SetOptimised(bool doOptimisation)
{
	optimised = doOptimisation;
}

void NodePreValOp::doAct()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	// ������� (cmdID+10): ��������� 10, �� ������ ���������� � INC � DEC �� INC_AT � DEC_AT

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(varInfo.count > 1 || shiftAddress)
		first->doAct();

	UINT shiftInStack = 0, sizeOn = 0;
	// ���� ��� ������ ��� ���� ���������� ����, �������� ���� ��� ����� � �����
	if(varInfo.count > 1 || shiftAddress)
		shiftInStack = bitShiftStk;
	// ���� ��� ������, �������� ����, ��� ������� ����������� �� ������� ������
	if(varInfo.count > 1)
		sizeOn = bitSizeOn;

	// ����� ����� ��� ������ ��������� ���������
	UINT addrType = absAddress ? bitAddrAbs : bitAddrRel;

	// ���� �������� ����� �������� �� ������������, ����� �������� �����������:
	// �������� �������� ����� � ����� ����������
	if(optimised)
	{
		// ������ �������� ���������� ����� �� ������
		cmds->AddData((USHORT)(cmdID+10));
		cmds->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
		// ����� ������ �������
		cmds->AddData(varAddress);
		// ���� ��� ������, ����� ������ ������� (� ������) � ����, ��� �������������� ������ �� ��� �������
		if(varInfo.count > 1)	
			cmds->AddData(varInfo.count * varInfo.varType->size);
	}else{
		// ���� ���������� - ������ ��� ���� ���������� ����
		if(varInfo.count > 1 || shiftAddress)
		{
			// ��������� ��� ��������� ����� �������
			// ������ ��� �� ��������� ����� ��������, � �� � ��� ���
			cmds->AddData(cmdCopy);
			cmds->AddData((UCHAR)(OTYPE_INT));
		}
		if(prefixOperator)			// ��� ����������� ��������� ++val/--val
		{
			// ������� �������� ����������, ����� ����� � ���� � ����� ��������

			// ������ �������� ���������� ����� �� ������
			cmds->AddData((USHORT)(cmdID+10));
			cmds->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);

			// �������� ����� �������� ����������
			cmds->AddData(cmdPush);
			cmds->AddData((USHORT)(newST | newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);
		}else{						// ���  ������������ ��������� val++/val--
			// �� �������� ����������, �� � ���� �������� ������ ��������
			
			// �������� �� ��������� �������� ����������
			cmds->AddData(cmdPush);
			cmds->AddData((USHORT)(newST | newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);
			
			// ���� ���������� - ������ ��� ���� ���������� ����
			// ������ � ����� ���������� ����� ����� ������, � ����� �������� ����������
			// ������� �������� �� �������, ��� ��� ��� ��������� ���������� ����� ����� ������ ������ �������
			if(varInfo.count > 1 || shiftAddress)
			{
				cmds->AddData(cmdSwap);
				cmds->AddData((USHORT)(STYPE_INT | (newDT == DTYPE_FLOAT ? DTYPE_DOUBLE : newDT)));
			}
			
			// ������ �������� ���������� ����� �� ������
			cmds->AddData((USHORT)(cmdID+10));
			cmds->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);
		}
	}
}
void NodePreValOp::doLog(ostringstream& ostr)
{
	static char* strs[] = { "++", "--" };
	if(cmdID != cmdInc &&  cmdID != cmdDec)
		throw std::string("ERROR: PreValOp error");
	drawLn(ostr); ostr << *typeInfo << "PreValOp<" << strs[cmdID-cmdInc] << "> :\r\n"; goDown(); if(first) first->doLog(ostr); goUp();
}

UINT NodePreValOp::getSize()
{
	UINT someSize = 0;
	if(first)
		someSize = ConvertToIntegerSize(first, podTypeToStackType[first->getTypeInfo()->type]);
	if(optimised)
	{
		if(varInfo.count > 1)
			return first->getSize() + sizeof(CmdID)+sizeof(USHORT)+2*sizeof(UINT)+someSize;
		else
			return sizeof(CmdID)+sizeof(USHORT)+sizeof(UINT);
	}else{
		if(prefixOperator){
			if(varInfo.count > 1)
				return first->getSize() + 3*sizeof(CmdID)+2*sizeof(USHORT)+sizeof(UCHAR)+4*sizeof(UINT)+someSize;
			else
				return 2*sizeof(CmdID)+2*sizeof(USHORT)+2*sizeof(UINT);
		}else{
			if(varInfo.count > 1)
				return first->getSize() + 4*sizeof(CmdID)+3*sizeof(USHORT)+sizeof(UCHAR)+4*sizeof(UINT)+someSize;
			else
				return 2*sizeof(CmdID)+2*sizeof(USHORT)+2*sizeof(UINT);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� �������� � ����� ����������
NodeTwoAndCmdOp::NodeTwoAndCmdOp(CmdID cmd)
{
	// �������� ��������
	cmdID = cmd;

	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();

	// ����� �������������� ���, ����� ���������� ��������
	typeInfo = ChooseBinaryOpResultType(first->getTypeInfo(), second->getTypeInfo());

	getLog() << __FUNCTION__ << "\r\n";
}
NodeTwoAndCmdOp::~NodeTwoAndCmdOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeTwoAndCmdOp::doAct()
{
	asmStackType fST = podTypeToStackType[first->getTypeInfo()->type], sST = podTypeToStackType[second->getTypeInfo()->type];
	
	// ����� ������ ��������
	first->doAct();
	// �����������, ���� ����, � ���, ������� ���������� ����� ���������� ��������� ��������
	fST = ConvertFirstForSecond(fST, sST);
	// ����� ������ ��������
	second->doAct();
	// �����������, ���� ����, � ���, ������� ���������� ����� ���������� ��������� ��������
	sST = ConvertFirstForSecond(sST, fST);
	// ��������� �������� �� ����������
	cmds->AddData(cmdID);
	cmds->AddData((UCHAR)(operTypeForStackType[fST]));
}
void NodeTwoAndCmdOp::doLog(ostringstream& ostr)
{
	static char* strs[] = { "+", "-", "*", "/", "^", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "bin.and", "bin.or", "bin.xor", "log.and", "log.or", "log.xor"};
	if((cmdID < cmdAdd) || (cmdID > cmdLogXor))
		throw std::string("ERROR: TwoAndCmd error");
	drawLn(ostr); ostr << *typeInfo << "TwoAndCmd<" << strs[cmdID-cmdAdd] << "> :\r\n"; goDown(); first->doLog(ostr); goUp(); goDownB(); second->doLog(ostr); goUp();
}
UINT NodeTwoAndCmdOp::getSize()
{
	asmStackType fST = podTypeToStackType[first->getTypeInfo()->type], sST = podTypeToStackType[second->getTypeInfo()->type];
	UINT resSize = 0;
	resSize += ConvertFirstForSecondSize(fST, sST).first;
	fST = ConvertFirstForSecondSize(fST, sST).second;
	resSize += ConvertFirstForSecondSize(sST, fST).first;
	return NodeTwoOP::getSize() + sizeof(CmdID) + 1 + resSize;
}

//////////////////////////////////////////////////////////////////////////
// ����, ���������� ��� ���������. �������� ��� NodeTwoOP �� ����������� ������ � ���.
// ����� ������? BUG 0002
NodeTwoExpression::NodeTwoExpression()
{
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeTwoExpression::~NodeTwoExpression()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeTwoExpression::doAct()
{
	NodeTwoOP::doAct();
}
void NodeTwoExpression::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "TwoExpression :\r\n";
	goDown();
	first->doLog(ostr);
	goUp();
	goDownB();
	second->doLog(ostr);
	goUp();
}
UINT NodeTwoExpression::getSize()
{
	return NodeTwoOP::getSize();
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ���� if(){}else{} ��� �������� �������� ?:
NodeIfElseExpr::NodeIfElseExpr(bool haveElse, bool isTerm)
{
	// ���� ������� ���� else{}
	if(haveElse)
	{
		third = getList()->back(); getList()->pop_back();
	}
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
	// ���� ��� �������� ��������, �� ������� ��� ���������� �������� �� void
	// ������������� ������ �������, ����� ������ �������������� �������� ����� ������ ����.
	// ������� ���������! BUG 0003
	if(isTerm)
		typeInfo = second->getTypeInfo();
}
NodeIfElseExpr::~NodeIfElseExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeIfElseExpr::doAct()
{
	// ��������� �������� ���������: if(first) second; else third;
	// ������ �������: first ? second : third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->getTypeInfo()->type]];
	// �������� �������
	first->doAct();

	// ���� false, ������� � ���� else ��� ������ �� ���������, ���� ������ ����� �� �������
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(4 + cmds->GetCurrPos() + second->getSize() + (third ? 6 : 0));

	// �������� ���� ��� ��������� ����������� ������� (true)
	second->doAct();
	// ���� ���� ���� else, �������� ���
	if(third)
	{
		// ������ �������� ����� �� ��������� ����� ��� �����, ����� �� ��������� ��� �����
		cmds->AddData(cmdJmp);
		cmds->AddData(4 + cmds->GetCurrPos() + third->getSize());

		// �������� ���� else (false)
		third->doAct();
	}
}
void NodeIfElseExpr::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "IfExpression :\r\n";
	goDown();
	first->doLog(ostr);
	if(!third)
	{
		goUp(); goDownB();
	}
	second->doLog(ostr);
	if(third)
	{
		goUp(); goDownB();
		third->doLog(ostr);
	}
	goUp();
}
UINT NodeIfElseExpr::getSize()
{
	UINT size = first->getSize() + second->getSize() + sizeof(CmdID) + sizeof(UINT) + sizeof(UCHAR);
	if(third)
		size += third->getSize() + sizeof(CmdID) + sizeof(UINT);
	return size;
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ���� for(){}
NodeForExpr::NodeForExpr()
{
	fourth = getList()->back(); getList()->pop_back();
	third = getList()->back(); getList()->pop_back();
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeForExpr::~NodeForExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeForExpr::doAct()
{
	// ��������� �������� ���������: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->getTypeInfo()->type]];

	// �������� �������������
	first->doAct();
	UINT posTestExpr = cmds->GetCurrPos();

	// ����� ��������� �������
	second->doAct();

	// ���� �����, ������ �� �����
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmds->GetCurrPos()+4+third->getSize()+fourth->getSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+third->getSize()+fourth->getSize()+2+4);

	// �������� ���������� �����
	fourth->doAct();
	// �������� ��������, ���������� ����� ������ ��������
	third->doAct();
	// ������� �� �������� �������
	cmds->AddData(cmdJmp);
	cmds->AddData(posTestExpr);
	indTemp.pop_back();
}
void NodeForExpr::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "ForExpression :\r\n";
	goDown();
	first->doLog(ostr);
	second->doLog(ostr);
	third->doLog(ostr);
	goUp(); goDownB(); 
	fourth->doLog(ostr);
	goUp();
}
UINT NodeForExpr::getSize()
{
	return NodeThreeOP::getSize() + fourth->getSize() + 2*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(UCHAR);
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ���� while(){}
NodeWhileExpr::NodeWhileExpr()
{
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeWhileExpr::~NodeWhileExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeWhileExpr::doAct()
{
	// ��������� �������� ���������: while(first) second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->getTypeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	// �������� �������
	first->doAct();
	// ���� ��� �����, ������ �� �����
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmds->GetCurrPos()+4+second->getSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+second->getSize()+2+4);
	// �������� ���������� �����
	second->doAct();
	// ������� �� �������� �������
	cmds->AddData(cmdJmp);
	cmds->AddData(posStart);
}
void NodeWhileExpr::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "WhileExpression :\r\n";
	goDown();
	first->doLog(ostr);
	goUp(); goDownB(); 
	second->doLog(ostr);
	goUp();
}
UINT NodeWhileExpr::getSize()
{
	return first->getSize() + second->getSize() + 2*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(UCHAR);
}

//////////////////////////////////////////////////////////////////////////
// ����, ����������� ���� do{}while()
NodeDoWhileExpr::NodeDoWhileExpr()
{
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeDoWhileExpr::~NodeDoWhileExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeDoWhileExpr::doAct()
{
	// ��������� �������� ���������: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->getTypeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmds->GetCurrPos()+first->getSize()+second->getSize()+2+4);
	// �������� ���������� �����
	first->doAct();
	// �������� �������
	second->doAct();
	// ���� ������� �����, ������� � ���������� ��������� �������� �����
	cmds->AddData(cmdJmpNZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(posStart);
	indTemp.pop_back();
}
void NodeDoWhileExpr::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "DoWhileExpression :\r\n";
	goDown();
	first->doLog(ostr);
	second->doLog(ostr);
	goUp();
}
UINT NodeDoWhileExpr::getSize()
{
	return first->getSize() + second->getSize() + sizeof(CmdID) + sizeof(UINT) + sizeof(UCHAR);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� break;
NodeBreakOp::NodeBreakOp(UINT c)
{
	// ������� �������� ����� ������ �� ����� ������ ����� ���������� (�_�)
	popCnt = c;

	getLog() << __FUNCTION__ << "\r\n";
}
NodeBreakOp::~NodeBreakOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeBreakOp::doAct()
{
	// ����� �������� �� ����� ������ ����� ����������
	for(UINT i = 0; i < popCnt; i++)
		cmds->AddData(cmdPopVTop);
	// ������ �� �����
	cmds->AddData(cmdJmp);
	cmds->AddData(indTemp.back());
}
void NodeBreakOp::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "BreakExpression\r\n";
}
UINT NodeBreakOp::getSize()
{
	return (1+popCnt)*sizeof(CmdID) + sizeof(UINT);
}

//////////////////////////////////////////////////////////////////////////
// ����, ��� ������� ����� case � switch
NodeCaseExpr::NodeCaseExpr()
{
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeCaseExpr::~NodeCaseExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeCaseExpr::doAct()
{
	// ��������� �������� ���������: case first: second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->getTypeInfo()->type]];

	// switch ����� ��������, �� ������� ������������ �����.
	// ��������� ���, ��� ��� ��������� ����� ���� �������� �� �����
	cmds->AddData(cmdCopy);
	// ������� ��������, �� �������� ����������� ������ case
	// �������� ����� ���������� � ��������!
	first->doAct();
	// ������� �� ���������
	cmds->AddData(cmdEqual);
	cmds->AddData((UCHAR)(aOT));
	// ���� �� �����, ������� �� ���� second
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(cmds->GetCurrPos()+4+second->getSize());
	// ����������� ��� �����
	second->doAct();
}
void NodeCaseExpr::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "CaseExpression :\r\n";
	goDown();
	first->doLog(ostr);
	second->doLog(ostr);
	goUp();
}
UINT NodeCaseExpr::getSize()
{
	return first->getSize() + second->getSize() + 3*sizeof(CmdID) + sizeof(UINT) + 2*sizeof(UCHAR);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ ��� ��� switch
NodeSwitchExpr::NodeSwitchExpr()
{
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeSwitchExpr::~NodeSwitchExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeSwitchExpr::doAct()
{
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->getTypeInfo()->type]];

	// �������� ������� ����� ����������
	cmds->AddData(cmdPushVTop);
	// ����� �������� �� �������� ����� �������� ������� ����
	first->doAct();
	// �������� �������� ��� ��������� break;
	indTemp.push_back(cmds->GetCurrPos()+second->getSize()+2);
	// ����������� ��� ��� ���� case'��
	second->doAct();
	// ���������� ������� ����� ��������
	cmds->AddData(cmdPopVTop);
	// ����� � ������� ����� �������� �� �������� ��������� ������� ����
	cmds->AddData(cmdPop);
	cmds->AddData((USHORT)(aOT));
	indTemp.pop_back();
}
void NodeSwitchExpr::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "SwitchExpression :\r\n";
	goDown();
	first->doLog(ostr);
	second->doLog(ostr);
	goUp();
}
UINT NodeSwitchExpr::getSize()
{
	return first->getSize() + second->getSize() + 3*sizeof(CmdID) + sizeof(USHORT);
}
