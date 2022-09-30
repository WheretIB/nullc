import std.vector;
import std.hashmap;
import reflist;
import stringutil;
import lexer;
import bytecode;

class CompilerContext;

class SynBase;
class SynIdentifier;

enum SynUnaryOpType
{
	SYN_UNARY_OP_UNKNOWN,

	SYN_UNARY_OP_PLUS,
	SYN_UNARY_OP_NEGATE,
	SYN_UNARY_OP_BIT_NOT,
	SYN_UNARY_OP_LOGICAL_NOT
}

bool bool(SynUnaryOpType x){ return x != SynUnaryOpType.SYN_UNARY_OP_UNKNOWN; }

char[] GetOpName(SynUnaryOpType type)
{
	switch(type)
	{
	case SynUnaryOpType.SYN_UNARY_OP_PLUS:
		return "+";
	case SynUnaryOpType.SYN_UNARY_OP_NEGATE:
		return "-";
	case SynUnaryOpType.SYN_UNARY_OP_BIT_NOT:
		return "~";
	case SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT:
		return "!";
	default:
		break;
	}

	assert(false, "unknown operation type");
	return "";
}

enum SynBinaryOpType
{
	SYN_BINARY_OP_UNKNOWN,

	SYN_BINARY_OP_ADD,
	SYN_BINARY_OP_SUB,
	SYN_BINARY_OP_MUL,
	SYN_BINARY_OP_DIV,
	SYN_BINARY_OP_MOD,
	SYN_BINARY_OP_POW,
	SYN_BINARY_OP_SHL,
	SYN_BINARY_OP_SHR,
	SYN_BINARY_OP_LESS,
	SYN_BINARY_OP_LESS_EQUAL,
	SYN_BINARY_OP_GREATER,
	SYN_BINARY_OP_GREATER_EQUAL,
	SYN_BINARY_OP_EQUAL,
	SYN_BINARY_OP_NOT_EQUAL,
	SYN_BINARY_OP_BIT_AND,
	SYN_BINARY_OP_BIT_OR,
	SYN_BINARY_OP_BIT_XOR,
	SYN_BINARY_OP_LOGICAL_AND,
	SYN_BINARY_OP_LOGICAL_OR,
	SYN_BINARY_OP_LOGICAL_XOR,
	SYN_BINARY_OP_IN
}

bool bool(SynBinaryOpType x){ return x != SynBinaryOpType.SYN_BINARY_OP_UNKNOWN; }

char[] GetOpName(SynBinaryOpType type)
{
	switch(type)
	{
	case SynBinaryOpType.SYN_BINARY_OP_ADD:
		return "+";
	case SynBinaryOpType.SYN_BINARY_OP_SUB:
		return "-";
	case SynBinaryOpType.SYN_BINARY_OP_MUL:
		return "*";
	case SynBinaryOpType.SYN_BINARY_OP_DIV:
		return "/";
	case SynBinaryOpType.SYN_BINARY_OP_MOD:
		return "%";
	case SynBinaryOpType.SYN_BINARY_OP_POW:
		return "**";
	case SynBinaryOpType.SYN_BINARY_OP_SHL:
		return "<<";
	case SynBinaryOpType.SYN_BINARY_OP_SHR:
		return ">>";
	case SynBinaryOpType.SYN_BINARY_OP_LESS:
		return "<";
	case SynBinaryOpType.SYN_BINARY_OP_LESS_EQUAL:
		return "<=";
	case SynBinaryOpType.SYN_BINARY_OP_GREATER:
		return ">";
	case SynBinaryOpType.SYN_BINARY_OP_GREATER_EQUAL:
		return ">=";
	case SynBinaryOpType.SYN_BINARY_OP_EQUAL:
		return "==";
	case SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL:
		return "!=";
	case SynBinaryOpType.SYN_BINARY_OP_BIT_AND:
		return "&";
	case SynBinaryOpType.SYN_BINARY_OP_BIT_OR:
		return "|";
	case SynBinaryOpType.SYN_BINARY_OP_BIT_XOR:
		return "^";
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND:
		return "&&";
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_OR:
		return "||";
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_XOR:
		return "^^";
	case SynBinaryOpType.SYN_BINARY_OP_IN:
		return "in";
	default:
		break;
	}

	assert(false, "unknown operation type");
	return "";
}

