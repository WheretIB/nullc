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
// Узел не имеющий дочерних узлов
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
// Узел, имеющий один дочерний узел
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
// Узел, имеющий два дочерних узла
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
// Узел, имеющий три дочерних подузла
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
// Узел, убирающий с вершины стека значение, оставленное дочерним узлом
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
	// Даём дочернему узлу вычеслить значение
	first->doAct();
	// Убираем его с вершины стека
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
// Узел, производящий выбраную унарную операцию над значением на вершине стека
NodeUnaryOp::NodeUnaryOp(CmdID cmd)
{
	// Унарная операция
	cmdID = cmd;

	first = getList()->back(); getList()->pop_back();
	// Тип результата такой же, как исходный
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

	// Даём дочернему узлу вычеслить значение
	first->doAct();
	// Выполним команду
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
// Узел, выполняющий возврат из функции или из программы
NodeReturnOp::NodeReturnOp(UINT c, TypeInfo* tinfo)
{
	// Сколько значений нужно убрать со стека вершин стека переменных (о_О)
	popCnt = c;
	// Тип результата предоставлен извне
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
	// Найдём значение, которое будем возвращать
	first->doAct();
	// Преобразуем его в тип возвратного значения функции
	if(typeInfo)
		ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], podTypeToStackType[typeInfo->type]);
	// Уберём значения со стека вершин стека переменных
	for(UINT i = 0; i < popCnt; i++)
		cmds->AddData(cmdPopVTop);
	// Выйдем из функции или программы
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
// Узел, содержащий выражение. Работает как NodeOneOP за исключением записи в лог.
// Стоит убрать? BUG 0001
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
// Узел, создающий место для новых переменных
NodeVarDef::NodeVarDef(UINT sh, std::string nm)
{
	// Сдвиг вершины стека переменных
	shift = sh;
	// Имя переменной
	name = nm;
	getLog() << __FUNCTION__ << "\r\n";
}
NodeVarDef::~NodeVarDef()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarDef::doAct()
{
	// Если сдвиг не равен нулю
	if(shift)
	{
		// Сдвинем вершину стека переменных
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
// Узел c содержимым блока {}
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
	// Сохраним значение вершины стека переменных
	cmds->AddData(cmdPushVTop);
	// Выполним содержимое блока (то же что first->doAct())
	NodeOneOP::doAct();
	// Востановим значение вершины стека переменных
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
	// Номер функции
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
	// Перед содержимым функции сделаем переход за её конец
	// Код функций может быть смешан с кодом в глобальной области видимости, и его надо пропускать
	cmds->AddData(cmdJmp);
	cmds->AddData(cmds->GetCurrPos() + sizeof(CmdID) + sizeof(UINT) + first->getSize());
	(*funcs)[funcID]->address = cmds->GetCurrPos();
	// Сгенерируем код функции
	first->doAct();
	// Добавим возврат из функции, если пользователь забыл (но ругать его всё ещё стоит, главное не падать)
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
// Узел, определяющий значение входных параметров перед вызовом функции
NodeFuncParam::NodeFuncParam(TypeInfo* tinfo)
{
	// Тип, который ожидает функция
	typeInfo = tinfo;

	first = getList()->back(); getList()->pop_back();
}
NodeFuncParam::~NodeFuncParam()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeFuncParam::doAct()
{
	// Определим значение
	first->doAct();
	// Преобразуем его в тип входного параметра функции
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
// Узел, производящий вызов функции
NodeFuncCall::NodeFuncCall(std::string name, UINT id, UINT argCnt, TypeInfo* retType)
{
	// Тип результата - тип возвратного значения функции
	typeInfo = retType;
	// Имя функции
	funcName = name;
	// Идентификатор функции
	funcID = id;

	// Если функция принимает параметры, следует взять узел, который определяет их значения
	if(argCnt)
	{
		first = getList()->back(); getList()->pop_back();
	}
	// Если функция не имеет идентификатора, значит она встроенная
	if(id == -1)
	{
		// clock() - единственная функция возвращающая int, а не double
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
	// Если имеются параметры, найдём их значения
	if(first)
		first->doAct();
	if(funcID == -1)		// Если функция встроенная
	{
		// Вызовем по имени
		cmds->AddData(cmdCallStd);
		cmds->AddData((UINT)funcName.length());
		cmds->AddData(funcName.c_str(), funcName.length());
	}else{					// Если функция определена пользователем
		// Вызовем по адресу
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
// Узел, вычисляющий величину сдвига от начала массива
NodePushShift::NodePushShift(int varSizeOf)
{
	// Возвращаем int
	typeInfo = typeInt;
	// Запомним размер типа
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
	// Вычислим индекс
	first->doAct();
	// Переведём его в целое число
	cmds->AddData(cmdCTI);
	// Умножив на размер элемента (сдвиг должен быть в байтах)
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
// Узел для присвоения значения переменной
NodeVarSet::NodeVarSet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool arraySetAll, bool absAddr)
{
	// информация о переменной
	varInfo = vInfo;
	// и её адрес
	varAddress = varAddr;
	// присвоить значение всем элементам массива
	arrSetAll = arraySetAll;
	// использовать абсолютную адресацию (для глобальных переменных)
	absAddress = absAddr;
	// тип изменяемого значения может быть другим, если переменная составная
	typeInfo = targetType;
	// применять динамически расчитываемый сдвиг к адресу переменной
	shiftAddress = shiftAddr;

	// получить узел, расчитывающий значение
	first = getList()->back(); getList()->pop_back();

	// если переменная - массив и обновляется одна ячейка
	// или если переменная - член составного типа и нужен сдвиг адреса
	if((varInfo.count > 1 && !arrSetAll) || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		second = getList()->back(); getList()->pop_back();

		// сдвиг адреса должен быть  целым числом
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

	if(varInfo.count == 1)	// если это не массив
	{
		if(shiftAddress)		// если переменная - член составного типа и нужен сдвиг адреса
		{
			// кладём сдвиг в стек (в байтах)
			second->doAct();
		}

		// расчитываем значение для присвоения переменной
		first->doAct();
		// преобразуем в тип переменной
		ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], newST);

		// добавляем команду присвоения
		cmds->AddData(cmdMov);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (shiftAddress ? bitShiftStk : 0)));
		cmds->AddData(varAddress);
	}else{						// если это массив
		if(arrSetAll)			// если указано присвоить значение всем ячейкам массива
		{
			// расчитываем значение для присвоения переменной
			first->doAct();
			// преобразуем в тип ячейки массива
			ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], newST);
			// для каждого элемента
			for(UINT n = 0; n < varInfo.count; n++)
			{
				// положем в стек сдвиг от начала массива (в байтах)
				cmds->AddData(cmdPush);
				cmds->AddData((USHORT)(STYPE_INT | DTYPE_INT));
				cmds->AddData(n * typeInfo->size);
				
				// поменяем местами значения на верхушке стека
				// (для инструкции MOV значение должно следовать за адресом)
				cmds->AddData(cmdSwap);
				cmds->AddData((USHORT)(newST | DTYPE_INT));
				
				// добавляем команду присвоения
				cmds->AddData(cmdMov);
				cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
				cmds->AddData(varAddress);
				cmds->AddData(varInfo.count * typeInfo->size);
			}
		}else{					// если указано присвоить значение одной ячейке массива
			// кладём сдвиг от начала массива (в байтах)
			second->doAct();
			
			// расчитываем значение для присвоения ячейке массива
			first->doAct();
			// преобразуем в тип ячейки массива
			ConvertFirstToSecond(podTypeToStackType[first->getTypeInfo()->type], newST);
			
			// добавляем команду присвоения
			cmds->AddData(cmdMov);
			cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
			// адрес начала массива
			cmds->AddData(varAddress);
			// кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
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
// Узел для получения значения переменной
NodeVarGet::NodeVarGet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool absAddr)
{
	// информация о переменной
	varInfo = vInfo;
	// и её адрес
	varAddress = varAddr;
	// использовать абсолютную адресацию (для глобальных переменных)
	absAddress = absAddr;
	// тип изменяемого значения может быть другим, если переменная составная
	typeInfo = targetType;
	// применять динамически расчитываемый сдвиг к адресу переменной
	shiftAddress = shiftAddr;

	// если переменная - массив или член составного типа, то нужен сдвиг адреса
	if(varInfo.count > 1 || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		first = getList()->back(); getList()->pop_back();

		// сдвиг адреса должен быть  целым числом
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
	if(varInfo.count > 1) 	// если это массив
	{
		// кладём сдвиг от начала массива (в байтах)
		first->doAct();

		// получаем значение переменной по адресу
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | bitShiftStk | bitSizeOn));
		// адрес начала массива
		cmds->AddData(varAddress);
		// кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
		cmds->AddData(varInfo.count * varInfo.varType->size);
	}else{						// если не это массив
		if(shiftAddress)		// если переменная - член составного типа и нужен сдвиг адреса
			first->doAct();		// кладём его в стек (в байтах)

		// получаем значение переменной по адресу
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (shiftAddress ? bitShiftStk : 0)));
		// адрес переменной
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
// Узел для изменения значения переменной
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
// Узел для инкремента или декремента значения переменной
NodePreValOp::NodePreValOp(VariableInfo vInfo, TypeInfo* targetType, UINT varAddress, bool shiftAddress, bool absAddress, CmdID cmd, bool preOp)
{
	// информация о переменной
	varInfo = vInfo;
	// и её адрес
	varAddress = varAddress;
	// тип изменяемого значения может быть другим, если переменная составная
	typeInfo = targetType;
	// применять динамически расчитываемый сдвиг к адресу переменной
	shiftAddress = shiftAddress;
	// использовать абсолютную адресацию (для глобальных переменных)
	absAddress = absAddress;
	// команду, которую применить к значению (DEC или INC)
	cmdID = cmd;
	// префиксный или постфиксный оператор
	prefixOperator = preOp;
	// если изменённое значение не используется, можно применить оптимизацию.
	optimised = false;	// по умолчанию выключено

	// если переменная - массив или член составного типа, то нужен сдвиг адреса
	if(varInfo.count > 1 || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		first = getList()->back(); getList()->pop_back();

		// сдвиг адреса должен быть  целым числом
		if(first->getTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodePreValOp::~NodePreValOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodePreValOp::doAct()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];
	if(optimised)
	{
		if(varInfo.count > 1){
			//Calculate array index
			first->doAct();
			//Convert it to integer number
			ConvertToInteger(first, podTypeToStackType[first->getTypeInfo()->type]);

			//Update variable in place
			cmds->AddData((USHORT)(cmdID+10));
			cmds->AddData((USHORT)(newDT | bitAddrRel | bitShiftStk | bitSizeOn));
			cmds->AddData(varAddress);
			cmds->AddData(varInfo.count * varInfo.varType->size);
		}else{
			//Update variable in place
			cmds->AddData((USHORT)(cmdID+10));
			cmds->AddData((USHORT)(newDT | bitAddrRel));
			cmds->AddData(varAddress);
		}
	}else{
		if(prefixOperator)
		{
			// ++i
			// First we increment the value, and return new value
			if(varInfo.count > 1)
			{
				//Calculate array index
				first->doAct();
				//Convert it to integer number
				ConvertToInteger(first, podTypeToStackType[first->getTypeInfo()->type]);

				//Copy array index for later use, when we will save our new value
				cmds->AddData(cmdCopy);
				cmds->AddData((UCHAR)(OTYPE_INT));
			}
			//Update variable in place
			cmds->AddData((USHORT)(cmdID+10));
			cmds->AddData((USHORT)(newDT | bitAddrRel | (bitShiftStk | bitSizeOn) * (varInfo.count > 1)));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);

			//Get variable data
			cmds->AddData(cmdPush);
			cmds->AddData((USHORT)(newST | newDT | bitAddrRel | (bitShiftStk | bitSizeOn) * (varInfo.count > 1)));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);
		}else{
			// i++
			// We change value, but return the old one
			if(varInfo.count > 1)
			{
				//Calculate array index
				first->doAct();
				//Convert it to integer number
				ConvertToInteger(first, podTypeToStackType[first->getTypeInfo()->type]);

				//Copy array index twice for later use, when we will save our new value
				cmds->AddData(cmdCopy);
				cmds->AddData((UCHAR)(OTYPE_INT));
			}
			// [Stack: ] (not array)   [Stack; index, index;] (array)

			//Get variable data
			//This will be the old value of the variable
			cmds->AddData(cmdPush);
			cmds->AddData((USHORT)(newST | newDT | bitAddrRel | (bitShiftStk | bitSizeOn) * (varInfo.count > 1)));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);
			// [Stack: i;] (not array)   [Stack; index, i;] (array)

			if(varInfo.count > 1){
				cmds->AddData(cmdSwap);
				cmds->AddData((USHORT)(STYPE_INT | newDT));
			}
			// [Stack: i;] (not array)   [Stack; i, index;] (array)
			
			//Update variable in place
			cmds->AddData((USHORT)(cmdID+10));
			cmds->AddData((USHORT)(newDT | bitAddrRel | (bitShiftStk | bitSizeOn) * (varInfo.count > 1)));
			cmds->AddData(varAddress);
			if(varInfo.count > 1)
				cmds->AddData(varInfo.count * varInfo.varType->size);
			// [Stack: i;] (not array)   [Stack; i;] (array)
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
// Узел, производящий бинарную операцию с двумя значениями
NodeTwoAndCmdOp::NodeTwoAndCmdOp(CmdID cmd)
{
	// Бинарная операция
	cmdID = cmd;

	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();

	// Найдём результирующий тип, после проведения операции
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
	
	// Найдём первое значение
	first->doAct();
	// Преобразуем, если надо, в тип, который получается после проведения выбранной операции
	fST = ConvertFirstForSecond(fST, sST);
	// Найдём второе значение
	second->doAct();
	// Преобразуем, если надо, в тип, который получается после проведения выбранной операции
	sST = ConvertFirstForSecond(sST, fST);
	// Произведём операцию со значениями
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
// Узел, содержащий два выражения. Работает как NodeTwoOP за исключением записи в лог.
// Стоит убрать? BUG 0002
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
// Узел, выполняющий блок if(){}else{} или условный оператор ?:
NodeIfElseExpr::NodeIfElseExpr(bool haveElse, bool isTerm)
{
	// Если имеется блок else{}
	if(haveElse)
	{
		third = getList()->back(); getList()->pop_back();
	}
	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();
	getLog() << __FUNCTION__ << "\r\n";
	// Если это условный оператор, то имеется тип результата отличный от void
	// Потенциальная ошибка имеется, когда разные результирующие варианты имеют разные типы.
	// Следует исправить! BUG 0003
	if(isTerm)
		typeInfo = second->getTypeInfo();
}
NodeIfElseExpr::~NodeIfElseExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeIfElseExpr::doAct()
{
	// Структура дочерних элементов: if(first) second; else third;
	// Второй вариант: first ? second : third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->getTypeInfo()->type]];
	// Вычислим условие
	first->doAct();

	// Если false, перейдём в блок else или выйдем из оператора, если такого блока не имеется
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(4 + cmds->GetCurrPos() + second->getSize() + (third ? 6 : 0));

	// Выполним блок для успешного прохождения условия (true)
	second->doAct();
	// Если есть блок else, выполним его
	if(third)
	{
		// Только поставим выход из оператора перед его кодом, чтобы не выполнять обе ветви
		cmds->AddData(cmdJmp);
		cmds->AddData(4 + cmds->GetCurrPos() + third->getSize());

		// Выполним блок else (false)
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
// Узел, выполняющий блок for(){}
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
	// Структура дочерних элементов: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->getTypeInfo()->type]];

	// Выполним инициализацию
	first->doAct();
	UINT posTestExpr = cmds->GetCurrPos();

	// Найдём результат условия
	second->doAct();

	// Если ложно, выйдем из цикла
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmds->GetCurrPos()+4+third->getSize()+fourth->getSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+third->getSize()+fourth->getSize()+2+4);

	// Выполним содержимое цикла
	fourth->doAct();
	// Выполним операцию, проводимую после каждой итерации
	third->doAct();
	// Перейдём на проверку условия
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
// Узел, выполняющий блок while(){}
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
	// Структура дочерних элементов: while(first) second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->getTypeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	// Выполним условие
	first->doAct();
	// Если оно ложно, выйдем из цикла
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmds->GetCurrPos()+4+second->getSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+second->getSize()+2+4);
	// Выполним содержимое цикла
	second->doAct();
	// Перейдём на проверку условия
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
// Узел, выполняющий блок do{}while()
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
	// Структура дочерних элементов: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->getTypeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmds->GetCurrPos()+first->getSize()+second->getSize()+2+4);
	// Выполним содержимое цикла
	first->doAct();
	// Выполним условие
	second->doAct();
	// Если условие верно, перейдём к выполнению следующей итерации цикла
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
// Узел, производящий операцию break;
NodeBreakOp::NodeBreakOp(UINT c)
{
	// Сколько значений нужно убрать со стека вершин стека переменных (о_О)
	popCnt = c;

	getLog() << __FUNCTION__ << "\r\n";
}
NodeBreakOp::~NodeBreakOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeBreakOp::doAct()
{
	// Уберём значения со стека вершин стека переменных
	for(UINT i = 0; i < popCnt; i++)
		cmds->AddData(cmdPopVTop);
	// Выйдем из цикла
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
// Узел, для каждого блока case в switch
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
	// Структура дочерних элементов: case first: second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->getTypeInfo()->type]];

	// switch нашёл значение, по которым производится выбор.
	// Скопируем его, так как сравнение уберёт одно значение со стека
	cmds->AddData(cmdCopy);
	// Положим значение, по которому срабатывает данный case
	// Значение может вычислятся в рантайме!
	first->doAct();
	// Сравним на равенство
	cmds->AddData(cmdEqual);
	cmds->AddData((UCHAR)(aOT));
	// Если не равны, перейдём за блок second
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(cmds->GetCurrPos()+4+second->getSize());
	// Сгенерируем код блока
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
// Узел, определяющий код для switch
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

	// Сохраним вершину стека переменных
	cmds->AddData(cmdPushVTop);
	// Найдём значение по которому будем выбирать вариант кода
	first->doAct();
	// Сохраним значение для оператора break;
	indTemp.push_back(cmds->GetCurrPos()+second->getSize()+2);
	// Сгенерируем код для всех case'ов
	second->doAct();
	// Востановим вершину стека значений
	cmds->AddData(cmdPopVTop);
	// Уберём с вершины стека значение по которому выбирался вариант кода
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
