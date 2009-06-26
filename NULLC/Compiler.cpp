#include "stdafx.h"

#include "SupSpi/SupSpi.h"
using namespace supspi;

#include "CodeInfo.h"
using namespace CodeInfo;

#include "Compiler.h"

#include <time.h>

//////////////////////////////////////////////////////////////////////////
//						Code gen ops
//////////////////////////////////////////////////////////////////////////
std::ostringstream		warningLog;

// Информация о вершинах стека переменных. При компиляции он служит для того, чтобы
// Удалять информацию о переменных, когда они выходят из области видимости
std::vector<VarTopInfo>		varInfoTop;
// Некоторые конструкции допускают оператор break, который должен знать, на сколько сдвинуть базу стека
// переменных, чтобы привести её в то состояние, в которым она находилась бы, если бы конструкция
// завершилась без преждевременного выхода. Этот стек (конструкции могут быть вложенными) хранит размер
// varInfoTop.
std::vector<UINT>			cycleBeginVarTop;
// Информация о количестве определённых функций на разных вложенностях блоков.
// Служит для того чтобы убирать функции по мере выхода из области видимости.
std::vector<UINT>			funcInfoTop;

// Немного предопределённых базовых типов
TypeInfo*	typeVoid = NULL;
TypeInfo*	typeChar = NULL;
TypeInfo*	typeShort = NULL;
TypeInfo*	typeInt = NULL;
TypeInfo*	typeFloat = NULL;
TypeInfo*	typeLong = NULL;
TypeInfo*	typeDouble = NULL;
TypeInfo*	typeFile = NULL;

// Temp variables
// Временные переменные:
// Количество минусов перед переменной, вершина стека переменных
UINT negCount, varTop;

// Выравнивание переменной или класса в байтах
UINT currAlign;

// Была ли определена переменная (для addVarSetNode)
bool varDefined;

// Является ли текущая переменная константной
bool currValConst;

// Номер скрытой переменной - константного массива
UINT inplaceArrayNum;

// Для определения новых типов
TypeInfo *newType = NULL;

// Информация о типе текущей переменной
TypeInfo*	currType = NULL;
// Стек ( :) )такой информации
// Для конструкций arr[arr[i.a.b].y].x;
std::vector<TypeInfo*>	currTypes;

// Массив временных строк
std::vector<std::string>	strs;
// Стек с количеством переменных переданных функции.
// Стек для ситуаций вроде foo(1, bar(2, 3, 4), 5), когда нужно сохранить количество параметров,
// для каждой из функцией для проверки количества переданных параметров с тем, которое принимает функция
std::vector<UINT>			callArgCount;
// Стек, который хранит титы значений, которые возвращает функция.
// Функции можно определять одну в другой (BUG: 0004 может, нафиг не надо?)
std::vector<TypeInfo*>		retTypeStack;
std::vector<FunctionInfo*>	currDefinedFunc;

int AddFunctionExternal(FunctionInfo* func, std::string name)
{
	for(UINT i = 0; i < func->external.size(); i++)
		if(func->external[i] == name)
			return i;

	compileLog << "Function " << currDefinedFunc.back()->name << " uses external variable " << name << "\r\n";
	func->external.push_back(name);
	return (int)func->external.size()-1;
}

// Преобразовать строку в число типа long long
long long parseLongLong(const char* str)
{
	int len = 0;
	while(isdigit(str[len++]));
	int len2 = len -= 1;
	long long res = 0;
	while(len)
		res = res * 10L + (long long)(str[len2-len--] - '0');
	return res;
}

// Проверяет, явлеется ли идентификатор зарезервированным или уже занятым
void checkIfDeclared(const std::string& str)
{
	if(str == "if" || str == "else" || str == "for" || str == "while" || str == "var" || str == "func" || str == "return" || str=="switch" || str=="case")
	{
		std::string fullError = std::string("ERROR: The name '" + str + "' is reserved");
		throw CompilerError(fullError, lastKnownStartPos);
	}
	for(UINT i = 0; i < funcInfo.size(); i++)
	{
		if(funcInfo[i]->name == str && funcInfo[i]->visible)
		{
			std::string fullError = std::string("ERROR: Name '" + str + "' is already taken for a function");
			throw CompilerError(fullError, lastKnownStartPos);
		}
	}
}

// Вызывается в начале блока {}, чтобы сохранить количество определённых переменных, к которому можно
// будет вернутся после окончания блока.
void blockBegin(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
	funcInfoTop.push_back((UINT)funcInfo.size());
}
// Вызывается в конце блока {}, чтобы убрать информацию о переменных внутри блока, тем самым обеспечивая
// их выход из области видимости. Также уменьшает вершину стека переменных в байтах.
void blockEnd(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	unsigned int varFormerTop = varTop;
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{ 
		varTop -= varInfo.back()->varType->size;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();

	for(UINT i = funcInfoTop.back(); i < funcInfo.size(); i++)
		funcInfo[i]->visible = false;
	funcInfoTop.pop_back();

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBlock(varFormerTop-varTop)));
}

// Функции для добавления узлов с константными числами разных типов
template<typename T>
void addNumberNode(char const*s, char const*e);

template<> void addNumberNode<char>(char const*s, char const*e)
{
	(void)e;	// C4100
	char res = s[1];
	if(res == '\\')
	{
		if(s[2] == 'n')
			res = '\n';
		if(s[2] == 'r')
			res = '\r';
		if(s[2] == 't')
			res = '\t';
		if(s[2] == '0')
			res = '\0';
		if(s[2] == '\'')
			res = '\'';
		if(s[2] == '\\')
			res = '\\';
	}
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(res, typeChar)));
}

template<> void addNumberNode<int>(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(atoi(s), typeInt)));
}
template<> void addNumberNode<float>(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<float>((float)atof(s), typeFloat)));
}
template<> void addNumberNode<long long>(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<long long>(parseLongLong(s), typeLong)));
}
template<> void addNumberNode<double>(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<double>(atof(s), typeDouble)));
}

void addVoidNode(char const*s, char const*e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP));
}

void addHexInt(char const*s, char const*e)
{
	s += 2;
	if(int(e-s) > 16)
		throw CompilerError("ERROR: Overflow in hexademical constant", s);
	unsigned long long mult = 1;
	unsigned long long res = 0;
	for(const char *p = s; p < e; p++)
	{
		if(*p >= '0' && *p <= '9')
			res = res * mult + (*p - '0');
		else
			res = res * mult + (tolower(*p) - 'a' + 10);
		mult = 16;
	}
	if(int(e-s) <= 8)
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>((UINT)res, typeInt)));
	else
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<long long>(res, typeLong)));
}
// Функция для создания узла, который кладёт массив в стек
// Используется NodeExpressionList, что не является самым быстрым и красивым вариантом
// но зато не надо писать отдельный класс с одинаковыми действиями внутри.
void addStringNode(char const*s, char const*e)
{
	lastKnownStartPos = s;

	const char *curr = s+1, *end = e-1;
	if(end-curr > 64*1024)
		throw CompilerError("ERROR: strings can't have length larger that 65536", s);

	// Replace escape-sequences with special codes
	static char cleanBuf[65536];
	UINT len = 0;
	for(; curr < end; curr++, len++)
	{
		cleanBuf[len] = *curr;
		if(*curr == '\\')
		{
			curr++;
			if(*curr == 'n')
				cleanBuf[len] = '\n';
			if(*curr == 'r')
				cleanBuf[len] = '\r';
			if(*curr == 't')
				cleanBuf[len] = '\t';
			if(*curr == '0')
				cleanBuf[len] = '\0';
			if(*curr == '\'')
				cleanBuf[len] = '\'';
			if(*curr == '\"')
				cleanBuf[len] = '\"';
			if(*curr == '\\')
				cleanBuf[len] = '\\';
		}
	}

	curr = cleanBuf;
	end = cleanBuf+len;

	// Clean the padding
	*(int*)(cleanBuf+len) = 0;

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(len+1, typeInt)));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(GetArrayType(typeChar))));

	shared_ptr<NodeZeroOP> temp = nodeList.back();
	nodeList.pop_back();

	NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp.get());

	while(end-curr >= 4)
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(*(int*)(curr), typeInt)));
		arrayList->AddNode();
		curr += 4;
	}
	int num = *(int*)(curr);
	*((char*)(&num)+(end-curr)) = 0;
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(num, typeInt)));
	arrayList->AddNode();

	nodeList.push_back(temp);

	strs.pop_back();
}

// Функция для создания узла, который уберёт значение со стека переменных
// Узел заберёт к себе последний узел в списке.
void addPopNode(char const* s, char const* e)
{
	nodeList.back()->SetCodeInfo(s, e);
	// Если последний узел в списке - узел с цислом, уберём его
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
	{
		nodeList.pop_back();
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
	}else if((*(nodeList.end()-1))->GetNodeType() == typeNodePreOrPostOp){
		// Если последний узел, это переменная, которую уменьшают или увеличивают на 1, не используя в
		// далнейшем её значение, то можно произвести оптимизацию кода.
		static_cast<NodePreOrPostOp*>(nodeList.back().get())->SetOptimised(true);
	}else{
		// Иначе просто создадим узёл, как и планировали в начале
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePopOp()));
	}
}

// Функция для создания узла, которые поменяет знак значения в стеке
// Узел заберёт к себе последний узел в списке.
void addNegNode(char const* s, char const* e)
{
	(void)e;	// C4100
	// Если количество смен знака чётное, то результирующий знак не поменяется
	// и нам не стоит тратить время
	if(negCount % 2 == 0)
		return;
	// Если последний узел это число, то просто поменяем знак у константы
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
	{
		TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
		NodeZeroOP* zOP = (nodeList.end()-1)->get();
		shared_ptr<NodeZeroOP > Rd;
		if(aType == typeDouble)
		{
			Rd.reset(new NodeNumber<double>(-static_cast<NodeNumber<double>* >(zOP)->GetVal(), zOP->GetTypeInfo()));
		}else if(aType == typeFloat){
			Rd.reset(new NodeNumber<float>(-static_cast<NodeNumber<float>* >(zOP)->GetVal(), zOP->GetTypeInfo()));
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<long long>(-static_cast<NodeNumber<long long>* >(zOP)->GetVal(), zOP->GetTypeInfo()));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(-static_cast<NodeNumber<int>* >(zOP)->GetVal(), zOP->GetTypeInfo()));
		}else{
			std::string fullError = std::string("addNegNode() ERROR: unknown type ") + aType->name;
			throw CompilerError(fullError, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// Иначе просто создадим узёл, как и планировали в начале
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeUnaryOp(cmdNeg)));
	}
	// Сбросим значение смен знака на 0
	negCount = 0;
}

// Функция для создания узла, которые произведёт логическое отрицания над значением в стеке
// Узел заберёт к себе последний узел в списке.
void addLogNotNode(char const* s, char const* e)
{
	(void)e;	// C4100
	// Если последний узел в списке - число, то произведём действие во время копиляции
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
	{
		TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
		NodeZeroOP* zOP = (nodeList.end()-1)->get();
		shared_ptr<NodeZeroOP > Rd;
		if(aType == typeDouble)
		{
			Rd.reset(new NodeNumber<double>(static_cast<NodeNumber<double>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeFloat){
			Rd.reset(new NodeNumber<float>(static_cast<NodeNumber<float>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo()));
		}else{
			std::string fullError = std::string("addLogNotNode() ERROR: unknown type ") + aType->name;
			throw CompilerError(fullError, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// Иначе просто создадим узёл, как и планировали в начале
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeUnaryOp(cmdLogNot)));
	}
}
void addBitNotNode(char const* s, char const* e)
{
	(void)e;	// C4100
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
	{
		TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
		NodeZeroOP* zOP = (nodeList.end()-1)->get();
		shared_ptr<NodeZeroOP > Rd;
		if(aType == typeDouble)
		{
			throw CompilerError("ERROR: bitwise NOT cannot be used on floating point numbers", s);
		}else if(aType == typeFloat){
			throw CompilerError("ERROR: bitwise NOT cannot be used on floating point numbers", s);
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo()));
		}else{
			std::string fullError = std::string("addBitNotNode() ERROR: unknown type ") + aType->name;
			throw CompilerError(fullError, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeUnaryOp(cmdBitNot)));
	}
}

template<typename T>
T optDoOperation(CmdID cmd, T a, T b, bool swap = false)
{
	if(swap)
		std::swap(a, b);
	if(cmd == cmdAdd)
		return a + b;
	if(cmd == cmdSub)
		return a - b;
	if(cmd == cmdMul)
		return a * b;
	if(cmd == cmdDiv)
		return a / b;
	if(cmd == cmdPow)
		return (T)pow((double)a, (double)b);
	if(cmd == cmdLess)
		return a < b;
	if(cmd == cmdGreater)
		return a > b;
	if(cmd == cmdGEqual)
		return a >= b;
	if(cmd == cmdLEqual)
		return a <= b;
	if(cmd == cmdEqual)
		return a == b;
	if(cmd == cmdNEqual)
		return a != b;
	return optDoSpecial(cmd, a, b);
}
template<typename T>
T optDoSpecial(CmdID cmd, T a, T b)
{
	(void)cmd; (void)b; (void)a;	// C4100
	throw CompilerError("ERROR: optDoSpecial call with unknown type", lastKnownStartPos);
}
template<> int optDoSpecial<>(CmdID cmd, int a, int b)
{
	if(cmd == cmdShl)
		return a << b;
	if(cmd == cmdShr)
		return a >> b;
	if(cmd == cmdMod)
		return a % b;
	if(cmd == cmdBitAnd)
		return a & b;
	if(cmd == cmdBitXor)
		return a ^ b;
	if(cmd == cmdBitOr)
		return a | b;
	if(cmd == cmdLogAnd)
		return a && b;
	if(cmd == cmdLogXor)
		return !!a ^ !!b;
	if(cmd == cmdLogOr)
		return a || b;
	throw CompilerError("ERROR: optDoSpecial<int> call with unknown command", lastKnownStartPos);
}
template<> long long optDoSpecial<>(CmdID cmd, long long a, long long b)
{
	if(cmd == cmdShl)
		return a << b;
	if(cmd == cmdShr)
		return a >> b;
	if(cmd == cmdMod)
		return a % b;
	if(cmd == cmdBitAnd)
		return a & b;
	if(cmd == cmdBitXor)
		return a ^ b;
	if(cmd == cmdBitOr)
		return a | b;
	if(cmd == cmdLogAnd)
		return a && b;
	if(cmd == cmdLogXor)
		return !!a ^ !!b;
	if(cmd == cmdLogOr)
		return a || b;
	throw CompilerError("ERROR: optDoSpecial<long long> call with unknown command", lastKnownStartPos);
}
template<> double optDoSpecial<>(CmdID cmd, double a, double b)
{
	if(cmd == cmdShl)
		throw CompilerError("ERROR: optDoSpecial<double> call with << operation is illegal", lastKnownStartPos);
	if(cmd == cmdShr)
		throw CompilerError("ERROR: optDoSpecial<double> call with >> operation is illegal", lastKnownStartPos);
	if(cmd == cmdMod)
		return fmod(a,b);
	if(cmd >= cmdBitAnd && cmd <= cmdBitXor)
		throw CompilerError("ERROR: optDoSpecial<double> call with binary operation is illegal", lastKnownStartPos);
	if(cmd == cmdLogAnd)
		return (int)a && (int)b;
	if(cmd == cmdLogXor)
		return !!(int)a ^ !!(int)b;
	if(cmd == cmdLogOr)
		return (int)a || (int)b;
	throw CompilerError("ERROR: optDoSpecial<double> call with unknown command", lastKnownStartPos);
}

