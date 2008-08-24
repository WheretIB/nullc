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

void NodeZeroOP::Compile()
{
}
void NodeZeroOP::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
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
	drawLn(ostr);
	ostr << *typeInfo << "OneOP :\r\n";
	goDown();
	first->LogToStream(ostr);
	goUp();
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
	drawLn(ostr);
	ostr << *typeInfo << "TwoOp :\r\n"; goDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	goUp();
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
	drawLn(ostr);
	ostr << *typeInfo << "ThreeOp :\r\n";
	goDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	third->LogToStream(ostr);
	goUp();
}
UINT NodeThreeOP::GetSize()
{
	return NodeTwoOP::GetSize() + third->GetSize();
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

void NodePopOp::Compile()
{
	// ��� ��������� ���� ��������� ��������
	first->Compile();
	// ������� ��� � ������� �����
	cmds->AddData(cmdPop);
	cmds->AddData((USHORT)(podTypeToStackType[first->GetTypeInfo()->type]));
}
void NodePopOp::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "PopOp :\r\n";
	goDownB();
	first->LogToStream(ostr);
	goUp();
}
UINT NodePopOp::GetSize()
{
	return NodeOneOP::GetSize() + sizeof(CmdID) + sizeof(USHORT);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� ������� �������� ��� ��������� �� ������� �����
NodeUnaryOp::NodeUnaryOp(CmdID cmd)
{
	// ������� ��������
	cmdID = cmd;

	first = getList()->back(); getList()->pop_back();
	// ��� ���������� ����� ��, ��� ��������
	typeInfo = first->GetTypeInfo();

	getLog() << __FUNCTION__ << "\r\n";
}
NodeUnaryOp::~NodeUnaryOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeUnaryOp::Compile()
{
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	// ��� ��������� ���� ��������� ��������
	first->Compile();
	// �������� �������
	cmds->AddData(cmdID);
	cmds->AddData((UCHAR)(aOT));
}
void NodeUnaryOp::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "UnaryOp :\r\n";
	goDown();
	first->LogToStream(ostr);
	goUp();
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

	first = getList()->back(); getList()->pop_back();
	
	getLog() << __FUNCTION__ << "\r\n";
}
NodeReturnOp::~NodeReturnOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeReturnOp::Compile()
{
	// ����� ��������, ������� ����� ����������
	first->Compile();
	// ����������� ��� � ��� ����������� �������� �������
	if(typeInfo)
		ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]);
	// ����� �������� �� ����� ������ ����� ����������
	//for(UINT i = 0; i < popCnt; i++)
	//	cmds->AddData(cmdPopVTop);

	// ������ �� ������� ��� ���������
	cmds->AddData(cmdReturn);
	cmds->AddData((UCHAR)(operTypeForStackType[podTypeToStackType[typeInfo ? typeInfo->type : first->GetTypeInfo()->type]]));
	cmds->AddData((UINT)(popCnt));
}
void NodeReturnOp::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	if(typeInfo)
		ostr << *typeInfo << "ReturnOp :\r\n";
	else
		ostr << *first->GetTypeInfo() << "ReturnOp :\r\n";
	goDownB(); first->LogToStream(ostr); goUp();
}
UINT NodeReturnOp::GetSize()
{
	return NodeOneOP::GetSize() + sizeof(CmdID) + sizeof(UINT) + 1 + (typeInfo ? ConvertFirstToSecondSize(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]) : 0);
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

