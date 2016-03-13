#pragma once

#include "Lexer.h"
#include "IntrusiveList.h"
#include "Array.h"
#include "Pool.h"

struct SynBase;
struct SynBinaryOpElement;

struct ParseContext
{
	ParseContext();

	LexemeType Peek();
	bool At(LexemeType type);
	bool Consume(LexemeType type);
	InplaceStr Consume();
	void Skip();

	const char* Position();

	char* AllocString(const char *pos, const char *category, unsigned length);

	Lexeme *currentLexeme;

	FastVector<SynBinaryOpElement> binaryOpStack;

	ChunkedStackPool<8 * 1024> stringPool;

	const char *errorPos;
	InplaceStr errorMsg;
};

struct SynType
{
	SynType(): next(0)
	{
	}

	virtual ~SynType()
	{
	}

	SynType *next;
};

struct SynTypeAuto: SynType
{
	SynTypeAuto(): SynType()
	{
	}
};

struct SynTypeGeneric: SynType
{
	SynTypeGeneric(): SynType()
	{
	}
};

struct SynTypeSimple: SynType
{
	SynTypeSimple(InplaceStr name): SynType(), name(name)
	{
	}

	InplaceStr name;
};

struct SynTypeArray: SynType
{
	SynTypeArray(SynType *type, SynBase *size): SynType(), type(type), size(size)
	{
	}

	SynType *type;
	SynBase *size;
};

struct SynTypeReference: SynType
{
	SynTypeReference(SynType *type): SynType(), type(type)
	{
	}

	SynType *type;
};

struct SynTypeFunction: SynType
{
	SynTypeFunction(SynType *returnType, IntrusiveList<SynType> arguments): SynType(), returnType(returnType), arguments(arguments)
	{
	}

	SynType *returnType;
	IntrusiveList<SynType> arguments;
};

struct SynBase
{
	SynBase(const char *pos): pos(pos), next(0)
	{
	}

	virtual ~SynBase()
	{
	}

	const char *pos;
	SynBase *next;
};

struct SynBool: SynBase
{
	SynBool(const char* pos, bool value): SynBase(pos), value(value)
	{
	}

	bool value;
};

struct SynNumber: SynBase
{
	SynNumber(const char* pos, InplaceStr value): SynBase(pos), value(value)
	{
	}

	InplaceStr value;
};

struct SynNullptr: SynBase
{
	SynNullptr(const char* pos): SynBase(pos)
	{
	}
};

struct SynCharacter: SynBase
{
	SynCharacter(const char* pos, InplaceStr value): SynBase(pos), value(value)
	{
	}

	InplaceStr value;
};

struct SynString: SynBase
{
	SynString(const char* pos, InplaceStr value): SynBase(pos), value(value)
	{
	}

	InplaceStr value;
};

struct SynArray: SynBase
{
	SynArray(const char* pos, IntrusiveList<SynBase> values): SynBase(pos), values(values)
	{
	}

	IntrusiveList<SynBase> values;
};

struct SynIdentifier: SynBase
{
	SynIdentifier(const char* pos, InplaceStr name): SynBase(pos), name(name)
	{
	}

	InplaceStr name;
};

struct SynTypedef: SynBase
{
	SynTypedef(const char* pos, SynType *type, InplaceStr alias): SynBase(pos), type(type), alias(alias)
	{
	}

	SynType *type;
	InplaceStr alias;
};

struct SynMemberAccess: SynBase
{
	SynMemberAccess(const char* pos, SynBase* value, InplaceStr member): SynBase(pos), value(value), member(member)
	{
	}

	SynBase* value;
	InplaceStr member;
};

struct SynCallArgument: SynBase
{
	SynCallArgument(const char* pos, InplaceStr name, SynBase* value): SynBase(pos), name(name), value(value)
	{
	}

	InplaceStr name;
	SynBase* value;
};

struct SynArrayIndex: SynBase
{
	SynArrayIndex(const char* pos, SynBase* value, IntrusiveList<SynCallArgument> arguments): SynBase(pos), value(value), arguments(arguments)
	{
	}

	SynBase* value;
	IntrusiveList<SynCallArgument> arguments;
};

struct SynFunctionCall: SynBase
{
	SynFunctionCall(const char* pos, SynBase* value, IntrusiveList<SynCallArgument> arguments): SynBase(pos), value(value), arguments(arguments)
	{
	}

	SynBase* value;
	IntrusiveList<SynCallArgument> arguments;
};

struct SynGetAddress: SynBase
{
	SynGetAddress(const char* pos, SynBase* value): SynBase(pos), value(value)
	{
	}

	SynBase* value;
};

struct SynDereference: SynBase
{
	SynDereference(const char* pos, SynBase* value): SynBase(pos), value(value)
	{
	}

	SynBase* value;
};

struct SynNew: SynBase
{
	SynNew(const char* pos, SynType *type, IntrusiveList<SynCallArgument> arguments, SynBase *count): SynBase(pos), type(type), arguments(arguments), count(count)
	{
	}

	SynType *type;
	IntrusiveList<SynCallArgument> arguments;
	SynBase *count;
};

