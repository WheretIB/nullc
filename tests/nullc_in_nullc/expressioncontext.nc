import std.vector;
import typetree;
import typetreehelpers;
import expressiontree;

class ExpressionContext
{
	void ExpressionContext(int optimizationLevel);

	void StopAt(SynBase ref source, StringRef pos, char[] msg, auto ref[] args);
	void Stop(SynBase ref source, char[] msg, auto ref[] args);

	void PushScope(ScopeType type);
	void PushScope(NamespaceData ref nameSpace);
	void PushScope(FunctionData ref function);
	void PushScope(TypeBase ref type);
	void PushLoopScope(bool allowBreak, bool allowContinue);
	void PushTemporaryScope();
	void PopScope(ScopeType type, bool ejectContents, bool keepFunctions);
	void PopScope(ScopeType type);
	void RestoreScopesAtPoint(ScopeData ref target, SynBase ref location);
	void SwitchToScopeAtPoint(ScopeData ref target, SynBase ref targetLocation);

	NamespaceData ref GetCurrentNamespace(ScopeData ref scopeData);
	FunctionData ref GetCurrentFunction(ScopeData ref scopeData);
	TypeBase ref GetCurrentType(ScopeData ref scopeData);

	FunctionData ref GetFunctionOwner(ScopeData ref scope);

	ScopeData ref NamespaceScopeFrom(ScopeData ref scope);
	ScopeData ref GlobalScopeFrom(ScopeData ref scope);

	bool IsGenericInstance(FunctionData ref function);

	void AddType(TypeBase ref type);
	void AddFunction(FunctionData ref function);
	void AddVariable(VariableData ref variable, bool visible);
	void AddAlias(AliasData ref alias);

	int GetTypeIndex(TypeBase ref type);
	int GetFunctionIndex(FunctionData ref data);

	void HideFunction(FunctionData ref function);

	bool IsGenericFunction(FunctionData ref function);

	bool IsIntegerType(TypeBase ref type);
	bool IsFloatingPointType(TypeBase ref type);
	bool IsNumericType(TypeBase ref type);
	TypeBase ref GetBinaryOpResultType(TypeBase ref a, TypeBase ref b);

	TypeError ref GetErrorType();
	TypeRef ref GetReferenceType(TypeBase ref type);
	TypeArray ref GetArrayType(TypeBase ref type, long size);
	TypeUnsizedArray ref GetUnsizedArrayType(TypeBase ref type);
	TypeFunction ref GetFunctionType(SynBase ref source, TypeBase ref returnType, RefList<TypeHandle> arguments);
	TypeFunction ref GetFunctionType(SynBase ref source, TypeBase ref returnType, ArrayView<ArgumentData> arguments);
	TypeFunctionSet ref GetFunctionSetType(ArrayView<TypeBase ref> types);
	TypeGenericAlias ref GetGenericAliasType(SynIdentifier ref baseName);
	TypeGenericClass ref GetGenericClassType(SynBase ref source, TypeGenericClassProto ref proto, RefList<TypeHandle> generics);

	ModuleData ref GetSourceOwner(LexemeRef lexeme);

	SynInternal ref MakeInternal(SynBase ref source);

	int optimizationLevel;

	// Full state info
	char[] code;

	vector<ModuleData ref> dependencies;
	vector<ModuleData ref> imports;
	vector<NamespaceData ref> namespaces;
	vector<TypeBase ref> types;
	vector<FunctionData ref> functions;
	vector<VariableData ref> variables;

	vector<ExprBase ref> definitions;
	vector<ExprBase ref> setup;

	vector<VariableData ref> vtables;
	hashmap<InplaceStr, VariableData ref> vtableMap;

	vector<VariableData ref> upvalues;
	hashmap<InplaceStr, VariableData ref> upvalueMap;

	vector<TypeFunction ref> functionTypes;
	vector<TypeFunctionSet ref> functionSetTypes;
	vector<TypeGenericAlias ref> genericAliasTypes;
	vector<TypeGenericClass ref> genericClassTypes;

	hashmap<int, TypeClass ref> genericTypeMap;

	hashmap<GenericFunctionInstanceTypeRequest, GenericFunctionInstanceTypeResponse> genericFunctionInstanceTypeMap;

	hashmap<TypePair, bool> noAssignmentOperatorForTypePair;

