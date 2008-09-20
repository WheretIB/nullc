#include "stdafx.h"
#include "SupSpi.h"
using namespace supspi;

#include "ParseCommand.h"
#include "ParseClass.h"
#include "Compiler.h"

//////////////////////////////////////////////////////////////////////////
//						Code gen ops
//////////////////////////////////////////////////////////////////////////
// Информация о функциях
std::vector<FunctionInfo*>	funcs;
// Информация о переменных
std::vector<VariableInfo>	varInfo;
// Информация о вершинах стека переменных. При компиляции он служит для того, чтобы
// Удалять информацию о переменных, когда они выходят из области видимости
std::vector<VarTopInfo>		varInfoTop;
// Некоторые конструкции допускают оператор break, который должен знать, на сколько сдвинуть базу стека
// переменных, чтобы привести её в то состояние, в которым она находилась бы, если бы конструкция
// завершилась без преждевременного выхода. Этот стек (конструкции могут быть вложенными) хранит размер
// varInfoTop.
std::vector<UINT>			undComandIndex;

// Массив с информацией о типах
std::vector<TypeInfo*>		typeInfo;
// Немного предопределённых базовых типов
TypeInfo*	typeVoid = NULL;
TypeInfo*	typeChar = NULL;
TypeInfo*	typeShort = NULL;
TypeInfo*	typeInt = NULL;
TypeInfo*	typeFloat = NULL;
TypeInfo*	typeLong = NULL;
TypeInfo*	typeDouble = NULL;

// Функция возвращает тип - указателя на исходный
TypeInfo* GetReferenceType(TypeInfo* type)
{
	// Поищем нужный тип в списке
	UINT targetRefLevel = type->refLevel+1;
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(type->name == typeInfo[i]->name && targetRefLevel == typeInfo[i]->refLevel)
			return typeInfo[i];
	}
	// Создадим новый тип
	TypeInfo* newInfo = new TypeInfo();
	newInfo->name = type->name;
	newInfo->size = 4;
	newInfo->type = TypeInfo::POD_INT;
	newInfo->refLevel = type->refLevel + 1;

	typeInfo.push_back(newInfo);
	return newInfo;
}

// Функиця возвращает тип, получаемый при разименовании указателя
TypeInfo* GetDereferenceType(TypeInfo* type)
{
	// Поищем нужный тип в списке
	UINT targetRefLevel = type->refLevel-1;
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(type->name == typeInfo[i]->name && targetRefLevel == typeInfo[i]->refLevel)
			return typeInfo[i];
	}
	throw std::string("Cannot dereference type ") + type->name + std::string(" there is no result type available");
}

// Информация о типе текцщей переменной
TypeInfo*	currType = NULL;
// Стек ( :) )такой информации
// Для конструкций arr[arr[i.a.b].y].x;
std::vector<TypeInfo*>	currTypes;
std::vector<bool>		valueByRef;

bool currValueByRef = false;
void pushValueByRef(char const*s, char const*e){ valueByRef.push_back(currValueByRef); }
void popValueByRef(char const*s, char const*e){ valueByRef.pop_back(); }

// Список узлов дерева
// Отдельные узлы помещаются сюда, и в дальнейшем объеденяются в более комплексные узлы,
// создавая дерево. После правильной компиляции количество узлов в этом массиве должно равнятся 1
std::vector<shared_ptr<NodeZeroOP> >	nodeList;

// Temp variables
// Временные переменные:
// Количество минусов перед переменной, количество определённых переменных, размер определённых переменных
// Вершина стека переменных
UINT negCount, varDefined, varSize, varTop;
// Является ли текущая переменная константной
bool currValConst;

// Массив временных строк
std::vector<std::string>	strs;
// Стек с количеством переменных переданных функции.
// Стек для ситуаций вроде foo(1, bar(2, 3, 4), 5), когда нужно сохранить количество параметров,
// для каждой из функцией для проверки количества переданных параметров с тем, которое принимает функция
std::vector<UINT>			callArgCount;
// Стек, который хранит титы значений, которые возвращает функция.
// Функции можно определять одну в другой (BUG: 0004 может, нафиг не надо?)
std::vector<TypeInfo*>		retTypeStack;

// Преобразовать строку в число типа long long
long long atoll(const char* str)
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
		throw std::string("ERROR: The name '" + str + "' is reserved");
	for(UINT i = 0; i < funcs.size(); i++)
		if(funcs[i]->name == str)
			throw std::string("ERROR: Name '" + str + "' is already taken for a function");
}

// Вызывается в начале блока {}, чтобы сохранить количество определённых переменных, к которому можно
// будет вернутся после окончания блока.
void blockBegin(char const* s, char const* e)
{
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
}
// Вызывается в конце блока {}, чтобы убрать информацию о переменных внутри блока, тем самым обеспечивая
// их выход из области видимости. Также уменьшает вершину стека переменных в байтах.
void blockEnd(char const* s, char const* e)
{
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{ 
		varTop -= varInfo.back().count*varInfo.back().varType->size;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}

// Функции для добавления узлов с константными числами разных типов
template<typename T>
void addNumberNode(char const*s, char const*e);

template<> void addNumberNode<int>(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(atoi(s), typeInt)));
};
template<> void addNumberNode<float>(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<float>((float)atof(s), typeFloat)));
};
template<> void addNumberNode<long long>(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<long long>(atoll(s), typeLong)));
};
template<> void addNumberNode<double>(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<double>(atof(s), typeDouble)));
};

