#include "Callbacks.h"
#include "CodeInfo.h"

#include "Parser.h"

const int ERR_BUF_SIZE = 256;
char	callbackError[ERR_BUF_SIZE];

// Temp variables

// variable position from base of current stack frame
unsigned int varTop;

// Current variable alignment, in bytes
unsigned int currAlign;

// Is some variable being defined at the moment
bool varDefined;

// Is current variable const
bool currValConst;

// Number of implicit array
unsigned int inplaceArrayNum;

// Stack of variable counts.
// Used to find how many variables are to be removed when their visibility ends.
// Also used to know how much space for local variables is required in stack frame.
FastVector<VarTopInfo>		varInfoTop(64);

// Stack of function counts.
// Used to find how many functions are to be removed when their visibility ends.
FastVector<unsigned int>	funcInfoTop(64);

// Cycle depth stack is used to determine what is the depth of the loop when compiling operators break and continue
FastVector<unsigned int>	cycleDepth(64);

// Stack of functions that are being defined at the moment
FastVector<FunctionInfo*>	currDefinedFunc(64);

// Information about current type
TypeInfo*	currType = NULL;

// For new type creation
TypeInfo *newType = NULL;

unsigned int	TypeInfo::buildInSize = 0;
ChunkedStackPool<4092> TypeInfo::typeInfoPool;
unsigned int	VariableInfo::buildInSize = 0;
ChunkedStackPool<4092> VariableInfo::variablePool;
unsigned int	FunctionInfo::buildInSize = 0;
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
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: Digit %d is not allowed in base %d", digit, base);
			ThrowError(callbackError, p);
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
		if(*str == '-')
			return (integer + fractional) * pow(10.0, (double)-parseInteger(str+1));
		else
			return (integer + fractional) * pow(10.0, (double)parseInteger(str));
	}
	return integer + fractional;
}

// Saves number of defined variables and functions
void BeginBlock()
{
	varInfoTop.push_back(VarTopInfo((unsigned int)CodeInfo::varInfo.size(), varTop));
	funcInfoTop.push_back((unsigned int)CodeInfo::funcInfo.size());
}
// Restores previous number of defined variables and functions to hide those that lost visibility
void EndBlock(bool hideFunctions)
{
	CodeInfo::varInfo.shrink(varInfoTop.back().activeVarCnt);
	varInfoTop.pop_back();

	if(hideFunctions)
	{
		for(unsigned int i = funcInfoTop.back(); i < CodeInfo::funcInfo.size(); i++)
			CodeInfo::funcInfo[i]->visible = false;
	}
	funcInfoTop.pop_back();
}

// Input is a symbol after '\'
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
		ThrowError("ERROR: unknown escape sequence", CodeInfo::lastKnownStartPos);
	return res;
}

unsigned int GetAlignmentOffset(const char *pos, unsigned int alignment)
{
	if(alignment > 16)
		ThrowError("ERROR: alignment must be less than 16 bytes", pos);
	// If alignment is set and address is not aligned
	if(alignment != 0 && varTop % alignment != 0)
		return alignment - (varTop % alignment);
	return 0;
}

void CheckForImmutable(TypeInfo* type, const char* pos)
{
	if(type->refLevel == 0)
	{
		SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: cannot change immutable value of type %s", type->GetFullTypeName());
		ThrowError(callbackError, pos);
	}
}
// Functions used to add node for constant numbers of different type
void AddNumberNodeChar(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos + 1;
	char res = pos[1];
	if(res == '\\')
		res = UnescapeSybmol(pos[2]);
	CodeInfo::nodeList.push_back(new NodeNumber(int(res), typeChar));
}

void AddNumberNodeInt(const char* pos)
{
	CodeInfo::nodeList.push_back(new NodeNumber(parseInteger(pos), typeInt));
}
void AddNumberNodeFloat(const char* pos)
{
	CodeInfo::nodeList.push_back(new NodeNumber((float)parseDouble(pos), typeFloat));
}
void AddNumberNodeLong(const char* pos, const char* end)
{
	CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 10), typeLong));
}
void AddNumberNodeDouble(const char* pos)
{
	CodeInfo::nodeList.push_back(new NodeNumber(parseDouble(pos), typeDouble));
}

void AddHexInteger(const char* pos, const char* end)
{
	pos += 2;
	if(int(end - pos) > 16)
		ThrowError("ERROR: Overflow in hexadecimal constant", pos);
	if(int(end - pos) <= 8)
		CodeInfo::nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 16), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 16), typeLong));
}

void AddOctInteger(const char* pos, const char* end)
{
	pos++;
	if(int(end - pos) > 21)
		ThrowError("ERROR: Overflow in octal constant", pos);
	if(int(end - pos) <= 10)
		CodeInfo::nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 8), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 8), typeLong));
}

void AddBinInteger(const char* pos, const char* end)
{
	if(int(end - pos) > 64)
		ThrowError("ERROR: Overflow in binary constant", pos);
	if(int(end - pos) <= 32)
		CodeInfo::nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 2), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 2), typeLong));
}

void AddVoidNode()
{
	CodeInfo::nodeList.push_back(new NodeZeroOP());
}

// Function that places string on stack, using list of NodeNumber in NodeExpressionList
void AddStringNode(const char* s, const char* e)
{
	CodeInfo::lastKnownStartPos = s;

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

	CodeInfo::nodeList.push_back(new NodeZeroOP());
	TypeInfo *targetType = CodeInfo::GetArrayType(typeChar, len + 1);
	if(!targetType)
		ThrowLastError();
	CodeInfo::nodeList.push_back(new NodeExpressionList(targetType));

	NodeZeroOP* temp = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

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
		CodeInfo::nodeList.push_back(new NodeNumber(*(int*)clean, typeInt));
		arrayList->AddNode();
	}
	if(len % 4 == 0)
	{
		CodeInfo::nodeList.push_back(new NodeNumber(0, typeInt));
		arrayList->AddNode();
	}
	CodeInfo::nodeList.push_back(temp);
}

// Function that creates node that removes value on top of the stack
void AddPopNode(const char* pos)
{
	// If the last node is a number, remove it completely
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
	{
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(new NodeZeroOP());
	}else if(CodeInfo::nodeList.back()->nodeType == typeNodePreOrPostOp){
		// If the last node is increment or decrement, then we do not need to keep the value on stack, and some optimizations can be done
		static_cast<NodePreOrPostOp*>(CodeInfo::nodeList.back())->SetOptimised(true);
	}else{
		// Otherwise, just create node
		CodeInfo::nodeList.push_back(new NodePopOp());
	}
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}

// Function that creates unary operation node that changes sign of value
void AddNegateNode(const char* pos)
{
	// If the last node is a number, we can just change sign of constant
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = CodeInfo::nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeDouble || aType == typeFloat)
		{
			Rd = new NodeNumber(-static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetDouble(), aType);
		}else if(aType == typeLong){
			Rd = new NodeNumber(-static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetLong(), aType);
		}else if(aType == typeInt || aType == typeShort || aType == typeChar){
			Rd = new NodeNumber(-static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger(), aType);
		}else{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "addNegNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, pos);
		}
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(Rd);
	}else{
		// Otherwise, just create node
		CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdNeg));
	}
}

