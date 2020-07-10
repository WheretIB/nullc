import std.vector;
import reflist;
import typetree;
import arrayview;

class ExprBase;

class GenericFunctionInstanceTypeRequest
{
	void GenericFunctionInstanceTypeRequest()
	{
	}

	void GenericFunctionInstanceTypeRequest(TypeBase ref parentType, FunctionData ref function, RefList<TypeHandle> arguments, RefList<MatchData> aliases)
	{
		this.parentType = parentType;
		this.function = function;
		this.arguments = arguments;
		this.aliases = aliases;

		hash = parentType ? parentType.nameHash : 0;

		hash = hash + (hash << 5) + function.nameHash;

		for(TypeHandle ref curr = arguments.head; curr; curr = curr.next)
			hash = hash + (hash << 5) + curr.type.nameHash;

		for(MatchData ref curr = aliases.head; curr; curr = curr.next)
			hash = hash + (hash << 5) + curr.type.nameHash;
	}

	int hash = 0;

	TypeBase ref parentType = nullptr;

	FunctionData ref function = nullptr;

	RefList<TypeHandle> arguments;
	RefList<MatchData> aliases;
}

bool operator==(GenericFunctionInstanceTypeRequest lhs, GenericFunctionInstanceTypeRequest rhs)
{
	if(lhs.parentType != rhs.parentType)
		return false;

	if(lhs.function != rhs.function)
		return false;

	for(TypeHandle ref leftArg = lhs.arguments.head, rightArg = rhs.arguments.head; leftArg || rightArg; { leftArg = leftArg.next; rightArg = rightArg.next; })
	{
		if(!leftArg || !rightArg)
			return false;

		if(leftArg.type != rightArg.type)
			return false;
	}

	for(MatchData ref leftAlias = lhs.aliases.head, rightAlias = rhs.aliases.head; leftAlias || rightAlias; { leftAlias = leftAlias.next; rightAlias = rightAlias.next; })
	{
		if(!leftAlias || !rightAlias)
			return false;

		if(leftAlias.name != rightAlias.name || leftAlias.type != rightAlias.type)
			return false;
	}

	return true;
}

bool operator!=(GenericFunctionInstanceTypeRequest lhs, GenericFunctionInstanceTypeRequest rhs)
{
	return !(lhs == rhs);
}

int hash_value(GenericFunctionInstanceTypeRequest key)
{
	return key.hash;
}

class GenericFunctionInstanceTypeResponse
{
	void GenericFunctionInstanceTypeResponse()
	{
	}

	void GenericFunctionInstanceTypeResponse(TypeFunction ref functionType, RefList<MatchData> aliases)
	{
		this.functionType = functionType;
		this.aliases = aliases;
	}

	TypeFunction ref functionType;

	RefList<MatchData> aliases;
}

class TypedFunctionInstanceRequest
{
	void TypedFunctionInstanceRequest()
	{
	}

	void TypedFunctionInstanceRequest(TypeBase ref instanceType, SynBase ref syntax)
	{
		this.instanceType = instanceType;
		this.syntax = syntax;

		hash = (instanceType ? instanceType.nameHash : 0);

		hash = hash + (hash << 5) + hash_value((auto ref)(syntax));
	}

	int hash;

	TypeBase ref instanceType;
	SynBase ref syntax;
}

bool operator==(TypedFunctionInstanceRequest lhs, rhs)
{
	if(lhs.instanceType != rhs.instanceType)
		return false;

	if(lhs.syntax != rhs.syntax)
		return false;

	return true;
}

bool operator!=(TypedFunctionInstanceRequest lhs, rhs)
{
	return !(lhs == rhs);
}

int hash_value(TypedFunctionInstanceRequest key)
{
	return key.hash;
}

class TypePair
{
	void TypePair()
	{
	}

	void TypePair(TypeBase ref a, TypeBase ref b)
	{
		this.a = a;
		this.b = b;
	}

	TypeBase ref a;
	TypeBase ref b;
}

bool operator==(TypePair lhs, rhs)
{
	return lhs.a == rhs.a && lhs.b == rhs.b;
}

bool operator!=(TypePair lhs, rhs)
{
	return lhs.a != rhs.a || lhs.b != rhs.b;
}

int hash_value(TypePair key)
{
	return key.a.nameHash + key.b.nameHash;
}
/*
int hash_value(TypeClass ref x)
{
	return hash_value((auto ref)(x));
}

int hash_value(FunctionData ref x)
{
	return hash_value((auto ref)(x));
}*/

