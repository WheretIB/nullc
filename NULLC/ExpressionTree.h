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

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())
#endif

struct GenericFunctionInstanceTypeRequest
{
	GenericFunctionInstanceTypeRequest(): hash(0), parentType(NULL), function(NULL)
	{
	}

	GenericFunctionInstanceTypeRequest(TypeBase *parentType, FunctionData *function, IntrusiveList<TypeHandle> arguments, IntrusiveList<MatchData> aliases): parentType(parentType), function(function), arguments(arguments), aliases(aliases)
	{
		hash = parentType ? parentType->nameHash : 0;

		hash = hash + (hash << 5) + function->nameHash;

		for(TypeHandle *curr = arguments.head; curr; curr = curr->next)
			hash = hash + (hash << 5) + curr->type->nameHash;

		for(MatchData *curr = aliases.head; curr; curr = curr->next)
			hash = hash + (hash << 5) + curr->type->nameHash;
	}

	bool operator==(const GenericFunctionInstanceTypeRequest& rhs) const
	{
		if(parentType != rhs.parentType)
			return false;

		if(function != rhs.function)
			return false;

		for(TypeHandle *leftArg = arguments.head, *rightArg = rhs.arguments.head; leftArg || rightArg; leftArg = leftArg->next, rightArg = rightArg->next)
		{
			if(!leftArg || !rightArg)
				return false;

			if(leftArg->type != rightArg->type)
				return false;
		}

		for(MatchData *leftAlias = aliases.head, *rightAlias = rhs.aliases.head; leftAlias || rightAlias; leftAlias = leftAlias->next, rightAlias = rightAlias->next)
		{
			if(!leftAlias || !rightAlias)
				return false;

			if(leftAlias->name != rightAlias->name || leftAlias->type != rightAlias->type)
				return false;
		}

		return true;
	}

	bool operator!=(const GenericFunctionInstanceTypeRequest& rhs) const
	{
		return !(*this == rhs);
	}

	unsigned hash;

	TypeBase *parentType;

	FunctionData *function;

	IntrusiveList<TypeHandle> arguments;
	IntrusiveList<MatchData> aliases;
};

struct GenericFunctionInstanceTypeRequestHasher
{
	unsigned operator()(const GenericFunctionInstanceTypeRequest& key)
	{
		return key.hash;
	}
};

struct GenericFunctionInstanceTypeResponse
{
	GenericFunctionInstanceTypeResponse(): functionType(NULL)
	{
	}

	GenericFunctionInstanceTypeResponse(TypeFunction *functionType, IntrusiveList<MatchData> aliases): functionType(functionType), aliases(aliases)
	{
	}

	TypeFunction *functionType;

	IntrusiveList<MatchData> aliases;
};

struct TypedFunctionInstanceRequest
{
	TypedFunctionInstanceRequest() : hash(0), instanceType(NULL), syntax(NULL)
	{
	}

	TypedFunctionInstanceRequest(TypeBase *instanceType, SynBase *syntax) : instanceType(instanceType), syntax(syntax)
	{
		hash = (instanceType ? instanceType->nameHash : 0);

		hash = hash + (hash << 5) + unsigned((uintptr_t(syntax) >> 16) + uintptr_t(syntax));
	}

	bool operator==(const TypedFunctionInstanceRequest& rhs) const
	{
		if(instanceType != rhs.instanceType)
			return false;

		if(syntax != rhs.syntax)
			return false;

		return true;
	}

	bool operator!=(const TypedFunctionInstanceRequest& rhs) const
	{
		return !(*this == rhs);
	}

	unsigned hash;

	TypeBase *instanceType;
	SynBase *syntax;
};

struct TypedFunctionInstanceRequestHasher
{
	unsigned operator()(const TypedFunctionInstanceRequest& key)
	{
		return key.hash;
	}
};

struct TypePair
{
	TypePair(): a(NULL), b(NULL)
	{
	}

	TypePair(TypeBase *a, TypeBase *b): a(a), b(b)
	{
	}

	bool operator==(const TypePair& rhs) const
	{
		return a == rhs.a && b == rhs.b;
	}

	bool operator!=(const TypePair& rhs) const
	{
		return a != rhs.a || b != rhs.b;
	}

	TypeBase *a;
	TypeBase *b;
};

struct TypePairHasher
{
	unsigned operator()(const TypePair& key)
	{
		return key.a->nameHash + key.b->nameHash;
	}
};

struct TypeModulePair
{
	TypeModulePair(): importModule(NULL)
	{
	}

	TypeModulePair(InplaceStr typeName, ModuleData *importModule): typeName(typeName), importModule(importModule)
	{
	}

