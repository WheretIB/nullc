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
// Узел, имеющий один дочерний узел
NodeOneOP::NodeOneOP()
{
}
NodeOneOP::~NodeOneOP()
{
}

void NodeOneOP::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

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
	UINT startCmdSize = cmdList->GetCurrPos();

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
	UINT startCmdSize = cmdList->GetCurrPos();

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
UINT NodeThreeOP::GetSize()
{
	return NodeTwoOP::GetSize() + third->GetSize();
}

//////////////////////////////////////////////////////////////////////////
// Вспомогательная функция для NodeNumber<T>
void NodeNumberPushCommand(USHORT cmdFlag, char* data, UINT dataSize)
{
	cmdList->AddData(cmdPush);
	cmdList->AddData(cmdFlag);
	cmdList->AddData(data, dataSize);
}
//////////////////////////////////////////////////////////////////////////
// Узел, убирающий с вершины стека значение, оставленное дочерним узлом
NodePopOp::NodePopOp()
{
	first = TakeLastNode();
}
NodePopOp::~NodePopOp()
{
}

void NodePopOp::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// Даём дочернему узлу вычислить значение
	first->Compile();
	if(first->GetTypeInfo() != typeVoid)
	{
		// Убираем его с вершины стека
		cmdList->AddData(cmdPop);
		cmdList->AddData((USHORT)(podTypeToStackType[first->GetTypeInfo()->type]));
		if(first->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX)
			cmdList->AddData(first->GetTypeInfo()->size);
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
// Узел, производящий выбраную унарную операцию над значением на вершине стека
NodeUnaryOp::NodeUnaryOp(CmdID cmd)
{
	// Унарная операция
	cmdID = cmd;

	first = TakeLastNode();
	// Тип результата такой же, как исходный
	typeInfo = first->GetTypeInfo();
}
NodeUnaryOp::~NodeUnaryOp()
{
}

void NodeUnaryOp::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	// Даём дочернему узлу вычислить значение
	first->Compile();
	// Выполним команду
	cmdList->AddData(cmdID);
	cmdList->AddData((UCHAR)(aOT));

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

	first = TakeLastNode();
}
NodeReturnOp::~NodeReturnOp()
{
}

void NodeReturnOp::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// Найдём значение, которое будем возвращать
	first->Compile();
	// Преобразуем его в тип возвратного значения функции
	if(typeInfo)
		ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]);

	// Выйдем из функции или программы
	TypeInfo *retType = typeInfo ? typeInfo : first->GetTypeInfo();
	asmOperType operType = operTypeForStackType[podTypeToStackType[retType->type]];
	cmdList->AddData(cmdReturn);
	cmdList->AddData((USHORT)(retType->type == TypeInfo::TYPE_COMPLEX ? retType->size : (bitRetSimple | operType)));
	cmdList->AddData((USHORT)(popCnt));

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
UINT NodeReturnOp::GetSize()
{
	return NodeOneOP::GetSize() + sizeof(CmdID) + 2 * sizeof(USHORT) + (typeInfo ? ConvertFirstToSecondSize(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->type]) : 0);
}

//////////////////////////////////////////////////////////////////////////
// Узел, содержащий выражение. Работает как NodeOneOP за исключением записи в лог.
// В основном такие узлы самостоятельны, и не оставляют за собой ничего (возвращают void)
// Но иногда можно назначить им тип, который они возвратят
// (чтобы не плодить лишних классов, делающих всего одно дополнительное действие)
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
	UINT startCmdSize = cmdList->GetCurrPos();

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
}
NodeVarDef::~NodeVarDef()
{
}

void NodeVarDef::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// Если сдвиг не равен нулю
	if(shift)
	{
		// Сдвинем вершину стека переменных
		cmdList->AddData(cmdPushV);
		cmdList->AddData(shift);
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел c содержимым блока {}
NodeBlock::NodeBlock()
{
	first = TakeLastNode();
}
NodeBlock::~NodeBlock()
{
}

void NodeBlock::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	// Сохраним значение вершины стека переменных
	cmdList->AddData(cmdPushVTop);
	// Выполним содержимое блока (то же что first->Compile())
	first->Compile();
	// Востановим значение вершины стека переменных
	cmdList->AddData(cmdPopVTop);

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
	return first->GetSize() + 2 * sizeof(CmdID);
}

NodeFuncDef::NodeFuncDef(FunctionInfo *info)
{
	// Структура описания функции
	funcInfo = info;

	first = TakeLastNode();
}
NodeFuncDef::~NodeFuncDef()
{
}