// Function that creates unary operation node for logical NOT
void AddLogNotNode(const char* pos)
{
	if(CodeInfo::nodeList.back()->typeInfo == typeDouble || CodeInfo::nodeList.back()->typeInfo == typeFloat)
		ThrowError("ERROR: logical NOT is not available on floating-point numbers", pos);
	// If the last node is a number, we can just make operation in compile-time
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = CodeInfo::nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeLong){
			Rd = new NodeNumber((long long)!static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetLong(), aType);
		}else if(aType == typeInt || aType == typeShort || aType == typeChar){
			Rd = new NodeNumber(!static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger(), aType);
		}else{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "addLogNotNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, pos);
		}
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(Rd);
	}else{
		// Otherwise, just create node
		CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdLogNot));
	}
}

// Function that creates unary operation node for binary NOT
void AddBitNotNode(const char* pos)
{
	if(CodeInfo::nodeList.back()->typeInfo == typeDouble || CodeInfo::nodeList.back()->typeInfo == typeFloat)
		ThrowError("ERROR: binary NOT is not available on floating-point numbers", pos);
	// If the last node is a number, we can just make operation in compile-time
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = CodeInfo::nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeLong){
			Rd = new NodeNumber(~static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetLong(), aType);
		}else if(aType == typeInt || aType == typeShort || aType == typeChar){
			Rd = new NodeNumber(~static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger(), aType);
		}else{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "addBitNotNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, pos);
		}
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(Rd);
	}else{
		CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdBitNot));
	}
}

// Functions to apply binary operations in compile-time
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
	{
		if(b == 0)
			ThrowError("ERROR: Division by zero during constant folding", CodeInfo::lastKnownStartPos);
		return a / b;
	}
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
	{
		if(b == 0)
			ThrowError("ERROR: Modulus division by zero during constant folding", CodeInfo::lastKnownStartPos);
		return a % b;
	}
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
	assert(!"optDoSpecial<int> with unknown command");
	return 0;
}
template<> long long optDoSpecial<>(CmdID cmd, long long a, long long b)
{
	if(cmd == cmdShl)
		return a << b;
	if(cmd == cmdShr)
		return a >> b;
	if(cmd == cmdMod)
	{
		if(b == 0)
			ThrowError("ERROR: Modulus division by zero during constant folding", CodeInfo::lastKnownStartPos);
		return a % b;
	}
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
	assert(!"optDoSpecial<long long> with unknown command");
	return 0;
}
template<> double optDoSpecial<>(CmdID cmd, double a, double b)
{
	if(cmd == cmdMod)
		return fmod(a,b);
	if(cmd == cmdShl)
		ThrowError("ERROR: << is illegal for floating-point numbers", CodeInfo::lastKnownStartPos);
	if(cmd == cmdShr)
		ThrowError("ERROR: >> is illegal for floating-point numbers", CodeInfo::lastKnownStartPos);
	if(cmd == cmdBitAnd)
		ThrowError("ERROR: & is illegal for floating-point numbers", CodeInfo::lastKnownStartPos);
	if(cmd == cmdBitOr)
		ThrowError("ERROR: | is illegal for floating-point numbers", CodeInfo::lastKnownStartPos);
	if(cmd == cmdBitXor)
		ThrowError("ERROR: ^ is illegal for floating-point numbers", CodeInfo::lastKnownStartPos);
	if(cmd == cmdLogAnd)
		ThrowError("ERROR: && is illegal for floating-point numbers", CodeInfo::lastKnownStartPos);
	if(cmd == cmdLogXor)
		ThrowError("ERROR: || is illegal for floating-point numbers", CodeInfo::lastKnownStartPos);
	if(cmd == cmdLogOr)
		ThrowError("ERROR: ^^ is illegal for floating-point numbers", CodeInfo::lastKnownStartPos);
	assert(!"optDoSpecial<double> with unknown command");
	return 0.0;
}

void RemoveLastNode(bool swap)
{
	if(swap)
	{
		NodeZeroOP* temp = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.back() = temp;
	}else{
		CodeInfo::nodeList.pop_back();
	}
}

void AddBinaryCommandNode(const char* pos, CmdID id)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int aNodeType = CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->nodeType;
	unsigned int bNodeType = CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->nodeType;

	if(aNodeType == typeNodeNumber && bNodeType == typeNodeNumber)
	{
		NodeNumber *Ad = static_cast<NodeNumber*>(CodeInfo::nodeList[CodeInfo::nodeList.size() - 2]);
		NodeNumber *Bd = static_cast<NodeNumber*>(CodeInfo::nodeList[CodeInfo::nodeList.size() - 1]);
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.pop_back();

		// If we have operation between two known numbers, we can optimize code by calculating the result in compile-time
		TypeInfo *aType = Ad->typeInfo;
		TypeInfo *bType = Bd->typeInfo;

		bool swapOper = false;
		// Swap operands, to reduce number of combinations
		if(((aType == typeFloat || aType == typeLong || aType == typeInt || aType == typeShort || aType == typeChar) && bType == typeDouble) ||
			((aType == typeLong || aType == typeInt || aType == typeShort || aType == typeChar) && bType == typeFloat) ||
			((aType == typeInt || aType == typeShort || aType == typeChar) && bType == typeLong))
		{
			Swap(Ad, Bd);
			swapOper = true;
		}

		NodeNumber *Rd = NULL;
		if(Ad->typeInfo == typeDouble || Ad->typeInfo == typeFloat)
			Rd = new NodeNumber(optDoOperation<double>(id, Ad->GetDouble(), Bd->GetDouble(), swapOper), typeDouble);
		else if(Ad->typeInfo == typeLong)
			Rd = new NodeNumber(optDoOperation<long long>(id, Ad->GetLong(), Bd->GetLong(), swapOper), typeLong);
		else if(Ad->typeInfo == typeInt || Ad->typeInfo == typeShort || Ad->typeInfo == typeChar)
			Rd = new NodeNumber(optDoOperation<int>(id, Ad->GetInteger(), Bd->GetInteger(), swapOper), typeInt);
		CodeInfo::nodeList.push_back(Rd);
		return;
	}else if(aNodeType == typeNodeNumber || bNodeType == typeNodeNumber){
		NodeZeroOP *opA = CodeInfo::nodeList[CodeInfo::nodeList.size() - 2];
		NodeZeroOP *opB = CodeInfo::nodeList[CodeInfo::nodeList.size() - 1];

		bool nodesAreSwaped = false;
		if(aNodeType != typeNodeNumber)
		{
			Swap(opA, opB);
			nodesAreSwaped = true;
		}

		// 1 * anything == anything		anything * 1 == anything
		// 0 + anything == anything		anything + 0 == anything
		if((id == cmdMul && static_cast<NodeNumber*>(opA)->GetDouble() == 1.0) ||
			(id == cmdAdd && static_cast<NodeNumber*>(opA)->GetDouble() == 0.0))
		{
			RemoveLastNode(!nodesAreSwaped);
			return;
		}
		// 0 - anything = -anything		anything - 0 == anything
		if(id == cmdSub && static_cast<NodeNumber*>(opA)->GetDouble() == 0.0)
		{
			if(!nodesAreSwaped)
			{
				RemoveLastNode(!nodesAreSwaped);
				AddNegateNode(NULL);
			}else{
				RemoveLastNode(!nodesAreSwaped);
			}
			return;
		}
	}

	const char *opNames[] = { "+", "-", "*", "/", "**", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "&", "|", "^", "and", "or", "xor" };
	// Optimizations failed, perform operation in run-time
	if(!AddFunctionCallNode(CodeInfo::lastKnownStartPos, opNames[id - cmdAdd], 2, true))
		CodeInfo::nodeList.push_back(new NodeBinaryOp(id));
	if(!CodeInfo::lastError.IsEmpty())
		ThrowLastError();
}

