#include "stdafx.h"

#include "SupSpi.h"
using namespace supspi;

#include "CodeInfo.h"
using namespace CodeInfo;

#include "Compiler.h"

//////////////////////////////////////////////////////////////////////////
//						Code gen ops
//////////////////////////////////////////////////////////////////////////
// ���������� � �������� ����� ����������. ��� ���������� �� ������ ��� ����, �����
// ������� ���������� � ����������, ����� ��� ������� �� ������� ���������
std::vector<VarTopInfo>		varInfoTop;
// ��������� ����������� ��������� �������� break, ������� ������ �����, �� ������� �������� ���� �����
// ����������, ����� �������� � � �� ���������, � ������� ��� ���������� ��, ���� �� �����������
// ����������� ��� ���������������� ������. ���� ���� (����������� ����� ���� ����������) ������ ������
// varInfoTop.
std::vector<UINT>			undComandIndex;

// ������� ��������������� ������� �����
TypeInfo*	typeVoid = NULL;
TypeInfo*	typeChar = NULL;
TypeInfo*	typeShort = NULL;
TypeInfo*	typeInt = NULL;
TypeInfo*	typeFloat = NULL;
TypeInfo*	typeLong = NULL;
TypeInfo*	typeDouble = NULL;

// Log stream
ostringstream	compileLog;

const char* lastKnownStartPos = NULL;

// Temp variables
// ��������� ����������:
// ���������� ������� ����� ����������, ������� ����� ����������
UINT negCount, varTop;

// ������������ ���������� ��� ������ � ������
UINT currAlign;

// ���� �� ���������� ���������� (��� addVarSetNode)
bool varDefined;

// �������� �� ������� ���������� �����������
bool currValConst;

// ����� ������� ���������� - ������������ �������
UINT inplaceArrayNum;

// ���������� � ���� ������� ����������
TypeInfo*	currType = NULL;
// ���� ( :) )����� ����������
// ��� ����������� arr[arr[i.a.b].y].x;
std::vector<TypeInfo*>	currTypes;
std::vector<bool>		valueByRef;

// ������ ��������� �����
std::vector<std::string>	strs;
// ���� � ����������� ���������� ���������� �������.
// ���� ��� �������� ����� foo(1, bar(2, 3, 4), 5), ����� ����� ��������� ���������� ����������,
// ��� ������ �� �������� ��� �������� ���������� ���������� ���������� � ���, ������� ��������� �������
std::vector<UINT>			callArgCount;
// ����, ������� ������ ���� ��������, ������� ���������� �������.
// ������� ����� ���������� ���� � ������ (BUG: 0004 �����, ����� �� ����?)
std::vector<TypeInfo*>		retTypeStack;

//////////////////////////////////////////////////////////////////////////
// ������� ���������� ��� - ��������� �� ��������
TypeInfo* GetReferenceType(TypeInfo* type)
{
	compileLog << "GetReferenceType(" << type->GetTypeName() << ")\r\n";
	// ������ ������ ��� � ������
	UINT targetRefLevel = type->refLevel+1;
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(type == typeInfo[i]->subType && type->name == typeInfo[i]->name && targetRefLevel == typeInfo[i]->refLevel)
		{
			compileLog << "  returns " << typeInfo[i]->GetTypeName() << "\r\n";
			return typeInfo[i];
		}
	}
	// �������� ����� ���
	TypeInfo* newInfo = new TypeInfo();
	newInfo->name = type->name;
	newInfo->size = 4;
	newInfo->type = TypeInfo::TYPE_INT;
	newInfo->refLevel = type->refLevel + 1;
	newInfo->subType = type;

	typeInfo.push_back(newInfo);
	compileLog << "  returns " << newInfo->GetTypeName() << "\r\n";
	return newInfo;
}

// ������� ���������� ���, ���������� ��� ������������� ���������
TypeInfo* GetDereferenceType(TypeInfo* type)
{
	compileLog << "GetDereferenceType(" << type->GetTypeName() << ")\r\n";
	if(!type->subType || type->refLevel == 0)
		throw CompilerError(std::string("Cannot dereference type ") + type->GetTypeName() + std::string(" there is no result type available"), lastKnownStartPos);
	compileLog << "  returns " << type->subType->GetTypeName() << "\r\n";
	return type->subType;
}

// ������� ���������� ��� - ������ �������� ����� (���-�� ��������� � varSize)
TypeInfo* GetArrayType(TypeInfo* type, UINT sizeInArgument = 0)
{
	int arrSize = -1;
	bool unFixed = false;
	if(sizeInArgument == 0)
	{
		// � ��������� ���� ������ ���������� ����������� �����
		if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
		{
			TypeInfo *aType = (*(nodeList.end()-1))->GetTypeInfo();
			NodeZeroOP* zOP = (nodeList.end()-1)->get();
			if(aType == typeDouble)
			{
				arrSize = (int)static_cast<NodeNumber<double>* >(zOP)->GetVal();
			}else if(aType == typeFloat){
				arrSize = (int)static_cast<NodeNumber<float>* >(zOP)->GetVal();
			}else if(aType == typeLong){
				arrSize = (int)static_cast<NodeNumber<long long>* >(zOP)->GetVal();
			}else if(aType == typeInt){
				arrSize = static_cast<NodeNumber<int>* >(zOP)->GetVal();
			}else if(aType == typeVoid){
				arrSize = -1;
				unFixed = true;
			}else{
				throw CompilerError(std::string("ERROR: unknown type of constant number node ") + aType->name, lastKnownStartPos);
			}
			nodeList.pop_back();
		}else{
			throw CompilerError("ERROR: Array size must be a constant expression", lastKnownStartPos);
		}
	}else{
		arrSize = sizeInArgument;
		if(arrSize == -1)
			unFixed = true;
	}

	if(!unFixed && arrSize < 1)
		throw CompilerError("ERROR: Array size can't be negative or zero", lastKnownStartPos);
	compileLog << "GetArrayType(" << type->GetTypeName() << ", " << arrSize << ")\r\n";
	// ������ ������ ��� � ������
	UINT targetArrLevel = type->arrLevel+1;
	for(UINT i = 0; i < typeInfo.size(); i++)
	{
		if(type == typeInfo[i]->subType && type->name == typeInfo[i]->name && targetArrLevel == typeInfo[i]->arrLevel && typeInfo[i]->arrSize == arrSize)
		{
			compileLog << "  returns " << typeInfo[i]->GetTypeName() << "\r\n";
			return typeInfo[i];
		}
	}
	// �������� ����� ���
	TypeInfo* newInfo = new TypeInfo();
	newInfo->name = type->name;

	if(unFixed)
	{
		newInfo->size = 4;
		newInfo->AddMember("size", typeInt);
	}else{
		newInfo->size = type->size * arrSize;
		if(newInfo->size % 4 != 0)
		{
			newInfo->paddingBytes = 4 - (newInfo->size % 4);
			newInfo->size += 4 - (newInfo->size % 4);
		}
	}

	newInfo->type = TypeInfo::TYPE_COMPLEX;
	newInfo->arrLevel = type->arrLevel + 1;
	newInfo->arrSize = arrSize;
	newInfo->subType = type;

	typeInfo.push_back(newInfo);
	compileLog << "  returns " << newInfo->GetTypeName() << "\r\n";
	return newInfo;
}

