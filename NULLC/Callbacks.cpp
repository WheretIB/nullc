#include "Callbacks.h"
#include "CodeInfo.h"
using namespace CodeInfo;

#include "Parser.h"

char	callbackError[256];

// Temp variables
// ��������� ����������:

// ������������ ���������� ��� ������ � ������
unsigned int currAlign;

// ���� �� ���������� ���������� (��� addVarSetNode)
bool varDefined;

// �������� �� ������� ���������� �����������
bool currValConst;

// ����� ������� ���������� - ������������ �������
unsigned int inplaceArrayNum;

// ���������� � �������� ����� ����������. ��� ���������� �� ������ ��� ����, �����
// ������� ���������� � ����������, ����� ��� ������� �� ������� ���������
FastVector<VarTopInfo>		varInfoTop(64);

// ��������� ����������� ��������� �������� break, ������� ������ �����, �� ������� �������� ���� �����
// ����������, ����� �������� � � �� ���������, � ������� ��� ���������� ��, ���� �� �����������
// ����������� ��� ���������������� ������. ���� ���� (����������� ����� ���� ����������) ������ ������
// varInfoTop.
FastVector<unsigned int>			cycleBeginVarTop(64);

// ���������� � ���������� ����������� ������� �� ������ ������������ ������.
// ������ ��� ���� ����� ������� ������� �� ���� ������ �� ������� ���������.
FastVector<unsigned int>			funcInfoTop(64);

// ����, ������� ������ ���� ��������, ������� ���������� �������.
// ������� ����� ���������� ���� � ������
FastVector<TypeInfo*>		retTypeStack(64);

// ���������� � ���� ������� ����������
TypeInfo*	currType = NULL;
// ���� ��� ����������� arr[arr[i.a.b].y].x;
FastVector<TypeInfo*>		currTypes(64);

// ��� ����������� ����� �����
TypeInfo *newType = NULL;

FastVector<FunctionInfo*>	currDefinedFunc(64);

unsigned int	TypeInfo::buildInSize = 0;
ChunkedStackPool<4092> TypeInfo::typeInfoPool;
ChunkedStackPool<4092> VariableInfo::variablePool;

template<typename T> void	Swap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
}

void SetTypeConst(bool isConst)
{
	currValConst = isConst;
}

void SetCurrentAlignment(unsigned int alignment)
{
	currAlign = alignment;
}

int AddFunctionExternal(FunctionInfo* func, InplaceStr name)
{
	unsigned int hash = GetStringHash(name.begin, name.end);
	for(unsigned int i = 0; i < func->external.size(); i++)
		if(func->external[i].nameHash == hash)
			return i;

#ifdef NULLC_LOG_FILES
	fprintf(compileLog, "Function %s uses external variable %s\r\n", currDefinedFunc.back()->name.c_str(), name.c_str());
#endif
	func->external.push_back(FunctionInfo::ExternalName(name, hash));
	return (int)func->external.size()-1;
}

long long parseInteger(char const* s, char const* e, int base)
{
	unsigned long long res = 0;
	for(const char *p = s; p < e; p++)
	{
		int digit = ((*p >= '0' && *p <= '9') ? *p - '0' : (*p & ~0x20) - 'A' + 10);
		if(digit < 0 || digit > base)
		{
			char error[64];
			sprintf(error, "ERROR: Digit %d is not allowed in base %d", digit, base);
			ThrowError(error, p);
		}
		res = res * base + digit;
	}
	return res;
}

// ���������� � ������ ����� {}, ����� ��������� ���������� ����������� ����������, � �������� �����
// ����� �������� ����� ��������� �����.
void blockBegin(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
	funcInfoTop.push_back((unsigned int)funcInfo.size());
}
// ���������� � ����� ����� {}, ����� ������ ���������� � ���������� ������ �����, ��� ����� �����������
// �� ����� �� ������� ���������. ����� ��������� ������� ����� ���������� � ������.
void blockEnd(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	unsigned int varFormerTop = varTop;
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
		varInfo.pop_back();
	varTop = varInfoTop.back().varStackSize;
	varInfoTop.pop_back();

	for(unsigned int i = funcInfoTop.back(); i < funcInfo.size(); i++)
		funcInfo[i]->visible = false;
	funcInfoTop.pop_back();

	nodeList.push_back(new NodeBlock(varFormerTop-varTop));
}

// input is a symbol after '\'
char UnescapeSybmol(char symbol)
{
	char res = -1;
	if(symbol == 'n')
		res = '\n';
	else if(symbol == 'r')
		res = '\r';
	else if(symbol == 't')
		res = '\t';
	else if(symbol == '0')
		res = '\0';
	else if(symbol == '\'')
		res = '\'';
	else if(symbol == '\\')
		res = '\\';
	else
		ThrowError("ERROR: unknown escape sequence", lastKnownStartPos);
	return res;
}

// ������� ��� ���������� ����� � ������������ ������� ������ �����
void addNumberNodeChar(char const*s, char const*e)
{
	(void)e;	// C4100
	char res = s[1];
	if(res == '\\')
		res = UnescapeSybmol(s[2]);
	nodeList.push_back(new NodeNumber<int>(res, typeChar));
}

int fastatoi(const char* str)
{
	unsigned int digit;
	int a = 0;
	while((digit = *str - '0') < 10)
	{
		a = a * 10 + digit;
		str++;
	}
	return a;
}

void addNumberNodeInt(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(new NodeNumber<int>(fastatoi(s), typeInt));
}
void addNumberNodeFloat(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(new NodeNumber<float>((float)atof(s), typeFloat));
}
void addNumberNodeLong(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(new NodeNumber<long long>(parseInteger(s, e, 10), typeLong));
}
void addNumberNodeDouble(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(new NodeNumber<double>(atof(s), typeDouble));
}

void addVoidNode(char const*s, char const*e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeZeroOP());
}

void addHexInt(char const*s, char const*e)
{
	s += 2;
	if(int(e-s) > 16)
		ThrowError("ERROR: Overflow in hexadecimal constant", s);
	if(int(e-s) <= 8)
		nodeList.push_back(new NodeNumber<int>((unsigned int)parseInteger(s, e, 16), typeInt));
	else
		nodeList.push_back(new NodeNumber<long long>(parseInteger(s, e, 16), typeLong));
}

void addOctInt(char const*s, char const*e)
{
	s++;
	if(int(e-s) > 21)
		ThrowError("ERROR: Overflow in octal constant", s);
	if(int(e-s) <= 10)
		nodeList.push_back(new NodeNumber<int>((unsigned int)parseInteger(s, e, 8), typeInt));
	else
		nodeList.push_back(new NodeNumber<long long>(parseInteger(s, e, 8), typeLong));
}

void addBinInt(char const*s, char const*e)
{
	if(int(e-s) > 64)
		ThrowError("ERROR: Overflow in binary constant", s);
	if(int(e-s) <= 32)
		nodeList.push_back(new NodeNumber<int>((unsigned int)parseInteger(s, e, 2), typeInt));
	else
		nodeList.push_back(new NodeNumber<long long>(parseInteger(s, e, 2), typeLong));
}
// ������� ��� �������� ����, ������� ����� ������ � ����
// ������������ NodeExpressionList, ��� �� �������� ����� ������� � �������� ���������
// �� ���� �� ���� ������ ��������� ����� � ����������� ���������� ������.
void addStringNode(char const* s, char const* e)
{
	lastKnownStartPos = s;

	const char *curr = s+1, *end = e-1;
	unsigned int len = 0;
	// Find the length of the string with collapsed escape-sequences
	for(; curr < end; curr++, len++)
	{
		if(*curr == '\\')
			curr++;
	}
	curr = s+1;
	end = e-1;

	nodeList.push_back(new NodeZeroOP());
	TypeInfo *targetType = GetArrayType(typeChar, len+1);
	if(!targetType)
		ThrowLastError();
	nodeList.push_back(new NodeExpressionList(targetType));

	NodeZeroOP* temp = nodeList.back();
	nodeList.pop_back();

	NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp);

	while(end-curr > 0)
	{
		char clean[4];
		*(int*)clean = 0;

		for(int i = 0; i < 4 && curr < end; i++, curr++)
		{
			clean[i] = *curr;
			if(*curr == '\\')
			{
				curr++;
				clean[i] = UnescapeSybmol(*curr);
			}
		}
		nodeList.push_back(new NodeNumber<int>(*(int*)clean, typeInt));
		arrayList->AddNode();
	}
	if(len % 4 == 0)
	{
		nodeList.push_back(new NodeNumber<int>(0, typeInt));
		arrayList->AddNode();
	}
	nodeList.push_back(temp);
}

