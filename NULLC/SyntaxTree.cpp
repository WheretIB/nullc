#include "stdafx.h"
#include "SyntaxTree.h"

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

FastVector<unsigned int>	breakAddr(64);
FastVector<unsigned int>	continueAddr(64);

static char* binCommandToText[] = { "+", "-", "*", "/", "^", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "bin.and", "bin.or", "bin.xor", "log.and", "log.or", "log.xor"};

//////////////////////////////////////////////////////////////////////////

int	level = 0;
char	linePrefix[256];
unsigned int prefixSize = 2;

bool preNeedChange = false;
void GoDown()
{
	level++;
	prefixSize -= 2;
	linePrefix[prefixSize] = 0;
	sprintf(linePrefix + prefixSize, "  |__");
	prefixSize += 5;
}
void GoDownB()
{
	GoDown();
	preNeedChange = true;
}
void GoUp()
{
	level--;
	prefixSize -= 5;
	linePrefix[prefixSize] = 0;
	sprintf(linePrefix + prefixSize, "__");
	prefixSize += 2;
}
void DrawLine(FILE *fGraph)
{
	fprintf(fGraph, "%s", linePrefix);
	if(preNeedChange)
	{
		preNeedChange = false;
		GoUp();
		level++;

		prefixSize -= 2;
		linePrefix[prefixSize] = 0;
		sprintf(linePrefix + prefixSize, "   __");
		prefixSize += 5;
	}
}

//Functions for work with types

