#pragma once

#include "Lexer.h"
#include "IntrusiveList.h"
#include "Allocator.h"
#include "Array.h"
#include "DenseMap.h"
#include "Statistics.h"

struct CompilerContext;

struct ByteCode;

struct ModuleData;

struct SynBase;
struct SynIdentifier;

enum SynUnaryOpType
{
	SYN_UNARY_OP_UNKNOWN,

	SYN_UNARY_OP_PLUS,
	SYN_UNARY_OP_NEGATE,
	SYN_UNARY_OP_BIT_NOT,
	SYN_UNARY_OP_LOGICAL_NOT
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
	SYN_BINARY_OP_IN
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
	SYN_MODIFY_ASSIGN_BIT_XOR
};

struct SynBinaryOpElement
{
	SynBinaryOpElement(): begin(0), end(0), type(SYN_BINARY_OP_UNKNOWN), value(0)
	{
	}

	SynBinaryOpElement(Lexeme* begin, Lexeme *end, SynBinaryOpType type, SynBase* value): begin(begin), end(end), type(type), value(value)
	{
	}

	Lexeme* begin;
	Lexeme* end;
	SynBinaryOpType type;
	SynBase* value;
};

struct SynNamespaceElement;

struct ErrorInfo
{
	ErrorInfo(Allocator *allocator, const char* messageStart, const char* messageEnd, Lexeme* begin, Lexeme* end, const char* pos): messageStart(messageStart), messageEnd(messageEnd), begin(begin), end(end), pos(pos), related(allocator)
	{
		parentModule = NULL;
	}

	const char* messageStart;
	const char* messageEnd;

	Lexeme* begin;
	Lexeme* end;

	const char* pos;

	ModuleData* parentModule;

	SmallArray<ErrorInfo*, 4> related;
};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())
#endif

struct ParseContext
{
	ParseContext(Allocator *allocator, int optimizationLevel, ArrayView<InplaceStr> activeImports);

	LexemeType Peek();
	InplaceStr Value();
	bool At(LexemeType type);
	bool Consume(LexemeType type);
	bool Consume(const char *str);
	InplaceStr Consume();
	void Skip();

	Lexeme* First();
	Lexeme* Current();
	Lexeme* Previous();
	Lexeme* Last();

	const char* Position();
	const char* LastEnding();

	SynNamespaceElement* IsNamespace(SynNamespaceElement *parent, InplaceStr name);
	SynNamespaceElement* PushNamespace(SynIdentifier *name);
	void PopNamespace();

	const char *code;

	const char *moduleRoot;

	char* (*bytecodeBuilder)(Allocator *allocator, InplaceStr moduleName, const char *moduleRoot, bool addExtension, const char **errorPos, char *errorBuf, unsigned errorBufSize, int optimizationLevel, ArrayView<InplaceStr> activeImports, CompilerStatistics *statistics);

	Lexer lexer;

	Lexeme *firstLexeme;
	Lexeme *currentLexeme;
	Lexeme *lastLexeme;

	SmallArray<SynBinaryOpElement, 32> binaryOpStack;

	unsigned expressionGroupDepth;
	unsigned expressionBlockDepth;
	unsigned statementBlockDepth;

	SmallArray<SynNamespaceElement*, 32> namespaceList;
	SynNamespaceElement *currentNamespace;

	int optimizationLevel;

	SmallArray<InplaceStr, 8> activeImports;

	bool errorHandlerActive;
	jmp_buf errorHandler;
	const char *errorPos;
	unsigned errorCount;
	char *errorBuf;
	unsigned errorBufSize;
	char *errorBufLocation;

	SmallArray<ErrorInfo*, 4> errorInfo;

	CompilerStatistics statistics;

	SmallDenseMap<unsigned, bool, SmallDenseMapUnsignedHasher, 128> nonTypeLocations;
	SmallDenseMap<unsigned, bool, SmallDenseMapUnsignedHasher, 128> nonFunctionDefinitionLocations;

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

struct SynBase
{
	SynBase(unsigned typeID, Lexeme *begin, Lexeme *end): typeID(typeID), begin(begin), end(end), pos(begin ? begin->pos : 0, end ? end->pos + end->length : 0), next(0), listed(false), isInternal(false)
	{
	}

	virtual ~SynBase()
	{
	}

	unsigned typeID;

	Lexeme *begin;
	Lexeme *end;

	InplaceStr pos;