// Функция для создания узла, который уберёт значение со стека переменных
// Узел заберёт к себе последний узел в списке.
void addPopNode(char const* s, char const* e)
{
	// Если последний узел в списке - узел с цислом, уберём его
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
	{
		nodeList.pop_back();
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
	}else if((*(nodeList.end()-1))->GetNodeType() == typeNodePreValOp){
		// Если последний узел, это переменная, которую уменьшают или увеличивают на 1, не используя в
		// далнейшем её значение, то можно произвести оптимизацию кода.
		static_cast<NodePreValOp*>(nodeList.back().get())->SetOptimised(true);
	}else{
		// Иначе просто создадим узёл, как и планировали в начале
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePopOp()));
	}
}

// Функция для создания узла, которые поменяет знак значения в стеке
// Узел заберёт к себе последний узел в списке.
void addNegNode(char const* s, char const* e)
{
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
			throw std::string("addBitNotNode() ERROR: unknown type ") + aType->name;
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
			throw std::string("addBitNotNode() ERROR: unknown type ") + aType->name;
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
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
	{
		TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
		NodeZeroOP* zOP = (nodeList.end()-1)->get();
		shared_ptr<NodeZeroOP > Rd;
		if(aType == typeDouble)
		{
			throw std::string("ERROR: bitwise NOT cannot be used on floating point numbers");
		}else if(aType == typeFloat){
			throw std::string("ERROR: bitwise NOT cannot be used on floating point numbers");
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo()));
		}else{
			throw std::string("addBitNotNode() ERROR: unknown type ") + aType->name;
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
	throw std::string("ERROR: optDoSpecial call with unknown type");
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
	throw std::string("ERROR: optDoSpecial<int> call with unknown command");
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
	throw std::string("ERROR: optDoSpecial<long long> call with unknown command");
}
template<> double optDoSpecial<>(CmdID cmd, double a, double b)
{
	if(cmd == cmdShl)
		throw std::string("ERROR: optDoSpecial<double> call with << operation is illegal");
	if(cmd == cmdShr)
		throw std::string("ERROR: optDoSpecial<double> call with >> operation is illegal");
	if(cmd == cmdMod)
		return fmod(a,b);
	if(cmd >= cmdBitAnd && cmd <= cmdBitXor)
		throw std::string("ERROR: optDoSpecial<double> call with binary operation is illegal");
	if(cmd == cmdLogAnd)
		return (int)a && (int)b;
	if(cmd == cmdLogXor)
		return !!(int)a ^ !!(int)b;
	if(cmd == cmdLogOr)
		return (int)a || (int)b;
	throw std::string("ERROR: optDoSpecial<double> call with unknown command");
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
		if(bNodeType != typeNodeTwoAndCmdOp && bNodeType != typeNodeVarGet)
		{
			// Иначе, выходим без оптимизаций
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoAndCmdOp(id)));
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
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoAndCmdOp(id)));
}

template<CmdID cmd> void createTwoAndCmd(char const* s, char const* e)
{
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
	throw std::string("ERROR: addCmd call with unknown command");
	return &createTwoAndCmd<cmdReturn>;
}

void addReturnNode(char const* s, char const* e)
{
	int t = (int)varInfoTop.size();
	int c = 0;
	if(funcs.size() != 0)
		while(t > (int)funcs.back()->vTopSize)
		{
			c++;
			t--;
		}
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeReturnOp(c, retTypeStack.back())));
	nodeList.back()->SetCodeInfo(s, e);
}

void addBreakNode(char const* s, char const* e)
{
	if(undComandIndex.empty())
		throw std::string("ERROR: break used outside loop statements");
	int t = (int)varInfoTop.size();
	int c = 0;
	while(t > (int)undComandIndex.back())
	{
		c++;
		t--;
	}
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBreakOp(c)));
}

//Finds TypeInfo in a typeInfo list by name
void selType(char const* s, char const* e)
{
	string vType = std::string(s,e);
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(typeInfo[i]->name == vType)
		{
			currType = typeInfo[i];
			return;
		}
	}
	throw std::string("ERROR: Variable type '" + vType + "' is unknown\r\n");
}

void addVar(char const* s, char const* e)
{
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');

	for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		if(varInfo[i].name == vName)
			throw std::string("ERROR: Name '" + vName + "' is already taken for a variable in current scope\r\n");
	checkIfDeclared(vName);

	if(varSize*currType->size > 64*1024*1024)
		throw std::string("ERROR: variable '" + vName + "' has to big length (>64 Mb)");
	
	varInfo.push_back(VariableInfo(vName, varTop, currType, varSize, currValConst));
	varDefined += varSize-1;
	varTop += varSize*currType->size;
	varSize = 1;
}

void addVarDefNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarDef(varDefined*currType->size, strs.back())));
	varDefined = 0;
}

void pushType(char const* s, char const* e)
{
	currTypes.push_back(currType);
}

void popType(char const* s, char const* e)
{
	currTypes.pop_back();
}

bool pushedShiftAddrNode = false;
bool pushedShiftAddr = false;

void popTypeAndAddrNode(char const* s, char const* e)
{
	currTypes.pop_back();
	if(pushedShiftAddr || pushedShiftAddrNode)
		nodeList.pop_back();
}

void getType(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = strs.back();
	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not defined [set]");
	currTypes.push_back(varInfo[i].varType);
	pushedShiftAddr = false;
	pushedShiftAddrNode = false;
}

