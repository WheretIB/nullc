// NULLC (c) NULL_PTR 2007-2011

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
unsigned		newVtblCount = 0;

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

unsigned	extendableVariableName = GetStringHash("$typeid");

unsigned GetFunctionHiddenName(char* buf, FunctionInfo &info)
{
	assert(info.funcType);
	return sprintf(buf, "$%s_%u_ext", info.name, info.type == FunctionInfo::LOCAL ? info.indexInArr : info.funcType->GetFullNameHash());
}

void	AddFunctionToSortedList(FunctionInfo *info)
{
	funcMap.insert(info->nameHash, info);
}

const char*	currFunction = NULL;
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

FastVector<NamespaceInfo*>	namespaceStack;
NamespaceInfo*				currNamespace = NULL;

FastVector<AliasInfo*>		explicitTypesStack;
AliasInfo*					currExplicitTypes = NULL;

FastVector<NodeZeroOP*>		namedArgumentBackup;

void PushNamespace(InplaceStr space)
{
	if(currDefinedFunc.size())
		ThrowError(space.begin, "ERROR: a namespace definition must appear either at file scope or immediately within another namespace definition");

	// Construct a name hash in a form of "previousname.currentname"
	unsigned hash;
	if(namespaceStack.size() == 1)
	{
		hash = GetStringHash(space.begin, space.end);
	}else{
		hash = namespaceStack.back()->hash;
		hash = StringHashContinue(hash, ".");
		hash = StringHashContinue(hash, space.begin, space.end);
	}

	// Find collisions with known names
	if(TypeInfo **info = CodeInfo::classMap.find(hash))
		ThrowError(space.begin, "ERROR: name '%.*s' is already taken for a class", space.length(), space.begin);
	CheckCollisionWithFunction(space.begin, space, hash, varInfoTop.size());

	// Find if namespace is already created
	unsigned i = 0;
	for(; i < CodeInfo::namespaceInfo.size(); i++)
	{
		if(CodeInfo::namespaceInfo[i]->hash == hash)
			break;
	}
	if(i != CodeInfo::namespaceInfo.size())
	{
		namespaceStack.push_back(CodeInfo::namespaceInfo[i]);
		return;
	}
	CodeInfo::namespaceInfo.push_back(new NamespaceInfo(space, hash, namespaceStack.size() == 1 ? NULL : namespaceStack.back()));
	namespaceStack.push_back(CodeInfo::namespaceInfo.back());
}

void PopNamespace()
{
	namespaceStack.pop_back();
}
NamespaceInfo* IsNamespace(InplaceStr space)
{
	for(int i = namespaceStack.size() - 1; i >= 0; i--)
	{
		unsigned hash = currNamespace ? currNamespace->hash : 0;
		if(i == 0)
		{
			if(currNamespace)
			{
				hash = StringHashContinue(hash, ".");
				hash = StringHashContinue(hash, space.begin, space.end);
			}else{
				hash = GetStringHash(space.begin, space.end);
			}
		}else{
			hash = namespaceStack[i]->hash;
			hash = StringHashContinue(hash, ".");
			hash = StringHashContinue(hash, space.begin, space.end);
		}
		for(unsigned k = 0; k < CodeInfo::namespaceInfo.size(); k++)
		{
			if(CodeInfo::namespaceInfo[k]->hash == hash)
				return CodeInfo::namespaceInfo[k];
		}
	}
	return NULL;
}
NamespaceInfo* GetCurrentNamespace()
{
	return currNamespace;
}
void SetCurrentNamespace(NamespaceInfo* space)
{
	currNamespace = space;
}

void PushExplicitTypes(AliasInfo* list)
{
	explicitTypesStack.push_back(currExplicitTypes);
	currExplicitTypes = list;
}

AliasInfo* GetExplicitTypes()
{
	return currExplicitTypes;
}

void PopExplicitTypes()
{
	currExplicitTypes = explicitTypesStack.back();
	explicitTypesStack.pop_back();
}

InplaceStr GetNameInNamespace(InplaceStr name, bool alwaysRelocate = false)
{
	if(namespaceStack.size() > 1)
	{
		// A full name must be created if we are in a namespace
		unsigned nameLength = name.length() + 1, lastLength = nameLength;
		for(unsigned int i = 1; i < namespaceStack.size(); i++)
			nameLength += namespaceStack[i]->nameLength + 1; // add space for namespace name and a point
		char *newName = AllocateString(nameLength), *currPos = newName;
		for(unsigned int i = 1; i < namespaceStack.size(); i++)
		{
			memcpy(currPos, namespaceStack[i]->name.begin, namespaceStack[i]->nameLength);
			currPos += namespaceStack[i]->nameLength;
			*currPos++ = '.';
		}
		memcpy(currPos, name.begin, lastLength);
		currPos[lastLength - 1] = 0;
		name = InplaceStr(newName);
	}else if(alwaysRelocate){
		char *newName = AllocateString(name.length() + 1);
		memcpy(newName, name.begin, name.length());
		newName[name.length()] = 0;
		name = InplaceStr(newName);
	}
	return name;
}
unsigned AddNamespaceToHash(unsigned hash, NamespaceInfo* info)
{
	if(info->parent)
		hash = AddNamespaceToHash(hash, info->parent);
	hash = StringHashContinue(hash, info->name.begin, info->name.end);
	hash = StringHashContinue(hash, ".");
	return hash;
}

template<typename T>
void NamespaceSelect(InplaceStr name, T& state)
{
	for(unsigned i = namespaceStack.size() - 1; i > 0 && state.proceed(); i--)
	{
		unsigned hash = namespaceStack[i]->hash;
		hash = StringHashContinue(hash, ".");
		if(currNamespace)
			hash = AddNamespaceToHash(hash, currNamespace);
		hash = StringHashContinue(hash, name.begin, name.end);
		state.action(hash);
	}
	if(state.proceed())
	{
		unsigned int hash = 0;
		if(NamespaceInfo *ns = currNamespace)
		{
			hash = ns->hash;
			hash = StringHashContinue(hash, ".");
			hash = StringHashContinue(hash, name.begin, name.end);
		}else{
			hash = GetStringHash(name.begin, name.end);
		}
		state.action(hash);
	}
}

ChunkedStackPool<65532> TypeInfo::typeInfoPool;
ChunkedStackPool<65532> VariableInfo::variablePool;
ChunkedStackPool<65532> FunctionInfo::functionPool;
ChunkedStackPool<65532> NamespaceInfo::namespacePool;

FastVector<FunctionInfo*>	bestFuncList;
FastVector<unsigned int>	bestFuncRating;

FastVector<FunctionInfo*>	bestFuncListBackup;
FastVector<unsigned int>	bestFuncRatingBackup;

FastVector<NamespaceInfo*>	namespaceBackup;

FastVector<NodeZeroOP*>		nodeBackup;

FastVector<NodeZeroOP*>		paramNodes;

struct ClassSelect
{
	ClassSelect(): type(NULL){}
	bool	proceed(){ return !type; }
	void	action(unsigned hash){ type = CodeInfo::classMap.find(hash); }
	TypeInfo **type;
};

struct VariableSelect
{
	VariableSelect(): curr(NULL){}
	bool	proceed(){ return !curr; }
	void	action(unsigned hash){ curr = varMap.first(hash); }
	HashMap<VariableInfo*>::Node *curr;
};

struct FunctionSelect
{
	FunctionSelect(): curr(NULL){}
	bool	proceed(){ return !curr; }
	void	action(unsigned hash){ curr = funcMap.first(hash); }
	HashMap<FunctionInfo*>::Node	*curr;
};

struct FunctionSelectCall
{
	FunctionSelectCall(int scope): scope(scope){}
	bool	proceed(){ return !bestFuncList.size(); }
	void	action(unsigned hash){ SelectFunctionsForHash(hash, scope); }
	unsigned scope;
};

TypeInfo* SelectTypeByName(InplaceStr name)
{
	ClassSelect f;
	NamespaceSelect(name, f);
	return f.type ? *f.type : NULL;
}

HashMap<VariableInfo*>::Node* SelectVariableByName(InplaceStr name)
{
	VariableSelect f;
	NamespaceSelect(name, f);
	HashMap<VariableInfo*>::Node *curr = f.curr;
	// In generic function instance, skip all variables that are defined after the base generic function
	while(curr && currDefinedFunc.size() && currDefinedFunc.back()->genericBase && !(curr->value->pos >> 24) && !(curr->value->parentType) && (curr->value->isGlobal ? currDefinedFunc.back()->genericBase->globalVarTop <= curr->value->pos : (currDefinedFunc.back()->genericBase->blockDepth < curr->value->blockDepth && currDefinedFunc.back() != curr->value->parentFunction)))
		curr = varMap.next(curr);
	return curr;
}

