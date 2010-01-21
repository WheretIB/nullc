// NULLC (c) NULL_PTR 2007-2010

#include "Callbacks.h"
#include "CodeInfo.h"

#include "Parser.h"

// Temp variables

// variable position from base of current stack frame
unsigned int varTop;

// Current variable alignment, in bytes
unsigned int currAlign;

// Is some variable being defined at the moment
bool varDefined;

// Number of implicit variable
unsigned int inplaceVariableNum;

// Stack of variable counts.
// Used to find how many variables are to be removed when their visibility ends.
// Also used to know how much space for local variables is required in stack frame.
FastVector<VarTopInfo>		varInfoTop;

VariableInfo	*lostGlobalList = NULL;

// Stack of function counts.
// Used to find how many functions are to be removed when their visibility ends.
FastVector<unsigned int>	funcInfoTop;

// Cycle depth stack is used to determine what is the depth of the loop when compiling operators break and continue
FastVector<unsigned int>	cycleDepth;

// Stack of functions that are being defined at the moment
FastVector<FunctionInfo*>	currDefinedFunc;

FastVector<FunctionInfo*>	sortedFunc;
void	AddFunctionToSortedList(void *info)
{
	sortedFunc.push_back((FunctionInfo*)info);
	unsigned int n = sortedFunc.size() - 1;
	while(n && ((FunctionInfo*)info)->nameHash < sortedFunc[n-1]->nameHash)
	{
		FunctionInfo *temp = sortedFunc[n-1];
		sortedFunc[n-1] = sortedFunc[n];
		sortedFunc[n] = temp;
		n--;
	}
}
// Finds a list of function, returns size of list, and offset to first element
unsigned int FindFirstFunctionIndex(unsigned int nameHash)
{
	int lowerBound = -1;
	int upperBound = sortedFunc.size();

	int pointer;
	while((pointer = (upperBound - lowerBound) >> 1) != 0)
	{
		pointer = lowerBound + pointer;
		unsigned int hash = sortedFunc[pointer]->nameHash;
		if(hash == nameHash)
			break;
		else if(hash < nameHash)
			lowerBound = pointer;
		else
			upperBound = pointer;
	}
	while(pointer && sortedFunc[pointer-1]->nameHash == nameHash)
		pointer--;
	return pointer;
}

// Information about current type
TypeInfo*		currType = NULL;

// For new type creation
TypeInfo*		newType = NULL;
unsigned int	methodCount = 0;

ChunkedStackPool<4092> TypeInfo::typeInfoPool;
ChunkedStackPool<4092> VariableInfo::variablePool;
ChunkedStackPool<4092>	FunctionInfo::functionPool;

FastVector<FunctionInfo*>	bestFuncList;
FastVector<unsigned int>	bestFuncRating;

FastVector<NodeZeroOP*>	paramNodes;

void AddInplaceVariable(const char* pos);
void ConvertArrayToUnsized(const char* pos, TypeInfo *dstType);
void ConvertFunctionToPointer(const char* pos);
void HandlePointerToObject(const char* pos, TypeInfo *dstType);

void AddExtraNode()
{
	NodeZeroOP *last = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	last->AddExtraNode();
	CodeInfo::nodeList.push_back(last);
}

template<typename T> void	Swap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
}

void SetCurrentAlignment(unsigned int alignment)
{
	currAlign = alignment;
}

// Finds variable inside function external variable list, and if not found, adds it to a list
FunctionInfo::ExternalInfo* AddFunctionExternal(FunctionInfo* func, VariableInfo* var)
{
	unsigned int hash = var->nameHash;
	unsigned int i = 0;
	for(FunctionInfo::ExternalInfo *curr = func->firstExternal; curr; curr = curr->next, i++)
		if(curr->variable->nameHash == hash)
			return curr;

	func->AddExternal(var);
	return func->lastExternal;
}

char* GetClassFunctionName(TypeInfo* type, const char* funcName)
{
	char *memberFuncName = AllocateString(type->GetFullNameLength() + 2 + (int)strlen(funcName) + 1);
	char *curr = memberFuncName;
	const char *typeName = type->GetFullTypeName();
	while(*typeName)
		*curr++ = *typeName++;
	*curr++ = ':';
	*curr++ = ':';
	while(*funcName)
		*curr++ = *funcName++;
	*curr = 0;
	return memberFuncName;
}
char* GetClassFunctionName(TypeInfo* type, InplaceStr funcName)
{
	char	*memberFuncName = AllocateString(type->GetFullNameLength() + 2 + (int)(funcName.end - funcName.begin) + 1);
	sprintf(memberFuncName, "%s::%.*s", type->GetFullTypeName(), (int)(funcName.end - funcName.begin), funcName.begin);
	return memberFuncName;
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
			ThrowError(p, "ERROR: Digit %d is not allowed in base %d", digit, base);
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
	
	if(*str == '.')
	{
		double power = 0.1f;
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
void EndBlock(bool hideFunctions, bool saveLocals)
{
	if(currDefinedFunc.size() > 0 && currDefinedFunc.back()->closeUpvals && (varInfoTop.size() - currDefinedFunc.back()->vTopSize - 1) < currDefinedFunc.back()->maxBlockDepth)
		CodeInfo::nodeList.push_back(new NodeBlock(currDefinedFunc.back(), varInfoTop.size() - currDefinedFunc.back()->vTopSize - 1));

	if(currDefinedFunc.size() > 0 && saveLocals)
	{
		FunctionInfo &lastFunc = *currDefinedFunc.back();
		for(unsigned int i = varInfoTop.back().activeVarCnt; i < CodeInfo::varInfo.size(); i++)
		{
			VariableInfo *firstNext = lastFunc.firstLocal;
			lastFunc.firstLocal = CodeInfo::varInfo[i];
			lastFunc.firstLocal->next = firstNext;
			if(lastFunc.firstLocal->next)
				lastFunc.firstLocal->next->prev = lastFunc.firstLocal;
			lastFunc.localCount++;
		}
	}else if(currDefinedFunc.size() == 0 && saveLocals){
		for(unsigned int i = varInfoTop.back().activeVarCnt; i < CodeInfo::varInfo.size(); i++)
		{
			CodeInfo::varInfo[i]->next = lostGlobalList;
			lostGlobalList = CodeInfo::varInfo[i];
		}
		
	}

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
	else if(symbol == '\"')
		res = '\"';
	else if(symbol == '\\')
		res = '\\';
	else
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: unknown escape sequence");
	return res;
}

unsigned int GetAlignmentOffset(const char *pos, unsigned int alignment)
{
	if(alignment > 16)
		ThrowError(pos, "ERROR: alignment must be less than 16 bytes");
	// If alignment is set and address is not aligned
	if(alignment != 0 && varTop % alignment != 0)
		return alignment - (varTop % alignment);
	return 0;
}

void CheckForImmutable(TypeInfo* type, const char* pos)
{
	if(type->refLevel == 0)
		ThrowError(pos, "ERROR: cannot change immutable value of type %s", type->GetFullTypeName());
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
		ThrowError(pos, "ERROR: Overflow in hexadecimal constant");
	// If number overflows integer number, create long number
	if(int(end - pos) <= 8)
		CodeInfo::nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 16), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 16), typeLong));
}

void AddOctInteger(const char* pos, const char* end)
{
	pos++;
	if(int(end - pos) > 21)
		ThrowError(pos, "ERROR: Overflow in octal constant");
	// If number overflows integer number, create long number
	if(int(end - pos) <= 10)
		CodeInfo::nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 8), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 8), typeLong));
}