void popLastNodeCond(bool swap)
{
	if(swap)
	{
		shared_ptr<NodeZeroOP> temp = nodeList.back();
		nodeList.pop_back();
		nodeList.back() = temp;
	}else{
		nodeList.pop_back();
	}
}

void addTwoAndCmpNode(CmdID id)
{
	UINT aNodeType = (*(nodeList.end()-2))->GetNodeType();
	UINT bNodeType = (*(nodeList.end()-1))->GetNodeType();
	UINT shA = 2, shB = 1;	//Shifts to operand A and B in array
	TypeInfo *aType, *bType;

	if(aNodeType == typeNodeNumber && bNodeType == typeNodeNumber)
	{
		//If we have operation between two known numbers, we can optimize code by calculating the result in place
		aType = (*(nodeList.end()-2))->GetTypeInfo();
		bType = (*(nodeList.end()-1))->GetTypeInfo();

		//Swap operands, to reduce number of combinations
		if((aType == typeFloat || aType == typeLong || aType == typeInt) && bType == typeDouble)
			std::swap(shA, shB);
		if((aType == typeLong || aType == typeInt) && bType == typeFloat)
			std::swap(shA, shB);
		if(aType == typeInt && bType == typeLong)
			std::swap(shA, shB);

		bool swapOper = shA != 2;

		aType = (*(nodeList.end()-shA))->GetTypeInfo();
		bType = (*(nodeList.end()-shB))->GetTypeInfo();
		if(aType == typeDouble)
		{
			NodeNumber<double> *Ad = static_cast<NodeNumber<double>* >((nodeList.end()-shA)->get());
			shared_ptr<NodeNumber<double> > Rd;
			if(bType == typeDouble)
			{
				NodeNumber<double> *Bd = static_cast<NodeNumber<double>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), Bd->GetVal()), typeDouble));
			}else if(bType == typeFloat){
				NodeNumber<float> *Bd = static_cast<NodeNumber<float>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble));
			}else if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble));
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble));
			}
			nodeList.pop_back(); nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeFloat){
			NodeNumber<float> *Ad = static_cast<NodeNumber<float>* >((nodeList.end()-shA)->get());
			shared_ptr<NodeNumber<float> > Rd;
			if(bType == typeFloat){
				NodeNumber<float> *Bd = static_cast<NodeNumber<float>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), Bd->GetVal()), typeFloat));
			}else if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), (float)Bd->GetVal(), swapOper), typeFloat));
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), (float)Bd->GetVal(), swapOper), typeFloat));
			}
			nodeList.pop_back(); nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeLong){
			NodeNumber<long long> *Ad = static_cast<NodeNumber<long long>* >((nodeList.end()-shA)->get());
			shared_ptr<NodeNumber<long long> > Rd;
			if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<long long>(optDoOperation<long long>(id, Ad->GetVal(), Bd->GetVal()), typeLong));
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >((nodeList.end()-shB)->get());
				Rd.reset(new NodeNumber<long long>(optDoOperation<long long>(id, Ad->GetVal(), (long long)Bd->GetVal(), swapOper), typeLong));
			}
			nodeList.pop_back(); nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeInt){
			NodeNumber<int> *Ad = static_cast<NodeNumber<int>* >((nodeList.end()-shA)->get());
			shared_ptr<NodeNumber<int> > Rd;
			//bType is also int!
			NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >((nodeList.end()-shB)->get());
			Rd.reset(new NodeNumber<int>(optDoOperation<int>(id, Ad->GetVal(), Bd->GetVal()), typeInt));
			nodeList.pop_back(); nodeList.pop_back();
			nodeList.push_back(Rd);
		}
		return;	// Оптимизация удалась, выходим
	}
	if(aNodeType == typeNodeNumber || bNodeType == typeNodeNumber)
	{
		// Если один из узлов - число, то поменяем операторы местами так, чтобы узел с числом был в A
		if(bNodeType == typeNodeNumber)
		{
			std::swap(shA, shB);
			std::swap(aNodeType, bNodeType);
		}

		// Оптимизацию можно произвести, если второй операнд - typeNodeTwoAndCmdOp или typeNodeVarGet
		if(bNodeType != typeNodeTwoAndCmdOp && bNodeType != typeNodeDereference)
		{
			// Иначе, выходим без оптимизаций
			try
			{
				nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoAndCmdOp(id)));
			}catch(const std::string& str){
				throw CompilerError(str.c_str(), lastKnownStartPos);
			}
			return;
		}

		// Оптимизацию можно произвести, если число == 0 или число == 1
		bool success = false;
		bType = (*(nodeList.end()-shA))->GetTypeInfo();
		if(bType == typeDouble)
		{
			NodeNumber<double> *Ad = static_cast<NodeNumber<double>* >((nodeList.end()-shA)->get());
			if(Ad->GetVal() == 0.0 && id == cmdMul)
			{
				popLastNodeCond(shA == 1); // a*0.0 -> 0.0
				success = true;
			}
			if((Ad->GetVal() == 0.0 && id == cmdAdd) || (Ad->GetVal() == 1.0 && id == cmdMul))
			{
				popLastNodeCond(shA == 2); // a+0.0 -> a || a*1.0 -> a
				success = true;
			}
		}else if(bType == typeFloat){
			NodeNumber<float> *Ad = static_cast<NodeNumber<float>* >((nodeList.end()-shA)->get());
			if(Ad->GetVal() == 0.0f && id == cmdMul)
			{
				popLastNodeCond(shA == 1); // a*0.0f -> 0.0f
				success = true;
			}
			if((Ad->GetVal() == 0.0f && id == cmdAdd) || (Ad->GetVal() == 1.0f && id == cmdMul))
			{
				popLastNodeCond(shA == 2); // a+0.0f -> a || a*1.0f -> a
				success = true;
			}
		}else if(bType == typeLong){
			NodeNumber<long long> *Ad = static_cast<NodeNumber<long long>* >((nodeList.end()-shA)->get());
			if(Ad->GetVal() == 0 && id == cmdMul)
			{
				popLastNodeCond(shA == 1); // a*0L -> 0L
				success = true;
			}
			if((Ad->GetVal() == 0 && id == cmdAdd) || (Ad->GetVal() == 1 && id == cmdMul))
			{
				popLastNodeCond(shA == 2); // a+0L -> a || a*1L -> a
				success = true;
			}
		}else if(bType == typeInt){
			NodeNumber<int> *Ad = static_cast<NodeNumber<int>* >((nodeList.end()-shA)->get());
			if(Ad->GetVal() == 0 && id == cmdMul)
			{
				popLastNodeCond(shA == 1); // a*0 -> 0
				success = true;
			}
			if((Ad->GetVal() == 0 && id == cmdAdd) || (Ad->GetVal() == 1 && id == cmdMul))
			{
				popLastNodeCond(shA == 2); // a+0 -> a || a*1 -> a
				success = true;
			}
		}
		if(success)	// Оптимизация удалась, выходим сразу
			return;
	}
	// Оптимизации не удались, сделаем операцию полностью
	try
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoAndCmdOp(id)));
	}catch(const std::string& str){
		throw CompilerError(str.c_str(), lastKnownStartPos);
	}
}

template<CmdID cmd> void createTwoAndCmd(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;
	addTwoAndCmpNode(cmd);
}

typedef void (*ParseFuncPtr)(char const* s, char const* e);

static ParseFuncPtr addCmd(CmdID cmd)
{
	if(cmd == cmdAdd) return &createTwoAndCmd<cmdAdd>;
	if(cmd == cmdSub) return &createTwoAndCmd<cmdSub>;
	if(cmd == cmdMul) return &createTwoAndCmd<cmdMul>;
	if(cmd == cmdDiv) return &createTwoAndCmd<cmdDiv>;
	if(cmd == cmdPow) return &createTwoAndCmd<cmdPow>;
	if(cmd == cmdLess) return &createTwoAndCmd<cmdLess>;
	if(cmd == cmdGreater) return &createTwoAndCmd<cmdGreater>;
	if(cmd == cmdLEqual) return &createTwoAndCmd<cmdLEqual>;
	if(cmd == cmdGEqual) return &createTwoAndCmd<cmdGEqual>;
	if(cmd == cmdEqual) return &createTwoAndCmd<cmdEqual>;
	if(cmd == cmdNEqual) return &createTwoAndCmd<cmdNEqual>;
	if(cmd == cmdShl) return &createTwoAndCmd<cmdShl>;
	if(cmd == cmdShr) return &createTwoAndCmd<cmdShr>;
	if(cmd == cmdMod) return &createTwoAndCmd<cmdMod>;
	if(cmd == cmdBitAnd) return &createTwoAndCmd<cmdBitAnd>;
	if(cmd == cmdBitOr) return &createTwoAndCmd<cmdBitOr>;
	if(cmd == cmdBitXor) return &createTwoAndCmd<cmdBitXor>;
	if(cmd == cmdLogAnd) return &createTwoAndCmd<cmdLogAnd>;
	if(cmd == cmdLogOr) return &createTwoAndCmd<cmdLogOr>;
	if(cmd == cmdLogXor) return &createTwoAndCmd<cmdLogXor>;
	throw CompilerError("ERROR: addCmd call with unknown command", lastKnownStartPos);
}

void addReturnNode(char const* s, char const* e)
{
	int t = (int)varInfoTop.size();
	int c = 0;
	if(funcInfo.size() != 0)
	{
		while(t > (int)funcInfo.back()->vTopSize)
		{
			c++;
			t--;
		}
	}
	TypeInfo *realRetType = nodeList.back()->GetTypeInfo();
	if(retTypeStack.back() && (retTypeStack.back()->type == TypeInfo::TYPE_COMPLEX || realRetType->type == TypeInfo::TYPE_COMPLEX) && retTypeStack.back() != realRetType)
		throw CompilerError("ERROR: function returns " + retTypeStack.back()->GetTypeName() + " but supposed to return " + realRetType->GetTypeName(), s);
	if(retTypeStack.back() && retTypeStack.back()->type == TypeInfo::TYPE_VOID && realRetType != typeVoid)
		throw CompilerError("ERROR: function returning a value", s);
	if(retTypeStack.back() && retTypeStack.back() != typeVoid && realRetType == typeVoid)
		throw CompilerError("ERROR: funtion should return " + retTypeStack.back()->GetTypeName(), s);
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeReturnOp(c, retTypeStack.back())));
	nodeList.back()->SetCodeInfo(s, e);
}

void addBreakNode(char const* s, char const* e)
{
	(void)e;	// C4100
	if(cycleBeginVarTop.empty())
		throw CompilerError("ERROR: break used outside loop statements", s);
	int t = (int)varInfoTop.size();
	int c = 0;
	while(t > (int)cycleBeginVarTop.back())
	{
		c++;
		t--;
	}
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBreakOp(c)));
}

void AddContinueNode(char const* s, char const* e)
{
	(void)e;	// C4100
	if(cycleBeginVarTop.empty())
		throw CompilerError("ERROR: continue used outside loop statements", s);
	int t = (int)varInfoTop.size();
	int c = 0;
	while(t > (int)cycleBeginVarTop.back())
	{
		c++;
		t--;
	}
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeContinueOp(c)));
}

//Finds TypeInfo in a typeInfo list by name
void selType(char const* s, char const* e)
{
	string vType = std::string(s,e);
	if(vType == "auto")
	{
		currType = NULL;
		return;
	}
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(typeInfo[i]->name == vType)
		{
			currType = typeInfo[i];
			return;
		}
	}
	throw CompilerError("ERROR: Variable type '" + vType + "' is unknown\r\n", s);
}

void addTwoExprNode(char const* s, char const* e);

UINT	offsetBytes = 0;

#include <set>
std::set<VariableInfo*> varInfoAll;

void addVar(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;
	string vName = strs.back();

	for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		if(varInfo[i]->name == vName)
			throw CompilerError("ERROR: Name '" + vName + "' is already taken for a variable in current scope\r\n", s);
	checkIfDeclared(vName);

	if(currType && currType->size == TypeInfo::UNSIZED_ARRAY)
		throw CompilerError("ERROR: variable '" + vName + "' can't be an unfixed size array", s);
	if(currType && currType->size > 64*1024*1024)
		throw CompilerError("ERROR: variable '" + vName + "' has to big length (>64 Mb)", s);
	
	if((currType && currType->alignBytes != 0) || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
	{
		UINT activeAlign = currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : currType->alignBytes;
		if(activeAlign > 16)
			throw CompilerError("ERROR: alignment must me less than 16 bytes", s);
		if(activeAlign != 0 && varTop % activeAlign != 0)
		{
			UINT offset = activeAlign - (varTop % activeAlign);
			varTop += offset;
			offsetBytes += offset;
		}
	}
	varInfo.push_back(new VariableInfo(vName, varTop, currType, currValConst));
	varInfoAll.insert(varInfo.back());
	varDefined = true;
	if(currType)
		varTop += currType->size;
}

void addVarDefNode(char const* s, char const* e)
{
	(void)e;	// C4100
	assert(varDefined);
	if(!currType)
		throw CompilerError("ERROR: auto variable must be initialized in place of definition", s);
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarDef(strs.back())));
	varInfo.back()->dataReserved = true;
	varDefined = 0;
	offsetBytes = 0;
}