enum SynModifyAssignType
{
	SYN_MODIFY_ASSIGN_UNKNOWN,

	SYN_MODIFY_ASSIGN_ADD,
	SYN_MODIFY_ASSIGN_SUB,
	SYN_MODIFY_ASSIGN_MUL,
	SYN_MODIFY_ASSIGN_DIV,
	SYN_MODIFY_ASSIGN_POW,
	SYN_MODIFY_ASSIGN_MOD,
	SYN_MODIFY_ASSIGN_SHL,
	SYN_MODIFY_ASSIGN_SHR,
	SYN_MODIFY_ASSIGN_BIT_AND,
	SYN_MODIFY_ASSIGN_BIT_OR,
	SYN_MODIFY_ASSIGN_BIT_XOR
}

bool bool(SynModifyAssignType x){ return x != SynModifyAssignType.SYN_MODIFY_ASSIGN_UNKNOWN; }

char[] GetOpName(SynModifyAssignType type)
{
	switch(type)
	{
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_ADD:
		return "+=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_SUB:
		return "-=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_MUL:
		return "*=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_DIV:
		return "/=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_POW:
		return "**=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_MOD:
		return "%=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_SHL:
		return "<<=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_SHR:
		return ">>=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_AND:
		return "&=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_OR:
		return "|=";
	case SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_XOR:
		return "^=";
	default:
		break;
	}

	assert(false, "unknown operation type");
	return "";
}

class SynBinaryOpElement
{
	void SynBinaryOpElement(LexemeRef begin, LexemeRef end, SynBinaryOpType type, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;
		this.type = type;
		this.value = value;
	}

	LexemeRef begin;
	LexemeRef end;
	SynBinaryOpType type;
	SynBase ref value;
}

class SynNamespaceElement;

class ErrorInfo
{
	void ErrorInfo(InplaceStr message, LexemeRef begin, LexemeRef end, StringRef pos)
	{
		this.message = message;
		this.begin = begin;
		this.end = end;
		this.pos = pos;
	}

	InplaceStr message;

	LexemeRef begin;
	LexemeRef end;

	StringRef pos;

	vector<ErrorInfo ref> related;
}

class SynBase extendable
{
	void SynBase(LexemeRef begin, LexemeRef end)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
	}

	// Positions into the lexeme stream
	LexemeRef begin;
	LexemeRef end;

	InplaceStr pos;

	// For RefList
	SynBase ref next;
	bool listed;

	bool isInternal;
}

bool isType<@T>(SynBase ref node)
{
	return node && typeid(node) == T;
}

auto getType<@T>(SynBase ref node)
{
	if(node && typeid(node) == T)
		return (T ref)(node);

	return nullptr;
}

class SynError: SynBase
{
	void SynError(LexemeRef begin, LexemeRef end)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
	}
}

class SynImportLocation: SynBase
{
	void SynImportLocation(LexemeRef begin, LexemeRef end)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
	}
}

class SynInternal: SynBase
{
	void SynInternal(SynBase ref source)
	{
		this.begin = source.begin;
		this.end = source.end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);

		isInternal = true;
	}
}

class SynNothing: SynBase
{
	void SynNothing(LexemeRef begin, LexemeRef end)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
	}
}

class SynIdentifier: SynBase
{
	void SynIdentifier(InplaceStr name)
	{
		this.name = name;
	}

	void SynIdentifier(SynIdentifier ref source, InplaceStr name)
	{
		this.begin = source.begin;
		this.end = source.end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);

		this.name = name;
	}

	void SynIdentifier(LexemeRef begin, LexemeRef end, InplaceStr name)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);

		this.name = name;
	}

	InplaceStr name;
}

class SynTypeAuto: SynBase
{
	void SynTypeAuto(LexemeRef begin, LexemeRef end)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
	}
}

class SynTypeGeneric: SynBase
{
	void SynTypeGeneric(LexemeRef begin, LexemeRef end)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
	}
}

class SynTypeSimple: SynBase
{
	void SynTypeSimple(LexemeRef begin, LexemeRef end, RefList<SynIdentifier> path, InplaceStr name)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.path = path;
		this.name = name;
	}

	RefList<SynIdentifier> path;
	InplaceStr name;
}