	hashmap<TypedFunctionInstanceRequest, ExprBase ref> newConstructorFunctions;

	int baseModuleFunctionCount;

	// Context info
	hashmap<int, TypeBase ref> typeMap;
	hashmap<int, FunctionData ref> functionMap;
	hashmap<int, VariableData ref> variableMap;

	ScopeData ref scope;

	ScopeData ref globalScope;
	vector<NamespaceData ref> globalNamespaces;
	RefList<CloseUpvaluesData> globalCloseUpvalues;

	int functionInstanceDepth;
	int classInstanceDepth;
	int expressionDepth;

	int memoryLimit;

	// Error info
	bool errorHandlerActive;
	bool errorHandlerNested;
	//jmp_buf errorHandler;
	int errorPos;
	int errorCount;
	char[] errorBuf;
	StringRef errorBufLocation;

	vector<ErrorInfo ref> errorInfo;

	// Base types
	TypeBase ref typeVoid;

	TypeBase ref typeBool;

	TypeBase ref typeChar;
	TypeBase ref typeShort;
	TypeBase ref typeInt;
	TypeBase ref typeLong;

	TypeBase ref typeFloat;
	TypeBase ref typeDouble;

	TypeBase ref typeTypeID;
	TypeBase ref typeFunctionID;
	TypeBase ref typeNullPtr;

	TypeBase ref typeGeneric;

	TypeBase ref typeAuto;
	TypeStruct ref typeAutoRef;
	TypeStruct ref typeAutoArray;

	// Counters
	int uniqueNamespaceId;
	int uniqueVariableId;
	int uniqueFunctionId;
	int uniqueAliasId;
	int uniqueScopeId;

	int unnamedFuncCount;
	int unnamedVariableCount;
}

void ExpressionContext:ExpressionContext(int optimizationLevel)
{
	this.optimizationLevel = optimizationLevel;

	code = nullptr;

	baseModuleFunctionCount = 0;

	errorHandlerActive = false;
	errorHandlerNested = false;
	errorPos = 0;
	errorCount = 0;
	errorBuf = nullptr;
	errorBufLocation = StringRef();

	typeVoid = nullptr;

	typeBool = nullptr;

	typeChar = nullptr;
	typeShort = nullptr;
	typeInt = nullptr;
	typeLong = nullptr;

	typeFloat = nullptr;
	typeDouble = nullptr;

	typeTypeID = nullptr;
	typeFunctionID = nullptr;
	typeNullPtr = nullptr;

	typeGeneric = nullptr;

	typeAuto = nullptr;
	typeAutoRef = nullptr;
	typeAutoArray = nullptr;

	scope = nullptr;

	globalScope = nullptr;

	functionInstanceDepth = 0;
	classInstanceDepth = 0;
	expressionDepth = 0;

	memoryLimit = 0;

	uniqueNamespaceId = 0;
	uniqueVariableId = 0;
	uniqueFunctionId = 0;
	uniqueAliasId = 0;
	uniqueScopeId = 0;

	unnamedFuncCount = 0;
	unnamedVariableCount = 0;
}

void ExpressionContext:StopAt(SynBase ref source, StringRef pos, char[] msg, auto ref[] args)
{
	//StopAt(*this, source, pos, msg, args);
}

void ExpressionContext:Stop(SynBase ref source, char[] msg, auto ref[] args)
{
	//StopAt(*this, source, source.pos.begin, msg, args);
}

void ExpressionContext:PushScope(ScopeType type)
{
	ScopeData ref next = new ScopeData(scope, uniqueScopeId++, type);

	if(scope)
	{
		scope.scopes.push_back(next);

		next.startOffset = scope.dataSize;
	}

	scope = next;
}

void ExpressionContext:PushScope(NamespaceData ref nameSpace)
{
	ScopeData ref next = new ScopeData(scope, uniqueScopeId++, nameSpace);

	if(scope)
	{
		scope.scopes.push_back(next);

		next.startOffset = scope.dataSize;
	}

	scope = next;
}

void ExpressionContext:PushScope(FunctionData ref function)
{
	ScopeData ref next = new ScopeData(scope, uniqueScopeId++, function);

	if(scope)
		scope.scopes.push_back(next);

	scope = next;
}

