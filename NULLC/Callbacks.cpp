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
unsigned int	VariableInfo::buildInSize = 0;
ChunkedStackPool<4092> VariableInfo::variablePool;
unsigned int	FunctionInfo::buildInSize;
ChunkedStackPool<4092>	FunctionInfo::functionPool;

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
	unsigned int i = 0;
	for(FunctionInfo::ExternalName *curr = func->firstExternal; curr; curr = curr->next, i++)
		if(curr->nameHash == hash)
			return i;

	func->AddExternal(name, hash);
	return func->externalCount - 1;
}

int parseInteger(const char* str)
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

long long parseLong(const char* s, const char* e, int base)
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

double parseDouble(const char *str)
{
	unsigned int digit;
	double integer = 0.0;
	while((digit = *str - '0') < 10)
	{
		integer = integer * 10.0 + digit;
		str++;
	}

	double fractional = 0.0;
	double power = 0.1f;
	
	if(*str == '.')
	{
		str++;
		while((digit = *str - '0') < 10)
		{
			fractional = fractional + power * digit;
			power /= 10.0;
			str++;
		}
	}
	if(*str == 'e')
	{
		str++;
		int power = parseInteger(str);
		return (integer + fractional) * pow(10, (double)power);
	}
	return integer + fractional;
}

