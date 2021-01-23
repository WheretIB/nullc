#pragma once

#include "stdafx.h"
#include "Array.h"
#include "DenseMap.h"
#include "IntrusiveList.h"
#include "StrAlgo.h"
#include "ParseTree.h"
#include "Trace.h"

struct ByteCode;

struct ExprBase;
struct ExprSequence;
struct ExpressionContext;

struct TypeBase;
struct TypeStruct;
struct TypeRef;
struct TypeArray;
struct TypeUnsizedArray;
struct TypeFunction;
struct TypeClass;

struct ScopeData;
struct NamespaceData;
struct VariableData;
struct FunctionData;
struct AliasData;

struct VmConstant;
struct VmFunction;

struct RegVmLoweredInstruction;

struct VariableHandle
{
	VariableHandle(SynBase *source, VariableData *variable): source(source), variable(variable), next(0), listed(false)
	{
	}

	SynBase *source;

	VariableData *variable;

	VariableHandle *next;
	bool listed;
};

struct FunctionHandle
{
	FunctionHandle(FunctionData *function): function(function), next(0), listed(false)
	{
	}

	FunctionData *function;

	FunctionHandle *next;
	bool listed;
};

struct TypeHandle
{
	TypeHandle(TypeBase *type): type(type), next(0), listed(false)
	{
	}

	TypeBase *type;

	TypeHandle *next;
	bool listed;
};

struct MemberHandle
{
	MemberHandle(SynBase *source, VariableData *variable, SynBase *initializer) : source(source), variable(variable), initializer(initializer), next(0), listed(false)
	{
	}

	SynBase *source;

	VariableData *variable;

	SynBase *initializer;

	MemberHandle *next;
	bool listed;
};

struct ShortFunctionHandle
{
	ShortFunctionHandle(FunctionData *function, ExprBase *context) : function(function), context(context), next(0), listed(false)
	{
	}

	FunctionData *function;
	ExprBase *context;

	ShortFunctionHandle *next;
	bool listed;
};

struct ModuleData
{
	ModuleData(SynBase *source, InplaceStr name): source(source), name(name)
	{
		importIndex = 0;

		bytecode = NULL;

		lexer = NULL;
		lexStream = NULL;
		lexStreamSize = 0;

		startingFunctionIndex = 0;
		importedFunctionCount = 0;
		moduleFunctionCount = 0;
	}

	SynBase *source;

	InplaceStr name;

	unsigned importIndex;

	ByteCode* bytecode;

	Lexer *lexer;
	Lexeme* lexStream;
	unsigned lexStreamSize;

	unsigned startingFunctionIndex;
	unsigned importedFunctionCount;
	unsigned moduleFunctionCount;
};

struct NamespaceData
{
	NamespaceData(Allocator *allocator, SynBase *source, ScopeData *scope, NamespaceData *parent, const SynIdentifier& name, unsigned uniqueId): source(source), scope(scope), parent(parent), children(allocator), name(name), uniqueId(uniqueId)
	{
		nameHash = name.name.hash();

		if(parent)
			fullNameHash = NULLC::StringHashContinue(NULLC::StringHashContinue(parent->fullNameHash, "."), name.name.begin, name.name.end);
		else
			fullNameHash = nameHash;
	}

	SynBase *source;

	ScopeData *scope;

	NamespaceData *parent;

	SmallArray<NamespaceData*, 2> children;

	SynIdentifier name;
	unsigned nameHash;

	unsigned fullNameHash;

	unsigned uniqueId;
};

struct IntHasher
{
	unsigned operator()(int key)
	{
		return key * 2654435761u;
	}
};

