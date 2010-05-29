// NULLC (c) NULL_PTR 2007-2010

#include "Callbacks.h"
#include "CodeInfo.h"

#include "Parser.h"
#include "HashMap.h"

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

VariableInfo	*vtblList = NULL;

// Stack of function counts.
// Used to find how many functions are to be removed when their visibility ends.
FastVector<unsigned int>	funcInfoTop;

// Cycle depth stack is used to determine what is the depth of the loop when compiling operators break and continue
FastVector<unsigned int>	cycleDepth;

// Stack of functions that are being defined at the moment
FastVector<FunctionInfo*>	currDefinedFunc;

HashMap<FunctionInfo*>		funcMap;
HashMap<VariableInfo*>		varMap;

void	AddFunctionToSortedList(void *info)
{
	funcMap.insert(((FunctionInfo*)info)->nameHash, (FunctionInfo*)info);
}

// Information about current type
TypeInfo*		currType = NULL;
TypeInfo*		typeObjectArray = NULL;

// For new type creation
TypeInfo*		newType = NULL;
unsigned int	methodCount = 0;

ChunkedStackPool<65532> TypeInfo::typeInfoPool;
ChunkedStackPool<65532> VariableInfo::variablePool;
ChunkedStackPool<65532> FunctionInfo::functionPool;

FastVector<FunctionInfo*>	bestFuncList;
FastVector<unsigned int>	bestFuncRating;

FastVector<NodeZeroOP*>	paramNodes;

void AddInplaceVariable(const char* pos, TypeInfo* targetType = NULL);
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
	var->usedAsExternal = true;
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
		if(digit < 0 || digit >= base)
			ThrowError(p, "ERROR: digit %d is not allowed in base %d", digit, base);
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
			if(!lastFunc.lastLocal)
				lastFunc.lastLocal = lastFunc.firstLocal;
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

	// Remove variable information from hash map
	for(int i = (int)CodeInfo::varInfo.size() - 1; i >= (int)varInfoTop.back().activeVarCnt; i--)
		varMap.remove(CodeInfo::varInfo[i]->nameHash, CodeInfo::varInfo[i]);
	// Remove variable information from array
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
		ThrowError(pos, "ERROR: overflow in hexadecimal constant");
	// If number overflows integer number, create long number
	if(int(end - pos) < 8)
		CodeInfo::nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 16), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 16), typeLong));
}

void AddOctInteger(const char* pos, const char* end)
{
	pos++;
	if(int(end - pos) > 21)
		ThrowError(pos, "ERROR: overflow in octal constant");
	// If number overflows integer number, create long number
	if(int(end - pos) <= 10)
		CodeInfo::nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 8), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 8), typeLong));
}

void AddBinInteger(const char* pos, const char* end)
{
	if(int(end - pos) > 64)
		ThrowError(pos, "ERROR: overflow in binary constant");
	// If number overflows integer number, create long number
	if(int(end - pos) < 32)
		CodeInfo::nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 2), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(parseLong(pos, end, 2), typeLong));
}

void AddVoidNode()
{
	CodeInfo::nodeList.push_back(new NodeZeroOP());
}