void getMember(char const* s, char const* e)
{
	string vName = std::string(s, e);
	// Да, это локальная переменная с именем, как у глобальной!
	TypeInfo *currType = currTypes.back();

	if(currTypes.back()->refLevel != 0)
		throw std::string("ERROR: references do not have members \"." + vName + "\"\r\n  try using \"->" + vName + "\"");
	int i = (int)currType->memberData.size()-1;
	while(i >= 0 && currType->memberData[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + vName + "' is not a member of '" + currType->name + "' [set]");
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(currType->memberData[i].offset, typeInt)));
	if(pushedShiftAddrNode | pushedShiftAddr)
		addTwoAndCmpNode(cmdAdd);
	pushedShiftAddrNode = false;
	pushedShiftAddr = true;
	currTypes.back() = currType->memberData[i].type;
}

void getAddress(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = strs.back();

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not defined [getaddr]");

	if(((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0)
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(varInfo[i].pos, typeInt)));
	else
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeGetAddress(varInfo[i], varInfo[i].pos-(int)(varInfoTop.back().varStackSize))));

	pushedShiftAddrNode = false;
	pushedShiftAddr = true;
}

void addAddressNode(char const* s, char const* e)
{
	if(nodeList.back()->GetNodeType() != typeNodeNumber)
		throw std::string("ERROR: addAddressNode() can't find a \r\n  number node on the top of node list");
	if(nodeList.back()->GetTypeInfo() != typeInt)
		throw std::string("ERROR: addAddressNode(): number node type is not int");

	shared_ptr<NodeZeroOP> temp = nodeList.back();
	nodeList.back().reset(new NodeNumber<int>(static_cast<NodeNumber<int>*>(temp.get())->GetVal(), GetReferenceType(currTypes.back())));
	//nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpression(GetReferenceType(currTypes.back()))));
	currTypes.pop_back();
	valueByRef.push_back(false);
}

void convertTypeToRef(char const* s, char const* e)
{
	currType = GetReferenceType(currType);
}

void addDereference(char const* s, char const* e)
{
	if(currTypes.back()->refLevel == 0)
		throw std::string("ERROR: cannot dereference ") + std::string(s, e);
	currTypes.push_back(currTypes.back());
	currTypes[currTypes.size()-2] = GetDereferenceType(currTypes.back());
	if(currTypes[currTypes.size()-2]->refLevel == 0)
		pushedShiftAddr = true;
	valueByRef.push_back(true);
	valueByRef.push_back(false);
}

void addShiftAddrNode(char const* s, char const* e)
{
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
	{
		TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
		NodeZeroOP* zOP = (nodeList.end()-1)->get();
		shared_ptr<NodeZeroOP > Rd;
		if(aType == typeDouble)
		{
			Rd.reset(new NodeNumber<int>(int(currTypes.back()->size*static_cast<NodeNumber<double>* >(zOP)->GetVal()), typeInt));
		}else if(aType == typeFloat){
			Rd.reset(new NodeNumber<int>(int(currTypes.back()->size*static_cast<NodeNumber<float>* >(zOP)->GetVal()), typeInt));
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<int>(int(currTypes.back()->size*static_cast<NodeNumber<long long>* >(zOP)->GetVal()), typeInt));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(int(currTypes.back()->size*static_cast<NodeNumber<int>* >(zOP)->GetVal()), typeInt));
		}else{
			throw std::string("addBitNotNode() ERROR: unknown type ") + aType->name;
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePushShift(currTypes.back()->size)));
	}
	
	pushedShiftAddrNode = true;
}

void addSetNode(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = strs.back().find('.');

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + vName + "' is not defined [set]");
	if(!currValConst && varInfo[i].isConst)
		throw std::string("ERROR: cannot change constant parameter '" + strs.back() + "' ");
	if(braceInd != -1 && varInfo[i].count == 1)
		throw std::string("ERROR: variable '" + vName + "' is not an array");
	if(braceInd == -1 && varInfo[i].count > 1)
		throw std::string("ERROR: variable '" + vName + "' is an array, but no index specified");

	bool aabsadr = ((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;
	int ashift = aabsadr ? 0 : varInfoTop.back().varStackSize;

	if(!valueByRef.empty() && valueByRef.back())
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSet(varInfo[i], currTypes.back(), 0, true, false, true, varDefined*currType->size)));
	else
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSet(varInfo[i], currTypes.back(), varInfo[i].pos-ashift, compoundType != -1, varDefined != 0 && braceInd != -1, aabsadr, varDefined*currType->size)));
	valueByRef.pop_back();
	currTypes.pop_back();

	currValConst = false;
	varDefined = 0;
}

void addGetNode(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = strs.back().find('.');

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + vName + "' is not defined [get]");
	if(braceInd != -1 && varInfo[i].count == 1)
		throw std::string("ERROR: variable '" + vName + "' is not an array");
	if(braceInd == -1 && varInfo[i].count > 1)
		throw std::string("ERROR: variable '" + vName + "' is an array, but no index specified");

	if(valueByRef.back())
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), 0, true, true)));
	else if(((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0)
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), varInfo[i].pos, compoundType != -1, true)));
	else
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), varInfo[i].pos-(int)(varInfoTop.back().varStackSize), compoundType != -1, false)));
	
	valueByRef.pop_back();
	currTypes.pop_back();
}

void addGetByRef(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + vName + "' is not defined [get]");

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), 0, true, true)));
	currTypes.pop_back();
}