struct VariableData
{
	VariableData(Allocator *allocator, SynBase *source, ScopeData *scope, unsigned alignment, TypeBase *type, SynIdentifier *name, unsigned offset, unsigned uniqueId): source(source), scope(scope), alignment(alignment), type(type), name(name), offset(offset), uniqueId(uniqueId), users(allocator), offsetUsers(allocator), regVmUsers(allocator)
	{
		importModule = NULL;

		nameHash = name->name.hash();

		isReference = false;
		isReadonly = false;

		usedAsExternal = false;

		lookupOnly = false;

		isAlloca = false;
		isVmAlloca = false;

		isVmRegSpill = false;

		if(alignment != 0)
			assert(offset % alignment == 0);
	}

	SynBase *source;

	ModuleData *importModule;

	ScopeData *scope;

	unsigned alignment;

	TypeBase *type;

	SynIdentifier *name;
	unsigned nameHash;

	bool isReference;
	bool isReadonly;

	bool usedAsExternal;

	bool lookupOnly;

	bool isAlloca;
	bool isVmAlloca;

	bool isVmRegSpill;

	unsigned offset;

	unsigned uniqueId;

	// Data for IR module construction
	SmallArray<VmConstant*, 8> users;
	SmallDenseMap<int, VmConstant*, IntHasher, 8> offsetUsers;

	// Data for RegVm module construction
	SmallArray<RegVmLoweredInstruction*, 4> regVmUsers;
};

struct VariableDataHasher
{
	unsigned operator()(VariableData* key)
	{
		return key->uniqueId;
	}
};

struct MatchData
{
	MatchData(SynIdentifier* name, TypeBase *type): name(name), type(type), next(0), listed(false)
	{
		assert(name);
	}

	SynIdentifier* name;
	TypeBase *type;

	MatchData *next;
	bool listed;
};

struct ArgumentData
{
	ArgumentData(): source(0), isExplicit(false), name(0), type(0), value(0), valueFunction(0)
	{
	}

	ArgumentData(SynBase *source, bool isExplicit, SynIdentifier *name, TypeBase *type, ExprBase *value): source(source), isExplicit(isExplicit), name(name), type(type), value(value), valueFunction(0)
	{
	}

	SynBase *source;
	bool isExplicit;
	SynIdentifier *name;
	TypeBase *type;
	ExprBase *value;
	FunctionData *valueFunction;
};

struct CallArgumentData
{
	CallArgumentData(): type(0), value(0)
	{
	}

	CallArgumentData(TypeBase *type, ExprBase *value): type(type), value(value)
	{
	}

	TypeBase *type;
	ExprBase *value;
};

struct UpvalueData
{
	UpvalueData(VariableData *variable, VariableData *target, VariableData *nextUpvalue, VariableData *copy): variable(variable), target(target), nextUpvalue(nextUpvalue), copy(copy), next(0), listed(false)
	{
	}

	VariableData *variable;
	VariableData *target;
	VariableData *nextUpvalue;
	VariableData *copy;

	UpvalueData *next;
	bool listed;
};

struct CoroutineStateData
{
	CoroutineStateData(VariableData *variable, VariableData *storage) : variable(variable), storage(storage), next(0), listed(false)
	{
	}

	VariableData *variable;
	VariableData *storage;

	CoroutineStateData *next;
	bool listed;
};

enum CloseUpvaluesType
{
	CLOSE_UPVALUES_FUNCTION,
	CLOSE_UPVALUES_BLOCK,
	CLOSE_UPVALUES_BREAK,
	CLOSE_UPVALUES_CONTINUE,
	CLOSE_UPVALUES_ARGUMENT
};

struct CloseUpvaluesData
{
	CloseUpvaluesData(ExprSequence *expr, CloseUpvaluesType type, SynBase *source, ScopeData *scope, unsigned depth): expr(expr), type(type), source(source), scope(scope), depth(depth), next(0), listed(false)
	{
	}

	ExprSequence *expr;

	CloseUpvaluesType type;
	SynBase *source;
	ScopeData *scope;
	unsigned depth;

	CloseUpvaluesData *next;
	bool listed;
};