void AddNullPointer()
{
	CodeInfo::nodeList.push_back(new NodeNumber(0, CodeInfo::GetReferenceType(typeVoid)));
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

void AddPositiveNode(const char* pos)
{
	AddFunctionCallNode(pos, "+", 1, true);
}

// Function that creates unary operation node that changes sign of value
void AddNegateNode(const char* pos)
{
	if(AddFunctionCallNode(pos, "-", 1, true))
		return;

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
	if(AddFunctionCallNode(pos, "!", 1, true))
		return;

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
	if(AddFunctionCallNode(pos, "~", 1, true))
		return;

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
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: division by zero during constant folding");
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
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: modulus division by zero during constant folding");
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
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: modulus division by zero during constant folding");
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

	if(id == cmdEqual || id == cmdNEqual)
	{
		NodeZeroOP *left = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
		NodeZeroOP *right = CodeInfo::nodeList[CodeInfo::nodeList.size()-1];

		if(right->typeInfo == typeObject && right->typeInfo == left->typeInfo)
		{
			CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo = typeLong;
			CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo = typeLong;
			CodeInfo::nodeList.push_back(new NodeBinaryOp(id));
			return;
		}
		if(right->typeInfo->funcType && right->typeInfo->funcType == left->typeInfo->funcType)
		{
			CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo = CodeInfo::funcInfo[0]->funcType;
			CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo = CodeInfo::funcInfo[0]->funcType;
			AddFunctionCallNode(pos, id == cmdEqual ? "__pcomp" : "__pncomp", 2);
			return;
		}
	}
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
		}else{
			ConvertArrayToUnsized(pos, currDefinedFunc.back()->retType);
			HandlePointerToObject(pos, currDefinedFunc.back()->retType);
			realRetType = CodeInfo::nodeList.back()->typeInfo;
		}
		// Take expected return type
		expectedType = currDefinedFunc.back()->retType;
		// Check for errors
		if(((expectedType->type == TypeInfo::TYPE_COMPLEX || realRetType->type == TypeInfo::TYPE_COMPLEX) && expectedType != realRetType) || expectedType->subType != realRetType->subType)
			ThrowError(pos, "ERROR: function returns %s but supposed to return %s", realRetType->GetFullTypeName(), expectedType->GetFullTypeName());
		if(expectedType == typeVoid && realRetType != typeVoid)
			ThrowError(pos, "ERROR: 'void' function returning a value");
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
		ThrowError(pos, "ERROR: break statement must be followed by ';' or a constant");

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
		ThrowError(pos, "ERROR: continue statement must be followed by ';' or a constant");

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

const char* GetSelectedTypeName()
{
	return currType->GetFullTypeName();
}

void* AddVariable(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	// Check for variables with the same name in current scope
	if(VariableInfo ** info = varMap.find(hash))
		if((*info)->blockDepth >= varInfoTop.size())
			ThrowError(pos, "ERROR: name '%.*s' is already taken for a variable in current scope", varName.end-varName.begin, varName.begin);
	// Check for functions with the same name
	HashMap<FunctionInfo*>::Node *curr = funcMap.first(hash);
	while(curr)
	{
		if(curr->value->visible)
			ThrowError(pos, "ERROR: name '%.*s' is already taken for a function", varName.end-varName.begin, varName.begin);
		curr = funcMap.next(curr);
	}

	if((currType && currType->alignBytes != 0) || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
	{
		unsigned int offset = GetAlignmentOffset(pos, currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : currType->alignBytes);
		varTop += offset;
	}
	CodeInfo::varInfo.push_back(new VariableInfo(currDefinedFunc.size() > 0 ? currDefinedFunc.back() : NULL, varName, hash, varTop, currType, currDefinedFunc.size() == 0));
	varDefined = true;
	CodeInfo::varInfo.back()->blockDepth = varInfoTop.size();
	if(currType)
		varTop += currType->size;
	if(varTop > (1 << 24))
		ThrowError(pos, "ERROR: global variable size limit exceeded");
	varMap.insert(hash, CodeInfo::varInfo.back());
	return CodeInfo::varInfo.back();
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

void GetTypeId()
{
	CodeInfo::nodeList.push_back(new NodeZeroOP(CodeInfo::GetReferenceType(currType)));
	CodeInfo::nodeList.push_back(new NodeConvertPtr(typeObject));
	CodeInfo::nodeList.back()->typeInfo = typeTypeid;
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
	VariableInfo **info = varMap.find(hash);
	if(!info)
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
			int length = sprintf(contextName, "$%s_%d_ext", CodeInfo::funcInfo[fID]->name, CodeInfo::FindFunctionByPtr(CodeInfo::funcInfo[fID]));
			unsigned int contextHash = GetStringHash(contextName);

			int i = CodeInfo::FindVariableByName(contextHash);
			if(i == -1)
			{
				CodeInfo::nodeList.push_back(new NodeNumber(0, CodeInfo::GetReferenceType(typeInt)));
			}else{
				AddGetAddressNode(pos, InplaceStr(contextName, length));
				CodeInfo::nodeList.push_back(new NodeDereference());
			}
		}else if(CodeInfo::funcInfo[fID]->type == FunctionInfo::THISCALL){
			AddGetAddressNode(pos, InplaceStr("this", 4));
			CodeInfo::nodeList.push_back(new NodeDereference());
		}

		// Create node that retrieves function address
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(CodeInfo::funcInfo[fID]));
	}else{
		VariableInfo *vInfo = *info;
		if(!vInfo->varType)
			ThrowError(pos, "ERROR: variable '%.*s' is being used while its type is unknown", varName.end-varName.begin, varName.begin);
		if(newType && currDefinedFunc.back()->type == FunctionInfo::THISCALL && vInfo->isGlobal)
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
				CodeInfo::nodeList.push_back(new NodeDereference());
				CodeInfo::nodeList.push_back(new NodeShiftAddress(curr));
				return;
			}
		}

		// If we try to access external variable from local function
		if(currDefinedFunc.size() > 1 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) &&
			vInfo->blockDepth > currDefinedFunc[0]->vTopSize && vInfo->blockDepth <= currDefinedFunc.back()->vTopSize)
		{
			// If that variable is not in current scope, we have to get it through current closure
			FunctionInfo *currFunc = currDefinedFunc.back();
			// Add variable to the list of function external variables
			FunctionInfo::ExternalInfo *external = AddFunctionExternal(currFunc, vInfo);

			assert(currFunc->allParamSize % 4 == 0);
			CodeInfo::nodeList.push_back(new NodeGetUpvalue(currFunc, currFunc->allParamSize, external->closurePos, CodeInfo::GetReferenceType(vInfo->varType)));
		}else{
			// Create node that places variable address on stack
			CodeInfo::nodeList.push_back(new NodeGetAddress(vInfo, vInfo->pos, vInfo->isGlobal, vInfo->varType));
			if(vInfo->autoDeref)
				AddGetVariableNode(pos);
		}
	}
}

// Function for array indexing
void AddArrayIndexNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->refLevel == 2)
	{
		NodeZeroOP *index = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(new NodeDereference());
		CodeInfo::nodeList.push_back(index);
	}
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
		ThrowError(pos, "ERROR: indexing variable that is not an array (%s)", currentType->GetFullTypeName());
	// Get result type
	currentType = CodeInfo::GetDereferenceType(currentType);

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
		ThrowError(pos, "ERROR: indexing variable that is not an array (%s)", currentType->GetFullTypeName());

#ifndef NULLC_ENABLE_C_TRANSLATION
	// If index is a number and previous node is an address, then indexing can be done in compile-time
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->nodeType == typeNodeGetAddress)
	{
		// Get shift value
		int shiftValue = static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger();

		// Check bounds
		if(shiftValue < 0)
			ThrowError(pos, "ERROR: array index cannot be negative");
		if((unsigned int)shiftValue >= currentType->arrSize)
			ThrowError(pos, "ERROR: array index out of bounds");

		// Index array
		static_cast<NodeGetAddress*>(CodeInfo::nodeList[CodeInfo::nodeList.size()-2])->IndexArray(shiftValue);
		CodeInfo::nodeList.pop_back();
	}else{
		// Otherwise, create array indexing node
		CodeInfo::nodeList.push_back(new NodeArrayIndex(currentType));
	}
#else
	CodeInfo::nodeList.push_back(new NodeArrayIndex(currentType));
#endif
	if(unifyTwo)
		AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
}