//This function converts a type according to result type of binary operation between types 'first' and 'second'
//For example,  int * double = double, so first operand will be transformed to double
//				double * int = double, no transformations
asmStackType	ConvertFirstForSecond(asmStackType first, asmStackType second)
{
	if(first == STYPE_INT && second == STYPE_DOUBLE)
	{
		cmdList.push_back(VMCmd(cmdItoD));
		return second;
	}
	if(first == STYPE_LONG && second == STYPE_DOUBLE)
	{
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

unsigned int ConvertFirstForSecondSize(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
		return 1;
	if(first == STYPE_INT && second == STYPE_LONG)
		return 1;
	return 0;
}
asmStackType	ConvertFirstForSecondType(asmStackType first, asmStackType second)
{
	if((first == STYPE_INT || first == STYPE_LONG) && second == STYPE_DOUBLE)
		return STYPE_DOUBLE;
	if(first == STYPE_INT && second == STYPE_LONG)
		return STYPE_LONG;
	return first;
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
// Узел не имеющий дочерних узлов

ChunkedStackPool<4092>	NodeZeroOP::nodePool;

NodeZeroOP::NodeZeroOP()
{
	typeInfo = typeVoid;
	strBegin = strEnd = NULL;
	prev = next = NULL;
	codeSize = 0;
	nodeType = typeNodeZeroOp;
}
NodeZeroOP::NodeZeroOP(TypeInfo* tinfo)
{
	typeInfo = tinfo;
	strBegin = strEnd = NULL;
	prev = next = NULL;
	codeSize = 0;
	nodeType = typeNodeZeroOp;
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
	fprintf(fGraph, "%s ZeroOp\r\n", typeInfo->GetFullTypeName());
}

void NodeZeroOP::SetCodeInfo(const char* start, const char* end)
{
	assert(end >= start);
	strBegin = start;
	strEnd = end;
}
//////////////////////////////////////////////////////////////////////////
// Узел, имеющий один дочерний узел
NodeOneOP::NodeOneOP()
{
	first = NULL;
	nodeType = typeNodeOneOp;
}
NodeOneOP::~NodeOneOP()
{
}

void NodeOneOP::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	first->Compile();

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeOneOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s OneOP :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, имеющий два дочерних узла
NodeTwoOP::NodeTwoOP()
{
	second = NULL;
	nodeType = typeNodeTwoOp;
}
NodeTwoOP::~NodeTwoOP()
{
}

void NodeTwoOP::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	NodeOneOP::Compile();
	second->Compile();

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeTwoOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s TwoOp :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, имеющий три дочерних узла
NodeThreeOP::NodeThreeOP()
{
	third = NULL;
	nodeType = typeNodeThreeOp;
}
NodeThreeOP::~NodeThreeOP()
{
}

void NodeThreeOP::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	NodeTwoOP::Compile();
	third->Compile();

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeThreeOP::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ThreeOp :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	third->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел который кладёт число в стек
void NodeNumber::Compile()
{
	if(codeSize == 2)
		cmdList.push_back(VMCmd(cmdPushImmt, quad.high));
	cmdList.push_back(VMCmd(cmdPushImmt, quad.low));
}
void NodeNumber::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Number\r\n", typeInfo->GetFullTypeName());
}

bool NodeNumber::ConvertTo(TypeInfo *target)
{
	if(target == typeInt)
	{
		integer = GetInteger();
		codeSize = 1;
	}else if(target == typeDouble || target == typeFloat){
		real = GetDouble();
		codeSize = 2;
	}else if(target == typeLong){
		integer64 = GetLong();
		codeSize = 2;
	}else{
		return false;
	}
	typeInfo = target;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Узел, убирающий с вершины стека значение, оставленное дочерним узлом
NodePopOp::NodePopOp()
{
	first = TakeLastNode();
	codeSize = first->codeSize;
	if(first->typeInfo != typeVoid)
		codeSize += 1;
	nodeType = typeNodePopOp;
}
NodePopOp::~NodePopOp()
{
}

void NodePopOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	// Даём дочернему узлу вычислить значение
	first->Compile();
	if(first->typeInfo != typeVoid)
	{
		// Убираем его с вершины стека
		cmdList.push_back(VMCmd(cmdPop, first->typeInfo->type == TypeInfo::TYPE_COMPLEX ? first->typeInfo->size : stackTypeSize[first->typeInfo->stackType]));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodePopOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s PopOp :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, производящий выбраную унарную операцию над значением на вершине стека
NodeUnaryOp::NodeUnaryOp(CmdID cmd)
{
	// Унарная операция
	cmdID = cmd;

	first = TakeLastNode();
	// Тип результата такой же, как исходный
	typeInfo = first->typeInfo;

	codeSize = first->codeSize + 1;
	nodeType = typeNodeUnaryOp;
}
NodeUnaryOp::~NodeUnaryOp()
{
}

void NodeUnaryOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	asmOperType aOT = operTypeForStackType[first->typeInfo->stackType];

	// Даём дочернему узлу вычислить значение
	first->Compile();
	// Выполним команду
	if(aOT == OTYPE_INT)
		cmdList.push_back(VMCmd((InstructionCode)cmdID));
	else if(aOT == OTYPE_DOUBLE)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID + 6)));
	else
		cmdList.push_back(VMCmd((InstructionCode)(cmdID + 3)));

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeUnaryOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s UnaryOp :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, выполняющий возврат из функции или из программы
NodeReturnOp::NodeReturnOp(unsigned int c, TypeInfo* tinfo)
{
	// Сколько значений нужно убрать со стека вершин стека переменных (о_О)
	popCnt = c;
	// Тип результата предоставлен извне
	typeInfo = tinfo;

	first = TakeLastNode();

	codeSize = first->codeSize + 1 + (typeInfo ? ConvertFirstToSecondSize(first->typeInfo->stackType, typeInfo->stackType) : 0);
	nodeType = typeNodeReturnOp;
}
NodeReturnOp::~NodeReturnOp()
{
}

void NodeReturnOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	// Найдём значение, которое будем возвращать
	first->Compile();
	// Преобразуем его в тип возвратного значения функции
	if(typeInfo)
		ConvertFirstToSecond(first->typeInfo->stackType, typeInfo->stackType);

	// Выйдем из функции или программы
	TypeInfo *retType = typeInfo ? typeInfo : first->typeInfo;
	asmOperType operType = operTypeForStackType[retType->stackType];

	if(retType->type == TypeInfo::TYPE_COMPLEX || retType->type == TypeInfo::TYPE_VOID)
		cmdList.push_back(VMCmd(cmdReturn, 0, (unsigned short)retType->size, popCnt));
	else
		cmdList.push_back(VMCmd(cmdReturn, 0, (unsigned short)(bitRetSimple | operType), popCnt));

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeReturnOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	if(typeInfo)
		fprintf(fGraph, "%s ReturnOp :\r\n", typeInfo->GetFullTypeName());
	else
		fprintf(fGraph, "%s ReturnOp :\r\n", first->typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
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

	codeSize = first->codeSize;
	nodeType = typeNodeExpression;
}
NodeExpression::~NodeExpression()
{
}

void NodeExpression::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	NodeOneOP::Compile();

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeExpression::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Expression :\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел c содержимым блока {}
NodeBlock::NodeBlock(unsigned int varShift, bool postPop)
{
	first = TakeLastNode();

	shift = varShift;
	popAfter = postPop;

	codeSize = first->codeSize + (popAfter ? 2 : 1) + (shift ? 1 : 0);
	nodeType = typeNodeBlock;
}
NodeBlock::~NodeBlock()
{
}

void NodeBlock::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Сохраним значение вершины стека переменных
	cmdList.push_back(VMCmd(cmdPushVTop));
	if(shift)
		cmdList.push_back(VMCmd(cmdPushV, shift));
	// Выполним содержимое блока (то же что first->Compile())
	first->Compile();
	// Востановим значение вершины стека переменных
	if(popAfter)
		cmdList.push_back(VMCmd(cmdPopVTop));

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeBlock::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Block (%d)\r\n", typeInfo->GetFullTypeName(), shift);
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////

NodeFuncDef::NodeFuncDef(FunctionInfo *info)
{
	// Структура описания функции
	funcInfo = info;

	disabled = false;

	first = TakeLastNode();

	codeSize = first->codeSize + 1;
	nodeType = typeNodeFuncDef;
}
NodeFuncDef::~NodeFuncDef()
{
}

void NodeFuncDef::Disable()
{
	codeSize = 0;
	disabled = true;
}

void NodeFuncDef::Compile()
{
	if(disabled)
		return;
	unsigned int startCmdSize = cmdList.size();

	funcInfo->address = cmdList.size();
	// Сгенерируем код функции
	first->Compile();

	if(funcInfo->retType == typeVoid)
	{
		cmdList.push_back(VMCmd(cmdReturn, 0, 0, 1));
		// Если функция не возвращает значения, то это пустой ret
	}else{
		// Остановим программу с ошибкой
		cmdList.push_back(VMCmd(cmdReturn, bitRetError, 0, 1));
	}

	funcInfo->codeSize = cmdList.size() - funcInfo->address;

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeFuncDef::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncDef %s %s\r\n", typeInfo->GetFullTypeName(), funcInfo->name, (disabled ? " disabled" : ""));
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
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
	typeInfo = funcType->retType;

	if(funcInfo && funcInfo->type == FunctionInfo::LOCAL)
		second = TakeLastNode();

	if(!funcInfo)
		first = TakeLastNode();

	if(funcType->paramCount > 0)
		paramHead = paramTail = TakeLastNode();
	else
		paramHead = paramTail = NULL;

	codeSize = 0;
	bool onlyStackTypes = true;
	TypeInfo	**paramType = funcType->paramType;
	if(*paramType == typeChar || *paramType == typeShort || *paramType == typeFloat)
		onlyStackTypes = false;
	if(funcInfo && funcInfo->address == -1 && funcInfo->funcPtr != NULL && *paramType == typeFloat)
		codeSize += 1;

	// Возьмём узлы каждого параметра
	for(unsigned int i = 1; i < funcType->paramCount; i++)
	{
		paramType++;
		if(*paramType == typeChar || *paramType == typeShort || *paramType == typeFloat)
			onlyStackTypes = false;
		if(funcInfo && funcInfo->address == -1 && funcInfo->funcPtr != NULL && *paramType == typeFloat)
			codeSize += 1;
		paramTail->next = TakeLastNode();
		paramTail->next->prev = paramTail;
		paramTail = paramTail->next;
	}

	if(funcInfo && funcInfo->type == FunctionInfo::THISCALL)
		second = TakeLastNode();
	
	unsigned int paramSize = ((!funcInfo || second) ? 4 : 0);

	if(funcType->paramCount > 0)
	{
		NodeZeroOP	*curr = paramTail;
		TypeInfo	**paramType = funcType->paramType;
		do
		{
			paramSize += (*paramType)->size;

			codeSize += curr->codeSize;
			codeSize += ConvertFirstToSecondSize(curr->typeInfo->stackType, (*paramType)->stackType);
			curr = curr->prev;
			paramType++;
		}while(curr);
	}
	if(!funcInfo || second)
	{
		if(second)
			codeSize += second->codeSize;
		else
			codeSize += first->codeSize;
		if(!onlyStackTypes)
			codeSize += 1;
	}
	
	if(funcInfo && funcInfo->address == -1)
	{
		codeSize += 1;
	}else{
		if(onlyStackTypes)
			codeSize += (paramSize ? 3 : 1);
		else
			codeSize += (paramSize ? 2 : 1) + (unsigned int)(funcType->paramCount);
	}
	nodeType = typeNodeFuncCall;
}
NodeFuncCall::~NodeFuncCall()
{
}

void NodeFuncCall::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Если имеются параметры, найдём их значения
	bool onlyStackTypes = true;
	if(funcInfo && funcInfo->address == -1 && funcInfo->funcPtr != NULL)
	{
		if(funcType->paramCount > 0)
		{
			NodeZeroOP	*curr = paramHead;
			TypeInfo	**paramType = funcType->paramType + funcType->paramCount - 1;
			do
			{
				// Определим значение параметра
				curr->Compile();
				// Преобразуем его в тип входного параметра функции
				ConvertFirstToSecond(curr->typeInfo->stackType, (*paramType)->stackType);
				if(*paramType == typeFloat)
					cmdList.push_back(VMCmd(cmdDtoF));
				curr = curr->next;
				paramType--;
			}while(curr);
		}
	}else{
		if(!funcInfo || second)
		{
			if(second)
				second->Compile();
			else
				first->Compile();
		}
		if(funcType->paramCount > 0)
		{
			NodeZeroOP	*curr = paramTail;
			TypeInfo	**paramType = funcType->paramType;
			do
			{
				// Определим значение параметра
				curr->Compile();
				// Преобразуем его в тип входного параметра функции
				ConvertFirstToSecond(curr->typeInfo->stackType, (*paramType)->stackType);
				if(*paramType == typeChar || *paramType == typeShort || *paramType == typeFloat)
					onlyStackTypes = false;
				curr = curr->prev;
				paramType++;
			}while(curr);
		}
	}
	if(funcInfo && funcInfo->address == -1)		// Если функция встроенная
	{
		// Вызовем по имени
		unsigned int ID = GetFuncIndexByPtr(funcInfo);
		cmdList.push_back(VMCmd(cmdCallStd, ID));
	}else{					// Если функция определена пользователем
		// Перенесём в локальные параметры прямо тут, фигле
		unsigned int paramSize = 0;
		for(unsigned int i = 0; i < funcType->paramCount; i++)
			paramSize += funcType->paramType[i]->size;
		paramSize += ((!funcInfo || second) ? 4 : 0);
		if(paramSize)
			cmdList.push_back(VMCmd(cmdReserveV, paramSize));

		unsigned int addr = 0;
		if(!onlyStackTypes)
		{
			for(int i = funcType->paramCount-1; i >= 0; i--)
			{
				asmDataType newDT = funcType->paramType[i]->dataType;
				cmdList.push_back(VMCmd(cmdPopTypeTop[newDT>>2], newDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)funcType->paramType[i]->size, addr));
				addr += funcType->paramType[i]->size;
			}
		}

		if(!funcInfo || second)
		{
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


		// Вызовем по адресу
		unsigned int ID = GetFuncIndexByPtr(funcInfo);
		unsigned short helper = (unsigned short)((typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->type == TypeInfo::TYPE_VOID) ? typeInfo->size : (bitRetSimple | operTypeForStackType[typeInfo->stackType]));
		cmdList.push_back(VMCmd(cmdCall, helper, funcInfo ? ID : -1));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeFuncCall::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FuncCall '%s' %d\r\n", typeInfo->GetFullTypeName(), (funcInfo ? funcInfo->name : "$ptr"), funcType->paramCount);
	GoDown();
	if(first)
		first->LogToStream(fGraph);
	if(second)
		second->LogToStream(fGraph);
	NodeZeroOP	*curr = paramTail;
	while(curr)
	{
		if(curr == paramHead)
		{
			GoUp();
			GoDownB();
		}
		curr->LogToStream(fGraph);
		curr = curr->prev;
	}
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Новый узел для получения значения переменной
NodeGetAddress::NodeGetAddress(VariableInfo* vInfo, int vAddress, bool absAddr, TypeInfo *retInfo)
{
	assert(retInfo);

	varInfo = vInfo;
	varAddress = vAddress;
	absAddress = absAddr;

	typeOrig = retInfo;
	typeInfo = GetReferenceType(typeOrig);

	codeSize = 1;
	nodeType = typeNodeGetAddress;
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
	assert(typeOrig->arrLevel != 0);
	varAddress += typeOrig->subType->size * shift;
	typeOrig = typeOrig->subType;
	typeInfo = GetReferenceType(typeOrig);
}

void NodeGetAddress::ShiftToMember(TypeInfo::MemberVariable *member)
{
	assert(member);
	varAddress += member->offset;
	typeOrig = member->type;
	typeInfo = GetReferenceType(typeOrig);
}

void NodeGetAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	if(absAddress)
		cmdList.push_back(VMCmd(cmdPushImmt, varAddress));
	else
		cmdList.push_back(VMCmd(cmdGetAddr, varAddress));

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeGetAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s GetAddress ", typeInfo->GetFullTypeName());
	if(varInfo)
		fprintf(fGraph, "%s%s '%.*s'", (varInfo->isConst ? "const " : ""), varInfo->varType->GetFullTypeName(), varInfo->name.end-varInfo->name.begin, varInfo->name.begin);
	else
		fprintf(fGraph, "$$$");
	fprintf(fGraph, " (%d %s)\r\n", (int)varAddress, (absAddress ? " absolute" : " relative"));
}

//////////////////////////////////////////////////////////////////////////
// Узел для присвоения значения переменной

NodeVariableSet::NodeVariableSet(TypeInfo* targetType, unsigned int pushVar, bool swapNodes)
{
	assert(targetType);
	typeInfo = targetType;

	if(swapNodes)
		second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	if(!swapNodes)
		second = TakeLastNode();

	// Если идёт первое определение переменной и массиву присваивается базовый тип
	arrSetAll = (pushVar && typeInfo->arrLevel != 0 && second->typeInfo->arrLevel == 0 && typeInfo->subType->type != TypeInfo::TYPE_COMPLEX && second->typeInfo->type != TypeInfo::TYPE_COMPLEX);

	if(second->typeInfo == typeVoid)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: cannot convert from void to %s", typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}
	if(typeInfo == typeVoid)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: cannot convert from %s to void", second->typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}

	if(second->nodeType == typeNodeNumber)
		static_cast<NodeNumber*>(second)->ConvertTo(typeInfo);

	// Если типы не равны
	if(second->typeInfo != typeInfo)
	{
		// Если это не встроенные базовые типы, или
		// если различаются размерности массивов, и при этом не происходит первое определение переменной, или
		// если различается глубина указателей, или
		// если это указатель, глубина указателей равна, но при этом тип, на который указывает указатель отличается, то
		// сообщим об ошибке несоответствия типов
		if(!(typeInfo->type != TypeInfo::TYPE_COMPLEX && second->typeInfo->type != TypeInfo::TYPE_COMPLEX) ||
			(typeInfo->arrLevel != second->typeInfo->arrLevel && !arrSetAll) ||
			(typeInfo->refLevel != second->typeInfo->refLevel) ||
			(typeInfo->refLevel && typeInfo->refLevel == second->typeInfo->refLevel && typeInfo->subType != second->typeInfo->subType))
		{
			if(!(typeInfo->arrLevel != 0 && second->typeInfo->arrLevel == 0 && arrSetAll))
			{
				char	errBuf[128];
				_snprintf(errBuf, 128, "ERROR: Cannot convert '%s' to '%s'", second->typeInfo->GetFullTypeName(), typeInfo->GetFullTypeName());
				lastError = CompilerError(errBuf, lastKnownStartPos);
				return;
			}
		}
	}

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
	}

	if(arrSetAll)
	{
		elemCount = typeInfo->size / typeInfo->subType->size;
		typeInfo = typeInfo->subType;
	}

	codeSize = second->codeSize;
	if(!knownAddress)
		codeSize += first->codeSize;
	codeSize += ConvertFirstToSecondSize(second->typeInfo->stackType, typeInfo->stackType);
	if(arrSetAll)
		codeSize += 2;
	else
		codeSize += 1;
	nodeType = typeNodeVariableSet;
}