void ExpressionContext:PushScope(TypeBase ref type)
{
	ScopeData ref next = new ScopeData(scope, uniqueScopeId++, type);

	if(scope)
		scope.scopes.push_back(next);

	scope = next;
}

void ExpressionContext:PushLoopScope(bool allowBreak, bool allowContinue)
{
	ScopeData ref next = new ScopeData(scope, uniqueScopeId++, ScopeType.SCOPE_LOOP);

	if(scope)
	{
		scope.scopes.push_back(next);

		next.startOffset = scope.dataSize;
	}

	if(allowBreak)
		next.breakDepth++;

	if(allowContinue)
		next.contiueDepth++;

	scope = next;
}

void ExpressionContext:PushTemporaryScope()
{
	scope = new ScopeData(scope, 0, ScopeType.SCOPE_TEMPORARY);
}

void ExpressionContext:PopScope(ScopeType scopeType, bool ejectContents, bool keepFunctions)
{
	assert(scope.type == scopeType);

	// When namespace scope ends, all the contents remain accessible through an outer namespace/global scope
	if(ejectContents && scope.ownerNamespace)
	{
		ScopeData ref adopter = scope.scope;

		while(!adopter.ownerNamespace && adopter.scope)
			adopter = adopter.scope;

		adopter.variables.push_back(scope.variables.data, scope.variables.size());
		adopter.functions.push_back(scope.functions.data, scope.functions.size());
		adopter.types.push_back(scope.types.data, scope.types.size());
		adopter.aliases.push_back(scope.aliases.data, scope.aliases.size());

		adopter.visibleVariables.push_back(scope.visibleVariables.data, scope.visibleVariables.size());

		adopter.allVariables.push_back(scope.allVariables.data, scope.allVariables.size());

		scope.variables.clear();
		scope.functions.clear();
		scope.types.clear();
		scope.aliases.clear();

		scope.visibleVariables.clear();

		scope.allVariables.clear();

		scope = scope.scope;
		return;
	}

	// Full set of scope variables is moved to the outer scope untill we reach function, namespace or a global scope
	if(ejectContents && scope.scope && (scope.type == ScopeType.SCOPE_EXPLICIT || scope.type == ScopeType.SCOPE_LOOP))
	{
		scope.scope.allVariables.push_back(scope.allVariables.data, scope.allVariables.size());

		scope.allVariables.clear();
	}

	// Remove scope members from lookup maps
	for(int i = int(scope.visibleVariables.count) - 1; i >= 0; i--)
	{
		VariableData ref variable = scope.visibleVariables[i];

		if(variableMap.find(variable.nameHash, variable))
			variableMap.remove(variable.nameHash, variable);
	}

	for(int i = int(scope.functions.count) - 1; i >= 0; i--)
	{
		FunctionData ref function = scope.functions[i];

		// Keep class functions visible
		if(function.scope.ownerType)
			continue;

		// Always hide local functions
		if(!keepFunctions || (!function.scope.ownerNamespace && GlobalScopeFrom(function.scope) == nullptr))
		{
			if(scope.scope && function.isPrototype && !function.implementation && !keepFunctions)
				Stop(function.source, "ERROR: local function '%.*s' went out of scope unimplemented", FMT_ISTR(function.name.name));

			if(functionMap.find(function.nameHash, function))
			{
				functionMap.remove(function.nameHash, function);

				if(function.isOperator)
					noAssignmentOperatorForTypePair.clear();
			}
		}
	}

	for(int i = int(scope.types.count) - 1; i >= 0; i--)
	{
		TypeBase ref type = scope.types[i];

		if(typeMap.find(type.nameHash, type))
		{
			if(!keepFunctions)
			{
				if(isType with<TypeClass>(type))
				{
					for(int k = 0; k < functions.size(); k++)
					{
						FunctionData ref function = functions[k];

						if(function.scope.ownerType == type && functionMap.find(function.nameHash, function))
						{
							functionMap.remove(function.nameHash, function);

							if(function.isOperator)
								noAssignmentOperatorForTypePair.clear();
						}
					}
				}
			}

			typeMap.remove(type.nameHash, type);
		}
	}

	for(int i = int(scope.aliases.count) - 1; i >= 0; i--)
	{
		AliasData ref alias = scope.aliases[i];

		if(typeMap.find(alias.nameHash, alias.type))
			typeMap.remove(alias.nameHash, alias.type);
	}

	for(int i = int(scope.shadowedVariables.count) - 1; i >= 0; i--)
	{
		VariableData ref variable = scope.shadowedVariables[i];

		if (variable)
			variableMap.insert(variable.nameHash, variable);
	}

	scope = scope.scope;
}