// ������� ��� �������� ����, ������� ����� �������� �� ����� ����������
// ���� ������ � ���� ��������� ���� � ������.
void addPopNode(char const* s, char const* e)
{
	nodeList.back()->SetCodeInfo(s, e);
	// ���� ��������� ���� � ������ - ���� � ������, ����� ���
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		nodeList.pop_back();
		nodeList.push_back(new NodeZeroOP());
	}else if(nodeList.back()->nodeType == typeNodePreOrPostOp){
		// ���� ��������� ����, ��� ����������, ������� ��������� ��� ����������� �� 1, �� ��������� �
		// ��������� � ��������, �� ����� ���������� ����������� ����.
		static_cast<NodePreOrPostOp*>(nodeList.back())->SetOptimised(true);
	}else{
		// ����� ������ �������� ���, ��� � ����������� � ������
		nodeList.push_back(new NodePopOp());
	}
}

// ������� ��� �������� ����, ������� �������� ���� �������� � �����
// ���� ������ � ���� ��������� ���� � ������.
void addNegNode(char const* s, char const* e)
{
	(void)e;	// C4100
	// ���� ��������� ���� ��� �����, �� ������ �������� ���� � ���������
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->GetTypeInfo();
		NodeZeroOP* zOP = nodeList.back();
		NodeZeroOP* Rd = NULL;
		if(aType == typeDouble)
		{
			Rd = new NodeNumber<double>(-static_cast<NodeNumber<double>* >(zOP)->GetVal(), zOP->GetTypeInfo());
		}else if(aType == typeFloat){
			Rd = new NodeNumber<float>(-static_cast<NodeNumber<float>* >(zOP)->GetVal(), zOP->GetTypeInfo());
		}else if(aType == typeLong){
			Rd = new NodeNumber<long long>(-static_cast<NodeNumber<long long>* >(zOP)->GetVal(), zOP->GetTypeInfo());
		}else if(aType == typeInt){
			Rd = new NodeNumber<int>(-static_cast<NodeNumber<int>* >(zOP)->GetVal(), zOP->GetTypeInfo());
		}else{
			sprintf(callbackError, "addNegNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// ����� ������ �������� ���, ��� � ����������� � ������
		nodeList.push_back(new NodeUnaryOp(cmdNeg));
	}
}

// ������� ��� �������� ����, ������� ��������� ���������� ��������� ��� ��������� � �����
// ���� ������ � ���� ��������� ���� � ������.
void addLogNotNode(char const* s, char const* e)
{
	(void)e;	// C4100
	// ���� ��������� ���� � ������ - �����, �� ��������� �������� �� ����� ���������
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->GetTypeInfo();
		NodeZeroOP* zOP = nodeList.back();
		NodeZeroOP* Rd = NULL;
		if(aType == typeDouble)
		{
			Rd = new NodeNumber<double>(static_cast<NodeNumber<double>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo());
		}else if(aType == typeFloat){
			Rd = new NodeNumber<float>(static_cast<NodeNumber<float>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo());
		}else if(aType == typeLong){
			Rd = new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo());
		}else if(aType == typeInt){
			Rd = new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetLogNotVal(), zOP->GetTypeInfo());
		}else{
			sprintf(callbackError, "addLogNotNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// ����� ������ �������� ���, ��� � ����������� � ������
		nodeList.push_back(new NodeUnaryOp(cmdLogNot));
	}
}
void addBitNotNode(char const* s, char const* e)
{
	(void)e;	// C4100
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->GetTypeInfo();
		NodeZeroOP* zOP = nodeList.back();
		NodeZeroOP* Rd = NULL;
		if(aType == typeDouble)
		{
			ThrowError("ERROR: bitwise NOT cannot be used on floating point numbers", s);
		}else if(aType == typeFloat){
			ThrowError("ERROR: bitwise NOT cannot be used on floating point numbers", s);
		}else if(aType == typeLong){
			Rd = new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo());
		}else if(aType == typeInt){
			Rd = new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo());
		}else{
			sprintf(callbackError, "addBitNotNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		nodeList.push_back(new NodeUnaryOp(cmdBitNot));
	}
}

template<typename T>
T optDoOperation(CmdID cmd, T a, T b, bool swap = false)
{
	if(swap)
		Swap(a, b);
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
	ThrowError("ERROR: optDoSpecial call with unknown type", lastKnownStartPos);
	return 0;
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
	ThrowError("ERROR: optDoSpecial<int> call with unknown command", lastKnownStartPos);
	return 0;
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
	ThrowError("ERROR: optDoSpecial<long long> call with unknown command", lastKnownStartPos);
	return 0;
}
template<> double optDoSpecial<>(CmdID cmd, double a, double b)
{
	if(cmd == cmdShl)
		ThrowError("ERROR: optDoSpecial<double> call with << operation is illegal", lastKnownStartPos);
	if(cmd == cmdShr)
		ThrowError("ERROR: optDoSpecial<double> call with >> operation is illegal", lastKnownStartPos);
	if(cmd == cmdMod)
		return fmod(a,b);
	if(cmd >= cmdBitAnd && cmd <= cmdBitXor)
		ThrowError("ERROR: optDoSpecial<double> call with binary operation is illegal", lastKnownStartPos);
	if(cmd == cmdLogAnd)
		return (int)a && (int)b;
	if(cmd == cmdLogXor)
		return !!(int)a ^ !!(int)b;
	if(cmd == cmdLogOr)
		return (int)a || (int)b;
	ThrowError("ERROR: optDoSpecial<double> call with unknown command", lastKnownStartPos);
	return 0.0;
}

void popLastNodeCond(bool swap)
{
	if(swap)
	{
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		nodeList.back() = temp;
	}else{
		nodeList.pop_back();
	}
}

