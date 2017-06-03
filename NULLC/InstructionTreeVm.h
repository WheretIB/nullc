#pragma once

#include "stdafx.h"
#include "IntrusiveList.h"

struct TypeBase;
struct ExprBase;
struct FunctionData;
struct ScopeData;

struct ExpressionContext;

struct VmValue;
struct VmConstant;
struct VmInstruction;
struct VmBlock;
struct VmFunction;
struct VmModule;

enum VmValueType
{
	VM_TYPE_VOID,

	VM_TYPE_INT,
	VM_TYPE_DOUBLE,
	VM_TYPE_LONG,

	VM_TYPE_LABEL,

	VM_TYPE_POINTER,
	VM_TYPE_FUNCTION_REF,
	VM_TYPE_ARRAY_REF,
	VM_TYPE_AUTO_REF,
	VM_TYPE_AUTO_ARRAY,

	VM_TYPE_STRUCT,
};

enum VmInstructionType
{
	VM_INST_LOAD_BYTE,
	VM_INST_LOAD_SHORT,
	VM_INST_LOAD_INT,
	VM_INST_LOAD_FLOAT,
	VM_INST_LOAD_DOUBLE,
	VM_INST_LOAD_LONG,
	VM_INST_LOAD_STRUCT,

	VM_INST_STORE_BYTE,
	VM_INST_STORE_SHORT,
	VM_INST_STORE_INT,
	VM_INST_STORE_FLOAT,
	VM_INST_STORE_DOUBLE,
	VM_INST_STORE_LONG,
	VM_INST_STORE_STRUCT,

	VM_INST_DOUBLE_TO_INT,
	VM_INST_DOUBLE_TO_LONG,
	VM_INST_DOUBLE_TO_FLOAT,
	VM_INST_INT_TO_DOUBLE,
	VM_INST_LONG_TO_DOUBLE,
	VM_INST_INT_TO_LONG,
	VM_INST_LONG_TO_INT,

	VM_INST_INDEX, // pointer, array_size, element_size, index
	VM_INST_INDEX_UNSIZED,

	VM_INST_FUNCTION_ADDRESS,
	VM_INST_TYPE_ID,

	VM_INST_SET_RANGE,

	VM_INST_JUMP,
	VM_INST_JUMP_Z,
	VM_INST_JUMP_NZ,

	VM_INST_CALL,

	VM_INST_RETURN,
	VM_INST_YIELD,

	VM_INST_ADD,
	VM_INST_SUB,
	VM_INST_MUL,
	VM_INST_DIV,
	VM_INST_POW,
	VM_INST_MOD,
	VM_INST_LESS,
	VM_INST_GREATER,
	VM_INST_LESS_EQUAL,
	VM_INST_GREATER_EQUAL,
	VM_INST_EQUAL,
	VM_INST_NOT_EQUAL,
	VM_INST_SHL,
	VM_INST_SHR,
	VM_INST_BIT_AND,
	VM_INST_BIT_OR,
	VM_INST_BIT_XOR,
	VM_INST_LOG_AND,
	VM_INST_LOG_OR,
	VM_INST_LOG_XOR,

	VM_INST_NEG,
	VM_INST_BIT_NOT,
	VM_INST_LOG_NOT,

	VM_INST_CREATE_CLOSURE,
	VM_INST_CLOSE_UPVALUES,
	VM_INST_CONVERT_POINTER,
	VM_INST_CHECKED_RETURN,

	VM_INST_CONSTRUCT, // Pseudo instruction to collect multiple elements into a single value
};

enum VmOptimizationType
{
	VM_OPT_PEEPHOLE,
	VM_OPT_CONSTANT_PROPAGATION,
	VM_OPT_DEAD_CODE_ELIMINATION,
	VM_OPT_CONTROL_FLOW_SIPLIFICATION,
	VM_OPT_LOAD_STORE_PROPAGATION,
	VM_OPT_COMMON_SUBEXPRESSION_ELIMINATION,
};

struct VmType
{
	VmType(VmValueType type, unsigned size): type(type), size(size)
	{
	}

	bool operator==(const VmType& rhs) const
	{
		return type == rhs.type && size == rhs.size;
	}
	bool operator!=(const VmType& rhs) const
	{
		return !(*this == rhs);
	}

	VmValueType type;
	unsigned size;

	static const VmType Void;
	static const VmType Int;
	static const VmType Double;
	static const VmType Long;
	static const VmType Label;
	static const VmType Pointer;
	static const VmType FunctionRef;
	static const VmType ArrayRef;
	static const VmType AutoRef;
	static const VmType AutoArray;

	static const VmType Struct(long long size)
	{
		assert(unsigned(size) == size);

		return VmType(VM_TYPE_STRUCT, unsigned(size));
	}
};

struct VmValue
{
	VmValue(unsigned typeID, VmType type): typeID(typeID), type(type)
	{
		hasSideEffects = false;
		hasMemoryAccess = false;
	}

