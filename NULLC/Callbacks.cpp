#include "Callbacks.h"
#include "CodeInfo.h"
using namespace CodeInfo;

char	callbackError[256];

std::string		warningLog;

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

// ������ ��������� �����
std::vector<std::string>	strs;

// ���������� � �������� ����� ����������. ��� ���������� �� ������ ��� ����, �����
// ������� ���������� � ����������, ����� ��� ������� �� ������� ���������
std::vector<VarTopInfo>		varInfoTop;

// ��������� ����������� ��������� �������� break, ������� ������ �����, �� ������� �������� ���� �����
// ����������, ����� �������� � � �� ���������, � ������� ��� ���������� ��, ���� �� �����������
// ����������� ��� ���������������� ������. ���� ���� (����������� ����� ���� ����������) ������ ������
// varInfoTop.
std::vector<unsigned int>			cycleBeginVarTop;

// ���������� � ���������� ����������� ������� �� ������ ������������ ������.
// ������ ��� ���� ����� ������� ������� �� ���� ������ �� ������� ���������.
std::vector<unsigned int>			funcInfoTop;

// ����, ������� ������ ���� ��������, ������� ���������� �������.
// ������� ����� ���������� ���� � ������
std::vector<TypeInfo*>		retTypeStack;

// ���������� � ���� ������� ����������
TypeInfo*	currType = NULL;
// ���� ��� ����������� arr[arr[i.a.b].y].x;
std::vector<TypeInfo*>	currTypes;

// ��� ����������� ����� �����
TypeInfo *newType = NULL;

std::vector<FunctionInfo*>	currDefinedFunc;

void SetTypeConst(bool isConst)
{
	currValConst = isConst;
}

void SetCurrentAlignment(unsigned int alignment)
{
	currAlign = alignment;
}

int AddFunctionExternal(FunctionInfo* func, std::string name)
{
	for(unsigned int i = 0; i < func->external.size(); i++)
		if(func->external[i] == name)
			return i;

#ifdef NULLC_LOG_FILES
	fprintf(compileLog, "Function %s uses external variable %s\r\n", currDefinedFunc.back()->name.c_str(), name.c_str());
#endif
	func->external.push_back(name);
	return (int)func->external.size()-1;
}

long long parseInteger(char const* s, char const* e, int base)
{
	unsigned long long res = 0;
	for(const char *p = s; p < e; p++)
	{
		int digit = ((*p >= '0' && *p <= '9') ? *p - '0' : tolower(*p) - 'a' + 10);
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

// ���������, �������� �� ������������� ����������������� ��� ��� �������
void checkIfDeclared(const std::string& str)
{
	if(str == "if" || str == "else" || str == "for" || str == "while" || str == "return" || str=="switch" || str=="case")
	{
		sprintf(callbackError, "ERROR: The name '%s' is reserved", str.c_str());
		ThrowError(callbackError, lastKnownStartPos);
	}
	for(unsigned int i = 0; i < funcInfo.size(); i++)
	{
		if(funcInfo[i]->name == str && funcInfo[i]->visible)
		{
			sprintf(callbackError, "ERROR: Name '%s' is already taken for a function", str.c_str());
			ThrowError(callbackError, lastKnownStartPos);
		}
	}
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

// ������� ��� ���������� ����� � ������������ ������� ������ �����
void addNumberNodeChar(char const*s, char const*e)
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
	nodeList.push_back(new NodeNumber<int>(res, typeChar));
}

void addNumberNodeInt(char const*s, char const*e)
{
	(void)e;	// C4100
	nodeList.push_back(new NodeNumber<int>(atoi(s), typeInt));
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
void addStringNode(char const*s, char const*e)
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
	nodeList.push_back(new NodeNumber<int>(len+1, typeInt));
	TypeInfo *targetType = GetArrayType(typeChar);
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
				if(*curr == 'n')
					clean[i] = '\n';
				if(*curr == 'r')
					clean[i] = '\r';
				if(*curr == 't')
					clean[i] = '\t';
				if(*curr == '0')
					clean[i] = '\0';
				if(*curr == '\'')
					clean[i] = '\'';
				if(*curr == '\"')
					clean[i] = '\"';
				if(*curr == '\\')
					clean[i] = '\\';
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

	strs.pop_back();
}

// ������� ��� �������� ����, ������� ����� �������� �� ����� ����������
// ���� ������ � ���� ��������� ���� � ������.
void addPopNode(char const* s, char const* e)
{
	nodeList.back()->SetCodeInfo(s, e);
	// ���� ��������� ���� � ������ - ���� � ������, ����� ���
	if(nodeList.back()->GetNodeType() == typeNodeNumber)
	{
		delete nodeList.back();
		nodeList.pop_back();
		nodeList.push_back(new NodeZeroOP());
	}else if(nodeList.back()->GetNodeType() == typeNodePreOrPostOp){
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
	if(nodeList.back()->GetNodeType() == typeNodeNumber)
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
			sprintf(callbackError, "addNegNode() ERROR: unknown type %s", aType->name.c_str());
			ThrowError(callbackError, s);
		}
		delete nodeList.back();
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
	if(nodeList.back()->GetNodeType() == typeNodeNumber)
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
			sprintf(callbackError, "addLogNotNode() ERROR: unknown type %s", aType->name.c_str());
			ThrowError(callbackError, s);
		}
		delete nodeList.back();
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
	if(nodeList.back()->GetNodeType() == typeNodeNumber)
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
			sprintf(callbackError, "addBitNotNode() ERROR: unknown type %s", aType->name.c_str());
			ThrowError(callbackError, s);
		}
		delete nodeList.back();
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
}