void AddBinaryCommandNode(CmdID id)
{
	unsigned int aNodeType = nodeList[nodeList.size()-2]->nodeType;
	unsigned int bNodeType = nodeList[nodeList.size()-1]->nodeType;
	unsigned int shA = 2, shB = 1;	//Shifts to operand A and B in array
	TypeInfo *aType, *bType;

	if(aNodeType == typeNodeNumber && bNodeType == typeNodeNumber)
	{
		//If we have operation between two known numbers, we can optimize code by calculating the result in place
		aType = nodeList[nodeList.size()-2]->GetTypeInfo();
		bType = nodeList[nodeList.size()-1]->GetTypeInfo();

		//Swap operands, to reduce number of combinations
		if((aType == typeFloat || aType == typeLong || aType == typeInt) && bType == typeDouble)
			Swap(shA, shB);
		if((aType == typeLong || aType == typeInt) && bType == typeFloat)
			Swap(shA, shB);
		if(aType == typeInt && bType == typeLong)
			Swap(shA, shB);

		bool swapOper = shA != 2;

		aType = nodeList[nodeList.size()-shA]->GetTypeInfo();
		bType = nodeList[nodeList.size()-shB]->GetTypeInfo();
		if(aType == typeDouble)
		{
			NodeNumber<double> *Ad = static_cast<NodeNumber<double>* >(nodeList[nodeList.size()-shA]);
			NodeNumber<double>* Rd = NULL;
			if(bType == typeDouble)
			{
				NodeNumber<double> *Bd = static_cast<NodeNumber<double>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), Bd->GetVal()), typeDouble);
			}else if(bType == typeFloat){
				NodeNumber<float> *Bd = static_cast<NodeNumber<float>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble);
			}else if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble);
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<double>(optDoOperation<double>(id, Ad->GetVal(), (double)Bd->GetVal(), swapOper), typeDouble);
			}
			nodeList.pop_back();
			nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeFloat){
			NodeNumber<float> *Ad = static_cast<NodeNumber<float>* >(nodeList[nodeList.size()-shA]);
			NodeNumber<float>* Rd = NULL;
			if(bType == typeFloat){
				NodeNumber<float> *Bd = static_cast<NodeNumber<float>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), Bd->GetVal()), typeFloat);
			}else if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), (float)Bd->GetVal(), swapOper), typeFloat);
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<float>(optDoOperation<float>(id, Ad->GetVal(), (float)Bd->GetVal(), swapOper), typeFloat);
			}
			nodeList.pop_back();
			nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeLong){
			NodeNumber<long long> *Ad = static_cast<NodeNumber<long long>* >(nodeList[nodeList.size()-shA]);
			NodeNumber<long long>* Rd = NULL;
			if(bType == typeLong){
				NodeNumber<long long> *Bd = static_cast<NodeNumber<long long>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<long long>(optDoOperation<long long>(id, Ad->GetVal(), Bd->GetVal()), typeLong);
			}else if(bType == typeInt){
				NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shB]);
				Rd = new NodeNumber<long long>(optDoOperation<long long>(id, Ad->GetVal(), (long long)Bd->GetVal(), swapOper), typeLong);
			}
			nodeList.pop_back();
			nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeInt){
			NodeNumber<int> *Ad = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shA]);
			NodeNumber<int>* Rd;
			//bType is also int!
			NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shB]);
			Rd = new NodeNumber<int>(optDoOperation<int>(id, Ad->GetVal(), Bd->GetVal()), typeInt);
			nodeList.pop_back();
			nodeList.pop_back();
			nodeList.push_back(Rd);
		}
		return;	// ����������� �������, �������
	}
	if(aNodeType == typeNodeNumber || bNodeType == typeNodeNumber)
	{
		// ���� ���� �� ����� - �����, �� �������� ��������� ������� ���, ����� ���� � ������ ��� � A
		if(bNodeType == typeNodeNumber)
		{
			Swap(shA, shB);
			Swap(aNodeType, bNodeType);
		}

		// ����������� ����� ����������, ���� ������ ������� - typeNodeTwoAndCmdOp ��� typeNodeVarGet
		if(bNodeType != typeNodeTwoAndCmdOp && bNodeType != typeNodeDereference)
		{
			// �����, ������� ��� �����������
			nodeList.push_back(new NodeTwoAndCmdOp(id));
			if(!lastError.IsEmpty())
				ThrowLastError();
			return;
		}

		// ����������� ����� ����������, ���� ����� == 0 ��� ����� == 1
		bool success = false;
		bType = nodeList[nodeList.size()-shA]->GetTypeInfo();
		if(bType == typeDouble)
		{
			NodeNumber<double> *Ad = static_cast<NodeNumber<double>* >(nodeList[nodeList.size()-shA]);
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
			NodeNumber<float> *Ad = static_cast<NodeNumber<float>* >(nodeList[nodeList.size()-shA]);
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
			NodeNumber<long long> *Ad = static_cast<NodeNumber<long long>* >(nodeList[nodeList.size()-shA]);
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
			NodeNumber<int> *Ad = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shA]);
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
		if(success)	// ����������� �������, ������� �����
			return;
	}
	// ����������� �� �������, ������� �������� ���������
	nodeList.push_back(new NodeTwoAndCmdOp(id));
	if(!lastError.IsEmpty())
		ThrowLastError();
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
	{
		sprintf(callbackError, "ERROR: function returns %s but supposed to return %s", realRetType->GetFullTypeName(), retTypeStack.back()->GetFullTypeName());
		ThrowError(callbackError, s);
	}
	if(retTypeStack.back() && retTypeStack.back()->type == TypeInfo::TYPE_VOID && realRetType != typeVoid)
		ThrowError("ERROR: function returning a value", s);
	if(retTypeStack.back() && retTypeStack.back() != typeVoid && realRetType == typeVoid)
	{
		sprintf(callbackError, "ERROR: function should return %s", retTypeStack.back()->GetFullTypeName());
		ThrowError(callbackError, s);
	}
	if(!retTypeStack.back() && realRetType == typeVoid)
		ThrowError("ERROR: global return cannot accept void", s);
	nodeList.push_back(new NodeReturnOp(c, retTypeStack.back()));
	nodeList.back()->SetCodeInfo(s, e);
}

void addBreakNode(char const* s, char const* e)
{
	(void)e;	// C4100
	if(cycleBeginVarTop.size() == 0)
		ThrowError("ERROR: break used outside loop statement", s);
	int t = (int)varInfoTop.size();
	int c = 0;
	while(t > (int)cycleBeginVarTop.back())
	{
		c++;
		t--;
	}
	nodeList.push_back(new NodeBreakOp(c));
}

void AddContinueNode(char const* s, char const* e)
{
	(void)e;	// C4100
	if(cycleBeginVarTop.size() == 0)
		ThrowError("ERROR: continue used outside loop statement", s);
	int t = (int)varInfoTop.size();
	int c = 0;
	while(t > (int)cycleBeginVarTop.back())
	{
		c++;
		t--;
	}
	nodeList.push_back(new NodeContinueOp(c));
}

void SelectAutoType()
{
	currType = NULL;
}

void SelectTypeByIndex(unsigned int index)
{
	currType = typeInfo[index];
}

void addTwoExprNode(char const* s, char const* e);

unsigned int	offsetBytes = 0;

