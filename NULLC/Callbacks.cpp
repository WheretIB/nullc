// NULLC (c) NULL_PTR 2007-2010

#include "Callbacks.h"
#include "CodeInfo.h"

#include "Parser.h"
#include "HashMap.h"
#include "ConstantFold.h"

// Temp variables

// variable position from base of current stack frame
unsigned int varTop;

// Current variable alignment, in bytes
unsigned int currAlign;

// Is some variable being defined at the moment
bool varDefined;

unsigned int	instanceDepth;
char			*errorReport = NULL;

// Number of implicit variable
unsigned int inplaceVariableNum;

FunctionInfo	*uncalledFunc = NULL;
const char		*uncalledPos = NULL;

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
// A list of generic function instances that should be created in the right scope
FastVector<FunctionInfo*>	delayedInstance;

HashMap<FunctionInfo*>		funcMap;
HashMap<VariableInfo*>		varMap;

void	AddFunctionToSortedList(FunctionInfo *info)
{
	funcMap.insert(info->nameHash, info);
}

const char*	currFunction;
const char*	SetCurrentFunction(const char* func)
{
	const char* lastFunction = currFunction;
	currFunction = func;
	return lastFunction;
}
unsigned	currArgument = 0;
unsigned	SetCurrentArgument(unsigned argument)
{
	unsigned lastArgument = currArgument;
	currArgument = argument;
	return lastArgument;
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

FastVector<FunctionInfo*>	bestFuncListBackup;
FastVector<unsigned int>	bestFuncRatingBackup;

FastVector<NodeZeroOP*>	paramNodes;

void AddInplaceVariable(const char* pos, TypeInfo* targetType = NULL);
void ConvertArrayToUnsized(const char* pos, TypeInfo *dstType);
NodeZeroOP* CreateGenericFunctionInstance(const char* pos, FunctionInfo* fInfo, FunctionInfo*& fResult, TypeInfo* forcedParentType = NULL);
void ConvertFunctionToPointer(const char* pos, TypeInfo *dstPreferred = NULL);
void HandlePointerToObject(const char* pos, TypeInfo *dstType);
void ThrowFunctionSelectError(const char* pos, unsigned minRating, char* errorReport, char* errPos, const char* funcName, unsigned callArgCount, unsigned count);

void AddExtraNode()
{
	NodeZeroOP *last = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	last->AddExtraNode();
	CodeInfo::nodeList.push_back(last);
}

void SetCurrentAlignment(unsigned int alignment)
{
	currAlign = alignment;
}

// Finds variable inside function external variable list, and if not found, adds it to a list
FunctionInfo::ExternalInfo* AddFunctionExternal(FunctionInfo* func, VariableInfo* var)
{
	unsigned int i = 0;
	for(FunctionInfo::ExternalInfo *curr = func->firstExternal; curr; curr = curr->next, i++)
		if(curr->variable == var)
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
		{
			// Check that local function prototypes have implementation before they go out of scope
			// Skip generic functions and function prototypes for generic function instances
			if(!CodeInfo::funcInfo[i]->implemented && !(CodeInfo::funcInfo[i]->address & 0x80000000) && !CodeInfo::funcInfo[i]->generic && !CodeInfo::funcInfo[i]->genericBase)
				ThrowError(CodeInfo::lastKnownStartPos, "ERROR: local function '%s' went out of scope unimplemented", CodeInfo::funcInfo[i]->name);
			// generic function instance will go out of scope only when base function goes out of scope
			// member function of a generic type will never go out of scope
			CodeInfo::funcInfo[i]->visible = CodeInfo::funcInfo[i]->parentClass ? true : (CodeInfo::funcInfo[i]->genericBase ? CodeInfo::funcInfo[i]->genericBase->parent->visible : false);
		}
	}
	funcInfoTop.pop_back();

	// Check delayed function instances
	for(unsigned i = 0; i < delayedInstance.size(); i++)
	{
		// If the current scope is equal to generic function scope, instantiate it
		if(delayedInstance[i] && delayedInstance[i]->vTopSize == varInfoTop.size())
		{
			// Take prototype info
			FunctionInfo *fProto = delayedInstance[i];
			// Mark function as instanced
			delayedInstance[i] = NULL;

			const char *pos = CodeInfo::lastKnownStartPos;
			FunctionType *fType = fProto->funcType->funcType;

			// Get ID of the function that will be created
			unsigned funcID = CodeInfo::funcInfo.size();
			FunctionInfo *fInfo = fProto->genericBase->parent;
			// There may be a type in definition, and we must save it
			TypeInfo *currDefinedType = newType;
			unsigned int currentDefinedTypeMethodCount = methodCount;
			newType = NULL;
			methodCount = 0;
			if(fInfo->type == FunctionInfo::THISCALL)
			{
				currType = fInfo->parentClass;
				TypeContinue(pos);
			}

			// Function return type
			currType = fType->retType;
			if(fInfo->parentClass)
				assert(strchr(fInfo->name, ':'));
			FunctionAdd(pos, fInfo->parentClass ? strchr(fInfo->name, ':') + 2 : fInfo->name);

			// Get aliases from prototype argument list
			AliasInfo *aliasFromParent = fProto->childAlias;
			while(aliasFromParent)
			{
				CodeInfo::classMap.insert(aliasFromParent->nameHash, aliasFromParent->type);
				aliasFromParent = aliasFromParent->next;
			}
			CodeInfo::funcInfo.back()->childAlias = fProto->childAlias;

			// New function type is equal to generic function type no matter where we create an instance of it
			CodeInfo::funcInfo.back()->type = fInfo->type;
			CodeInfo::funcInfo.back()->parentClass = fInfo->parentClass;

			// Start of function source
			Lexeme *start = CodeInfo::lexStart + fInfo->generic->start;
			CodeInfo::lastKnownStartPos = NULL;

			// Get function parameters from function prototype
			for(VariableInfo *curr = fProto->firstParam; curr; curr = curr->next)
			{
				currType = curr->varType;
				FunctionParameter(pos, curr->name);
			}
			// Skip function parameters in source
			unsigned parenthesis = 1;
			while(parenthesis)
			{
				if(start->type == lex_oparen)
					parenthesis++;
				else if(start->type == lex_cparen)
					parenthesis--;
				start++;
			}
			start++;

			// Because we reparse a generic function, our function may be erroneously marked as generic
			CodeInfo::funcInfo[funcID]->generic = NULL;
			CodeInfo::funcInfo[funcID]->genericBase = fInfo->generic;

			fProto->address |= 0x80000000;
			// Reparse function code
			FunctionStart(pos);
			const char *lastFunc = SetCurrentFunction(NULL);
			if(!ParseCode(&start))
				AddVoidNode();
			SetCurrentFunction(lastFunc);
			FunctionEnd(start->pos);
			if(fInfo->type == FunctionInfo::THISCALL)
				TypeStop();
			// Restore type that was in definition
			methodCount = currentDefinedTypeMethodCount;
			newType = currDefinedType;

			// Remove function definition node, it was placed to funcDefList in FunctionEnd
			CodeInfo::nodeList.pop_back();
		}
	}
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

void CheckCollisionWithFunction(const char* pos, InplaceStr varName, unsigned hash, unsigned scope)
{
	HashMap<FunctionInfo*>::Node *curr = funcMap.first(hash);
	while(curr)
	{
		if(curr->value->visible && curr->value->vTopSize == scope)
			ThrowError(pos, "ERROR: name '%.*s' is already taken for a function", varName.end - varName.begin, varName.begin);
		curr = funcMap.next(curr);
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
	// skip leading zeros
	while(*pos == '0')
		pos++;
	if(int(end - pos) > 16)
		ThrowError(pos, "ERROR: overflow in hexadecimal constant");
	long long num = parseLong(pos, end, 16);
	// If number overflows integer number, create long number
	if(int(num) == num)
		CodeInfo::nodeList.push_back(new NodeNumber(int(num), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(num, typeLong));
}

void AddOctInteger(const char* pos, const char* end)
{
	pos++;
	// skip leading zeros
	while(*pos == '0')
		pos++;
	if(int(end - pos) > 22 || (int(end - pos) > 21 && *pos != '1'))
		ThrowError(pos, "ERROR: overflow in octal constant");
	long long num = parseLong(pos, end, 8);
	// If number overflows integer number, create long number
	if(int(num) == num)
		CodeInfo::nodeList.push_back(new NodeNumber(int(num), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(num, typeLong));
}

void AddBinInteger(const char* pos, const char* end)
{
	// skip leading zeros
	while(*pos == '0')
		pos++;
	if(int(end - pos) > 64)
		ThrowError(pos, "ERROR: overflow in binary constant");
	long long num = parseLong(pos, end, 2);
	// If number overflows integer number, create long number
	if(int(num) == num)
		CodeInfo::nodeList.push_back(new NodeNumber(int(num), typeInt));
	else
		CodeInfo::nodeList.push_back(new NodeNumber(num, typeLong));
}

void AddGeneratorReturnData(const char *pos)
{
	// Check that we are inside the function
	assert(currDefinedFunc.size());
	TypeInfo *retType = currDefinedFunc.back()->retType;
	// Check that return type is known
	if(!retType)
		ThrowError(pos, "ERROR: not a single element is generated, and an array element type is unknown");
	if(retType == typeVoid)
		ThrowError(pos, "ERROR: cannot generate an array of 'void' element type");
	currType = retType;
	CodeInfo::nodeList.push_back(new NodeZeroOP(retType));
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
void AddStringNode(const char* s, const char* e, bool unescaped)
{
	CodeInfo::lastKnownStartPos = s;

	const char *curr = s + 1, *end = e - 1;
	unsigned int len = 0;
	// Find the length of the string with collapsed escape-sequences
	for(; curr < end; curr++, len++)
	{
		if(*curr == '\\' && !unescaped)
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
			if(*curr == '\\' && !unescaped)
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
	CodeInfo::lastKnownStartPos = pos;
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
	CodeInfo::lastKnownStartPos = pos;
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
	CodeInfo::lastKnownStartPos = pos;
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

	const char *opNames[] = { "+", "-", "*", "/", "**", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "&", "|", "^", "&&", "||", "^^" };

	if(id == cmdEqual || id == cmdNEqual)
	{
		NodeZeroOP *left = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
		NodeZeroOP *right = CodeInfo::nodeList[CodeInfo::nodeList.size()-1];

		if(right->typeInfo == typeObject && right->typeInfo == left->typeInfo)
		{
			AddFunctionCallNode(pos, id == cmdEqual ? "__rcomp" : "__rncomp", 2);
			return;
		}
		bool swapped = false;
		if(left->typeInfo == typeVoid->refType)
		{
			CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = right;
			CodeInfo::nodeList[CodeInfo::nodeList.size()-1] = left;
			NodeZeroOP *tmp = left; left = right; right = tmp;
			swapped = true;
		}
		if(right->typeInfo == typeVoid->refType)
		{
			HandlePointerToObject(pos, left->typeInfo);
			left = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
			right = CodeInfo::nodeList[CodeInfo::nodeList.size()-1];
			if(right->typeInfo == typeVoid->refType && swapped)
			{
				CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = left;
				CodeInfo::nodeList[CodeInfo::nodeList.size()-1] = right;
				NodeZeroOP *tmp = left; left = right; right = tmp;
			}
		}
		if(right->typeInfo->funcType && right->typeInfo->funcType == left->typeInfo->funcType)
		{
			CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo = CodeInfo::funcInfo[0]->funcType;
			CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo = CodeInfo::funcInfo[0]->funcType;
			AddFunctionCallNode(pos, id == cmdEqual ? "__pcomp" : "__pncomp", 2);
			return;
		}
		if(right->typeInfo->arrLevel && right->typeInfo->arrSize == TypeInfo::UNSIZED_ARRAY && right->typeInfo == left->typeInfo)
		{
			if(!AddFunctionCallNode(CodeInfo::lastKnownStartPos, opNames[id - cmdAdd], 2, true))
				AddFunctionCallNode(pos, id == cmdEqual ? "__acomp" : "__ancomp", 2);
			return;
		}
		if(left->nodeType == typeNodeConvertPtr && left->typeInfo == typeTypeid && left->typeInfo == right->typeInfo && left->nodeType == right->nodeType)
		{
			CodeInfo::nodeList.pop_back();
			CodeInfo::nodeList.pop_back();
			if(((NodeConvertPtr*)left)->GetFirstNode()->typeInfo == ((NodeConvertPtr*)right)->GetFirstNode()->typeInfo)
				CodeInfo::nodeList.push_back(new NodeNumber(id == cmdEqual, typeInt));
			else
				CodeInfo::nodeList.push_back(new NodeNumber(id == cmdNEqual, typeInt));
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

	// Optimizations failed, perform operation in run-time
	if(!AddFunctionCallNode(CodeInfo::lastKnownStartPos, opNames[id - cmdAdd], 2, true))
		CodeInfo::nodeList.push_back(new NodeBinaryOp(id));
}

void AddReturnNode(const char* pos, bool yield)
{
	bool localReturn = currDefinedFunc.size() != 0;

	// If new function is returned, convert it to pointer
	ConvertFunctionToPointer(pos, currDefinedFunc.size() ? currDefinedFunc.back()->retType : NULL);
	
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
		if(expectedType->refLevel || (expectedType->arrLevel && expectedType->arrSize == TypeInfo::UNSIZED_ARRAY))
		{
			if(CodeInfo::nodeList.back()->nodeType != typeNodeZeroOp)
			{
				CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdCheckedRet, expectedType->arrLevel ? expectedType->typeIndex : expectedType->subType->typeIndex));
				((NodeUnaryOp*)CodeInfo::nodeList.back())->SetParentFunc(currDefinedFunc.back());
			}
		}
		// Check for errors
		if(((expectedType->type == TypeInfo::TYPE_COMPLEX || realRetType->type == TypeInfo::TYPE_COMPLEX) && expectedType != realRetType) || expectedType->subType != realRetType->subType)
			ThrowError(pos, "ERROR: function returns %s but supposed to return %s", realRetType->GetFullTypeName(), expectedType->GetFullTypeName());
		if(expectedType == typeVoid && realRetType != typeVoid)
			ThrowError(pos, "ERROR: 'void' function returning a value");
		if(expectedType != typeVoid && realRetType == typeVoid)
			ThrowError(pos, "ERROR: function should return %s", expectedType->GetFullTypeName());
		if(yield && currDefinedFunc.back()->type != FunctionInfo::COROUTINE)
			ThrowError(pos, "ERROR: yield can only be used inside a coroutine");
#ifdef NULLC_ENABLE_C_TRANSLATION
		if(yield)
			currDefinedFunc.back()->yieldCount++;
#endif
		currDefinedFunc.back()->explicitlyReturned = true;
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
	CodeInfo::nodeList.push_back(new NodeReturnOp(localReturn, expectedType, currDefinedFunc.size() ? currDefinedFunc.back() : NULL, yield));
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

void SelectTypeByPointer(TypeInfo* type)
{
	currType = type;
}

void SelectTypeForGeneric(unsigned nodeIndex)
{
	if(CodeInfo::nodeList[nodeIndex]->nodeType == typeNodeFuncDef)
		currType = ((NodeFuncDef*)CodeInfo::nodeList[nodeIndex])->GetFuncInfo()->funcType;
	else if(CodeInfo::nodeList[nodeIndex]->typeInfo->arrLevel && CodeInfo::nodeList[nodeIndex]->typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY && CodeInfo::nodeList[nodeIndex]->nodeType != typeNodeZeroOp)
		currType = CodeInfo::GetArrayType(CodeInfo::nodeList[nodeIndex]->typeInfo->subType, TypeInfo::UNSIZED_ARRAY);
	else
		currType = CodeInfo::nodeList[nodeIndex]->typeInfo;
}

void SelectTypeByIndex(unsigned int index)
{
	currType = CodeInfo::typeInfo[index];
}

TypeInfo* GetSelectedType()
{
	return currType;
}

const char* GetSelectedTypeName()
{
	return currType->GetFullTypeName();
}

VariableInfo* AddVariable(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	if(TypeInfo **info = CodeInfo::classMap.find(hash))
		ThrowError(pos, "ERROR: name '%.*s' is already taken for a class", varName.end-varName.begin, varName.begin);

	// Check for variables with the same name in current scope
	if(VariableInfo ** info = varMap.find(hash))
		if((*info)->blockDepth >= varInfoTop.size())
			ThrowError(pos, "ERROR: name '%.*s' is already taken for a variable in current scope", varName.end-varName.begin, varName.begin);
	// Check for functions with the same name
	CheckCollisionWithFunction(pos, varName, hash, varInfoTop.size());

	if((currType && currType->alignBytes != 0) || currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT)
	{
		unsigned int offset = GetAlignmentOffset(pos, currAlign != TypeInfo::UNSPECIFIED_ALIGNMENT ? currAlign : currType->alignBytes);
		varTop += offset;
	}
	if(currType && currType->hasFinalizer)
		ThrowError(pos, "ERROR: cannot create '%s' that implements 'finalize' on stack", currType->GetFullTypeName());
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
	varDefined = false;
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
	{
		if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && CodeInfo::nodeList.back()->typeInfo == typeVoid)
		{
			CodeInfo::nodeList.pop_back();
			currType = typeAutoArray;
			return;
		}
		ThrowError(pos, "ERROR: cannot specify array size for auto variable");
	}
	currType = CodeInfo::GetArrayType(currType);
}

//////////////////////////////////////////////////////////////////////////
//					New functions for work with variables

void GetTypeSize(const char* pos, bool sizeOfExpr)
{
	if(!sizeOfExpr && !currType)
		ThrowError(pos, "ERROR: sizeof(auto) is illegal");
	if(!currType->hasFinished)
		ThrowError(pos, "ERROR: cannot take size of a type in definition");
	if(sizeOfExpr)
	{
		currType = CodeInfo::nodeList.back()->typeInfo;
		CodeInfo::nodeList.pop_back();
	}
	CodeInfo::nodeList.push_back(new NodeNumber((int)currType->size, typeInt));
}

void GetTypeId(const char* pos)
{
	if(!currType)
		ThrowError(pos, "ERROR: cannot take typeid from auto type");
	if(CodeInfo::nodeList.size() && CodeInfo::nodeList.back()->nodeType == typeNodeNumber && CodeInfo::nodeList.back()->typeInfo == typeVoid)
	{
		CodeInfo::nodeList.back()->typeInfo = typeInt;
		return;
	}
	CodeInfo::nodeList.push_back(new NodeZeroOP(CodeInfo::GetReferenceType(currType)));
	CodeInfo::nodeList.push_back(new NodeConvertPtr(typeObject));
	CodeInfo::nodeList.back()->typeInfo = typeTypeid;
}

void SetTypeOfLastNode()
{
	currType = CodeInfo::nodeList.back()->typeInfo;
	CodeInfo::nodeList.pop_back();
}

void GetFunctionContext(const char* pos, FunctionInfo *fInfo, bool handleThisCall)
{
	if(fInfo->type == FunctionInfo::LOCAL || fInfo->type == FunctionInfo::COROUTINE)
	{
		VariableInfo **info = NULL;
		InplaceStr	context;
		// If function is already implemented, we can take context variable information from function information
		if(fInfo->funcContext)
		{
			info = &fInfo->funcContext;
			context = fInfo->funcContext->name;
		}else{
			char	*contextName = AllocateString(fInfo->nameLength + 24);
			int length = sprintf(contextName, "$%s_%d_ext", fInfo->name, fInfo->indexInArr);
			unsigned int contextHash = GetStringHash(contextName);
			info = varMap.find(contextHash);
			context = InplaceStr(contextName, length);
		}
		if(!info)
		{
			CodeInfo::nodeList.push_back(new NodeNumber(0, CodeInfo::GetReferenceType(typeInt)));
		}else{
			AddGetAddressNode(pos, context);
			// This could be a forward-declared context, so make sure address will be correct if it changes
			if(CodeInfo::nodeList.back()->nodeType == typeNodeGetAddress)
				((NodeGetAddress*)CodeInfo::nodeList.back())->SetAddressTracking();
			CodeInfo::nodeList.push_back(new NodeDereference());
		}
	}else if(fInfo->type == FunctionInfo::THISCALL && handleThisCall){
		AddGetAddressNode(pos, InplaceStr("this", 4));
		CodeInfo::nodeList.push_back(new NodeDereference());
	}
}

// Function that retrieves variable address
void AddGetAddressNode(const char* pos, InplaceStr varName, TypeInfo *forcedPreferredType, NodeZeroOP *forcedThisNode)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	HashMap<VariableInfo*>::Node *curr = varMap.first(hash);
	
	// In generic function instance, skip all variables that are defined after the base generic function
	while(curr && currDefinedFunc.size() && currDefinedFunc.back()->genericBase && !(curr->value->pos >> 24) && !(curr->value->parentType) && (curr->value->isGlobal ? currDefinedFunc.back()->genericBase->globalVarTop <= curr->value->pos : (currDefinedFunc.back()->genericBase->blockDepth < curr->value->blockDepth && currDefinedFunc.back() != curr->value->parentFunction)))
		curr = varMap.next(curr);

	if(!curr)
	{
		int fID = -1;
		if(newType)
		{
			unsigned int hash = GetStringHash(GetClassFunctionName(newType, varName));
			fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);

			if(CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
			{
				FunctionInfo *fInfo = CodeInfo::funcInfo[fID];
				if(fInfo->generic)
					ThrowError(pos, "ERROR: can't take pointer to a generic function");
				assert(!forcedThisNode);
				GetFunctionContext(pos, fInfo, true);
				fInfo->pure = false;
				CodeInfo::nodeList.push_back(new NodeFunctionProxy(fInfo, pos, false, true));
				return;
			}
		}
		if(fID == -1)
			fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);
		if(fID == -1)
			ThrowError(pos, "ERROR: variable or function '%.*s' is not defined", varName.end-varName.begin, varName.begin);

		if(CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
		{
			int fIdFirst = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);
			fID = -1;
			TypeInfo *preferredType = forcedPreferredType;
			if(preferredType)
			{
				HashMap<FunctionInfo*>::Node *curr = funcMap.first(hash);
				while(curr)
				{
					FunctionInfo *func = curr->value;
					if(func->visible && !((func->address & 0x80000000) && (func->address != -1)) && func->funcType && func->funcType == preferredType)
					{
						fID = func->indexInArr;
						break;
					}
					curr = funcMap.next(curr);
				}
			}
			if(fID == -1)
			{
				CodeInfo::nodeList.push_back(new NodeFunctionProxy(CodeInfo::funcInfo[fIdFirst], pos));
				return;
			}
		}
		FunctionInfo *fInfo = CodeInfo::funcInfo[fID];
		if(fInfo->generic)
			ThrowError(pos, "ERROR: can't take pointer to a generic function");

		GetFunctionContext(pos, fInfo, !forcedThisNode);
		if(forcedThisNode)
			CodeInfo::nodeList.push_back(forcedThisNode);
		// Create node that retrieves function address
		fInfo->pure = false;
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(fInfo));
	}else{
		VariableInfo *vInfo = curr->value;
		if(!vInfo->varType)
			ThrowError(pos, "ERROR: variable '%.*s' is being used while its type is unknown", varName.end-varName.begin, varName.begin);
		if(newType && vInfo->isGlobal)
		{
			TypeInfo::MemberVariable *curr = newType->firstVariable;
			for(; curr; curr = curr->next)
				if(curr->nameHash == hash)
					break;
			if(curr && currDefinedFunc.size())
			{
				// Class members are accessed through 'this' pointer
				FunctionInfo *currFunc = currDefinedFunc.back();

				TypeInfo *temp = CodeInfo::GetReferenceType(newType);
				if(currDefinedFunc.back()->type == FunctionInfo::LOCAL)
				{
					// For local function, add "this" to context and get it from upvalue
					assert(currDefinedFunc[0]->type == FunctionInfo::THISCALL);
					FunctionInfo::ExternalInfo *external = AddFunctionExternal(currFunc, currDefinedFunc[0]->extraParam);
					CodeInfo::nodeList.push_back(new NodeGetUpvalue(currFunc, currFunc->allParamSize, external->closurePos, CodeInfo::GetReferenceType(temp)));
				}else{
					CodeInfo::nodeList.push_back(new NodeGetAddress(currFunc->extraParam, currFunc->allParamSize, temp));
				}
				CodeInfo::nodeList.push_back(new NodeDereference());
				CodeInfo::nodeList.push_back(new NodeShiftAddress(curr));
				return;
			}else if(curr){ // If we are in a type definition but not in a function, then this must be an typeof expression
				CodeInfo::nodeList.push_back(new NodeGetAddress(vInfo, vInfo->pos, vInfo->varType));
				return;
			}
		}

		bool externalAccess = currDefinedFunc.size() && vInfo->blockDepth > currDefinedFunc[0]->vTopSize && vInfo->blockDepth <= currDefinedFunc.back()->vTopSize;
		// If we try to access external variable from local function, or if we try to access external variable or local variable from coroutine
		if(currDefinedFunc.size() &&
			(
				(currDefinedFunc.back()->type == FunctionInfo::LOCAL && externalAccess)
				||
				(currDefinedFunc.back()->type == FunctionInfo::COROUTINE && (externalAccess || (vInfo->blockDepth > currDefinedFunc.back()->vTopSize && vInfo->pos > currDefinedFunc.back()->allParamSize)))
			)
		)
		{
			// If that variable is not in current scope, we have to get it through current closure
			FunctionInfo *currFunc = currDefinedFunc.back();
			// Add variable to the list of function external variables
			FunctionInfo::ExternalInfo *external = AddFunctionExternal(currFunc, vInfo);

			assert(currFunc->allParamSize % 4 == 0);
			CodeInfo::nodeList.push_back(new NodeGetUpvalue(currFunc, currFunc->allParamSize, external->closurePos, CodeInfo::GetReferenceType(vInfo->varType)));
			if(external->variable->autoDeref)
				AddGetVariableNode(pos);
		}else{
			// Create node that places variable address on stack
			CodeInfo::nodeList.push_back(new NodeGetAddress(vInfo, vInfo->pos, vInfo->varType));
			if(vInfo->autoDeref)
				AddGetVariableNode(pos);
			if(currDefinedFunc.size() && vInfo->blockDepth <= currDefinedFunc.back()->vTopSize)
				currDefinedFunc.back()->pure = false;	// Access of external variables invalidates function purity
		}
	}
}

TypeInfo* GetCurrentArgumentType(const char *pos, unsigned arguments)
{
	if(!currFunction)
		ThrowError(pos, "ERROR: cannot infer type for inline function outside of the function call");

	HashMap<FunctionInfo*>::Node *currF = NULL;
	if(newType)
	{
		unsigned betterHash = newType->nameHash;
		betterHash = StringHashContinue(betterHash, "::");
		betterHash = StringHashContinue(betterHash, currFunction);
		currF = funcMap.first(betterHash);
	}
	if(!currF)
		currF = funcMap.first(GetStringHash(currFunction));
	TypeInfo **typeInstance = NULL;
	const char *tmpPos = NULL;
	
	if(!currF && (tmpPos = strchr(currFunction, '<')) != NULL && tmpPos < strchr(currFunction, ':'))
	{
		// find class to enable aliases
		typeInstance = CodeInfo::classMap.find(GetStringHash(currFunction, strchr(currFunction, ':')));
		unsigned betterHash = GetStringHash(currFunction, strchr(currFunction, '<'));
		betterHash = StringHashContinue(betterHash, strchr(currFunction, ':'));
		currF = funcMap.first(betterHash);
	}
	AliasInfo *info = typeInstance ? (*typeInstance)->childAlias : NULL;
	while(info)
	{
		CodeInfo::classMap.insert(info->nameHash, info->type);
		info = info->next;
	}
	TypeInfo *preferredType = NULL;
	while(currF)
	{
		FunctionInfo *func = currF->value;
		if(func->visible && !((func->address & 0x80000000) && (func->address != -1)) && func->funcType && func->paramCount > currArgument)
		{
			unsigned tmpCount = func->funcType->funcType->paramCount;
			func->funcType->funcType->paramCount = currArgument;
			unsigned rating = GetFunctionRating(func->funcType->funcType, currArgument);
			func->funcType->funcType->paramCount = tmpCount;
			if(rating == ~0u)
			{
				currF = funcMap.next(currF);
				continue;
			}
			TypeInfo *argType = NULL;
			if(func->generic)
			{
				// Having only a partial set of arguments, begin parsing arguments to the point that the current argument will be known
				Lexeme *start = CodeInfo::lexStart + func->generic->start;

				// Save current type
				TypeInfo *lastType = currType;

				unsigned nodeOffset = CodeInfo::nodeList.size() - currArgument;
				// Move through all the arguments
				VariableInfo *tempList = NULL;
				for(unsigned argID = 0; argID <= currArgument; argID++)
				{
					if(argID)
					{
						assert(start->type == lex_comma);
						start++;
					}

					bool genericArg = false, genericRef = false;

					if(!ParseSelectType(&start))
						genericArg = start->type == lex_generic ? !!(start++) : false;
					genericRef = start->type == lex_ref ? !!(start++) : false;

					if(genericArg && genericRef && start->type == lex_oparen)
					{
						start--;
						currType = NULL;
						ParseTypePostExpressions(&start, false, false, true, true);
						genericArg = genericRef = false;
					}

					if(argID != currArgument)
					{
						if(genericArg)
							SelectTypeForGeneric(nodeOffset + argID);
						if(genericRef && !currType->refLevel)
							currType = CodeInfo::GetReferenceType(currType);

						assert(start->type == lex_string);
						// Insert variable to a list so that a typeof can be taken from it
						InplaceStr paramName = InplaceStr(start->pos, start->length);
						VariableInfo *n = new VariableInfo(NULL, paramName, GetStringHash(paramName.begin, paramName.end), 0, currType, false);
						varMap.insert(n->nameHash, n);
						n->next = tempList;
						tempList = n;
						start++;
						
						assert(start->type != lex_set);
					}else{
						if(genericArg)
							currType = typeGeneric;
					}
				}
				while(tempList)
				{
					varMap.remove(tempList->nameHash, tempList);
					tempList = tempList->next;
				}
				argType = currType;
				currType = lastType;
			}else{
				argType = func->funcType->funcType->paramType[currArgument];
			}
			if(argType->funcType && argType->funcType->paramCount == arguments)
			{
				if(preferredType && argType != preferredType)
					ThrowError(pos, "ERROR: there are multiple function '%s' overloads expecting different function types as an argument #%d", currFunction, currArgument);
				preferredType = argType;
			}
		}
		currF = funcMap.next(currF);
	}
	if(!preferredType)
	{
		HashMap<VariableInfo*>::Node *currV = varMap.first(GetStringHash(currFunction));
		while(currV)
		{
			VariableInfo *var = currV->value;
			if(!var->varType->funcType || var->varType->funcType->paramCount <= currArgument)
				break;	// If the first found variable doesn't match, we can't move to the next, because it's hidden by this one
			// Temporarily change function pointer argument count to match current argument count
			unsigned tmpCount = var->varType->funcType->paramCount;
			var->varType->funcType->paramCount = currArgument;
			unsigned rating = GetFunctionRating(var->varType->funcType, currArgument);
			var->varType->funcType->paramCount = tmpCount;
			if(rating == ~0u)
				break;	// If the first found variable doesn't match, we can't move to the next, because it's hidden by this one
			TypeInfo *argType = var->varType->funcType->paramType[currArgument];
			if(argType->funcType && argType->funcType->paramCount == arguments)
				preferredType = argType;
			break; // If the first found variable matches, we can't move to the next, because it's hidden by this one
		}
	}
	if(!preferredType)
		ThrowError(pos, "ERROR: cannot find function or variable '%s' which accepts a function with %d argument(s) as an argument #%d", currFunction, arguments, currArgument);

	info = typeInstance ? (*typeInstance)->childAlias : NULL;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}
	return preferredType;
}

void InlineFunctionImplicitReturn(const char* pos)
{
	(void)pos;
	if(currDefinedFunc.back()->explicitlyReturned)
		return;
	if(currDefinedFunc.back()->retType == typeVoid)
		return;
	if(CodeInfo::nodeList.back()->nodeType != typeNodeExpressionList)
		return;
	NodeZeroOP *curr = ((NodeExpressionList*)CodeInfo::nodeList.back())->GetFirstNode();
	if(curr->next)
	{
		while(curr->next)
			curr = curr->next;
		if(curr->nodeType != typeNodePopOp)
			return;

		NodeZeroOP *node = ((NodePopOp*)curr)->GetFirstNode();
		CodeInfo::nodeList.push_back(node);
		curr->prev->next = new NodeReturnOp(true, currDefinedFunc.back()->retType, currDefinedFunc.back(), false);
		if(!currDefinedFunc.back()->retType)
		{
			currDefinedFunc.back()->retType = node->typeInfo;
			currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back()->retType, currDefinedFunc.back()->firstParam, currDefinedFunc.back()->paramCount);
		}
	}else{
		if(curr->nodeType != typeNodePopOp)
			return;
		NodeZeroOP *node = ((NodePopOp*)curr)->GetFirstNode();
		CodeInfo::nodeList.back() = node;
		CodeInfo::nodeList.push_back(new NodeReturnOp(true, currDefinedFunc.back()->retType, currDefinedFunc.back(), false));
		if(!currDefinedFunc.back()->retType)
		{
			currDefinedFunc.back()->retType = node->typeInfo;
			currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back()->retType, currDefinedFunc.back()->firstParam, currDefinedFunc.back()->paramCount);
		}
		AddOneExpressionNode(currDefinedFunc.back()->retType);
	}
	currDefinedFunc.back()->explicitlyReturned = true;
}