NodeVariableSet::~NodeVariableSet()
{
}


void NodeVariableSet::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	asmStackType asmST = typeInfo->stackType;
	asmDataType asmDT = typeInfo->dataType;

	second->Compile();
	ConvertFirstToSecond(second->typeInfo->stackType, asmST);

	if(!knownAddress)
		first->Compile();
	if(arrSetAll)
	{
		assert(knownAddress);
		cmdList.push_back(VMCmd(cmdPushImmt, elemCount));
		cmdList.push_back(VMCmd(cmdSetRange, (unsigned short)(asmDT), addrShift));
	}else{
		if(knownAddress)
		{
			cmdList.push_back(VMCmd(cmdMovType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
		}else{
			cmdList.push_back(VMCmd(cmdMovTypeStk[asmDT>>2], asmST == STYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
		}
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeVariableSet::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s VariableSet %s\r\n", typeInfo->GetFullTypeName(), (arrSetAll ? "set all elements" : ""));
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел для изменения значения переменной (операции += -= *= /= и т.п.)

NodeVariableModify::NodeVariableModify(TypeInfo* targetType, CmdID cmd)
{
	assert(targetType);
	typeInfo = targetType;

	cmdID = cmd;

	second = TakeLastNode();

	// Address of the target variable
	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	if(second->typeInfo == typeVoid)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: cannot convert from void to %s", typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}
	if(typeInfo == typeVoid)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: cannot convert from %s to void", second->typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}

	// Если типы не равны
	if(second->typeInfo != typeInfo)
	{
		// Если это не встроенные базовые типы, или
		// если различается глубина указателей, или
		// если это указатель, глубина указателей равна, но при этом тип, на который указывает указатель отличается, то
		// сообщим об ошибке несоответствия типов
		if(!(typeInfo->type != TypeInfo::TYPE_COMPLEX && second->typeInfo->type != TypeInfo::TYPE_COMPLEX) ||
			(typeInfo->arrLevel != second->typeInfo->arrLevel) ||
			(typeInfo->refLevel != second->typeInfo->refLevel) ||
			(typeInfo->refLevel && typeInfo->refLevel == second->typeInfo->refLevel && typeInfo->subType != second->typeInfo->subType))
		{
			char	errBuf[128];
			_snprintf(errBuf, 128, "ERROR: Cannot convert '%s' to '%s'", second->typeInfo->GetFullTypeName(), typeInfo->GetFullTypeName());
			lastError = CompilerError(errBuf, lastKnownStartPos);
			return;
		}
	}

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
	}

	asmStackType asmSTfirst = typeInfo->stackType;
	asmStackType asmSTsecond = second->typeInfo->stackType;

	codeSize = second->codeSize;
	if(!knownAddress)
		codeSize += 2 * first->codeSize;
	codeSize += ConvertFirstForSecondSize(asmSTfirst, asmSTsecond);
	asmStackType asmSTresult = ConvertFirstForSecondType(asmSTfirst, asmSTsecond);
	codeSize += ConvertFirstForSecondSize(asmSTsecond, asmSTresult);
	codeSize += ConvertFirstToSecondSize(asmSTresult, asmSTfirst);
	codeSize += 3;
	nodeType = typeNodeVariableModify;
}

NodeVariableModify::~NodeVariableModify()
{
}

void NodeVariableModify::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	asmStackType asmSTfirst = typeInfo->stackType;
	asmDataType asmDT = typeInfo->dataType;

	asmStackType asmSTsecond = second->typeInfo->stackType;

	// Если надо, расчитаем адрес первого операнда
	if(!knownAddress)
		first->Compile();

	// И положим его в стек
	if(knownAddress)
	{
		cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}

	// Преобразуем, если надо, в тип, который получается после проведения выбранной операции
	asmStackType asmSTresult = ConvertFirstForSecond(asmSTfirst, asmSTsecond);

	// Оперделим второй операнд
	second->Compile();

	// Преобразуем, если надо, в тип, который получается после проведения выбранной операции
	ConvertFirstForSecond(asmSTsecond, asmSTresult);

	// Произведём операцию со значениями
	if(asmSTresult == STYPE_INT)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID)));
	else if(asmSTresult == STYPE_LONG)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddL)));
	else if(asmSTresult == STYPE_DOUBLE)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddD)));
	else
		assert(!"unknown operator type in NodeVariableModify");

	// Преобразуем результат в тип первого операнда
	ConvertFirstToSecond(asmSTresult, asmSTfirst);

	// Если надо, расчитаем адрес первого операнда
	if(!knownAddress)
		first->Compile();

	// И запишем новое значение переменной
	if(knownAddress)
	{
		cmdList.push_back(VMCmd(cmdMovType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdMovTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeVariableModify::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s VariableModify\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
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

	if(second->nodeType == typeNodeNumber)
	{
		shiftValue = typeParent->subType->size * static_cast<NodeNumber*>(second)->GetInteger();
		knownShift = true;
	}

	if(knownShift)
		codeSize = first->codeSize + 2;
	else
		codeSize = first->codeSize + second->codeSize + 1 + (typeParent->subType->size == 1 && second->typeInfo->stackType == STYPE_INT ? 0 : 1);
	nodeType = typeNodeArrayIndex;
}

NodeArrayIndex::~NodeArrayIndex()
{
}

void NodeArrayIndex::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	asmOperType oAsmType = operTypeForStackType[second->typeInfo->stackType];

	// Возьмём указатель на начало массива
	first->Compile();

	if(knownShift)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, shiftValue));
	}else{
		// Вычислим индекс
		second->Compile();
		// Переведём его в целое число.  Умножив на размер элемента
		if(typeParent->subType->size != 1)
		{
			cmdList.push_back(VMCmd(cmdImmtMulType[oAsmType], typeParent->subType->size));
		}else{
			if(oAsmType != OTYPE_INT)
				cmdList.push_back(VMCmd(oAsmType == OTYPE_DOUBLE ? cmdDtoI : cmdLtoI));
		}

	}
	// Сложим с адресом, который был на вершине
	cmdList.push_back(VMCmd(cmdAdd));

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeArrayIndex::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ArrayIndex %s known: %d shiftval: %d\r\n", typeInfo->GetFullTypeName(), typeParent->GetFullTypeName(), knownShift, shiftValue);
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел для взятия значения по указателю