void pushType(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	currTypes.push_back(currType);
}

void popType(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	currTypes.pop_back();
}

void convertTypeToRef(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;
	if(!currType)
		throw CompilerError("ERROR: auto variable cannot have reference flag", s);
	currType = GetReferenceType(currType);
}

void convertTypeToArray(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;
	if(!currType)
		throw CompilerError("ERROR: cannot specify array size for auto variable", s);
	currType = GetArrayType(currType);
}

//////////////////////////////////////////////////////////////////////////
//					New functions for work with variables

void GetVariableType(char const* s, char const* e)
{
	int fID = -1;
	// Ищем переменную по имени
	int i = (int)varInfo.size()-1;
	string vName(s, e);
	while(i >= 0 && varInfo[i]->name != vName)
		i--;
	if(i == -1)
	{
		// Ищем функцию по имени
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->name == vName && funcInfo[k]->visible)
			{
				if(fID != -1)
					throw CompilerError("ERROR: there are more than one '" + vName + "' function, and the decision isn't clear", s);
				fID = k;
			}
		}
		if(fID == -1)
			throw CompilerError("ERROR: variable '" + vName + "' is not defined", s);
	}

	if(fID == -1)
		currType = varInfo[i]->varType;
	else
		currType = funcInfo[fID]->funcType;
}

bool sizeOfExpr = false;
void GetTypeSize(char const* s, char const* e)
{
	(void)e;	// C4100
	if(!sizeOfExpr && !currTypes.back())
		throw CompilerError("ERROR: sizeof(auto) is illegal", s);
	if(sizeOfExpr)
	{
		currTypes.back() = nodeList.back()->GetTypeInfo();
		nodeList.pop_back();
	}
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(currTypes.back()->size, typeInt)));

	sizeOfExpr = false;
}

void SetTypeOfLastNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	currType = nodeList.back()->GetTypeInfo();
	nodeList.pop_back();
}

void AddInplaceArray(char const* s, char const* e);
void AddInplaceFunction(char const* s, char const* e);
void AddDereferenceNode(char const* s, char const* e);
void AddArrayIndexNode(char const* s, char const* e);
void AddMemberAccessNode(char const* s, char const* e);
void addFuncCallNode(char const* s, char const* e);

// Функция для получения адреса переменной, имя которое передаётся в параметрах
void AddGetAddressNode(char const* s, char const* e)
{
	lastKnownStartPos = s;

	int fID = -1;
	// Ищем переменную по имени
	int i = (int)varInfo.size()-1;
	string vName(s, e);
	while(i >= 0 && varInfo[i]->name != vName)
		i--;
	if(i == -1)
	{
		// Ищем функцию по имени
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->name == vName && funcInfo[k]->visible)
			{
				if(fID != -1)
					throw CompilerError("ERROR: there are more than one '" + vName + "' function, and the decision isn't clear", s);
				fID = k;
			}
		}
		if(fID == -1)
			throw CompilerError("ERROR: variable '" + vName + "' is not defined", s);
	}
	// Кладём в стек типов её тип
	if(fID == -1)
		currTypes.push_back(varInfo[i]->varType);
	else
		currTypes.push_back(funcInfo[fID]->funcType);

	if(newType && (currDefinedFunc.back()->type == FunctionInfo::THISCALL) && vName != "this")
	{
		bool member = false;
		for(UINT i = 0; i < newType->memberData.size(); i++)
			if(newType->memberData[i].name == vName)
				member = true;
		if(member)
		{
			// Переменные типа адресуются через указатель this
			std::string bName = "this";

			FunctionInfo *currFunc = currDefinedFunc.back();

			TypeInfo *temp = GetReferenceType(newType);
			currTypes.push_back(temp);

			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp)));

			AddDereferenceNode(0,0);
			strs.push_back(vName);
			AddMemberAccessNode(s, e);

			currTypes.pop_back();
			return;
		}
	}
	// Если мы находимся в локальной функции, и переменная находится в наружной области видимости
	if((int)retTypeStack.size() > 1 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) && i < (int)varInfoTop[currDefinedFunc.back()->vTopSize].activeVarCnt)
	{
		FunctionInfo *currFunc = currDefinedFunc.back();
		// Добавим имя переменной в список внешних переменных функции
		int num = AddFunctionExternal(currFunc, vName);

		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>((int)currDefinedFunc.back()->external.size(), typeInt)));
		TypeInfo *temp = GetReferenceType(GetArrayType(GetReferenceType(typeInt)));
		currTypes.push_back(temp);

		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp)));
		AddDereferenceNode(0,0);
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(num, typeInt)));
		AddArrayIndexNode(0,0);
		AddDereferenceNode(0,0);
		// Убрали текущий тип
		currTypes.pop_back();
	}else{
		if(fID == -1)
		{
			// Если переменная находится в глобальной области видимости, её адрес - абсолютный, без сдвигов
			bool absAddress = ((varInfoTop.size() > 1) && (varInfo[i]->pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;

			int varAddress = varInfo[i]->pos;
			if(!absAddress)
				varAddress -= (int)(varInfoTop.back().varStackSize);

			// Создаем узел для получения указателя на переменную
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeGetAddress(varInfo[i], varAddress, absAddress)));
		}else{
			if(funcInfo[fID]->funcPtr != 0)
				throw CompilerError("ERROR: Can't get a pointer to an extern function", s);
			if(funcInfo[fID]->address == -1 && funcInfo[fID]->funcPtr == NULL)
				throw CompilerError("ERROR: Can't get a pointer to a build-in function", s);
			if(funcInfo[fID]->type == FunctionInfo::LOCAL)
			{
				std::string bName = "$" + funcInfo[fID]->name + "_ext";
				int i = (int)varInfo.size()-1;
				while(i >= 0 && varInfo[i]->name != bName)
					i--;
				if(i == -1)
				{
					nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(0, GetReferenceType(typeInt))));
				}else{
					AddGetAddressNode(bName.c_str(), bName.c_str()+bName.length());
					currTypes.pop_back();
				}
			}

			// Создаем узел для получения указателя на функцию
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFunctionAddress(funcInfo[fID])));
		}
	}
}

// Функция вызывается для индексации массива
void AddArrayIndexNode(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;

	// Тип должен быть массивом
	if(currTypes.back()->arrLevel == 0)
		throw CompilerError("ERROR: indexing variable that is not an array", s);
	// Если это безразмерный массив (указатель на массив)
	if(currTypes.back()->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		// То перед индексацией необходимо получить указатель на массив, который хранится в переменной
		shared_ptr<NodeZeroOP> temp = nodeList.back();
		nodeList.pop_back();
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeDereference(GetReferenceType(currTypes.back()->subType))));
		nodeList.push_back(temp);
	}
	// Если индекс - константное число и текущий узел - адрес
	if(nodeList.back()->GetNodeType() == typeNodeNumber && (*(nodeList.end()-2))->GetNodeType() == typeNodeGetAddress)
	{
		// Получаем значение сдвига
		int shiftValue;
		shared_ptr<NodeZeroOP> indexNode = nodeList.back();
		TypeInfo *aType = indexNode->GetTypeInfo();
		NodeZeroOP* zOP = indexNode.get();
		if(aType == typeDouble)
		{
			shiftValue = (int)static_cast<NodeNumber<double>* >(zOP)->GetVal();
		}else if(aType == typeFloat){
			shiftValue = (int)static_cast<NodeNumber<float>* >(zOP)->GetVal();
		}else if(aType == typeLong){
			shiftValue = (int)static_cast<NodeNumber<long long>* >(zOP)->GetVal();
		}else if(aType == typeInt){
			shiftValue = static_cast<NodeNumber<int>* >(zOP)->GetVal();
		}else{
			throw CompilerError("AddArrayIndexNode() ERROR: unknown index type " + aType->name, lastKnownStartPos);
		}

		// Проверим индекс на выход за пределы массива
		if(shiftValue < 0)
			throw CompilerError("ERROR: Array index cannot be negative", s);
		if((unsigned int)shiftValue >= currTypes.back()->arrSize)
			throw CompilerError("ERROR: Array index out of bounds", s);

		// Индексируем относительно него
		static_cast<NodeGetAddress*>((*(nodeList.end()-2)).get())->IndexArray(shiftValue);
		nodeList.pop_back();
	}else{
		// Иначе создаём узел индексации
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeArrayIndex(currTypes.back())));
	}
	// Теперь текущий тип - тип элемента массива
	currTypes.back() = currTypes.back()->subType;
}

// Функция вызывается для разыменования указателя
void AddDereferenceNode(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;

	// Создаём узел разыменования
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeDereference(currTypes.back())));
	// Теперь текущий тип - тип на который указывала ссылка
	currTypes.back() = GetDereferenceType(currTypes.back());
}

// Компилятор в начале предполагает, что после переменной будет слодовать знак присваивания
// Часто его нету, поэтому требуется удалить узел
void FailedSetVariable(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.pop_back();
}

// Функция вызывается для определния переменной с одновременным присваиванием ей значения
void AddDefineVariableNode(char const* s, char const* e)
{
	lastKnownStartPos = s;

	// Ищем переменную по имени
	int i = (int)varInfo.size()-1;
	string vName = strs.back();
	while(i >= 0 && varInfo[i]->name != vName)
		i--;
	if(i == -1)
		throw CompilerError("ERROR: variable '" + vName + "' is not defined", s);
	// Кладём в стек типов её тип
	currTypes.push_back(varInfo[i]->varType);

	// Если переменная находится в глобальной области видимости, её адрес - абсолютный, без сдвигов
	bool absAddress = ((varInfoTop.size() > 1) && (varInfo[i]->pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;

	// Если указатель на текущий тип равен NULL, значит тип переменной обозначен как автоматически выводимый (auto)
	// В таком случае, в качестве типа берётся возвращаемый последним узлом AST
	TypeInfo *realCurrType = currTypes.back() ? currTypes.back() : nodeList.back()->GetTypeInfo();

	// Возможно, для определения значения переменной понадобится добавить вспомогательный узел дерева
	// Переменная служит как флаг, обозначающий, что два узла надо объеденить в один
	bool unifyTwo = false;
	// Если тип переменной - безразмерный массив, а присваевается ей значение другого типа
	if(realCurrType->arrSize == TypeInfo::UNSIZED_ARRAY && realCurrType != nodeList.back()->GetTypeInfo())
	{
		TypeInfo *nodeType = nodeList.back()->GetTypeInfo();
		// Если подтип обоих значений (предположительно, массивов) совпадает
		if(realCurrType->subType == nodeType->subType)
		{
			// И если справа не находится узел получения значения переменной
			if(nodeList.back()->GetNodeType() != typeNodeDereference)
			{
				// Тогда, если справа - определение массива списком
				if(nodeList.back()->GetNodeType() == typeNodeExpressionList)
				{
					// Добавим узел, присваивающий скрытой переменной значения этого списка
					AddInplaceArray(s, e);
					// Добавили лишний узел, потребуется их объеденить в конце
					unifyTwo = true;
				}else{
					// Иначе, типы не совместимы, поэтому свидетельствуем об ошибке
					throw CompilerError("ERROR: cannot convert from " + nodeList.back()->GetTypeInfo()->GetTypeName() + " to " + realCurrType->GetTypeName(), s);
				}
			}
			// Далее, так как мы присваиваем безразменому массиву значение размерного,
			// нам надо преобразовать его в пару указатель;размер
			// Возьмём указатель на массив, он - узел, находящийся в узле разыменования указателя
			nodeList.back() = static_cast<NodeDereference*>(nodeList.back().get())->GetFirstNode();
			// Найдем размер массива
			UINT typeSize = (nodeType->size - nodeType->paddingBytes) / nodeType->subType->size;
			// Создадим список выражений, возвращающий тип безразмерного массива
			// Конструктор списка захватит предыдущий узел в себя
			shared_ptr<NodeExpressionList> listExpr(new NodeExpressionList(varInfo[i]->varType));
			// Создадим узел, возвращающий число - размер массива
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(typeSize, typeInt)));
			// Добавим в список
			listExpr->AddNode();
			// Положим список в список узлов
			nodeList.push_back(listExpr);
		}
	}
	// Если переменной присваивается функция, то возьмём указатель на неё
	if(nodeList.back()->GetNodeType() == typeNodeFuncDef ||
		(nodeList.back()->GetNodeType() == typeNodeExpressionList && static_cast<NodeExpressionList*>(nodeList.back().get())->GetFirstNode()->GetNodeType() == typeNodeFuncDef))
	{
		AddInplaceFunction(s, e);
		currTypes.pop_back();
		unifyTwo = true;
		realCurrType = nodeList.back()->GetTypeInfo();
		varDefined = true;
		varTop -= realCurrType->size;
	}

	// Переменная показывает, на сколько байт расширить стек переменных
	UINT varSizeAdd = offsetBytes;	// По умолчанию, она хранит выравнивающий сдвиг
	offsetBytes = 0;	// Который сразу же обнуляется
	// Если тип не был ранее известен, значит в функции добавления переменной выравнивание не было произведено
	if(!currTypes.back())
	{
		// Если выравнивание по умолчанию для типа значения справа не равно нулю (без выравнивания)
		// Или если выравнивание указано пользователем
		if(realCurrType->alignBytes != 0 || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
		{
			// Выбираем выравниваени. Указанное пользователем имеет больший приоритет, чем выравнивание по умолчанию
			UINT activeAlign = currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : realCurrType->alignBytes;
			if(activeAlign > 16)
				throw CompilerError("ERROR: alignment must me less than 16 bytes", s);
			// Если требуется выравнивание (нету спецификации noalign, и адрес ещё не выравнен)
			if(activeAlign != 0 && varTop % activeAlign != 0)
			{
				UINT offset = activeAlign - (varTop % activeAlign);
				varSizeAdd += offset;
				varInfo[i]->pos += offset;
				varTop += offset;
			}
		}
		varInfo[i]->varType = realCurrType;
		varTop += realCurrType->size;
	}
	varSizeAdd += !varInfo[i]->dataReserved ? realCurrType->size : 0;
	varInfo[i]->dataReserved = true;

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeGetAddress(varInfo[i], varInfo[i]->pos-(int)(varInfoTop.back().varStackSize), absAddress)));

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVariableSet(realCurrType, varSizeAdd, false)));

	if(unifyTwo)
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(nodeList.back()->GetTypeInfo())));
		shared_ptr<NodeZeroOP> temp = nodeList.back();
		nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp.get())->AddNode();
		nodeList.push_back(temp);
	}
}