	SynBase *next;
	bool listed;

	bool isInternal;
};

template<typename T>
bool isType(SynBase *node)
{
	return node && node->typeID == T::myTypeID;
}

template<typename T>
T* getType(SynBase *node)
{
	if(node && node->typeID == T::myTypeID)
		return static_cast<T*>(node);

	return 0;
}

namespace SynNode
{
	enum SynNodeId
	{
		SynError,
		SynImportLocation,
		SynInternal,
		SynNothing,
		SynIdentifier,
		SynTypeAuto,
		SynTypeGeneric,
		SynTypeSimple,
		SynTypeAlias,
		SynTypeArray,
		SynTypeReference,
		SynTypeFunction,
		SynTypeGenericInstance,
		SynTypeof,
		SynBool,
		SynNumber,
		SynNullptr,
		SynCharacter,
		SynString,
		SynArray,
		SynGenerator,
		SynAlign,
		SynTemplate,
		SynTypedef,
		SynMemberAccess,
		SynCallArgument,
		SynArrayIndex,
		SynFunctionCall,
		SynPreModify,
		SynPostModify,
		SynGetAddress,
		SynDereference,
		SynSizeof,
		SynNew,
		SynConditional,
		SynReturn,
		SynYield,
		SynBreak,
		SynContinue,
		SynGoto,
		SynLabel,
		SynBlock,
		SynIfElse,
		SynFor,
		SynForEachIterator,
		SynForEach,
		SynWhile,
		SynDoWhile,
		SynSwitchCase,
		SynSwitch,
		SynUnaryOp,
		SynBinaryOp,
		SynAssignment,
		SynModifyAssignment,
		SynVariableDefinition,
		SynVariableDefinitions,
		SynAccessor,
		SynFunctionArgument,
		SynFunctionDefinition,
		SynShortFunctionArgument,
		SynShortFunctionDefinition,
		SynConstant,
		SynConstantSet,
		SynClassPrototype,
		SynClassStaticIf,
		SynClassElements,
		SynClassDefinition,
		SynEnumDefinition,
		SynNamespaceDefinition,
		SynModuleImport,
		SynModule,
	};
}

struct SynError: SynBase
{
	SynError(Lexeme *begin, Lexeme *end): SynBase(myTypeID, begin, end)
	{
	}

	static const unsigned myTypeID = SynNode::SynError;
};

struct SynImportLocation: SynBase
{
	SynImportLocation(Lexeme *begin, Lexeme *end): SynBase(myTypeID, begin, end)
	{
	}

	static const unsigned myTypeID = SynNode::SynImportLocation;
};

struct SynInternal: SynBase
{
	SynInternal(SynBase *source): SynBase(myTypeID, source->begin, source->end)
	{
		isInternal = true;
	}

	static const unsigned myTypeID = SynNode::SynInternal;
};

struct SynNothing: SynBase
{
	SynNothing(Lexeme *begin, Lexeme *end): SynBase(myTypeID, begin, end)
	{
	}

	static const unsigned myTypeID = SynNode::SynNothing;
};

struct SynIdentifier: SynBase
{
	explicit SynIdentifier(InplaceStr name): SynBase(myTypeID, 0, 0), name(name)
	{
	}

	SynIdentifier(const SynIdentifier& source, InplaceStr name): SynBase(myTypeID, source.begin, source.end), name(name)
	{
	}

	SynIdentifier(SynIdentifier* source, InplaceStr name): SynBase(myTypeID, source->begin, source->end), name(name)
	{
	}

	SynIdentifier(Lexeme *begin, Lexeme *end, InplaceStr name): SynBase(myTypeID, begin, end), name(name)
	{
	}

	InplaceStr name;

	static const unsigned myTypeID = SynNode::SynIdentifier;
};

struct SynTypeAuto: SynBase
{
	SynTypeAuto(Lexeme *begin, Lexeme *end): SynBase(myTypeID, begin, end)
	{
	}

	static const unsigned myTypeID = SynNode::SynTypeAuto;
};

struct SynTypeGeneric: SynBase
{
	SynTypeGeneric(Lexeme *begin, Lexeme *end): SynBase(myTypeID, begin, end)
	{
	}

	static const unsigned myTypeID = SynNode::SynTypeGeneric;
};

struct SynTypeSimple: SynBase
{
	SynTypeSimple(Lexeme *begin, Lexeme *end, IntrusiveList<SynIdentifier> path, InplaceStr name): SynBase(myTypeID, begin, end), path(path), name(name)
	{
	}

