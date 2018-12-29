#pragma once

#include "stdafx.h"
#include "Array.h"
#include "IntrusiveList.h"
#include "StrAlgo.h"

#include "ParseTree.h"

struct ExprBase;
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

struct VariableHandle
{
	VariableHandle(VariableData *variable): variable(variable), next(0), listed(false)
	{
	}

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

struct NamespaceData
{
	NamespaceData(SynBase *source, ScopeData *scope, NamespaceData *parent, InplaceStr name, unsigned uniqueId): source(source), scope(scope), parent(parent), name(name), uniqueId(uniqueId)
	{
		nameHash = GetStringHash(name.begin, name.end);

		if(parent)
			fullNameHash = StringHashContinue(StringHashContinue(parent->fullNameHash, "."), name.begin, name.end);
		else
			fullNameHash = nameHash;
	}

	SynBase *source;

	ScopeData *scope;

	NamespaceData *parent;

	InplaceStr name;
	unsigned nameHash;

	unsigned fullNameHash;

	unsigned uniqueId;
};

struct VariableData
{
	VariableData(Allocator *allocator, SynBase *source, ScopeData *scope, unsigned alignment, TypeBase *type, InplaceStr name, unsigned offset, unsigned uniqueId): source(source), scope(scope), alignment(alignment), type(type), name(name), offset(offset), uniqueId(uniqueId), users(allocator)
	{
		imported = false;

		nameHash = GetStringHash(name.begin, name.end);

		isReference = false;
		isReadonly = false;

		usedAsExternal = false;

		lookupOnly = false;

		if(alignment != 0)
			assert(offset % alignment == 0);
	}

	SynBase *source;

	bool imported;

	ScopeData *scope;

	unsigned alignment;

	TypeBase *type;

	InplaceStr name;
	unsigned nameHash;

	bool isReference;
	bool isReadonly;

	bool usedAsExternal;

	bool lookupOnly;

	unsigned offset;

	unsigned uniqueId;

	SmallArray<VmConstant*, 8> users;
};

struct MatchData
{
	MatchData(InplaceStr name, TypeBase *type): name(name), type(type), next(0), listed(false)
	{
	}

	InplaceStr name;
	TypeBase *type;

	MatchData *next;
	bool listed;
};

struct ArgumentData
{
	ArgumentData(): source(0), isExplicit(false), type(0), value(0)
	{
	}

	ArgumentData(SynBase *source, bool isExplicit, InplaceStr name, TypeBase *type, ExprBase *value): source(source), isExplicit(isExplicit), name(name), type(type), value(value)
	{
	}

	SynBase *source;
	bool isExplicit;
	InplaceStr name;
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

struct FunctionData
{
	FunctionData(Allocator *allocator, SynBase *source, ScopeData *scope, bool coroutine, bool accessor, bool isOperator, TypeFunction *type, TypeBase *contextType, InplaceStr name, IntrusiveList<MatchData> generics, unsigned uniqueId): source(source), scope(scope), coroutine(coroutine), accessor(accessor), isOperator(isOperator), type(type), contextType(contextType), name(name), generics(generics), uniqueId(uniqueId), arguments(allocator), instances(allocator)
	{
		imported = false;

		isInternal = false;

		nameHash = GetStringHash(name.begin, name.end);

		functionIndex = ~0u;

		isPrototype = false;
		implementation = NULL;

		proto = NULL;
		isGenericInstance = false;

		definition = NULL;

		declaration = NULL;

		functionScope = NULL;
		stackSize = 0;

		contextArgument = NULL;

		coroutineJumpOffset = NULL;

		contextVariable = NULL;

		yieldCount = 0;

		hasExplicitReturn = false;

		vmFunction = NULL;
	}

	SynBase *source;

	bool imported;

	bool isInternal;

	ScopeData *scope;

	bool coroutine;
	bool accessor;
	bool isOperator;

	TypeFunction *type;

	TypeBase *contextType;

	InplaceStr name;
	unsigned nameHash;

	unsigned functionIndex;

	unsigned uniqueId;

	IntrusiveList<MatchData> generics;

	IntrusiveList<MatchData> aliases;

	SmallArray<ArgumentData, 8> arguments;

	bool isPrototype;
	FunctionData *implementation;

	FunctionData *proto;
	bool isGenericInstance;

	SmallArray<FunctionData*, 8> instances;

	SynFunctionDefinition *definition;

	ExprBase *declaration;

	// Scope where function variables reside
	ScopeData *functionScope;
	long long stackSize;

	// Variables for arguments
	IntrusiveList<VariableHandle> argumentVariables;