// ������� ���������� ��� �������� �������
TypeInfo* GetArrayElementType(TypeInfo* type)
{
	compileLog << "GetArrayElementType(" << type->GetTypeName() << ")\r\n";
	if(!type->subType || type->arrLevel == 0)
		throw CompilerError(std::string("Cannot return array element type, ") + type->GetTypeName() + std::string(" is not an array"), lastKnownStartPos);
	compileLog << "  returns " << type->subType->GetTypeName() << "\r\n";
	return type->subType;
}

bool currValueByRef = false;
void pushValueByRef(char const*s, char const*e){ valueByRef.push_back(currValueByRef); }
void popValueByRef(char const*s, char const*e){ valueByRef.pop_back(); }

// ������������� ������ � ����� ���� long long
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

// ���������, �������� �� ������������� ����������������� ��� ��� �������
void checkIfDeclared(const std::string& str)
{
	if(str == "if" || str == "else" || str == "for" || str == "while" || str == "var" || str == "func" || str == "return" || str=="switch" || str=="case")
		throw CompilerError(std::string("ERROR: The name '" + str + "' is reserved"), lastKnownStartPos);
	for(UINT i = 0; i < funcs.size(); i++)
		if(funcs[i]->name == str)
			throw CompilerError(std::string("ERROR: Name '" + str + "' is already taken for a function"), lastKnownStartPos);
}

// ���������� � ������ ����� {}, ����� ��������� ���������� ����������� ����������, � �������� �����
// ����� �������� ����� ��������� �����.
void blockBegin(char const* s, char const* e)
{
	varInfoTop.push_back(VarTopInfo((UINT)varInfo.size(), varTop));
}
// ���������� � ����� ����� {}, ����� ������ ���������� � ���������� ������ �����, ��� ����� �����������
// �� ����� �� ������� ���������. ����� ��������� ������� ����� ���������� � ������.
void blockEnd(char const* s, char const* e)
{
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{ 
		varTop -= varInfo.back().varType->size;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}

// ������� ��� ���������� ����� � ������������ ������� ������ �����
template<typename T>
void addNumberNode(char const*s, char const*e);

template<> void addNumberNode<char>(char const*s, char const*e)
{
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
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(atoi(s), typeInt)));
}
template<> void addNumberNode<float>(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<float>((float)atof(s), typeFloat)));
}
template<> void addNumberNode<long long>(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<long long>(atoll(s), typeLong)));
}
template<> void addNumberNode<double>(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<double>(atof(s), typeDouble)));
}

void addVoidNode(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP));
}

// ������� ��� �������� ����, ������� ����� ������ � ����
// ������������ NodeExpressionList, ��� �� �������� ����� ������� � �������� ���������
// �� ���� �� ���� ������ ��������� ����� � ����������� ���������� ������.
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
			if(*curr == '\\')
				cleanBuf[len] = '\\';
		}
	}

	curr = cleanBuf;
	end = cleanBuf+len;

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

// ������� ��� �������� ����, ������� ����� �������� �� ����� ����������
// ���� ������ � ���� ��������� ���� � ������.
void addPopNode(char const* s, char const* e)
{
	// ���� ��������� ���� � ������ - ���� � ������, ����� ���
	if((*(nodeList.end()-1))->GetNodeType() == typeNodeNumber)
	{
		nodeList.pop_back();
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
	}else if((*(nodeList.end()-1))->GetNodeType() == typeNodePreValOp){
		// ���� ��������� ����, ��� ����������, ������� ��������� ��� ����������� �� 1, �� ��������� �
		// ��������� � ��������, �� ����� ���������� ����������� ����.
		static_cast<NodePreValOp*>(nodeList.back().get())->SetOptimised(true);
	}else{
		// ����� ������ �������� ���, ��� � ����������� � ������
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePopOp()));
	}
}

// ������� ��� �������� ����, ������� �������� ���� �������� � �����
// ���� ������ � ���� ��������� ���� � ������.
void addNegNode(char const* s, char const* e)
{
	// ���� ���������� ���� ����� ������, �� �������������� ���� �� ����������
	// � ��� �� ����� ������� �����
	if(negCount % 2 == 0)
		return;
	// ���� ��������� ���� ��� �����, �� ������ �������� ���� � ���������
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
			throw CompilerError(std::string("addNegNode() ERROR: unknown type ") + aType->name, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// ����� ������ �������� ���, ��� � ����������� � ������
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeUnaryOp(cmdNeg)));
	}
	// ������� �������� ���� ����� �� 0
	negCount = 0;
}

