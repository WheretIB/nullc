#pragma once

#include "stdafx.h"
#include "IntrusiveList.h"
#include "Allocator.h"
#include "Array.h"
#include "HashMap.h"
#include "StrAlgo.h"

#include "ParseTree.h"
#include "TypeTree.h"

struct ExprBase;

struct ExpressionContext
{
	ExpressionContext(Allocator *allocator, const char *code);

	void Stop(const char *pos, const char *msg, ...);
	void Stop(InplaceStr pos, const char *msg, ...);

	void PushScope(ScopeType type);
	void PushScope(NamespaceData *nameSpace);
	void PushScope(FunctionData *function);
	void PushScope(TypeBase *type);
	void PushLoopScope();
	void PushTemporaryScope();
	void PopScope(ScopeType type, SynBase *location, bool keepFunctions);
	void PopScope(ScopeType type);
	void RestoreScopesAtPoint(ScopeData *target, SynBase *location);
	void SwitchToScopeAtPoint(SynBase *currLocation, ScopeData *target, SynBase *targetLocation);

	NamespaceData* GetCurrentNamespace();
	FunctionData* GetCurrentFunction();
	TypeBase* GetCurrentType();

	FunctionData* GetFunctionOwner(ScopeData *scope);

	unsigned GetGenericClassInstantiationDepth();

	void AddType(TypeBase *type);
	void AddFunction(FunctionData *function);
	void AddVariable(VariableData *variable);
	void AddAlias(AliasData *alias);

	unsigned GetTypeIndex(TypeBase *type);
	unsigned GetFunctionIndex(FunctionData *data);

	void HideFunction(FunctionData *function);

	bool IsGenericFunction(FunctionData *function);

	bool IsIntegerType(TypeBase* type);
	bool IsFloatingPointType(TypeBase* type);
	bool IsNumericType(TypeBase* type);
	TypeBase* GetBinaryOpResultType(TypeBase* a, TypeBase* b);

	TypeRef* GetReferenceType(TypeBase* type);
	TypeArray* GetArrayType(TypeBase* type, long long size);
	TypeUnsizedArray* GetUnsizedArrayType(TypeBase* type);
	TypeFunction* GetFunctionType(TypeBase* returnType, IntrusiveList<TypeHandle> arguments);
	TypeFunction* GetFunctionType(TypeBase* returnType, SmallArray<ArgumentData, 32> &arguments);

	// Source code
	const char *code;

	// Full state info
	SmallArray<NamespaceData*, 128> namespaces;
	SmallArray<TypeBase*, 128> types;
	SmallArray<FunctionData*, 128> functions;
	SmallArray<VariableData*, 128> variables;

	SmallArray<ExprBase*, 128> definitions;
	SmallArray<ExprBase*, 128> setup;
	SmallArray<VariableData*, 128> vtables;
	SmallArray<VariableData*, 128> upvalues;

	SmallArray<TypeFunction*, 128> functionTypes;

	HashMap<TypeClass*> genericTypeMap;

	unsigned baseModuleFunctionCount;

	// Context info
	HashMap<TypeBase*> typeMap;
	HashMap<FunctionData*> functionMap;
	HashMap<VariableData*> variableMap;

	ScopeData *scope;

	ScopeData *globalScope;

	// Error info
	jmp_buf errorHandler;
	const char *errorPos;
	char *errorBuf;
	unsigned errorBufSize;

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
	TypeBase* typeNullPtr;

	TypeBase* typeAuto;
	TypeStruct* typeAutoRef;
	TypeStruct* typeAutoArray;

	// Counters
	unsigned uniqueNamespaceId;
	unsigned uniqueVariableId;
	unsigned uniqueFunctionId;
	unsigned uniqueAliasId;
	unsigned uniqueScopeId;

	unsigned unnamedFuncCount;
	unsigned unnamedVariableCount;

	// Memory pool
	Allocator *allocator;