// Function for variable assignment in place of definition
void AddDefineVariableNode(const char* pos, void* varInfo, bool noOverload)
{
	CodeInfo::lastKnownStartPos = pos;
	VariableInfo *variableInfo = (VariableInfo*)varInfo;

	// If a function is being assigned to variable, then take it's address
	ConvertFunctionToPointer(pos);

	// If current type is set to NULL, it means that current type is auto
	// Is such case, type is retrieved from last AST node
	TypeInfo *realCurrType = variableInfo->varType ? variableInfo->varType : CodeInfo::nodeList.back()->typeInfo;

	// If variable type is array without explicit size, and it is being defined with value of different type
	ConvertArrayToUnsized(pos, realCurrType);
	HandlePointerToObject(pos, realCurrType);

	// If type wasn't known until assignment, it means that variable alignment wasn't performed in AddVariable function
	if(!variableInfo->varType)
	{
		variableInfo->pos = varTop;
		// If type has default alignment or if user specified it
		if(realCurrType->alignBytes != 0 || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
		{
			// Find address offset. Alignment selected by user has higher priority than default alignment
			unsigned int offset = GetAlignmentOffset(pos, currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : realCurrType->alignBytes);
			variableInfo->pos += offset;
			varTop += offset;
		}
		variableInfo->varType = realCurrType;
		varTop += realCurrType->size;
	}

	CodeInfo::nodeList.push_back(new NodeGetAddress(variableInfo, variableInfo->pos, variableInfo->isGlobal, variableInfo->varType));

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
			CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = ((NodeFuncCall*)left)->GetFirstNode();
			if(AddFunctionCallNode(pos, fInfo->name, 1, true))
				return;
			else
				CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = left;
		}
	}

	// Call overloaded operator with error suppression
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "=", 2, true))
		return;

	if(left->typeInfo == typeObject && CodeInfo::nodeList.back()->typeInfo->refLevel == 0)
	{
		NodeZeroOP *right = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(new NodeConvertPtr(CodeInfo::GetReferenceType(right->typeInfo)));
		CodeInfo::nodeList.push_back(right);
	}
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
	}else if(lastType->funcType == NULL && lastType->refLevel != 0){
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
	// But if structure was, for example, returned from a function, then we have it on stack "by value", so we save it to a hidden variable and take a pointer to it
	if(currentType->refLevel == 0)
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
			ThrowError(pos, "ERROR: array doesn't have member with this name");
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(new NodeNumber((int)currentType->arrSize, typeVoid));
		currentType = typeInt;
		if(unifyTwo)
			AddExtraNode();
		return;
	}
 
	// Find member
	TypeInfo::MemberVariable *curr = currentType->firstVariable;
	for(; curr; curr = curr->next)
		if(curr->nameHash == hash)
			break;
	// If member variable is not found, try searching for member function
	FunctionInfo *memberFunc = NULL;
	if(!curr)
	{
		// Construct function name in a for of Class::Function
		unsigned int hash = currentType->nameHash;
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, varName.begin, varName.end);

		// Search for it
		HashMap<FunctionInfo*>::Node *func = funcMap.first(hash);
		if(!func)
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
		}else{
			memberFunc = func->value;
		}
		if(func && funcMap.next(func))
			ThrowError(pos, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);
	}
	
	// In case of a variable
	if(!memberFunc)
	{
		// Shift pointer to member
#ifndef NULLC_ENABLE_C_TRANSLATION
		if(CodeInfo::nodeList.back()->nodeType == typeNodeGetAddress)
			static_cast<NodeGetAddress*>(CodeInfo::nodeList.back())->ShiftToMember(curr);
		else
#endif
			CodeInfo::nodeList.push_back(new NodeShiftAddress(curr));
	}else{
		// In case of a function, get it's address
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(memberFunc));
	}

	if(unifyTwo)
		AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
}

void AddUnaryModifyOpNode(const char* pos, bool isInc, bool prefixOp)
{
	NodeZeroOP *pointer = CodeInfo::nodeList.back();
	CheckForImmutable(pointer->typeInfo, pos);

	// For indexes that aren't known at compile-time
	if(pointer->nodeType == typeNodeArrayIndex && !((NodeArrayIndex*)pointer)->knownShift)
	{
		// Create variable that will hold calculated pointer
		AddInplaceVariable(pos);
		// Get pointer from it
		CodeInfo::nodeList.push_back(new NodeDereference());
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
		CodeInfo::nodeList.push_back(new NodeDereference());
		// Restore value
		CodeInfo::nodeList.push_back(value);
		CodeInfo::nodeList.push_back(new NodeVariableModify(pointer->typeInfo, cmd));
		AddExtraNode();
	}else{
		CodeInfo::nodeList.push_back(new NodeVariableModify(pointer->typeInfo, cmd));
	}
}