class ExprBase extendable
{
	void ExprBase(SynBase ref source, TypeBase ref type)
	{
		this.source = source;
		this.type = type;
	}

	SynBase ref source;
	TypeBase ref type;
	ExprBase ref next;
	bool listed;
}
/*
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
		ExprModule
	}
}
*/
class ExprError: ExprBase
{
	void ExprError(SynBase ref source, TypeBase ref type)
	{
		this.source = source;
		this.type = type;
	}

	void ExprError(SynBase ref source, TypeBase ref type, ExprBase ref value)
	{
		this.source = source;
		this.type = type;

		values.push_back(value);
	}

	void ExprError(SynBase ref source, TypeBase ref type, ExprBase ref value1, ExprBase ref value2)
	{
		this.source = source;
		this.type = type;

		values.push_back(value1);
		values.push_back(value2);
	}

	void ExprError(SynBase ref source, TypeBase ref type, ArrayView<ExprBase ref> arr)
	{
		this.source = source;
		this.type = type;

		for(int i = 0; i < arr.size(); i++)
			values.push_back(arr[i]);
	}

	void ExprError(SynBase ref source, TypeBase ref type, vector<ExprBase ref> arr)
	{
		this.source = source;
		this.type = type;

		for(int i = 0; i < arr.size(); i++)
			values.push_back(arr[i]);
	}

	vector<ExprBase ref> values;
}

class ExprErrorTypeMemberAccess: ExprBase
{
	void ExprErrorTypeMemberAccess(SynBase ref source, TypeBase ref type, TypeBase ref value)
	{
		this.source = source;
		this.type = type;

		this.value = value;
	}

	TypeBase ref value;
}

class ExprVoid: ExprBase
{
	void ExprVoid(SynBase ref source, TypeBase ref type)
	{
		this.source = source;
		this.type = type;

	}
}

class ExprBoolLiteral: ExprBase
{
	void ExprBoolLiteral(SynBase ref source, TypeBase ref type, bool value)
	{
		this.source = source;
		this.type = type;

		this.value = value;
	}

	bool value;
}

class ExprCharacterLiteral: ExprBase
{
	void ExprCharacterLiteral(SynBase ref source, TypeBase ref type, char value)
	{
		this.source = source;
		this.type = type;

		this.value = value;
	}

	char value;
}

class ExprStringLiteral: ExprBase
{
	void ExprStringLiteral(SynBase ref source, TypeBase ref type, char[] value, int length)
	{
		this.source = source;
		this.type = type;

		this.value = value;
		this.length = length;
	}

	char[] value;
	int length;
}

class ExprIntegerLiteral: ExprBase
{
	void ExprIntegerLiteral(SynBase ref source, TypeBase ref type, long value)
	{
		this.source = source;
		this.type = type;

		this.value = value;
	}

	long value;
}

class ExprRationalLiteral: ExprBase
{
	void ExprRationalLiteral(SynBase ref source, TypeBase ref type, double value)
	{
		this.source = source;
		this.type = type;

		this.value = value;
	}

	double value;
}

class ExprTypeLiteral: ExprBase
{
	void ExprTypeLiteral(SynBase ref source, TypeBase ref type, TypeBase ref value)
	{
		this.source = source;
		this.type = type;

		this.value = value;
	}

	TypeBase ref value;
}

class ExprNullptrLiteral: ExprBase
{
	void ExprNullptrLiteral(SynBase ref source, TypeBase ref type)
	{
		this.source = source;
		this.type = type;

	}
}

class ExprFunctionIndexLiteral: ExprBase
{
	void ExprFunctionIndexLiteral(SynBase ref source, TypeBase ref type, FunctionData ref function)
	{
		this.source = source;
		this.type = type;

		this.function = function;
	}

	FunctionData ref function;
}

class ExprFunctionLiteral: ExprBase
{
	void ExprFunctionLiteral(SynBase ref source, TypeBase ref type, FunctionData ref data, ExprBase ref context)
	{
		this.source = source;
		this.type = type;

		this.data = data;
		this.context = context;
	}

	FunctionData ref data;

	ExprBase ref context;
}

class EvalMemoryBlock
{
	void EvalMemoryBlock(int address, char[] buffer)
	{
		this.address = address;
		this.buffer = buffer;
	}

	int address;
	char[] buffer;
}

