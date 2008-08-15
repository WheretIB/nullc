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

//class implementation
NodeZeroOP::NodeZeroOP(){ m_typeInfo = typeVoid; }
NodeZeroOP::NodeZeroOP(TypeInfo* tinfo){ m_typeInfo = tinfo; }
NodeZeroOP::~NodeZeroOP(){}

void NodeZeroOP::doAct(){}
void NodeZeroOP::doLog(ostringstream& ostr){ drawLn(ostr); ostr << *m_typeInfo << "ZeroOp\r\n"; }
UINT NodeZeroOP::getSize(){ return 0; }
TypeInfo* NodeZeroOP::typeInfo()
{
	return m_typeInfo;
}

NodeOneOP::NodeOneOP(){ }
NodeOneOP::~NodeOneOP(){ }

void NodeOneOP::doAct(){ first->doAct(); }
void NodeOneOP::doLog(ostringstream& ostr){ drawLn(ostr); ostr << *m_typeInfo << "OneOP :\r\n"; goDown(); first->doLog(ostr); goUp(); }
UINT NodeOneOP::getSize(){ return first->getSize(); }

NodeTwoOP::NodeTwoOP(){ }
NodeTwoOP::~NodeTwoOP(){ }

void NodeTwoOP::doAct(){ NodeOneOP::doAct(); second->doAct(); }
void NodeTwoOP::doLog(ostringstream& ostr){ drawLn(ostr); ostr << *m_typeInfo << "TwoOp :\r\n"; goDown(); first->doLog(ostr); second->doLog(ostr); goUp(); }
UINT NodeTwoOP::getSize(){ return NodeOneOP::getSize() + second->getSize(); }

NodeThreeOP::NodeThreeOP(){ }
NodeThreeOP::~NodeThreeOP(){ }

void NodeThreeOP::doAct(){ NodeTwoOP::doAct(); third->doAct(); }
void NodeThreeOP::doLog(ostringstream& ostr){ drawLn(ostr); ostr << *m_typeInfo << "ThreeOp :\r\n"; goDown(); first->doLog(ostr); second->doLog(ostr); third->doLog(ostr); goUp(); }
UINT NodeThreeOP::getSize(){ return NodeTwoOP::getSize() + third->getSize(); }


NodePopOp::NodePopOp(){ first = getList()->back(); getList()->pop_back();  getLog() << __FUNCTION__ << "\r\n"; }
NodePopOp::~NodePopOp(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodePopOp::doAct()
{
	first->doAct();
	cmds->AddData(cmdPop);
	cmds->AddData((USHORT)(podTypeToStackType[first->typeInfo()->type]));
}
void NodePopOp::doLog(ostringstream& ostr){ drawLn(ostr); ostr << *m_typeInfo << "PopOp :\r\n"; goDownB(); first->doLog(ostr); goUp(); }
UINT NodePopOp::getSize()
{
	return NodeOneOP::getSize() + sizeof(CmdID) + sizeof(USHORT);
}


NodeUnaryOp::NodeUnaryOp(CmdID op)
{
	m_op = op;
	first = getList()->back(); getList()->pop_back();
	m_typeInfo = first->typeInfo();
	getLog() << __FUNCTION__ << "\r\n";
}
NodeUnaryOp::~NodeUnaryOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeUnaryOp::doAct()
{
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->typeInfo()->type]];
	first->doAct();
	cmds->AddData(m_op);
	cmds->AddData((UCHAR)(aOT));
}
void NodeUnaryOp::doLog(ostringstream& ostr){ drawLn(ostr); ostr << *m_typeInfo << "UnaryOp :\r\n"; goDown(); first->doLog(ostr); goUp(); }
UINT NodeUnaryOp::getSize()
{
	return NodeOneOP::getSize() + sizeof(CmdID) + sizeof(UCHAR);
}