struct FunctionData
{
	FunctionData(Allocator *allocator, SynBase *source, ScopeData *scope, bool coroutine, bool accessor, bool isOperator, TypeFunction *type, TypeBase *contextType, SynIdentifier *name, IntrusiveList<MatchData> generics, unsigned uniqueId): source(source), scope(scope), coroutine(coroutine), accessor(accessor), isOperator(isOperator), type(type), contextType(contextType), name(name), generics(generics), uniqueId(uniqueId), arguments(allocator), instances(allocator), upvalueVariableMap(allocator), upvalueNameSet(allocator), coroutineStateVariableMap(allocator), coroutineStateNameSet(allocator)
	{
		importModule = 0;

		isInternal = false;

		isHidden = false;

		attributes = 0;

		nameHash = name->name.hash();

		functionIndex = ~0u;

		isPrototype = false;
		implementation = NULL;

		proto = NULL;
		isGenericInstance = false;

		definition = NULL;
		delayedDefinition = NULL;

		declaration = NULL;

		functionScope = NULL;
		argumentsSize = 0;
		stackSize = 0;

		contextArgument = NULL;

		coroutineJumpOffset = NULL;

		yieldCount = 0;

		hasExplicitReturn = false;

		vmFunction = NULL;

		nextTranslateRestoreBlock = 0;
	}

	SynBase *source;

	ModuleData *importModule;

	bool isInternal;

	bool isHidden;

	unsigned attributes;

	ScopeData *scope;

	bool coroutine;
	bool accessor;
	bool isOperator;

	TypeFunction *type;

	TypeBase *contextType;

	SynIdentifier *name;
	unsigned nameHash;

	unsigned functionIndex;

	// Explicit generic function type arguments
	IntrusiveList<MatchData> generics;

	unsigned uniqueId;

	// All function generic type aliases including explicit type arguments and type aliases from argument types
	IntrusiveList<MatchData> aliases;

	SmallArray<ArgumentData, 4> arguments;

	bool isPrototype;
	FunctionData *implementation;

	FunctionData *proto;
	bool isGenericInstance;

	SmallArray<FunctionData*, 8> instances;

	SynFunctionDefinition *definition;
	Lexeme *delayedDefinition;

	ExprBase *declaration;

	// Scope where function variables reside
	ScopeData *functionScope;
	long long argumentsSize;
	long long stackSize;

	// Variables for arguments
	IntrusiveList<VariableHandle> argumentVariables;

	// Variable for the argument containing reference to function context
	VariableData *contextArgument;

	VariableData *coroutineJumpOffset;

	IntrusiveList<UpvalueData> upvalues;

	SmallDenseMap<VariableData*, UpvalueData*, VariableDataHasher, 2> upvalueVariableMap;
	SmallDenseSet<InplaceStr, InplaceStrHasher, 2> upvalueNameSet;

	IntrusiveList<CoroutineStateData> coroutineState;

	SmallDenseMap<VariableData*, CoroutineStateData*, VariableDataHasher, 2> coroutineStateVariableMap;
	SmallDenseSet<InplaceStr, InplaceStrHasher, 2> coroutineStateNameSet;

	unsigned yieldCount;

	bool hasExplicitReturn;

	IntrusiveList<CloseUpvaluesData> closeUpvalues;

	// Data for IR module construction
	VmFunction *vmFunction;

	unsigned nextTranslateRestoreBlock;
};

struct AliasData
{
	AliasData(SynBase *source, ScopeData *scope, TypeBase *type, SynIdentifier *name, unsigned uniqueId): source(source), scope(scope), type(type), name(name), uniqueId(uniqueId)
	{
		importModule = NULL;

		nameHash = name->name.hash();
	}

	SynBase *source;

	ModuleData *importModule;

	ScopeData *scope;

	TypeBase *type;

	SynIdentifier *name;
	unsigned nameHash;

	unsigned uniqueId;
};