void popLastNodeCond(bool swap)
{
	if(swap)
	{
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		delete nodeList.back();
		nodeList.back() = temp;
	}else{
		delete nodeList.back();
		nodeList.pop_back();
	}
}

void addTwoAndCmpNode(CmdID id)
{
	unsigned int aNodeType = nodeList[nodeList.size()-2]->GetNodeType();
	unsigned int bNodeType = nodeList[nodeList.size()-1]->GetNodeType();
	unsigned int shA = 2, shB = 1;	//Shifts to operand A and B in array
	TypeInfo *aType, *bType;

	if(aNodeType == typeNodeNumber && bNodeType == typeNodeNumber)
	{
		//If we have operation between two known numbers, we can optimize code by calculating the result in place
		aType = nodeList[nodeList.size()-2]->GetTypeInfo();
		bType = nodeList[nodeList.size()-1]->GetTypeInfo();

		//Swap operands, to reduce number of combinations
		if((aType == typeFloat || aType == typeLong || aType == typeInt) && bType == typeDouble)
			std::swap(shA, shB);
		if((aType == typeLong || aType == typeInt) && bType == typeFloat)
			std::swap(shA, shB);
		if(aType == typeInt && bType == typeLong)
			std::swap(shA, shB);

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
			delete nodeList.back();
			nodeList.pop_back();
			delete nodeList.back();
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
			delete nodeList.back();
			nodeList.pop_back();
			delete nodeList.back();
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
			delete nodeList.back();
			nodeList.pop_back();
			delete nodeList.back();
			nodeList.pop_back();
			nodeList.push_back(Rd);
		}else if(aType == typeInt){
			NodeNumber<int> *Ad = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shA]);
			NodeNumber<int>* Rd;
			//bType is also int!
			NodeNumber<int> *Bd = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shB]);
			Rd = new NodeNumber<int>(optDoOperation<int>(id, Ad->GetVal(), Bd->GetVal()), typeInt);
			delete nodeList.back();
			nodeList.pop_back();
			delete nodeList.back();
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
			std::swap(shA, shB);
			std::swap(aNodeType, bNodeType);
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

template<CmdID cmd> void createTwoAndCmd(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;
	addTwoAndCmpNode(cmd);
}

typedef void (*ParseFuncPtr)(char const* s, char const* e);

ParseFuncPtr addCmd(CmdID cmd)
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
	ThrowError("ERROR: addCmd call with unknown command", lastKnownStartPos);
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
		sprintf(callbackError, "ERROR: function returns %s but supposed to return %s", retTypeStack.back()->GetTypeName().c_str(), realRetType->GetTypeName().c_str());
		ThrowError(callbackError, s);
	}
	if(retTypeStack.back() && retTypeStack.back()->type == TypeInfo::TYPE_VOID && realRetType != typeVoid)
		ThrowError("ERROR: function returning a value", s);
	if(retTypeStack.back() && retTypeStack.back() != typeVoid && realRetType == typeVoid)
	{
		sprintf(callbackError, "ERROR: function should return %s", retTypeStack.back()->GetTypeName().c_str());
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
	if(cycleBeginVarTop.empty())
		ThrowError("ERROR: break used outside loop statements", s);
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
	if(cycleBeginVarTop.empty())
		ThrowError("ERROR: continue used outside loop statements", s);
	int t = (int)varInfoTop.size();
	int c = 0;
	while(t > (int)cycleBeginVarTop.back())
	{
		c++;
		t--;
	}
	nodeList.push_back(new NodeContinueOp(c));
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
	for(unsigned int i = 0; i < typeInfo.size(); i++)
	{
		if(typeInfo[i]->name == vType)
		{
			currType = typeInfo[i];
			return;
		}
	}
	sprintf(callbackError, "ERROR: Variable type '%s' is unknown", vType.c_str());
	ThrowError(callbackError, s);
}

void addTwoExprNode(char const* s, char const* e);

unsigned int	offsetBytes = 0;

std::set<VariableInfo*> varInfoAll;