	bool operator==(const TypeModulePair& rhs) const
	{
		return typeName == rhs.typeName && importModule == rhs.importModule;
	}

	bool operator!=(const TypeModulePair& rhs) const
	{
		return typeName != rhs.typeName || importModule != rhs.importModule;
	}

	InplaceStr typeName;
	ModuleData *importModule;
};

struct TypeModulePairHasher
{
	unsigned operator()(const TypeModulePair& key)
	{
		return key.typeName.hash() + key.importModule->name.hash();
	}
};

struct FunctionTypeRequest
{
	FunctionTypeRequest(): returnType(NULL), hash(0)
	{
	}

	FunctionTypeRequest(TypeBase* returnType, IntrusiveList<TypeHandle> arguments): returnType(returnType), arguments(arguments)
	{
		hash = returnType->nameHash;

		for(TypeHandle *arg = arguments.head; arg; arg = arg->next)
			hash += arg->type->nameHash;
	}

	bool operator==(const FunctionTypeRequest& rhs) const
	{
		if(returnType != rhs.returnType)
			return false;

		TypeHandle *leftArg = arguments.head;
		TypeHandle *rightArg = rhs.arguments.head;

		while(leftArg && rightArg && leftArg->type == rightArg->type)
		{
			leftArg = leftArg->next;
			rightArg = rightArg->next;
		}

		if(leftArg != rightArg)
			return false;

		return true;
	}

	bool operator!=(const FunctionTypeRequest& rhs) const
	{
		return !(*this == rhs);
	}

	TypeBase* returnType;
	IntrusiveList<TypeHandle> arguments;
	unsigned hash;
};

struct FunctionTypeRequestHasher
{
	unsigned operator()(const FunctionTypeRequest& key)
	{
		return key.hash;
	}
};

struct ExpressionContext
{
	ExpressionContext(Allocator *allocator, int optimizationLevel);

	void StopAt(SynBase *source, const char *pos, const char *msg, ...) NULLC_PRINT_FORMAT_CHECK(4, 5);
	void Stop(SynBase *source, const char *msg, ...) NULLC_PRINT_FORMAT_CHECK(3, 4);

	void PushScope(ScopeType type);
	void PushScope(NamespaceData *nameSpace);
	void PushScope(FunctionData *function);
	void PushScope(TypeBase *type);
	void PushLoopScope(bool allowBreak, bool allowContinue);
	void PushTemporaryScope();
	void PopScope(ScopeType type, bool ejectContents, bool keepFunctions);
	void PopScope(ScopeType type);
	void RestoreScopesAtPoint(ScopeData *target, SynBase *location);
	void SwitchToScopeAtPoint(ScopeData *target, SynBase *targetLocation);

	NamespaceData* GetCurrentNamespace(ScopeData *scopeData);
	FunctionData* GetCurrentFunction(ScopeData *scopeData);
	TypeBase* GetCurrentType(ScopeData *scopeData);

	FunctionData* GetFunctionOwner(ScopeData *scope);

	ScopeData* NamespaceScopeFrom(ScopeData *scope);
	ScopeData* GlobalScopeFrom(ScopeData *scope);

	bool IsGenericInstance(FunctionData *function);

	void AddType(TypeBase *type);
	void AddFunction(FunctionData *function);
	void AddVariable(VariableData *variable, bool visible);
	void AddAlias(AliasData *alias);

	unsigned GetTypeIndex(TypeBase *type);
	unsigned GetFunctionIndex(FunctionData *data);

	void HideFunction(FunctionData *function);

	bool IsGenericFunction(FunctionData *function);

	bool IsIntegerType(TypeBase* type);
	bool IsFloatingPointType(TypeBase* type);
	bool IsNumericType(TypeBase* type);
	TypeBase* GetBinaryOpResultType(TypeBase* a, TypeBase* b);

	TypeError* GetErrorType();
	TypeRef* GetReferenceType(TypeBase* type);
	TypeArray* GetArrayType(TypeBase* type, long long size);
	TypeUnsizedArray* GetUnsizedArrayType(TypeBase* type);
	TypeFunction* GetFunctionType(SynBase *source, TypeBase* returnType, IntrusiveList<TypeHandle> arguments);
	TypeFunction* GetFunctionType(SynBase *source, TypeBase* returnType, ArrayView<ArgumentData> arguments);
	TypeFunctionSet* GetFunctionSetType(ArrayView<TypeBase*> types);
	TypeGenericAlias* GetGenericAliasType(SynIdentifier *baseName);
	TypeGenericClass* GetGenericClassType(SynBase *source, TypeGenericClassProto *proto, IntrusiveList<TypeHandle> generics);