void AddInplaceVariable(const char* pos, TypeInfo* targetType = NULL);
void ConvertArrayToUnsized(const char* pos, TypeInfo *dstType);
NodeZeroOP* CreateGenericFunctionInstance(const char* pos, FunctionInfo* fInfo, FunctionInfo*& fResult, unsigned callArgCount, TypeInfo* forcedParentType = NULL);
void ConvertFunctionToPointer(const char* pos, TypeInfo *dstPreferred = NULL);
void HandlePointerToObject(const char* pos, TypeInfo *dstType);
void ConvertDerivedToBase(const char* pos, TypeInfo *dstType);
void ConvertBaseToDerived(const char* pos, TypeInfo *dstType);
void ThrowFunctionSelectError(const char* pos, unsigned minRating, char* errorReport, char* errPos, const char* funcName, unsigned callArgCount, unsigned count);
void RestoreNamespaces(bool undo, NamespaceInfo *parent, unsigned& prevBackupSize, unsigned& prevStackSize, NamespaceInfo*& lastNS);

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
	for(FunctionInfo::ExternalInfo *curr = func->firstExternal; curr; curr = curr->next)
	{
		if(curr->variable == var)
			return curr;
	}

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
	char	*memberFuncName = AllocateString(type->GetFullNameLength() + 2 + funcName.length() + 1);
	sprintf(memberFuncName, "%s::%.*s", type->GetFullTypeName(), funcName.length(), funcName.begin);
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

			// Backup namespace state and restore it to function definition state
			unsigned prevBackupSize = 0, prevStackSize = 0;
			NamespaceInfo *lastNS = NULL;
			RestoreNamespaces(false, fInfo->parentNamespace, prevBackupSize, prevStackSize, lastNS);

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
			const char *name = fInfo->name;
			if(fInfo->parentClass)
			{
				assert(strchr(fInfo->name, ':'));
				name = strchr(fInfo->name, ':') + 2;
			}
			if(const char* pos = strrchr(name, '.'))
				name = pos + 1;
			FunctionAdd(pos, name);

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

				// Steal the argument default value
				CodeInfo::funcInfo.back()->lastParam->defaultValue = curr->defaultValue;
				curr->defaultValue = NULL;
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

			RestoreNamespaces(true, fInfo->parentNamespace, prevBackupSize, prevStackSize, lastNS);
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
	if(alignment & (alignment - 1))
		ThrowError(pos, "ERROR: alignment must be power of two");

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
			ThrowError(pos, "ERROR: name '%.*s' is already taken for a function", varName.length(), varName.begin);
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
				CodeInfo::lastKnownStartPos = curr;
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
	// If the last node is increment or decrement, then we do not need to keep the value on stack, and some optimizations can be done
	if(CodeInfo::nodeList.back()->nodeType == typeNodePreOrPostOp)
	{
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
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && CodeInfo::nodeList.back()->typeInfo != typeBool)
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
			ThrowError(pos, "ERROR: unary operation '-' is not supported on '%s'", aType->GetFullTypeName());

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
			Rd = new NodeNumber((long long)!static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetLong(), typeBool);
		else if(aType == typeInt || aType == typeShort || aType == typeChar || aType == typeBool)
			Rd = new NodeNumber(!static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger(), typeBool);
		else
			ThrowError(pos, "ERROR: unary operation '!' is not supported on '%s'", aType->GetFullTypeName());

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
	if(CodeInfo::nodeList.back()->nodeType == typeNodeNumber && CodeInfo::nodeList.back()->typeInfo != typeBool)
	{
		TypeInfo *aType = CodeInfo::nodeList.back()->typeInfo;
		NodeZeroOP* Rd = NULL;
		if(aType == typeLong)
			Rd = new NodeNumber(~static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetLong(), aType);
		else if(aType == typeInt || aType == typeShort || aType == typeChar)
			Rd = new NodeNumber(~static_cast<NodeNumber*>(CodeInfo::nodeList.back())->GetInteger(), aType);
		else
			ThrowError(pos, "ERROR: unary operation '~' is not supported on '%s'", aType->GetFullTypeName());

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

void WrapNodeToFunction(const char* pos)
{
	NodeZeroOP *wrapee = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	char	*functionName = AllocateString(16);
	sprintf(functionName, "$funcw%d", inplaceVariableNum++);
	currType = NULL;
	FunctionAdd(pos, functionName);
	FunctionStart(pos);
	const char *lastFunc = SetCurrentFunction(NULL);
	CodeInfo::nodeList.push_back(wrapee);
	AddReturnNode(pos);
	SetCurrentFunction(lastFunc);
	FunctionEnd(pos);
	ConvertFunctionToPointer(pos);
}

void HandleNullPointerConversion(const char* pos)
{
	bool swapped = false;
	if(CodeInfo::nodeList[CodeInfo::nodeList.size() - 2]->typeInfo == typeVoid->refType)
	{
		// Swap nodes so that 'void ref' type node will be on top
		NodeZeroOP *tmp = CodeInfo::nodeList[CodeInfo::nodeList.size() - 2];
		CodeInfo::nodeList[CodeInfo::nodeList.size() - 2] = CodeInfo::nodeList.back();
		CodeInfo::nodeList.back() = tmp;
		swapped = true;
	}
	if(CodeInfo::nodeList.back()->typeInfo == typeVoid->refType)
	{
		// Convert 'void ref' to the other operand type
		HandlePointerToObject(pos, CodeInfo::nodeList[CodeInfo::nodeList.size() - 2]->typeInfo);
		// If nodes were swapped before, swap them back
		if(swapped)
		{
			NodeZeroOP *tmp = CodeInfo::nodeList[CodeInfo::nodeList.size() - 2];
			CodeInfo::nodeList[CodeInfo::nodeList.size() - 2] = CodeInfo::nodeList.back();
			CodeInfo::nodeList.back() = tmp;
		}
	}
}

void AddBinaryCommandNode(const char* pos, CmdID id)
{
	CodeInfo::lastKnownStartPos = pos;

	const char *opNames[] = { "+", "-", "*", "/", "**", "%", "<", ">", "<=", ">=", "==", "!=", "<<", ">>", "&", "|", "^", "&&", "||", "^^", "in" };

	if(id == cmdNop)
	{
		AddFunctionCallNode(CodeInfo::lastKnownStartPos, "in", 2);
		return;
	}
	if(id == cmdEqual || id == cmdNEqual)
	{
		HandleNullPointerConversion(pos);

		NodeZeroOP *left = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
		NodeZeroOP *right = CodeInfo::nodeList[CodeInfo::nodeList.size()-1];

		if(right->typeInfo == typeObject && right->typeInfo == left->typeInfo)
		{
			AddFunctionCallNode(pos, id == cmdEqual ? "__rcomp" : "__rncomp", 2);
			return;
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

	NodeNumber *Ad = static_cast<NodeNumber*>(CodeInfo::nodeList[CodeInfo::nodeList.size() - 2]);
	NodeNumber *Bd = static_cast<NodeNumber*>(CodeInfo::nodeList[CodeInfo::nodeList.size() - 1]);

	unsigned int aNodeType = Ad->nodeType;
	unsigned int bNodeType = Bd->nodeType;

	bool protect = (Ad->typeInfo->firstVariable || Bd->typeInfo->firstVariable) && Ad->typeInfo != Bd->typeInfo;
	protect |= Ad->typeInfo->refLevel || Bd->typeInfo->refLevel;

	if(aNodeType == typeNodeNumber && bNodeType == typeNodeNumber && !protect)
	{
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.pop_back();

		// If we have operation between two known numbers, we can optimize code by calculating the result in compile-time
		TypeInfo *aType = Ad->typeInfo;
		TypeInfo *bType = Bd->typeInfo;
		TypeInfo *resType = ChooseBinaryOpResultType(aType, bType);
		if(Ad->typeInfo != resType)
			Ad->ConvertTo(resType);
		if(Bd->typeInfo != resType)
			Bd->ConvertTo(resType);

		bool logicalOp = (id >= cmdLess && id <= cmdNEqual) || (id >= cmdLogAnd && id <= cmdLogXor);
		NodeNumber *Rd = NULL;
		if(resType->stackType == STYPE_DOUBLE)
			Rd = new NodeNumber(optDoOperation<double>(id, Ad->GetDouble(), Bd->GetDouble()), resType);
		else if(resType->stackType == STYPE_LONG)
			Rd = new NodeNumber(optDoOperation<long long>(id, Ad->GetLong(), Bd->GetLong()), resType);
		else if(resType->stackType == STYPE_INT)
			Rd = new NodeNumber(optDoOperation<int>(id, Ad->GetInteger(), Bd->GetInteger()), resType);
		assert(Rd);
		if(logicalOp || (Ad->typeInfo == typeBool && Bd->typeInfo == typeBool && id >= cmdBitAnd && id <= cmdBitXor))
			Rd->ConvertTo(typeBool);
		CodeInfo::nodeList.push_back(Rd);
		return;
	}else if(((aNodeType == typeNodeNumber && Bd->typeInfo->type != TypeInfo::TYPE_COMPLEX) || (bNodeType == typeNodeNumber && Ad->typeInfo->type != TypeInfo::TYPE_COMPLEX)) && !protect){
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

	TypeInfo *condType = CodeInfo::nodeList.back()->typeInfo;
	if((id == cmdLogAnd || id == cmdLogOr) && condType->type == TypeInfo::TYPE_COMPLEX && !(condType->funcType || condType->arrLevel))
	{
		WrapNodeToFunction(pos);
		AddFunctionCallNode(CodeInfo::lastKnownStartPos, opNames[id - cmdAdd], 2);
		return;
	}
	// Handle build-in types such as function pointers and unsized arrays
	if(id == cmdLogAnd || id == cmdLogOr)
	{
		if(condType->funcType || condType->arrLevel)
		{
			AddNullPointer();
			AddBinaryCommandNode(pos, cmdNEqual);
		}
		condType = CodeInfo::nodeList[CodeInfo::nodeList.size() - 2]->typeInfo;
		if(condType->funcType || condType->arrLevel)
		{
			NodeZeroOP *tmp = CodeInfo::nodeList.back();
			CodeInfo::nodeList.pop_back();
			AddNullPointer();
			AddBinaryCommandNode(pos, cmdNEqual);
			CodeInfo::nodeList.push_back(tmp);
		}
	}
	// Optimizations failed, perform operation in run-time
	if(!AddFunctionCallNode(CodeInfo::lastKnownStartPos, opNames[id - cmdAdd], 2, true))
		CodeInfo::nodeList.push_back(new NodeBinaryOp(id));
}

void AddReturnNode(const char* pos, bool yield)
{
	bool localReturn = currDefinedFunc.size() != 0;

	if(yield && !localReturn)
		ThrowError(pos, "ERROR: global yield is not allowed");

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
			ConvertBaseToDerived(pos, currDefinedFunc.back()->retType);
			ConvertDerivedToBase(pos, currDefinedFunc.back()->retType);
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
		if(((expectedType->type == TypeInfo::TYPE_COMPLEX || realRetType->type == TypeInfo::TYPE_COMPLEX || expectedType->firstVariable || realRetType->firstVariable) && expectedType != realRetType) || expectedType->subType != realRetType->subType)
			ThrowError(pos, "ERROR: function returns %s but supposed to return %s", realRetType->GetFullTypeName(), expectedType->GetFullTypeName());
		if(expectedType == typeVoid && realRetType != typeVoid)
			ThrowError(pos, "ERROR: 'void' function returning a value");
		if(expectedType != typeVoid && realRetType == typeVoid)
			ThrowError(pos, "ERROR: function should return %s", expectedType->GetFullTypeName());
		if(yield && currDefinedFunc.back()->type != FunctionInfo::COROUTINE)
			ThrowError(pos, "ERROR: yield can only be used inside a coroutine");
#if defined(NULLC_ENABLE_C_TRANSLATION) || defined(NULLC_LLVM_SUPPORT)
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

void SelectTypeForGeneric(Lexeme* pos, unsigned nodeIndex)
{
	if(CodeInfo::nodeList[nodeIndex]->nodeType == typeNodeFuncDef)
	{
		currType = ((NodeFuncDef*)CodeInfo::nodeList[nodeIndex])->GetFuncInfo()->funcType;
	}else if(CodeInfo::nodeList[nodeIndex]->nodeType == typeNodeExpressionList && ((NodeExpressionList*)CodeInfo::nodeList[nodeIndex])->GetFirstNode()->nodeType == typeNodeFunctionProxy){
		NodeFunctionProxy *fProxy = (NodeFunctionProxy*)((NodeExpressionList*)CodeInfo::nodeList.back())->GetFirstNode();
		Lexeme *tmp = pos;
		bool instanceFailure = false;

		if(!ParseSelectType(&tmp, ALLOW_ARRAY | ALLOW_GENERIC_TYPE | ALLOW_EXTENDED_TYPEOF | ALLOW_AUTO_RETURN_TYPE, fProxy->funcInfo->funcType, &instanceFailure) && !instanceFailure)
			ThrowError(pos->pos, "ERROR: there is no function available that will satisfy the argument");

		if(currType->dependsOnGeneric)
			currType = InstanceGenericFunctionTypeForType(pos->pos, fProxy->funcInfo, currType, bestFuncList.size(), true, false);
	}else if(CodeInfo::nodeList[nodeIndex]->nodeType == typeNodeFunctionProxy){
		HashMap<FunctionInfo*>::Node *func = funcMap.first(((NodeFunctionProxy*)CodeInfo::nodeList[nodeIndex])->funcInfo->nameHash);

		do
		{
			Lexeme *tmp = pos;
			bool instanceFailure = false;
			if(ParseSelectType(&tmp, ALLOW_ARRAY | ALLOW_GENERIC_TYPE | ALLOW_EXTENDED_TYPEOF | ALLOW_AUTO_RETURN_TYPE, func->value->funcType, &instanceFailure) && !instanceFailure)
				break;
			func = funcMap.next(func);
		}while(func);

		if(!func)
			ThrowError(pos->pos, "ERROR: there is no function available that will satisfy the argument");

		if(currType->dependsOnGeneric)
			currType = InstanceGenericFunctionTypeForType(pos->pos, func->value, currType, bestFuncList.size(), true, false);
	}else if(CodeInfo::nodeList[nodeIndex]->typeInfo->arrLevel && CodeInfo::nodeList[nodeIndex]->typeInfo->arrSize != TypeInfo::UNSIZED_ARRAY && CodeInfo::nodeList[nodeIndex]->nodeType != typeNodeZeroOp){
		currType = CodeInfo::GetArrayType(CodeInfo::nodeList[nodeIndex]->typeInfo->subType, TypeInfo::UNSIZED_ARRAY);
	}else{
			currType = CodeInfo::nodeList[nodeIndex]->typeInfo;
	}
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

FunctionInfo* GetCurrentFunction()
{
	return currDefinedFunc.empty() ? NULL : currDefinedFunc.back();
}

VariableInfo* AddVariable(const char* pos, InplaceStr variableName, bool preserveNamespace, bool allowThis, bool allowCollision)
{
	static unsigned thisHash = GetStringHash("this");

	CodeInfo::lastKnownStartPos = pos;

	InplaceStr varName = preserveNamespace ? GetNameInNamespace(variableName) : variableName;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	if(thisHash == hash && !allowThis)
		ThrowError(pos, "ERROR: 'this' is a reserved keyword");

	if(TypeInfo **info = CodeInfo::classMap.find(hash))
		ThrowError(pos, "ERROR: name '%.*s' is already taken for a class", varName.end-varName.begin, varName.begin);

	// Check for variables with the same name in current scope
	if(VariableInfo **info = varMap.find(hash))
	{
		if((*info)->blockDepth >= varInfoTop.size() && !allowCollision)
			ThrowError(pos, "ERROR: name '%.*s' is already taken for a variable in current scope", varName.end-varName.begin, varName.begin);
	}

	// Check for functions with the same name
	CheckCollisionWithFunction(pos, varName, hash, varInfoTop.size());

	// If alignment if explicitly specified or the variable type has a default alignment, align variable address
	if(currAlign != TypeInfo::ALIGNMENT_UNSPECIFIED || (currType && currType->alignBytes != TypeInfo::ALIGNMENT_UNSPECIFIED))
		varTop += GetAlignmentOffset(pos, currAlign != TypeInfo::ALIGNMENT_UNSPECIFIED ? currAlign : currType->alignBytes);

	if(currType && currType->hasFinalizer)
		ThrowError(pos, "ERROR: cannot create '%s' that implements 'finalize' on stack", currType->GetFullTypeName());
	if(currType && !currType->hasFinished && currType != newType)
		ThrowError(pos, "ERROR: type '%s' is not fully defined. You can use '%s ref' or '%s[]' at this point", currType->GetFullTypeName(), currType->GetFullTypeName(), currType->GetFullTypeName());

	CodeInfo::varInfo.push_back(new VariableInfo(currDefinedFunc.size() > 0 ? currDefinedFunc.back() : NULL, varName, hash, varTop, currType, currDefinedFunc.size() == 0));
	varDefined = true;
	CodeInfo::varInfo.back()->blockDepth = varInfoTop.size();
	if(currType)
		varTop += currType->size;
	if(varTop > (1 << 24))
		ThrowError(pos, "ERROR: variable size limit exceeded");
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
	
	if(sizeOfExpr)
	{
		currType = CodeInfo::nodeList.back()->typeInfo;
		CodeInfo::nodeList.pop_back();
	}

	if(!currType->hasFinished)
		ThrowError(pos, "ERROR: cannot take size of a type in definition");

	CodeInfo::nodeList.push_back(new NodeNumber((int)currType->size, typeInt));
}

void GetTypeId(const char* pos)
{
	if(!currType)
		ThrowError(pos, "ERROR: cannot take typeid from auto type");
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
			context = (*info)->name;
		}else{
			char *contextName = AllocateString(fInfo->nameLength + 24);
			GetFunctionHiddenName(contextName, *fInfo);

			unsigned prevBackupSize = 0, prevStackSize = 0;
			NamespaceInfo *lastNS = NULL;
			RestoreNamespaces(false, fInfo->parentNamespace, prevBackupSize, prevStackSize, lastNS);
			InplaceStr fullName = GetNameInNamespace(InplaceStr(contextName));
			RestoreNamespaces(true, fInfo->parentNamespace, prevBackupSize, prevStackSize, lastNS);

			unsigned int contextHash = GetStringHash(fullName.begin);
			info = varMap.find(contextHash);
			context = fullName;
		}
		if(!info)
		{
			CodeInfo::nodeList.push_back(new NodeNumber(0, CodeInfo::GetReferenceType(typeInt)));
		}else{
			NamespaceInfo *lastNS = GetCurrentNamespace();
			SetCurrentNamespace(NULL);
			AddGetAddressNode(pos, context);
			SetCurrentNamespace(lastNS);
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
void AddGetAddressNode(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);
	HashMap<VariableInfo*>::Node *curr = SelectVariableByName(varName);

	if(!curr)
	{
		int fID = -1;
		if(newType)
		{
			// Construct function name in a for of Class::Function
			unsigned int hash = newType->nameHash;
			hash = StringHashContinue(hash, "::");
			hash = StringHashContinue(hash, varName.begin, varName.end);

			fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);
			if(CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
			{
				FunctionInfo *fInfo = CodeInfo::funcInfo[fID];
				GetFunctionContext(pos, fInfo, true);
				fInfo->pure = false;
				CodeInfo::nodeList.push_back(new NodeFunctionProxy(fInfo, pos, false, true));
				return;
			}
			// If a member function is not found, try an accessor
			if(fID == -1)
			{
				hash = StringHashContinue(hash, "$");

				fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);

				if(fID == -1 && newType->genericBase)
				{
					unsigned int hash = newType->genericBase->nameHash;
					hash = StringHashContinue(hash, "::");
					hash = StringHashContinue(hash, varName.begin, varName.end);
					hash = StringHashContinue(hash, "$");
					fID = CodeInfo::FindFunctionByName(hash, CodeInfo::funcInfo.size()-1);
				}
				if(fID != -1)
				{
					AddGetAddressNode(pos, InplaceStr("this", 4));
					CodeInfo::nodeList.push_back(new NodeDereference());
					AddMemberAccessNode(pos, varName);
					return;
				}
			}
		}
		if(fID == -1)
		{
			FunctionSelect f;
			NamespaceSelect(varName, f);
			hash = f.curr ? f.curr->value->nameHash : ~0u;
			fID = f.curr ? f.curr->value->indexInArr : -1;
		}
		if(fID == -1)
			ThrowError(pos, "ERROR: unknown identifier '%.*s'", varName.end-varName.begin, varName.begin);

		if(CodeInfo::FindFunctionByName(hash, fID - 1) != -1)
		{
			CodeInfo::nodeList.push_back(new NodeFunctionProxy(CodeInfo::funcInfo[fID], pos));
			return;
		}
		FunctionInfo *fInfo = CodeInfo::funcInfo[fID];
		if(fInfo->generic)
		{
			CodeInfo::nodeList.push_back(new NodeFunctionProxy(fInfo, pos, true));
			return;
		}
		if(!fInfo->funcType)
			ThrowError(pos, "ERROR: function '%.*s' type is unresolved at this point", varName.end-varName.begin, varName.begin);

		GetFunctionContext(pos, fInfo, true);
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
			if(curr && curr->defaultValue)
			{
				CodeInfo::nodeList.push_back(curr->defaultValue);
				return;
			}
			if(curr && currDefinedFunc.size())
			{
				// Class members are accessed through 'this' pointer
				AddGetAddressNode(pos, InplaceStr("this", 4));
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
	{
		FunctionSelect f;
		NamespaceSelect(InplaceStr(currFunction), f);
		currF = f.curr;
	}
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
			unsigned rating = GetFunctionRating(func->funcType->funcType, currArgument, func->firstParam);
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

				AliasInfo *lastAlias = func->childAlias;

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

					TypeInfo *referenceType = NULL;
					if(argID != currArgument)
					{
						SelectTypeForGeneric(start, nodeOffset + argID);
						referenceType = currType;
					}
					// Set generic function as being in definition so that type aliases will get into functions alias list and will not spill to outer scope
					currDefinedFunc.push_back(func);
					// Flag of instantiation failure
					bool instanceFailure = false;
					if(!ParseSelectType(&start, ALLOW_ARRAY | (referenceType ? ALLOW_GENERIC_TYPE : 0) | ALLOW_EXTENDED_TYPEOF, referenceType, &instanceFailure))
						genericArg = start->type == lex_generic ? !!(start++) : false;
					genericRef = start->type == lex_ref ? !!(start++) : false;
					currDefinedFunc.pop_back();

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
							SelectTypeForGeneric(start, nodeOffset + argID);
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
						
						// If there is a default value
						if(start->type == lex_set)
						{
							start++;
							if(!ParseTernaryExpr(&start))
								assert(0);
							CodeInfo::nodeList.pop_back();
						}
					}else{
						if(genericArg)
							currType = typeGeneric;
					}
				}
				// Remove all added aliases
				while(func->childAlias != lastAlias)
				{
					CodeInfo::classMap.remove(func->childAlias->nameHash, func->childAlias->type);
					func->childAlias = func->childAlias->next;
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
		HashMap<VariableInfo*>::Node *currV = SelectVariableByName(InplaceStr(currFunction));
		while(currV)
		{
			VariableInfo *var = currV->value;
			if(!var->varType->funcType || var->varType->funcType->paramCount <= currArgument)
				break;	// If the first found variable doesn't match, we can't move to the next, because it's hidden by this one
			// Temporarily change function pointer argument count to match current argument count
			unsigned tmpCount = var->varType->funcType->paramCount;
			var->varType->funcType->paramCount = currArgument;
			unsigned rating = GetFunctionRating(var->varType->funcType, currArgument, NULL);
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
		if(!currDefinedFunc.back()->retType)
		{
			currDefinedFunc.back()->retType = node->typeInfo;
			currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back()->retType, currDefinedFunc.back()->firstParam, currDefinedFunc.back()->paramCount);
		}
		curr->prev->next = new NodeReturnOp(true, currDefinedFunc.back()->retType, currDefinedFunc.back(), false);
	}else{
		if(curr->nodeType != typeNodePopOp)
			return;
		NodeZeroOP *node = ((NodePopOp*)curr)->GetFirstNode();
		CodeInfo::nodeList.back() = node;
		if(!currDefinedFunc.back()->retType)
		{
			currDefinedFunc.back()->retType = node->typeInfo;
			currDefinedFunc.back()->funcType = CodeInfo::GetFunctionType(currDefinedFunc.back()->retType, currDefinedFunc.back()->firstParam, currDefinedFunc.back()->paramCount);
		}
		CodeInfo::nodeList.push_back(new NodeReturnOp(true, currDefinedFunc.back()->retType, currDefinedFunc.back(), false));
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
	// Get array type
	TypeInfo *currentType = CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - argumentCount]->typeInfo;
	// Ignore errors only if we have one index and we are indexing array or a pointer to array
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "[]", argumentCount + 1, argumentCount == 1 && (currentType->arrLevel || (currentType->subType && currentType->subType->arrLevel))))
		return;

	bool unifyTwo = false;
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
	// Current type must be a reference to an array
	assert(currentType->refLevel && currentType->subType->arrLevel);

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
	if(CodeInfo::nodeList.back()->argName)
		ThrowError(pos, "ERROR: overloaded [] operator must be supplied to use named function arguments");
#if !defined(NULLC_ENABLE_C_TRANSLATION) && !defined(NULLC_LLVM_SUPPORT)
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
	bool wasAuto = variableInfo->varType == NULL;

	// If type wasn't known until assignment, it means that variable alignment wasn't performed in AddVariable function
	if(!variableInfo->varType)
	{
		if(realCurrType == typeVoid)
			ThrowError(pos, "ERROR: r-value type is 'void'");

		// If alignment if explicitly specified or the variable type has a default alignment, align variable address
		if(currAlign != TypeInfo::ALIGNMENT_UNSPECIFIED || (realCurrType->alignBytes != TypeInfo::ALIGNMENT_UNSPECIFIED))
			varTop += GetAlignmentOffset(pos, currAlign != TypeInfo::ALIGNMENT_UNSPECIFIED ? currAlign : realCurrType->alignBytes);

		variableInfo->pos = varTop;

		variableInfo->varType = realCurrType;
		varTop += realCurrType->size;

		if(variableInfo->varType->hasFinalizer)
			ThrowError(pos, "ERROR: cannot create '%s' that implements 'finalize' on stack", variableInfo->varType->GetFullTypeName());
		if(!variableInfo->varType->hasFinished && variableInfo->varType != newType)
			ThrowError(pos, "ERROR: type '%s' is not fully defined", variableInfo->varType->GetFullTypeName());
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
		if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "$defaultAssign", 2, true))
			return;

		temp = CodeInfo::nodeList[CodeInfo::nodeList.size()-1];
		CodeInfo::nodeList[CodeInfo::nodeList.size()-1] = CodeInfo::nodeList[CodeInfo::nodeList.size()-2];
		CodeInfo::nodeList[CodeInfo::nodeList.size()-2] = temp;
	}

	if(!wasAuto)
	{
		NodeZeroOP *temp = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		// Perform implicit conversions
		ConvertBaseToDerived(pos, realCurrType);
		ConvertDerivedToBase(pos, realCurrType);
		ConvertArrayToUnsized(pos, realCurrType);
		HandlePointerToObject(pos, realCurrType);
		CodeInfo::nodeList.push_back(temp);
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
				unsigned minRatingIndex = SelectBestFunction(bestFuncList.size(), 1, minRating, currentType);
				if(minRating != ~0u)
				{
					// If a function is found and it is a generic function, instance it
					FunctionInfo *fInfo = bestFuncList[minRatingIndex];
					if(fInfo && fInfo->generic)
					{
						CreateGenericFunctionInstance(pos, fInfo, fInfo, ~0u, currentType);
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
	// Call default operator with error suppression
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, "$defaultAssign", 2, true))
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
	ConvertBaseToDerived(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);
	ConvertDerivedToBase(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);
	ConvertArrayToUnsized(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);
	ConvertFunctionToPointer(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);
	HandlePointerToObject(pos, CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo->subType);

	CodeInfo::nodeList.push_back(new NodeVariableSet(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo, 0, true));
}

void AddGetVariableNode(const char* pos, bool forceError)
{
	CodeInfo::lastKnownStartPos = pos;

	TypeInfo *lastType = CodeInfo::nodeList.back()->typeInfo;

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

FunctionInfo* GetAutoRefFunction(const char* pos, const char* funcName, unsigned int callArgCount, bool forFunctionCall, TypeInfo* preferedParent);

void AddMemberAccessNode(const char* pos, InplaceStr varName)
{
	CodeInfo::lastKnownStartPos = pos;

	unsigned int hash = GetStringHash(varName.begin, varName.end);

	bool unifyTwo = false;
	// Get variable type
	TypeInfo *currentType = CodeInfo::nodeList.back()->typeInfo;
	bool virtualCall = CodeInfo::nodeList.back()->nodeType == typeNodeFuncCall && currentType->refLevel == 1 && currentType->subType->firstVariable && currentType->subType->firstVariable->nameHash == extendableVariableName;
	virtualCall |= currentType->refLevel == 2 && currentType->subType->subType->firstVariable && currentType->subType->subType->firstVariable->nameHash == extendableVariableName;

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
		if(currentType == typeObject || virtualCall)
		{
			if(virtualCall)
			{
				CodeInfo::nodeList.push_back(new NodeConvertPtr(typeObject, true));
				AddInplaceVariable(pos);
				AddTwoExpressionNode(CodeInfo::GetReferenceType(typeObject));
			}
			char *funcName = AllocateString(varName.length() + 1);
			SafeSprintf(funcName, varName.length() + 1, "%s", varName.begin);
			GetAutoRefFunction(pos, funcName, 0, false, virtualCall ? currentType : NULL);
			NodeZeroOP *autoRef = CodeInfo::nodeList.back();
			CodeInfo::nodeList.pop_back();
			CodeInfo::nodeList.pop_back();
			CodeInfo::nodeList.push_back(autoRef);
			if(unifyTwo)
				AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
			return;
		}
		// Construct function name in a for of Class::Function
		unsigned int hash = currentType->nameHash;
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, varName.begin, varName.end);

		// Search for it
		HashMap<FunctionInfo*>::Node *func = funcMap.first(hash);
		if(!func)
		{
			// Try calling a "get" function
			int memberNameLength = varName.length();
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
				unsigned minRatingIndex = SelectBestFunction(bestFuncList.size(), 0, minRating);
				if(minRating != ~0u)
				{
					// If a function is found and it is a generic function, instance it
					FunctionInfo *fInfo = bestFuncList[minRatingIndex];
					if(fInfo && fInfo->generic)
					{
						CreateGenericFunctionInstance(pos, fInfo, memberFunc, ~0u, currentType);
						memberFunc->parentClass = currentType;
					}
				}else{
					// Clear function selection
					bestFuncList.clear();
					SelectFunctionsForHash(accessorBaseHash, 0); // Select accessor functions from generic type base
					SelectFunctionsForHash(GetStringHash(memberFuncName), 0); // Select accessor functions from instance class
					// Choose best accessor
					unsigned minRating = ~0u;
					unsigned minRatingIndex = SelectBestFunction(bestFuncList.size(), 0, minRating);
					if(minRating != ~0u)
					{
						// If a function is found and it is a generic function, instance it
						FunctionInfo *fInfo = bestFuncList[minRatingIndex];
						if(fInfo && fInfo->generic)
						{
							CreateGenericFunctionInstance(pos, fInfo, fInfo, ~0u, currentType);
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
		if(curr->defaultValue)
		{
			CodeInfo::nodeList.pop_back();
			CodeInfo::nodeList.push_back(curr->defaultValue);
		}else{
			// Shift pointer to member
#if !defined(NULLC_ENABLE_C_TRANSLATION) && !defined(NULLC_LLVM_SUPPORT)
			if(CodeInfo::nodeList.back()->nodeType == typeNodeGetAddress)
				static_cast<NodeGetAddress*>(CodeInfo::nodeList.back())->ShiftToMember(curr);
			else
#endif
				CodeInfo::nodeList.push_back(new NodeShiftAddress(curr));
			if(currentType->arrLevel || currentType == typeObject || currentType == typeAutoArray)
				CodeInfo::nodeList.push_back(new NodeDereference(NULL, 0, true));
		}
	}else{
		if(memberFunc->generic)
		{
			if(unifyTwo)
				AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
			CodeInfo::nodeList.push_back(new NodeFunctionProxy(memberFunc, pos, false, true));
			return;
		}
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

void AddModifyVariableNode(const char* pos, CmdID cmd, const char* name)
{
	CodeInfo::lastKnownStartPos = pos;

	// Call overloaded operator with error suppression
	if(AddFunctionCallNode(CodeInfo::lastKnownStartPos, name, 2, true))
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
	NamespaceInfo	*lastNS = GetCurrentNamespace();
	SetCurrentNamespace(NULL);
	AddGetAddressNode(pos, InplaceStr(arrName, length));
	SetCurrentNamespace(lastNS);

	// Restore variable creation state
	varDefined = saveVarDefined;
	currType = saveCurrType;
}

void AddInplaceHeapVariable(const char* pos)
{
	NodeZeroOP *value = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	char *arrName = AllocateString(16);
	int length = sprintf(arrName, "$temp%d", inplaceVariableNum++);

	// Save variable creation state
	TypeInfo *saveCurrType = currType;
	bool saveVarDefined = varDefined;

	// Set type to char[N] ref
	currType = CodeInfo::GetReferenceType(value->typeInfo);
	// Add hidden variable
	VariableInfo *varInfo = AddVariable(pos, InplaceStr(arrName, length));

	// Allocate storage for it
	currType = value->typeInfo;
	GetTypeSize(pos, false);
	AddTypeAllocation(pos, false); // $$ Check that the non-array allocation type ID is valid
	AddDefineVariableNode(pos, varInfo, true);
	AddPopNode(pos);

	NamespaceInfo *lastNS = GetCurrentNamespace();
	SetCurrentNamespace(NULL);

	// Place data in heap storage
	AddGetAddressNode(pos, InplaceStr(arrName, length));
	AddGetVariableNode(pos);
	CodeInfo::nodeList.push_back(value);
	AddSetVariableNode(pos);
	AddPopNode(pos);

	// Get pointer to the variable in the heap
	AddGetAddressNode(pos, InplaceStr(arrName, length));
	AddGetVariableNode(pos);

	SetCurrentNamespace(lastNS);

	// Restore variable creation state
	varDefined = saveVarDefined;
	currType = saveCurrType;

	AddTwoExpressionNode(CodeInfo::GetReferenceType(value->typeInfo));
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
			AddInplaceHeapVariable(pos);
			hasImplicitNode = true;
		}else{
			NodeZeroOP	*oldNode = CodeInfo::nodeList.back();
			CodeInfo::nodeList.back() = static_cast<NodeDereference*>(oldNode)->GetFirstNode();
			static_cast<NodeDereference*>(oldNode)->SetFirstNode(NULL);
		}
		
		TypeInfo *inplaceType = CodeInfo::nodeList.back()->typeInfo;
		assert(inplaceType->refLevel == 1);
		assert(inplaceType->subType->arrLevel != 0);

		NodeCreateUnsizedArray *arrayExpr = new NodeCreateUnsizedArray(dstType, new NodeNumber((int)inplaceType->subType->arrSize, typeInt));
		if(hasImplicitNode)
			arrayExpr->AddExtraNode();

		CodeInfo::nodeList.push_back(arrayExpr);

	}else if(nodeType->refLevel == 1 && nodeType->subType->arrSize != TypeInfo::UNSIZED_ARRAY && dstType->subType->subType == nodeType->subType->subType){
		// type[N] ref to type[] ref conversion
		AddGetVariableNode(pos);
		AddInplaceVariable(pos, CodeInfo::GetArrayType(nodeType->subType->subType, TypeInfo::UNSIZED_ARRAY));
		AddExtraNode();
	}
}

unsigned BackupFunctionSelection(unsigned count)
{
	unsigned prevBackupSize = bestFuncListBackup.size();
	if(count)
	{
		bestFuncListBackup.push_back(&bestFuncList[0], count);
		bestFuncRatingBackup.push_back(&bestFuncRating[0], count);
	}
	return prevBackupSize;
}

void RestoreFunctionSelection(unsigned prevBackupSize, unsigned count)
{
	bestFuncList.clear();
	bestFuncRating.clear();

	if(count)
	{
		bestFuncList.push_back(&bestFuncListBackup[prevBackupSize], count);
		bestFuncRating.push_back(&bestFuncRatingBackup[prevBackupSize], count);
	}

	bestFuncListBackup.shrink(prevBackupSize);
	bestFuncRatingBackup.shrink(prevBackupSize);
}

FunctionInfo* InstanceGenericFunctionForType(const char* pos, FunctionInfo *info, TypeInfo *dstPreferred, unsigned count, bool create, bool silentError, NodeZeroOP **funcDefNode)
{
	// There could be function calls in constrain expression, so we backup current function and rating list
	unsigned prevBackupSize = BackupFunctionSelection(count);

	if(!dstPreferred)
	{
		HashMap<FunctionInfo*>::Node *func = funcMap.first(info->nameHash), *funcS = func;
		bestFuncList.clear();
		bestFuncRating.clear();
		do
		{
			bestFuncList.push_back(func->value);
			bestFuncRating.push_back(0);

			if(func->value->genericBase)
				func->value->genericBase->instancedType = NULL;

			func = funcMap.next(func);
		}while(func);

		char	*errPos = errorReport;
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: ambiguity, there is more than one overloaded function available:\r\n");
		ThrowFunctionSelectError(pos, 0, errorReport, errPos, funcS->value->name, 0, bestFuncList.size());
	}
	if(!dstPreferred->funcType)
		ThrowError(pos, "ERROR: cannot select function overload for a type '%s'", dstPreferred->GetFullTypeName());

	bestFuncList.clear();
	// select all functions
	SelectFunctionsForHash(info->nameHash, 0);

	for(unsigned i = 0; i < dstPreferred->funcType->paramCount; i++) // push function argument placeholders
		CodeInfo::nodeList.push_back(new NodeZeroOP(dstPreferred->funcType->paramType[i]));

	unsigned minRating = ~0u;
	unsigned minRatingIndex = SelectBestFunction(bestFuncList.size(), dstPreferred->funcType->paramCount, minRating);

	if(minRating == ~0u)
	{
		if(!silentError)
		{
			char *errPos = errorReport;
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: unable to select function '%s' overload for a type '%s'\r\n", info->name, dstPreferred->GetFullTypeName());
			ThrowFunctionSelectError(pos, minRating, errorReport, errPos, NULL, dstPreferred->funcType->paramCount, bestFuncList.size());
		}
	}

	// If multiple alternatives are available, choose the one that has the matching return type
	unsigned matchingCount = 0;
	unsigned matchingCountWithReturn = 0;
	for(unsigned k = 0; k < bestFuncList.size(); k++)
	{
		if(bestFuncRating[k] == minRating)
		{
			if(bestFuncList[k]->retType == dstPreferred->funcType->retType)
			{
				matchingCountWithReturn++;
				minRatingIndex = k;
			}
			matchingCount++;
		}
	}
	// Check that there is only one matching function
	if((matchingCountWithReturn == 0 && matchingCount > 1) || matchingCountWithReturn > 1)
	{
		minRatingIndex = ~0u;

		if(!silentError)
		{
			char *errPos = errorReport;
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: unable to select function '%s' overload for a type '%s'\r\n", info->name, dstPreferred->GetFullTypeName());
			ThrowFunctionSelectError(pos, minRating, errorReport, errPos, NULL, dstPreferred->funcType->paramCount, bestFuncList.size());
		}
	}

	FunctionInfo *fInfo = minRatingIndex == ~0u ? NULL : bestFuncList[minRatingIndex];
	if(fInfo && fInfo->generic && create)
	{
		NodeZeroOP *extraNode = CreateGenericFunctionInstance(pos, fInfo, fInfo, ~0u);
		if(funcDefNode)
			*funcDefNode = extraNode;
	}

	// Remove function argument placeholders
	for(unsigned i = 0; i < dstPreferred->funcType->paramCount; i++) 
		CodeInfo::nodeList.pop_back();

	// Restore old function list
	RestoreFunctionSelection(prevBackupSize, count);

	return fInfo;
}

TypeInfo* InstanceGenericFunctionTypeForType(const char* pos, FunctionInfo *info, TypeInfo *dstPreferred, unsigned count, bool create, bool silentError, NodeZeroOP **funcDefNode)
{
	FunctionInfo *fInfo = InstanceGenericFunctionForType(pos, info, dstPreferred, count, create, silentError, funcDefNode);

	return fInfo ? fInfo->funcType : NULL;
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

		NodeZeroOP *funcDefAtEnd = NULL;
		FunctionInfo *fTarget = InstanceGenericFunctionForType(pos, fProxy->funcInfo, dstPreferred, bestFuncList.size(), true, false, &funcDefAtEnd);

		// generated generic function instance can be different from the type we wanted to get
		if(dstPreferred != fTarget->funcType)
			ThrowError(pos, "ERROR: cannot convert from '%s' to '%s'", fTarget->funcType->GetFullTypeName(), dstPreferred->GetFullTypeName());

		if(funcDefAtEnd)
		{
			CodeInfo::nodeList.push_back(funcDefAtEnd);
			AddExtraNode();
		}

		// Take an address of a generic function instance
		GetFunctionContext(pos, fTarget, true);
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(fTarget));
		AddExtraNode();
	}else if(CodeInfo::nodeList.back()->nodeType == typeNodeFunctionProxy){ // If it is an unresolved function overload selection
		FunctionInfo *info = ((NodeFunctionProxy*)CodeInfo::nodeList.back())->funcInfo;
		NodeZeroOP *thisNode = ((NodeFunctionProxy*)CodeInfo::nodeList.back())->GetFirstNode();
		CodeInfo::nodeList.pop_back();
		
		FunctionInfo *fInfo = InstanceGenericFunctionForType(pos, info, dstPreferred, bestFuncList.size(), true, false);

		GetFunctionContext(pos, fInfo, !thisNode);
		if(thisNode)
			CodeInfo::nodeList.push_back(thisNode);
		fInfo->pure = false;
		CodeInfo::nodeList.push_back(new NodeFunctionAddress(fInfo));
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
			CodeInfo::nodeList.push_back(new NodeCreateUnsizedArray(dstType, new NodeNumber(0, typeInt)));
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

	// Unboxing
	if(srcType == typeObject && dstType->refLevel == 0)
	{
		CodeInfo::nodeList.push_back(new NodeConvertPtr(CodeInfo::GetReferenceType(dstType)));
		CodeInfo::nodeList.push_back(new NodeDereference());
		return;
	}
	// Boxing
	if(dstType == typeObject && srcType->refLevel == 0)
	{
		if(!AddFunctionCallNode(pos, "duplicate", 1, true))
			ThrowError(pos, "ERROR: failed to convert from '%s' to 'auto ref'", srcType->GetFullTypeName());
		return;
	}
	CodeInfo::nodeList.push_back(new NodeConvertPtr(dstType == typeObject ? typeObject : dstType, dstType == typeObject));
}

void ConvertDerivedToBase(const char* pos, TypeInfo *dstType)
{
	(void)pos;
	// Handle base class pointer = derived class pointer
	if(dstType->refLevel == 1 && CodeInfo::nodeList.back()->typeInfo->refLevel == 1 && CodeInfo::nodeList.back()->typeInfo->subType->parentType)
	{
		TypeInfo *parentType = CodeInfo::nodeList.back()->typeInfo->subType->parentType;
		while(parentType)
		{
			if(dstType->subType == parentType)
			{
				CodeInfo::nodeList.push_back(new NodePointerCast(dstType));
				break;
			}
			parentType = parentType->parentType;
		}
	}
	// Handle base class = derived class
	if(CodeInfo::nodeList.back()->typeInfo->parentType)
	{
		TypeInfo *parentType = CodeInfo::nodeList.back()->typeInfo->parentType;
		while(parentType)
		{
			if(dstType == parentType)
			{
				AddInplaceVariable(pos);
				CodeInfo::nodeList.push_back(new NodePointerCast(CodeInfo::GetReferenceType(dstType)));
				CodeInfo::nodeList.push_back(new NodeDereference());

				AddTwoExpressionNode(CodeInfo::nodeList.back()->typeInfo);
				break;
			}
			parentType = parentType->parentType;
		}
	}
}

void ConvertBaseToDerived(const char* pos, TypeInfo *dstType)
{
	(void)pos;
	// Handle derived class pointer = base class pointer
	if(dstType->refLevel == 1 && CodeInfo::nodeList.back()->typeInfo->refLevel == 1 && dstType->subType->parentType)
	{
		TypeInfo *parentType = dstType->subType->parentType;
		while(parentType)
		{
			if(CodeInfo::nodeList.back()->typeInfo->subType == parentType)
			{
				CodeInfo::nodeList.push_back(new NodePointerCast(CodeInfo::GetReferenceType(typeVoid)));

				// Push base type
				CodeInfo::nodeList.push_back(new NodeZeroOP(CodeInfo::GetReferenceType(dstType->subType)));
				CodeInfo::nodeList.push_back(new NodeConvertPtr(typeObject));
				CodeInfo::nodeList.back()->typeInfo = typeTypeid;
				
				AddFunctionCallNode(pos, "assert_derived_from_base", 2);
				CodeInfo::nodeList.push_back(new NodePointerCast(dstType));

				break;
			}
			parentType = parentType->parentType;
		}
	}
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

	if(currentType == typeShort || currentType == typeChar || currentType == typeBool)
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
			!((realType == typeShort || realType == typeChar || realType == typeBool) && currentType == typeInt) &&
			!((realType == typeShort || realType == typeChar || realType == typeBool || realType == typeInt || realType == typeFloat) && currentType == typeDouble))
				ThrowError(pos, "ERROR: element %d doesn't match the type of element 0 (%s)", arrElementCount - i - 1, currentType->GetFullTypeName());
		if((realType == typeShort || realType == typeChar || realType == typeBool || realType == typeInt) && currentType == typeDouble)
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

	// Special implementation of for each for built-in arrays
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
			if(((NodeFunctionAddress*)getIterator)->funcInfo->type != FunctionInfo::COROUTINE)
				ThrowError(pos, "ERROR: function is not a coroutine");
			// Here we have a node that holds pointer to context imagine that we have a int** that we will dereference two times to get "$jmpOffset_ext"
			CodeInfo::nodeList.push_back(new NodePointerCast(CodeInfo::GetReferenceType(CodeInfo::GetReferenceType(typeInt))));
		}else{
			// If we got the function pointer from a variable, we can't access member variable "$jmpOffset_ext" directly, so we get function context
			CodeInfo::nodeList.push_back(new NodeGetFunctionContext());
		}
		CodeInfo::nodeList.push_back(new NodeGetCoroutineState());

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

void CallAllocationFunction(const char* pos, const char* name)
{
	(void)pos;

	HashMap<FunctionInfo*>::Node *curr = funcMap.first(GetStringHash(name));
	assert(curr);
	FunctionInfo *func = curr->value;
	CodeInfo::nodeList.push_back(new NodeFuncCall(func, func->funcType->funcType));
}

void AddTypeAllocation(const char* pos, bool arrayType)
{
	TypeInfo *elementType = arrayType ? currType->subType : currType;

	if(elementType == typeVoid)
		ThrowError(pos, "ERROR: cannot allocate void objects");

	CodeInfo::nodeList.push_back(new NodeZeroOP(typeInt));
	CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdPushTypeID, elementType->typeIndex));

	if(!arrayType)
	{
		CallAllocationFunction(pos, "__newS");
		CodeInfo::nodeList.back()->typeInfo = CodeInfo::GetReferenceType(currType);
	}else{
		assert(currType->arrSize == TypeInfo::UNSIZED_ARRAY);

		CallAllocationFunction(pos, "__newA");
		CodeInfo::nodeList.back()->typeInfo = currType;
	}
}

void AddDefaultConstructorCall(const char* pos, const char* name)
{
	TypeInfo *type = CodeInfo::nodeList.back()->typeInfo;

	if(const char* tmp = strrchr(name, '.'))
		name = tmp + 1;

	assert(type->subType);
	if(!type->subType->arrLevel)
	{
		AddMemberFunctionCall(pos, name, 0);
		AddPopNode(pos);
		return;
	}

	IncreaseCycleDepth();
	BeginBlock();

	assert(type->refLevel && type->subType->arrLevel);
	type = type->subType->subType;
	assert(!type->refLevel && !type->funcType);

	char *arrName = AllocateString(16);
	int length = sprintf(arrName, "$temp%d", inplaceVariableNum++);
	InplaceStr vName = InplaceStr(arrName, length);

	AddGetVariableNode(pos);
	AddArrayIterator(pos, vName, NULL);

	AddGetAddressNode(pos, vName);
	AddDefaultConstructorCall(pos, name);

	EndBlock();
	AddForEachNode(pos);
	
	NodeZeroOP *last = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	NodeOneOP *wrap = new NodeOneOP();
	wrap->SetFirstNode(last);
	CodeInfo::nodeList.push_back(wrap);
}

NodeZeroOP* PrepareConstructorCall(const char* pos)
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

	return getPointer;
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

const char*	FindConstructorName(TypeInfo* type)
{
	// Remove class arguments
	if(type->genericBase)
		type = type->genericBase;
	if(!type->name)
		return NULL;
	// Remove namespaces
	if(const char* pos = strrchr(type->name, '.'))
		return pos + 1;
	return type->name;
}

bool HasConstructor(TypeInfo* type, unsigned arguments, bool* callDefault)
{
	bestFuncList.clear();

	if(type->refLevel || type->arrLevel || type->funcType)
		return false;

	unsigned funcHash = type->nameHash;
	funcHash = StringHashContinue(funcHash, "::");
	funcHash = StringHashContinue(funcHash, FindConstructorName(type));
	SelectFunctionsForHash(funcHash, 0);

	// For a generic type instance, check if base class has a constructor function
	unsigned funcBaseHash = 0;
	if(type->genericBase)
	{
		funcBaseHash = type->genericBase->nameHash;
		funcBaseHash = StringHashContinue(funcBaseHash, "::");
		funcBaseHash = StringHashContinue(funcBaseHash, FindConstructorName(type));
		SelectFunctionsForHash(funcBaseHash, 0);
	}

	// remove functions with wrong argument count
	for(unsigned i = 0; i < bestFuncList.size(); i++)
	{
		FunctionInfo *curr = bestFuncList[i];

		VariableInfo *param = curr->firstParam;
		for(unsigned int n = 0; n < arguments && param; n++)
			param = param->next;

		if(curr->paramCount != arguments && !(param && param->defaultValue))
		{
			bestFuncList[i] = bestFuncList.back();
			bestFuncList.pop_back();
			i--;
		}
	}

	if(!bestFuncList.size())
	{
		if(callDefault)
			*callDefault = true;
		funcHash = StringHashContinue(funcHash, "$");
		SelectFunctionsForHash(funcHash, 0);
		if(type->genericBase)
		{
			funcBaseHash = StringHashContinue(funcBaseHash, "$");
			SelectFunctionsForHash(funcBaseHash, 0);
		}
	}

	unsigned minRating = ~0u;
	SelectBestFunction(bestFuncList.size(), arguments, minRating, type->genericBase ? type : NULL);
	return minRating != ~0u;
}

bool defineCoroutine = false;
void BeginCoroutine()
{
	defineCoroutine = true;
}

void FunctionAdd(const char* pos, const char* funcName, bool isOperator)
{
	static unsigned int hashFinalizer = GetStringHash("finalize");

	bool functionLocal = false;
	if(newType ? varInfoTop.size() > (newType->definitionDepth + 1) : varInfoTop.size() > 1)
		functionLocal = true;

	funcName = (isOperator || (newType && !functionLocal)) ? funcName : GetNameInNamespace(InplaceStr(funcName)).begin;

	unsigned int funcNameHash = GetStringHash(funcName), origHash = funcNameHash;
	for(unsigned int i = varInfoTop.back().activeVarCnt; i < CodeInfo::varInfo.size(); i++)
	{
		if(CodeInfo::varInfo[i]->nameHash == funcNameHash)
			ThrowError(pos, "ERROR: name '%s' is already taken for a variable in current scope", funcName);
	}

	char *funcNameCopy = (char*)funcName;
	if(newType && !functionLocal)
	{
		funcNameCopy = GetClassFunctionName(newType, funcName);
		funcNameHash = GetStringHash(funcNameCopy);
	}

	CodeInfo::funcInfo.push_back(new FunctionInfo(funcNameCopy, funcNameHash, origHash));
	FunctionInfo* lastFunc = CodeInfo::funcInfo.back();
	lastFunc->parentFunc = currDefinedFunc.size() > 0 ? currDefinedFunc.back() : NULL;

	static unsigned int hashNewS = GetStringHash("__newA");
	static unsigned int hashNewA = GetStringHash("__newS");
	if(funcNameHash == hashNewS || funcNameHash == hashNewA)
	{
		if(funcMap.first(funcNameHash))
			ThrowError(pos, "ERROR: function '%s' is reserved", funcName);
		lastFunc->visible = false;
	}

	if(!isOperator)
		lastFunc->parentNamespace = namespaceStack.size() > 1 ? namespaceStack.back() : NULL;

	AddFunctionToSortedList(lastFunc);

	lastFunc->indexInArr = CodeInfo::funcInfo.size() - 1;
	lastFunc->vTopSize = (unsigned int)varInfoTop.size();
	lastFunc->retType = currType;
	currDefinedFunc.push_back(lastFunc);
	if(newType && !functionLocal)
	{
		if(origHash == hashFinalizer)
			newType->hasFinalizer = true;
		if(newType->nameHash == origHash)
			lastFunc->typeConstructor = true;
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
	if(isOperator)
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
			ThrowError(pos, "ERROR: parameter with name '%.*s' is already defined", info->name.length(), info->name.begin);
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

void FunctionParameterExplicit()
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	lastFunc.lastParam->isExplicit = true;
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
	
	// If types don't match and it it is not built-in basic types or if pointers point to different types
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
		varMap.remove(curr->nameHash, curr);
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
		int length = GetFunctionHiddenName(hiddenHame, lastFunc);
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
		if(func->visible && func->funcType == lastFunc.funcType && func != &lastFunc && !(func->parentClass && func->parentClass->genericInfo && lastFunc.parentClass && lastFunc.parentClass->genericInfo && func->parentClass->genericInfo->aliasCount != lastFunc.parentClass->genericInfo->aliasCount))
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

FunctionInfo* FunctionImplementPrototype(const char* pos, FunctionInfo &lastFunc)
{
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

			// Check explicit generic function list
			bool explicitTypesEqual = true;
			AliasInfo *aliasNew = info->explicitTypes;
			AliasInfo *aliasOld = lastFunc.explicitTypes;
			for(; aliasNew && aliasOld; aliasNew = aliasNew->next, aliasOld = aliasOld->next)
			{
				if(aliasNew->type != aliasOld->type)
					explicitTypesEqual = false;
			}
			// Check that both lists ended at the same time
			if(!!aliasNew != !!aliasOld)
				explicitTypesEqual = false;

			if(paramsEqual && explicitTypesEqual)
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

	return implementedPrototype;
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

	// Implement function prototype if the return type is known
	lastFunc.implementedPrototype = lastFunc.retType ? FunctionImplementPrototype(pos, lastFunc) : NULL;

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
	}else if(lastFunc.funcType){
		length = GetFunctionHiddenName(hiddenHame, lastFunc);
	}else{
		memcpy(hiddenHame, "$context", 9);
		length = 8;
	}
	currType = CodeInfo::GetReferenceType(lastFunc.type == FunctionInfo::THISCALL ? lastFunc.parentClass : typeInt);
	currAlign = 4;
	lastFunc.extraParam = (VariableInfo*)AddVariable(pos, InplaceStr(hiddenHame, length), true, true);

	if(lastFunc.type == FunctionInfo::COROUTINE)
	{
		currType = typeInt;
		VariableInfo *jumpOffset = (VariableInfo*)AddVariable(pos, InplaceStr("$jmpOffset"));
		AddFunctionExternal(&lastFunc, jumpOffset);
	}
	varDefined = false;

	// If this is a constructor
	if(lastFunc.typeConstructor)
	{
		TypeInfo::MemberVariable *curr = newType->firstVariable;
		int exprCount = 0;
		for(; curr; curr = curr->next)
		{
			if(curr->nameHash == extendableVariableName)
			{
				AddGetAddressNode(pos, InplaceStr("this", 4));
				CodeInfo::nodeList.push_back(new NodeDereference());
				AddMemberAccessNode(pos,  InplaceStr(curr->name));
				CodeInfo::nodeList.push_back(new NodeZeroOP(CodeInfo::GetReferenceType(newType)));
				CodeInfo::nodeList.push_back(new NodeConvertPtr(typeObject));
				CodeInfo::nodeList.back()->typeInfo = typeTypeid;
				AddSetVariableNode(CodeInfo::lastKnownStartPos);
				AddPopNode(CodeInfo::lastKnownStartPos);
				exprCount++;
				continue;
			}
			// Handle array types
			TypeInfo *base = curr->type;
			while(base && base->arrLevel && base->arrSize != TypeInfo::UNSIZED_ARRAY) // Unsized arrays are not initialized
				base = base->subType;
			bool callDefault = false;
			bool hasConstructor = base ? HasConstructor(base, 0, &callDefault) : false;
			if(hasConstructor)
			{
				const char *name = base->genericBase ? base->genericBase->name : base->name;
				if(callDefault)
					name = GetDefaultConstructorName(name);
				AddGetAddressNode(pos, InplaceStr("this", 4));
				CodeInfo::nodeList.push_back(new NodeDereference());
				AddMemberAccessNode(pos,  InplaceStr(curr->name));
				AddDefaultConstructorCall(pos, name);
				AddPopNode(pos);
				exprCount++;
			}
		}
		if(!exprCount)
			CodeInfo::nodeList.push_back(new NodeZeroOP());
		while(exprCount-- > 1)
			AddTwoExpressionNode();
	}
}

void FunctionEnd(const char* pos)
{
	FunctionInfo &lastFunc = *currDefinedFunc.back();

	if(lastFunc.retType && lastFunc.retType != typeVoid && !lastFunc.explicitlyReturned)
		ThrowError(pos, "ERROR: function must return a value of type '%s'", lastFunc.retType->GetFullTypeName());

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

	if(!lastFunc.retType)
	{
		lastFunc.retType = typeVoid;
		lastFunc.funcType = CodeInfo::GetFunctionType(lastFunc.retType, lastFunc.firstParam, lastFunc.paramCount);

		lastFunc.implementedPrototype = FunctionImplementPrototype(pos, lastFunc);
	}
	currDefinedFunc.pop_back();

	FunctionInfo *implementedPrototype = lastFunc.implementedPrototype;

	if(!lastFunc.retType->hasFinished && newType != lastFunc.retType)
		ThrowError(pos, "ERROR: type '%s' is not fully defined", lastFunc.retType->GetFullTypeName());

	if(newType && lastFunc.retType != typeVoid)
	{
		unsigned hash = newType->GetFullNameHash();
		hash = StringHashContinue(hash, "::");
		hash = StringHashContinue(hash, newType->GetFullTypeName());
		if(hash == lastFunc.nameHash)
			ThrowError(pos, "ERROR: type constructor return type must be void");
	}

	// Remove aliases defined in a function
	AliasInfo *info = lastFunc.childAlias;
	while(info)
	{
		CodeInfo::classMap.remove(info->nameHash, info->type);
		info = info->next;
	}

	if(lastFunc.typeConstructor)
		AddTwoExpressionNode();

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
		int length = GetFunctionHiddenName(hiddenHame, lastFunc);

		TypeInfo *saveCurrType = currType;
		bool saveVarDefined = varDefined;

		// Save data about currently defined type
		TypeInfo *currentDefinedType = newType;
		unsigned int currentDefinedTypeMethodCount = methodCount;
		// Create closure type
		newType = NULL;
		SetCurrentAlignment(4);

		unsigned bufSize = lastFunc.nameLength + 32;
		char *tempName = AllocateString(bufSize);
		SafeSprintf(tempName, bufSize, "__%s_%d_cls", lastFunc.name, CodeInfo::FindFunctionByPtr(&lastFunc));
		TypeBegin(tempName, tempName + strlen(tempName), false);

		// Add closure elements
		for(FunctionInfo::ExternalInfo *curr = lastFunc.firstExternal; curr; curr = curr->next)
		{
			// Hide coroutine jump offset so that the user will not be able to change it
			if(curr->variable->name == InplaceStr("$jmpOffset"))
			{
				newType->size += NULLC_PTR_SIZE + NULLC_PTR_SIZE + 4 + 4;
				continue;
			}

			unsigned int bufSize = curr->variable->name.length() + 8;

			// Pointer to target variable
			char	*memberName = AllocateString(bufSize);
			SafeSprintf(memberName, bufSize, "%.*s_target", curr->variable->name.length(), curr->variable->name.begin);
			newType->AddMemberVariable(memberName, CodeInfo::GetReferenceType(curr->variable->varType));

			// Reserve space for pointer to the next upvalue and the size of the data
			newType->size += NULLC_PTR_SIZE + 4;

			// Place for a copy of target variable
			memberName = AllocateString(bufSize);
			SafeSprintf(memberName, bufSize, "%.*s_copy", curr->variable->name.length(), curr->variable->name.begin);
			newType->AddMemberVariable(memberName, curr->variable->varType);
		}
		TypeInfo *closureType = newType;

		assert(newType->size % 4 == 0); // resulting type should never require padding

		// Shift new types generated inside up, so that declaration will be in the correct order in C translation
		for(unsigned int i = newType->originalIndex + 1; i < CodeInfo::typeInfo.size(); i++)
			CodeInfo::typeInfo[i]->originalIndex--;
		newType->originalIndex = CodeInfo::typeInfo.size() - 1;
		newType->hasFinished = true;
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

		// Allocate closure in dynamic memory
		CodeInfo::nodeList.push_back(new NodeNumber(int(closureType->size), typeInt));
		CodeInfo::nodeList.push_back(new NodeZeroOP(typeInt));
		CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdPushTypeID, closureType->typeIndex));
		CallAllocationFunction(pos, "__newS");
		CodeInfo::nodeList.back()->typeInfo = CodeInfo::GetReferenceType(closureType);

		assert(closureType->size >= lastFunc.externalSize);

		// Set it to pointer variable
		AddDefineVariableNode(pos, varInfo);

		// Previous closure may not exist if it's the end of a global coroutine definition
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
					ThrowError(pos, "Can't capture variable %.*s", currVar->name.length(), currVar->name.begin);
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

	if(lastFunc.type == FunctionInfo::THISCALL || ((lastFunc.type == FunctionInfo::LOCAL || lastFunc.type == FunctionInfo::COROUTINE) && lastFunc.externalCount != 0))
		lastFunc.contextType = lastFunc.extraParam->varType;

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
	static unsigned int hashLogAnd = GetStringHash("&&");
	static unsigned int hashLogOr = GetStringHash("||");

	FunctionInfo &lastFunc = *currDefinedFunc.back();
	if(lastFunc.nameHash != hashFunc && lastFunc.nameHash != hashIndex && lastFunc.paramCount != 2 && !(lastFunc.paramCount == 1 && (lastFunc.nameHash == hashAdd || lastFunc.nameHash == hashSub || lastFunc.nameHash == hashBitNot || lastFunc.nameHash == hashLogNot)))
		ThrowError(pos, "ERROR: binary operator definition or overload must accept exactly two arguments");
	if((lastFunc.nameHash == hashLogAnd || lastFunc.nameHash == hashLogOr) && !lastFunc.lastParam->varType->funcType)
		ThrowError(pos, "ERROR: && or || operator definition or overload must accept a function returning desired type as the second argument (try %s)", CodeInfo::GetFunctionType(lastFunc.lastParam->varType, lastFunc.lastParam->next, 0)->GetFullTypeName());
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

TypeInfo* GetGenericFunctionRating(FunctionInfo *fInfo, unsigned &newRating, unsigned count)
{
	// There could be function calls in constrain expression, so we backup current function and rating list
	unsigned prevBackupSize = BackupFunctionSelection(count);

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

	// Add aliases from explicit generic type list
	AliasInfo *aliasExplicit = currExplicitTypes;
	AliasInfo *aliasExpected = fInfo->explicitTypes;
	while(aliasExplicit && aliasExpected)
	{
		AliasInfo *info = TypeInfo::CreateAlias(aliasExpected->name, aliasExplicit->type);
		info->next = fInfo->childAlias;
		fInfo->childAlias = info;

		CodeInfo::classMap.insert(info->nameHash, info->type);

		aliasExplicit->name = aliasExpected->name;
		aliasExplicit->nameHash = aliasExpected->nameHash;

		aliasExplicit = aliasExplicit->next;
		aliasExpected = aliasExpected->next;
	}

	// apply resolved argument types and test if it is ok
	unsigned nodeOffset = CodeInfo::nodeList.size() - argumentCount;
	// Move through all the arguments
	VariableInfo *tempListS = NULL, *tempListE = NULL;
	Lexeme *prevArg = NULL; // previous argument type position
	for(unsigned argID = 0; argID < argumentCount; argID++)
	{
		if(argID)
		{
			assert(start->type == lex_comma);
			start++;
		}

		// Get type to which we resolve our generic argument
		Lexeme *temp = start;
		SelectTypeForGeneric(ParseSelectType(&temp, ALLOW_ARRAY | ALLOW_GENERIC_TYPE | ALLOW_EXTENDED_TYPEOF) ? start : prevArg, nodeOffset + argID);
		TypeInfo *referenceType = currType;
		// Flag of instantiation failure
		bool instanceFailure = false;
		// Set generic function as being in definition so that type aliases will get into functions alias list and will not spill to outer scope
		currDefinedFunc.push_back(fInfo);
		Lexeme *oldStart = start;
		// Try to reparse the type
		if(!ParseSelectType(&start, ALLOW_ARRAY | ALLOW_GENERIC_TYPE | ALLOW_EXTENDED_TYPEOF, referenceType, &instanceFailure))
		{
			if(!instanceFailure)
			{
				assert(argID);
				// Try to reparse the type with previous argument type
				oldStart = prevArg;
				if(!ParseSelectType(&prevArg, ALLOW_ARRAY | ALLOW_GENERIC_TYPE | ALLOW_EXTENDED_TYPEOF, referenceType, &instanceFailure))
				{
					newRating = ~0u; // function is not instanced
					break;
				}
			}
		}
		prevArg = oldStart;
		currDefinedFunc.pop_back();
		// Check that reference type and instanced type follow the same suffixes
		if(!instanceFailure)
		{
			TypeInfo *instancedType = currType, *referenceTypeTmp = referenceType;
			while(referenceType->subType && instancedType->subType)
			{
				if(instancedType->refLevel != referenceType->refLevel || instancedType->arrLevel != referenceType->arrLevel)
				{
					newRating = ~0u; // function is not instanced
					break;
				}
				instancedType = instancedType->subType;
				referenceType = referenceType->subType;
			}
			referenceType = referenceTypeTmp;
		}
		if(instanceFailure)
		{
			newRating = ~0u; // function is not instanced
			break;
		}
		// type must be followed by argument name
		assert(start->type == lex_string);

		assert(currType);
		if(currType->dependsOnGeneric)
		{
			for(unsigned int n = 0; n < argumentCount; n++)
				CodeInfo::nodeList.pop_back();
			ThrowError(CodeInfo::lastKnownStartPos, "ERROR: couldn't fully resolve type '%s' for an argument %d of a function '%s'", referenceType->GetFullTypeName(), argID, fInfo->name);
		}

		// Insert variable to a list so that a typeof can be taken from it
		InplaceStr paramName = InplaceStr(start->pos, start->length);

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

		// If there is a default value
		if(start->type == lex_set)
		{
			start++;
			if(!ParseTernaryExpr(&start))
				assert(0);
			CodeInfo::nodeList.pop_back();
		}
	}

	// Remove aliases that were created
	AliasInfo *aliasCurr = fInfo->childAlias;
	while(aliasCurr)
	{
		// Find if there are aliases with the same name
		AliasInfo *aliasNext = aliasCurr->next;
		while(aliasNext)
		{
			if(aliasCurr->nameHash == aliasNext->nameHash && aliasCurr->type != aliasNext->type)
				newRating = ~0u; // function is not instanced
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
	TypeInfo *tmpType = NULL;
	if(newRating != ~0u)
	{
		tmpType = CodeInfo::GetFunctionType(NULL, tempListS, argumentCount);
		newRating = GetFunctionRating(tmpType->funcType, argumentCount, fInfo->firstParam);
	}

	// Remove function arguments
	while(tempListS)
	{
		varMap.remove(tempListS->nameHash, tempListS);
		tempListS = tempListS->next;
	}
	currType = lastType;

	// Restore old function list, because the one we've started with could've been replaced
	RestoreFunctionSelection(prevBackupSize, count);

	return tmpType;
}

unsigned int GetFunctionRating(FunctionType *currFunc, unsigned int callArgCount, VariableInfo *currArgument)
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
			if(expectedType == typeGeneric)
			{
				// generic function argument
				continue;
			}else if(expectedType->dependsOnGeneric){
				// generic function argument that is derivative from generic
				continue;
			}else if(currArgument && currArgument->isExplicit){
				return ~0u; // Argument type must perfectly match
			}else if(expectedType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->arrSize != 0 && paramType->subType == expectedType->subType){
				// array -> class (unsized array)
				fRating += 2;
			}else if(expectedType == typeAutoArray && paramType->arrLevel){
				// array -> auto[]
				fRating += 5;
			}else if(expectedType->refLevel == 1 && paramType->refLevel == 1 && expectedType->subType->arrSize == TypeInfo::UNSIZED_ARRAY && paramType->subType->subType == expectedType->subType->subType){
				// array[N] ref -> array[] -> array[] ref
				fRating += 10;
			}else if(expectedType->refLevel == 1 && paramType->refLevel == 1 && paramType->subType->parentType){
				// derived ref -> base ref
				TypeInfo *parent = paramType->subType->parentType;
				while(parent)
				{
					if(expectedType->subType == parent)
						break;
					parent = parent->parentType;
				}
				if(parent)
					fRating += 5;
				else
					return ~0u;
			}else if(expectedType->refLevel == 1 && paramType->refLevel == 1 && expectedType->subType->parentType){
				// base ref -> derived ref
				TypeInfo *parent = expectedType->subType->parentType;
				while(parent)
				{
					if(paramType->subType == parent)
						break;
					parent = parent->parentType;
				}
				if(parent)
					fRating += 10;
				else
					return ~0u;
			}else if(paramType->parentType){
				// derived -> base
				TypeInfo *parent = paramType->parentType;
				while(parent)
				{
					if(expectedType == parent)
						break;
					parent = parent->parentType;
				}
				if(parent)
					fRating += 5;
				else
					return ~0u;
			}else if(expectedType->funcType != NULL){
				// Function type conversions
				if(nodeType == typeNodeFuncDef && ((NodeFuncDef*)activeNode)->GetFuncInfo()->funcType == expectedType)
					continue;		// Inline function definition doesn't cost anything
				else if(nodeType == typeNodeFunctionProxy && ((NodeFunctionProxy*)activeNode)->HasType(expectedType))
					continue;		// If a set of function overloads has an expected overload, this doesn't cont anything
				else if(nodeType == typeNodeFunctionProxy && InstanceGenericFunctionTypeForType(NULL, ((NodeFunctionProxy*)activeNode)->funcInfo, expectedType, bestFuncList.size(), true, true) == expectedType)
					continue;		// The same
				else if(nodeType == typeNodeExpressionList && ((NodeExpressionList*)activeNode)->GetFirstNode()->nodeType == typeNodeFunctionProxy)
					continue;		// Generic function is expected to have an appropriate instance, but there will be a check after instancing
				else
					return ~0u;		// Otherwise this function is not a match
			}else if(expectedType->refLevel == paramType->refLevel + 1 && expectedType->subType == paramType){
				// type -> type ref
				fRating += 5;
			}else if(expectedType == typeObject && paramType->refLevel){
				// type ref -> auto ref
				fRating += 5;
			}else if(expectedType == typeObject){
				// type -> type ref -> auto ref
				fRating += 10;
			}else if(expectedType->type == TypeInfo::TYPE_COMPLEX || paramType->type == TypeInfo::TYPE_COMPLEX || paramType->type == TypeInfo::TYPE_VOID || expectedType->firstVariable || paramType->firstVariable){
				// If one of types is complex, and they aren't equal, function cannot match
				return ~0u;
			}else if(paramType->subType != expectedType->subType){
				// Pointer or array with a different type inside. Doesn't matter if simple or complex.
				return ~0u;
			}else{
				// Build-in types can convert to each other, but the fact of conversion tells us, that there could be a better suited function
				// type -> type
				fRating += 1;
			}
		}

		if(currArgument)
			currArgument = currArgument->next;
	}
	return fRating;
}