void AddSetVariableNode(char const* s, char const* e)
{
	lastKnownStartPos = s;

	TypeInfo *realCurrType = currTypes.back();
	bool unifyTwo = false;
	if(realCurrType->arrSize == TypeInfo::UNSIZED_ARRAY && realCurrType != nodeList.back()->GetTypeInfo())
	{
		TypeInfo *nodeType = nodeList.back()->GetTypeInfo();
		if(realCurrType->subType == nodeType->subType)
		{
			if(nodeList.back()->GetNodeType() != typeNodeDereference)
			{
				if(nodeList.back()->GetNodeType() == typeNodeExpressionList)
				{
					AddInplaceArray(s, e);
					currTypes.pop_back();
					unifyTwo = true;
				}else{
					throw CompilerError("ERROR: cannot convert from " + nodeList.back()->GetTypeInfo()->GetTypeName() + " to " + realCurrType->GetTypeName(), s);
				}
			}
			nodeList.back() = static_cast<NodeDereference*>(nodeList.back().get())->GetFirstNode();
			UINT typeSize = (nodeType->size - nodeType->paddingBytes) / nodeType->subType->size;
			shared_ptr<NodeExpressionList> listExpr(new NodeExpressionList(realCurrType));
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(typeSize, typeInt)));
			listExpr->AddNode();
			nodeList.push_back(listExpr);

			if(unifyTwo)
				std::swap(*(nodeList.end()-2), *(nodeList.end()-3));
		}
	}
	if(nodeList.back()->GetNodeType() == typeNodeFuncDef ||
		(nodeList.back()->GetNodeType() == typeNodeExpressionList && static_cast<NodeExpressionList*>(nodeList.back().get())->GetFirstNode()->GetNodeType() == typeNodeFuncDef))
	{
		AddInplaceFunction(s, e);
		unifyTwo = true;
		std::swap(*(nodeList.end()-2), *(nodeList.end()-3));
	}

	try
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVariableSet(currTypes.back(), 0, true)));
	}catch(const std::string& str){
		throw CompilerError(str.c_str(), s);
	}

	if(unifyTwo)
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(nodeList.back()->GetTypeInfo())));
		shared_ptr<NodeZeroOP> temp = nodeList.back();
		nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp.get())->AddNode();
		nodeList.push_back(temp);
	}
}

void AddGetVariableNode(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;

	if(nodeList.back()->GetTypeInfo()->funcType == NULL)
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeDereference(currTypes.back())));
}

void AddMemberAccessNode(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;

	std::string memberName = strs.back();

	// Да, это локальная переменная с именем, как у глобальной!
	TypeInfo *currType = currTypes.back();

	if(currType->refLevel == 1)
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeDereference(currTypes.back())));
		currTypes.back() = GetDereferenceType(currTypes.back());
		currType = currTypes.back();
	}
 
	int fID = -1;
	int i = (int)currType->memberData.size()-1;
	while(i >= 0 && currType->memberData[i].name != memberName)
		i--;
	if(i == -1)
	{
		// Ищем функцию по имени
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->name == (currType->name + "::" + memberName) && funcInfo[k]->visible)
			{
				if(fID != -1)
					throw CompilerError("ERROR: there are more than one '" + memberName + "' function, and the decision isn't clear", s);
				fID = k;
			}
		}
		if(fID == -1)
			throw CompilerError("ERROR: variable '" + memberName + "' is not a member of '" + currType->GetTypeName() + "'", s);
	}
	
	if(fID == -1)
	{
		if(nodeList.back()->GetNodeType() == typeNodeGetAddress)
		{
			static_cast<NodeGetAddress*>(nodeList.back().get())->ShiftToMember(i);
		}else{
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeShiftAddress(currType->memberData[i].offset, currType->memberData[i].type)));
		}
		currTypes.back() = currType->memberData[i].type;
	}else{
		// Создаем узел для получения указателя на функцию
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFunctionAddress(funcInfo[fID])));

		currTypes.back() = funcInfo[fID]->funcType;
	}

	strs.pop_back();
}

void AddMemberFunctionCall(char const* s, char const* e)
{
	strs.back() = currTypes.back()->name + "::" + strs.back();
	addFuncCallNode(s, e);
	currTypes.back() = nodeList.back()->GetTypeInfo();
}

void AddPreOrPostOpNode(CmdID postCmd, bool prefixOp)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePreOrPostOp(currTypes.back(), postCmd, prefixOp)));
}

template<CmdID cmd>
struct AddPreOrPostOp
{
	AddPreOrPostOp(bool isPrefixOp)
	{
		prefixOp = isPrefixOp;
	}

	void operator() (char const* s, char const* e)
	{
		(void)e;	// C4100
		lastKnownStartPos = s;
		AddPreOrPostOpNode(cmd, prefixOp);
	}
	bool prefixOp;
};

void AddModifyVariableNode(char const* s, char const* e, CmdID cmd)
{
	shared_ptr<NodeZeroOP> temp = *(nodeList.end()-2);
	nodeList.push_back(temp);
	AddGetVariableNode(s, e);
	std::swap(*(nodeList.end()-1), *(nodeList.end()-2));
	addTwoAndCmpNode(cmd);
	AddSetVariableNode(s, e);
}

template<CmdID cmd>
struct AddModifyVariable
{
	void operator() (char const* s, char const* e)
	{
		lastKnownStartPos = s;
		AddModifyVariableNode(s, e, cmd);
	}
};

void AddInplaceArray(char const* s, char const* e)
{
	char asString[16];
	sprintf(asString, "%d", inplaceArrayNum++);
	strs.push_back("$carr");
	strs.back() += asString;

	TypeInfo *saveCurrType = currType;
	bool saveVarDefined = varDefined;

	currType = NULL;
	addVar(s, e);

	AddDefineVariableNode(s, e);
	addPopNode(s, e);
	currTypes.pop_back();

	AddGetAddressNode(strs.back().c_str(), strs.back().c_str()+strs.back().length());
	AddGetVariableNode(s, e);

	varDefined = saveVarDefined;
	currType = saveCurrType;
	strs.pop_back();
}

void AddInplaceFunction(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	std::string fName = funcInfo.back()->name;
	AddGetAddressNode(fName.c_str(), fName.c_str()+fName.length());
}

//////////////////////////////////////////////////////////////////////////
void addOneExprNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList()));
}
void addTwoExprNode(char const* s, char const* e)
{
	if(nodeList.back()->GetNodeType() != typeNodeExpressionList)
		addOneExprNode(s, e);
	// Take the expression list from the top
	shared_ptr<NodeZeroOP> temp = nodeList.back();
	nodeList.pop_back();
	static_cast<NodeExpressionList*>(temp.get())->AddNode();
	nodeList.push_back(temp);
}

std::vector<UINT> arrElementCount;

void addArrayConstructor(char const* s, char const* e)
{
	arrElementCount.back()++;

	TypeInfo *currType = (*(nodeList.end()-arrElementCount.back()))->GetTypeInfo();

	if(currType == typeShort || currType == typeChar)
	{
		currType = typeInt;
		warningLog << "WARNING: short and char will be promoted to int during array construction\r\n At " << std::string(s, e) << "\r\n";
	}
	if(currType == typeFloat)
	{
		currType = typeDouble;
		warningLog << "WARNING: float will be promoted to double during array construction\r\n At " << std::string(s, e) << "\r\n";
	}
	if(currType == typeVoid)
		throw CompilerError("ERROR: array cannot be constructed from void type elements", s);

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(arrElementCount.back(), typeInt)));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(GetArrayType(currType))));

	shared_ptr<NodeZeroOP> temp = nodeList.back();
	nodeList.pop_back();

	NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp.get());

	TypeInfo *realType = nodeList.back()->GetTypeInfo();
	for(UINT i = 0; i < arrElementCount.back(); i++)
	{
		if(realType != currType && !((realType == typeShort || realType == typeChar) && currType == typeInt) && !(realType == typeFloat && currType == typeDouble))
		{
			char tempStr[16];
			sprintf(tempStr, "%d", arrElementCount.back()-i-1);
			throw CompilerError(std::string("ERROR: element ") + tempStr + " doesn't match the type of element 0 (" + currType->GetTypeName() + ")", s);
		}
		arrayList->AddNode(false);
	}

	nodeList.push_back(temp);

	arrElementCount.pop_back();
}

void FunctionAdd(char const* s, char const* e)
{
	(void)e;	// C4100
	for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		if(varInfo[i]->name == strs.back())
			throw CompilerError("ERROR: Name '" + strs.back() + "' is already taken for a variable in current scope", s);
	std::string name = strs.back();
	if(name == "if" || name == "else" || name == "for" || name == "while" || name == "var" || name == "func" || name == "return" || name=="switch" || name=="case")
		throw CompilerError("ERROR: The name '" + name + "' is reserved", s);
	if(!currType)
		throw CompilerError("ERROR: function return type cannot be auto", s);
	funcInfo.push_back(new FunctionInfo());
	funcInfo.back()->name = name;
	funcInfo.back()->vTopSize = (UINT)varInfoTop.size();
	retTypeStack.push_back(currType);
	funcInfo.back()->retType = currType;
	if(newType)
		funcInfo.back()->type = FunctionInfo::THISCALL;
	if(newType ? varInfoTop.size() > 2 : varInfoTop.size() > 1)
		funcInfo.back()->type = FunctionInfo::LOCAL;
	currDefinedFunc.push_back(funcInfo.back());

	if(newType)
		funcInfo.back()->name = newType->name + "::" + name;
	if(varDefined && varInfo.back()->varType == NULL)
		varTop += 8;
}

void FunctionParam(char const* s, char const* e)
{
	(void)e;	// C4100
	if(!currType)
		throw CompilerError("ERROR: function parameter cannot be an auto type", s);
	funcInfo.back()->params.push_back(VariableInfo(strs.back(), 0, currType, currValConst));
	funcInfo.back()->allParamSize += currType->size;
	strs.pop_back();
}
void FunctionStart(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));

	for(int i = (int)funcInfo.back()->params.size()-1; i >= 0; i--)
	{
		strs.push_back(funcInfo.back()->params[i].name);
		
		currValConst = funcInfo.back()->params[i].isConst;
		currType = funcInfo.back()->params[i].varType;
		currAlign = 1;
		addVar(0,0);
		varDefined = false;

		strs.pop_back();
	}

	strs.push_back("$" + funcInfo.back()->name + "_ext");
	currType = GetReferenceType(typeInt);
	currAlign = 1;
	addVar(0, 0);
	varDefined = false;
	strs.pop_back();

	funcInfo.back()->funcType = GetFunctionType(funcInfo.back());
}
void FunctionEnd(char const* s, char const* e)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	std::string fName = strs.back();
	if(newType)
	{
		fName = newType->name + "::" + fName;
	}

	int i = (int)funcInfo.size()-1;
	while(i >= 0 && funcInfo[i]->name != fName)
		i--;

	// Find all the functions with the same name
	//int count = 0;
	for(int n = 0; n < i; n++)
	{
		if(funcInfo[n]->name == funcInfo[i]->name && funcInfo[n]->params.size() == funcInfo[i]->params.size() && funcInfo[n]->visible)
		{
			// Check all parameter types
			bool paramsEqual = true;
			for(UINT k = 0; k < funcInfo[i]->params.size(); k++)
			{
				if(funcInfo[n]->params[k].varType->GetTypeName() != funcInfo[i]->params[k].varType->GetTypeName())
					paramsEqual = false;
			}
			if(paramsEqual)
				throw CompilerError("ERROR: function '" + funcInfo[i]->name + "' is being defined with the same set of parameters", s);
		}
	}

	unsigned int varFormerTop = varTop;
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop -= varInfo.back()->varType->size;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBlock(varFormerTop-varTop, false)));

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncDef(funcInfo[i])));
	strs.pop_back();

	retTypeStack.pop_back();
	currDefinedFunc.pop_back();

	// If function is local, create function parameters block
	if(lastFunc.type == FunctionInfo::LOCAL && !lastFunc.external.empty())
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>((int)lastFunc.external.size(), typeInt)));
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(GetArrayType(GetReferenceType(typeInt)))));

		shared_ptr<NodeZeroOP> temp = nodeList.back();
		nodeList.pop_back();

		NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp.get());

		for(UINT n = 0; n < lastFunc.external.size(); n++)
		{
			const char *s = lastFunc.external[n].c_str(), *e = s + lastFunc.external[n].length();
			AddGetAddressNode(s, e);
			arrayList->AddNode();
		}
		nodeList.push_back(temp);

		strs.push_back("$" + lastFunc.name + "_ext");

		TypeInfo *saveCurrType = currType;
		bool saveVarDefined = varDefined;

		currType = NULL;
		addVar(s, e);

		AddDefineVariableNode(s, e);
		addPopNode(s, e);
		currTypes.pop_back();

		varDefined = saveVarDefined;
		currType = saveCurrType;
		strs.pop_back();
		addTwoExprNode(0,0);
	}

	if(newType)
	{
		newType->memberFunctions.push_back(TypeInfo::MemberFunction());
		TypeInfo::MemberFunction &clFunc = newType->memberFunctions.back();
		clFunc.name = lastFunc.name;
		clFunc.func = &lastFunc;
		clFunc.defNode = nodeList.back().get();
	}
}

