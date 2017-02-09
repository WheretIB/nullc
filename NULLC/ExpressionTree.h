#pragma once

#include "ParseTree.h"
#include "IntrusiveList.h"
#include "Array.h"
#include "HashMap.h"

struct TypeBase;
struct TypeStruct;
struct TypeRef;
struct TypeArray;
struct TypeUnsizedArray;
struct TypeFunction;

struct ScopeData;
struct NamespaceData;
struct VariableData;
struct FunctionData;

struct ExprBase;

struct VariableHandle
{
	VariableHandle(VariableData *variable): variable(variable), next(0)
	{
	}

	VariableData *variable;

	VariableHandle *next;
};

struct TypeHandle
{
	TypeHandle(TypeBase *type): type(type), next(0)
	{
	}

	TypeBase *type;

	TypeHandle *next;
};

struct NamespaceData
{
	NamespaceData(ScopeData *scope, InplaceStr name): scope(scope), name(name)
	{
		nameHash = GetStringHash(name.begin, name.end);
	}

	ScopeData *scope;

	InplaceStr name;
	unsigned nameHash;
};

struct VariableData
{
	VariableData(ScopeData *scope, unsigned alignment, TypeBase *type, InplaceStr name): scope(scope), alignment(alignment), type(type), name(name)
	{
		nameHash = GetStringHash(name.begin, name.end);
	}

	ScopeData *scope;

	unsigned alignment;

	TypeBase *type;

	InplaceStr name;
	unsigned nameHash;
};

struct FunctionData
{
	FunctionData(ScopeData *scope, TypeFunction *type, InplaceStr name): scope(scope), type(type), name(name)
	{
		nameHash = GetStringHash(name.begin, name.end);
	}

	ScopeData *scope;

	TypeFunction *type;

	InplaceStr name;
	unsigned nameHash;
};

struct ScopeData
{
	ScopeData(unsigned depth, ScopeData *scope): depth(depth), scope(scope), ownerNamespace(0), ownerFunction(0), ownerType(0)
	{
	}

	ScopeData(unsigned depth, ScopeData *scope, NamespaceData *ownerNamespace): depth(depth), scope(scope), ownerNamespace(ownerNamespace), ownerFunction(0), ownerType(0)
	{
	}

	ScopeData(unsigned depth, ScopeData *scope, FunctionData *ownerFunction): depth(depth), scope(scope), ownerNamespace(0), ownerFunction(ownerFunction), ownerType(0)
	{
	}

	ScopeData(unsigned depth, ScopeData *scope, TypeBase *ownerType): depth(depth), scope(scope), ownerNamespace(0), ownerFunction(0), ownerType(ownerType)
	{
	}

	unsigned depth;

	ScopeData *scope;

	NamespaceData *ownerNamespace;
	FunctionData *ownerFunction;
	TypeBase *ownerType;

	FastVector<TypeBase*> types;
	FastVector<FunctionData*> functions;
	FastVector<VariableData*> variables;
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

struct ExpressionContext
{
	ExpressionContext();

	void PushScope();
	void PushScope(NamespaceData *nameSpace);
	void PushScope(FunctionData *function);
	void PushScope(TypeBase *type);
	void PopScope();

	void AddType(TypeBase *type);
	void AddFunction(FunctionData *function);
	void AddVariable(VariableData *variable);

	TypeRef* GetReferenceType(TypeBase* type);
	TypeArray* GetArrayType(TypeBase* type, long long size);
	TypeUnsizedArray* GetUnsizedArrayType(TypeBase* type);
	TypeFunction* GetFunctionType(TypeBase* returnType, IntrusiveList<TypeHandle> arguments);

	// Full state info
	FastVector<NamespaceData*> namespaces;
	FastVector<TypeBase*> types;
	FastVector<FunctionData*> functions;
	FastVector<VariableData*> variables;

	// Context info
	HashMap<TypeBase*> typeMap;
	HashMap<FunctionData*> functionMap;
	HashMap<VariableData*> variableMap;

	FastVector<ScopeData*> scopes;

	// Error info
	const char *errorPos;
	InplaceStr errorMsg;

	// Base types
	TypeBase* typeVoid;

	TypeBase* typeBool;

	TypeBase* typeChar;
	TypeBase* typeShort;
	TypeBase* typeInt;
	TypeBase* typeLong;

	TypeBase* typeFloat;
	TypeBase* typeDouble;

	TypeBase* typeTypeID;
	TypeBase* typeFunctionID;

	TypeBase* typeGeneric;

	TypeBase* typeAuto;
	TypeStruct* typeAutoRef;
	TypeStruct* typeAutoArray;
};

struct TypeBase
{
	TypeBase(unsigned typeID, InplaceStr name): typeID(typeID), name(name)
	{
		nameHash = GetStringHash(name.begin, name.end);

		size = 0;
		alignment = 0;

		refType = 0;
		unsizedArrayType = 0;
	}

