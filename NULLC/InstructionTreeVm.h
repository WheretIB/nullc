#pragma once

#include "stdafx.h"
#include "IntrusiveList.h"

struct SynBase;

struct TypeBase;
struct ExprBase;
struct VariableData;
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

	VM_TYPE_BLOCK,
	VM_TYPE_FUNCTION,

	VM_TYPE_POINTER,
	VM_TYPE_FUNCTION_REF,
	VM_TYPE_ARRAY_REF,
	VM_TYPE_AUTO_REF,
	VM_TYPE_AUTO_ARRAY,

	VM_TYPE_STRUCT
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

	VM_INST_LOAD_IMMEDIATE,

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
	VM_INST_MEM_COPY,

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

	VM_INST_ADD_LOAD,
	VM_INST_SUB_LOAD,
	VM_INST_MUL_LOAD,
	VM_INST_DIV_LOAD,
	VM_INST_POW_LOAD,
	VM_INST_MOD_LOAD,
	VM_INST_LESS_LOAD,
	VM_INST_GREATER_LOAD,
	VM_INST_LESS_EQUAL_LOAD,
	VM_INST_GREATER_EQUAL_LOAD,
	VM_INST_EQUAL_LOAD,
	VM_INST_NOT_EQUAL_LOAD,
	VM_INST_SHL_LOAD,
	VM_INST_SHR_LOAD,
	VM_INST_BIT_AND_LOAD,
	VM_INST_BIT_OR_LOAD,
	VM_INST_BIT_XOR_LOAD,

	VM_INST_NEG,
	VM_INST_BIT_NOT,
	VM_INST_LOG_NOT,

	VM_INST_CONVERT_POINTER,

	VM_INST_ABORT_NO_RETURN,

	VM_INST_CONSTRUCT, // Pseudo instruction to collect multiple elements into a single value
	VM_INST_ARRAY, // Pseudo instruction to collect multiple elements into a single array
	VM_INST_EXTRACT, // Pseudo instruction to extract an element from a composite value
	VM_INST_UNYIELD, // Pseudo instruction to restore function execution state
	VM_INST_PHI, // Pseudo instruction to create a value based on control flow
	VM_INST_BITCAST, // Pseudo instruction to transform value type
	VM_INST_MOV, // Pseudo instruction to create a separate value copy
};

enum VmPassType
{
	VM_PASS_OPT_PEEPHOLE,
	VM_PASS_OPT_CONSTANT_PROPAGATION,
	VM_PASS_OPT_DEAD_CODE_ELIMINATION,
	VM_PASS_OPT_CONTROL_FLOW_SIPLIFICATION,
	VM_PASS_OPT_LOAD_STORE_PROPAGATION,
	VM_PASS_OPT_COMMON_SUBEXPRESSION_ELIMINATION,
	VM_PASS_OPT_DEAD_ALLOCA_STORE_ELIMINATION,
	VM_PASS_OPT_MEMORY_TO_REGISTER,
	VM_PASS_OPT_ARRAY_TO_ELEMENTS,
	VM_PASS_OPT_LATE_PEEPHOLE,

	VM_PASS_UPDATE_LIVE_SETS,
	VM_PASS_PREPARE_SSA_EXIT,

	VM_PASS_CREATE_ALLOCA_STORAGE,

	VM_PASS_LEGALIZE_ARRAY_VALUES,
	VM_PASS_LEGALIZE_BITCASTS,
	VM_PASS_LEGALIZE_EXTRACTS,
};

struct VmType
{
	VmType(VmValueType type, unsigned size, TypeBase *structType): type(type), size(size), structType(structType)
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

	TypeBase *structType;

	static const VmType Void;
	static const VmType Int;
	static const VmType Double;
	static const VmType Long;
	static const VmType Block;
	static const VmType Function;
	static const VmType AutoRef;
	static const VmType AutoArray;

	static const VmType Pointer(TypeBase *structType)
	{
		assert(structType);

		return VmType(VM_TYPE_POINTER, NULLC_PTR_SIZE, structType);
	}

	static const VmType FunctionRef(TypeBase *structType)
	{
		assert(structType);

		return VmType(VM_TYPE_FUNCTION_REF, NULLC_PTR_SIZE + 4, structType);
	}

	static const VmType ArrayRef(TypeBase *structType)
	{
		assert(structType);

		return VmType(VM_TYPE_ARRAY_REF, NULLC_PTR_SIZE + 4, structType);
	}

	static const VmType Struct(long long size, TypeBase *structType)
	{
		assert(unsigned(size) == size);
		assert(structType);

		return VmType(VM_TYPE_STRUCT, unsigned(size), structType);
	}
};

struct VmValue
{
	VmValue(unsigned typeID, Allocator *allocator, VmType type, SynBase *source): typeID(typeID), type(type), source(source), users(allocator)
	{
		hasSideEffects = false;
		hasMemoryAccess = false;

		hasKnownSimpleUse = false;
		hasKnownNonSimpleUse = false;

		canBeRemoved = true;
	}

	virtual ~VmValue()
	{
	}

	void AddUse(VmValue* user);
	void RemoveUse(VmValue* user);