void ExpressionContext:PopScope(ScopeType type)
{
	PopScope(type, true, false);
}

void ExpressionContext:RestoreScopesAtPoint(ScopeData ref target, SynBase ref location)
{
	// Restore parent first, up to the current scope
	if(target.scope != scope)
		RestoreScopesAtPoint(target.scope, location);

	for(int i = 0, e = target.visibleVariables.count; i < e; i++)
	{
		VariableData ref variable = target.visibleVariables.data[i];

		if(!location || variable.importModule != nullptr || variable.source.pos.begin <= location.pos.begin)
			variableMap.insert(variable.nameHash, variable);
	}

	// For functions, restore variable shadowing state and local functions
	for(int i = 0, e = target.functions.count; i < e; i++)
	{
		FunctionData ref function = target.functions.data[i];

		if(function.scope.ownerType)
			continue;

		if(!location || function.importModule != nullptr || function.source.pos.begin <= location.pos.begin)
		{
			VariableData ref ref variable = variableMap.find(function.nameHash);

			while(variable)
			{
				variableMap.remove(function.nameHash, *variable);

				variable = variableMap.find(function.nameHash);
			}

			if(!function.scope.ownerNamespace && GlobalScopeFrom(function.scope) == nullptr)
				functionMap.insert(function.nameHash, function);
		}
	}

	for(int i = 0, e = target.types.count; i < e; i++)
	{
		TypeBase ref type = target.types.data[i];

		if(TypeClass ref exact = getType with<TypeClass>(type))
		{
			if(!location || exact.importModule != nullptr || exact.source.pos.begin <= location.pos.begin)
				typeMap.insert(type.nameHash, type);
		}
		else if(TypeGenericClassProto ref exact = getType with<TypeGenericClassProto>(type))
		{
			if(!location || exact.definition.imported || exact.definition.pos.begin <= location.pos.begin)
				typeMap.insert(type.nameHash, type);
		}
		else
		{
			typeMap.insert(type.nameHash, type);
		}
	}

	for(int i = 0, e = target.aliases.count; i < e; i++)
	{
		AliasData ref alias = target.aliases.data[i];

		if(!location || alias.importModule != nullptr || alias.source.pos.begin <= location.pos.begin)
			typeMap.insert(alias.nameHash, alias.type);
	}

	scope = target;
}

void ExpressionContext:SwitchToScopeAtPoint(ScopeData ref target, SynBase ref targetLocation)
{
	// Reach the same depth
	while(scope.scopeDepth > target.scopeDepth)
		PopScope(scope.type, false, true);

	// Reach the same parent
	ScopeData ref curr = target;

	while(curr.scopeDepth > scope.scopeDepth)
		curr = curr.scope;

	while(scope.scope != curr.scope)
	{
		PopScope(scope.type, false, true);

		curr = curr.scope;
	}

	// We have common parent, but we are in different scopes, go to common parent
	if(scope != curr)
		PopScope(scope.type, false, true);

	// When the common parent is reached, remove it without ejecting namespace variables into the outer scope
	PopScope(scope.type, false, true);

	// Now restore each namespace data up to the source location
	RestoreScopesAtPoint(target, targetLocation);
}

NamespaceData ref ExpressionContext:GetCurrentNamespace(ScopeData ref scopeData)
{
	// Simply walk up the scopes and find the current one
	for(ScopeData ref curr = scopeData; curr; curr = curr.scope)
	{
		if(NamespaceData ref ns = curr.ownerNamespace)
			return ns;
	}

	return nullptr;
}

FunctionData ref ExpressionContext:GetCurrentFunction(ScopeData ref scopeData)
{
	// Walk up, but if we reach a type owner, stop - we're not in a context of a function
	for(ScopeData ref curr = scopeData; curr; curr = curr.scope)
	{
		if(curr.ownerType)
			return nullptr;

		if(FunctionData ref function = curr.ownerFunction)
			return function;
	}

	return nullptr;
}