	virtual ~TypeBase()
	{
	}

	unsigned typeID;

	InplaceStr name;
	unsigned nameHash;
	
	long long size;
	unsigned alignment;

	TypeRef *refType; // Reference type to this type
	FastVector<TypeArray*> arrayTypes; // Array types derived from this type
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
	}

	TypeBase *subType;

	static const unsigned myTypeID = __LINE__;
};

struct TypeArray: TypeBase
{
	TypeArray(InplaceStr name, TypeBase *subType, long long length): TypeBase(myTypeID, name), subType(subType), length(length)
	{
		size = subType->size * length;
	}

	TypeBase *subType;
	long long length;

	static const unsigned myTypeID = __LINE__;
};

struct TypeUnsizedArray: TypeStruct
{
	TypeUnsizedArray(InplaceStr name, TypeBase *subType): TypeStruct(myTypeID, name), subType(subType)
	{
	}

	TypeBase *subType;

	static const unsigned myTypeID = __LINE__;
};

struct TypeFunction: TypeBase
{
	TypeFunction(InplaceStr name, TypeBase *returnType, IntrusiveList<TypeHandle> arguments): TypeBase(myTypeID, name), returnType(returnType), arguments(arguments)
	{
	}

	TypeBase *returnType;
	IntrusiveList<TypeHandle> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct TypeClass: TypeStruct
{
	TypeClass(ScopeData *scope, InplaceStr name, bool extenable, TypeBase *baseClass): TypeStruct(myTypeID, name), scope(scope), extenable(extenable), baseClass(baseClass)
	{
	}

	ScopeData *scope;

	bool extenable;

	TypeBase *baseClass;

	static const unsigned myTypeID = __LINE__;
};

struct TypeGenericClassProto: TypeBase
{
	TypeGenericClassProto(InplaceStr name, bool extenable, SynClassDefinition *definition): TypeBase(myTypeID, name), extenable(extenable), definition(definition)
	{
	}

	bool extenable;

	SynClassDefinition *definition;

	static const unsigned myTypeID = __LINE__;
};

struct ExprBase
{
	ExprBase(unsigned typeID, TypeBase *type): typeID(typeID), type(type), next(0)
	{
	}

	virtual ~ExprBase()
	{
	}

	unsigned typeID;

