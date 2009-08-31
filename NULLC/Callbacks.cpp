#include "Callbacks.h"
#include "CodeInfo.h"
using namespace CodeInfo;

#include "Parser.h"

char	callbackError[256];

// Temp variables

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

// Stack of current types
FastVector<TypeInfo*>		currTypes(64);

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

// Saves number of defined variables and functions
void BeginBlock()
{
	varInfoTop.push_back(VarTopInfo((unsigned int)varInfo.size(), varTop));
	funcInfoTop.push_back((unsigned int)funcInfo.size());
}
// Restores previous number of defined variables and functions to hide those that lost visibility
void EndBlock(bool hideFunctions)
{
	varInfo.shrink(varInfoTop.back().activeVarCnt);
	varInfoTop.pop_back();

	if(hideFunctions)
	{
		for(unsigned int i = funcInfoTop.back(); i < funcInfo.size(); i++)
			funcInfo[i]->visible = false;
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
		ThrowError("ERROR: unknown escape sequence", lastKnownStartPos);
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

// Functions used to add node for constant numbers of different type
void AddNumberNodeChar(const char* pos)
{
	char res = pos[1];
	if(res == '\\')
		res = UnescapeSybmol(pos[2]);
	nodeList.push_back(new NodeNumber(int(res), typeChar));
}

void AddNumberNodeInt(const char* pos)
{
	nodeList.push_back(new NodeNumber(parseInteger(pos), typeInt));
}
void AddNumberNodeFloat(const char* pos)
{
	nodeList.push_back(new NodeNumber((float)parseDouble(pos), typeFloat));
}
void AddNumberNodeLong(const char* pos, const char* end)
{
	nodeList.push_back(new NodeNumber(parseLong(pos, end, 10), typeLong));
}
void AddNumberNodeDouble(const char* pos)
{
	nodeList.push_back(new NodeNumber(parseDouble(pos), typeDouble));
}

void AddHexInteger(const char* pos, const char* end)
{
	pos += 2;
	if(int(end - pos) > 16)
		ThrowError("ERROR: Overflow in hexadecimal constant", pos);
	if(int(end - pos) <= 8)
		nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 16), typeInt));
	else
		nodeList.push_back(new NodeNumber(parseLong(pos, end, 16), typeLong));
}

void AddOctInteger(const char* pos, const char* end)
{
	pos++;
	if(int(end - pos) > 21)
		ThrowError("ERROR: Overflow in octal constant", pos);
	if(int(end - pos) <= 10)
		nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 8), typeInt));
	else
		nodeList.push_back(new NodeNumber(parseLong(pos, end, 8), typeLong));
}

void AddBinInteger(const char* pos, const char* end)
{
	if(int(end - pos) > 64)
		ThrowError("ERROR: Overflow in binary constant", pos);
	if(int(end - pos) <= 32)
		nodeList.push_back(new NodeNumber((int)parseLong(pos, end, 2), typeInt));
	else
		nodeList.push_back(new NodeNumber(parseLong(pos, end, 2), typeLong));
}

void AddVoidNode()
{
	nodeList.push_back(new NodeZeroOP());
}

// Function that places string on stack, using list of NodeNumber in NodeExpressionList
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
		nodeList.push_back(new NodeNumber(*(int*)clean, typeInt));
		arrayList->AddNode();
	}
	if(len % 4 == 0)
	{
		nodeList.push_back(new NodeNumber(0, typeInt));
		arrayList->AddNode();
	}
	nodeList.push_back(temp);
}

// Function that creates node that removes value on top of the stack
void AddPopNode(const char* s, const char* e)
{
	nodeList.back()->SetCodeInfo(s, e);
	// If the last node is a number, remove it completely
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		nodeList.pop_back();
		nodeList.push_back(new NodeZeroOP());
	}else if(nodeList.back()->nodeType == typeNodePreOrPostOp){
		// If the last node is increment or decrement, then we do not need to keep the value on stack, and some optimizations can be done
		static_cast<NodePreOrPostOp*>(nodeList.back())->SetOptimised(true);
	}else{
		// Otherwise, just create node
		nodeList.push_back(new NodePopOp());
	}
}