TypeBase ref ExpressionContext:GetCurrentType(ScopeData ref scopeData)
{
	// Simply walk up the scopes and find the current one
	for(ScopeData ref curr = scopeData; curr; curr = curr.scope)
	{
		if(TypeBase ref type = curr.ownerType)
			return type;
	}

	return nullptr;
}

FunctionData ref ExpressionContext:GetFunctionOwner(ScopeData ref scopeData)
{
	// Temporary scopes have no owner
	if(scopeData.type == ScopeType.SCOPE_TEMPORARY)
		return nullptr;

	// Walk up, but if we reach a type or namespace owner, stop - we're not in a context of a function
	for(ScopeData ref curr = scopeData; curr; curr = curr.scope)
	{
		if(curr.ownerType)
			return nullptr;

		if(curr.ownerNamespace)
			return nullptr;

		if(FunctionData ref function = curr.ownerFunction)
			return function;
	}

	return nullptr;
}

ScopeData ref ExpressionContext:NamespaceScopeFrom(ScopeData ref scopeData)
{
	if(!scopeData || scopeData.ownerNamespace)
		return scopeData;

	return NamespaceScopeFrom(scopeData.scope);
}

ScopeData ref ExpressionContext:GlobalScopeFrom(ScopeData ref scopeData)
{
	if(!scopeData)
		return nullptr;

	if(scopeData.type == ScopeType.SCOPE_TEMPORARY)
		return nullptr;

	if(scopeData.ownerFunction)
		return nullptr;

	if(scopeData.ownerType)
		return nullptr;

	if(scopeData.scope)
		return GlobalScopeFrom(scopeData.scope);

	return scopeData;
}

bool ExpressionContext:IsGenericInstance(FunctionData ref function)
{
	if(function.isGenericInstance)
		return true;

	if(function.proto != nullptr)
		return true;

	if(function.scope.ownerType)
	{
		if(TypeClass ref typeClass = getType with<TypeClass>(function.scope.ownerType))
		{
			if(typeClass.proto)
				return true;
		}
	}

	return false;
}

void ExpressionContext:AddType(TypeBase ref type)
{
	scope.types.push_back(type);

	types.push_back(type);

	if(TypeClass ref typeClass = getType with<TypeClass>(type))
	{
		if(!typeClass.isInternal)
			typeMap.insert(type.nameHash, type);
	}
	else
	{
		typeMap.insert(type.nameHash, type);
	}
}

void ExpressionContext:AddFunction(FunctionData ref function)
{
	scope.functions.push_back(function);

	function.functionIndex = functions.size();

	functions.push_back(function);

	// Don't add internal functions to named lookup
	if(function.name.name && function.name.name[0] != '$')
	{
		functionMap.insert(function.nameHash, function);

		if(function.isOperator)
			noAssignmentOperatorForTypePair.clear();
	}

	VariableData ref ref variable = variableMap.find(function.nameHash);

	while(variable)
	{
		variableMap.remove(function.nameHash, *variable);

		scope.shadowedVariables.push_back(*variable);

		variable = variableMap.find(function.nameHash);
	}
}

void ExpressionContext:AddVariable(VariableData ref variable, bool visible)
{
	scope.variables.push_back(variable);
	scope.allVariables.push_back(variable);

	variables.push_back(variable);

	if(visible)
	{
		scope.visibleVariables.push_back(variable);

		variableMap.insert(variable.nameHash, variable);
	}
}

void ExpressionContext:AddAlias(AliasData ref alias)
{
	scope.aliases.push_back(alias);

	typeMap.insert(alias.nameHash, alias.type);
}

int ExpressionContext:GetTypeIndex(TypeBase ref type)
{
	int index = -1;

	for(int i = 0, e = types.count; i < e; i++)
	{
		if(types.data[i] == type)
		{
			index = i;
			break;
		}
	}

	assert(index != -1);

	return index;
}

int ExpressionContext:GetFunctionIndex(FunctionData ref data)
{
	return data.functionIndex;
}