void AddInplaceVariable(const char* pos, TypeInfo* targetType)
{
	char	*arrName = AllocateString(16);
	int length = sprintf(arrName, "$temp%d", inplaceVariableNum++);

	// Save variable creation state
	TypeInfo *saveCurrType = currType;
	bool saveVarDefined = varDefined;

	// Set type to auto
	currType = targetType;
	// Add hidden variable
	void *varInfo = AddVariable(pos, InplaceStr(arrName, length));
	// Set it to value on top of the stack
	AddDefineVariableNode(pos, varInfo, true);
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
	if(!dstType->subType || (dstType->arrSize != TypeInfo::UNSIZED_ARRAY && dstType->subType->arrSize != TypeInfo::UNSIZED_ARRAY) || dstType == nodeType)
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
	}else if(nodeType->refLevel == 1 && nodeType->subType->arrSize != TypeInfo::UNSIZED_ARRAY && dstType->subType->subType == nodeType->subType->subType){
		// type[N] ref to type[] ref conversion
		AddGetVariableNode(pos);
		AddInplaceVariable(pos, CodeInfo::GetArrayType(nodeType->subType->subType, TypeInfo::UNSIZED_ARRAY));
		AddExtraNode();
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
	if(typeVoid->refType && srcType == typeVoid->refType && dstType->refLevel != 0)
	{
		CodeInfo::nodeList.back()->typeInfo = dstType;
		return;
	}
	if(!((dstType == typeObject) ^ (srcType == typeObject)))
		return;

	if(srcType == typeObject && dstType->refLevel == 0)
	{
		CodeInfo::nodeList.push_back(new NodeConvertPtr(CodeInfo::GetReferenceType(dstType)));
		CodeInfo::nodeList.push_back(new NodeDereference());
		return;
	}
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

	bool arrayMulti = false;
	if(currentType->arrLevel != 0)
	{
		for(unsigned int i = 0; i < arrElementCount; i++)
		{
			TypeInfo *nodeType = CodeInfo::nodeList[CodeInfo::nodeList.size()-arrElementCount+i]->typeInfo;
			if(nodeType->subType != currentType->subType)
				ThrowError(pos, "ERROR: element %d doesn't match the type of element 0 (%s)", arrElementCount-i-1, currentType->GetFullTypeName());
			if(nodeType != currentType)
				arrayMulti = true;
		}
		if(arrayMulti)
			currentType = CodeInfo::GetArrayType(currentType->subType, TypeInfo::UNSIZED_ARRAY);
	}

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
		if(arrayMulti)
			ConvertArrayToUnsized(pos, currentType);
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

void AddArrayIterator(const char* pos, InplaceStr varName, void* type)
{
	// type size will be needed to shift pointer from one element to another
	unsigned int typeSize = CodeInfo::nodeList.back()->typeInfo->subType ? CodeInfo::nodeList.back()->typeInfo->subType->size : ~0u;
	unsigned int arrLevel = CodeInfo::nodeList.back()->typeInfo->arrLevel;

	bool extraExpression = false;
	// If value dereference was already called, but we need to get a pointer to array
	if(CodeInfo::nodeList.back()->nodeType == typeNodeDereference)
	{
		CodeInfo::nodeList.back() = ((NodeOneOP*)CodeInfo::nodeList.back())->GetFirstNode();
	}else{
		// Or if it wasn't called, for example, inplace array definition/function call/etc, we make a temporary variable
		AddInplaceVariable(pos);
		// And flag that we have added an extra node
		extraExpression = true;
	}

	// Special implementation of for each for build-in arrays
	if(arrLevel)
	{
		assert(typeSize != ~0u);

		// Add temporary variable that holds a pointer
		AddInplaceVariable(pos);
		NodeZeroOP *getArray = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();

		// Initialization part "iterator_type $temp = 0;"
		CodeInfo::nodeList.push_back(new NodeNumber(0, typeInt));
		AddInplaceVariable(pos);
		NodeZeroOP *getIterator = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();

		AddTwoExpressionNode(NULL);
		if(extraExpression)
			AddTwoExpressionNode(NULL);

		// Condition part "$temp < arr.size"
		CodeInfo::nodeList.push_back(getIterator);
		AddGetVariableNode(pos);
		CodeInfo::nodeList.push_back(getArray);
		AddMemberAccessNode(pos, InplaceStr("size"));
		AddGetVariableNode(pos);
		CodeInfo::nodeList.push_back(new NodeBinaryOp(cmdLess));

		// Iteration part "element_type varName = arr[$temp++];
		NodeOneOP *wrap = new NodeOneOP();
		wrap->SetFirstNode(getArray);
		CodeInfo::nodeList.push_back(wrap);
		NodeOneOP *wrap2 = new NodeOneOP();
		wrap2->SetFirstNode(getIterator);
		CodeInfo::nodeList.push_back(wrap2);
		AddUnaryModifyOpNode(pos, OP_INCREMENT, OP_POSTFIX);
		AddArrayIndexNode(pos);
		currType = (TypeInfo*)type ? CodeInfo::GetReferenceType((TypeInfo*)type) : NULL;
		VariableInfo *it = (VariableInfo*)AddVariable(pos, varName);
		AddDefineVariableNode(pos, it);
		AddPopNode(pos);

		it->autoDeref = true;

		return;
	}

	// Initialization part "iterator_type $temp = arr.start();"
	PrepareMemberCall(pos);
	AddMemberFunctionCall(pos, "start", 0);

	AddInplaceVariable(pos);
	// This node will return iterator variable
	NodeZeroOP *getIterator = CodeInfo::nodeList.back();

	// We have to find out iterator return type
	PrepareMemberCall(pos);
	AddMemberFunctionCall(pos, "next", 0);
	TypeInfo *iteratorReturnType = CodeInfo::nodeList.back()->typeInfo;
	CodeInfo::nodeList.pop_back();

	// Initialization part "element_type varName;"
	currType = (TypeInfo*)type ? CodeInfo::GetReferenceType((TypeInfo*)type) : iteratorReturnType;
	VariableInfo *it = (VariableInfo*)AddVariable(pos, varName);
	if(extraExpression)
		AddTwoExpressionNode(NULL);

	// Condition part "$temp.hasnext()"
	NodeOneOP *wrapCond = new NodeOneOP();
	wrapCond->SetFirstNode(getIterator);
	CodeInfo::nodeList.push_back(wrapCond);
	PrepareMemberCall(pos);
	AddMemberFunctionCall(pos, "hasnext", 0);

	// Increment part "varName = $tmp.next();"
	AddGetAddressNode(pos, varName);

	NodeOneOP *wrap = new NodeOneOP();
	wrap->SetFirstNode(getIterator);
	CodeInfo::nodeList.push_back(wrap);
	PrepareMemberCall(pos);
	AddMemberFunctionCall(pos, "next", 0);
	AddSetVariableNode(pos);
	AddPopNode(pos);

	it->autoDeref = true;
}

void MergeArrayIterators()
{
	NodeZeroOP *lastIter = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	NodeZeroOP *lastCond = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	NodeZeroOP *lastInit = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	NodeZeroOP *firstIter = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	NodeZeroOP *firstCond = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	CodeInfo::nodeList.push_back(lastInit);
	AddTwoExpressionNode(NULL);

	CodeInfo::nodeList.push_back(firstCond);
	CodeInfo::nodeList.push_back(lastCond);
	CodeInfo::nodeList.push_back(new NodeBinaryOp(cmdLogAnd));

	CodeInfo::nodeList.push_back(firstIter);
	CodeInfo::nodeList.push_back(lastIter);
	AddTwoExpressionNode(NULL);
}

void AddForEachNode(const char* pos)
{
	// Unite increment_part and body
	AddTwoExpressionNode(NULL);
	// Generate while cycle
	CodeInfo::nodeList.push_back(new NodeWhileExpr());
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
	// Unite initialization_part and while
	AddTwoExpressionNode(NULL);
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
	unsigned int funcNameHash = GetStringHash(funcName), origHash = funcNameHash;
	for(unsigned int i = varInfoTop.back().activeVarCnt; i < CodeInfo::varInfo.size(); i++)
	{
		if(CodeInfo::varInfo[i]->nameHash == funcNameHash)
			ThrowError(pos, "ERROR: name '%s' is already taken for a variable in current scope", funcName);
	}
	char *funcNameCopy = (char*)funcName;
	if(newType)
	{
		funcNameCopy = GetClassFunctionName(newType, funcName);
		funcNameHash = GetStringHash(funcNameCopy);
	}

	CodeInfo::funcInfo.push_back(new FunctionInfo(funcNameCopy, funcNameHash, origHash));
	FunctionInfo* lastFunc = CodeInfo::funcInfo.back();
	lastFunc->parentFunc = currDefinedFunc.size() > 0 ? currDefinedFunc.back() : NULL;

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
	if(funcName[0] != '$' && !(chartype_table[(unsigned char)funcName[0]] & ct_start_symbol))
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

#if defined(__CELLOS_LV2__)
	unsigned bigEndianShift = currType == typeChar ? 3 : (currType == typeShort ? 2 : 0);
#else
	unsigned bigEndianShift = 0;
#endif
	lastFunc.AddParameter(new VariableInfo(&lastFunc, paramName, hash, lastFunc.allParamSize + bigEndianShift, currType, false));
	if(currType)
		lastFunc.allParamSize += currType->size < 4 ? 4 : currType->size;
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
		lastFunc.allParamSize += right->size < 4 ? 4 : right->size;
	}
	if(left == typeVoid)
		ThrowError(pos, "ERROR: function parameter cannot be a void type", right->GetFullTypeName(), left->GetFullTypeName());
	
	// If types don't match and it it is not build-in basic types or if pointers point to different types
	if(right == typeVoid || (left != right && (left->type == TypeInfo::TYPE_COMPLEX || right->type == TypeInfo::TYPE_COMPLEX || left->subType != right->subType)))
		ThrowError(pos, "ERROR: cannot convert from '%s' to '%s'", right->GetFullTypeName(), left->GetFullTypeName());

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
	if(!lastFunc.visible && lastFunc.firstParam && (lastFunc.firstParam->varType->type == TypeInfo::TYPE_COMPLEX || lastFunc.firstParam->varType->subType))
		lastFunc.visible = true;

	BeginBlock();
	cycleDepth.push_back(0);

	varTop = 0;

	for(VariableInfo *curr = lastFunc.firstParam; curr; curr = curr->next)
	{
		varTop += GetAlignmentOffset(pos, 4);
		CodeInfo::varInfo.push_back(curr);
		CodeInfo::varInfo.back()->blockDepth = varInfoTop.size();
		varMap.insert(curr->nameHash, curr);
		varTop += curr->varType->size;
	}

	char	*hiddenHame = AllocateString(lastFunc.nameLength + 24);
	int length = 0;
	if(lastFunc.type == FunctionInfo::THISCALL)
	{
		memcpy(hiddenHame, "this", 5);
		length = 4;
	}else{
		length = sprintf(hiddenHame, "$%s_%d_ext", lastFunc.name, CodeInfo::FindFunctionByPtr(&lastFunc));
	}
	currType = CodeInfo::GetReferenceType(lastFunc.type == FunctionInfo::THISCALL ? lastFunc.parentClass : typeInt);
	currAlign = 4;
	lastFunc.extraParam = (VariableInfo*)AddVariable(pos, InplaceStr(hiddenHame, length));
	varDefined = false;
}