class ExprPointerLiteral: ExprBase
{
	void ExprPointerLiteral(SynBase ref source, TypeBase ref type, EvalMemoryBlock ptr, int start, int end)
	{
		this.source = source;
		this.type = type;

		this.ptr = ptr;

		this.start = start;
		this.end = end;

		assert(start <= end);
	}

	EvalMemoryBlock ptr;
	int start;
	int end;
}

class ExprMemoryLiteral: ExprBase
{
	void ExprMemoryLiteral(SynBase ref source, TypeBase ref type, ExprPointerLiteral ref ptr)
	{
		this.source = source;
		this.type = type;

		this.ptr = ptr;
	}

	ExprPointerLiteral ref ptr;
}

class ExprPassthrough: ExprBase
{
	void ExprPassthrough(SynBase ref source, TypeBase ref type, ExprBase ref value)
	{
		this.source = source;
		this.type = type;

		this.value = value;

		assert(type == value.type);
	}

	ExprBase ref value;
}

class ExprArray: ExprBase
{
	void ExprArray(SynBase ref source, TypeBase ref type, RefList<ExprBase> values)
	{
		this.source = source;
		this.type = type;

		this.values = values;
	}

	RefList<ExprBase> values;
}

class ExprPreModify: ExprBase
{
	void ExprPreModify(SynBase ref source, TypeBase ref type, ExprBase ref value, bool isIncrement)
	{
		this.source = source;
		this.type = type;

		this.value = value;
		this.isIncrement = isIncrement;
	}

	ExprBase ref value;
	bool isIncrement;
}

class ExprPostModify: ExprBase
{
	void ExprPostModify(SynBase ref source, TypeBase ref type, ExprBase ref value, bool isIncrement)
	{
		this.source = source;
		this.type = type;

		this.value = value;
		this.isIncrement = isIncrement;
	}

	ExprBase ref value;
	bool isIncrement;
}

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
}

class ExprTypeCast: ExprBase
{
	void ExprTypeCast(SynBase ref source, TypeBase ref type, ExprBase ref value, ExprTypeCastCategory category)
	{
		this.source = source;
		this.type = type;

		this.value = value;
		this.category = category;
	}

	ExprBase ref value;

	ExprTypeCastCategory category;
}

class ExprUnaryOp: ExprBase
{
	void ExprUnaryOp(SynBase ref source, TypeBase ref type, SynUnaryOpType op, ExprBase ref value)
	{
		this.source = source;
		this.type = type;

		this.op = op;
		this.value = value;
	}

	SynUnaryOpType op;

	ExprBase ref value;
}

class ExprBinaryOp: ExprBase
{
	void ExprBinaryOp(SynBase ref source, TypeBase ref type, SynBinaryOpType op, ExprBase ref lhs, ExprBase ref rhs)
	{
		this.source = source;
		this.type = type;

		this.op = op;
		this.lhs = lhs;
		this.rhs = rhs;
	}

	SynBinaryOpType op;

	ExprBase ref lhs;
	ExprBase ref rhs;
}

class ExprGetAddress: ExprBase
{
	void ExprGetAddress(SynBase ref source, TypeBase ref type, VariableHandle ref variable)
	{
		this.source = source;
		this.type = type;

		this.variable = variable;

		assert(!variable.variable.lookupOnly);
	}

	VariableHandle ref variable;
}

class ExprDereference: ExprBase
{
	void ExprDereference(SynBase ref source, TypeBase ref type, ExprBase ref value)
	{
		this.source = source;
		this.type = type;

		this.value = value;
	}

	ExprBase ref value;
}

class ExprUnboxing: ExprBase
{
	void ExprUnboxing(SynBase ref source, TypeBase ref type, ExprBase ref value)
	{
		this.source = source;
		this.type = type;

		this.value = value;
	}

	ExprBase ref value;
}

class ExprConditional: ExprBase
{
	void ExprConditional(SynBase ref source, TypeBase ref type, ExprBase ref condition, ExprBase ref trueBlock, ExprBase ref falseBlock)
	{
		this.source = source;
		this.type = type;

		this.condition = condition;
		this.trueBlock = trueBlock;
		this.falseBlock = falseBlock;
	}

	ExprBase ref condition;
	ExprBase ref trueBlock;
	ExprBase ref falseBlock;
}