bool PrepareMemberCall(const char* pos, const char* funcName)
{
	TypeInfo *currentType = CodeInfo::nodeList.back()->typeInfo;
	// Implicit conversion of type ref ref to type ref (only for non-extendable types)
	if(currentType->refLevel >= 2)
	{
		if(!(currentType->subType->subType->firstVariable && currentType->subType->subType->firstVariable->nameHash == extendableVariableName && funcName))
		{
			CodeInfo::nodeList.push_back(new NodeDereference());
			currentType = currentType->subType;
		}
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

unsigned GetRatingHelper(FunctionInfo* fInfo, TypeInfo* forcedParentType, unsigned argumentCount, unsigned functionCount)
{
	// Check if there is an explicit type list for generic function instancing
	AliasInfo *provided = currExplicitTypes;
	AliasInfo *expected = fInfo->explicitTypes;

	while(provided && expected)
	{
		if(provided->type != expected->type && expected->type != typeGeneric)
			return ~0u;

		provided = provided->next;
		expected = expected->next;
	}
	// Fail if provided explicit type list is larger than expected explicit type list
	if(provided && !expected)
		return ~0u;

	unsigned rating = GetFunctionRating(fInfo->funcType->funcType, argumentCount, fInfo->firstParam);

	// If we have a positive rating, check that generic function constraints are satisfied
	if(fInfo->generic)
	{
		if(rating != ~0u)
		{
			TypeInfo *parentClass = fInfo->parentClass;
			if(forcedParentType && fInfo->parentClass != forcedParentType)
			{
				// Function may be a child of a generic class with a different number of generic arguments 
				if(forcedParentType->genericBase != fInfo->parentClass)
				{
					rating = ~0u;
					fInfo->generic->instancedType = NULL;
					return rating;
				}

				fInfo->parentClass = forcedParentType;
			}
			TypeInfo *tmpType = GetGenericFunctionRating(fInfo, rating, functionCount);
			fInfo->parentClass = parentClass;

			fInfo->generic->instancedType = tmpType;
		}else{
			fInfo->generic->instancedType = NULL;
		}
	}
	return rating;
}


bool	IsNamedFunctionCall(unsigned argumentCount)
{
	for(unsigned i = CodeInfo::nodeList.size() - argumentCount; i < CodeInfo::nodeList.size(); i++)
	{
		if(CodeInfo::nodeList[i]->argName)
			return true;
	}
	return false;
}

bool	GoodForNamedFunctionCall(unsigned argumentCount, FunctionInfo* func, bool& namedArgCall)
{
	bool	okByNamedArgs = true;
	namedArgCall = false;
	for(unsigned i = 0; i < argumentCount && okByNamedArgs; i++)
	{
		NodeZeroOP *arg = CodeInfo::nodeList[CodeInfo::nodeList.size() - argumentCount + i];
		if(arg->argName)
		{
			namedArgCall = true;

			unsigned argHash = GetStringHash(arg->argName->pos, arg->argName->pos + arg->argName->length);
			bool found = false;
			for(VariableInfo *currArg = func->firstParam; currArg && !found; currArg = currArg->next)
			{
				if(currArg->nameHash == argHash)
					found = true;
			}
			if(!found)
				okByNamedArgs = false;
		}
	}
	return okByNamedArgs;
}

bool	ShuffleArgumentsForNamedFunctionCall(unsigned argumentCount, FunctionInfo* func)
{
	bool okByNamedArgs = true;
	unsigned oldArgCount = argumentCount;

	// Add additional arguments
	for(unsigned i = argumentCount; i < func->paramCount; i++)
		CodeInfo::nodeList.push_back(NULL);
	
	// Remove extra arguments
	if(argumentCount > func->paramCount)
		CodeInfo::nodeList.shrink(CodeInfo::nodeList.size() - (argumentCount - func->paramCount));

	argumentCount = func->paramCount;

	// Find out, how many arguments on stack are function first non-named arguments
	unsigned	fixedCount = 0;
	for(unsigned i = CodeInfo::nodeList.size() - argumentCount; i < CodeInfo::nodeList.size(); i++)
	{
		if(!CodeInfo::nodeList[i] || CodeInfo::nodeList[i]->argName)
			break;
		fixedCount++;
	}

	// Clear all function arguments in the stack
	for(unsigned i = CodeInfo::nodeList.size() - argumentCount + fixedCount; i < CodeInfo::nodeList.size(); i++)
		CodeInfo::nodeList[i] = NULL;

	// Shuffle named arguments in accordance to the real arguments
	unsigned param = CodeInfo::nodeList.size() - argumentCount;
	for(VariableInfo *currArg = func->firstParam; currArg; currArg = currArg->next)
	{
		for(unsigned int i = 0; i < oldArgCount; i++)
		{
			NodeZeroOP *arg = namedArgumentBackup[namedArgumentBackup.size() - oldArgCount + i];
			if(arg && arg->argName)
			{
				unsigned argHash = GetStringHash(arg->argName->pos, arg->argName->pos + arg->argName->length);
				if(argHash == currArg->nameHash)
				{
					// If parameter is already set, situation's looking pretty ugly
					if(CodeInfo::nodeList[param])
						ThrowError(arg->argName->pos, "ERROR: argument '%.*s' value is being defined the second time", currArg->name.length(), currArg->name.begin);
					CodeInfo::nodeList[param] = arg;
				}
			}
		}
		param++;
	}
	// All parameters should be set
	param = CodeInfo::nodeList.size() - argumentCount;
	for(VariableInfo *currArg = func->firstParam; currArg; currArg = currArg->next)
	{
		if(!CodeInfo::nodeList[param])
		{
			if(!currArg->defaultValue)
			{
				okByNamedArgs = false;
				break;
			}
			CodeInfo::nodeList[param] = currArg->defaultValue;
		}
		param++;
	}
	return okByNamedArgs;
}

unsigned SelectBestFunction(unsigned count, unsigned callArgCount, unsigned int &minRating, TypeInfo* forcedParentType, bool hideGenerics)
{
	// Find the best suited function
	bestFuncRating.resize(count);

	unsigned int minGenericRating = ~0u;
	unsigned int minRatingIndex = ~0u, minGenericIndex = ~0u;
	for(unsigned int k = 0; k < count; k++)
	{
		unsigned int argumentCount = callArgCount;

		// Filter functions by named function arguments
		bool namedArgCall = false;
		if(!GoodForNamedFunctionCall(argumentCount, bestFuncList[k], namedArgCall))
		{
			bestFuncRating[k] = ~0u;
			continue;
		}
		// Prepare argument list for named function argument call, if that's the case
		if(namedArgCall)
		{
			// Backup parameters
			namedArgumentBackup.push_back(&CodeInfo::nodeList[CodeInfo::nodeList.size() - argumentCount], argumentCount);

			argumentCount = bestFuncList[k]->paramCount;

			if(!ShuffleArgumentsForNamedFunctionCall(callArgCount, bestFuncList[k]))
			{
				bestFuncRating[k] = ~0u;
			}else{
				bestFuncRating[k] = GetRatingHelper(bestFuncList[k], forcedParentType, argumentCount, count);

				if(bestFuncRating[k] < (bestFuncList[k]->generic ? minGenericRating : minRating))
				{
					(bestFuncList[k]->generic ? minGenericRating : minRating) = bestFuncRating[k];
					(bestFuncList[k]->generic ? minGenericIndex : minRatingIndex) = k;
				}
			}

			// Restore parameters
			CodeInfo::nodeList.shrink(CodeInfo::nodeList.size() - argumentCount);
			argumentCount = callArgCount;
			CodeInfo::nodeList.push_back(&namedArgumentBackup[namedArgumentBackup.size() - argumentCount], argumentCount);
			namedArgumentBackup.shrink(namedArgumentBackup.size() - argumentCount);
			continue;
		}

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
			unsigned prevBackupSize = nodeBackup.size();
			unsigned int redundant = argumentCount - bestFuncList[k]->paramCount;
			if(redundant == ~0u)
			{
				CodeInfo::nodeList.push_back(new NodeZeroOP(typeObjectArray));
			}else if(redundant){
				nodeBackup.push_back(&CodeInfo::nodeList[CodeInfo::nodeList.size() - redundant], redundant);
				CodeInfo::nodeList.shrink(CodeInfo::nodeList.size() - redundant);
			}

			// Change things in a way that this function will be selected (lie about real argument count and match last argument type)
			TypeInfo *nodeType = CodeInfo::nodeList.back()->typeInfo;
			CodeInfo::nodeList.back()->typeInfo = CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY);
			bestFuncRating[k] = GetRatingHelper(bestFuncList[k], forcedParentType, bestFuncList[k]->paramCount, count);
			CodeInfo::nodeList.back()->typeInfo = nodeType;

			if(redundant == ~0u)
			{
				CodeInfo::nodeList.pop_back();
			}else if(redundant){
				CodeInfo::nodeList.push_back(&nodeBackup[prevBackupSize], redundant);
				nodeBackup.shrink(prevBackupSize);
			}
			if(bestFuncRating[k] != ~0u)
				bestFuncRating[k] += 10 + (redundant == ~0u ? 5 : redundant * 5);	// Cost of variable arguments function
		}else{
			bestFuncRating[k] = GetRatingHelper(bestFuncList[k], forcedParentType, argumentCount, count);
		}
		if(bestFuncRating[k] < (bestFuncList[k]->generic ? minGenericRating : minRating))
		{
			(bestFuncList[k]->generic ? minGenericRating : minRating) = bestFuncRating[k];
			(bestFuncList[k]->generic ? minGenericIndex : minRatingIndex) = k;
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
		if(hideGenerics)
		{
			// Otherwise, go as far as disabling all generic functions from selection
			for(unsigned int k = 0; k < count; k++)
				if(bestFuncList[k]->generic)
					bestFuncRating[k] = ~0u;
		}
	}
	return minRatingIndex;
}

FunctionInfo* GetAutoRefFunction(const char* pos, const char* funcName, unsigned int callArgCount, bool forFunctionCall, TypeInfo* preferedParent)
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
		if(!(func->nameHashOrig == fHash && func->parentClass && func->visible && !((func->address & 0x80000000) && (func->address != -1)) && func->funcType))
			continue;
		TypeInfo *copy = preferedParent;
		if(copy)
		{
			do
			{
				if(copy == func->parentClass)
					break;
				copy = copy->parentType;
			}while(copy);
			if(!copy)
				continue;
		}
		bestFuncList.push_back(func);
	}
	unsigned int count = bestFuncList.size();
	if(count == 0)
		ThrowError(pos, "ERROR: function '%s' is undefined in any of existing classes", funcName);

	// Find best function fit
	unsigned minRating = ~0u;
	unsigned minRatingIndex = SelectBestFunction(count, callArgCount, minRating, NULL, false);
	// If this is not for a function call, but for a function pointer retrieval, mark all functions as perfect except for generic ones
	if(!forFunctionCall)
	{
		minRating = ~0u;
		minRatingIndex = ~0u;
		for(unsigned k = 0; k < count; k++)
		{
			bestFuncRating[k] = bestFuncList[k]->generic ? ~0u : 0;
			if(!bestFuncList[k]->generic)
			{
				minRating = 0;
				minRatingIndex = k;
			}
		}
	}
	if(minRating == ~0u)
	{
		char	*errPos = errorReport;
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: none of the member ::%s functions can handle the supplied parameter list without conversions\r\n", funcName);
		ThrowFunctionSelectError(pos, minRating, errorReport, errPos, funcName, callArgCount, count);
	}
	FunctionInfo *fInfo = bestFuncList[minRatingIndex];

	// Check, is there are more than one function, that share the same rating
	for(unsigned k = 0; k < count; k++)
	{
		FunctionInfo *fAlternative = bestFuncList[k];
		// If the other function has a different type and the same rating and it is not a generic function (or it actually is, in which case it cannot come from the same parent class)
		if(k != minRatingIndex && bestFuncRating[k] == minRating && fAlternative->funcType != fInfo->funcType && (!fAlternative->generic || (fInfo->generic && fAlternative->parentClass == fInfo->parentClass)))
		{
			char	*errPos = errorReport;
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE, "ERROR: ambiguity, there is more than one overloaded function available for the call:\r\n");
			ThrowFunctionSelectError(pos, minRating, errorReport, errPos, funcName, callArgCount, count);
		}
	}

	// Backup selected functions
	unsigned prevBackupSize = BackupFunctionSelection(count);

	// Exclude generic functions that are already instantiated
	for(unsigned i = 0; i < count; i++)
	{
		if(bestFuncRatingBackup[prevBackupSize + i] != minRating || !bestFuncListBackup[prevBackupSize + i]->genericBase)
			continue;

		for(unsigned k = 0; k < count; k++)
		{
			if(i == k)
				continue;
			if(bestFuncListBackup[prevBackupSize + i]->genericBase->parent == bestFuncListBackup[prevBackupSize + k])
				bestFuncRatingBackup[prevBackupSize + k] = ~0u;
		}
	}

	// Generate generic function instances
	for(unsigned i = 0; i < count; i++)
	{
		if(bestFuncRatingBackup[prevBackupSize + i] != minRating || !bestFuncListBackup[prevBackupSize + i]->generic)
			continue;

		fInfo = bestFuncListBackup[prevBackupSize + i];
		CreateGenericFunctionInstance(pos, fInfo, fInfo, callArgCount, fInfo->parentClass);
		bestFuncListBackup[prevBackupSize + i] = fInfo;
	}

	// Restore old function list, because the one we've started with could've been replaced
	RestoreFunctionSelection(prevBackupSize, count);

	// Find the most specialized function for extendable member function call
	bool found = false;
	while(preferedParent && !found)
	{
		for(unsigned i = 0; i < count && !found; i++)
		{
			if(bestFuncRating[i] != minRating)
				continue;
			if(preferedParent == bestFuncList[i]->parentClass)
			{
				fInfo = bestFuncList[i];
				found = true;
			}
		}
		preferedParent = preferedParent->parentType;
	}

	// Get function type
	TypeInfo *fType = fInfo->funcType;

	unsigned int lenStr = 5 + (int)strlen(funcName) + 1 + 32;
	char *vtblName = AllocateString(lenStr);
	SafeSprintf(vtblName, lenStr, "$vtbl%010u%s", fType->GetFullNameHash(), funcName);
	unsigned int hash = GetStringHash(vtblName);
	VariableInfo *target = vtblList;
	while(target && target->nameHash != hash)
		target = target->next;

	// If vtbl cannot be found, create it
	if(!target)
	{
		// Create array of function pointers (function pointer is an integer index)
		VariableInfo *vInfo = new VariableInfo(0, InplaceStr(vtblName), hash, 500000, CodeInfo::GetArrayType(typeFunction, TypeInfo::UNSIZED_ARRAY), true);
		vInfo->next = vtblList;
		vInfo->prev = (VariableInfo*)fType;	// $$ not type safe at all
		target = vtblList = vInfo;
		newVtblCount++;
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
	HashMap<FunctionInfo*>::Node *curr = funcMap.first(GetStringHash(forFunctionCall ? "__redirect" : "__redirect_ptr"));
	assert(curr);
	(void)curr;
	// Call redirection function
	AddFunctionCallNode(pos, forFunctionCall ? "__redirect" : "__redirect_ptr", 2);
	// Rename return function type
	CodeInfo::nodeList.back()->typeInfo = fType;

	return fInfo;
}