enum ScopeType
{
	SCOPE_EXPLICIT,
	SCOPE_NAMESPACE,
	SCOPE_FUNCTION,
	SCOPE_TYPE,
	SCOPE_LOOP,
	SCOPE_TEMPORARY
};

struct IdentifierLookupResult
{
	IdentifierLookupResult() : variable(0), function(0)
	{
	}

	explicit IdentifierLookupResult(VariableData *variable) : variable(variable), function(0)
	{
	}

	explicit IdentifierLookupResult(FunctionData *function) : variable(0), function(function)
	{
	}

	bool operator==(const IdentifierLookupResult &rhs) const
	{
		return variable == rhs.variable && function == rhs.function;
	}

	bool operator!=(const IdentifierLookupResult &rhs) const
	{
		return variable != rhs.variable || function != rhs.function;
	}

	VariableData *variable;
	FunctionData *function;
};

struct TypeLookupResult
{
	TypeLookupResult() : type(0), alias(0)
	{
	}

	explicit TypeLookupResult(TypeBase *type) : type(type), alias(0)
	{
	}

	explicit TypeLookupResult(AliasData *alias) : type(0), alias(alias)
	{
	}

	bool operator==(const TypeLookupResult &rhs) const
	{
		return type == rhs.type && alias == rhs.alias;
	}

	bool operator!=(const TypeLookupResult &rhs) const
	{
		return type != rhs.type || alias != rhs.alias;
	}

	TypeBase *type;
	AliasData *alias;
};

struct ScopeData
{
	ScopeData(Allocator *allocator, ScopeData *scope, unsigned uniqueId, ScopeType type): scope(scope), uniqueId(uniqueId), type(type), ownerNamespace(0), ownerFunction(0), ownerType(0), types(allocator), functions(allocator), variables(allocator), aliases(allocator), scopes(allocator), idLookupMap(allocator), typeLookupMap(allocator), allVariables(allocator)
	{
		scopeDepth = scope ? scope->scopeDepth + 1 : 0;
		breakDepth = scope ? scope->breakDepth : 0;
		contiueDepth = scope ? scope->contiueDepth : 0;

		startOffset = 0;
		dataSize = 0;

		unrestricted = false;
	}

	ScopeData(Allocator *allocator, ScopeData *scope, unsigned uniqueId, NamespaceData *ownerNamespace): scope(scope), uniqueId(uniqueId), type(SCOPE_NAMESPACE), ownerNamespace(ownerNamespace), ownerFunction(0), ownerType(0), types(allocator), functions(allocator), variables(allocator), aliases(allocator), scopes(allocator), idLookupMap(allocator), typeLookupMap(allocator), allVariables(allocator)
	{
		scopeDepth = scope ? scope->scopeDepth + 1 : 0;
		breakDepth = 0;
		contiueDepth = 0;

		startOffset = 0;
		dataSize = 0;

		unrestricted = false;
	}

	ScopeData(Allocator *allocator, ScopeData *scope, unsigned uniqueId, FunctionData *ownerFunction): scope(scope), uniqueId(uniqueId), type(SCOPE_FUNCTION), ownerNamespace(0), ownerFunction(ownerFunction), ownerType(0), types(allocator), functions(allocator), variables(allocator), aliases(allocator), scopes(allocator), idLookupMap(allocator), typeLookupMap(allocator), allVariables(allocator)
	{
		scopeDepth = scope ? scope->scopeDepth + 1 : 0;
		breakDepth = 0;
		contiueDepth = 0;

		startOffset = 0;
		dataSize = 0;

		unrestricted = false;
	}