// Function for array indexing
void AddArrayIndexNode(const char* pos, unsigned argumentCount)
{
	CodeInfo::lastKnownStartPos = pos;

	if(CodeInfo::nodeList[CodeInfo::nodeList.size() - argumentCount - 1]->typeInfo->refLevel == 2)
	{
		NodeZeroOP *array = CodeInfo::nodeList[CodeInfo::nodeList.size() - argumentCount - 1];
		CodeInfo::nodeList.push_back(array);
		CodeInfo::nodeList.push_back(new NodeDereference());
		array = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList[CodeInfo::nodeList.size() - argumentCount - 1] = array;
	}
	// Call overloaded operator with error suppression
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "[]", argumentCount + 1, argumentCount == 1)) // silent error only if there's one argument
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
void AddDefineVariableNode(const char* pos, VariableInfo* varInfo, bool noOverload)
{
	CodeInfo::lastKnownStartPos = pos;
	VariableInfo *variableInfo = (VariableInfo*)varInfo;

	// If a function is being assigned to variable, then take it's address
	ConvertFunctionToPointer(pos, variableInfo->varType);

	// If current type is set to NULL, it means that current type is auto
	// Is such case, type is retrieved from last AST node
	TypeInfo *realCurrType = variableInfo->varType ? variableInfo->varType : CodeInfo::nodeList.back()->typeInfo;

	// If variable type is array without explicit size, and it is being defined with value of different type
	ConvertArrayToUnsized(pos, realCurrType);
	HandlePointerToObject(pos, realCurrType);

	// If type wasn't known until assignment, it means that variable alignment wasn't performed in AddVariable function
	if(!variableInfo->varType)
	{
		if(realCurrType == typeVoid)
			ThrowError(pos, "ERROR: r-value type is 'void'");

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

		if(variableInfo->varType->hasFinalizer)
			ThrowError(pos, "ERROR: cannot create '%s' that implements 'finalize' on stack", variableInfo->varType->GetFullTypeName());
	}

	if(currDefinedFunc.size() && currDefinedFunc.back()->type == FunctionInfo::COROUTINE && variableInfo->blockDepth > currDefinedFunc[0]->vTopSize && variableInfo->pos > currDefinedFunc.back()->allParamSize)
	{
		// If that variable is not in current scope, we have to get it through current closure
		FunctionInfo *currFunc = currDefinedFunc.back();
		// Add variable to the list of function external variables
		FunctionInfo::ExternalInfo *external = AddFunctionExternal(currFunc, variableInfo);

		assert(currFunc->allParamSize % 4 == 0);
		CodeInfo::nodeList.push_back(new NodeGetUpvalue(currFunc, currFunc->allParamSize, external->closurePos, CodeInfo::GetReferenceType(variableInfo->varType)));
	}else{
		CodeInfo::nodeList.push_back(new NodeGetAddress(variableInfo, variableInfo->pos, variableInfo->varType));
	}

	varDefined = false;

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

	// Take l-value
	NodeZeroOP *left = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
	// If we have a function call as an l-value...
	if(left->nodeType == typeNodeFuncCall && ((NodeFuncCall*)left)->funcInfo)
	{
		// ...and that was an accessor "get" function call, we have to replace "get" function with "set" function
		FunctionInfo *fInfo = ((NodeFuncCall*)left)->funcInfo;
		if(fInfo->name[fInfo->nameLength-1] == '$')
		{
			// Replace function call node with 'this' pointer that function node had
			CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = ((NodeFuncCall*)left)->GetFirstNode();
			// If this is an accessor of a generic type, maybe we have to instance a "set" accessor function
			if(fInfo->parentClass && fInfo->parentClass->genericBase)
			{
				TypeInfo *currentType = fInfo->parentClass;
				// Get hash of the base class accessor name
				unsigned accessorBaseHash = currentType->genericBase->nameHash;
				accessorBaseHash = StringHashContinue(accessorBaseHash, "::");
				const char *skipClassName = strchr(fInfo->name, ':');
				assert(skipClassName);
				accessorBaseHash = StringHashContinue(accessorBaseHash, skipClassName + 2);
				// Clear selected function list
				bestFuncList.clear();
				SelectFunctionsForHash(accessorBaseHash, 0); // Select accessor functions from generic type base class
				SelectFunctionsForHash(fInfo->nameHash, 0); // Select accessor functions from instanced class
				// Select best function for the call
				unsigned minRating = ~0u;
				unsigned minRatingIndex = SelectBestFunction(pos, bestFuncList.size(), 1, minRating);
				if(minRating != ~0u)
				{
					// If a function is found and it is a generic function, instance it
					FunctionInfo *fInfo = bestFuncList[minRatingIndex];
					if(fInfo && fInfo->generic)
					{
						CreateGenericFunctionInstance(pos, fInfo, fInfo, currentType);
						assert(fInfo->parentClass == currentType);
						fInfo->parentClass = currentType;
					}
				}
			}
			// Try to call "set" accessor
			if(AddFunctionCallNode(pos, fInfo->name, 1, true))
				return;	// If a call is successful, we are done
			else
				CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = left; // If failed, this is a read-only accessor so return l-value to original state
		}
	}

	// Call overloaded operator with error suppression
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "=", 2, true))
		return;
	// Handle "auto ref" = "non-reference type" conversion
	if(left->typeInfo == typeObject && CodeInfo::nodeList.back()->typeInfo->refLevel == 0)
	{
		NodeZeroOP *right = CodeInfo::nodeList.back(); // take r-value
		CodeInfo::nodeList.pop_back(); // temporarily remove it
		CodeInfo::nodeList.push_back(new NodeConvertPtr(CodeInfo::GetReferenceType(right->typeInfo))); // convert "auto ref" to a reference of r-value type
		CodeInfo::nodeList.push_back(right); // restore r-value, and now we have a natural "type ref" = "type" assignment 
	}
	CheckForImmutable(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, pos);

	// Make necessary implicit conversions
	ConvertArrayToUnsized(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);
	ConvertFunctionToPointer(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);
	HandlePointerToObject(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);

	CodeInfo::nodeList.push_back(new NodeVariableSet(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, 0, true));
}