// Function that creates unary operation node that changes sign of value
void AddNegateNode(const char* pos)
{
	// If the last node is a number, we can just change sign of constant
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeDouble || aType == typeFloat)
		{
			Rd = new NodeNumber(-static_cast<NodeNumber*>(nodeList.back())->GetDouble(), aType);
		}else if(aType == typeLong){
			Rd = new NodeNumber(-static_cast<NodeNumber*>(nodeList.back())->GetLong(), aType);
		}else if(aType == typeInt){
			Rd = new NodeNumber(-static_cast<NodeNumber*>(nodeList.back())->GetInteger(), aType);
		}else{
			sprintf(callbackError, "addNegNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, pos);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// Otherwise, just create node
		nodeList.push_back(new NodeUnaryOp(cmdNeg));
	}
}

// Function that creates unary operation node for logical NOT
void AddLogNotNode(const char* pos)
{
	if(nodeList.back()->typeInfo == typeDouble || nodeList.back()->typeInfo == typeFloat)
		ThrowError("ERROR: logical NOT is not available on floating-point numbers", pos);
	// If the last node is a number, we can just make operation in compile-time
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeLong){
			Rd = new NodeNumber(!static_cast<NodeNumber*>(nodeList.back())->GetLong(), aType);
		}else if(aType == typeInt){
			Rd = new NodeNumber(!static_cast<NodeNumber*>(nodeList.back())->GetInteger(), aType);
		}else{
			sprintf(callbackError, "addLogNotNode() ERROR: unknown type %s", aType->name);
			ThrowError(callbackError, pos);
		}
		nodeList.pop_back();
		nodeList.push_back(Rd);
	}else{
		// Otherwise, just create node
		nodeList.push_back(new NodeUnaryOp(cmdLogNot));
	}
}