struct SynReturn: SynBase
{
	SynReturn(const char* pos, SynBase* value): SynBase(pos), value(value)
	{
	}

	SynBase *value;
};

enum SynUnaryOpType
{
	SYN_UNARY_OP_UNKNOWN,

	SYN_UNARY_OP_PLUS,
	SYN_UNARY_OP_NEGATE,
	SYN_UNARY_OP_BIT_NOT,
	SYN_UNARY_OP_LOGICAL_NOT,
};

struct SynUnaryOp: SynBase
{
	SynUnaryOp(const char* pos, SynUnaryOpType type, SynBase* value): SynBase(pos), type(type), value(value)
	{
	}

	SynUnaryOpType type;
	SynBase* value;
};

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
	SYN_BINARY_OP_IN,
};

struct SynBinaryOpElement
{
	SynBinaryOpElement(): pos(0), type(SYN_BINARY_OP_UNKNOWN), value(0)
	{
	}

	SynBinaryOpElement(const char* pos, SynBinaryOpType type, SynBase* value): pos(pos), type(type), value(value)
	{
	}

	const char* pos;
	SynBinaryOpType type;
	SynBase* value;
};

struct SynBinaryOp: SynBase
{
	SynBinaryOp(const char* pos, SynBinaryOpType type, SynBase* lhs, SynBase* rhs): SynBase(pos), type(type), lhs(lhs), rhs(rhs)
	{
	}

	SynBinaryOpType type;
	SynBase* lhs;
	SynBase* rhs;
};

struct SynAssignment: SynBase
{
	SynAssignment(const char* pos, SynBase* lhs, SynBase* rhs): SynBase(pos), lhs(lhs), rhs(rhs)
	{
	}

	SynBase* lhs;
	SynBase* rhs;
};

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
	SYN_MODIFY_ASSIGN_BIT_XOR,
};

struct SynModifyAssignment: SynBase
{
	SynModifyAssignment(const char* pos, SynModifyAssignType type, SynBase* lhs, SynBase* rhs): SynBase(pos), type(type), lhs(lhs), rhs(rhs)
	{
	}

	SynModifyAssignType type;
	SynBase* lhs;
	SynBase* rhs;
};

struct SynVariableDefinition: SynBase
{
	SynVariableDefinition(const char* pos, InplaceStr name, SynBase *initializer): SynBase(pos), name(name), initializer(initializer)
	{
	}

	InplaceStr name;
	SynBase *initializer;
};

struct SynVariableDefinitions: SynBase
{
	SynVariableDefinitions(const char* pos, SynType *type, IntrusiveList<SynVariableDefinition> definitions): SynBase(pos), type(type), definitions(definitions)
	{
	}

	SynType *type;
	IntrusiveList<SynVariableDefinition> definitions;
};

struct SynFunctionArgument: SynBase
{
	SynFunctionArgument(const char* pos, SynType* type, InplaceStr name, SynBase* defaultValue): SynBase(pos), type(type), name(name), defaultValue(defaultValue)
	{
	}

	SynType* type;
	InplaceStr name;
	SynBase* defaultValue;
};

struct SynFunctionDefinition: SynBase
{
	SynFunctionDefinition(const char* pos, bool prototype, SynType *returnType, InplaceStr name, IntrusiveList<SynFunctionArgument> arguments, IntrusiveList<SynBase> expressions): SynBase(pos), prototype(prototype), returnType(returnType), name(name), arguments(arguments), expressions(expressions)
	{
	}

	bool prototype;
	SynType *returnType;
	InplaceStr name;
	IntrusiveList<SynFunctionArgument> arguments;
	IntrusiveList<SynBase> expressions;
};

struct SynClassPototype: SynBase
{
	SynClassPototype(const char* pos, InplaceStr name): SynBase(pos), name(name)
	{
	}

	InplaceStr name;
};

struct SynClassDefinition: SynBase
{
	SynClassDefinition(const char* pos, InplaceStr name, bool extendable, SynType *baseClass, IntrusiveList<SynTypedef> typedefs, IntrusiveList<SynFunctionDefinition> functions, IntrusiveList<SynVariableDefinitions> members): SynBase(pos), name(name), extendable(extendable), baseClass(baseClass), typedefs(typedefs), functions(functions), members(members)
	{
	}

	InplaceStr name;
	bool extendable;
	SynType *baseClass;
	IntrusiveList<SynTypedef> typedefs;
	IntrusiveList<SynFunctionDefinition> functions;
	IntrusiveList<SynVariableDefinitions> members;
};

struct SynModuleImport: SynBase
{
	SynModuleImport(const char* pos, InplaceStr name): SynBase(pos), name(name)
	{
	}

	InplaceStr name;
};

struct SynModule: SynBase
{
	SynModule(const char* pos, IntrusiveList<SynModuleImport> imports, IntrusiveList<SynBase> expressions): SynBase(pos), imports(imports), expressions(expressions)
	{
	}

	IntrusiveList<SynModuleImport> imports;

	IntrusiveList<SynBase> expressions;
};

SynBase* Parse(ParseContext &context, const char *code);