void AddGetVariableNode(const char* pos, bool forceError)
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
		CodeInfo::nodeList.push_back(new NodeDereference());
	}
	if(forceError && !lastType->refLevel)
		ThrowError(pos, "ERROR: cannot dereference type '%s' that is not a pointer", lastType->GetFullTypeName());
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

			// If we have a generic type instance
			if(currentType->genericBase)
			{
				// Get hash of an accessor function in a generic type base class
				unsigned accessorBaseHash = currentType->genericBase->nameHash;
				accessorBaseHash = StringHashContinue(accessorBaseHash, "::");
				accessorBaseHash = StringHashContinue(accessorBaseHash, memberGet);

				// Get hashes of a regular functions in generic type base and instance class
				memberGet[memberNameLength] = 0;
				unsigned funcBaseHash = currentType->genericBase->nameHash;
				funcBaseHash = StringHashContinue(funcBaseHash, "::");
				funcBaseHash = StringHashContinue(funcBaseHash, memberGet);
				unsigned funcInstancedHash = currentType->nameHash;
				funcInstancedHash = StringHashContinue(funcInstancedHash, "::");
				funcInstancedHash = StringHashContinue(funcInstancedHash, memberGet);

				// Clear function selection
				bestFuncList.clear();
				SelectFunctionsForHash(funcBaseHash, 0); // Select regular functions from generic type base
				SelectFunctionsForHash(funcInstancedHash, 0); // Select regular functions from instance class
				// Choose best function
				unsigned minRating = ~0u;
				unsigned minRatingIndex = SelectBestFunction(pos, bestFuncList.size(), 0, minRating);
				if(minRating != ~0u)
				{
					// If a function is found and it is a generic function, instance it
					FunctionInfo *fInfo = bestFuncList[minRatingIndex];
					if(fInfo && fInfo->generic)
					{
						CreateGenericFunctionInstance(pos, fInfo, memberFunc, currentType);
						assert(memberFunc->parentClass == currentType);
						memberFunc->parentClass = currentType;
					}
				}else{
					// Clear function selection
					bestFuncList.clear();
					SelectFunctionsForHash(accessorBaseHash, 0); // Select accessor functions from generic type base
					SelectFunctionsForHash(GetStringHash(memberFuncName), 0); // Select accessor functions from instance class
					// Choose best accessor
					unsigned minRating = ~0u;
					unsigned minRatingIndex = SelectBestFunction(pos, bestFuncList.size(), 0, minRating);
					if(minRating != ~0u)
					{
						// If a function is found and it is a generic function, instance it
						FunctionInfo *fInfo = bestFuncList[minRatingIndex];
						if(fInfo && fInfo->generic)
						{
							CreateGenericFunctionInstance(pos, fInfo, fInfo, currentType);
							assert(fInfo->parentClass == currentType);
							fInfo->parentClass = currentType;
						}
						// Call accessor
						if(AddFunctionCallNode(pos, memberFuncName, 0, true))
						{
							if(unifyTwo)
								AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
							return; // Exit
						}
					}
					// If an accessor is not found, throw an error
					ThrowError(pos, "ERROR: member variable or function '%.*s' is not defined in class '%s'", varName.end-varName.begin, varName.begin, currentType->GetFullTypeName());
				}
			}else{
				// Call accessor
				if(AddFunctionCallNode(pos, memberFuncName, 0, true))
				{
					if(unifyTwo)
						AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
					return;
				}
				// If an accessor is not found, throw an error
				ThrowError(pos, "ERROR: member variable or function '%.*s' is not defined in class '%s'", varName.end-varName.begin, varName.begin, currentType->GetFullTypeName());
			}
		}else{
			memberFunc = func->value;
			if(memberFunc->generic)
				ThrowError(pos, "ERROR: can't take pointer to a generic function");
		}
		if(func && funcMap.next(func))
		{
			if(unifyTwo)
				AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
			CodeInfo::nodeList.push_back(new NodeFunctionProxy(func->value, pos, false, true));
			return;
		}
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
		if(currentType->arrLevel || currentType == typeObject || currentType == typeAutoArray)
			CodeInfo::nodeList.push_back(new NodeDereference(NULL, 0, true));
	}else{
		// In case of a function, get it's address
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(memberFunc));
	}

	if(unifyTwo)
		AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
}

void UndoDereferceNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;

	if(CodeInfo::nodeList.back()->nodeType == typeNodeDereference)
		((NodeDereference*)CodeInfo::nodeList.back())->Neutralize();
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
	VariableInfo *varInfo = AddVariable(pos, InplaceStr(arrName, length));
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
	// Get r-value type
	TypeInfo *nodeType = CodeInfo::nodeList.back()->typeInfo;
	// If l-value type or r-value type doesn't have subType, make no conversions
	if(!dstType->subType || !nodeType->subType)
		return;
	// If l-value type is not an unsized array type and l-value subType is not an unsized array type, make no conversions
	if(dstType->arrSize != TypeInfo::UNSIZED_ARRAY && dstType->subType->arrSize != TypeInfo::UNSIZED_ARRAY)
		return;
	// If l-value type is equal to r-value type, make no conversions
	if(dstType == nodeType)
		return;
	// If r-value type is not an array and r-value subType is not an array, make no conversions
	if(!nodeType->arrLevel && !nodeType->subType->arrLevel)
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

void ConvertFunctionToPointer(const char* pos, TypeInfo *dstPreferred)
{
	// If node is a function definition or a pair of { function definition, function closure setup }
	if(CodeInfo::nodeList.back()->nodeType == typeNodeFuncDef)
	{
		// Take its address and hide its name
		FunctionInfo *fInfo = ((NodeFuncDef*)CodeInfo::nodeList.back())->GetFuncInfo();
		assert(!fInfo->generic);
		GetFunctionContext(pos, fInfo, true);
		fInfo->pure = false;
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(fInfo));
		AddExtraNode();
		fInfo->visible = false;
	}else if(CodeInfo::nodeList.back()->nodeType == typeNodeExpressionList && ((NodeExpressionList*)CodeInfo::nodeList.back())->GetFirstNode()->nodeType == typeNodeFunctionProxy){ // generic function
		if(!dstPreferred)
			ThrowError(pos, "ERROR: cannot instance generic function, because target type is not known");

		NodeFunctionProxy *fProxy = (NodeFunctionProxy*)((NodeExpressionList*)CodeInfo::nodeList.back())->GetFirstNode();
		assert(fProxy->noError);
		assert(fProxy->funcInfo);
		if(!dstPreferred->funcType)
			ThrowError(pos, "ERROR: cannot instance generic function to a type '%s'", dstPreferred->GetFullTypeName());

		FunctionInfo *fInfo = fProxy->funcInfo, *fTarget = NULL;

		for(unsigned i = 0; i < dstPreferred->funcType->paramCount; i++) // push function argument placeholders
			CodeInfo::nodeList.push_back(new NodeZeroOP(dstPreferred->funcType->paramType[i]));
		NodeZeroOP *funcDefAtEnd = CreateGenericFunctionInstance(pos, fInfo, fTarget);
		for(unsigned i = 0; i < dstPreferred->funcType->paramCount; i++) // remove function argument placeholders
			CodeInfo::nodeList.pop_back();

		// generated generic function instance can be different from the type we wanted to get
		if(dstPreferred != fTarget->funcType)
			ThrowError(pos, "ERROR: cannot convert from '%s' to '%s'", fTarget->funcType->GetFullTypeName(), dstPreferred->GetFullTypeName());

		assert(funcDefAtEnd); // This node will be missing if we had a generic function prototype which doesn't happen

		CodeInfo::nodeList.push_back(funcDefAtEnd);
		AddExtraNode();

		// Take an address of a generic function instance
		GetFunctionContext(pos, fTarget, true);
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(fTarget));
		AddExtraNode();
	}else if(CodeInfo::nodeList.back()->nodeType == typeNodeFunctionProxy){ // If it is an unresolved function overload selection
		FunctionInfo *info = ((NodeFunctionProxy*)CodeInfo::nodeList.back())->funcInfo;
		NodeZeroOP *thisNode = ((NodeFunctionProxy*)CodeInfo::nodeList.back())->GetFirstNode();
		CodeInfo::nodeList.pop_back();
		if(!dstPreferred)
		{
			HashMap<FunctionInfo*>::Node *func = funcMap.first(info->nameHash), *funcS = func;
			bestFuncList.clear();
			bestFuncRating.clear();
			do
			{
				bestFuncList.push_back(func->value);
				bestFuncRating.push_back(0);
				func = funcMap.next(func);
			}while(func);

			char	*errPos = errorReport;
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: ambiguity, there is more than one overloaded function available:\r\n");
			ThrowFunctionSelectError(pos, 0, errorReport, errPos, funcS->value->name, funcS->value->paramCount, bestFuncList.size());
		}
		if(!dstPreferred->funcType)
			ThrowError(pos, "ERROR: cannot select function overload for a type '%s'", dstPreferred->GetFullTypeName());
		AddGetAddressNode(pos, InplaceStr(info->name), dstPreferred, thisNode);
		if(CodeInfo::nodeList.back()->nodeType == typeNodeFunctionProxy)
			ThrowError(pos, "ERROR: unable to select function overload for a type '%s'", dstPreferred->GetFullTypeName());
	}
}