	IntrusiveList<SynIdentifier> path;
	InplaceStr name;

	static const unsigned myTypeID = SynNode::SynTypeSimple;
};

struct SynTypeAlias: SynBase
{
	SynTypeAlias(Lexeme *begin, Lexeme *end, SynIdentifier *name): SynBase(myTypeID, begin, end), name(name)
	{
	}

	SynIdentifier *name;

	static const unsigned myTypeID = SynNode::SynTypeAlias;
};

struct SynTypeArray: SynBase
{
	SynTypeArray(Lexeme *begin, Lexeme *end, SynBase *type, IntrusiveList<SynBase> sizes): SynBase(myTypeID, begin, end), type(type), sizes(sizes)
	{
	}

	SynBase *type;
	IntrusiveList<SynBase> sizes;

	static const unsigned myTypeID = SynNode::SynTypeArray;
};

struct SynTypeReference: SynBase
{
	SynTypeReference(Lexeme *begin, Lexeme *end, SynBase *type): SynBase(myTypeID, begin, end), type(type)
	{
	}

	SynBase *type;

	static const unsigned myTypeID = SynNode::SynTypeReference;
};

struct SynTypeFunction: SynBase
{
	SynTypeFunction(Lexeme *begin, Lexeme *end, SynBase *returnType, IntrusiveList<SynBase> arguments): SynBase(myTypeID, begin, end), returnType(returnType), arguments(arguments)
	{
	}

	SynBase *returnType;
	IntrusiveList<SynBase> arguments;

	static const unsigned myTypeID = SynNode::SynTypeFunction;
};

struct SynTypeGenericInstance: SynBase
{
	SynTypeGenericInstance(Lexeme *begin, Lexeme *end, SynBase *baseType, IntrusiveList<SynBase> types): SynBase(myTypeID, begin, end), baseType(baseType), types(types)
	{
	}

	SynBase *baseType;
	IntrusiveList<SynBase> types;

	static const unsigned myTypeID = SynNode::SynTypeGenericInstance;
};

struct SynTypeof: SynBase
{
	SynTypeof(Lexeme *begin, Lexeme *end, SynBase* value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	SynBase *value;

	static const unsigned myTypeID = SynNode::SynTypeof;
};

struct SynBool: SynBase
{
	SynBool(Lexeme *begin, Lexeme *end, bool value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	bool value;

	static const unsigned myTypeID = SynNode::SynBool;
};

struct SynNumber: SynBase
{
	SynNumber(Lexeme *begin, Lexeme *end, InplaceStr value, InplaceStr suffix): SynBase(myTypeID, begin, end), value(value), suffix(suffix)
	{
	}

	InplaceStr value;
	InplaceStr suffix;

	static const unsigned myTypeID = SynNode::SynNumber;
};

struct SynNullptr: SynBase
{
	SynNullptr(Lexeme *begin, Lexeme *end): SynBase(myTypeID, begin, end)
	{
	}

	static const unsigned myTypeID = SynNode::SynNullptr;
};

struct SynCharacter: SynBase
{
	SynCharacter(Lexeme *begin, Lexeme *end, InplaceStr value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	InplaceStr value;

	static const unsigned myTypeID = SynNode::SynCharacter;
};

struct SynString: SynBase
{
	SynString(Lexeme *begin, Lexeme *end, bool rawLiteral, InplaceStr value): SynBase(myTypeID, begin, end), rawLiteral(rawLiteral), value(value)
	{
	}

	bool rawLiteral;
	InplaceStr value;

	static const unsigned myTypeID = SynNode::SynString;
};

struct SynArray: SynBase
{
	SynArray(Lexeme *begin, Lexeme *end, IntrusiveList<SynBase> values): SynBase(myTypeID, begin, end), values(values)
	{
	}

	IntrusiveList<SynBase> values;

	static const unsigned myTypeID = SynNode::SynArray;
};

struct SynGenerator: SynBase
{
	SynGenerator(Lexeme *begin, Lexeme *end, IntrusiveList<SynBase> expressions): SynBase(myTypeID, begin, end), expressions(expressions)
	{
	}

	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = SynNode::SynGenerator;
};

struct SynTemplate: SynBase
{
	SynTemplate(Lexeme *begin, Lexeme *end, IntrusiveList<SynIdentifier> typeAliases): SynBase(myTypeID, begin, end), typeAliases(typeAliases)
	{
	}