void AddReturnNode(const char* pos)
{
	int localReturn = currDefinedFunc.size() != 0;

	TypeInfo *realRetType = CodeInfo::nodeList.back()->typeInfo;
	TypeInfo *expectedType = NULL;
	if(currDefinedFunc.size() != 0)
	{
		if(!currDefinedFunc.back()->retType)
		{
			currDefinedFunc.back()->retType = realRetType;
			currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back());
		}
		expectedType = currDefinedFunc.back()->retType;
		if((expectedType->type == TypeInfo::TYPE_COMPLEX || realRetType->type == TypeInfo::TYPE_COMPLEX) && expectedType != realRetType)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: function returns %s but supposed to return %s", realRetType->GetFullTypeName(), expectedType->GetFullTypeName());
			ThrowError(callbackError, pos);
		}
		if(expectedType->type == TypeInfo::TYPE_VOID && realRetType != typeVoid)
			ThrowError("ERROR: function returning a value", pos);
		if(expectedType != typeVoid && realRetType == typeVoid)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: function should return %s", expectedType->GetFullTypeName());
			ThrowError(callbackError, pos);
		}
	}else{
		if(currDefinedFunc.size() == 0 && realRetType == typeVoid)
			ThrowError("ERROR: global return cannot accept void", pos);
		expectedType = realRetType;
	}
	CodeInfo::nodeList.push_back(new NodeReturnOp(localReturn, expectedType));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}

void AddBreakNode(const char* pos)
{
	unsigned int breakDepth = 1;
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
		breakDepth = static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger();
	else if(CodeInfo::nodeList.back()->nodeType != typeNodeZeroOp)
		ThrowError("ERROR: break must be followed by ';' or a constant", pos);

	CodeInfo::nodeList.pop_back();

	if(breakDepth == 0)
		ThrowError("ERROR: break level cannot be 0", pos);
	if(cycleDepth.back() < breakDepth)
		ThrowError("ERROR: break level is greater that loop depth", pos);

	CodeInfo::nodeList.push_back(new NodeBreakOp(breakDepth));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}

void AddContinueNode(const char* pos)
{
	unsigned int continueDepth = 1;
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
		continueDepth = static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger();
	else if(CodeInfo::nodeList.back()->nodeType != typeNodeZeroOp)
		ThrowError("ERROR: continue must be followed by ';' or a constant", pos);

	CodeInfo::nodeList.pop_back();

	if(continueDepth == 0)
		ThrowError("ERROR: continue level cannot be 0", pos);
	if(cycleDepth.back() < continueDepth)
		ThrowError("ERROR: continue level is greater that loop depth", pos);

	CodeInfo::nodeList.push_back(new NodeContinueOp(continueDepth));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}

void SelectTypeByPointer(void* type)
{
	currType = (TypeInfo*)type;
}

void SelectTypeByIndex(unsigned int index)
{
	currType = CodeInfo::typeInfo[index];
}

void* GetSelectedType()
{
	return (void*)currType;
}

void AddVariable(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// Check for variables with the same name in current scope
	for(unsigned int i = varInfoTop.back().activeVarCnt; i < CodeInfo::varInfo.size(); i++)
	{
		if(CodeInfo::varInfo[i]->nameHash == hash)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: Name '%.*s' is already taken for a variable in current scope", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}
	}
	// Check for functions with the same name
	if(CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size() - 1) != -1)
	{
		SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: Name '%.*s' is already taken for a function", varName.end-varName.begin, varName.begin);
		ThrowError(callbackError, pos);
	}

	if((currType && currType->alignBytes != 0) || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
	{
		unsigned int offset = GetAlignmentOffset(pos, currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : currType->alignBytes);
		varTop += offset;
	}
	CodeInfo::varInfo.push_back(new VariableInfo(varName, hash, varTop, currType, currValConst));
	varDefined = true;
	if(currType)
		varTop += currType->size;
}

void AddVariableReserveNode(const char* pos)
{
	assert(varDefined);
	if(!currType)
		ThrowError("ERROR: auto variable must be initialized in place of definition", pos);
	CodeInfo::nodeList.push_back(new NodeZeroOP());
	CodeInfo::varInfo.back()->dataReserved = true;
	varDefined = 0;
}

void ConvertTypeToReference(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	if(!currType)
		ThrowError("ERROR: auto variable cannot have reference flag", pos);
	currType = CodeInfo::GetReferenceType(currType);
}

void ConvertTypeToArray(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	if(!currType)
		ThrowError("ERROR: cannot specify array size for auto variable", pos);
	currType = CodeInfo::GetArrayType(currType);
	if(!currType)
		ThrowLastError();
}

//////////////////////////////////////////////////////////////////////////
//					New functions for work with variables

void GetTypeSize(const char* pos, bool sizeOfExpr)
{
	if(!sizeOfExpr && !currType)
		ThrowError("ERROR: sizeof(auto) is illegal", pos);
	if(sizeOfExpr)
	{
		currType = CodeInfo::nodeList.back()->typeInfo;
		CodeInfo::nodeList.pop_back();
	}
	CodeInfo::nodeList.push_back(new NodeNumber((int)currType->size, typeInt));
}

void SetTypeOfLastNode()
{
	currType = CodeInfo::nodeList.back()->typeInfo;
	CodeInfo::nodeList.pop_back();
}

bool ConvertArrayToUnsized(const char* pos, TypeInfo *dstType);
bool ConvertFunctionToPointer(const char* pos);

// Function that retrieves variable address
void AddGetAddressNode(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// Find in variable list
	int i = CodeInfo::FindVariableByName(hash);
	if(i == -1)
	{ 
		int fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);
		if(fID == -1)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: function '%.*s' is not defined", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}

		if(CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}

		if(CodeInfo::funcInfo[fID]->type == FunctionInfo::LOCAL)
		{
			char	*contextName = AllocateString(CodeInfo::funcInfo[fID]->nameLength + 6);
			int length = sprintf(contextName, "$%s_ext", CodeInfo::funcInfo[fID]->name);
			unsigned int contextHash = GetStringHash(contextName);

			int i = CodeInfo::FindVariableByName(contextHash);
			if(i == -1)
				CodeInfo::nodeList.push_back(new NodeNumber(0, CodeInfo::GetReferenceType(typeInt)));
			else
				AddGetAddressNode(pos, InplaceStr(contextName, length));
		}

		// Create node that retrieves function address
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(CodeInfo::funcInfo[fID]));
	}else{
		if(!CodeInfo::varInfo[i]->varType)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: variable '%.*s' is being used while its type is unknown", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}
		if(newType && currDefinedFunc.back()->type == FunctionInfo::THISCALL)
		{
			bool member = false;
			for(TypeInfo::MemberVariable *curr = newType->firstVariable; curr; curr = curr->next)
				if(curr->nameHash == hash)
					member = true;
			if(member)
			{
				// Class members are accessed through 'this' pointer
				FunctionInfo *currFunc = currDefinedFunc.back();

				TypeInfo *temp = CodeInfo::GetReferenceType(newType);
				CodeInfo::nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));

				AddDereferenceNode(pos);
				AddMemberAccessNode(pos, varName);
				return;
			}
		}

		// If we try to access external variable from local function
		if(currDefinedFunc.size() > 1 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) &&
			i >= (int)varInfoTop[currDefinedFunc[0]->vTopSize].activeVarCnt && i < (int)varInfoTop[currDefinedFunc.back()->vTopSize].activeVarCnt)
		{
			FunctionInfo *currFunc = currDefinedFunc.back();
			// Add variable name to the list of function external variables
			int num = AddFunctionExternal(currFunc, varName);

			TypeInfo *temp = CodeInfo::GetReferenceType(typeInt);
			temp = CodeInfo::GetArrayType(temp, (int)currDefinedFunc.back()->externalCount);
			if(!temp)
				ThrowLastError();
			temp = CodeInfo::GetReferenceType(temp);

			CodeInfo::nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));
			AddDereferenceNode(pos);
			CodeInfo::nodeList.push_back(new NodeNumber(num, typeInt));
			AddArrayIndexNode(pos);
			AddDereferenceNode(pos);
		}else{
			// If variable is in global scope, use absolute address
			bool absAddress = currDefinedFunc.size() == 0 || ((varInfoTop.size() > 1) && (CodeInfo::varInfo[i]->pos < varInfoTop[currDefinedFunc[0]->vTopSize].varStackSize));

			int varAddress = CodeInfo::varInfo[i]->pos;
			if(!absAddress)
				varAddress -= (int)(varInfoTop[currDefinedFunc.back()->vTopSize].varStackSize);

			// Create node that places variable address on stack
			CodeInfo::nodeList.push_back(new NodeGetAddress(CodeInfo::varInfo[i], varAddress, absAddress, CodeInfo::varInfo[i]->varType));
		}
	}
}