void HandlePointerToObject(const char* pos, TypeInfo *dstType)
{
	TypeInfo *srcType = CodeInfo::nodeList.back()->typeInfo;
	if(typeVoid->refType && srcType == typeVoid->refType)
	{
		// nullptr to type ref conversion
		if(dstType->refLevel)
		{
			CodeInfo::nodeList.back()->typeInfo = dstType;
			return;
		}
		// nullptr to type[] conversion
		if(dstType->arrLevel && dstType->arrSize == TypeInfo::UNSIZED_ARRAY)
		{
			NodeZeroOP *tmp = CodeInfo::nodeList.back();
			CodeInfo::nodeList.back() = new NodeNumber(0, typeInt);
			CodeInfo::nodeList.push_back(tmp);
			AddTwoExpressionNode(dstType);
			return;
		}
		// nullptr to function type conversion
		if(dstType->funcType)
		{
			CodeInfo::nodeList.pop_back();
			CodeInfo::nodeList.push_back(new NodeFunctionAddress(CodeInfo::funcInfo[0]));
			CodeInfo::nodeList.back()->typeInfo = dstType;
			return;
		}
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
void AddOneExpressionNode(TypeInfo *retType)
{
	CodeInfo::nodeList.push_back(new NodeExpressionList(retType ? (TypeInfo*)retType : typeVoid));
}

void AddTwoExpressionNode(TypeInfo *retType)
{
	if(CodeInfo::nodeList.back()->nodeType != typeNodeExpressionList)
		AddOneExpressionNode(retType);
	// Take the expression list from the top
	NodeZeroOP* temp = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	if(CodeInfo::nodeList.back()->nodeType == typeNodeZeroOp)
		CodeInfo::nodeList.pop_back();
	else
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

void AddArrayIterator(const char* pos, InplaceStr varName, TypeInfo* type, bool extra)
{
	CodeInfo::lastKnownStartPos = pos;

	// Save current node type, because some information from it is required later
	TypeInfo *startType = CodeInfo::nodeList.back()->typeInfo;

	bool extraExpression = false;
	// If value dereference was already called, but we need to get a pointer to array
	if(CodeInfo::nodeList.back()->nodeType == typeNodeDereference)
	{
		((NodeDereference*)CodeInfo::nodeList.back())->Neutralize();
	}else if(CodeInfo::nodeList.back()->nodeType != typeNodeFunctionAddress){
		// Or if it wasn't called, for example, inplace array definition/function call/etc, we make a temporary variable
		AddInplaceVariable(pos);
		// And flag that we have added an extra node
		extraExpression = true;
	}

	// Special implementation of for each for build-in arrays
	if(startType->arrLevel)
	{
		assert(startType->subType->size != ~0u);

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
	}else if(CodeInfo::nodeList.back()->nodeType == typeNodeFunctionAddress || (CodeInfo::nodeList.back()->typeInfo->subType && CodeInfo::nodeList.back()->typeInfo->subType->funcType)){
		// This is the case of an iteration of a values generated by a coroutine
		// It can be for(* in function_expression) or for(* in variable_expression)

		// Find out element type
		TypeInfo *elemType = CodeInfo::nodeList.back()->nodeType == typeNodeFunctionAddress ? CodeInfo::nodeList.back()->typeInfo->funcType->retType : CodeInfo::nodeList.back()->typeInfo->subType->funcType->retType;
		// Node after 'in' calculates function pointer and we have to use it multiple times
		NodeZeroOP *getIterator = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();

		// If function doesn't have a context, it's not a coroutine
		if(getIterator->nodeType == typeNodeFunctionAddress && !((NodeOneOP*)getIterator)->GetFirstNode())
			ThrowError(pos, "ERROR: function is not a coroutine");

		// Initialization part "element_type varName;"
		currType = elemType;
		AddVariable(pos, varName);
		if(getIterator->nodeType == typeNodeFunctionAddress)
		{
			AddVoidNode();
		}else{
			// If we receive function pointer from a variable, we can only check for coroutine in run-time
			NodeOneOP *wrap = new NodeOneOP();
			wrap->SetFirstNode(getIterator);
			CodeInfo::nodeList.push_back(wrap);
			AddFunctionCallNode(pos, "__assertCoroutine", 1);
			AddPopNode(pos);
		}
		if(extraExpression)
			AddTwoExpressionNode(NULL);
		if(extra)
			AddTwoExpressionNode(NULL);

		// Condition part "varName = func()
		AddGetAddressNode(pos, varName);
		NodeOneOP *wrap1 = new NodeOneOP();
		wrap1->SetFirstNode(getIterator);
		CodeInfo::nodeList.push_back(wrap1);
		AddFunctionCallNode(pos, NULL, 0);
		AddSetVariableNode(pos);
		AddPopNode(pos);

		// Condition part "!isCoroutineReset(func)
		NodeOneOP *wrap2 = new NodeOneOP();
		wrap2->SetFirstNode(getIterator->nodeType == typeNodeFunctionAddress ? ((NodeOneOP*)getIterator)->GetFirstNode() : getIterator);
		CodeInfo::nodeList.push_back(wrap2);
		if(getIterator->nodeType == typeNodeFunctionAddress)
		{
			// First closure element must be the coroutine state variable "$jmpOffset_ext"
			TypeInfo *currentType = CodeInfo::nodeList.back()->typeInfo;
			assert(currentType->subType);
			TypeInfo::MemberVariable *curr = currentType->subType->firstVariable;
			if(curr->nameHash != GetStringHash("$jmpOffset_ext"))
				ThrowError(pos, "ERROR: function is not a coroutine");
			CodeInfo::nodeList.push_back(new NodeShiftAddress(curr));
		}else{
			// If we got the function pointer from a variable, we can't access member variable "$jmpOffset_ext" directly, so we cheat
			wrap2->typeInfo = CodeInfo::GetReferenceType(CodeInfo::GetReferenceType(CodeInfo::GetReferenceType(typeInt)));
			CodeInfo::nodeList.push_back(new NodeDereference());
		}
		CodeInfo::nodeList.push_back(new NodeDereference());
		CodeInfo::nodeList.push_back(new NodeDereference());

		AddTwoExpressionNode(typeInt);

		// Iteration part is empty
		AddVoidNode();

		return;
	}

	// Initialization part "iterator_type $temp = arr.start();"
	PrepareMemberCall(pos);
	AddMemberFunctionCall(pos, "start", 0);

	AddInplaceVariable(pos);
	// If start returned a function, then go to handling in the 'else if' above (iteration over a coroutine)
	if(CodeInfo::nodeList.back()->typeInfo->subType && CodeInfo::nodeList.back()->typeInfo->subType->funcType)
	{
		AddGetVariableNode(pos);
		AddArrayIterator(pos, varName, type, true);
		return;
	}
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

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}

void AddTypeAllocation(const char* pos, bool arrayType)
{
	if(currType == typeVoid)
		ThrowError(pos, "ERROR: cannot allocate space for void type");
	CodeInfo::nodeList.push_back(new NodeZeroOP(typeInt));
	CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdPushTypeID, (currType->arrLevel ? currType->subType : currType)->typeIndex));
	if(!arrayType)
	{
		AddFunctionCallNode(pos, "__newS", 2);
		CodeInfo::nodeList.back()->typeInfo = CodeInfo::GetReferenceType(currType);
	}else{
		assert(currType->arrSize == TypeInfo::UNSIZED_ARRAY);
		AddFunctionCallNode(pos, "__newA", 3);
		CodeInfo::nodeList.back()->typeInfo = currType;
	}
}

void AddArrayConstructorCall(const char* pos)
{
	IncreaseCycleDepth();
	BeginBlock();

	TypeInfo *type = CodeInfo::nodeList.back()->typeInfo;
	assert(type->refLevel && type->subType->arrLevel);
	type = type->subType->subType;
	assert(!type->refLevel && !type->arrLevel && !type->funcType);

	char *arrName = AllocateString(16);
	int length = sprintf(arrName, "$temp%d", inplaceVariableNum++);
	InplaceStr vName = InplaceStr(arrName, length);

	AddGetVariableNode(pos);
	AddArrayIterator(pos, vName, NULL);

	AddGetAddressNode(pos, vName);
	AddMemberFunctionCall(pos, type->genericBase ? type->genericBase->name : type->name, 0);
	AddPopNode(pos);

	EndBlock();
	AddForEachNode(pos);
	
	NodeZeroOP *last = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	NodeOneOP *wrap = new NodeOneOP();
	wrap->SetFirstNode(last);
	CodeInfo::nodeList.push_back(wrap);
}

void PrepareConstructorCall(const char* pos)
{
	AddInplaceVariable(pos);
	// This node will return pointer
	NodeZeroOP *getPointer = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	// Place pointer on stack twice
	NodeOneOP *wrap = new NodeOneOP();
	wrap->SetFirstNode(getPointer);
	CodeInfo::nodeList.push_back(wrap);
	AddGetVariableNode(pos);

	wrap = new NodeOneOP();
	wrap->SetFirstNode(getPointer);
	CodeInfo::nodeList.push_back(wrap);

	PrepareMemberCall(pos);
}

void FinishConstructorCall(const char* pos)
{
	// Constructor return type must be void
	if(CodeInfo::nodeList.back()->typeInfo != typeVoid)
		ThrowError(pos, "ERROR: constructor cannot be used after 'new' expression if return type is not void");
	
	// Wrap allocation, and variable pointer copy node and constructor call node into one
	TypeInfo *resultType = CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo;
	AddTwoExpressionNode(resultType);
	AddTwoExpressionNode(resultType);
}

bool HasConstructor(const char* pos, TypeInfo* type, unsigned arguments)
{
	bestFuncList.clear();

	if(type->refLevel || type->arrLevel || type->funcType)
		return false;

	unsigned funcHash = type->nameHash;
	funcHash = StringHashContinue(funcHash, "::");
	funcHash = StringHashContinue(funcHash, type->genericBase ? type->genericBase->name : type->name);
	SelectFunctionsForHash(funcHash, 0);

	// For a generic type instance, check if base class has a constructor function
	if(type->genericBase)
	{
		unsigned funcBaseHash = type->genericBase->nameHash;
		funcBaseHash = StringHashContinue(funcBaseHash, "::");
		funcBaseHash = StringHashContinue(funcBaseHash, type->genericBase->name);
		SelectFunctionsForHash(funcBaseHash, 0);
	}

	unsigned minRating = ~0u;
	SelectBestFunction(pos, bestFuncList.size(), arguments, minRating, type->genericBase ? type : NULL);
	return minRating != ~0u;
}

bool defineCoroutine = false;
void BeginCoroutine()
{
	defineCoroutine = true;
}

void FunctionAdd(const char* pos, const char* funcName)
{
	static unsigned int hashFinalizer = GetStringHash("finalize");
	unsigned int funcNameHash = GetStringHash(funcName), origHash = funcNameHash;
	for(unsigned int i = varInfoTop.back().activeVarCnt; i < CodeInfo::varInfo.size(); i++)
	{
		if(CodeInfo::varInfo[i]->nameHash == funcNameHash)
			ThrowError(pos, "ERROR: name '%s' is already taken for a variable in current scope", funcName);
	}
	bool functionLocal = false;
	if(newType ? varInfoTop.size() > (newType->definitionDepth + 1) : varInfoTop.size() > 1)
		functionLocal = true;

	char *funcNameCopy = (char*)funcName;
	if(newType && !functionLocal)
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
	currDefinedFunc.push_back(lastFunc);
	if(newType && !functionLocal)
	{
		if(origHash == hashFinalizer)
			newType->hasFinalizer = true;
		lastFunc->type = FunctionInfo::THISCALL;
		lastFunc->parentClass = newType;
		if(newType->genericInfo)
			FunctionGeneric(true);
		if(newType->genericBase)
			lastFunc->genericBase = newType->genericBase->genericInfo;
	}
	if(functionLocal)
	{
		lastFunc->type = FunctionInfo::LOCAL;
		lastFunc->parentClass = NULL;
	}
	if(defineCoroutine)
	{
		if(newType && lastFunc->type == FunctionInfo::THISCALL)
			ThrowError(pos, "ERROR: coroutine cannot be a member function");
		lastFunc->type = FunctionInfo::COROUTINE;
		lastFunc->parentClass = NULL;
		defineCoroutine = false;
	}
	if(funcName[0] != '$' && !(chartype_table[(unsigned char)funcName[0]] & ct_start_symbol))
		lastFunc->visible = false;
}

bool FunctionGeneric(bool setGeneric, unsigned pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	if(setGeneric)
	{
		lastFunc.generic = currDefinedFunc.back()->CreateGenericContext(pos);
		lastFunc.generic->globalVarTop = varTop;
		lastFunc.generic->blockDepth = lastFunc.vTopSize;
		lastFunc.generic->parent = &lastFunc;
	}
	return !!lastFunc.generic;
}

void FunctionParameter(const char* pos, InplaceStr paramName)
{
	if(currType == typeVoid)
		ThrowError(pos, "ERROR: function parameter cannot be a void type");
	unsigned int hash = GetStringHash(paramName.begin, paramName.end);
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	if(lastFunc.lastParam && !lastFunc.lastParam->varType && !lastFunc.generic)
		ThrowError(pos, "ERROR: function parameter cannot be an auto type");

	for(VariableInfo *info = lastFunc.firstParam; info; info = info->next)
	{
		if(info->nameHash == hash)
			ThrowError(pos, "ERROR: parameter with name '%.*s' is already defined", int(info->name.end - info->name.begin), info->name.begin);
	}

#if defined(__CELLOS_LV2__)
	unsigned bigEndianShift = currType == typeChar ? 3 : (currType == typeShort ? 2 : 0);
#else
	unsigned bigEndianShift = 0;
#endif
	lastFunc.AddParameter(new VariableInfo(&lastFunc, paramName, hash, lastFunc.allParamSize + bigEndianShift, currType, false));
	if(currType)
		lastFunc.allParamSize += currType->size < 4 ? 4 : currType->size;

	// Insert variable to a list so that a typeof can be taken from it
	varMap.insert(lastFunc.lastParam->nameHash, lastFunc.lastParam);
}

void FunctionPrepareDefault()
{
	// Remove function variables before parsing the default parameter so that arguments wouldn't be referenced
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	for(VariableInfo *curr = lastFunc.firstParam; curr; curr = curr->next)
		varMap.remove(curr->nameHash, curr);
}

void FunctionParameterDefault(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	TypeInfo *left = lastFunc.lastParam->varType;

	ConvertFunctionToPointer(pos, left);
	if(left)
		HandlePointerToObject(pos, left);

	TypeInfo *right = CodeInfo::nodeList.back()->typeInfo;

	if(!lastFunc.lastParam->varType)
	{
		left = lastFunc.lastParam->varType = right;
		lastFunc.allParamSize += right->size < 4 ? 4 : right->size;
	}
	if(left == typeVoid)
		ThrowError(pos, "ERROR: function parameter cannot be a void type", right->GetFullTypeName(), left->GetFullTypeName());
	
	// If types don't match and it it is not build-in basic types or if pointers point to different types
	if(right == typeVoid || (left != right && (left->type == TypeInfo::TYPE_COMPLEX || right->type == TypeInfo::TYPE_COMPLEX || left->subType != right->subType)) &&
		!(left->arrLevel == right->arrLevel && left->arrSize == TypeInfo::UNSIZED_ARRAY && right->arrSize != TypeInfo::UNSIZED_ARRAY))
		ThrowError(pos, "ERROR: cannot convert from '%s' to '%s'", right->GetFullTypeName(), left->GetFullTypeName());

	lastFunc.lastParam->defaultValue = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	// Restore function arguments so that typeof could be taken
	for(VariableInfo *curr = lastFunc.firstParam; curr; curr = curr->next)
		varMap.insert(curr->nameHash, curr);
}

void FunctionPrototype(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	// Remove function arguments used to enable typeof
	for(VariableInfo *curr = lastFunc.firstParam; curr; curr = curr->next)
	{
		if(lastFunc.generic && curr->defaultValue)
			ThrowError(pos, "ERROR: default argument values are unsupported in generic functions");
		varMap.remove(curr->nameHash, curr);
	}
	if(!lastFunc.retType && !lastFunc.generic)
		ThrowError(pos, "ERROR: function prototype with unresolved return type");
	lastFunc.funcType = CodeInfo::GetFunctionType(lastFunc.retType, lastFunc.firstParam, lastFunc.paramCount);
	currDefinedFunc.pop_back();
	if(newType && lastFunc.type == FunctionInfo::THISCALL)
		methodCount++;
	if(lastFunc.generic)
	{
		CodeInfo::nodeList.push_back(new NodeFunctionProxy(&lastFunc, pos, true));
		AddOneExpressionNode(typeVoid);
		lastFunc.afterNode = (NodeExpressionList*)CodeInfo::nodeList.back();
	}else if(lastFunc.type == FunctionInfo::LOCAL || lastFunc.type == FunctionInfo::COROUTINE){
		// For a local or a coroutine function prototype, we must create a forward declaration of a context variable
		// When a function will be implemented, it will fill up the closure type with members, update this context variable and create closure initialization
		char *hiddenHame = AllocateString(lastFunc.nameLength + 24);
		int length = sprintf(hiddenHame, "$%s_%d_ext", lastFunc.name, lastFunc.indexInArr);
		unsigned beginPos = lastFunc.parentFunc ? lastFunc.parentFunc->allParamSize + NULLC_PTR_SIZE : 0; // so that a coroutine will not mistake this for argument, choose starting position carefully
		lastFunc.funcContext = new VariableInfo(lastFunc.parentFunc, InplaceStr(hiddenHame, length), GetStringHash(hiddenHame), beginPos, CodeInfo::GetReferenceType(typeInt), !lastFunc.parentFunc);
		lastFunc.funcContext->blockDepth = varInfoTop.size();
		// Allocate node where the context initialization will be placed
		CodeInfo::nodeList.push_back(new NodeZeroOP());
		AddOneExpressionNode(typeVoid);
		lastFunc.afterNode = (NodeExpressionList*)CodeInfo::nodeList.back();
		// Context variable should be visible
		varMap.insert(lastFunc.funcContext->nameHash, lastFunc.funcContext);
	}else{
		AddVoidNode();
	}
	// Check if this function was already defined
	HashMap<FunctionInfo*>::Node *curr = funcMap.first(lastFunc.nameHash);
	while(curr)
	{
		FunctionInfo *func = curr->value;
		if(func->visible && func->funcType == lastFunc.funcType && func != &lastFunc)
			ThrowError(pos, "ERROR: function is already defined");
		curr = funcMap.next(curr);
	}
	// Remove aliases defined in a prototype argument list
	AliasInfo *info = lastFunc.childAlias;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}
}