	// Variable for the argument containing reference to function context
	VariableData *contextArgument;

	VariableData *coroutineJumpOffset;

	IntrusiveList<UpvalueData> upvalues;

	IntrusiveList<CoroutineStateData> coroutineState;

	// Variable containing a pointer to the function context
	VariableData *contextVariable;

	unsigned yieldCount;

	bool hasExplicitReturn;

	VmFunction *vmFunction;
};

struct AliasData
{
	AliasData(SynBase *source, ScopeData *scope, TypeBase *type, InplaceStr name, unsigned uniqueId): source(source), scope(scope), type(type), name(name), uniqueId(uniqueId)
	{
		imported = false;

		nameHash = GetStringHash(name.begin, name.end);
	}

	SynBase *source;

	bool imported;

	ScopeData *scope;

	TypeBase *type;

	InplaceStr name;
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

struct ScopeData
{
	ScopeData(Allocator *allocator, ScopeData *scope, unsigned uniqueId, ScopeType type): scope(scope), uniqueId(uniqueId), type(type), ownerNamespace(0), ownerFunction(0), ownerType(0), types(allocator), functions(allocator), variables(allocator), aliases(allocator), scopes(allocator), shadowedVariables(allocator)
	{
		scopeDepth = scope ? scope->scopeDepth + 1 : 0;
		loopDepth = scope ? scope->loopDepth : 0;

		startOffset = 0;
		dataSize = 0;
	}

	ScopeData(Allocator *allocator, ScopeData *scope, unsigned uniqueId, NamespaceData *ownerNamespace): scope(scope), uniqueId(uniqueId), type(SCOPE_NAMESPACE), ownerNamespace(ownerNamespace), ownerFunction(0), ownerType(0), types(allocator), functions(allocator), variables(allocator), aliases(allocator), scopes(allocator), shadowedVariables(allocator)
	{
		scopeDepth = scope ? scope->scopeDepth + 1 : 0;
		loopDepth = 0;

		startOffset = 0;
		dataSize = 0;
	}

	ScopeData(Allocator *allocator, ScopeData *scope, unsigned uniqueId, FunctionData *ownerFunction): scope(scope), uniqueId(uniqueId), type(SCOPE_FUNCTION), ownerNamespace(0), ownerFunction(ownerFunction), ownerType(0), types(allocator), functions(allocator), variables(allocator), aliases(allocator), scopes(allocator), shadowedVariables(allocator)
	{
		scopeDepth = scope ? scope->scopeDepth + 1 : 0;
		loopDepth = 0;

		startOffset = 0;
		dataSize = 0;
	}

	ScopeData(Allocator *allocator, ScopeData *scope, unsigned uniqueId, TypeBase *ownerType): scope(scope), uniqueId(uniqueId), type(SCOPE_TYPE), ownerNamespace(0), ownerFunction(0), ownerType(ownerType), types(allocator), functions(allocator), variables(allocator), aliases(allocator), scopes(allocator), shadowedVariables(allocator)
	{
		scopeDepth = scope ? scope->scopeDepth + 1 : 0;
		loopDepth = 0;

		startOffset = 0;
		dataSize = 0;
	}

	ScopeData *scope;

	unsigned uniqueId;

	ScopeType type;
	
	NamespaceData *ownerNamespace;
	FunctionData *ownerFunction;
	TypeBase *ownerType;

	unsigned scopeDepth;
	unsigned loopDepth;

	long long startOffset;
	long long dataSize;

	SmallArray<TypeBase*, 4> types;
	SmallArray<FunctionData*, 4> functions;
	SmallArray<VariableData*, 4> variables;
	SmallArray<AliasData*, 4> aliases;
	SmallArray<ScopeData*, 4> scopes;

	SmallArray<VariableData*, 4> shadowedVariables;
};

struct FunctionValue
{
	FunctionValue(): function(0), context(0)
	{
	}

	FunctionValue(FunctionData *function, ExprBase *context): function(function), context(context)
	{
		assert(context);
	}

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
	ConstantData(InplaceStr name, ExprBase *value): name(name), value(value), next(0), listed(false)
	{
	}

	InplaceStr name;
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

struct TypeBase
{
	TypeBase(unsigned typeID, InplaceStr name): typeID(typeID), name(name)
	{
		nameHash = GetStringHash(name.begin, name.end);

		typeIndex = ~0u;

		size = 0;
		alignment = 0;
		padding = 0;

		isGeneric = false;

		refType = 0;
		unsizedArrayType = 0;
	}

	virtual ~TypeBase()
	{
	}

	unsigned typeID;

	InplaceStr name;
	unsigned nameHash;

