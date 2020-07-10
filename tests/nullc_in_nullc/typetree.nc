import std.hashmap;
import parsetree;
import stringutil;
import nullcdef;
import common;

class ByteCode;

class ExprBase;
class ExprSequence;
class ExpressionContext;

class TypeBase;
class TypeStruct;
class TypeRef;
class TypeArray;
class TypeUnsizedArray;
class TypeFunction;
class TypeClass;

class ScopeData;
class NamespaceData;
class VariableData;
class FunctionData;
class AliasData;

class VmConstant;
class VmFunction;

class VmLoweredInstruction;

class RegVmLoweredInstruction;

class VariableHandle
{
	void VariableHandle(SynBase ref source, VariableData ref variable)
	{
		this.source = source;
		this.variable = variable;
	}

	SynBase ref source;

	VariableData ref variable;

	VariableHandle ref next;
	bool listed;
}

class FunctionHandle
{
	void FunctionHandle(FunctionData ref function)
	{
		this.function = function;
	}

	FunctionData ref function;

	FunctionHandle ref next;
	bool listed;
}

class TypeHandle
{
	void TypeHandle(TypeBase ref type)
	{
		this.type = type;
	}

	TypeBase ref type;

	TypeHandle ref next;
	bool listed;
}

class MemberHandle
{
	void MemberHandle(SynBase ref source, VariableData ref variable, SynBase ref initializer)
	{
		this.source = source;
		this.variable = variable;
		this.initializer = initializer;
	}

	SynBase ref source;

	VariableData ref variable;

	SynBase ref initializer;

	MemberHandle ref next;
	bool listed;
}

class ShortFunctionHandle
{
	void ShortFunctionHandle(FunctionData ref function, ExprBase ref context)
	{
		this.function = function;
		this.context = context;
	}

	FunctionData ref function;
	ExprBase ref context;

	ShortFunctionHandle ref next;
	bool listed;
}

class ModuleData
{
	void ModuleData(SynBase ref source, InplaceStr name)
	{
		this.source = source;
		this.name = name;
	}

	SynBase ref source;

	InplaceStr name;

	int importIndex;
	int dependencyIndex;

	ByteCode ref bytecode;

	Lexer ref lexer;
	Lexeme ref lexStream;
	int lexStreamSize;

	int startingFunctionIndex;
	int importedFunctionCount;
	int moduleFunctionCount;

	int startingDependencyIndex;
}

class NamespaceData
{
	void NamespaceData(SynBase ref source, ScopeData ref scope, NamespaceData ref parent, SynIdentifier name, int uniqueId)
	{
		this.source = source;
		this.scope = scope;
		this.parent = parent;
		this.name = name;
		this.uniqueId = uniqueId;

		nameHash = name.name.hash();

		if(parent)
			fullNameHash = StringHashContinue(StringHashContinue(parent.fullNameHash, "."), name.name);
		else
			fullNameHash = nameHash;
	}

	SynBase ref source;

	ScopeData ref scope;

	NamespaceData ref parent;

	vector<NamespaceData ref> children;

	SynIdentifier name;
	int nameHash;

	int fullNameHash;

	int uniqueId;
}

class VariableData
{
	void VariableData(SynBase ref source, ScopeData ref scope, int alignment, TypeBase ref type, SynIdentifier ref name, int offset, int uniqueId)
	{
		this.source = source;
		this.scope = scope;
		this.alignment = alignment;
		this.type = type;
		this.name = name;
		this.offset = offset;
		this.uniqueId = uniqueId;

		nameHash = name.name.hash();

		if(alignment != 0)
			assert(offset % alignment == 0);
	}

	SynBase ref source;

	ModuleData ref importModule;

	ScopeData ref scope;

	int alignment;

	TypeBase ref type;

	SynIdentifier ref name;
	int nameHash;

	bool isReference;
	bool isReadonly;

	bool usedAsExternal;

	bool lookupOnly;

	bool isAlloca;
	bool isVmAlloca;

	bool isVmRegSpill;

	int offset;

	int uniqueId;

	// Data for IR module construction
	vector<VmConstant ref> users;
	hashmap<int, VmConstant ref> offsetUsers;

	// Data for VM module construction
	vector<VmLoweredInstruction ref> lowUsers;

	// Data for RegVm module construction
	vector<RegVmLoweredInstruction ref> regVmUsers;
}

int hash_value(VariableData ref key)
{
	return key.uniqueId;
}

class MatchData
{
	void MatchData(SynIdentifier ref name, TypeBase ref type)
	{
		this.name = name;
		this.type = type;

		assert(name != nullptr);
	}