void NodeExpression::Compile()
{
	NodeOneOP::Compile();
}
void NodeExpression::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "Expression :\r\n";
	goDownB();
	first->LogToStream(ostr);
	goUp();
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
	getLog() << __FUNCTION__ << "\r\n";
}
NodeVarDef::~NodeVarDef()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarDef::Compile()
{
	// ���� ����� �� ����� ����
	if(shift)
	{
		// ������� ������� ����� ����������
		cmds->AddData(cmdPushV);
		cmds->AddData(shift);
	}
}
void NodeVarDef::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
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
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeBlock::~NodeBlock()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeBlock::Compile()
{
	// �������� �������� ������� ����� ����������
	cmds->AddData(cmdPushVTop);
	// �������� ���������� ����� (�� �� ��� first->Compile())
	NodeOneOP::Compile();
	// ���������� �������� ������� ����� ����������
	cmds->AddData(cmdPopVTop);
}
void NodeBlock::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "Block :\r\n";
	goDownB();
	first->LogToStream(ostr);
	goUp();
}
UINT NodeBlock::GetSize()
{
	return NodeOneOP::GetSize() + 2 * sizeof(CmdID);
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

void NodeFuncDef::Compile()
{
	// ����� ���������� ������� ������� ������� �� � �����
	// ��� ������� ����� ���� ������ � ����� � ���������� ������� ���������, � ��� ���� ����������
	cmds->AddData(cmdJmp);
	cmds->AddData(cmds->GetCurrPos() + sizeof(CmdID) + 2*sizeof(UINT) + 1 + first->GetSize());
	(*funcs)[funcID]->address = cmds->GetCurrPos();
	// ����������� ��� �������
	first->Compile();
	// ������� ������� �� �������, ���� ������������ ����� (�� ������ ��� �� ��� �����, ������� �� ������)
	cmds->AddData(cmdReturn);
	cmds->AddData((UCHAR)(operTypeForStackType[podTypeToStackType[(*funcs)[funcID]->retType->type]]));
	cmds->AddData((UINT)(1));
}
void NodeFuncDef::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "FuncDef :\r\n";
	goDownB(); first->LogToStream(ostr); goUp();
}
UINT NodeFuncDef::GetSize()
{
	return first->GetSize() + 2*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(UCHAR);
}

//////////////////////////////////////////////////////////////////////////
// ����, ������������ �������� ������� ���������� ����� ������� �������
NodeFuncParam::NodeFuncParam(TypeInfo* tinfo, int paramIndex)
{
	// ���, ������� ������� �������
	typeInfo = tinfo;
	// ����� ���������
	idParam = paramIndex;

	first = getList()->back(); getList()->pop_back();
}
NodeFuncParam::~NodeFuncParam()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeFuncParam::Compile()
{
	if(idParam == 1)
		cmds->AddData(cmdProlog);//cmds->AddData(cmdPushVTop);
	// ��������� ��������
	first->Compile();
	// ����������� ��� � ��� �������� ��������� �������
	ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]);
}
void NodeFuncParam::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "FuncParam :\r\n";
	goDown();
	first->LogToStream(ostr);
	goUp();
}
UINT NodeFuncParam::GetSize()
{
	return (idParam == 1 ? sizeof(CmdID) : 0) + first->GetSize() + ConvertFirstToSecondSize(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]);
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