// ������� ��� �������� ����, ������� ��������� ���������� ��������� ��� ��������� � �����
// ���� ������ � ���� ��������� ���� � ������.
void addLogNotNode(char const* s, char const* e)
{
	// ���� ��������� ���� � ������ - �����, �� ��������� �������� �� ����� ���������
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
			throw CompilerError(std::string("addLogNotNode() ERROR: unknown type ") + aType->name, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// ����� ������ �������� ���, ��� � ����������� � ������
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
			throw CompilerError("ERROR: bitwise NOT cannot be used on floating point numbers", s);
		}else if(aType == typeFloat){
			throw CompilerError("ERROR: bitwise NOT cannot be used on floating point numbers", s);
		}else if(aType == typeLong){
			Rd.reset(new NodeNumber<long long>(static_cast<NodeNumber<long long>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo()));
		}else if(aType == typeInt){
			Rd.reset(new NodeNumber<int>(static_cast<NodeNumber<int>* >(zOP)->GetBitNotVal(), zOP->GetTypeInfo()));
		}else{
			throw CompilerError(std::string("addBitNotNode() ERROR: unknown type ") + aType->name, s);
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
		return;	// ����������� �������, �������
	}
	if(aNodeType == typeNodeGetAddress && bNodeType == typeNodeNumber && (*(nodeList.end()-1))->GetTypeInfo() == typeInt)
	{
		NodeGetAddress *addrNode = static_cast<NodeGetAddress*>((nodeList.end()-2)->get());
		NodeNumber<int> *numNode = static_cast<NodeNumber<int>* >((nodeList.end()-1)->get());
		addrNode->SetAddress(addrNode->GetAddress()+numNode->GetVal());
		nodeList.pop_back();
		return;
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
		if(bNodeType != typeNodeTwoAndCmdOp && bNodeType != typeNodeVarGet)
		{
			// �����, ������� ��� �����������
			try
			{
				nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoAndCmdOp(id)));
			}catch(const std::string& str){
				throw CompilerError(str.c_str(), lastKnownStartPos);
			}
			return;
		}

		// ����������� ����� ����������, ���� ����� == 0 ��� ����� == 1
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
		if(success)	// ����������� �������, ������� �����
			return;
	}
	// ����������� �� �������, ������� �������� ���������
	try
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeTwoAndCmdOp(id)));
	}catch(const std::string& str){
		throw CompilerError(str.c_str(), lastKnownStartPos);
	}
}

template<CmdID cmd> void createTwoAndCmd(char const* s, char const* e)
{
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
	return &createTwoAndCmd<cmdReturn>;
}

void addReturnNode(char const* s, char const* e)
{
	int t = (int)varInfoTop.size();
	int c = 0;
	if(funcs.size() != 0)
	{
		while(t > (int)funcs.back()->vTopSize)
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
	if(undComandIndex.empty())
		throw CompilerError("ERROR: break used outside loop statements", s);
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

void addVar(char const* s, char const* e)
{
	lastKnownStartPos = s;
	string vName = *(strs.end()-2);

	for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		if(varInfo[i].name == vName)
			throw CompilerError("ERROR: Name '" + vName + "' is already taken for a variable in current scope\r\n", s);
	checkIfDeclared(vName);

	if(currType && currType->size == -1)
		throw CompilerError("ERROR: variable '" + vName + "' can't be an unfixed size array", s);
	if(currType && currType->size > 64*1024*1024)
		throw CompilerError("ERROR: variable '" + vName + "' has to big length (>64 Mb)", s);
	
	if((currType && currType->alignBytes != 0) || currAlign != -1)
	{
		UINT activeAlign = currAlign != -1 ? currAlign : currType->alignBytes;
		if(activeAlign > 16)
			throw CompilerError("ERROR: alignment must me less than 16 bytes", s);
		if(activeAlign != 0 && varTop % activeAlign != 0)
		{
			UINT offset = activeAlign - (varTop % activeAlign);
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarDef(offset, "offset")));
			addTwoExprNode(0,0);
			varTop += offset;
		}
	}
	varInfo.push_back(VariableInfo(vName, varTop, currType, currValConst));
	varDefined = true;
	if(currType)
		varTop += currType->size;
}

void addVarDefNode(char const* s, char const* e)
{
	assert(varDefined);
	if(!currType)
		throw CompilerError("ERROR: auto variable must be initialized in place of definition", s);
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarDef(currType->size, strs.back())));
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
	pushedShiftAddr = false;
	pushedShiftAddrNode = false;
}

void getType(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = strs.back();
	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw CompilerError("ERROR: variable '" + strs.back() + "' is not defined [set]", s);
	currTypes.push_back(varInfo[i].varType);
	pushedShiftAddr = false;
	pushedShiftAddrNode = false;
}

void addDereference(char const* s, char const* e);
void addSetNode(char const* s, char const* e);
void addGetNode(char const* s, char const* e);
void getAddress(char const* s, char const* e);

void addInplaceArray(char const* s, char const* e)
{
	char asString[16];
	strs.push_back("$carr");
	strs.back() += _itoa(inplaceArrayNum++, asString, 10);
	strs.push_back(strs.back());
	TypeInfo *saveCurrType = currType;
	bool saveVarDefined = varDefined;
	currType = NULL;
	addVar(s, e);
	currTypes.push_back(NULL);
	valueByRef.push_back(false);
	addSetNode(s, e);
	addPopNode(s, e);
	currTypes.push_back(varInfo.back().varType);
	valueByRef.push_back(false);
	addGetNode(s, e);

	varDefined = saveVarDefined;
	currType = saveCurrType;
	strs.pop_back();
	strs.pop_back();
}

void getMember(char const* s, char const* e)
{
	string vName = std::string(s, e);

	if(currTypes.back()->refLevel != 0)
	{
		currValueByRef = true;
		addDereference(0,0);
		addGetNode(0,0);
	}

	// ��, ��� ��������� ���������� � ������, ��� � ����������!
	TypeInfo *currType = currTypes.back();

	int i = (int)currType->memberData.size()-1;
	while(i >= 0 && currType->memberData[i].name != vName)
		i--;
	if(i == -1)
		throw CompilerError("ERROR: variable '" + vName + "' is not a member of '" + currType->GetTypeName() + "' [set]", s);
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(currType->memberData[i].offset, typeInt)));
	if(pushedShiftAddrNode | pushedShiftAddr)
		addTwoAndCmpNode(cmdAdd);
	pushedShiftAddrNode = false;
	pushedShiftAddr = true;
	if(currTypes.back()->arrSize == -1)
		currTypes.back() = typeVoid;
	else
		currTypes.back() = currType->memberData[i].type;
}

void getAddress(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = strs.back();

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw CompilerError("ERROR: variable '" + strs.back() + "' is not defined [getaddr]", s);

	if(((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0)
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(varInfo[i].pos, typeInt)));
	else
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeGetAddress(varInfo[i], varInfo[i].pos-(int)(varInfoTop.back().varStackSize))));

	pushedShiftAddrNode = false;
	pushedShiftAddr = true;
}