void FunctionStart(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();
	// Remove function arguments used to enable typeof
	for(VariableInfo *curr = lastFunc.firstParam; curr; curr = curr->next)
		varMap.remove(curr->nameHash, curr);
	if(lastFunc.lastParam && !lastFunc.lastParam->varType)
		ThrowError(pos, "ERROR: function parameter cannot be an auto type");

	lastFunc.implemented = true;
	// Resolve function type if the return type is known
	lastFunc.funcType = lastFunc.retType ? CodeInfo::GetFunctionType(lastFunc.retType, lastFunc.firstParam, lastFunc.paramCount) : NULL;
	if(lastFunc.type == FunctionInfo::NORMAL)
		lastFunc.pure = true;	// normal function is pure by default
	if(lastFunc.parentFunc)
		lastFunc.parentFunc->pure = false;	// local function invalidates parent function purity

	BeginBlock();
	cycleDepth.push_back(0);

	varTop = 0;

	for(VariableInfo *curr = lastFunc.firstParam; curr; curr = curr->next)
	{
		CheckCollisionWithFunction(pos, curr->name, curr->nameHash, varInfoTop.size());
		varTop += GetAlignmentOffset(pos, 4);
		CodeInfo::varInfo.push_back(curr);
		CodeInfo::varInfo.back()->blockDepth = varInfoTop.size();
		varMap.insert(curr->nameHash, curr);
		varTop += curr->varType->size ? curr->varType->size : 4;	// 0-byte size arguments use up 4 bytes
		if(curr->varType->type == TypeInfo::TYPE_COMPLEX || curr->varType->refLevel)
			lastFunc.pure = false;	// Pure functions can only accept simple types
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

	if(lastFunc.type == FunctionInfo::COROUTINE)
	{
		currType = typeInt;
		VariableInfo *jumpOffset = (VariableInfo*)AddVariable(pos, InplaceStr("$jmpOffset", 10));
		AddFunctionExternal(&lastFunc, jumpOffset);
	}
	varDefined = false;
}

void FunctionEnd(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	if(lastFunc.retType && lastFunc.retType != typeVoid && !lastFunc.explicitlyReturned)
		ThrowError(pos, "ERROR: function must return a value of type '%s'", lastFunc.retType->GetFullTypeName());

	FunctionInfo *implementedPrototype = NULL;
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
				implementedPrototype = info;
			}
		}
		curr = funcMap.next(curr);
	}
	if(implementedPrototype && implementedPrototype->parentFunc != lastFunc.parentFunc)
		ThrowError(pos, "ERROR: function implementation is found in scope different from function prototype");

	assert(cycleDepth.back() == 0);
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
		if(lastFunc.firstLocal->varType->hasPointers)
			lastFunc.pure = false;	// Pure functions locals must be simple types
	}
	if(lastFunc.firstLocal)
		lastFunc.firstLocal->prev = NULL;

	if(!currDefinedFunc.back()->retType)
	{
		currDefinedFunc.back()->retType = typeVoid;
		currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back()->retType, currDefinedFunc.back()->firstParam, currDefinedFunc.back()->paramCount);
	}
	currDefinedFunc.pop_back();

	// Remove aliases defined in a function
	AliasInfo *info = lastFunc.childAlias;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}

	unsigned int varFormerTop = varTop;
	varTop = varInfoTop[lastFunc.vTopSize].varStackSize;
	EndBlock(true, false);

	NodeZeroOP *funcNode = new NodeFuncDef(&lastFunc, varFormerTop);
	lastFunc.functionNode = (void*)funcNode;
	CodeInfo::nodeList.push_back(funcNode);
	funcNode->SetCodeInfo(pos);
	CodeInfo::funcDefList.push_back(funcNode);

	if(lastFunc.retType->type == TypeInfo::TYPE_COMPLEX || lastFunc.retType->refLevel)
		lastFunc.pure = false;	// Pure functions return value must be simple type

	// If function is local, create function parameters block
	if((lastFunc.type == FunctionInfo::LOCAL || lastFunc.type == FunctionInfo::COROUTINE) && lastFunc.externalCount != 0)
	{
		char *hiddenHame = AllocateString(lastFunc.nameLength + 24);
		int length = sprintf(hiddenHame, "$%s_%d_ext", lastFunc.name, CodeInfo::FindFunctionByPtr(&lastFunc));

		TypeInfo *saveCurrType = currType;
		bool saveVarDefined = varDefined;

		// Save data about currently defined type
		TypeInfo *currentDefinedType = newType;
		unsigned int currentDefinedTypeMethodCount = methodCount;
		// Create closure type
		newType = NULL;
		SetCurrentAlignment(4);
		char tempName[NULLC_MAX_VARIABLE_NAME_LENGTH + 32];
		SafeSprintf(tempName, NULLC_MAX_VARIABLE_NAME_LENGTH + 32, "__%s_%d_cls", lastFunc.name, CodeInfo::FindFunctionByPtr(&lastFunc));
		TypeBegin(tempName, tempName + strlen(tempName));
		for(FunctionInfo::ExternalInfo *curr = lastFunc.firstExternal; curr; curr = curr->next)
		{
			unsigned int bufSize = int(curr->variable->name.end - curr->variable->name.begin) + 5;
			// Pointer to target variable
			char	*memberName = AllocateString(bufSize);
			SafeSprintf(memberName, bufSize, "%.*s_ext", int(curr->variable->name.end - curr->variable->name.begin), curr->variable->name.begin);
			newType->AddMemberVariable(memberName, CodeInfo::GetReferenceType(curr->variable->varType));

			// Place for a copy of target variable
			memberName = AllocateString(bufSize);
			SafeSprintf(memberName, bufSize, "%.*s_dat", int(curr->variable->name.end - curr->variable->name.begin), curr->variable->name.begin);
			unsigned int reqBytes = (curr->variable->varType->size < (NULLC_PTR_SIZE + 4) ? (NULLC_PTR_SIZE + 4) : curr->variable->varType->size);
			newType->AddMemberVariable(memberName, CodeInfo::GetArrayType(typeInt, reqBytes / 4));
		}
		TypeInfo *closureType = newType;

		assert(newType->size % 4 == 0); // resulting type should never require padding

		// Shift new types generated inside up, so that declaration will be in the correct order in C translation
		for(unsigned int i = newType->originalIndex + 1; i < CodeInfo::typeInfo.size(); i++)
			CodeInfo::typeInfo[i]->originalIndex--;
		newType->originalIndex = CodeInfo::typeInfo.size() - 1;
		// Restore data about previously defined type
		newType = currentDefinedType;
		methodCount = currentDefinedTypeMethodCount;
		EndBlock(false, false);

		// Create a pointer to array
		currType = CodeInfo::GetReferenceType(closureType);
		lastFunc.extraParam->varType = currType;
		VariableInfo *varInfo = NULL;
		// If we've implemented a prototype of a local/coroutine function, update context variable placement
		if(implementedPrototype && implementedPrototype->funcContext)
		{
			varInfo = implementedPrototype->funcContext;
			varInfo->pos = varTop;
			varInfo->varType = currType;
			CodeInfo::varInfo.push_back(varInfo);
			CodeInfo::varInfo.back()->blockDepth = varInfoTop.size();
			varTop += currType->size;
		}else{
			varInfo = AddVariable(pos, InplaceStr(hiddenHame, length));
		}

		lastFunc.funcContext = varInfo; // Save function context variable

		// Allocate array in dynamic memory
		CodeInfo::nodeList.push_back(new NodeNumber((int)(lastFunc.externalSize), typeInt));
		CodeInfo::nodeList.push_back(new NodeNumber(0, typeInt));
		AddFunctionCallNode(pos, "__newS", 2);
		assert(closureType->size >= lastFunc.externalSize);
		CodeInfo::nodeList.back()->typeInfo = CodeInfo::GetReferenceType(closureType);

		// Set it to pointer variable
		AddDefineVariableNode(pos, varInfo);

		// Previous closure may not exists if it's the end of a global coroutine definition
		CodeInfo::nodeList.push_back(new NodeDereference(&lastFunc, currDefinedFunc.size() ? currDefinedFunc.back()->allParamSize : 0));

		for(FunctionInfo::ExternalInfo *curr = lastFunc.firstExternal; curr; curr = curr->next)
		{
			VariableInfo *currVar = curr->variable;
			// Take special care for local coroutine variables that are accessed like external variables
			if(lastFunc.type == FunctionInfo::COROUTINE && currVar->parentFunction == &lastFunc)
			{
				assert(currVar->parentFunction == &lastFunc);
				// If not found, and this function is not a coroutine, than it's an internal compiler error
				if(lastFunc.type != FunctionInfo::COROUTINE)
					ThrowError(pos, "Can't capture variable %.*s", currVar->name.end - currVar->name.begin, currVar->name.begin);
				// Mark position as invalid so that when closure is created, this variable will be closed immediately
				curr->targetLocal = true;
				curr->targetPos = ~0u;
				curr->targetFunc = 0;
				curr->targetDepth = 0;
				continue;
			}

			assert(currDefinedFunc.size());

			// Function that scopes target variable
			FunctionInfo *parentFunc = currVar->parentFunction;
			
			// If variable is not in current scope (or a local in case of a coroutine), get it through closure
			bool externalAccess = !currVar->isGlobal && currVar->parentFunction != currDefinedFunc.back();

			// all coroutine variables except parameters are accessed through an external closure
			bool coroutineParameter = false;
			if(currDefinedFunc.back()->type == FunctionInfo::COROUTINE && !externalAccess)
			{
				for(VariableInfo *param = currDefinedFunc.back()->firstParam; param && !coroutineParameter; param = param->next)
					coroutineParameter = param == currVar;
			}
			if((currDefinedFunc.back()->type == FunctionInfo::LOCAL && externalAccess) || (currDefinedFunc.back()->type == FunctionInfo::COROUTINE && (!currVar->isGlobal && !coroutineParameter)))
			{
				// Add variable name to the list of function external variables
				FunctionInfo::ExternalInfo *external = AddFunctionExternal(currDefinedFunc.back(), currVar);

				curr->targetLocal = false;
				curr->targetPos = external->closurePos;
			}else{
				// Or get it from current scope
				curr->targetLocal = true;
				curr->targetPos = currVar->pos;
				curr->targetVar = currVar;
			}
			curr->targetFunc = parentFunc->indexInArr;
			assert(curr->targetFunc == CodeInfo::FindFunctionByPtr(parentFunc));
			curr->targetDepth = curr->variable->blockDepth - parentFunc->vTopSize - 1;

			if(curr->variable->blockDepth - parentFunc->vTopSize > parentFunc->maxBlockDepth)
				parentFunc->maxBlockDepth = curr->variable->blockDepth - parentFunc->vTopSize;
			parentFunc->closeUpvals = true;
		}

		varDefined = saveVarDefined;
		currType = saveCurrType;
		funcNode->AddExtraNode();
		// Context creation should be placed at the point of a function prototype if this function is its implementation
		if(implementedPrototype && implementedPrototype->afterNode)
		{
			implementedPrototype->afterNode->AddNode();
			AddVoidNode();
		}
	}
	// If there were no external variables, the context created after function prototype should be updated
	if((lastFunc.type == FunctionInfo::LOCAL || lastFunc.type == FunctionInfo::COROUTINE) && lastFunc.externalCount == 0 && implementedPrototype)
	{
		assert(implementedPrototype->funcContext);
		implementedPrototype->funcContext->pos = varTop;
		CodeInfo::varInfo.push_back(implementedPrototype->funcContext);
		CodeInfo::varInfo.back()->blockDepth = varInfoTop.size();
		varTop += currType->size;
	}

	if(newType && lastFunc.type == FunctionInfo::THISCALL)
		methodCount++;

	if(lastFunc.maxBlockDepth > 255)
		ThrowError(pos, "ERROR: function block depth (%d) is too large to handle", lastFunc.maxBlockDepth);

	currType = lastFunc.retType;
}

void FunctionToOperator(const char* pos)
{
	static unsigned int hashAdd = GetStringHash("+");
	static unsigned int hashSub = GetStringHash("-");
	static unsigned int hashBitNot = GetStringHash("~");
	static unsigned int hashLogNot = GetStringHash("!");
	static unsigned int hashIndex = GetStringHash("[]");
	static unsigned int hashFunc = GetStringHash("()");

	FunctionInfo &lastFunc = *currDefinedFunc.back();
	if(lastFunc.nameHash != hashFunc && lastFunc.nameHash != hashIndex && lastFunc.paramCount != 2 && !(lastFunc.paramCount == 1 && (lastFunc.nameHash == hashAdd || lastFunc.nameHash == hashSub || lastFunc.nameHash == hashBitNot || lastFunc.nameHash == hashLogNot)))
		ThrowError(pos, "ERROR: binary operator definition or overload must accept exactly two arguments");
	lastFunc.visible = true;
}

void ResetConstantFoldError()
{
	uncalledFunc = NULL;
}
void ThrowConstantFoldError(const char *pos)
{
	if(CodeInfo::nodeList.back()->nodeType != typeNodeNumber)
	{
		if(uncalledFunc)
			ThrowError(uncalledPos, "ERROR: array size must be a constant expression. During constant folding, '%s' function couldn't be evaluated", uncalledFunc->name);
		else
			ThrowError(pos, "ERROR: array size must be a constant expression");
	}
}

TypeInfo* GetGenericFunctionRating(const char *pos, FunctionInfo *fInfo, unsigned &newRating, unsigned count)
{
	// There could be function calls in constrain expression, so we backup current function and rating list
	unsigned prevBackupSize = bestFuncListBackup.size();
	bestFuncListBackup.push_back(&bestFuncList[0], count);
	bestFuncRatingBackup.push_back(&bestFuncRating[0], count);

	// Out function
	unsigned argumentCount = fInfo->paramCount;

	Lexeme *start = CodeInfo::lexStart + fInfo->generic->start;

	// Save current type
	TypeInfo *lastType = currType;

	// Add aliases from member function parent type if it has one
	if(fInfo->generic->parent->parentClass)
	{
		AliasInfo *baseClassAlias = fInfo->generic->parent->parentClass->childAlias;
		while(baseClassAlias)
		{
			CodeInfo::classMap.insert(baseClassAlias->nameHash, baseClassAlias->type);
			baseClassAlias = baseClassAlias->next;
		}
	}
	// Reset function alias list
	fInfo->childAlias = NULL;

	// apply resolved argument types and test if it is ok
	unsigned nodeOffset = CodeInfo::nodeList.size() - argumentCount;
	// Move through all the arguments
	VariableInfo *tempListS = NULL, *tempListE = NULL;
	for(unsigned argID = 0; argID < argumentCount; argID++)
	{
		if(argID)
		{
			assert(start->type == lex_comma);
			start++;
		}

		bool genericArg = false, genericRef = false;
		// Try to reparse the type
		while(!ParseSelectType(&start, true, true))
		{
			if(start->type == lex_generic) // If failed because of generic
			{
				genericArg = !!(start++); // move forward and mark argument as a generic
				if(start[0].type == lex_ref && start[1].type == lex_oparen)
				{
					TypeInfo *referenceType = CodeInfo::nodeList[nodeOffset + argID]->typeInfo; // Get type to which we are trying to specialize
					if(CodeInfo::nodeList[nodeOffset + argID]->nodeType == typeNodeFuncDef)
						referenceType = ((NodeFuncDef*)CodeInfo::nodeList[nodeOffset + argID])->GetFuncInfo()->funcType;
					if(!ParseGenericFuntionType(&start, referenceType))
					{
						newRating = ~0u; // function is not instanced
						return NULL;
					}
					genericArg = false;
				}
			}else if(start->type == lex_less){ // If failed because of generic type specialization
				genericArg = false;

				TypeInfo *referenceType = CodeInfo::nodeList[nodeOffset + argID]->typeInfo; // Get type to which we are trying to specialize
				// Strip type from references and arrays
				TypeInfo *refTypeBase = referenceType;
				while(refTypeBase->refLevel || refTypeBase->arrLevel)
					refTypeBase = refTypeBase->subType;
				TypeInfo *expectedType = currType;
				if(refTypeBase->genericBase != expectedType) // If its generic base is not equal to generic type expected at this point
				{
					newRating = ~0u; // function is not instanced
					return NULL;
				}
				// Set generic function as being in definition so that 
				currDefinedFunc.push_back(fInfo);
				if(!ParseGenericType(&start, refTypeBase->childAlias ? refTypeBase : NULL)) // It is possible that generic type argument count is incorrect
				{
					newRating = ~0u; // function is not instanced
					return NULL;
				}
				currDefinedFunc.pop_back();
				// Check that both types follow the same tree
				refTypeBase = referenceType;
				TypeInfo *selectedType = currType;
				// Follow both types along a type tree
				while(refTypeBase->refLevel || refTypeBase->arrLevel)
				{
					if((refTypeBase->refLevel != selectedType->refLevel) || (refTypeBase->arrLevel != selectedType->arrLevel || refTypeBase->arrSize != selectedType->arrSize))
						break; // function type will be instanced to show a better function selection error
					refTypeBase = refTypeBase->subType;
					selectedType = selectedType->subType;
				}
				if(selectedType->genericBase != expectedType && !(selectedType->refLevel && selectedType->subType->genericBase == expectedType))
					break; // function type will be instanced to show a better function selection error
			}else{ // If failed for other reasons, such as typeof that depends on generic
				assert(argID);
				TypeInfo *argType = fInfo->funcType->funcType->paramType[argID - 1];
				genericArg = argType == typeGeneric || argType->dependsOnGeneric; // mark argument as a generic
			}
			break;
		}
		if(start->type == lex_oparen)
		{
			start--;
			TypeInfo *referenceType = CodeInfo::nodeList[nodeOffset + argID]->typeInfo; // Get type to which we are trying to specialize
			if(CodeInfo::nodeList[nodeOffset + argID]->nodeType == typeNodeFuncDef)
				referenceType = ((NodeFuncDef*)CodeInfo::nodeList[nodeOffset + argID])->GetFuncInfo()->funcType;
			if(!ParseGenericFuntionType(&start, referenceType))
			{
				newRating = ~0u; // function is not instanced
				return NULL;
			}
			genericArg = false;
		}
		genericRef = start->type == lex_ref ? !!(start++) : false;

		if(genericArg)
			SelectTypeForGeneric(nodeOffset + argID);
		if(genericRef && !currType->refLevel)
			currType = CodeInfo::GetReferenceType(currType);

		assert(start->type == lex_string);
		// Insert variable to a list so that a typeof can be taken from it
		InplaceStr paramName = InplaceStr(start->pos, start->length);
		assert(currType);
		VariableInfo *n = new VariableInfo(NULL, paramName, GetStringHash(paramName.begin, paramName.end), 0, currType, false);
		varMap.insert(n->nameHash, n);
		if(!tempListS)
		{
			tempListS = tempListE = n;
		}else{
			tempListE->next = n;
			tempListE = n;
		}
		start++;
	}
	assert(start->type == lex_cparen);
	start++;

	// Remove aliases that were created
	AliasInfo *aliasCurr = fInfo->childAlias;
	while(aliasCurr)
	{
		// Find if there are aliases with the same name
		AliasInfo *aliasNext = aliasCurr->next;
		while(aliasNext)
		{
			if(aliasCurr->nameHash == aliasNext->nameHash)
				ThrowError(pos, "ERROR: function '%s' argument list has multiple '%.*s' aliases", fInfo->name, aliasCurr->name.end - aliasCurr->name.begin, aliasCurr->name.begin);
			aliasNext = aliasNext->next;
		}
		CodeInfo::classMap.remove(aliasCurr->nameHash, aliasCurr->type);
		aliasCurr = aliasCurr->next;
	}
	// Remove aliases from member function parent type if it has one
	if(fInfo->generic->parent->parentClass)
	{
		AliasInfo *baseClassAlias = fInfo->generic->parent->parentClass->childAlias;
		while(baseClassAlias)
		{
			CodeInfo::classMap.remove(baseClassAlias->nameHash, baseClassAlias->type);
			baseClassAlias = baseClassAlias->next;
		}
	}

	// We have to create a function type for generated parameters
	TypeInfo *tmpType = CodeInfo::GetFunctionType(NULL, tempListS, argumentCount);
	newRating = GetFunctionRating(tmpType->funcType, argumentCount);

	// Remove function arguments
	while(tempListS)
	{
		varMap.remove(tempListS->nameHash, tempListS);
		tempListS = tempListS->next;
	}
	currType = lastType;

	// Restore old function list, because the one we've started with could've been replaced
	bestFuncList.clear();
	bestFuncRating.clear();

	bestFuncList.push_back(&bestFuncListBackup[prevBackupSize], count);
	bestFuncRating.push_back(&bestFuncRatingBackup[prevBackupSize], count);

	bestFuncListBackup.shrink(prevBackupSize);
	bestFuncRatingBackup.shrink(prevBackupSize);

	return tmpType;
}