class ExprAssignment: ExprBase
{
	void ExprAssignment(SynBase ref source, TypeBase ref type, ExprBase ref lhs, ExprBase ref rhs)
	{
		this.source = source;
		this.type = type;

		this.lhs = lhs;
		this.rhs = rhs;

		if(!isType with<TypeError>(type))
		{
			TypeRef ref refType = getType with<TypeRef>(lhs.type);

			assert(refType != nullptr);
			assert(refType.subType == rhs.type);
		}
	}

	ExprBase ref lhs;
	ExprBase ref rhs;
}

class ExprMemberAccess: ExprBase
{
	void ExprMemberAccess(SynBase ref source, TypeBase ref type, ExprBase ref value, VariableHandle ref member)
	{
		this.source = source;
		this.type = type;

		this.value = value;
		this.member = member;
	}

	ExprBase ref value;
	VariableHandle ref member;
}

class ExprArrayIndex: ExprBase
{
	void ExprArrayIndex(SynBase ref source, TypeBase ref type, ExprBase ref value, ExprBase ref index)
	{
		this.source = source;
		this.type = type;

		this.value = value;
		this.index = index;
	}

	ExprBase ref value;
	ExprBase ref index;
}

class ExprReturn: ExprBase
{
	void ExprReturn(SynBase ref source, TypeBase ref type, ExprBase ref value, ExprBase ref coroutineStateUpdate, ExprBase ref closures)
	{
		this.source = source;
		this.type = type;

		this.value = value;
		this.coroutineStateUpdate = coroutineStateUpdate;
		this.closures = closures;
	}

	ExprBase ref value;

	ExprBase ref coroutineStateUpdate;

	ExprBase ref closures;
}

class ExprYield: ExprBase
{
	void ExprYield(SynBase ref source, TypeBase ref type, ExprBase ref value, ExprBase ref coroutineStateUpdate, ExprBase ref closures, int order)
	{
		this.source = source;
		this.type = type;

		this.value = value;
		this.coroutineStateUpdate = coroutineStateUpdate;
		this.closures = closures;
		this.order = order;
	}

	ExprBase ref value;

	ExprBase ref coroutineStateUpdate;

	ExprBase ref closures;

	// 1-based index of the yield in the current function
	int order;
}

class ExprVariableDefinition: ExprBase
{
	void ExprVariableDefinition(SynBase ref source, TypeBase ref type, VariableHandle ref variable, ExprBase ref initializer)
	{
		this.source = source;
		this.type = type;

		this.variable = variable;
		this.initializer = initializer;
	}

	VariableHandle ref variable;

	ExprBase ref initializer;
}

class ExprArraySetup: ExprBase
{
	void ExprArraySetup(SynBase ref source, TypeBase ref type, ExprBase ref lhs, ExprBase ref initializer)
	{
		this.source = source;
		this.type = type;

		this.lhs = lhs;
		this.initializer = initializer;
	}

	ExprBase ref lhs;

	ExprBase ref initializer;
}

class ExprVariableDefinitions: ExprBase
{
	void ExprVariableDefinitions(SynBase ref source, TypeBase ref type, TypeBase ref definitionType, RefList<ExprBase> definitions)
	{
		this.source = source;
		this.type = type;

		this.definitionType = definitionType;
		this.definitions = definitions;
	}

	TypeBase ref definitionType;

	RefList<ExprBase> definitions;
}

class ExprVariableAccess: ExprBase
{
	void ExprVariableAccess(SynBase ref source, TypeBase ref type, VariableData ref variable)
	{
		this.source = source;
		this.type = type;

		this.variable = variable;

		assert(variable != nullptr);
		assert(type == variable.type);
		assert(!variable.lookupOnly);
	}

	VariableData ref variable;
}

class ExprFunctionContextAccess: ExprBase
{
	void ExprFunctionContextAccess(SynBase ref source, TypeBase ref type, FunctionData ref function, VariableData ref contextVariable)
	{
		this.source = source;
		this.type = type;

		this.function = function;
		this.contextVariable = contextVariable;

		assert(function != nullptr);
		assert(type == contextVariable.type);
		assert(!contextVariable.lookupOnly);
	}

	FunctionData ref function;

	VariableData ref contextVariable;
}

class ExprFunctionDefinition: ExprBase
{
	void ExprFunctionDefinition(SynBase ref source, TypeBase ref type, FunctionData ref function, ExprVariableDefinition ref contextArgument, RefList<ExprVariableDefinition> arguments, ExprBase ref coroutineStateRead, RefList<ExprBase> expressions, ExprVariableDefinition ref contextVariableDefinition, VariableData ref contextVariable)
	{
		this.source = source;
		this.type = type;

		this.function = function;
		this.contextArgument = contextArgument;
		this.arguments = arguments;
		this.coroutineStateRead = coroutineStateRead;
		this.expressions = expressions;
		this.contextVariableDefinition = contextVariableDefinition;
		this.contextVariable = contextVariable;

		if(contextVariableDefinition)
			contextVariable = contextVariableDefinition.variable.variable;
	}