void ExpressionContext:HideFunction(FunctionData ref function)
{
	// Don't have to remove internal functions since they are never added to lookup
	if(function.name.name && function.name.name[0] != '$')
	{
		functionMap.remove(function.nameHash, function);

		for(int i = int(scope.shadowedVariables.count) - 1; i >= 0; i--)
		{
			VariableData ref variable = scope.shadowedVariables[i];

			if(variable && variable.nameHash == function.nameHash)
			{
				variableMap.insert(variable.nameHash, variable);

				scope.shadowedVariables[i] = nullptr;
			}
		}

		if(function.isOperator)
			noAssignmentOperatorForTypePair.clear();
	}

	vector<FunctionData ref> scopeFunctions = function.scope.functions;

	for(int i = 0; i < scopeFunctions.size(); i++)
	{
		if(scopeFunctions[i] == function)
		{
			scopeFunctions[i] = *scopeFunctions.back();
			scopeFunctions.pop_back();
		}
	}
}

bool ExpressionContext:IsGenericFunction(FunctionData ref function)
{
	if(function.type.isGeneric)
		return true;

	if(function.scope.ownerType && function.scope.ownerType.isGeneric)
		return true;

	for(MatchData ref curr = function.generics.head; curr; curr = curr.next)
	{
		if(curr.type.isGeneric)
			return true;
	}

	return false;
}

bool ExpressionContext:IsIntegerType(TypeBase ref type)
{
	if(type == typeBool)
		return true;

	if(type == typeChar)
		return true;

	if(type == typeShort)
		return true;

	if(type == typeInt)
		return true;

	if(type == typeLong)
		return true;

	return false;
}

bool ExpressionContext:IsFloatingPointType(TypeBase ref type)
{
	if(type == typeFloat)
		return true;

	if(type == typeDouble)
		return true;

	return false;
}

bool ExpressionContext:IsNumericType(TypeBase ref type)
{
	return IsIntegerType(type) || IsFloatingPointType(type);
}

TypeBase ref ExpressionContext:GetBinaryOpResultType(TypeBase ref a, TypeBase ref b)
{
	if(a == typeDouble || b == typeDouble)
		return typeDouble;

	if(a == typeFloat || b == typeFloat)
		return typeFloat;

	if(a == typeLong || b == typeLong)
		return typeLong;

	if(a == typeInt || b == typeInt)
		return typeInt;

	if(a == typeShort || b == typeShort)
		return typeShort;

	if(a == typeChar || b == typeChar)
		return typeChar;

	if(a == typeBool || b == typeBool)
		return typeBool;

	return nullptr;
}

TypeError ref ExpressionContext:GetErrorType()
{
	return new TypeError();
}

TypeRef ref ExpressionContext:GetReferenceType(TypeBase ref type)
{
	// Can't derive from pseudo types
	assert(!isType with<TypeArgumentSet>(type) && !isType with<TypeMemberSet>(type) && !isType with<TypeFunctionSet>(type));
	assert(!isType with<TypeError>(type));

	// Can't create reference to auto this way
	assert(type != typeAuto);

	if(type.refType)
		return type.refType;

	// Create new type
	TypeRef ref result = new TypeRef(GetReferenceTypeName(*this, type), type);

	// Save it for future use
	type.refType = result;

	types.push_back(result);

	return result;
}

TypeArray ref ExpressionContext:GetArrayType(TypeBase ref type, long length)
{
	// Can't have array of void
	assert(type != typeVoid);

	// Can't have array of auto
	assert(type != typeAuto);

	// Can't derive from pseudo types
	assert(!isType with<TypeArgumentSet>(type) && !isType with<TypeMemberSet>(type) && !isType with<TypeFunctionSet>(type));
	assert(!isType with<TypeError>(type));

	if(TypeClass ref typeClass = getType with<TypeClass>(type))
	{
		assert(typeClass.completed && !typeClass.hasFinalizer);
	}

	for(TypeHandle ref curr = type.arrayTypes.head; curr; curr = curr.next)
	{
		if(TypeArray ref typeArray = getType with<TypeArray>(curr.type))
		{
			if(typeArray.length == length)
				return typeArray;
		}
	}

	assert(type.size < 64 * 1024);

	// Create new type
	TypeArray ref result = new TypeArray(GetArrayTypeName(*this, type, length), type, length);

	result.alignment = type.alignment;

	int maximumAlignment = result.alignment < 4 ? 4 : result.alignment;

	if(result.size % maximumAlignment != 0)
	{
		result.padding = maximumAlignment - (result.size % maximumAlignment);
		result.size += result.padding;
	}

	// Save it for future use
	type.arrayTypes.push_back(new TypeHandle(result));

	types.push_back(result);

	return result;
}