	SynIdentifier ref name;
	TypeBase ref type;

	MatchData ref next;
	bool listed;
}

class ArgumentData
{
	void ArgumentData()
	{
	}

	void ArgumentData(SynBase ref source, bool isExplicit, SynIdentifier ref name, TypeBase ref type, ExprBase ref value)
	{
		this.source = source;
		this.isExplicit = isExplicit;
		this.name = name;
		this.type = type;
		this.value = value;
	}

	SynBase ref source;
	bool isExplicit;
	SynIdentifier ref name;
	TypeBase ref type;
	ExprBase ref value;
	FunctionData ref valueFunction;
}

class CallArgumentData
{
	void CallArgumentData()
	{
	}

	void CallArgumentData(TypeBase ref type, ExprBase ref value)
	{
		this.type = type;
		this.value = value;
	}

	TypeBase ref type;
	ExprBase ref value;
}

class UpvalueData
{
	void UpvalueData(VariableData ref variable, VariableData ref target, VariableData ref nextUpvalue, VariableData ref copy)
	{
		this.variable = variable;
		this.target = target;
		this.nextUpvalue = nextUpvalue;
		this.copy = copy;
	}

	VariableData ref variable;
	VariableData ref target;
	VariableData ref nextUpvalue;
	VariableData ref copy;

	UpvalueData ref next;
	bool listed;
}

class CoroutineStateData
{
	void CoroutineStateData(VariableData ref variable, VariableData ref storage)
	{
		this.variable = variable;
		this.storage = storage;
	}

	VariableData ref variable;
	VariableData ref storage;

	CoroutineStateData ref next;
	bool listed;
}

enum CloseUpvaluesType
{
	CLOSE_UPVALUES_FUNCTION,
	CLOSE_UPVALUES_BLOCK,
	CLOSE_UPVALUES_BREAK,
	CLOSE_UPVALUES_CONTINUE,
	CLOSE_UPVALUES_ARGUMENT
}

class CloseUpvaluesData
{
	void CloseUpvaluesData(ExprSequence ref expr, CloseUpvaluesType type, SynBase ref source, ScopeData ref scope, int depth)
	{
		this.expr = expr;
		this.type = type;
		this.source = source;
		this.scope = scope;
		this.depth = depth;
	}

	ExprSequence ref expr;

	CloseUpvaluesType type;
	SynBase ref source;
	ScopeData ref scope;
	int depth;

	CloseUpvaluesData ref next;
	bool listed;
}

class FunctionData
{
	void FunctionData(SynBase ref source, ScopeData ref scope, bool isCoroutine, bool accessor, bool isOperator, TypeFunction ref type, TypeBase ref contextType, SynIdentifier ref name, RefList<MatchData> generics, int uniqueId)
	{
		this.source = source;
		this.scope = scope;
		this.isCoroutine = isCoroutine;
		this.accessor = accessor;
		this.isOperator = isOperator;
		this.type = type;
		this.contextType = contextType;
		this.name = name;
		this.generics = generics;
		this.uniqueId = uniqueId;

		isInternal = false;

		isHidden = false;

		nameHash = name.name.hash();

		functionIndex = -1;

		isPrototype = false;
		implementation = nullptr;

		proto = nullptr;
		isGenericInstance = false;

		definition = nullptr;

		declaration = nullptr;

		functionScope = nullptr;
		argumentsSize = 0;
		stackSize = 0;

		contextArgument = nullptr;

		coroutineJumpOffset = nullptr;

		yieldCount = 0;

		hasExplicitReturn = false;

		vmFunction = nullptr;

		nextTranslateRestoreBlock = 0;
	}

	SynBase ref source;

	ModuleData ref importModule;

	bool isInternal;

	bool isHidden;

	ScopeData ref scope;

	bool isCoroutine;
	bool accessor;
	bool isOperator;

	TypeFunction ref type;

	TypeBase ref contextType;

	SynIdentifier ref name;
	int nameHash;

	int functionIndex;

	// Explicit generic function type arguments
	RefList<MatchData> generics;

	int uniqueId;

	// All function generic type aliases including explicit type arguments and type aliases from argument types
	RefList<MatchData> aliases;

	vector<ArgumentData> arguments;

	bool isPrototype;
	FunctionData ref implementation;

	FunctionData ref proto;
	bool isGenericInstance;

	vector<FunctionData ref> instances;

	SynFunctionDefinition ref definition;
	LexemeRef delayedDefinition;

	ExprBase ref declaration;

	// Scope where function variables reside
	ScopeData ref functionScope;
	long argumentsSize;
	long stackSize;