// Function that creates unary operation node for binary NOT
void AddBitNotNode(const char* pos)
{
	if(nodeList.back()->typeInfo == typeDouble || nodeList.back()->typeInfo == typeFloat)
		ThrowError("ERROR: binary NOT is not available on floating-point numbers", pos);
	// If the last node is a number, we can just make operation in compile-time
	if(nodeList.back()->nodeType == typeNodeNumber)
	{
		TypeInfo *aType = nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeLong){
			Rd = new NodeNumber(~static_cast<NodeNumber*>(nodeList.back())->GetLong(), aType);
		}else if(aType == typeInt){
			Rd = new NodeNumber(~static_cast<NodeNumber*>(nodeList.back())->GetInteger(), aType);
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

	if(aNodeType == typeNodeNumber && bNodeType == typeNodeNumber)
	{
		NodeNumber *Ad = static_cast<NodeNumber*>(nodeList[nodeList.size() - 2]);
		NodeNumber *Bd = static_cast<NodeNumber*>(nodeList[nodeList.size() - 1]);
		nodeList.pop_back();
		nodeList.pop_back();

		// If we have operation between two known numbers, we can optimize code by calculating the result in compile-time
		TypeInfo *aType = Ad->typeInfo;
		TypeInfo *bType = Bd->typeInfo;

		bool swapOper = false;
		// Swap operands, to reduce number of combinations
		if(((aType == typeFloat || aType == typeLong || aType == typeInt) && bType == typeDouble) ||
			((aType == typeLong || aType == typeInt) && bType == typeFloat) ||
			(aType == typeInt && bType == typeLong))
		{
			Swap(Ad, Bd);
			swapOper = true;
		}

		NodeNumber *Rd = NULL;
		if(Ad->typeInfo == typeDouble || Ad->typeInfo == typeFloat)
			Rd = new NodeNumber(optDoOperation<double>(id, Ad->GetDouble(), Bd->GetDouble(), swapOper), typeDouble);
		else if(Ad->typeInfo == typeLong)
			Rd = new NodeNumber(optDoOperation<long long>(id, Ad->GetLong(), Bd->GetLong(), swapOper), typeLong);
		else if(Ad->typeInfo == typeInt)
			Rd = new NodeNumber(optDoOperation<int>(id, Ad->GetInteger(), Bd->GetInteger(), swapOper), typeInt);
		nodeList.push_back(Rd);
		return;
	}else if(aNodeType == typeNodeNumber || bNodeType == typeNodeNumber){
		NodeZeroOP *opA = nodeList[nodeList.size() - 2];
		NodeZeroOP *opB = nodeList[nodeList.size() - 1];

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

	// Optimizations failed, perform operation in run-time
	nodeList.push_back(new NodeBinaryOp(id));
	if(!lastError.IsEmpty())
		ThrowLastError();
}

void AddReturnNode(const char* pos, const char* end)
{
	int localReturn = currDefinedFunc.size() != 0;

	TypeInfo *realRetType = nodeList.back()->typeInfo;
	TypeInfo *expectedType = NULL;
	if(currDefinedFunc.size() != 0)
	{
		if(!currDefinedFunc.back()->retType)
		{
			currDefinedFunc.back()->retType = realRetType;
			currDefinedFunc.back()->funcType = GetFunctionType(currDefinedFunc.back());
		}
		expectedType = currDefinedFunc.back()->retType;
		if((expectedType->type == TypeInfo::TYPE_COMPLEX || realRetType->type == TypeInfo::TYPE_COMPLEX) && expectedType != realRetType)
		{
			sprintf(callbackError, "ERROR: function returns %s but supposed to return %s", realRetType->GetFullTypeName(), expectedType->GetFullTypeName());
			ThrowError(callbackError, pos);
		}
		if(expectedType->type == TypeInfo::TYPE_VOID && realRetType != typeVoid)
			ThrowError("ERROR: function returning a value", pos);
		if(expectedType != typeVoid && realRetType == typeVoid)
		{
			sprintf(callbackError, "ERROR: function should return %s", expectedType->GetFullTypeName());
			ThrowError(callbackError, pos);
		}
	}else{
		if(currDefinedFunc.size() == 0 && realRetType == typeVoid)
			ThrowError("ERROR: global return cannot accept void", pos);
		expectedType = realRetType;
	}
	nodeList.push_back(new NodeReturnOp(localReturn, expectedType));
	nodeList.back()->SetCodeInfo(pos, end);
}

void AddBreakNode(const char* pos)
{
	if(cycleDepth.back() == 0)
		ThrowError("ERROR: break used outside loop statement", pos);

	nodeList.push_back(new NodeBreakOp());
}

void AddContinueNode(const char* pos)
{
	if(cycleDepth.back() == 0)
		ThrowError("ERROR: continue used outside loop statement", pos);

	nodeList.push_back(new NodeContinueOp());
}

void SelectAutoType()
{
	currType = NULL;
}

void SelectTypeByIndex(unsigned int index)
{
	currType = typeInfo[index];
}

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
		unsigned int offset = GetAlignmentOffset(pos, currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : currType->alignBytes);
		varTop += offset;
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
	nodeList.push_back(new NodeNumber((int)currTypes.back()->size, typeInt));
}

void SetTypeOfLastNode()
{
	currType = nodeList.back()->typeInfo;
	nodeList.pop_back();
}

void AddInplaceArray(const char* pos);

// Function that retrieves variable address
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
			sprintf(callbackError, "ERROR: function '%.*s' is not defined", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}

		if(FindFunctionByName(hash, fID - 1) != -1)
		{
			sprintf(callbackError, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}

		// Push the type in type stack
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
				nodeList.push_back(new NodeNumber(0, GetReferenceType(typeInt)));
			}else{
				AddGetAddressNode(pos, InplaceStr(contextName, length));
				currTypes.pop_back();
			}
		}

		// Create node that retrieves function address
		nodeList.push_back(new NodeFunctionAddress(funcInfo[fID]));
	}else{
		// Push the type in type stack
		currTypes.push_back(varInfo[i]->varType);

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

				TypeInfo *temp = GetReferenceType(newType);
				currTypes.push_back(temp);

				nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));

				AddDereferenceNode(pos);
				AddMemberAccessNode(pos, varName);

				currTypes.pop_back();
				return;
			}
		}

		// If we try to access external variable from local function
		if(currDefinedFunc.size() != 0 && (currDefinedFunc.back()->type == FunctionInfo::LOCAL) && i < (int)varInfoTop[currDefinedFunc.back()->vTopSize].activeVarCnt)
		{
			FunctionInfo *currFunc = currDefinedFunc.back();
			// Add variable name to the list of function external variables
			int num = AddFunctionExternal(currFunc, varName);

			TypeInfo *temp = GetReferenceType(typeInt);
			temp = GetArrayType(temp, (int)currDefinedFunc.back()->externalCount);
			if(!temp)
				ThrowLastError();
			temp = GetReferenceType(temp);
			currTypes.push_back(temp);

			nodeList.push_back(new NodeGetAddress(NULL, currFunc->allParamSize, false, temp));
			AddDereferenceNode(pos);
			nodeList.push_back(new NodeNumber(num, typeInt));
			AddArrayIndexNode(pos);
			AddDereferenceNode(pos);
			// Remove current type
			currTypes.pop_back();
		}else{
			// If variable is in global scope, use absolute address
			bool absAddress = ((varInfoTop.size() > 1) && (varInfo[i]->pos < varInfoTop[1].varStackSize)) || currDefinedFunc.size() == 0;

			int varAddress = varInfo[i]->pos;
			if(!absAddress)
				varAddress -= (int)(varInfoTop[currDefinedFunc.back()->vTopSize].varStackSize);

			// Create node that places variable address on stack
			nodeList.push_back(new NodeGetAddress(varInfo[i], varAddress, absAddress, varInfo[i]->varType));
		}
	}
}