	template<typename T>
	T* get()
	{
		return (T*)allocator->alloc(sizeof(T));
	}
};

struct ExprBase
{
	ExprBase(unsigned typeID, SynBase *source, TypeBase *type): typeID(typeID), source(source), type(type), next(0), listed(false)
	{
	}

	virtual ~ExprBase()
	{
	}

	unsigned typeID;

	SynBase *source;
	TypeBase *type;
	ExprBase *next;
	bool listed;
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

struct ExprFunctionIndexLiteral: ExprBase
{
	ExprFunctionIndexLiteral(SynBase *source, TypeBase *type, FunctionData *function): ExprBase(myTypeID, source, type), function(function)
	{
	}

	FunctionData *function;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionLiteral: ExprBase
{
	ExprFunctionLiteral(SynBase *source, TypeBase *type, FunctionData *data, ExprBase *context): ExprBase(myTypeID, source, type), data(data), context(context)
	{
	}

	FunctionData *data;

	ExprBase *context;

	static const unsigned myTypeID = __LINE__;
};

struct ExprPointerLiteral: ExprBase
{
	ExprPointerLiteral(SynBase *source, TypeBase *type, unsigned char *ptr, unsigned char *end): ExprBase(myTypeID, source, type), ptr(ptr), end(end)
	{
		assert(ptr <= end);
	}

	unsigned char *ptr;
	unsigned char *end;

	static const unsigned myTypeID = __LINE__;
};

struct ExprMemoryLiteral: ExprBase
{
	ExprMemoryLiteral(SynBase *source, TypeBase *type, ExprPointerLiteral *ptr): ExprBase(myTypeID, source, type), ptr(ptr)
	{
	}

	ExprPointerLiteral *ptr;

	static const unsigned myTypeID = __LINE__;
};

struct ExprPassthrough: ExprBase
{
	ExprPassthrough(SynBase *source, TypeBase *type, ExprBase *value): ExprBase(myTypeID, source, type), value(value)
	{
		assert(type == value->type);
	}

	ExprBase *value;

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

enum ExprTypeCastCategory
{
	EXPR_CAST_NUMERICAL,
	EXPR_CAST_PTR_TO_BOOL,
	EXPR_CAST_UNSIZED_TO_BOOL,
	EXPR_CAST_FUNCTION_TO_BOOL,
	EXPR_CAST_NULL_TO_PTR,
	EXPR_CAST_NULL_TO_AUTO_PTR,
	EXPR_CAST_NULL_TO_UNSIZED,
	EXPR_CAST_NULL_TO_AUTO_ARRAY,
	EXPR_CAST_NULL_TO_FUNCTION,
	EXPR_CAST_ARRAY_PTR_TO_UNSIZED,
	EXPR_CAST_ARRAY_PTR_TO_UNSIZED_PTR,
	EXPR_CAST_PTR_TO_AUTO_PTR,
	EXPR_CAST_ANY_TO_PTR,
	EXPR_CAST_AUTO_PTR_TO_PTR,
	EXPR_CAST_UNSIZED_TO_AUTO_ARRAY,
	EXPR_CAST_DERIVED_TO_BASE,
	EXPR_CAST_REINTERPRET,
};

struct ExprTypeCast: ExprBase
{
	ExprTypeCast(SynBase *source, TypeBase *type, ExprBase* value, ExprTypeCastCategory category): ExprBase(myTypeID, source, type), value(value), category(category)
	{
	}

	ExprBase* value;

	ExprTypeCastCategory category;

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
	ExprGetAddress(SynBase *source, TypeBase *type, VariableData *variable): ExprBase(myTypeID, source, type), variable(variable)
	{
		assert(!variable->lookupOnly);
	}

	VariableData *variable;

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

struct ExprUnboxing: ExprBase
{
	ExprUnboxing(SynBase *source, TypeBase *type, ExprBase* value): ExprBase(myTypeID, source, type), value(value)
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
		TypeRef *refType = getType<TypeRef>(lhs->type);