class SynTypeAlias: SynBase
{
	void SynTypeAlias(LexemeRef begin, LexemeRef end, SynIdentifier ref name)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.name = name;
	}

	SynIdentifier ref name;
}

class SynTypeArray: SynBase
{
	void SynTypeArray(LexemeRef begin, LexemeRef end, SynBase ref type, RefList<SynBase> sizes)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.sizes = sizes;
	}

	SynBase ref type;
	RefList<SynBase> sizes;
}

class SynTypeReference: SynBase
{
	void SynTypeReference(LexemeRef begin, LexemeRef end, SynBase ref type)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
	}

	SynBase ref type;
}

class SynTypeFunction: SynBase
{
	void SynTypeFunction(LexemeRef begin, LexemeRef end, SynBase ref returnType, RefList<SynBase> arguments)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.returnType = returnType;
		this.arguments = arguments;
	}

	SynBase ref returnType;
	RefList<SynBase> arguments;
}

class SynTypeGenericInstance: SynBase
{
	void SynTypeGenericInstance(LexemeRef begin, LexemeRef end, SynBase ref baseType, RefList<SynBase> types)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.baseType = baseType;
		this.types = types;
	}

	SynBase ref baseType;
	RefList<SynBase> types;
}

class SynTypeof: SynBase
{
	void SynTypeof(LexemeRef begin, LexemeRef end, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	SynBase ref value;
}

class SynBool: SynBase
{
	void SynBool(LexemeRef begin, LexemeRef end, bool value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	bool value;
}

class SynNumber: SynBase
{
	void SynNumber(LexemeRef begin, LexemeRef end, InplaceStr value, InplaceStr suffix)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
		this.suffix = suffix;
	}

	InplaceStr value;
	InplaceStr suffix;
}

class SynNullptr: SynBase
{
	void SynNullptr(LexemeRef begin, LexemeRef end)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
	}
}

class SynCharacter: SynBase
{
	void SynCharacter(LexemeRef begin, LexemeRef end, InplaceStr value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	InplaceStr value;
}

class SynString: SynBase
{
	void SynString(LexemeRef begin, LexemeRef end, bool rawLiteral, InplaceStr value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.rawLiteral = rawLiteral;
		this.value = value;
	}

	bool rawLiteral;
	InplaceStr value;
}

class SynArray: SynBase
{
	void SynArray(LexemeRef begin, LexemeRef end, RefList<SynBase> values)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.values = values;
	}

	RefList<SynBase> values;
}

class SynGenerator: SynBase
{
	void SynGenerator(LexemeRef begin, LexemeRef end, RefList<SynBase> expressions)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.expressions = expressions;
	}

	RefList<SynBase> expressions;
}

class SynAlign: SynBase
{
	void SynAlign(LexemeRef begin, LexemeRef end, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	SynBase ref value;
}

class SynTypedef: SynBase
{
	void SynTypedef(LexemeRef begin, LexemeRef end, SynBase ref type, SynIdentifier ref alias)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.alias = alias;
	}

	SynBase ref type;
	SynIdentifier ref alias;
}

class SynMemberAccess: SynBase
{
	void SynMemberAccess(LexemeRef begin, LexemeRef end, SynBase ref value, SynIdentifier ref member)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
		this.member = member;
	}

	SynBase ref value;
	SynIdentifier ref member;
}

class SynCallArgument: SynBase
{
	void SynCallArgument(LexemeRef begin, LexemeRef end, SynIdentifier ref name, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.name = name;
		this.value = value;
	}

	SynIdentifier ref name;
	SynBase ref value;
}

class SynArrayIndex: SynBase
{
	void SynArrayIndex(LexemeRef begin, LexemeRef end, SynBase ref value, RefList<SynCallArgument> arguments)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
		this.arguments = arguments;
	}

	SynBase ref value;
	RefList<SynCallArgument> arguments;
}

class SynFunctionCall: SynBase
{
	void SynFunctionCall(LexemeRef begin, LexemeRef end, SynBase ref value, RefList<SynBase> aliases, RefList<SynCallArgument> arguments)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
		this.aliases = aliases;
		this.arguments = arguments;
	}

	SynBase ref value;
	RefList<SynBase> aliases;
	RefList<SynCallArgument> arguments;
}