void AddVariable(char const* pos, InplaceStr varName)
{
	lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// Check for variables with the same name in current scope
	for(unsigned int i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
	{
		if(varInfo[i]->nameHash == hash)
		{
			sprintf(callbackError, "ERROR: Name '%.*s' is already taken for a variable in current scope", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}
	}
	// Check for functions with the same name
	for(unsigned int i = 0; i < funcInfo.size(); i++)
	{
		if(funcInfo[i]->nameHash == hash && funcInfo[i]->visible)
		{
			sprintf(callbackError, "ERROR: Name '%.*s' is already taken for a function", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}
	}

	if(currType && currType->size == TypeInfo::UNSIZED_ARRAY)
	{
		sprintf(callbackError, "ERROR: variable '%.*s' can't be an unfixed size array", varName.end-varName.begin, varName.begin);
		ThrowError(callbackError, pos);
	}
	if(currType && currType->size > 64*1024*1024)
	{
		sprintf(callbackError, "ERROR: variable '%.*s' has to big length (>64 Mb)", varName.end-varName.begin, varName.begin);
		ThrowError(callbackError, pos);
	}
	
	if((currType && currType->alignBytes != 0) || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
	{
		unsigned int activeAlign = currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : currType->alignBytes;
		if(activeAlign > 16)
			ThrowError("ERROR: alignment must be less than 16 bytes", pos);
		if(activeAlign != 0 && varTop % activeAlign != 0)
		{
			unsigned int offset = activeAlign - (varTop % activeAlign);
			varTop += offset;
			offsetBytes += offset;
		}
	}
	varInfo.push_back(new VariableInfo(varName, hash, varTop, currType, currValConst));
	varDefined = true;
	if(currType)
		varTop += currType->size;
}

void AddVariableReserveNode(char const* pos)
{
	assert(varDefined);
	if(!currType)
		ThrowError("ERROR: auto variable must be initialized in place of definition", pos);
	nodeList.push_back(new NodeZeroOP());
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
		ThrowError("ERROR: auto variable cannot have reference flag", s);
	currType = GetReferenceType(currType);
}

void convertTypeToArray(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;
	if(!currType)
		ThrowError("ERROR: cannot specify array size for auto variable", s);
	currType = GetArrayType(currType);
	if(!currType)
		ThrowLastError();
}

//////////////////////////////////////////////////////////////////////////
//					New functions for work with variables

void GetTypeSize(char const* s, char const* e, bool sizeOfExpr)
{
	(void)e;	// C4100
	if(!sizeOfExpr && !currTypes.back())
		ThrowError("ERROR: sizeof(auto) is illegal", s);
	if(sizeOfExpr)
	{
		currTypes.back() = nodeList.back()->GetTypeInfo();
		nodeList.pop_back();
	}
	nodeList.push_back(new NodeNumber<int>(currTypes.back()->size, typeInt));
}

void SetTypeOfLastNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	currType = nodeList.back()->GetTypeInfo();
	nodeList.pop_back();
}

void AddInplaceArray(char const* pos);
void AddDereferenceNode(char const* s, char const* e);
void AddArrayIndexNode(char const* s, char const* e);
void AddMemberAccessNode(char const* pos, InplaceStr varName);
void AddFunctionCallNode(char const* pos, char const* funcName, unsigned int callArgCount);

// ������� ��� ��������� ������ ����������, ��� ������� ��������� � ����������
void AddGetAddressNode(char const* pos, InplaceStr varName)
{
	lastKnownStartPos = pos;

	int fID = -1;

	// Find variable name hash
	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// Find in variable list
	int i = (int)varInfo.size()-1;
	while(i >= 0 && varInfo[i]->nameHash != hash)
		i--;
	if(i == -1)
	{
		// ���� ������� �� �����
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->nameHash == hash && funcInfo[k]->visible)
			{
				if(fID != -1)
				{
					sprintf(callbackError, "ERROR: there are more than one '%s' function, and the decision isn't clear", varName);
					ThrowError(callbackError, pos);
				}
				fID = k;
			}
		}
		if(fID == -1)
		{
			sprintf(callbackError, "ERROR: variable '%s' is not defined", varName);
			ThrowError(callbackError, pos);
		}
	}
	// ����� � ���� ����� � ���
	if(fID == -1)
		currTypes.push_back(varInfo[i]->varType);
	else
		currTypes.push_back(funcInfo[fID]->funcType);

	static unsigned int thisHash = GetStringHash("this");

	if(newType && (currDefinedFunc.back()->type == FunctionInfo::THISCALL) && hash != thisHash)
	{
		bool member = false;
		for(TypeInfo::MemberVariable *curr = newType->firstVariable; curr; curr = curr->next)
			if(curr->nameHash == hash)
				member = true;
		if(member)
		{
			// ���������� ���� ���������� ����� ��������� this
			FunctionInfo *currFunc = currDefinedFunc.back();

			TypeInfo *temp = GetReferenceType(newType);
			currTypes.push_back(temp);

			nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));

			AddDereferenceNode(pos, 0);
			AddMemberAccessNode(pos, varName);

			currTypes.pop_back();
			return;
		}
	}
	// ���� �� ��������� � ��������� �������, � ���������� ��������� � �������� ������� ���������
	if((int)retTypeStack.size() > 1 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) && i != -1 && i < (int)varInfoTop[currDefinedFunc.back()->vTopSize].activeVarCnt)
	{
		FunctionInfo *currFunc = currDefinedFunc.back();
		// ������� ��� ���������� � ������ ������� ���������� �������
		int num = AddFunctionExternal(currFunc, varName);

		TypeInfo *temp = GetReferenceType(typeInt);
		temp = GetArrayType(temp, (int)currDefinedFunc.back()->external.size());
		if(!temp)
			ThrowLastError();
		temp = GetReferenceType(temp);
		currTypes.push_back(temp);

		nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));
		AddDereferenceNode(0,0);
		nodeList.push_back(new NodeNumber<int>(num, typeInt));
		AddArrayIndexNode(0,0);
		AddDereferenceNode(0,0);
		// ������ ������� ���
		currTypes.pop_back();
	}else{
		if(fID == -1)
		{
			// ���� ���������� ��������� � ���������� ������� ���������, � ����� - ����������, ��� �������
			bool absAddress = ((varInfoTop.size() > 1) && (varInfo[i]->pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;

			int varAddress = varInfo[i]->pos;
			if(!absAddress)
				varAddress -= (int)(varInfoTop.back().varStackSize);

			// ������� ���� ��� ��������� ��������� �� ����������
			nodeList.push_back(new NodeGetAddress(varInfo[i], varAddress, absAddress));
		}else{
			if(funcInfo[fID]->funcPtr != 0)
				ThrowError("ERROR: Can't get a pointer to an extern function", pos);
			if(funcInfo[fID]->address == -1 && funcInfo[fID]->funcPtr == NULL)
				ThrowError("ERROR: Can't get a pointer to a build-in function", pos);
			if(funcInfo[fID]->type == FunctionInfo::LOCAL)
			{
				char	*contextName = AllocateString(funcInfo[fID]->nameLength + 6);
				int length = sprintf(contextName, "$%s_ext", funcInfo[fID]->name);
				unsigned int contextHash = GetStringHash(contextName);

				int i = (int)varInfo.size()-1;
				while(i >= 0 && varInfo[i]->nameHash != contextHash)
					i--;
				if(i == -1)
				{
					nodeList.push_back(new NodeNumber<int>(0, GetReferenceType(typeInt)));
				}else{
					AddGetAddressNode(pos, InplaceStr(contextName, length));
					currTypes.pop_back();
				}
			}

			// ������� ���� ��� ��������� ��������� �� �������
			nodeList.push_back(new NodeFunctionAddress(funcInfo[fID]));
		}
	}
}

// ������� ���������� ��� ���������� �������
void AddArrayIndexNode(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;

	// ��� ������ ���� ��������
	if(currTypes.back()->arrLevel == 0)
		ThrowError("ERROR: indexing variable that is not an array", s);
	// ���� ��� ������������ ������ (��������� �� ������)
	if(currTypes.back()->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		// �� ����� ����������� ���������� �������� ��������� �� ������, ������� �������� � ����������
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		nodeList.push_back(new NodeDereference(GetReferenceType(currTypes.back()->subType)));
		nodeList.push_back(temp);
	}
	// ���� ������ - ����������� ����� � ������� ���� - �����
	if(nodeList.back()->nodeType == typeNodeNumber && nodeList[nodeList.size()-2]->nodeType == typeNodeGetAddress)
	{
		// �������� �������� ������
		int shiftValue = 0;
		NodeZeroOP* indexNode = nodeList.back();
		TypeInfo *aType = indexNode->GetTypeInfo();
		NodeZeroOP* zOP = indexNode;
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
			sprintf(callbackError, "AddArrayIndexNode() ERROR: unknown index type %s", aType->name);
			ThrowError(callbackError, lastKnownStartPos);
		}

		// �������� ������ �� ����� �� ������� �������
		if(shiftValue < 0)
			ThrowError("ERROR: Array index cannot be negative", s);
		if((unsigned int)shiftValue >= currTypes.back()->arrSize)
			ThrowError("ERROR: Array index out of bounds", s);

		// ����������� ������������ ����
		static_cast<NodeGetAddress*>(nodeList[nodeList.size()-2])->IndexArray(shiftValue);
		nodeList.pop_back();
	}else{
		// ����� ������ ���� ����������
		nodeList.push_back(new NodeArrayIndex(currTypes.back()));
	}
	// ������ ������� ��� - ��� �������� �������
	currTypes.back() = currTypes.back()->subType;
}

// ������� ���������� ��� ������������� ���������
void AddDereferenceNode(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;

	// ������ ���� �������������
	nodeList.push_back(new NodeDereference(currTypes.back()));
	// ������ ������� ��� - ��� �� ������� ��������� ������
	currTypes.back() = GetDereferenceType(currTypes.back());
	if(!currTypes.back())
		ThrowLastError();
}

// ���������� � ������ ������������, ��� ����� ���������� ����� ��������� ���� ������������
// ����� ��� ����, ������� ��������� ������� ����
void FailedSetVariable(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.pop_back();
}