NodeReturnOp::NodeReturnOp(UINT c, TypeInfo* tinfo)
{
	m_typeInfo = tinfo;
	first = getList()->back(); getList()->pop_back();
	m_popCnt = c;
	getLog() << __FUNCTION__ << "\r\n";
}
NodeReturnOp::~NodeReturnOp(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeReturnOp::doAct()
{
	first->doAct();
	if(m_typeInfo)
		ConvertFirstToSecond(podTypeToStackType[first->typeInfo()->type], podTypeToStackType[m_typeInfo->type]);
	for(UINT i = 0; i < m_popCnt; i++)
		cmds->AddData(cmdPopVTop);
	cmds->AddData(cmdReturn);
}
void NodeReturnOp::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	if(m_typeInfo)
		ostr << *m_typeInfo << "ReturnOp :\r\n";
	else
		ostr << *first->typeInfo() << "ReturnOp :\r\n";
	goDownB(); first->doLog(ostr); goUp();
}
UINT NodeReturnOp::getSize()
{
	return NodeOneOP::getSize() + sizeof(CmdID) * (1+m_popCnt) + (m_typeInfo ? ConvertFirstToSecondSize(podTypeToStackType[first->typeInfo()->type], podTypeToStackType[m_typeInfo->type]) : 0);
}