bool AddMemberFunctionCall(const char* pos, const char* funcName, unsigned int callArgCount, bool silent)
{
	// Check if type has any member functions
	NodeZeroOP *currentNode = CodeInfo::nodeList[CodeInfo::nodeList.size() - callArgCount - 1];
	TypeInfo *currentType = CodeInfo::nodeList[CodeInfo::nodeList.size() - callArgCount - 1]->typeInfo;

	// For extendable type pointers, call member function according to type
	bool virtualCall = currentType->refLevel == 2 && currentType->subType->subType->firstVariable && currentType->subType->subType->firstVariable->nameHash == extendableVariableName;
	virtualCall |= currentNode->nodeType == typeNodeFuncCall && currentType->refLevel == 1 && currentType->subType->firstVariable && currentType->subType->firstVariable->nameHash == extendableVariableName;
	if(virtualCall)
	{
		CodeInfo::nodeList.push_back(currentNode);
		if(currentType->refLevel == 2)
			CodeInfo::nodeList.push_back(new NodeDereference());
		currentType = CodeInfo::nodeList.back()->typeInfo->subType;

		CodeInfo::nodeList.push_back(new NodeConvertPtr(typeObject, true));
		AddInplaceVariable(pos);
		AddTwoExpressionNode(CodeInfo::GetReferenceType(typeObject));

		CodeInfo::nodeList[CodeInfo::nodeList.size() - callArgCount - 2] = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
	}

	// For auto ref types, we redirect call to a target type
	if(currentType == CodeInfo::GetReferenceType(typeObject) || virtualCall)
	{
		FunctionInfo *fInfo = GetAutoRefFunction(pos, funcName, callArgCount, true, virtualCall ? currentType : NULL);
		// Call target function
		AddFunctionCallNode(pos, NULL, callArgCount, false, virtualCall ? fInfo : NULL);
		NodeZeroOP *autoRef = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.pop_back();
		CodeInfo::nodeList.push_back(autoRef);
		return true;
	}
	CheckForImmutable(currentType, pos);
	
	// Go through the inheritance tree
	TypeInfo *type = currentType->subType;
	
	char *memberFuncName = NULL;
	do
	{
		// Construct name in a form of Class::Function
		memberFuncName = GetClassFunctionName(type, funcName);
		bestFuncList.clear();
		SelectFunctionsForHash(GetStringHash(memberFuncName), 0);
		type = type->parentType;
	}while(type && !bestFuncList.size());
	if(!bestFuncList.size())
		memberFuncName = GetClassFunctionName(currentType->subType, funcName);

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
		unsigned minRatingIndex = SelectBestFunction(bestFuncList.size(), callArgCount, minRating, currentType->subType);
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
			CreateGenericFunctionInstance(pos, fInfo, fInfo, ~0u, currentType->subType);
			fInfo->parentClass = currentType->subType;
		}
	}
	// Call it
	return AddFunctionCallNode(pos, memberFuncName, callArgCount, silent, NULL, currentType->subType);
}