// Function for array indexing
void AddArrayIndexNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "[]", 2, true))
		return;

	TypeInfo *currentType = CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo;
	if(currentType->refLevel == 0)
		ThrowError("ERROR: indexing variable that is not an array", pos);
	currentType = CodeInfo::GetDereferenceType(currentType);

	// If we are indexing pointer to array
	if(currentType->refLevel == 1 && currentType->subType->arrLevel != 0)
	{
		// Then, before indexing it, we need to get address from this variable
		NodeZeroOP* temp = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(new NodeDereference());
		CodeInfo::nodeList.push_back(temp);
		currentType = currentType->subType;
	}
	// If it is array without explicit size (pointer to array)
	if(currentType->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		// Then, before indexing it, we need to get address from this variable
		NodeZeroOP* temp = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(new NodeDereference());
		CodeInfo::nodeList.push_back(temp);
	}
	
	// Current type must be an array
	if(currentType->arrLevel == 0)
		ThrowError("ERROR: indexing variable that is not an array", pos);
	
	// If index is a number and previous node is an address, then indexing can be done in compile-time
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->nodeType == typeNodeGetAddress)
	{
		// Get shift value
		int shiftValue = static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger();

		// Check bounds
		if(shiftValue < 0)
			ThrowError("ERROR: Array index cannot be negative", pos);
		if((unsigned int)shiftValue >= currentType->arrSize)
			ThrowError("ERROR: Array index out of bounds", pos);

		// Index array
		static_cast<NodeGetAddress*>(CodeInfo::nodeList[CodeInfo::nodeList.size()-2])->IndexArray(shiftValue);
		CodeInfo::nodeList.pop_back();
	}else{
		// Otherwise, create array indexing node
		CodeInfo::nodeList.push_back(new NodeArrayIndex(currentType));
		if(!CodeInfo::lastError.IsEmpty())
			ThrowLastError();
	}
}

// Function for pointer dereferencing
void AddDereferenceNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	// Change current type to type that pointer pointed to
	if(!CodeInfo::GetDereferenceType(CodeInfo::nodeList.back()->typeInfo))
		ThrowLastError();
	// Create dereference node
	CodeInfo::nodeList.push_back(new NodeDereference());
}

// Compiler expects that after variable there will be assignment operator
// If it's not the case, last node has to be removed
void FailedSetVariable()
{
	CodeInfo::nodeList.pop_back();
}

// Function for variable assignment in place of definition
void AddDefineVariableNode(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	int i = CodeInfo::FindVariableByName(hash);
	if(i == -1)
	{
		SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: variable '%.*s' is not defined", varName.end-varName.begin, varName.begin);
		ThrowError(callbackError, pos);
	}

	// If variable is in global scope, use absolute address
	bool absAddress = currDefinedFunc.size() == 0 || ((varInfoTop.size() > 1) && (CodeInfo::varInfo[i]->pos < varInfoTop[currDefinedFunc[0]->vTopSize].varStackSize));

	// If current type is set to NULL, it means that current type is auto
	// Is such case, type is retrieved from last AST node
	TypeInfo *realCurrType = CodeInfo::varInfo[i]->varType ? CodeInfo::varInfo[i]->varType : CodeInfo::nodeList.back()->typeInfo;

	// Maybe additional node will be needed to define variable
	// Variable signalizes that two nodes need to be unified in one
	bool unifyTwo = false;
	// If variable type is array without explicit size, and it is being defined with value of different type
	if(ConvertArrayToUnsized(pos, realCurrType))
		unifyTwo = true;

	// If a function is being assigned to variable, then take it's address
	if(ConvertFunctionToPointer(pos))
	{
		unifyTwo = true;
		if(!CodeInfo::varInfo[i]->varType)
			realCurrType = CodeInfo::nodeList.back()->typeInfo;
		varDefined = true;
	}
	// If type wasn't known until assignment, it means that variable alignment wasn't performed in AddVariable function
	if(!CodeInfo::varInfo[i]->varType)
	{
		CodeInfo::varInfo[i]->pos = varTop;
		// If type has default alignment or if user specified it
		if(realCurrType->alignBytes != 0 || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
		{
			// Find address offset. Alignment selected by user has higher priority than default alignment
			unsigned int offset = GetAlignmentOffset(pos, currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : realCurrType->alignBytes);
			CodeInfo::varInfo[i]->pos += offset;
			varTop += offset;
		}
		CodeInfo::varInfo[i]->varType = realCurrType;
		varTop += realCurrType->size;
	}
	CodeInfo::varInfo[i]->dataReserved = true;

	unsigned int varPosition = CodeInfo::varInfo[i]->pos;
	if(!absAddress)
		varPosition -= (int)(varInfoTop[currDefinedFunc.back()->vTopSize].varStackSize);

	CodeInfo::nodeList.push_back(new NodeGetAddress(CodeInfo::varInfo[i], varPosition, absAddress, CodeInfo::varInfo[i]->varType));

	CodeInfo::nodeList.push_back(new NodeVariableSet(CodeInfo::GetReferenceType(realCurrType), true, false));
	if(!CodeInfo::lastError.IsEmpty())
		ThrowLastError();

	if(unifyTwo)
	{
		CodeInfo::nodeList.push_back(new NodeExpressionList(CodeInfo::nodeList.back()->typeInfo));
		NodeZeroOP* temp = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp)->AddNode();
		CodeInfo::nodeList.push_back(temp);
	}
}

void AddSetVariableNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	CheckForImmutable(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, pos);

	bool unifyTwo = false;
	if(ConvertArrayToUnsized(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType))
		unifyTwo = true;
	if(ConvertFunctionToPointer(pos))
		unifyTwo = true;
	if(unifyTwo)
		Swap(CodeInfo::nodeList[CodeInfo::nodeList.size()-2], CodeInfo::nodeList[CodeInfo::nodeList.size()-3]);

	CodeInfo::nodeList.push_back(new NodeVariableSet(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, 0, true));
	if(!CodeInfo::lastError.IsEmpty())
		ThrowLastError();

	if(unifyTwo)
	{
		CodeInfo::nodeList.push_back(new NodeExpressionList(CodeInfo::nodeList.back()->typeInfo));
		NodeZeroOP* temp = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		static_cast<NodeExpressionList*>(temp)->AddNode();
		CodeInfo::nodeList.push_back(temp);
	}
}

void AddGetVariableNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && CodeInfo::nodeList.back()->typeInfo == typeVoid)
	{
		CodeInfo::nodeList.back()->typeInfo = typeInt;
	}else if(CodeInfo::nodeList.back()->typeInfo->funcType == NULL){
		CheckForImmutable(CodeInfo::nodeList.back()->typeInfo, pos);
		CodeInfo::nodeList.push_back(new NodeDereference());
	}
}

void AddMemberAccessNode(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// Beware that there is a global variable with the same name
	TypeInfo *currentType = CodeInfo::nodeList.back()->typeInfo;
	CheckForImmutable(currentType, pos);

	currentType = currentType->subType;
	if(currentType->refLevel == 1)
	{
		CodeInfo::nodeList.push_back(new NodeDereference());
		currentType = CodeInfo::GetDereferenceType(currentType);
		if(!currentType)
			ThrowLastError();
	}
	if(currentType->arrLevel != 0 && currentType->arrSize != TypeInfo::UNSIZED_ARRAY)
	{
		if(hash != GetStringHash("size"))
			ThrowError("ERROR: Array doesn't have member with this name", pos);
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(new NodeNumber((int)currentType->arrSize, typeVoid));
		currentType = typeInt;
		return;
	}
 
	int fID = -1;
	TypeInfo::MemberVariable *curr = currentType->firstVariable;
	for(; curr; curr = curr->next)
		if(curr->nameHash == hash)
			break;
	if(!curr)
	{
		unsigned int hash = currentType->nameHash;
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, varName.begin, varName.end);

		fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);
		if(fID == -1)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: function '%.*s' is not defined", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}

		if(CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}
	}
	
	if(fID == -1)
	{
		if(CodeInfo::nodeList.back()->nodeType == typeNodeGetAddress)
			static_cast<NodeGetAddress*>(CodeInfo::nodeList.back())->ShiftToMember(curr);
		else
			CodeInfo::nodeList.push_back(new NodeShiftAddress(curr->offset, curr->type));
	}else{
		// Node that gets function address
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(CodeInfo::funcInfo[fID]));
	}
}

void AddMemberFunctionCall(const char* pos, const char* funcName, unsigned int callArgCount)
{
	TypeInfo *parentType = CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount-1]->typeInfo->subType;
	if(!parentType->name)
	{
		SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: Type %s doesn't have any methods", parentType->GetFullTypeName());
		ThrowError(callbackError, pos);
	}
	char	*memberFuncName = AllocateString((int)strlen(parentType->name) + 2 + (int)strlen(funcName) + 1);
	sprintf(memberFuncName, "%s::%s", parentType->name, funcName);
	AddFunctionCallNode(pos, memberFuncName, callArgCount);
}

void AddPreOrPostOpNode(const char* pos, bool isInc, bool prefixOp)
{
	CheckForImmutable(CodeInfo::nodeList.back()->typeInfo, pos);
	CodeInfo::nodeList.push_back(new NodePreOrPostOp(isInc, prefixOp));
	if(!CodeInfo::lastError.IsEmpty())
		ThrowLastError();
}

void AddModifyVariableNode(const char* pos, CmdID cmd)
{
	CodeInfo::lastKnownStartPos = pos;

	CheckForImmutable(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, pos);
	CodeInfo::nodeList.push_back(new NodeVariableModify(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, cmd));
	if(!CodeInfo::lastError.IsEmpty())
		ThrowLastError();
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
	AddPopNode(pos);

	AddGetAddressNode(pos, InplaceStr(arrName, length));
	AddGetVariableNode(pos);

	varDefined = saveVarDefined;
	currType = saveCurrType;
}

bool ConvertArrayToUnsized(const char* pos, TypeInfo *dstType)
{
	bool unifyTwo = false;
	TypeInfo *nodeType = CodeInfo::nodeList.back()->typeInfo;
	// If subtype of both variables (presumably, arrays) is equal
	if(dstType->arrSize == TypeInfo::UNSIZED_ARRAY && dstType != nodeType)
	{
		bool createList = false;
		int typeSize = 0;
		if(dstType->subType == nodeType->subType)
		{
			// And if to the right of assignment operator there is no pointer dereference
			if(CodeInfo::nodeList.back()->nodeType != typeNodeDereference)
			{
				// Then, to the right of assignment operator there is array definition using inplace array
				if(CodeInfo::nodeList.back()->nodeType == typeNodeExpressionList || CodeInfo::nodeList.back()->nodeType == typeNodeFuncCall)
				{
					// Create node for assignment of inplace array to implicit variable
					AddInplaceArray(pos);
					// Now we have two nodes, so they are to be unified
					unifyTwo = true;
				}else{
					// Or if not, then types aren't compatible, so throw error
					SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: Cannot convert from %s to %s", CodeInfo::nodeList.back()->typeInfo->GetFullTypeName(), dstType->GetFullTypeName());
					ThrowError(callbackError, pos);
				}
			}
			// Because we are assigning array of explicit size to a pointer to array, we have to put a pair of pointer:size on top of stack
			// Take pointer to an array (node that is inside of pointer dereference node)
			NodeZeroOP	*oldNode = CodeInfo::nodeList.back();
			CodeInfo::nodeList.back() = static_cast<NodeDereference*>(oldNode)->GetFirstNode();
			static_cast<NodeDereference*>(oldNode)->SetFirstNode(NULL);
			// Find the size of an array
			typeSize = (nodeType->size - nodeType->paddingBytes) / nodeType->subType->size;
			createList = true;
		}else if(nodeType->refLevel == 1 && dstType->subType == nodeType->subType->subType){
			if(nodeType->subType->arrSize == TypeInfo::UNSIZED_ARRAY)
			{
				SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: Cannot convert from %s to %s", CodeInfo::nodeList.back()->typeInfo->GetFullTypeName(), dstType->GetFullTypeName());
				ThrowError(callbackError, pos);
			}
			// Set the size of an array
			typeSize = nodeType->subType->arrSize;
			createList = true;
		}
		if(createList)
		{
			// Create expression list with return type of implicit size array
			// Node constructor will take last node
			NodeExpressionList *listExpr = new NodeExpressionList(dstType);
			// Create node that places array size on top of the stack
			CodeInfo::nodeList.push_back(new NodeNumber(typeSize, typeInt));
			// Add it to expression list
			listExpr->AddNode();
			// Add expression list to node list
			CodeInfo::nodeList.push_back(listExpr);
		}
	}
	return unifyTwo;
}