	ModuleData* GetSourceOwner(Lexeme *lexeme);

	SynInternal* MakeInternal(SynBase *source);

	int optimizationLevel;

	// Full state info
	const char *code;
	const char *codeEnd;

	const char *moduleRoot;

	SmallArray<ModuleData*, 128> uniqueDependencies;
	SmallArray<ModuleData*, 128> imports;
	SmallArray<ModuleData*, 128> implicitImports;
	SmallArray<NamespaceData*, 128> namespaces;
	SmallArray<TypeBase*, 128> types;
	SmallArray<FunctionData*, 128> functions;
	SmallArray<VariableData*, 128> variables;

	SmallArray<ExprBase*, 128> definitions;
	SmallArray<ExprBase*, 128> setup;

	SmallArray<VariableData*, 128> vtables;
	SmallDenseMap<InplaceStr, VariableData*, InplaceStrHasher, 128> vtableMap;

	SmallArray<VariableData*, 128> upvalues;
	SmallDenseMap<InplaceStr, VariableData*, InplaceStrHasher, 128> upvalueMap;

	SmallArray<TypeFunction*, 128> functionTypes;
	SmallArray<TypeFunctionSet*, 128> functionSetTypes;
	SmallArray<TypeGenericAlias*, 128> genericAliasTypes;
	SmallArray<TypeGenericClass*, 128> genericClassTypes;

	HashMap<TypeClass*> genericTypeMap;

	SmallDenseMap<GenericFunctionInstanceTypeRequest, GenericFunctionInstanceTypeResponse, GenericFunctionInstanceTypeRequestHasher, 32> genericFunctionInstanceTypeMap;

	SmallDenseSet<TypePair, TypePairHasher, 32> noAssignmentOperatorForTypePair;

	SmallDenseMap<TypedFunctionInstanceRequest, ExprBase*, TypedFunctionInstanceRequestHasher, 32> newConstructorFunctions;

	unsigned baseModuleFunctionCount;

	// Context info
	HashMap<TypeBase*> typeMap;
	HashMap<FunctionData*> functionMap;
	HashMap<VariableData*> variableMap;

	ScopeData *scope;

	ScopeData *globalScope;
	SmallArray<NamespaceData*, 2> globalNamespaces;
	IntrusiveList<CloseUpvaluesData> globalCloseUpvalues;

	SmallDenseMap<TypeModulePair, TypeBase*, TypeModulePairHasher, 32> internalTypeMap;
	SmallDenseMap<FunctionTypeRequest, TypeFunction*, FunctionTypeRequestHasher, 32> functionTypeMap;

	unsigned functionInstanceDepth;
	unsigned classInstanceDepth;
	unsigned expressionDepth;

	unsigned memoryLimit;

	// Error info
	bool errorHandlerActive;
	bool errorHandlerNested;
	jmp_buf errorHandler;
	const char *errorPos;
	unsigned errorCount;
	char *errorBuf;
	unsigned errorBufSize;
	char *errorBufLocation;

	SmallArray<ErrorInfo*, 4> errorInfo;

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

	TypeBase* typeGeneric;

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

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

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

namespace ExprNode
{
	enum ExprNodeId
	{
		ExprError,
		ExprErrorTypeMemberAccess,
		ExprVoid,
		ExprBoolLiteral,
		ExprCharacterLiteral,
		ExprStringLiteral,
		ExprIntegerLiteral,
		ExprRationalLiteral,
		ExprTypeLiteral,
		ExprNullptrLiteral,
		ExprFunctionIndexLiteral,
		ExprFunctionLiteral,
		ExprPointerLiteral,
		ExprMemoryLiteral,
		ExprPassthrough,
		ExprArray,
		ExprPreModify,
		ExprPostModify,
		ExprTypeCast,
		ExprUnaryOp,
		ExprBinaryOp,
		ExprGetAddress,
		ExprDereference,
		ExprUnboxing,
		ExprConditional,
		ExprAssignment,
		ExprMemberAccess,
		ExprArrayIndex,
		ExprReturn,
		ExprYield,
		ExprVariableDefinition,
		ExprZeroInitialize,
		ExprArraySetup,
		ExprVariableDefinitions,
		ExprVariableAccess,
		ExprFunctionContextAccess,
		ExprFunctionDefinition,
		ExprGenericFunctionPrototype,
		ExprFunctionAccess,
		ExprFunctionOverloadSet,
		ExprShortFunctionOverloadSet,
		ExprFunctionCall,
		ExprAliasDefinition,
		ExprClassPrototype,
		ExprGenericClassPrototype,
		ExprClassDefinition,
		ExprEnumDefinition,
		ExprIfElse,
		ExprFor,
		ExprWhile,
		ExprDoWhile,
		ExprSwitch,
		ExprBreak,
		ExprContinue,
		ExprBlock,
		ExprSequence,
		ExprModule,
	};
}

struct ExprError: ExprBase
{
	ExprError(SynBase *source, TypeBase *type): ExprBase(myTypeID, source, type)
	{
	}