void NodeFuncCall::Compile()
{
	// ���� ������� ���������, ����� �� ��������
	if(first)
		first->Compile();
	if(funcID == -1)		// ���� ������� ����������
	{
		// ������� �� �����
		cmds->AddData(cmdCallStd);
		cmds->AddData((UINT)funcName.length());
		cmds->AddData(funcName.c_str(), funcName.length());
	}else{					// ���� ������� ���������� �������������
		// �������� � ��������� ��������� ����� ���, �����
		cmds->AddData(cmdProlog);
		cmds->AddData(cmdPushVTop);

		// ����, ������� �������� ��� ����������
		UINT allSize=0;
		for(UINT i = 0; i < (*funcs)[funcID]->params.size(); i++)
			allSize += (*funcs)[funcID]->params[i].varType->size;

		// �������� ���� ���������� �� ��� ��������
		cmds->AddData(cmdPushV);
		cmds->AddData(allSize);

		UINT addr = 0;
		for(int i = int((*funcs)[funcID]->params.size())-1; i >= 0; i--)
		{
			asmStackType newST = podTypeToStackType[(*funcs)[funcID]->params[i].varType->type];
			asmDataType newDT = podTypeToDataType[(*funcs)[funcID]->params[i].varType->type];
			cmds->AddData(cmdMov);
			cmds->AddData((USHORT)(newST | newDT | bitAddrRel));
			// ����� ������ �������
			cmds->AddData(addr);
			addr += (*funcs)[funcID]->params[i].varType->size;

			cmds->AddData(cmdPop);
			cmds->AddData((USHORT)(newST));
		}
		//cmds->AddData(cmdPopVTop);
		
		// ������� �� ������
		cmds->AddData(cmdCall);
		cmds->AddData((*funcs)[funcID]->address);
	}
}
void NodeFuncCall::LogToStream(ostringstream& ostr)
{
	drawLn(ostr); ostr << "FuncCall '" << funcName << "' :\r\n"; goDownB(); if(first) first->LogToStream(ostr); goUp();
}
UINT NodeFuncCall::GetSize()
{
	UINT size = 0;
	if(first)
		size += first->GetSize();

	if(funcID == -1)
		size += sizeof(CmdID) + sizeof(UINT) + (UINT)funcName.length();
	else
		size += 4*sizeof(CmdID) + 2*sizeof(UINT) + (UINT)((*funcs)[funcID]->params.size()) * (2*sizeof(CmdID)+2+4+2);
	
	/*if(first)
		if(funcID == -1)
			return first->GetSize() + sizeof(CmdID) + sizeof(UINT) + funcName.length();// + ConvertToRealSize(first, podTypeToStackType[first->typeInfo()->type]);
		else
			return first->GetSize() + sizeof(CmdID) + sizeof(UINT);// + ConvertToRealSize(first, podTypeToStackType[first->typeInfo()->type]);
	else*/
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

	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodePushShift::~NodePushShift()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodePushShift::Compile()
{
	asmOperType oAsmType = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// �������� ������
	first->Compile();
	// �������� ��� � ����� �����
	cmds->AddData(cmdCTI);
	// ��������� ��� ��������
	cmds->AddData((UCHAR)(oAsmType));
	// ������� �� ������ �������� (����� ������ ���� � ������)
	cmds->AddData(sizeOfType);
}
void NodePushShift::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "PushShift " << sizeOfType << "\r\n";
	goDown();
	first->LogToStream(ostr);
	goUp();
}
UINT NodePushShift::GetSize()
{
	return first->GetSize() + sizeof(CmdID) + sizeof(UCHAR) + sizeof(UINT);
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
		if(second->GetTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarSet() address shift must be an integer number");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
};

NodeVarSet::~NodeVarSet()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarSet::Compile()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	if(varInfo.count == 1)	// ���� ��� �� ������
	{
		if(shiftAddress)		// ���� ���������� - ���� ���������� ���� � ����� ����� ������
		{
			// ����� ����� � ���� (� ������)
			second->Compile();
		}

		// ����������� �������� ��� ���������� ����������
		first->Compile();
		// ����������� � ��� ����������
		ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);

		// ��������� ������� ����������
		cmds->AddData(cmdMov);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (shiftAddress ? bitShiftStk : 0)));
		cmds->AddData(varAddress);
	}else{						// ���� ��� ������
		if(arrSetAll)			// ���� ������� ��������� �������� ���� ������� �������
		{
			// ����������� �������� ��� ���������� ����������
			first->Compile();
			// ����������� � ��� ������ �������
			ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);
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
			second->Compile();
			
			// ����������� �������� ��� ���������� ������ �������
			first->Compile();
			// ����������� � ��� ������ �������
			ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);
			
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
void NodeVarSet::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "VarSet " << varInfo << " " << varAddress << "\r\n";
	goDown();
	if(first)
		first->LogToStream(ostr);
	goUp();
	goDownB();
	if(second)
		second->LogToStream(ostr);
	goUp();
}
UINT NodeVarSet::GetSize()
{
	asmStackType fST =	podTypeToStackType[typeInfo->type];
	asmStackType sST = first ? podTypeToStackType[first->GetTypeInfo()->type] : STYPE_INT;
	asmStackType tST = second ? podTypeToStackType[second->GetTypeInfo()->type] : STYPE_INT;

	UINT size = 0;
	if(varInfo.count == 1)
	{
		if(shiftAddress)
			size += second->GetSize();
		size += first->GetSize();
		size += ConvertFirstToSecondSize(sST, fST);
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	}else{
		if(arrSetAll)
		{
			size += first->GetSize();
			size += ConvertFirstToSecondSize(sST, fST);
			size += varInfo.count * (3 * sizeof(CmdID) + 3 * sizeof(USHORT) + 3 * sizeof(UINT));
		}else{
			size += second->GetSize();
			size += first->GetSize();
			size += ConvertFirstToSecondSize(sST, fST);
			size += sizeof(CmdID) + sizeof(USHORT) + 2 * sizeof(UINT);
		}
	}
	return size;
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
		if(first->GetTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}

NodeVarGet::~NodeVarGet()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarGet::Compile()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];
	if(varInfo.count > 1) 	// ���� ��� ������
	{
		// ����� ����� �� ������ ������� (� ������)
		first->Compile();

		// �������� �������� ���������� �� ������
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
		// ����� ������ �������
		cmds->AddData(varAddress);
		// ����� ������ ������� (� ������) � ����, ��� �������������� ������ �� ��� �������
		cmds->AddData(varInfo.count * varInfo.varType->size);
	}else{						// ���� �� ��� ������
		if(shiftAddress)		// ���� ���������� - ���� ���������� ���� � ����� ����� ������
			first->Compile();		// ����� ��� � ���� (� ������)

		// �������� �������� ���������� �� ������
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (shiftAddress ? bitShiftStk : 0)));
		// ����� ����������
		cmds->AddData(varAddress);
	}
}
void NodeVarGet::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "VarGet (array:" << (varInfo.count > 1) << ") '" << varInfo.name << "' " << (int)(varAddress) << "\r\n";
	goDown();
	if(first)
		first->LogToStream(ostr);
	goUp();
}
UINT NodeVarGet::GetSize()
{
	UINT size = 0;
	if(varInfo.count > 1)
	{
		size += first->GetSize();
		size += sizeof(CmdID) + sizeof(USHORT) + 2 * sizeof(UINT);
	}else{
		if(shiftAddress)
			size += first->GetSize();
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	}
	return size;
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

	first = getList()->back(); getList()->pop_back();

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(varInfo.count > 1 || shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		second = getList()->back(); getList()->pop_back();

		// ����� ������ ������ ����  ����� ������
		if(second->GetTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodeVarSetAndOp::~NodeVarSetAndOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarSetAndOp::Compile()
{
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
	if(varInfo.count > 1 || shiftAddress)
	{
		second->Compile();
		// ����� ��� ��������, � ������ ����� �����
		cmds->AddData(cmdCopy);
		cmds->AddData((UCHAR)(OTYPE_INT));
	}

	UINT shiftInStack = 0, sizeOn = 0;
	// ���� ��� ������ ��� ���� ���������� ����, �������� ���� ��� ����� � �����
	if(varInfo.count > 1 || shiftAddress)
		shiftInStack = bitShiftStk;
	// ���� ��� ������, �������� ����, ��� ������� ����������� �� ������� ������
	if(varInfo.count > 1)
		sizeOn = bitSizeOn;

	// ����� ����� ��� ������ ��������� ���������
	UINT addrType = absAddress ? bitAddrAbs : bitAddrRel;

	// �������� �������� ���������� � ����
	cmds->AddData(cmdPush);
	cmds->AddData((USHORT)(thisST | thisDT | addrType | shiftInStack | sizeOn));
	cmds->AddData(varAddress);
	if(varInfo.count > 1)
		cmds->AddData(varInfo.count * varInfo.varType->size);

	// ����������� � ��� ���������� �������� ��������
	resultST = ConvertFirstForSecond(thisST, firstST);

	// ������ �������� ��� ��������
	aOT = operTypeForStackType[resultST];

	// ����� �������� ������� ��������
	first->Compile();
	// ����������� � ��� ���������� �������� ��������
	firstST = ConvertFirstForSecond(firstST, thisST);

	// �������� �������� ��������
	cmds->AddData(cmdID);
	cmds->AddData((UCHAR)(aOT));

	// ����������� �������� � ����� � ��� ����������
	ConvertFirstToSecond(resultST, thisST);

	// �������� ����� �������� � ���������
	cmds->AddData(cmdMov);
	cmds->AddData((USHORT)(thisST | thisDT | addrType | shiftInStack | sizeOn));
	cmds->AddData(varAddress);
	if(varInfo.count > 1)
		cmds->AddData(varInfo.count * varInfo.varType->size);
}
void NodeVarSetAndOp::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *typeInfo << "VarSet (array:" << (varInfo.count > 1) << ") '" << varInfo.name << "' " << (int)(varAddress) << "\r\n";
	goDown();
	first->LogToStream(ostr);
	goUp();
	goDownB();
	if(varInfo.count > 1 || shiftAddress)
		second->LogToStream(ostr);
	goUp();
}
UINT NodeVarSetAndOp::GetSize()
{
	asmStackType thisST = podTypeToStackType[typeInfo->type];
	asmStackType firstST = podTypeToStackType[first->GetTypeInfo()->type];
	asmStackType resultST = ConvertFirstForSecondSize(thisST, firstST).second;
	asmDataType thisDT = podTypeToDataType[typeInfo->type];

	UINT size = 0;
	if(varInfo.count > 1 || shiftAddress)
	{
		size += second->GetSize();
		size += sizeof(CmdID) + sizeof(UCHAR);
	}
	if(varInfo.count > 1)
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

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(varInfo.count > 1 || shiftAddress)	
	{
		// �������� ����, ������������� ����� ������
		first = getList()->back(); getList()->pop_back();

		// ����� ������ ������ ����  ����� ������
		if(first->GetTypeInfo() != typeInt)
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

void NodePreValOp::Compile()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	// ������� (cmdID+10): ��������� 10, �� ������ ���������� � INC � DEC �� INC_AT � DEC_AT

	// ���� ���������� - ������ ��� ���� ���������� ����, �� ����� ����� ������
	if(varInfo.count > 1 || shiftAddress)
		first->Compile();

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
		cmds->AddData((CmdID)(cmdID+10));
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
			cmds->AddData((CmdID)(cmdID+10));
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
			cmds->AddData((CmdID)(cmdID+10));
			cmds->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);
		}
	}
}
void NodePreValOp::LogToStream(ostringstream& ostr)
{
	static char* strs[] = { "++", "--" };
	if(cmdID != cmdInc &&  cmdID != cmdDec)
		throw std::string("ERROR: PreValOp error");
	drawLn(ostr); ostr << *typeInfo << "PreValOp<" << strs[cmdID-cmdInc] << "> :\r\n"; goDown(); if(first) first->LogToStream(ostr); goUp();
}