void addVar(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;
	string vName = strs.back();

	for(unsigned int i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
	{
		if(varInfo[i]->name == vName)
		{
			sprintf(callbackError, "ERROR: Name '%s' is already taken for a variable in current scope", vName.c_str());
			ThrowError(callbackError, s);
		}
	}
	checkIfDeclared(vName);

	if(currType && currType->size == TypeInfo::UNSIZED_ARRAY)
	{
		sprintf(callbackError, "ERROR: variable '%s' can't be an unfixed size array", vName.c_str());
		ThrowError(callbackError, s);
	}
	if(currType && currType->size > 64*1024*1024)
	{
		sprintf(callbackError, "ERROR: variable '%s' has to big length (>64 Mb)", vName.c_str());
		ThrowError(callbackError, s);
	}
	
	if((currType && currType->alignBytes != 0) || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
	{
		unsigned int activeAlign = currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : currType->alignBytes;
		if(activeAlign > 16)
			ThrowError("ERROR: alignment must me less than 16 bytes", s);
		if(activeAlign != 0 && varTop % activeAlign != 0)
		{
			unsigned int offset = activeAlign - (varTop % activeAlign);
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
		ThrowError("ERROR: auto variable must be initialized in place of definition", s);
	nodeList.push_back(new NodeVarDef(strs.back()));
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

void GetVariableType(char const* s, char const* e)
{
	int fID = -1;
	// ���� ���������� �� �����
	int i = (int)varInfo.size()-1;
	string vName(s, e);
	while(i >= 0 && varInfo[i]->name != vName)
		i--;
	if(i == -1)
	{
		// ���� ������� �� �����
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->name == vName && funcInfo[k]->visible)
			{
				if(fID != -1)
				{
					sprintf(callbackError, "ERROR: there are more than one '%s' function, and the decision isn't clear", vName.c_str());
					ThrowError(callbackError, s);
				}
				fID = k;
			}
		}
		if(fID == -1)
		{
			sprintf(callbackError, "ERROR: variable '%s' is not defined", vName.c_str());
			ThrowError(callbackError, s);
		}
	}

	if(fID == -1)
		currType = varInfo[i]->varType;
	else
		currType = funcInfo[fID]->funcType;
}

void GetTypeSize(char const* s, char const* e, bool sizeOfExpr)
{
	(void)e;	// C4100
	if(!sizeOfExpr && !currTypes.back())
		ThrowError("ERROR: sizeof(auto) is illegal", s);
	if(sizeOfExpr)
	{
		currTypes.back() = nodeList.back()->GetTypeInfo();
		delete nodeList.back();
		nodeList.pop_back();
	}
	nodeList.push_back(new NodeNumber<int>(currTypes.back()->size, typeInt));
}

void SetTypeOfLastNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	currType = nodeList.back()->GetTypeInfo();
	delete nodeList.back();
	nodeList.pop_back();
}

void AddInplaceArray(char const* s, char const* e);
void AddDereferenceNode(char const* s, char const* e);
void AddArrayIndexNode(char const* s, char const* e);
void AddMemberAccessNode(char const* s, char const* e);
void addFuncCallNode(char const* s, char const* e, unsigned int callArgCount);

// ������� ��� ��������� ������ ����������, ��� ������� ��������� � ����������
void AddGetAddressNode(char const* s, char const* e)
{
	lastKnownStartPos = s;

	int fID = -1;
	// ���� ���������� �� �����
	int i = (int)varInfo.size()-1;
	string vName(s, e);
	while(i >= 0 && varInfo[i]->name != vName)
		i--;
	if(i == -1)
	{
		// ���� ������� �� �����
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->name == vName && funcInfo[k]->visible)
			{
				if(fID != -1)
				{
					sprintf(callbackError, "ERROR: there are more than one '%s' function, and the decision isn't clear", vName.c_str());
					ThrowError(callbackError, s);
				}
				fID = k;
			}
		}
		if(fID == -1)
		{
			sprintf(callbackError, "ERROR: variable '%s' is not defined", vName.c_str());
			ThrowError(callbackError, s);
		}
	}
	// ����� � ���� ����� � ���
	if(fID == -1)
		currTypes.push_back(varInfo[i]->varType);
	else
		currTypes.push_back(funcInfo[fID]->funcType);

	if(newType && (currDefinedFunc.back()->type == FunctionInfo::THISCALL) && vName != "this")
	{
		bool member = false;
		for(unsigned int i = 0; i < newType->memberData.size(); i++)
			if(newType->memberData[i].name == vName)
				member = true;
		if(member)
		{
			// ���������� ���� ���������� ����� ��������� this
			std::string bName = "this";

			FunctionInfo *currFunc = currDefinedFunc.back();

			TypeInfo *temp = GetReferenceType(newType);
			currTypes.push_back(temp);

			nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));

			AddDereferenceNode(0,0);
			strs.push_back(vName);
			AddMemberAccessNode(s, e);

			currTypes.pop_back();
			return;
		}
	}
	// ���� �� ��������� � ��������� �������, � ���������� ��������� � �������� ������� ���������
	if((int)retTypeStack.size() > 1 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) && i != -1 && i < (int)varInfoTop[currDefinedFunc.back()->vTopSize].activeVarCnt)
	{
		FunctionInfo *currFunc = currDefinedFunc.back();
		// ������� ��� ���������� � ������ ������� ���������� �������
		int num = AddFunctionExternal(currFunc, vName);

		nodeList.push_back(new NodeNumber<int>((int)currDefinedFunc.back()->external.size(), typeInt));
		TypeInfo *temp = GetReferenceType(typeInt);
		temp = GetArrayType(temp);
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
				ThrowError("ERROR: Can't get a pointer to an extern function", s);
			if(funcInfo[fID]->address == -1 && funcInfo[fID]->funcPtr == NULL)
				ThrowError("ERROR: Can't get a pointer to a build-in function", s);
			if(funcInfo[fID]->type == FunctionInfo::LOCAL)
			{
				std::string bName = "$" + funcInfo[fID]->name + "_ext";
				int i = (int)varInfo.size()-1;
				while(i >= 0 && varInfo[i]->name != bName)
					i--;
				if(i == -1)
				{
					nodeList.push_back(new NodeNumber<int>(0, GetReferenceType(typeInt)));
				}else{
					AddGetAddressNode(bName.c_str(), bName.c_str()+bName.length());
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
	if(nodeList.back()->GetNodeType() == typeNodeNumber && nodeList[nodeList.size()-2]->GetNodeType() == typeNodeGetAddress)
	{
		// �������� �������� ������
		int shiftValue;
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
			sprintf(callbackError, "AddArrayIndexNode() ERROR: unknown index type %s", aType->name.c_str());
			ThrowError(callbackError, lastKnownStartPos);
		}

		// �������� ������ �� ����� �� ������� �������
		if(shiftValue < 0)
			ThrowError("ERROR: Array index cannot be negative", s);
		if((unsigned int)shiftValue >= currTypes.back()->arrSize)
			ThrowError("ERROR: Array index out of bounds", s);

		// ����������� ������������ ����
		static_cast<NodeGetAddress*>(nodeList[nodeList.size()-2])->IndexArray(shiftValue);
		delete nodeList.back();
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
	delete nodeList.back();
	nodeList.pop_back();
}

// ������� ���������� ��� ���������� ���������� � ������������� ������������� �� ��������
void AddDefineVariableNode(char const* s, char const* e)
{
	lastKnownStartPos = s;

	// ���� ���������� �� �����
	int i = (int)varInfo.size()-1;
	string vName = strs.back();
	while(i >= 0 && varInfo[i]->name != vName)
		i--;
	if(i == -1)
	{
		sprintf(callbackError, "ERROR: variable '%s' is not defined", vName.c_str());
		ThrowError(callbackError, s);
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
			if(nodeList.back()->GetNodeType() != typeNodeDereference)
			{
				// �����, ���� ������ - ����������� ������� �������
				if(nodeList.back()->GetNodeType() == typeNodeExpressionList)
				{
					// ������� ����, ������������� ������� ���������� �������� ����� ������
					AddInplaceArray(s, e);
					// �������� ������ ����, ����������� �� ���������� � �����
					unifyTwo = true;
				}else{
					// �����, ���� �� ����������, ������� ��������������� �� ������
					sprintf(callbackError, "ERROR: cannot convert from %s to %s", nodeList.back()->GetTypeInfo()->GetTypeName().c_str(), realCurrType->GetTypeName().c_str());
					ThrowError(callbackError, s);
				}
			}
			// �����, ��� ��� �� ����������� ������������ ������� �������� ����������,
			// ��� ���� ������������� ��� � ���� ���������;������
			// ������ ��������� �� ������, �� - ����, ����������� � ���� ������������� ���������
			NodeZeroOP	*oldNode = nodeList.back();
			nodeList.back() = static_cast<NodeDereference*>(oldNode)->GetFirstNode();
			static_cast<NodeDereference*>(oldNode)->SetFirstNode(NULL);
			delete oldNode;
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
	if(nodeList.back()->GetNodeType() == typeNodeFuncDef ||
		(nodeList.back()->GetNodeType() == typeNodeExpressionList && static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode()->GetNodeType() == typeNodeFuncDef))
	{
		NodeFuncDef*	funcDefNode = (NodeFuncDef*)(nodeList.back()->GetNodeType() == typeNodeFuncDef ? nodeList.back() : static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode());
		AddGetAddressNode(funcDefNode->GetFuncInfo()->name.c_str(), funcDefNode->GetFuncInfo()->name.c_str() + funcDefNode->GetFuncInfo()->name.length());
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
				ThrowError("ERROR: alignment must me less than 16 bytes", s);
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
					sprintf(callbackError, "ERROR: cannot convert from %s to %s", nodeList.back()->GetTypeInfo()->GetTypeName().c_str(), realCurrType->GetTypeName().c_str());
					ThrowError(callbackError, s);
				}
			}
			NodeZeroOP	*oldNode = nodeList.back();
			nodeList.back() = static_cast<NodeDereference*>(oldNode)->GetFirstNode();
			static_cast<NodeDereference*>(oldNode)->SetFirstNode(NULL);
			delete oldNode;

			unsigned int typeSize = (nodeType->size - nodeType->paddingBytes) / nodeType->subType->size;
			NodeExpressionList *listExpr = new NodeExpressionList(realCurrType);
			nodeList.push_back(new NodeNumber<int>(typeSize, typeInt));
			listExpr->AddNode();
			nodeList.push_back(listExpr);

			if(unifyTwo)
				std::swap(nodeList[nodeList.size()-2], nodeList[nodeList.size()-3]);
		}
	}
	if(nodeList.back()->GetNodeType() == typeNodeFuncDef ||
		(nodeList.back()->GetNodeType() == typeNodeExpressionList && static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode()->GetNodeType() == typeNodeFuncDef))
	{
		NodeFuncDef*	funcDefNode = (NodeFuncDef*)(nodeList.back()->GetNodeType() == typeNodeFuncDef ? nodeList.back() : static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode());
		AddGetAddressNode(funcDefNode->GetFuncInfo()->name.c_str(), funcDefNode->GetFuncInfo()->name.c_str() + funcDefNode->GetFuncInfo()->name.length());
		currTypes.pop_back();
		unifyTwo = true;
		std::swap(nodeList[nodeList.size()-2], nodeList[nodeList.size()-3]);
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

void AddMemberAccessNode(char const* s, char const* e)
{
	(void)e;	// C4100
	lastKnownStartPos = s;

	std::string memberName = strs.back();

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
	int i = (int)currType->memberData.size()-1;
	while(i >= 0 && currType->memberData[i].name != memberName)
		i--;
	if(i == -1)
	{
		// ���� ������� �� �����
		for(int k = 0; k < (int)funcInfo.size(); k++)
		{
			if(funcInfo[k]->name == (currType->name + "::" + memberName) && funcInfo[k]->visible)
			{
				if(fID != -1)
				{
					sprintf(callbackError, "ERROR: there are more than one '%s' function, and the decision isn't clear", memberName.c_str());
					ThrowError(callbackError, s);
				}
				fID = k;
			}
		}
		if(fID == -1)
		{
			sprintf(callbackError, "ERROR: variable '%s' is not a member of '%s'", memberName.c_str(), currType->GetTypeName().c_str());
			ThrowError(callbackError, s);
		}
	}
	
	if(fID == -1)
	{
		if(nodeList.back()->GetNodeType() == typeNodeGetAddress)
		{
			static_cast<NodeGetAddress*>(nodeList.back())->ShiftToMember(i);
		}else{
			nodeList.push_back(new NodeShiftAddress(currType->memberData[i].offset, currType->memberData[i].type));
		}
		currTypes.back() = currType->memberData[i].type;
	}else{
		// ������� ���� ��� ��������� ��������� �� �������
		nodeList.push_back(new NodeFunctionAddress(funcInfo[fID]));

		currTypes.back() = funcInfo[fID]->funcType;
	}

	strs.pop_back();
}

void AddMemberFunctionCall(char const* s, char const* e, unsigned int callArgCount)
{
	strs.back() = currTypes.back()->name + "::" + strs.back();
	addFuncCallNode(s, e, callArgCount);
	currTypes.back() = nodeList.back()->GetTypeInfo();
}

void AddPreOrPostOpNode(bool isInc, bool prefixOp)
{
	nodeList.push_back(new NodePreOrPostOp(currTypes.back(), isInc, prefixOp));
}

struct AddPreOrPostOp
{
	AddPreOrPostOp(bool isInc, bool isPrefixOp)
	{
		incOp = isInc;
		prefixOp = isPrefixOp;
	}

	void operator() (char const* s, char const* e)
	{
		(void)e;	// C4100
		lastKnownStartPos = s;
		AddPreOrPostOpNode(incOp, prefixOp);
	}
	bool incOp;
	bool prefixOp;
};

void AddModifyVariableNode(char const* s, char const* e, CmdID cmd)
{
	lastKnownStartPos = s;
	(void)e;	// C4100
	TypeInfo *targetType = GetDereferenceType(nodeList[nodeList.size()-2]->GetTypeInfo());
	if(!targetType)
		ThrowLastError();
	nodeList.push_back(new NodeVariableModify(targetType, cmd));
}

template<CmdID cmd>
struct AddModifyVariable
{
	void operator() (char const* s, char const* e)
	{
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

//////////////////////////////////////////////////////////////////////////
void addOneExprNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	nodeList.push_back(new NodeExpressionList());
}
void addTwoExprNode(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	if(nodeList.back()->GetNodeType() != typeNodeExpressionList)
		addOneExprNode(NULL, NULL);
	// Take the expression list from the top
	NodeZeroOP* temp = nodeList.back();
	nodeList.pop_back();
	static_cast<NodeExpressionList*>(temp)->AddNode();
	nodeList.push_back(temp);
}

void addArrayConstructor(char const* s, char const* e, unsigned int arrElementCount)
{
	arrElementCount++;

	TypeInfo *currType = nodeList[nodeList.size()-arrElementCount]->GetTypeInfo();

	if(currType == typeShort || currType == typeChar)
	{
		currType = typeInt;
		warningLog.append("WARNING: short and char will be promoted to int during array construction\r\n At ");
		warningLog.append(s, e);
		warningLog.append("\r\n");
	}
	if(currType == typeFloat)
	{
		currType = typeDouble;
		warningLog.append("WARNING: float will be promoted to double during array construction\r\n At ");
		warningLog.append(s, e);
		warningLog.append("\r\n");
	}
	if(currType == typeVoid)
		ThrowError("ERROR: array cannot be constructed from void type elements", s);

	nodeList.push_back(new NodeZeroOP());
	nodeList.push_back(new NodeNumber<int>(arrElementCount, typeInt));
	TypeInfo *targetType = GetArrayType(currType);
	if(!targetType)
		ThrowLastError();
	nodeList.push_back(new NodeExpressionList(targetType));

	NodeZeroOP* temp = nodeList.back();
	nodeList.pop_back();

	NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp);

	TypeInfo *realType = nodeList.back()->GetTypeInfo();
	for(unsigned int i = 0; i < arrElementCount; i++)
	{
		if(realType != currType && !((realType == typeShort || realType == typeChar) && currType == typeInt) && !(realType == typeFloat && currType == typeDouble))
		{
			sprintf(callbackError, "ERROR: element %d doesn't match the type of element 0 (%s)", arrElementCount-i-1, currType->GetTypeName().c_str());
			ThrowError(callbackError, s);
		}
		arrayList->AddNode(false);
	}

	nodeList.push_back(temp);
}

void FunctionAdd(char const* s, char const* e)
{
	(void)e;	// C4100
	for(unsigned int i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
	{
		if(varInfo[i]->name == strs.back())
		{
			sprintf(callbackError, "ERROR: Name '%s' is already taken for a variable in current scope", strs.back().c_str());
			ThrowError(callbackError, s);
		}
	}
	std::string name = strs.back();
	if(name == "if" || name == "else" || name == "for" || name == "while" || name == "return" || name=="switch" || name=="case")
	{
		sprintf(callbackError, "ERROR: The name '%s' is reserved", name.c_str());
		ThrowError(callbackError, s);
	}
	if(!currType)
		ThrowError("ERROR: function return type cannot be auto", s);
	funcInfo.push_back(new FunctionInfo());
	funcInfo.back()->name = name;
	funcInfo.back()->vTopSize = (unsigned int)varInfoTop.size();
	retTypeStack.push_back(currType);
	funcInfo.back()->retType = currType;
	if(newType)
		funcInfo.back()->type = FunctionInfo::THISCALL;
	if(newType ? varInfoTop.size() > 2 : varInfoTop.size() > 1)
		funcInfo.back()->type = FunctionInfo::LOCAL;
	currDefinedFunc.push_back(funcInfo.back());

	if(newType)
		funcInfo.back()->name = newType->name + "::" + name;

	funcInfo.back()->nameHash = GetStringHash(funcInfo.back()->name.c_str());

	if(varDefined && varInfo.back()->varType == NULL)
		varTop += 8;
}

void FunctionParam(char const* s, char const* e)
{
	(void)e;	// C4100
	if(!currType)
		ThrowError("ERROR: function parameter cannot be an auto type", s);
	funcInfo.back()->params.push_back(VariableInfo(strs.back(), 0, currType, currValConst));
	funcInfo.back()->allParamSize += currType->size;
	strs.pop_back();
}
void FunctionStart(char const* s, char const* e)
{
	(void)s; (void)e;	// C4100
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));

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
			for(unsigned int k = 0; k < funcInfo[i]->params.size(); k++)
			{
				if(funcInfo[n]->params[k].varType->GetTypeName() != funcInfo[i]->params[k].varType->GetTypeName())
					paramsEqual = false;
			}
			if(paramsEqual)
			{
				sprintf(callbackError, "ERROR: function '%s' is being defined with the same set of parameters", funcInfo[i]->name.c_str());
				ThrowError(callbackError, s);
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
	strs.pop_back();

	retTypeStack.pop_back();
	currDefinedFunc.pop_back();

	// If function is local, create function parameters block
	if(lastFunc.type == FunctionInfo::LOCAL && !lastFunc.external.empty())
	{
		nodeList.push_back(new NodeZeroOP());
		nodeList.push_back(new NodeNumber<int>((int)lastFunc.external.size(), typeInt));
		TypeInfo *targetType = GetReferenceType(typeInt);
		if(!targetType)
			ThrowLastError();
		targetType = GetArrayType(targetType);
		if(!targetType)
			ThrowLastError();
		nodeList.push_back(new NodeExpressionList(targetType));

		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();

		NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp);

		for(unsigned int n = 0; n < lastFunc.external.size(); n++)
		{
			const char *s = lastFunc.external[n].c_str(), *e = s + lastFunc.external[n].length();
			AddGetAddressNode(s, e);
			currTypes.pop_back();
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
		clFunc.defNode = nodeList.back();
	}
}

void addFuncCallNode(char const* s, char const* e, unsigned int callArgCount)
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
		unsigned int	fRating[32];
		memset(fRating, 0, 32*4);

		unsigned int count = 0;
		for(int k = 0; k < (int)funcInfo.size(); k++)
			if(funcInfo[k]->name == fname && funcInfo[k]->visible)
				fList[count++] = funcInfo[k];
		if(count == 0)
		{
			sprintf(callbackError, "ERROR: function '%s' is undefined", fname.c_str());
			ThrowError(callbackError, s);
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
				unsigned int	nodeType = activeNode->GetNodeType();
				TypeInfo *expectedType = fList[k]->params[n].varType;
				if(expectedType != paramType)
				{
					if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->arrSize != 0 && paramType->subType == expectedType->subType)
						fRating[k] += 5;
					else if(expectedType->funcType != NULL && nodeType == typeNodeFuncDef ||
							(nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(activeNode)->GetFirstNode()->GetNodeType() == typeNodeFuncDef))
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
			std::string errTemp;
			errTemp.append("ERROR: can't find function '");
			errTemp.append(fname);
			errTemp.append("' with following parameters:\r\n  ");
			errTemp.append(fname);
			errTemp.append("(");
			for(unsigned int n = 0; n < callArgCount; n++)
			{
				errTemp.append(nodeList[nodeList.size()-callArgCount+n]->GetTypeInfo()->GetTypeName());
				errTemp.append(n != callArgCount-1 ? ", " : "");
			}
			errTemp.append(")\r\n");
			errTemp.append(" the only available are:\r\n");
			for(unsigned int n = 0; n < count; n++)
			{
				errTemp.append("  ");
				errTemp.append(fname);
				errTemp.append("(");
				for(unsigned int m = 0; m < fList[n]->params.size(); m++)
				{
					errTemp.append(fList[n]->params[m].varType->GetTypeName());
					errTemp.append(m != fList[n]->params.size()-1 ? ", " : "");
				}
				errTemp.append(")\r\n");
			}
			lastError = CompilerError(errTemp.c_str(), s);
			ThrowLastError();
		}
		// Check, is there are more than one function, that share the same rating
		for(unsigned int k = 0; k < count; k++)
		{
			if(k != minRatingIndex && fRating[k] == minRating)
			{
				std::string errTemp;
				errTemp.append("ERROR: ambiguity, there is more than one overloaded function available for the call.\r\n");
				errTemp.append("  ");
				errTemp.append(fname);
				errTemp.append("(");
				for(unsigned int n = 0; n < callArgCount; n++)
				{
					errTemp.append(nodeList[nodeList.size()-callArgCount+n]->GetTypeInfo()->GetTypeName());
					errTemp.append(n != callArgCount-1 ? ", " : "");
				}
				errTemp.append(")\r\n");
				errTemp.append(" candidates are:\r\n");
				for(unsigned int n = 0; n < count; n++)
				{
					if(fRating[n] != minRating)
						continue;
					errTemp.append("  ");
					errTemp.append(fname);
					errTemp.append("(");
					for(unsigned int m = 0; m < fList[n]->params.size(); m++)
					{
						errTemp.append(fList[n]->params[m].varType->GetTypeName());
						errTemp.append(m != fList[n]->params.size()-1 ? ", " : "");
					}
					errTemp.append(")\r\n");
				}
				lastError = CompilerError(errTemp.c_str(), s);
				ThrowLastError();
			}
		}
		fType = fList[minRatingIndex]->funcType->funcType;
		fInfo = fList[minRatingIndex];
	}else{
		AddGetAddressNode(fname.c_str(), fname.length()+fname.c_str());
		AddGetVariableNode(s, e);
		fType = nodeList.back()->GetTypeInfo()->funcType;
	}

	vector<NodeZeroOP*> paramNodes;
	for(unsigned int i = 0; i < fType->paramType.size(); i++)
	{
		paramNodes.push_back(nodeList.back());
		nodeList.pop_back();
	}
	vector<NodeZeroOP*> inplaceArray;

	for(unsigned int i = 0; i < fType->paramType.size(); i++)
	{
		unsigned int index = (unsigned int)(fType->paramType.size()) - i - 1;

		TypeInfo *expectedType = fType->paramType[i];
		TypeInfo *realType = paramNodes[index]->GetTypeInfo();
		
		if(paramNodes[index]->GetNodeType() == typeNodeFuncDef ||
			(paramNodes[index]->GetNodeType() == typeNodeExpressionList && static_cast<NodeExpressionList*>(paramNodes[index])->GetFirstNode()->GetNodeType() == typeNodeFuncDef))
		{
			NodeFuncDef*	funcDefNode = (NodeFuncDef*)(paramNodes[index]->GetNodeType() == typeNodeFuncDef ? paramNodes[index] : static_cast<NodeExpressionList*>(paramNodes[index])->GetFirstNode());
			AddGetAddressNode(funcDefNode->GetFuncInfo()->name.c_str(), funcDefNode->GetFuncInfo()->name.c_str() + funcDefNode->GetFuncInfo()->name.length());
			currTypes.pop_back();

			NodeExpressionList* listExpr = new NodeExpressionList(paramNodes[index]->GetTypeInfo());
			nodeList.push_back(paramNodes[index]);
			listExpr->AddNode();
			nodeList.push_back(listExpr);
		}else if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && expectedType->subType == realType->subType && expectedType != realType){
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
					sprintf(callbackError, "ERROR: array expected as a parameter %d", i);
					ThrowError(callbackError, s);
				}
			}
			unsigned int typeSize = (paramNodes[index]->GetTypeInfo()->size - paramNodes[index]->GetTypeInfo()->paddingBytes) / paramNodes[index]->GetTypeInfo()->subType->size;
			nodeList.push_back(static_cast<NodeDereference*>(paramNodes[index])->GetFirstNode());
			static_cast<NodeDereference*>(paramNodes[index])->SetFirstNode(NULL);
			delete paramNodes[index];
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
		std::string bName = "$" + fInfo->name + "_ext";
		int i = (int)varInfo.size()-1;
		while(i >= 0 && varInfo[i]->name != bName)
			i--;
		if(i == -1)
		{
			nodeList.push_back(new NodeNumber<int>(0, GetReferenceType(typeInt)));
		}else{
			AddGetAddressNode(bName.c_str(), bName.c_str()+bName.length());
			if(currTypes.back()->refLevel == 1)
				AddDereferenceNode(s, e);
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
			addTwoExprNode(s, e);
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
		sprintf(callbackError, "ERROR: ternary operator ?: \r\n result types are not equal (%s : %s)", typeB->name.c_str(), typeA->name.c_str());
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
		ThrowError("ERROR: alignment must me less than 16 bytes", s);
	newType = new TypeInfo();
	newType->name = std::string(s, e);
	newType->nameHash = GetStringHash(newType->name.c_str());
	newType->type = TypeInfo::TYPE_COMPLEX;
	newType->alignBytes = currAlign;
	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;

	typeInfo.push_back(newType);
	
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
}