void FunctionEnd(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	HashMap<FunctionInfo*>::Node *curr = funcMap.first(lastFunc.nameHash);
	while(curr)
	{
		FunctionInfo *info = curr->value;
		if(info == &lastFunc)
		{
			curr = funcMap.next(curr);
			continue;
		}

		if(info->paramCount == lastFunc.paramCount && info->visible && info->retType == lastFunc.retType)
		{
			// Check all parameter types
			bool paramsEqual = true;
			for(VariableInfo *currN = info->firstParam, *currI = lastFunc.firstParam; currN; currN = currN->next, currI = currI->next)
			{
				if(currN->varType != currI->varType)
					paramsEqual = false;
			}
			if(paramsEqual)
			{
				if(info->implemented)
					ThrowError(pos, "ERROR: function '%s' is being defined with the same set of parameters", lastFunc.name);
				else
					info->address = lastFunc.indexInArr | 0x80000000;
			}
		}
		curr = funcMap.next(curr);
	}

	cycleDepth.pop_back();
	// Save info about all local variables
	for(int i = CodeInfo::varInfo.size()-1; i > (int)(varInfoTop.back().activeVarCnt + lastFunc.paramCount); i--)
	{
		VariableInfo *firstNext = lastFunc.firstLocal;
		lastFunc.firstLocal = CodeInfo::varInfo[i];
		if(!lastFunc.lastLocal)
			lastFunc.lastLocal = lastFunc.firstLocal;
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
		char *hiddenHame = AllocateString(lastFunc.nameLength + 24);
		int length = sprintf(hiddenHame, "$%s_%d_ext", lastFunc.name, CodeInfo::FindFunctionByPtr(&lastFunc));

		TypeInfo *saveCurrType = currType;
		bool saveVarDefined = varDefined;

		// Create a pointer to array
		currType = NULL;
		void *varInfo = AddVariable(pos, InplaceStr(hiddenHame, length));

		// Allocate array in dynamic memory
		CodeInfo::nodeList.push_back(new NodeNumber((int)(lastFunc.externalSize), typeInt));
		AddFunctionCallNode(pos, "__newS", 1);
		CodeInfo::nodeList.back()->typeInfo = CodeInfo::GetReferenceType(CodeInfo::GetArrayType(CodeInfo::GetReferenceType(typeInt), lastFunc.externalSize / NULLC_PTR_SIZE));

		// Set it to pointer variable
		AddDefineVariableNode(pos, varInfo);

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
	AliasInfo *info = lastFunc.childAlias;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}

	if(newType && lastFunc.type == FunctionInfo::THISCALL)
		methodCount++;

	currType = lastFunc.retType;
}

void FunctionToOperator(const char* pos)
{
	static unsigned int hashAdd = GetStringHash("+");
	static unsigned int hashSub = GetStringHash("-");
	static unsigned int hashBitNot = GetStringHash("~");
	static unsigned int hashLogNot = GetStringHash("!");

	FunctionInfo &lastFunc = *currDefinedFunc.back();
	if(lastFunc.paramCount != 2 && !(lastFunc.paramCount == 1 && (lastFunc.nameHash == hashAdd || lastFunc.nameHash == hashSub || lastFunc.nameHash == hashBitNot || lastFunc.nameHash == hashLogNot)))
		ThrowError(pos, "ERROR: binary operator definition or overload must accept exactly two arguments");
	lastFunc.visible = true;
}