void addSetAndOpNode(CmdID cmd)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = strs.back().find('.');

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable " + strs.back() + " is not defined");
	if(!currValConst && varInfo[i].isConst)
		throw std::string("ERROR: cannot change constant parameter '" + strs.back() + "' ");
	if(braceInd == -1 && varInfo[i].count > 1)
		throw std::string("ERROR: variable '" + strs.back() + "' is an array, but no index specified");

	bool aabsadr = ((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;
	int ashift = aabsadr ? 0 : varInfoTop.back().varStackSize;

	if(valueByRef.back())
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSetAndOp(varInfo[i], currTypes.back(), 0, true, true, cmd)));
	else 
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSetAndOp(varInfo[i], currTypes.back(), varInfo[i].pos-ashift, compoundType != -1, aabsadr, cmd)));

	valueByRef.pop_back();
	currTypes.pop_back();

	varDefined = 0;
}
void addAddSetNode(char const* s, char const* e)
{
	addSetAndOpNode(cmdAdd);
}
void addSubSetNode(char const* s, char const* e)
{
	addSetAndOpNode(cmdSub);
}
void addMulSetNode(char const* s, char const* e)
{
	addSetAndOpNode(cmdMul);
}
void addDivSetNode(char const* s, char const* e)
{
	addSetAndOpNode(cmdDiv);
}
void addPowSetNode(char const* s, char const* e)
{
	addSetAndOpNode(cmdPow);
}

void addPreOpNode(CmdID cmd, bool pre)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = strs.back().find('.');

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw std::string("ERROR: variable '" + strs.back() + "' is not defined [set]");
	if(!currValConst && varInfo[i].isConst)
		throw std::string("ERROR: cannot change constant parameter '" + strs.back() + "' ");
	if(braceInd == -1 && varInfo[i].count > 1)
		throw std::string("ERROR: variable '" + strs.back() + "' is an array, but no index specified");

	bool aabsadr = ((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;
	int ashift = aabsadr ? 0 : varInfoTop.back().varStackSize;

	if(valueByRef.back())
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePreValOp(varInfo[i], currTypes.back(), 0, true, true, cmd, pre)));
	else
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePreValOp(varInfo[i], currTypes.back(), varInfo[i].pos-ashift, compoundType != -1, aabsadr, cmd, pre)));
	valueByRef.pop_back();
	currTypes.pop_back();
}
void addPreDecNode(char const* s, char const* e)
{
	addPreOpNode(cmdDecAt, true);
}
void addPreIncNode(char const* s, char const* e)
{
	addPreOpNode(cmdIncAt, true);
}
void addPostDecNode(char const* s, char const* e)
{
	addPreOpNode(cmdDecAt, false);
}
void addPostIncNode(char const* s, char const* e)
{
	addPreOpNode(cmdIncAt, false);
}

void addOneExprNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList()));
}
void addTwoExprNode(char const* s, char const* e)
{
	if(nodeList.back()->GetNodeType() != typeNodeExpressionList)
		addOneExprNode(s, e);//throw std::string("addTwoExprNode, no NodeExpressionList found");
	// Take the expression list from the top
	shared_ptr<NodeZeroOP> temp = nodeList.back();
	nodeList.pop_back();
	static_cast<NodeExpressionList*>(temp.get())->AddNode();
	nodeList.push_back(temp);
}
void addBlockNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBlock()));
}

void funcAdd(char const* s, char const* e)
{
	for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		if(varInfo[i].name == strs.back())
			throw std::string("ERROR: Name '" + strs.back() + "' is already taken for a variable in current scope\r\n");
	checkIfDeclared(strs.back());
	funcs.push_back(new FunctionInfo());
	funcs.back()->name = strs.back();
	funcs.back()->vTopSize = (UINT)varInfoTop.size();
	retTypeStack.push_back(currType);
	funcs.back()->retType = currType;
}
void funcParam(char const* s, char const* e)
{
	funcs.back()->params.push_back(VariableInfo(strs.back(), 0, currType, 1, currValConst));
	strs.pop_back();
}
void funcStart(char const* s, char const* e)
{
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
	
	for(int i = (int)funcs.back()->params.size()-1; i >= 0; i--)
	{
		strs.push_back(funcs.back()->params[i].name);
		strs.push_back(funcs.back()->params[i].name);

		currValConst = funcs.back()->params[i].isConst;
		currType = funcs.back()->params[i].varType;
		addVar(0,0);

		strs.pop_back();
		strs.pop_back();
	}
}
void funcEnd(char const* s, char const* e)
{
	int i = (int)funcs.size()-1;
	while(i >= 0 && funcs[i]->name != strs.back())
		i--;

	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop -= varInfo.back().count*varInfo.back().varType->size;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncDef(i)));
	strs.pop_back();
	retTypeStack.pop_back();
}


void addFuncPushParamNode(char const* s, char const* e)
{
	string fname = strs.back();
	if(fname == "cos" || fname == "sin" || fname == "tan" || fname == "ctg" || fname == "ceil" || fname == "floor" || 
		fname == "sqrt" || fname == "clock")
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncParam(typeDouble, callArgCount.back(), true)));
	}else{
		int i = (int)funcs.size()-1;
		while(i >= 0 && funcs[i]->name != fname)
			i--;
		if(i == -1)
			throw std::string("ERROR: function " + fname + " is undefined");
		VariableInfo* vinfo = &funcs[i]->params[callArgCount.back()-1];
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncParam(vinfo->varType, callArgCount.back(), false)));
	}
}
void addFuncCallNode(char const* s, char const* e)
{
	string fname = strs.back();
	strs.pop_back();

	char strCnt[32];
	//Find standard function
	if(fname == "cos" || fname == "sin" || fname == "tan" || fname == "ctg" || fname == "ceil" || fname == "floor" || 
		fname == "sqrt" || fname == "clock")
	{
		if(fname == "clock" && callArgCount.back() != 0)
			throw std::string("ERROR: function " + fname + " takes no argumets\r\n");
		if(fname != "clock" && callArgCount.back() != 1)
			throw std::string("ERROR: function " + fname + " takes one argument\r\n");
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncCall(fname, -1, callArgCount.back(), (fname == "clock" ? typeInt : typeDouble))));
	}else{	//Find user-defined function
		int i = (int)funcs.size()-1;
		while(i >= 0 && funcs[i]->name != fname)
			i--;
		if(i == -1)
			throw std::string("ERROR: function " + fname + " is undefined");
		if(funcs[i]->params.size() != callArgCount.back())
			throw std::string("ERROR: function ") + fname + std::string(" takes ") + _itoa((int)funcs[i]->params.size(), strCnt, 10) + std::string(" argument(s)\r\n");

		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncCall(fname, i, callArgCount.back(), funcs[i]->retType)));
	}
	callArgCount.pop_back();
}

void addIfNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(false)));
}
void addIfElseNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(true)));
}
void addIfElseTermNode(char const* s, char const* e)
{
	TypeInfo* typeA = nodeList[nodeList.size()-1]->GetTypeInfo();
	TypeInfo* typeB = nodeList[nodeList.size()-2]->GetTypeInfo();
	if(typeA != typeB)
		throw std::string("ERROR: trinary operator ?: \r\n result types are not equal (" + typeB->name + " : " + typeA->name + ")");
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeIfElseExpr(true, true)));
}

void saveVarTop(char const* s, char const* e)
{
	undComandIndex.push_back((UINT)varInfoTop.size());
}
void addForNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeForExpr()));
	undComandIndex.pop_back();
}
void addWhileNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeWhileExpr()));
	undComandIndex.pop_back();
}
void addDoWhileNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeDoWhileExpr()));
	undComandIndex.pop_back();
}

void preSwitchNode(char const* s, char const* e)
{
	undComandIndex.push_back((UINT)varInfoTop.size());
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
}
void addCaseNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeCaseExpr()));
}
void addSwitchNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeSwitchExpr()));
	undComandIndex.pop_back();
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop -= varInfo.back().count;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace CompilerGrammar
{
	// Функции, кладушие и убирающие строки со стека строк
	// Стек строк может использоваться для удобного получения элемента более сложной грамматики
	// Например для правила использования переменной a[i], можно поместить "a" в стек,
	// потому что в функцию передаётся "a[i]" целиком
	void ParseStrPush(char const *s, char const *e){ strs.push_back(string(s,e)); }
	void ParseStrPop(char const *s, char const *e){ strs.pop_back(); }
	void ParseStrCopy(char const *s, char const *e){ strs.push_back(strs.back()); }

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
		nodeList.back()->SetCodeInfo(sIndexes.back().indexS, sIndexes.back().indexE);
		sIndexes.pop_back();
	}

	// Callbacks
	typedef void (*parserCallback)(char const*, char const*);
	parserCallback addInt, addFloat, addLong, addDouble;
	parserCallback strPush, strPop, strCopy;

	// Parser rules
	Rule group, term5, term4_9, term4_8, term4_85, term4_7, term4_75, term4_6, term4_65, term4_4, term4_2, term4_1, term4, term3, term2, term1, expression;
	Rule varname, funccall, funcdef, funcvars, block, vardef, vardefsub, applyval, applyref, ifexpr, whileexpr, forexpr, retexpr;
	Rule doexpr, breakexpr, switchexpr, isconst, addvarp, seltype;

	Rule code, mySpaceP;

	class ThrowError
	{
	public:
		ThrowError(): err(NULL){ }
		ThrowError(const char* str): err(str){ }

		void operator() (char const* s, char const* e)
		{
			//ASSERT(err);
			throw std::string(err);
		}
	private:
		const char* err;
	};

	void InitGrammar()
	{
		strPush	=	CompilerGrammar::ParseStrPush;
		strPop	=	CompilerGrammar::ParseStrPop;
		strCopy =	CompilerGrammar::ParseStrCopy;

		addInt		=	addNumberNode<int>;
		addFloat	=	addNumberNode<float>;
		addLong		=	addNumberNode<long long>;
		addDouble	=	addNumberNode<double>;

		seltype		=	varname[selType];

		isconst		=	epsP[AssignVar<bool>(currValConst,false)] >> !strP("const")[AssignVar<bool>(currValConst,true)];
		varname		=	lexemeD[alphaP >> *alnumP];

		funccall	=	varname[strPush] >> 
			('(' | (epsP[strPop] >> nothingP)) >>
			epsP[PushBackVal<std::vector<UINT>, UINT>(callArgCount, 0)] >> 
			!(
			term5[ArrBackInc<std::vector<UINT> >(callArgCount)][addFuncPushParamNode] >>
			*(',' >> term5[ArrBackInc<std::vector<UINT> >(callArgCount)][addFuncPushParamNode])[addTwoExprNode]
			) >>
			(')' | epsP[ThrowError("ERROR: ')' not found after function call")]);
		funcvars	=	!(seltype >> isconst >> !strP("ref")[convertTypeToRef] >> varname[strPush][funcParam]) >> *(',' >> seltype >> isconst >> !strP("ref")[convertTypeToRef] >> varname[strPush][funcParam]);
		funcdef		=	strP("func") >> seltype >> varname[strPush][funcAdd] >> '(' >>  funcvars[funcStart] >> chP(')') >> chP('{') >> code[funcEnd] >> chP('}');

		applyval	=
			(
				(varname - strP("case"))[strPush] >> (~chP('(') | (epsP[strPop] >> nothingP)) >> epsP[getType] >>
				!('[' >> term5 >> ']')[addShiftAddrNode] >>
				*(
					(strP("->")[AssignVar<bool>(currValueByRef, true)][addDereference][strCopy][addGetNode][strPop] >>
					(varname - strP("case"))[getMember] >>
					!('[' >> term5 >> ']')[addShiftAddrNode][addCmd(cmdAdd)]) |

					('.' >>
					(varname - strP("case"))[getMember] >>
					!('[' >> term5 >> ']')[addShiftAddrNode][addCmd(cmdAdd)])					
				)
			)[strPush];
		applyref	=
			(
				varname[strPush][getType][getAddress] >>
				!('[' >> term5 >> ']')[addShiftAddrNode] >>
				*(
					'.' >>
					varname[getMember] >>
					!('[' >> term5 >> ']')[addShiftAddrNode][addCmd(cmdAdd)]
				)
			);
		addvarp		=
			(
			(varname[strPush][pushType] >>
			epsP[AssignVar<UINT>(varSize,1)] >>
			!('[' >> intP[StrToInt(varSize)] >> ']'))
			)[strPush][addVar][IncVar<UINT>(varDefined)] >>
			(('=' >> term5)[AssignVar<bool>(currValueByRef, false)][pushValueByRef][addSetNode][addPopNode] | epsP[addVarDefNode][popType])[strPop][strPop];
		vardefsub	=
			((strP("ref")[convertTypeToRef] >> addvarp)[SetStringToLastNode] | addvarp[SetStringToLastNode]) >>
			*(',' >> vardefsub)[addTwoExprNode];
		vardef		=
			seltype >>
			isconst >>
			vardefsub;

		ifexpr		=	(strP("if") >> ('(' >> term5 >> ')'))[SaveStringIndex] >> expression >> ((strP("else") >> expression)[addIfElseNode] | epsP[addIfNode])[SetStringFromIndex];
		forexpr		=	(strP("for")[saveVarTop] >> '(' >> ((strP("var") >> vardef) | term5[addPopNode] | block) >> ';' >> term5 >> ';' >> (term5[addPopNode] | block) >> ')')[SaveStringIndex] >> expression[addForNode][SetStringFromIndex];
		whileexpr	=
			strP("while")[saveVarTop] >>
			(
			('(' | epsP[ThrowError("ERROR: '(' not found after 'while'")]) >>
			(term5 | epsP[ThrowError("ERROR: expression expected after 'while('")]) >>
			(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'while' statement")])
			) >>
			(expression[addWhileNode] | epsP[ThrowError("ERROR: expression expected after 'while(...)'")]);
		doexpr		=	
			strP("do")[saveVarTop] >> 
			(expression | epsP[ThrowError("ERROR: expression expected after 'do'")]) >> 
			(strP("while") | epsP[ThrowError("ERROR: 'while' expected after 'do' statement")]) >>
			(
			('(' | epsP[ThrowError("ERROR: '(' not found after 'while'")]) >> 
			(term5 | epsP[ThrowError("ERROR: expression not found after 'while('")]) >> 
			(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'while' statement")])
			)[addDoWhileNode] >> 
			(';' | epsP[ThrowError("ERROR: while(...) should be followed by ';'")]);
		switchexpr	=
			strP("switch")[preSwitchNode] >>
			('(' | epsP[ThrowError("ERROR: '(' not found after 'switch'")]) >>
			(term5 | epsP[ThrowError("ERROR: expression not found after 'switch('")]) >>
			(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'switch' statement")]) >>
			('{' | epsP[ThrowError("ERROR: '{' not found after 'switch(...)'")]) >>
			(strP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode] >>
			*(strP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode][addTwoExprNode] >>
			//(strP("case") >> term5 >> ':' >> code)[addCaseNode] >>
			('}' | epsP[ThrowError("ERROR: '}' not found after 'switch' statement")])[addSwitchNode];

		retexpr		=	(strP("return") >> term5 >> +chP(';'))[addReturnNode];
		breakexpr	=	(
			strP("break") >>
			(+chP(';') | epsP[ThrowError("ERROR: break must be followed by ';'")])
			)[addBreakNode];

		group		=	'(' >> term5 >> ')';
		term1		=
			(chP('&') >> applyref)[addAddressNode][strPop] |
			(strP("--") >> epsP[AssignVar<bool>(currValueByRef, false)] >> applyval[pushValueByRef])[addPreDecNode][strPop][strPop] | 
			(strP("++") >> epsP[AssignVar<bool>(currValueByRef, false)] >> applyval[pushValueByRef])[addPreIncNode][strPop][strPop] |
			(+(chP('-')[IncVar<UINT>(negCount)]) >> term1)[addNegNode] | (+chP('+') >> term1) | ('!' >> term1)[addLogNotNode] | ('~' >> term1)[addBitNotNode] |
			longestD[((intP >> chP('l'))[addLong] | (intP[addInt])) | ((realP >> chP('f'))[addFloat] | (realP[addDouble]))] |
			group |
			funccall[addFuncCallNode] |
			(('*' >> applyval)[addDereference][addGetNode] | (epsP[AssignVar<bool>(currValueByRef, false)] >> applyval[pushValueByRef])) >>
			(
				strP("++")[addPostIncNode] |
				strP("--")[addPostDecNode] |
				epsP[addGetNode]
			)[strPop][strPop];
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
		term5		=	(
			(('*' >> applyval)[addDereference][addGetNode] | (epsP[AssignVar<bool>(currValueByRef, false)] >> applyval[pushValueByRef])) >> (
			(strP("=") >> term5)[addSetNode] |
			(strP("+=") >> term5)[addAddSetNode] |
			(strP("-=") >> term5)[addSubSetNode] |
			(strP("*=") >> term5)[addMulSetNode] |
			(strP("/=") >> term5)[addDivSetNode] |
			(strP("^=") >> term5)[addPowSetNode] |
			(epsP[strPop][strPop][popTypeAndAddrNode][popValueByRef] >> nothingP))
			)[SetStringToLastNode][strPop][strPop] |
			term4_9;

		block		=	chP('{')[blockBegin] >> code >> chP('}')[blockEnd];
		expression	=	*chP(';') >> ((strP("var") >> vardef >> +chP(';')) | breakexpr | ifexpr | forexpr | whileexpr | doexpr | switchexpr | retexpr | (term5 >> (+chP(';')  | epsP[ThrowError("ERROR: ';' not found after expression")]))[addPopNode] | block[addBlockNode]);
		code		=	((funcdef | expression) >> (code[addTwoExprNode] | epsP[addOneExprNode]));
	
		mySpaceP = spaceP | ((strP("//") >> *(anycharP - eolP)) | (strP("/*") >> *(anycharP - strP("*/")) >> strP("*/")));
	}
};

Compiler::Compiler(CommandList* cmds)
{
	cmdList = cmds;

	TypeInfo* info;
	info = new TypeInfo();
	info->name = "void";
	info->size = 0;
	info->type = TypeInfo::POD_VOID;
	typeVoid = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "double";
	info->size = 8;
	info->type = TypeInfo::POD_DOUBLE;
	typeDouble = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "float";
	info->size = 4;
	info->type = TypeInfo::POD_FLOAT;
	typeFloat = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "long";
	info->size = 8;
	info->type = TypeInfo::POD_LONG;
	typeLong = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "int";
	info->size = 4;
	info->type = TypeInfo::POD_INT;
	typeInt = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "short";
	info->size = 2;
	info->type = TypeInfo::POD_SHORT;
	typeShort = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "char";
	info->size = 1;
	info->type = TypeInfo::POD_CHAR;
	typeChar = info;
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "float2";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "float3";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	info->AddMember("z", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "float4";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeFloat);
	info->AddMember("y", typeFloat);
	info->AddMember("z", typeFloat);
	info->AddMember("w", typeFloat);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "double2";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "double3";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	info->AddMember("z", typeDouble);
	typeInfo.push_back(info);

	info = new TypeInfo();
	info->name = "double4";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("x", typeDouble);
	info->AddMember("y", typeDouble);
	info->AddMember("z", typeDouble);
	info->AddMember("w", typeFloat);
	typeInfo.push_back(info);

	TypeInfo *typeFloat4 = info;

	info = new TypeInfo();
	info->name = "float4x4";
	info->type = TypeInfo::NOT_POD;
	info->AddMember("row1", typeFloat4);
	info->AddMember("row2", typeFloat4);
	info->AddMember("row3", typeFloat4);
	info->AddMember("row4", typeFloat4);
	typeInfo.push_back(info);

	CompilerGrammar::InitGrammar();

}
Compiler::~Compiler()
{
}

bool Compiler::Compile(string str)
{
	varInfoTop.clear();
	varInfo.clear();
	funcs.clear();

	callArgCount.clear();
	retTypeStack.clear();

	currTypes.clear();
	valueByRef.clear();

	nodeList.clear();

	varDefined = 0;
	negCount = 0;
	varSize = 1;
	varTop = 24;

	varInfo.push_back(VariableInfo("ERROR", 0, typeDouble, 1, true));
	varInfo.push_back(VariableInfo("pi", 8, typeDouble, 1, true));
	varInfo.push_back(VariableInfo("e", 16, typeDouble, 1, true));
	varInfoTop.push_back(VarTopInfo(0,0));

	retTypeStack.push_back(NULL);	//global return can return anything

	logAST.str("");
	cmdList->Clear();

	SetCommandList(cmdList);
	SetFunctionList(&funcs);
	SetLogStream(&logAST);
	SetNodeList(&nodeList);

	if(getList()->size() != 0)
		getList()->pop_back();

	ofstream m_FileStream("code.txt", std::ios::binary);
	m_FileStream << str;
	m_FileStream.flush();
	m_FileStream.close();

	char* ptr = (char*)str.c_str();

	ofstream m_TempStream("time.txt", std::ios::binary);

	UINT t = GetTickCount();
	ParseResult pRes = Parse(CompilerGrammar::code, ptr, CompilerGrammar::mySpaceP);
	if(pRes == PARSE_NOTFULL)
		throw std::string("Parsing wasn't full");
	if(pRes = PARSE_FAILED)
		throw std::string("Parsing failed");
	UINT tem = GetTickCount()-t;
	m_TempStream << "Parsing and AST tree gen. time: " << tem << "ms\r\n";
	
	t = GetTickCount();
	if(getList()->back())
		getList()->back()->Compile();
	tem = GetTickCount()-t;
	m_TempStream << "Compile time: " << tem << "ms\r\n";

	m_TempStream.flush();
	m_TempStream.close();

	ostringstream		graphlog;
	ofstream graphFile("graph.txt", std::ios::binary);
	if(getList()->back())
		getList()->back()->LogToStream(graphlog);
	graphFile << graphlog.str();
	graphFile.close();

	if(nodeList.size() != 1)
		throw std::string("Compilation failed, AST contains more than one node");

	return true; // Зачем тут return true, если вместо return false используются исключения?
}

void Compiler::GenListing()
{
	UINT pos = 0, pos2 = 0;
	CmdID	cmd;
	//double	val;
	char	name[512];
	UINT	valind, valind2;
	logASM.str("");

	char* typeInfoS[] = { "int", "long", "float", "double" };
	char* typeInfoD[] = { "char", "short", "int", "long", "float", "double" };
	UINT typeSizeS[] = { 4, 8, 4, 8 };
	UINT typeSizeD[] = { 1, 2, 4, 8, 4, 8 };
	CmdFlag cFlag;
	OperFlag oFlag;
	while(cmdList->GetData(pos, cmd))
	{
		pos2 = pos;
		pos += 2;
		switch(cmd)
		{
		case cmdCallStd:
			{
				size_t len;
				cmdList->GetData(pos, len);
				pos += sizeof(size_t);
				if(len >= 511)
					break;
				cmdList->GetData(pos, name, len);
				pos += (UINT)len;
				name[len] = 0;
				logASM << dec << showbase << pos2 << dec << " CALLS " << name << ";";
			}
			break;
		case cmdPushVTop:
			logASM << dec << showbase << pos2 << dec << " PUSHT;";
			break;
		case cmdPopVTop:
			logASM << dec << showbase << pos2 << dec << " POPT;";
			break;
		case cmdCall:
			cmdList->GetUINT(pos, valind);
			pos += sizeof(UINT);
			cmdList->GetUINT(pos, valind2);
			pos += sizeof(UINT);
			logASM << dec << showbase << pos2 << " CALL " << valind << " size:" << valind2 << ";";
			break;
		case cmdProlog:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << dec << showbase << pos2 << " PROLOG " << (UINT)(oFlag) << ";";
			break;
		case cmdReturn:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetData(pos, &valind, sizeof(UINT));
			pos += sizeof(UINT);
			logASM << dec << showbase << pos2 << " RET " << valind;
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
		case cmdPushV:
			{
				int valind;
				cmdList->GetData(pos, &valind, sizeof(int));
				pos += sizeof(int);
				logASM << dec << showbase << pos2 << " PUSHV " << valind << dec << ";";
			}
			break;
		case cmdNop:
			logASM << dec << showbase << pos2 << dec << " NOP;";
			break;
		case cmdCTI:
			logASM << dec << showbase << pos2 << dec << " CTI addr*";
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
		case cmdPush:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				logASM << pos2 << " PUSH ";
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
					}
					logASM << valind;
					if(flagAddrRel(cFlag))
						logASM << "+top";
					if(flagShiftStk(cFlag))
						logASM << "+shift(stack)";
					
					logASM << "] ";
					if(flagSizeStk(cFlag))
						logASM << "size(stack)";
					if(flagSizeOn(cFlag))
					{
						cmdList->GetINT(pos, valind);
						pos += 4;
						logASM << "size(" << valind << ")";
					}
				}
			}
			
			break;
		case cmdMov:
			{
				cmdList->GetUSHORT(pos, cFlag);
				pos += 2;
				logASM << pos2 << " MOV ";
				logASM << typeInfoS[cFlag&0x00000003] << "->";
				logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
				asmStackType st = flagStackType(cFlag);
				asmDataType dt = flagDataType(cFlag);
				UINT	highDW = 0, lowDW = 0;
				int valind;

				
				if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
				{
					cmdList->GetINT(pos, valind);
					pos += 4;
				}
				logASM << valind;
				if(flagAddrRel(cFlag))
					logASM << "+top";

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
			}
			break;
		case cmdPop:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " POP ";
			logASM << typeInfoS[cFlag&0x00000003];
			break;
		case cmdRTOI:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " RTOI ";
			logASM << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdITOR:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " ITOR ";
			logASM << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdITOL:
			logASM << pos2 << " ITOL";
			break;
		case cmdLTOI:
			logASM << pos2 << " LTOI";
			break;
		case cmdSwap:
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " SWAP ";
			logASM << typeInfoS[cFlag&0x00000003] << "<->";
			logASM << typeInfoD[(cFlag>>2)&0x00000007];
			break;
		case cmdCopy:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << pos2 << " COPY ";
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
			logASM << pos2 << " JMP " << valind;
			break;
		case cmdJmpZ:
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << pos2 << " JMPZ";
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
			logASM << pos2 << " JMPNZ";
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
			logASM << pos2 << " SETRANGE " << typeInfoD[(cFlag>>2)&0x00000007] << " " << valind << " " << valind2 << ';';
			break;
		case cmdGetAddr:
			cmdList->GetUINT(pos, valind);
			pos += 4;
			logASM << pos2 << " GETADDR " << valind << ';';
			break;
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
		{
			cmdList->GetUCHAR(pos, oFlag);
			pos += 1;
			logASM << pos2 << ' ';
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
			logASM << pos2 << ' ';
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
				logASM << pos2 << " INCAT ";
			if(cmd == cmdDecAt)
				logASM << pos2 << " DECAT ";
			logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
			asmStackType st = flagStackType(cFlag);
			asmDataType dt = flagDataType(cFlag);
			UINT	highDW = 0, lowDW = 0;
			int valind;

			if(flagAddrRel(cFlag) || flagAddrAbs(cFlag)){
				cmdList->GetINT(pos, valind);
				pos += 4;
			}
			logASM << valind;
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

std::vector<VariableInfo>* Compiler::GetVariableInfo()
{
	return &varInfo;
}