UINT NodePreValOp::GetSize()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	UINT size = 0;
	if(varInfo.count > 1 || shiftAddress)
		size += first->GetSize();
	if(optimised)
	{
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
		if(varInfo.count > 1)
			size += sizeof(UINT);
	}else{
		if(varInfo.count > 1 || shiftAddress)
			size += sizeof(CmdID) + sizeof(UCHAR);
		if(prefixOperator)
		{
			size += 2 * (sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT));
			if(varInfo.count > 1)
				size += 2 * sizeof(UINT);
		}else{
			if(varInfo.count > 1)
				size += 2 * sizeof(UINT);
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if(varInfo.count > 1 || shiftAddress)
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

	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();

	// ����� �������������� ���, ����� ���������� ��������
	typeInfo = ChooseBinaryOpResultType(first->GetTypeInfo(), second->GetTypeInfo());

	getLog() << __FUNCTION__ << "\r\n";
}
NodeTwoAndCmdOp::~NodeTwoAndCmdOp()
{
	getLog() << __FUNCTION__ << "\r\n";
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
	cmds->AddData(cmdID);
	cmds->AddData((UCHAR)(operTypeForStackType[fST]));
}
void NodeTwoAndCmdOp::LogToStream(ostringstream& ostr)
{
	static char* strs[] = { "+", "-", "*", "/", "^", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "bin.and", "bin.or", "bin.xor", "log.and", "log.or", "log.xor"};
	if((cmdID < cmdAdd) || (cmdID > cmdLogXor))
		throw std::string("ERROR: TwoAndCmd error");
	drawLn(ostr); ostr << *typeInfo << "TwoAndCmd<" << strs[cmdID-cmdAdd] << "> :\r\n"; goDown(); first->LogToStream(ostr); goUp(); goDownB(); second->LogToStream(ostr); goUp();
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

void NodeTwoExpression::Compile()
{
	NodeTwoOP::Compile();
}
void NodeTwoExpression::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "TwoExpression :\r\n";
	goDown();
	first->LogToStream(ostr);
	goUp();
	goDownB();
	second->LogToStream(ostr);
	goUp();
}
UINT NodeTwoExpression::GetSize()
{
	return NodeTwoOP::GetSize();
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
		typeInfo = second->GetTypeInfo();
}
NodeIfElseExpr::~NodeIfElseExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeIfElseExpr::Compile()
{
	// ��������� �������� ���������: if(first) second; else third;
	// ������ �������: first ? second : third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// �������� �������
	first->Compile();

	// ���� false, ������� � ���� else ��� ������ �� ���������, ���� ������ ����� �� �������
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(4 + cmds->GetCurrPos() + second->GetSize() + (third ? 6 : 0));

	// �������� ���� ��� ��������� ����������� ������� (true)
	second->Compile();
	// ���� ���� ���� else, �������� ���
	if(third)
	{
		// ������ �������� ����� �� ��������� ����� ��� �����, ����� �� ��������� ��� �����
		cmds->AddData(cmdJmp);
		cmds->AddData(4 + cmds->GetCurrPos() + third->GetSize());

		// �������� ���� else (false)
		third->Compile();
	}
}
void NodeIfElseExpr::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "IfExpression :\r\n";
	goDown();
	first->LogToStream(ostr);
	if(!third)
	{
		goUp(); goDownB();
	}
	second->LogToStream(ostr);
	if(third)
	{
		goUp(); goDownB();
		third->LogToStream(ostr);
	}
	goUp();
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