unsigned int GetFunctionRating(FunctionType *currFunc, unsigned int callArgCount)
{
	if(currFunc->paramCount != callArgCount)
		return ~0u;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.

	unsigned int fRating = 0;
	for(unsigned int i = 0; i < currFunc->paramCount; i++)
	{
		NodeZeroOP* activeNode = CodeInfo::nodeList[CodeInfo::nodeList.size() - currFunc->paramCount + i];
		TypeInfo *paramType = activeNode->typeInfo;
		unsigned int	nodeType = activeNode->nodeType;
		TypeInfo *expectedType = currFunc->paramType[i];
		if(expectedType != paramType)
		{
			if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->arrSize != 0 && paramType->subType == expectedType->subType)
				fRating += 2;
			else if(expectedType->refLevel == 1 && expectedType->refLevel == paramType->refLevel && expectedType->subType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->subType->subType == expectedType->subType->subType)
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
	if(currentType->refLevel == 0)
	{
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
	// For auto ref types, we redirect call to a target type
	if(currentType == CodeInfo::GetReferenceType(typeObject))
	{
		// We need to know what function overload is called, and this is a long process:
		// Get function name hash
		unsigned int fHash = GetStringHash(funcName);

		// Clear function list
		bestFuncList.clear();
		// Find all functions with called name that are member functions
		for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		{
			FunctionInfo *func = CodeInfo::funcInfo[i];
			if(func->nameHashOrig == fHash && func->parentClass && func->visible && !((func->address & 0x80000000) && (func->address != -1)) && func->funcType)
				bestFuncList.push_back(func);
		}
		unsigned int count = bestFuncList.size();
		if(count == 0)
			ThrowError(pos, "ERROR: function '%s' is undefined in any of existing classes", funcName);

		// Find best function fit
		bestFuncRating.resize(count);

		unsigned int minRating = ~0u;
		unsigned int minRatingIndex = ~0u;
		for(unsigned int k = 0; k < count; k++)
		{
			unsigned int argumentCount = callArgCount;
			bestFuncRating[k] = GetFunctionRating(bestFuncList[k]->funcType->funcType, argumentCount);
			if(bestFuncRating[k] < minRating)
			{
				minRating = bestFuncRating[k];
				minRatingIndex = k;
			}
		}
		if(minRating != 0)
			ThrowError(pos, "ERROR: none of the member ::%s functions can handle the supplied parameter list without conversions", funcName);
		// Get function type
		TypeInfo *fType = bestFuncList[minRatingIndex]->funcType;

		unsigned int lenStr = 5 + (int)strlen(funcName) + 1 + 32;
		char *vtblName = AllocateString(lenStr);
		SafeSprintf(vtblName, lenStr, "$vtbl%010u%s", fType->GetFullNameHash(), funcName);
		unsigned int hash = GetStringHash(vtblName);
		VariableInfo *target = vtblList;
		if(target && target->nameHash != hash)
			target = target->next;

		// If vtbl cannot be found, create it
		if(!target)
		{
			// Create array of function pointer (function pointer is an integer index)
			VariableInfo *vInfo = new VariableInfo(0, InplaceStr(vtblName), hash, 500000, CodeInfo::GetArrayType(typeInt, TypeInfo::UNSIZED_ARRAY), true);
			vInfo->next = vtblList;
			vInfo->prev = (VariableInfo*)fType;	// $$ not type safe at all
			target = vtblList = vInfo;
		}
		// Take node with auto ref computation
		NodeZeroOP *autoRef = CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount-1];
		// Push it on the top
		CodeInfo::nodeList.push_back(autoRef);
		// We have auto ref ref, so dereference it
		AddGetVariableNode(pos);
		// Push address to array
		CodeInfo::nodeList.push_back(new NodeGetAddress(target, target->pos, target->isGlobal, target->varType));
		((NodeGetAddress*)CodeInfo::nodeList.back())->SetAddressTracking();
		//AddGetVariableNode(pos);
		// Find redirection function
		HashMap<FunctionInfo*>::Node *curr = funcMap.first(GetStringHash("__redirect"));
		if(!curr)
			ThrowError(pos, "ERROR: cannot find redirection function");
		// Call redirection function
		AddFunctionCallNode(pos, "__redirect", 2);
		// Rename return function type
		CodeInfo::nodeList.back()->typeInfo = fType;
		// Call target function
		AddFunctionCallNode(pos, NULL, callArgCount);
		autoRef = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(autoRef);
		return;
	}
	CheckForImmutable(currentType, pos);
	// Construct name in a form of Class::Function
	char *memberFuncName = GetClassFunctionName(currentType->subType, funcName);
	// Call it
	AddFunctionCallNode(pos, memberFuncName, callArgCount);
}

bool AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount, bool silent)
{
	unsigned int funcNameHash = ~0u;

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	NodeZeroOP	*funcAddr = NULL;

	if(newType)
	{
		unsigned int hash = newType->nameHash;
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, funcName);

		if(funcMap.find(hash))
		{
			funcNameHash = hash;

			FunctionInfo *currFunc = currDefinedFunc.back();
			TypeInfo *temp = CodeInfo::GetReferenceType(newType);
			CodeInfo::nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));
			CodeInfo::nodeList.push_back(new NodeDereference());
			funcAddr = CodeInfo::nodeList.back();
			CodeInfo::nodeList.pop_back();
		}else if(funcName){
			funcNameHash = GetStringHash(funcName);
		}
	}else if(funcName){
		funcNameHash = GetStringHash(funcName);
	}
	// Handle type(auto ref) -> type, if no user function is defined.
	if(!silent && callArgCount == 1 && CodeInfo::nodeList[CodeInfo::nodeList.size() - 1]->typeInfo == typeObject)
	{
		TypeInfo *autoRefToType = NULL;
		TypeInfo **type = CodeInfo::classMap.find(funcNameHash);
		if(type)
			autoRefToType = *type;
		for(unsigned int i = 0; i < CodeInfo::typeInfo.size() && !autoRefToType; i++)
		{
			if(CodeInfo::typeInfo[i]->GetFullNameHash() == funcNameHash)
				autoRefToType = CodeInfo::typeInfo[i];
		}
		if(autoRefToType)
		{
			if(AddFunctionCallNode(pos, funcName, 1, true))
				return true;
			CodeInfo::nodeList.push_back(new NodeConvertPtr(CodeInfo::GetReferenceType(autoRefToType)));
			CodeInfo::nodeList.push_back(new NodeDereference());
			return true;
		}
	}

	//Find all functions with given name
	bestFuncList.clear();

	HashMap<FunctionInfo*>::Node *curr = funcMap.first(funcNameHash);
	while(curr)
	{
		FunctionInfo *func = curr->value;
		if(func->visible && !((func->address & 0x80000000) && (func->address != -1)) && func->funcType)
			bestFuncList.push_back(func);
		curr = funcMap.next(curr);
	}
	unsigned int count = bestFuncList.size();
	unsigned int vID = ~0u;
	if(count == 0)
	{
		if(silent)
			return false;
		vID = CodeInfo::FindVariableByName(funcNameHash);
		if(vID == ~0u && funcName)
			ThrowError(pos, "ERROR: function '%s' is undefined", funcName);
	}

	if(vID == ~0u && funcName)
	{
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
			if(argumentCount >= bestFuncList[k]->funcType->funcType->paramCount && bestFuncList[k]->lastParam && bestFuncList[k]->lastParam->varType == typeObjectArray)
			{
				// If function accepts variable argument list
				TypeInfo *&lastType = bestFuncList[k]->funcType->funcType->paramType[bestFuncList[k]->funcType->funcType->paramCount - 1];
				assert(lastType == CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY));
				unsigned int redundant = argumentCount - bestFuncList[k]->funcType->funcType->paramCount;
				CodeInfo::nodeList.shrink(CodeInfo::nodeList.size() - redundant);
				// Change things in a way that this function will be selected (lie about real argument count and match last argument type)
				lastType = CodeInfo::nodeList.back()->typeInfo;
				bestFuncRating[k] = GetFunctionRating(bestFuncList[k]->funcType->funcType, bestFuncList[k]->funcType->funcType->paramCount);
				CodeInfo::nodeList.resize(CodeInfo::nodeList.size() + redundant);
				if(bestFuncRating[k] != ~0u)
					bestFuncRating[k] += 10;	// Cost of variable arguments function
				lastType = CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY);
			}else{
				bestFuncRating[k] = GetFunctionRating(bestFuncList[k]->funcType->funcType, argumentCount);
			}
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
		}else{
			fType = bestFuncList[minRatingIndex]->funcType->funcType;
			fInfo = bestFuncList[minRatingIndex];
		}
		// Check, is there are more than one function, that share the same rating
		for(unsigned int k = 0; k < count; k++)
		{
			if(k != minRatingIndex && bestFuncRating[k] == minRating)
			{
				char errTemp[512];
				char	*errPos = errTemp;
				errPos += SafeSprintf(errPos, 512, "ERROR: ambiguity, there is more than one overloaded function available for the call.\r\n  %s(", funcName);
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
	}else{
		if(funcName)
			AddGetAddressNode(pos, InplaceStr(funcName, (int)strlen(funcName)));
		AddGetVariableNode(pos);
		ConvertFunctionToPointer(pos);
		funcAddr = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		fType = funcAddr->typeInfo->funcType;
		if(!fType)
			ThrowError(pos, "ERROR: variable is not a pointer to function");
		unsigned int bestRating = ~0u;
		if(callArgCount >= fType->paramCount && fType->paramCount && fType->paramType[fType->paramCount-1] == typeObjectArray)
		{
			// If function accepts variable argument list
			TypeInfo *&lastType = fType->paramType[fType->paramCount - 1];
			assert(lastType == CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY));
			unsigned int redundant = callArgCount - fType->paramCount;
			CodeInfo::nodeList.shrink(CodeInfo::nodeList.size() - redundant);
			// Change things in a way that this function will be selected (lie about real argument count and match last argument type)
			lastType = CodeInfo::nodeList.back()->typeInfo;
			bestRating = GetFunctionRating(fType, fType->paramCount);
			CodeInfo::nodeList.resize(CodeInfo::nodeList.size() + redundant);
			if(bestRating != ~0u)
				bestRating += 10;	// Cost of variable arguments function
			lastType = CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY);
		}else{
			bestRating = GetFunctionRating(fType, callArgCount);
		}
		if(bestRating == ~0u)
		{
			char	errTemp[512];
			char	*errPos = errTemp;
			if(callArgCount != fType->paramCount)
				errPos += SafeSprintf(errPos, 512, "ERROR: function expects %d argument(s), while %d are supplied\r\n", fType->paramCount, callArgCount);
			else
				errPos += SafeSprintf(errPos, 512, "ERROR: there is no conversion from specified arguments and the ones that function accepts\r\n");
			errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "\tExpected: (");
			for(unsigned int n = 0; n < fType->paramCount; n++)
				errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "%s%s", fType->paramType[n]->GetFullTypeName(), n != fType->paramCount - 1 ? ", " : "");
			errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), ")\r\n");
			
			errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "\tProvided: (");
			for(unsigned int n = 0; n < callArgCount; n++)
				errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), "%s%s", CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount+n]->typeInfo->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
			errPos += SafeSprintf(errPos, 512 - int(errPos - errTemp), ")");
			ThrowError(pos, errTemp);
		}
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
	// If it's variable argument function
	if(callArgCount >= fType->paramCount && fType->paramCount && fType->paramType[fType->paramCount - 1] == CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY))
	{
		// Pack last argument and all others into an auto ref array
		paramNodes.clear();
		unsigned int argsToPack = callArgCount - fType->paramCount + 1;
		for(unsigned int i = 0; i < argsToPack; i++)
		{
			// Convert type to auto ref
			if(CodeInfo::nodeList.back()->typeInfo != typeObject)
			{
				if(CodeInfo::nodeList.back()->nodeType == typeNodeDereference)
				{
					((NodeDereference*)CodeInfo::nodeList.back())->Neutralize();
				}else{
					// type[N] is converted to type[] first
					if(CodeInfo::nodeList.back()->typeInfo->arrLevel)
						ConvertArrayToUnsized(pos, CodeInfo::GetArrayType(CodeInfo::nodeList.back()->typeInfo->subType, TypeInfo::UNSIZED_ARRAY));
					AddInplaceVariable(pos);
					AddExtraNode();
				}
				HandlePointerToObject(pos, typeObject);
			}
			paramNodes.push_back(CodeInfo::nodeList.back());
			CodeInfo::nodeList.pop_back();
		}
		// Push arguments on stack
		for(unsigned int i = 0; i < argsToPack; i++)
			CodeInfo::nodeList.push_back(paramNodes[argsToPack - i - 1]);
		// Convert to array of arguments
		AddArrayConstructor(pos, argsToPack - 1);
		// Change current argument type
		callArgCount -= argsToPack;
		callArgCount++;
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
		// implicit conversion from type to type ref (or auto ref)
		if((fType->paramType[i]->refLevel == CodeInfo::nodeList.back()->typeInfo->refLevel + 1 && fType->paramType[i]->subType == CodeInfo::nodeList.back()->typeInfo) ||
			(CodeInfo::nodeList.back()->typeInfo->refLevel == 0 && fType->paramType[i] == typeObject))
		{
			if(CodeInfo::nodeList.back()->typeInfo != typeObject)
			{
				if(CodeInfo::nodeList.back()->nodeType == typeNodeDereference)
				{
					((NodeDereference*)CodeInfo::nodeList.back())->Neutralize();
				}else{
					AddInplaceVariable(pos);
					AddExtraNode();
				}
			}
		}
		HandlePointerToObject(pos, fType->paramType[i]);
	}

	if(fInfo && (fInfo->type == FunctionInfo::LOCAL))
	{
		char	*contextName = AllocateString(fInfo->nameLength + 24);
		int length = sprintf(contextName, "$%s_%d_ext", fInfo->name, CodeInfo::FindFunctionByPtr(fInfo));
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
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 2);

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
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 3);

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
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 3);

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
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 4);

	CodeInfo::nodeList.push_back(new NodeForExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}
void AddWhileNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 2);

	CodeInfo::nodeList.push_back(new NodeWhileExpr());
	CodeInfo::nodeList.back()->SetCodeInfo(pos);

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}
void AddDoWhileNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 2);

	CodeInfo::nodeList.push_back(new NodeDoWhileExpr());
	CodeInfo::nodeList.back()->SetCodeInfo(pos);

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}

void BeginSwitch(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 1);
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
		ThrowError(pos, "ERROR: different type is being defined");
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
	newType = new TypeInfo(CodeInfo::typeInfo.size(), typeNameCopy, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	newType->alignBytes = currAlign;
	newType->originalIndex = CodeInfo::typeInfo.size();
	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;
	methodCount = 0;

	CodeInfo::typeInfo.push_back(newType);
	CodeInfo::classMap.insert(newType->GetFullNameHash(), newType);

	BeginBlock();
}

void TypeAddMember(const char* pos, const char* varName)
{
	if(!currType)
		ThrowError(pos, "ERROR: auto cannot be used for class members");
	// Align members to their default alignment, but not larger that 4 bytes
	unsigned int alignment = currType->alignBytes > 4 ? 4 : currType->alignBytes;
	if(alignment && newType->size % alignment != 0)
		newType->size += alignment - (newType->size % alignment);
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

	// Shift new types generated inside up, so that declaration will be in the correct order in C translation
	for(unsigned int i = newType->originalIndex + 1; i < CodeInfo::typeInfo.size(); i++)
		CodeInfo::typeInfo[i]->originalIndex--;
	newType->originalIndex = CodeInfo::typeInfo.size() - 1;

	AliasInfo *info = newType->childAlias;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}

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
	AliasInfo *info = newType->childAlias;
	while(info)
	{
		CodeInfo::classMap.insert(info->nameHash, info->type);
		info = info->next;
	}
}