unsigned PrintArgumentName(NodeZeroOP* activeNode, char* pos, unsigned limit)
{
	const char* start = pos;
	if(activeNode->nodeType == typeNodeFunctionProxy)
	{
		int fID = CodeInfo::funcInfo.size(), fCount = 0;
		while((fID = CodeInfo::FindFunctionByName(((NodeFunctionProxy*)activeNode)->funcInfo->nameHash, fID - 1)) != -1)
			pos += SafeSprintf(pos, limit - int(pos - start), "%s%s", fCount++ != 0 ? " or " : "", CodeInfo::funcInfo[fID]->funcType->GetFullTypeName());
	}else if(activeNode->nodeType == typeNodeExpressionList && ((NodeExpressionList*)activeNode)->GetFirstNode()->nodeType == typeNodeFunctionProxy){
		pos += SafeSprintf(pos, limit - int(pos - start), "`function`");
	}else{
		TypeInfo *nodeType = activeNode->nodeType == typeNodeFuncDef ? ((NodeFuncDef*)activeNode)->GetFuncInfo()->funcType : activeNode->typeInfo;
		pos += SafeSprintf(pos, limit - int(pos - start), "%s", nodeType->GetFullTypeName());
	}
	return unsigned(pos - start);
}

void ThrowFunctionSelectError(const char* pos, unsigned minRating, char* errorReport, char* errPos, const char* funcName, unsigned callArgCount, unsigned count)
{
	if(funcName)
	{
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "  %s(", funcName);
		for(unsigned int n = 0; n < callArgCount; n++)
		{
			if(n != 0)
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ", ");
			errPos += PrintArgumentName(CodeInfo::nodeList[CodeInfo::nodeList.size()-callArgCount+n], errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport));
		}
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")\r\n");
	}
	errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), minRating == ~0u ? " the only available are:\r\n" : " candidates are:\r\n");
	for(unsigned int n = 0; n < count; n++)
	{
		if(bestFuncRating[n] != minRating)
			continue;
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "  %s %s(", bestFuncList[n]->retType ? bestFuncList[n]->retType->GetFullTypeName() : "auto", bestFuncList[n]->name);
		for(VariableInfo *curr = bestFuncList[n]->firstParam; curr; curr = curr->next)
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s%s", curr->isExplicit ? "explicit " : "", curr->varType->GetFullTypeName(), curr != bestFuncList[n]->lastParam ? ", " : "");
		if(bestFuncList[n]->generic)
		{
			if(!bestFuncList[n]->generic->instancedType)
			{
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ") (wasn't instanced here");
			}else{
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ") instanced to\r\n    %s(", bestFuncList[n]->name);
				FunctionType *tmpType = bestFuncList[n]->generic->instancedType->funcType;
				VariableInfo *curr = bestFuncList[n]->firstParam;
				for(unsigned c = 0; c < tmpType->paramCount; c++, curr = curr->next)
					errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s%s", curr && curr->isExplicit ? "explicit " : "", tmpType->paramType[c]->GetFullTypeName(), (c + 1) != tmpType->paramCount ? ", " : "");
			}
		}
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")\r\n");
	}
	CodeInfo::lastError = CompilerError(errorReport, pos);
	longjmp(CodeInfo::errorHandler, 1);
}