	ExprError(SynBase *source, TypeBase *type, ExprBase *value): ExprBase(myTypeID, source, type)
	{
		values.push_back(value);
	}

	ExprError(SynBase *source, TypeBase *type, ExprBase *value1, ExprBase *value2): ExprBase(myTypeID, source, type)
	{
		values.push_back(value1);
		values.push_back(value2);
	}

	ExprError(Allocator *allocator, SynBase *source, TypeBase *type, ArrayView<ExprBase*> arr): ExprBase(myTypeID, source, type), values(allocator)
	{
		for(unsigned i = 0; i < arr.size(); i++)
			values.push_back(arr[i]);
	}

	SmallArray<ExprBase*, 4> values;

	static const unsigned myTypeID = ExprNode::ExprError;
};

struct ExprErrorTypeMemberAccess: ExprBase
{
	ExprErrorTypeMemberAccess(SynBase *source, TypeBase *type, TypeBase *value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	TypeBase *value;

	static const unsigned myTypeID = ExprNode::ExprErrorTypeMemberAccess;
};

struct ExprVoid: ExprBase
{
	ExprVoid(SynBase *source, TypeBase *type): ExprBase(myTypeID, source, type)
	{
	}

	static const unsigned myTypeID = ExprNode::ExprVoid;
};

struct ExprBoolLiteral: ExprBase
{
	ExprBoolLiteral(SynBase *source, TypeBase *type, bool value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	bool value;

	static const unsigned myTypeID = ExprNode::ExprBoolLiteral;
};

struct ExprCharacterLiteral: ExprBase
{
	ExprCharacterLiteral(SynBase *source, TypeBase *type, signed char value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	signed char value;

	static const unsigned myTypeID = ExprNode::ExprCharacterLiteral;
};

struct ExprStringLiteral: ExprBase
{
	ExprStringLiteral(SynBase *source, TypeBase *type, char *value, unsigned length): ExprBase(myTypeID, source, type), value(value), length(length)
	{
	}

	char *value;
	unsigned length;

	static const unsigned myTypeID = ExprNode::ExprStringLiteral;
};

struct ExprIntegerLiteral: ExprBase
{
	ExprIntegerLiteral(SynBase *source, TypeBase *type, long long value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	long long value;

	static const unsigned myTypeID = ExprNode::ExprIntegerLiteral;
};

struct ExprRationalLiteral: ExprBase
{
	ExprRationalLiteral(SynBase *source, TypeBase *type, double value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	double value;

	static const unsigned myTypeID = ExprNode::ExprRationalLiteral;
};

struct ExprTypeLiteral: ExprBase
{
	ExprTypeLiteral(SynBase *source, TypeBase *type, TypeBase *value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	TypeBase *value;

	static const unsigned myTypeID = ExprNode::ExprTypeLiteral;
};

struct ExprNullptrLiteral: ExprBase
{
	ExprNullptrLiteral(SynBase *source, TypeBase *type): ExprBase(myTypeID, source, type)
	{
	}

	static const unsigned myTypeID = ExprNode::ExprNullptrLiteral;
};

struct ExprFunctionIndexLiteral: ExprBase
{
	ExprFunctionIndexLiteral(SynBase *source, TypeBase *type, FunctionData *function): ExprBase(myTypeID, source, type), function(function)
	{
	}

	FunctionData *function;

	static const unsigned myTypeID = ExprNode::ExprFunctionIndexLiteral;
};

struct ExprFunctionLiteral: ExprBase
{
	ExprFunctionLiteral(SynBase *source, TypeBase *type, FunctionData *data, ExprBase *context): ExprBase(myTypeID, source, type), data(data), context(context)
	{
	}

	FunctionData *data;

	ExprBase *context;

	static const unsigned myTypeID = ExprNode::ExprFunctionLiteral;
};

struct ExprPointerLiteral: ExprBase
{
	ExprPointerLiteral(SynBase *source, TypeBase *type, unsigned char *ptr, unsigned char *end): ExprBase(myTypeID, source, type), ptr(ptr), end(end)
	{
		assert(ptr <= end);
	}