	virtual ~VmValue()
	{
	}

	void AddUse(VmValue* user);
	void RemoveUse(VmValue* user);

	unsigned typeID;

	VmType type;

	InplaceStr comment;

	bool hasSideEffects;
	bool hasMemoryAccess;

	SmallArray<VmValue*, 8> users;
};

struct VmVoid: VmValue
{
	VmVoid(): VmValue(myTypeID, VmType::Void)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct VmConstant: VmValue
{
	VmConstant(VmType type): VmValue(myTypeID, type)
	{
		iValue = 0;
		dValue = 0.0;
		lValue = 0ll;
		sValue = NULL;

		isFrameOffset = false;
	}

	bool operator==(const VmConstant& rhs) const
	{
		return type == rhs.type && iValue == rhs.iValue && dValue == rhs.dValue && lValue == rhs.lValue && sValue == rhs.sValue && isFrameOffset == rhs.isFrameOffset;
	}

	int iValue;
	double dValue;
	long long lValue;
	char *sValue;

	bool isFrameOffset;

	static const unsigned myTypeID = __LINE__;
};

struct VmInstruction: VmValue
{
	VmInstruction(VmType type, VmInstructionType cmd, unsigned uniqueId): VmValue(myTypeID, type), cmd(cmd), uniqueId(uniqueId)
	{
		parent = NULL;

		prevSibling = NULL;
		nextSibling = NULL;
	}

	void AddArgument(VmValue *argument);

	VmInstructionType cmd;

	unsigned uniqueId;

	SmallArray<VmValue*, 8> arguments;

	VmBlock *parent;

	VmInstruction *prevSibling;
	VmInstruction *nextSibling;

	static const unsigned myTypeID = __LINE__;
};

struct VmBlock: VmValue
{
	VmBlock(InplaceStr name, unsigned uniqueId): VmValue(myTypeID, VmType::Label), name(name), uniqueId(uniqueId)
	{
		parent = NULL;

		prevSibling = NULL;
		nextSibling = NULL;

		firstInstruction = NULL;
		lastInstruction = NULL;
	}

	void AddInstruction(VmInstruction* instruction);
	void RemoveInstruction(VmInstruction* instruction);

	InplaceStr name;

	unsigned uniqueId;

	VmFunction *parent;

	VmBlock *prevSibling;
	VmBlock *nextSibling;

	VmInstruction *firstInstruction;
	VmInstruction *lastInstruction;

	static const unsigned myTypeID = __LINE__;
};

struct VmFunction: VmValue
{
	VmFunction(VmType type, FunctionData *function, ScopeData *scope, VmType returnType): VmValue(myTypeID, type), function(function), scope(scope), returnType(returnType)
	{
		firstBlock = NULL;
		lastBlock = NULL;

		next = NULL;
	}

	void AddBlock(VmBlock *block);
	void RemoveBlock(VmBlock *block);

	FunctionData *function;
	ScopeData *scope;

	VmType returnType;

	VmBlock *firstBlock;
	VmBlock *lastBlock;

	VmFunction *next;

	static const unsigned myTypeID = __LINE__;
};

struct VmModule
{
	VmModule()
	{
		skipFunctionDefinitions = false;

		currentFunction = NULL;
		currentBlock = NULL;

		nextBlockId = 1;
		nextInstructionId = 1;

		peepholeOptimizations = 0;
		constantPropagations = 0;
		deadCodeEliminations = 0;
		controlFlowSimplifications = 0;
		loadStorePropagations = 0;
		commonSubexprEliminations = 0;
	}

	IntrusiveList<VmFunction> functions;

	bool skipFunctionDefinitions;

	VmFunction *currentFunction;
	VmBlock *currentBlock;

	unsigned nextBlockId;
	unsigned nextInstructionId;

	unsigned peepholeOptimizations;
	unsigned constantPropagations;
	unsigned deadCodeEliminations;
	unsigned controlFlowSimplifications;
	unsigned loadStorePropagations;
	unsigned commonSubexprEliminations;

	struct LoadStoreInfo
	{
		LoadStoreInfo()
		{
			loadInst = 0;
			storeInst = 0;

			address = 0;
		}

		VmInstruction *loadInst;
		VmInstruction *storeInst;

		VmConstant *address;
	};

	FastVector<LoadStoreInfo> loadStoreInfo;
};

template<typename T>
bool isType(VmValue *node)
{
	return node->typeID == typename T::myTypeID;
}

template<typename T>
T* getType(VmValue *node)
{
	if(node && isType<T>(node))
		return static_cast<T*>(node);

	return 0;
}

VmType GetVmType(ExpressionContext &ctx, TypeBase *type);

VmValue* CompileVm(ExpressionContext &ctx, VmModule *module, ExprBase *expression);
VmModule* CompileVm(ExpressionContext &ctx, ExprBase *expression);

void RunOptimizationPass(VmModule *module, VmOptimizationType type);