void AddBinInteger(const char* pos, const char* end)
{
	if(int(end - pos) > 64)
		ThrowError(pos, "ERROR: Overflow in binary constant");
	// If number overflows integer number, create long number
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
			Rd = new NodeNumber(-static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetDouble(), aType);
		else if(aType == typeLong)
			Rd = new NodeNumber(-static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetLong(), aType);
		else if(aType == typeInt || aType == typeShort || aType == typeChar)
			Rd = new NodeNumber(-static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger(), aType);
		else
			ThrowError(pos, "addNegNode() ERROR: unknown type %s", aType->name);

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
		ThrowError(pos, "ERROR: logical NOT is not available on floating-point numbers");
	// If the last node is a number, we can just make operation in compile-time
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = CodeInfo::nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeLong)
			Rd = new NodeNumber((long long)!static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetLong(), aType);
		else if(aType == typeInt || aType == typeShort || aType == typeChar)
			Rd = new NodeNumber(!static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger(), aType);
		else
			ThrowError(pos, "addLogNotNode() ERROR: unknown type %s", aType->name);

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
		ThrowError(pos, "ERROR: binary NOT is not available on floating-point numbers");
	// If the last node is a number, we can just make operation in compile-time
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = CodeInfo::nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeLong)
			Rd = new NodeNumber(~static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetLong(), aType);
		else if(aType == typeInt || aType == typeShort || aType == typeChar)
			Rd = new NodeNumber(~static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger(), aType);
		else
			ThrowError(pos, "addBitNotNode() ERROR: unknown type %s", aType->name);

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
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Division by zero during constant folding");
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
	ThrowError(CodeInfo::lastKnownStartPos, "ERROR: optDoSpecial call with unknown type");
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
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Modulus division by zero during constant folding");
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
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: Modulus division by zero during constant folding");
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
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: << is illegal for floating-point numbers");
	if(cmd == cmdShr)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: >> is illegal for floating-point numbers");
	if(cmd == cmdBitAnd)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: & is illegal for floating-point numbers");
	if(cmd == cmdBitOr)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: | is illegal for floating-point numbers");
	if(cmd == cmdBitXor)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: ^ is illegal for floating-point numbers");
	if(cmd == cmdLogAnd)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: && is illegal for floating-point numbers");
	if(cmd == cmdLogXor)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: ^^ is illegal for floating-point numbers");
	if(cmd == cmdLogOr)
		ThrowError(CodeInfo::lastKnownStartPos, "ERROR: || is illegal for floating-point numbers");
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

	const char *opNames[] = { "+", "-", "*", "/", "**", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "&", "|", "^", "&&", "||", "^^" };
	// Optimizations failed, perform operation in run-time
	if(!AddFunctionCallNode(CodeInfo::lastKnownStartPos, opNames[id - cmdAdd], 2, true))
		CodeInfo::nodeList.push_back(new NodeBinaryOp(id));
}

void AddReturnNode(const char* pos)
{
	bool localReturn = currDefinedFunc.size() != 0;

	// If new function is returned, convert it to pointer
	ConvertFunctionToPointer(pos);

	// Type that is being returned
	TypeInfo *realRetType = CodeInfo::nodeList.back()->typeInfo;
	// Type that should be returned
	TypeInfo *expectedType = NULL;
	if(currDefinedFunc.size() != 0)
	{
		// If return type is auto, set it to type that is being returned
		if(!currDefinedFunc.back()->retType)
		{
			currDefinedFunc.back()->retType = realRetType;
			currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back()->retType, currDefinedFunc.back()->firstParam, currDefinedFunc.back()->paramCount);
		}
		// Take expected return type
		expectedType = currDefinedFunc.back()->retType;
		// Check for errors
		if((expectedType->type == TypeInfo::TYPE_COMPLEX || realRetType->type == TypeInfo::TYPE_COMPLEX) && expectedType != realRetType)
			ThrowError(pos, "ERROR: function returns %s but supposed to return %s", realRetType->GetFullTypeName(), expectedType->GetFullTypeName());
		if(expectedType == typeVoid && realRetType != typeVoid)
			ThrowError(pos, "ERROR: function returning a value");
		if(expectedType != typeVoid && realRetType == typeVoid)
			ThrowError(pos, "ERROR: function should return %s", expectedType->GetFullTypeName());
	}else{
		// Check for errors
		if(realRetType == typeVoid)
			ThrowError(pos, "ERROR: global return cannot accept void");
		else if(realRetType->type == TypeInfo::TYPE_COMPLEX)
			ThrowError(pos, "ERROR: global return cannot accept complex types");
		// Global return return type is auto, so we take type that is being returned
		expectedType = realRetType;
	}
	// Add node and link source to instruction
	CodeInfo::nodeList.push_back(new NodeReturnOp(localReturn, expectedType, currDefinedFunc.size() ? currDefinedFunc.back() : NULL));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}

void AddBreakNode(const char* pos)
{
	unsigned int breakDepth = 1;
	// break depth is 1 by default, but can be set to a constant number
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
		breakDepth = static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger();
	else if(CodeInfo::nodeList.back()->nodeType != typeNodeZeroOp)
		ThrowError(pos, "ERROR: break must be followed by ';' or a constant");

	CodeInfo::nodeList.pop_back();

	// Check for errors
	if(breakDepth == 0)
		ThrowError(pos, "ERROR: break level cannot be 0");
	if(cycleDepth.back() < breakDepth)
		ThrowError(pos, "ERROR: break level is greater that loop depth");

	// Add node and link source to instruction
	CodeInfo::nodeList.push_back(new NodeBreakOp(breakDepth));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}

void AddContinueNode(const char* pos)
{
	unsigned int continueDepth = 1;
	// continue depth is 1 by default, but can be set to a constant number
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber)
		continueDepth = static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger();
	else if(CodeInfo::nodeList.back()->nodeType != typeNodeZeroOp)
		ThrowError(pos, "ERROR: continue must be followed by ';' or a constant");

	CodeInfo::nodeList.pop_back();

	// Check for errors
	if(continueDepth == 0)
		ThrowError(pos, "ERROR: continue level cannot be 0");
	if(cycleDepth.back() < continueDepth)
		ThrowError(pos, "ERROR: continue level is greater that loop depth");

	// Add node and link source to instruction
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
			ThrowError(pos, "ERROR: Name '%.*s' is already taken for a variable in current scope", varName.end-varName.begin, varName.begin);
	}
	// Check for functions with the same name
	unsigned int index = FindFirstFunctionIndex(hash);
	if(sortedFunc[index]->nameHash == hash)
		ThrowError(pos, "ERROR: Name '%.*s' is already taken for a function", varName.end-varName.begin, varName.begin);

	if((currType && currType->alignBytes != 0) || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
	{
		unsigned int offset = GetAlignmentOffset(pos, currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : currType->alignBytes);
		varTop += offset;
	}
	CodeInfo::varInfo.push_back(new VariableInfo(varName, hash, varTop, currType, currDefinedFunc.size() == 0));
	varDefined = true;
	CodeInfo::varInfo.back()->blockDepth = varInfoTop.size();
	if(currType)
		varTop += currType->size;
}

void AddVariableReserveNode(const char* pos)
{
	assert(varDefined);
	if(!currType)
		ThrowError(pos, "ERROR: auto variable must be initialized in place of definition");
	CodeInfo::nodeList.push_back(new NodeZeroOP());
	varDefined = 0;
}

void ConvertTypeToReference(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	if(!currType)
		currType = typeObject;
	else
		currType = CodeInfo::GetReferenceType(currType);
}

void ConvertTypeToArray(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	if(!currType)
		ThrowError(pos, "ERROR: cannot specify array size for auto variable");
	currType = CodeInfo::GetArrayType(currType);
}

//////////////////////////////////////////////////////////////////////////
//					New functions for work with variables