	ScopeData(Allocator *allocator, ScopeData *scope, unsigned uniqueId, TypeBase *ownerType): scope(scope), uniqueId(uniqueId), type(SCOPE_TYPE), ownerNamespace(0), ownerFunction(0), ownerType(ownerType), types(allocator), functions(allocator), variables(allocator), aliases(allocator), scopes(allocator), idLookupMap(allocator), typeLookupMap(allocator), allVariables(allocator)
	{
		scopeDepth = scope ? scope->scopeDepth + 1 : 0;
		breakDepth = 0;
		contiueDepth = 0;

		startOffset = 0;
		dataSize = 0;

		unrestricted = false;
	}

	ScopeData *scope;

	unsigned uniqueId;

	ScopeType type;
	
	NamespaceData *ownerNamespace;
	FunctionData *ownerFunction;
	TypeBase *ownerType;

	unsigned scopeDepth;
	unsigned breakDepth;
	unsigned contiueDepth;

	long long startOffset;
	long long dataSize;

	SmallArray<TypeBase*, 2> types;
	SmallArray<FunctionData*, 2> functions;
	SmallArray<VariableData*, 4> variables;
	SmallArray<AliasData*, 2> aliases;
	SmallArray<ScopeData*, 2> scopes;

	// Lookup tables
	DirectChainedMap<IdentifierLookupResult> idLookupMap;
	DirectChainedMap<TypeLookupResult> typeLookupMap;

	bool unrestricted;
	
	// Full set of variables including all from nested scopes that have been closed
	SmallArray<VariableData*, 4> allVariables;
};

struct FunctionValue
{
	FunctionValue(): source(0), function(0), context(0)
	{
	}

	FunctionValue(SynBase *source, FunctionData *function, ExprBase *context): source(source), function(function), context(context)
	{
		assert(context);
	}

	SynBase *source;
	FunctionData *function;
	ExprBase *context;

	// Safe bool cast
	typedef void (FunctionValue::*bool_type)() const;
	void safe_bool() const{}

	operator bool_type() const
	{
		return function ? &FunctionValue::safe_bool : 0;
	}

private:
	template <typename T>
	bool operator!=(const T& rhs) const;
	template <typename T>
	bool operator==(const T& rhs) const;
};

struct ConstantData
{
	ConstantData(SynIdentifier *name, ExprBase *value): name(name), value(value), next(0), listed(false)
	{
	}

	SynIdentifier *name;
	ExprBase *value;

	ConstantData *next;
	bool listed;
};

template<typename T>
unsigned GetTypeAlignment()
{
	struct Helper
	{
		char x;
		T y;
	};

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
		TypeMemberSet,
	};
}

struct TypeBase
{
	TypeBase(unsigned typeID, InplaceStr name): typeID(typeID), name(name)
	{
		nameHash = NULLC::GetStringHash(name.begin, name.end);

		importModule = NULL;

		typeIndex = ~0u;

		size = 0;
		alignment = 0;
		padding = 0;

		isGeneric = false;
		hasPointers = false;

		hasTranslation = false;

		refType = 0;
		unsizedArrayType = 0;
	}

	virtual ~TypeBase()
	{
	}

	unsigned typeID;

	unsigned nameHash;
	InplaceStr name;

	ModuleData *importModule;

	unsigned typeIndex;

	unsigned alignment;
	long long size;
	unsigned padding;

	bool isGeneric;
	bool hasPointers;

	bool hasTranslation;

	TypeRef *refType; // Reference type to this type
	IntrusiveList<TypeHandle> arrayTypes; // Array types derived from this type
	TypeUnsizedArray *unsizedArrayType; // An unsized array type derived from this type
};

struct TypeBaseHasher
{
	unsigned operator()(TypeBase* key)
	{
		return key->nameHash;
	}
};

struct TypeError: TypeBase
{
	TypeError(): TypeBase(myTypeID, InplaceStr("%error-type%"))
	{
	}

	static const unsigned myTypeID = TypeNode::TypeError;
};

struct TypeVoid: TypeBase
{
	TypeVoid(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 0;
		alignment = 0;
	}

	static const unsigned myTypeID = TypeNode::TypeVoid;
};