class SynPreModify: SynBase
{
	void SynPreModify(LexemeRef begin, LexemeRef end, SynBase ref value, bool isIncrement)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
		this.isIncrement = isIncrement;
	}

	SynBase ref value;
	bool isIncrement;
}

class SynPostModify: SynBase
{
	void SynPostModify(LexemeRef begin, LexemeRef end, SynBase ref value, bool isIncrement)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
		this.isIncrement = isIncrement;
	}

	SynBase ref value;
	bool isIncrement;
}

class SynGetAddress: SynBase
{
	void SynGetAddress(LexemeRef begin, LexemeRef end, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	SynBase ref value;
}

class SynDereference: SynBase
{
	void SynDereference(LexemeRef begin, LexemeRef end, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	SynBase ref value;
}

class SynSizeof: SynBase
{
	void SynSizeof(LexemeRef begin, LexemeRef end, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	SynBase ref value;
}

class SynNew: SynBase
{
	void SynNew(LexemeRef begin, LexemeRef end, SynBase ref type, RefList<SynCallArgument> arguments, SynBase ref count, RefList<SynBase> constructor)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.arguments = arguments;
		this.count = count;
		this.constructor = constructor;
	}

	SynBase ref type;
	RefList<SynCallArgument> arguments;
	SynBase ref count;
	RefList<SynBase> constructor;
}

class SynConditional: SynBase
{
	void SynConditional(LexemeRef begin, LexemeRef end, SynBase ref condition, SynBase ref trueBlock, SynBase ref falseBlock)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.condition = condition;
		this.trueBlock = trueBlock;
		this.falseBlock = falseBlock;
	}

	SynBase ref condition;
	SynBase ref trueBlock;
	SynBase ref falseBlock;
}

class SynReturn: SynBase
{
	void SynReturn(LexemeRef begin, LexemeRef end, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	SynBase ref value;
}

class SynYield: SynBase
{
	void SynYield(LexemeRef begin, LexemeRef end, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
	}

	SynBase ref value;
}

class SynBreak: SynBase
{
	void SynBreak(LexemeRef begin, LexemeRef end, SynNumber ref number)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.number = number;
	}

	SynNumber ref number;
}

class SynContinue: SynBase
{
	void SynContinue(LexemeRef begin, LexemeRef end, SynNumber ref number)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.number = number;
	}

	SynNumber ref number;
}

class SynLabel: SynBase
{
	void SynLabel(LexemeRef begin, LexemeRef end, SynNumber ref number)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.number = number;
	}

	SynNumber ref number;
}

class SynGoto: SynBase
{
	void SynGoto(LexemeRef begin, LexemeRef end, SynNumber ref number)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.number = number;
	}

	SynNumber ref number;
}

class SynBlock: SynBase
{
	void SynBlock(LexemeRef begin, LexemeRef end, RefList<SynBase> expressions)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.expressions = expressions;
	}

	RefList<SynBase> expressions;
}

class SynIfElse: SynBase
{
	void SynIfElse(LexemeRef begin, LexemeRef end, bool staticIf, SynBase ref condition, SynBase ref trueBlock, SynBase ref falseBlock)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.staticIf = staticIf;
		this.condition = condition;
		this.trueBlock = trueBlock;
		this.falseBlock = falseBlock;
	}

	bool staticIf;
	SynBase ref condition;
	SynBase ref trueBlock;
	SynBase ref falseBlock;
}

class SynFor: SynBase
{
	void SynFor(LexemeRef begin, LexemeRef end, SynBase ref initializer, SynBase ref condition, SynBase ref increment, SynBase ref body)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.initializer = initializer;
		this.condition = condition;
		this.increment = increment;
		this.body = body;
	}

	SynBase ref initializer;
	SynBase ref condition;
	SynBase ref increment;
	SynBase ref body;
}

class SynForEachIterator: SynBase
{
	void SynForEachIterator(LexemeRef begin, LexemeRef end, SynBase ref type, SynIdentifier ref name, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.name = name;
		this.value = value;
	}

	SynBase ref type;
	SynIdentifier ref name;
	SynBase ref value;
}