NodeDereference::NodeDereference(TypeInfo* type)
{
	assert(type);
	typeInfo = type;

	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
	}

	codeSize = (!knownAddress ? first->codeSize : 0) + 1;
	nodeType = typeNodeDereference;
}

NodeDereference::~NodeDereference()
{
}


void NodeDereference::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	asmDataType asmDT = typeInfo->dataType;
	
	if(!knownAddress)
		first->Compile();

	if(knownAddress)
	{
		cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
	}else{
		cmdList.push_back(VMCmd(cmdPushTypeStk[asmDT>>2], asmDT == DTYPE_DOUBLE ? 1 : 0, (unsigned short)typeInfo->size, addrShift));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeDereference::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s Dereference\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел сдвигающий адрес до члена класса
NodeShiftAddress::NodeShiftAddress(unsigned int shift, TypeInfo* resType)
{
	memberShift = shift;
	typeInfo = GetReferenceType(resType);

	first = TakeLastNode();

	codeSize = first->codeSize;
	if(memberShift)
		codeSize += 2;
	nodeType = typeNodeShiftAddress;
}

NodeShiftAddress::~NodeShiftAddress()
{
}


void NodeShiftAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	first->Compile();

	if(memberShift)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, memberShift));
		// Сложим с адресом, который был на вершине
		cmdList.push_back(VMCmd(cmdAdd));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeShiftAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ShiftAddress\r\n", typeInfo->GetFullTypeName());
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел для постинкримента или постдекримента
NodePreOrPostOp::NodePreOrPostOp(TypeInfo* resType, bool isInc, bool preOp)
{
	assert(resType);
	typeInfo = resType;

	first = TakeLastNode();
	assert(first->typeInfo->refLevel != 0);

	incOp = isInc;

	if(typeInfo->type == TypeInfo::TYPE_COMPLEX || typeInfo->refLevel != 0)
	{
		char	errBuf[128];
		_snprintf(errBuf, 128, "ERROR: %s is not supported on '%s'", (isInc ? "Increment" : "Decrement"), typeInfo->GetFullTypeName());
		lastError = CompilerError(errBuf, lastKnownStartPos);
		return;
	}

	prefixOp = preOp;

	optimised = false;

	absAddress = true;
	knownAddress = false;
	addrShift = 0;

	if(first->nodeType == typeNodeGetAddress)
	{
		absAddress = static_cast<NodeGetAddress*>(first)->IsAbsoluteAddress();
		addrShift = static_cast<NodeGetAddress*>(first)->varAddress;
		knownAddress = true;
	}
	if(first->nodeType == typeNodeShiftAddress)
	{
		addrShift = static_cast<NodeShiftAddress*>(first)->memberShift;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeShiftAddress*>(first)->first;
		static_cast<NodeShiftAddress*>(oldFirst)->first = NULL;
	}
	if(first->nodeType == typeNodeArrayIndex && static_cast<NodeArrayIndex*>(first)->knownShift)
	{
		addrShift = static_cast<NodeArrayIndex*>(first)->shiftValue;
		NodeZeroOP	*oldFirst = first;
		first = static_cast<NodeArrayIndex*>(first)->first;
		static_cast<NodeArrayIndex*>(oldFirst)->first = NULL;
	}

	codeSize = (!knownAddress ? first->codeSize : 0);
	if(knownAddress)
	{
		codeSize += 3;
		if(!prefixOp)
			codeSize++;
	}else{
		codeSize++;
	}
	nodeType = typeNodePreOrPostOp;
}