// Function for array indexing
void AddArrayIndexNode(const char* pos)
{
	lastKnownStartPos = pos;

	// Current type must be an array
	if(currTypes.back()->arrLevel == 0)
		ThrowError("ERROR: indexing variable that is not an array", pos);
	// If it is array without explicit size (pointer to array)
	if(currTypes.back()->arrSize == TypeInfo::UNSIZED_ARRAY)
	{
		// Then, before indexing it, we need to get address from this variable
		NodeZeroOP* temp = nodeList.back();
		nodeList.pop_back();
		nodeList.push_back(new NodeDereference(GetReferenceType(currTypes.back()->subType)));
		nodeList.push_back(temp);
	}
	// If index is a number and previous node is an address, then indexing can be done in compile-time
	if(nodeList.back()->nodeType == typeNodeNumber && nodeList[nodeList.size()-2]->nodeType == typeNodeGetAddress)
	{
		// Get shift value
		int shiftValue = static_cast<NodeNumber*>(nodeList.back())->GetInteger();

		// Check bounds
		if(shiftValue < 0)
			ThrowError("ERROR: Array index cannot be negative", pos);
		if((unsigned int)shiftValue >= currTypes.back()->arrSize)
			ThrowError("ERROR: Array index out of bounds", pos);

		// Index array
		static_cast<NodeGetAddress*>(nodeList[nodeList.size()-2])->IndexArray(shiftValue);
		nodeList.pop_back();
	}else{
		// Otherwise, create array indexing node
		nodeList.push_back(new NodeArrayIndex(currTypes.back()));
		if(!lastError.IsEmpty())
			ThrowLastError();
	}
	// Change current type to array element type
	currTypes.back() = currTypes.back()->subType;
}

// Function for pointer dereferencing
void AddDereferenceNode(const char* pos)
{
	lastKnownStartPos = pos;

	// Create dereference node
	nodeList.push_back(new NodeDereference(currTypes.back()));
	// Change current type to type that pointer pointed to
	currTypes.back() = GetDereferenceType(currTypes.back());
	if(!currTypes.back())
		ThrowLastError();
}

// Compiler expects that after variable there will be assignment operator
// If it's not the case, last node has to be removed
void FailedSetVariable()
{
	nodeList.pop_back();
}