class SynForEach: SynBase
{
	void SynForEach(LexemeRef begin, LexemeRef end, RefList<SynForEachIterator> iterators, SynBase ref body)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.iterators = iterators;
		this.body = body;
	}

	RefList<SynForEachIterator> iterators;
	SynBase ref body;
}

class SynWhile: SynBase
{
	void SynWhile(LexemeRef begin, LexemeRef end, SynBase ref condition, SynBase ref body)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.condition = condition;
		this.body = body;
	}

	SynBase ref condition;
	SynBase ref body;
}

class SynDoWhile: SynBase
{
	void SynDoWhile(LexemeRef begin, LexemeRef end, RefList<SynBase> expressions, SynBase ref condition)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.expressions = expressions;
		this.condition = condition;
	}

	RefList<SynBase> expressions;
	SynBase ref condition;
}

class SynSwitchCase: SynBase
{
	void SynSwitchCase(LexemeRef begin, LexemeRef end, SynBase ref value, RefList<SynBase> expressions)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.value = value;
		this.expressions = expressions;
	}

	SynBase ref value;
	RefList<SynBase> expressions;
}

class SynSwitch: SynBase
{
	void SynSwitch(LexemeRef begin, LexemeRef end, SynBase ref condition, RefList<SynSwitchCase> cases)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.condition = condition;
		this.cases = cases;
	}

	SynBase ref condition;
	RefList<SynSwitchCase> cases;
}

class SynUnaryOp: SynBase
{
	void SynUnaryOp(LexemeRef begin, LexemeRef end, SynUnaryOpType type, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.value = value;
	}

	SynUnaryOpType type;
	SynBase ref value;
}

class SynBinaryOp: SynBase
{
	void SynBinaryOp(LexemeRef begin, LexemeRef end, SynBinaryOpType type, SynBase ref lhs, SynBase ref rhs)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.lhs = lhs;
		this.rhs = rhs;
	}

	SynBinaryOpType type;
	SynBase ref lhs;
	SynBase ref rhs;
}

class SynAssignment: SynBase
{
	void SynAssignment(LexemeRef begin, LexemeRef end, SynBase ref lhs, SynBase ref rhs)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.lhs = lhs;
		this.rhs = rhs;
	}

	SynBase ref lhs;
	SynBase ref rhs;
}

class SynModifyAssignment: SynBase
{
	void SynModifyAssignment(LexemeRef begin, LexemeRef end, SynModifyAssignType type, SynBase ref lhs, SynBase ref rhs)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.lhs = lhs;
		this.rhs = rhs;
	}

	SynModifyAssignType type;
	SynBase ref lhs;
	SynBase ref rhs;
}

class SynVariableDefinition: SynBase
{
	void SynVariableDefinition(LexemeRef begin, LexemeRef end, SynIdentifier ref name, SynBase ref initializer)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.name = name;
		this.initializer = initializer;
	}

	SynIdentifier ref name;
	SynBase ref initializer;
}

class SynVariableDefinitions: SynBase
{
	void SynVariableDefinitions(LexemeRef begin, LexemeRef end, SynAlign ref alignment, SynBase ref type, RefList<SynVariableDefinition> definitions)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.alignment = alignment;
		this.type = type;
		this.definitions = definitions;
	}

	SynAlign ref alignment;
	SynBase ref type;
	RefList<SynVariableDefinition> definitions;
}

class SynAccessor: SynBase
{
	void SynAccessor(LexemeRef begin, LexemeRef end, SynBase ref type, SynIdentifier ref name, SynBase ref getBlock, SynBase ref setBlock, SynIdentifier ref setName)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.name = name;
		this.getBlock = getBlock;
		this.setBlock = setBlock;
		this.setName = setName;
	}

	SynBase ref type;
	SynIdentifier ref name;
	SynBase ref getBlock;
	SynBase ref setBlock;
	SynIdentifier ref setName;
}

class SynFunctionArgument: SynBase
{
	void SynFunctionArgument(LexemeRef begin, LexemeRef end, bool isExplicit, SynBase ref type, SynIdentifier ref name, SynBase ref initializer)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.isExplicit = isExplicit;
		this.type = type;
		this.name = name;
		this.initializer = initializer;