	unsigned typeID;

	VmType type;

	SynBase *source;

	InplaceStr comment;

	bool hasSideEffects;
	bool hasMemoryAccess;

	bool canBeRemoved;

	bool hasKnownSimpleUse;
	bool hasKnownNonSimpleUse;

	SmallArray<VmValue*, 8> users;
};

struct VmVoid: VmValue
{
	VmVoid(Allocator *allocator): VmValue(myTypeID, allocator, VmType::Void, NULL)
	{
	}

	static const unsigned myTypeID = __LINE__;
};

struct VmConstant: VmValue
{
	VmConstant(Allocator *allocator, VmType type, SynBase *source): VmValue(myTypeID, allocator, type, source)
	{
		iValue = 0;
		dValue = 0.0;
		lValue = 0ll;
		sValue = NULL;
		bValue = NULL;
		fValue = NULL;

		container = NULL;

		isFloat = false;
		isReference = false;
	}

	bool operator==(const VmConstant& rhs) const
	{
		return type == rhs.type && iValue == rhs.iValue && dValue == rhs.dValue && lValue == rhs.lValue && sValue == rhs.sValue && bValue == rhs.bValue && fValue == rhs.fValue && container == rhs.container;
	}

	int iValue;
	double dValue;
	long long lValue;
	const char *sValue;
	VmBlock *bValue;
	VmFunction *fValue;

	VariableData *container;

	bool isFloat;
	bool isReference;

	static const unsigned myTypeID = __LINE__;
};

struct VmInstruction: VmValue
{
	VmInstruction(Allocator *allocator, VmType type, SynBase *source, VmInstructionType cmd, unsigned uniqueId): VmValue(myTypeID, allocator, type, source), cmd(cmd), uniqueId(uniqueId), arguments(allocator), regVmRegisters(allocator)
	{
		parent = NULL;

		prevSibling = NULL;
		nextSibling = NULL;

		regVmAllocated = false;

		regVmCompletedUsers = 0;

		regVmSearchMarker = 0;

		color = 0;
		marker = 0;

		idom = NULL;
		intersectingIdom = NULL;
	}

	void AddArgument(VmValue *argument);

	VmInstructionType cmd;

	unsigned uniqueId;

	SmallArray<VmValue*, 4> arguments;

	VmBlock *parent;

	VmInstruction *prevSibling;
	VmInstruction *nextSibling;

	bool regVmAllocated;
	SmallArray<unsigned char, 8> regVmRegisters;

	unsigned regVmCompletedUsers;

	unsigned regVmSearchMarker;

	unsigned color;
	unsigned marker;

	VmInstruction *idom;
	VmInstruction *intersectingIdom;

	static const unsigned myTypeID = __LINE__;
};

struct VmBlock: VmValue
{
	VmBlock(Allocator *allocator, SynBase *source, InplaceStr name, unsigned uniqueId): VmValue(myTypeID, allocator, VmType::Block, source), name(name), uniqueId(uniqueId), predecessors(allocator), successors(allocator), dominanceFrontier(allocator), dominanceChildren(allocator), liveIn(allocator), liveOut(allocator)
	{
		parent = NULL;

		prevSibling = NULL;
		nextSibling = NULL;

		firstInstruction = NULL;
		lastInstruction = NULL;

		insertPoint = NULL;

		address = ~0u;

		visited = false;

		controlGraphPreOrderId = ~0u;
		controlGraphPostOrderId = ~0u;

		dominanceGraphPreOrderId = ~0u;
		dominanceGraphPostOrderId = ~0u;

		idom = NULL;

		hasAssignmentForId = 0;
		hasPhiNodeForId = 0;
	}

	void AddInstruction(VmInstruction* instruction);
	void DetachInstruction(VmInstruction* instruction);
	void RemoveInstruction(VmInstruction* instruction);

	InplaceStr name;

	unsigned uniqueId;

	VmFunction *parent;

	VmBlock *prevSibling;
	VmBlock *nextSibling;

	VmInstruction *firstInstruction;
	VmInstruction *lastInstruction;

	VmInstruction *insertPoint;

	unsigned address;

	// Dominator frontier creation
	SmallArray<VmBlock*, 4> predecessors;
	SmallArray<VmBlock*, 4> successors;

	bool visited;

	unsigned controlGraphPreOrderId;
	unsigned controlGraphPostOrderId;

	unsigned dominanceGraphPreOrderId;
	unsigned dominanceGraphPostOrderId;

	VmBlock *idom;

	SmallArray<VmBlock*, 4> dominanceFrontier;
	SmallArray<VmBlock*, 4> dominanceChildren;

	unsigned hasAssignmentForId;
	unsigned hasPhiNodeForId;

	SmallArray<VmInstruction*, 4> liveIn;
	SmallArray<VmInstruction*, 4> liveOut;

	static const unsigned myTypeID = __LINE__;
};