NodePreOrPostOp::~NodePreOrPostOp()
{
}


void NodePreOrPostOp::SetOptimised(bool doOptimisation)
{
	if(prefixOp)
	{
		if(!optimised && doOptimisation)
			codeSize++;
		if(optimised && !doOptimisation)
			codeSize--;
	}
	optimised = doOptimisation;
}


void NodePreOrPostOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	asmStackType asmST = typeInfo->stackType;
	asmDataType asmDT = typeInfo->dataType;
	asmOperType aOT = operTypeForStackType[typeInfo->stackType];
	
	if(!knownAddress)
		first->Compile();

	if(knownAddress)
	{
		cmdList.push_back(VMCmd(cmdPushType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
		cmdList.push_back(VMCmd(incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
		cmdList.push_back(VMCmd(cmdMovType[asmDT>>2], absAddress ? ADDRESS_ABOLUTE : ADDRESS_RELATIVE, (unsigned short)typeInfo->size, addrShift));
		if(!prefixOp && !optimised)
			cmdList.push_back(VMCmd(!incOp ? cmdIncType[aOT] : cmdDecType[aOT]));
		if(optimised)
			cmdList.push_back(VMCmd(cmdPop, stackTypeSize[asmST]));
	}else{
		cmdList.push_back(VMCmd(cmdAddAtTypeStk[asmDT>>2], optimised ? 0 : (prefixOp ? bitPushAfter : bitPushBefore), incOp ? 1 : -1, addrShift));
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodePreOrPostOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s PreOrPostOp %s\r\n", typeInfo->GetFullTypeName(), (prefixOp ? "prefix" : "postfix"));
	GoDownB();
	first->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, получающий адрес функции

NodeFunctionAddress::NodeFunctionAddress(FunctionInfo* functionInfo)
{
	funcInfo = functionInfo;
	typeInfo = funcInfo->funcType;

	if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL)
		first = TakeLastNode();

	codeSize = 1;
	if(funcInfo->type == FunctionInfo::NORMAL)
		codeSize += 1;
	else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL)
		codeSize += first->codeSize;
	nodeType = typeNodeFunctionAddress;
}

NodeFunctionAddress::~NodeFunctionAddress()
{
}


void NodeFunctionAddress::Compile()
{
	unsigned int startCmdSize = cmdList.size();
	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	unsigned int ID = GetFuncIndexByPtr(funcInfo);
	cmdList.push_back(VMCmd(cmdFuncAddr, ID));

	if(funcInfo->type == FunctionInfo::NORMAL)
	{
		cmdList.push_back(VMCmd(cmdPushImmt, 0));
	}else if(funcInfo->type == FunctionInfo::LOCAL || funcInfo->type == FunctionInfo::THISCALL){
		first->Compile();
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}

void NodeFunctionAddress::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s FunctionAddress %s %s\r\n", typeInfo->GetFullTypeName(), funcInfo->name, (funcInfo->funcPtr ? " external" : ""));
	if(first)
	{
		GoDownB();
		first->LogToStream(fGraph);
		GoUp();
	}
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
	if(first->typeInfo->refLevel == 0)
	{
		if(first->typeInfo->type == TypeInfo::TYPE_COMPLEX || second->typeInfo->type == TypeInfo::TYPE_COMPLEX)
		{
			char	errBuf[128];
			_snprintf(errBuf, 128, "ERROR: Operation %s is not supported on '%s' and '%s'", binCommandToText[cmdID - cmdAdd], first->typeInfo->GetFullTypeName(), second->typeInfo->GetFullTypeName());
			lastError = CompilerError(errBuf, lastKnownStartPos);
			return;
		}
	}
	if(first->typeInfo == typeVoid)
	{
		lastError = CompilerError("ERROR: first operator returns void", lastKnownStartPos);
		return;
	}
	if(second->typeInfo == typeVoid)
	{
		lastError = CompilerError("ERROR: second operator returns void", lastKnownStartPos);
		return;
	}

	// Найдём результирующий тип, после проведения операции
	typeInfo = ChooseBinaryOpResultType(first->typeInfo, second->typeInfo);

	asmStackType fST = first->typeInfo->stackType, sST = second->typeInfo->stackType;
	codeSize = ConvertFirstForSecondSize(fST, sST);
	fST = ConvertFirstForSecondType(fST, sST);
	codeSize += ConvertFirstForSecondSize(sST, fST);
	codeSize += first->codeSize + second->codeSize + 1;
	nodeType = typeNodeTwoAndCmdOp;
}
NodeTwoAndCmdOp::~NodeTwoAndCmdOp()
{
}

void NodeTwoAndCmdOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	asmStackType fST = first->typeInfo->stackType, sST = second->typeInfo->stackType;
	
	// Найдём первое значение
	first->Compile();
	// Преобразуем, если надо, в тип, который получается после проведения выбранной операции
	fST = ConvertFirstForSecond(fST, sST);
	// Найдём второе значение
	second->Compile();
	// Преобразуем, если надо, в тип, который получается после проведения выбранной операции
	sST = ConvertFirstForSecond(sST, fST);
	// Произведём операцию со значениями
	if(fST == STYPE_INT)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID)));
	else if(fST == STYPE_LONG)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddL)));
	else if(fST == STYPE_DOUBLE)
		cmdList.push_back(VMCmd((InstructionCode)(cmdID - cmdAdd + cmdAddD)));
	else
		assert(!"unknown operator type in NodeTwoAndCmdOp");

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeTwoAndCmdOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s TwoAndCmd<%s> :\r\n", typeInfo->GetFullTypeName(), binCommandToText[cmdID-cmdAdd]);
	assert(cmdID >= cmdAdd);
	assert(cmdID <= cmdNEqualD);
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
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
	if(isTerm)
		typeInfo = second->typeInfo != third->typeInfo ? ChooseBinaryOpResultType(second->typeInfo, third->typeInfo) : second->typeInfo;

	codeSize = first->codeSize + second->codeSize + 1;
	if(third)
		codeSize += third->codeSize + 1 + (second->typeInfo != third->typeInfo ? 1 : 0);
	nodeType = typeNodeIfElseExpr;
}
NodeIfElseExpr::~NodeIfElseExpr()
{
}

void NodeIfElseExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	// Структура дочерних элементов: if(first) second; else third;
	// Второй вариант: first ? second : third;
	asmOperType aOT = operTypeForStackType[first->typeInfo->stackType];
	// Вычислим условие
	first->Compile();

	// Если false, перейдём в блок else или выйдем из оператора, если такого блока не имеется
	cmdList.push_back(VMCmd(cmdJmpZType[aOT], 1 + cmdList.size() + second->codeSize + (third ? 1 + ConvertFirstForSecondSize(second->typeInfo->stackType, third->typeInfo->stackType) : 0)));

	// Выполним блок для успешного прохождения условия (true)
	second->Compile();
	if(typeInfo != typeVoid)
		ConvertFirstForSecond(second->typeInfo->stackType, third->typeInfo->stackType);
	// Если есть блок else, выполним его
	if(third)
	{
		// Только поставим выход из оператора перед его кодом, чтобы не выполнять обе ветви
		cmdList.push_back(VMCmd(cmdJmp, 1 + cmdList.size() + third->codeSize + ConvertFirstForSecondSize(third->typeInfo->stackType, second->typeInfo->stackType)));

		// Выполним блок else (false)
		third->Compile();
		if(typeInfo != typeVoid)
			ConvertFirstForSecond(third->typeInfo->stackType, second->typeInfo->stackType);
	}

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeIfElseExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s IfExpression :\r\n", typeInfo->GetFullTypeName());
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