void addFuncCallNode(char const* s, char const* e)
{
	string fname = strs.back();
	strs.pop_back();

	// Searching, if fname is actually a variable name (which means, either it is a pointer to function, or an error)
	int vID = (int)varInfo.size()-1;
	while(vID >= 0 && varInfo[vID]->name != fname)
		vID--;

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	if(vID == -1)
	{
		//Find all functions with given name
		FunctionInfo *fList[32];
		UINT	fRating[32];
		memset(fRating, 0, 32*4);

		unsigned int count = 0;
		for(int k = 0; k < (int)funcInfo.size(); k++)
			if(funcInfo[k]->name == fname && funcInfo[k]->visible)
				fList[count++] = funcInfo[k];
		if(count == 0)
			throw CompilerError("ERROR: function '" + fname + "' is undefined", s);
		// Find the best suited function
		UINT minRating = 1024*1024;
		UINT minRatingIndex = (unsigned int)~0;
		for(unsigned int k = 0; k < count; k++)
		{
			if(fList[k]->params.size() != callArgCount.back())
			{
				fRating[k] += 65000;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.
				continue;
			}
			for(UINT n = 0; n < fList[k]->params.size(); n++)
			{
				shared_ptr<NodeZeroOP> activeNode = nodeList[nodeList.size()-fList[k]->params.size()+n];
				TypeInfo *paramType = activeNode->GetTypeInfo();
				UINT	nodeType = activeNode->GetNodeType();
				TypeInfo *expectedType = fList[k]->params[n].varType;
				if(expectedType != paramType)
				{
					if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->arrSize != 0 && paramType->subType == expectedType->subType)
						fRating[k] += 5;
					else if(expectedType->funcType != NULL && nodeType == typeNodeFuncDef ||
							(nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(activeNode.get())->GetFirstNode()->GetNodeType() == typeNodeFuncDef))
						fRating[k] += 5;
					else if(expectedType->type == TypeInfo::TYPE_COMPLEX)
						fRating[k] += 65000;	// Definitely, this isn't the function we are trying to call. Function excepts different complex type.
					else if(paramType->type == TypeInfo::TYPE_COMPLEX)
						fRating[k] += 65000;	// Again. Function excepts complex type, and all we have is simple type (cause previous condition failed).
					else if(paramType->subType != expectedType->subType)
						fRating[k] += 65000;	// Pointer or array with a different types inside. Doesn't matter if simple or complex.
					else	// Build-in types can convert to each other, but the fact of conversion tells us, that there could be a better suited function
						fRating[k] += 1;
				}
			}
			if(fRating[k] < minRating)
			{
				minRating = fRating[k];
				minRatingIndex = k;
			}
		}
		// Maybe the function we found can't be used at all
		if(minRating > 1000)
		{
			ostringstream errTemp;
			errTemp << "ERROR: can't find function '" + fname + "' with following parameters:\r\n  ";
			errTemp << fname << "(";
			for(UINT n = 0; n < callArgCount.back(); n++)
				errTemp << nodeList[nodeList.size()-callArgCount.back()+n]->GetTypeInfo()->GetTypeName() << (n != callArgCount.back()-1 ? ", " : "");
			errTemp << ")\r\n";
			errTemp << " the only available are:\r\n";
			for(unsigned int n = 0; n < count; n++)
			{
				errTemp << "  " << fname << "(";
				for(UINT m = 0; m < fList[n]->params.size(); m++)
					errTemp << fList[n]->params[m].varType->GetTypeName() << (m != fList[n]->params.size()-1 ? ", " : "");
				errTemp << ")\r\n";
			}
			throw errTemp.str();
		}
		// Check, is there are more than one function, that share the same rating
		for(unsigned int k = 0; k < count; k++)
		{
			if(k != minRatingIndex && fRating[k] == minRating)
			{
				ostringstream errTemp;
				errTemp << "ERROR: ambiguity, there is more than one overloaded function available for the call.\r\n";
				errTemp << "  " << fname << "(";
				for(UINT n = 0; n < callArgCount.back(); n++)
					errTemp << nodeList[nodeList.size()-callArgCount.back()+n]->GetTypeInfo()->GetTypeName() << (n != callArgCount.back()-1 ? ", " : "");
				errTemp << ")\r\n";
				errTemp << " candidates are:\r\n";
				for(unsigned int n = 0; n < count; n++)
				{
					if(fRating[n] != minRating)
						continue;
					errTemp << "  " << fname << "(";
					for(UINT m = 0; m < fList[n]->params.size(); m++)
						errTemp << fList[n]->params[m].varType->GetTypeName() << (m != fList[n]->params.size()-1 ? ", " : "");
					errTemp << ")\r\n";
				}
				throw errTemp.str();
			}
		}
		fType = fList[minRatingIndex]->funcType->funcType;
		fInfo = fList[minRatingIndex];
	}else{
		AddGetAddressNode(fname.c_str(), fname.length()+fname.c_str());
		AddGetVariableNode(s, e);
		fType = nodeList.back()->GetTypeInfo()->funcType;
	}

	vector<shared_ptr<NodeZeroOP> > paramNodes;
	for(UINT i = 0; i < fType->paramType.size(); i++)
	{
		paramNodes.push_back(nodeList.back());
		nodeList.pop_back();
	}
	vector<shared_ptr<NodeZeroOP> > inplaceArray;

	for(UINT i = 0; i < fType->paramType.size(); i++)
	{
		UINT index = (UINT)(fType->paramType.size()) - i - 1;

		TypeInfo *expectedType = fType->paramType[i];
		TypeInfo *realType = paramNodes[index]->GetTypeInfo();
		
		if(paramNodes[index]->GetNodeType() == typeNodeFuncDef ||
			(paramNodes[index]->GetNodeType() == typeNodeExpressionList && static_cast<NodeExpressionList*>(paramNodes[index].get())->GetFirstNode()->GetNodeType() == typeNodeFuncDef))
		{
			AddInplaceFunction(s, e);
			currTypes.pop_back();

			shared_ptr<NodeExpressionList> listExpr(new NodeExpressionList(paramNodes[index]->GetTypeInfo()));
			listExpr->AddNode();
			nodeList.push_back(listExpr);
		}
		if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && expectedType->subType == realType->subType && expectedType != realType)
		{
			if(paramNodes[index]->GetNodeType() != typeNodeDereference)
			{
				if(paramNodes[index]->GetNodeType() == typeNodeExpressionList)
				{
					nodeList.push_back(paramNodes[index]);
					AddInplaceArray(s, e);

					paramNodes[index] = nodeList.back();
					nodeList.pop_back();
					inplaceArray.push_back(nodeList.back());
					nodeList.pop_back();
				}else{
					char chTemp[16];
					sprintf(chTemp, "%d", i);
					throw CompilerError(std::string("ERROR: array expected as a parameter ") + chTemp, s);
				}
			}
			UINT typeSize = (paramNodes[index]->GetTypeInfo()->size - paramNodes[index]->GetTypeInfo()->paddingBytes) / paramNodes[index]->GetTypeInfo()->subType->size;
			nodeList.push_back(static_cast<NodeDereference*>(paramNodes[index].get())->GetFirstNode());
			shared_ptr<NodeExpressionList> listExpr(new NodeExpressionList(varInfo[i]->varType));
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(typeSize, typeInt)));
			listExpr->AddNode();
			nodeList.push_back(listExpr);
		}else{
			nodeList.push_back(paramNodes[index]);
		}
	}

	if(fInfo && (fInfo->type == FunctionInfo::LOCAL))
	{
		std::string bName = "$" + fInfo->name + "_ext";
		int i = (int)varInfo.size()-1;
		while(i >= 0 && varInfo[i]->name != bName)
			i--;
		if(i == -1)
		{
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(0, GetReferenceType(typeInt))));
		}else{
			AddGetAddressNode(bName.c_str(), bName.c_str()+bName.length());
			if(currTypes.back()->refLevel == 1)
				AddDereferenceNode(s, e);
			currTypes.pop_back();
		}
	}

	if(!fInfo)
		currTypes.pop_back();

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncCall(fInfo, fType)));

	if(inplaceArray.size() > 0)
	{
		shared_ptr<NodeZeroOP> temp = nodeList.back();
		nodeList.pop_back();
		for(UINT i = 0; i < inplaceArray.size(); i++)
			nodeList.push_back(inplaceArray[i]);
		nodeList.push_back(temp);

		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(temp->GetTypeInfo())));
		for(UINT i = 0; i < inplaceArray.size(); i++)
			addTwoExprNode(s, e);
	}
	callArgCount.pop_back();
}

void addIfNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(false)));
}
void addIfElseNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(true)));
}
void addIfElseTermNode(char const* s, char const* e)
{
	(void)e;	// C4100
	TypeInfo* typeA = nodeList[nodeList.size()-1]->GetTypeInfo();
	TypeInfo* typeB = nodeList[nodeList.size()-2]->GetTypeInfo();
	if(typeA != typeB)
		throw CompilerError("ERROR: trinary operator ?: \r\n result types are not equal (" + typeB->name + " : " + typeA->name + ")", s);
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(true, true)));
}

void saveVarTop(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	cycleBeginVarTop.push_back((UINT)varInfoTop.size());
}
void addForNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeForExpr()));
	cycleBeginVarTop.pop_back();
}
void addWhileNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeWhileExpr()));
	cycleBeginVarTop.pop_back();
}
void addDoWhileNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeDoWhileExpr()));
	cycleBeginVarTop.pop_back();
}

void preSwitchNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	cycleBeginVarTop.push_back((UINT)varInfoTop.size());
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeSwitchExpr()));
}
void addCaseNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	shared_ptr<NodeZeroOP> temp = *(nodeList.end()-3);
	static_cast<NodeSwitchExpr*>(temp.get())->AddCase();
}
void addSwitchNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	cycleBeginVarTop.pop_back();
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop--;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}

void TypeBegin(char const* s, char const* e)
{
	if(newType)
		throw CompilerError("ERROR: Different type is being defined", s);
	if((int)currAlign < 0)
		throw CompilerError("ERROR: alignment must be a positive number", s);
	if(currAlign > 16)
		throw CompilerError("ERROR: alignment must me less than 16 bytes", s);
	newType = new TypeInfo();
	newType->name = std::string(s, e);
	newType->type = TypeInfo::TYPE_COMPLEX;
	newType->alignBytes = currAlign;
	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;

	typeInfo.push_back(newType);
	
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
}

void TypeAddMember(char const* s, char const* e)
{
	if(!currType)
		throw CompilerError("ERROR: auto cannot be used for class members", s);
	newType->AddMember(std::string(s, e), currType);

	strs.push_back(std::string(s, e));
	addVar(0,0);
	strs.pop_back();
}

void TypeFinish(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	if(newType->size % 4 != 0)
	{
		newType->paddingBytes = 4 - (newType->size % 4);
		newType->size += 4 - (newType->size % 4);
	}

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
	for(unsigned int i = 0; i < newType->memberFunctions.size(); i++)
		addTwoExprNode(0,0);

	newType = NULL;

	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{ 
		varTop -= varInfo.back()->varType->size;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void addUnfixedArraySize(char const*s, char const*e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(1, typeVoid)));
}

namespace CompilerGrammar
{
	// Функции, кладушие и убирающие строки со стека строк
	// Стек строк может использоваться для удобного получения элемента более сложной грамматики
	// Например для правила использования переменной a[i], можно поместить "a" в стек,
	// потому что в функцию передаётся "a[i]" целиком
	void ParseStrPush(char const *s, char const *e)
	{
		strs.push_back(string(s,e));
	}
	void ParseStrPop(char const *s, char const *e)
	{
		(void)s; (void)e;	// C4100
		strs.pop_back();
	}
	void ParseStrCopy(char const *s, char const *e)
	{
		(void)s; (void)e;	// C4100
		strs.push_back(*(strs.end()-2));
	}
	void ParseStrAdd(char const *s, char const *e)
	{
		strs.back() += std::string(s, e);
	}

	// Эти функции вызываются, чтобы привязать строку кода к узлу, который его компилирует
	void SetStringToLastNode(char const *s, char const *e)
	{
		nodeList.back()->SetCodeInfo(s, e);
	}
	struct StringIndex
	{
		StringIndex(char const *s, char const *e)
		{
			indexS = s;
			indexE = e;
		}
		const char *indexS, *indexE;
	};
	vector<StringIndex> sIndexes;
	void SaveStringIndex(char const *s, char const *e)
	{
		sIndexes.push_back(StringIndex(s, e));
	}
	void SetStringFromIndex(char const *s, char const *e)
	{
		(void)s; (void)e;	// C4100
		nodeList.back()->SetCodeInfo(sIndexes.back().indexS, sIndexes.back().indexE);
		sIndexes.pop_back();
	}

	// Callbacks
	typedef void (*parserCallback)(char const*, char const*);
	parserCallback addChar, addInt, addFloat, addLong, addDouble;
	parserCallback strPush, strPop, strCopy;

	// Parser rules
	Rule group, term5, term4_9, term4_8, term4_85, term4_7, term4_75, term4_6, term4_65, term4_4, term4_2, term4_1, term4, term3, term2, term1, expression;
	Rule varname, funccall, funcdef, funcvars, block, vardef, vardefsub, ifexpr, whileexpr, forexpr, retexpr;
	Rule doexpr, breakexpr, switchexpr, isconst, addvarp, seltype, arrayDef;
	Rule classdef, variable, postExpr, continueExpr;
	Rule funcProt;	// user function prototype

	Rule code, mySpaceP;

	class ThrowError
	{
	public:
		ThrowError(): err(NULL){ }
		ThrowError(const char* str): err(str){ }