bool ConvertFunctionToPointer(const char* pos)
{
	if(CodeInfo::nodeList.back()->nodeType == typeNodeFuncDef ||
		(CodeInfo::nodeList.back()->nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(CodeInfo::nodeList.back())->GetFirstNode()->nodeType == typeNodeFuncDef))
	{
		NodeFuncDef*	funcDefNode = (NodeFuncDef*)(CodeInfo::nodeList.back()->nodeType == typeNodeFuncDef ? CodeInfo::nodeList.back() : static_cast<NodeExpressionList*>(CodeInfo::nodeList.back())->GetFirstNode());
		AddGetAddressNode(pos, InplaceStr(funcDefNode->GetFuncInfo()->name, funcDefNode->GetFuncInfo()->nameLength));
		funcDefNode->GetFuncInfo()->visible = false;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void AddOneExpressionNode()
{
	CodeInfo::nodeList.push_back(new NodeExpressionList());
}
void AddTwoExpressionNode()
{
	if(CodeInfo::nodeList.back()->nodeType != typeNodeExpressionList)
		AddOneExpressionNode();
	// Take the expression list from the top
	NodeZeroOP* temp = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	static_cast<NodeExpressionList*>(temp)->AddNode();
	CodeInfo::nodeList.push_back(temp);
}

void AddArrayConstructor(const char* pos, unsigned int arrElementCount)
{
	arrElementCount++;

	TypeInfo *currentType = CodeInfo::nodeList[CodeInfo::nodeList.size()-arrElementCount]->typeInfo;

	if(currentType == typeShort || currentType == typeChar)
		currentType = typeInt;
	if(currentType == typeFloat)
		currentType = typeDouble;
	if(currentType == typeVoid)
		ThrowError("ERROR: array cannot be constructed from void type elements", pos);

	CodeInfo::nodeList.push_back(new NodeZeroOP());
	TypeInfo *targetType = CodeInfo::GetArrayType(currentType, arrElementCount);
	if(!targetType)
		ThrowLastError();

	NodeExpressionList *arrayList = new NodeExpressionList(targetType);

	TypeInfo *realType = CodeInfo::nodeList.back()->typeInfo;
	for(unsigned int i = 0; i < arrElementCount; i++)
	{
		if(realType != currentType && !((realType == typeShort || realType == typeChar) && currentType == typeInt) && !(realType == typeFloat && currentType == typeDouble))
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: element %d doesn't match the type of element 0 (%s)", arrElementCount-i-1, currentType->GetFullTypeName());
			ThrowError(callbackError, pos);
		}
		arrayList->AddNode(false);
	}

	CodeInfo::nodeList.push_back(arrayList);
}

void AddTypeAllocation(const char* pos)
{
	if(currType->arrLevel == 0)
	{
		AddFunctionCallNode(pos, "__newS", 1);
		CodeInfo::nodeList.back()->typeInfo = CodeInfo::GetReferenceType(currType);
	}else{
		AddFunctionCallNode(pos, "__newA", 2);
		CodeInfo::nodeList.back()->typeInfo = currType;
	}
}

void FunctionAdd(const char* pos, const char* funcName)
{
	unsigned int funcNameHash = GetStringHash(funcName);
	for(unsigned int i = varInfoTop.back().activeVarCnt; i < CodeInfo::varInfo.size(); i++)
	{
		if(CodeInfo::varInfo[i]->nameHash == funcNameHash)
		{
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: Name '%s' is already taken for a variable in current scope", funcName);
			ThrowError(callbackError, pos);
		}
	}
	char *funcNameCopy = (char*)funcName;
	if(newType)
	{
		funcNameCopy = AllocateString((int)strlen(newType->name) + 2 + (int)strlen(funcName) + 1);
		sprintf(funcNameCopy, "%s::%s", newType->name, funcName);
	}
	funcNameHash = GetStringHash(funcNameCopy);
	int k = CodeInfo::funcInfo.size();
	do
	{
		k = CodeInfo::FindFunctionByName(funcNameHash, k-1);
	}while(k != -1 && CodeInfo::funcInfo[k]->implemented);

	FunctionInfo* lastFunc = NULL;
	if(k != -1)
	{
		lastFunc = CodeInfo::funcInfo[k];
		lastFunc->allParamSize = 0;

		lastFunc->firstParam = lastFunc->lastParam = NULL;
		lastFunc->paramCount = 0;
	}else{
		CodeInfo::funcInfo.push_back(new FunctionInfo(funcNameCopy));
		lastFunc = CodeInfo::funcInfo.back();
	}

	lastFunc->vTopSize = (unsigned int)varInfoTop.size();
	lastFunc->retType = currType;
	if(newType)
		lastFunc->type = FunctionInfo::THISCALL;
	if(newType ? varInfoTop.size() > 2 : varInfoTop.size() > 1)
		lastFunc->type = FunctionInfo::LOCAL;
	if(funcName[0] != '$' && !(chartype_table[funcName[0]] & ct_start_symbol))
		lastFunc->visible = false;
	currDefinedFunc.push_back(lastFunc);
}

void FunctionParameter(const char* pos, InplaceStr paramName)
{
	if(!currType)
		ThrowError("ERROR: function parameter cannot be an auto type", pos);
	if(currType == typeVoid)
		ThrowError("ERROR: function parameter cannot be a void type", pos);
	unsigned int hash = GetStringHash(paramName.begin, paramName.end);
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	lastFunc.AddParameter(new VariableInfo(paramName, hash, 0, currType, currValConst));
	lastFunc.allParamSize += currType->size < 4 ? 4 : currType->size;
}

void FunctionPrototype()
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	lastFunc.funcType = lastFunc.retType ? CodeInfo::GetFunctionType(CodeInfo::funcInfo.back()) : NULL;
	currDefinedFunc.pop_back();
}