	// Variables for arguments
	RefList<VariableHandle> argumentVariables;

	// Variable for the argument containing reference to function context
	VariableData ref contextArgument;

	VariableData ref coroutineJumpOffset;

	RefList<UpvalueData> upvalues;

	hashmap<VariableData ref, UpvalueData ref> upvalueVariableMap;
	hashmap<InplaceStr, bool> upvalueNameSet;

	RefList<CoroutineStateData> coroutineState;

	hashmap<VariableData ref, CoroutineStateData ref> coroutineStateVariableMap;
	hashmap<InplaceStr, bool> coroutineStateNameSet;

	int yieldCount;

	bool hasExplicitReturn;

	RefList<CloseUpvaluesData> closeUpvalues;

	// Data for IR module construction
	VmFunction ref vmFunction;

	int nextTranslateRestoreBlock;
}

class AliasData
{
	void AliasData(SynBase ref source, ScopeData ref scope, TypeBase ref type, SynIdentifier ref name, int uniqueId)
	{
		this.source = source;
		this.scope = scope;
		this.type = type;
		this.name = name;
		this.uniqueId = uniqueId;

		importModule = nullptr;

		nameHash = name.name.hash();
	}

	SynBase ref source;

	ModuleData ref importModule;

	ScopeData ref scope;

	TypeBase ref type;

	SynIdentifier ref name;
	int nameHash;

	int uniqueId;
}

enum ScopeType
{
	SCOPE_EXPLICIT,
	SCOPE_NAMESPACE,
	SCOPE_FUNCTION,
	SCOPE_TYPE,
	SCOPE_LOOP,
	SCOPE_TEMPORARY
}

class ScopeData
{
	void ScopeData(ScopeData ref scope, int uniqueId, ScopeType type)
	{
		this.scope = scope;
		this.uniqueId = uniqueId;
		this.type = type;

		scopeDepth = scope ? scope.scopeDepth + 1 : 0;
		breakDepth = scope ? scope.breakDepth : 0;
		contiueDepth = scope ? scope.contiueDepth : 0;

		startOffset = 0;
		dataSize = 0;
	}

	void ScopeData(ScopeData ref scope, int uniqueId, NamespaceData ref ownerNamespace)
	{
		this.scope = scope;
		this.uniqueId = uniqueId;
		this.ownerNamespace = ownerNamespace;

		type = ScopeType.SCOPE_NAMESPACE;

		scopeDepth = scope ? scope.scopeDepth + 1 : 0;
		breakDepth = 0;
		contiueDepth = 0;

		startOffset = 0;
		dataSize = 0;
	}

	void ScopeData(ScopeData ref scope, int uniqueId, FunctionData ref ownerFunction)
	{
		this.scope = scope;
		this.uniqueId = uniqueId;
		this.ownerFunction = ownerFunction;

		type = ScopeType.SCOPE_FUNCTION;

		scopeDepth = scope ? scope.scopeDepth + 1 : 0;
		breakDepth = 0;
		contiueDepth = 0;

		startOffset = 0;
		dataSize = 0;
	}

	void ScopeData(ScopeData ref scope, int uniqueId, TypeBase ref ownerType)
	{
		this.scope = scope;
		this.uniqueId = uniqueId;
		this.ownerType = ownerType;

		type = ScopeType.SCOPE_TYPE;

		scopeDepth = scope ? scope.scopeDepth + 1 : 0;
		breakDepth = 0;
		contiueDepth = 0;

		startOffset = 0;
		dataSize = 0;
	}

	ScopeData ref scope;

	int uniqueId;

	ScopeType type;
	
	NamespaceData ref ownerNamespace;
	FunctionData ref ownerFunction;
	TypeBase ref ownerType;

	int scopeDepth;
	int breakDepth;
	int contiueDepth;

	long startOffset;
	long dataSize;

	vector<TypeBase ref> types;
	vector<FunctionData ref> functions;
	vector<VariableData ref> variables;
	vector<AliasData ref> aliases;
	vector<ScopeData ref> scopes;

	// Set of variables that are accessible from code
	vector<VariableData ref> visibleVariables;

	// Set of variables that are currently shadowed by function names
	vector<VariableData ref> shadowedVariables;
	
	// Full set of variables including all from nested scopes that have been closed
	vector<VariableData ref> allVariables;
}

class FunctionValue
{
	void FunctionValue()
	{
	}

	void FunctionValue(SynBase ref source, FunctionData ref function, ExprBase ref context)
	{
		this.source = source;
		this.function = function;
		this.context = context;

		assert(context != nullptr);
	}

	SynBase ref source;
	FunctionData ref function;
	ExprBase ref context;
}