		void operator() (char const* s, char const* e)
		{
			(void)e;	// C4100
			assert(err);
			throw CompilerError(err, s);
		}
	private:
		const char* err;
	};
	class TypeNameP: public BaseP
	{
	public:
		TypeNameP(Rule a){ m_a.set(a); }
		virtual ~TypeNameP(){ m_a.detach(); }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space) throw()
		{
			SkipSpaces(str, space);
			char* curr = *str;
			m_a->Parse(str, shared_ptr<BaseP>((BaseP*)NULL));
			if(curr == *str)
				return false;
			std::string type(curr, *str);
			for(UINT i = 0; i < typeInfo.size(); i++)
				if(typeInfo[i]->name == type)
					return true;
			return false;
		}
	protected:
		Rule m_a;
	};
	Rule	typenameP(Rule a) throw(){ return Rule(shared_ptr<BaseP>(new TypeNameP(a))); }
	Rule	strWP(char* str){ return (lexemeD[strP(str) >> (epsP - alnumP)]); }

	void InitGrammar() throw()
	{
		strPush	=	CompilerGrammar::ParseStrPush;
		strPop	=	CompilerGrammar::ParseStrPop;
		strCopy =	CompilerGrammar::ParseStrCopy;

		addChar		=	addNumberNode<char>;
		addInt		=	addNumberNode<int>;
		addFloat	=	addNumberNode<float>;
		addLong		=	addNumberNode<long long>;
		addDouble	=	addNumberNode<double>;

		arrayDef	=	('[' >> (term4_9 | epsP[addUnfixedArraySize]) >> ']' >> !arrayDef)[convertTypeToArray];
		seltype		=	((strP("auto") | typenameP(varname))[selType] | (strP("typeof") >> chP('(') >> ((varname[GetVariableType] >> chP(')')) | (term5[SetTypeOfLastNode] >> chP(')'))))) >> *((lexemeD[strP("ref") >> (~alnumP | nothingP)])[convertTypeToRef] | arrayDef);

		isconst		=	epsP[AssignVar<bool>(currValConst, false)] >> !strP("const")[AssignVar<bool>(currValConst, true)];
		varname		=	lexemeD[alphaP >> *(alnumP | '_')];

		classdef	=	((strP("align") >> '(' >> intP[StrToInt(currAlign)] >> ')') | (strP("noalign") | epsP)[AssignVar<UINT>(currAlign, 0)]) >>
						strP("class") >> varname[TypeBegin] >> chP('{') >>
						*(
							funcdef |
							(seltype >> varname[TypeAddMember] >> *(',' >> varname[TypeAddMember]) >> chP(';'))
						)
						>> chP('}')[TypeFinish];

		funccall	=	varname[strPush] >> 
				('(' | (epsP[strPop] >> nothingP)) >>
				epsP[PushBackVal<std::vector<UINT>, UINT>(callArgCount, 0)] >> 
				!(
				term5[ArrBackInc<std::vector<UINT> >(callArgCount)] >>
				*(',' >> term5[ArrBackInc<std::vector<UINT> >(callArgCount)])
				) >>
				(')' | epsP[ThrowError("ERROR: ')' not found after function call")]);

		funcvars	=	!(isconst >> seltype >> varname[strPush][FunctionParam]) >> *(',' >> isconst >> seltype >> varname[strPush][FunctionParam]);
		funcdef		=	seltype >> varname[strPush] >> (chP('(')[FunctionAdd] | (epsP[strPop] >> nothingP)) >>  funcvars[FunctionStart] >> chP(')') >> chP('{') >> code[FunctionEnd] >> chP('}');
		funcProt	=	seltype >> varname[strPush] >> (chP('(')[FunctionAdd] | (epsP[strPop] >> nothingP)) >>  funcvars >> chP(')') >> chP(';');

		addvarp		=
			(
				varname[strPush] >>
				!('[' >> (term4_9 | epsP[addUnfixedArraySize]) >> ']')[convertTypeToArray]
			)[pushType][addVar] >>
			(('=' >> (term5 | epsP[ThrowError("ERROR: expression not found after '='")]))[AddDefineVariableNode][addPopNode][popType] | epsP[addVarDefNode])[popType][strPop];
		
		vardefsub	= addvarp[SetStringToLastNode] >> *(',' >> vardefsub)[addTwoExprNode];
		vardef		=
			epsP[AssignVar<UINT>(currAlign, 0xFFFFFFFF)] >>
			isconst >>
			!(strP("noalign")[AssignVar<UINT>(currAlign, 0)] | (strP("align") >> '(' >> intP[StrToInt(currAlign)] >> ')')) >>
			seltype >>
			vardefsub;

		ifexpr		=
			(
				strWP("if") >>
				('(' | epsP[ThrowError("ERROR: '(' not found after 'if'")]) >>
				(term5 | epsP[ThrowError("ERROR: condition not found in 'if' statement")]) >>
				(')' | epsP[ThrowError("ERROR: closing ')' not found after 'if' condition")])
			)[SaveStringIndex] >>
			expression >>
			((strP("else") >> expression)[addIfElseNode] | epsP[addIfNode])[SetStringFromIndex];
		forexpr		=
			(
				strWP("for")[saveVarTop] >>
				('(' | epsP[ThrowError("ERROR: '(' not found after 'for'")]) >>
				(
					(
						chP('{') >>
						(code | epsP[addVoidNode]) >>
						(chP('}') | epsP[ThrowError("ERROR: '}' not found after '{'")])
					) |
					vardef |
					term5[addPopNode] |
					epsP[addVoidNode]
				) >>
				(';' | epsP[ThrowError("ERROR: ';' not found after initializer in 'for'")]) >>
				(term5 | epsP[ThrowError("ERROR: condition not found in 'for' statement")]) >>
				(';' | epsP[ThrowError("ERROR: ';' not found after condition in 'for'")]) >>
				((chP('{') >> code >> chP('}')) | term5[addPopNode] | epsP[addVoidNode]) >>
				(')' | epsP[ThrowError("ERROR: ')' not found after 'for' statement")])
			)[SaveStringIndex] >> expression[addForNode][SetStringFromIndex];
		whileexpr	=
			strWP("while")[saveVarTop] >>
			(
				('(' | epsP[ThrowError("ERROR: '(' not found after 'while'")]) >>
				(term5 | epsP[ThrowError("ERROR: expression expected after 'while('")]) >>
				(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'while' statement")])
			) >>
			(expression[addWhileNode] | epsP[ThrowError("ERROR: expression expected after 'while(...)'")]);
		doexpr		=	
			strWP("do")[saveVarTop] >> 
			(expression | epsP[ThrowError("ERROR: expression expected after 'do'")]) >> 
			(strP("while") | epsP[ThrowError("ERROR: 'while' expected after 'do' statement")]) >>
			(
				('(' | epsP[ThrowError("ERROR: '(' not found after 'while'")]) >> 
				(term5 | epsP[ThrowError("ERROR: expression not found after 'while('")]) >> 
				(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'while' statement")])
			)[addDoWhileNode] >> 
			(';' | epsP[ThrowError("ERROR: while(...) should be followed by ';'")]);
		switchexpr	=
			strP("switch") >>
			('(') >>
			(term5 | epsP[ThrowError("ERROR: expression not found after 'switch('")])[preSwitchNode] >>
			(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'switch' statement")]) >>
			('{' | epsP[ThrowError("ERROR: '{' not found after 'switch(...)'")]) >>
			(strWP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode] >>
			*(strWP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode] >>
			('}' | epsP[ThrowError("ERROR: '}' not found after 'switch' statement")])[addSwitchNode];

		retexpr		=
			(
				strWP("return") >>
				(term5 | epsP[addVoidNode]) >>
				(+chP(';') | epsP[ThrowError("ERROR: return must be followed by ';'")])
			)[addReturnNode];
		breakexpr	=	(
			strWP("break") >>
			(+chP(';') | epsP[ThrowError("ERROR: break must be followed by ';'")])
			)[addBreakNode];
		continueExpr	=
			(
				strWP("continue") >>
				(+chP(';') | epsP[ThrowError("ERROR: continue must be followed by ';'")])
			)[AddContinueNode];

		group		=	'(' >> term5 >> (')' | epsP[ThrowError("ERROR: closing ')' not found after '('")]);

		variable	= (chP('*') >> variable)[AddDereferenceNode] | (((varname - strP("case")) >> (~chP('(') | nothingP))[AddGetAddressNode] >> *postExpr);
		postExpr	=	('.' >> varname[strPush] >> (~chP('(') | (epsP[strPop] >> nothingP)))[AddMemberAccessNode] |
						('[' >> term5 >> ']')[AddArrayIndexNode];

		term1		=
			funcdef |
			(strP("sizeof") >> chP('(')[pushType] >> (seltype[pushType][GetTypeSize][popType] | term5[AssignVar<bool>(sizeOfExpr, true)][GetTypeSize]) >> chP(')')[popType]) |
			(chP('&') >> variable)[popType] |
			(strP("--") >> variable[AddPreOrPostOp<cmdDecAt>(true)])[popType] | 
			(strP("++") >> variable[AddPreOrPostOp<cmdIncAt>(true)])[popType] |
			(+(chP('-')[IncVar<UINT>(negCount)]) >> term1)[addNegNode] | (+chP('+') >> term1) | ('!' >> term1)[addLogNotNode] | ('~' >> term1)[addBitNotNode] |
			(chP('\"') >> *(strP("\\\"") | (anycharP - chP('\"'))) >> chP('\"'))[strPush][addStringNode] |
			lexemeD[strP("0x") >> +(digitP | chP('a') | chP('b') | chP('c') | chP('d') | chP('e') | chP('f') | chP('A') | chP('B') | chP('C') | chP('D') | chP('E') | chP('F'))][addHexInt] |
			longestD[((intP >> chP('l'))[addLong] | (intP[addInt])) | ((realP >> chP('f'))[addFloat] | (realP[addDouble]))] |
			(chP('\'') >> ((chP('\\') >> anycharP) | anycharP) >> chP('\''))[addChar] |
			(chP('{')[PushBackVal<std::vector<UINT>, UINT>(arrElementCount, 0)] >> term5 >> *(chP(',') >> term5[ArrBackInc<std::vector<UINT> >(arrElementCount)]) >> chP('}'))[addArrayConstructor] |
			group |
			funccall[addFuncCallNode] |
			(variable >>
				(
					strP("++")[AddPreOrPostOp<cmdIncAt>(false)] |
					strP("--")[AddPreOrPostOp<cmdDecAt>(false)] |
					('.' >> funccall)[AddMemberFunctionCall] |
					epsP[AddGetVariableNode]
				)[popType]
			);

		term2		=	term1 >> *((strP("**") >> term1)[addCmd(cmdPow)]);
		term3		=	term2 >> *(('*' >> term2)[addCmd(cmdMul)] | ('/' >> term2)[addCmd(cmdDiv)] | ('%' >> term2)[addCmd(cmdMod)]);
		term4		=	term3 >> *(('+' >> term3)[addCmd(cmdAdd)] | ('-' >> term3)[addCmd(cmdSub)]);
		term4_1		=	term4 >> *((strP("<<") >> term4)[addCmd(cmdShl)] | (strP(">>") >> term4)[addCmd(cmdShr)]);
		term4_2		=	term4_1 >> *(('<' >> term4_1)[addCmd(cmdLess)] | ('>' >> term4_1)[addCmd(cmdGreater)] | (strP("<=") >> term4_1)[addCmd(cmdLEqual)] | (strP(">=") >> term4_1)[addCmd(cmdGEqual)]);
		term4_4		=	term4_2 >> *((strP("==") >> term4_2)[addCmd(cmdEqual)] | (strP("!=") >> term4_2)[addCmd(cmdNEqual)]);
		term4_6		=	term4_4 >> *(strP("&") >> (term4_4 | epsP[ThrowError("ERROR: expression not found after &")]))[addCmd(cmdBitAnd)];
		term4_65	=	term4_6 >> *(strP("^") >> (term4_6 | epsP[ThrowError("ERROR: expression not found after ^")]))[addCmd(cmdBitXor)];
		term4_7		=	term4_65 >> *(strP("|") >> (term4_65 | epsP[ThrowError("ERROR: expression not found after |")]))[addCmd(cmdBitOr)];
		term4_75	=	term4_7 >> *(strP("and") >> (term4_7 | epsP[ThrowError("ERROR: expression not found after and")]))[addCmd(cmdLogAnd)];
		term4_8		=	term4_75 >> *(strP("xor") >> (term4_75 | epsP[ThrowError("ERROR: expression not found after xor")]))[addCmd(cmdLogXor)];
		term4_85	=	term4_8 >> *(strP("or") >> (term4_8 | epsP[ThrowError("ERROR: expression not found after or")]))[addCmd(cmdLogOr)];
		term4_9		=	term4_85 >> !('?' >> term5 >> ':' >> term5)[addIfElseTermNode];
#ifdef _MSC_VER
#pragma warning(disable: 4709)
#endif
		term5		=	(!(seltype) >>
						variable >> (
						(strP("=") >> term5)[AddSetVariableNode][popType] |
						(strP("+=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '+='")]))[AddModifyVariable<cmdAdd>()][popType] |
						(strP("-=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '-='")]))[AddModifyVariable<cmdSub>()][popType] |
						(strP("*=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '*='")]))[AddModifyVariable<cmdMul>()][popType] |
						(strP("/=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '/='")]))[AddModifyVariable<cmdDiv>()][popType] |
						(strP("**=") >> (term5 | epsP[ThrowError("ERROR: expression not found after '**='")]))[AddModifyVariable<cmdPow>()][popType] |
						(epsP[FailedSetVariable][popType] >> nothingP))
						) |
						term4_9;
