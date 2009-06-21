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

std::vector<UINT>	breakAddr;
std::vector<UINT>	continueAddr;

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
	cmdList->AddData(cmdPushImmt);
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
	ostr << *typeInfo << "VarDef '" << name << "' " << shift << "\r\n";
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
	ostr << *typeInfo << "Block :\r\n";
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
	ostr << *typeInfo << "FuncDef " << funcInfo->name << "\r\n";
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
NodeFuncCall::NodeFuncCall(FunctionInfo *info, FunctionType *type)
{
	// Структура описания функции
	funcInfo = info;

	// Структура описания типа функции
	funcType = type;

	// Тип результата - тип возвратного значения функции
	typeInfo = type->retType;

	if(funcInfo && funcInfo->type == FunctionInfo::LOCAL)
		second = TakeLastNode();

	if(!funcInfo)
		first = TakeLastNode();

	// Возьмём узлы каждого параметра
	for(UINT i = 0; i < type->paramType.size(); i++)
		paramList.push_back(TakeLastNode());

	if(funcInfo && funcInfo->type == FunctionInfo::THISCALL)
		second = TakeLastNode();
}
NodeFuncCall::~NodeFuncCall()
{
}

void NodeFuncCall::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	// Если имеются параметры, найдём их значения
	UINT currParam = 0;
	
	if(funcInfo && funcInfo->address == -1 && funcInfo->funcPtr != NULL)
	{
		std::list<shared_ptr<NodeZeroOP> >::iterator s, e;
		s = paramList.begin();
		e = paramList.end();
		for(; s != e; s++)
		{
			// Определим значение параметра
			(*s)->Compile();
			// Преобразуем его в тип входного параметра функции
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[currParam]->type]);
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
			ConvertFirstToSecond(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[currParam]->type]);
			currParam++;
		}
	}
	if(funcInfo && funcInfo->address == -1)		// Если функция встроенная
	{
		// Вызовем по имени
		cmdList->AddData(cmdCallStd);
		cmdList->AddData(funcInfo);
	}else{					// Если функция определена пользователем
		// Перенесём в локальные параметры прямо тут, фигле
		UINT addr = 0;
		for(int i = int(funcType->paramType.size())-1; i >= 0; i--)
		{
			asmStackType newST = podTypeToStackType[funcType->paramType[i]->type];
			asmDataType newDT = podTypeToDataType[funcType->paramType[i]->type];
			cmdList->AddData(cmdMov);
			cmdList->AddData((USHORT)(newST | newDT | bitAddrRelTop));
			// адрес начала массива
			cmdList->AddData(addr);
			addr += funcType->paramType[i]->size;
			if(newST == STYPE_COMPLEX_TYPE)
				cmdList->AddData(funcType->paramType[i]->size);

			cmdList->AddData(cmdPop);
			cmdList->AddData((USHORT)(newST));
			if(newST == STYPE_COMPLEX_TYPE)
				cmdList->AddData(funcType->paramType[i]->size);
		}

		if(!funcInfo || second)
		{
			if(second)
				second->Compile();
			else
				first->Compile();
			cmdList->AddData(cmdMov);
			cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT | bitAddrRelTop));
			cmdList->AddData(addr);

			cmdList->AddData(cmdPop);
			cmdList->AddData((USHORT)(STYPE_INT));
		}

		cmdList->AddData(cmdPushVTop);

		// Надём, сколько занимают все переменные
		UINT allSize = 0;
		for(UINT i = 0; i < funcType->paramType.size(); i++)
			allSize += funcType->paramType[i]->size;

		// Расширим стек переменные на это значение
		cmdList->AddData(cmdPushV);
		cmdList->AddData(allSize + 4);//(funcInfo ? (funcInfo->type == FunctionInfo::LOCAL ? 4 : 0) : 4));

		// Вызовем по адресу
		cmdList->AddData(cmdCall);
		cmdList->AddData(funcInfo ? funcInfo->address : -1);
		cmdList->AddData((USHORT)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[podTypeToStackType[typeInfo->type]])));
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
UINT NodeFuncCall::GetSize()
{
	UINT size = 0;
	if(!funcInfo || second)
	{
		if(second)
			size += second->GetSize();
		else
			size += first->GetSize();
		size += 2*sizeof(CmdID) + sizeof(UINT) + 2*sizeof(USHORT);
	}

	UINT currParam = 0;
	for(paramPtr s = paramList.rbegin(), e = paramList.rend(); s != e; s++)
	{
		size += (*s)->GetSize();
		size += ConvertFirstToSecondSize(podTypeToStackType[(*s)->GetTypeInfo()->type], podTypeToStackType[funcType->paramType[currParam]->type]);
		currParam++;
	}
	
	if(funcInfo && funcInfo->address == -1)
	{
		size += sizeof(CmdID) + sizeof(funcInfo);
	}else{
		size += 3*sizeof(CmdID) + 2*sizeof(UINT) + sizeof(USHORT) + (UINT)(funcType->paramType.size()) * (2*sizeof(CmdID)+2+4+2);
		for(int i = int(funcType->paramType.size())-1; i >= 0; i--)
		{
			if(funcType->paramType[i]->type == TypeInfo::TYPE_COMPLEX)
				size += 2*sizeof(UINT);
		}
	}

	return size;
}