	IntrusiveList<SynIdentifier> typeAliases;

	static const unsigned myTypeID = SynNode::SynTemplate;
};

struct SynAlign: SynBase
{
	SynAlign(Lexeme *begin, Lexeme *end, SynBase* value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	SynBase* value;

	static const unsigned myTypeID = SynNode::SynAlign;
};

struct SynTypedef: SynBase
{
	SynTypedef(Lexeme *begin, Lexeme *end, SynBase *type, SynIdentifier *alias): SynBase(myTypeID, begin, end), type(type), alias(alias)
	{
	}

	SynBase *type;
	SynIdentifier *alias;

	static const unsigned myTypeID = SynNode::SynTypedef;
};

struct SynMemberAccess: SynBase
{
	SynMemberAccess(Lexeme *begin, Lexeme *end, SynBase* value, SynIdentifier* member): SynBase(myTypeID, begin, end), value(value), member(member)
	{
	}

	SynBase* value;
	SynIdentifier* member;

	static const unsigned myTypeID = SynNode::SynMemberAccess;
};

struct SynCallArgument: SynBase
{
	SynCallArgument(Lexeme *begin, Lexeme *end, SynIdentifier* name, SynBase* value): SynBase(myTypeID, begin, end), name(name), value(value)
	{
	}

	SynIdentifier* name;
	SynBase* value;

	static const unsigned myTypeID = SynNode::SynCallArgument;
};

struct SynArrayIndex: SynBase
{
	SynArrayIndex(Lexeme *begin, Lexeme *end, SynBase* value, IntrusiveList<SynCallArgument> arguments): SynBase(myTypeID, begin, end), value(value), arguments(arguments)
	{
	}

	SynBase* value;
	IntrusiveList<SynCallArgument> arguments;

	static const unsigned myTypeID = SynNode::SynArrayIndex;
};

struct SynFunctionCall: SynBase
{
	SynFunctionCall(Lexeme *begin, Lexeme *end, SynBase* value, IntrusiveList<SynBase> aliases, IntrusiveList<SynCallArgument> arguments): SynBase(myTypeID, begin, end), value(value), aliases(aliases), arguments(arguments)
	{
	}

	SynBase* value;
	IntrusiveList<SynBase> aliases;
	IntrusiveList<SynCallArgument> arguments;

	static const unsigned myTypeID = SynNode::SynFunctionCall;
};

struct SynPreModify: SynBase
{
	SynPreModify(Lexeme *begin, Lexeme *end, SynBase* value, bool isIncrement): SynBase(myTypeID, begin, end), value(value), isIncrement(isIncrement)
	{
	}

	SynBase* value;
	bool isIncrement;

	static const unsigned myTypeID = SynNode::SynPreModify;
};

struct SynPostModify: SynBase
{
	SynPostModify(Lexeme *begin, Lexeme *end, SynBase* value, bool isIncrement): SynBase(myTypeID, begin, end), value(value), isIncrement(isIncrement)
	{
	}

	SynBase* value;
	bool isIncrement;

	static const unsigned myTypeID = SynNode::SynPostModify;
};

struct SynGetAddress: SynBase
{
	SynGetAddress(Lexeme *begin, Lexeme *end, SynBase* value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	SynBase* value;

	static const unsigned myTypeID = SynNode::SynGetAddress;
};

struct SynDereference: SynBase
{
	SynDereference(Lexeme *begin, Lexeme *end, SynBase* value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	SynBase* value;

	static const unsigned myTypeID = SynNode::SynDereference;
};

struct SynSizeof: SynBase
{
	SynSizeof(Lexeme *begin, Lexeme *end, SynBase* value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	SynBase* value;

	static const unsigned myTypeID = SynNode::SynSizeof;
};

struct SynNew: SynBase
{
	SynNew(Lexeme *begin, Lexeme *end, SynBase *type, IntrusiveList<SynCallArgument> arguments, SynBase *count, IntrusiveList<SynBase> constructor): SynBase(myTypeID, begin, end), type(type), arguments(arguments), count(count), constructor(constructor)
	{
	}

	SynBase *type;
	IntrusiveList<SynCallArgument> arguments;
	SynBase *count;
	IntrusiveList<SynBase> constructor;