void FunctionStart(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	lastFunc.implemented = true;
	lastFunc.funcType = lastFunc.retType ? CodeInfo::GetFunctionType(CodeInfo::funcInfo.back()) : NULL;

	BeginBlock();
	cycleDepth.push_back(0);

	for(VariableInfo *curr = lastFunc.lastParam; curr; curr = curr->prev)
	{
		currValConst = curr->isConst;
		currType = curr->varType;
		currAlign = 4;
		AddVariable(pos, curr->name);
		varDefined = false;
	}

	char	*hiddenHame = AllocateString(lastFunc.nameLength + 8);
	int length = sprintf(hiddenHame, "$%s_ext", lastFunc.name);
	currType = CodeInfo::GetReferenceType(typeInt);
	currAlign = 4;
	AddVariable(pos, InplaceStr(hiddenHame, length));
	varDefined = false;
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

	int i = CodeInfo::FindFunctionByName(funcNameHash, CodeInfo::funcInfo.size()-1);
	// Find all the functions with the same name
	for(int n = 0; n < i; n++)
	{
		if(CodeInfo::funcInfo[n]->nameHash == CodeInfo::funcInfo[i]->nameHash && CodeInfo::funcInfo[n]->paramCount == CodeInfo::funcInfo[i]->paramCount && CodeInfo::funcInfo[n]->visible)
		{
			// Check all parameter types
			bool paramsEqual = true;
			for(VariableInfo *currN = CodeInfo::funcInfo[n]->firstParam, *currI = CodeInfo::funcInfo[i]->firstParam; currN; currN = currN->next, currI = currI->next)
			{
				if(currN->varType != currI->varType)
					paramsEqual = false;
			}
			if(paramsEqual)
			{
				SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: function '%s' is being defined with the same set of parameters", CodeInfo::funcInfo[i]->name);
				ThrowError(callbackError, pos);
			}
		}
	}

	cycleDepth.pop_back();
	// Save info about all local variables
	for(int i = CodeInfo::varInfo.size()-1; i > (int)(varInfoTop.back().activeVarCnt + lastFunc.paramCount); i--)
	{
		VariableInfo *firstNext = lastFunc.firstLocal;
		lastFunc.firstLocal = CodeInfo::varInfo[i];
		lastFunc.firstLocal->next = firstNext;
		if(lastFunc.firstLocal->next)
			lastFunc.firstLocal->next->prev = lastFunc.firstLocal;
		lastFunc.localCount++;
	}
	if(lastFunc.firstLocal)
		lastFunc.firstLocal->prev = NULL;

	EndBlock();
	unsigned int varFormerTop = varTop;
	varTop = varInfoTop[lastFunc.vTopSize].varStackSize;

	CodeInfo::nodeList.push_back(new NodeFuncDef(&lastFunc, varFormerTop-varTop));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
	CodeInfo::funcDefList.push_back(CodeInfo::nodeList.back());

	if(!currDefinedFunc.back()->retType)
	{
		currDefinedFunc.back()->retType = typeVoid;
		currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back());
	}
	currDefinedFunc.pop_back();

	// If function is local, create function parameters block
	if(lastFunc.type == FunctionInfo::LOCAL && lastFunc.externalCount != 0)
	{
		CodeInfo::nodeList.push_back(new NodeZeroOP());
		TypeInfo *targetType = CodeInfo::GetReferenceType(typeInt);
		if(!targetType)
			ThrowLastError();
		targetType = CodeInfo::GetArrayType(targetType, lastFunc.externalCount);
		if(!targetType)
			ThrowLastError();
		CodeInfo::nodeList.push_back(new NodeExpressionList(targetType));

		NodeZeroOP* temp = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();

		NodeExpressionList *arrayList = static_cast<NodeExpressionList*>(temp);

		for(FunctionInfo::ExternalName *curr = lastFunc.firstExternal; curr; curr = curr->next)
		{
			AddGetAddressNode(pos, curr->name);
			arrayList->AddNode();
		}
		CodeInfo::nodeList.push_back(temp);

		char	*hiddenHame = AllocateString(lastFunc.nameLength + 8);
		int length = sprintf(hiddenHame, "$%s_ext", lastFunc.name);

		TypeInfo *saveCurrType = currType;
		bool saveVarDefined = varDefined;

		currType = NULL;
		AddVariable(pos, InplaceStr(hiddenHame, length));

		AddDefineVariableNode(pos, InplaceStr(hiddenHame, length));
		AddPopNode(pos);

		varDefined = saveVarDefined;
		currType = saveCurrType;
		AddTwoExpressionNode();
	}

	if(newType)
	{
		newType->AddMemberFunction();
		newType->lastFunction->func = &lastFunc;
	}
}

void FunctionToOperator(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	if(lastFunc.paramCount != 2)
		ThrowError("ERROR: binary operator definition or overload must accept exactly two arguments", pos);
	if(lastFunc.type != FunctionInfo::NORMAL)
		ThrowError("ERROR: binary operator definition or overload must be placed in global scope", pos);
	lastFunc.visible = true;
}

FastVector<NodeZeroOP*> paramNodes(32);
FastVector<NodeZeroOP*> inplaceArray(32);

unsigned int GetFunctionRating(FunctionInfo *currFunc, unsigned int callArgCount)
{
	if(currFunc->paramCount != callArgCount)
		return ~0u;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.

	unsigned int fRating = 0;
	unsigned int n = 0;
	for(VariableInfo *curr = currFunc->firstParam; curr; curr = curr->next, n++)
	{
		NodeZeroOP* activeNode = CodeInfo::nodeList[CodeInfo::nodeList.size() - currFunc->paramCount + n];
		TypeInfo *paramType = activeNode->typeInfo;
		unsigned int	nodeType = activeNode->nodeType;
		TypeInfo *expectedType = curr->varType;
		if(expectedType != paramType)
		{
			if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->arrSize != 0 && paramType->subType == expectedType->subType)
				fRating += 5;
			else if(expectedType->funcType != NULL && nodeType == typeNodeFuncDef ||
					(nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(activeNode)->GetFirstNode()->nodeType == typeNodeFuncDef))
				fRating += 5;
			else if(expectedType->type == TypeInfo::TYPE_COMPLEX || paramType->type == TypeInfo::TYPE_COMPLEX)
				return ~0u;	// If one of types is complex, and they aren't equal, function cannot match
			else if(paramType->subType != expectedType->subType)
				return ~0u;	// Pointer or array with a different type inside. Doesn't matter if simple or complex.
			else	// Build-in types can convert to each other, but the fact of conversion tells us, that there could be a better suited function
				fRating += 1;
		}
	}
	return fRating;
}

FastVector<FunctionInfo*>	bestFuncList;
FastVector<unsigned int>	bestFuncRating;