void NodeForExpr::Compile()
{
	// ��������� �������� ���������: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// �������� �������������
	first->Compile();
	UINT posTestExpr = cmds->GetCurrPos();

	// ����� ��������� �������
	second->Compile();

	// ���� �����, ������ �� �����
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmds->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);

	// �������� ���������� �����
	fourth->Compile();
	// �������� ��������, ���������� ����� ������ ��������
	third->Compile();
	// ������� �� �������� �������
	cmds->AddData(cmdJmp);
	cmds->AddData(posTestExpr);
	indTemp.pop_back();
}
void NodeForExpr::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "ForExpression :\r\n";
	goDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	third->LogToStream(ostr);
	goUp(); goDownB(); 
	fourth->LogToStream(ostr);
	goUp();
}
UINT NodeForExpr::GetSize()
{
	return NodeThreeOP::GetSize() + fourth->GetSize() + 2*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(UCHAR);
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

void NodeWhileExpr::Compile()
{
	// ��������� �������� ���������: while(first) second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	// �������� �������
	first->Compile();
	// ���� ��� �����, ������ �� �����
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmds->GetCurrPos()+4+second->GetSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+second->GetSize()+2+4);
	// �������� ���������� �����
	second->Compile();
	// ������� �� �������� �������
	cmds->AddData(cmdJmp);
	cmds->AddData(posStart);
}
void NodeWhileExpr::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "WhileExpression :\r\n";
	goDown();
	first->LogToStream(ostr);
	goUp(); goDownB(); 
	second->LogToStream(ostr);
	goUp();
}
UINT NodeWhileExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + 2*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(UCHAR);
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