	TypeBase *type;
	ExprBase *next;
};

struct ExprVoid: ExprBase
{
	ExprVoid(TypeBase *type): ExprBase(myTypeID, type)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct ExprBoolLiteral: ExprBase
{
	ExprBoolLiteral(TypeBase *type, bool value): ExprBase(myTypeID, type), value(value)
	{
	}

	bool value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprCharacterLiteral: ExprBase
{
	ExprCharacterLiteral(TypeBase *type, unsigned char value): ExprBase(myTypeID, type), value(value)
	{
	}

	unsigned char value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprStringLiteral: ExprBase
{
	ExprStringLiteral(TypeBase *type, char *value, unsigned length): ExprBase(myTypeID, type), value(value), length(length)
	{
	}

	char *value;
	unsigned length;

	static const unsigned myTypeID = __LINE__;
};

struct ExprIntegerLiteral: ExprBase
{
	ExprIntegerLiteral(TypeBase *type, long long value): ExprBase(myTypeID, type), value(value)
	{
	}

	long long value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprRationalLiteral: ExprBase
{
	ExprRationalLiteral(TypeBase *type, double value): ExprBase(myTypeID, type), value(value)
	{
	}

	double value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprTypeLiteral: ExprBase
{
	ExprTypeLiteral(TypeBase *type, TypeBase *value): ExprBase(myTypeID, type), value(value)
	{
	}

	TypeBase *value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprNullptrLiteral: ExprBase
{
	ExprNullptrLiteral(TypeBase *type): ExprBase(myTypeID, type)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct ExprUnaryOp: ExprBase
{
	ExprUnaryOp(TypeBase *type, SynUnaryOpType op, ExprBase* value): ExprBase(myTypeID, type), op(op), value(value)
	{
	}

	SynUnaryOpType op;

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprBinaryOp: ExprBase
{
	ExprBinaryOp(TypeBase *type, SynBinaryOpType op, ExprBase* lhs, ExprBase* rhs): ExprBase(myTypeID, type), op(op), lhs(lhs), rhs(rhs)
	{
	}

	SynBinaryOpType op;

	ExprBase* lhs;
	ExprBase* rhs;

	static const unsigned myTypeID = __LINE__;
};

struct ExprGetAddress: ExprBase
{
	ExprGetAddress(TypeBase *type, ExprBase* value): ExprBase(myTypeID, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprDereference: ExprBase
{
	ExprDereference(TypeBase *type, ExprBase* value): ExprBase(myTypeID, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprConditional: ExprBase
{
	ExprConditional(TypeBase *type, ExprBase *condition, ExprBase *trueBlock, ExprBase *falseBlock): ExprBase(myTypeID, type), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	ExprBase *condition;
	ExprBase *trueBlock;
	ExprBase *falseBlock;

	static const unsigned myTypeID = __LINE__;
};

struct ExprAssignment: ExprBase
{
	ExprAssignment(TypeBase *type, ExprBase *lhs, ExprBase *rhs): ExprBase(myTypeID, type), lhs(lhs), rhs(rhs)
	{
	}

	ExprBase *lhs;
	ExprBase *rhs;

	static const unsigned myTypeID = __LINE__;
};

struct ExprMemberAccess: ExprBase
{
	ExprMemberAccess(TypeBase *type, ExprBase *value, VariableData *member): ExprBase(myTypeID, type), value(value), member(member)
	{
	}

	ExprBase *value;
	VariableData *member;

	static const unsigned myTypeID = __LINE__;
};

struct ExprArrayIndex: ExprBase
{
	ExprArrayIndex(TypeBase *type, ExprBase *value, ExprBase *index): ExprBase(myTypeID, type), value(value), index(index)
	{
	}

	ExprBase *value;
	ExprBase *index;

	static const unsigned myTypeID = __LINE__;
};

struct ExprReturn: ExprBase
{
	ExprReturn(TypeBase *type, ExprBase* value): ExprBase(myTypeID, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprVariableDefinition: ExprBase
{
	ExprVariableDefinition(TypeBase *type, VariableData* variable, ExprBase* initializer): ExprBase(myTypeID, type), variable(variable), initializer(initializer)
	{
	}

	VariableData* variable;

	ExprBase* initializer;

	static const unsigned myTypeID = __LINE__;
};

struct ExprVariableDefinitions: ExprBase
{
	ExprVariableDefinitions(TypeBase *type, IntrusiveList<ExprVariableDefinition> definitions): ExprBase(myTypeID, type), definitions(definitions)
	{
	}

	IntrusiveList<ExprVariableDefinition> definitions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprVariableAccess: ExprBase
{
	ExprVariableAccess(TypeBase *type, VariableData *variable): ExprBase(myTypeID, type), variable(variable)
	{
	}

	VariableData *variable;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionDefinition: ExprBase
{
	ExprFunctionDefinition(TypeBase *type, bool prototype, FunctionData* function, IntrusiveList<ExprVariableDefinition> arguments, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, type), prototype(prototype), function(function), arguments(arguments), expressions(expressions)
	{
	}

	bool prototype;

	FunctionData* function;

	IntrusiveList<ExprVariableDefinition> arguments;

	IntrusiveList<ExprBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionAccess: ExprBase
{
	ExprFunctionAccess(TypeBase *type, FunctionData *function): ExprBase(myTypeID, type), function(function)
	{
	}

	FunctionData *function;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionCall: ExprBase
{
	ExprFunctionCall(TypeBase *type, ExprBase *function, IntrusiveList<ExprBase> arguments): ExprBase(myTypeID, type), function(function), arguments(arguments)
	{
	}

	ExprBase *function;
	IntrusiveList<ExprBase> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct ExprClassDefinition: ExprBase
{
	ExprClassDefinition(TypeBase *type, TypeClass *classType): ExprBase(myTypeID, type), classType(classType)
	{
	}

	TypeClass *classType;

	IntrusiveList<ExprFunctionDefinition> functions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprGenericClassPrototype: ExprBase
{
	ExprGenericClassPrototype(TypeBase *type, TypeGenericClassProto *genericProtoType): ExprBase(myTypeID, type), genericProtoType(genericProtoType)
	{
	}

	TypeGenericClassProto *genericProtoType;

	static const unsigned myTypeID = __LINE__;
};

struct ExprIfElse: ExprBase
{
	ExprIfElse(TypeBase *type, ExprBase *condition, ExprBase *trueBlock, ExprBase *falseBlock): ExprBase(myTypeID, type), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	ExprBase *condition;
	ExprBase *trueBlock;
	ExprBase *falseBlock;

	static const unsigned myTypeID = __LINE__;
};

struct ExprBlock: ExprBase
{
	ExprBlock(TypeBase *type, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, type), expressions(expressions)
	{
	}

	IntrusiveList<ExprBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprModule: ExprBase
{
	ExprModule(TypeBase *type, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, type), expressions(expressions)
	{
	}

	IntrusiveList<ExprBase> expressions;

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
	if(node && (isType<TypeAutoRef>(node) || isType<TypeAutoArray>(node) || isType<TypeUnsizedArray>(node) || isType<TypeClass>(node)))
		return static_cast<TypeStruct*>(node);

	return 0;
}

template<typename T>
bool isType(ExprBase *node)
{
	return node->typeID == typename T::myTypeID;
}

template<typename T>
T* getType(ExprBase *node)
{
	if(node && isType<T>(node))
		return static_cast<T*>(node);

	return 0;
}

ExprBase* Analyze(ExpressionContext &context, SynBase *syntax);