	unsigned char *ptr;
	unsigned char *end;

	static const unsigned myTypeID = ExprNode::ExprPointerLiteral;
};

struct ExprMemoryLiteral: ExprBase
{
	ExprMemoryLiteral(SynBase *source, TypeBase *type, ExprPointerLiteral *ptr): ExprBase(myTypeID, source, type), ptr(ptr)
	{
	}

	ExprPointerLiteral *ptr;

	static const unsigned myTypeID = ExprNode::ExprMemoryLiteral;
};

struct ExprPassthrough: ExprBase
{
	ExprPassthrough(SynBase *source, TypeBase *type, ExprBase *value): ExprBase(myTypeID, source, type), value(value)
	{
		assert(type == value->type);
	}

	ExprBase *value;

	static const unsigned myTypeID = ExprNode::ExprPassthrough;
};

struct ExprArray: ExprBase
{
	ExprArray(SynBase *source, TypeBase *type, IntrusiveList<ExprBase> values): ExprBase(myTypeID, source, type), values(values)
	{
	}

	IntrusiveList<ExprBase> values;

	static const unsigned myTypeID = ExprNode::ExprArray;
};

struct ExprPreModify: ExprBase
{
	ExprPreModify(SynBase *source, TypeBase *type, ExprBase* value, bool isIncrement): ExprBase(myTypeID, source, type), value(value), isIncrement(isIncrement)
	{
	}

	ExprBase* value;
	bool isIncrement;

	static const unsigned myTypeID = ExprNode::ExprPreModify;
};

struct ExprPostModify: ExprBase
{
	ExprPostModify(SynBase *source, TypeBase *type, ExprBase* value, bool isIncrement): ExprBase(myTypeID, source, type), value(value), isIncrement(isIncrement)
	{
	}

	ExprBase* value;
	bool isIncrement;

	static const unsigned myTypeID = ExprNode::ExprPostModify;
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
	EXPR_CAST_PTR_TO_AUTO_PTR,
	EXPR_CAST_AUTO_PTR_TO_PTR,
	EXPR_CAST_UNSIZED_TO_AUTO_ARRAY,
	EXPR_CAST_REINTERPRET
};

struct ExprTypeCast: ExprBase
{
	ExprTypeCast(SynBase *source, TypeBase *type, ExprBase* value, ExprTypeCastCategory category): ExprBase(myTypeID, source, type), value(value), category(category)
	{
	}

	ExprBase* value;

	ExprTypeCastCategory category;

	static const unsigned myTypeID = ExprNode::ExprTypeCast;
};

struct ExprUnaryOp: ExprBase
{
	ExprUnaryOp(SynBase *source, TypeBase *type, SynUnaryOpType op, ExprBase* value): ExprBase(myTypeID, source, type), op(op), value(value)
	{
	}

	SynUnaryOpType op;

	ExprBase* value;

	static const unsigned myTypeID = ExprNode::ExprUnaryOp;
};

struct ExprBinaryOp: ExprBase
{
	ExprBinaryOp(SynBase *source, TypeBase *type, SynBinaryOpType op, ExprBase* lhs, ExprBase* rhs): ExprBase(myTypeID, source, type), op(op), lhs(lhs), rhs(rhs)
	{
	}

	SynBinaryOpType op;

	ExprBase* lhs;
	ExprBase* rhs;

	static const unsigned myTypeID = ExprNode::ExprBinaryOp;
};

struct ExprGetAddress: ExprBase
{
	ExprGetAddress(SynBase *source, TypeBase *type, VariableHandle *variable): ExprBase(myTypeID, source, type), variable(variable)
	{
		assert(!variable->variable->lookupOnly);
	}

	VariableHandle *variable;

	static const unsigned myTypeID = ExprNode::ExprGetAddress;
};

struct ExprDereference: ExprBase
{
	ExprDereference(SynBase *source, TypeBase *type, ExprBase* value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = ExprNode::ExprDereference;
};

struct ExprUnboxing: ExprBase
{
	ExprUnboxing(SynBase *source, TypeBase *type, ExprBase* value): ExprBase(myTypeID, source, type), value(value)
	{
	}

	ExprBase* value;

	static const unsigned myTypeID = ExprNode::ExprUnboxing;
};

struct ExprConditional: ExprBase
{
	ExprConditional(SynBase *source, TypeBase *type, ExprBase *condition, ExprBase *trueBlock, ExprBase *falseBlock): ExprBase(myTypeID, source, type), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	ExprBase *condition;
	ExprBase *trueBlock;
	ExprBase *falseBlock;