	static const unsigned myTypeID = SynNode::SynNew;
};

struct SynConditional: SynBase
{
	SynConditional(Lexeme *begin, Lexeme *end, SynBase* condition, SynBase* trueBlock, SynBase* falseBlock): SynBase(myTypeID, begin, end), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	SynBase* condition;
	SynBase* trueBlock;
	SynBase* falseBlock;

	static const unsigned myTypeID = SynNode::SynConditional;
};

struct SynReturn: SynBase
{
	SynReturn(Lexeme *begin, Lexeme *end, SynBase* value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	SynBase *value;

	static const unsigned myTypeID = SynNode::SynReturn;
};

struct SynYield: SynBase
{
	SynYield(Lexeme *begin, Lexeme *end, SynBase* value): SynBase(myTypeID, begin, end), value(value)
	{
	}

	SynBase *value;

	static const unsigned myTypeID = SynNode::SynYield;
};

struct SynBreak: SynBase
{
	SynBreak(Lexeme *begin, Lexeme *end, SynNumber* number): SynBase(myTypeID, begin, end), number(number)
	{
	}

	SynNumber* number;

	static const unsigned myTypeID = SynNode::SynBreak;
};

struct SynContinue: SynBase
{
	SynContinue(Lexeme *begin, Lexeme *end, SynNumber* number): SynBase(myTypeID, begin, end), number(number)
	{
	}

	SynNumber* number;

	static const unsigned myTypeID = SynNode::SynContinue;
};

struct SynLabel: SynBase
{
	SynLabel(Lexeme *begin, Lexeme *end, SynIdentifier *labeIdentifier): SynBase(myTypeID, begin, end), labeIdentifier(labeIdentifier)
	{
	}

	SynIdentifier *labeIdentifier;

	static const unsigned myTypeID = SynNode::SynLabel;
};

struct SynGoto: SynBase
{
	SynGoto(Lexeme *begin, Lexeme *end, SynIdentifier *labeIdentifier): SynBase(myTypeID, begin, end), labeIdentifier(labeIdentifier)
	{
	}

	SynIdentifier *labeIdentifier;

	static const unsigned myTypeID = SynNode::SynGoto;
};

struct SynBlock: SynBase
{
	SynBlock(Lexeme *begin, Lexeme *end, IntrusiveList<SynBase> expressions): SynBase(myTypeID, begin, end), expressions(expressions)
	{
	}

	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = SynNode::SynBlock;
};

struct SynIfElse: SynBase
{
	SynIfElse(Lexeme *begin, Lexeme *end, bool staticIf, SynBase* condition, SynBase* trueBlock, SynBase* falseBlock): SynBase(myTypeID, begin, end), staticIf(staticIf), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	bool staticIf;
	SynBase* condition;
	SynBase* trueBlock;
	SynBase* falseBlock;

	static const unsigned myTypeID = SynNode::SynIfElse;
};

struct SynFor: SynBase
{
	SynFor(Lexeme *begin, Lexeme *end, SynBase* initializer, SynBase* condition, SynBase* increment, SynBase* body): SynBase(myTypeID, begin, end), initializer(initializer), condition(condition), increment(increment), body(body)
	{
	}

	SynBase* initializer;
	SynBase* condition;
	SynBase* increment;
	SynBase* body;

	static const unsigned myTypeID = SynNode::SynFor;
};

struct SynForEachIterator: SynBase
{
	SynForEachIterator(Lexeme *begin, Lexeme *end, SynBase* type, SynIdentifier* name, SynBase* value): SynBase(myTypeID, begin, end), type(type), name(name), value(value)
	{
	}

	SynBase* type;
	SynIdentifier* name;
	SynBase* value;

	static const unsigned myTypeID = SynNode::SynForEachIterator;
};

struct SynForEach: SynBase
{
	SynForEach(Lexeme *begin, Lexeme *end, IntrusiveList<SynForEachIterator> iterators, SynBase* body): SynBase(myTypeID, begin, end), iterators(iterators), body(body)
	{
	}

	IntrusiveList<SynForEachIterator> iterators;
	SynBase* body;

	static const unsigned myTypeID = SynNode::SynForEach;
};

struct SynWhile: SynBase
{
	SynWhile(Lexeme *begin, Lexeme *end, SynBase* condition, SynBase* body): SynBase(myTypeID, begin, end), condition(condition), body(body)
	{
	}