bool bool(FunctionValue value)
{
	return value.function != nullptr;
}

bool operator!(FunctionValue value)
{
	return value.function == nullptr;
}

class ConstantData
{
	void ConstantData(SynIdentifier ref name, ExprBase ref value)
	{
		this.name = name;
		this.value = value;
	}

	SynIdentifier ref name;
	ExprBase ref value;

	ConstantData ref next;
	bool listed;
}
/*
template<typename T>
int GetTypeAlignment()
{
	class Helper
	{
		char x;
		T y;
	}

	return sizeof(Helper) - sizeof(T);
}

namespace TypeNode
{
	enum TypeNodeId
	{
		TypeError,
		TypeVoid,
		TypeBool,
		TypeChar,
		TypeShort,
		TypeInt,
		TypeLong,
		TypeFloat,
		TypeDouble,
		TypeTypeID,
		TypeFunctionID,
		TypeNullptr,
		TypeGeneric,
		TypeGenericAlias,
		TypeAuto,
		TypeStruct,
		TypeAutoRef,
		TypeAutoArray,
		TypeRef,
		TypeArray,
		TypeUnsizedArray,
		TypeFunction,
		TypeGenericClassProto,
		TypeGenericClass,
		TypeClass,
		TypeEnum,
		TypeFunctionSet,
		TypeArgumentSet,
		TypeMemberSet
	}
}
*/
class TypeBase extendable
{
	void TypeBase(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
	}

	InplaceStr name;
	int nameHash;

	ModuleData ref importModule;

	int typeIndex = -1;
	
	long size;
	int alignment;
	int padding;

	bool isGeneric;
	bool hasPointers;

	bool hasTranslation;

	TypeRef ref refType; // Reference type to this type
	RefList<TypeHandle> arrayTypes; // Array types derived from this type
	TypeUnsizedArray ref unsizedArrayType; // An unsized array type derived from this type
}

class TypeError: TypeBase
{
	void TypeError()
	{
		this.name = InplaceStr("%error-type%");
		nameHash = GetStringHash(name.data, name.begin, name.end);
	}
}

class TypeVoid: TypeBase
{
	void TypeVoid(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		size = 0;
		alignment = 0;
	}
}

class TypeBool: TypeBase
{
	void TypeBool(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		size = 1;
	}
}

class TypeChar: TypeBase
{
	void TypeChar(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = 1;
	}
}

class TypeShort: TypeBase
{
	void TypeShort(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = 2;
		alignment = 2;
	}
}

class TypeInt: TypeBase
{
	void TypeInt(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = 4;
		alignment = 4;
	}
}

class TypeLong: TypeBase
{
	void TypeLong(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = 8;
		alignment = 8;
	}
}

class TypeFloat: TypeBase
{
	void TypeFloat(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = 4;
		alignment = 4;
	}
}

class TypeDouble: TypeBase
{
	void TypeDouble(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = 8;
		alignment = 8;
	}
}

class TypeTypeID: TypeBase
{
	void TypeTypeID(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = 4;
		alignment = 4;
	}
}

class TypeFunctionID: TypeBase
{
	void TypeFunctionID(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = 4;
		alignment = 4;
	}
}

class TypeNullptr: TypeBase
{
	void TypeNullptr(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		size = NULLC_PTR_SIZE;
		alignment = 4;
	}
}

class TypeGeneric: TypeBase
{
	void TypeGeneric(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		isGeneric = true;
	}
}

class TypeGenericAlias: TypeBase
{
	void TypeGenericAlias(InplaceStr name, SynIdentifier ref baseName)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.baseName = baseName;
		
		assert(baseName != nullptr);

		isGeneric = true;
	}

	SynIdentifier ref baseName;
}

class TypeAuto: TypeBase
{
	void TypeAuto(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
	}
}

class TypeStruct: TypeBase
{
	void TypeStruct(int myTypeID, InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		typeScope = nullptr;
	}

	// Scope where class members reside
	ScopeData ref typeScope;

	RefList<MemberHandle> members;

	RefList<ConstantData> constants;
}

class TypeAutoRef: TypeStruct
{
	void TypeAutoRef(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		hasPointers = true;
	}
}

class TypeAutoArray: TypeStruct
{
	void TypeAutoArray(InplaceStr name)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		hasPointers = true;
	}
}

class TypeRef: TypeBase
{
	void TypeRef(InplaceStr name, TypeBase ref subType)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.subType = subType;

		size = NULLC_PTR_SIZE;
		alignment = 4;

		isGeneric = subType.isGeneric;

		hasPointers = true;
	}

	TypeBase ref subType;
}