struct TypeBool: TypeBase
{
	TypeBool(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 1;
	}

	static const unsigned myTypeID = TypeNode::TypeBool;
};

struct TypeChar: TypeBase
{
	TypeChar(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 1;
	}

	static const unsigned myTypeID = TypeNode::TypeChar;
};

struct TypeShort: TypeBase
{
	TypeShort(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 2;
		alignment = GetTypeAlignment<short>();
	}

	static const unsigned myTypeID = TypeNode::TypeShort;
};

struct TypeInt: TypeBase
{
	TypeInt(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = TypeNode::TypeInt;
};

struct TypeLong: TypeBase
{
	TypeLong(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 8;
		alignment = GetTypeAlignment<long>();
	}

	static const unsigned myTypeID = TypeNode::TypeLong;
};

struct TypeFloat: TypeBase
{
	TypeFloat(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<float>();
	}

	static const unsigned myTypeID = TypeNode::TypeFloat;
};

struct TypeDouble: TypeBase
{
	TypeDouble(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 8;
		alignment = GetTypeAlignment<double>();
	}

	static const unsigned myTypeID = TypeNode::TypeDouble;
};

struct TypeTypeID: TypeBase
{
	TypeTypeID(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = TypeNode::TypeTypeID;
};

struct TypeFunctionID: TypeBase
{
	TypeFunctionID(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = TypeNode::TypeFunctionID;
};

struct TypeNullptr: TypeBase
{
	TypeNullptr(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = NULLC_PTR_SIZE;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = TypeNode::TypeNullptr;
};

struct TypeGeneric: TypeBase
{
	TypeGeneric(InplaceStr name): TypeBase(myTypeID, name)
	{
		isGeneric = true;
	}

	static const unsigned myTypeID = TypeNode::TypeGeneric;
};

struct TypeGenericAlias: TypeBase
{
	TypeGenericAlias(InplaceStr name, SynIdentifier *baseName): TypeBase(myTypeID, name), baseName(baseName)
	{
		assert(baseName);

		isGeneric = true;
	}

	SynIdentifier *baseName;

	static const unsigned myTypeID = TypeNode::TypeGenericAlias;
};

struct TypeAuto: TypeBase
{
	TypeAuto(InplaceStr name): TypeBase(myTypeID, name)
	{
	}

	static const unsigned myTypeID = TypeNode::TypeAuto;
};

struct TypeStruct: TypeBase
{
	TypeStruct(unsigned myTypeID, InplaceStr name): TypeBase(myTypeID, name)
	{
		typeScope = NULL;
	}

	// Scope where class members reside
	ScopeData *typeScope;

	IntrusiveList<MemberHandle> members;

	IntrusiveList<ConstantData> constants;
};

struct TypeAutoRef: TypeStruct
{
	TypeAutoRef(InplaceStr name): TypeStruct(myTypeID, name)
	{
		hasPointers = true;
	}

	static const unsigned myTypeID = TypeNode::TypeAutoRef;
};

struct TypeAutoArray: TypeStruct
{
	TypeAutoArray(InplaceStr name): TypeStruct(myTypeID, name)
	{
		hasPointers = true;
	}

	static const unsigned myTypeID = TypeNode::TypeAutoArray;
};

struct TypeRef: TypeBase
{
	TypeRef(InplaceStr name, TypeBase *subType): TypeBase(myTypeID, name), subType(subType)
	{
		size = NULLC_PTR_SIZE;
		alignment = 4;

		isGeneric = subType->isGeneric;

		hasPointers = true;
	}

	TypeBase *subType;

	static const unsigned myTypeID = TypeNode::TypeRef;
};

struct TypeArray: TypeBase
{
	TypeArray(InplaceStr name, TypeBase *subType, long long length): TypeBase(myTypeID, name), subType(subType), length(length)
	{
		size = subType->size * length;

		isGeneric = subType->isGeneric;

		hasPointers = subType->hasPointers;
	}