	static const unsigned myTypeID = ExprNode::ExprConditional;
};

struct ExprAssignment: ExprBase
{
	ExprAssignment(SynBase *source, TypeBase *type, ExprBase *lhs, ExprBase *rhs): ExprBase(myTypeID, source, type), lhs(lhs), rhs(rhs)
	{
		if(!isType<TypeError>(type))
		{
			TypeRef *refType = getType<TypeRef>(lhs->type);

			(void)refType;
			assert(refType);
			assert(refType->subType == rhs->type);
		}
	}

	ExprBase *lhs;
	ExprBase *rhs;

	static const unsigned myTypeID = ExprNode::ExprAssignment;
};

struct ExprMemberAccess: ExprBase
{
	ExprMemberAccess(SynBase *source, TypeBase *type, ExprBase *value, VariableHandle *member): ExprBase(myTypeID, source, type), value(value), member(member)
	{
	}

	ExprBase *value;
	VariableHandle *member;

	static const unsigned myTypeID = ExprNode::ExprMemberAccess;
};

struct ExprArrayIndex: ExprBase
{
	ExprArrayIndex(SynBase *source, TypeBase *type, ExprBase *value, ExprBase *index): ExprBase(myTypeID, source, type), value(value), index(index)
	{
	}

	ExprBase *value;
	ExprBase *index;

	static const unsigned myTypeID = ExprNode::ExprArrayIndex;
};

struct ExprReturn: ExprBase
{
	ExprReturn(SynBase *source, TypeBase *type, ExprBase* value, ExprBase *coroutineStateUpdate, ExprBase *closures): ExprBase(myTypeID, source, type), value(value), coroutineStateUpdate(coroutineStateUpdate), closures(closures)
	{
	}

	ExprBase* value;

	ExprBase *coroutineStateUpdate;

	ExprBase *closures;

	static const unsigned myTypeID = ExprNode::ExprReturn;
};

struct ExprYield: ExprBase
{
	ExprYield(SynBase *source, TypeBase *type, ExprBase* value, ExprBase *coroutineStateUpdate, ExprBase *closures, unsigned order): ExprBase(myTypeID, source, type), value(value), coroutineStateUpdate(coroutineStateUpdate), closures(closures), order(order)
	{
	}

	ExprBase* value;

	ExprBase *coroutineStateUpdate;

	ExprBase *closures;

	// 1-based index of the yield in the current function
	unsigned order;

	static const unsigned myTypeID = ExprNode::ExprYield;
};

struct ExprVariableDefinition: ExprBase
{
	ExprVariableDefinition(SynBase *source, TypeBase *type, VariableHandle* variable, ExprBase* initializer): ExprBase(myTypeID, source, type), variable(variable), initializer(initializer)
	{
	}

	VariableHandle* variable;

	ExprBase* initializer;

	static const unsigned myTypeID = ExprNode::ExprVariableDefinition;
};

struct ExprZeroInitialize : ExprBase
{
	ExprZeroInitialize(SynBase *source, TypeBase *type, ExprBase *address) : ExprBase(myTypeID, source, type), address(address)
	{
		assert(isType<TypeRef>(address->type));
	}

	ExprBase *address;

	static const unsigned myTypeID = ExprNode::ExprZeroInitialize;
};

struct ExprArraySetup: ExprBase
{
	ExprArraySetup(SynBase *source, TypeBase *type, ExprBase *lhs, ExprBase* initializer): ExprBase(myTypeID, source, type), lhs(lhs), initializer(initializer)
	{
	}

	ExprBase *lhs;

	ExprBase* initializer;

	static const unsigned myTypeID = ExprNode::ExprArraySetup;
};

struct ExprVariableDefinitions: ExprBase
{
	ExprVariableDefinitions(SynBase *source, TypeBase *type, TypeBase *definitionType, IntrusiveList<ExprBase> definitions): ExprBase(myTypeID, source, type), definitionType(definitionType), definitions(definitions)
	{
	}

	TypeBase *definitionType;

	IntrusiveList<ExprBase> definitions;

	static const unsigned myTypeID = ExprNode::ExprVariableDefinitions;
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

	static const unsigned myTypeID = ExprNode::ExprVariableAccess;
};

struct ExprFunctionContextAccess: ExprBase
{
	ExprFunctionContextAccess(SynBase *source, TypeBase *type, FunctionData *function, VariableData *contextVariable): ExprBase(myTypeID, source, type), function(function), contextVariable(contextVariable)
	{
		assert(function);
		assert(type == contextVariable->type);
		assert(!contextVariable->lookupOnly);
	}

