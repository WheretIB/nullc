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

static char* binCommandToText[] = { "+", "-", "*", "/", "^", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "bin.and", "bin.or", "bin.xor", "log.and", "log.or", "log.xor"};

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

void NodeZeroOP::SetCodeInfo(const char* start, const char* end)
{
	strBegin = start;
	strEnd = end;
}
//////////////////////////////////////////////////////////////////////////
// Узел, имеющий один дочерний узел
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
// Узел, имеющий два дочерних узла
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
// Узел, имеющий три дочерних узла
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

void NodePopOp::Compile()
{
	if(strBegin && strEnd)
		cmds->AddDescription(cmds->GetCurrPos(), strBegin, strEnd);

	// Даём дочернему узлу вычислить значение
	first->Compile();
	// Убираем его с вершины стека
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
// Узел, производящий выбраную унарную операцию над значением на вершине стека
NodeUnaryOp::NodeUnaryOp(CmdID cmd)
{
	// Унарная операция
	cmdID = cmd;

	first = getList()->back(); getList()->pop_back();
	// Тип результата такой же, как исходный
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

	// Даём дочернему узлу вычислить значение
	first->Compile();
	// Выполним команду
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

void NodeReturnOp::Compile()
{
	if(strBegin && strEnd)
		cmds->AddDescription(cmds->GetCurrPos(), strBegin, strEnd);

	// Найдём значение, которое будем возвращать
	first->Compile();
	// Преобразуем его в тип возвратного значения функции
	if(typeInfo)
		ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]);

	// Выйдем из функции или программы
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

void NodeVarDef::Compile()
{
	if(strBegin && strEnd)
		cmds->AddDescription(cmds->GetCurrPos(), strBegin, strEnd);

	// Если сдвиг не равен нулю
	if(shift)
	{
		// Сдвинем вершину стека переменных
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

void NodeBlock::Compile()
{
	// Сохраним значение вершины стека переменных
	cmds->AddData(cmdPushVTop);
	// Выполним содержимое блока (то же что first->Compile())
	NodeOneOP::Compile();
	// Востановим значение вершины стека переменных
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
	// Номер функции
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
	// Перед содержимым функции сделаем переход за её конец
	// Код функций может быть смешан с кодом в глобальной области видимости, и его надо пропускать
	cmds->AddData(cmdJmp);
	cmds->AddData(cmds->GetCurrPos() + sizeof(CmdID) + 2*sizeof(UINT) + 1 + first->GetSize());
	(*funcs)[funcID]->address = cmds->GetCurrPos();
	// Сгенерируем код функции
	first->Compile();
	// Добавим возврат из функции, если пользователь забыл (но ругать его всё ещё стоит, главное не падать)
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
// Узел, определяющий значение входных параметров перед вызовом функции
NodeFuncParam::NodeFuncParam(TypeInfo* tinfo, int paramIndex, bool funcStd)
{
	// Тип, который ожидает функция
	typeInfo = tinfo;
	// Номер параметра
	idParam = paramIndex;
	// Стандартная ли функция
	stdFunction = funcStd;

	first = getList()->back(); getList()->pop_back();
}
NodeFuncParam::~NodeFuncParam()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeFuncParam::Compile()
{
	if(idParam == 1 && !stdFunction)
	{
		cmds->AddData(cmdProlog);
		cmds->AddData((UCHAR)(1));
	}
	// Определим значение
	first->Compile();
	// Преобразуем его в тип входного параметра функции
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
	return ((idParam == 1  && !stdFunction) ? sizeof(CmdID)+1 : 0) + first->GetSize() + ConvertFirstToSecondSize(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]);
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

void NodeFuncCall::Compile()
{
	// Если имеются параметры, найдём их значения
	if(first)
		first->Compile();
	if(funcID == -1)		// Если функция встроенная
	{
		// Вызовем по имени
		cmds->AddData(cmdCallStd);
		cmds->AddData((UINT)funcName.length());
		cmds->AddData(funcName.c_str(), funcName.length());
	}else{					// Если функция определена пользователем
		// Перенесём в локальные параметры прямо тут, фигле
		cmds->AddData(cmdProlog);
		cmds->AddData((UCHAR)(2));

		cmds->AddData(cmdPushVTop);

		// Надём, сколько занимают все переменные
		UINT allSize=0;
		for(UINT i = 0; i < (*funcs)[funcID]->params.size(); i++)
			allSize += (*funcs)[funcID]->params[i].varType->size;

		// Расширим стек переменные на это значение
		cmds->AddData(cmdPushV);
		cmds->AddData(allSize);

		UINT addr = 0;
		for(int i = int((*funcs)[funcID]->params.size())-1; i >= 0; i--)
		{
			asmStackType newST = podTypeToStackType[(*funcs)[funcID]->params[i].varType->type];
			asmDataType newDT = podTypeToDataType[(*funcs)[funcID]->params[i].varType->type];
			cmds->AddData(cmdMov);
			cmds->AddData((USHORT)(newST | newDT | bitAddrRel));
			// адрес начала массива
			cmds->AddData(addr);
			addr += (*funcs)[funcID]->params[i].varType->size;

			cmds->AddData(cmdPop);
			cmds->AddData((USHORT)(newST));
		}
		//cmds->AddData(cmdPopVTop);
		
		// Вызовем по адресу
		cmds->AddData(cmdCall);
		cmds->AddData((*funcs)[funcID]->address);
		cmds->AddData((UINT)(typeInfo->size));
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
		size += 4*sizeof(CmdID) + 1 + 3*sizeof(UINT) + (UINT)((*funcs)[funcID]->params.size()) * (2*sizeof(CmdID)+2+4+2);

	return size;
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

void NodePushShift::Compile()
{
	asmOperType oAsmType = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// Вычислим индекс
	first->Compile();
	// Переведём его в целое число
	cmds->AddData(cmdCTI);
	// Передадим тип операнда
	cmds->AddData((UCHAR)(oAsmType));
	// Умножив на размер элемента (сдвиг должен быть в байтах)
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
// Узел для присвоения значения переменной
NodeVarSet::NodeVarSet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool arraySetAll, bool absAddr, UINT pushBytes)
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
	// на сколько байт следует увеличить стек переменных
	bytesToPush = pushBytes;

	// сдвиг уже прибавлен к адресу
	bakedShift = false;

	// получить узел, расчитывающий значение
	first = getList()->back(); getList()->pop_back();
	if(typeInfo->type == TypeInfo::NOT_POD)
	{
		if(first->GetTypeInfo()->type != TypeInfo::NOT_POD)
			throw std::string("ERROR: Cannot convert " + first->GetTypeInfo()->name + " to " + typeInfo->name);
		if(first->GetTypeInfo()->type == TypeInfo::NOT_POD && first->GetTypeInfo()->name != typeInfo->name)
			throw std::string("ERROR: Cannot convert " + first->GetTypeInfo()->name + " to " + typeInfo->name);
	}

	// если переменная - массив и обновляется одна ячейка
	// или если переменная - член составного типа и нужен сдвиг адреса
	if((varInfo.count > 1 && !arrSetAll) || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		second = getList()->back(); getList()->pop_back();

		// сдвиг адреса должен быть целым числом
		if(second->GetTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarSet() address shift must be an integer number");

		if(second->GetNodeType() == typeNodeNumber)
		{
			if(varInfo.count > 1 && static_cast<NodeNumber<UINT>* >(second.get())->GetVal() > varInfo.count * varInfo.varType->size)
				throw std::string("ERROR: array index out of range");
			varAddress += static_cast<NodeNumber<int>* >(second.get())->GetVal();
			bakedShift = true;
		}
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

	if(strBegin && strEnd)
		cmds->AddDescription(cmds->GetCurrPos(), strBegin, strEnd);

	if(bytesToPush)
	{
		cmds->AddData(cmdPushV);
		cmds->AddData(bytesToPush);
	}
	if(varInfo.count == 1)	// если это не массив
	{
		// расчитываем значение для присвоения переменной
		first->Compile();
		// преобразуем в тип переменной
		ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);

		if(shiftAddress && !bakedShift)		// если переменная - член составного типа и нужен сдвиг адреса
		{
			// кладём сдвиг в стек (в байтах)
			second->Compile();
		}

		// добавляем команду присвоения
		cmds->AddData(cmdMov);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | ((shiftAddress && !bakedShift) ? bitShiftStk : 0)));
		cmds->AddData(varAddress);
	}else{						// если это массив
		if(arrSetAll)			// если указано присвоить значение всем ячейкам массива
		{
			// расчитываем значение для присвоения переменной
			first->Compile();
			// преобразуем в тип ячейки массива
			ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);
			// Установим значение массиву
			cmds->AddData(cmdSetRange);
			cmds->AddData((USHORT)(newDT));
			cmds->AddData(varAddress);
			cmds->AddData(varInfo.count);
		}else{					// если указано присвоить значение одной ячейке массива
			// расчитываем значение для присвоения ячейке массива
			first->Compile();
			// преобразуем в тип ячейки массива
			ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);

			// кладём сдвиг от начала массива (в байтах)
			if(!bakedShift)
				second->Compile();
			
			// добавляем команду присвоения
			cmds->AddData(cmdMov);
			cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (bakedShift ? 0 : (bitShiftStk | bitSizeOn))));
			// адрес начала массива
			cmds->AddData(varAddress);
			// кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
			if(!bakedShift)
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
	if(bytesToPush)
		size += sizeof(CmdID) + sizeof(UINT);
	if(varInfo.count == 1 && !bakedShift)
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
			size += sizeof(CmdID) + sizeof(USHORT) + 2 * sizeof(UINT);
		}else{
			if(!bakedShift)
				size += second->GetSize();
			size += first->GetSize();
			size += ConvertFirstToSecondSize(sST, fST);
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if(!bakedShift)
				size += sizeof(UINT);
		}
	}
	return size;
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

	// сдвиг уже прибавлен к адресу
	bakedShift = false;

	// если переменная - массив или член составного типа, то нужен сдвиг адреса
	if(varInfo.count > 1 || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		first = getList()->back(); getList()->pop_back();

		// сдвиг адреса должен быть  целым числом
		if(first->GetTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");

		if(first->GetNodeType() == typeNodeNumber)
		{
			if(varInfo.count > 1 && static_cast<NodeNumber<UINT>* >(first.get())->GetVal() > varInfo.count * varInfo.varType->size)
				throw std::string("ERROR: array index out of range (overflow)");
			varAddress += static_cast<NodeNumber<int>* >(first.get())->GetVal();
			bakedShift = true;
		}
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
	if(varInfo.count > 1) 	// если это массив
	{
		// кладём сдвиг от начала массива (в байтах)
		if(!bakedShift)
			first->Compile();

		// получаем значение переменной по адресу
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (bakedShift ? 0 : (bitShiftStk | bitSizeOn))));
		// адрес начала массива
		cmds->AddData(varAddress);
		// кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
		if(!bakedShift)
			cmds->AddData(varInfo.count * varInfo.varType->size);
	}else{						// если не это массив
		if(shiftAddress && !bakedShift)		// если переменная - член составного типа и нужен сдвиг адреса
			first->Compile();		// кладём его в стек (в байтах)

		// получаем значение переменной по адресу
		cmds->AddData(cmdPush);
		cmds->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | ((shiftAddress && !bakedShift) ? bitShiftStk : 0)));
		// адрес переменной
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
		if(!bakedShift)
			size += first->GetSize();
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
		if(!bakedShift)
			size += sizeof(UINT);
	}else{
		if(shiftAddress && !bakedShift)
			size += first->GetSize();
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////
// Узел для изменения значения переменной (a += b, и т.п.)
NodeVarSetAndOp::NodeVarSetAndOp(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool absAddr, CmdID cmd)
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
	// команда, выполняемая с двумя операндами
	cmdID = cmd;

	// сдвиг уже прибавлен к адресу
	bakedShift = false;

	first = getList()->back(); getList()->pop_back();

	// На данный момент операции с композитными типами отсутствуют
	if(typeInfo->type == TypeInfo::NOT_POD)
		throw std::string("ERROR: Operation " + std::string(binCommandToText[cmdID - cmdAdd]) + "= is not supported on " + typeInfo->name + " and " + first->GetTypeInfo()->name);

	// если переменная - массив или член составного типа, то нужен сдвиг адреса
	if(varInfo.count > 1 || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		second = getList()->back(); getList()->pop_back();

		// сдвиг адреса должен быть  целым числом
		if(second->GetTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");

		if(second->GetNodeType() == typeNodeNumber)
		{
			if(varInfo.count > 1 && static_cast<NodeNumber<UINT>* >(second.get())->GetVal() > varInfo.count * varInfo.varType->size)
				throw std::string("ERROR: array index out of range");
			varAddress += static_cast<NodeNumber<int>* >(second.get())->GetVal();
			bakedShift = true;
		}
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodeVarSetAndOp::~NodeVarSetAndOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeVarSetAndOp::Compile()
{
	if(strBegin && strEnd)
		cmds->AddDescription(cmds->GetCurrPos(), strBegin, strEnd);

	// Тип нашей переменной в основном стеке
	asmStackType thisST = podTypeToStackType[typeInfo->type];
	// Тип второго значения в основном стеке
	asmStackType firstST = podTypeToStackType[first->GetTypeInfo()->type];
	// Cтековый тип, который получится после выполнения бинарной операции
	asmStackType resultST;// = ConvertFirstForSecond(thisST, firstST);

	// Тип нашей переменной в стеке переменных
	asmDataType thisDT = podTypeToDataType[typeInfo->type];

	// Тип, над которым работает операция
	asmOperType aOT;// = operTypeForStackType[resultST];

	// Если переменная - массив или член составного типа, то нужен сдвиг адреса
	if((varInfo.count > 1 || shiftAddress) && !bakedShift)
		second->Compile();

	UINT shiftInStack = 0, sizeOn = 0;
	// Если это массив или член составного типа, включаем флаг что сдвиг в стеке
	if((varInfo.count > 1 || shiftAddress) && !bakedShift)
		shiftInStack = bitShiftStk;
	// Если это массив, включаем флаг, что имеется ограничение по размеру сдвига
	if((varInfo.count > 1) && !bakedShift)
		sizeOn = bitSizeOn;

	// Выбор флага для разных вариантов адресации
	UINT addrType = absAddress ? bitAddrAbs : bitAddrRel;

	// Поместим значение переменной в стек
	cmds->AddData(cmdPush);
	cmds->AddData((USHORT)(thisST | thisDT | addrType | shiftInStack | sizeOn));
	cmds->AddData(varAddress);
	if((varInfo.count > 1) && !bakedShift)
		cmds->AddData(varInfo.count * varInfo.varType->size);

	// Преобразуем в тип результата бинарной операции
	resultST = ConvertFirstForSecond(thisST, firstST);

	// Теперь известем тип операции
	aOT = operTypeForStackType[resultST];

	// Найдём значение второго операнда
	first->Compile();
	// Преобразуем в тип результата бинарной операции
	firstST = ConvertFirstForSecond(firstST, thisST);

	// Выполним бинарную операцию
	cmds->AddData(cmdID);
	cmds->AddData((UCHAR)(aOT));

	// Преобразуем значение в стеке в тип переменной
	ConvertFirstToSecond(resultST, thisST);

	// Если переменная - массив или член составного типа, то нужен сдвиг адреса
	if((varInfo.count > 1 || shiftAddress) && !bakedShift)
		second->Compile();

	// Помещаем новое значение в пременную
	cmds->AddData(cmdMov);
	cmds->AddData((USHORT)(thisST | thisDT | addrType | shiftInStack | sizeOn));
	cmds->AddData(varAddress);
	if((varInfo.count > 1) && !bakedShift)
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
	if((varInfo.count > 1 || shiftAddress) && !bakedShift)
		size += 2*second->GetSize();
	if((varInfo.count > 1) && !bakedShift)
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
// Узел для инкремента или декремента значения переменной
NodePreValOp::NodePreValOp(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool absAddr, CmdID cmd, bool preOp)
{
	// информация о переменной
	varInfo = vInfo;
	// и её адрес
	varAddress = varAddr;
	// тип изменяемого значения может быть другим, если переменная составная
	typeInfo = targetType;
	// применять динамически расчитываемый сдвиг к адресу переменной
	shiftAddress = shiftAddr;
	// использовать абсолютную адресацию (для глобальных переменных)
	absAddress = absAddr;
	// команду, которую применить к значению (DEC или INC)
	cmdID = cmd;
	// префиксный или постфиксный оператор
	prefixOperator = preOp;
	// если изменённое значение не используется, можно применить оптимизацию.
	optimised = false;	// по умолчанию выключено

	// сдвиг уже прибавлен к адресу
	bakedShift = false;

	if(typeInfo->type == TypeInfo::NOT_POD)
		throw std::string("ERROR: Increment and decrement is no supported on " + typeInfo->name);

	// если переменная - массив или член составного типа, то нужен сдвиг адреса
	if(varInfo.count > 1 || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		first = getList()->back(); getList()->pop_back();

		// сдвиг адреса должен быть  целым числом
		if(first->GetTypeInfo() != typeInt)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");

		if(first->GetNodeType() == typeNodeNumber)
		{
			if(varInfo.count > 1 && static_cast<NodeNumber<UINT>* >(first.get())->GetVal() > varInfo.count * varInfo.varType->size)
				throw std::string("ERROR: array index out of range (overflow)");
			varAddress += static_cast<NodeNumber<int>* >(first.get())->GetVal();
			bakedShift = true;
		}
	}
	getLog() << __FUNCTION__ << "\r\n"; 
}
NodePreValOp::~NodePreValOp()
{
	getLog() << __FUNCTION__ << "\r\n";
}

// Вывод о возможности использования оптимизации делает компилятор (Compiler.cpp)
// Для включения оптимизации ему предоставляется функция.
void NodePreValOp::SetOptimised(bool doOptimisation)
{
	optimised = doOptimisation;
}

void NodePreValOp::Compile()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	// Заметка (cmdID+10): Прибавляя 10, мы меняем инструкцию с INC и DEC на INC_AT и DEC_AT

	// Если переменная - массив или член составного типа, то нужен сдвиг адреса
	if((varInfo.count > 1 || shiftAddress) && !bakedShift)
		first->Compile();

	UINT shiftInStack = 0, sizeOn = 0;
	// Если это массив или член составного типа, включаем флаг что сдвиг в стеке
	if((varInfo.count > 1 || shiftAddress) && !bakedShift)
		shiftInStack = bitShiftStk;
	// Если это массив, включаем флаг, что имеется ограничение по размеру сдвига
	if((varInfo.count > 1) && !bakedShift)
		sizeOn = bitSizeOn;

	// Выбор флага для разных вариантов адресации
	UINT addrType = absAddress ? bitAddrAbs : bitAddrRel;

	// Если значение после операции не используется, можно провести оптимизацию:
	// изменять значение прямо в стеке переменных
	if(optimised)
	{
		// Меняем значение переменной прямо по адресу
		cmds->AddData(cmdID);
		cmds->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
		// адрес начала массива
		cmds->AddData(varAddress);
		// Если это массив, кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
		if((varInfo.count > 1) && !bakedShift)
			cmds->AddData(varInfo.count * varInfo.varType->size);
	}else{
		// Если переменная - массив или член составного типа
		if((varInfo.count > 1 || shiftAddress) && !bakedShift)
		{
			// Скопируем уже найденный сдвиг адресса
			// Потому что он пропадает после операций, а их у нас две
			cmds->AddData(cmdCopy);
			cmds->AddData((UCHAR)(OTYPE_INT));
		}
		if(prefixOperator)			// Для префиксного оператора ++val/--val
		{
			// Сначала изменяем переменную, затем кладём в стек её новое значение

			// Меняем значение переменной прямо по адресу
			cmds->AddData(cmdID);
			cmds->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if((varInfo.count > 1) && !bakedShift)
				cmds->AddData(varInfo.count * varInfo.varType->size);

			// Получаем новое значение переменной
			cmds->AddData(cmdPush);
			cmds->AddData((USHORT)(newST | newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if((varInfo.count > 1) && !bakedShift)
				cmds->AddData(varInfo.count * varInfo.varType->size);
		}else{						// Для  постфиксного оператора val++/val--
			// Мы изменяем переменную, но в стек помещаем старое значение
			
			// Получаем не изменённое значение переменной
			cmds->AddData(cmdPush);
			cmds->AddData((USHORT)(newST | newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if((varInfo.count > 1) && !bakedShift)
				cmds->AddData(varInfo.count * varInfo.varType->size);
			
			// Если переменная - массив или член составного типа
			// Теперь в стеке переменных лежит сдвиг адреса, а затем значение переменной
			// Следует поменять их местами, так как для следующий инструкции сдвиг адрес должен лежать наверху
			if((varInfo.count > 1 || shiftAddress) && !bakedShift)
			{
				cmds->AddData(cmdSwap);
				cmds->AddData((USHORT)(STYPE_INT | (newDT == DTYPE_FLOAT ? DTYPE_DOUBLE : newDT)));
			}
			
			// Меняем значение переменной прямо по адресу
			cmds->AddData(cmdID);
			cmds->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
			cmds->AddData(varAddress);
			if((varInfo.count > 1) && !bakedShift)
				cmds->AddData(varInfo.count * varInfo.varType->size);
		}
	}
}
void NodePreValOp::LogToStream(ostringstream& ostr)
{
	static char* strs[] = { "++", "--" };
	if(cmdID != cmdIncAt &&  cmdID != cmdDecAt)
		throw std::string("ERROR: PreValOp error");
	drawLn(ostr); ostr << *typeInfo << "PreValOp<" << strs[cmdID-cmdIncAt] << "> :\r\n"; goDown(); if(first) first->LogToStream(ostr); goUp();
}

UINT NodePreValOp::GetSize()
{
	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	UINT size = 0;
	if((varInfo.count > 1 || shiftAddress) && !bakedShift)
		size += first->GetSize();
	if(optimised)
	{
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
		if((varInfo.count > 1) && !bakedShift)
			size += sizeof(UINT);
	}else{
		if((varInfo.count > 1 || shiftAddress) && !bakedShift)
			size += sizeof(CmdID) + sizeof(UCHAR);
		if(prefixOperator)
		{
			size += 2 * (sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT));
			if((varInfo.count > 1) && !bakedShift)
				size += 2 * sizeof(UINT);
		}else{
			if((varInfo.count > 1) && !bakedShift)
				size += 2 * sizeof(UINT);
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if((varInfo.count > 1 || shiftAddress) && !bakedShift)
				size += sizeof(CmdID) + sizeof(USHORT);
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
		}
	}
	return size;
}

//////////////////////////////////////////////////////////////////////////
// Узел, производящий бинарную операцию с двумя значениями
NodeTwoAndCmdOp::NodeTwoAndCmdOp(CmdID cmd)
{
	// Бинарная операция
	cmdID = cmd;

	second = getList()->back(); getList()->pop_back();
	first = getList()->back(); getList()->pop_back();

	// На данный момент операции с композитными типами отсутствуют
	if(first->GetTypeInfo()->type == TypeInfo::NOT_POD || second->GetTypeInfo()->type == TypeInfo::NOT_POD)
		throw std::string("ERROR: Operation " + std::string(binCommandToText[cmdID - cmdAdd]) + " is not supported on " + first->GetTypeInfo()->name + " and " + second->GetTypeInfo()->name);

	// Найдём результирующий тип, после проведения операции
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
	
	// Найдём первое значение
	first->Compile();
	// Преобразуем, если надо, в тип, который получается после проведения выбранной операции
	fST = ConvertFirstForSecond(fST, sST);
	// Найдём второе значение
	second->Compile();
	// Преобразуем, если надо, в тип, который получается после проведения выбранной операции
	sST = ConvertFirstForSecond(sST, fST);
	// Произведём операцию со значениями
	cmds->AddData(cmdID);
	cmds->AddData((UCHAR)(operTypeForStackType[fST]));
}
void NodeTwoAndCmdOp::LogToStream(ostringstream& ostr)
{
	if((cmdID < cmdAdd) || (cmdID > cmdLogXor))
		throw std::string("ERROR: TwoAndCmd error");
	drawLn(ostr); ostr << *typeInfo << "TwoAndCmd<" << binCommandToText[cmdID-cmdAdd] << "> :\r\n"; goDown(); first->LogToStream(ostr); goUp(); goDownB(); second->LogToStream(ostr); goUp();
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
		typeInfo = second->GetTypeInfo();
}
NodeIfElseExpr::~NodeIfElseExpr()
{
	getLog() << __FUNCTION__ << "\r\n";
}

void NodeIfElseExpr::Compile()
{
	if(strBegin && strEnd)
		cmds->AddDescription(cmds->GetCurrPos(), strBegin, strEnd);

	// Структура дочерних элементов: if(first) second; else third;
	// Второй вариант: first ? second : third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// Вычислим условие
	first->Compile();

	// Если false, перейдём в блок else или выйдем из оператора, если такого блока не имеется
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(4 + cmds->GetCurrPos() + second->GetSize() + (third ? 6 : 0));

	// Выполним блок для успешного прохождения условия (true)
	second->Compile();
	// Если есть блок else, выполним его
	if(third)
	{
		// Только поставим выход из оператора перед его кодом, чтобы не выполнять обе ветви
		cmds->AddData(cmdJmp);
		cmds->AddData(4 + cmds->GetCurrPos() + third->GetSize());

		// Выполним блок else (false)
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

void NodeForExpr::Compile()
{
	if(strBegin && strEnd)
		cmds->AddDescription(cmds->GetCurrPos(), strBegin, strEnd);

	// Структура дочерних элементов: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// Выполним инициализацию
	first->Compile();
	UINT posTestExpr = cmds->GetCurrPos();

	// Найдём результат условия
	second->Compile();

	// Если ложно, выйдем из цикла
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmds->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);

	// Выполним содержимое цикла
	fourth->Compile();
	// Выполним операцию, проводимую после каждой итерации
	third->Compile();
	// Перейдём на проверку условия
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

void NodeWhileExpr::Compile()
{
	// Структура дочерних элементов: while(first) second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	// Выполним условие
	first->Compile();
	// Если оно ложно, выйдем из цикла
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmds->GetCurrPos()+4+second->GetSize()+2+4);
	cmds->AddData(cmds->GetCurrPos()+4+second->GetSize()+2+4);
	// Выполним содержимое цикла
	second->Compile();
	// Перейдём на проверку условия
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

void NodeDoWhileExpr::Compile()
{
	// Структура дочерних элементов: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	UINT posStart = cmds->GetCurrPos();
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmds->GetCurrPos()+first->GetSize()+second->GetSize()+2+4);
	// Выполним содержимое цикла
	first->Compile();
	// Выполним условие
	second->Compile();
	// Если условие верно, перейдём к выполнению следующей итерации цикла
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

void NodeBreakOp::Compile()
{
	// Уберём значения со стека вершин стека переменных
	for(UINT i = 0; i < popCnt; i++)
		cmds->AddData(cmdPopVTop);
	// Выйдем из цикла
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

void NodeCaseExpr::Compile()
{
	// Структура дочерних элементов: case first: second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	// switch нашёл значение, по которым производится выбор.
	// Скопируем его, так как сравнение уберёт одно значение со стека
	cmds->AddData(cmdCopy);
	// Положим значение, по которому срабатывает данный case
	// Значение может вычислятся в рантайме!
	first->Compile();
	// Сравним на равенство
	cmds->AddData(cmdEqual);
	cmds->AddData((UCHAR)(aOT));
	// Если не равны, перейдём за блок second
	cmds->AddData(cmdJmpZ);
	cmds->AddData((UCHAR)(aOT));
	cmds->AddData(cmds->GetCurrPos()+4+second->GetSize());
	// Сгенерируем код блока
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

void NodeSwitchExpr::Compile()
{
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// Сохраним вершину стека переменных
	cmds->AddData(cmdPushVTop);
	// Найдём значение по которому будем выбирать вариант кода
	first->Compile();
	// Сохраним значение для оператора break;
	indTemp.push_back(cmds->GetCurrPos()+second->GetSize()+2);
	// Сгенерируем код для всех case'ов
	second->Compile();
	// Востановим вершину стека значений
	cmds->AddData(cmdPopVTop);
	// Уберём с вершины стека значение по которому выбирался вариант кода
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