unsigned int GetFunctionRating(FunctionType *currFunc, unsigned int callArgCount)
{
	if(currFunc->paramCount != callArgCount)
		return ~0u;	// Definitely, this isn't the function we are trying to call. Parameter count does not match.

	unsigned int fRating = 0;
	for(unsigned int i = 0; i < callArgCount; i++)
	{
		NodeZeroOP* activeNode = CodeInfo::nodeList[CodeInfo::nodeList.size() - callArgCount + i];
		TypeInfo *paramType = activeNode->typeInfo;
		unsigned int	nodeType = activeNode->nodeType;
		TypeInfo *expectedType = currFunc->paramType[i];
		if(expectedType != paramType)
		{
			if(expectedType == typeGeneric)	// generic function argument
				continue;
			else if(expectedType->dependsOnGeneric)	// generic function argument that is derivative from generic
				continue;
			else if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->arrSize != 0 && paramType->subType == expectedType->subType)
				fRating += 2;	// array -> class (unsized array)
			else if(expectedType == typeAutoArray && paramType->arrLevel)
				fRating += 10;	// array -> auto[]
			else if(expectedType->refLevel == 1 && expectedType->refLevel == paramType->refLevel && expectedType->subType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->subType->subType == expectedType->subType->subType)
				fRating += 5;	// array[N] ref -> array[] -> array[] ref
			else if(expectedType->funcType != NULL){
				if(nodeType == typeNodeFuncDef && ((NodeFuncDef*)activeNode)->GetFuncInfo()->funcType == expectedType)
					continue;		// Inline function definition doesn't cost anything
				else if(nodeType == typeNodeFunctionProxy && ((NodeFunctionProxy*)activeNode)->HasType(expectedType))
					continue;		// If a set of function overloads has an expected overload, this doesn't cont anything
				else if(nodeType == typeNodeExpressionList && ((NodeExpressionList*)CodeInfo::nodeList.back())->GetFirstNode()->nodeType == typeNodeFunctionProxy)
					continue;		// Generic function is expected to have an appropriate instance, but there will be a check after instancing
				else
					return ~0u;		// Otherwise this function is not a match
				
			}else if(expectedType->refLevel == paramType->refLevel + 1 && expectedType->subType == paramType)
				fRating += 5;	// type -> type ref
			else if(expectedType == typeObject && paramType->refLevel)
				fRating += 5;	// type ref -> auto ref
			else if(expectedType == typeObject)
				fRating += 10;	// type -> type ref -> auto ref
			else if(expectedType->type == TypeInfo::TYPE_COMPLEX || paramType->type == TypeInfo::TYPE_COMPLEX || paramType->type == TypeInfo::TYPE_VOID)
				return ~0u;		// If one of types is complex, and they aren't equal, function cannot match
			else if(paramType->subType != expectedType->subType)
				return ~0u;		// Pointer or array with a different type inside. Doesn't matter if simple or complex.
			else				// Build-in types can convert to each other, but the fact of conversion tells us, that there could be a better suited function
				fRating += 1;	// type -> type
		}
	}
	return fRating;
}

bool PrepareMemberCall(const char* pos, const char* funcName)
{
	TypeInfo *currentType = CodeInfo::nodeList.back()->typeInfo;

	// Implicit conversion of type ref ref to type ref
	if(currentType->refLevel == 2)
	{
		CodeInfo::nodeList.push_back(new NodeDereference());
		currentType = currentType->subType;
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
		currentType = CodeInfo::nodeList.back()->typeInfo;
	}
	// Check class members, in case we have to get function pointer from class member before function call
	if(funcName)
	{
		// Consider that function name is actually a member name
		unsigned int hash = GetStringHash(funcName);
		TypeInfo::MemberVariable *curr = currentType->subType->firstVariable;
		for(; curr; curr = curr->next)
			if(curr->nameHash == hash)
				break;
		// If a member is found
		if(curr)
		{
			// Get it
			AddMemberAccessNode(pos, InplaceStr(funcName));
			// Return false to signal that this is not a member function call
			return false;
		}
	}
	// Return true to signal that this is a member function call
	return true;
}

void SelectFunctionsForHash(unsigned funcNameHash, unsigned scope)
{
	HashMap<FunctionInfo*>::Node *curr = funcMap.first(funcNameHash);
	while(curr)
	{
		FunctionInfo *func = curr->value;
		if(func->visible && !((func->address & 0x80000000) && (func->address != -1)) && func->funcType && func->vTopSize >= scope)
			bestFuncList.push_back(func);
		curr = funcMap.next(curr);
	}
}

unsigned SelectBestFunction(const char *pos, unsigned count, unsigned callArgCount, unsigned int &minRating, TypeInfo* forcedParentType)
{
	// Find the best suited function
	bestFuncRating.resize(count);

	unsigned int minGenericRating = ~0u;
	unsigned int minRatingIndex = ~0u, minGenericIndex = ~0u;
	for(unsigned int k = 0; k < count; k++)
	{
		unsigned int argumentCount = callArgCount;
		// Act as if default parameter values were passed
		if(argumentCount < bestFuncList[k]->paramCount)
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
		if(argumentCount >= (bestFuncList[k]->paramCount - 1) && bestFuncList[k]->lastParam && bestFuncList[k]->lastParam->varType == typeObjectArray &&
			!(argumentCount == bestFuncList[k]->paramCount && CodeInfo::nodeList.back()->typeInfo == typeObjectArray))
		{
			// If function accepts variable argument list
			TypeInfo *&lastType = bestFuncList[k]->funcType->funcType->paramType[bestFuncList[k]->paramCount - 1];
			assert(lastType == CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY));
			unsigned int redundant = argumentCount - bestFuncList[k]->paramCount;
			if(redundant == ~0u)
				CodeInfo::nodeList.push_back(new NodeZeroOP(typeObjectArray));
			else
				CodeInfo::nodeList.shrink(CodeInfo::nodeList.size() - redundant);
			// Change things in a way that this function will be selected (lie about real argument count and match last argument type)
			lastType = CodeInfo::nodeList.back()->typeInfo;
			bestFuncRating[k] = GetFunctionRating(bestFuncList[k]->funcType->funcType, bestFuncList[k]->paramCount);
			if(redundant == ~0u)
				CodeInfo::nodeList.pop_back();
			else
				CodeInfo::nodeList.resize(CodeInfo::nodeList.size() + redundant);
			if(bestFuncRating[k] != ~0u)
				bestFuncRating[k] += 10 + (redundant == ~0u ? 5 : redundant * 5);	// Cost of variable arguments function
			lastType = CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY);
		}else{
			bestFuncRating[k] = GetFunctionRating(bestFuncList[k]->funcType->funcType, argumentCount);
		}
		if(bestFuncList[k]->generic)
		{
			// If we have a positive rating, check that generic function constraints are satisfied
			if(bestFuncRating[k] != ~0u)
			{
				unsigned newRating = bestFuncRating[k];
				TypeInfo *parentClass = bestFuncList[k]->parentClass;
				if(forcedParentType && bestFuncList[k]->parentClass != forcedParentType)
				{
					assert(forcedParentType->genericBase == bestFuncList[k]->parentClass);
					bestFuncList[k]->parentClass = forcedParentType;
				}
				TypeInfo *tmpType = GetGenericFunctionRating(pos, bestFuncList[k], newRating, count);
				bestFuncList[k]->parentClass = parentClass;

				bestFuncList[k]->generic->instancedType = tmpType;
				bestFuncRating[k] = newRating;
			}else{
				bestFuncList[k]->generic->instancedType = NULL;
			}
			if(bestFuncRating[k] < minGenericRating)
			{
				minGenericRating = bestFuncRating[k];
				minGenericIndex = k;
			}
		}else{
			if(bestFuncRating[k] < minRating)
			{
				minRating = bestFuncRating[k];
				minRatingIndex = k;
			}
		}
		while(argumentCount > callArgCount)
		{
			CodeInfo::nodeList.pop_back();
			argumentCount--;
		}
	}
	// Use generic function only if it is better that selected
	if(minGenericRating < minRating)
	{
		minRating = bestFuncRating[minGenericIndex];
		minRatingIndex = minGenericIndex;
	}else{
		// Otherwise, go as far as disabling all generic functions from selection
		for(unsigned int k = 0; k < count; k++)
			if(bestFuncList[k]->generic)
				bestFuncRating[k] = ~0u;
	}
	return minRatingIndex;
}

bool AddMemberFunctionCall(const char* pos, const char* funcName, unsigned int callArgCount, bool silent)
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
		unsigned minRating = ~0u;
		unsigned minRatingIndex = SelectBestFunction(pos, bestFuncList.size(), callArgCount, minRating);
		if(minRating == ~0u)
			ThrowError(pos, "ERROR: none of the member ::%s functions can handle the supplied parameter list without conversions", funcName);
		FunctionInfo *fInfo = bestFuncList[minRatingIndex];
		if(fInfo && fInfo->generic)
			CreateGenericFunctionInstance(pos, fInfo, fInfo, fInfo->parentClass);

		// Get function type
		TypeInfo *fType = fInfo->funcType;

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
		CodeInfo::nodeList.push_back(new NodeGetAddress(target, target->pos, target->varType));
		((NodeGetAddress*)CodeInfo::nodeList.back())->SetAddressTracking();
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
		return true;
	}
	CheckForImmutable(currentType, pos);
	// Construct name in a form of Class::Function
	char *memberFuncName = GetClassFunctionName(currentType->subType, funcName);
	// If this is generic type instance, maybe the function is not instanced at the moment
	if(currentType->subType->genericBase)
	{
		bestFuncList.clear();

		SelectFunctionsForHash(GetStringHash(memberFuncName), 0);
		// Check that base class has this function
		char *memberOrigName = GetClassFunctionName(currentType->subType->genericBase, funcName);
		SelectFunctionsForHash(GetStringHash(memberOrigName), 0);

		if(!silent && !bestFuncList.size())
			ThrowError(pos, "ERROR: function '%s' is undefined", memberOrigName);

		unsigned minRating = ~0u;
		unsigned minRatingIndex = SelectBestFunction(pos, bestFuncList.size(), callArgCount, minRating, currentType->subType);
		if(minRating == ~0u)
		{
			if(silent)
				return false;
			char	*errPos = errorReport;
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: can't find function '%s' with following parameters:\r\n", funcName);
			ThrowFunctionSelectError(pos, minRating, errorReport, errPos, memberOrigName, callArgCount, bestFuncList.size());
		}
		FunctionInfo *fInfo = bestFuncList[minRatingIndex];
		if(fInfo && fInfo->generic)
		{
			CreateGenericFunctionInstance(pos, fInfo, fInfo, currentType->subType);
			assert(fInfo->parentClass == currentType->subType);
			fInfo->parentClass = currentType->subType;
		}
	}
	// Call it
	return AddFunctionCallNode(pos, memberFuncName, callArgCount, silent);
}

void ThrowFunctionSelectError(const char* pos, unsigned minRating, char* errorReport, char* errPos, const char* funcName, unsigned callArgCount, unsigned count)
{
	errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "  %s(", funcName);
	for(unsigned int n = 0; n < callArgCount; n++)
	{
		if(n != 0)
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ", ");
		NodeZeroOP *activeNode = CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount+n];
		if(activeNode->nodeType == typeNodeFunctionProxy)
		{
			int fID = CodeInfo::funcInfo.size(), fCount = 0;
			while((fID = CodeInfo::FindFunctionByName(((NodeFunctionProxy*)activeNode)->funcInfo->nameHash, fID - 1)) != -1)
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s", fCount++ != 0 ? " or " : "", CodeInfo::funcInfo[fID]->funcType->GetFullTypeName());
		}else if(activeNode->nodeType == typeNodeExpressionList && ((NodeExpressionList*)activeNode)->GetFirstNode()->nodeType == typeNodeFunctionProxy){
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "`function`");
		}else{
			TypeInfo *nodeType = activeNode->nodeType == typeNodeFuncDef ? ((NodeFuncDef*)activeNode)->GetFuncInfo()->funcType : activeNode->typeInfo;
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s", nodeType->GetFullTypeName());
		}
	}
	errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), minRating == ~0u ? ")\r\n the only available are:\r\n" : ")\r\n candidates are:\r\n");
	for(unsigned int n = 0; n < count; n++)
	{
		if(bestFuncRating[n] != minRating)
			continue;
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "  %s %s(", bestFuncList[n]->retType ? bestFuncList[n]->retType->GetFullTypeName() : "auto", funcName);
		for(VariableInfo *curr = bestFuncList[n]->firstParam; curr; curr = curr->next)
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s", curr->varType->GetFullTypeName(), curr != bestFuncList[n]->lastParam ? ", " : "");
		if(bestFuncList[n]->generic)
		{
			if(!bestFuncList[n]->generic->instancedType)
			{
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ") (wasn't instanced here");
			}else{
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ") instanced to\r\n    %s(", funcName);
				FunctionType *tmpType = bestFuncList[n]->generic->instancedType->funcType;
				for(unsigned c = 0; c < tmpType->paramCount; c++)
					errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s", tmpType->paramType[c]->GetFullTypeName(), (c + 1) != tmpType->paramCount ? ", " : "");
			}
		}
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")\r\n");
	}
	CodeInfo::lastError = CompilerError(errorReport, pos);
	longjmp(CodeInfo::errorHandler, 1);
}