	FunctionData ref function;

	ExprVariableDefinition ref contextArgument;

	RefList<ExprVariableDefinition> arguments;

	ExprBase ref coroutineStateRead;

	RefList<ExprBase> expressions;

	ExprVariableDefinition ref contextVariableDefinition;
	VariableData ref contextVariable;
}

class ExprGenericFunctionPrototype: ExprBase
{
	void ExprGenericFunctionPrototype(SynBase ref source, TypeBase ref type, FunctionData ref function)
	{
		this.source = source;
		this.type = type;

		this.function = function;
	}

	FunctionData ref function;

	RefList<ExprVariableDefinition> contextVariables;
}

class ExprFunctionAccess: ExprBase
{
	void ExprFunctionAccess(SynBase ref source, TypeBase ref type, FunctionData ref function, ExprBase ref context)
	{
		this.source = source;
		this.type = type;

		this.function = function;
		this.context = context;

		assert(context != nullptr);

		assert(context.type.name != InplaceStr("auto ref"));

		if(!function.type.isGeneric && !(function.scope.ownerType && function.scope.ownerType.isGeneric))
			assert(function.contextType == context.type);
	}

	FunctionData ref function;

	ExprBase ref context;
}

class ExprFunctionOverloadSet: ExprBase
{
	void ExprFunctionOverloadSet(SynBase ref source, TypeBase ref type, RefList<FunctionHandle> functions, ExprBase ref context)
	{
		this.source = source;
		this.type = type;

		this.functions = functions;
		this.context = context;
	}

	RefList<FunctionHandle> functions;

	ExprBase ref context;
}

class ExprShortFunctionOverloadSet : ExprBase
{
	void ExprShortFunctionOverloadSet(SynBase ref source, TypeBase ref type, RefList<ShortFunctionHandle> functions)
	{
		this.source = source;
		this.type = type;

		this.functions = functions;
	}

	RefList<ShortFunctionHandle> functions;
}

class ExprFunctionCall: ExprBase
{
	void ExprFunctionCall(SynBase ref source, TypeBase ref type, ExprBase ref function, RefList<ExprBase> arguments)
	{
		this.source = source;
		this.type = type;

		this.function = function;
		this.arguments = arguments;
	}

	ExprBase ref function;
	RefList<ExprBase> arguments;
}

class ExprAliasDefinition: ExprBase
{
	void ExprAliasDefinition(SynBase ref source, TypeBase ref type, AliasData ref alias)
	{
		this.source = source;
		this.type = type;

		this.alias = alias;
	}

	AliasData ref alias;
}

class ExprClassPrototype: ExprBase
{
	void ExprClassPrototype(SynBase ref source, TypeBase ref type, TypeClass ref classType)
	{
		this.source = source;
		this.type = type;

		this.classType = classType;
	}

	TypeClass ref classType;
}

class ExprGenericClassPrototype: ExprBase
{
	void ExprGenericClassPrototype(SynBase ref source, TypeBase ref type, TypeGenericClassProto ref genericProtoType)
	{
		this.source = source;
		this.type = type;

		this.genericProtoType = genericProtoType;
	}

	TypeGenericClassProto ref genericProtoType;
}

class ExprClassDefinition: ExprBase
{
	void ExprClassDefinition(SynBase ref source, TypeBase ref type, TypeClass ref classType)
	{
		this.source = source;
		this.type = type;

		this.classType = classType;
	}

	TypeClass ref classType;

	RefList<ExprAliasDefinition> aliases;
	RefList<ExprBase> functions;
}

class ExprEnumDefinition: ExprBase
{
	void ExprEnumDefinition(SynBase ref source, TypeBase ref type, TypeEnum ref enumType, ExprBase ref toInt, ExprBase ref toEnum)
	{
		this.source = source;
		this.type = type;

		this.enumType = enumType;
		this.toInt = toInt;
		this.toEnum = toEnum;
	}

	TypeEnum ref enumType;

	ExprBase ref toInt;
	ExprBase ref toEnum;
}