void addAddressNode(char const* s, char const* e)
{
	if(nodeList.back()->GetNodeType() != typeNodeNumber && nodeList.back()->GetNodeType() != typeNodeTwoAndCmdOp && nodeList.back()->GetNodeType() != typeNodeGetAddress)
		throw CompilerError("ERROR: addAddressNode() can't find a \r\n  number node on the top of node list", s);
	if(nodeList.back()->GetTypeInfo() != typeInt)
		throw CompilerError("ERROR: addAddressNode(): number node type is not int", s);

	shared_ptr<NodeZeroOP> temp = nodeList.back();
	if(nodeList.back()->GetNodeType() == typeNodeNumber)
		nodeList.back().reset(new NodeNumber<int>(static_cast<NodeNumber<int>*>(temp.get())->GetVal(), GetReferenceType(currTypes.back())));
	else
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpression(GetReferenceType(currTypes.back()))));
	currTypes.pop_back();
	valueByRef.push_back(false);
}

void convertTypeToRef(char const* s, char const* e)
{
	lastKnownStartPos = s;
	if(!currType)
		throw CompilerError("ERROR: auto variable cannot have reference flag", s);
	currType = GetReferenceType(currType);
}

void convertTypeToArray(char const* s, char const* e)
{
	lastKnownStartPos = s;
	if(!currType)
		throw CompilerError("ERROR: cannot specify array size for auto variable", s);
	currType = GetArrayType(currType);
}

void addDereference(char const* s, char const* e)
{
	lastKnownStartPos = s;
	if(currTypes.back()->refLevel == 0)
		throw CompilerError("ERROR: cannot dereference " + *(strs.end()-2), s);
	currTypes.push_back(currTypes.back());
	currTypes[currTypes.size()-2] = GetDereferenceType(currTypes.back());
	pushedShiftAddr = true;
	valueByRef.push_back(true);
	valueByRef.push_back(false);
}

void addShiftAddrNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	currTypes.back() = GetArrayElementType(currTypes.back());
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
			throw CompilerError("addBitNotNode() ERROR: unknown type " + aType->name, s);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePushShift(currTypes.back()->size)));
	}

	if(pushedShiftAddrNode)
	{
		addTwoAndCmpNode(cmdAdd);
	}else{
		pushedShiftAddrNode = true;
	}
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
		throw CompilerError("ERROR: variable '" + vName + "' is not defined [set]", s);
	if(!currValConst && varInfo[i].isConst)
		throw CompilerError("ERROR: cannot change constant parameter '" + strs.back() + "' ", s);

	TypeInfo *realCurrType = currTypes.back() ? currTypes.back() : nodeList.back()->GetTypeInfo();

	bool unifyTwo = false;
	if(realCurrType->arrSize == -1)
	{
		TypeInfo *nodeType = nodeList.back()->GetTypeInfo();
		if(realCurrType->subType == nodeType->subType)
		{
			if(nodeList.back()->GetNodeType() != typeNodeVarGet)
			{
				if(nodeList.back()->GetNodeType() == typeNodeExpressionList)
				{
					addInplaceArray(s, e);
					unifyTwo = true;
				}else{
					throw CompilerError("ERROR: cannot convert from " + nodeList.back()->GetTypeInfo()->GetTypeName() + " to " + realCurrType->GetTypeName(), s);
				}
			}
			strs.push_back(static_cast<NodeVarGet*>(nodeList.back().get())->GetVarName());
			UINT typeSize = (nodeType->size - nodeType->paddingBytes) / nodeType->subType->size;
			nodeList.pop_back();
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(typeSize, typeInt)));
			getAddress(0,0);
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(varInfo[i].varType)));
			shared_ptr<NodeZeroOP> temp = nodeList.back();
			nodeList.pop_back();
			NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp.get());
			arrayList->AddNode();
			nodeList.push_back(temp);
			strs.pop_back();
		}
	}

	UINT varSizeAdd = 0;
	if(!currTypes.back())
	{
		if(realCurrType->alignBytes != 0 || currAlign != -1)
		{
			UINT activeAlign = currAlign != -1 ? currAlign : realCurrType->alignBytes;
			if(activeAlign > 16)
				throw CompilerError("ERROR: alignment must me less than 16 bytes", s);
			if(activeAlign != 0 && varTop % activeAlign != 0)
			{
				UINT offset = activeAlign - (varTop % activeAlign);
				varSizeAdd += offset;
				varInfo[i].pos += offset;
				varTop += offset;
			}
		}
		varInfo[i].varType = realCurrType;
		varTop += realCurrType->size;
	}
	varSizeAdd += varDefined ? realCurrType->size : 0;

	try
	{
		if(!valueByRef.empty() && valueByRef.back())
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSet(varInfo[i], realCurrType, 0, true, true, varSizeAdd)));
		else
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSet(varInfo[i], realCurrType, varInfo[i].pos-varInfoTop.back().varStackSize, braceInd != -1 || compoundType != -1, false, varSizeAdd)));
	}catch(const std::string& str){
		throw CompilerError(str.c_str(), s);
	}
	valueByRef.pop_back();
	currTypes.pop_back();

	currValConst = false;
	varDefined = false;

	if(unifyTwo)
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(nodeList.back()->GetTypeInfo())));
		shared_ptr<NodeZeroOP> temp = nodeList.back();
		nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp.get())->AddNode();
		nodeList.push_back(temp);
	}
}

void addGetNode(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	size_t braceInd = strs.back().find('[');
	size_t compoundType = (currTypes.back()->refLevel != 0 ? -1 : strs.back().find('.'));

	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw CompilerError("ERROR: variable '" + vName + "' is not defined [get]", s);

	try
	{
		if(valueByRef.back())
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), 0, true, true)));
		else if(((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0)
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), varInfo[i].pos, braceInd != -1 || compoundType != -1, true)));
		else
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), varInfo[i].pos-(int)(varInfoTop.back().varStackSize), braceInd != -1 || compoundType != -1, false)));
	}catch(const std::string& str){
		throw CompilerError(str.c_str(), s);
	}

	valueByRef.pop_back();
	currTypes.pop_back();

	pushedShiftAddrNode = false;
}