void GetTypeSize(const char* pos, bool sizeOfExpr)
{
	if(!sizeOfExpr && !currType)
		ThrowError(pos, "ERROR: sizeof(auto) is illegal");
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

// Function that retrieves variable address
void AddGetAddressNode(const char* pos, InplaceStr varName, bool preferLastFunction)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// Find in variable list
	int i = CodeInfo::FindVariableByName(hash);
	if(i == -1)
	{
		int fID = -1;
		if(newType)
		{
			unsigned int hash = GetStringHash(GetClassFunctionName(newType, varName));
			fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);

			if(!preferLastFunction && CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
				ThrowError(pos, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);
		}
		if(fID == -1)
			fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);
		if(fID == -1)
			ThrowError(pos, "ERROR: variable or function '%.*s' is not defined", varName.end-varName.begin, varName.begin);

		if(!preferLastFunction && CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
			ThrowError(pos, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);

		if(CodeInfo::funcInfo[fID]->type == FunctionInfo::LOCAL)
		{
			char	*contextName = AllocateString(CodeInfo::funcInfo[fID]->nameLength + 24);
			int length = sprintf(contextName, "$%s_%p_ext", CodeInfo::funcInfo[fID]->name, CodeInfo::funcInfo[fID]);
			unsigned int contextHash = GetStringHash(contextName);

			int i = CodeInfo::FindVariableByName(contextHash);
			if(i == -1)
			{
				CodeInfo::nodeList.push_back(new NodeNumber(0, CodeInfo::GetReferenceType(typeInt)));
			}else{
				AddGetAddressNode(pos, InplaceStr(contextName, length));
				AddGetVariableNode(pos);
			}
		}else if(CodeInfo::funcInfo[fID]->type == FunctionInfo::THISCALL){
			AddGetAddressNode(pos, InplaceStr("this", 4));
			AddDereferenceNode(pos);
		}

		// Create node that retrieves function address
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(CodeInfo::funcInfo[fID]));
	}else{
		if(!CodeInfo::varInfo[i]->varType)
			ThrowError(pos, "ERROR: variable '%.*s' is being used while its type is unknown", varName.end-varName.begin, varName.begin);
		if(newType && currDefinedFunc.back()->type == FunctionInfo::THISCALL && CodeInfo::varInfo[i]->isGlobal)
		{
			TypeInfo::MemberVariable *curr = newType->firstVariable;
			for(; curr; curr = curr->next)
				if(curr->nameHash == hash)
					break;
			if(curr)
			{
				// Class members are accessed through 'this' pointer
				FunctionInfo *currFunc = currDefinedFunc.back();

				TypeInfo *temp = CodeInfo::GetReferenceType(newType);
				CodeInfo::nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));

				AddDereferenceNode(pos);
				CodeInfo::nodeList.push_back(new NodeShiftAddress(curr->offset, curr->type));
				return;
			}
		}

		// If we try to access external variable from local function
		if(currDefinedFunc.size() > 1 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) &&
			i >= (int)varInfoTop[currDefinedFunc[0]->vTopSize].activeVarCnt && i < (int)varInfoTop[currDefinedFunc.back()->vTopSize].activeVarCnt)
		{
			// If that variable is not in current scope, we have to get it through current closure
			FunctionInfo *currFunc = currDefinedFunc.back();
			// Add variable to the list of function external variables
			FunctionInfo::ExternalInfo *external = AddFunctionExternal(currFunc, CodeInfo::varInfo[i]);

			assert(currFunc->allParamSize % 4 == 0);
			CodeInfo::nodeList.push_back(new NodeGetUpvalue(currFunc->allParamSize, external->closurePos, CodeInfo::GetReferenceType(CodeInfo::varInfo[i]->varType)));
		}else{
			// Create node that places variable address on stack
			CodeInfo::nodeList.push_back(new NodeGetAddress(CodeInfo::varInfo[i], CodeInfo::varInfo[i]->pos, CodeInfo::varInfo[i]->isGlobal, CodeInfo::varInfo[i]->varType));
		}
	}
}

// Function for array indexing
void AddArrayIndexNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	// Call overloaded operator with error suppression
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "[]", 2, true))
		return;

	bool unifyTwo = false;

	// Get array type
	TypeInfo *currentType = CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo;
	// If it's an inplace array, set it to hidden variable, and put it's address on stack
	if(currentType->arrLevel != 0)
	{
		NodeZeroOP *index = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		AddInplaceVariable(pos);
		currentType = CodeInfo::GetReferenceType(currentType);
		CodeInfo::nodeList.push_back(index);
		unifyTwo = true;
	}
	if(currentType->refLevel == 0)
		ThrowError(pos, "ERROR: indexing variable that is not an array");
	// Get result type
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
		ThrowError(pos, "ERROR: indexing variable that is not an array");
	
	// If index is a number and previous node is an address, then indexing can be done in compile-time
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->nodeType == typeNodeGetAddress)
	{
		// Get shift value
		int shiftValue = static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger();

		// Check bounds
		if(shiftValue < 0)
			ThrowError(pos, "ERROR: Array index cannot be negative");
		if((unsigned int)shiftValue >= currentType->arrSize)
			ThrowError(pos, "ERROR: Array index out of bounds");

		// Index array
		static_cast<NodeGetAddress*>(CodeInfo::nodeList[CodeInfo::nodeList.size()-2])->IndexArray(shiftValue);
		CodeInfo::nodeList.pop_back();
	}else{
		// Otherwise, create array indexing node
		CodeInfo::nodeList.push_back(new NodeArrayIndex(currentType));
	}
	if(unifyTwo)
		AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
}

// Function for pointer dereferencing
void AddDereferenceNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	// Change current type to type that pointer pointed to
	CodeInfo::GetDereferenceType(CodeInfo::nodeList.back()->typeInfo);

	// Create dereference node
	CodeInfo::nodeList.push_back(new NodeDereference());
}

// Function for variable assignment in place of definition
void AddDefineVariableNode(const char* pos, InplaceStr varName, bool noOverload)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	int i = CodeInfo::FindVariableByName(hash);
	if(i == -1)
		ThrowError(pos, "ERROR: variable '%.*s' is not defined", varName.end-varName.begin, varName.begin);

	// If a function is being assigned to variable, then take it's address
	ConvertFunctionToPointer(pos);

	// If current type is set to NULL, it means that current type is auto
	// Is such case, type is retrieved from last AST node
	TypeInfo *realCurrType = CodeInfo::varInfo[i]->varType ? CodeInfo::varInfo[i]->varType : CodeInfo::nodeList.back()->typeInfo;

	// If variable type is array without explicit size, and it is being defined with value of different type
	ConvertArrayToUnsized(pos, realCurrType);
	HandlePointerToObject(pos, realCurrType);

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

	CodeInfo::nodeList.push_back(new NodeGetAddress(CodeInfo::varInfo[i], CodeInfo::varInfo[i]->pos, CodeInfo::varInfo[i]->isGlobal, CodeInfo::varInfo[i]->varType));

	// Call overloaded operator with error suppression
	if(!noOverload)
	{
		NodeZeroOP *temp = CodeInfo::nodeList[CodeInfo::nodeList.size()-1];
		CodeInfo::nodeList[CodeInfo::nodeList.size()-1] = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
		CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = temp;
		
		if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "=", 2, true))
			return;

		temp = CodeInfo::nodeList[CodeInfo::nodeList.size()-1];
		CodeInfo::nodeList[CodeInfo::nodeList.size()-1] = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
		CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = temp;
	}

	CodeInfo::nodeList.push_back(new NodeVariableSet(CodeInfo::GetReferenceType(realCurrType), true, false));
}

void AddSetVariableNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	NodeZeroOP *left = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
	if(left->nodeType == typeNodeFuncCall && ((NodeFuncCall*)left)->funcInfo)
	{
		FunctionInfo *fInfo = ((NodeFuncCall*)left)->funcInfo;
		if(fInfo->name[fInfo->nameLength-1] == '$')
		{
			char *memberSet = AllocateString(fInfo->nameLength+1);
			memcpy(memberSet, fInfo->name, fInfo->nameLength);
			memberSet[fInfo->nameLength-1] = '@';
			memberSet[fInfo->nameLength] = 0;
			CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = ((NodeFuncCall*)left)->GetFirstNode();
			if(AddFunctionCallNode(pos, memberSet, 1, true))
				return;
			else
				CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = left;
		}
	}

	// Call overloaded operator with error suppression
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "=", 2, true))
		return;

	CheckForImmutable(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, pos);

	// Make necessary implicit conversions
	ConvertArrayToUnsized(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);
	ConvertFunctionToPointer(pos);
	HandlePointerToObject(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);

	CodeInfo::nodeList.push_back(new NodeVariableSet(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, 0, true));
}

void AddGetVariableNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	TypeInfo *lastType = CodeInfo::nodeList.back()->typeInfo;
	if(!lastType)
		ThrowError(pos, "ERROR: variable type is unknown");

	// If array size is known at compile time, then it's placed on stack when "array.size" is compiled, but member access usually shifts pointer, that is dereferenced now
	// So in this special case, dereferencing is not done. Yeah...
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && lastType == typeVoid)
	{
		CodeInfo::nodeList.back()->typeInfo = typeInt;
	}else if(lastType->funcType == NULL && (lastType->refLevel != 0 || CodeInfo::nodeList.back()->nodeType != typeNodeFuncCall)){
		CheckForImmutable(lastType, pos);
		CodeInfo::nodeList.push_back(new NodeDereference());
	}
}

void AddMemberAccessNode(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	bool unifyTwo = false;
	// Get variable type
	TypeInfo *currentType = CodeInfo::nodeList.back()->typeInfo;
	// For member access, we expect to see a pointer to variable on top of the stack, so the shift to a member could be made
	// But if structure was returned from a function, then we have it on stack "by variable", so we save it to a hidden variable
	// And we take a pointer to that variable
	if(currentType->refLevel == 0 && currentType->type == TypeInfo::TYPE_COMPLEX)
	{
		AddInplaceVariable(pos);
		currentType = CodeInfo::GetReferenceType(currentType);
		unifyTwo = true;
	}
	CheckForImmutable(currentType, pos);

	// Get type after dereference
	currentType = currentType->subType;
	// If it's still a dereference, dereference it again (so that "var.member" will work if var is a pointer)
	if(currentType->refLevel == 1)
	{
		CodeInfo::nodeList.push_back(new NodeDereference());
		currentType = CodeInfo::GetDereferenceType(currentType);
	}
	// If it's an array, only member that can be accessed is .size
	if(currentType->arrLevel != 0 && currentType->arrSize != TypeInfo::UNSIZED_ARRAY)
	{
		if(hash != GetStringHash("size"))
			ThrowError(pos, "ERROR: Array doesn't have member with this name");
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(new NodeNumber((int)currentType->arrSize, typeVoid));
		currentType = typeInt;
		if(unifyTwo)
			AddExtraNode();
		return;
	}
 
	// Find member
	int fID = -1;
	TypeInfo::MemberVariable *curr = currentType->firstVariable;
	for(; curr; curr = curr->next)
		if(curr->nameHash == hash)
			break;
	// If member variable is not found, try searching for member function
	if(!curr)
	{
		// Construct function name in a for of Class::Function
		unsigned int hash = currentType->nameHash;
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, varName.begin, varName.end);

		// Search for it
		fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);
		if(fID == -1)
		{
			// Try calling a "get" function
			int memberNameLength = int(varName.end - varName.begin);
			char *memberGet = AllocateString(memberNameLength + 2);
			memcpy(memberGet, varName.begin, memberNameLength);
			memberGet[memberNameLength] = '$';
			memberGet[memberNameLength+1] = 0;
			char *memberFuncName = GetClassFunctionName(currentType, memberGet);
			if(AddFunctionCallNode(pos, memberFuncName, 0, true))
			{
				if(unifyTwo)
					AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
				return;
			}
			ThrowError(pos, "ERROR: member variable or function '%.*s' is not defined in class '%s'", varName.end-varName.begin, varName.begin, currentType->GetFullTypeName());
		}
		if(CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
			ThrowError(pos, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);
	}
	
	// In case of a variable
	if(fID == -1)
	{
		// Shift pointer to member
		if(CodeInfo::nodeList.back()->nodeType == typeNodeGetAddress)
			static_cast<NodeGetAddress*>(CodeInfo::nodeList.back())->ShiftToMember(curr);
		else
			CodeInfo::nodeList.push_back(new NodeShiftAddress(curr->offset, curr->type));
	}else{
		// In case of a function, get it's address
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(CodeInfo::funcInfo[fID]));
	}

	if(unifyTwo)
		AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
}

void PrepareMemberCall(const char* pos)
{
	TypeInfo *currentType = CodeInfo::nodeList.back()->typeInfo;

	// Implicit conversion of type ref ref to type ref
	if(currentType->refLevel == 2)
	{
		CodeInfo::nodeList.push_back(new NodeDereference());
		return;
	}
	// Implicit conversion from type[N] ref to type[]
	if(currentType->refLevel == 1 && currentType->subType->arrLevel && currentType->subType->arrSize != TypeInfo::UNSIZED_ARRAY)
	{
		CodeInfo::nodeList.push_back(new NodeDereference());
		currentType = CodeInfo::nodeList.back()->typeInfo;
	}
	// Implicit conversion from type to type ref
	if(currentType->refLevel == 0 && currentType->type == TypeInfo::TYPE_COMPLEX){
		// And from type[] to type[] ref
		if(currentType->arrLevel != 0 && currentType->arrSize != TypeInfo::UNSIZED_ARRAY)
			ConvertArrayToUnsized(pos, CodeInfo::GetArrayType(currentType->subType, TypeInfo::UNSIZED_ARRAY));

		AddInplaceVariable(pos);
		AddExtraNode();
	}

}

void AddMemberFunctionCall(const char* pos, const char* funcName, unsigned int callArgCount)
{
	// Check if type has any member functions
	TypeInfo *currentType = CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount-1]->typeInfo;
	CheckForImmutable(currentType, pos);
	// Construct name in a form of Class::Function
	char *memberFuncName = GetClassFunctionName(currentType->subType, funcName);
	// Call it
	AddFunctionCallNode(pos, memberFuncName, callArgCount);
}

void AddPreOrPostOpNode(const char* pos, bool isInc, bool prefixOp)
{
	NodeZeroOP *pointer = CodeInfo::nodeList.back();
	CheckForImmutable(pointer->typeInfo, pos);

	// For indexes that aren't known at compile-time
	if(pointer->nodeType == typeNodeArrayIndex && !((NodeArrayIndex*)pointer)->knownShift)
	{
		// Create variable that will hold calculated pointer
		AddInplaceVariable(pos);
		// Get pointer from it
		AddDereferenceNode(pos);
		CodeInfo::nodeList.push_back(new NodePreOrPostOp(isInc, prefixOp));
		AddExtraNode();
	}else{
		CodeInfo::nodeList.push_back(new NodePreOrPostOp(isInc, prefixOp));
	}
}