TypeUnsizedArray ref ExpressionContext:GetUnsizedArrayType(TypeBase ref type)
{
	// Can't have array of void
	assert(type != typeVoid);

	// Can't create array of auto types this way
	assert(type != typeAuto);

	// Can't derive from pseudo types
	assert(!isType with<TypeArgumentSet>(type) && !isType with<TypeMemberSet>(type) && !isType with<TypeFunctionSet>(type));
	assert(!isType with<TypeError>(type));

	if(type.unsizedArrayType)
		return type.unsizedArrayType;

	assert(type.size < 64 * 1024);

	// Create new type
	TypeUnsizedArray ref result = new TypeUnsizedArray(GetUnsizedArrayTypeName(*this, type), type);

	PushScope(result);

	result.typeScope = scope;

	result.members.push_back(new MemberHandle(nullptr, new VariableData(nullptr, scope, 4, typeInt, new SynIdentifier(InplaceStr("size")), NULLC_PTR_SIZE, uniqueVariableId++), nullptr));
	result.members.tail.variable.isReadonly = true;

	result.alignment = 4;
	result.size = NULLC_PTR_SIZE + 4;

	PopScope(ScopeType.SCOPE_TYPE);

	// Save it for future use
	type.unsizedArrayType = result;

	types.push_back(result);

	return result;
}

TypeFunction ref ExpressionContext:GetFunctionType(SynBase ref source, TypeBase ref returnType, RefList<TypeHandle> arguments)
{
	// Can't derive from pseudo types
	assert(!isType with<TypeArgumentSet>(returnType) && !isType with<TypeMemberSet>(returnType) && !isType with<TypeFunctionSet>(returnType));
	assert(!isType with<TypeError>(returnType));

	for(TypeHandle ref curr = arguments.head; curr; curr = curr.next)
	{
		assert(!isType with<TypeArgumentSet>(curr.type) && !isType with<TypeMemberSet>(curr.type) && !isType with<TypeFunctionSet>(curr.type));
		assert(!isType with<TypeError>(curr.type));

		// Can't have auto as argument
		assert(curr.type != typeAuto);
	}

	for(int i = 0, e = functionTypes.count; i < e; i++)
	{
		if(TypeFunction ref type = functionTypes.data[i])
		{
			if(type.returnType != returnType)
				continue;

			TypeHandle ref leftArg = type.arguments.head;
			TypeHandle ref rightArg = arguments.head;

			while(leftArg && rightArg && leftArg.type == rightArg.type)
			{
				leftArg = leftArg.next;
				rightArg = rightArg.next;
			}

			if(leftArg != rightArg)
				continue;

			return type;
		}
	}

	// Create new type
	TypeFunction ref result = new TypeFunction(GetFunctionTypeName(*this, returnType, arguments), returnType, arguments);

	if(result.name.length() > NULLC_MAX_TYPE_NAME_LENGTH)
		Stop(source, "ERROR: generated function type name exceeds maximum type length '%d'", NULLC_MAX_TYPE_NAME_LENGTH);

	result.alignment = 4;

	functionTypes.push_back(result);
	types.push_back(result);

	return result;
}

TypeFunction ref ExpressionContext:GetFunctionType(SynBase ref source, TypeBase ref returnType, ArrayView<ArgumentData> arguments)
{
	// Can't derive from pseudo types
	assert(!isType with<TypeArgumentSet>(returnType) && !isType with<TypeMemberSet>(returnType) && !isType with<TypeFunctionSet>(returnType));
	assert(!isType with<TypeError>(returnType));

	for(int i = 0; i < arguments.size(); i++)
	{
		TypeBase ref curr = arguments[i].type;

		assert(!isType with<TypeArgumentSet>(curr) && !isType with<TypeMemberSet>(curr) && !isType with<TypeFunctionSet>(curr));
		assert(!isType with<TypeError>(curr));

		// Can't have auto as argument
		assert(curr != typeAuto);
	}

	for(int i = 0, e = functionTypes.count; i < e; i++)
	{
		if(TypeFunction ref type = functionTypes.data[i])
		{
			if(type.returnType != returnType)
				continue;

			TypeHandle ref leftArg = type.arguments.head;

			bool match = true;

			for(int i = 0; i < arguments.size(); i++)
			{
				if(!leftArg || leftArg.type != arguments[i].type)
				{
					match = false;
					break;
				}

				leftArg = leftArg.next;
			}

			if(!match)
				continue;

			if(leftArg)
				continue;

			return type;
		}
	}

	RefList<TypeHandle> argumentTypes;

	for(int i = 0; i < arguments.size(); i++)
		argumentTypes.push_back(new TypeHandle(arguments[i].type));

	return GetFunctionType(source, returnType, argumentTypes);
}

