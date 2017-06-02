#pragma once

#include "Lexer.h"
#include "IntrusiveList.h"
#include "Array.h"

struct SynBase;

struct SynBinaryOpElement;
struct SynNamespaceElement;

struct ParseContext
{
	ParseContext();

	LexemeType Peek();
	InplaceStr Value();
	bool At(LexemeType type);
	bool Consume(LexemeType type);
	bool Consume(const char *str);
	InplaceStr Consume();
	void Skip();

	const char* Position();

	SynNamespaceElement* IsNamespace(SynNamespaceElement *parent, InplaceStr name);
	SynNamespaceElement* PushNamespace(InplaceStr name);
	void PopNamespace();

	Lexeme *currentLexeme;

	FastVector<SynBinaryOpElement> binaryOpStack;

	FastVector<SynNamespaceElement*> namespaceList;
	SynNamespaceElement *currentNamespace;

	const char *errorPos;
	char *errorBuf;
	unsigned errorBufSize;
};

struct SynBase
{
	SynBase(unsigned typeID, const char *pos): typeID(typeID), pos(pos), next(0)
	{
	}

	virtual ~SynBase()
	{
	}

	unsigned typeID;

	const char *pos;
	SynBase *next;
};

template<typename T>
bool isType(SynBase *node)
{
	return node->typeID == typename T::myTypeID;
}

template<typename T>
T* getType(SynBase *node)
{
	if(node && isType<T>(node))
		return static_cast<T*>(node);

	return 0;
}