	TypeBase *subType;
	long long length;

	static const unsigned myTypeID = TypeNode::TypeArray;
};

struct TypeUnsizedArray: TypeStruct
{
	TypeUnsizedArray(InplaceStr name, TypeBase *subType): TypeStruct(myTypeID, name), subType(subType)
	{
		isGeneric = subType->isGeneric;

		hasPointers = true;
	}

	TypeBase *subType;

	static const unsigned myTypeID = TypeNode::TypeUnsizedArray;
};

struct TypeFunction: TypeBase
{
	TypeFunction(InplaceStr name, TypeBase *returnType, IntrusiveList<TypeHandle> arguments): TypeBase(myTypeID, name), returnType(returnType), arguments(arguments)
	{
		size = 4 + NULLC_PTR_SIZE;

		isGeneric = returnType->isGeneric;

		for(TypeHandle *el = arguments.head; el; el = el->next)
			isGeneric |= el->type->isGeneric;

		hasPointers = true;
	}

	TypeBase *returnType;
	IntrusiveList<TypeHandle> arguments;

	static const unsigned myTypeID = TypeNode::TypeFunction;
};

struct TypeGenericClassProto: TypeBase
{
	TypeGenericClassProto(const SynIdentifier& identifier, SynBase *source, ScopeData *scope, SynClassDefinition *definition): TypeBase(myTypeID, identifier.name), identifier(identifier), source(source), scope(scope), definition(definition)
	{
		isGeneric = true;
	}

	SynIdentifier identifier;

	SynBase *source;

	ScopeData *scope;

	SynClassDefinition *definition;

	IntrusiveList<ExprBase> instances;

	static const unsigned myTypeID = TypeNode::TypeGenericClassProto;
};

struct TypeGenericClass: TypeBase
{
	TypeGenericClass(InplaceStr name, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> generics): TypeBase(myTypeID, name), proto(proto), generics(generics)
	{
		isGeneric = true;
	}

	TypeGenericClassProto *proto;

	IntrusiveList<TypeHandle> generics;

	static const unsigned myTypeID = TypeNode::TypeGenericClass;
};

struct TypeClass: TypeStruct
{
	TypeClass(Allocator *allocator, const SynIdentifier& identifier, SynBase *source, ScopeData *scope, TypeGenericClassProto *proto, IntrusiveList<MatchData> generics, bool extendable, TypeClass *baseClass): TypeStruct(myTypeID, identifier.name), identifier(identifier), source(source), scope(scope), proto(proto), generics(generics), extendable(extendable), baseClass(baseClass), methods(allocator), methodMap(allocator)
	{
		completed = false;
		isInternal = false;

		hasFinalizer = false;

		defaultAssign = NULL;
	}

	SynIdentifier identifier;

	SynBase *source;

	ScopeData *scope;

	TypeGenericClassProto *proto;

	IntrusiveList<MatchData> generics;

	IntrusiveList<MatchData> aliases;

	bool extendable;

	TypeClass *baseClass;

	bool completed;
	bool isInternal;

	bool hasFinalizer;

	FunctionData *defaultAssign;

	SmallArray<FunctionData*, 4> methods;
	DirectChainedMap<FunctionData*> methodMap;

	static const unsigned myTypeID = TypeNode::TypeClass;
};

struct TypeEnum: TypeStruct
{
	TypeEnum(const SynIdentifier& identifier, SynBase *source, ScopeData *scope): TypeStruct(myTypeID, identifier.name), identifier(identifier), source(source), scope(scope)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	SynIdentifier identifier;

	SynBase *source;

	ScopeData *scope;

	static const unsigned myTypeID = TypeNode::TypeEnum;
};

struct TypeFunctionSet: TypeBase
{
	TypeFunctionSet(InplaceStr name, IntrusiveList<TypeHandle> types): TypeBase(myTypeID, name), types(types)
	{
		isGeneric = true;
	}