//////////////////////////////////////////////////////////////////////////
// Новый узел для получения значения переменной
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
	UINT startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	if(absAddress)
	{
		cmdList->AddData(cmdPushImmt);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT));
		// адрес начала массива
		cmdList->AddData(varAddress);
	}else{
		cmdList->AddData(cmdGetAddr);
		// относительный адрес переменной
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

UINT NodeGetAddress::GetSize()
{
	if(absAddress)
		return sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	// else
	return sizeof(CmdID) + sizeof(UINT);
}

TypeInfo* NodeGetAddress::GetTypeInfo()
{
	return GetReferenceType(typeInfo);
}

//////////////////////////////////////////////////////////////////////////
// Узел для присвоения значения переменной

NodeVariableSet::NodeVariableSet(TypeInfo* targetType, UINT pushVar, bool swapNodes)
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

	// Если идёт первое определение переменной и массиву присваивается базовый тип
	arrSetAll = (bytesToPush && typeInfo->arrLevel != 0 && second->GetTypeInfo()->arrLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && second->GetTypeInfo()->type != TypeInfo::TYPE_COMPLEX);

	if(second->GetTypeInfo() == typeVoid)
		throw std::string("ERROR: cannot convert from void to " + typeInfo->GetTypeName());
	if(typeInfo == typeVoid)
		throw std::string("ERROR: cannot convert from " + second->GetTypeInfo()->GetTypeName() + " to void");

	// Если типы не равны
	if(second->GetTypeInfo() != typeInfo)
	{
		// Если это не встроенные базовые типы, или
		// если различаются размерности массивов, и при этом не происходит первое определение переменной, или
		// если различается глубина указателей, или
		// если это указатель, глубина указателей равна, но при этом тип, на который указывает указатель отличается, то
		// сообщим об ошибке несоответствия типов
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
	UINT startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	asmStackType asmST = podTypeToStackType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)];
	asmDataType asmDT = podTypeToDataType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)];
	
	if(bytesToPush)
	{
		cmdList->AddData(cmdPushV);
		cmdList->AddData(bytesToPush);
	}

	second->Compile();
	ConvertFirstToSecond(podTypeToStackType[second->GetTypeInfo()->type], asmST);

	if(!knownAddress)
		first->Compile();
	if(arrSetAll)
	{
		assert(knownAddress);
		cmdList->AddData(cmdSetRange);
		cmdList->AddData((USHORT)(podTypeToDataType[typeInfo->subType->type]));
		cmdList->AddData(addrShift);
		cmdList->AddData(typeInfo->size / typeInfo->subType->size);
	}else{
		cmdList->AddData(cmdMov);
		cmdList->AddData((USHORT)(asmST | asmDT | (absAddress ? bitAddrAbs : bitAddrRel) | (knownAddress ? 0 : bitShiftStk)));
		// адрес начала массива
		cmdList->AddData(addrShift);
		if(typeInfo->type == TypeInfo::TYPE_COMPLEX)
			cmdList->AddData(typeInfo->size);
	}

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}

void NodeVariableSet::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "VariableSet " << (arrSetAll ? "set all elements" : "") << " pushv: " << bytesToPush << "\r\n";
	GoDown();
	first->LogToStream(ostr);
	GoUp();
	GoDownB();
	second->LogToStream(ostr);
	GoUp();
}

UINT NodeVariableSet::GetSize()
{
	UINT size = second->GetSize();
	if(!knownAddress)
		size += first->GetSize();
	if(bytesToPush)
		size += sizeof(CmdID) + sizeof(UINT);
	size += ConvertFirstToSecondSize(podTypeToStackType[second->GetTypeInfo()->type], podTypeToStackType[(arrSetAll ? typeInfo->subType->type : typeInfo->type)]);
	if(arrSetAll)
		size += sizeof(CmdID) + sizeof(USHORT) + 2*sizeof(UINT);
	else
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT) + (typeInfo->type == TypeInfo::TYPE_COMPLEX ? sizeof(UINT) : 0);
	return size;
}

