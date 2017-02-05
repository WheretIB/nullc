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

struct VariableData;
struct FunctionData;

struct ExprBase;

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
};

struct VariableData
{
	VariableData(unsigned alignment, TypeBase *type, InplaceStr name): alignment(alignment), type(type), name(name), next(0)
	{
		nameHash = GetStringHash(name.begin, name.end);
	}

	unsigned alignment;

	TypeBase *type;

	InplaceStr name;
	unsigned nameHash;

	VariableData *next;
};

struct FunctionData
{
	FunctionData(TypeFunction *type, InplaceStr name): type(type), name(name)
	{
		nameHash = GetStringHash(name.begin, name.end);
	}

	TypeFunction *type;

	InplaceStr name;
	unsigned nameHash;
};

struct ScopeData
{
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
	TypeBase(unsigned typeID): typeID(typeID)
	{
		name = 0;
		nameHash = 0;

		size = 0;
		padding = 0;
		alignment = 0;

		refType = 0;
		unsizedArrayType = 0;
	}

	virtual ~TypeBase()
	{
	}

	unsigned typeID;

	const char *name;
	unsigned nameHash;
	
	unsigned size;
	unsigned padding;
	unsigned alignment;

	TypeRef *refType; // Reference type to this type
	FastVector<TypeArray*> arrayTypes; // Array types derived from this type
	TypeUnsizedArray *unsizedArrayType; // An unsized array type derived from this type
};

struct TypeVoid: TypeBase
{
	TypeVoid(): TypeBase(myTypeID)
	{
		name = "void";
		nameHash = GetStringHash(name);

		size = 0;
		padding = 0;
		alignment = 0;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeBool: TypeBase
{
	TypeBool(): TypeBase(myTypeID)
	{
		name = "bool";
		nameHash = GetStringHash(name);

		size = 1;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeChar: TypeBase
{
	TypeChar(): TypeBase(myTypeID)
	{
		name = "char";
		nameHash = GetStringHash(name);

		size = 1;
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeShort: TypeBase
{
	TypeShort(): TypeBase(myTypeID)
	{
		name = "short";
		nameHash = GetStringHash(name);

		size = 2;
		alignment = GetTypeAlignment<short>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeInt: TypeBase
{
	TypeInt(): TypeBase(myTypeID)
	{
		name = "int";
		nameHash = GetStringHash(name);

		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeLong: TypeBase
{
	TypeLong(): TypeBase(myTypeID)
	{
		name = "long";
		nameHash = GetStringHash(name);

		size = 8;
		alignment = GetTypeAlignment<long>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeFloat: TypeBase
{
	TypeFloat(): TypeBase(myTypeID)
	{
		name = "float";
		nameHash = GetStringHash(name);

		size = 4;
		alignment = GetTypeAlignment<float>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeDouble: TypeBase
{
	TypeDouble(): TypeBase(myTypeID)
	{
		name = "double";
		nameHash = GetStringHash(name);

		size = 8;
		alignment = GetTypeAlignment<double>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeTypeID: TypeBase
{
	TypeTypeID(): TypeBase(myTypeID)
	{
		name = "typeid";
		nameHash = GetStringHash(name);

		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeFunctionID: TypeBase
{
	TypeFunctionID(): TypeBase(myTypeID)
	{
		name = "__function";
		nameHash = GetStringHash(name);

		size = 4;
		alignment = GetTypeAlignment<int>();
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeGeneric: TypeBase
{
	TypeGeneric(): TypeBase(myTypeID)
	{
		name = "generic";
		nameHash = GetStringHash(name);
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeAuto: TypeBase
{
	TypeAuto(): TypeBase(myTypeID)
	{
		name = "auto";
		nameHash = GetStringHash(name);
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeStruct: TypeBase
{
	TypeStruct(unsigned myTypeID): TypeBase(myTypeID)
	{
	}

	struct Member
	{
		Member(const char *name, TypeBase *type, unsigned alignment): name(name), type(type), alignment(alignment)
		{
			nameHash = GetStringHash(name);

			offset = 0;

			next = 0;
		}

		const char *name;
		unsigned nameHash;

		TypeBase *type;
		unsigned alignment;

		unsigned offset;

		Member *next;
	};

	IntrusiveList<Member> members;
};

struct TypeAutoRef: TypeStruct
{
	TypeAutoRef(): TypeStruct(myTypeID)
	{
		name = "auto ref";
		nameHash = GetStringHash(name);
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeAutoArray: TypeStruct
{
	TypeAutoArray(): TypeStruct(myTypeID)
	{
		name = "auto[]";
		nameHash = GetStringHash(name);
	}

	static const unsigned myTypeID = __LINE__;
};

struct TypeRef: TypeBase
{
	TypeRef(TypeBase *subType): TypeBase(myTypeID), subType(subType)
	{
		// TODO: create name

		size = NULLC_PTR_SIZE;
		alignment = 4;
	}

	TypeBase *subType;

	static const unsigned myTypeID = __LINE__;
};

struct TypeArray: TypeStruct
{
	TypeArray(TypeBase *subType, long long length): TypeStruct(myTypeID), subType(subType), length(length)
	{
		// TODO: create name
	}

	TypeBase *subType;
	long long length;

	static const unsigned myTypeID = __LINE__;
};

struct TypeUnsizedArray: TypeStruct
{
	TypeUnsizedArray(TypeBase *subType): TypeStruct(myTypeID), subType(subType)
	{
		// TODO: create name
	}

	TypeBase *subType;

	static const unsigned myTypeID = __LINE__;
};

struct TypeFunction: TypeBase
{
	TypeFunction(TypeBase *returnType, IntrusiveList<TypeHandle> arguments): TypeBase(myTypeID), returnType(returnType), arguments(arguments)
	{
		// TODO: create name
	}

	TypeBase *returnType;
	IntrusiveList<TypeHandle> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct TypeClass: TypeStruct
{
	TypeClass(): TypeStruct(myTypeID)
	{
		// TODO: create name
	}

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