#ifdef _MSC_VER
#pragma warning(default: 4709)
#endif

		block		=	chP('{')[blockBegin] >> (code | epsP[ThrowError("ERROR: {} block cannot be empty")]) >> chP('}')[blockEnd];
		expression	=	*chP(';') >> (classdef | (vardef >> +chP(';')) | block | breakexpr | continueExpr | ifexpr | forexpr | whileexpr | doexpr | switchexpr | retexpr | (term5 >> (+chP(';')  | epsP[ThrowError("ERROR: ';' not found after expression")]))[addPopNode]);
		code		=	((funcdef | expression) >> (code[addTwoExprNode] | epsP[addOneExprNode]));
	
		mySpaceP = spaceP | ((strP("//") >> *(anycharP - eolP)) | (strP("/*") >> *(anycharP - strP("*/")) >> strP("*/")));
	}
	void DeInitGrammar() throw()
	{
		mySpaceP.detach();	code.detach();	expression.detach();
		block.detach();	term5.detach();	term4_9.detach();	term4_85.detach();	term4_8.detach();	term4_75.detach();
		term4_7.detach();	term4_65.detach();	term4_6.detach(); term4_4.detach();	term4_2.detach();	term4_1.detach();
		term4.detach();	term3.detach();	term2.detach(); term1.detach();	postExpr.detach();	variable.detach();
		group.detach();	continueExpr.detach();	breakexpr.detach(); retexpr.detach();	switchexpr.detach();	doexpr.detach();
		whileexpr.detach();	forexpr.detach();	ifexpr.detach(); vardef.detach();	vardefsub.detach();	addvarp.detach();
		funcProt.detach();	funcdef.detach();	funcvars.detach(); funccall.detach();	classdef.detach();	varname.detach();
		isconst.detach();	seltype.detach();	arrayDef.detach();
	}
};

UINT buildInFuncs;
UINT buildInTypes;

CompilerError::CompilerError(const std::string& errStr, const char* apprPos)
{
	Init(errStr.c_str(), apprPos);
}
CompilerError::CompilerError(const char* errStr, const char* apprPos)
{
	Init(errStr, apprPos);
}

void CompilerError::Init(const char* errStr, const char* apprPos)
{
	UINT len = (UINT)strlen(errStr) < 128 ? (UINT)strlen(errStr) : 127;
	memcpy(error, errStr, len);
	error[len] = 0;
	if(apprPos)
	{
		const char *begin = apprPos;
		while((begin > codeStart) && (*begin != '\n') && (*begin != '\r'))
			begin--;
		if(begin < apprPos)
			begin++;

		lineNum = 1;
		const char *scan = codeStart;
		while(scan < begin)
			if(*(scan++) == '\n')
				lineNum++;

		const char *end = apprPos;
		while((*end != '\r') && (*end != '\n') && (*end != 0))
			end++;
		len = (UINT)(end - begin) < 128 ? (UINT)(end - begin) : 127;
		memcpy(line, begin, len);
		line[len] = 0;
		shift = (UINT)(apprPos-begin);
	}else{
		line[0] = 0;
		shift = 0;
		lineNum = 0;
	}
}
const char *CompilerError::codeStart = NULL;

Compiler::Compiler()
{
	// Add types
	TypeInfo* info;
	info = new TypeInfo();
	info->name = "void";
	info->size = 0;
	info->type = TypeInfo::TYPE_VOID;
	typeVoid = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 8;
	info->name = "double";
	info->size = 8;
	info->type = TypeInfo::TYPE_DOUBLE;
	typeDouble = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 4;
	info->name = "float";
	info->size = 4;
	info->type = TypeInfo::TYPE_FLOAT;
	typeFloat = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "long";
	info->size = 8;
	info->type = TypeInfo::TYPE_LONG;
	typeLong = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 4;
	info->name = "int";
	info->size = 4;
	info->type = TypeInfo::TYPE_INT;
	typeInt = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "short";
	info->size = 2;
	info->type = TypeInfo::TYPE_SHORT;
	typeShort = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "char";
	info->size = 1;
	info->type = TypeInfo::TYPE_CHAR;
	typeChar = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 4;
	info->name = "float2";
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 4;
	info->name = "float3";
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	info->AddMember("z", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 4;
	info->name = "float4";
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	info->AddMember("z", typeFloat);
	info->AddMember("w", typeFloat);
	typeInfo.push_back(info);

	TypeInfo *typeFloat4 = info;

	info = new TypeInfo();
	info->alignBytes = 8;
	info->name = "double2";
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 8;
	info->name = "double3";
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	info->AddMember("z", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 8;
	info->name = "double4";
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	info->AddMember("z", typeDouble);
	info->AddMember("w", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->alignBytes = 4;
	info->name = "float4x4";
	info->type = TypeInfo::TYPE_COMPLEX;
	info->AddMember("row1", typeFloat4);
	info->AddMember("row2", typeFloat4);
	info->AddMember("row3", typeFloat4);
	info->AddMember("row4", typeFloat4);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "file";
	info->size = 4;
	info->type = TypeInfo::TYPE_COMPLEX;
	typeFile = info;
	typeInfo.push_back(info);

	// Add functions
	FunctionInfo	*fInfo;
	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "cos";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "sin";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "tan";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "ctg";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "ceil";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "floor";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "sqrt";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);

	/*fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "clock";
	fInfo->retType = typeInt;
	fInfo->vTopSize = 1;
	fInfo->funcType = GetFunctionType(fInfo);
	funcInfo.push_back(fInfo);*/

	buildInTypes = (int)typeInfo.size();
	buildInFuncs = (int)funcInfo.size();

	CompilerGrammar::InitGrammar();

}
Compiler::~Compiler()
{
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		delete typeInfo[i]->funcType;
		delete typeInfo[i];
	}
	for(UINT i = 0; i < funcInfo.size(); i++)
		delete funcInfo[i];

	CompilerGrammar::DeInitGrammar();

	for(std::set<VariableInfo*>::iterator s = varInfoAll.begin(), e = varInfoAll.end(); s!=e; s++)
		delete *s;

	varInfoAll.clear();
	varInfo.clear();
	funcInfo.clear();
	typeInfo.clear();
	/*for(UINT i = 0; i < varInfo.size(); i++)
		delete varInfo[i];*/
}

void Compiler::ClearState()
{
	varInfoTop.clear();
	funcInfoTop.clear();

	for(std::set<VariableInfo*>::iterator s = varInfoAll.begin(), e = varInfoAll.end(); s!=e; s++)
		delete *s;
	varInfoAll.clear();

	/*for(UINT i = 0; i < varInfo.size(); i++)
		delete varInfo[i];*/
	varInfo.clear();

	for(unsigned int i = buildInTypes; i < typeInfo.size(); i++)
	{
		delete typeInfo[i]->funcType;
		delete typeInfo[i];
	}
	for(unsigned int i = buildInFuncs; i < funcInfo.size(); i++)
		delete funcInfo[i];

	typeInfo.resize(buildInTypes);
	funcInfo.resize(buildInFuncs);

	callArgCount.clear();
	retTypeStack.clear();
	currDefinedFunc.clear();

	currTypes.clear();

	nodeList.clear();

	varDefined = 0;
	negCount = 0;
	varTop = 24;
	newType = NULL;

	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;
	inplaceArrayNum = 1;

	varInfo.push_back(new VariableInfo("ERROR", 0, typeDouble, true));
	varInfoAll.insert(varInfo.back());
	varInfo.push_back(new VariableInfo("pi", 8, typeDouble, true));
	varInfoAll.insert(varInfo.back());
	varInfo.push_back(new VariableInfo("e", 16, typeDouble, true));
	varInfoAll.insert(varInfo.back());

	varInfoTop.push_back(VarTopInfo(0,0));

	funcInfoTop.push_back(0);

	retTypeStack.push_back(NULL);	//global return can return anything

	arrElementCount.clear();

	logAST.str("");
	compileLog.str("");
	warningLog.str("");
}

bool Compiler::AddExternalFunction(void (NCDECL *ptr)(), const char* prototype)
{
	ClearState();

	ParseResult pRes;

	try{
		pRes = Parse(CompilerGrammar::funcProt, (char*)prototype, CompilerGrammar::mySpaceP);
	}catch(const CompilerError& compileErr){
		compileLog << compileErr;
		return false;
	}
	if(pRes == PARSE_NOTFULL)
		return false;
	if(pRes == PARSE_FAILED)
		return false;

	funcInfo.back()->address = -1;
	funcInfo.back()->funcPtr = (void*)ptr;

	strs.pop_back();
	retTypeStack.pop_back();
	buildInFuncs++;

	FunctionInfo	&lastFunc = *funcInfo.back();
	// Find out the function type
	TypeInfo	*bestFit = NULL;
	// Search through active types
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(typeInfo[i]->funcType)
		{
			if(typeInfo[i]->funcType->retType != lastFunc.retType)
				continue;
			if(typeInfo[i]->funcType->paramType.size() != lastFunc.params.size())
				continue;
			bool good = true;
			for(UINT n = 0; n < lastFunc.params.size(); n++)
			{
				if(lastFunc.params[n].varType != typeInfo[i]->funcType->paramType[n])
				{
					good = false;
					break;
				}
			}
			if(good)
			{
				bestFit = typeInfo[i];
				break;
			}
		}
	}
	// If none found, create new
	if(!bestFit)
	{
		typeInfo.push_back(new TypeInfo());
		typeInfo.back()->funcType = new FunctionType();
		typeInfo.back()->size = 8;
		bestFit = typeInfo.back();

		bestFit->type = TypeInfo::TYPE_COMPLEX;

		bestFit->funcType->retType = lastFunc.retType;
		for(UINT n = 0; n < lastFunc.params.size(); n++)
		{
			bestFit->funcType->paramType.push_back(lastFunc.params[n].varType);
		}
	}
	lastFunc.funcType = bestFit;

	buildInTypes = (int)typeInfo.size();

	return true;
}

bool Compiler::Compile(string str)
{
	ClearState();

	cmdList->Clear();

	if(nodeList.size() != 0)
		nodeList.pop_back();

	ofstream m_FileStream("code.txt", std::ios::binary);
	m_FileStream << str;
	m_FileStream.flush();
	m_FileStream.close();

	char* ptr = (char*)str.c_str();
	CompilerError::codeStart = ptr;

	ofstream m_TempStream("time.txt", std::ios::binary);

	UINT t = clock();
	ParseResult pRes = Parse(CompilerGrammar::code, ptr, CompilerGrammar::mySpaceP);
	if(pRes == PARSE_NOTFULL)
		throw std::string("Parsing wasn't full");
	if(pRes == PARSE_FAILED)
		throw std::string("Parsing failed");
	UINT tem = clock()-t;
	m_TempStream << "Parsing and AST tree gen. time: " << tem * 1000 / CLOCKS_PER_SEC << "ms\r\n";
	
	// Emulate global block end
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBlock(varTop)));

	t = clock();
	if(nodeList.back())
		nodeList.back()->Compile();
	tem = clock()-t;
	m_TempStream << "Compile time: " << tem * 1000 / CLOCKS_PER_SEC << "ms\r\n";

	m_TempStream.flush();
	m_TempStream.close();

	ostringstream		graphlog;
	ofstream graphFile("graph.txt", std::ios::binary);
	if(nodeList.back())
		nodeList.back()->LogToStream(graphlog);
	graphFile << graphlog.str();
	graphFile.close();

	logAST << "\r\n" << warningLog.str();

	compileLog << "\r\nActive types (" << typeInfo.size() << "):\r\n";
	for(UINT i = 0; i < typeInfo.size(); i++)
		compileLog << typeInfo[i]->GetTypeName() << " (" << typeInfo[i]->size << " bytes)\r\n";

	compileLog << "\r\nActive functions (" << funcInfo.size() << "):\r\n";
	for(UINT i = 0; i < funcInfo.size(); i++)
	{
		FunctionInfo &currFunc = *funcInfo[i];
		compileLog << (currFunc.type == FunctionInfo::LOCAL ? "local " : (currFunc.type == FunctionInfo::NORMAL ? "global " : "thiscall ")) << currFunc.retType->GetTypeName() << " " << currFunc.name;
		compileLog << "(";
		for(UINT n = 0; n < currFunc.params.size(); n++)
			compileLog << currFunc.params[n].varType->GetTypeName() << " " << currFunc.params[n].name << (n==currFunc.params.size()-1 ? "" :", ");
		compileLog << ")\r\n";
		if(currFunc.type == FunctionInfo::LOCAL)
		{
			for(UINT n = 0; n < currFunc.external.size(); n++)
				compileLog << "  external var: " << currFunc.external[n] << "\r\n";
		}
	}
	logAST << "\r\n" << compileLog.str();

	for(unsigned int n = 0; n < CodeInfo::funcInfo.size(); n++)
	{
		FunctionInfo* funcInfoPtr = CodeInfo::funcInfo[n];
		
		if (funcInfoPtr->funcPtr && !funcInfoPtr->CreateExternalInfo())
    		throw std::string("External function compilation failed");
	}

	if(nodeList.size() != 1)
		throw std::string("Compilation failed, AST contains more than one node");

	return true; // Зачем тут return true, если вместо return false используются исключения?
}