void TypeStop()
{
	varTop = varInfoTop.back().varStackSize;
	AliasInfo *info = newType->childAlias;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}
	newType = NULL;
	EndBlock(false, false);
}

void AddAliasType(InplaceStr aliasName)
{
	if(newType && !currDefinedFunc.size())
	{
		AliasInfo *info = TypeInfo::CreateAlias(GetStringHash(aliasName.begin, aliasName.end), currType);
		info->next = newType->childAlias;
		newType->childAlias = info;
	}else if(currDefinedFunc.size()){
		AliasInfo *info = TypeInfo::CreateAlias(GetStringHash(aliasName.begin, aliasName.end), currType);
		info->next = currDefinedFunc.back()->childAlias;
		currDefinedFunc.back()->childAlias = info;
	}
	CodeInfo::classMap.insert(GetStringHash(aliasName.begin, aliasName.end), currType);
}

void AddUnfixedArraySize()
{
	CodeInfo::nodeList.push_back(new NodeNumber(1, typeVoid));
}

void CreateRedirectionTables()
{
	VariableInfo *curr = vtblList;
	unsigned int num = 0;
	while(curr)
	{
		unsigned int hash = GetStringHash(curr->name.begin + 15);

		currType = curr->varType;
		CodeInfo::nodeList.push_back(new NodeNumber(4, typeInt));
		AddFunctionCallNode(CodeInfo::lastKnownStartPos, "__typeCount", 0);
		AddFunctionCallNode(CodeInfo::lastKnownStartPos, "__newA", 2);
		CodeInfo::nodeList.back()->typeInfo = currType;
		CodeInfo::varInfo.push_back(curr);
		curr->pos = varTop;
		varTop += curr->varType->size;

		AddDefineVariableNode(CodeInfo::lastKnownStartPos, curr, true);
		AddPopNode(CodeInfo::lastKnownStartPos);

		TypeInfo *funcType = (TypeInfo*)curr->prev;

		bestFuncList.clear();
		// Find all functions with called name that are member functions and have target type
		for(unsigned int i = 0; i < CodeInfo::funcInfo.size(); i++)
		{
			FunctionInfo *func = CodeInfo::funcInfo[i];
			if(func->nameHashOrig == hash && func->parentClass && func->visible && !((func->address & 0x80000000) && (func->address != -1)) && func->funcType == funcType)
				bestFuncList.push_back(func);
		}

		for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
		{
			for(unsigned int k = 0; k < bestFuncList.size(); k++)
			{
				if(bestFuncList[k]->parentClass == CodeInfo::typeInfo[i])
				{
					// Get array variable
					CodeInfo::nodeList.push_back(new NodeGetAddress(curr, curr->pos, curr->isGlobal, curr->varType));
					CodeInfo::nodeList.push_back(new NodeDereference());

					// Push index (typeID number is index)
					CodeInfo::nodeList.push_back(new NodeZeroOP(typeInt));
					CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdPushTypeID, bestFuncList[k]->parentClass->typeIndex));

					// Index array
					CodeInfo::nodeList.push_back(new NodeArrayIndex(curr->varType));

					// Push functionID
					CodeInfo::nodeList.push_back(new NodeZeroOP(typeInt));
					CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdFuncAddr, bestFuncList[k]->indexInArr));

					// Set array element value
					CodeInfo::nodeList.push_back(new NodeVariableSet(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, 0, true));

					// Remove value from stack
					AddPopNode(CodeInfo::lastKnownStartPos);
					// Fold node
					AddTwoExpressionNode(NULL);
					break;
				}
			}
		}
		// Add node in front of global code
		static_cast<NodeExpressionList*>(CodeInfo::nodeList[0])->AddNode(true);

		curr = curr->next;
		num++;
	}
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

	funcMap.init();
	funcMap.clear();

	varMap.init();
	varMap.clear();

	ResetTreeGlobals();

	vtblList = NULL;

	CodeInfo::classMap.clear();
	CodeInfo::typeArrays.clear();
	CodeInfo::typeFunctions.clear();
	// Add build-in type info to hash map and special lists
	for(unsigned int i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		CodeInfo::classMap.insert(CodeInfo::typeInfo[i]->GetFullNameHash(), CodeInfo::typeInfo[i]);
		if(CodeInfo::typeInfo[i]->arrLevel)
			CodeInfo::typeArrays.push_back(CodeInfo::typeInfo[i]);
		if(CodeInfo::typeInfo[i]->funcType)
			CodeInfo::typeFunctions.push_back(CodeInfo::typeInfo[i]);
	}

	typeObjectArray = CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY);
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
	funcMap.reset();
	varMap.reset();

	TypeInfo::typeInfoPool.~ChunkedStackPool();
	TypeInfo::SetPoolTop(0);
	VariableInfo::variablePool.~ChunkedStackPool();
	VariableInfo::SetPoolTop(0);
	FunctionInfo::functionPool.~ChunkedStackPool();
	FunctionInfo::SetPoolTop(0);
}