class TypeArray: TypeBase
{
	void TypeArray(InplaceStr name, TypeBase ref subType, long length)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.subType = subType;
		this.length = length;

		size = subType.size * length;

		isGeneric = subType.isGeneric;

		hasPointers = subType.hasPointers;
	}

	TypeBase ref subType;
	long length;
}

class TypeUnsizedArray: TypeStruct
{
	void TypeUnsizedArray(InplaceStr name, TypeBase ref subType)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.subType = subType;

		isGeneric = subType.isGeneric;

		hasPointers = true;
	}

	TypeBase ref subType;
}

class TypeFunction: TypeBase
{
	void TypeFunction(InplaceStr name, TypeBase ref returnType, RefList<TypeHandle> arguments)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.returnType = returnType;
		this.arguments = arguments;

		size = 4 + NULLC_PTR_SIZE;

		isGeneric = returnType.isGeneric;

		for(TypeHandle ref el = arguments.head; el; el = el.next)
			isGeneric |= el.type.isGeneric;

		hasPointers = true;
	}

	TypeBase ref returnType;
	RefList<TypeHandle> arguments;
}

class TypeGenericClassProto: TypeBase
{
	void TypeGenericClassProto(SynIdentifier identifier, SynBase ref source, ScopeData ref scope, SynClassDefinition ref definition)
	{
		this.name = identifier.name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.identifier = identifier;
		this.source = source;
		this.scope = scope;
		this.definition = definition;

		isGeneric = true;
	}

	SynIdentifier identifier;

	SynBase ref source;

	ScopeData ref scope;

	SynClassDefinition ref definition;

	vector<ExprBase ref> instances;
}

class TypeGenericClass: TypeBase
{
	void TypeGenericClass(InplaceStr name, TypeGenericClassProto ref proto, RefList<TypeHandle> generics)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.proto = proto;
		this.generics = generics;

		isGeneric = true;
	}

	TypeGenericClassProto ref proto;

	RefList<TypeHandle> generics;
}

class TypeClass: TypeStruct
{
	void TypeClass(SynIdentifier identifier, SynBase ref source, ScopeData ref scope, TypeGenericClassProto ref proto, RefList<MatchData> generics, bool isExtendable, TypeClass ref baseClass)
	{
		this.name = identifier.name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.identifier = identifier;
		this.source = source;
		this.scope = scope;
		this.proto = proto;
		this.generics = generics;
		this.isExtendable = isExtendable;
		this.baseClass = baseClass;

		completed = false;
		isInternal = false;

		hasFinalizer = false;
	}

	SynIdentifier identifier;

	SynBase ref source;

	ScopeData ref scope;

	TypeGenericClassProto ref proto;

	RefList<MatchData> generics;

	RefList<MatchData> aliases;

	bool isExtendable;

	TypeClass ref baseClass;

	bool completed;
	bool isInternal;

	bool hasFinalizer;
}

class TypeEnum: TypeStruct
{
	void TypeEnum(SynIdentifier identifier, SynBase ref source, ScopeData ref scope)
	{
		this.name = identifier.name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		this.identifier = identifier;
		this.source = source;
		this.scope = scope;

		size = 4;
		alignment = 4;
	}

	SynIdentifier identifier;

	SynBase ref source;

	ScopeData ref scope;
}

class TypeFunctionSet: TypeBase
{
	void TypeFunctionSet(InplaceStr name, RefList<TypeHandle> types)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.types = types;

		isGeneric = true;
	}

	RefList<TypeHandle> types;
}

class TypeArgumentSet: TypeBase
{
	void TypeArgumentSet(InplaceStr name, RefList<TypeHandle> types)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);

		this.types = types;

		isGeneric = true;
	}

	RefList<TypeHandle> types;
}

class TypeMemberSet: TypeBase
{
	void TypeMemberSet(InplaceStr name, TypeStruct ref type)
	{
		this.name = name;
		nameHash = GetStringHash(name.data, name.begin, name.end);
		
		this.type = type;

		isGeneric = true;
	}

	TypeStruct ref type;
}

bool isType<@T>(TypeBase ref node)
{
	return node && typeid(node) == T;
}

auto getType<@T>(TypeBase ref node)
{
	if(T == TypeStruct)
	{
		if(node && (typeid(node) == TypeAutoRef || typeid(node) == TypeAutoArray || typeid(node) == TypeUnsizedArray || typeid(node) == TypeClass || typeid(node) == TypeEnum))
			return (T ref)(node);
	}
	
	if(node && typeid(node) == T)
		return (T ref)(node);

	return nullptr;
}