TypeInfo* NodeVariableSet::GetTypeInfo()
{
	return (arrSetAll ? typeInfo->subType : typeInfo);
}

//////////////////////////////////////////////////////////////////////////
// Узел для получения элемента массива
NodeArrayIndex::NodeArrayIndex(TypeInfo* parentType)
{
	assert(parentType);
	typeParent = parentType;
	typeInfo = GetReferenceType(parentType->subType);

	// получить узел, расчитывающий индекс
	second = TakeLastNode();

	// получить узел, расчитывающий адрес начала массива
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
	UINT startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	asmOperType oAsmType = operTypeForStackType[podTypeToStackType[second->GetTypeInfo()->type]];

	// Возьмём указатель на начало массива
	first->Compile();

	if(knownShift)
	{
		cmdList->AddData(cmdPushImmt);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT));
		// адрес начала массива
		cmdList->AddData(shiftValue);
	}else{
		// Вычислим индекс
		second->Compile();
		// Переведём его в целое число
		cmdList->AddData(cmdCTI);
		// Передадим тип операнда
		cmdList->AddData((UCHAR)(oAsmType));
		// Умножив на размер элемента (сдвиг должен быть в байтах)
		cmdList->AddData(typeParent->subType->size);
	}
	// Сложим с адресом, который был на вершине
	cmdList->AddData(cmdAdd);
	// Передадим тип операнда
	cmdList->AddData((UCHAR)(OTYPE_INT));

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

UINT NodeArrayIndex::GetSize()
{
	if(knownShift)
		return first->GetSize() + 2*sizeof(CmdID) + sizeof(USHORT) + sizeof(UCHAR) + sizeof(UINT);
	// else
	return first->GetSize() + second->GetSize() + 2*sizeof(CmdID) + 2*sizeof(UCHAR) + sizeof(UINT);
}

TypeInfo* NodeArrayIndex::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// Узел для взятия значения по указателю

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
	UINT startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	asmStackType asmST = podTypeToStackType[typeInfo->type];
	asmDataType asmDT = podTypeToDataType[typeInfo->type];
	
	if(!knownAddress)
		first->Compile();

	cmdList->AddData(cmdPush);
	cmdList->AddData((USHORT)(asmST | asmDT | (absAddress ? bitAddrAbs : bitAddrRel) | (knownAddress ? 0 : bitShiftStk)));
	// адрес начала массива
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

UINT NodeDereference::GetSize()
{
	return (!knownAddress ? first->GetSize() : 0) + sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT) + (typeInfo->type == TypeInfo::TYPE_COMPLEX ? sizeof(UINT) : 0);
}

TypeInfo* NodeDereference::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// Узел сдвигающий адрес до члена класса
NodeShiftAddress::NodeShiftAddress(UINT shift, TypeInfo* resType)
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
	UINT startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	first->Compile();

	if(memberShift)
	{
		cmdList->AddData(cmdPushImmt);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT));
		// сдвиг до члена типа
		cmdList->AddData(memberShift);

		// Сложим с адресом, который был на вершине
		cmdList->AddData(cmdAdd);
		cmdList->AddData((UCHAR)(OTYPE_INT));
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

UINT NodeShiftAddress::GetSize()
{
	UINT retSize = first->GetSize();
	if(memberShift)
		retSize += 2*sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT) + sizeof(UCHAR);
	return retSize;
}

TypeInfo* NodeShiftAddress::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// Узел для постинкримента или постдекримента
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
	UINT startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	asmDataType asmDT = podTypeToDataType[typeInfo->type];
	
	if(!knownAddress)
		first->Compile();

	// Меняем значение переменной прямо по адресу
	cmdList->AddData(cmdID);
	cmdList->AddData((USHORT)(asmDT | (optimised ? 0 : (prefixOp ? bitPushAfter : bitPushBefore)) | (absAddress ? bitAddrAbs : bitAddrRel) | (knownAddress ? 0 : bitShiftStk)));
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

UINT NodePreOrPostOp::GetSize()
{
	return (!knownAddress ? first->GetSize() : 0) + sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
}

TypeInfo* NodePreOrPostOp::GetTypeInfo()
{
	return typeInfo;
}