	IntrusiveList<TypeHandle> types;

	static const unsigned myTypeID = TypeNode::TypeFunctionSet;
};

struct TypeArgumentSet: TypeBase
{
	TypeArgumentSet(InplaceStr name, IntrusiveList<TypeHandle> types): TypeBase(myTypeID, name), types(types)
	{
		isGeneric = true;
	}

	IntrusiveList<TypeHandle> types;

	static const unsigned myTypeID = TypeNode::TypeArgumentSet;
};

struct TypeMemberSet: TypeBase
{
	TypeMemberSet(InplaceStr name, TypeStruct *type): TypeBase(myTypeID, name), type(type)
	{
		isGeneric = true;
	}

	TypeStruct *type;

	static const unsigned myTypeID = TypeNode::TypeMemberSet;
};

template<typename T>
bool isType(TypeBase *node)
{
	return node && node->typeID == T::myTypeID;
}

template<typename T>
T* getType(TypeBase *node)
{
	if(node && node->typeID == T::myTypeID)
		return static_cast<T*>(node);

	return 0;
}

template<>
inline TypeStruct* getType(TypeBase *node)
{
	if(node && (node->typeID == TypeAutoRef::myTypeID || node->typeID == TypeAutoArray::myTypeID || node->typeID == TypeUnsizedArray::myTypeID || node->typeID == TypeClass::myTypeID || node->typeID == TypeEnum::myTypeID))
		return static_cast<TypeStruct*>(node);

	return 0;
}

InplaceStr GetOperatorName(InplaceStr name);

InplaceStr GetReferenceTypeName(ExpressionContext &ctx, TypeBase* type);
InplaceStr GetArrayTypeName(ExpressionContext &ctx, TypeBase* type, long long length);
InplaceStr GetUnsizedArrayTypeName(ExpressionContext &ctx, TypeBase* type);
InplaceStr GetFunctionTypeName(ExpressionContext &ctx, TypeBase* returnType, IntrusiveList<TypeHandle> arguments);
InplaceStr GetGenericClassTypeName(ExpressionContext &ctx, TypeBase* proto, IntrusiveList<TypeHandle> generics);
InplaceStr GetFunctionSetTypeName(ExpressionContext &ctx, IntrusiveList<TypeHandle> types);
InplaceStr GetArgumentSetTypeName(ExpressionContext &ctx, IntrusiveList<TypeHandle> types);
InplaceStr GetMemberSetTypeName(ExpressionContext &ctx, TypeBase* type);
InplaceStr GetGenericAliasTypeName(ExpressionContext &ctx, InplaceStr name);

InplaceStr GetFunctionContextTypeName(ExpressionContext &ctx, InplaceStr functionName, unsigned index);
InplaceStr GetFunctionContextVariableName(ExpressionContext &ctx, FunctionData *function, unsigned index);
InplaceStr GetFunctionTableName(ExpressionContext &ctx, FunctionData *function);
InplaceStr GetFunctionContextMemberName(ExpressionContext &ctx, InplaceStr prefix, InplaceStr suffix, unsigned index);
InplaceStr GetFunctionVariableUpvalueName(ExpressionContext &ctx, VariableData *variable);

InplaceStr GetTypeNameInScope(ExpressionContext &ctx, ScopeData *scope, InplaceStr str);
InplaceStr GetVariableNameInScope(ExpressionContext &ctx, ScopeData *scope, InplaceStr str);
InplaceStr GetFunctionNameInScope(ExpressionContext &ctx, ScopeData *scope, TypeBase *parentType, InplaceStr str, bool isOperator, bool isAccessor);

InplaceStr GetTemporaryName(ExpressionContext &ctx, unsigned index, const char *suffix);

unsigned GetAlignmentOffset(long long offset, unsigned alignment);