		assert(refType);
		assert(refType->subType == rhs->type);
	}

	ExprBase *lhs;
	ExprBase *rhs;

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
	ExprReturn(SynBase *source, TypeBase *type, ExprBase* value, ExprBase *coroutineStateUpdate, IntrusiveList<ExprBase> closures): ExprBase(myTypeID, source, type), value(value), coroutineStateUpdate(coroutineStateUpdate), closures(closures)
	{
	}

	ExprBase* value;

	ExprBase *coroutineStateUpdate;

	IntrusiveList<ExprBase> closures;

	static const unsigned myTypeID = __LINE__;
};

struct ExprYield: ExprBase
{
	ExprYield(SynBase *source, TypeBase *type, ExprBase* value, ExprBase *coroutineStateUpdate, IntrusiveList<ExprBase> closures, unsigned order): ExprBase(myTypeID, source, type), value(value), coroutineStateUpdate(coroutineStateUpdate), closures(closures), order(order)
	{
	}

	ExprBase* value;

	ExprBase *coroutineStateUpdate;

	IntrusiveList<ExprBase> closures;

	// 1-based index of the yield in the current function
	unsigned order;

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

struct ExprArraySetup: ExprBase
{
	ExprArraySetup(SynBase *source, TypeBase *type, ExprBase *lhs, ExprBase* initializer): ExprBase(myTypeID, source, type), lhs(lhs), initializer(initializer)
	{
	}

	ExprBase *lhs;

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
		assert(variable);
		assert(type == variable->type);
		assert(!variable->lookupOnly);
	}

	VariableData *variable;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionDefinition: ExprBase
{
	ExprFunctionDefinition(SynBase *source, TypeBase *type, FunctionData *function, ExprVariableDefinition *contextArgument, IntrusiveList<ExprVariableDefinition> arguments, ExprBase *coroutineStateRead, IntrusiveList<ExprBase> expressions, ExprVariableDefinition *contextVariable): ExprBase(myTypeID, source, type), function(function), contextArgument(contextArgument), arguments(arguments), coroutineStateRead(coroutineStateRead), expressions(expressions), contextVariable(contextVariable)
	{
	}

	FunctionData* function;

	ExprVariableDefinition *contextArgument;

	IntrusiveList<ExprVariableDefinition> arguments;

	ExprBase *coroutineStateRead;

	IntrusiveList<ExprBase> expressions;

	ExprVariableDefinition *contextVariable;

	static const unsigned myTypeID = __LINE__;
};

struct ExprGenericFunctionPrototype: ExprBase
{
	ExprGenericFunctionPrototype(SynBase *source, TypeBase *type, FunctionData* function): ExprBase(myTypeID, source, type), function(function)
	{
	}

	FunctionData* function;

	IntrusiveList<ExprVariableDefinition> contextVariables;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionAccess: ExprBase
{
	ExprFunctionAccess(SynBase *source, TypeBase *type, FunctionData *function, ExprBase *context): ExprBase(myTypeID, source, type), function(function), context(context)
	{
		assert(context);

		assert(context->type->name != InplaceStr("auto ref"));

		if(!function->type->isGeneric && !(function->scope->ownerType && function->scope->ownerType->isGeneric))
			assert(function->contextType == context->type);
	}

	FunctionData *function;

	ExprBase *context;

	static const unsigned myTypeID = __LINE__;
};

struct ExprFunctionOverloadSet: ExprBase
{
	ExprFunctionOverloadSet(SynBase *source, TypeBase *type, IntrusiveList<FunctionHandle> functions, ExprBase *context): ExprBase(myTypeID, source, type), functions(functions), context(context)
	{
	}

	IntrusiveList<FunctionHandle> functions;

	ExprBase *context;

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

struct ExprAliasDefinition: ExprBase
{
	ExprAliasDefinition(SynBase *source, TypeBase *type, AliasData *alias): ExprBase(myTypeID, source, type), alias(alias)
	{
	}