// ������� ���������� ��� ���������� ���������� � ������������� ������������� �� ��������
void AddDefineVariableNode(char const* pos, InplaceStr varName)
{
	lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// ���� ���������� �� �����
	int i = (int)varInfo.size()-1;
	while(i >= 0 && varInfo[i]->nameHash != hash)
		i--;
	if(i == -1)
	{
		sprintf(callbackError, "ERROR: variable '%.*s' is not defined", varName.end-varName.begin, varName.begin);
		ThrowError(callbackError, pos);
	}
	// ����� � ���� ����� � ���
	currTypes.push_back(varInfo[i]->varType);

	// ���� ���������� ��������� � ���������� ������� ���������, � ����� - ����������, ��� �������
	bool absAddress = ((varInfoTop.size() > 1) && (varInfo[i]->pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;

	// ���� ��������� �� ������� ��� ����� NULL, ������ ��� ���������� ��������� ��� ������������� ��������� (auto)
	// � ����� ������, � �������� ���� ������ ������������ ��������� ����� AST
	TypeInfo *realCurrType = currTypes.back() ? currTypes.back() : nodeList.back()->GetTypeInfo();

	// ��������, ��� ����������� �������� ���������� ����������� �������� ��������������� ���� ������
	// ���������� ������ ��� ����, ������������, ��� ��� ���� ���� ���������� � ����
	bool unifyTwo = false;
	// ���� ��� ���������� - ������������ ������, � ������������� �� �������� ������� ����
	if(realCurrType->arrSize == TypeInfo::UNSIZED_ARRAY && realCurrType != nodeList.back()->GetTypeInfo())
	{
		TypeInfo *nodeType = nodeList.back()->GetTypeInfo();
		// ���� ������ ����� �������� (����������������, ��������) ���������
		if(realCurrType->subType == nodeType->subType)
		{
			// � ���� ������ �� ��������� ���� ��������� �������� ����������
			if(nodeList.back()->nodeType != typeNodeDereference)
			{
				// �����, ���� ������ - ����������� ������� �������
				if(nodeList.back()->nodeType == typeNodeExpressionList)
				{
					// ������� ����, ������������� ������� ���������� �������� ����� ������
					AddInplaceArray(pos);
					// �������� ������ ����, ����������� �� ���������� � �����
					unifyTwo = true;
				}else{
					// �����, ���� �� ����������, ������� ��������������� �� ������
					sprintf(callbackError, "ERROR: cannot convert from %s to %s", nodeList.back()->GetTypeInfo()->GetFullTypeName(), realCurrType->GetFullTypeName());
					ThrowError(callbackError, pos);
				}
			}
			// �����, ��� ��� �� ����������� ������������ ������� �������� ����������,
			// ��� ���� ������������� ��� � ���� ���������;������
			// ������ ��������� �� ������, �� - ����, ����������� � ���� ������������� ���������
			NodeZeroOP	*oldNode = nodeList.back();
			nodeList.back() = static_cast<NodeDereference*>(oldNode)->GetFirstNode();
			static_cast<NodeDereference*>(oldNode)->SetFirstNode(NULL);
			// ������ ������ �������
			unsigned int typeSize = (nodeType->size - nodeType->paddingBytes) / nodeType->subType->size;
			// �������� ������ ���������, ������������ ��� ������������� �������
			// ����������� ������ �������� ���������� ���� � ����
			NodeExpressionList *listExpr = new NodeExpressionList(varInfo[i]->varType);
			// �������� ����, ������������ ����� - ������ �������
			nodeList.push_back(new NodeNumber<int>(typeSize, typeInt));
			// ������� � ������
			listExpr->AddNode();
			// ������� ������ � ������ �����
			nodeList.push_back(listExpr);
		}
	}
	// ���� ���������� ������������� �������, �� ������ ��������� �� ��
	if(nodeList.back()->nodeType == typeNodeFuncDef ||
		(nodeList.back()->nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode()->nodeType == typeNodeFuncDef))
	{
		NodeFuncDef*	funcDefNode = (NodeFuncDef*)(nodeList.back()->nodeType == typeNodeFuncDef ? nodeList.back() : static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode());
		AddGetAddressNode(pos, InplaceStr(funcDefNode->GetFuncInfo()->name, funcDefNode->GetFuncInfo()->nameLength));
		currTypes.pop_back();
		unifyTwo = true;
		realCurrType = nodeList.back()->GetTypeInfo();
		varDefined = true;
		varTop -= realCurrType->size;
	}

	// ���������� ����������, �� ������� ���� ��������� ���� ����������
	unsigned int varSizeAdd = offsetBytes;	// �� ���������, ��� ������ ������������� �����
	offsetBytes = 0;	// ������� ����� �� ����������
	// ���� ��� �� ��� ����� ��������, ������ � ������� ���������� ���������� ������������ �� ���� �����������
	if(!currTypes.back())
	{
		// ���� ������������ �� ��������� ��� ���� �������� ������ �� ����� ���� (��� ������������)
		// ��� ���� ������������ ������� �������������
		if(realCurrType->alignBytes != 0 || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
		{
			// �������� ������������. ��������� ������������� ����� ������� ���������, ��� ������������ �� ���������
			unsigned int activeAlign = currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : realCurrType->alignBytes;
			if(activeAlign > 16)
				ThrowError("ERROR: alignment must be less than 16 bytes", pos);
			// ���� ��������� ������������ (���� ������������ noalign, � ����� ��� �� ��������)
			if(activeAlign != 0 && varTop % activeAlign != 0)
			{
				unsigned int offset = activeAlign - (varTop % activeAlign);
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

	nodeList.push_back(new NodeGetAddress(varInfo[i], varInfo[i]->pos-(int)(varInfoTop.back().varStackSize), absAddress));

	nodeList.push_back(new NodeVariableSet(realCurrType, varSizeAdd, false));

	if(unifyTwo)
	{
		nodeList.push_back(new NodeExpressionList(nodeList.back()->GetTypeInfo()));
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp)->AddNode();
		nodeList.push_back(temp);
	}
}

void AddSetVariableNode(char const* s, char const* e)
{
	(void)e;
	lastKnownStartPos = s;

	TypeInfo *realCurrType = currTypes.back();
	bool unifyTwo = false;
	if(realCurrType->arrSize == TypeInfo::UNSIZED_ARRAY && realCurrType != nodeList.back()->GetTypeInfo())
	{
		TypeInfo *nodeType = nodeList.back()->GetTypeInfo();
		if(realCurrType->subType == nodeType->subType)
		{
			if(nodeList.back()->nodeType != typeNodeDereference)
			{
				if(nodeList.back()->nodeType == typeNodeExpressionList)
				{
					AddInplaceArray(s);
					currTypes.pop_back();
					unifyTwo = true;
				}else{
					sprintf(callbackError, "ERROR: cannot convert from %s to %s", nodeList.back()->GetTypeInfo()->GetFullTypeName(), realCurrType->GetFullTypeName());
					ThrowError(callbackError, s);
				}
			}
			NodeZeroOP	*oldNode = nodeList.back();
			nodeList.back() = static_cast<NodeDereference*>(oldNode)->GetFirstNode();
			static_cast<NodeDereference*>(oldNode)->SetFirstNode(NULL);

			unsigned int typeSize = (nodeType->size - nodeType->paddingBytes) / nodeType->subType->size;
			NodeExpressionList *listExpr = new NodeExpressionList(realCurrType);
			nodeList.push_back(new NodeNumber<int>(typeSize, typeInt));
			listExpr->AddNode();
			nodeList.push_back(listExpr);

			if(unifyTwo)
				Swap(nodeList[nodeList.size()-2], nodeList[nodeList.size()-3]);
		}
	}
	if(nodeList.back()->nodeType == typeNodeFuncDef ||
		(nodeList.back()->nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode()->nodeType == typeNodeFuncDef))
	{
		NodeFuncDef*	funcDefNode = (NodeFuncDef*)(nodeList.back()->nodeType == typeNodeFuncDef ? nodeList.back() : static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode());
		AddGetAddressNode(s, InplaceStr(funcDefNode->GetFuncInfo()->name, funcDefNode->GetFuncInfo()->nameLength));
		currTypes.pop_back();
		unifyTwo = true;
		Swap(nodeList[nodeList.size()-2], nodeList[nodeList.size()-3]);
	}

	nodeList.push_back(new NodeVariableSet(currTypes.back(), 0, true));
	if(!lastError.IsEmpty())
		ThrowLastError();

	if(unifyTwo)
	{
		nodeList.push_back(new NodeExpressionList(nodeList.back()->GetTypeInfo()));
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp)->AddNode();
		nodeList.push_back(temp);
	}
}

void AddGetVariableNode(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;

	if(nodeList.back()->GetTypeInfo()->funcType == NULL)
		nodeList.push_back(new NodeDereference(currTypes.back()));
}

void AddMemberAccessNode(char const* pos, InplaceStr varName)
{
	lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// ��, ��� ��������� ���������� � ������, ��� � ����������!
	TypeInfo *currType = currTypes.back();

	if(currType->refLevel == 1)
	{
		nodeList.push_back(new NodeDereference(currTypes.back()));
		currTypes.back() = GetDereferenceType(currTypes.back());
		if(!currTypes.back())
			ThrowLastError();
		currType = currTypes.back();
	}
 
	int fID = -1;
	TypeInfo::MemberVariable *curr = currType->firstVariable;
	for(; curr; curr = curr->next)
		if(curr->nameHash == hash)
			break;
	if(!curr)
	{
		unsigned int hash = currType->nameHash;
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, varName.begin, varName.end);

		// ���� ������� �� �����
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->nameHash == hash && funcInfo[k]->visible)
			{
				if(fID != -1)
				{
					sprintf(callbackError, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);
					ThrowError(callbackError, pos);
				}
				fID = k;
			}
		}
		if(fID == -1)
		{
			sprintf(callbackError, "ERROR: variable '%.*s' is not a member of '%s'", varName.end-varName.begin, varName.begin, currType->GetFullTypeName());
			ThrowError(callbackError, pos);
		}
	}
	
	if(fID == -1)
	{
		if(nodeList.back()->nodeType == typeNodeGetAddress)
		{
			static_cast<NodeGetAddress*>(nodeList.back())->ShiftToMember(curr);
		}else{
			nodeList.push_back(new NodeShiftAddress(curr->offset, curr->type));
		}
		currTypes.back() = curr->type;
	}else{
		// ������� ���� ��� ��������� ��������� �� �������
		nodeList.push_back(new NodeFunctionAddress(funcInfo[fID]));

		currTypes.back() = funcInfo[fID]->funcType;
	}
}

void AddMemberFunctionCall(char const* pos, char const* funcName, unsigned int callArgCount)
{
	char	*memberFuncName = AllocateString((int)strlen(currTypes.back()->name) + 2 + (int)strlen(funcName) + 1);
	sprintf(memberFuncName, "%s::%s", currTypes.back()->name, funcName);
	AddFunctionCallNode(pos, memberFuncName, callArgCount);
	currTypes.back() = nodeList.back()->GetTypeInfo();
}

void AddPreOrPostOpNode(bool isInc, bool prefixOp)
{
	nodeList.push_back(new NodePreOrPostOp(currTypes.back(), isInc, prefixOp));
}

void AddModifyVariableNode(char const* s, char const* e, CmdID cmd)
{
	lastKnownStartPos = s;
	(void)e;	// C4100
	TypeInfo *targetType = GetDereferenceType(nodeList[nodeList.size()-2]->GetTypeInfo());
	if(!targetType)
		ThrowLastError();
	nodeList.push_back(new NodeVariableModify(targetType, cmd));
}

void AddInplaceArray(char const* pos)
{
	char	*arrName = AllocateString(16);
	int length = sprintf(arrName, "$carr%d", inplaceArrayNum++);

	TypeInfo *saveCurrType = currType;
	bool saveVarDefined = varDefined;

	currType = NULL;
	AddVariable(pos, InplaceStr(arrName, length));

	AddDefineVariableNode(pos, InplaceStr(arrName, length));
	addPopNode(pos, pos);
	currTypes.pop_back();

	AddGetAddressNode(pos, InplaceStr(arrName, length));
	AddGetVariableNode(pos, pos);

	varDefined = saveVarDefined;
	currType = saveCurrType;
}

//////////////////////////////////////////////////////////////////////////
void addOneExprNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeExpressionList());
}
void addTwoExprNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	if(nodeList.back()->nodeType != typeNodeExpressionList)
		addOneExprNode(NULL, NULL);
	// Take the expression list from the top
	NodeZeroOP* temp = nodeList.back();
	nodeList.pop_back();
	static_cast<NodeExpressionList*>(temp)->AddNode();
	nodeList.push_back(temp);
}