// Function expects that nodes with argument types are on top of nodeList
// Return node with function definition if there was one
NodeZeroOP* CreateGenericFunctionInstance(const char* pos, FunctionInfo* fInfo /* generic function */, FunctionInfo*& fResult /* function instance */, TypeInfo* forcedParentType)
{
	if(instanceDepth++ > NULLC_MAX_GENERIC_INSTANCE_DEPTH)
		ThrowError(pos, "ERROR: reached maximum generic function instance depth (%d)", NULLC_MAX_GENERIC_INSTANCE_DEPTH);
	// Get ID of the function that will be created
	unsigned funcID = CodeInfo::funcInfo.size();

	// There may be a type in definition, and we must save it
	TypeInfo *currDefinedType = newType;
	unsigned int currentDefinedTypeMethodCount = methodCount;
	newType = NULL;
	methodCount = 0;
	if(fInfo->type == FunctionInfo::THISCALL)
	{
		currType = forcedParentType ? forcedParentType : fInfo->parentClass;
		TypeContinue(pos);
	}

	FunctionType *fType = fInfo->funcType->funcType;

	// Function return type
	currType = fType->retType;
	if(forcedParentType)
		assert(strchr(fInfo->name, ':'));
	else if(fInfo->parentClass)
		assert(strchr(fInfo->name, ':'));
	FunctionAdd(pos, forcedParentType ? strchr(fInfo->name, ':') + 2 : (fInfo->parentClass ? strchr(fInfo->name, ':') + 2 : fInfo->name));

	// New function type is equal to generic function type no matter where we create an instance of it
	CodeInfo::funcInfo.back()->type = fInfo->type;
	CodeInfo::funcInfo.back()->parentClass = forcedParentType ? forcedParentType : fInfo->parentClass;

	// Get aliases defined in a base function argument list
	AliasInfo *aliasFromParent = fInfo->generic->parent->childAlias;
	while(aliasFromParent)
	{
		CodeInfo::classMap.insert(aliasFromParent->nameHash, aliasFromParent->type);
		aliasFromParent = aliasFromParent->next;
	}
	CodeInfo::funcInfo.back()->childAlias = fInfo->generic->parent->childAlias;
	AliasInfo *aliasParent = CodeInfo::funcInfo.back()->childAlias;

	Lexeme *start = CodeInfo::lexStart + fInfo->generic->start;
	CodeInfo::lastKnownStartPos = NULL;

	unsigned argOffset = CodeInfo::nodeList.size() - fType->paramCount;

	NodeZeroOP *funcDefAtEnd = NULL;

	jmp_buf oldHandler;
	memcpy(oldHandler, CodeInfo::errorHandler, sizeof(jmp_buf));
	if(!setjmp(CodeInfo::errorHandler))
	{
		ParseFunctionVariables(&start, CodeInfo::nodeList.size() - fType->paramCount + 1);
		assert(start->type == lex_cparen);
		start++;
		assert(start->type == lex_ofigure);
		start++;

		// Because we reparse a generic function, our function may be erroneously marked as generic
		CodeInfo::funcInfo[funcID]->generic = NULL;
		CodeInfo::funcInfo[funcID]->genericBase = fInfo->generic;

		FunctionStart(pos);
		const char *lastFunc = SetCurrentFunction(NULL);
		if(!ParseCode(&start))
			AddVoidNode();
		SetCurrentFunction(lastFunc);

		// A true function instance can only be created in the same scope as the base function
		// So we must act as we've made only a function prototype if current scope is wrong
		if(CodeInfo::funcInfo[funcID]->vTopSize != fInfo->vTopSize && !forcedParentType)
		{
			// Special case of FunctionEnd function
			FunctionInfo &lastFunc = *currDefinedFunc.back();
			cycleDepth.pop_back();
			varTop = varInfoTop[lastFunc.vTopSize].varStackSize;
			EndBlock(true, false);	// close function block
			if(!currDefinedFunc.back()->retType)	// resolve function return type if 'auto'
			{
				currDefinedFunc.back()->retType = typeVoid;
				currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back()->retType, currDefinedFunc.back()->firstParam, currDefinedFunc.back()->paramCount);
			}
			currDefinedFunc.pop_back();
			AliasInfo *info = lastFunc.childAlias; // Remove all typedefs made in function
			while(info)
			{
				CodeInfo::classMap.remove(info->nameHash, info->type);
				info = info->next;
			}
			while(lastFunc.childAlias != aliasParent)
				lastFunc.childAlias = lastFunc.childAlias->next;
			lastFunc.pure = false; // Function cannot be evaluated at compile-time
			lastFunc.implemented = false;	// This is a function prototype
			lastFunc.parentFunc = fInfo->parentFunc;	// Fix instance function parent function
			delayedInstance.push_back(&lastFunc); // Add this function to be instanced later
			CodeInfo::nodeList.pop_back(); // Remove function code
		}else{
			FunctionEnd(start->pos);
			funcDefAtEnd = CodeInfo::nodeList.back();
			CodeInfo::nodeList.pop_back();
		}
	}else{
		memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));

		char	*errPos = errorReport;
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "ERROR: while instantiating generic function %s(", fInfo->name);
		for(unsigned i = 0; i < fType->paramCount; i++)
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s", i == 0 ? "" : ", ", fType->paramType[i]->GetFullTypeName());
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")\r\n\tusing argument vector (");
		for(unsigned i = 0; i < fType->paramCount; i++)
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s", i == 0 ? "" : ", ", CodeInfo::nodeList[argOffset + i]->typeInfo->GetFullTypeName());
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")\r\n");
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s", CodeInfo::lastError.GetErrorString());
		if(errPos[-2] == '\r' && errPos[-1] == '\n')
			errPos -= 2;
		*errPos++ = 0;
		CodeInfo::lastError = CompilerError(errorReport, pos);
		longjmp(CodeInfo::errorHandler, 1);
	}
	memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));

	if(fInfo->type == FunctionInfo::THISCALL)
		TypeStop();
	// Restore type that was in definition
	methodCount = currentDefinedTypeMethodCount;
	newType = currDefinedType;

	// A true function instance can only be created in the same scope as the base function
	if(CodeInfo::funcInfo[funcID]->vTopSize != fInfo->vTopSize)
	{
		FunctionInfo &lastFunc = *CodeInfo::funcInfo[funcID];
		if(lastFunc.type == FunctionInfo::LOCAL || lastFunc.type == FunctionInfo::COROUTINE)
		{
			char *hiddenHame = AllocateString(lastFunc.nameLength + 24);
			int length = sprintf(hiddenHame, "$%s_%d_ext", lastFunc.name, lastFunc.indexInArr);
			unsigned beginPos = fInfo->parentFunc ? fInfo->parentFunc->allParamSize + NULLC_PTR_SIZE : 0; // so that a coroutine will not mistake this for argument, choose starting position carefully
			lastFunc.funcContext = new VariableInfo(lastFunc.parentFunc, InplaceStr(hiddenHame, length), GetStringHash(hiddenHame), beginPos, CodeInfo::GetReferenceType(typeInt), !lastFunc.parentFunc);
			lastFunc.funcContext->blockDepth = fInfo->vTopSize;
			// Use generic function expression list
			assert(fInfo->afterNode);
			lastFunc.afterNode = fInfo->afterNode;
			varMap.insert(lastFunc.funcContext->nameHash, lastFunc.funcContext);
		}
	}
	// Fix function outer scope level
	CodeInfo::funcInfo[funcID]->vTopSize = fInfo->vTopSize;

	fResult = CodeInfo::funcInfo[funcID];

	instanceDepth--;
	return funcDefAtEnd;
}

bool AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount, bool silent)
{
	unsigned int funcNameHash = ~0u;

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	NodeZeroOP	*funcAddr = NULL;
	TypeInfo	*forcedParentType = NULL;

	// If we are inside member function, transform function name to a member function name (they have priority over global functions)
	if(newType && funcName)
	{
		unsigned int hash = newType->nameHash, hashBase = ~0u;
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, funcName);

		if(newType->genericBase)
		{
			hashBase = newType->genericBase->nameHash;
			hashBase = StringHashContinue(hashBase, "::");
			hashBase = StringHashContinue(hashBase, funcName);
		}

		FunctionInfo **info = funcMap.find(hash);
		if(info || (newType->genericBase && funcMap.find(hashBase)))
		{
			forcedParentType = newType;
			// If function is found, change function name hash to member function name hash
			funcNameHash = info ? hash : hashBase;

			// Take "this" pointer
			FunctionInfo *currFunc = currDefinedFunc.back();
			TypeInfo *temp = CodeInfo::GetReferenceType(newType);
			CodeInfo::nodeList.push_back(new NodeGetAddress(currFunc->extraParam, currFunc->allParamSize, temp));
			CodeInfo::nodeList.push_back(new NodeDereference());
			// "this" pointer will be passed as extra parameter
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
		// Find class by name
		TypeInfo **type = CodeInfo::classMap.find(funcNameHash);
		// If found, select it
		if(type)
			autoRefToType = *type;
		// If class wasn't found, try all other types
		for(unsigned int i = 0; i < CodeInfo::typeInfo.size() && !autoRefToType; i++)
		{
			if(CodeInfo::typeInfo[i]->GetFullNameHash() == funcNameHash)
				autoRefToType = CodeInfo::typeInfo[i];
		}
		// If a type was found
		if(autoRefToType)
		{
			// Call user function
			if(AddFunctionCallNode(pos, funcName, 1, true))
				return true;
			// If unsuccessful, perform build-in conversion
			if(autoRefToType->refLevel)
			{
				CodeInfo::nodeList.push_back(new NodeConvertPtr(autoRefToType));
			}else{
				CodeInfo::nodeList.push_back(new NodeConvertPtr(CodeInfo::GetReferenceType(autoRefToType)));
				CodeInfo::nodeList.push_back(new NodeDereference());
			}
			return true;
		}
	}

	VariableInfo *vInfo = NULL;
	unsigned	scope = 0;
	// If there was a function name, try to find a variable which may be a function pointer and which will hide functions
	if(funcName)
	{
		VariableInfo **info = varMap.find(funcNameHash);
		if(info)
		{
			vInfo = *info;
			scope = vInfo->blockDepth;
		}
	}

	//Find all functions with given name
	bestFuncList.clear();
	SelectFunctionsForHash(funcNameHash, scope);
	unsigned int count = bestFuncList.size();
	
	TypeInfo **info = NULL;
	// If no functions are found, function name is a type name and a type has member constructors
	if(count == 0 && NULL != (info = CodeInfo::classMap.find(funcNameHash)))
	{
		if((*info)->genericInfo)
			ThrowError(pos, "ERROR: generic type arguments in <> are not found after constructor name");
		if(HasConstructor(pos, *info, callArgCount))
		{
			// Create temporary variable name
			char *arrName = AllocateString(16);
			int length = sprintf(arrName, "$temp%d", inplaceVariableNum++);
			TypeInfo *saveCurrType = currType; // Save type in definition
			currType = *info;
			VariableInfo *varInfo = AddVariable(pos, InplaceStr(arrName, length)); // Create temporary variable
			AddGetAddressNode(pos, InplaceStr(arrName, length)); // Get address
			for(unsigned k = 0; k < callArgCount; k++) // Move address before arguments
			{
				NodeZeroOP *temp = CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - k];
				CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - k] = CodeInfo::nodeList[CodeInfo::nodeList.size() - 2 - k];
				CodeInfo::nodeList[CodeInfo::nodeList.size() - 2 - k] = temp;
			}
			assert(0 == strcmp(funcName, (*info)->name));
			AddMemberFunctionCall(pos, (*info)->genericBase ? (*info)->genericBase->name : (*info)->name, callArgCount); // Call member constructor
			AddPopNode(pos); // Remove result
			CodeInfo::nodeList.push_back(new NodeGetAddress(varInfo, varInfo->pos, varInfo->varType)); // Get address
			AddGetVariableNode(pos); // Dereference
			AddTwoExpressionNode(*info); // Pack two nodes together
			currType = saveCurrType; // Restore type in definition
			return true;
		}
	}

	// If function wasn't found
	if(count == 0)
	{
		// If error is silenced, return false
		if(silent)
			return false;
		// If there was a function name, try to find a variable which may be a function pointer
		if(funcName && !vInfo)
			ThrowError(pos, "ERROR: function '%s' is undefined", funcName);
	}else{
		vInfo = NULL;
	}
	unsigned int minRating = ~0u;
	// If there is a name and it's not a variable that holds 
	if(!vInfo && funcName)
	{
		unsigned minRatingIndex = SelectBestFunction(pos, count, callArgCount, minRating, forcedParentType);
		// Maybe the function we found can't be used at all
		if(minRatingIndex == ~0u)
		{
			if(silent)
				return false;
			assert(minRating == ~0u);
			char	*errPos = errorReport;
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: can't find function '%s' with following parameters:\r\n", funcName);
			ThrowFunctionSelectError(pos, minRating, errorReport, errPos, funcName, callArgCount, count);
		}else{
			fType = bestFuncList[minRatingIndex]->funcType->funcType;
			fInfo = bestFuncList[minRatingIndex];
		}
		// Check, is there are more than one function, that share the same rating
		for(unsigned int k = 0; k < count; k++)
		{
			if(k != minRatingIndex && bestFuncRating[k] == minRating)
			{
				char	*errPos = errorReport;
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: ambiguity, there is more than one overloaded function available for the call:\r\n");
				ThrowFunctionSelectError(pos, minRating, errorReport, errPos, funcName, callArgCount, count);
			}
		}
	}else{
		// If we have variable name, get it
		if(funcName)
			AddGetAddressNode(pos, InplaceStr(funcName, (int)strlen(funcName)));
		// Retrieve value
		AddGetVariableNode(pos);
		// Get function pointer in case we have an in-lined function call
		ConvertFunctionToPointer(pos);
		// Save this node that calculates function pointer for later use
		funcAddr = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		fType = funcAddr->typeInfo->funcType;
		// If we don't have a function type
		if(!fType)
		{
			// Push node that was supposed to calculate function pointer
			CodeInfo::nodeList.push_back(funcAddr);
			// Raise it up to be placed before arguments
			for(unsigned int i = 0; i < callArgCount; i++)
			{
				NodeZeroOP *tmp = CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - i];
				CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - i] = CodeInfo::nodeList[CodeInfo::nodeList.size() - 2 - i];
				CodeInfo::nodeList[CodeInfo::nodeList.size() - 2 - i] = tmp;
			}
			// Call operator()
			return AddFunctionCallNode(pos, "()", callArgCount + 1);
		}
		unsigned int bestRating = ~0u;
		if(callArgCount >= (fType->paramCount-1) && fType->paramCount && fType->paramType[fType->paramCount-1] == typeObjectArray &&
			!(callArgCount == fType->paramCount && CodeInfo::nodeList.back()->typeInfo == typeObjectArray))
		{
			// If function accepts variable argument list
			TypeInfo *&lastType = fType->paramType[fType->paramCount - 1];
			assert(lastType == CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY));
			unsigned int redundant = callArgCount - fType->paramCount;
			if(redundant == ~0u)
				CodeInfo::nodeList.push_back(new NodeZeroOP(typeObjectArray));
			else
				CodeInfo::nodeList.shrink(CodeInfo::nodeList.size() - redundant);
			// Change things in a way that this function will be selected (lie about real argument count and match last argument type)
			lastType = CodeInfo::nodeList.back()->typeInfo;
			bestRating = GetFunctionRating(fType, fType->paramCount);
			if(redundant == ~0u)
				CodeInfo::nodeList.pop_back();
			else
				CodeInfo::nodeList.resize(CodeInfo::nodeList.size() + redundant);
			lastType = CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY);
		}else{
			bestRating = GetFunctionRating(fType, callArgCount);
		}
		if(bestRating == ~0u)
		{
			char	*errPos = errorReport;
			if(callArgCount != fType->paramCount)
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: function expects %d argument(s), while %d are supplied\r\n", fType->paramCount, callArgCount);
			else
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: there is no conversion from specified arguments and the ones that function accepts\r\n");
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "\tExpected: (");
			for(unsigned int n = 0; n < fType->paramCount; n++)
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s", fType->paramType[n]->GetFullTypeName(), n != fType->paramCount - 1 ? ", " : "");
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")\r\n");
			
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "\tProvided: (");
			for(unsigned int n = 0; n < callArgCount; n++)
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s", CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount+n]->typeInfo->GetFullTypeName(), n != callArgCount-1 ? ", " : "");
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")");
			CodeInfo::lastError = CompilerError(errorReport, pos);
			longjmp(CodeInfo::errorHandler, 1);
		}
	}
	NodeZeroOP *funcDefAtEnd = NULL;
	if(fInfo && fInfo->generic)
	{
		funcDefAtEnd = CreateGenericFunctionInstance(pos, fInfo, fInfo, forcedParentType);
		fType = fInfo->funcType->funcType;
	}

	if(fInfo && callArgCount < fType->paramCount)
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
	if(callArgCount >= (fType->paramCount-1) && fType->paramCount && fType->paramType[fType->paramCount - 1] == typeObjectArray &&
		!(callArgCount == fType->paramCount && CodeInfo::nodeList.back()->typeInfo == typeObjectArray))
	{
		if(callArgCount >= fType->paramCount)
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
		}else{
			AddNullPointer();
			HandlePointerToObject(pos, CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY));
			callArgCount++;
		}
	}

	paramNodes.clear();
	for(unsigned int i = 0; i < callArgCount; i++)
	{
		paramNodes.push_back(CodeInfo::nodeList.back());
		CodeInfo::nodeList.pop_back();
	}

	if(funcName && funcAddr && newType && !vInfo)
	{
		CodeInfo::nodeList.push_back(funcAddr);
		funcAddr = NULL;
	}
	for(unsigned int i = 0; i < fType->paramCount; i++)
	{
		unsigned int index = fType->paramCount - i - 1;

		CodeInfo::nodeList.push_back(paramNodes[index]);

		ConvertFunctionToPointer(pos, fType->paramType[i]);
		ConvertArrayToUnsized(pos, fType->paramType[i]);
		if(fType->paramType[i] == typeAutoArray && CodeInfo::nodeList.back()->typeInfo->arrLevel)
		{
			TypeInfo *actualType = CodeInfo::nodeList.back()->typeInfo;
			if(actualType->arrSize != TypeInfo::UNSIZED_ARRAY)
				ConvertArrayToUnsized(pos, CodeInfo::GetArrayType(actualType->subType, TypeInfo::UNSIZED_ARRAY));
			// type[] on stack: size; ptr (top)
			CodeInfo::nodeList.push_back(new NodeZeroOP(typeInt));
			CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdPushTypeID, actualType->subType->typeIndex));
			AddTwoExpressionNode(typeAutoArray);
		}
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

	if(fInfo)
		GetFunctionContext(pos, fInfo, false);
	if(funcAddr)
		CodeInfo::nodeList.push_back(funcAddr);
	CodeInfo::nodeList.push_back(new NodeFuncCall(fInfo, fType));
	if(currDefinedFunc.size() && fInfo && !fInfo->pure)
		currDefinedFunc.back()->pure = false;	// non-pure function call invalidates function purity
#ifdef NULLC_PURE_FUNCTIONS
	// Pure function evaluation
	if(fInfo && fInfo->pure && !(currDefinedFunc.size() && currDefinedFunc.back() == fInfo))
	{
#ifdef _DEBUG
		static int inside = false;
		assert(!inside);
		inside = true;
#endif
		static char memory[1024];
		NodeFuncCall::baseShift = 0;
		if(NodeNumber *value = CodeInfo::nodeList.back()->Evaluate(memory, 1024))
		{
			CodeInfo::nodeList.back() = value;
		}else{
			if(!uncalledFunc)
			{
				uncalledFunc = fInfo;
				uncalledPos = pos;
			}
		}
#ifdef _DEBUG
		inside = false;
#endif
	}
	if(fInfo && !fInfo->pure && !uncalledFunc)
	{
		uncalledFunc = fInfo;
		uncalledPos = pos;
	}
#endif

	if(funcDefAtEnd)
	{
		CodeInfo::nodeList.push_back(funcDefAtEnd);
		NodeExpressionList *temp = new NodeExpressionList(CodeInfo::nodeList[CodeInfo::nodeList.size() - 2]->typeInfo);
		temp->AddNode(false);
		CodeInfo::nodeList.push_back(temp);
	}

	return true;
}
bool OptimizeIfElse(bool hasElse)
{
	if(CodeInfo::nodeList[CodeInfo::nodeList.size() - (hasElse ? 3 : 2)]->nodeType == typeNodeNumber)
	{
		if(!hasElse)
			CodeInfo::nodeList.push_back(new NodeZeroOP());
		int condition = ((NodeNumber*)CodeInfo::nodeList[CodeInfo::nodeList.size()-3])->GetInteger();
		NodeZeroOP *remainingNode = condition ? CodeInfo::nodeList[CodeInfo::nodeList.size()-2] : CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.back() = remainingNode;
		return true;
	}
	return false;
}
void AddIfNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 2);

	if(OptimizeIfElse(false))
		return;

	CodeInfo::nodeList.push_back(new NodeIfElseExpr(false));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}
void AddIfElseNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 3);

	if(OptimizeIfElse(true))
		return;

	CodeInfo::nodeList.push_back(new NodeIfElseExpr(true));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}
void AddIfElseTermNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 3);

	if(OptimizeIfElse(true))
		return;

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

// Begin type definition
void TypeBegin(const char* pos, const char* end)
{
	if(newType)
		ThrowError(pos, "ERROR: different type is being defined");
	if(currAlign > 16)
		ThrowError(pos, "ERROR: alignment must be less than 16 bytes");

	char *typeNameCopy = AllocateString((int)(end - pos) + 1);
	sprintf(typeNameCopy, "%.*s", (int)(end - pos), pos);

	unsigned int hash = GetStringHash(typeNameCopy);
	TypeInfo **type = CodeInfo::classMap.find(hash);
	if(type)
		ThrowError(pos, "ERROR: '%s' is being redefined", typeNameCopy);
	newType = new TypeInfo(CodeInfo::typeInfo.size(), typeNameCopy, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	newType->alignBytes = currAlign;
	newType->originalIndex = CodeInfo::typeInfo.size();
	newType->definitionDepth = varInfoTop.size();
	newType->hasFinished = false;
	currAlign = TypeInfo::UNSPECIFIED_ALIGNMENT;
	methodCount = 0;

	CodeInfo::typeInfo.push_back(newType);
	CodeInfo::classMap.insert(newType->GetFullNameHash(), newType);

	BeginBlock();
}

// Add class member
void TypeAddMember(const char* pos, const char* varName)
{
	if(!currType)
		ThrowError(pos, "ERROR: auto cannot be used for class members");
	if(!currType->hasFinished)
		ThrowError(pos, "ERROR: Type '%s' is currently being defined. You can use '%s ref' or '%s[]' at this point", currType->GetFullTypeName(), currType->GetFullTypeName(), currType->GetFullTypeName());
	// Align members to their default alignment, but not larger that 4 bytes
	unsigned int alignment = currType->alignBytes > 4 ? 4 : currType->alignBytes;
	if(alignment && newType->size % alignment != 0)
		newType->size += alignment - (newType->size % alignment);
	newType->AddMemberVariable(varName, currType);
	if(newType->size > 64 * 1024)
		ThrowError(pos, "ERROR: class size cannot exceed 65535 bytes");
	if(currType->hasFinalizer)
		ThrowError(pos, "ERROR: class '%s' implements 'finalize' so only a reference or an unsized array of '%s' can be put in a class", currType->GetFullTypeName(), currType->GetFullTypeName());

	VariableInfo *varInfo = (VariableInfo*)AddVariable(pos, InplaceStr(varName, (int)strlen(varName)));
	varInfo->isGlobal = true;
	varInfo->parentType = newType;
}

// End of type definition
void TypeFinish()
{
	// Class members changed variable top, here we restore it to original position
	varTop -= newType->size;
	// In NULLC, all classes have sizes multiple of 4, so add padding if necessary
	if(newType->size % 4 != 0)
	{
		newType->paddingBytes = 4 - (newType->size % 4);
		newType->size += 4 - (newType->size % 4);
	}

	// Wrap all member function definitions into one expression list
	CodeInfo::nodeList.push_back(new NodeZeroOP());
	AddOneExpressionNode();
	for(unsigned int i = 0; i < methodCount; i++)
		AddTwoExpressionNode();
	newType->definitionList = CodeInfo::nodeList.back();

	// Shift new types generated inside up, so that declaration will be in the correct order in C translation
	for(unsigned int i = newType->originalIndex + 1; i < CodeInfo::typeInfo.size(); i++)
		CodeInfo::typeInfo[i]->originalIndex--;
	newType->originalIndex = CodeInfo::typeInfo.size() - 1;

	// Remove all aliases defined inside class definition
	AliasInfo *info = newType->childAlias;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}
	newType->hasFinished = true;

	newType = NULL;

	// Remove class members from global scope
	EndBlock(false, false);
}

// Before we add member function outside of the class definition, we should imitate that we're inside class definition
void TypeContinue(const char* pos)
{
	if(newType)
		ThrowError(pos, "ERROR: cannot continue type '%s' definition inside '%s' type. Possible cause: external member function definition syntax inside a class", currType->GetFullTypeName(), newType->GetFullTypeName());
	newType = currType;
	newType->definitionDepth = varInfoTop.size();
	// Add all member variables to global scope
	BeginBlock();
	for(TypeInfo::MemberVariable *curr = newType->firstVariable; curr; curr = curr->next)
	{
		currType = curr->type;
		currAlign = 4;
		VariableInfo *varInfo = (VariableInfo*)AddVariable(pos, InplaceStr(curr->name));
		varInfo->isGlobal = true;
		varInfo->parentType = newType;
		varDefined = false;
	}
	// Restore all type aliases defined inside a class
	AliasInfo *info = newType->childAlias;
	while(info)
	{
		CodeInfo::classMap.insert(info->nameHash, info->type);
		info = info->next;
	}
}

// End of outside class member definition
void TypeStop()
{
	// Class members changed variable top, here we restore it to original position
	varTop = varInfoTop.back().varStackSize;
	// Remove all aliases defined inside class definition
	AliasInfo *info = newType->childAlias;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}
	newType = NULL;
	// Remove class members from global scope
	EndBlock(false, false);
}

void TypeGeneric(unsigned pos)
{
	newType->genericInfo = newType->CreateGenericContext(pos);
	newType->genericInfo->globalVarTop = varTop;
	newType->genericInfo->blockDepth = currDefinedFunc.size() ? currDefinedFunc.back()->vTopSize : 1;
	currType = newType;
}

void TypeInstanceGeneric(const char* pos, TypeInfo* base, unsigned aliases)
{
	Lexeme *start = CodeInfo::lexStart + base->genericInfo->start;
	NodeZeroOP **aliasType = &CodeInfo::nodeList[CodeInfo::nodeList.size() - aliases];

	// Generate instance name
	char tempName[NULLC_MAX_VARIABLE_NAME_LENGTH];
	char *namePos = tempName;
	namePos += SafeSprintf(namePos, NULLC_MAX_VARIABLE_NAME_LENGTH - int(namePos - tempName), "%s<", base->name);
	for(unsigned i = 0; i < aliases; i++)
		namePos += SafeSprintf(namePos, NULLC_MAX_VARIABLE_NAME_LENGTH - int(namePos - tempName), "%s%s", aliasType[i]->typeInfo->GetFullTypeName(), i == aliases - 1 ? "" : ", ");
	namePos += SafeSprintf(namePos, NULLC_MAX_VARIABLE_NAME_LENGTH - int(namePos - tempName), ">");

	// Check name length
	if(NULLC_MAX_VARIABLE_NAME_LENGTH - int(namePos - tempName) == 0)
		ThrowError(pos, "ERROR: generated generic type name exceeds maximum type length '%d'", NULLC_MAX_VARIABLE_NAME_LENGTH);

	AliasInfo *aliasList = NULL;

	unsigned aliasID = 0;
	// We are reparsing original class definition
	do
	{
		currType = aliasType[aliasID]->typeInfo;
		assert(start->type == lex_string); // This was already checked during parsing

		InplaceStr aliasName = InplaceStr(start->pos, start->length);
		AliasInfo *info = TypeInfo::CreateAlias(aliasName, currType);
		info->next = aliasList;
		aliasList = info;
		CodeInfo::classMap.insert(info->nameHash, currType);

		start++;
		aliasID++;
	}while(start->type == lex_comma ? !!start++ : false);
	if(aliasID > aliases)
		ThrowError(pos, "ERROR: there where only '%d' argument(s) to a generic type that expects '%d'", aliases, aliasID);
	else if(aliasID < aliases)
		ThrowError(pos, "ERROR: type has only '%d' generic argument(s) while '%d' specified", aliasID, aliases);
	assert(start->type == lex_greater);
	start++;

	base->genericInfo->aliasCount = aliases;

	// Remove type nodes used to instance class
	for(unsigned i = 0; i < aliases; i++)
		CodeInfo::nodeList.pop_back();

	// Search if the type was already defined
	if(TypeInfo **lastType = CodeInfo::classMap.find(GetStringHash(tempName, namePos)))
	{
		while(aliasList)
		{
			CodeInfo::classMap.remove(aliasList->nameHash, aliasList->type);
			aliasList = aliasList->next;
		}
		currType = *lastType;
		return;
	}

	if(instanceDepth++ > NULLC_MAX_GENERIC_INSTANCE_DEPTH)
		ThrowError(pos, "ERROR: reached maximum generic type instance depth (%d)", NULLC_MAX_GENERIC_INSTANCE_DEPTH);

	// If not found, create a new type
	SetCurrentAlignment(base->alignBytes);
	// Save the type that may be in definition
	TypeInfo *currentDefinedType = newType;
	unsigned int currentDefinedTypeMethodCount = methodCount;
	// Begin new type
	newType = NULL;
	TypeBegin(tempName, namePos);
	TypeInfo *instancedType = newType;
	newType->genericBase = base;
	newType->childAlias = aliasList;

	// Reparse type body and format errors so that the user will know where it happened
	jmp_buf oldHandler;
	memcpy(oldHandler, CodeInfo::errorHandler, sizeof(jmp_buf));
	if(!setjmp(CodeInfo::errorHandler))
	{
		ParseClassBody(&start);
	}else{
		memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));

		char	*errPos = errorReport;
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "ERROR: while instantiating generic type %s:\r\n", instancedType->name);
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s", CodeInfo::lastError.GetErrorString());
		if(errPos[-2] == '\r' && errPos[-1] == '\n')
			errPos -= 2;
		*errPos++ = 0;
		CodeInfo::lastError = CompilerError(errorReport, pos);
		longjmp(CodeInfo::errorHandler, 1);
	}
	// Stop type definition
	TypeFinish();
	((NodeExpressionList*)base->definitionList)->AddNode();
	// Restore old error handler
	memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));

	currType = instancedType;
	// Restore type that may have been in definition
	methodCount = currentDefinedTypeMethodCount;
	newType = currentDefinedType;

	instanceDepth--;
}

void AddAliasType(InplaceStr aliasName)
{
	AliasInfo *info = TypeInfo::CreateAlias(aliasName, currType);
	if(newType && !currDefinedFunc.size())	// If we're inside a class definition, but _not_ inside a function
	{
		// Create alias and add it to type alias list, so that after type definition is finished, local aliases will be deleted
		info->next = newType->childAlias;
		newType->childAlias = info;
	}else if(currDefinedFunc.size()){	// If we're inside a function
		// Create alias and add it to function alias list, so that after function is finished, local aliases will be deleted
		info->next = currDefinedFunc.back()->childAlias;
		currDefinedFunc.back()->childAlias = info;
	}else{
		// Create alias and add it to global alias list
		info->next = CodeInfo::globalAliases;
		CodeInfo::globalAliases = info;
	}
	// Insert hash->type pair to class map, so that target type can be found by alias name
	CodeInfo::classMap.insert(GetStringHash(aliasName.begin, aliasName.end), currType);
}

void AddUnfixedArraySize()
{
	CodeInfo::nodeList.push_back(new NodeNumber(1, typeVoid));
}

void CreateRedirectionTables()
{
	// Search for virtual function tables in imported modules so that we can add functions from new classes
	unsigned continuations = 0;
	for(unsigned i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		// Name must start from $vtbl and must be at least 15 characters
		if(int(CodeInfo::varInfo[i]->name.end - CodeInfo::varInfo[i]->name.begin) < 15 || memcmp(CodeInfo::varInfo[i]->name.begin, "$vtbl", 5) != 0)
			continue;
		CodeInfo::varInfo[i]->next = vtblList;
		vtblList = CodeInfo::varInfo[i];
		continuations++;
	}
	VariableInfo *curr = vtblList;
	unsigned int num = 0;
	while(curr)
	{
		TypeInfo *funcType = NULL;
		unsigned int hash = GetStringHash(curr->name.begin + 15); // 15 to skip $vtbl0123456789 from name

		// If this is continuation of an imported virtual table, find function type from has code in name
		if(continuations)
		{
			unsigned typeHash = parseInteger(curr->name.begin + 5); // 5 to skip $vtbl
			for(unsigned c = 0; !funcType && c < CodeInfo::typeFunctions.size(); c++)
			{
				if(CodeInfo::typeFunctions[c]->GetFullNameHash() == typeHash)
					funcType = CodeInfo::typeFunctions[c];
			}
			AddVoidNode();
			continuations--;
		}else{
			currType = curr->varType;
			CodeInfo::nodeList.push_back(new NodeNumber(4, typeInt));
			AddFunctionCallNode(CodeInfo::lastKnownStartPos, "__typeCount", 0);
			CodeInfo::nodeList.push_back(new NodeNumber(0, typeInt));
			AddFunctionCallNode(CodeInfo::lastKnownStartPos, "__newA", 3);
			CodeInfo::nodeList.back()->typeInfo = currType;
			CodeInfo::varInfo.push_back(curr);
			curr->pos = varTop;
			varTop += curr->varType->size;

			AddDefineVariableNode(CodeInfo::lastKnownStartPos, curr, true);
			AddPopNode(CodeInfo::lastKnownStartPos);

			funcType = (TypeInfo*)curr->prev;
		}

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
					CodeInfo::nodeList.push_back(new NodeGetAddress(curr, curr->pos, curr->varType));
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

struct TypeHandler
{
	TypeInfo	*varType;
	TypeHandler	*next;
};

void AddListGenerator(const char* pos, TypeInfo *rType)
{
//typedef int generic;
	TypeInfo *retType = rType;

	currType = CodeInfo::GetArrayType(retType, TypeInfo::UNSIZED_ARRAY);

//generic[] gen_list(generic ref() y)
	char *functionName = AllocateString(16);
	sprintf(functionName, "$gen_list");
	FunctionAdd(pos, functionName);

	TypeHandler* h = NULL;
	SelectTypeByPointer(CodeInfo::GetFunctionType(retType, h, 0));
	FunctionParameter(pos, InplaceStr("y"));
	FunctionStart(pos);

//	auto[] res = auto_array(generic, 1);
	currType = typeAutoArray;
	VariableInfo *varInfo = AddVariable(pos, InplaceStr("res"));
	currType = retType;
	GetTypeId(pos);
	CodeInfo::nodeList.push_back(new NodeNumber(1, typeInt));
	AddFunctionCallNode(pos, "auto_array", 2);
	AddDefineVariableNode(pos, varInfo);
	AddPopNode(pos);
//	int pos = 0;
	currType = typeInt;
	varInfo = AddVariable(pos, InplaceStr("pos"));
	CodeInfo::nodeList.push_back(new NodeNumber(0, typeInt));
	AddDefineVariableNode(pos, varInfo);
	AddPopNode(pos);

//	for(x in y)
	IncreaseCycleDepth();
	BeginBlock();

	AddGetAddressNode(pos, InplaceStr("y"));
	AddGetVariableNode(pos);
	AddArrayIterator("x", InplaceStr("x"), NULL);

//		res.set(x, pos++);
	AddGetAddressNode(pos, InplaceStr("res"));
	AddGetAddressNode(pos, InplaceStr("x"));
	AddGetAddressNode(pos, InplaceStr("pos"));
	AddUnaryModifyOpNode(pos, OP_INCREMENT, OP_POSTFIX);
	AddMemberFunctionCall(pos, "set", 2);
	AddPopNode(pos);

	EndBlock();
	AddForEachNode(pos);

//	__force_size(&res, pos);
	AddGetAddressNode(pos, InplaceStr("res"));
	AddGetAddressNode(pos, InplaceStr("pos"));
	AddGetVariableNode(pos);
	AddFunctionCallNode(pos, "__force_size", 2);
	AddPopNode(pos);

//	generic[] r = res;
	currType = CodeInfo::GetArrayType(retType, TypeInfo::UNSIZED_ARRAY);
	varInfo = AddVariable(pos, InplaceStr("r"));
	AddGetAddressNode(pos, InplaceStr("res"));
	AddGetVariableNode(pos);
	AddDefineVariableNode(pos, varInfo);
	AddPopNode(pos);
//	return r;
	AddGetAddressNode(pos, InplaceStr("r"));
	AddGetVariableNode(pos);
	AddReturnNode(pos);
//}
	AddTwoExpressionNode();
	AddTwoExpressionNode();
	AddTwoExpressionNode();
	AddTwoExpressionNode();
	AddTwoExpressionNode();
	FunctionEnd(pos);
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

	CodeInfo::globalAliases = NULL;

	varDefined = false;
	varTop = 0;
	newType = NULL;
	instanceDepth = 0;

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

	delayedInstance.clear();

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

	defineCoroutine = false;

	if(!errorReport)
		errorReport = (char*)NULLC::alloc(NULLC_ERROR_BUFFER_SIZE);
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
	currFunction = NULL;
	currArgument = 0;

	varInfoTop.reset();
	funcInfoTop.reset();
	cycleDepth.reset();
	currDefinedFunc.reset();
	delayedInstance.reset();
	bestFuncList.reset();
	bestFuncRating.reset();
	bestFuncListBackup.reset();
	bestFuncRatingBackup.reset();
	paramNodes.reset();
	funcMap.reset();
	varMap.reset();

	TypeInfo::typeInfoPool.~ChunkedStackPool();
	TypeInfo::SetPoolTop(0);
	VariableInfo::variablePool.~ChunkedStackPool();
	VariableInfo::SetPoolTop(0);
	FunctionInfo::functionPool.~ChunkedStackPool();
	FunctionInfo::SetPoolTop(0);

	NULLC::dealloc(errorReport);
	errorReport = NULL;
}