// Function for variable assignment in place of definition
void AddDefineVariableNode(const char* pos, InplaceStr varName)
{
	lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	int i = FindVariableByName(hash);
	if(i == -1)
	{
		sprintf(callbackError, "ERROR: variable '%.*s' is not defined", varName.end-varName.begin, varName.begin);
		ThrowError(callbackError, pos);
	}
	// Put variable type in type stack
	currTypes.push_back(varInfo[i]->varType);

	// If variable is in global scope, use absolute address
	bool absAddress = ((varInfoTop.size() > 1) && (varInfo[i]->pos < varInfoTop[1].varStackSize)) || currDefinedFunc.size() == 0;

	// If current type is set to NULL, it means that current type is auto
	// Is such case, type is retrieved from last AST node
	TypeInfo *realCurrType = currTypes.back() ? currTypes.back() : nodeList.back()->typeInfo;

	// Maybe additional node will be needed to define variable
	// Variable signalizes that two nodes need to be unified in one
	bool unifyTwo = false;
	// If variable type is array without explicit size, and it is being defined with value of different type
	if(realCurrType->arrSize == TypeInfo::UNSIZED_ARRAY && realCurrType != nodeList.back()->typeInfo)
	{
		TypeInfo *nodeType = nodeList.back()->typeInfo;
		// If subtype of both variables (presumably, arrays) is equal
		if(realCurrType->subType == nodeType->subType)
		{
			// And if to the right of assignment operator there is no pointer dereference
			if(nodeList.back()->nodeType != typeNodeDereference)
			{
				// Then, to the right of assignment operator there is array definition using inplace array
				if(nodeList.back()->nodeType == typeNodeExpressionList)
				{
					// Create node for assignment of inplace array to implicit variable
					AddInplaceArray(pos);
					// Now we have two nodes, so they are to be unified
					unifyTwo = true;
				}else{
					// Or if not, then types aren't compatible, so throw error
					sprintf(callbackError, "ERROR: cannot convert from %s to %s", nodeList.back()->typeInfo->GetFullTypeName(), realCurrType->GetFullTypeName());
					ThrowError(callbackError, pos);
				}
			}
			// Because we are assigning array of explicit size to a pointer to array, we have to put a pair of pointer:size on top of stack
			// Take pointer to an array (node that is inside of pointer dereference node)
			NodeZeroOP	*oldNode = nodeList.back();
			nodeList.back() = static_cast<NodeDereference*>(oldNode)->GetFirstNode();
			static_cast<NodeDereference*>(oldNode)->SetFirstNode(NULL);
			// Find the size of an array
			unsigned int typeSize = (nodeType->size - nodeType->paddingBytes) / nodeType->subType->size;
			// Create expression list with return type of implicit size array
			// Node constructor will take last node
			NodeExpressionList *listExpr = new NodeExpressionList(varInfo[i]->varType);
			// Create node that places array size on top of the stack
			nodeList.push_back(new NodeNumber((int)typeSize, typeInt));
			// Add it to expression list
			listExpr->AddNode();
			// Add expression list to node list
			nodeList.push_back(listExpr);
		}
	}
	// If a function is being assigned to variable, then take it's address
	if(nodeList.back()->nodeType == typeNodeFuncDef ||
		(nodeList.back()->nodeType == typeNodeExpressionList && static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode()->nodeType == typeNodeFuncDef))
	{
		NodeFuncDef*	funcDefNode = (NodeFuncDef*)(nodeList.back()->nodeType == typeNodeFuncDef ? nodeList.back() : static_cast<NodeExpressionList*>(nodeList.back())->GetFirstNode());
		AddGetAddressNode(pos, InplaceStr(funcDefNode->GetFuncInfo()->name, funcDefNode->GetFuncInfo()->nameLength));
		currTypes.pop_back();
		unifyTwo = true;
		if(!currTypes.back())
			realCurrType = nodeList.back()->typeInfo;
		varDefined = true;
		varTop -= realCurrType->size;
	}
	// If type wasn't known until assignment, it means that variable alignment wasn't performed in AddVariable function
	if(!currTypes.back())
	{
		// If type has default alignment or if user specified it
		if(realCurrType->alignBytes != 0 || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
		{
			// Find address offset. Alignment selected by user has higher priority than default alignment
			unsigned int offset = GetAlignmentOffset(pos, currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : realCurrType->alignBytes);
			varInfo[i]->pos += offset;
			varTop += offset;
		}
		varInfo[i]->varType = realCurrType;
		varTop += realCurrType->size;
	}
	varInfo[i]->dataReserved = true;

	unsigned int varPosition = varInfo[i]->pos;
	if(!absAddress)
		varPosition -= (int)(varInfoTop[currDefinedFunc.back()->vTopSize].varStackSize);

	nodeList.push_back(new NodeGetAddress(varInfo[i], varPosition, absAddress, varInfo[i]->varType));

	nodeList.push_back(new NodeVariableSet(realCurrType, true, false));
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
			nodeList.push_back(new NodeNumber((int)typeSize, typeInt));
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

	// Beware that there is a global variable with the same name
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
			sprintf(callbackError, "ERROR: function '%.*s' is not defined", varName.end-varName.begin, varName.begin);
			ThrowError(callbackError, pos);
		}

		if(FindFunctionByName(hash, fID - 1) != -1)
		{
			sprintf(callbackError, "ERROR: there are more than one '%.*s' function, and the decision isn't clear", varName.end-varName.begin, varName.begin);
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
		// Node that gets function address
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
	if(!lastError.IsEmpty())
		ThrowLastError();
}

void AddModifyVariableNode(const char* pos, CmdID cmd)
{
	lastKnownStartPos = pos;

	TypeInfo *targetType = GetDereferenceType(nodeList[nodeList.size()-2]->typeInfo);
	if(!targetType)
		ThrowLastError();
	nodeList.push_back(new NodeVariableModify(targetType, cmd));
	if(!lastError.IsEmpty())
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
	funcInfo.push_back(new FunctionInfo(funcNameCopy));
	funcInfo.back()->vTopSize = (unsigned int)varInfoTop.size();
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
	BeginBlock();
	cycleDepth.push_back(0);

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

	funcInfo.back()->funcType = funcInfo.back()->retType ? GetFunctionType(funcInfo.back()) : NULL;
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

	cycleDepth.pop_back();
	EndBlock();
	unsigned int varFormerTop = varTop;
	varTop = varInfoTop[lastFunc.vTopSize].varStackSize;

	nodeList.push_back(new NodeFuncDef(funcInfo[i], varFormerTop-varTop));
	funcDefList.push_back(nodeList.back());

	if(!currDefinedFunc.back()->retType)
	{
		currDefinedFunc.back()->retType = typeVoid;
		currDefinedFunc.back()->funcType = GetFunctionType(currDefinedFunc.back());
	}
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

unsigned int GetFunctionRating(FunctionInfo *currFunc, unsigned int callArgCount)
{
	if(currFunc->paramCount != callArgCount)
		return ~0u;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.

	unsigned int fRating = 0;
	unsigned int n = 0;
	for(VariableInfo *curr = currFunc->firstParam; curr; curr = curr->next, n++)
	{
		NodeZeroOP* activeNode = nodeList[nodeList.size() - currFunc->paramCount + n];
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

void AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount)
{
	unsigned int funcNameHash = GetStringHash(funcName);

	// Searching, if function name is actually a variable name (which means, either it is a pointer to function, or an error)
	int vID = FindVariableByName(funcNameHash);

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	if(vID == -1)
	{
		//Find all functions with given name
		bestFuncList.clear();

		for(unsigned int k = 0; k < funcInfo.size(); k++)
		{
			if(funcInfo[k]->nameHash == funcNameHash && funcInfo[k]->visible)
				bestFuncList.push_back(funcInfo[k]);
		}
		unsigned int count = bestFuncList.size();
		if(count == 0)
		{
			sprintf(callbackError, "ERROR: function '%s' is undefined", funcName);
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
		if(minRatingIndex == -1)
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
				for(VariableInfo *curr = bestFuncList[n]->firstParam; curr; curr = curr->next)
					errPos += sprintf(errPos, "%s%s", curr->varType->GetFullTypeName(), curr != bestFuncList[n]->lastParam ? ", " : "");
				errPos += sprintf(errPos, ")\r\n");
			}
			lastError = CompilerError(errTemp, pos);
			ThrowLastError();
		}
		// Check, is there are more than one function, that share the same rating
		for(unsigned int k = 0; k < count; k++)
		{
			if(k != minRatingIndex && bestFuncRating[k] == minRating)
			{
				char errTemp[512];
				char	*errPos = errTemp;
				errPos += sprintf(errPos, "ambiguity, there is more than one overloaded function available for the call.\r\n  %s(", funcName);
				for(unsigned int n = 0; n < callArgCount; n++)
					errPos += sprintf(errPos, "%s%s", nodeList[nodeList.size()-callArgCount+n]->typeInfo->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
				errPos += sprintf(errPos, ")\r\n  candidates are:\r\n");
				for(unsigned int n = 0; n < count; n++)
				{
					if(bestFuncRating[n] != minRating)
						continue;
					errPos += sprintf(errPos, "  %s(", funcName);
					for(VariableInfo *curr = bestFuncList[n]->firstParam; curr; curr = curr->next)
						errPos += sprintf(errPos, "%s%s", curr->varType->GetFullTypeName(), curr != bestFuncList[n]->lastParam ? ", " : "");
					errPos += sprintf(errPos, ")\r\n");
				}
				lastError = CompilerError(errTemp, pos);
				ThrowLastError();
			}
		}
		fType = bestFuncList[minRatingIndex]->funcType->funcType;
		fInfo = bestFuncList[minRatingIndex];
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
			nodeList.push_back(new NodeNumber((int)typeSize, typeInt));
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
			nodeList.push_back(new NodeNumber(0, GetReferenceType(typeInt)));
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

void AddIfNode(const char* pos)
{
	assert(nodeList.size() >= 2);
	if(nodeList[nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	nodeList.push_back(new NodeIfElseExpr(false));
}
void AddIfElseNode(const char* pos)
{
	assert(nodeList.size() >= 3);
	if(nodeList[nodeList.size()-3]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	nodeList.push_back(new NodeIfElseExpr(true));
}
void AddIfElseTermNode(const char* pos)
{
	assert(nodeList.size() >= 3);
	if(nodeList[nodeList.size()-3]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	TypeInfo* typeA = nodeList[nodeList.size()-1]->typeInfo;
	TypeInfo* typeB = nodeList[nodeList.size()-2]->typeInfo;
	if(typeA != typeB && (typeA->type == TypeInfo::TYPE_COMPLEX || typeB->type == TypeInfo::TYPE_COMPLEX))
	{
		sprintf(callbackError, "ERROR: ternary operator ?: \r\n result types are not equal (%s : %s)", typeB->name, typeA->name);
		ThrowError(callbackError, pos);
	}
	nodeList.push_back(new NodeIfElseExpr(true, true));
}

void IncreaseCycleDepth()
{
	assert(cycleDepth.size() != 0);
	cycleDepth.back()++;
}
void AddForNode(const char* pos)
{
	assert(nodeList.size() >= 4);
	if(nodeList[nodeList.size()-3]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	nodeList.push_back(new NodeForExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}
void AddWhileNode(const char* pos)
{
	assert(nodeList.size() >= 2);
	if(nodeList[nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	nodeList.push_back(new NodeWhileExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}
void AddDoWhileNode(const char* pos)
{
	assert(nodeList.size() >= 2);
	if(nodeList[nodeList.size()-1]->typeInfo == typeVoid)
		ThrowError("ERROR: condition type cannot be void", pos);
	nodeList.push_back(new NodeDoWhileExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}

void BeginSwitch(const char* pos)
{
	assert(nodeList.size() >= 1);
	if(nodeList[nodeList.size()-1]->typeInfo == typeVoid)
		ThrowError("ERROR: cannot switch by void type", pos);

	assert(cycleDepth.size() != 0);
	cycleDepth.back()++;

	BeginBlock();
	nodeList.push_back(new NodeSwitchExpr());
}
void AddCaseNode(const char* pos)
{
	assert(nodeList.size() >= 3);
	if(nodeList[nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError("ERROR: case value type cannot be void", pos);
	NodeZeroOP* temp = nodeList[nodeList.size()-3];
	static_cast<NodeSwitchExpr*>(temp)->AddCase();
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

	BeginBlock();
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
	varTop -= newType->size;

	nodeList.push_back(new NodeZeroOP());
	for(TypeInfo::MemberFunction *curr = newType->firstFunction; curr; curr = curr->next)
		AddTwoExpressionNode();

	newType = NULL;

	EndBlock(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AddUnfixedArraySize()
{
	nodeList.push_back(new NodeNumber(1, typeVoid));
}

// This function map source code lines to instructions
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
	VariableInfo::DeleteVariableInformation();

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

	varInfoTop.clear();
	varInfoTop.push_back(VarTopInfo(0,0));

	funcInfoTop.clear();
	funcInfoTop.push_back(0);

	cycleDepth.clear();
	cycleDepth.push_back(0);
}

void CallbackDeinitialize()
{
	VariableInfo::DeleteVariableInformation();
}