void addGetByRef(char const* s, char const* e)
{
	int i = (int)varInfo.size()-1;
	string vName = *(strs.end()-2);
	while(i >= 0 && varInfo[i].name != vName)
		i--;
	if(i == -1)
		throw CompilerError("ERROR: variable '" + vName + "' is not defined [get]", s);

	try
	{
		nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarGet(varInfo[i], currTypes.back(), 0, true, true)));
	}catch(const std::string& str){
		throw CompilerError(str.c_str(), s);
	}
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
		throw CompilerError("ERROR: variable " + strs.back() + " is not defined", lastKnownStartPos);
	if(!currValConst && varInfo[i].isConst)
		throw CompilerError("ERROR: cannot change constant parameter '" + strs.back() + "' ", lastKnownStartPos);
	if(braceInd == -1 && varInfo[i].varType->arrLevel != 0)
		throw CompilerError("ERROR: variable '" + strs.back() + "' is an array, but no index specified", lastKnownStartPos);

	bool aabsadr = ((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;
	int ashift = aabsadr ? 0 : varInfoTop.back().varStackSize;

	try
	{
		if(valueByRef.back())
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSetAndOp(varInfo[i], currTypes.back(), 0, true, true, cmd)));
		else 
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeVarSetAndOp(varInfo[i], currTypes.back(), varInfo[i].pos-ashift, compoundType != -1, aabsadr, cmd)));
	}catch(const std::string& str){
		throw CompilerError(str.c_str(), lastKnownStartPos);
	}
	valueByRef.pop_back();
	currTypes.pop_back();
}
void addAddSetNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	addSetAndOpNode(cmdAdd);
}
void addSubSetNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	addSetAndOpNode(cmdSub);
}
void addMulSetNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	addSetAndOpNode(cmdMul);
}
void addDivSetNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	addSetAndOpNode(cmdDiv);
}
void addPowSetNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
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
		throw CompilerError("ERROR: variable '" + strs.back() + "' is not defined [set]", lastKnownStartPos);
	if(!currValConst && varInfo[i].isConst)
		throw CompilerError("ERROR: cannot change constant parameter '" + strs.back() + "' ", lastKnownStartPos);
	if(braceInd == -1 && varInfo[i].varType->arrLevel != 0)
		throw CompilerError("ERROR: variable '" + strs.back() + "' is an array, but no index specified", lastKnownStartPos);

	bool aabsadr = ((varInfoTop.size() > 1) && (varInfo[i].pos < varInfoTop[1].varStackSize)) || varInfoTop.back().varStackSize == 0;
	int ashift = aabsadr ? 0 : varInfoTop.back().varStackSize;

	try
	{
		if(valueByRef.back())
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePreValOp(varInfo[i], currTypes.back(), 0, true, true, cmd, pre)));
		else
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodePreValOp(varInfo[i], currTypes.back(), varInfo[i].pos-ashift, compoundType != -1, aabsadr, cmd, pre)));
	}catch(const std::string& str){
		throw CompilerError(str.c_str(), lastKnownStartPos);
	}
	valueByRef.pop_back();
	currTypes.pop_back();
}
void addPreDecNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	addPreOpNode(cmdDecAt, true);
}
void addPreIncNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	addPreOpNode(cmdIncAt, true);
}
void addPostDecNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	addPreOpNode(cmdDecAt, false);
}
void addPostIncNode(char const* s, char const* e)
{
	lastKnownStartPos = s;
	addPreOpNode(cmdIncAt, false);
}

void addOneExprNode(char const* s, char const* e)
{
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
void addBlockNode(char const* s, char const* e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeBlock()));
}

UINT arrElementCount = 0;
void addArrayConstructor(char const* s, char const* e)
{
	arrElementCount++;

	TypeInfo *currType = (*(nodeList.end()-arrElementCount))->GetTypeInfo();

	if(currType == typeShort)
		currType = typeInt;
	//	throw CompilerWarning("WARNING: short will be promoted to int during array construction", s);
	if(currType == typeVoid)
		throw CompilerError("ERROR: array cannot be constructed from void type elements", s);

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(arrElementCount, currType)));
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(GetArrayType(currType))));

	shared_ptr<NodeZeroOP> temp = nodeList.back();
	nodeList.pop_back();

	NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp.get());

	char tempStr[16];
	for(int i = 0; i < arrElementCount; i++)
	{
		if(nodeList.back()->GetTypeInfo() != currType && !(nodeList.back()->GetTypeInfo() == typeShort && currType == typeInt))
			throw CompilerError(std::string("ERROR: element ") + _itoa(arrElementCount-i-1, tempStr, 10) + " doesn't match the type of element 0 (" + currType->GetTypeName() + ")", s);
		arrayList->AddNode(false);
	}

	nodeList.push_back(temp);

	arrElementCount = 0;
}

void funcAdd(char const* s, char const* e)
{
	for(UINT i = varInfoTop.back().activeVarCnt; i < varInfo.size(); i++)
		if(varInfo[i].name == strs.back())
			throw CompilerError("ERROR: Name '" + strs.back() + "' is already taken for a variable in current scope", s);
	std::string name = strs.back();
	if(name == "if" || name == "else" || name == "for" || name == "while" || name == "var" || name == "func" || name == "return" || name=="switch" || name=="case")
		throw CompilerError("ERROR: The name '" + name + "' is reserved", s);
	if(!currType)
		throw CompilerError("ERROR: function return type cannot be auto", s);
	funcs.push_back(new FunctionInfo());
	funcs.back()->name = name;
	funcs.back()->vTopSize = (UINT)varInfoTop.size();
	retTypeStack.push_back(currType);
	funcs.back()->retType = currType;
}
void funcParam(char const* s, char const* e)
{
	if(!currType)
		throw CompilerError("ERROR: function parameter cannot be an auto type", s);
	funcs.back()->params.push_back(VariableInfo(strs.back(), 0, currType, currValConst));
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
		currAlign = 1;
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

	// Find all the functions with the same name
	int count = 0;
	for(int n = 0; n < i; n++)
	{
		if(funcs[n]->name == funcs[i]->name && funcs[n]->params.size() == funcs[i]->params.size())
		{
			// Check all parameter types
			bool paramsEqual = true;
			for(UINT k = 0; k < funcs[i]->params.size(); k++)
			{
				if(funcs[n]->params[k].varType->GetTypeName() != funcs[i]->params[k].varType->GetTypeName())
					paramsEqual = false;
			}
			if(paramsEqual)
				throw CompilerError("ERROR: function '" + funcs[i]->name + "' is being defined with the same set of parameters", s);
		}
	}

	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop -= varInfo.back().varType->size;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncDef(funcs[i])));
	strs.pop_back();
	retTypeStack.pop_back();
}