	AliasData *alias;

	static const unsigned myTypeID = __LINE__;
};

struct ExprClassPrototype: ExprBase
{
	ExprClassPrototype(SynBase *source, TypeBase *type, TypeClass *classType): ExprBase(myTypeID, source, type), classType(classType)
	{
	}

	TypeClass *classType;

	static const unsigned myTypeID = __LINE__;
};

struct ExprGenericClassPrototype: ExprBase
{
	ExprGenericClassPrototype(SynBase *source, TypeBase *type, TypeGenericClassProto *genericProtoType): ExprBase(myTypeID, source, type), genericProtoType(genericProtoType)
	{
	}

	TypeGenericClassProto *genericProtoType;

	static const unsigned myTypeID = __LINE__;
};

struct ExprClassDefinition: ExprBase
{
	ExprClassDefinition(SynBase *source, TypeBase *type, TypeClass *classType): ExprBase(myTypeID, source, type), classType(classType)
	{
	}

	TypeClass *classType;

	IntrusiveList<ExprAliasDefinition> aliases;
	IntrusiveList<ExprBase> functions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprEnumDefinition: ExprBase
{
	ExprEnumDefinition(SynBase *source, TypeBase *type, TypeEnum *enumType, ExprBase *toInt, ExprBase *toEnum): ExprBase(myTypeID, source, type), enumType(enumType), toInt(toInt), toEnum(toEnum)
	{
	}

	TypeEnum *enumType;

	ExprBase *toInt;
	ExprBase *toEnum;

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

struct ExprSwitch: ExprBase
{
	ExprSwitch(SynBase *source, TypeBase *type, ExprBase *condition, IntrusiveList<ExprBase> cases, IntrusiveList<ExprBase> blocks, ExprBase *defaultBlock): ExprBase(myTypeID, source, type), condition(condition), cases(cases), blocks(blocks), defaultBlock(defaultBlock)
	{
	}

	ExprBase *condition;
	IntrusiveList<ExprBase> cases;
	IntrusiveList<ExprBase> blocks;
	ExprBase *defaultBlock;

	static const unsigned myTypeID = __LINE__;
};

struct ExprBreak: ExprBase
{
	ExprBreak(SynBase *source, TypeBase *type, unsigned depth, IntrusiveList<ExprBase> closures): ExprBase(myTypeID, source, type), depth(depth), closures(closures)
	{
	}

	unsigned depth;

	IntrusiveList<ExprBase> closures;

	static const unsigned myTypeID = __LINE__;
};

struct ExprContinue: ExprBase
{
	ExprContinue(SynBase *source, TypeBase *type, unsigned depth, IntrusiveList<ExprBase> closures): ExprBase(myTypeID, source, type), depth(depth), closures(closures)
	{
	}

	unsigned depth;

	IntrusiveList<ExprBase> closures;

	static const unsigned myTypeID = __LINE__;
};

struct ExprBlock: ExprBase
{
	ExprBlock(SynBase *source, TypeBase *type, IntrusiveList<ExprBase> expressions, IntrusiveList<ExprBase> closures): ExprBase(myTypeID, source, type), expressions(expressions), closures(closures)
	{
	}

	IntrusiveList<ExprBase> expressions;

	IntrusiveList<ExprBase> closures;

	static const unsigned myTypeID = __LINE__;
};

struct ExprSequence: ExprBase
{
	ExprSequence(SynBase *source, TypeBase *type, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, source, type), expressions(expressions)
	{
	}

	IntrusiveList<ExprBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct ExprModule: ExprBase
{
	ExprModule(Allocator *allocator, SynBase *source, TypeBase *type, ScopeData *moduleScope, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, source, type), moduleScope(moduleScope), expressions(expressions), definitions(allocator)
	{
	}

	ScopeData *moduleScope;

	SmallArray<ExprBase*, 32> definitions;

	IntrusiveList<ExprBase> setup;

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