void RestoreNamespaceStack(NamespaceInfo* info)
{
	if(!info)
		return;
	if(info->parent)
		RestoreNamespaceStack(info->parent);
	namespaceStack.push_back(info);
}
void RestoreNamespaces(bool undo, NamespaceInfo *parent, unsigned& prevBackupSize, unsigned& prevStackSize, NamespaceInfo*& lastNS)
{
	if(!undo)
	{
		prevBackupSize = namespaceBackup.size();
		prevStackSize = namespaceStack.size();
		if(namespaceStack.size() > 1)
			namespaceBackup.push_back(&namespaceStack[1], namespaceStack.size() - 1);
		namespaceStack.shrink(1);
		RestoreNamespaceStack(parent);
		lastNS = currNamespace;
		currNamespace = NULL;
	}else{
		namespaceStack.shrink(1);
		if(prevStackSize > 1)
			namespaceStack.push_back(&namespaceBackup[prevBackupSize], prevStackSize - 1);
		namespaceBackup.shrink(prevBackupSize);
		currNamespace = lastNS;
	}
}

// Function expects that nodes with argument types are on top of nodeList
// Return node with function definition if there was one
NodeZeroOP* CreateGenericFunctionInstance(const char* pos, FunctionInfo* fInfo, FunctionInfo*& fResult, unsigned callArgCount, TypeInfo* forcedParentType)
{
	if(instanceDepth++ > NULLC_MAX_GENERIC_INSTANCE_DEPTH)
		ThrowError(pos, "ERROR: reached maximum generic function instance depth (%d)", NULLC_MAX_GENERIC_INSTANCE_DEPTH);
	// Get ID of the function that will be created
	unsigned funcID = CodeInfo::funcInfo.size();

	// Backup namespace state and restore it to function definition state
	unsigned prevBackupSize = 0, prevStackSize = 0;
	NamespaceInfo *lastNS = NULL;
	RestoreNamespaces(false, fInfo->parentNamespace, prevBackupSize, prevStackSize, lastNS);

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
	const char *name = fInfo->name;
	if(forcedParentType || fInfo->parentClass)
	{
		assert(strchr(fInfo->name, ':'));
		name = strchr(fInfo->name, ':') + 2;
	}
	if(const char* pos = strrchr(name, '.'))
		name = pos + 1;
	FunctionAdd(pos, name);

	FunctionInfo *functionInstance = CodeInfo::funcInfo.back();

	if(callArgCount != ~0u && callArgCount < fType->paramCount)
	{
		// Move to the last parameter
		VariableInfo *param = fInfo->firstParam;
		for(unsigned int i = 0; i < callArgCount; i++)
			param = param->next;
		// While there are default values, put them
		while(param && param->defaultValue)
		{
			CodeInfo::nodeList.push_back(param->defaultValue);
			param = param->next;
		}
	}

	// New function type is equal to generic function type no matter where we create an instance of it
	functionInstance->type = fInfo->type;
	functionInstance->parentClass = forcedParentType ? forcedParentType : fInfo->parentClass;
	functionInstance->visible = true;

	// Get aliases defined in a base function argument list
	AliasInfo *aliasFromParent = fInfo->childAlias;
	while(aliasFromParent)
	{
		CodeInfo::classMap.insert(aliasFromParent->nameHash, aliasFromParent->type);
		aliasFromParent = aliasFromParent->next;
	}

	// Take alias list from generic function base into generic function instance
	functionInstance->childAlias = fInfo->childAlias;
	
	// Reset generic function base alias list
	fInfo->childAlias = NULL;

	// Add explicit generic type list
	AliasInfo *aliasExplicit = currExplicitTypes;
	AliasInfo *aliasExpected = fInfo->explicitTypes;
	AliasInfo *aliasCurr = NULL;
	while(aliasExplicit && aliasExpected)
	{
		AliasInfo *info = TypeInfo::CreateAlias(aliasExpected->name, aliasExplicit->type);
		info->next = functionInstance->childAlias;
		functionInstance->childAlias = info;

		CodeInfo::classMap.insert(info->nameHash, info->type);

		info = TypeInfo::CreateAlias(aliasExpected->name, aliasExplicit->type);

		if(!functionInstance->explicitTypes)
			functionInstance->explicitTypes = aliasCurr = info;
		else
			aliasCurr->next = info;
		aliasCurr = info;

		aliasExplicit = aliasExplicit->next;
		aliasExpected = aliasExpected->next;
	}

	AliasInfo *aliasParent = functionInstance->childAlias;

	Lexeme *start = CodeInfo::lexStart + fInfo->generic->start;
	CodeInfo::lastKnownStartPos = NULL;

	unsigned argOffset = CodeInfo::nodeList.size() - fType->paramCount;

	NodeZeroOP *funcDefAtEnd = NULL;

	// Extra argument offset for correct node selection for type inference in variable argument functions
	int argumentOffset = callArgCount != ~0u && callArgCount > fType->paramCount ? callArgCount - fType->paramCount : 0;

	jmp_buf oldHandler;
	memcpy(oldHandler, CodeInfo::errorHandler, sizeof(jmp_buf));
	if(!setjmp(CodeInfo::errorHandler))
	{
		ParseFunctionVariables(&start, CodeInfo::nodeList.size() - fType->paramCount - argumentOffset + 1);
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

		char *errPos = errorReport;
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "ERROR: while instantiating generic function %s(", functionInstance->name);
		for(unsigned i = 0; i < fType->paramCount; i++)
			errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s%s", i == 0 ? "" : ", ", fType->paramType[i]->GetFullTypeName());
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")\r\n\tusing argument vector (");
		for(unsigned i = 0; i < fType->paramCount; i++)
		{
			if(i != 0)
				errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ", ");
			errPos += PrintArgumentName(CodeInfo::nodeList[argOffset + i], errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport));
		}
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), ")\r\n");
		errPos += SafeSprintf(errPos, NULLC_ERROR_BUFFER_SIZE - int(errPos - errorReport), "%s", CodeInfo::lastError.GetErrorString());
		if(errPos[-2] == '\r' && errPos[-1] == '\n')
			errPos -= 2;
		*errPos++ = 0;
		CodeInfo::lastError = CompilerError(errorReport, pos);
		longjmp(CodeInfo::errorHandler, 1);
	}
	memcpy(CodeInfo::errorHandler, oldHandler, sizeof(jmp_buf));

	if(callArgCount != ~0u && callArgCount < fType->paramCount)
	{
		for(unsigned i = callArgCount; i < fType->paramCount; i++)
			CodeInfo::nodeList.pop_back();
	}

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
			int length = GetFunctionHiddenName(hiddenHame, lastFunc);
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

	// Restore old namespace stack
	RestoreNamespaces(true, fInfo->parentNamespace, prevBackupSize, prevStackSize, lastNS);

	instanceDepth--;
	return funcDefAtEnd;
}