void AddModifyVariableNode(const char* pos, CmdID cmd)
{
	CodeInfo::lastKnownStartPos = pos;

	// Only five operators
	assert(cmd - cmdAdd < 5);
	// Operator names
	const char *opNames[] = { "+=", "-=", "*=", "/=", "**=" };
	// Call overloaded operator with error suppression
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, opNames[cmd - cmdAdd], 2, true))
		return;

	NodeZeroOP *pointer = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
	NodeZeroOP *value = CodeInfo::nodeList.back();
	// Add node for variable modification
	CheckForImmutable(pointer->typeInfo, pos);
	// For indexes that aren't known at compile-time
	if(pointer->nodeType == typeNodeArrayIndex && !((NodeArrayIndex*)pointer)->knownShift)
	{
		// Remove value
		CodeInfo::nodeList.pop_back();
		// Create variable that will hold calculated pointer
		AddInplaceVariable(pos);
		// Get pointer from it
		AddDereferenceNode(pos);
		// Restore value
		CodeInfo::nodeList.push_back(value);
		CodeInfo::nodeList.push_back(new NodeVariableModify(pointer->typeInfo, cmd));
		AddExtraNode();
	}else{
		CodeInfo::nodeList.push_back(new NodeVariableModify(pointer->typeInfo, cmd));
	}
}

void AddInplaceVariable(const char* pos)
{
	char	*arrName = AllocateString(16);
	int length = sprintf(arrName, "$temp%d", inplaceVariableNum++);

	// Save variable creation state
	TypeInfo *saveCurrType = currType;
	bool saveVarDefined = varDefined;

	// Set type to auto
	currType = NULL;
	// Add hidden variable
	AddVariable(pos, InplaceStr(arrName, length));
	// Set it to value on top of the stack
	AddDefineVariableNode(pos, InplaceStr(arrName, length), true);
	// Remove it from stack
	AddPopNode(pos);
	// Put pointer to hidden variable on stack
	AddGetAddressNode(pos, InplaceStr(arrName, length));

	// Restore variable creation state
	varDefined = saveVarDefined;
	currType = saveCurrType;
}

void ConvertArrayToUnsized(const char* pos, TypeInfo *dstType)
{
	TypeInfo *nodeType = CodeInfo::nodeList.back()->typeInfo;
	if(dstType->arrSize != TypeInfo::UNSIZED_ARRAY || dstType == nodeType)
		return;
	if(dstType->subType == nodeType->subType)
	{
		bool hasImplicitNode = false;
		if(CodeInfo::nodeList.back()->nodeType != typeNodeDereference)
		{
			AddInplaceVariable(pos);
			hasImplicitNode = true;
		}else{
			NodeZeroOP	*oldNode = CodeInfo::nodeList.back();
			CodeInfo::nodeList.back() = static_cast<NodeDereference*>(oldNode)->GetFirstNode();
			static_cast<NodeDereference*>(oldNode)->SetFirstNode(NULL);
		}
		
		TypeInfo *inplaceType = CodeInfo::nodeList.back()->typeInfo;
		assert(inplaceType->refLevel == 1);
		assert(inplaceType->subType->arrLevel != 0);

		NodeExpressionList *listExpr = new NodeExpressionList(dstType);
		// Create node that places array size on top of the stack
		CodeInfo::nodeList.push_back(new NodeNumber((int)inplaceType->subType->arrSize, typeInt));
		// Add it to expression list
		listExpr->AddNode();
		if(hasImplicitNode)
			listExpr->AddNode();
		// Add expression list to node list
		CodeInfo::nodeList.push_back(listExpr);
	}
}

void ConvertFunctionToPointer(const char* pos)
{
	// If node is a function definition or a pair of { function definition, function closure setup }
	if(CodeInfo::nodeList.back()->nodeType == typeNodeFuncDef)
	{
		// Take it's address and hide it's name
		NodeFuncDef*	funcDefNode = (NodeFuncDef*)CodeInfo::nodeList.back();
		AddGetAddressNode(pos, InplaceStr(funcDefNode->GetFuncInfo()->name, funcDefNode->GetFuncInfo()->nameLength), true);

		AddExtraNode();

		funcDefNode->GetFuncInfo()->visible = false;
	}
}

void HandlePointerToObject(const char* pos, TypeInfo *dstType)
{
	TypeInfo *srcType = CodeInfo::nodeList.back()->typeInfo;
	if(!((dstType == typeObject) ^ (srcType == typeObject)))
		return;

	CheckForImmutable(dstType == typeObject ? srcType : dstType, pos);
	CodeInfo::nodeList.push_back(new NodeConvertPtr(dstType == typeObject ? typeObject : dstType));
}

//////////////////////////////////////////////////////////////////////////
void AddOneExpressionNode(void *retType)
{
	CodeInfo::nodeList.push_back(new NodeExpressionList(retType ? (TypeInfo*)retType : typeVoid));
}

void AddTwoExpressionNode(void *retType)
{
	if(CodeInfo::nodeList.back()->nodeType != typeNodeExpressionList)
		AddOneExpressionNode(retType);
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
		ThrowError(pos, "ERROR: array cannot be constructed from void type elements");

	CodeInfo::nodeList.push_back(new NodeZeroOP());
	TypeInfo *targetType = CodeInfo::GetArrayType(currentType, arrElementCount);

	NodeExpressionList *arrayList = new NodeExpressionList(targetType);

	for(unsigned int i = 0; i < arrElementCount; i++)
	{
		TypeInfo *realType = CodeInfo::nodeList.back()->typeInfo;
		if(realType != currentType &&
			!((realType == typeShort || realType == typeChar) && currentType == typeInt) &&
			!((realType == typeShort || realType == typeChar || realType == typeInt || realType == typeFloat) && currentType == typeDouble))
				ThrowError(pos, "ERROR: element %d doesn't match the type of element 0 (%s)", arrElementCount-i-1, currentType->GetFullTypeName());
		if((realType == typeShort || realType == typeChar || realType == typeInt) && currentType == typeDouble)
			AddFunctionCallNode(pos, "double", 1);
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
			ThrowError(pos, "ERROR: Name '%s' is already taken for a variable in current scope", funcName);
	}
	char *funcNameCopy = (char*)funcName;
	if(newType)
		funcNameCopy = GetClassFunctionName(newType, funcName);
	funcNameHash = GetStringHash(funcNameCopy);

	CodeInfo::funcInfo.push_back(new FunctionInfo(funcNameCopy));
	FunctionInfo* lastFunc = CodeInfo::funcInfo.back();

	AddFunctionToSortedList(lastFunc);

	lastFunc->indexInArr = CodeInfo::funcInfo.size() - 1;
	lastFunc->vTopSize = (unsigned int)varInfoTop.size();
	lastFunc->retType = currType;
	if(newType)
	{
		lastFunc->type = FunctionInfo::THISCALL;
		lastFunc->parentClass = newType;
	}
	if(newType ? varInfoTop.size() > 2 : varInfoTop.size() > 1)
		lastFunc->type = FunctionInfo::LOCAL;
	if(funcName[0] != '$' && !(chartype_table[funcName[0]] & ct_start_symbol))
		lastFunc->visible = false;
	currDefinedFunc.push_back(lastFunc);
}

void FunctionParameter(const char* pos, InplaceStr paramName)
{
	if(currType == typeVoid)
		ThrowError(pos, "ERROR: function parameter cannot be a void type");
	unsigned int hash = GetStringHash(paramName.begin, paramName.end);
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	if(lastFunc.lastParam && !lastFunc.lastParam->varType)
		ThrowError(pos, "ERROR: function parameter cannot be an auto type");

	lastFunc.AddParameter(new VariableInfo(paramName, hash, lastFunc.allParamSize, currType, false));
	if(currType)
		lastFunc.allParamSize += currType->size < 4 ? (currType->size ? 4 : 0) : currType->size;
}