		assert(name != nullptr);
	}

	bool isExplicit;
	SynBase ref type;
	SynIdentifier ref name;
	SynBase ref initializer;
}

class SynFunctionDefinition: SynBase
{
	void SynFunctionDefinition(LexemeRef begin, LexemeRef end, bool isPrototype, bool isCoroutine, SynBase ref parentType, bool isAccessor, SynBase ref returnType, bool isOperator, SynIdentifier ref name, RefList<SynIdentifier> aliases, RefList<SynFunctionArgument> arguments, RefList<SynBase> expressions)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.isPrototype = isPrototype;
		this.isCoroutine = isCoroutine;
		this.parentType = parentType;
		this.isAccessor = isAccessor;
		this.returnType = returnType;
		this.isOperator = isOperator;
		this.name = name;
		this.aliases = aliases;
		this.arguments = arguments;
		this.expressions = expressions;
	}

	bool isPrototype;
	bool isCoroutine;
	SynBase ref parentType;
	bool isAccessor;
	SynBase ref returnType;
	bool isOperator;
	SynIdentifier ref name;
	RefList<SynIdentifier> aliases;
	RefList<SynFunctionArgument> arguments;
	RefList<SynBase> expressions;
}

class SynShortFunctionArgument: SynBase
{
	void SynShortFunctionArgument(LexemeRef begin, LexemeRef end, SynBase ref type, SynIdentifier ref name)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.name = name;
	}

	SynBase ref type;
	SynIdentifier ref name;
}

class SynShortFunctionDefinition: SynBase
{
	void SynShortFunctionDefinition(LexemeRef begin, LexemeRef end, RefList<SynShortFunctionArgument> arguments, RefList<SynBase> expressions)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.arguments = arguments;
		this.expressions = expressions;
	}

	RefList<SynShortFunctionArgument> arguments;
	RefList<SynBase> expressions;
}

class SynConstant: SynBase
{
	void SynConstant(LexemeRef begin, LexemeRef end, SynIdentifier ref name, SynBase ref value)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.name = name;
		this.value = value;
	}

	SynIdentifier ref name;
	SynBase ref value;
}

class SynConstantSet: SynBase
{
	void SynConstantSet(LexemeRef begin, LexemeRef end, SynBase ref type, RefList<SynConstant> constants)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.type = type;
		this.constants = constants;
	}

	SynBase ref type;
	RefList<SynConstant> constants;
}

class SynClassPrototype: SynBase
{
	void SynClassPrototype(LexemeRef begin, LexemeRef end, SynIdentifier ref name)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.name = name;
	}

	SynIdentifier ref name;
}

class SynClassElements;

class SynClassStaticIf: SynBase
{
	void SynClassStaticIf(LexemeRef begin, LexemeRef end, SynBase ref condition, SynClassElements ref trueBlock, SynClassElements ref falseBlock)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.condition = condition;
		this.trueBlock = trueBlock;
		this.falseBlock = falseBlock;
	}

	SynBase ref condition;
	SynClassElements ref trueBlock;
	SynClassElements ref falseBlock;
}

class SynClassElements: SynBase
{
	void SynClassElements(LexemeRef begin, LexemeRef end, RefList<SynTypedef> typedefs, RefList<SynFunctionDefinition> functions, RefList<SynAccessor> accessors, RefList<SynVariableDefinitions> members, RefList<SynConstantSet> constantSets, RefList<SynClassStaticIf> staticIfs)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.typedefs = typedefs;
		this.functions = functions;
		this.accessors = accessors;
		this.members = members;
		this.constantSets = constantSets;
		this.staticIfs = staticIfs;
	}

	RefList<SynTypedef> typedefs;
	RefList<SynFunctionDefinition> functions;
	RefList<SynAccessor> accessors;
	RefList<SynVariableDefinitions> members;
	RefList<SynConstantSet> constantSets;
	RefList<SynClassStaticIf> staticIfs;
}

class SynClassDefinition: SynBase
{
	void SynClassDefinition(LexemeRef begin, LexemeRef end, SynAlign ref alignment, SynIdentifier ref name, RefList<SynIdentifier> aliases, bool isExtendable, bool isStruct, SynBase ref baseClass, SynClassElements ref elements)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.alignment = alignment;
		this.name = name;
		this.aliases = aliases;
		this.isExtendable = isExtendable;
		this.isStruct = isStruct;
		this.baseClass = baseClass;
		this.elements = elements;

