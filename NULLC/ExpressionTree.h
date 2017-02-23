#pragma once

#include "stdafx.h"
#include "IntrusiveList.h"
#include "Array.h"
#include "HashMap.h"
#include "StrAlgo.h"

#include "ParseTree.h"
#include "TypeTree.h"

struct ExprBase;

struct ExpressionContext
{
	ExpressionContext();

	void Stop(const char *pos, const char *msg, ...);

	void PushScope();
	void PushScope(NamespaceData *nameSpace);
	void PushScope(FunctionData *function);
	void PushScope(TypeBase *type);
	void PopScope();

	NamespaceData* GetCurrentNamespace();
	FunctionData* GetCurrentFunction();
	TypeBase* GetCurrentType();

	unsigned GetGenericClassInstantiationDepth();

	void AddType(TypeBase *type);
	void AddFunction(FunctionData *function);
	void AddVariable(VariableData *variable);
	void AddAlias(AliasData *alias);

	bool IsIntegerType(TypeBase* type);
	bool IsFloatingPointType(TypeBase* type);
	bool IsNumericType(TypeBase* type);
	TypeBase* GetBinaryOpResultType(TypeBase* a, TypeBase* b);

	TypeRef* GetReferenceType(TypeBase* type);
	TypeArray* GetArrayType(TypeBase* type, long long size);
	TypeUnsizedArray* GetUnsizedArrayType(TypeBase* type);
	TypeFunction* GetFunctionType(TypeBase* returnType, IntrusiveList<TypeHandle> arguments);

	// Full state info
	FastVector<NamespaceData*> namespaces;
	FastVector<TypeBase*> types;
	FastVector<FunctionData*> functions;
	FastVector<VariableData*> variables;

	HashMap<TypeClass*> genericTypeMap;

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

	TypeBase* typeAuto;
	TypeStruct* typeAutoRef;
	TypeStruct* typeAutoArray;
};

struct ExprBase
{
	ExprBase(unsigned typeID, SynBase *source, TypeBase *type): typeID(typeID), source(source), type(type), next(0)
	{
	}

	virtual ~ExprBase()
	{
	}

	unsigned typeID;

	SynBase *source;
	TypeBase *type;
	ExprBase *next;
};