bool AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount, bool silent)
{
	unsigned int funcNameHash = GetStringHash(funcName);

	// Searching, if function name is actually a variable name (which means, either it is a pointer to function, or an error)
	int vID = CodeInfo::FindVariableByName(funcNameHash);

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	if(vID == -1)
	{
		//Find all functions with given name
		bestFuncList.clear();

		for(unsigned int k = 0; k < CodeInfo::funcInfo.size(); k++)
		{
			if(CodeInfo::funcInfo[k]->nameHash == funcNameHash && CodeInfo::funcInfo[k]->visible)
				bestFuncList.push_back(CodeInfo::funcInfo[k]);
		}
		unsigned int count = bestFuncList.size();
		if(count == 0)
		{
			if(silent)
				return false;
			SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: function '%s' is undefined", funcName);
			ThrowError(callbackError, pos);
		}
		// Find the best suited function
		bestFuncRating.resize(count);

		unsigned int minRating = ~0u;
		unsigned int minRatingIndex = ~0u;
		for(unsigned int k = 0; k < count; k++)
		{
			bestFuncRating[k] = GetFunctionRating(bestFuncList[k], callArgCount);
			if(bestFuncRating[k] < minRating)
			{
				minRating = bestFuncRating[k];
				minRatingIndex = k;
			}
		}
		// Maybe the function we found can't be used at all
		if(minRatingIndex == ~0u)
		{
			if(silent)
				return false;
			char	errTemp[512];
			char	*errPos = errTemp;
			errPos += SafeSprintf(errPos, 512, "ERROR: can't find function '%s' with following parameters:\r\n  %s(", funcName, funcName);
			for(unsigned int n = 0; n < callArgCount; n++)
				errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "%s%s", CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount+n]->typeInfo->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
			errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), ")\r\n the only available are:\r\n");
			for(unsigned int n = 0; n < count; n++)
			{
				errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "  %s(", funcName);
				for(VariableInfo *curr = bestFuncList[n]->firstParam; curr; curr = curr->next)
					errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "%s%s", curr->varType->GetFullTypeName(), curr != bestFuncList[n]->lastParam ? ", " : "");
				errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), ")\r\n");
			}
			CodeInfo::lastError = CompilerError(errTemp, pos);
			ThrowLastError();
		}
		// Check, is there are more than one function, that share the same rating
		for(unsigned int k = 0; k < count; k++)
		{
			if(k != minRatingIndex && bestFuncRating[k] == minRating)
			{
				char errTemp[512];
				char	*errPos = errTemp;
				errPos += SafeSprintf(errPos, 512, "ERROR: Ambiguity, there is more than one overloaded function available for the call.\r\n  %s(", funcName);
				for(unsigned int n = 0; n < callArgCount; n++)
					errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "%s%s", CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount+n]->typeInfo->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
				errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), ")\r\n  candidates are:\r\n");
				for(unsigned int n = 0; n < count; n++)
				{
					if(bestFuncRating[n] != minRating)
						continue;
					errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "  %s(", funcName);
					for(VariableInfo *curr = bestFuncList[n]->firstParam; curr; curr = curr->next)
						errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "%s%s", curr->varType->GetFullTypeName(), curr != bestFuncList[n]->lastParam ? ", " : "");
					errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), ")\r\n");
				}
				CodeInfo::lastError = CompilerError(errTemp, pos);
				ThrowLastError();
			}
		}
		fType = bestFuncList[minRatingIndex]->funcType->funcType;
		fInfo = bestFuncList[minRatingIndex];
	}else{
		AddGetAddressNode(pos, InplaceStr(funcName, (int)strlen(funcName)));
		AddGetVariableNode(pos);
		fType = CodeInfo::nodeList.back()->typeInfo->funcType;
	}

	paramNodes.clear();
	for(unsigned int i = 0; i < fType->paramCount; i++)
	{
		paramNodes.push_back(CodeInfo::nodeList.back());
		CodeInfo::nodeList.pop_back();
	}
	inplaceArray.clear();

	for(unsigned int i = 0; i < fType->paramCount; i++)
	{
		unsigned int index = fType->paramCount - i - 1;

		CodeInfo::nodeList.push_back(paramNodes[index]);
		if(ConvertFunctionToPointer(pos))
		{
			NodeExpressionList* listExpr = new NodeExpressionList(paramNodes[index]->typeInfo);
			listExpr->AddNode();
			CodeInfo::nodeList.push_back(listExpr);
		}else if(ConvertArrayToUnsized(pos, fType->paramType[i])){
			inplaceArray.push_back(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]);
			CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = CodeInfo::nodeList.back();
			CodeInfo::nodeList.pop_back();
		}
	}

	if(fInfo && (fInfo->type == FunctionInfo::LOCAL))
	{
		char	*contextName = AllocateString(fInfo->nameLength + 6);
		int length = sprintf(contextName, "$%s_ext", fInfo->name);
		unsigned int contextHash = GetStringHash(contextName);

		int i = CodeInfo::FindVariableByName(contextHash);
		if(i == -1)
			CodeInfo::nodeList.push_back(new NodeNumber(0, CodeInfo::GetReferenceType(typeInt)));
		else
			AddGetAddressNode(pos, InplaceStr(contextName, length));
	}

	CodeInfo::nodeList.push_back(new NodeFuncCall(fInfo, fType));

	if(inplaceArray.size() > 0)
	{
		NodeZeroOP* temp = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		for(unsigned int i = 0; i < inplaceArray.size(); i++)
			CodeInfo::nodeList.push_back(inplaceArray[i]);
		CodeInfo::nodeList.push_back(temp);

		CodeInfo::nodeList.push_back(new NodeExpressionList(temp->typeInfo));
		for(unsigned int i = 0; i < inplaceArray.size(); i++)
			AddTwoExpressionNode();
	}
	return true;
}

void AddIfNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 2);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	CodeInfo::nodeList.push_back(new NodeIfElseExpr(false));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}
void AddIfElseNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 3);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-3]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	CodeInfo::nodeList.push_back(new NodeIfElseExpr(true));
}
void AddIfElseTermNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 3);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-3]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	TypeInfo* typeA = CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo;
	TypeInfo* typeB = CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo;
	if(typeA != typeB && (typeA->type == TypeInfo::TYPE_COMPLEX || typeB->type == TypeInfo::TYPE_COMPLEX))
	{
		SafeSprintf(callbackError, ERR_BUF_SIZE, "ERROR: ternary operator ?: \r\n result types are not equal (%s : %s)", typeB->name, typeA->name);
		ThrowError(callbackError, pos);
	}
	CodeInfo::nodeList.push_back(new NodeIfElseExpr(true, true));
}

void IncreaseCycleDepth()
{
	assert(cycleDepth.size() != 0);
	cycleDepth.back()++;
}
void AddForNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 4);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-3]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	CodeInfo::nodeList.push_back(new NodeForExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}
void AddWhileNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 2);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	CodeInfo::nodeList.push_back(new NodeWhileExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}
void AddDoWhileNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 2);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	CodeInfo::nodeList.push_back(new NodeDoWhileExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}

void BeginSwitch(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 1);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo == typeVoid)
		ThrowError("ERROR: cannot switch by void type", pos);

	assert(cycleDepth.size() != 0);
	cycleDepth.back()++;

	BeginBlock();
	CodeInfo::nodeList.push_back(new NodeSwitchExpr());
}

void AddCaseNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 3);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError("ERROR: case value type cannot be void", pos);
	NodeZeroOP* temp = CodeInfo::nodeList[CodeInfo::nodeList.size()-3];
	static_cast<NodeSwitchExpr*>(temp)->AddCase();
}

void AddDefaultNode()
{
	assert(CodeInfo::nodeList.size() >= 2);
	NodeZeroOP* temp = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
	static_cast<NodeSwitchExpr*>(temp)->AddDefault();
}

void EndSwitch()
{
	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;

	EndBlock();
}

void TypeBegin(const char* pos, const char* end)
{
	if(newType)
		ThrowError("ERROR: Different type is being defined", pos);
	if(currAlign > 16)
		ThrowError("ERROR: alignment must be less than 16 bytes", pos);

	char *typeNameCopy = AllocateString((int)(end - pos) + 1);
	sprintf(typeNameCopy, "%.*s", (int)(end - pos), pos);

	newType = new TypeInfo(CodeInfo::typeInfo.size(), typeNameCopy, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	newType->alignBytes = currAlign;
	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;

	CodeInfo::typeInfo.push_back(newType);

	BeginBlock();
}

void TypeAddMember(const char* pos, const char* varName)
{
	if(!currType)
		ThrowError("ERROR: auto cannot be used for class members", pos);
	newType->AddMemberVariable(varName, currType);
	if(newType->size > 64 * 1024)
		ThrowError("ERROR: class size cannot exceed 65535 bytes", pos);

	AddVariable(pos, InplaceStr(varName, (int)strlen(varName)));
}

void TypeFinish()
{
	if(newType->size % 4 != 0)
	{
		newType->paddingBytes = 4 - (newType->size % 4);
		newType->size += 4 - (newType->size % 4);
	}
	varTop -= newType->size;

	CodeInfo::nodeList.push_back(new NodeZeroOP());
	for(TypeInfo::MemberFunction *curr = newType->firstFunction; curr; curr = curr->next)
		AddTwoExpressionNode();

	newType = NULL;

	EndBlock(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AddUnfixedArraySize()
{
	CodeInfo::nodeList.push_back(new NodeNumber(1, typeVoid));
}

void CallbackInitialize()
{
	VariableInfo::DeleteVariableInformation();

	currDefinedFunc.clear();

	CodeInfo::funcDefList.clear();

	varDefined = 0;
	varTop = 0;
	newType = NULL;

	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;
	inplaceArrayNum = 1;

	varInfoTop.clear();
	varInfoTop.push_back(VarTopInfo(0,0));

	funcInfoTop.clear();
	funcInfoTop.push_back(0);

	cycleDepth.clear();
	cycleDepth.push_back(0);
}

unsigned int GetGlobalSize()
{
	return varTop;
}

void CallbackDeinitialize()
{
	VariableInfo::DeleteVariableInformation();
}