//////////////////////////////////////////////////////////////////////////
// Узел, получающий адрес функции

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
	UINT startCmdSize = cmdList->GetCurrPos();
	if(strBegin && strEnd)
		cmdList->AddDescription(cmdList->GetCurrPos(), strBegin, strEnd);

	cmdList->AddData(cmdFuncAddr);
	cmdList->AddData(funcInfo);

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		cmdList->AddData(cmdPushImmt);
		cmdList->AddData((USHORT)(STYPE_INT | DTYPE_INT));
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

UINT NodeFunctionAddress::GetSize()
{
	UINT size = sizeof(CmdID) + sizeof(FunctionInfo*);
	if(funcInfo->type == FunctionInfo::NORMAL)
		size += sizeof(CmdID) + sizeof(USHORT) + sizeof(UINT);
	else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL)
		size += first->GetSize();
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
	breakAddr.push_back(cmdList->GetCurrPos()+4+third->GetSize()+fourth->GetSize()+2+4);
	cmdList->AddData(breakAddr.back());

	// Сохраним адрес для перехода к следующей операции оператором continue;
	continueAddr.push_back(cmdList->GetCurrPos()+fourth->GetSize());

	// Выполним содержимое цикла
	fourth->Compile();
	// Выполним операцию, проводимую после каждой итерации
	third->Compile();
	// Перейдём на проверку условия
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
	breakAddr.push_back(cmdList->GetCurrPos()+4+second->GetSize()+2+4);
	cmdList->AddData(breakAddr.back());

	// Сохраним адрес для перехода к следующей операции оператором continue;
	continueAddr.push_back(cmdList->GetCurrPos()+second->GetSize());

	// Выполним содержимое цикла
	second->Compile();
	// Перейдём на проверку условия
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
	breakAddr.push_back(cmdList->GetCurrPos()+first->GetSize()+second->GetSize()+2+4);

	// Сохраним адрес для перехода к следующей операции оператором continue;
	continueAddr.push_back(cmdList->GetCurrPos()+first->GetSize());

	// Выполним содержимое цикла
	first->Compile();
	// Выполним условие
	second->Compile();
	// Если условие верно, перейдём к выполнению следующей итерации цикла
	cmdList->AddData(cmdJmpNZ);
	cmdList->AddData((UCHAR)(aOT));
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
	cmdList->AddData(breakAddr.back());

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeBreakOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "BreakExpression\r\n";
}
UINT NodeBreakOp::GetSize()
{
	return (1+popCnt)*sizeof(CmdID) + sizeof(UINT);
}

//////////////////////////////////////////////////////////////////////////
// Узел, производящий досрочный переход к следующей итерации цикла

NodeContinueOp::NodeContinueOp(UINT c)
{
	// Сколько значений нужно убрать со стека вершин стека переменных (о_О)
	popCnt = c;
}
NodeContinueOp::~NodeContinueOp()
{
}

void NodeContinueOp::Compile()
{
	UINT startCmdSize = cmdList->GetCurrPos();

	// Уберём значения со стека вершин стека переменных
	for(UINT i = 0; i < popCnt; i++)
		cmdList->AddData(cmdPopVTop);

	// Выйдем из цикла
	cmdList->AddData(cmdJmp);
	cmdList->AddData(continueAddr.back());

	assert((cmdList->GetCurrPos()-startCmdSize) == GetSize());
}
void NodeContinueOp::LogToStream(ostringstream& ostr)
{
	DrawLine(ostr);
	ostr << *typeInfo << "ContinueOp\r\n";
}
UINT NodeContinueOp::GetSize()
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
	UINT blockNum = 0;
	for(casePtr s = caseBlockList.begin(), e = caseBlockList.end(); s != e; s++, blockNum++)
		switchEnd += (*s)->GetSize() + sizeof(CmdID) + sizeof(USHORT) + (blockNum != caseBlockList.size()-1 ? sizeof(CmdID) + sizeof(UINT) : 0);

	// Сохраним адрес для оператора break;
	breakAddr.push_back(switchEnd+2);

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
UINT NodeSwitchExpr::GetSize()
{
	UINT size = 0;
	size += first->GetSize();
	for(casePtr s = caseCondList.begin(), e = caseCondList.end(); s != e; s++)
		size += (*s)->GetSize();
	UINT blockNum = 0;
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

shared_ptr<NodeZeroOP> NodeExpressionList::GetFirstNode()
{
	if(exprList.empty())
		throw std::string("NodeExpressionList::GetLastNode() List is empty");
	return *(exprList.begin());
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