		imported = false;
	}

	bool imported;
	SynAlign ref alignment;
	SynIdentifier ref name;
	RefList<SynIdentifier> aliases;
	bool isExtendable;
	bool isStruct;
	SynBase ref baseClass;
	SynClassElements ref elements;
}

class SynEnumDefinition: SynBase
{
	void SynEnumDefinition(LexemeRef begin, LexemeRef end, SynIdentifier ref name, RefList<SynConstant> values)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.name = name;
		this.values = values;
	}

	SynIdentifier ref name;
	RefList<SynConstant> values;
}

class SynNamespaceElement
{
	void SynNamespaceElement(SynNamespaceElement ref parent, SynIdentifier ref name)
	{
		this.parent = parent;
		this.name = name;

		if(parent)
			fullNameHash = StringHashContinue(StringHashContinue(parent.fullNameHash, "."), name.name);
		else
			fullNameHash = name.name.hash();
	}

	SynNamespaceElement ref parent;
	SynIdentifier ref name;
	int fullNameHash;
}

class SynNamespaceDefinition: SynBase
{
	void SynNamespaceDefinition(LexemeRef begin, LexemeRef end, RefList<SynIdentifier> path, RefList<SynBase> expressions)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.path = path;
		this.expressions = expressions;
	}

	RefList<SynIdentifier> path;
	RefList<SynBase> expressions;
}

class SynModuleImport: SynBase
{
	void SynModuleImport(LexemeRef begin, LexemeRef end, RefList<SynIdentifier> path, ByteCode ref bytecode)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.path = path;
	}

	RefList<SynIdentifier> path;
	ByteCode ref bytecode;
}

class SynModule: SynBase
{
	void SynModule(LexemeRef begin, LexemeRef end, RefList<SynModuleImport> imports, RefList<SynBase> expressions)
	{
		this.begin = begin;
		this.end = end;

		if(begin.owner && end.owner)
			this.pos = InplaceStr(begin.owner.code, begin.pos.pos, end.pos.pos + end.length);
		
		this.imports = imports;
		this.expressions = expressions;
	}

	RefList<SynModuleImport> imports;

	RefList<SynBase> expressions;
}

SynUnaryOpType GetUnaryOpType(LexemeType type)
{
	switch(type)
	{
	case LexemeType.lex_add:
		return SynUnaryOpType.SYN_UNARY_OP_PLUS;
	case LexemeType.lex_sub:
		return SynUnaryOpType.SYN_UNARY_OP_NEGATE;
	case LexemeType.lex_bitnot:
		return SynUnaryOpType.SYN_UNARY_OP_BIT_NOT;
	case LexemeType.lex_lognot:
		return SynUnaryOpType.SYN_UNARY_OP_LOGICAL_NOT;
	default:
		break;
	}

	return SynUnaryOpType.SYN_UNARY_OP_UNKNOWN;
}