void NodeFuncDef::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	// Перед содержимым функции сделаем переход за её конец
	// Код функций может быть смешан с кодом в глобальной области видимости, и его надо пропускать
	cmdList->AddData(cmdJmp);
	cmdList->AddData(cmdList->GetCurrPos() + sizeof(CmdID) + sizeof(UINT) + 2*sizeof(USHORT) + first->GetSize());
	funcInfo->address = cmdList->GetCurrPos();
	// Сгенерируем код функции
	first->Compile();

	cmdList->AddData(cmdReturn);
	if(funcInfo->retType == typeVoid)
	{
		// Если функция не возвращает значения, то это пустой ret
		cmdList->AddData((USHORT)(0));	// Возвращает значение размером 0 байт
		cmdList->AddData((USHORT)(1));
	}else{
		// Остановим программу с ошибкой
		cmdList->AddData((USHORT)(bitRetError));
		cmdList->AddData((USHORT)(1));
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел, производящий вызов функции
NodeFuncCall::NodeFuncCall(FunctionInfo *info)
{
	// Структура описания функции
	funcInfo = info;
	// Тип результата - тип возвратного значения функции
	typeInfo = funcInfo->retType;

	// Возьмём узлы каждого параметра
	for(UINT i = 0; i < funcInfo->params.size(); i++)
		paramList.push_back(TakeLastNode());
}
NodeFuncCall::~NodeFuncCall()
{
}

void NodeFuncCall::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	// Если имеются параметры, найдём их значения
	UINT currParam = 0;
	
	if(funcInfo->address == -1 && funcInfo->funcPtr != NULL)
	{
		std::list<shared_ptr<NodeZeroOP> >::iterator s, e;
		s = paramList.begin();
		e = paramList.end();
		for(; s != e; s++)
		{
			// Определим значение параметра
			(*s)->Compile();
			// Преобразуем его в тип входного параметра функции
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcInfo->params[currParam].varType->type]);
			currParam++;
		}
	}else{
		std::list<shared_ptr<NodeZeroOP> >::reverse_iterator s, e;
		s = paramList.rbegin();
		e = paramList.rend();
		for(; s != e; s++)
		{
			// Определим значение параметра
			(*s)->Compile();
			// Преобразуем его в тип входного параметра функции
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcInfo->params[currParam].varType->type]);
			currParam++;
		}
	}
	if(funcInfo->address == -1)		// Если функция встроенная
	{
		// Вызовем по имени
		cmdList->AddData(cmdCallStd);
		cmdList->AddData(funcInfo);
	}else{					// Если функция определена пользователем
		// Перенесём в локальные параметры прямо тут, фигле
		UINT addr = 0;
		for(int i = int(funcInfo->params.size())-1; i >= 0; i--)
		{
			asmStackType newST = podTypeToStackType[funcInfo->params[i].varType->type];
			asmDataType newDT = podTypeToDataType[funcInfo->params[i].varType->type];
			cmdList->AddData(cmdMov);
			cmdList->AddData((USHORT)(newST | newDT | bitAddrRelTop));
			// адрес начала массива
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

		// Надём, сколько занимают все переменные
		UINT allSize=0;
		for(UINT i = 0; i < funcInfo->params.size(); i++)
			allSize += funcInfo->params[i].varType->size;

		// Расширим стек переменные на это значение
		cmdList->AddData(cmdPushV);
		cmdList->AddData(allSize);

		// Вызовем по адресу
		cmdList->AddData(cmdCall);
		cmdList->AddData(funcInfo->address);
		cmdList->AddData((USHORT)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[podTypeToStackType[typeInfo->type]])));
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
		size += ConvertFirstToSecondSize(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcInfo->params[currParam].varType->type]);
		currParam++;
	}
	
	if(funcInfo->address == -1)
	{
		size += sizeof(CmdID) + sizeof(funcInfo);
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
// Узел, вычисляющий величину сдвига от начала массива
NodePushShift::NodePushShift(int varSizeOf)
{
	// Возвращаем int
	typeInfo = typeInt;
	// Запомним размер типа
	sizeOfType = varSizeOf;

	first = TakeLastNode();
}
NodePushShift::~NodePushShift()
{
}

void NodePushShift::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	asmOperType oAsmType = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// Вычислим индекс
	first->Compile();
	// Переведём его в целое число
	cmdList->AddData(cmdCTI);
	// Передадим тип операнда
	cmdList->AddData((UCHAR)(oAsmType));
	// Умножив на размер элемента (сдвиг должен быть в байтах)
	cmdList->AddData(sizeOfType);

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел для получения адреса переменной относительно базы стека
NodeGetAddress::NodeGetAddress(VariableInfo vInfo, UINT varAddr)
{
	// информация о переменной
	varInfo = vInfo;
	// и её адрес
	varAddress = varAddr;
	// возвращает "указатель" - целое число
	typeInfo = typeInt;
}
NodeGetAddress::~NodeGetAddress()
{
}

void NodeGetAddress::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	// Положим в стек адрес переменной, относительно базы стека
	cmdList->AddData(cmdGetAddr);
	cmdList->AddData(varAddress);

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел для присвоения значения переменной
NodeVarSet::NodeVarSet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddr, bool shiftAddr, bool absAddr, UINT pushBytes)
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
	// на сколько байт следует увеличить стек переменных
	bytesToPush = pushBytes;

	// сдвиг уже прибавлен к адресу
	bakedShift = false;

	// получить узел, расчитывающий значение
	first = TakeLastNode();

	// Если идёт первое определение переменной и массиву присваивается базовый тип
	arrSetAll = (bytesToPush && typeInfo->arrLevel != 0 && first->GetTypeInfo()->arrLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && first->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX);//arraySetAll;
	if(arrSetAll)
		shiftAddress = false;

	if(first->GetTypeInfo() == typeVoid)
		throw std::string("ERROR: cannot convert from void to " + typeInfo->GetTypeName());
	if(typeInfo == typeVoid)
		throw std::string("ERROR: cannot convert from " + first->GetTypeInfo()->GetTypeName() + " to void");

	// Если типы не равны
	if(first->GetTypeInfo() != typeInfo)
	{
		// Если это не встроенные базовые типы, или
		// если различаются размерности массивов, и при этом не происходит первое определение переменной, или
		// если различается глубина указателей, или
		// если это указатель, глубина указателей равна, но при этом тип, на который указывает указатель отличается, то
		// сообщим об ошибке несоответствия типов
		if(!(typeInfo->type != TypeInfo::TYPE_COMPLEX && first->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX) ||
			(typeInfo->arrLevel != first->GetTypeInfo()->arrLevel && !arrSetAll) ||
			(typeInfo->refLevel != first->GetTypeInfo()->refLevel) ||
			(typeInfo->refLevel && typeInfo->refLevel == first->GetTypeInfo()->refLevel && typeInfo->subType != first->GetTypeInfo()->subType))
		{
			if(!(typeInfo->arrLevel != 0 && first->GetTypeInfo()->arrLevel == 0 && arrSetAll))
				throw std::string("ERROR: Cannot convert '" + first->GetTypeInfo()->GetTypeName() + "' to '" + typeInfo->GetTypeName() + "'");
		}
	}

	// если переменная - массив и обновляется одна ячейка
	// или если переменная - член составного типа и нужен сдвиг адреса
	if(shiftAddress)
	{
		// получить узел, расчитывающий сдвиг адреса
		second = TakeLastNode();

		// сдвиг адреса должен быть целым числом
		if(second->GetTypeInfo()->type != TypeInfo::TYPE_INT)
			throw std::string("ERROR: NodeVarSet() address shift must be an integer number");

		if(second->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize != -1)
		{
			if(varInfo.varType->arrLevel != 0 && static_cast<NodeNumber<UINT>* >(second.get())->GetVal() > varInfo.varType->size)
				throw std::string("ERROR: array index out of range");
			varAddress += static_cast<NodeNumber<int>* >(second.get())->GetVal();
			bakedShift = true;
		}
		if(second->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize == -1)
			bakedShift = true;
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
	UINT startCmdSize = cmdList->GetCurrPos();

	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	if(bytesToPush)
	{
		cmdList->AddData(cmdPushV);
		cmdList->AddData(bytesToPush);
	}

	// расчитываем значение для присвоения переменной
	first->Compile();

	if(varInfo.varType->arrSize == -1 && shiftAddress)
	{
		// преобразуем в тип ячейки массива
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

		// добавляем команду присвоения
		cmdList->AddData(cmdMov);
		cmdList->AddData((USHORT)(newST | newDT | bitAddrAbs | (bakedShift ? bitShiftStk : (bitShiftStk/* | bitSizeOn*/))));
		// сдвиг от начала массива
		cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(second.get())->GetVal() : 0);
		// кладём размер массива (в байтах) в команду, для предотвращения выхода за его пределы
		/*if(!bakedShift)
			cmdList->AddData(varInfo.varType->size);
		if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
			cmdList->AddData(typeInfo->size);*/
	}else{
		if((shiftAddress || arrSetAll) && varInfo.varType->arrLevel != 0) 	// если это массив с индексом или заполняется целиком
		{
			if(arrSetAll)			// если указано присвоить значение всем ячейкам массива
			{
				// преобразуем в тип ячейки массива
				ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], podTypeToStackType[typeInfo->subType->type]);
				// Установим значение массиву
				cmdList->AddData(cmdSetRange);
				cmdList->AddData((USHORT)(podTypeToDataType[typeInfo->subType->type]));
				cmdList->AddData(varAddress);
				cmdList->AddData(varInfo.varType->size/typeInfo->subType->size);
			}else{					// если указано присвоить значение одной ячейке массива
				// преобразуем в тип ячейки массива
				ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);

				// кладём сдвиг от начала массива (в байтах)
				if(!bakedShift)
					second->Compile();

				// добавляем команду присвоения
				cmdList->AddData(cmdMov);
				cmdList->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | (bakedShift ? 0 : (bitShiftStk | bitSizeOn))));
				// адрес начала массива
				cmdList->AddData(varAddress);
				// кладём размер массива (в байтах) в команду, для предотвращения выхода за его пределы
				if(!bakedShift)
					cmdList->AddData(varInfo.varType->size);
				if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
					cmdList->AddData(typeInfo->size);
			}
		}else{	// если это не массив или массив без индекса
			// преобразуем в тип переменной
			if(varInfo.varType->arrLevel == 0)
				ConvertFirstToSecond(podTypeToStackType[first->GetTypeInfo()->type], newST);

			if(shiftAddress && !bakedShift)		// если переменная - член составного типа и нужен сдвиг адреса
			{
				// кладём сдвиг в стек (в байтах)
				second->Compile();
			}

			// добавляем команду присвоения
			cmdList->AddData(cmdMov);
			cmdList->AddData((USHORT)(newST | newDT | (absAddress ? bitAddrAbs : bitAddrRel) | ((shiftAddress && !bakedShift) ? bitShiftStk : 0)));
			cmdList->AddData(varAddress);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				cmdList->AddData(typeInfo->size);
		}
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeVarSet::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << (*typeInfo) << "VarSet " << varInfo << " (" << (int)varAddress << (absAddress ? " absolute" : "") << ")";
	ostr << (bakedShift ? " baked" : "") << (shiftAddress ? " shift" : "") << (arrSetAll ? " set whole array" : "");
	if(bytesToPush)
		ostr << " pusht " << bytesToPush;
	ostr << "\r\n";
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
		if((shiftAddress || arrSetAll) && varInfo.varType->arrLevel != 0) 	// если это массив с индексом или заполняется целиком
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
	if(shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		first = TakeLastNode();

		// сдвиг адреса должен быть  целым числом
		if(first->GetTypeInfo()->type != TypeInfo::TYPE_INT)
			throw std::string("ERROR: NodeVarGet() address shift must be an integer number");

		if(first->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize != -1)
		{
			if(varInfo.varType->arrLevel != 0 && static_cast<NodeNumber<UINT>* >(first.get())->GetVal() > varInfo.varType->size)
				throw std::string("ERROR: array index out of range (overflow)");
			varAddress += static_cast<NodeNumber<int>* >(first.get())->GetVal();
			bakedShift = true;
		}
		if(first->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize == -1 && typeInfo != typeVoid)
			bakedShift = true;
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
	UINT startCmdSize = cmdList->GetCurrPos();

	asmStackType asmST = podTypeToStackType[typeInfo->type];
	asmDataType asmDT = podTypeToDataType[typeInfo->type];
	if(varInfo.varType->arrSize == -1 && typeInfo != typeVoid && shiftAddress)
	{
		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT | (absAddress ? bitAddrAbs : bitAddrRel)));
		cmdList->AddData(varAddress);
		if(!bakedShift)
		{
			first->Compile();
			cmdList->AddData(cmdAdd);
			cmdList->AddData((UCHAR)(OTYPE_INT));
		}

		// добавляем команду присвоения
		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(asmST | asmDT | bitAddrAbs | (bakedShift ? bitShiftStk : (bitShiftStk/* | bitSizeOn*/))));
		// адрес начала массива
		cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(first.get())->GetVal() : 0);
	}else{
		if(shiftAddress && varInfo.varType->arrLevel != 0 && varInfo.varType->arrSize != -1) 	// если это массив с индексом
		{
			// кладём сдвиг от начала массива (в байтах)
			if(!bakedShift)
				first->Compile();

			// получаем значение переменной по адресу
			cmdList->AddData(cmdPush);
			cmdList->AddData((USHORT)(asmST | asmDT | (absAddress ? bitAddrAbs : bitAddrRel) | (bakedShift ? 0 : (bitShiftStk | bitSizeOn))));
			// адрес начала массива
			cmdList->AddData(varAddress);
			// кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
			if(!bakedShift)
				cmdList->AddData(varInfo.varType->size);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				cmdList->AddData(typeInfo->size);
		}else{						// если не это массив
			if(shiftAddress && !bakedShift)		// если переменная - член составного типа и нужен сдвиг адреса
				first->Compile();		// кладём его в стек (в байтах)

			// получаем значение переменной по адресу
			cmdList->AddData(cmdPush);
			cmdList->AddData((USHORT)(asmST | asmDT | (absAddress ? bitAddrAbs : bitAddrRel) | ((shiftAddress && !bakedShift) ? bitShiftStk : 0)));
			// адрес переменной
			cmdList->AddData(varAddress);
			if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
				cmdList->AddData(typeInfo->size);
		}
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
	if(varInfo.varType->arrSize == -1 && typeInfo != typeVoid && shiftAddress)
	{
		size += 2 * sizeof(CmdID) + 2 * sizeof(USHORT) + 2 * sizeof(UINT);
		if(!bakedShift)
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

	first = TakeLastNode();

	// На данный момент операции с композитными типами отсутствуют
	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0 || first->GetTypeInfo()->refLevel != 0)
		throw std::string("ERROR: Operation " + std::string(binCommandToText[cmdID - cmdAdd]) + "= is not supported on '" + typeInfo->GetTypeName() + "' and '" + first->GetTypeInfo()->GetTypeName() + "'");
	if(typeInfo == typeVoid)
		throw std::string("ERROR: cannot modify void type");

	// если переменная - массив или член составного типа, то нужен сдвиг адреса
	if(varInfo.varType->arrLevel != 0 || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		second = TakeLastNode();

		// сдвиг адреса должен быть  целым числом
		if(second->GetTypeInfo()->type != TypeInfo::TYPE_INT)
			throw std::string("ERROR: NodeVarSetAndOp() address shift must be an integer number");

		if(second->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize != -1)
		{
			if(varInfo.varType->arrLevel != 0 && static_cast<NodeNumber<UINT>* >(second.get())->GetVal() > varInfo.varType->size)
				throw std::string("ERROR: array index out of range");
			varAddress += static_cast<NodeNumber<int>* >(second.get())->GetVal();
			bakedShift = true;
		}
		if(second->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize == -1)
			bakedShift = true;
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
	UINT startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// Тип нашей переменной в основном стеке
	asmStackType thisST = podTypeToStackType[typeInfo->type];
	// Тип второго значения в основном стеке
	asmStackType firstST = podTypeToStackType[first->GetTypeInfo()->type];
	// Cтековый тип, который получится после выполнения бинарной операции
	asmStackType resultST;// = ConvertFirstForSecond(thisST, firstST);

	// Тип нашей переменной в стеке переменных
	asmDataType thisDT = podTypeToDataType[typeInfo->type];

	// Тип, над которым работает операция
	asmOperType aOT;

	// Если переменная - массив или член составного типа, то нужен сдвиг адреса
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift && varInfo.varType->arrSize != -1)
		second->Compile();

	UINT shiftInStack = 0, sizeOn = 0;
	// Если это массив или член составного типа, включаем флаг что сдвиг в стеке
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		shiftInStack = bitShiftStk;
	// Если это массив, включаем флаг, что имеется ограничение по размеру сдвига
	if((varInfo.varType->arrLevel != 0) && !bakedShift)
		sizeOn = bitSizeOn;

	// Выбор флага для разных вариантов адресации
	UINT addrType = absAddress ? bitAddrAbs : bitAddrRel;

	if(varInfo.varType->arrSize == -1)
	{
		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT | addrType));
		cmdList->AddData(varAddress);
		if(!bakedShift)
		{
			second->Compile();
			cmdList->AddData(cmdAdd);
			cmdList->AddData((UCHAR)(OTYPE_INT));
		}

		// добавляем команду присвоения
		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(thisST | thisDT | bitAddrAbs | bitShiftStk));
		cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(first.get())->GetVal() : 0);
	}else{
		// Поместим значение переменной в стек
		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(thisST | thisDT | addrType | shiftInStack | sizeOn));
		cmdList->AddData(varAddress);
		if((varInfo.varType->arrLevel != 0) && !bakedShift)
			cmdList->AddData(varInfo.varType->size);
	}

	// Преобразуем в тип результата бинарной операции
	resultST = ConvertFirstForSecond(thisST, firstST);

	// Теперь известем тип операции
	aOT = operTypeForStackType[resultST];

	// Найдём значение второго операнда
	first->Compile();
	// Преобразуем в тип результата бинарной операции
	firstST = ConvertFirstForSecond(firstST, thisST);

	// Выполним бинарную операцию
	cmdList->AddData(cmdID);
	cmdList->AddData((UCHAR)(aOT));

	// Преобразуем значение в стеке в тип переменной
	ConvertFirstToSecond(resultST, thisST);

	// Если переменная - массив или член составного типа, то нужен сдвиг адреса
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift && varInfo.varType->arrSize != -1)
		second->Compile();

	if(varInfo.varType->arrSize == -1)
	{
		cmdList->AddData(cmdPush);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT | addrType));
		cmdList->AddData(varAddress);
		if(!bakedShift)
		{
			second->Compile();
			cmdList->AddData(cmdAdd);
			cmdList->AddData((UCHAR)(OTYPE_INT));
		}

		// добавляем команду присвоения
		cmdList->AddData(cmdMov);
		cmdList->AddData((USHORT)(thisST | thisDT | bitAddrAbs | bitShiftStk));
		// сдвиг от начала массива
		cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(second.get())->GetVal() : 0);
	}else{
		// Помещаем новое значение в пременную
		cmdList->AddData(cmdMov);
		cmdList->AddData((USHORT)(thisST | thisDT | addrType | shiftInStack | sizeOn));
		cmdList->AddData(varAddress);
		if((varInfo.varType->arrLevel != 0) && !bakedShift)
			cmdList->AddData(varInfo.varType->size);
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
	if((varInfo.varType->arrLevel != 0) && !bakedShift && varInfo.varType->arrSize != -1)
		size += 2 * sizeof(UINT);

	if(varInfo.varType->arrSize == -1)
		size += 2 * (sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT)) + (!bakedShift ? (sizeof(CmdID) + 1) : 0);
	else
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);

	size += ConvertFirstForSecondSize(thisST, firstST).first;

	size += first->GetSize();
	size += ConvertFirstForSecondSize(firstST, thisST).first;

	size += sizeof(CmdID) + sizeof(UCHAR);

	size += ConvertFirstToSecondSize(resultST, thisST);

	if(varInfo.varType->arrSize == -1)
		size += 2 * (sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT)) + (!bakedShift ? (sizeof(CmdID) + 1) : 0);
	else
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

	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0)
		throw std::string("ERROR: ") + (cmdID == cmdIncAt ? "increment" : "decrement") + std::string(" is not supported on '") + typeInfo->GetTypeName() + "'";

	// если переменная - массив или член составного типа, то нужен сдвиг адреса
	if(varInfo.varType->arrLevel != 0 || shiftAddress)	
	{
		// получить узел, расчитывающий сдвиг адреса
		first = TakeLastNode();

		// сдвиг адреса должен быть  целым числом
		if(first->GetTypeInfo()->type != TypeInfo::TYPE_INT)
			throw std::string("ERROR: NodePreValOp() address shift must be an integer number");

		if(first->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize != -1)
		{
			if(varInfo.varType->arrLevel != 0 && static_cast<NodeNumber<UINT>* >(first.get())->GetVal() > varInfo.varType->size)
				throw std::string("ERROR: array index out of range (overflow)");
			varAddress += static_cast<NodeNumber<int>* >(first.get())->GetVal();
			bakedShift = true;
		}
		if(first->GetNodeType() == typeNodeNumber && varInfo.varType->arrSize == -1)
			bakedShift = true;

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
NodePreValOp::~NodePreValOp()
{
}

// Вывод о возможности использования оптимизации делает компилятор (Compiler.cpp)
// Для включения оптимизации ему предоставляется функция.
void NodePreValOp::SetOptimised(bool doOptimisation)
{
	optimised = doOptimisation;
}

void NodePreValOp::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	asmStackType newST = podTypeToStackType[typeInfo->type];
	asmDataType newDT = podTypeToDataType[typeInfo->type];

	// Заметка (cmdID+10): Прибавляя 10, мы меняем инструкцию с INC и DEC на INC_AT и DEC_AT

	// Если переменная - массив или член составного типа, то нужен сдвиг адреса
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift && varInfo.varType->arrSize != -1)
		first->Compile();

	UINT shiftInStack = 0, sizeOn = 0;
	// Если это массив или член составного типа, включаем флаг что сдвиг в стеке
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift)
		shiftInStack = bitShiftStk;
	// Если это массив, включаем флаг, что имеется ограничение по размеру сдвига
	if((varInfo.varType->arrLevel != 0) && !bakedShift)
		sizeOn = bitSizeOn;

	// Выбор флага для разных вариантов адресации
	UINT addrType = absAddress ? bitAddrAbs : bitAddrRel;

	// Если значение после операции не используется, можно провести оптимизацию:
	// изменять значение прямо в стеке переменных
	if(optimised)
	{
		if(varInfo.varType->arrSize == -1)
		{
			cmdList->AddData(cmdPush);
			cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT | addrType));
			cmdList->AddData(varAddress);
			if(!bakedShift)
			{
				first->Compile();
				cmdList->AddData(cmdAdd);
				cmdList->AddData((UCHAR)(OTYPE_INT));
			}

			// добавляем команду присвоения
			cmdList->AddData(cmdID);
			cmdList->AddData((USHORT)(newDT | bitAddrAbs | bitShiftStk));
			// сдвиг от начала массива
			cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(first.get())->GetVal() : 0);
		}else{
			// Меняем значение переменной прямо по адресу
			cmdList->AddData(cmdID);
			cmdList->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
			// адрес начала массива
			cmdList->AddData(varAddress);
			// Если это массив, кладём размер массива (в байтах) в стек, для предотвращения выхода за его пределы
			if((varInfo.varType->arrLevel != 0) && !bakedShift)
				cmdList->AddData(varInfo.varType->size);
		}
	}else{
		if(varInfo.varType->arrSize == -1)
		{
			cmdList->AddData(cmdPush);
			cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT | addrType));
			cmdList->AddData(varAddress);
			if(!bakedShift)
			{
				first->Compile();
				cmdList->AddData(cmdAdd);
				cmdList->AddData((UCHAR)(OTYPE_INT));
			}
		}
		// Если переменная - массив или член составного типа
		if(((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift) || varInfo.varType->arrSize == -1)
		{
			// Скопируем уже найденный сдвиг адресса
			// Потому что он пропадает после операций, а их у нас две
			cmdList->AddData(cmdCopy);
			cmdList->AddData((UCHAR)(OTYPE_INT));
		}
		if(prefixOperator)			// Для префиксного оператора ++val/--val
		{
			// Сначала изменяем переменную, затем кладём в стек её новое значение

			// Меняем значение переменной прямо по адресу
			cmdList->AddData(cmdID);
			if(varInfo.varType->arrSize == -1)
			{
				cmdList->AddData((USHORT)(newDT | bitAddrAbs | bitShiftStk));
				// сдвиг от начала массива
				cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(first.get())->GetVal() : 0);
			}else{
				cmdList->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
				cmdList->AddData(varAddress);
				if((varInfo.varType->arrLevel != 0) && !bakedShift)
					cmdList->AddData(varInfo.varType->size);
			}

			// Получаем новое значение переменной
			cmdList->AddData(cmdPush);
			if(varInfo.varType->arrSize == -1)
			{
				cmdList->AddData((USHORT)(newST | newDT | bitAddrAbs | bitShiftStk));
				// сдвиг от начала массива
				cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(first.get())->GetVal() : 0);
			}else{
				cmdList->AddData((USHORT)(newST | newDT | addrType | shiftInStack | sizeOn));
				cmdList->AddData(varAddress);
				if((varInfo.varType->arrLevel != 0) && !bakedShift)
					cmdList->AddData(varInfo.varType->size);
			}
		}else{						// Для  постфиксного оператора val++/val--
			// Мы изменяем переменную, но в стек помещаем старое значение
			
			// Получаем не изменённое значение переменной
			cmdList->AddData(cmdPush);
			if(varInfo.varType->arrSize == -1)
			{
				cmdList->AddData((USHORT)(newST | newDT | bitAddrAbs | bitShiftStk));
				// сдвиг от начала массива
				cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(first.get())->GetVal() : 0);
			}else{
				cmdList->AddData((USHORT)(newST | newDT | addrType | shiftInStack | sizeOn));
				cmdList->AddData(varAddress);
				if((varInfo.varType->arrLevel != 0) && !bakedShift)
					cmdList->AddData(varInfo.varType->size);
			}
			
			// Если переменная - массив или член составного типа
			// Теперь в стеке переменных лежит сдвиг адреса, а затем значение переменной
			// Следует поменять их местами, так как для следующий инструкции сдвиг адрес должен лежать наверху
			if(((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift) || varInfo.varType->arrSize == -1)
			{
				cmdList->AddData(cmdSwap);
				cmdList->AddData((USHORT)(STYPE_INT | (newDT == DTYPE_FLOAT ? DTYPE_DOUBLE : newDT)));
			}
			
			// Меняем значение переменной прямо по адресу
			cmdList->AddData(cmdID);
			if(varInfo.varType->arrSize == -1)
			{
				cmdList->AddData((USHORT)(newDT | bitAddrAbs | bitShiftStk));
				// сдвиг от начала массива
				cmdList->AddData(bakedShift ? static_cast<NodeNumber<int>* >(first.get())->GetVal() : 0);
			}else{
				cmdList->AddData((USHORT)(newDT | addrType | shiftInStack | sizeOn));
				cmdList->AddData(varAddress);
				if((varInfo.varType->arrLevel != 0) && !bakedShift)
					cmdList->AddData(varInfo.varType->size);
			}
		}
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
	if((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift && varInfo.varType->arrSize != -1)
		size += first->GetSize();
	if(optimised)
	{
		if(varInfo.varType->arrSize == -1)
		{
			size += 2 * (sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT));
			if(!bakedShift)
				size += first->GetSize() + sizeof(CmdID) + 1;
		}else{
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if((varInfo.varType->arrLevel != 0) && !bakedShift)
				size += sizeof(UINT);
		}
	}else{
		if(varInfo.varType->arrSize == -1)
		{
			size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
			if(!bakedShift)
				size += first->GetSize() + sizeof(CmdID) + 1;
		}
		if(((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift) || varInfo.varType->arrSize == -1)
			size += sizeof(CmdID) + sizeof(UCHAR);
		size += 2 * (sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT));
		if((varInfo.varType->arrLevel != 0) && !bakedShift && varInfo.varType->arrSize != -1)
			size += 2 * sizeof(UINT);
		if(!prefixOperator)
		{
			if(((varInfo.varType->arrLevel != 0 || shiftAddress) && !bakedShift) || varInfo.varType->arrSize == -1)
				size += sizeof(CmdID) + sizeof(USHORT);
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

	second = TakeLastNode();
	first = TakeLastNode();

	// На данный момент операции с композитными типами отсутствуют
	if(first->GetTypeInfo()->refLevel == 0)
		if(first->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX || second->GetTypeInfo()->type == TypeInfo::TYPE_COMPLEX)
			throw std::string("ERROR: Operation " + std::string(binCommandToText[cmdID - cmdAdd]) + " is not supported on '" + first->GetTypeInfo()->GetTypeName() + "' and '" + second->GetTypeInfo()->GetTypeName() + "'");
	if(first->GetTypeInfo() == typeVoid)
		throw std::string("ERROR: first operator returns void");
	if(second->GetTypeInfo() == typeVoid)
		throw std::string("ERROR: second operator returns void");

	// Найдём результирующий тип, после проведения операции
	typeInfo = ChooseBinaryOpResultType(first->GetTypeInfo(), second->GetTypeInfo());
}
NodeTwoAndCmdOp::~NodeTwoAndCmdOp()
{
}

void NodeTwoAndCmdOp::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

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
	cmdList->AddData(cmdID);
	cmdList->AddData((UCHAR)(operTypeForStackType[fST]));

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
// Узел, выполняющий блок if(){}else{} или условный оператор ?:
NodeIfElseExpr::NodeIfElseExpr(bool haveElse, bool isTerm)
{
	// Если имеется блок else{}
	if(haveElse)
	{
		third = TakeLastNode();
	}
	second = TakeLastNode();
	first = TakeLastNode();
	// Если это условный оператор, то имеется тип результата отличный от void
	// Потенциальная ошибка имеется, когда разные результирующие варианты имеют разные типы.
	// Следует исправить! BUG 0003
	if(isTerm)
		typeInfo = second->GetTypeInfo();
}
NodeIfElseExpr::~NodeIfElseExpr()
{
}

void NodeIfElseExpr::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// Структура дочерних элементов: if(first) second; else third;
	// Второй вариант: first ? second : third;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];
	// Вычислим условие
	first->Compile();

	// Если false, перейдём в блок else или выйдем из оператора, если такого блока не имеется
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((UCHAR)(aOT));
	cmdList->AddData(4 + cmdList->GetCurrPos() + second->GetSize() + (third ? 6 : 0));

	// Выполним блок для успешного прохождения условия (true)
	second->Compile();
	// Если есть блок else, выполним его
	if(third)
	{
		// Только поставим выход из оператора перед его кодом, чтобы не выполнять обе ветви
		cmdList->AddData(cmdJmp);
		cmdList->AddData(4 + cmdList->GetCurrPos() + third->GetSize());

		// Выполним блок else (false)
		third->Compile();
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел, выполняющий блок for(){}
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
	UINT startCmdSize = cmdList->GetCurrPos();

	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	// Структура дочерних элементов: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// Выполним инициализацию
	first->Compile();
	UINT posTestExpr = cmdList->GetCurrPos();

	// Найдём результат условия
	second->Compile();

	// Если ложно, выйдем из цикла
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((UCHAR)(aOT));
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmdList->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);
	cmdList->AddData(cmdList->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);

	// Выполним содержимое цикла
	fourth->Compile();
	// Выполним операцию, проводимую после каждой итерации
	third->Compile();
	// Перейдём на проверку условия
	cmdList->AddData(cmdJmp);
	cmdList->AddData(posTestExpr);
	indTemp.pop_back();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел, выполняющий блок while(){}
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
	UINT startCmdSize = cmdList->GetCurrPos();

	// Структура дочерних элементов: while(first) second;
	asmOperType aOT = operTypeForStackType[podTypeToStackType[first->GetTypeInfo()->type]];

	UINT posStart = cmdList->GetCurrPos();
	// Выполним условие
	first->Compile();
	// Если оно ложно, выйдем из цикла
	cmdList->AddData(cmdJmpZ);
	cmdList->AddData((UCHAR)(aOT));
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmdList->GetCurrPos()+4+second->GetSize()+2+4);
	cmdList->AddData(cmdList->GetCurrPos()+4+second->GetSize()+2+4);
	// Выполним содержимое цикла
	second->Compile();
	// Перейдём на проверку условия
	cmdList->AddData(cmdJmp);
	cmdList->AddData(posStart);

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел, выполняющий блок do{}while()
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
	UINT startCmdSize = cmdList->GetCurrPos();

	// Структура дочерних элементов: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	UINT posStart = cmdList->GetCurrPos();
	// Сохраним адрес для выхода из цикла оператором break;
	indTemp.push_back(cmdList->GetCurrPos()+first->GetSize()+second->GetSize()+2+4);
	// Выполним содержимое цикла
	first->Compile();
	// Выполним условие
	second->Compile();
	// Если условие верно, перейдём к выполнению следующей итерации цикла
	cmdList->AddData(cmdJmpNZ);
	cmdList->AddData((UCHAR)(aOT));
	cmdList->AddData(posStart);
	indTemp.pop_back();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел, производящий операцию break;