	SynBase* condition;
	SynBase* body;

	static const unsigned myTypeID = SynNode::SynWhile;
};

struct SynDoWhile: SynBase
{
	SynDoWhile(Lexeme *begin, Lexeme *end, IntrusiveList<SynBase> expressions, SynBase* condition): SynBase(myTypeID, begin, end), expressions(expressions), condition(condition)
	{
	}

	IntrusiveList<SynBase> expressions;
	SynBase* condition;

	static const unsigned myTypeID = SynNode::SynDoWhile;
};

struct SynSwitchCase: SynBase
{
	SynSwitchCase(Lexeme *begin, Lexeme *end, SynBase* value, IntrusiveList<SynBase> expressions): SynBase(myTypeID, begin, end), value(value), expressions(expressions)
	{
	}

	SynBase* value;
	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = SynNode::SynSwitchCase;
};

struct SynSwitch: SynBase
{
	SynSwitch(Lexeme *begin, Lexeme *end, SynBase* condition, IntrusiveList<SynSwitchCase> cases): SynBase(myTypeID, begin, end), condition(condition), cases(cases)
	{
	}

	SynBase* condition;
	IntrusiveList<SynSwitchCase> cases;

	static const unsigned myTypeID = SynNode::SynSwitch;
};

struct SynUnaryOp: SynBase
{
	SynUnaryOp(Lexeme *begin, Lexeme *end, SynUnaryOpType type, SynBase* value): SynBase(myTypeID, begin, end), type(type), value(value)
	{
	}

	SynUnaryOpType type;
	SynBase* value;

	static const unsigned myTypeID = SynNode::SynUnaryOp;
};

struct SynBinaryOp: SynBase
{
	SynBinaryOp(Lexeme *begin, Lexeme *end, SynBinaryOpType type, SynBase* lhs, SynBase* rhs): SynBase(myTypeID, begin, end), type(type), lhs(lhs), rhs(rhs)
	{
	}

	SynBinaryOpType type;
	SynBase* lhs;
	SynBase* rhs;

	static const unsigned myTypeID = SynNode::SynBinaryOp;
};

struct SynAssignment: SynBase
{
	SynAssignment(Lexeme *begin, Lexeme *end, SynBase* lhs, SynBase* rhs): SynBase(myTypeID, begin, end), lhs(lhs), rhs(rhs)
	{
	}

	SynBase* lhs;
	SynBase* rhs;

	static const unsigned myTypeID = SynNode::SynAssignment;
};

struct SynModifyAssignment: SynBase
{
	SynModifyAssignment(Lexeme *begin, Lexeme *end, SynModifyAssignType type, SynBase* lhs, SynBase* rhs): SynBase(myTypeID, begin, end), type(type), lhs(lhs), rhs(rhs)
	{
	}

	SynModifyAssignType type;
	SynBase* lhs;
	SynBase* rhs;

	static const unsigned myTypeID = SynNode::SynModifyAssignment;
};

struct SynVariableDefinition: SynBase
{
	SynVariableDefinition(Lexeme *begin, Lexeme *end, SynIdentifier *name, SynBase *initializer): SynBase(myTypeID, begin, end), name(name), initializer(initializer)
	{
	}

	SynIdentifier *name;
	SynBase *initializer;

	static const unsigned myTypeID = SynNode::SynVariableDefinition;
};

struct SynVariableDefinitions: SynBase
{
	SynVariableDefinitions(Lexeme *begin, Lexeme *end, SynAlign* align, SynBase *type, IntrusiveList<SynVariableDefinition> definitions): SynBase(myTypeID, begin, end), align(align), type(type), definitions(definitions)
	{
	}

	SynAlign* align;
	SynBase *type;
	IntrusiveList<SynVariableDefinition> definitions;

	static const unsigned myTypeID = SynNode::SynVariableDefinitions;
};

struct SynAccessor: SynBase
{
	SynAccessor(Lexeme *begin, Lexeme *end, SynBase *type, SynIdentifier *name, SynBase *getBlock, SynBase *setBlock, SynIdentifier *setName): SynBase(myTypeID, begin, end), type(type), name(name), getBlock(getBlock), setBlock(setBlock), setName(setName)
	{
	}

	SynBase *type;
	SynIdentifier *name;
	SynBase *getBlock;
	SynBase *setBlock;
	SynIdentifier *setName;