class ExprIfElse: ExprBase
{
	void ExprIfElse(SynBase ref source, TypeBase ref type, ExprBase ref condition, ExprBase ref trueBlock, ExprBase ref falseBlock)
	{
		this.source = source;
		this.type = type;

		this.condition = condition;
		this.trueBlock = trueBlock;
		this.falseBlock = falseBlock;
	}

	ExprBase ref condition;
	ExprBase ref trueBlock;
	ExprBase ref falseBlock;
}

class ExprFor: ExprBase
{
	void ExprFor(SynBase ref source, TypeBase ref type, ExprBase ref initializer, ExprBase ref condition, ExprBase ref increment, ExprBase ref body)
	{
		this.source = source;
		this.type = type;

		this.initializer = initializer;
		this.condition = condition;
		this.increment = increment;
		this.body = body;
	}

	ExprBase ref initializer;
	ExprBase ref condition;
	ExprBase ref increment;
	ExprBase ref body;
}

class ExprWhile: ExprBase
{
	void ExprWhile(SynBase ref source, TypeBase ref type, ExprBase ref condition, ExprBase ref body)
	{
		this.source = source;
		this.type = type;

		this.condition = condition;
		this.body = body;
	}

	ExprBase ref condition;
	ExprBase ref body;
}

class ExprDoWhile: ExprBase
{
	void ExprDoWhile(SynBase ref source, TypeBase ref type, ExprBase ref body, ExprBase ref condition)
	{
		this.source = source;
		this.type = type;

		this.body = body;
		this.condition = condition;
	}

	ExprBase ref body;
	ExprBase ref condition;
}

class ExprSwitch: ExprBase
{
	void ExprSwitch(SynBase ref source, TypeBase ref type, ExprBase ref condition, RefList<ExprBase> cases, RefList<ExprBase> blocks, ExprBase ref defaultBlock)
	{
		this.source = source;
		this.type = type;

		this.condition = condition;
		this.cases = cases;
		this.blocks = blocks;
		this.defaultBlock = defaultBlock;
	}

	ExprBase ref condition;
	RefList<ExprBase> cases;
	RefList<ExprBase> blocks;
	ExprBase ref defaultBlock;
}

class ExprBreak: ExprBase
{
	void ExprBreak(SynBase ref source, TypeBase ref type, int depth, ExprBase ref closures)
	{
		this.source = source;
		this.type = type;

		this.depth = depth;
		this.closures = closures;
	}

	int depth;

	ExprBase ref closures;
}

class ExprContinue: ExprBase
{
	void ExprContinue(SynBase ref source, TypeBase ref type, int depth, ExprBase ref closures)
	{
		this.source = source;
		this.type = type;

		this.depth = depth;
		this.closures = closures;
	}

	int depth;

	ExprBase ref closures;
}

class ExprBlock: ExprBase
{
	void ExprBlock(SynBase ref source, TypeBase ref type, RefList<ExprBase> expressions, ExprBase ref closures)
	{
		this.source = source;
		this.type = type;

		this.expressions = expressions;
		this.closures = closures;
	}

	RefList<ExprBase> expressions;

	ExprBase ref closures;
}

class ExprSequence: ExprBase
{
	void ExprSequence(SynBase ref source, TypeBase ref type, ArrayView<ExprBase ref> arr)
	{
		this.source = source;
		this.type = type;

		for(int i = 0; i < arr.size(); i++)
			expressions.push_back(arr[i]);
	}

	vector<ExprBase ref> expressions;
}

class ExprModule: ExprBase
{
	void ExprModule(SynBase ref source, TypeBase ref type, ScopeData ref moduleScope, RefList<ExprBase> expressions)
	{
		this.source = source;
		this.type = type;

		this.moduleScope = moduleScope;
		this.expressions = expressions;
	}

	ScopeData ref moduleScope;

	vector<ExprBase ref> definitions;

	RefList<ExprBase> setup;

	RefList<ExprBase> expressions;
}

bool isType<@T>(ExprBase ref node)
{
	return node && typeid(node) == T;
}

auto getType<@T>(ExprBase ref node)
{
	if(node && typeid(node) == T)
		return (T ref)(node);

	return nullptr;
}
/*
ExprModule ref Analyze(ExpressionContext ref context, SynModule ref syntax, char[] code);
void VisitExpressionTreeNodes(ExprBase ref expression, void ref context, void ref(void ref, ExprBase ref) visit);
char[] GetExpressionTreeNodeName(ExprBase ref expression);
*/