	FunctionData *function;

	VariableData *contextVariable;

	static const unsigned myTypeID = ExprNode::ExprFunctionContextAccess;
};

struct ExprFunctionDefinition: ExprBase
{
	ExprFunctionDefinition(SynBase *source, TypeBase *type, FunctionData *function, ExprVariableDefinition *contextArgument, IntrusiveList<ExprVariableDefinition> arguments, ExprBase *coroutineStateRead, IntrusiveList<ExprBase> expressions, ExprVariableDefinition *contextVariableDefinition, VariableData *contextVariable): ExprBase(myTypeID, source, type), function(function), contextArgument(contextArgument), arguments(arguments), coroutineStateRead(coroutineStateRead), expressions(expressions), contextVariableDefinition(contextVariableDefinition), contextVariable(contextVariable)
	{
		if(contextVariableDefinition)
			contextVariable = contextVariableDefinition->variable->variable;
	}

	FunctionData* function;

	ExprVariableDefinition *contextArgument;

	IntrusiveList<ExprVariableDefinition> arguments;

	ExprBase *coroutineStateRead;

	IntrusiveList<ExprBase> expressions;

	ExprVariableDefinition *contextVariableDefinition;
	VariableData *contextVariable;

	static const unsigned myTypeID = ExprNode::ExprFunctionDefinition;
};

struct ExprGenericFunctionPrototype: ExprBase
{
	ExprGenericFunctionPrototype(SynBase *source, TypeBase *type, FunctionData* function): ExprBase(myTypeID, source, type), function(function)
	{
	}

	FunctionData* function;

	IntrusiveList<ExprVariableDefinition> contextVariables;

	static const unsigned myTypeID = ExprNode::ExprGenericFunctionPrototype;
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

	static const unsigned myTypeID = ExprNode::ExprFunctionAccess;
};

struct ExprFunctionOverloadSet: ExprBase
{
	ExprFunctionOverloadSet(SynBase *source, TypeBase *type, IntrusiveList<FunctionHandle> functions, ExprBase *context): ExprBase(myTypeID, source, type), functions(functions), context(context)
	{
	}

	IntrusiveList<FunctionHandle> functions;

	ExprBase *context;

	static const unsigned myTypeID = ExprNode::ExprFunctionOverloadSet;
};

struct ExprShortFunctionOverloadSet : ExprBase
{
	ExprShortFunctionOverloadSet(SynBase *source, TypeBase *type, IntrusiveList<ShortFunctionHandle> functions) : ExprBase(myTypeID, source, type), functions(functions)
	{
	}

	IntrusiveList<ShortFunctionHandle> functions;

	static const unsigned myTypeID = ExprNode::ExprShortFunctionOverloadSet;
};

struct ExprFunctionCall: ExprBase
{
	ExprFunctionCall(SynBase *source, TypeBase *type, ExprBase *function, IntrusiveList<ExprBase> arguments): ExprBase(myTypeID, source, type), function(function), arguments(arguments)
	{
	}

	ExprBase *function;
	IntrusiveList<ExprBase> arguments;

	static const unsigned myTypeID = ExprNode::ExprFunctionCall;
};

struct ExprAliasDefinition: ExprBase
{
	ExprAliasDefinition(SynBase *source, TypeBase *type, AliasData *alias): ExprBase(myTypeID, source, type), alias(alias)
	{
	}

	AliasData *alias;

	static const unsigned myTypeID = ExprNode::ExprAliasDefinition;
};

struct ExprClassPrototype: ExprBase
{
	ExprClassPrototype(SynBase *source, TypeBase *type, TypeClass *classType): ExprBase(myTypeID, source, type), classType(classType)
	{
	}

	TypeClass *classType;

	static const unsigned myTypeID = ExprNode::ExprClassPrototype;
};

struct ExprGenericClassPrototype: ExprBase
{
	ExprGenericClassPrototype(SynBase *source, TypeBase *type, TypeGenericClassProto *genericProtoType): ExprBase(myTypeID, source, type), genericProtoType(genericProtoType)
	{
	}

	TypeGenericClassProto *genericProtoType;

	static const unsigned myTypeID = ExprNode::ExprGenericClassPrototype;
};

struct ExprClassDefinition: ExprBase
{
	ExprClassDefinition(SynBase *source, TypeBase *type, TypeClass *classType): ExprBase(myTypeID, source, type), classType(classType)
	{
	}

	TypeClass *classType;

	IntrusiveList<ExprAliasDefinition> aliases;
	IntrusiveList<ExprBase> functions;