void addArrayConstructor(char const* s, char const* e, unsigned int arrElementCount)
{
	(void)e;
	arrElementCount++;

	TypeInfo *currType = nodeList[nodeList.size()-arrElementCount]->GetTypeInfo();

	if(currType == typeShort || currType == typeChar)
		currType = typeInt;
	if(currType == typeFloat)
		currType = typeDouble;
	if(currType == typeVoid)
		ThrowError("ERROR: array cannot be constructed from void type elements", s);

	nodeList.push_back(new NodeZeroOP());
	TypeInfo *targetType = GetArrayType(currType, arrElementCount);
	if(!targetType)
		ThrowLastError();

	NodeExpressionList *arrayList = new NodeExpressionList(targetType);

	TypeInfo *realType = nodeList.back()->GetTypeInfo();
	for(unsigned int i = 0; i < arrElementCount; i++)
	{
		if(realType != currType && !((realType == typeShort || realType == typeChar) && currType == typeInt) && !(realType == typeFloat && currType == typeDouble))
		{
			sprintf(callbackError, "ERROR: element %d doesn't match the type of element 0 (%s)", arrElementCount-i-1, currType->GetFullTypeName());
			ThrowError(callbackError, s);
		}
		arrayList->AddNode(false);
	}

	nodeList.push_back(arrayList);
}

void FunctionAdd(char const* pos, char const* funcName)
{
	unsigned int funcNameHash = GetStringHash(funcName);
	for(unsigned int i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
	{
		if(varInfo[i]->nameHash == funcNameHash)
		{
			sprintf(callbackError, "ERROR: Name '%s' is already taken for a variable in current scope", funcName);
			ThrowError(callbackError, pos);
		}
	}
	char *funcNameCopy = (char*)funcName;
	if(newType)
	{
		funcNameCopy = AllocateString((int)strlen(newType->name) + 2 + (int)strlen(funcName) + 1);
		sprintf(funcNameCopy, "%s::%s", newType->name, funcName);
	}
	if(!currType)
		ThrowError("ERROR: function return type cannot be auto", pos);
	funcInfo.push_back(new FunctionInfo(funcNameCopy));
	funcInfo.back()->vTopSize = (unsigned int)varInfoTop.size();
	retTypeStack.push_back(currType);
	funcInfo.back()->retType = currType;
	if(newType)
		funcInfo.back()->type = FunctionInfo::THISCALL;
	if(newType ? varInfoTop.size() > 2 : varInfoTop.size() > 1)
		funcInfo.back()->type = FunctionInfo::LOCAL;
	currDefinedFunc.push_back(funcInfo.back());

	if(varDefined && varInfo.back()->varType == NULL)
		varTop += 8;
}

void FunctionParameter(char const* pos, InplaceStr paramName)
{
	if(!currType)
		ThrowError("ERROR: function parameter cannot be an auto type", pos);
	unsigned int hash = GetStringHash(paramName.begin, paramName.end);
	funcInfo.back()->params.push_back(VariableInfo(paramName, hash, 0, currType, currValConst));
	funcInfo.back()->allParamSize += currType->size;
}
void FunctionStart(char const* pos)
{
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));

	for(int i = (int)funcInfo.back()->params.size()-1; i >= 0; i--)
	{
		currValConst = funcInfo.back()->params[i].isConst;
		currType = funcInfo.back()->params[i].varType;
		currAlign = 1;
		AddVariable(pos, funcInfo.back()->params[i].name);
		varDefined = false;
	}

	char	*hiddenHame = AllocateString(funcInfo.back()->nameLength + 8);
	int length = sprintf(hiddenHame, "$%s_ext", funcInfo.back()->name);
	currType = GetReferenceType(typeInt);
	currAlign = 1;
	AddVariable(pos, InplaceStr(hiddenHame, length));
	varDefined = false;

	funcInfo.back()->funcType = GetFunctionType(funcInfo.back());
}