bool AddFunctionCallNode(const char* pos, const char* funcName, unsigned int callArgCount, bool silent, FunctionInfo* templateFunc, TypeInfo *forcedParentType)
{
	unsigned int funcNameHash = ~0u;

	FunctionInfo	*fInfo = NULL;
	FunctionType	*fType = NULL;

	NodeZeroOP	*funcAddr = NULL;

	// Check if there is a type with the same name
	TypeInfo *info = funcName ? SelectTypeByName(InplaceStr(funcName)) : NULL;

	// If we are inside member function, transform function name to a member function name (they have priority over global functions)
	// But it's important not to do it if we have function namespace name or it is a constructor name
	if(newType && funcName && !currNamespace && !info)
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
			AddGetAddressNode(pos, InplaceStr("this", 4));
			CodeInfo::nodeList.push_back(new NodeDereference());
			// "this" pointer will be passed as extra parameter
			funcAddr = CodeInfo::nodeList.back();
			CodeInfo::nodeList.pop_back();
		}
	}
	// Handle type(auto ref) -> type, if no user function is defined.
	if(!silent && callArgCount == 1 && CodeInfo::nodeList.back()->typeInfo == typeObject && funcName)
	{
		// Find class by name
		TypeInfo *autoRefToType = SelectTypeByName(InplaceStr(funcName));
		// If class wasn't found, try all other types
		for(unsigned int i = 0; i < CodeInfo::typeInfo.size() && !autoRefToType; i++)
		{
			if(CodeInfo::typeInfo[i]->GetFullNameHash() == GetStringHash(funcName))
				autoRefToType = CodeInfo::typeInfo[i];
		}
		// If a type was found
		if(autoRefToType)
		{
			// Call user function
			if(AddFunctionCallNode(pos, funcName, 1, true))
				return true;
			// If unsuccessful, perform built-in conversion
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
		HashMap<VariableInfo*>::Node *info = SelectVariableByName(InplaceStr(funcName));
		if(info)
		{
			vInfo = info->value;
			scope = vInfo->blockDepth;
		}
	}

	// Find all functions with given name
	bestFuncList.clear();
	if(funcName)
	{
		if(funcAddr)
		{
			SelectFunctionsForHash(funcNameHash, scope);
		}else{
			FunctionSelectCall f(scope);
			NamespaceSelect(InplaceStr(funcName), f);
		}
	}
	unsigned int count = bestFuncList.size();
	
	// Reset current namespace after this point, it's already been used up
	NamespaceInfo *lastNS = currNamespace;
	currNamespace = NULL;
	// If no functions are found, function name is a type name and a type has member constructors
	if(count == 0 && info)
	{
		if(info->genericInfo)
			ThrowError(pos, "ERROR: generic type arguments in <> are not found after constructor name");

		bool hasConstructor = HasConstructor(info, callArgCount);

		if(hasConstructor || callArgCount == 0)
		{
			// Create temporary variable name
			char *arrName = AllocateString(16);
			int length = sprintf(arrName, "$temp%d", inplaceVariableNum++);
			TypeInfo *saveCurrType = currType; // Save type in definition
			currType = info;
			VariableInfo *varInfo = AddVariable(pos, InplaceStr(arrName, length)); // Create temporary variable
			AddGetAddressNode(pos, InplaceStr(arrName, length)); // Get address
			// Call constructor if it's available
			if(hasConstructor)
			{
				for(unsigned k = 0; k < callArgCount; k++) // Move address before arguments
				{
					NodeZeroOP *temp = CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - k];
					CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - k] = CodeInfo::nodeList[CodeInfo::nodeList.size() - 2 - k];
					CodeInfo::nodeList[CodeInfo::nodeList.size() - 2 - k] = temp;
				}
				AddMemberFunctionCall(pos, FindConstructorName(info), callArgCount); // Call member constructor
				AddPopNode(pos); // Remove result
				CodeInfo::nodeList.push_back(new NodeGetAddress(varInfo, varInfo->pos, varInfo->varType)); // Get address
				AddGetVariableNode(pos); // Dereference
				AddTwoExpressionNode(info); // Pack two nodes together
			}else{
				// Imitate default constructor call
				AddGetVariableNode(pos, true);
			}
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
		unsigned minRatingIndex = SelectBestFunction(count, callArgCount, minRating, forcedParentType);
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
		currNamespace = lastNS;
		// If we have variable name, get it
		if(funcName)
			AddGetAddressNode(pos, InplaceStr(funcName, (int)strlen(funcName)));
		currNamespace = NULL;
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
			if(!AddFunctionCallNode(pos, "()", callArgCount + 1, true))
			{
				if(bestFuncList.size() == 0)
					ThrowError(pos, "ERROR: operator '()' accepting %d argument(s) is undefined for a class '%s'", callArgCount, CodeInfo::nodeList[CodeInfo::nodeList.size() - callArgCount - 1]->typeInfo->GetFullTypeName());
				else
					AddFunctionCallNode(pos, "()", callArgCount + 1);
			}
			return true;
		}
		unsigned int bestRating = ~0u;
		if(templateFunc)
		{
			bestFuncList.clear();
			bestFuncList.push_back(templateFunc);
			SelectBestFunction(1, callArgCount, bestRating);
			if(bestRating != ~0u)
				fInfo = templateFunc;
		}else if(callArgCount >= (fType->paramCount-1) && fType->paramCount && fType->paramType[fType->paramCount-1] == typeObjectArray &&
			!(callArgCount == fType->paramCount && CodeInfo::nodeList.back()->typeInfo == typeObjectArray)){
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
			bestRating = GetFunctionRating(fType, fType->paramCount, fInfo ? fInfo->firstParam : NULL);
			if(redundant == ~0u)
				CodeInfo::nodeList.pop_back();
			else
				CodeInfo::nodeList.resize(CodeInfo::nodeList.size() + redundant);
			lastType = CodeInfo::GetArrayType(typeObject, TypeInfo::UNSIZED_ARRAY);
		}else{
			bestRating = GetFunctionRating(fType, callArgCount, fInfo ? fInfo->firstParam : NULL);
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

	// Check for named function argument call
	bool namedArgCall = false;
	if(fInfo)
		GoodForNamedFunctionCall(callArgCount, fInfo, namedArgCall);
	else if(IsNamedFunctionCall(callArgCount))
		ThrowError(pos, "ERROR: function argument names are unknown at this point");
	// Prepare argument list for named function argument call, if that's the case
	if(namedArgCall)
	{ 
		namedArgumentBackup.push_back(&CodeInfo::nodeList[CodeInfo::nodeList.size() - callArgCount], callArgCount);

		if(!ShuffleArgumentsForNamedFunctionCall(callArgCount, fInfo))
			ThrowError(pos, "ERROR: internal compiler error (named argument call)");

		namedArgumentBackup.shrink(namedArgumentBackup.size() - callArgCount);
		callArgCount = fInfo->paramCount;
	}

	NodeZeroOP *funcDefAtEnd = NULL;
	if(fInfo && fInfo->generic)
	{
		funcDefAtEnd = CreateGenericFunctionInstance(pos, fInfo, fInfo, callArgCount, forcedParentType);
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

	// Template function is only needed to support default function arguments and named argument function call
	if(templateFunc)
		fInfo = NULL;

	if(funcName && funcAddr && newType && !vInfo)
	{
		CodeInfo::nodeList.push_back(funcAddr);
		funcAddr = NULL;
		for(unsigned int i = 0; i < fType->paramCount; i++)
		{
			NodeZeroOP *tmp = CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - i];
			CodeInfo::nodeList[CodeInfo::nodeList.size() - 1 - i] = CodeInfo::nodeList[CodeInfo::nodeList.size() - 2 - i];
			CodeInfo::nodeList[CodeInfo::nodeList.size() - 2 - i] = tmp;
		}
	}

	for(unsigned int i = 0; i < fType->paramCount; i++)
	{
		CodeInfo::nodeList.push_back(CodeInfo::nodeList[CodeInfo::nodeList.size() - fType->paramCount + i]);

		ConvertBaseToDerived(pos, fType->paramType[i]);
		ConvertDerivedToBase(pos, fType->paramType[i]);
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

		CodeInfo::nodeList[CodeInfo::nodeList.size() - fType->paramCount + i - 1] = CodeInfo::nodeList.back();
		CodeInfo::nodeList.pop_back();
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

void PromoteToBool(const char* pos)
{
	TypeInfo *condType = CodeInfo::nodeList.back()->typeInfo;
	if(condType->type == TypeInfo::TYPE_COMPLEX && condType != typeObject)
	{
		if(!AddFunctionCallNode(pos, "bool", 1, true) && (condType->funcType || condType->arrLevel))
		{
			AddNullPointer();
			AddBinaryCommandNode(pos, cmdNEqual);
		}
	}
}

void AddIfNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 2);

	if(OptimizeIfElse(false))
		return;

	NodeZeroOP *body = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	PromoteToBool(pos);

	CodeInfo::nodeList.push_back(body);

	CodeInfo::nodeList.push_back(new NodeIfElseExpr(false));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}

void AddIfElseNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 3);

	if(OptimizeIfElse(true))
		return;

	NodeZeroOP *bodyE = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	NodeZeroOP *bodyT = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	PromoteToBool(pos);

	CodeInfo::nodeList.push_back(bodyT);
	CodeInfo::nodeList.push_back(bodyE);

	CodeInfo::nodeList.push_back(new NodeIfElseExpr(true));
	CodeInfo::nodeList.back()->SetCodeInfo(pos);
}

void AddIfElseTermNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 3);

	if(OptimizeIfElse(true))
		return;

	HandleNullPointerConversion(pos);

	NodeZeroOP *bodyE = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();
	NodeZeroOP *bodyT = CodeInfo::nodeList.back();
	CodeInfo::nodeList.pop_back();

	PromoteToBool(pos);

	CodeInfo::nodeList.push_back(bodyT);
	CodeInfo::nodeList.push_back(bodyE);

	TypeInfo* typeA = CodeInfo::nodeList[CodeInfo::nodeList.size()-1]->typeInfo;
	TypeInfo* typeB = CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo;
	if(typeA == typeVoid || typeB == typeVoid)
		ThrowError(pos, "ERROR: one of ternary operator ?: result type is void (%s : %s)", typeB->GetFullTypeName(), typeA->GetFullTypeName());

	// Different numeric types are supported
	TypeInfo* typeCommon = NULL;
	if(typeA->type != TypeInfo::TYPE_COMPLEX && typeA->refLevel == 0 && typeA->arrLevel == 0 && typeB->type != TypeInfo::TYPE_COMPLEX && typeB->refLevel == 0 && typeB->arrLevel == 0)
		typeCommon = ChooseBinaryOpResultType(typeA, typeB);

	if(typeA != typeB && typeCommon == NULL)
		ThrowError(pos, "ERROR: ternary operator ?: result types are not equal (%s : %s)", typeB->GetFullTypeName(), typeA->GetFullTypeName());

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

	NodeZeroOP *body = CodeInfo::nodeList.back(); CodeInfo::nodeList.pop_back();
	NodeZeroOP *increment = CodeInfo::nodeList.back(); CodeInfo::nodeList.pop_back();

	PromoteToBool(pos);

	CodeInfo::nodeList.push_back(increment);
	CodeInfo::nodeList.push_back(body);

	CodeInfo::nodeList.push_back(new NodeForExpr());

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}

void AddWhileNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 2);

	NodeZeroOP *body = CodeInfo::nodeList.back(); CodeInfo::nodeList.pop_back();

	PromoteToBool(pos);

	CodeInfo::nodeList.push_back(body);

	CodeInfo::nodeList.push_back(new NodeWhileExpr());
	CodeInfo::nodeList.back()->SetCodeInfo(pos);

	assert(cycleDepth.size() != 0);
	cycleDepth.back()--;
}

void AddDoWhileNode(const char* pos)
{
	CodeInfo::lastKnownStartPos = pos;
	assert(CodeInfo::nodeList.size() >= 2);

	PromoteToBool(pos);

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

	TypeInfo *type = CodeInfo::nodeList.back()->typeInfo;
	if(type->type == TypeInfo::TYPE_COMPLEX && type != typeObject && type != typeTypeid)
	{
		AddInplaceVariable(pos);
		CodeInfo::nodeList.push_back(new NodeDereference());
		CodeInfo::nodeList.push_back(new NodeSwitchExpr(true));
	}else{
		CodeInfo::nodeList.push_back(new NodeSwitchExpr());
	}
}