void addFuncCallNode(char const* s, char const* e)
{
	string fname = strs.back();
	strs.pop_back();

	//Find all functions with given name
	FunctionInfo *fList[32];
	UINT	fRating[32];
	memset(fRating, 0, 32*4);

	int count = 0;
	for(int k = 0; k < (int)funcs.size(); k++)
		if(funcs[k]->name == fname)
			fList[count++] = funcs[k];
	if(count == 0)
		throw CompilerError("ERROR: function '" + fname + "' is undefined", s);
	// Find the best suited function
	UINT minRating = 1024*1024;
	UINT minRatingIndex = -1;
	for(int k = 0; k < count; k++)
	{
		if(fList[k]->params.size() != callArgCount.back())
		{
			fRating[k] += 65000;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.
			continue;
		}
		for(UINT n = 0; n < fList[k]->params.size(); n++)
		{
			TypeInfo *paramType = nodeList[nodeList.size()-fList[k]->params.size()+n]->GetTypeInfo();
			TypeInfo *expectedType = fList[k]->params[n].varType;
			if(expectedType != paramType)
			{
				if(expectedType->arrSize == -1 && paramType->arrSize != 0 && paramType->subType == expectedType->subType)
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
		for(int n = 0; n < count; n++)
		{
			errTemp << "  " << fname << "(";
			for(UINT m = 0; m < fList[n]->params.size(); m++)
				errTemp << fList[n]->params[m].varType->GetTypeName() << (m != fList[n]->params.size()-1 ? ", " : "");
			errTemp << ")\r\n";
		}
		throw errTemp.str();
	}
	// Check, is there are more than one function, that share the same rating
	for(int k = 0; k < count; k++)
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
			for(int n = 0; n < count; n++)
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
	vector<shared_ptr<NodeZeroOP> > paramNodes;
	for(UINT i = 0; i < fList[minRatingIndex]->params.size(); i++)
	{
		paramNodes.push_back(nodeList.back());
		nodeList.pop_back();
	}
	vector<shared_ptr<NodeZeroOP> > inplaceArray;

	for(UINT i = 0; i < fList[minRatingIndex]->params.size(); i++)
	{
		UINT index = (UINT)(fList[minRatingIndex]->params.size()) - i - 1;
		if(fList[minRatingIndex]->params[i].varType->arrSize == -1 && fList[minRatingIndex]->params[i].varType->subType == paramNodes[index]->GetTypeInfo()->subType)
		{
			if(paramNodes[index]->GetNodeType() != typeNodeVarGet)
			{
				if(paramNodes[index]->GetNodeType() == typeNodeExpressionList)
				{
					nodeList.push_back(paramNodes[index]);
					addInplaceArray(s, e);

					paramNodes[index] = nodeList.back();
					nodeList.pop_back();
					inplaceArray.push_back(nodeList.back());
					nodeList.pop_back();
				}else{
					throw CompilerError("ERROR: to the right side of '=' must be a get node", s);
				}
			}
			strs.push_back(static_cast<NodeVarGet*>(paramNodes[index].get())->GetVarName());
			UINT typeSize = (paramNodes[index]->GetTypeInfo()->size - paramNodes[index]->GetTypeInfo()->paddingBytes) / paramNodes[index]->GetTypeInfo()->subType->size;
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(typeSize, typeInt)));
			getAddress(0,0);
			nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeExpressionList(fList[minRatingIndex]->params[i].varType)));
			shared_ptr<NodeZeroOP> temp = nodeList.back();
			nodeList.pop_back();
			NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp.get());
			arrayList->AddNode();
			nodeList.push_back(temp);
			strs.pop_back();
		}else{
			nodeList.push_back(paramNodes[index]);
		}
	}

	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeFuncCall(fList[minRatingIndex])));

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
		throw CompilerError("ERROR: trinary operator ?: \r\n result types are not equal (" + typeB->name + " : " + typeA->name + ")", s);
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
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeSwitchExpr()));
}
void addCaseNode(char const* s, char const* e)
{
	shared_ptr<NodeZeroOP> temp = *(nodeList.end()-3);
	static_cast<NodeSwitchExpr*>(temp.get())->AddCase();
}
void addSwitchNode(char const* s, char const* e)
{
	undComandIndex.pop_back();
	while(varInfo.size() > varInfoTop.back().activeVarCnt)
	{
		varTop--;
		varInfo.pop_back();
	}
	varInfoTop.pop_back();
}

TypeInfo *newType = NULL;
void beginType(char const* s, char const* e)
{
	if(newType)
		throw CompilerError("ERROR: Different type is being defined", s);
	if(currAlign < 0)
		throw CompilerError("ERROR: alignment must be a positive number", s);
	if(currAlign > 16)
		throw CompilerError("ERROR: alignment must me less than 16 bytes", s);
	newType = new TypeInfo();
	newType->name = std::string(s, e);
	newType->type = TypeInfo::TYPE_COMPLEX;
	newType->alignBytes = currAlign;
	currAlign = -1;
}

void addMember(char const* s, char const* e)
{
	if(!currType)
		throw CompilerError("ERROR: auto cannot be used for class members", s);
	newType->AddMember(std::string(s, e), currType);
}

void addType(char const* s, char const* e)
{
	if(newType->size % 4 != 0)
	{
		newType->paddingBytes = 4 - (newType->size % 4);
		newType->size += 4 - (newType->size % 4);
	}
	typeInfo.push_back(newType);
	newType = NULL;
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeZeroOP()));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void addUnfixedArraySize(char const*s, char const*e)
{
	nodeList.push_back(shared_ptr<NodeZeroOP>(new NodeNumber<int>(1, typeVoid)));
}