	static const unsigned myTypeID = SynNode::SynAccessor;
};

struct SynFunctionArgument: SynBase
{
	SynFunctionArgument(Lexeme *begin, Lexeme *end, bool isExplicit, SynBase* type, SynIdentifier* name, SynBase* initializer): SynBase(myTypeID, begin, end), isExplicit(isExplicit), type(type), name(name), initializer(initializer)
	{
		assert(name);
	}

	bool isExplicit;
	SynBase* type;
	SynIdentifier* name;
	SynBase* initializer;

	static const unsigned myTypeID = SynNode::SynFunctionArgument;
};

struct SynFunctionDefinition: SynBase
{
	SynFunctionDefinition(Lexeme *begin, Lexeme *end, bool prototype, bool coroutine, SynBase *parentType, bool accessor, SynBase *returnType, bool isOperator, SynIdentifier *name, IntrusiveList<SynIdentifier> aliases, IntrusiveList<SynFunctionArgument> arguments, IntrusiveList<SynBase> expressions): SynBase(myTypeID, begin, end), prototype(prototype), coroutine(coroutine), parentType(parentType), accessor(accessor), returnType(returnType), isOperator(isOperator), name(name), aliases(aliases), arguments(arguments), expressions(expressions)
	{
	}

	bool prototype;
	bool coroutine;
	SynBase *parentType;
	bool accessor;
	SynBase *returnType;
	bool isOperator;
	SynIdentifier *name;
	IntrusiveList<SynIdentifier> aliases;
	IntrusiveList<SynFunctionArgument> arguments;
	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = SynNode::SynFunctionDefinition;
};

struct SynShortFunctionArgument: SynBase
{
	SynShortFunctionArgument(Lexeme *begin, Lexeme *end, SynBase* type, SynIdentifier* name): SynBase(myTypeID, begin, end), type(type), name(name)
	{
	}

	SynBase* type;
	SynIdentifier* name;

	static const unsigned myTypeID = SynNode::SynShortFunctionArgument;
};

struct SynShortFunctionDefinition: SynBase
{
	SynShortFunctionDefinition(Lexeme *begin, Lexeme *end, IntrusiveList<SynShortFunctionArgument> arguments, IntrusiveList<SynBase> expressions): SynBase(myTypeID, begin, end), arguments(arguments), expressions(expressions)
	{
	}

	IntrusiveList<SynShortFunctionArgument> arguments;
	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = SynNode::SynShortFunctionDefinition;
};

struct SynConstant: SynBase
{
	SynConstant(Lexeme *begin, Lexeme *end, SynIdentifier *name, SynBase *value): SynBase(myTypeID, begin, end), name(name), value(value)
	{
	}

	SynIdentifier *name;
	SynBase *value;

	static const unsigned myTypeID = SynNode::SynConstant;
};

struct SynConstantSet: SynBase
{
	SynConstantSet(Lexeme *begin, Lexeme *end, SynBase *type, IntrusiveList<SynConstant> constants): SynBase(myTypeID, begin, end), type(type), constants(constants)
	{
	}

	SynBase *type;
	IntrusiveList<SynConstant> constants;

	static const unsigned myTypeID = SynNode::SynConstantSet;
};

struct SynClassPrototype: SynBase
{
	SynClassPrototype(Lexeme *begin, Lexeme *end, SynIdentifier *name): SynBase(myTypeID, begin, end), name(name)
	{
	}

	SynIdentifier *name;

	static const unsigned myTypeID = SynNode::SynClassPrototype;
};

struct SynClassElements;

struct SynClassStaticIf: SynBase
{
	SynClassStaticIf(Lexeme *begin, Lexeme *end, SynBase *condition, SynClassElements *trueBlock, SynClassElements *falseBlock): SynBase(myTypeID, begin, end), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock)
	{
	}

	SynBase *condition;
	SynClassElements *trueBlock;
	SynClassElements *falseBlock;

	static const unsigned myTypeID = SynNode::SynClassStaticIf;
};

struct SynClassElements: SynBase
{
	SynClassElements(Lexeme *begin, Lexeme *end, IntrusiveList<SynTypedef> typedefs, IntrusiveList<SynFunctionDefinition> functions, IntrusiveList<SynAccessor> accessors, IntrusiveList<SynVariableDefinitions> members, IntrusiveList<SynConstantSet> constantSets, IntrusiveList<SynClassStaticIf> staticIfs): SynBase(myTypeID, begin, end), typedefs(typedefs), functions(functions), accessors(accessors), members(members), constantSets(constantSets), staticIfs(staticIfs)
	{
	}