struct SynNothing: SynBase
{
	SynNothing(const char* pos): SynBase(myTypeID, pos)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct SynIdentifier: SynBase
{
	SynIdentifier(const char* pos, InplaceStr name): SynBase(myTypeID, pos), name(name)
	{
	}

	InplaceStr name;

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeAuto: SynBase
{
	SynTypeAuto(const char *pos): SynBase(myTypeID, pos)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeGeneric: SynBase
{
	SynTypeGeneric(const char *pos): SynBase(myTypeID, pos)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeSimple: SynBase
{
	SynTypeSimple(const char *pos, IntrusiveList<SynIdentifier> path, InplaceStr name): SynBase(myTypeID, pos), path(path), name(name)
	{
	}

	IntrusiveList<SynIdentifier> path;
	InplaceStr name;

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeAlias: SynBase
{
	SynTypeAlias(const char *pos, InplaceStr name): SynBase(myTypeID, pos), name(name)
	{
	}

	InplaceStr name;

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeArray: SynBase
{
	SynTypeArray(const char *pos, SynBase *type, IntrusiveList<SynBase> sizes): SynBase(myTypeID, pos), type(type), sizes(sizes)
	{
	}

	SynBase *type;
	IntrusiveList<SynBase> sizes;

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeReference: SynBase
{
	SynTypeReference(const char *pos, SynBase *type): SynBase(myTypeID, pos), type(type)
	{
	}

	SynBase *type;

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeFunction: SynBase
{
	SynTypeFunction(const char *pos, SynBase *returnType, IntrusiveList<SynBase> arguments): SynBase(myTypeID, pos), returnType(returnType), arguments(arguments)
	{
	}

	SynBase *returnType;
	IntrusiveList<SynBase> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeGenericInstance: SynBase
{
	SynTypeGenericInstance(const char *pos, SynBase *baseType, IntrusiveList<SynBase> types): SynBase(myTypeID, pos), baseType(baseType), types(types)
	{
	}

	SynBase *baseType;
	IntrusiveList<SynBase> types;

	static const unsigned myTypeID = __LINE__;
};

struct SynTypeof: SynBase
{
	SynTypeof(const char *pos, SynBase* value): SynBase(myTypeID, pos), value(value)
	{
	}

	SynBase *value;

	static const unsigned myTypeID = __LINE__;
};

struct SynBool: SynBase
{
	SynBool(const char* pos, bool value): SynBase(myTypeID, pos), value(value)
	{
	}

	bool value;

	static const unsigned myTypeID = __LINE__;
};

struct SynNumber: SynBase
{
	SynNumber(const char* pos, InplaceStr value, InplaceStr suffix): SynBase(myTypeID, pos), value(value), suffix(suffix)
	{
	}

	InplaceStr value;
	InplaceStr suffix;

	static const unsigned myTypeID = __LINE__;
};

struct SynNullptr: SynBase
{
	SynNullptr(const char* pos): SynBase(myTypeID, pos)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct SynCharacter: SynBase
{
	SynCharacter(const char* pos, InplaceStr value): SynBase(myTypeID, pos), value(value)
	{
	}

	InplaceStr value;

	static const unsigned myTypeID = __LINE__;
};

struct SynString: SynBase
{
	SynString(const char* pos, bool rawLiteral, InplaceStr value): SynBase(myTypeID, pos), rawLiteral(rawLiteral), value(value)
	{
	}

	bool rawLiteral;
	InplaceStr value;

	static const unsigned myTypeID = __LINE__;
};

struct SynArray: SynBase
{
	SynArray(const char* pos, IntrusiveList<SynBase> values): SynBase(myTypeID, pos), values(values)
	{
	}

	IntrusiveList<SynBase> values;

	static const unsigned myTypeID = __LINE__;
};

struct SynGenerator: SynBase
{
	SynGenerator(const char* pos, IntrusiveList<SynBase> expressions): SynBase(myTypeID, pos), expressions(expressions)
	{
	}

	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct SynAlign: SynBase
{
	SynAlign(const char* pos, SynNumber* value): SynBase(myTypeID, pos), value(value)
	{
	}

	SynNumber* value;

	static const unsigned myTypeID = __LINE__;
};

struct SynTypedef: SynBase
{
	SynTypedef(const char* pos, SynBase *type, InplaceStr alias): SynBase(myTypeID, pos), type(type), alias(alias)
	{
	}

	SynBase *type;
	InplaceStr alias;

	static const unsigned myTypeID = __LINE__;
};

struct SynMemberAccess: SynBase
{
	SynMemberAccess(const char* pos, SynBase* value, InplaceStr member): SynBase(myTypeID, pos), value(value), member(member)
	{
	}

	SynBase* value;
	InplaceStr member;

	static const unsigned myTypeID = __LINE__;
};

struct SynCallArgument: SynBase
{
	SynCallArgument(const char* pos, InplaceStr name, SynBase* value): SynBase(myTypeID, pos), name(name), value(value)
	{
	}

	InplaceStr name;
	SynBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct SynArrayIndex: SynBase
{
	SynArrayIndex(const char* pos, SynBase* value, IntrusiveList<SynCallArgument> arguments): SynBase(myTypeID, pos), value(value), arguments(arguments)
	{
	}

	SynBase* value;
	IntrusiveList<SynCallArgument> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct SynFunctionCall: SynBase
{
	SynFunctionCall(const char* pos, SynBase* value, IntrusiveList<SynBase> aliases, IntrusiveList<SynCallArgument> arguments): SynBase(myTypeID, pos), value(value), aliases(aliases), arguments(arguments)
	{
	}

	SynBase* value;
	IntrusiveList<SynBase> aliases;
	IntrusiveList<SynCallArgument> arguments;

	static const unsigned myTypeID = __LINE__;
};

struct SynPreModify: SynBase
{
	SynPreModify(const char* pos, SynBase* value, bool isIncrement): SynBase(myTypeID, pos), value(value), isIncrement(isIncrement)
	{
	}

	SynBase* value;
	bool isIncrement;

	static const unsigned myTypeID = __LINE__;
};

struct SynPostModify: SynBase
{
	SynPostModify(const char* pos, SynBase* value, bool isIncrement): SynBase(myTypeID, pos), value(value), isIncrement(isIncrement)
	{
	}

	SynBase* value;
	bool isIncrement;

	static const unsigned myTypeID = __LINE__;
};

struct SynGetAddress: SynBase
{
	SynGetAddress(const char* pos, SynBase* value): SynBase(myTypeID, pos), value(value)
	{
	}

	SynBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct SynDereference: SynBase
{
	SynDereference(const char* pos, SynBase* value): SynBase(myTypeID, pos), value(value)
	{
	}

	SynBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct SynSizeof: SynBase
{
	SynSizeof(const char* pos, SynBase* value): SynBase(myTypeID, pos), value(value)
	{
	}

	SynBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct SynNew: SynBase
{
	SynNew(const char* pos, SynBase *type, IntrusiveList<SynCallArgument> arguments, SynBase *count, IntrusiveList<SynBase> constructor): SynBase(myTypeID, pos), type(type), arguments(arguments), count(count), constructor(constructor)
	{
	}

	SynBase *type;
	IntrusiveList<SynCallArgument> arguments;
	SynBase *count;
	IntrusiveList<SynBase> constructor;

	static const unsigned myTypeID = __LINE__;
};

struct SynConditional: SynBase
{
	SynConditional(const char* pos, SynBase* condition, SynBase* trueBlock, SynBase* falseBlock): SynBase(myTypeID, pos), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	SynBase* condition;
	SynBase* trueBlock;
	SynBase* falseBlock;

	static const unsigned myTypeID = __LINE__;
};

struct SynReturn: SynBase
{
	SynReturn(const char* pos, SynBase* value): SynBase(myTypeID, pos), value(value)
	{
	}

	SynBase *value;

	static const unsigned myTypeID = __LINE__;
};

struct SynYield: SynBase
{
	SynYield(const char* pos, SynBase* value): SynBase(myTypeID, pos), value(value)
	{
	}

	SynBase *value;

	static const unsigned myTypeID = __LINE__;
};

struct SynBreak: SynBase
{
	SynBreak(const char* pos, SynNumber* number): SynBase(myTypeID, pos), number(number)
	{
	}

	SynNumber* number;

	static const unsigned myTypeID = __LINE__;
};

struct SynContinue: SynBase
{
	SynContinue(const char* pos, SynNumber* number): SynBase(myTypeID, pos), number(number)
	{
	}

	SynNumber* number;

	static const unsigned myTypeID = __LINE__;
};

struct SynBlock: SynBase
{
	SynBlock(const char* pos, IntrusiveList<SynBase> expressions): SynBase(myTypeID, pos), expressions(expressions)
	{
	}

	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct SynIfElse: SynBase
{
	SynIfElse(const char* pos, bool staticIf, SynBase* condition, SynBase* trueBlock, SynBase* falseBlock): SynBase(myTypeID, pos), staticIf(staticIf), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	bool staticIf;
	SynBase* condition;
	SynBase* trueBlock;
	SynBase* falseBlock;

	static const unsigned myTypeID = __LINE__;
};

struct SynFor: SynBase
{
	SynFor(const char* pos, SynBase* initializer, SynBase* condition, SynBase* increment, SynBase* body): SynBase(myTypeID, pos), initializer(initializer), condition(condition), increment(increment), body(body)
	{
	}

	SynBase* initializer;
	SynBase* condition;
	SynBase* increment;
	SynBase* body;

	static const unsigned myTypeID = __LINE__;
};

struct SynForEachIterator: SynBase
{
	SynForEachIterator(const char* pos, SynBase* type, InplaceStr name, SynBase* value): SynBase(myTypeID, pos), type(type), name(name), value(value)
	{
	}

	SynBase* type;
	InplaceStr name;
	SynBase* value;

	static const unsigned myTypeID = __LINE__;
};

struct SynForEach: SynBase
{
	SynForEach(const char* pos, IntrusiveList<SynForEachIterator> iterators, SynBase* body): SynBase(myTypeID, pos), iterators(iterators), body(body)
	{
	}

	IntrusiveList<SynForEachIterator> iterators;
	SynBase* body;

	static const unsigned myTypeID = __LINE__;
};

struct SynWhile: SynBase
{
	SynWhile(const char* pos, SynBase* condition, SynBase* body): SynBase(myTypeID, pos), condition(condition), body(body)
	{
	}

	SynBase* condition;
	SynBase* body;

	static const unsigned myTypeID = __LINE__;
};

struct SynDoWhile: SynBase
{
	SynDoWhile(const char* pos, IntrusiveList<SynBase> expressions, SynBase* condition): SynBase(myTypeID, pos), expressions(expressions), condition(condition)
	{
	}

	IntrusiveList<SynBase> expressions;
	SynBase* condition;

	static const unsigned myTypeID = __LINE__;
};

struct SynSwitchCase: SynBase
{
	SynSwitchCase(const char* pos, SynBase* value, IntrusiveList<SynBase> expressions): SynBase(myTypeID, pos), value(value), expressions(expressions)
	{
	}

	SynBase* value;
	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct SynSwitch: SynBase
{
	SynSwitch(const char* pos, SynBase* condition, IntrusiveList<SynSwitchCase> cases): SynBase(myTypeID, pos), condition(condition), cases(cases)
	{
	}

	SynBase* condition;
	IntrusiveList<SynSwitchCase> cases;

	static const unsigned myTypeID = __LINE__;
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
	SynUnaryOp(const char* pos, SynUnaryOpType type, SynBase* value): SynBase(myTypeID, pos), type(type), value(value)
	{
	}

	SynUnaryOpType type;
	SynBase* value;

	static const unsigned myTypeID = __LINE__;
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
	SynBinaryOp(const char* pos, SynBinaryOpType type, SynBase* lhs, SynBase* rhs): SynBase(myTypeID, pos), type(type), lhs(lhs), rhs(rhs)
	{
	}

	SynBinaryOpType type;
	SynBase* lhs;
	SynBase* rhs;

	static const unsigned myTypeID = __LINE__;
};

struct SynAssignment: SynBase
{
	SynAssignment(const char* pos, SynBase* lhs, SynBase* rhs): SynBase(myTypeID, pos), lhs(lhs), rhs(rhs)
	{
	}

	SynBase* lhs;
	SynBase* rhs;

	static const unsigned myTypeID = __LINE__;
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
	SynModifyAssignment(const char* pos, SynModifyAssignType type, SynBase* lhs, SynBase* rhs): SynBase(myTypeID, pos), type(type), lhs(lhs), rhs(rhs)
	{
	}

	SynModifyAssignType type;
	SynBase* lhs;
	SynBase* rhs;

	static const unsigned myTypeID = __LINE__;
};

struct SynVariableDefinition: SynBase
{
	SynVariableDefinition(const char* pos, InplaceStr name, SynBase *initializer): SynBase(myTypeID, pos), name(name), initializer(initializer)
	{
	}

	InplaceStr name;
	SynBase *initializer;

	static const unsigned myTypeID = __LINE__;
};

struct SynVariableDefinitions: SynBase
{
	SynVariableDefinitions(const char* pos, SynAlign* align, SynBase *type, IntrusiveList<SynVariableDefinition> definitions): SynBase(myTypeID, pos), align(align), type(type), definitions(definitions)
	{
	}

	SynAlign* align;
	SynBase *type;
	IntrusiveList<SynVariableDefinition> definitions;

	static const unsigned myTypeID = __LINE__;
};

struct SynAccessor: SynBase
{
	SynAccessor(const char* pos, SynBase *type, InplaceStr name, SynBlock *getBlock, SynBlock *setBlock, InplaceStr setName): SynBase(myTypeID, pos), type(type), name(name), getBlock(getBlock), setBlock(setBlock), setName(setName)
	{
	}

	SynBase *type;
	InplaceStr name;
	SynBlock *getBlock;
	SynBlock *setBlock;
	InplaceStr setName;

	static const unsigned myTypeID = __LINE__;
};

struct SynFunctionArgument: SynBase
{
	SynFunctionArgument(const char* pos, bool isExplicit, SynBase* type, InplaceStr name, SynBase* initializer): SynBase(myTypeID, pos), isExplicit(isExplicit), type(type), name(name), initializer(initializer)
	{
	}

	bool isExplicit;
	SynBase* type;
	InplaceStr name;
	SynBase* initializer;

	static const unsigned myTypeID = __LINE__;
};

struct SynFunctionDefinition: SynBase
{
	SynFunctionDefinition(const char* pos, bool prototype, bool coroutine, SynBase *parentType, bool accessor, SynBase *returnType, bool isOperator, InplaceStr name, IntrusiveList<SynIdentifier> aliases, IntrusiveList<SynFunctionArgument> arguments, IntrusiveList<SynBase> expressions): SynBase(myTypeID, pos), prototype(prototype), coroutine(coroutine), parentType(parentType), accessor(accessor), returnType(returnType), isOperator(isOperator), name(name), aliases(aliases), arguments(arguments), expressions(expressions)
	{
	}

	bool prototype;
	bool coroutine;
	SynBase *parentType;
	bool accessor;
	SynBase *returnType;
	bool isOperator;
	InplaceStr name;
	IntrusiveList<SynIdentifier> aliases;
	IntrusiveList<SynFunctionArgument> arguments;
	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct SynShortFunctionArgument: SynBase
{
	SynShortFunctionArgument(const char* pos, SynBase* type, InplaceStr name): SynBase(myTypeID, pos), type(type), name(name)
	{
	}

	SynBase* type;
	InplaceStr name;

	static const unsigned myTypeID = __LINE__;
};

struct SynShortFunctionDefinition: SynBase
{
	SynShortFunctionDefinition(const char* pos, IntrusiveList<SynShortFunctionArgument> arguments, IntrusiveList<SynBase> expressions): SynBase(myTypeID, pos), arguments(arguments), expressions(expressions)
	{
	}

	IntrusiveList<SynShortFunctionArgument> arguments;
	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct SynConstant: SynBase
{
	SynConstant(const char* pos, InplaceStr name, SynBase *value): SynBase(myTypeID, pos), name(name), value(value)
	{
	}

	InplaceStr name;
	SynBase *value;

	static const unsigned myTypeID = __LINE__;
};

struct SynConstantSet: SynBase
{
	SynConstantSet(const char* pos, SynBase *type, IntrusiveList<SynConstant> constants): SynBase(myTypeID, pos), type(type), constants(constants)
	{
	}

	SynBase *type;
	IntrusiveList<SynConstant> constants;

	static const unsigned myTypeID = __LINE__;
};

struct SynClassPrototype: SynBase
{
	SynClassPrototype(const char* pos, InplaceStr name): SynBase(myTypeID, pos), name(name)
	{
	}

	InplaceStr name;

	static const unsigned myTypeID = __LINE__;
};

struct SynClassElements;

struct SynClassStaticIf: SynBase
{
	SynClassStaticIf(const char* pos, SynBase *condition, SynClassElements *trueBlock, SynClassElements *falseBlock): SynBase(myTypeID, pos), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	SynBase *condition;
	SynClassElements *trueBlock;
	SynClassElements *falseBlock;

	static const unsigned myTypeID = __LINE__;
};

struct SynClassElements: SynBase
{
	SynClassElements(const char* pos, IntrusiveList<SynTypedef> typedefs, IntrusiveList<SynFunctionDefinition> functions, IntrusiveList<SynAccessor> accessors, IntrusiveList<SynVariableDefinitions> members, IntrusiveList<SynConstantSet> constantSets, IntrusiveList<SynClassStaticIf> staticIfs): SynBase(myTypeID, pos), typedefs(typedefs), functions(functions), accessors(accessors), members(members), constantSets(constantSets), staticIfs(staticIfs)
	{
	}

	IntrusiveList<SynTypedef> typedefs;
	IntrusiveList<SynFunctionDefinition> functions;
	IntrusiveList<SynAccessor> accessors;
	IntrusiveList<SynVariableDefinitions> members;
	IntrusiveList<SynConstantSet> constantSets;
	IntrusiveList<SynClassStaticIf> staticIfs;

	static const unsigned myTypeID = __LINE__;
};

struct SynClassDefinition: SynBase
{
	SynClassDefinition(const char* pos, SynAlign* align, InplaceStr name, IntrusiveList<SynIdentifier> aliases, bool extendable, SynBase *baseClass, SynClassElements *elements): SynBase(myTypeID, pos), align(align), name(name), aliases(aliases), extendable(extendable), baseClass(baseClass), elements(elements)
	{
		imported = false;
	}

	bool imported;
	SynAlign* align;
	InplaceStr name;
	IntrusiveList<SynIdentifier> aliases;
	bool extendable;
	SynBase *baseClass;
	SynClassElements *elements;

	static const unsigned myTypeID = __LINE__;
};

struct SynEnumDefinition: SynBase
{
	SynEnumDefinition(const char* pos, InplaceStr name, IntrusiveList<SynConstant> values): SynBase(myTypeID, pos), name(name), values(values)
	{
	}

	InplaceStr name;
	IntrusiveList<SynConstant> values;

	static const unsigned myTypeID = __LINE__;
};

struct SynNamespaceElement
{
	SynNamespaceElement(): parent(0)
	{
	}

	SynNamespaceElement *parent;
	InplaceStr name;
};

struct SynNamespaceDefinition: SynBase
{
	SynNamespaceDefinition(const char* pos, IntrusiveList<SynIdentifier> path, IntrusiveList<SynBase> expressions): SynBase(myTypeID, pos), path(path), expressions(expressions)
	{
	}

	IntrusiveList<SynIdentifier> path;
	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

struct SynModuleImport: SynBase
{
	SynModuleImport(const char* pos, IntrusiveList<SynIdentifier> path): SynBase(myTypeID, pos), path(path)
	{
	}

	IntrusiveList<SynIdentifier> path;

	static const unsigned myTypeID = __LINE__;
};

struct SynModule: SynBase
{
	SynModule(const char* pos, IntrusiveList<SynModuleImport> imports, IntrusiveList<SynBase> expressions): SynBase(myTypeID, pos), imports(imports), expressions(expressions)
	{
	}

	IntrusiveList<SynModuleImport> imports;

	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = __LINE__;
};

SynBase* ParseClassDefinition(ParseContext &ctx);
SynBase* Parse(ParseContext &context, const char *code);

const char* GetOpName(SynUnaryOpType type);
const char* GetOpName(SynBinaryOpType type);
const char* GetOpName(SynModifyAssignType type);