	static const unsigned myTypeID = ExprNode::ExprClassDefinition;
};

struct ExprEnumDefinition: ExprBase
{
	ExprEnumDefinition(SynBase *source, TypeBase *type, TypeEnum *enumType, ExprBase *toInt, ExprBase *toEnum): ExprBase(myTypeID, source, type), enumType(enumType), toInt(toInt), toEnum(toEnum)
	{
	}

	TypeEnum *enumType;

	ExprBase *toInt;
	ExprBase *toEnum;

	static const unsigned myTypeID = ExprNode::ExprEnumDefinition;
};

struct ExprIfElse: ExprBase
{
	ExprIfElse(SynBase *source, TypeBase *type, ExprBase *condition, ExprBase *trueBlock, ExprBase *falseBlock): ExprBase(myTypeID, source, type), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	ExprBase *condition;
	ExprBase *trueBlock;
	ExprBase *falseBlock;

	static const unsigned myTypeID = ExprNode::ExprIfElse;
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

	static const unsigned myTypeID = ExprNode::ExprFor;
};

struct ExprWhile: ExprBase
{
	ExprWhile(SynBase *source, TypeBase *type, ExprBase *condition, ExprBase *body): ExprBase(myTypeID, source, type), condition(condition), body(body)
	{
	}

	ExprBase *condition;
	ExprBase *body;

	static const unsigned myTypeID = ExprNode::ExprWhile;
};

struct ExprDoWhile: ExprBase
{
	ExprDoWhile(SynBase *source, TypeBase *type, ExprBase *body, ExprBase *condition): ExprBase(myTypeID, source, type), body(body), condition(condition)
	{
	}

	ExprBase *body;
	ExprBase *condition;

	static const unsigned myTypeID = ExprNode::ExprDoWhile;
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

	static const unsigned myTypeID = ExprNode::ExprSwitch;
};

struct ExprBreak: ExprBase
{
	ExprBreak(SynBase *source, TypeBase *type, unsigned depth, ExprBase *closures): ExprBase(myTypeID, source, type), depth(depth), closures(closures)
	{
	}

	unsigned depth;

	ExprBase *closures;

	static const unsigned myTypeID = ExprNode::ExprBreak;
};

struct ExprContinue: ExprBase
{
	ExprContinue(SynBase *source, TypeBase *type, unsigned depth, ExprBase *closures): ExprBase(myTypeID, source, type), depth(depth), closures(closures)
	{
	}

	unsigned depth;

	ExprBase *closures;

	static const unsigned myTypeID = ExprNode::ExprContinue;
};

struct ExprBlock: ExprBase
{
	ExprBlock(SynBase *source, TypeBase *type, IntrusiveList<ExprBase> expressions, ExprBase *closures): ExprBase(myTypeID, source, type), expressions(expressions), closures(closures)
	{
	}

	IntrusiveList<ExprBase> expressions;

	ExprBase *closures;

	static const unsigned myTypeID = ExprNode::ExprBlock;
};

struct ExprSequence: ExprBase
{
	ExprSequence(Allocator *allocator, SynBase *source, TypeBase *type, ArrayView<ExprBase*> arr): ExprBase(myTypeID, source, type), expressions(allocator)
	{
		for(unsigned i = 0; i < arr.size(); i++)
			expressions.push_back(arr[i]);
	}

	SmallArray<ExprBase*, 4> expressions;

	static const unsigned myTypeID = ExprNode::ExprSequence;
};

struct ExprModule: ExprBase
{
	ExprModule(Allocator *allocator, SynBase *source, TypeBase *type, ScopeData *moduleScope, IntrusiveList<ExprBase> expressions): ExprBase(myTypeID, source, type), moduleScope(moduleScope), definitions(allocator), expressions(expressions)
	{
	}

	ScopeData *moduleScope;

	SmallArray<ExprBase*, 32> definitions;

	IntrusiveList<ExprBase> setup;

	IntrusiveList<ExprBase> expressions;

	static const unsigned myTypeID = ExprNode::ExprModule;
};

template<typename T>
bool isType(ExprBase *node)
{
	return node && node->typeID == T::myTypeID;
}

template<typename T>
T* getType(ExprBase *node)
{
	if(node && node->typeID == T::myTypeID)
		return static_cast<T*>(node);

	return 0;
}

ExprModule* Analyze(ExpressionContext &context, SynModule *syntax, const char *code, const char *moduleRoot);
void VisitExpressionTreeNodes(ExprBase *expression, void *context, void(*accept)(void *context, ExprBase *child));
const char* GetExpressionTreeNodeName(ExprBase *expression);