void AddCaseNode(const char* pos)
{
	assert(CodeInfo::nodeList.size() >= 3);
	if(CodeInfo::nodeList[CodeInfo::nodeList.size()-2]->typeInfo == typeVoid)
		ThrowError(pos, "ERROR: case value type cannot be void");
	NodeZeroOP* temp = CodeInfo::nodeList[CodeInfo::nodeList.size()-3];
	assert(temp->nodeType == typeNodeSwitchExpr);
	NodeSwitchExpr *node = (NodeSwitchExpr*)temp;
	if(NodeZeroOP *cond = node->IsComplex())
	{
		NodeZeroOP *body = CodeInfo::nodeList.back(); CodeInfo::nodeList.pop_back();

		NodeOneOP *wrap = new NodeOneOP();
		wrap->SetFirstNode(cond);
		CodeInfo::nodeList.push_back(wrap);
		AddFunctionCallNode(pos, "==", 2);
		if(CodeInfo::nodeList.back()->typeInfo->stackType != STYPE_INT)
			ThrowError(pos, "ERROR: '==' operator result type must be bool, char, short or int");
		CodeInfo::nodeList.push_back(body);
	}
	node->AddCase();
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
TypeInfo* TypeBegin(const char* pos, const char* end, bool addNamespace, unsigned aliasCount)
{
	if(newType)
		ThrowError(pos, "ERROR: different type is being defined");

	if(currAlign & (currAlign - 1))
		ThrowError(pos, "ERROR: alignment must be power of two");

	if(currAlign > 16)
		ThrowError(pos, "ERROR: alignment must be less than 16 bytes");

	char *typeNameCopy = NULL;
	if(addNamespace)
	{
		typeNameCopy = (char*)GetNameInNamespace(InplaceStr(pos, end), true).begin;
	}else{
		typeNameCopy = AllocateString((unsigned)strlen(pos) + 1);
		memcpy(typeNameCopy, pos, strlen(pos) + 1);
	}

	if(IsNamespace(InplaceStr(typeNameCopy)))
		ThrowError(pos, "ERROR: name is already taken for a namespace");

	unsigned int hash = GetStringHash(typeNameCopy);

	HashMap<TypeInfo*>::Node *type = CodeInfo::classMap.first(hash);

	// Check if the new type if generic and is defined with a different number of type parameters
	if(aliasCount && type && type->value->genericInfo)
	{
		while(type && aliasCount != type->value->genericInfo->aliasCount)
			type = CodeInfo::classMap.next(type);
	}

	if(type && type->value->hasFinished)
		ThrowError(pos, "ERROR: '%s' is being redefined", typeNameCopy);
	else if(type && !type->value->hasFinished)
		newType = type->value;
	else
		newType = new TypeInfo(CodeInfo::typeInfo.size(), typeNameCopy, 0, 0, 1, NULL, TypeInfo::TYPE_COMPLEX);
	newType->alignBytes = currAlign;
	newType->originalIndex = CodeInfo::typeInfo.size();
	newType->definitionDepth = varInfoTop.size();
	newType->hasFinished = false;
	newType->parentNamespace = namespaceStack.size() > 1 ? namespaceStack.back() : NULL;
	currAlign = TypeInfo::ALIGNMENT_UNSPECIFIED;
	methodCount = 0;

	if(!type)
	{
		CodeInfo::typeInfo.push_back(newType);
		CodeInfo::classMap.insert(newType->GetFullNameHash(), newType);
	}

	BeginBlock();

	return type ? type->value : NULL;
}

// Add class member
void TypeAddMember(const char* pos, const char* varName)
{
	if(!currType)
		ThrowError(pos, "ERROR: auto cannot be used for class members");
	if(!currType->hasFinished)
		ThrowError(pos, "ERROR: type '%s' is not fully defined. You can use '%s ref' or '%s[]' at this point", currType->GetFullTypeName(), currType->GetFullTypeName(), currType->GetFullTypeName());

	// Align members to their default alignment unless the class alignment is specified to be a smaller value or there is an explicit alignment specification for the member
	unsigned classAlign = newType->alignBytes;
	unsigned memberAlign = currAlign != TypeInfo::ALIGNMENT_UNSPECIFIED ? currAlign : currType->alignBytes;

	unsigned int alignment = classAlign != TypeInfo::ALIGNMENT_UNSPECIFIED && classAlign < memberAlign ? classAlign : memberAlign;
	if(alignment != TypeInfo::ALIGNMENT_UNSPECIFIED && newType->size % alignment != 0)
		newType->size += alignment - (newType->size % alignment);

	newType->AddMemberVariable(varName, currType);
	newType->lastVariable->alignBytes = alignment;

	if(newType->size > 64 * 1024)
		ThrowError(pos, "ERROR: class size cannot exceed 65535 bytes");
	if(currType->hasFinalizer)
		ThrowError(pos, "ERROR: class '%s' implements 'finalize' so only a reference or an unsized array of '%s' can be put in a class", currType->GetFullTypeName(), currType->GetFullTypeName());

	unsigned oldVarTop = varTop;

	VariableInfo *varInfo = (VariableInfo*)AddVariable(pos, InplaceStr(varName, (int)strlen(varName)), false);
	varInfo->pos = 0;
	varInfo->isGlobal = true;
	varInfo->parentType = newType;

	varTop = oldVarTop;
}

void TypeAddConstant(const char* pos, const char* constName)
{
	if(CodeInfo::nodeList.back()->nodeType != typeNodeNumber)
		ThrowError(pos, "ERROR: expression didn't evaluate to a constant number");

	if(currType)
		((NodeNumber*)CodeInfo::nodeList.back())->ConvertTo(currType);

	newType->AddMemberVariable(constName, currType ? currType : CodeInfo::nodeList.back()->typeInfo, false, CodeInfo::nodeList.back());
	CodeInfo::nodeList.pop_back();

	for(TypeInfo::MemberVariable *curr = newType->firstVariable; curr && curr != newType->lastVariable; curr = curr->next)
	{
		if(curr->nameHash == newType->lastVariable->nameHash)
			ThrowError(pos, "ERROR: name '%s' is already taken for a variable in current scope", constName);
	}

	unsigned oldVarTop = varTop;

	VariableInfo *varInfo = (VariableInfo*)AddVariable(pos, InplaceStr(constName, (int)strlen(constName)));
	varInfo->pos = 0;
	varInfo->isGlobal = true;
	varInfo->parentType = newType;

	varTop = oldVarTop;
}

void TypePrototypeFinish()
{
	CodeInfo::nodeList.push_back(new NodeZeroOP());
	newType = NULL;

	// Remove class members from global scope
	EndBlock(false, false);
}

// End of type definition
void TypeFinish()
{
	unsigned maximumAlignment = 0;

	// Additional padding may apply to preserve the alignment of members
	for(TypeInfo::MemberVariable *curr = newType->firstVariable; curr; curr = curr->next)
		maximumAlignment = maximumAlignment > curr->alignBytes ? maximumAlignment : curr->alignBytes;

	// If explicit alignment is not specified, then class must be aligned to the maximum alignment of the members
	if(newType->alignBytes == 0)
		newType->alignBytes = maximumAlignment;

	// In NULLC, all classes have sizes multiple of 4, so add additional padding if necessary
	maximumAlignment = newType->alignBytes < 4 ? 4 : newType->alignBytes;

	if(newType->size % maximumAlignment != 0)
	{
		newType->paddingBytes = maximumAlignment - (newType->size % maximumAlignment);
		newType->size += maximumAlignment - (newType->size % maximumAlignment);
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

	// Check if custom default assignment operator is required
	bool customConstructor = false;
	TypeInfo::MemberVariable *curr = newType->firstVariable;
	for(; curr; curr = curr->next)
	{
		TypeInfo *base = curr->type;
		while(base && base->arrLevel && base->arrSize != TypeInfo::UNSIZED_ARRAY) // Unsized arrays are not initialized
			base = base->subType;
		if(HasConstructor(base, 0))
			customConstructor = true;
		if(curr->nameHash == extendableVariableName)
			customConstructor = true;
	}

	if(customConstructor)
	{
		currType = typeVoid;
		const char *constructorName = FindConstructorName(newType);
		unsigned length = (unsigned)strlen(constructorName);
		char *name = AllocateString(length + 2);
		SafeSprintf(name, length + 2, "%s$", constructorName);
		FunctionAdd(CodeInfo::lastKnownStartPos, name);
		currDefinedFunc.back()->typeConstructor = true;
		FunctionStart(CodeInfo::lastKnownStartPos);
		AddVoidNode();
		FunctionEnd(CodeInfo::lastKnownStartPos);
		AddTwoExpressionNode();
	}

	// Remove class members from global scope
	EndBlock(false, false);

	TypeInfo *lastType = newType;
	newType = NULL;

	bool customAssign = false;
	for(curr = lastType->firstVariable; curr; curr = curr->next)
	{
		if(curr->nameHash == extendableVariableName)
		{
			customAssign = true;
			continue;
		}
		if(curr->defaultValue || curr->type->refLevel || curr->type->arrLevel || curr->type->funcType)
			continue;
		// Check assignment operator by virtually calling a = function with (Type ref, Type) arguments
		CodeInfo::nodeList.push_back(new NodeZeroOP(CodeInfo::GetReferenceType(curr->type)));
		CodeInfo::nodeList.push_back(new NodeZeroOP(curr->type));

		if(!AddFunctionCallNode(CodeInfo::lastKnownStartPos, "=", 2, true))
		{
			CodeInfo::nodeList.pop_back();
			CodeInfo::nodeList.pop_back();
		}else{
			CodeInfo::nodeList.pop_back();
			customAssign = true;
		}
	}

	// Generate a function, if required
	if(customAssign)
	{
		currType = CodeInfo::GetReferenceType(lastType);
		FunctionAdd(CodeInfo::lastKnownStartPos, "$defaultAssign");
		currType = CodeInfo::GetReferenceType(lastType);
		FunctionParameter(CodeInfo::lastKnownStartPos, InplaceStr("left"));
		currType = lastType;
		FunctionParameter(CodeInfo::lastKnownStartPos, InplaceStr("right"));
		FunctionStart(CodeInfo::lastKnownStartPos);
		
		unsigned exprCount = 0;
		for(curr = lastType->firstVariable; curr; curr = curr->next)
		{
			// Skip constants
			if(curr->defaultValue)
				continue;
			// Custom assignment operator will never change extendable class typeid
			if(curr->nameHash == extendableVariableName)
				continue;
			AddGetAddressNode(CodeInfo::lastKnownStartPos, InplaceStr("left"));
			AddGetVariableNode(CodeInfo::lastKnownStartPos);
			AddMemberAccessNode(CodeInfo::lastKnownStartPos, InplaceStr(curr->name));
			AddGetAddressNode(CodeInfo::lastKnownStartPos, InplaceStr("right"));
			AddMemberAccessNode(CodeInfo::lastKnownStartPos, InplaceStr(curr->name));
			AddGetVariableNode(CodeInfo::lastKnownStartPos);
			AddSetVariableNode(CodeInfo::lastKnownStartPos);
			AddPopNode(CodeInfo::lastKnownStartPos);
			exprCount++;
		}

		AddGetAddressNode(CodeInfo::lastKnownStartPos, InplaceStr("left"));
		AddGetVariableNode(CodeInfo::lastKnownStartPos);
		AddReturnNode(CodeInfo::lastKnownStartPos);

		while(exprCount--)
			AddTwoExpressionNode();

		FunctionEnd(CodeInfo::lastKnownStartPos);
		AddTwoExpressionNode();
	}
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
		VariableInfo *varInfo = (VariableInfo*)AddVariable(pos, InplaceStr(curr->name), false);
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

void TypeInstanceGeneric(const char* pos, TypeInfo* base, unsigned aliases, bool genericTemp)
{
	NodeZeroOP **aliasType = &CodeInfo::nodeList[CodeInfo::nodeList.size() - aliases];

	// Generate instance name
	unsigned nameLength = base->GetFullNameLength() + 3;
	for(unsigned i = 0; i < aliases; i++)
		nameLength += aliasType[i]->typeInfo->GetFullNameLength() + (i == aliases - 1 ? 0 : 2);
	if(nameLength >= NULLC_MAX_VARIABLE_NAME_LENGTH)
		ThrowError(pos, "ERROR: generated generic type name exceeds maximum type length '%d'", NULLC_MAX_VARIABLE_NAME_LENGTH);

	char *tempName = AllocateString(nameLength);
	char *namePos = tempName;
	namePos += SafeSprintf(namePos, nameLength - int(namePos - tempName), "%s<", base->name);
	for(unsigned i = 0; i < aliases; i++)
		namePos += SafeSprintf(namePos, nameLength - int(namePos - tempName), "%s%s", aliasType[i]->typeInfo->GetFullTypeName(), i == aliases - 1 ? "" : ", ");
	namePos += SafeSprintf(namePos, nameLength - int(namePos - tempName), ">");

	HashMap<TypeInfo*>::Node *type = CodeInfo::classMap.first(base->nameHash);

	unsigned overloadCount = 0;

	// Check if the new type if generic and is defined with a different number of type parameters
	while(type && aliases != type->value->genericInfo->aliasCount)
	{
		overloadCount++;
		type = CodeInfo::classMap.next(type);
	}

	if(!type)
	{
		if(overloadCount == 1)
		{
			if(aliases < base->genericInfo->aliasCount)
				ThrowError(pos, "ERROR: there where only '%d' argument(s) to a generic type that expects '%d'", aliases, base->genericInfo->aliasCount);
			if(aliases > base->genericInfo->aliasCount)
				ThrowError(pos, "ERROR: type has only '%d' generic argument(s) while '%d' specified", base->genericInfo->aliasCount, aliases);
		}else{
			ThrowError(pos, "ERROR: none of the '%s' type overloads accept '%d' type argument(s)", base->name, aliases);
		}
	}

	base = type->value;

	Lexeme *start = CodeInfo::lexStart + base->genericInfo->start;

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
	TypeBegin(tempName, namePos, false);
	TypeInfo *instancedType = newType;
	newType->genericBase = base;
	newType->childAlias = aliasList;
	newType->parentNamespace = base->parentNamespace;

	// Backup namespace state and restore it to class definition state
	unsigned prevBackupSize = 0, prevStackSize = 0;
	NamespaceInfo *lastNS = NULL;
	RestoreNamespaces(false, newType->parentNamespace, prevBackupSize, prevStackSize, lastNS);

	jmp_buf oldHandler;
	memcpy(oldHandler, CodeInfo::errorHandler, sizeof(jmp_buf));
	if(!genericTemp)
	{
		// Reparse type body and format errors so that the user will know where it happened
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
	}else{
		newType->dependsOnGeneric = true;
	}
	// Restore namespace stack
	RestoreNamespaces(true, NULL, prevBackupSize, prevStackSize, lastNS);
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

void TypeDeriveFrom(const char* pos, TypeInfo* type)
{
	assert(newType);
	newType->parentType = type;

	if(!type)
		ThrowError(pos, "ERROR: auto type cannot be used as a base class");
	if(!type->hasFinished)
		ThrowError(pos, "ERROR: type '%s' is not fully defined. You can use '%s ref' or '%s[]' at this point", type->GetFullTypeName(), type->GetFullTypeName(), type->GetFullTypeName());
	if(!type->firstVariable || type->firstVariable->nameHash != extendableVariableName)
		ThrowError(pos, "ERROR: type '%s' is not extendable", type->GetFullTypeName());

	// Alignment is inherited unless a new one is specified
	newType->alignBytes = currAlign != TypeInfo::ALIGNMENT_UNSPECIFIED ? currAlign : type->alignBytes;

	// Inherit aliases
	AliasInfo *aliasList = type->childAlias;
	while(aliasList)
	{
		AliasInfo *currAlias = newType->childAlias;
		while(currAlias)
		{
			if(currAlias->nameHash == aliasList->nameHash)
				break;
			currAlias = currAlias->next;
		}
		if(!currAlias)
		{
			AliasInfo *info = TypeInfo::CreateAlias(aliasList->name, aliasList->type);
			info->next = newType->childAlias;
			newType->childAlias = info;
			CodeInfo::classMap.insert(aliasList->nameHash, aliasList->type);
		}
		aliasList = aliasList->next;
	}
	// Inherit member variables
	for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
	{
		currType = curr->type;

		unsigned size = newType->size;
		unsigned memberCount = newType->memberCount;
		bool hasPointers = newType->hasPointers;

		// No need to add extendable class typeid again
		if(newType->firstVariable && newType->firstVariable->nameHash == extendableVariableName && curr->nameHash == extendableVariableName)
			continue;

		SetCurrentAlignment(TypeInfo::ALIGNMENT_UNSPECIFIED);

		TypeAddMember(pos, curr->name);

		// There is no way to tell what alignment was specified when the base class was defined, so the base class member offset is taken
		newType->lastVariable->offset = curr->offset;

		newType->lastVariable->defaultValue = curr->defaultValue;
		if(curr->defaultValue)
		{
			newType->size = size;
			newType->memberCount = memberCount;
			newType->hasPointers = hasPointers;
			newType->lastVariable->offset = 0;
			continue;
		}
	}
	// Type size is taken directly from base class because it is now unknown what alignment was used during base class definition
	newType->size = type->size;
}

TypeInfo* GetDefinedType()
{
	return newType;
}

unsigned ResetDefinedTypeState()
{
	unsigned prevCount = methodCount;
	newType = NULL;
	methodCount = 0;
	return prevCount;
}
void RestoreDefinedTypeState(TypeInfo* type, unsigned mCount)
{
	methodCount = mCount;
	newType = type;
}

void TypeExtendable(const char* pos)
{
	assert(newType);
	assert(!newType->firstVariable);
	currType = typeTypeid;

	SetCurrentAlignment(TypeInfo::ALIGNMENT_UNSPECIFIED);

	TypeAddMember(pos, "$typeid");
}

void AddAliasType(InplaceStr aliasName)
{
	AliasInfo *info = TypeInfo::CreateAlias(aliasName, currType);
	if(newType && (!currDefinedFunc.size() || newType->definitionDepth > currDefinedFunc.back()->vTopSize))	// If we're inside a class definition, but _not_ inside a function
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

void RestoreRedirectionTables()
{
	assert(newVtblCount == 0);
	// Search for virtual function tables in imported modules so that we can add functions from new classes
	for(unsigned i = 0; i < CodeInfo::varInfo.size(); i++)
	{
		// Name must start from $vtbl and must be at least 15 characters
		if(CodeInfo::varInfo[i]->name.length() < 15 || memcmp(CodeInfo::varInfo[i]->name.begin, "$vtbl", 5) != 0)
			continue;
		CodeInfo::varInfo[i]->next = vtblList;
		vtblList = CodeInfo::varInfo[i];
	}
}

void CreateRedirectionTables()
{
	VariableInfo *curr = vtblList;
	while(curr)
	{
		TypeInfo *funcType = NULL;
		unsigned int hash = GetStringHash(curr->name.begin + 15); // 15 to skip $vtbl0123456789 from name

		// If this is continuation of an imported virtual table, find function type from hash code in name
		if(!newVtblCount)
		{
			unsigned typeHash = parseInteger(curr->name.begin + 5); // 5 to skip $vtbl
			for(unsigned c = 0; !funcType && c < CodeInfo::typeFunctions.size(); c++)
			{
				if(CodeInfo::typeFunctions[c]->GetFullNameHash() == typeHash)
					funcType = CodeInfo::typeFunctions[c];
			}
			AddVoidNode();
		}else{
			currType = curr->varType;
			CodeInfo::nodeList.push_back(new NodeNumber(4, typeInt));
			AddFunctionCallNode(CodeInfo::lastKnownStartPos, "__typeCount", 0);
			CodeInfo::nodeList.push_back(new NodeNumber(0, typeInt));
			CallAllocationFunction(CodeInfo::lastKnownStartPos, "__newA");
			CodeInfo::nodeList.back()->typeInfo = currType;
			CodeInfo::varInfo.push_back(curr);
			curr->pos = varTop;
			varTop += curr->varType->size;

			AddDefineVariableNode(CodeInfo::lastKnownStartPos, curr, true);
			AddPopNode(CodeInfo::lastKnownStartPos);

			funcType = (TypeInfo*)curr->prev;
			newVtblCount--;
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
				TypeInfo *type = CodeInfo::typeInfo[i];
				// Stepping through the class inheritance tree will ensure that the base class function will be used if the derived class function is not available
				while(type)
				{
					// Walk up to the class base class
					if(bestFuncList[k]->parentClass == type)
					{
						// Get array variable
						CodeInfo::nodeList.push_back(new NodeGetAddress(curr, curr->pos, curr->varType));
						CodeInfo::nodeList.push_back(new NodeDereference());

						// Push index (typeID number is index)
						CodeInfo::nodeList.push_back(new NodeZeroOP(typeInt));
						CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdPushTypeID, CodeInfo::typeInfo[i]->typeIndex));

						// Index array
						CodeInfo::nodeList.push_back(new NodeArrayIndex(curr->varType));

						// Push functionID
						CodeInfo::nodeList.push_back(new NodeZeroOP(typeFunction));
						CodeInfo::nodeList.push_back(new NodeUnaryOp(cmdFuncAddr, bestFuncList[k]->indexInArr));

						// Set array element value
						CodeInfo::nodeList.push_back(new NodeVariableSet(CodeInfo::nodeList[CodeInfo::nodeList.size() - 2]->typeInfo, 0, true));

						// Remove value from stack
						AddPopNode(CodeInfo::lastKnownStartPos);
						// Fold node
						AddTwoExpressionNode(NULL);
						break;
					}
					type = type->parentType;
				}
			}
		}
		// Add node in front of global code
		static_cast<NodeExpressionList*>(CodeInfo::nodeList[0])->AddNode(true);

		curr = curr->next;
	}
}

void RestoreScopedGlobals()
{
	for(unsigned i = 0; i < CodeInfo::typeInfo.size(); i++)
	{
		if(!CodeInfo::typeInfo[i]->hasFinished)
			ThrowError(NULL, "ERROR: type '%s' implementation is not found", CodeInfo::typeInfo[i]->name);
	}
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

	currAlign = TypeInfo::ALIGNMENT_UNSPECIFIED;
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
	newVtblCount = 0;

	// Clear namespace list and place an unnamed namespace on top of namespace stack
	CodeInfo::namespaceInfo.clear();
	namespaceStack.clear();
	namespaceStack.push_back(new NamespaceInfo(InplaceStr(""), GetStringHash(""), NULL));
	currNamespace = NULL;

	explicitTypesStack.clear();
	currExplicitTypes = NULL;

	namedArgumentBackup.clear();

	CodeInfo::classMap.clear();
	CodeInfo::typeArrays.clear();
	CodeInfo::typeFunctions.clear();
	// Add built-in type info to hash map and special lists
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

	currExplicitTypes = NULL;

	varInfoTop.reset();
	funcInfoTop.reset();
	cycleDepth.reset();
	currDefinedFunc.reset();
	delayedInstance.reset();
	bestFuncList.reset();
	bestFuncRating.reset();
	bestFuncListBackup.reset();
	bestFuncRatingBackup.reset();
	nodeBackup.reset();
	namespaceBackup.reset();
	paramNodes.reset();
	funcMap.reset();
	varMap.reset();

	namespaceStack.reset();
	explicitTypesStack.reset();
	namedArgumentBackup.reset();

	TypeInfo::typeInfoPool.~ChunkedStackPool();
	TypeInfo::SetPoolTop(0);
	VariableInfo::variablePool.~ChunkedStackPool();
	VariableInfo::SetPoolTop(0);
	FunctionInfo::functionPool.~ChunkedStackPool();
	FunctionInfo::SetPoolTop(0);
	NamespaceInfo::namespacePool.~ChunkedStackPool();
	NamespaceInfo::ResetPool();

	NULLC::dealloc(errorReport);
	errorReport = NULL;
}