namespace CompilerGrammar
{
	// �������, �������� � ��������� ������ �� ����� �����
	// ���� ����� ����� �������������� ��� �������� ��������� �������� ����� ������� ����������
	// �������� ��� ������� ������������� ���������� a[i], ����� ��������� "a" � ����,
	// ������ ��� � ������� ��������� "a[i]" �������
	void ParseStrPush(char const *s, char const *e){ strs.push_back(string(s,e)); }
	void ParseStrPop(char const *s, char const *e){ strs.pop_back(); }
	void ParseStrCopy(char const *s, char const *e){ strs.push_back(*(strs.end()-2)); }
	void ParseStrAdd(char const *s, char const *e){ strs.back() += std::string(s, e); }

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
		sIndexes.push_back(StringIndex(s, e));
	}
	void SetStringFromIndex(char const *s, char const *e)
	{
		nodeList.back()->SetCodeInfo(sIndexes.back().indexS, sIndexes.back().indexE);
		sIndexes.pop_back();
	}

	// Callbacks
	typedef void (*parserCallback)(char const*, char const*);
	parserCallback addChar, addInt, addFloat, addLong, addDouble;
	parserCallback strPush, strPop, strCopy;

	// Parser rules
	Rule group, term5, term4_9, term4_8, term4_85, term4_7, term4_75, term4_6, term4_65, term4_4, term4_2, term4_1, term4, term3, term2, term1, expression;
	Rule varname, funccall, funcdef, funcvars, block, vardef, vardefsub, applyval, applyref, ifexpr, whileexpr, forexpr, retexpr;
	Rule doexpr, breakexpr, switchexpr, isconst, addvarp, seltype, arrayDef;
	Rule classdef;
	Rule funcProt;	// user function prototype

	Rule code, mySpaceP;

	class ThrowError
	{
	public:
		ThrowError(): err(NULL){ }
		ThrowError(const char* str): err(str){ }

		void operator() (char const* s, char const* e)
		{
			//ASSERT(err);
			throw CompilerError(err, s);
		}
	private:
		const char* err;
	};
	class TypeNameP: public BaseP
	{
	public:
		TypeNameP(Rule a){ m_a.set(a); }
		virtual ~TypeNameP(){}

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
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
	Rule	typenameP(Rule a){ return Rule(shared_ptr<BaseP>(new TypeNameP(a))); }

	void InitGrammar()
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
		seltype		=	(strP("auto") | typenameP(varname))[selType] >> *((lexemeD[strP("ref") >> (~alnumP | nothingP)])[convertTypeToRef] | arrayDef);

		isconst		=	epsP[AssignVar<bool>(currValConst, false)] >> !strP("const")[AssignVar<bool>(currValConst, true)];
		varname		=	lexemeD[alphaP >> *alnumP];

		classdef	=	((strP("align") >> '(' >> intP[StrToInt(currAlign)] >> ')') | (strP("noalign") | epsP)[AssignVar<UINT>(currAlign, 0)]) >>
						strP("class") >> varname[beginType] >> chP('{') >>
						*(seltype >> varname[addMember] >> *(',' >> varname[addMember]) >> chP(';'))
						>> chP('}')[addType];

		funccall	=	varname[strPush] >> 
				('(' | (epsP[strPop] >> nothingP)) >>
				epsP[PushBackVal<std::vector<UINT>, UINT>(callArgCount, 0)] >> 
				!(
				term5[ArrBackInc<std::vector<UINT> >(callArgCount)] >>
				*(',' >> term5[ArrBackInc<std::vector<UINT> >(callArgCount)])
				) >>
				(')' | epsP[ThrowError("ERROR: ')' not found after function call")]);

		funcvars	=	!(isconst >> seltype >> varname[strPush][funcParam]) >> *(',' >> isconst >> seltype >> varname[strPush][funcParam]);
		funcdef		=	seltype >> varname[strPush] >> (chP('(')[funcAdd] | (epsP[strPop] >> nothingP)) >>  funcvars[funcStart] >> chP(')') >> chP('{') >> code[funcEnd] >> chP('}');
		funcProt	=	seltype >> varname[strPush] >> (chP('(')[funcAdd] | (epsP[strPop] >> nothingP)) >>  funcvars >> chP(')') >> chP(';');

		applyval	=
			(
				(varname - strP("case"))[strPush] >> (~chP('(') | (epsP[strPop] >> nothingP)) >> epsP[getType] >>
				!((chP('[')[strPush] | (epsP[strPush] >> nothingP)) >> term5 >> ']')[addShiftAddrNode] >>
				*('[' >> term5 >> ']')[addShiftAddrNode] >>
				*(
					chP('.')[ParseStrAdd] >>
					(varname - strP("case"))[getMember] >>
					!('[' >> term5 >> ']')[addShiftAddrNode][addCmd(cmdAdd)]				
				)
			);
		applyref	=
			(
				varname[strPush][getType][getAddress] >>
				!('[' >> term5 >> ']')[addShiftAddrNode][addCmd(cmdAdd)] >>
				*(
					'.' >>
					varname[getMember] >>
					!('[' >> term5 >> ']')[addShiftAddrNode][addCmd(cmdAdd)]
				)
			);

		addvarp		=
			(
				varname[strPush] >>
				!('[' >> (term4_9 | epsP[addUnfixedArraySize]) >> ']')[convertTypeToArray]
			)[pushType][strPush][addVar] >>
			(('=' >> term5)[AssignVar<bool>(currValueByRef, false)][pushValueByRef][addSetNode][addPopNode] | epsP[addVarDefNode][popType])[strPop][strPop];
		
		vardefsub	= addvarp[SetStringToLastNode] >> *(',' >> vardefsub)[addTwoExprNode];
		vardef		=
			epsP[AssignVar<UINT>(currAlign, -1)] >>
			isconst >>
			!(strP("noalign")[AssignVar<UINT>(currAlign, 0)] | (strP("align") >> '(' >> intP[StrToInt(currAlign)] >> ')')) >>
			seltype >>
			vardefsub;

		ifexpr		=	(strP("if") >> ('(' >> term5 >> ')'))[SaveStringIndex] >> expression >> ((strP("else") >> expression)[addIfElseNode] | epsP[addIfNode])[SetStringFromIndex];
		forexpr		=	(strP("for")[saveVarTop] >> '(' >> (vardef | term5[addPopNode] | block) >> ';' >> term5 >> ';' >> (term5[addPopNode] | block) >> ')')[SaveStringIndex] >> expression[addForNode][SetStringFromIndex];
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
			strP("switch") >>
			('(' | epsP[ThrowError("ERROR: '(' not found after 'switch'")]) >>
			(term5 | epsP[ThrowError("ERROR: expression not found after 'switch('")])[preSwitchNode] >>
			(')' | epsP[ThrowError("ERROR: closing ')' not found after expression in 'switch' statement")]) >>
			('{' | epsP[ThrowError("ERROR: '{' not found after 'switch(...)'")]) >>
			(strP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode] >>
			*(strP("case") >> term5 >> ':' >> expression >> *expression[addTwoExprNode])[addCaseNode] >>
			('}' | epsP[ThrowError("ERROR: '}' not found after 'switch' statement")])[addSwitchNode];

		retexpr		=	(strP("return") >> (term5 | epsP[addVoidNode]) >> +chP(';'))[addReturnNode];
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
			(chP('\"') >> *(anycharP - chP('\"')) >> chP('\"'))[strPush][addStringNode] |
			longestD[((intP >> chP('l'))[addLong] | (intP[addInt])) | ((realP >> chP('f'))[addFloat] | (realP[addDouble]))] |
			(chP('\'') >> ((chP('\\') >> anycharP) | anycharP) >> chP('\''))[addChar] |
			(chP('{') >> term5 >> *(chP(',') >> term5[IncVar<UINT>(arrElementCount)]) >> chP('}'))[addArrayConstructor] |
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
		expression	=	*chP(';') >> (classdef | (vardef >> +chP(';')) | breakexpr | ifexpr | forexpr | whileexpr | doexpr | switchexpr | retexpr | (term5 >> (+chP(';')  | epsP[ThrowError("ERROR: ';' not found after expression")]))[addPopNode] | block[addBlockNode]);
		code		=	((funcdef | expression) >> (code[addTwoExprNode] | epsP[addOneExprNode]));
	
		mySpaceP = spaceP | ((strP("//") >> *(anycharP - eolP)) | (strP("/*") >> *(anycharP - strP("*/")) >> strP("*/")));
	}
};