void NodeDoWhileExpr::Compile()
{
	// ��������� �������� ���������: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	// �������� ����� ��� ������ �� ����� ���������� break;
	indTemp.push_back(cmds->GetCurrPos()+first->GetSize()+second->GetSize()+2+4);
	// �������� ���������� �����
	first->Compile();
	// �������� �������
	second->Compile();
	// ���� ������� �����, ������� � ���������� ��������� �������� �����
	cmds->AddData(cmdJmpNZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(posStart);
	indTemp.pop_back();
}
void NodeDoWhileExpr::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "DoWhileExpression :\r\n";
	goDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	goUp();
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

	getLog() << __FUNCTION__ << "\r\n";
}
NodeBreakOp::~NodeBreakOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeBreakOp::Compile()
{
	// ����� �������� �� ����� ������ ����� ����������
	for(UINT i = 0; i < popCnt; i++)
		cmds->AddData(cmdPopVTop);
	// ������ �� �����
	cmds->AddData(cmdJmp);
	cmds->AddData(indTemp.back());
}
void NodeBreakOp::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "BreakExpression\r\n";
}
UINT NodeBreakOp::GetSize()
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

void NodeCaseExpr::Compile()
{
	// ��������� �������� ���������: case first: second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	// switch ����� ��������, �� ������� ������������ �����.
	// ��������� ���, ��� ��� ��������� ����� ���� �������� �� �����
	cmds->AddData(cmdCopy);
	// ������� ��������, �� �������� ����������� ������ case
	// �������� ����� ���������� � ��������!
	first->Compile();
	// ������� �� ���������
	cmds->AddData(cmdEqual);
	cmds->AddData((UCHAR)(aOT));
	// ���� �� �����, ������� �� ���� second
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(cmds->GetCurrPos()+4+second->GetSize());
	// ����������� ��� �����
	second->Compile();
}
void NodeCaseExpr::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "CaseExpression :\r\n";
	goDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	goUp();
}
UINT NodeCaseExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + 3*sizeof(CmdID) + sizeof(UINT) + 2*sizeof(UCHAR);
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

void NodeSwitchExpr::Compile()
{
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// �������� ������� ����� ����������
	cmds->AddData(cmdPushVTop);
	// ����� �������� �� �������� ����� �������� ������� ����
	first->Compile();
	// �������� �������� ��� ��������� break;
	indTemp.push_back(cmds->GetCurrPos()+second->GetSize()+2);
	// ����������� ��� ��� ���� case'��
	second->Compile();
	// ���������� ������� ����� ��������
	cmds->AddData(cmdPopVTop);
	// ����� � ������� ����� �������� �� �������� ��������� ������� ����
	cmds->AddData(cmdPop);
	cmds->AddData((USHORT)(aOT));
	indTemp.pop_back();
}
void NodeSwitchExpr::LogToStream(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "SwitchExpression :\r\n";
	goDown();
	first->LogToStream(ostr);
	second->LogToStream(ostr);
	goUp();
}
UINT NodeSwitchExpr::GetSize()
{
	return first->GetSize() + second->GetSize() + 3*sizeof(CmdID) + sizeof(USHORT);
}