	IntrusiveList<SynTypedef> typedefs;
	IntrusiveList<SynFunctionDefinition> functions;
	IntrusiveList<SynAccessor> accessors;
	IntrusiveList<SynVariableDefinitions> members;
	IntrusiveList<SynConstantSet> constantSets;
	IntrusiveList<SynClassStaticIf> staticIfs;

	static const unsigned myTypeID = SynNode::SynClassElements;
};

struct SynClassDefinition: SynBase
{
	SynClassDefinition(Lexeme *begin, Lexeme *end, SynAlign* align, SynIdentifier* name, IntrusiveList<SynIdentifier> aliases, bool extendable, bool isStruct, SynBase *baseClass, SynClassElements *elements): SynBase(myTypeID, begin, end), align(align), name(name), aliases(aliases), extendable(extendable), isStruct(isStruct), baseClass(baseClass), elements(elements)
	{
		imported = false;
	}

	bool imported;
	SynAlign* align;
	SynIdentifier* name;
	IntrusiveList<SynIdentifier> aliases;
	bool extendable;
	bool isStruct;
	SynBase *baseClass;
	SynClassElements *elements;

	static const unsigned myTypeID = SynNode::SynClassDefinition;
};

struct SynEnumDefinition: SynBase
{
	SynEnumDefinition(Lexeme *begin, Lexeme *end, SynIdentifier* name, IntrusiveList<SynConstant> values): SynBase(myTypeID, begin, end), name(name), values(values)
	{
	}

	SynIdentifier* name;
	IntrusiveList<SynConstant> values;

	static const unsigned myTypeID = SynNode::SynEnumDefinition;
};

struct SynNamespaceElement
{
	SynNamespaceElement(SynNamespaceElement *parent, SynIdentifier* name): parent(parent), name(name)
	{
		if(parent)
			fullNameHash = NULLC::StringHashContinue(NULLC::StringHashContinue(parent->fullNameHash, "."), name->name.begin, name->name.end);
		else
			fullNameHash = name->name.hash();
	}

	SynNamespaceElement *parent;
	SynIdentifier* name;
	unsigned fullNameHash;
};

struct SynNamespaceDefinition: SynBase
{
	SynNamespaceDefinition(Lexeme *begin, Lexeme *end, IntrusiveList<SynIdentifier> path, IntrusiveList<SynBase> expressions): SynBase(myTypeID, begin, end), path(path), expressions(expressions)
	{
	}

	IntrusiveList<SynIdentifier> path;
	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = SynNode::SynNamespaceDefinition;
};

struct SynModuleImport: SynBase
{
	SynModuleImport(Lexeme *begin, Lexeme *end, IntrusiveList<SynIdentifier> path, ByteCode *bytecode): SynBase(myTypeID, begin, end), path(path), bytecode(bytecode)
	{
	}

	IntrusiveList<SynIdentifier> path;
	ByteCode *bytecode;

	static const unsigned myTypeID = SynNode::SynModuleImport;
};

struct SynModule: SynBase
{
	SynModule(Lexeme *begin, Lexeme *end, IntrusiveList<SynModuleImport> imports, IntrusiveList<SynBase> expressions): SynBase(myTypeID, begin, end), imports(imports), expressions(expressions)
	{
	}

	IntrusiveList<SynModuleImport> imports;

	IntrusiveList<SynBase> expressions;

	static const unsigned myTypeID = SynNode::SynModule;
};

void ImportModuleNamespaces(ParseContext &ctx, Lexeme *pos, ByteCode *bCode);

SynFunctionDefinition* ParseFunctionDefinition(ParseContext &ctx);
SynBase* ParseClassDefinition(ParseContext &ctx);
SynModule* Parse(ParseContext &context, const char *code, const char *moduleRoot);

void VisitParseTreeNodes(SynBase *syntax, void *context, void(*accept)(void *context, SynBase *child));
const char* GetParseTreeNodeName(SynBase *syntax);

const char* GetOpName(SynUnaryOpType type);
const char* GetOpName(SynBinaryOpType type);
const char* GetOpName(SynModifyAssignType type);

InplaceStr GetModuleName(Allocator *allocator, const char *moduleRoot, IntrusiveList<SynIdentifier> parts);