	unsigned typeIndex;
	
	long long size;
	unsigned alignment;
	unsigned padding;

	bool isGeneric;

	TypeRef *refType; // Reference type to this type
	IntrusiveList<TypeHandle> arrayTypes; // Array types derived from this type
	TypeUnsizedArray *unsizedArrayType; // An unsized array type derived from this type
};

struct TypeVoid: TypeBase
{
	TypeVoid(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 0;
		alignment = 0;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeBool: TypeBase
{
	TypeBool(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 1;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeChar: TypeBase
{
	TypeChar(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 1;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeShort: TypeBase
{
	TypeShort(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 2;
		alignment = GetTypeAlignment<short>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeInt: TypeBase
{
	TypeInt(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeLong: TypeBase
{
	TypeLong(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 8;
		alignment = GetTypeAlignment<long>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeFloat: TypeBase
{
	TypeFloat(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<float>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeDouble: TypeBase
{
	TypeDouble(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 8;
		alignment = GetTypeAlignment<double>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeTypeID: TypeBase
{
	TypeTypeID(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeFunctionID: TypeBase
{
	TypeFunctionID(InplaceStr name): TypeBase(myTypeID, name)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeGeneric: TypeBase
{
	TypeGeneric(InplaceStr name): TypeBase(myTypeID, name)
	{
		isGeneric = true;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeAuto: TypeBase
{
	TypeAuto(InplaceStr name): TypeBase(myTypeID, name)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeStruct: TypeBase
{
	TypeStruct(unsigned myTypeID, InplaceStr name): TypeBase(myTypeID, name)
	{
	}

	IntrusiveList<VariableHandle> members;

	IntrusiveList<ConstantData> constants;
};

struct TypeAutoRef: TypeStruct
{
	TypeAutoRef(InplaceStr name): TypeStruct(myTypeID, name)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeAutoArray: TypeStruct
{
	TypeAutoArray(InplaceStr name): TypeStruct(myTypeID, name)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeRef: TypeBase
{
	TypeRef(InplaceStr name, TypeBase *subType): TypeBase(myTypeID, name), subType(subType)
	{
		size = NULLC_PTR_SIZE;
		alignment = 4;

		isGeneric = subType->isGeneric;
	}

	TypeBase *subType;

	static const unsigned myTypeID = __LINE__;
};

struct TypeArray: TypeBase
{
	TypeArray(InplaceStr name, TypeBase *subType, long long length): TypeBase(myTypeID, name), subType(subType), length(length)
	{
		size = subType->size * length;

		isGeneric = subType->isGeneric;
	}

	TypeBase *subType;
	long long length;

	static const unsigned myTypeID = __LINE__;
};

struct TypeUnsizedArray: TypeStruct
{
	TypeUnsizedArray(InplaceStr name, TypeBase *subType): TypeStruct(myTypeID, name), subType(subType)
	{
		isGeneric = subType->isGeneric;
	}

	TypeBase *subType;

	static const unsigned myTypeID = __LINE__;
};

struct TypeFunction: TypeBase
{
	TypeFunction(InplaceStr name, TypeBase *returnType, IntrusiveList<TypeHandle> arguments): TypeBase(myTypeID, name), returnType(returnType), arguments(arguments)
	{
		size = 4 + NULLC_PTR_SIZE;

		isGeneric = returnType->isGeneric;

		for(TypeHandle *el = arguments.head; el; el = el->next)
			isGeneric |= el->type->isGeneric;
	}

	TypeBase *returnType;
	IntrusiveList<TypeHandle> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct TypeGenericClassProto: TypeBase
{
	TypeGenericClassProto(SynBase *source, ScopeData *scope, InplaceStr name, SynClassDefinition *definition): TypeBase(myTypeID, name), source(source), scope(scope), definition(definition)
	{
		isGeneric = true;
	}

	SynBase *source;

	ScopeData *scope;

	SynClassDefinition *definition;

	IntrusiveList<ExprBase> instances;

	static const unsigned myTypeID = __LINE__;
};

struct TypeGenericClass: TypeBase
{
	TypeGenericClass(InplaceStr name, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> generics): TypeBase(myTypeID, name), proto(proto), generics(generics)
	{
		isGeneric = true;
	}

	TypeGenericClassProto *proto;

	IntrusiveList<TypeHandle> generics;

	static const unsigned myTypeID = __LINE__;
};

struct TypeClass: TypeStruct
{
	TypeClass(SynBase *source, ScopeData *scope, InplaceStr name, TypeGenericClassProto *proto, IntrusiveList<MatchData> generics, bool extendable, TypeClass *baseClass): TypeStruct(myTypeID, name), source(source), scope(scope), proto(proto), generics(generics), extendable(extendable), baseClass(baseClass)
	{
		imported = false;

		completed = false;

		hasFinalizer = false;
	}

	SynBase *source;

	bool imported;

	ScopeData *scope;

	TypeGenericClassProto *proto;

	IntrusiveList<MatchData> generics;

	IntrusiveList<MatchData> aliases;

	bool extendable;

	TypeClass *baseClass;

	bool completed;

	bool hasFinalizer;

	// Scope where class members reside
	ScopeData *typeScope;

	static const unsigned myTypeID = __LINE__;
};

struct TypeEnum: TypeStruct
{
	TypeEnum(SynBase *source, ScopeData *scope, InplaceStr name): TypeStruct(myTypeID, name), source(source), scope(scope)
	{
		size = 4;
		alignment = GetTypeAlignment<int>();

		imported = false;
	}

	SynBase *source;

	bool imported;

	ScopeData *scope;

	static const unsigned myTypeID = __LINE__;
};

struct TypeFunctionSet: TypeBase
{
	TypeFunctionSet(InplaceStr name, IntrusiveList<TypeHandle> types): TypeBase(myTypeID, name), types(types)
	{
		isGeneric = true;
	}

	IntrusiveList<TypeHandle> types;

	static const unsigned myTypeID = __LINE__;
};

struct TypeArgumentSet: TypeBase
{
	TypeArgumentSet(InplaceStr name, IntrusiveList<TypeHandle> types): TypeBase(myTypeID, name), types(types)
	{
		isGeneric = true;
	}

	IntrusiveList<TypeHandle> types;

	static const unsigned myTypeID = __LINE__;
};

struct TypeMemberSet: TypeBase
{
	TypeMemberSet(InplaceStr name, TypeStruct *type): TypeBase(myTypeID, name), type(type)
	{
		isGeneric = true;
	}

	TypeStruct *type;

	static const unsigned myTypeID = __LINE__;
};

template<typename T>
bool isType(TypeBase *node)
{
	return node->typeID == typename T::myTypeID;
}

template<typename T>
T* getType(TypeBase *node)
{
	if(node && isType<T>(node))
		return static_cast<T*>(node);

	return 0;
}

template<>
inline TypeStruct* getType(TypeBase *node)
{
	if(node && (isType<TypeAutoRef>(node) || isType<TypeAutoArray>(node) || isType<TypeUnsizedArray>(node) || isType<TypeClass>(node) || isType<TypeEnum>(node)))
		return static_cast<TypeStruct*>(node);

	return 0;
}

InplaceStr GetReferenceTypeName(ExpressionContext &ctx, TypeBase* type);
InplaceStr GetArrayTypeName(ExpressionContext &ctx, TypeBase* type, long long length);
InplaceStr GetUnsizedArrayTypeName(ExpressionContext &ctx, TypeBase* type);
InplaceStr GetFunctionTypeName(ExpressionContext &ctx, TypeBase* returnType, IntrusiveList<TypeHandle> arguments);
InplaceStr GetGenericClassTypeName(ExpressionContext &ctx, TypeBase* proto, IntrusiveList<TypeHandle> generics);
InplaceStr GetFunctionSetTypeName(ExpressionContext &ctx, IntrusiveList<TypeHandle> types);
InplaceStr GetArgumentSetTypeName(ExpressionContext &ctx, IntrusiveList<TypeHandle> types);
InplaceStr GetMemberSetTypeName(ExpressionContext &ctx, TypeBase* type);

InplaceStr GetFunctionContextTypeName(ExpressionContext &ctx, InplaceStr functionName, unsigned index);
InplaceStr GetFunctionContextVariableName(ExpressionContext &ctx, FunctionData *function);
InplaceStr GetFunctionTableName(ExpressionContext &ctx, FunctionData *function);
InplaceStr GetFunctionContextMemberName(ExpressionContext &ctx, InplaceStr prefix, InplaceStr suffix);
InplaceStr GetFunctionVariableUpvalueName(ExpressionContext &ctx, VariableData *variable);

InplaceStr GetTypeNameInScope(ExpressionContext &ctx, ScopeData *scope, InplaceStr str);
InplaceStr GetVariableNameInScope(ExpressionContext &ctx, ScopeData *scope, InplaceStr str);
InplaceStr GetFunctionNameInScope(ExpressionContext &ctx, ScopeData *scope, TypeBase *parentType, InplaceStr str, bool isOperator, bool isAccessor);

unsigned GetAlignmentOffset(long long offset, unsigned alignment);