UINT buildInFuncs;
UINT buildInTypes;

CompilerError::CompilerError(std::string& errStr, const char* apprPos)
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
		begin++;

		const char *end = apprPos;
		while((*end != '\r') && (*end != '\n') && (*end != 0))
			end++;
		len = (UINT)(end - begin) < 128 ? (UINT)(end - begin) : 127;
		memcpy(line, begin, len);
		line[len] = 0;
	}else{
		line[0] = 0;
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
	TypeInfo *typeFile = info;
	typeInfo.push_back(info);

	buildInTypes = (int)typeInfo.size();

	// Add functions
	FunctionInfo	*fInfo;
	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "cos";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	funcs.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "sin";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	funcs.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "tan";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	funcs.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "ctg";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	funcs.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "ceil";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	funcs.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "floor";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	funcs.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "sqrt";
	fInfo->params.push_back(VariableInfo("deg", 0, typeDouble));
	fInfo->retType = typeDouble;
	fInfo->vTopSize = 1;
	funcs.push_back(fInfo);

	fInfo = new FunctionInfo();
	fInfo->address = -1;
	fInfo->name = "clock";
	fInfo->retType = typeInt;
	fInfo->vTopSize = 1;
	funcs.push_back(fInfo);

	buildInFuncs = (int)funcs.size();

	CompilerGrammar::InitGrammar();

}
Compiler::~Compiler()
{
}

void Compiler::ClearState()
{
	varInfoTop.clear();
	varInfo.clear();
	typeInfo.resize(buildInTypes);
	funcs.resize(buildInFuncs);

	callArgCount.clear();
	retTypeStack.clear();

	currTypes.clear();
	valueByRef.clear();

	nodeList.clear();

	varDefined = 0;
	negCount = 0;
	varTop = 24;
	newType = NULL;

	currAlign = -1;
	inplaceArrayNum = 1;

	varInfo.push_back(VariableInfo("ERROR", 0, typeDouble, true));
	varInfo.push_back(VariableInfo("pi", 8, typeDouble, true));
	varInfo.push_back(VariableInfo("e", 16, typeDouble, true));
	varInfoTop.push_back(VarTopInfo(0,0));

	retTypeStack.push_back(NULL);	//global return can return anything

	logAST.str("");
	compileLog.str("");
}

bool Compiler::AddExternalFunction(void (_cdecl *ptr)(), const char* prototype)
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
	if(pRes = PARSE_FAILED)
		return false;

	funcs.back()->address = -1;
	funcs.back()->funcPtr = ptr;

	strs.pop_back();
	retTypeStack.pop_back();
	buildInFuncs++;
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

	UINT t = GetTickCount();
	ParseResult pRes = Parse(CompilerGrammar::code, ptr, CompilerGrammar::mySpaceP);
	if(pRes == PARSE_NOTFULL)
		throw std::string("Parsing wasn't full");
	if(pRes = PARSE_FAILED)
		throw std::string("Parsing failed");
	UINT tem = GetTickCount()-t;
	m_TempStream << "Parsing and AST tree gen. time: " << tem << "ms\r\n";
	
	t = GetTickCount();
	if(nodeList.back())
		nodeList.back()->Compile();
	tem = GetTickCount()-t;
	m_TempStream << "Compile time: " << tem << "ms\r\n";

	m_TempStream.flush();
	m_TempStream.close();

	ostringstream		graphlog;
	ofstream graphFile("graph.txt", std::ios::binary);
	if(nodeList.back())
		nodeList.back()->LogToStream(graphlog);
	graphFile << graphlog.str();
	graphFile.close();

	compileLog << "\r\nActive types (" << typeInfo.size() << "):\r\n";
	for(UINT i = 0; i < typeInfo.size(); i++)
		compileLog << typeInfo[i]->GetTypeName() << "\r\n";
	logAST << "\r\n" << compileLog.str();

	if(nodeList.size() != 1)
		throw std::string("Compilation failed, AST contains more than one node");

	return true; // ����� ��� return true, ���� ������ return false ������������ ����������?
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
			cmdList->GetData(pos, funcInfo);
			pos += sizeof(FunctionInfo*);
			logASM << dec << showbase << pos2 << " CALLS " << funcInfo->name << ";";
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
			cmdList->GetUSHORT(pos, shVal1);
			pos += 2;
			logASM << dec << showbase << pos2 << " CALL " << valind;
			if(shVal1 & bitRetSimple)
				logASM << " simple";
			logASM << " size:" << (shVal1&0x0FFF) << ";";
			break;
		case cmdReturn:
			cmdList->GetUSHORT(pos, shVal1);
			pos += 2;
			cmdList->GetUSHORT(pos, shVal2);
			pos += 2;
			logASM << dec << showbase << pos2 << " RET " << shVal2;
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

				
				if(flagAddrRel(cFlag) || flagAddrAbs(cFlag) || flagAddrRelTop(cFlag))
				{
					cmdList->GetINT(pos, valind);
					pos += 4;
				}
				logASM << valind;
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
			cmdList->GetUSHORT(pos, cFlag);
			pos += 2;
			logASM << pos2 << " POP ";
			logASM << typeInfoS[cFlag&0x00000003];
			if(flagStackType(cFlag) == STYPE_COMPLEX_TYPE)
			{
				cmdList->GetUINT(pos, valind);
				pos += 4;
				logASM << " sizeof(" << valind << ")";
			}
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