struct VmFunction: VmValue
{
	VmFunction(Allocator *allocator, VmType type, SynBase *source, FunctionData *function, ScopeData *scope, VmType returnType): VmValue(myTypeID, allocator, type, source), function(function), scope(scope), returnType(returnType), allocas(allocator), restoreBlocks(allocator)
	{
		firstBlock = NULL;
		lastBlock = NULL;

		nextBlockId = 1;
		nextInstructionId = 1;

		next = NULL;
		listed = false;

		vmAddress = ~0u;
		vmCodeSize = 0;

		regVmAddress = ~0u;
		regVmCodeSize = 0;
		regVmRegisters = 0;

		nextRestoreBlock = 0;

		nextColor = 0;
		nextSearchMarker = 1;
	}

	void AddBlock(VmBlock *block);
	void DetachBlock(VmBlock *block);
	void RemoveBlock(VmBlock *block);

	void MoveEntryBlockToStart();

	void UpdateDominatorTree(VmModule *module, bool clear);
	void UpdateLiveSets(VmModule *module);

	FunctionData *function;
	ScopeData *scope;

	VmType returnType;

	VmBlock *firstBlock;
	VmBlock *lastBlock;

	unsigned nextBlockId;
	unsigned nextInstructionId;

	VmFunction *next;
	bool listed;

	unsigned vmAddress;
	unsigned vmCodeSize;

	unsigned regVmAddress;
	unsigned regVmCodeSize;
	unsigned regVmRegisters;

	SmallArray<VariableData*, 4> allocas;

	unsigned nextRestoreBlock;
	SmallArray<VmBlock*, 4> restoreBlocks;

	unsigned nextColor;
	unsigned nextSearchMarker;

	static const unsigned myTypeID = __LINE__;
};

struct VmModule
{
	VmModule(Allocator *allocator, const char *code): code(code), loopInfo(allocator), loadStoreInfo(allocator), tempUsers(allocator), tempInstructions(allocator), allocator(allocator)
	{
		vmGlobalCodeStart = 0;
		regVmGlobalCodeStart = 0;

		skipFunctionDefinitions = false;

		currentFunction = NULL;
		currentBlock = NULL;

		peepholeOptimizations = 0;
		constantPropagations = 0;
		deadCodeEliminations = 0;
		controlFlowSimplifications = 0;
		loadStorePropagations = 0;
		commonSubexprEliminations = 0;
		deadAllocaStoreEliminations = 0;
	}

	const char *code;

	IntrusiveList<VmFunction> functions;

	unsigned vmGlobalCodeStart;
	unsigned regVmGlobalCodeStart;

	bool skipFunctionDefinitions;

	VmFunction *currentFunction;
	VmBlock *currentBlock;

	struct LoopInfo
	{
		LoopInfo(): breakBlock(0), continueBlock(0)
		{
		}

		LoopInfo(VmBlock* breakBlock, VmBlock* continueBlock): breakBlock(breakBlock), continueBlock(continueBlock)
		{
		}

		VmBlock* breakBlock;
		VmBlock* continueBlock;
	};

	SmallArray<LoopInfo, 32> loopInfo;

	unsigned peepholeOptimizations;
	unsigned constantPropagations;
	unsigned deadCodeEliminations;
	unsigned controlFlowSimplifications;
	unsigned loadStorePropagations;
	unsigned commonSubexprEliminations;
	unsigned deadAllocaStoreEliminations;

	struct LoadStoreInfo
	{
		LoadStoreInfo()
		{
			loadInst = 0;
			storeInst = 0;
			copyInst = 0;

			accessSize = 0;

			loadAddress = 0;

			loadPointer = 0;
			loadOffset = 0;

			storeAddress = 0;

			storePointer = 0;
			storeOffset = 0;

			noLoadOrNoContainerAlias = false;
			noStoreOrNoContainerAlias = false;
		}

		VmInstruction *loadInst;
		VmInstruction *storeInst;
		VmInstruction *copyInst;

		unsigned accessSize;

		VmConstant *loadAddress;

		VmValue *loadPointer;
		VmConstant *loadOffset;

		VmConstant *storeAddress;

		VmValue *storePointer;
		VmConstant *storeOffset;

		bool noLoadOrNoContainerAlias;
		bool noStoreOrNoContainerAlias;
	};

	SmallArray<LoadStoreInfo, 32> loadStoreInfo;

	SmallArray<VmValue*, 128> tempUsers;

	SmallArray<VmInstruction*, 128> tempInstructions;

	// Memory pool
	Allocator *allocator;

	template<typename T>
	T* get()
	{
		return (T*)allocator->alloc(sizeof(T));
	}
};

template<typename T>
bool isType(VmValue *node)
{
	return node && node->typeID == T::myTypeID;
}

template<typename T>
T* getType(VmValue *node)
{
	if(node && node->typeID == T::myTypeID)
		return static_cast<T*>(node);

	return 0;
}

VmType GetVmType(ExpressionContext &ctx, TypeBase *type);
void FinalizeAlloca(ExpressionContext &ctx, VmModule *module, VariableData *variable);

VmValue* CompileVm(ExpressionContext &ctx, VmModule *module, ExprBase *expression);
VmModule* CompileVm(ExpressionContext &ctx, ExprBase *expression, const char *code);

void RunVmPass(ExpressionContext &ctx, VmModule *module, VmPassType type);