// ���������� � ������ ����� {}, ����� ��������� ���������� ����������� ����������, � �������� �����
// ����� �������� ����� ��������� �����.
void BeginBlock()
{
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
	funcInfoTop.push_back((unsigned int)funcInfo.size());
}
// ���������� � ����� ����� {}, ����� ������ ���������� � ���������� ������ �����, ��� ����� �����������
// �� ����� �� ������� ���������. ����� ��������� ������� ����� ���������� � ������.
void EndBlock()
{
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
void AddNumberNodeChar(const char* pos)
{
	char res = pos[1];
	if(res == '\\')
		res = UnescapeSybmol(pos[2]);
	nodeList.push_back(new NodeNumber<int>(res, typeChar));
}

void AddNumberNodeInt(const char* pos)
{
	nodeList.push_back(new NodeNumber<int>(parseInteger(pos), typeInt));
}
void AddNumberNodeFloat(const char* pos)
{
	nodeList.push_back(new NodeNumber<float>((float)parseDouble(pos), typeFloat));
}
void AddNumberNodeLong(const char* pos, const char* end)
{
	nodeList.push_back(new NodeNumber<long long>(parseLong(pos, end, 10), typeLong));
}
void AddNumberNodeDouble(const char* pos)
{
	nodeList.push_back(new NodeNumber<double>(parseDouble(pos), typeDouble));
}

void AddVoidNode()
{
	nodeList.push_back(new NodeZeroOP());
}

void AddHexInteger(const char* pos, const char* end)
{
	pos += 2;
	if(int(end - pos) > 16)
		ThrowError("ERROR: Overflow in hexadecimal constant", pos);
	if(int(end - pos) <= 8)
		nodeList.push_back(new NodeNumber<int>((unsigned int)parseLong(pos, end, 16), typeInt));
	else
		nodeList.push_back(new NodeNumber<long long>(parseLong(pos, end, 16), typeLong));
}

void AddOctInteger(const char* pos, const char* end)
{
	pos++;
	if(int(end - pos) > 21)
		ThrowError("ERROR: Overflow in octal constant", pos);
	if(int(end - pos) <= 10)
		nodeList.push_back(new NodeNumber<int>((unsigned int)parseLong(pos, end, 8), typeInt));
	else
		nodeList.push_back(new NodeNumber<long long>(parseLong(pos, end, 8), typeLong));
}

void AddBinInteger(const char* pos, const char* end)
{
	if(int(end - pos) > 64)
		ThrowError("ERROR: Overflow in binary constant", pos);
	if(int(end - pos) <= 32)
		nodeList.push_back(new NodeNumber<int>((unsigned int)parseLong(pos, end, 2), typeInt));
	else
		nodeList.push_back(new NodeNumber<long long>(parseLong(pos, end, 2), typeLong));
}
// ������� ��� �������� ����, ������� ����� ������ � ����
// ������������ NodeExpressionList, ��� �� �������� ����� ������� � �������� ���������
// �� ���� �� ���� ������ ��������� ����� � ����������� ���������� ������.
void AddStringNode(const char* s, const char* e)
{
	lastKnownStartPos = s;

	const char *curr = s + 1, *end = e - 1;
	unsigned int len = 0;
	// Find the length of the string with collapsed escape-sequences
	for(; curr < end; curr++, len++)
	{
		if(*curr == '\\')
			curr++;
	}
	curr = s + 1;
	end = e - 1;

	nodeList.push_back(new NodeZeroOP());
	TypeInfo *targetType = GetArrayType(typeChar, len + 1);
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
void AddPopNode(const char* s, const char* e)
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
void AddNegateNode(const char* pos)
{
	// ���� ��������� ���� ��� �����, �� ������ �������� ���� � ���������
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->typeInfo;
		NodeZeroOP* zOP = nodeList.back();
		NodeZeroOP* Rd = NULL;
		if(aType == typeDouble)
		{
			Rd = new NodeNumber<double>(-static_cast<NodeNumber<double>* >(zOP)->GetVal(), zOP->typeInfo);
		}else if(aType == typeFloat){
			Rd = new NodeNumber<float>(-static_cast<NodeNumber<float>* >(zOP)->GetVal(), zOP->typeInfo);
		}else if(aType == typeLong){
			Rd = new NodeNumber<long long>(-static_cast<NodeNumber<long long>* >(zOP)->GetVal(), zOP->typeInfo);
		}else if(aType == typeInt){
			Rd = new NodeNumber<int>(-static_cast<NodeNumber<int>* >(zOP)->GetVal(), zOP->typeInfo);
		}else{
			sprintf(callbackError, "addNegNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, pos);
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
void AddLogNotNode(const char* pos)
{
	// ���� ��������� ���� � ������ - �����, �� ��������� �������� �� ����� ���������
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->typeInfo;
		NodeZeroOP* zOP = nodeList.back();
		NodeZeroOP* Rd = NULL;
		if(aType == typeDouble)
		{
			Rd = new NodeNumber<double>(static_cast<NodeNumber<double>* >(zOP)->GetLogNotVal(), zOP->typeInfo);
		}else if(aType == typeFloat){
			Rd = new NodeNumber<float>(static_cast<NodeNumber<float>* >(zOP)->GetLogNotVal(), zOP->typeInfo);
		}else if(aType == typeLong){
			Rd = new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetLogNotVal(), zOP->typeInfo);
		}else if(aType == typeInt){
			Rd = new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetLogNotVal(), zOP->typeInfo);
		}else{
			sprintf(callbackError, "addLogNotNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, pos);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// ����� ������ �������� ���, ��� � ����������� � ������
		nodeList.push_back(new NodeUnaryOp(cmdLogNot));
	}
}
void AddBitNotNode(const char* pos)
{
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->typeInfo;
		NodeZeroOP* zOP = nodeList.back();
		NodeZeroOP* Rd = NULL;
		if(aType == typeDouble)
		{
			ThrowError("ERROR: bitwise NOT cannot be used on floating point numbers", pos);
		}else if(aType == typeFloat){
			ThrowError("ERROR: bitwise NOT cannot be used on floating point numbers", pos);
		}else if(aType == typeLong){
			Rd = new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetBitNotVal(), zOP->typeInfo);
		}else if(aType == typeInt){
			Rd = new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetBitNotVal(), zOP->typeInfo);
		}else{
			sprintf(callbackError, "addBitNotNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, pos);
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

void RemoveLastNode(bool swap)
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
		aType = nodeList[nodeList.size()-2]->typeInfo;
		bType = nodeList[nodeList.size()-1]->typeInfo;

		//Swap operands, to reduce number of combinations
		if((aType == typeFloat || aType == typeLong || aType == typeInt) && bType == typeDouble)
			Swap(shA, shB);
		if((aType == typeLong || aType == typeInt) && bType == typeFloat)
			Swap(shA, shB);
		if(aType == typeInt && bType == typeLong)
			Swap(shA, shB);

		bool swapOper = shA != 2;

		aType = nodeList[nodeList.size()-shA]->typeInfo;
		bType = nodeList[nodeList.size()-shB]->typeInfo;
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
		bType = nodeList[nodeList.size()-shA]->typeInfo;
		if(bType == typeDouble)
		{
			NodeNumber<double> *Ad = static_cast<NodeNumber<double>* >(nodeList[nodeList.size()-shA]);
			if(Ad->GetVal() == 0.0 && id == cmdMul)
			{
				RemoveLastNode(shA == 1); // a*0.0 -> 0.0
				success = true;
			}
			if((Ad->GetVal() == 0.0 && id == cmdAdd) || (Ad->GetVal() == 1.0 && id == cmdMul))
			{
				RemoveLastNode(shA == 2); // a+0.0 -> a || a*1.0 -> a
				success = true;
			}
		}else if(bType == typeFloat){
			NodeNumber<float> *Ad = static_cast<NodeNumber<float>* >(nodeList[nodeList.size()-shA]);
			if(Ad->GetVal() == 0.0f && id == cmdMul)
			{
				RemoveLastNode(shA == 1); // a*0.0f -> 0.0f
				success = true;
			}
			if((Ad->GetVal() == 0.0f && id == cmdAdd) || (Ad->GetVal() == 1.0f && id == cmdMul))
			{
				RemoveLastNode(shA == 2); // a+0.0f -> a || a*1.0f -> a
				success = true;
			}
		}else if(bType == typeLong){
			NodeNumber<long long> *Ad = static_cast<NodeNumber<long long>* >(nodeList[nodeList.size()-shA]);
			if(Ad->GetVal() == 0 && id == cmdMul)
			{
				RemoveLastNode(shA == 1); // a*0L -> 0L
				success = true;
			}
			if((Ad->GetVal() == 0 && id == cmdAdd) || (Ad->GetVal() == 1 && id == cmdMul))
			{
				RemoveLastNode(shA == 2); // a+0L -> a || a*1L -> a
				success = true;
			}
		}else if(bType == typeInt){
			NodeNumber<int> *Ad = static_cast<NodeNumber<int>* >(nodeList[nodeList.size()-shA]);
			if(Ad->GetVal() == 0 && id == cmdMul)
			{
				RemoveLastNode(shA == 1); // a*0 -> 0
				success = true;
			}
			if((Ad->GetVal() == 0 && id == cmdAdd) || (Ad->GetVal() == 1 && id == cmdMul))
			{
				RemoveLastNode(shA == 2); // a+0 -> a || a*1 -> a
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

void AddReturnNode(const char* pos, const char* end)
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
	TypeInfo *realRetType = nodeList.back()->typeInfo;
	if(retTypeStack.back() && (retTypeStack.back()->type == TypeInfo::TYPE_COMPLEX || realRetType->type == TypeInfo::TYPE_COMPLEX) && retTypeStack.back() != realRetType)
	{
		sprintf(callbackError, "ERROR: function returns %s but supposed to return %s", realRetType->GetFullTypeName(), retTypeStack.back()->GetFullTypeName());
		ThrowError(callbackError, pos);
	}
	if(retTypeStack.back() && retTypeStack.back()->type == TypeInfo::TYPE_VOID && realRetType != typeVoid)
		ThrowError("ERROR: function returning a value", pos);
	if(retTypeStack.back() && retTypeStack.back() != typeVoid && realRetType == typeVoid)
	{
		sprintf(callbackError, "ERROR: function should return %s", retTypeStack.back()->GetFullTypeName());
		ThrowError(callbackError, pos);
	}
	if(!retTypeStack.back() && realRetType == typeVoid)
		ThrowError("ERROR: global return cannot accept void", pos);
	nodeList.push_back(new NodeReturnOp(c, retTypeStack.back()));
	nodeList.back()->SetCodeInfo(pos, end);
}

void AddBreakNode(const char* pos)
{
	if(cycleBeginVarTop.size() == 0)
		ThrowError("ERROR: break used outside loop statement", pos);
	int t = (int)varInfoTop.size();
	int c = 0;
	while(t > (int)cycleBeginVarTop.back())
	{
		c++;
		t--;
	}
	nodeList.push_back(new NodeBreakOp(c));
}

void AddContinueNode(const char* pos)
{
	if(cycleBeginVarTop.size() == 0)
		ThrowError("ERROR: continue used outside loop statement", pos);
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

unsigned int	offsetBytes = 0;

void AddVariable(const char* pos, InplaceStr varName)
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
	if(FindFunctionByName(hash, funcInfo.size() - 1) != -1)
	{
		sprintf(callbackError, "ERROR: Name '%.*s' is already taken for a function", varName.end-varName.begin, varName.begin);
		ThrowError(callbackError, pos);
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

void AddVariableReserveNode(const char* pos)
{
	assert(varDefined);
	if(!currType)
		ThrowError("ERROR: auto variable must be initialized in place of definition", pos);
	nodeList.push_back(new NodeZeroOP());
	varInfo.back()->dataReserved = true;
	varDefined = 0;
	offsetBytes = 0;
}

void PushType()
{
	currTypes.push_back(currType);
}

void PopType()
{
	currTypes.pop_back();
}

void ConvertTypeToReference(const char* pos)
{
	lastKnownStartPos = pos;
	if(!currType)
		ThrowError("ERROR: auto variable cannot have reference flag", pos);
	currType = GetReferenceType(currType);
}

void ConvertTypeToArray(const char* pos)
{
	lastKnownStartPos = pos;
	if(!currType)
		ThrowError("ERROR: cannot specify array size for auto variable", pos);
	currType = GetArrayType(currType);
	if(!currType)
		ThrowLastError();
}

//////////////////////////////////////////////////////////////////////////
//					New functions for work with variables

void GetTypeSize(const char* pos, bool sizeOfExpr)
{
	if(!sizeOfExpr && !currTypes.back())
		ThrowError("ERROR: sizeof(auto) is illegal", pos);
	if(sizeOfExpr)
	{
		currTypes.back() = nodeList.back()->typeInfo;
		nodeList.pop_back();
	}
	nodeList.push_back(new NodeNumber<int>(currTypes.back()->size, typeInt));
}

void SetTypeOfLastNode()
{
	currType = nodeList.back()->typeInfo;
	nodeList.pop_back();
}

void AddInplaceArray(const char* pos);

// ������� ��� ��������� ������ ����������, ��� ������� ��������� � ����������
void AddGetAddressNode(const char* pos, InplaceStr varName)
{
	lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// Find in variable list
	int i = FindVariableByName(hash);
	if(i == -1)
	{ 
		int fID = FindFunctionByName(hash, funcInfo.size()-1);
		if(fID == -1)
		{
			sprintf(callbackError, "ERROR: function '%s' is not defined", varName);
			ThrowError(callbackError, pos);
		}

		if(FindFunctionByName(hash, fID - 1) != -1)
		{
			sprintf(callbackError, "ERROR: there are more than one '%s' function, and the decision isn't clear", varName);
			ThrowError(callbackError, pos);
		}

		// ����� � ���� ����� ���
		currTypes.push_back(funcInfo[fID]->funcType);

		if(funcInfo[fID]->funcPtr != 0)
			ThrowError("ERROR: Can't get a pointer to an extern function", pos);
		if(funcInfo[fID]->address == -1 && funcInfo[fID]->funcPtr == NULL)
			ThrowError("ERROR: Can't get a pointer to a build-in function", pos);
		if(funcInfo[fID]->type == FunctionInfo::LOCAL)
		{
			char	*contextName = AllocateString(funcInfo[fID]->nameLength + 6);
			int length = sprintf(contextName, "$%s_ext", funcInfo[fID]->name);
			unsigned int contextHash = GetStringHash(contextName);

			int i = FindVariableByName(contextHash);
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
	}else{
		// ����� � ���� ����� ���
		currTypes.push_back(varInfo[i]->varType);

		if(newType && currDefinedFunc.back()->type == FunctionInfo::THISCALL)
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

				AddDereferenceNode(pos);
				AddMemberAccessNode(pos, varName);

				currTypes.pop_back();
				return;
			}
		}

		// ���� �� ��������� � ��������� �������, � ���������� ��������� � �������� ������� ���������
		if((int)retTypeStack.size() > 1 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) && i < (int)varInfoTop[currDefinedFunc.back()->vTopSize].activeVarCnt)
		{
			FunctionInfo *currFunc = currDefinedFunc.back();
			// ������� ��� ���������� � ������ ������� ���������� �������
			int num = AddFunctionExternal(currFunc, varName);

			TypeInfo *temp = GetReferenceType(typeInt);
			temp = GetArrayType(temp, (int)currDefinedFunc.back()->externalCount);
			if(!temp)
				ThrowLastError();
			temp = GetReferenceType(temp);
			currTypes.push_back(temp);

			nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));
			AddDereferenceNode(pos);
			nodeList.push_back(new NodeNumber<int>(num, typeInt));
			AddArrayIndexNode(pos);
			AddDereferenceNode(pos);
			// ������ ������� ���
			currTypes.pop_back();
		}else{
			// ���� ���������� ��������� � ���������� ������� ���������, � ����� - ����������, ��� �������
			bool absAddress = ((varInfoTop.size() > 1) && (varInfo[i]->pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;

			int varAddress = varInfo[i]->pos;
			if(!absAddress)
				varAddress -= (int)(varInfoTop.back().varStackSize);

			// ������� ���� ��� ��������� ��������� �� ����������
			nodeList.push_back(new NodeGetAddress(varInfo[i], varAddress, absAddress, varInfo[i]->varType));
		}
	}
}

// ������� ���������� ��� ���������� �������
void AddArrayIndexNode(const char* pos)
{
	lastKnownStartPos = pos;

	// ��� ������ ���� ��������
	if(currTypes.back()->arrLevel == 0)
		ThrowError("ERROR: indexing variable that is not an array", pos);
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
		TypeInfo *aType = indexNode->typeInfo;
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
			ThrowError(callbackError, pos);
		}

		// �������� ������ �� ����� �� ������� �������
		if(shiftValue < 0)
			ThrowError("ERROR: Array index cannot be negative", pos);
		if((unsigned int)shiftValue >= currTypes.back()->arrSize)
			ThrowError("ERROR: Array index out of bounds", pos);

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
void AddDereferenceNode(const char* pos)
{
	lastKnownStartPos = pos;

	// ������ ���� �������������
	nodeList.push_back(new NodeDereference(currTypes.back()));
	// ������ ������� ��� - ��� �� ������� ��������� ������
	currTypes.back() = GetDereferenceType(currTypes.back());
	if(!currTypes.back())
		ThrowLastError();
}

// ���������� � ������ ������������, ��� ����� ���������� ����� ��������� ���� ������������
// ����� ��� ����, ������� ��������� ������� ����
void FailedSetVariable()
{
	nodeList.pop_back();
}

// ������� ���������� ��� ���������� ���������� � ������������� ������������� �� ��������
void AddDefineVariableNode(const char* pos, InplaceStr varName)
{
	lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// ���� ���������� �� �����
	int i = FindVariableByName(hash);
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
	TypeInfo *realCurrType = currTypes.back() ? currTypes.back() : nodeList.back()->typeInfo;

	// ��������, ��� ����������� �������� ���������� ����������� �������� ��������������� ���� ������
	// ���������� ������ ��� ����, ������������, ��� ��� ���� ���� ���������� � ����
	bool unifyTwo = false;
	// ���� ��� ���������� - ������������ ������, � ������������� �� �������� ������� ����
	if(realCurrType->arrSize == TypeInfo::UNSIZED_ARRAY && realCurrType != nodeList.back()->typeInfo)
	{
		TypeInfo *nodeType = nodeList.back()->typeInfo;
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
					sprintf(callbackError, "ERROR: cannot convert from %s to %s", nodeList.back()->typeInfo->GetFullTypeName(), realCurrType->GetFullTypeName());
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
		realCurrType = nodeList.back()->typeInfo;
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

	nodeList.push_back(new NodeGetAddress(varInfo[i], varInfo[i]->pos-(int)(varInfoTop.back().varStackSize), absAddress, varInfo[i]->varType));

	nodeList.push_back(new NodeVariableSet(realCurrType, varSizeAdd, false));

	if(unifyTwo)
	{
		nodeList.push_back(new NodeExpressionList(nodeList.back()->typeInfo));
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp)->AddNode();
		nodeList.push_back(temp);
	}
}

void AddSetVariableNode(const char* pos)
{
	lastKnownStartPos = pos;

	TypeInfo *realCurrType = currTypes.back();
	bool unifyTwo = false;
	if(realCurrType->arrSize == TypeInfo::UNSIZED_ARRAY && realCurrType != nodeList.back()->typeInfo)
	{
		TypeInfo *nodeType = nodeList.back()->typeInfo;
		if(realCurrType->subType == nodeType->subType)
		{
			if(nodeList.back()->nodeType != typeNodeDereference)
			{
				if(nodeList.back()->nodeType == typeNodeExpressionList)
				{
					AddInplaceArray(pos);
					currTypes.pop_back();
					unifyTwo = true;
				}else{
					sprintf(callbackError, "ERROR: cannot convert from %s to %s", nodeList.back()->typeInfo->GetFullTypeName(), realCurrType->GetFullTypeName());
					ThrowError(callbackError, pos);
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
		AddGetAddressNode(pos, InplaceStr(funcDefNode->GetFuncInfo()->name, funcDefNode->GetFuncInfo()->nameLength));
		currTypes.pop_back();
		unifyTwo = true;
		Swap(nodeList[nodeList.size()-2], nodeList[nodeList.size()-3]);
	}

	nodeList.push_back(new NodeVariableSet(currTypes.back(), 0, true));
	if(!lastError.IsEmpty())
		ThrowLastError();

	if(unifyTwo)
	{
		nodeList.push_back(new NodeExpressionList(nodeList.back()->typeInfo));
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp)->AddNode();
		nodeList.push_back(temp);
	}
}

void AddGetVariableNode(const char* pos)
{
	lastKnownStartPos = pos;

	if(nodeList.back()->typeInfo->funcType == NULL)
		nodeList.push_back(new NodeDereference(currTypes.back()));
}

void AddMemberAccessNode(const char* pos, InplaceStr varName)
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

		fID = FindFunctionByName(hash, funcInfo.size()-1);
		if(fID == -1)
		{
			sprintf(callbackError, "ERROR: function '%s' is not defined", varName);
			ThrowError(callbackError, pos);
		}

		if(FindFunctionByName(hash, fID - 1) != -1)
		{
			sprintf(callbackError, "ERROR: there are more than one '%s' function, and the decision isn't clear", varName);
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

void AddMemberFunctionCall(const char* pos, const char* funcName, unsigned int callArgCount)
{
	char	*memberFuncName = AllocateString((int)strlen(currTypes.back()->name) + 2 + (int)strlen(funcName) + 1);
	sprintf(memberFuncName, "%s::%s", currTypes.back()->name, funcName);
	AddFunctionCallNode(pos, memberFuncName, callArgCount);
	currTypes.back() = nodeList.back()->typeInfo;
}

void AddPreOrPostOpNode(bool isInc, bool prefixOp)
{
	nodeList.push_back(new NodePreOrPostOp(currTypes.back(), isInc, prefixOp));
}

void AddModifyVariableNode(const char* pos, CmdID cmd)
{
	lastKnownStartPos = pos;

	TypeInfo *targetType = GetDereferenceType(nodeList[nodeList.size()-2]->typeInfo);
	if(!targetType)
		ThrowLastError();
	nodeList.push_back(new NodeVariableModify(targetType, cmd));
}

void AddInplaceArray(const char* pos)
{
	char	*arrName = AllocateString(16);
	int length = sprintf(arrName, "$carr%d", inplaceArrayNum++);

	TypeInfo *saveCurrType = currType;
	bool saveVarDefined = varDefined;

	currType = NULL;
	AddVariable(pos, InplaceStr(arrName, length));

	AddDefineVariableNode(pos, InplaceStr(arrName, length));
	AddPopNode(pos, pos);
	currTypes.pop_back();

	AddGetAddressNode(pos, InplaceStr(arrName, length));
	AddGetVariableNode(pos);

	varDefined = saveVarDefined;
	currType = saveCurrType;
}

//////////////////////////////////////////////////////////////////////////
void AddOneExpressionNode()
{
	nodeList.push_back(new NodeExpressionList());
}
void AddTwoExpressionNode()
{
	if(nodeList.back()->nodeType != typeNodeExpressionList)
		AddOneExpressionNode();
	// Take the expression list from the top
	NodeZeroOP* temp = nodeList.back();
	nodeList.pop_back();
	static_cast<NodeExpressionList*>(temp)->AddNode();
	nodeList.push_back(temp);
}

void AddArrayConstructor(const char* pos, unsigned int arrElementCount)
{
	arrElementCount++;

	TypeInfo *currType = nodeList[nodeList.size()-arrElementCount]->typeInfo;

	if(currType == typeShort || currType == typeChar)
		currType = typeInt;
	if(currType == typeFloat)
		currType = typeDouble;
	if(currType == typeVoid)
		ThrowError("ERROR: array cannot be constructed from void type elements", pos);

	nodeList.push_back(new NodeZeroOP());
	TypeInfo *targetType = GetArrayType(currType, arrElementCount);
	if(!targetType)
		ThrowLastError();

	NodeExpressionList *arrayList = new NodeExpressionList(targetType);

	TypeInfo *realType = nodeList.back()->typeInfo;
	for(unsigned int i = 0; i < arrElementCount; i++)
	{
		if(realType != currType && !((realType == typeShort || realType == typeChar) && currType == typeInt) && !(realType == typeFloat && currType == typeDouble))
		{
			sprintf(callbackError, "ERROR: element %d doesn't match the type of element 0 (%s)", arrElementCount-i-1, currType->GetFullTypeName());
			ThrowError(callbackError, pos);
		}
		arrayList->AddNode(false);
	}

	nodeList.push_back(arrayList);
}

void FunctionAdd(const char* pos, const char* funcName)
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

void FunctionParameter(const char* pos, InplaceStr paramName)
{
	if(!currType)
		ThrowError("ERROR: function parameter cannot be an auto type", pos);
	unsigned int hash = GetStringHash(paramName.begin, paramName.end);
	funcInfo.back()->AddParameter(new VariableInfo(paramName, hash, 0, currType, currValConst));
	funcInfo.back()->allParamSize += currType->size;
}
void FunctionStart(const char* pos)
{
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));

	for(VariableInfo *curr = funcInfo.back()->lastParam; curr; curr = curr->prev)
	{
		currValConst = curr->isConst;
		currType = curr->varType;
		currAlign = 1;
		AddVariable(pos, curr->name);
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

void FunctionEnd(const char* pos, const char* funcName)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	unsigned int funcNameHash = GetStringHash(funcName);

	if(newType)
	{
		funcNameHash = newType->nameHash;
		funcNameHash = StringHashContinue(funcNameHash, "::");
		funcNameHash = StringHashContinue(funcNameHash, funcName);
	}

	int i = FindFunctionByName(funcNameHash, funcInfo.size()-1);
	// Find all the functions with the same name
	for(int n = 0; n < i; n++)
	{
		if(funcInfo[n]->nameHash == funcInfo[i]->nameHash && funcInfo[n]->paramCount == funcInfo[i]->paramCount && funcInfo[n]->visible)
		{
			// Check all parameter types
			bool paramsEqual = true;
			for(VariableInfo *currN = funcInfo[n]->firstParam, *currI = funcInfo[i]->firstParam; currN; currN = currN->next, currI = currI->next)
			{
				if(currN->varType != currI->varType)
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
	if(lastFunc.type == FunctionInfo::LOCAL && lastFunc.externalCount != 0)
	{
		nodeList.push_back(new NodeZeroOP());
		TypeInfo *targetType = GetReferenceType(typeInt);
		if(!targetType)
			ThrowLastError();
		targetType = GetArrayType(targetType, lastFunc.externalCount);
		if(!targetType)
			ThrowLastError();
		nodeList.push_back(new NodeExpressionList(targetType));

		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();

		NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp);

		for(FunctionInfo::ExternalName *curr = lastFunc.firstExternal; curr; curr = curr->next)
		{
			AddGetAddressNode(pos, curr->name);
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
		AddPopNode(pos, pos);
		currTypes.pop_back();

		varDefined = saveVarDefined;
		currType = saveCurrType;
		AddTwoExpressionNode();
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

void AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount)
{
	unsigned int funcNameHash = GetStringHash(funcName);

	// Searching, if fname is actually a variable name (which means, either it is a pointer to function, or an error)
	int vID = FindVariableByName(funcNameHash);

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	if(vID == -1)
	{
		//Find all functions with given name
		FunctionInfo	*fList[32];
		unsigned int	fRating[32];

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
			if(fList[k]->paramCount != callArgCount)
			{
				fRating[k] += 65000;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.
				continue;
			}
			unsigned int n = 0;
			for(VariableInfo *curr = fList[k]->firstParam; curr; curr = curr->next, n++)//; n < fList[k]->params.size(); n++)
			{
				NodeZeroOP* activeNode = nodeList[nodeList.size() - fList[k]->paramCount + n];
				TypeInfo *paramType = activeNode->typeInfo;
				unsigned int	nodeType = activeNode->nodeType;
				TypeInfo *expectedType = curr->varType;
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
				errPos += sprintf(errPos, "%s%s", nodeList[nodeList.size()-callArgCount+n]->typeInfo->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
			errPos += sprintf(errPos, ")\r\n the only available are:\r\n");
			for(unsigned int n = 0; n < count; n++)
			{
				errPos += sprintf(errPos, "  %s(", funcName);
				for(VariableInfo *curr = fList[n]->firstParam; curr; curr = curr->next)
					errPos += sprintf(errPos, "%s%s", curr->varType->GetFullTypeName(), curr != fList[n]->lastParam ? ", " : "");
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
					errPos += sprintf(errPos, "%s%s", nodeList[nodeList.size()-callArgCount+n]->typeInfo->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
				errPos += sprintf(errPos, ")\r\n  candidates are:\r\n");
				for(unsigned int n = 0; n < count; n++)
				{
					if(fRating[n] != minRating)
						continue;
					errPos += sprintf(errPos, "  %s(", funcName);
					for(VariableInfo *curr = fList[n]->firstParam; curr; curr = curr->next)
						errPos += sprintf(errPos, "%s%s", curr->varType->GetFullTypeName(), curr != fList[n]->lastParam ? ", " : "");
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
		AddGetVariableNode(pos);
		fType = nodeList.back()->typeInfo->funcType;
	}

	paramNodes.clear();
	for(unsigned int i = 0; i < fType->paramCount; i++)
	{
		paramNodes.push_back(nodeList.back());
		nodeList.pop_back();
	}
	inplaceArray.clear();

	for(unsigned int i = 0; i < fType->paramCount; i++)
	{
		unsigned int index = fType->paramCount - i - 1;

		TypeInfo *expectedType = fType->paramType[i];
		TypeInfo *realType = paramNodes[index]->typeInfo;
		
		if(paramNodes[index]->nodeType == typeNodeFuncDef ||
			(paramNodes[index]->nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(paramNodes[index])->GetFirstNode()->nodeType == typeNodeFuncDef))
		{
			NodeFuncDef*	funcDefNode = (NodeFuncDef*)(paramNodes[index]->nodeType == typeNodeFuncDef ? paramNodes[index] : static_cast<NodeExpressionList*>(paramNodes[index])->GetFirstNode());
			AddGetAddressNode(pos, InplaceStr(funcDefNode->GetFuncInfo()->name, funcDefNode->GetFuncInfo()->nameLength));
			currTypes.pop_back();

			NodeExpressionList* listExpr = new NodeExpressionList(paramNodes[index]->typeInfo);
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
			unsigned int typeSize = (paramNodes[index]->typeInfo->size - paramNodes[index]->typeInfo->paddingBytes) / paramNodes[index]->typeInfo->subType->size;
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

		int i = FindVariableByName(contextHash);
		if(i == -1)
		{
			nodeList.push_back(new NodeNumber<int>(0, GetReferenceType(typeInt)));
		}else{
			AddGetAddressNode(pos, InplaceStr(contextName, length));
			if(currTypes.back()->refLevel == 1)
				AddDereferenceNode(pos);
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

		nodeList.push_back(new NodeExpressionList(temp->typeInfo));
		for(unsigned int i = 0; i < inplaceArray.size(); i++)
			AddTwoExpressionNode();
	}
}

void AddIfNode()
{
	nodeList.push_back(new NodeIfElseExpr(false));
}
void AddIfElseNode()
{
	nodeList.push_back(new NodeIfElseExpr(true));
}
void AddIfElseTermNode(const char* pos)
{
	TypeInfo* typeA = nodeList[nodeList.size()-1]->typeInfo;
	TypeInfo* typeB = nodeList[nodeList.size()-2]->typeInfo;
	if(typeA != typeB)
	{
		sprintf(callbackError, "ERROR: ternary operator ?: \r\n result types are not equal (%s : %s)", typeB->name, typeA->name);
		ThrowError(callbackError, pos);
	}
	nodeList.push_back(new NodeIfElseExpr(true, true));
}

void SaveVariableTop()
{
	cycleBeginVarTop.push_back((unsigned int)varInfoTop.size());
}
void AddForNode()
{
	nodeList.push_back(new NodeForExpr());
	cycleBeginVarTop.pop_back();
}
void AddWhileNode()
{
	nodeList.push_back(new NodeWhileExpr());
	cycleBeginVarTop.pop_back();
}
void AddDoWhileNode()
{
	nodeList.push_back(new NodeDoWhileExpr());
	cycleBeginVarTop.pop_back();
}

void BeginSwitch()
{
	cycleBeginVarTop.push_back((unsigned int)varInfoTop.size());
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
	nodeList.push_back(new NodeSwitchExpr());
}
void AddCaseNode()
{
	NodeZeroOP* temp = nodeList[nodeList.size()-3];
	static_cast<NodeSwitchExpr*>(temp)->AddCase();
}
void EndSwitch()
{
	cycleBeginVarTop.pop_back();
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop--;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}

void TypeBegin(const char* pos, const char* end)
{
	if(newType)
		ThrowError("ERROR: Different type is being defined", pos);
	if((int)currAlign < 0)
		ThrowError("ERROR: alignment must be a positive number", pos);
	if(currAlign > 16)
		ThrowError("ERROR: alignment must be less than 16 bytes", pos);

	char *typeNameCopy = AllocateString((int)(end - pos) + 1);
	sprintf(typeNameCopy, "%.*s", (int)(end - pos), pos);

	newType = new TypeInfo(typeInfo.size(), typeNameCopy, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	newType->alignBytes = currAlign;
	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;

	typeInfo.push_back(newType);
	
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
}

void TypeAddMember(const char* pos, const char* varName)
{
	if(!currType)
		ThrowError("ERROR: auto cannot be used for class members", pos);
	newType->AddMemberVariable(varName, currType);

	AddVariable(pos, InplaceStr(varName, (int)strlen(varName)));
}

void TypeFinish()
{
	if(newType->size % 4 != 0)
	{
		newType->paddingBytes = 4 - (newType->size % 4);
		newType->size += 4 - (newType->size % 4);
	}

	nodeList.push_back(new NodeZeroOP());
	for(TypeInfo::MemberFunction *curr = newType->firstFunction; curr; curr = curr->next)
		AddTwoExpressionNode();

	newType = NULL;

	while(varInfo.size() > varInfoTop.back().activeVarCnt)
		varInfo.pop_back();
	varTop = varInfoTop.back().varStackSize;
	varInfoTop.pop_back();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AddUnfixedArraySize()
{
	nodeList.push_back(new NodeNumber<int>(1, typeVoid));
}

// ��� ������� ����������, ����� ��������� ������ ���� � ����, ������� ��� �����������
void SetStringToLastNode(const char* pos, const char* end)
{
	nodeList.back()->SetCodeInfo(pos, end);
}
struct StringIndex
{
	StringIndex(){}
	StringIndex(const char *s, const char *e)
	{
		indexS = s;
		indexE = e;
	}
	const char *indexS, *indexE;
};

FastVector<StringIndex> sIndexes(16);
void SaveStringIndex(const char *s, const char *e)
{
	assert(e > s);
	sIndexes.push_back(StringIndex(s, e));
}
void SetStringFromIndex()
{
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