void FunctionEnd(char const* pos, char const* funcName)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	unsigned int funcNameHash = GetStringHash(funcName);

	if(newType)
	{
		funcNameHash = newType->nameHash;
		funcNameHash = StringHashContinue(funcNameHash, "::");
		funcNameHash = StringHashContinue(funcNameHash, funcName);
	}

	int i = (int)funcInfo.size()-1;
	while(i >= 0 && funcInfo[i]->nameHash != funcNameHash)
		i--;

	// Find all the functions with the same name
	for(int n = 0; n < i; n++)
	{
		if(funcInfo[n]->nameHash == funcInfo[i]->nameHash && funcInfo[n]->params.size() == funcInfo[i]->params.size() && funcInfo[n]->visible)
		{
			// Check all parameter types
			bool paramsEqual = true;
			for(unsigned int k = 0; k < funcInfo[i]->params.size(); k++)
			{
				if(funcInfo[n]->params[k].varType != funcInfo[i]->params[k].varType)
					paramsEqual = false;
			}
			if(paramsEqual)
			{
				sprintf(callbackError, "ERROR: function '%s' is being defined with the same set of parameters", funcInfo[i]->name);
				ThrowError(callbackError, pos);
			}
		}
	}

	unsigned int varFormerTop = varTop;
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
		varInfo.pop_back();
	varTop = varInfoTop.back().varStackSize;
	varInfoTop.pop_back();
	nodeList.push_back(new NodeBlock(varFormerTop-varTop, false));

	nodeList.push_back(new NodeFuncDef(funcInfo[i]));
	funcDefList.push_back(nodeList.back());

	retTypeStack.pop_back();
	currDefinedFunc.pop_back();

	// If function is local, create function parameters block
	if(lastFunc.type == FunctionInfo::LOCAL && lastFunc.external.size() != 0)
	{
		nodeList.push_back(new NodeZeroOP());
		TypeInfo *targetType = GetReferenceType(typeInt);
		if(!targetType)
			ThrowLastError();
		targetType = GetArrayType(targetType, (int)lastFunc.external.size());
		if(!targetType)
			ThrowLastError();
		nodeList.push_back(new NodeExpressionList(targetType));

		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();

		NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp);

		for(unsigned int n = 0; n < lastFunc.external.size(); n++)
		{
			AddGetAddressNode(pos, lastFunc.external[n].name);
			currTypes.pop_back();
			arrayList->AddNode();
		}
		nodeList.push_back(temp);

		char	*hiddenHame = AllocateString(lastFunc.nameLength + 8);
		int length = sprintf(hiddenHame, "$%s_ext", lastFunc.name);

		TypeInfo *saveCurrType = currType;
		bool saveVarDefined = varDefined;

		currType = NULL;
		AddVariable(pos, InplaceStr(hiddenHame, length));

		AddDefineVariableNode(pos, InplaceStr(hiddenHame, length));
		addPopNode(pos, pos);
		currTypes.pop_back();

		varDefined = saveVarDefined;
		currType = saveCurrType;
		addTwoExprNode(pos, pos);
	}

	if(newType)
	{
		newType->AddMemberFunction();
		newType->lastFunction->func = &lastFunc;
		newType->lastFunction->defNode = nodeList.back();
	}
}

FastVector<NodeZeroOP*> paramNodes(32);
FastVector<NodeZeroOP*> inplaceArray(32);

void AddFunctionCallNode(char const* pos, char const* funcName, unsigned int callArgCount)
{
	unsigned int funcNameHash = GetStringHash(funcName);

	// Searching, if fname is actually a variable name (which means, either it is a pointer to function, or an error)
	int vID = (int)varInfo.size()-1;
	while(vID >= 0 && varInfo[vID]->nameHash != funcNameHash)
		vID--;

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	if(vID == -1)
	{
		//Find all functions with given name
		FunctionInfo *fList[32];
		unsigned int	fRating[32];
		//memset(fRating, 0, 32*4);

		unsigned int count = 0;
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->nameHash == funcNameHash && funcInfo[k]->visible)
			{
				fRating[count] = 0;
				fList[count++] = funcInfo[k];
			}
		}
		if(count == 0)
		{
			sprintf(callbackError, "ERROR: function '%s' is undefined", funcName);
			ThrowError(callbackError, pos);
		}
		// Find the best suited function
		unsigned int minRating = 1024*1024;
		unsigned int minRatingIndex = (unsigned int)~0;
		for(unsigned int k = 0; k < count; k++)
		{
			if(fList[k]->params.size() != callArgCount)
			{
				fRating[k] += 65000;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.
				continue;
			}
			for(unsigned int n = 0; n < fList[k]->params.size(); n++)
			{
				NodeZeroOP* activeNode = nodeList[nodeList.size()-fList[k]->params.size()+n];
				TypeInfo *paramType = activeNode->GetTypeInfo();
				unsigned int	nodeType = activeNode->nodeType;
				TypeInfo *expectedType = fList[k]->params[n].varType;
				if(expectedType != paramType)
				{
					if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->arrSize != 0 && paramType->subType == expectedType->subType)
						fRating[k] += 5;
					else if(expectedType->funcType != NULL && nodeType == typeNodeFuncDef ||
							(nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(activeNode)->GetFirstNode()->nodeType == typeNodeFuncDef))
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
			char errTemp[512];
			char	*errPos = errTemp;
			errPos += sprintf(errPos, "ERROR: can't find function '%s' with following parameters:\r\n  %s(", funcName, funcName);
			for(unsigned int n = 0; n < callArgCount; n++)
				errPos += sprintf(errPos, "%s%s", nodeList[nodeList.size()-callArgCount+n]->GetTypeInfo()->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
			errPos += sprintf(errPos, ")\r\n the only available are:\r\n");
			for(unsigned int n = 0; n < count; n++)
			{
				errPos += sprintf(errPos, "  %s(", funcName);
				for(unsigned int m = 0; m < fList[n]->params.size(); m++)
					errPos += sprintf(errPos, "%s%s", fList[n]->params[m].varType->GetFullTypeName(), m != fList[n]->params.size()-1 ? ", " : "");
				errPos += sprintf(errPos, ")\r\n");
			}
			lastError = CompilerError(errTemp, pos);
			ThrowLastError();
		}
		// Check, is there are more than one function, that share the same rating
		for(unsigned int k = 0; k < count; k++)
		{
			if(k != minRatingIndex && fRating[k] == minRating)
			{
				char errTemp[512];
				char	*errPos = errTemp;
				errPos += sprintf(errPos, "ambiguity, there is more than one overloaded function available for the call.\r\n  %s(", funcName);
				for(unsigned int n = 0; n < callArgCount; n++)
					errPos += sprintf(errPos, "%s%s", nodeList[nodeList.size()-callArgCount+n]->GetTypeInfo()->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
				errPos += sprintf(errPos, ")\r\n  candidates are:\r\n");
				for(unsigned int n = 0; n < count; n++)
				{
					if(fRating[n] != minRating)
						continue;
					errPos += sprintf(errPos, "  %s(", funcName);
					for(unsigned int m = 0; m < fList[n]->params.size(); m++)
						errPos += sprintf(errPos, "%s%s", fList[n]->params[m].varType->GetFullTypeName(), m != fList[n]->params.size()-1 ? ", " : "");
					errPos += sprintf(errPos, ")\r\n");
				}
				lastError = CompilerError(errTemp, pos);
				ThrowLastError();
			}
		}
		fType = fList[minRatingIndex]->funcType->funcType;
		fInfo = fList[minRatingIndex];
	}else{
		AddGetAddressNode(pos, InplaceStr(funcName, (int)strlen(funcName)));
		AddGetVariableNode(pos, NULL);
		fType = nodeList.back()->GetTypeInfo()->funcType;
	}

	paramNodes.clear();
	for(unsigned int i = 0; i < fType->paramType.size(); i++)
	{
		paramNodes.push_back(nodeList.back());
		nodeList.pop_back();
	}
	inplaceArray.clear();

	for(unsigned int i = 0; i < fType->paramType.size(); i++)
	{
		unsigned int index = (unsigned int)(fType->paramType.size()) - i - 1;

		TypeInfo *expectedType = fType->paramType[i];
		TypeInfo *realType = paramNodes[index]->GetTypeInfo();
		
		if(paramNodes[index]->nodeType == typeNodeFuncDef ||
			(paramNodes[index]->nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(paramNodes[index])->GetFirstNode()->nodeType == typeNodeFuncDef))
		{
			NodeFuncDef*	funcDefNode = (NodeFuncDef*)(paramNodes[index]->nodeType == typeNodeFuncDef ? paramNodes[index] : static_cast<NodeExpressionList*>(paramNodes[index])->GetFirstNode());
			AddGetAddressNode(pos, InplaceStr(funcDefNode->GetFuncInfo()->name, funcDefNode->GetFuncInfo()->nameLength));
			currTypes.pop_back();

			NodeExpressionList* listExpr = new NodeExpressionList(paramNodes[index]->GetTypeInfo());
			nodeList.push_back(paramNodes[index]);
			listExpr->AddNode();
			nodeList.push_back(listExpr);
		}else if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && expectedType->subType == realType->subType && expectedType != realType){
			if(paramNodes[index]->nodeType != typeNodeDereference)
			{
				if(paramNodes[index]->nodeType == typeNodeExpressionList)
				{
					nodeList.push_back(paramNodes[index]);
					AddInplaceArray(pos);

					paramNodes[index] = nodeList.back();
					nodeList.pop_back();
					inplaceArray.push_back(nodeList.back());
					nodeList.pop_back();
				}else{
					sprintf(callbackError, "ERROR: array expected as a parameter %d", i);
					ThrowError(callbackError, pos);
				}
			}
			unsigned int typeSize = (paramNodes[index]->GetTypeInfo()->size - paramNodes[index]->GetTypeInfo()->paddingBytes) / paramNodes[index]->GetTypeInfo()->subType->size;
			nodeList.push_back(static_cast<NodeDereference*>(paramNodes[index])->GetFirstNode());
			static_cast<NodeDereference*>(paramNodes[index])->SetFirstNode(NULL);
			paramNodes[index] = NULL;
			NodeExpressionList *listExpr = new NodeExpressionList(varInfo[i]->varType);
			nodeList.push_back(new NodeNumber<int>(typeSize, typeInt));
			listExpr->AddNode();
			nodeList.push_back(listExpr);
		}else{
			nodeList.push_back(paramNodes[index]);
		}
	}

	if(fInfo && (fInfo->type == FunctionInfo::LOCAL))
	{
		char	*contextName = AllocateString(fInfo->nameLength + 6);
		int length = sprintf(contextName, "$%s_ext", fInfo->name);
		unsigned int contextHash = GetStringHash(contextName);

		int i = (int)varInfo.size()-1;
		while(i >= 0 && varInfo[i]->nameHash != contextHash)
			i--;
		if(i == -1)
		{
			nodeList.push_back(new NodeNumber<int>(0, GetReferenceType(typeInt)));
		}else{
			AddGetAddressNode(pos, InplaceStr(contextName, length));
			if(currTypes.back()->refLevel == 1)
				AddDereferenceNode(pos, NULL);
			currTypes.pop_back();
		}
	}

	if(!fInfo)
		currTypes.pop_back();

	nodeList.push_back(new NodeFuncCall(fInfo, fType));

	if(inplaceArray.size() > 0)
	{
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		for(unsigned int i = 0; i < inplaceArray.size(); i++)
			nodeList.push_back(inplaceArray[i]);
		nodeList.push_back(temp);

		nodeList.push_back(new NodeExpressionList(temp->GetTypeInfo()));
		for(unsigned int i = 0; i < inplaceArray.size(); i++)
			addTwoExprNode(pos, pos);
	}
}

void addIfNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeIfElseExpr(false));
}
void addIfElseNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeIfElseExpr(true));
}
void addIfElseTermNode(char const* s, char const* e)
{
	(void)e;	// C4100
	TypeInfo* typeA = nodeList[nodeList.size()-1]->GetTypeInfo();
	TypeInfo* typeB = nodeList[nodeList.size()-2]->GetTypeInfo();
	if(typeA != typeB)
	{
		sprintf(callbackError, "ERROR: ternary operator ?: \r\n result types are not equal (%s : %s)", typeB->name, typeA->name);
		ThrowError(callbackError, s);
	}
	nodeList.push_back(new NodeIfElseExpr(true, true));
}

void saveVarTop(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	cycleBeginVarTop.push_back((unsigned int)varInfoTop.size());
}
void addForNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeForExpr());
	cycleBeginVarTop.pop_back();
}
void addWhileNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeWhileExpr());
	cycleBeginVarTop.pop_back();
}
void addDoWhileNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeDoWhileExpr());
	cycleBeginVarTop.pop_back();
}

void preSwitchNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	cycleBeginVarTop.push_back((unsigned int)varInfoTop.size());
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
	nodeList.push_back(new NodeSwitchExpr());
}
void addCaseNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	NodeZeroOP* temp = nodeList[nodeList.size()-3];
	static_cast<NodeSwitchExpr*>(temp)->AddCase();
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
		ThrowError("ERROR: Different type is being defined", s);
	if((int)currAlign < 0)
		ThrowError("ERROR: alignment must be a positive number", s);
	if(currAlign > 16)
		ThrowError("ERROR: alignment must be less than 16 bytes", s);

	char *typeNameCopy = AllocateString((int)(e - s) + 1);
	sprintf(typeNameCopy, "%.*s", (int)(e-s), s);

	newType = new TypeInfo(typeInfo.size(), typeNameCopy, 0, 0, 1, NULL);
	newType->type = TypeInfo::TYPE_COMPLEX;
	newType->alignBytes = currAlign;
	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;

	typeInfo.push_back(newType);
	
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
}

void TypeAddMember(char const* pos, const char* varName)
{
	if(!currType)
		ThrowError("ERROR: auto cannot be used for class members", pos);
	newType->AddMemberVariable(varName, currType);

	AddVariable(pos, InplaceStr(varName, (int)strlen(varName)));
}

void TypeFinish(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	if(newType->size % 4 != 0)
	{
		newType->paddingBytes = 4 - (newType->size % 4);
		newType->size += 4 - (newType->size % 4);
	}

	nodeList.push_back(new NodeZeroOP());
	for(TypeInfo::MemberFunction *curr = newType->firstFunction; curr; curr = curr->next)
		addTwoExprNode(0,0);

	newType = NULL;

	while(varInfo.size() > varInfoTop.back().activeVarCnt)
		varInfo.pop_back();
	varTop = varInfoTop.back().varStackSize;
	varInfoTop.pop_back();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void addUnfixedArraySize(char const*s, char const*e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeNumber<int>(1, typeVoid));
}

// ��� ������� ����������, ����� ��������� ������ ���� � ����, ������� ��� �����������
void SetStringToLastNode(char const *s, char const *e)
{
	nodeList.back()->SetCodeInfo(s, e);
}
struct StringIndex
{
	StringIndex(){}
	StringIndex(char const *s, char const *e)
	{
		indexS = s;
		indexE = e;
	}
	const char *indexS, *indexE;
};

FastVector<StringIndex> sIndexes(16);
void SaveStringIndex(char const *s, char const *e)
{
	assert(e > s);
	sIndexes.push_back(StringIndex(s, e));
}
void SetStringFromIndex(char const *s, char const *e)
{
	(void)s; (void)e;	// C4100
	nodeList.back()->SetCodeInfo(sIndexes.back().indexS, sIndexes.back().indexE);
	sIndexes.pop_back();
}

void CallbackInitialize()
{
	varInfoTop.clear();
	funcInfoTop.clear();

	VariableInfo::DeleteVariableInformation();

	retTypeStack.clear();
	currDefinedFunc.clear();

	currTypes.clear();

	funcDefList.clear();

	varDefined = 0;
	varTop = 24;
	newType = NULL;

	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;
	inplaceArrayNum = 1;

	varInfo.push_back(new VariableInfo(InplaceStr("ERROR"), GetStringHash("ERROR"), 0, typeDouble, true));
	varInfo.push_back(new VariableInfo(InplaceStr("pi"), GetStringHash("pi"), 8, typeDouble, true));
	varInfo.push_back(new VariableInfo(InplaceStr("e"), GetStringHash("e"), 16, typeDouble, true));

	varInfoTop.push_back(VarTopInfo(0,0));

	funcInfoTop.push_back(0);

	retTypeStack.push_back(NULL);	//global return can return anything
}

void CallbackDeinitialize()
{
	VariableInfo::DeleteVariableInformation();
}