NodeBreakOp::NodeBreakOp(UINT c)
{
	// Сколько значений нужно убрать со стека вершин стека переменных (о_О)
	popCnt = c;
}
NodeBreakOp::~NodeBreakOp()
{
}

void NodeBreakOp::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	// Уберём значения со стека вершин стека переменных
	for(UINT i = 0; i < popCnt; i++)
		cmdList->AddData(cmdPopVTop);
	// Выйдем из цикла
	cmdList->AddData(cmdJmp);
	cmdList->AddData(indTemp.back());

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел, определяющий код для switch
NodeSwitchExpr::NodeSwitchExpr()
{
	// Возьмём узел с условием
	first = TakeLastNode();
}
NodeSwitchExpr::~NodeSwitchExpr()
{
}

void NodeSwitchExpr::AddCase()
{
	// Возьмём с верхушки блок
	caseBlockList.push_back(TakeLastNode());
	// Возьмём условие для блока
	caseCondList.push_back(TakeLastNode());
}

void NodeSwitchExpr::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	asmStackType aST = podTypeToStackType[first->GetTypeInfo()->type];
	asmOperType aOT = operTypeForStackType[aST];
	// Сохраним вершину стека переменных
	cmdList->AddData(cmdPushVTop);
	// Найдём значение по которому будем выбирать вариант кода
	first->Compile();

	// Найдём конец свитча
	UINT switchEnd = cmdList->GetCurrPos() + 2*sizeof(CmdID) + sizeof(UINT) + sizeof(USHORT) + caseCondList.size() * (3*sizeof(CmdID) + 3 + sizeof(UINT));
	for(casePtr s = caseCondList.begin(), e = caseCondList.end(); s != e; s++)
		switchEnd += (*s)->GetSize();
	UINT condEnd = switchEnd;
	int blockNum = 0;
	for(casePtr s = caseBlockList.begin(), e = caseBlockList.end(); s != e; s++, blockNum++)
		switchEnd += (*s)->GetSize() + sizeof(CmdID) + sizeof(USHORT) + (blockNum != caseBlockList.size()-1 ? sizeof(CmdID) + sizeof(UINT) : 0);

	// Сохраним адрес для оператора break;
	indTemp.push_back(switchEnd+2);

	// Сгенерируем код для всех case'ов
	casePtr cond = caseCondList.begin(), econd = caseCondList.end();
	casePtr block = caseBlockList.begin(), eblocl = caseBlockList.end();
	UINT caseAddr = condEnd;
	for(; cond != econd; cond++, block++)
	{
		cmdList->AddData(cmdCopy);
		cmdList->AddData((UCHAR)(aOT));

		(*cond)->Compile();
		// Сравним на равенство
		cmdList->AddData(cmdEqual);
		cmdList->AddData((UCHAR)(aOT));
		// Если равны, перейдём на нужный кейс
		cmdList->AddData(cmdJmpNZ);
		cmdList->AddData((UCHAR)(aOT));
		cmdList->AddData(caseAddr);
		caseAddr += (*block)->GetSize() + 2*sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	}
	// Уберём с вершины стека значение по которому выбирался вариант кода
	cmdList->AddData(cmdPop);
	cmdList->AddData((USHORT)(aST));

	cmdList->AddData(cmdJmp);
	cmdList->AddData(switchEnd);
	blockNum = 0;
	for(block = caseBlockList.begin(), eblocl = caseBlockList.end(); block != eblocl; block++, blockNum++)
	{
		// Уберём с вершины стека значение по которому выбирался вариант кода
		cmdList->AddData(cmdPop);
		cmdList->AddData((USHORT)(aST));
		(*block)->Compile();
		if(blockNum != caseBlockList.size()-1)
		{
			cmdList->AddData(cmdJmp);
			cmdList->AddData(cmdList->GetCurrPos() + sizeof(UINT) + sizeof(CmdID) + sizeof(USHORT));
		}
	}

	// Востановим вершину стека значений
	cmdList->AddData(cmdPopVTop);

	indTemp.pop_back();

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
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
// Узел, содержащий список выражений.
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
	UINT startCmdSize = cmdList->GetCurrPos();

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
UINT NodeExpressionList::GetSize()
{
	UINT size = 0;
	for(listPtr s = exprList.begin(), e = exprList.end(); s != e; s++)
		size += (*s)->GetSize();
	return size;
}