//////////////////////////////////////////////////////////////////////////
// Узел, выполняющий блок for(){}
NodeForExpr::NodeForExpr()
{
	fourth = TakeLastNode();
	third = TakeLastNode();
	second = TakeLastNode();
	first = TakeLastNode();

	codeSize = first->codeSize + second->codeSize + third->codeSize + fourth->codeSize + 2;
	nodeType = typeNodeForExpr;
}
NodeForExpr::~NodeForExpr()
{
}

void NodeForExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	if(strBegin && strEnd)
		cmdInfoList.AddDescription(cmdList.size(), strBegin, strEnd);

	// Структура дочерних элементов: for(first, second, third) fourth;
	asmOperType aOT = operTypeForStackType[second->typeInfo->stackType];

	// Выполним инициализацию
	first->Compile();
	unsigned int posTestExpr = cmdList.size();

	// Найдём результат условия
	second->Compile();

	// Сохраним адрес для выхода из цикла оператором break;
	breakAddr.push_back(cmdList.size() + 1 + third->codeSize + fourth->codeSize + 1);

	// Если ложно, выйдем из цикла
	cmdList.push_back(VMCmd(cmdJmpZType[aOT], breakAddr.back()));

	// Сохраним адрес для перехода к следующей операции оператором continue;
	continueAddr.push_back(cmdList.size()+fourth->codeSize);

	// Выполним содержимое цикла
	fourth->Compile();
	// Выполним операцию, проводимую после каждой итерации
	third->Compile();
	// Перейдём на проверку условия
	cmdList.push_back(VMCmd(cmdJmp, posTestExpr));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeForExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ForExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	second->LogToStream(fGraph);
	third->LogToStream(fGraph);
	GoUp();
	GoDownB(); 
	fourth->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, выполняющий блок while(){}
NodeWhileExpr::NodeWhileExpr()
{
	second = TakeLastNode();
	first = TakeLastNode();

	codeSize = first->codeSize + second->codeSize + 2;
	nodeType = typeNodeWhileExpr;
}
NodeWhileExpr::~NodeWhileExpr()
{
}

void NodeWhileExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Структура дочерних элементов: while(first) second;
	asmOperType aOT = operTypeForStackType[first->typeInfo->stackType];

	unsigned int posStart = cmdList.size();
	// Выполним условие
	first->Compile();

	// Сохраним адрес для выхода из цикла оператором break;
	breakAddr.push_back(cmdList.size() + 1 + second->codeSize + 1);

	// Если оно ложно, выйдем из цикла
	cmdList.push_back(VMCmd(cmdJmpZType[aOT], breakAddr.back()));

	// Сохраним адрес для перехода к следующей операции оператором continue;
	continueAddr.push_back(cmdList.size() + second->codeSize);

	// Выполним содержимое цикла
	second->Compile();
	// Перейдём на проверку условия
	cmdList.push_back(VMCmd(cmdJmp, posStart));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeWhileExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s WhileExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB(); 
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, выполняющий блок do{}while()
NodeDoWhileExpr::NodeDoWhileExpr()
{
	second = TakeLastNode();
	first = TakeLastNode();

	codeSize = first->codeSize + second->codeSize + 1;
	nodeType = typeNodeDoWhileExpr;
}
NodeDoWhileExpr::~NodeDoWhileExpr()
{
}

void NodeDoWhileExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Структура дочерних элементов: do{ first; }while(second)
	asmOperType aOT = operTypeForStackType[second->typeInfo->stackType];

	unsigned int posStart = cmdList.size();
	// Сохраним адрес для выхода из цикла оператором break;
	breakAddr.push_back(cmdList.size() + first->codeSize + second->codeSize + 1);

	// Сохраним адрес для перехода к следующей операции оператором continue;
	continueAddr.push_back(cmdList.size() + first->codeSize);

	// Выполним содержимое цикла
	first->Compile();
	// Выполним условие
	second->Compile();
	// Если условие верно, перейдём к выполнению следующей итерации цикла
	cmdList.push_back(VMCmd(cmdJmpNZType[aOT], posStart));

	breakAddr.pop_back();
	continueAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeDoWhileExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s DoWhileExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	GoUp();
	GoDownB();
	second->LogToStream(fGraph);
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, производящий операцию break;
NodeBreakOp::NodeBreakOp(unsigned int c)
{
	// Сколько значений нужно убрать со стека вершин стека переменных (о_О)
	popCnt = c;

	codeSize = 1 + popCnt;
	nodeType = typeNodeBreakOp;
}
NodeBreakOp::~NodeBreakOp()
{
}

void NodeBreakOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Уберём значения со стека вершин стека переменных
	for(unsigned int i = 0; i < popCnt; i++)
		cmdList.push_back(VMCmd(cmdPopVTop));
	// Выйдем из цикла
	cmdList.push_back(VMCmd(cmdJmp, breakAddr.back()));

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeBreakOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s BreakExpression\r\n", typeInfo->GetFullTypeName());
}

//////////////////////////////////////////////////////////////////////////
// Узел, производящий досрочный переход к следующей итерации цикла

NodeContinueOp::NodeContinueOp(unsigned int c)
{
	// Сколько значений нужно убрать со стека вершин стека переменных (о_О)
	popCnt = c;

	codeSize = 1 + popCnt;
	nodeType = typeNodeContinueOp;
}
NodeContinueOp::~NodeContinueOp()
{
}