struct ExprVoid: ExprBase
{
	ExprVoid(SynBase *source, TypeBase *type): ExprBase(myTypeID, source, type)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct ExprBoolLiteral: ExprBase
{
	ExprBoolLiteral(SynBase *source, TypeBase *type, bool value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	bool value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprCharacterLiteral: ExprBase
{
	ExprCharacterLiteral(SynBase *source, TypeBase *type, unsigned char value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	unsigned char value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprStringLiteral: ExprBase
{
	ExprStringLiteral(SynBase *source, TypeBase *type, char *value, unsigned length): ExprBase(myTypeID, source, type), value(value), length(length)
	{
	}

	char *value;
	unsigned length;

	static const unsigned myTypeID = __LINE__;
};

struct ExprIntegerLiteral: ExprBase
{
	ExprIntegerLiteral(SynBase *source, TypeBase *type, long long value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	long long value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprRationalLiteral: ExprBase
{
	ExprRationalLiteral(SynBase *source, TypeBase *type, double value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	double value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprTypeLiteral: ExprBase
{
	ExprTypeLiteral(SynBase *source, TypeBase *type, TypeBase *value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	TypeBase *value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprNullptrLiteral: ExprBase
{
	ExprNullptrLiteral(SynBase *source, TypeBase *type): ExprBase(myTypeID, source, type)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct ExprArray: ExprBase
{
	ExprArray(SynBase *source, TypeBase *type, IntrusiveList<ExprBase> values): ExprBase(myTypeID, source, type), values(values)
	{
	}

	IntrusiveList<ExprBase> values;

	static const unsigned myTypeID = __LINE__;
};

struct ExprPreModify: ExprBase
{
	ExprPreModify(SynBase *source, TypeBase *type, ExprBase* value, bool isIncrement): ExprBase(myTypeID, source, type), value(value), isIncrement(isIncrement)
	{
	}

	ExprBase* value;
	bool isIncrement;

	static const unsigned myTypeID = __LINE__;
};

struct ExprPostModify: ExprBase
{
	ExprPostModify(SynBase *source, TypeBase *type, ExprBase* value, bool isIncrement): ExprBase(myTypeID, source, type), value(value), isIncrement(isIncrement)
	{
	}

	ExprBase* value;
	bool isIncrement;

	static const unsigned myTypeID = __LINE__;
};

struct ExprTypeCast: ExprBase
{
	ExprTypeCast(SynBase *source, TypeBase *type, ExprBase* value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprUnaryOp: ExprBase
{
	ExprUnaryOp(SynBase *source, TypeBase *type, SynUnaryOpType op, ExprBase* value): ExprBase(myTypeID, source, type), op(op), value(value)
	{
	}

	SynUnaryOpType op;

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprBinaryOp: ExprBase
{
	ExprBinaryOp(SynBase *source, TypeBase *type, SynBinaryOpType op, ExprBase* lhs, ExprBase* rhs): ExprBase(myTypeID, source, type), op(op), lhs(lhs), rhs(rhs)
	{
	}

	SynBinaryOpType op;

	ExprBase* lhs;
	ExprBase* rhs;

	static const unsigned myTypeID = __LINE__;
};

struct ExprGetAddress: ExprBase
{
	ExprGetAddress(SynBase *source, TypeBase *type, ExprBase* value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprDereference: ExprBase
{
	ExprDereference(SynBase *source, TypeBase *type, ExprBase* value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprConditional: ExprBase
{
	ExprConditional(SynBase *source, TypeBase *type, ExprBase *condition, ExprBase *trueBlock, ExprBase *falseBlock): ExprBase(myTypeID, source, type), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	ExprBase *condition;
	ExprBase *trueBlock;
	ExprBase *falseBlock;

	static const unsigned myTypeID = __LINE__;
};

struct ExprAssignment: ExprBase
{
	ExprAssignment(SynBase *source, TypeBase *type, ExprBase *lhs, ExprBase *rhs): ExprBase(myTypeID, source, type), lhs(lhs), rhs(rhs)
	{
	}

	ExprBase *lhs;
	ExprBase *rhs;

	static const unsigned myTypeID = __LINE__;
};

struct ExprModifyAssignment: ExprBase
{
	ExprModifyAssignment(SynBase *source, TypeBase *type, SynModifyAssignType op, ExprBase* lhs, ExprBase* rhs): ExprBase(myTypeID, source, type), op(op), lhs(lhs), rhs(rhs)
	{
	}

	SynModifyAssignType op;

	ExprBase* lhs;
	ExprBase* rhs;

	static const unsigned myTypeID = __LINE__;
};

struct ExprMemberAccess: ExprBase
{
	ExprMemberAccess(SynBase *source, TypeBase *type, ExprBase *value, VariableData *member): ExprBase(myTypeID, source, type), value(value), member(member)
	{
	}

	ExprBase *value;
	VariableData *member;

	static const unsigned myTypeID = __LINE__;
};

struct ExprArrayIndex: ExprBase
{
	ExprArrayIndex(SynBase *source, TypeBase *type, ExprBase *value, ExprBase *index): ExprBase(myTypeID, source, type), value(value), index(index)
	{
	}

	ExprBase *value;
	ExprBase *index;

	static const unsigned myTypeID = __LINE__;
};

struct ExprReturn: ExprBase
{
	ExprReturn(SynBase *source, TypeBase *type, ExprBase* value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct ExprVariableDefinition: ExprBase
{
	ExprVariableDefinition(SynBase *source, TypeBase *type, VariableData* variable, ExprBase* initializer): ExprBase(myTypeID, source, type), variable(variable), initializer(initializer)
	{
	}

	VariableData* variable;

	ExprBase* initializer;

	static const unsigned myTypeID = __LINE__;
};

struct ExprVariableDefinitions: ExprBase
{
	ExprVariableDefinitions(SynBase *source, TypeBase *type, IntrusiveList<ExprVariableDefinition> definitions): ExprBase(myTypeID, source, type), definitions(definitions)
	{
	}

	IntrusiveList<ExprVariableDefinition> definitions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprVariableAccess: ExprBase
{
	ExprVariableAccess(SynBase *source, TypeBase *type, VariableData *variable): ExprBase(myTypeID, source, type), variable(variable)
	{
	}

	VariableData *variable;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionDefinition: ExprBase
{
	ExprFunctionDefinition(SynBase *source, TypeBase *type, bool prototype, FunctionData* function, IntrusiveList<ExprVariableDefinition> arguments, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, source, type), prototype(prototype), function(function), arguments(arguments), expressions(expressions)
	{
	}

	bool prototype;

	FunctionData* function;

	IntrusiveList<ExprVariableDefinition> arguments;

	IntrusiveList<ExprBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprGenericFunctionPrototype: ExprBase
{
	ExprGenericFunctionPrototype(SynBase *source, TypeBase *type, FunctionData* function): ExprBase(myTypeID, source, type), function(function)
	{
	}

	FunctionData* function;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionAccess: ExprBase
{
	ExprFunctionAccess(SynBase *source, TypeBase *type, FunctionData *function): ExprBase(myTypeID, source, type), function(function)
	{
	}

	FunctionData *function;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionCall: ExprBase
{
	ExprFunctionCall(SynBase *source, TypeBase *type, ExprBase *function, IntrusiveList<ExprBase> arguments): ExprBase(myTypeID, source, type), function(function), arguments(arguments)
	{
	}

	ExprBase *function;
	IntrusiveList<ExprBase> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct ExprClassDefinition: ExprBase
{
	ExprClassDefinition(SynBase *source, TypeBase *type, TypeClass *classType): ExprBase(myTypeID, source, type), classType(classType)
	{
	}

	TypeClass *classType;

	IntrusiveList<ExprBase> functions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprIfElse: ExprBase
{
	ExprIfElse(SynBase *source, TypeBase *type, ExprBase *condition, ExprBase *trueBlock, ExprBase *falseBlock): ExprBase(myTypeID, source, type), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	ExprBase *condition;
	ExprBase *trueBlock;
	ExprBase *falseBlock;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFor: ExprBase
{
	ExprFor(SynBase *source, TypeBase *type, ExprBase *initializer, ExprBase *condition, ExprBase *increment, ExprBase *body): ExprBase(myTypeID, source, type), initializer(initializer), condition(condition), increment(increment), body(body)
	{
	}

	ExprBase *initializer;
	ExprBase *condition;
	ExprBase *increment;
	ExprBase *body;

	static const unsigned myTypeID = __LINE__;
};

struct ExprWhile: ExprBase
{
	ExprWhile(SynBase *source, TypeBase *type, ExprBase *condition, ExprBase *body): ExprBase(myTypeID, source, type), condition(condition), body(body)
	{
	}

	ExprBase *condition;
	ExprBase *body;

	static const unsigned myTypeID = __LINE__;
};

struct ExprDoWhile: ExprBase
{
	ExprDoWhile(SynBase *source, TypeBase *type, ExprBase *body, ExprBase *condition): ExprBase(myTypeID, source, type), body(body), condition(condition)
	{
	}

	ExprBase *body;
	ExprBase *condition;

	static const unsigned myTypeID = __LINE__;
};

struct ExprBlock: ExprBase
{
	ExprBlock(SynBase *source, TypeBase *type, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, source, type), expressions(expressions)
	{
	}

	IntrusiveList<ExprBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprModule: ExprBase
{
	ExprModule(SynBase *source, TypeBase *type, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, source, type), expressions(expressions)
	{
	}

	IntrusiveList<ExprBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

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