void Compiler::GenListing()
{
	UINT pos = 0, pos2 = 0;
	CmdID	cmd;

	UINT	valind, valind2;
	USHORT	shVal1, shVal2;
	logASM.str("");

	FunctionInfo *funcInfo;

	char* typeInfoS[] = { "int", "long", "complex", "double" };
	char* typeInfoD[] = { "char", "short", "int", "long", "float", "double", "complex" };

	CmdFlag cFlag;
	OperFlag oFlag;
	while(cmdList->GetData(pos, cmd))
	{
		pos2 = pos;
		pos += 2;

		logASM << pos2;
		switch(cmd)
		{
		case cmdDTOF:
			logASM << " DTOF;";
			break;
		case cmdCallStd:
			cmdList->GetData(pos, funcInfo);
			pos += sizeof(FunctionInfo*);
			logASM << " CALLS " << funcInfo->name << ";";
			break;
		case cmdPushVTop:
			logASM << " PUSHT;";
			break;
		case cmdPopVTop:
			logASM << " POPT;";
			break;
		case cmdCall:
			cmdList->GetUINT(pos, valind);
			pos += sizeof(UINT);
			cmdList->GetUSHORT(pos, shVal1);
			pos += 2;
			logASM << " CALL " << valind;
			if(shVal1 & bitRetSimple)
				logASM << " simple";
			logASM << " size:" << (shVal1&0x0FFF) << ";";
			break;
		case cmdReturn:
			cmdList->GetUSHORT(pos, shVal1);
			pos += 2;
			cmdList->GetUSHORT(pos, shVal2);
			pos += 2;
			logASM << " RET " << shVal2;
			if(shVal1 & bitRetError)
			{
				logASM << " ERROR;";
				break;
			}
			if(shVal1 & bitRetSimple)
			{
				switch(shVal1 & 0x0FFF)
				{
				case OTYPE_DOUBLE:
					logASM << " double;";
					break;
				case OTYPE_LONG:
					logASM << " long;";
					break;
				case OTYPE_INT:
					logASM << " int;";
					break;
				}
			}else{
				logASM << " bytes: " << shVal1;
			}
			break;
		case cmdPushV:
			{
				int valind;
				cmdList->GetData(pos, &valind, sizeof(int));
				pos += sizeof(int);
				logASM << " PUSHV " << valind << dec << ";";
			}
			break;
		case cmdNop:
			logASM << dec << " NOP;";
			break;
		case cmdCTI:
			logASM << dec << " CTI addr*";
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += sizeof(UINT);
			logASM << valind;

			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double;";
				break;
			case OTYPE_LONG:
				logASM << " long;";
				break;
			case OTYPE_INT:
				logASM << " int;";
				break;
			default:
				logASM << "ERROR: OperFlag expected after ";
			}
			break;
		case cmdPushImmt:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				logASM << " PUSHIMMT ";
				logASM << typeInfoS[cFlag&0x00000003] << "<-";
				logASM << typeInfoD[(cFlag>>2)&0x00000007];

				asmDataType dt = flagDataType(cFlag);
				UINT	DWords[2];
				USHORT sdata;
				UCHAR cdata;
				
				if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG){
					cmdList->GetUINT(pos, DWords[0]); pos += 4;
					cmdList->GetUINT(pos, DWords[1]); pos += 4;
				}
				if(dt == DTYPE_FLOAT || dt == DTYPE_INT){ cmdList->GetUINT(pos, DWords[0]); pos += 4; }
				if(dt == DTYPE_SHORT){ cmdList->GetUSHORT(pos, sdata); pos += 2; DWords[0] = sdata; }
				if(dt == DTYPE_CHAR){ cmdList->GetUCHAR(pos, cdata); pos += 1; DWords[0] = cdata; }

				if(dt == DTYPE_DOUBLE)
					logASM << " (" << *((double*)(&DWords[0])) << ')';
				if(dt == DTYPE_LONG)
					logASM << " (" << *((long long*)(&DWords[0])) << ')';
				if(dt == DTYPE_FLOAT)
					logASM << " (" << *((float*)(&DWords[0])) << dec << ')';
				if(dt == DTYPE_INT)
					logASM << " (" << *((int*)(&DWords[0])) << dec << ')';
				if(dt == DTYPE_SHORT)
					logASM << " (" << *((short*)(&DWords[0])) << dec << ')';
				if(dt == DTYPE_CHAR)
					logASM << " (" << *((char*)(&DWords[0])) << ')';
			}
			break;
		case cmdPush:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				logASM << " PUSH ";
				logASM << typeInfoS[cFlag&0x00000003] << "<-";
				logASM << typeInfoD[(cFlag>>2)&0x00000007];

				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);
				UINT	DWords[2];
				USHORT sdata;
				UCHAR cdata;
				int valind;
				if(flagNoAddr(cFlag)){
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG){
						cmdList->GetUINT(pos, DWords[0]); pos += 4;
						cmdList->GetUINT(pos, DWords[1]); pos += 4;
					}
					if(dt == DTYPE_FLOAT || dt == DTYPE_INT){ cmdList->GetUINT(pos, DWords[0]); pos += 4; }
					if(dt == DTYPE_SHORT){ cmdList->GetUSHORT(pos, sdata); pos += 2; DWords[0] = sdata; }
					if(dt == DTYPE_CHAR){ cmdList->GetUCHAR(pos, cdata); pos += 1; DWords[0] = cdata; }

					if(dt == DTYPE_DOUBLE)
						logASM << " (" << *((double*)(&DWords[0])) << ')';
					if(dt == DTYPE_LONG)
						logASM << " (" << *((long long*)(&DWords[0])) << ')';
					if(dt == DTYPE_FLOAT)
						logASM << " (" << *((float*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_INT)
						logASM << " (" << *((int*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_SHORT)
						logASM << " (" << *((short*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_CHAR)
						logASM << " (" << *((char*)(&DWords[0])) << ')';
				}else{
					logASM << " PTR[";
					if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
						logASM << valind;
					}
					if(flagAddrRel(cFlag))
						logASM << "+top";
					if(flagAddrRelTop(cFlag))
						logASM << "+max";
					if(flagShiftStk(cFlag))
						logASM << "+shift(stack)";
					
					logASM << "] ";
					if(flagSizeStk(cFlag))
						logASM << "size(stack)";
					if(flagSizeOn(cFlag))
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
						logASM << " max size(" << valind << ") ";
					}
					if(st == STYPE_COMPLEX_TYPE)
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
						logASM << "sizeof(" << valind << ")";
					}
				}
			}
			
			break;
		case cmdMovRTaP:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;

				logASM << " MOVRTAP ";
				logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
				int valind;
				cmdList->GetINT(pos, valind);
				pos += 4;
				logASM << valind;
				logASM << "+max] ";
				if(flagDataType(cFlag) == DTYPE_COMPLEX_TYPE)
				{
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "sizeof(" << valind << ")";
				}
			}
			break;
		case cmdMov:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				logASM << " MOV ";
				logASM << typeInfoS[cFlag&0x00000003] << "->";
				logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
				asmStackType st = flagStackType(cFlag);
				int valind;
				
				if(flagAddrRel(cFlag) || flagAddrAbs(cFlag) || flagAddrRelTop(cFlag))
				{
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << valind;
				}
				if(flagAddrRel(cFlag))
					logASM << "+top";

				if(flagAddrRelTop(cFlag))
					logASM << "+max";

				if(flagShiftStk(cFlag))
					logASM << "+shift(stack)";

				logASM << "] ";
				if(flagSizeStk(cFlag))
					logASM << "size(stack)";
				if(flagSizeOn(cFlag))
				{
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "size: " << valind;
				}
				if(st == STYPE_COMPLEX_TYPE)
				{
					cmdList->GetINT(pos, valind);
					pos += 4;
					logASM << "sizeof(" << valind << ")";
				}
			}
			break;
		case cmdPop:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << " POP" << " sizeof(" << valind << ")";
			break;
		case cmdRTOI:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << " RTOI ";
			logASM << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdITOR:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << " ITOR ";
			logASM << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdITOL:
			logASM << " ITOL";
			break;
		case cmdLTOI:
			logASM << " LTOI";
			break;
		case cmdSwap:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << " SWAP ";
			logASM << typeInfoS[cFlag&0x00000003] << "<->";
			logASM << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdCopy:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << " COPY ";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double;";
				break;
			case OTYPE_LONG:
				logASM << " long;";
				break;
			case OTYPE_INT:
				logASM << " int;";
				break;
			}
			break;
		case cmdJmp:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << " JMP " << valind;
			break;
		case cmdJmpZ:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << " JMPZ";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double";
				break;
			case OTYPE_LONG:
				logASM << " long";
				break;
			case OTYPE_INT:
				logASM << " int";
				break;
			}
			logASM << ' ' << valind << ';';
			break;
		case cmdJmpNZ:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << " JMPNZ";
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double";
				break;
			case OTYPE_LONG:
				logASM << " long";
				break;
			case OTYPE_INT:
				logASM << " int";
				break;
			}
			logASM << ' ' << valind << ';';
			break;
		case cmdSetRange:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			cmdList->GetUINT(pos, valind2);
			pos += 4;
			logASM << " SETRANGE " << typeInfoD[(cFlag>>2)&0x00000007] << " " << valind << " " << valind2 << ';';
			break;
		case cmdGetAddr:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << " GETADDR " << (int)valind << ';';
			break;
		case cmdFuncAddr:
			{
				FunctionInfo	*fInfo;
				cmdList->GetData(pos, fInfo);
				pos += sizeof(FunctionInfo*);
				logASM << " FUNCADDR " << fInfo->name;
				break;
			}
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << ' ';
			switch(cmd)
			{
			case cmdAdd:
				logASM << "ADD";
				break;
			case cmdSub:
				logASM << "SUB";
				break;
			case cmdMul:
				logASM << "MUL";
				break;
			case cmdDiv:
				logASM << "DIV";
				break;
			case cmdPow:
				logASM << "POW";
				break;
			case cmdMod:
				logASM << "MOD";
				break;
			case cmdLess:
				logASM << "LES";
				break;
			case cmdGreater:
				logASM << "GRT";
				break;
			case cmdLEqual:
				logASM << "LEQL";
				break;
			case cmdGEqual:
				logASM << "GEQL";
				break;
			case cmdEqual:
				logASM << "EQL";
				break;
			case cmdNEqual:
				logASM << "NEQL";
				break;
			case cmdShl:
				logASM << "SHL";
				if(oFlag == OTYPE_DOUBLE)
					throw string("Invalid operation: SHL used on float");
				break;
			case cmdShr:
				logASM << "SHR";
				if(oFlag == OTYPE_DOUBLE)
					throw string("Invalid operation: SHR used on float");
				break;
			case cmdBitAnd:
				logASM << "BAND";
				if(oFlag == OTYPE_DOUBLE)
					throw string("Invalid operation: BAND used on float");
				break;
			case cmdBitOr:
				logASM << "BOR";
				if(oFlag == OTYPE_DOUBLE)
					throw string("Invalid operation: BOR used on float");
				break;
			case cmdBitXor:
				logASM << "BXOR";
				if(oFlag == OTYPE_DOUBLE)
					throw string("Invalid operation: BXOR used on float");
				break;
			case cmdLogAnd:
				logASM << "LAND";
				break;
			case cmdLogOr:
				logASM << "LOR";
				break;
			case cmdLogXor:
				logASM << "LXOR";
				break;
			}
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double;";
				break;
			case OTYPE_LONG:
				logASM << " long;";
				break;
			case OTYPE_INT:
				logASM << " int;";
				break;
			default:
				logASM << "ERROR: OperFlag expected after instruction";
			}
		}
		if(cmd >= cmdNeg && cmd <= cmdLogNot)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << ' ';
			switch(cmd)
			{
			case cmdNeg:
				logASM << "NEG";
				break;
			case cmdBitNot:
				logASM << "BNOT";
				if(oFlag == OTYPE_DOUBLE)
					throw string("Invalid operation: BNOT used on float");
				break;
			case cmdLogNot:
				logASM << "LNOT;";
				break;
			}
			switch(oFlag)
			{
			case OTYPE_DOUBLE:
				logASM << " double;";
				break;
			case OTYPE_LONG:
				logASM << " long;";
				break;
			case OTYPE_INT:
				logASM << " int;";
				break;
			default:
				logASM << "ERROR: OperFlag expected after ";
			}
		}
		if(cmd >= cmdIncAt && cmd <= cmdDecAt)
		{
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			if(cmd == cmdIncAt)
				logASM << " INCAT ";
			if(cmd == cmdDecAt)
				logASM << " DECAT ";
			logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";

			int valind;

			if(flagAddrRel(cFlag) || flagAddrAbs(cFlag)){
				cmdList->GetINT(pos, valind);
				pos += 4;
				logASM << valind;
			}
			if(flagAddrRel(cFlag))
				logASM << "+top";

			if(flagShiftStk(cFlag)){
				logASM << "+shift";
			}
			logASM << "] ";
			if(flagSizeStk(cFlag)){
				logASM << "size: stack";
			}
			if(flagSizeOn(cFlag)){
				cmdList->GetINT(pos, valind);
				pos += 4;
				logASM << "size: " << valind;
			}
		}
		logASM << "\r\n";
	}

	ofstream m_FileStream("asm.txt", std::ios::binary);
	m_FileStream << logASM.str();
	m_FileStream.flush();
}

string Compiler::GetListing()
{
	return logASM.str();
}

string Compiler::GetLog()
{
	return logAST.str();
}

#if defined(_MSC_VER)
bool FunctionInfo::CreateExternalInfo()
{
	externalInfo.bytesToPop = 0;
	for(UINT i = 0; i < params.size(); i++)
	{
		UINT paramSize = params[i].varType->size > 4 ? params[i].varType->size : 4;
		externalInfo.bytesToPop += paramSize;
	}
	
	return true;
}
#elif defined(__CELLOS_LV2__)
bool FunctionInfo::CreateExternalInfo()
{
    unsigned int rCount = 0, fCount = 0;
    unsigned int rMaxCount = sizeof(externalInfo.rOffsets) / sizeof(externalInfo.rOffsets[0]);
    unsigned int fMaxCount = sizeof(externalInfo.fOffsets) / sizeof(externalInfo.fOffsets[0]);
    
    // parse all parameters, fill offsets
    unsigned int offset = 0;
    
	for (UINT i = 0; i < params.size(); i++)
	{
	    const TypeInfo& type = *params[i].varType;
	    
	    switch (type.type)
	    {
	    case TypeInfo::TYPE_CHAR:
	    case TypeInfo::TYPE_SHORT:
	    case TypeInfo::TYPE_INT:
	        if (rCount >= rMaxCount) return false; // too many r parameters
	        externalInfo.rOffsets[rCount++] = offset;
	        offset++;
	        break;
	    
	    case TypeInfo::TYPE_FLOAT:
	    case TypeInfo::TYPE_DOUBLE:
	        if (fCount >= fMaxCount || rCount >= rMaxCount) return false; // too many f/r parameters
	        externalInfo.rOffsets[rCount++] = offset;
	        externalInfo.fOffsets[fCount++] = offset;
	        offset += 2;
	        break;
	        
	    default:
	        return false; // unsupported type
	    }
    }
    
    // clear remaining offsets
    for (unsigned int i = rCount; i < rMaxCount; ++i) externalInfo.rOffsets[i] = 0;
    for (unsigned int i = fCount; i < fMaxCount; ++i) externalInfo.fOffsets[i] = 0;
    
    return true;
}
#endif