void FunctionParameterDefault(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	ConvertFunctionToPointer(pos);

	TypeInfo *left = lastFunc.lastParam->varType;
	TypeInfo *right = CodeInfo::nodeList.back()->typeInfo;

	if(!lastFunc.lastParam->varType)
	{
		left = lastFunc.lastParam->varType = right;
		lastFunc.allParamSize += right->size < 4 ? (right->size ? 4 : 0) : right->size;
	}
	if(right == typeVoid)
		ThrowError(pos, "ERROR: Cannot convert from void to %s", left->GetFullTypeName());
	
	// If types don't match and it it is not build-in basic types or if pointers point to different types
	if(left != right && (left->type == TypeInfo::TYPE_COMPLEX || right->type == TypeInfo::TYPE_COMPLEX || left->subType != right->subType))
		ThrowError(pos, "ERROR: Cannot convert from '%s' to '%s'", right->GetFullTypeName(), left->GetFullTypeName());

	lastFunc.lastParam->defaultValue = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
}

void FunctionPrototype(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	if(!lastFunc.retType)
		ThrowError(pos, "ERROR: function prototype with unresolved return type");
	lastFunc.funcType = CodeInfo::GetFunctionType(lastFunc.retType, lastFunc.firstParam, lastFunc.paramCount);
	currDefinedFunc.pop_back();
	if(newType && lastFunc.type == FunctionInfo::THISCALL)
		methodCount++;
}

void FunctionStart(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	if(lastFunc.lastParam && !lastFunc.lastParam->varType)
		ThrowError(pos, "ERROR: function parameter cannot be an auto type");

	lastFunc.implemented = true;
	lastFunc.funcType = lastFunc.retType ? CodeInfo::GetFunctionType(lastFunc.retType, lastFunc.firstParam, lastFunc.paramCount) : NULL;

	BeginBlock();
	cycleDepth.push_back(0);

	varTop = 0;

	for(VariableInfo *curr = lastFunc.firstParam; curr; curr = curr->next)
	{
		currType = curr->varType;
		currAlign = 4;
		AddVariable(pos, curr->name);
		varDefined = false;
	}

	char	*hiddenHame = AllocateString(lastFunc.nameLength + 24);
	int length = 0;
	if(lastFunc.type == FunctionInfo::THISCALL)
	{
		memcpy(hiddenHame, "this", 5);
		length = 4;
	}else{
		length = sprintf(hiddenHame, "$%s_%p_ext", lastFunc.name, &lastFunc);
	}
	currType = CodeInfo::GetReferenceType(lastFunc.type == FunctionInfo::THISCALL ? lastFunc.parentClass : typeInt);
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
		funcNameHash = newType->GetFullNameHash();
		funcNameHash = StringHashContinue(funcNameHash, "::");
		funcNameHash = StringHashContinue(funcNameHash, funcName);
	}

	unsigned int index = FindFirstFunctionIndex(funcNameHash);
	while(index < sortedFunc.size() && sortedFunc[index]->nameHash == funcNameHash)
	{
		if(sortedFunc[index] == &lastFunc)
		{
			index++;
			continue;
		}

		if(sortedFunc[index]->paramCount == lastFunc.paramCount && sortedFunc[index]->visible && sortedFunc[index]->retType == lastFunc.retType)
		{
			// Check all parameter types
			bool paramsEqual = true;
			for(VariableInfo *currN = sortedFunc[index]->firstParam, *currI = lastFunc.firstParam; currN; currN = currN->next, currI = currI->next)
			{
				if(currN->varType != currI->varType)
					paramsEqual = false;
			}
			if(paramsEqual)
			{
				if(sortedFunc[index]->implemented)
					ThrowError(pos, "ERROR: function '%s' is being defined with the same set of parameters", lastFunc.name);
				else
					sortedFunc[index]->address = lastFunc.indexInArr | 0x80000000;
			}
		}
		index++;
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

	unsigned int varFormerTop = varTop;
	varTop = varInfoTop[lastFunc.vTopSize].varStackSize;
	EndBlock(true, false);

	NodeZeroOP *funcNode = new NodeFuncDef(&lastFunc, varFormerTop);
	CodeInfo::nodeList.push_back(funcNode);
	funcNode->SetCodeInfo(pos);
	CodeInfo::funcDefList.push_back(funcNode);

	if(!currDefinedFunc.back()->retType)
	{
		currDefinedFunc.back()->retType = typeVoid;
		currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back()->retType, currDefinedFunc.back()->firstParam, currDefinedFunc.back()->paramCount);
	}
	currDefinedFunc.pop_back();

	// If function is local, create function parameters block
	if(lastFunc.type == FunctionInfo::LOCAL && lastFunc.externalCount != 0)
	{
		char	*hiddenHame = AllocateString(lastFunc.nameLength + 24);
		int length = sprintf(hiddenHame, "$%s_%p_ext", lastFunc.name, &lastFunc);

		TypeInfo *saveCurrType = currType;
		bool saveVarDefined = varDefined;

		// Create a pointer to array
		currType = NULL;
		AddVariable(pos, InplaceStr(hiddenHame, length));

		// Allocate array in dynamic memory
		CodeInfo::nodeList.push_back(new NodeNumber((int)(lastFunc.externalSize), typeInt));
		AddFunctionCallNode(pos, "__newS", 1);
		CodeInfo::nodeList.back()->typeInfo = CodeInfo::GetReferenceType(CodeInfo::GetArrayType(CodeInfo::GetReferenceType(typeInt), lastFunc.externalSize / 4));

		// Set it to pointer variable
		AddDefineVariableNode(pos, InplaceStr(hiddenHame, length));

		CodeInfo::nodeList.push_back(new NodeDereference(&lastFunc, currDefinedFunc.back()->allParamSize));

		for(FunctionInfo::ExternalInfo *curr = lastFunc.firstExternal; curr; curr = curr->next)
		{
			// Find in variable list
			int i = CodeInfo::FindVariableByName(curr->variable->nameHash);
			if(i == -1)
				ThrowError(pos, "Can't capture variable %.*s", curr->variable->name.end-curr->variable->name.begin, curr->variable->name.begin);

			// Find the function that scopes target variable
			FunctionInfo *parentFunc = currDefinedFunc.back();
			for(unsigned int k = 0; k < currDefinedFunc.size()-1; k++)
			{
				if(i >= (int)varInfoTop[currDefinedFunc[k]->vTopSize].activeVarCnt && i < (int)varInfoTop[currDefinedFunc[k+1]->vTopSize].activeVarCnt)
				{
					parentFunc = currDefinedFunc[k];
					break;
				}
			}

			// If variable is not in current scope, get it through closure
			if(currDefinedFunc.size() > 1 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) &&
				i >= (int)varInfoTop[currDefinedFunc[0]->vTopSize].activeVarCnt && i < (int)varInfoTop[currDefinedFunc.back()->vTopSize].activeVarCnt)
			{
				// Add variable name to the list of function external variables
				FunctionInfo::ExternalInfo *external = AddFunctionExternal(currDefinedFunc.back(), CodeInfo::varInfo[i]);

				curr->targetLocal = false;
				curr->targetPos = external->closurePos;
			}else{
				// Or get it from current scope
				curr->targetLocal = true;
				curr->targetPos = CodeInfo::varInfo[i]->pos;
			}
			curr->targetFunc = CodeInfo::FindFunctionByPtr(parentFunc);
			curr->targetDepth = curr->variable->blockDepth - parentFunc->vTopSize - 1;

			if(curr->variable->blockDepth - parentFunc->vTopSize > parentFunc->maxBlockDepth)
				parentFunc->maxBlockDepth = curr->variable->blockDepth - parentFunc->vTopSize;
			parentFunc->closeUpvals = true;
		}

		varDefined = saveVarDefined;
		currType = saveCurrType;
		funcNode->AddExtraNode();
	}

	if(newType && lastFunc.type == FunctionInfo::THISCALL)
		methodCount++;

	currType = lastFunc.retType;
}