SynBinaryOpType GetBinaryOpType(LexemeType type)
{
	switch(type)
	{
	case LexemeType.lex_add:
		return SynBinaryOpType.SYN_BINARY_OP_ADD;
	case LexemeType.lex_sub:
		return SynBinaryOpType.SYN_BINARY_OP_SUB;
	case LexemeType.lex_mul:
		return SynBinaryOpType.SYN_BINARY_OP_MUL;
	case LexemeType.lex_div:
		return SynBinaryOpType.SYN_BINARY_OP_DIV;
	case LexemeType.lex_mod:
		return SynBinaryOpType.SYN_BINARY_OP_MOD;
	case LexemeType.lex_pow:
		return SynBinaryOpType.SYN_BINARY_OP_POW;
	case LexemeType.lex_less:
		return SynBinaryOpType.SYN_BINARY_OP_LESS;
	case LexemeType.lex_lequal:
		return SynBinaryOpType.SYN_BINARY_OP_LESS_EQUAL;
	case LexemeType.lex_shl:
		return SynBinaryOpType.SYN_BINARY_OP_SHL;
	case LexemeType.lex_greater:
		return SynBinaryOpType.SYN_BINARY_OP_GREATER;
	case LexemeType.lex_gequal:
		return SynBinaryOpType.SYN_BINARY_OP_GREATER_EQUAL;
	case LexemeType.lex_shr:
		return SynBinaryOpType.SYN_BINARY_OP_SHR;
	case LexemeType.lex_equal:
		return SynBinaryOpType.SYN_BINARY_OP_EQUAL;
	case LexemeType.lex_nequal:
		return SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL;
	case LexemeType.lex_bitand:
		return SynBinaryOpType.SYN_BINARY_OP_BIT_AND;
	case LexemeType.lex_bitor:
		return SynBinaryOpType.SYN_BINARY_OP_BIT_OR;
	case LexemeType.lex_bitxor:
		return SynBinaryOpType.SYN_BINARY_OP_BIT_XOR;
	case LexemeType.lex_logand:
		return SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND;
	case LexemeType.lex_logor:
		return SynBinaryOpType.SYN_BINARY_OP_LOGICAL_OR;
	case LexemeType.lex_logxor:
		return SynBinaryOpType.SYN_BINARY_OP_LOGICAL_XOR;
	case LexemeType.lex_in:
		return SynBinaryOpType.SYN_BINARY_OP_IN;
	default:
		break;
	}

	return SynBinaryOpType.SYN_BINARY_OP_UNKNOWN;
}

int GetBinaryOpPrecedence(SynBinaryOpType op)
{
	switch(op)
	{
	case SynBinaryOpType.SYN_BINARY_OP_ADD:
		return 2;
	case SynBinaryOpType.SYN_BINARY_OP_SUB:
		return 2;
	case SynBinaryOpType.SYN_BINARY_OP_MUL:
		return 1;
	case SynBinaryOpType.SYN_BINARY_OP_DIV:
		return 1;
	case SynBinaryOpType.SYN_BINARY_OP_MOD:
		return 1;
	case SynBinaryOpType.SYN_BINARY_OP_POW:
		return 0;
	case SynBinaryOpType.SYN_BINARY_OP_LESS:
		return 4;
	case SynBinaryOpType.SYN_BINARY_OP_LESS_EQUAL:
		return 4;
	case SynBinaryOpType.SYN_BINARY_OP_SHL:
		return 3;
	case SynBinaryOpType.SYN_BINARY_OP_GREATER:
		return 4;
	case SynBinaryOpType.SYN_BINARY_OP_GREATER_EQUAL:
		return 4;
	case SynBinaryOpType.SYN_BINARY_OP_SHR:
		return 3;
	case SynBinaryOpType.SYN_BINARY_OP_EQUAL:
		return 5;
	case SynBinaryOpType.SYN_BINARY_OP_NOT_EQUAL:
		return 5;
	case SynBinaryOpType.SYN_BINARY_OP_BIT_AND:
		return 6;
	case SynBinaryOpType.SYN_BINARY_OP_BIT_OR:
		return 8;
	case SynBinaryOpType.SYN_BINARY_OP_BIT_XOR:
		return 7;
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_AND:
		return 9;
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_OR:
		return 11;
	case SynBinaryOpType.SYN_BINARY_OP_LOGICAL_XOR:
		return 10;
	case SynBinaryOpType.SYN_BINARY_OP_IN:
		return 12;
	default:
		break;
	}

	return 0;
}

SynModifyAssignType GetModifyAssignType(LexemeType type)
{
	switch(type)
	{
	case LexemeType.lex_addset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_ADD;
	case LexemeType.lex_subset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_SUB;
	case LexemeType.lex_mulset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_MUL;
	case LexemeType.lex_divset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_DIV;
	case LexemeType.lex_powset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_POW;
	case LexemeType.lex_modset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_MOD;
	case LexemeType.lex_shlset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_SHL;
	case LexemeType.lex_shrset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_SHR;
	case LexemeType.lex_andset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_AND;
	case LexemeType.lex_orset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_OR;
	case LexemeType.lex_xorset:
		return SynModifyAssignType.SYN_MODIFY_ASSIGN_BIT_XOR;
	default:
		break;
	}

	return SynModifyAssignType.SYN_MODIFY_ASSIGN_UNKNOWN;
}