TypeFunctionSet ref ExpressionContext:GetFunctionSetType(ArrayView<TypeBase ref> setTypes)
{
	for(int i = 0, e = functionSetTypes.count; i < e; i++)
	{
		if(TypeFunctionSet ref type = functionSetTypes.data[i])
		{
			TypeHandle ref leftArg = type.types.head;

			bool match = true;

			for(int i = 0; i < setTypes.size(); i++)
			{
				if(!leftArg || leftArg.type != setTypes[i])
				{
					match = false;
					break;
				}

				leftArg = leftArg.next;
			}

			if(!match)
				continue;

			if(leftArg)
				continue;

			return type;
		}
	}

	RefList<TypeHandle> setTypeList;

	for(int i = 0; i < setTypes.size(); i++)
		setTypeList.push_back(new TypeHandle(setTypes[i]));

	// Create new type
	TypeFunctionSet ref result = new TypeFunctionSet(GetFunctionSetTypeName(*this, setTypeList), setTypeList);

	functionSetTypes.push_back(result);

	// This type is not added to export list

	return result;
}

TypeGenericAlias ref ExpressionContext:GetGenericAliasType(SynIdentifier ref baseName)
{
	for(int i = 0, e = genericAliasTypes.count; i < e; i++)
	{
		if(TypeGenericAlias ref type = genericAliasTypes.data[i])
		{
			if(type.baseName.name == baseName.name)
				return type;
		}
	}

	// Create new type
	TypeGenericAlias ref result = new TypeGenericAlias(GetGenericAliasTypeName(*this, baseName.name), baseName);

	genericAliasTypes.push_back(result);
	types.push_back(result);

	return result;
}

TypeGenericClass ref ExpressionContext:GetGenericClassType(SynBase ref source, TypeGenericClassProto ref proto, RefList<TypeHandle> generics)
{
	for(int i = 0, e = genericClassTypes.count; i < e; i++)
	{
		if(TypeGenericClass ref type = genericClassTypes.data[i])
		{
			if(type.proto != proto)
				continue;

			TypeHandle ref leftArg = type.generics.head;
			TypeHandle ref rightArg = generics.head;

			while(leftArg && rightArg && leftArg.type == rightArg.type)
			{
				leftArg = leftArg.next;
				rightArg = rightArg.next;
			}

			if(leftArg != rightArg)
				continue;

			return type;
		}
	}

	// Create new type
	TypeGenericClass ref result = new TypeGenericClass(GetGenericClassTypeName(*this, proto, generics), proto, generics);

	if(result.name.length() > NULLC_MAX_TYPE_NAME_LENGTH)
		Stop(source, "ERROR: generated generic type name exceeds maximum type length '%d'", NULLC_MAX_TYPE_NAME_LENGTH);

	genericClassTypes.push_back(result);
	types.push_back(result);

	return result;
}

ModuleData ref ExpressionContext:GetSourceOwner(LexemeRef lexeme)
{
	// Fast check for current module
	/*if(lexeme.pos >= code && lexeme.pos <= codeEnd)
		return nullptr;

	for(int i = 0; i < dependencies.size(); i++)
	{
		ModuleData ref moduleData = dependencies[i];

		if(lexeme >= moduleData.lexStream && lexeme <= moduleData.lexStream + moduleData.lexStreamSize)
			return moduleData;
	}*/

	// Should not get here
	return nullptr;
}

SynInternal ref ExpressionContext:MakeInternal(SynBase ref source)
{
	if(SynInternal ref synInternal = getType with<SynInternal>(source))
		return synInternal;

	return new SynInternal(source);
}