void FunctionToOperator(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	if(lastFunc.paramCount != 2)
		ThrowError(pos, "ERROR: binary operator definition or overload must accept exactly two arguments");
	if(lastFunc.type != FunctionInfo::NORMAL)
		ThrowError(pos, "ERROR: binary operator definition or overload must be placed in global scope");
	lastFunc.visible = true;
}

unsigned int GetFunctionRating(FunctionType *currFunc, unsigned int callArgCount)
{
	if(currFunc->paramCount != callArgCount)
		return ~0u;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.

	unsigned int fRating = 0;
	for(unsigned int i = 0; i < currFunc->paramCount; i++)//TypeInfo *expectedType = currFunc->paramType; curr; curr = curr->next, n++)
	{
		NodeZeroOP* activeNode = CodeInfo::nodeList[CodeInfo::nodeList.size() - currFunc->paramCount + i];
		TypeInfo *paramType = activeNode->typeInfo;
		unsigned int	nodeType = activeNode->nodeType;
		TypeInfo *expectedType = currFunc->paramType[i];
		if(expectedType != paramType)
		{
			if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->arrSize != 0 && paramType->subType == expectedType->subType)
				fRating += 5;
			else if(expectedType->funcType != NULL && nodeType == typeNodeFuncDef)
				fRating += 5;
			else if(expectedType->refLevel == paramType->refLevel+1 && expectedType->subType == paramType)
				fRating += 5;
			else if(expectedType == typeObject)
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

bool AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount, bool silent)
{
	unsigned int funcNameHash = funcName ? GetStringHash(funcName) : ~0u;

	// Searching, if function name is actually a variable name (which means, either it is a pointer to function, or an error)
	int vID = CodeInfo::FindVariableByName(funcNameHash);

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	NodeZeroOP	*funcAddr = NULL;

	if(newType)
	{
		unsigned int hash = newType->nameHash;
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, funcName);

		unsigned int index = FindFirstFunctionIndex(hash);
		if(sortedFunc[index]->nameHash == hash)
		{
			funcNameHash = hash;
			vID = -1;

			AddGetAddressNode(pos, InplaceStr("this", 4));
			AddDereferenceNode(pos);
			funcAddr = CodeInfo::nodeList.back();
			CodeInfo::nodeList.pop_back();
		}
	}
	if(vID == -1 && funcName)
	{
		//Find all functions with given name
		bestFuncList.clear();

		unsigned int index = FindFirstFunctionIndex(funcNameHash);

		while(index < sortedFunc.size() && sortedFunc[index]->nameHash == funcNameHash)
		{
			if(sortedFunc[index]->visible && !((sortedFunc[index]->address & 0x80000000) && (sortedFunc[index]->address != -1)) && sortedFunc[index]->funcType)
				bestFuncList.push_back(sortedFunc[index]);
			index++;
		}
		unsigned int count = bestFuncList.size();
		if(count == 0)
		{
			if(silent)
				return false;
			ThrowError(pos, "ERROR: function '%s' is undefined", funcName);
		}
		// Find the best suited function
		bestFuncRating.resize(count);

		unsigned int minRating = ~0u;
		unsigned int minRatingIndex = ~0u;
		for(unsigned int k = 0; k < count; k++)
		{
			unsigned int argumentCount = callArgCount;
			// Act as if default parameter values were passed
			if(argumentCount < bestFuncList[k]->funcType->funcType->paramCount)
			{
				// Move to the last parameter
				VariableInfo *param = bestFuncList[k]->firstParam;
				for(unsigned int i = 0; i < argumentCount; i++)
					param = param->next;
				// While there are default values, put them
				while(param && param->defaultValue)
				{
					argumentCount++;
					CodeInfo::nodeList.push_back(param->defaultValue);
					param = param->next;
				}
			}
			bestFuncRating[k] = GetFunctionRating(bestFuncList[k]->funcType->funcType, argumentCount);
			if(bestFuncRating[k] < minRating)
			{
				minRating = bestFuncRating[k];
				minRatingIndex = k;
			}
			while(argumentCount > callArgCount)
			{
				CodeInfo::nodeList.pop_back();
				argumentCount--;
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
			ThrowError(pos, errTemp);
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
				ThrowError(pos, errTemp);
			}
		}

		fType = bestFuncList[minRatingIndex]->funcType->funcType;
		fInfo = bestFuncList[minRatingIndex];
	}else{
		if(funcName)
			AddGetAddressNode(pos, InplaceStr(funcName, (int)strlen(funcName)));
		AddGetVariableNode(pos);
		funcAddr = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		fType = funcAddr->typeInfo->funcType;
		if(!fType)
			ThrowError(pos, "ERROR: variable is not a pointer to function");
		if(callArgCount != fType->paramCount)
			ThrowError(pos, "ERROR: function expects %d argument(s), while %d are supplied", fType->paramCount, callArgCount);
		if(GetFunctionRating(fType, callArgCount) == ~0u)
			ThrowError(pos, "ERROR: there is no conversion from specified arguments and the ones that function accepts");
	}

	if(callArgCount < fType->paramCount)
	{
		// Move to the last parameter
		VariableInfo *param = fInfo->firstParam;
		for(unsigned int i = 0; i < callArgCount; i++)
			param = param->next;
		// While there are default values, put them
		while(param && param->defaultValue)
		{
			callArgCount++;
			NodeOneOP *wrap = new NodeOneOP();
			wrap->SetFirstNode(param->defaultValue);
			CodeInfo::nodeList.push_back(wrap);
			param = param->next;
		}
	}

	paramNodes.clear();
	for(unsigned int i = 0; i < callArgCount; i++)
	{
		paramNodes.push_back(CodeInfo::nodeList.back());
		CodeInfo::nodeList.pop_back();
	}

	if(funcAddr && newType)
	{
		CodeInfo::nodeList.push_back(funcAddr);
		funcAddr = NULL;
	}
	for(unsigned int i = 0; i < fType->paramCount; i++)
	{
		unsigned int index = fType->paramCount - i - 1;

		CodeInfo::nodeList.push_back(paramNodes[index]);

		ConvertFunctionToPointer(pos);
		ConvertArrayToUnsized(pos, fType->paramType[i]);
		if((fType->paramType[i]->refLevel == CodeInfo::nodeList.back()->typeInfo->refLevel+1 && fType->paramType[i]->subType == CodeInfo::nodeList.back()->typeInfo) ||
			(CodeInfo::nodeList.back()->typeInfo->refLevel == 0 && fType->paramType[i] == typeObject))
		{
			if(CodeInfo::nodeList.back()->nodeType == typeNodeDereference)
			{
				((NodeDereference*)CodeInfo::nodeList.back())->Neutralize();
			}else{
				AddInplaceVariable(pos);
				AddExtraNode();
			}
		}
		HandlePointerToObject(pos, fType->paramType[i]);
	}

	if(fInfo && (fInfo->type == FunctionInfo::LOCAL))
	{
		char	*contextName = AllocateString(fInfo->nameLength + 24);
		int length = sprintf(contextName, "$%s_%p_ext", fInfo->name, fInfo);
		unsigned int contextHash = GetStringHash(contextName);

		int i = CodeInfo::FindVariableByName(contextHash);
		if(i == -1)
		{
			CodeInfo::nodeList.push_back(new NodeNumber(0, CodeInfo::GetReferenceType(typeInt)));
		}else{
			AddGetAddressNode(pos, InplaceStr(contextName, length));
			AddGetVariableNode(pos);
		}
	}

	if(funcAddr)
		CodeInfo::nodeList.push_back(funcAddr);
	CodeInfo::nodeList.push_back(new NodeFuncCall(fInfo, fType));

	return true;
}

void AddIfNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 2);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError(pos, "ERROR: condition type cannot be void");
	// If condition is constant
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->nodeType == typeNodeNumber)
	{
		int condition = ((NodeNumber*)CodeInfo::nodeList[CodeInfo::nodeList.size()-2])->GetInteger();
		NodeZeroOP *remainingNode = condition ? CodeInfo::nodeList.back() : (new NodeZeroOP);
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.back() = remainingNode;
		return;
	}
	CodeInfo::nodeList.push_back(new NodeIfElseExpr(false));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}
void AddIfElseNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 3);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-3]->typeInfo == typeVoid)
		ThrowError(pos, "ERROR: condition type cannot be void");
	// If condition is constant
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-3]->nodeType == typeNodeNumber)
	{
		int condition = ((NodeNumber*)CodeInfo::nodeList[CodeInfo::nodeList.size()-3])->GetInteger();
		NodeZeroOP *remainingNode = condition ? CodeInfo::nodeList[CodeInfo::nodeList.size()-2] : CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.back() = remainingNode;
		return;
	}
	CodeInfo::nodeList.push_back(new NodeIfElseExpr(true));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}
void AddIfElseTermNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 3);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-3]->typeInfo == typeVoid)
		ThrowError(pos, "ERROR: condition type cannot be void");
	TypeInfo* typeA = CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo;
	TypeInfo* typeB = CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo;
	if(typeA == typeVoid || typeB == typeVoid)
		ThrowError(pos, "ERROR: one of ternary operator ?: result type is void (%s : %s)", typeB->name, typeA->name);
	if(typeA != typeB && (typeA->type == TypeInfo::TYPE_COMPLEX || typeB->type == TypeInfo::TYPE_COMPLEX))
		ThrowError(pos, "ERROR: ternary operator ?: result types are not equal (%s : %s)", typeB->name, typeA->name);
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
		ThrowError(pos, "ERROR: condition type cannot be void");
	CodeInfo::nodeList.push_back(new NodeForExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}
void AddWhileNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 2);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError(pos, "ERROR: condition type cannot be void");
	CodeInfo::nodeList.push_back(new NodeWhileExpr());
	CodeInfo::nodeList.back()->SetCodeInfo(pos);

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}
void AddDoWhileNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 2);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo == typeVoid)
		ThrowError(pos, "ERROR: condition type cannot be void");
	CodeInfo::nodeList.push_back(new NodeDoWhileExpr());
	CodeInfo::nodeList.back()->SetCodeInfo(pos);

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}

void BeginSwitch(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 1);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo == typeVoid)
		ThrowError(pos, "ERROR: cannot switch by void type");

	assert(cycleDepth.size() != 0);
	cycleDepth.back()++;

	BeginBlock();
	CodeInfo::nodeList.push_back(new NodeSwitchExpr());
}

void AddCaseNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 3);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError(pos, "ERROR: case value type cannot be void");
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
		ThrowError(pos, "ERROR: Different type is being defined");
	if(currAlign > 16)
		ThrowError(pos, "ERROR: alignment must be less than 16 bytes");

	char *typeNameCopy = AllocateString((int)(end - pos) + 1);
	sprintf(typeNameCopy, "%.*s", (int)(end - pos), pos);

	unsigned int hash = GetStringHash(typeNameCopy);
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		if(CodeInfo::typeInfo[i]->nameHash == hash)
			ThrowError(pos, "ERROR: '%s' is being redefined", typeNameCopy);
	}
	newType = new TypeInfo(CodeInfo::classCount, typeNameCopy, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	newType->alignBytes = currAlign;
	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;
	methodCount = 0;

	if(CodeInfo::classCount == CodeInfo::typeInfo.size())
	{
		CodeInfo::typeInfo.push_back(newType);
	}else{
		CodeInfo::typeInfo[CodeInfo::classCount]->typeIndex = CodeInfo::typeInfo.size();
		CodeInfo::typeInfo.push_back(CodeInfo::typeInfo[CodeInfo::classCount]);
		CodeInfo::typeInfo[CodeInfo::classCount] = newType;
	}
	
	CodeInfo::classCount++;

	BeginBlock();
}

void TypeAddMember(const char* pos, const char* varName)
{
	if(!currType)
		ThrowError(pos, "ERROR: auto cannot be used for class members");
	newType->AddMemberVariable(varName, currType);
	if(newType->size > 64 * 1024)
		ThrowError(pos, "ERROR: class size cannot exceed 65535 bytes");

	AddVariable(pos, InplaceStr(varName, (int)strlen(varName)));
}

void TypeFinish()
{
	varTop -= newType->size;
	if(newType->size % 4 != 0)
	{
		newType->paddingBytes = 4 - (newType->size % 4);
		newType->size += 4 - (newType->size % 4);
	}

	CodeInfo::nodeList.push_back(new NodeZeroOP());
	for(unsigned int i = 0; i < methodCount; i++)
		AddTwoExpressionNode();

	newType = NULL;

	EndBlock(false, false);
}

void TypeContinue(const char* pos)
{
	newType = currType;
	BeginBlock();
	for(TypeInfo::MemberVariable *curr = newType->firstVariable; curr; curr = curr->next)
	{
		currType = curr->type;
		currAlign = 4;
		AddVariable(pos, InplaceStr(curr->name));
		varDefined = false;
	}
}

void TypeStop()
{
	varTop = varInfoTop.back().varStackSize;
	newType = NULL;
	EndBlock(false, false);
}

void AddAliasType(InplaceStr aliasName)
{
	AliasInfo info;
	info.targetType = currType;
	info.name = aliasName;
	info.nameHash = GetStringHash(aliasName.begin, aliasName.end);;
	CodeInfo::aliasInfo.push_back(info);
}

void AddUnfixedArraySize()
{
	CodeInfo::nodeList.push_back(new NodeNumber(1, typeVoid));
}

void RestoreScopedGlobals()
{
	while(lostGlobalList)
	{
		CodeInfo::varInfo.push_back(lostGlobalList);
		lostGlobalList = lostGlobalList->next;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CallbackInitialize()
{
	currDefinedFunc.clear();

	CodeInfo::funcDefList.clear();

	varDefined = 0;
	varTop = 0;
	newType = NULL;

	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;
	inplaceVariableNum = 1;

	varInfoTop.clear();
	varInfoTop.push_back(VarTopInfo(0,0));

	funcInfoTop.clear();
	funcInfoTop.push_back(0);

	cycleDepth.clear();
	cycleDepth.push_back(0);

	lostGlobalList = NULL;

	sortedFunc.clear();

	for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		AddFunctionToSortedList(CodeInfo::funcInfo[i]);
}

unsigned int GetGlobalSize()
{
	return varTop;
}

void SetGlobalSize(unsigned int offset)
{
	varTop = offset;
}

void CallbackReset()
{
	varInfoTop.reset();
	funcInfoTop.reset();
	cycleDepth.reset();
	currDefinedFunc.reset();
	bestFuncList.reset();
	bestFuncRating.reset();
	paramNodes.reset();

	TypeInfo::typeInfoPool.~ChunkedStackPool();
	TypeInfo::SetPoolTop(0);
	VariableInfo::variablePool.~ChunkedStackPool();
	VariableInfo::SetPoolTop(0);
	FunctionInfo::functionPool.~ChunkedStackPool();
	FunctionInfo::SetPoolTop(0);
}