NodeExpression::NodeExpression(){ first = getList()->back(); getList()->pop_back();  getLog() << __FUNCTION__ << "\r\n"; }
NodeExpression::~NodeExpression(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeExpression::doAct(){ NodeOneOP::doAct(); }
void NodeExpression::doLog(ostringstream& ostr){ drawLn(ostr); ostr << "Expression :\r\n"; goDownB(); first->doLog(ostr); goUp(); }
UINT NodeExpression::getSize(){ return NodeOneOP::getSize(); }

NodeVarDef::NodeVarDef(UINT sh, std::string nm){ shift = sh; name = nm;  getLog() << __FUNCTION__ << "\r\n"; }
NodeVarDef::~NodeVarDef(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeVarDef::doAct()
{
	if(shift)
	{
		cmds->AddData(cmdPushV);
		cmds->AddData(shift);
	}
}
void NodeVarDef::doLog(ostringstream& ostr){ drawLn(ostr); ostr << "VarDef '" << name << "' " << shift << "\r\n"; }
UINT NodeVarDef::getSize()
{
	return shift ? (sizeof(CmdID) + sizeof(UINT)) : 0;
}

NodeBlock::NodeBlock(){ first = getList()->back(); getList()->pop_back(); getLog() << __FUNCTION__ << "\r\n"; }
NodeBlock::~NodeBlock(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeBlock::doAct()
{
	cmds->AddData(cmdPushVTop);
	NodeOneOP::doAct();
	cmds->AddData(cmdPopVTop);
}
void NodeBlock::doLog(ostringstream& ostr){ drawLn(ostr); ostr << "Block :\r\n"; goDownB(); first->doLog(ostr); goUp(); }
UINT NodeBlock::getSize()
{
	return NodeOneOP::getSize() + 2 * sizeof(CmdID);
}

NodeFuncDef::NodeFuncDef(UINT id){ m_id = id; first = getList()->back(); getList()->pop_back(); getLog() << __FUNCTION__ << "\r\n"; }
NodeFuncDef::~NodeFuncDef(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeFuncDef::doAct()
{
	cmds->AddData(cmdJmp);
	cmds->AddData(cmds->GetCurrPos() + sizeof(CmdID) + sizeof(UINT) + first->getSize());
	(*funcs)[m_id]->address = cmds->GetCurrPos();
	first->doAct();
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

NodeFuncParam::NodeFuncParam(TypeInfo* tinfo)
{
	m_typeInfo = tinfo;
	first = getList()->back(); getList()->pop_back();
}
NodeFuncParam::~NodeFuncParam()
{
}

void NodeFuncParam::doAct()
{
	first->doAct();
	ConvertFirstToSecond(podTypeToStackType[first->typeInfo()->type], podTypeToStackType[m_typeInfo->type]);
}
void NodeFuncParam::doLog(ostringstream& ostr)
{
	drawLn(ostr); ostr << *m_typeInfo << "FuncParam :\r\n"; goDown(); first->doLog(ostr); goUp();
}
UINT NodeFuncParam::getSize()
{
	return first->getSize() + ConvertFirstToSecondSize(podTypeToStackType[first->typeInfo()->type], podTypeToStackType[m_typeInfo->type]);
}

NodeFuncCall::NodeFuncCall(std::string name, UINT id, UINT argCnt, TypeInfo* retType)
{
	m_typeInfo = retType; m_name = name; m_id = id;
	if(argCnt){
		first = getList()->back(); getList()->pop_back();
	}
	if(m_id == -1)
	{
		if(m_name == "clock")
			m_typeInfo = typeInt;
		else
			m_typeInfo = typeDouble;
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodeFuncCall::~NodeFuncCall()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeFuncCall::doAct()
{
	if(first)
		first->doAct();
	if(m_id == -1){
		cmds->AddData(cmdCallStd);
		cmds->AddData((UINT)m_name.length());
		cmds->AddData(m_name.c_str(), m_name.length());
	}else{
		cmds->AddData(cmdCall);
		cmds->AddData((*funcs)[m_id]->address);
	}
}
void NodeFuncCall::doLog(ostringstream& ostr)
{
	drawLn(ostr); ostr << "FuncCall '" << m_name << "' :\r\n"; goDownB(); if(first) first->doLog(ostr); goUp();
}
UINT NodeFuncCall::getSize()
{
	if(first)
		if(m_id == -1)
			return first->getSize() + sizeof(CmdID) + sizeof(UINT) + m_name.length();// + ConvertToRealSize(first, podTypeToStackType[first->typeInfo()->type]);
		else
			return first->getSize() + sizeof(CmdID) + sizeof(UINT);// + ConvertToRealSize(first, podTypeToStackType[first->typeInfo()->type]);
	else
		if(m_id == -1)
			return sizeof(CmdID) + sizeof(UINT) + (UINT)m_name.length();
		else
			return sizeof(CmdID) + sizeof(UINT);
}

NodePushShift::NodePushShift(int varSizeOf)
{
	sizeOfType = varSizeOf;
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodePushShift::~NodePushShift(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodePushShift::doAct()
{
	first->doAct();
	cmds->AddData(cmdCTI);
	cmds->AddData(sizeOfType);
}
void NodePushShift::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *m_typeInfo << "PushShift " << sizeOfType << "\r\n";
	goDown();
	first->doLog(ostr);
	goUp();
}
UINT NodePushShift::getSize()
{
	return first->getSize() + sizeof(CmdID) + sizeof(UINT);
}

// Узел для присвоения значение переменной
NodeVarSet::NodeVarSet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddress, bool shiftAddress, bool arrSetAll, bool absAddress)
{
	// информация о переменной
	m_varInfo = vInfo;
	// и её адрес
	m_varAddress = varAddress;
	// присвоить значение всем элементам массива
	m_arrSetAll = arrSetAll;
	// использовать абсолютную адресацию (для глобальных переменных)
	m_absAddress = absAddress;
	// тип изменяемого значения может быть другим, если переменная составная
	m_typeInfo = targetType;
	// применять динамически расчитываемый сдвиг к адресу переменной
	m_shiftAddress = shiftAddress;

	// получить узел, расчитывающий значение
	first = getList()->back(); getList()->pop_back();

	// если переменная - массив и обновляется одна ячейка
	// или если переменная - член составного типа и нужен сдвиг адреса
	if((m_varInfo.count > 1 && !m_arrSetAll) || m_shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		second = getList()->back(); getList()->pop_back();

		// сдвиг адреса должен быть расчитан в узле типа NodePushShift или быть указаным целым числом
		if(second->getType() != typeNodePushShift && second->getType() != typeNodeNumber)
			throw std::string("ERROR: NodeVarSet() address shift must be prepared by NodePushShift");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
};

NodeVarSet::~NodeVarSet(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeVarSet::doAct()
{
	asmStackType newST = podTypeToStackType[m_typeInfo->type];
	asmDataType newDT = podTypeToDataType[m_typeInfo->type];

	if(m_varInfo.count == 1)	// если это не массив
	{
		if(m_shiftAddress)		// если переменная - член составного типа и нужен сдвиг адреса
		{
			// кладём сдвиг от начала массива (в байтах)
			second->doAct();
		}

		// расчитываем значение для присвоения переменной
		first->doAct();
		// преобразуем в тип переменной
		ConvertFirstToSecond(podTypeToStackType[first->typeInfo()->type], newST);

		// добавляем команду присвоения
		cmds->AddData(cmdMov);
		cmds->AddData((USHORT)(newST | newDT | (m_absAddress ? bitAddrAbs : bitAddrRel) | (m_shiftAddress ? bitShiftStk : 0)));
		cmds->AddData(m_varAddress);
	}else{						// если это массив
		if(m_arrSetAll)			// если указано присвоить значение всем ячейкам массива
		{
			// расчитываем значение для присвоения переменной
			first->doAct();
			// преобразуем в тип ячейки массива
			ConvertFirstToSecond(podTypeToStackType[first->typeInfo()->type], newST);
			// для каждого элемента
			for(UINT n = 0; n < m_varInfo.count; n++)
			{
				// положем в стек сдвиг от начала массива (в байтах)
				cmds->AddData(cmdPush);
				cmds->AddData((USHORT)(STYPE_INT | DTYPE_INT));
				cmds->AddData(n * m_typeInfo->size);
				
				// поменяем местами значения на верхушке стека
				// (для инструкции MOV значение должно следовать за адресом)
				cmds->AddData(cmdSwap);
				cmds->AddData((USHORT)(newST | DTYPE_INT));
				
				// добавляем команду присвоения
				cmds->AddData(cmdMov);
				cmds->AddData((USHORT)(newST | newDT | (m_absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
				cmds->AddData(m_varAddress);
				cmds->AddData(m_varInfo.count * m_typeInfo->size);
			}
		}else{					// если указано присвоить значение одной ячейке массива
			// кладём сдвиг от начала массива (в байтах)
			second->doAct();
			
			// расчитываем значение для присвоения ячейке массива
			first->doAct();
			// преобразуем в тип ячейки массива
			ConvertFirstToSecond(podTypeToStackType[first->typeInfo()->type], newST);
			
			// добавляем команду присвоения
			cmds->AddData(cmdMov);
			cmds->AddData((USHORT)(newST | newDT | (m_absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
			// адрес начала массива
			cmds->AddData(m_varAddress);
			// кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
			cmds->AddData(m_varInfo.count * m_typeInfo->size);
		}
	}
}
void NodeVarSet::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *m_typeInfo << "VarSet " << m_varInfo << " " << m_varAddress << "\r\n";
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
	asmStackType fST =	podTypeToStackType[m_typeInfo->type],
						sST = first ? podTypeToStackType[first->typeInfo()->type] : STYPE_INT,
						tST = second ? podTypeToStackType[second->typeInfo()->type] : STYPE_INT;
	if(m_varInfo.count == 1)
	{
		if(first)
			return (m_shiftAddress ? second->getSize() : 0) + first->getSize() + ConvertFirstToSecondSize(fST, sST) + sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
		else
			return sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	}else{
		if(m_arrSetAll)
		{
			return first->getSize() + ConvertFirstToSecondSize(fST, sST) + m_varInfo.count * (3*sizeof(CmdID)+3*sizeof(USHORT)+3*sizeof(UINT));
		}else{
			return first->getSize() + ConvertFirstToSecondSize(fST, sST) + second->getSize() + /*ConvertToIntegerSize(second, tST) + */sizeof(CmdID)+sizeof(USHORT)+2*sizeof(UINT);
		}
	}
}

NodeVarGet::NodeVarGet(VariableInfo vInfo, UINT adrShift, bool adrAbs)
{
	m_typeInfo = vInfo.varType; m_vpos = vInfo.pos+adrShift; m_name = vInfo.name;
	m_arr = vInfo.count>1; m_size = vInfo.count; m_absadr = adrAbs;
	if(m_arr)
	{
		first = getList()->back(); getList()->pop_back();

		// индекс должен быть расчитан в узле типа NodePushShift
		if(first->getType() != typeNodePushShift)
			throw std::string("ERROR: NodeVarGet() array index must be prepared by NodePushShift");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}

NodeVarGet::~NodeVarGet(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeVarGet::doAct()
{
	asmStackType newST = podTypeToStackType[m_typeInfo->type];
	asmDataType newDT = podTypeToDataType[m_typeInfo->type];
	if(!m_arr) 					// если это не массив
	{
		// получаем значение переменной по адресу
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (m_absadr ? bitAddrAbs : bitAddrRel)));
		// адрес переменной
		cmds->AddData(m_vpos);
	}else{						// если это массив
		// кладём сдвиг от начала массива (в байтах)
		first->doAct();
		
		// получаем значение переменной по адресу
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (m_absadr ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
		// адрес начала массива
		cmds->AddData(m_vpos);
		// кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
		cmds->AddData(m_size * m_typeInfo->size);
	}
}
void NodeVarGet::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << *m_typeInfo << "VarGet (array:" << m_arr << ") '" << m_name << "' " << (int)(m_vpos) << "\r\n";
	goDown();
	if(first)
		first->doLog(ostr);
	goUp();
}
UINT NodeVarGet::getSize()
{
	if(!m_arr)
		return sizeof(CmdID)+sizeof(USHORT) + sizeof(UINT);
	else
		return first->getSize() +sizeof(CmdID)+sizeof(USHORT)+2*sizeof(UINT) + ConvertToIntegerSize(first, podTypeToStackType[first->typeInfo()->type]);
}

NodeVarSetAndOp::NodeVarSetAndOp(TypeInfo* tinfo, UINT vpos, std::string name, bool arr, UINT size, CmdID cmd)
{
	m_typeInfo = tinfo; m_vpos = vpos; m_name = name; m_arr = arr; m_size = size; m_cmd = cmd;
	first = getList()->back(); getList()->pop_back();
	if(arr){
		second = getList()->back(); getList()->pop_back();
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodeVarSetAndOp::~NodeVarSetAndOp(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeVarSetAndOp::doAct()
{
	asmOperType aOT;
	asmStackType meST, firstST, newST;
	asmDataType meDT = podTypeToDataType[m_typeInfo->type];
	meST = podTypeToStackType[m_typeInfo->type];
	firstST = podTypeToStackType[first->typeInfo()->type];

	UINT additFlag = bitAddrRel;
	if(m_arr)
	{
		additFlag = bitAddrRel | bitShiftStk | bitSizeOn;
		second->doAct();
		ConvertToInteger(second, podTypeToStackType[second->typeInfo()->type]);
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
void NodeVarSetAndOp::doLog(ostringstream& ostr){ drawLn(ostr); ostr << *m_typeInfo << "VarSet (array:" << m_arr << ") '" << m_name << "' " << (int)(m_vpos) << "\r\n"; goDown(); first->doLog(ostr); goUp(); goDownB(); if(m_arr) second->doLog(ostr); goUp(); }
UINT NodeVarSetAndOp::getSize()
{
	asmStackType meST, firstST, newST;
	asmDataType meDT = podTypeToDataType[m_typeInfo->type];
	meST = podTypeToStackType[m_typeInfo->type];
	firstST = podTypeToStackType[first->typeInfo()->type];

	UINT resSize = 0;
	if(m_arr)
		resSize += ConvertToIntegerSize(second, podTypeToStackType[second->typeInfo()->type]);
	resSize += ConvertFirstForSecondSize(meST, firstST).first;
	newST = ConvertFirstForSecondSize(meST, firstST).second;
	resSize += ConvertFirstForSecondSize(firstST, newST).first;
	resSize += ConvertFirstToSecondSize(newST, meST);

	if(m_arr)
		return second->getSize() + first->getSize() + 4*sizeof(CmdID)+2*sizeof(USHORT)+2*sizeof(UCHAR)+4*sizeof(UINT) + resSize;
	else
		return first->getSize() + 3*sizeof(CmdID)+2*sizeof(USHORT)+sizeof(UCHAR)+2*sizeof(UINT) + resSize;
}

NodePreValOp::NodePreValOp(TypeInfo* tinfo, UINT vpos, std::string name, bool arr, UINT size, CmdID cmd, bool pre)
{
	m_typeInfo = tinfo; m_vpos = vpos; m_name = name; m_arr = arr; m_size = size; m_cmd = cmd; m_pre = pre; m_optimised = false;
	if(arr){
		first = getList()->back(); getList()->pop_back();
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodePreValOp::~NodePreValOp(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodePreValOp::doAct()
{
	asmStackType newST = podTypeToStackType[m_typeInfo->type];
	asmDataType newDT = podTypeToDataType[m_typeInfo->type];
	if(m_optimised)
	{
		if(m_arr){
			//Calculate array index
			first->doAct();
			//Convert it to integer number
			ConvertToInteger(first, podTypeToStackType[first->typeInfo()->type]);

			//Update variable in place
			cmds->AddData((USHORT)(m_cmd+10));
			cmds->AddData((USHORT)(newDT | bitAddrRel | bitShiftStk | bitSizeOn));
			cmds->AddData(m_vpos);
			cmds->AddData(m_size);
		}else{
			//Update variable in place
			cmds->AddData((USHORT)(m_cmd+10));
			cmds->AddData((USHORT)(newDT | bitAddrRel));
			cmds->AddData(m_vpos);
		}
	}else{
		if(m_pre)
		{
			// ++i
			// First we increment the value, and return new value
			if(m_arr)
			{
				//Calculate array index
				first->doAct();
				//Convert it to integer number
				ConvertToInteger(first, podTypeToStackType[first->typeInfo()->type]);

				//Copy array index for later use, when we will save our new value
				cmds->AddData(cmdCopy);
				cmds->AddData((UCHAR)(OTYPE_INT));
			}
			//Update variable in place
			cmds->AddData((USHORT)(m_cmd+10));
			cmds->AddData((USHORT)(newDT | bitAddrRel | (bitShiftStk | bitSizeOn) * m_arr));
			cmds->AddData(m_vpos);
			if(m_arr)
				cmds->AddData(m_size);

			//Get variable data
			cmds->AddData(cmdPush);
			cmds->AddData((USHORT)(newST | newDT | bitAddrRel | (bitShiftStk | bitSizeOn) * m_arr));
			cmds->AddData(m_vpos);
			if(m_arr)
				cmds->AddData(m_size);
		}else{
			// i++
			// We change value, but return the old one
			if(m_arr)
			{
				//Calculate array index
				first->doAct();
				//Convert it to integer number
				ConvertToInteger(first, podTypeToStackType[first->typeInfo()->type]);

				//Copy array index twice for later use, when we will save our new value
				cmds->AddData(cmdCopy);
				cmds->AddData((UCHAR)(OTYPE_INT));
			}
			// [Stack: ] (not array)   [Stack; index, index;] (array)

			//Get variable data
			//This will be the old value of the variable
			cmds->AddData(cmdPush);
			cmds->AddData((USHORT)(newST | newDT | bitAddrRel | (bitShiftStk | bitSizeOn) * m_arr));
			cmds->AddData(m_vpos);
			if(m_arr)
				cmds->AddData(m_size);
			// [Stack: i;] (not array)   [Stack; index, i;] (array)

			if(m_arr){
				cmds->AddData(cmdSwap);
				cmds->AddData((USHORT)(STYPE_INT | newDT));
			}
			// [Stack: i;] (not array)   [Stack; i, index;] (array)
			
			//Update variable in place
			cmds->AddData((USHORT)(m_cmd+10));
			cmds->AddData((USHORT)(newDT | bitAddrRel | (bitShiftStk | bitSizeOn) * m_arr));
			cmds->AddData(m_vpos);
			if(m_arr)
				cmds->AddData(m_size);
			// [Stack: i;] (not array)   [Stack; i;] (array)
		}
	}

}
void NodePreValOp::doLog(ostringstream& ostr){
	static char* strs[] = { "++", "--" };
	if(m_cmd != cmdInc &&  m_cmd != cmdDec)
		throw std::string("ERROR: PreValOp error");
	drawLn(ostr); ostr << *m_typeInfo << "PreValOp<" << strs[m_cmd-cmdInc] << "> :\r\n"; goDown(); if(first) first->doLog(ostr); goUp();
}

UINT NodePreValOp::getSize()
{
	UINT someSize = 0;
	if(first)
		someSize = ConvertToIntegerSize(first, podTypeToStackType[first->typeInfo()->type]);
	if(m_optimised)
	{
		if(m_arr)
			return first->getSize() + sizeof(CmdID)+sizeof(USHORT)+2*sizeof(UINT)+someSize;
		else
			return sizeof(CmdID)+sizeof(USHORT)+sizeof(UINT);
	}else{
		if(m_pre){
			if(m_arr)
				return first->getSize() + 3*sizeof(CmdID)+2*sizeof(USHORT)+sizeof(UCHAR)+4*sizeof(UINT)+someSize;
			else
				return 2*sizeof(CmdID)+2*sizeof(USHORT)+2*sizeof(UINT);
		}else{
			if(m_arr)
				return first->getSize() + 4*sizeof(CmdID)+3*sizeof(USHORT)+sizeof(UCHAR)+4*sizeof(UINT)+someSize;
			else
				return 2*sizeof(CmdID)+2*sizeof(USHORT)+2*sizeof(UINT);
		}
	}
}

NodeTwoAndCmdOp::NodeTwoAndCmdOp(CmdID cmd)
{
	m_cmd = cmd;
	second = getList()->back();
	getList()->pop_back();
	first = getList()->back();
	getList()->pop_back();
	m_typeInfo = ChooseBinaryOpResultType(first->typeInfo(), second->typeInfo());
	getLog() << __FUNCTION__ << "\r\n";
}
NodeTwoAndCmdOp::~NodeTwoAndCmdOp(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeTwoAndCmdOp::doAct()
{
	asmStackType fST = podTypeToStackType[first->typeInfo()->type], sST = podTypeToStackType[second->typeInfo()->type];
	first->doAct();
	fST = ConvertFirstForSecond(fST, sST);
	second->doAct();
	sST = ConvertFirstForSecond(sST, fST);
	cmds->AddData(m_cmd);
	cmds->AddData((UCHAR)(operTypeForStackType[fST]));
}
void NodeTwoAndCmdOp::doLog(ostringstream& ostr)
{
	static char* strs[] = { "+", "-", "*", "/", "^", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "bin.and", "bin.or", "bin.xor", "log.and", "log.or", "log.xor"};
	if((m_cmd < cmdAdd) || (m_cmd > cmdLogXor))
		throw std::string("ERROR: TwoAndCmd error");
	drawLn(ostr); ostr << *m_typeInfo << "TwoAndCmd<" << strs[m_cmd-cmdAdd] << "> :\r\n"; goDown(); first->doLog(ostr); goUp(); goDownB(); second->doLog(ostr); goUp();
}
UINT NodeTwoAndCmdOp::getSize()
{
	asmStackType fST = podTypeToStackType[first->typeInfo()->type], sST = podTypeToStackType[second->typeInfo()->type];
	UINT resSize = 0;
	resSize += ConvertFirstForSecondSize(fST, sST).first;
	fST = ConvertFirstForSecondSize(fST, sST).second;
	resSize += ConvertFirstForSecondSize(sST, fST).first;
	return NodeTwoOP::getSize() + sizeof(CmdID) + 1 + resSize;
}

NodeTwoExpression::NodeTwoExpression(){ second = getList()->back(); getList()->pop_back(); first = getList()->back(); getList()->pop_back(); getLog() << __FUNCTION__ << "\r\n"; }
NodeTwoExpression::~NodeTwoExpression(){ getLog() << __FUNCTION__ << "\r\n"; }

void NodeTwoExpression::doAct(){ NodeTwoOP::doAct(); }
void NodeTwoExpression::doLog(ostringstream& ostr){ drawLn(ostr); ostr << "TwoExpression :\r\n"; goDown(); first->doLog(ostr); goUp(); goDownB(); second->doLog(ostr); goUp(); }
UINT NodeTwoExpression::getSize(){ return NodeTwoOP::getSize(); }

NodeIfElseExpr::NodeIfElseExpr(bool haveElse, bool isTerm)
{
	if(haveElse){
		third = getList()->back(); getList()->pop_back();
	}
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
	if(isTerm)
		m_typeInfo = second->typeInfo();
}
NodeIfElseExpr::~NodeIfElseExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeIfElseExpr::doAct()
{
	//Structure: if(first) second; else third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->typeInfo()->type]];
	first->doAct();

	//If false, jump to beginning of third
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(4 + cmds->GetCurrPos() + second->getSize() + (third ? 6 : 0));

	second->doAct();
	if(third)
	{
		cmds->AddData(cmdJmp);
		cmds->AddData(4 + cmds->GetCurrPos() + third->getSize());

		third->doAct();
	}
}
void NodeIfElseExpr::doLog(ostringstream& ostr)
{
	drawLn(ostr);
	ostr << "IfExpression :\r\n";
	goDown();
	first->doLog(ostr);
	if(!third){
		goUp(); goDownB();
	}
	second->doLog(ostr);
	if(third){
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
	//for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->typeInfo()->type]];

	first->doAct();
	UINT posTestExpr = cmds->GetCurrPos();

	second->doAct();
	//If false, exit the cycle
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	indTemp.push_back(cmds->GetCurrPos()+4+third->getSize()+fourth->getSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+third->getSize()+fourth->getSize()+2+4);

	fourth->doAct();
	third->doAct();
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
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->typeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	first->doAct();
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	indTemp.push_back(cmds->GetCurrPos()+4+second->getSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+second->getSize()+2+4);
	second->doAct();
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
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->typeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	indTemp.push_back(cmds->GetCurrPos()+first->getSize()+second->getSize()+2+4);
	first->doAct();
	second->doAct();
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

NodeBreakOp::NodeBreakOp(UINT c)
{
	m_popCnt = c;
	getLog() << __FUNCTION__ << "\r\n";
}
NodeBreakOp::~NodeBreakOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeBreakOp::doAct()
{
	for(UINT i = 0; i < m_popCnt; i++)
		cmds->AddData(cmdPopVTop);
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
	return (1+m_popCnt)*sizeof(CmdID) + sizeof(UINT);
}

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
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->typeInfo()->type]];

	cmds->AddData(cmdCopy);
	first->doAct();
	cmds->AddData(cmdEqual);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(cmds->GetCurrPos()+4+second->getSize());
	//cmds->AddData(cmdPop);
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
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->typeInfo()->type]];

	cmds->AddData(cmdPushVTop);
	first->doAct();
	indTemp.push_back(cmds->GetCurrPos()+second->getSize()+2);
	second->doAct();
	cmds->AddData(cmdPopVTop);
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