void NodeContinueOp::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	// Уберём значения со стека вершин стека переменных
	for(unsigned int i = 0; i < popCnt; i++)
		cmdList.push_back(VMCmd(cmdPopVTop));

	// Выйдем из цикла
	cmdList.push_back(VMCmd(cmdJmp, continueAddr.back()));

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeContinueOp::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s ContinueOp\r\n", typeInfo->GetFullTypeName());
}

//////////////////////////////////////////////////////////////////////////
// Узел, определяющий код для switch
NodeSwitchExpr::NodeSwitchExpr()
{
	// Возьмём узел с условием
	first = TakeLastNode();
	conditionHead = conditionTail = NULL;
	blockHead = blockTail = NULL;
	caseCount = 0;

	codeSize = first->codeSize - 1;
	codeSize += 4;
	nodeType = typeNodeSwitchExpr;
}
NodeSwitchExpr::~NodeSwitchExpr()
{
}

void NodeSwitchExpr::AddCase()
{
	caseCount++;
	// Возьмём с верхушки блок
	if(blockTail)
	{
		blockTail->next = TakeLastNode();
		blockTail->next->prev = blockTail;
		blockTail = blockTail->next;
	}else{
		blockHead = blockTail = TakeLastNode();
	}
	// Возьмём условие для блока
	if(conditionTail)
	{
		conditionTail->next = TakeLastNode();
		conditionTail->next->prev = conditionTail;
		conditionTail = conditionTail->next;
	}else{
		conditionHead = conditionTail = TakeLastNode();
	}
	codeSize += conditionTail->codeSize;
	codeSize += blockTail->codeSize + 2;
	codeSize += 3;
}

void NodeSwitchExpr::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	asmStackType aST = first->typeInfo->stackType;
	asmOperType aOT = operTypeForStackType[aST];
	// Сохраним вершину стека переменных
	cmdList.push_back(VMCmd(cmdPushVTop));
	// Найдём значение по которому будем выбирать вариант кода
	first->Compile();

	NodeZeroOP *curr, *currBlock;

	// Найдём конец свитча
	unsigned int switchEnd = cmdList.size() + 2 + caseCount * 3;
	for(curr = conditionHead; curr; curr = curr->next)
		switchEnd += curr->codeSize;
	unsigned int condEnd = switchEnd;
	for(curr = blockHead; curr; curr = curr->next)
		switchEnd += curr->codeSize + 1 + (curr != blockTail ? 1 : 0);

	// Сохраним адрес для оператора break;
	breakAddr.push_back(switchEnd+1);

	// Сгенерируем код для всех case'ов
	unsigned int caseAddr = condEnd;
	for(curr = conditionHead, currBlock = blockHead; curr; curr = curr->next, currBlock = currBlock->next)
	{
		if(aOT == OTYPE_INT)
			cmdList.push_back(VMCmd(cmdCopyI));
		else
			cmdList.push_back(VMCmd(cmdCopyDorL));

		curr->Compile();
		// Сравним на равенство
		if(aOT == OTYPE_INT)
			cmdList.push_back(VMCmd(cmdEqual));
		else if(aOT == OTYPE_DOUBLE)
			cmdList.push_back(VMCmd(cmdEqualD));
		else
			cmdList.push_back(VMCmd(cmdEqualL));
		// Если равны, перейдём на нужный кейс
		cmdList.push_back(VMCmd(cmdJmpNZType[aOT], caseAddr));
		caseAddr += currBlock->codeSize + 2;
	}
	// Уберём с вершины стека значение по которому выбирался вариант кода
	cmdList.push_back(VMCmd(cmdPop, stackTypeSize[aST]));

	cmdList.push_back(VMCmd(cmdJmp, switchEnd));
	for(curr = blockHead; curr; curr = curr->next)
	{
		// Уберём с вершины стека значение по которому выбирался вариант кода
		cmdList.push_back(VMCmd(cmdPop, stackTypeSize[aST]));
		curr->Compile();
		if(curr != blockTail)
		{
			cmdList.push_back(VMCmd(cmdJmp, cmdList.size() + 2));
		}
	}

	// Востановим вершину стека значений
	cmdList.push_back(VMCmd(cmdPopVTop));

	breakAddr.pop_back();

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeSwitchExpr::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s SwitchExpression :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	first->LogToStream(fGraph);
	for(NodeZeroOP *curr = conditionHead, *block = blockHead; curr; curr = curr->next, block = block->next)
	{
		curr->LogToStream(fGraph);
		if(curr == conditionTail)
		{
			GoUp();
			GoDownB();
		}
		block->LogToStream(fGraph);
	}
	GoUp();
}

//////////////////////////////////////////////////////////////////////////
// Узел, содержащий список выражений.
NodeExpressionList::NodeExpressionList(TypeInfo *returnType)
{
	typeInfo = returnType;
	tail = first = TakeLastNode();

	codeSize = tail->codeSize;
	nodeType = typeNodeExpressionList;
}
NodeExpressionList::~NodeExpressionList()
{
}

void NodeExpressionList::AddNode(bool reverse)
{
	// If reverse is set, add before the head
	if(reverse)
	{
		NodeZeroOP *firstNext = first;
		first = TakeLastNode();
		codeSize += first->nodeType == typeNodeFuncDef ? 0 : first->codeSize;
		first->next = firstNext;
		first->next->prev = first;
	}else{
		tail->next = TakeLastNode();
		tail->next->prev = tail;
		tail = tail->next;
		codeSize += tail->nodeType == typeNodeFuncDef ? 0 : tail->codeSize;
	}
}

NodeZeroOP* NodeExpressionList::GetFirstNode()
{
	assert(first);
	return first;
}

void NodeExpressionList::Compile()
{
	unsigned int startCmdSize = cmdList.size();

	NodeZeroOP	*curr = first;
	do 
	{
		curr->Compile();
		curr = curr->next;
	}while(curr);

	assert((cmdList.size()-startCmdSize) == codeSize);
}
void NodeExpressionList::LogToStream(FILE *fGraph)
{
	DrawLine(fGraph);
	fprintf(fGraph, "%s NodeExpressionList :\r\n", typeInfo->GetFullTypeName());
	GoDown();
	NodeZeroOP	*curr = first;
	do 
	{
		if(curr == tail)
		{
			GoUp();
			GoDownB();
		}
		curr->LogToStream(fGraph);
		curr = curr->next;
	}while(curr);
	GoUp();
}