void TypeAddMember(char const* s, char const* e)
{
	if(!currType)
		ThrowError("ERROR: auto cannot be used for class members", s);
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

	nodeList.push_back(new NodeZeroOP());
	for(unsigned int i = 0; i < newType->memberFunctions.size(); i++)
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

// �������, �������� � ��������� ������ �� ����� �����
// ���� ����� ����� �������������� ��� �������� ��������� �������� ����� ������� ����������
// �������� ��� ������� ������������� ���������� a[i], ����� ��������� "a" � ����,
// ������ ��� � ������� ��������� "a[i]" �������
void ParseStrPush(char const *s, char const *e)
{
	strs.push_back(string(s,e));
}
void ParseStrPop(char const *s, char const *e)
{
	(void)s; (void)e;	// C4100
	strs.pop_back();
}

// ��� ������� ����������, ����� ��������� ������ ���� � ����, ������� ��� �����������
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

	for(std::set<VariableInfo*>::iterator s = varInfoAll.begin(), e = varInfoAll.end(); s!=e; s++)
		delete *s;
	varInfoAll.clear();

	retTypeStack.clear();
	currDefinedFunc.clear();

	currTypes.clear();

	funcDefList.clear();

	varDefined = 0;
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

	warningLog.clear();

}

void CallbackDeinitialize()
{
	for(std::set<VariableInfo*>::iterator s = varInfoAll.begin(), e = varInfoAll.end(); s!=e; s++)
		delete *s;
	varInfoAll.clear();
}
