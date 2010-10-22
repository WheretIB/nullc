#pragma once
#include "stdafx.h"
#include "InstructionSet.h"
#include "ParseClass.h"

void	ResetTreeGlobals();

TypeInfo*	ChooseBinaryOpResultType(TypeInfo* a, TypeInfo* b);

extern const char* binCommandToText[];
extern const char* unaryCommandToText[];

#ifdef NULLC_ENABLE_C_TRANSLATION
void	GetCFunctionName(char* fName, unsigned int size, FunctionInfo *funcInfo);
void	OutputCFunctionName(FILE *fOut, FunctionInfo *funcInfo);
void	ResetTranslationState();
#define COMPILE_TRANSLATION(x) x
#else
#define COMPILE_TRANSLATION(x)
#endif

#ifdef NULLC_LLVM_SUPPORT
void		StartLLVMGeneration(unsigned functionsInModules);
const char*	GetLLVMIR(unsigned& length);
void		EndLLVMGeneration();
void		StartGlobalCode();
#define COMPILE_LLVM(x) x
#else
#define COMPILE_LLVM(x)
#endif

//////////////////////////////////////////////////////////////////////////
enum NodeType
{
	typeNodeZeroOp,
	typeNodeForExpr,
	typeNodeFuncCall,
	typeNodeFuncDef,
	typeNodeIfElseExpr,
	typeNodeOneOp,
	typeNodePopOp,
	typeNodeReturnOp,
	typeNodeThreeOp,
	typeNodeBinaryOp,
	typeNodeTwoOp,
	typeNodeWhileExpr,
	typeNodeDoWhileExpr,
	typeNodeBreakOp,
	typeNodeSwitchExpr,
	typeNodeNumber,
	typeNodeUnaryOp,
	typeNodeExpressionList,
	typeNodeArrayIndex,
	typeNodeDereference,
	typeNodeShiftAddress,
	typeNodeGetAddress,
	typeNodeVariableSet,
	typeNodePreOrPostOp,
	typeNodeFunctionAddress,
	typeNodeContinueOp,
	typeNodeVariableModify,
	typeNodeGetUpvalue,
	typeNodeBlockOp,
	typeNodeConvertPtr,
	typeNodeFunctionProxy,
};
//////////////////////////////////////////////////////////////////////////
class NodeNumber;

class NodeZeroOP
{
public:
	NodeZeroOP();
	explicit NodeZeroOP(TypeInfo* tinfo);
	virtual ~NodeZeroOP();

	// Code generation
	virtual void Compile();
	// Graph formatted printout to file
	virtual void LogToStream(FILE *fGraph);
	// Translation of NULLC code into C
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	// Evaluate node value
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	// Binding of code position to bytecode that node generates
	virtual void SetCodeInfo(const char* newSourcePos);
	// Add last node to the list as a new head
	virtual void AddExtraNode();
	// Compile extra nodes
	virtual void CompileExtra();
	// Log extra nodes
	virtual void LogToStreamExtra(FILE *fGraph);
	// Translation extra nodes
	COMPILE_TRANSLATION(virtual void TranslateToCExtra(FILE *fOut));
	// Compilation to LLVM IR code
	COMPILE_LLVM(virtual void CompileLLVM());
#ifdef NULLC_LLVM_SUPPORT
	virtual void CompileLLVMExtra();
#endif

	void*		operator new(size_t size)
	{
		return nodePool.Allocate((unsigned int)size);
	}
	void		operator delete(void *ptr, size_t size)
	{
		(void)ptr; (void)size;
		assert(!"Cannot delete NodeZeroOp");
	}

	static	ChunkedStackPool<65532>	nodePool;
	static void	DeleteNodes(){ nodePool.Clear(); }
	static void	ResetNodes(){ nodePool.~ChunkedStackPool(); }
protected:
	const char		*sourcePos;
public:
	TypeInfo		*typeInfo;
	NodeType		nodeType;
	NodeZeroOP		*prev, *next, *head;	// For organizing intrusive node lists
};

//////////////////////////////////////////////////////////////////////////
class NodeOneOP: public NodeZeroOP
{
public:
	NodeOneOP();
	virtual ~NodeOneOP();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());

	NodeZeroOP*	GetFirstNode()
	{
		return first;
	}
	void		SetFirstNode(NodeZeroOP* node)
	{
		first = node;
		if(first)
			typeInfo = first->typeInfo;
	}
protected:
	NodeZeroOP*	first;
};

class NodeTwoOP: public NodeOneOP
{
public:
	NodeTwoOP();
	virtual ~NodeTwoOP();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);

	NodeZeroOP*	GetSecondNode(){ return second; }
protected:
	NodeZeroOP*	second;
};

class NodeThreeOP: public NodeTwoOP
{
public:
	NodeThreeOP();
	virtual ~NodeThreeOP();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);

	NodeZeroOP*	GetTrirdNode(){ return third; }
protected:
	NodeZeroOP*	third;
};

//Zero child operators
class NodeNumber: public NodeZeroOP
{
public:
	NodeNumber(int number, TypeInfo* ptrType)
	{
		num.integer = number;
		typeInfo = ptrType;
		nodeType = typeNodeNumber;
	}
	NodeNumber(long long number, TypeInfo* ptrType)
	{
		num.integer64 = number;
		typeInfo = ptrType;
		nodeType = typeNodeNumber;
	}
	NodeNumber(double number, TypeInfo* ptrType)
	{
		num.real = number;
		typeInfo = ptrType;
		nodeType = typeNodeNumber;
	}
	virtual ~NodeNumber(){}

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());

	int			GetInteger()
	{
		if(typeInfo == typeLong)
			return (int)num.integer64;
		else if(typeInfo == typeDouble || typeInfo == typeFloat)
			return (int)num.real;
		return num.integer;
	}
	long long	GetLong()
	{
		if(typeInfo == typeLong)
			return num.integer64;
		else if(typeInfo == typeDouble || typeInfo == typeFloat)
			return (long long)num.real;
		return num.integer;
	}
	double		GetDouble()
	{
		if(typeInfo == typeDouble || typeInfo == typeFloat)
			return num.real;
		else if(typeInfo == typeLong)
			return (double)num.integer64;
		return num.integer;
	}

	bool		ConvertTo(TypeInfo *target);
protected:
	union Numbers
	{
		int integer;
		long long	integer64;
		double real;
		struct QuadWord
		{
			int low, high;
		} quad;
	} num;
};

//One child operators
class NodePopOp: public NodeOneOP
{
public:
	NodePopOp();
	virtual ~NodePopOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
};

class NodeUnaryOp: public NodeOneOP
{
public:
	NodeUnaryOp(CmdID cmd, unsigned int argument = 0);
	virtual ~NodeUnaryOp();

			void SetParentFunc(FunctionInfo* parent);

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	VMCmd vmCmd;
	FunctionInfo *parentFunc;
};

class NodeReturnOp: public NodeOneOP
{
public:
	NodeReturnOp(bool localRet, TypeInfo* tinfo, FunctionInfo* parentFunc, bool yield);
	virtual ~NodeReturnOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	bool			localReturn;
	bool			yieldResult;
	FunctionInfo	*parentFunction;
};

class NodeBlock: public NodeOneOP
{
public:
	NodeBlock(FunctionInfo* parentFunc, unsigned int shift);
	virtual ~NodeBlock();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	FunctionInfo	*parentFunction;
	unsigned int	stackFrameShift;
};

class NodeFuncDef: public NodeOneOP
{
public:
	NodeFuncDef(FunctionInfo *info, unsigned int varShift);
	virtual ~NodeFuncDef();

	virtual void Enable();
	virtual void Disable();
	virtual FunctionInfo*	GetFuncInfo(){ return funcInfo; }

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	FunctionInfo	*funcInfo;
	unsigned int shift;
	bool disabled;
};

//////////////////////////////////////////////////////////////////////////
class NodeGetAddress: public NodeZeroOP
{
public:
			NodeGetAddress(VariableInfo* variableInfo, int variableAddress, TypeInfo *retInfo);
	virtual ~NodeGetAddress();

			bool IsAbsoluteAddress();
			void SetAddressTracking();

			void IndexArray(int shift);
			void ShiftToMember(TypeInfo::MemberVariable *member);

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
			void TranslateToCEx(FILE *fOut, bool takeAddress);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
	friend class NodeVariableModify;
	friend class NodePreOrPostOp;
	friend class NodeFunctionAddress;
	friend class NodeReturnOp;

	TypeInfo		*typeOrig;
	VariableInfo	*varInfo;
	int				varAddress;
	bool			trackAddress;
};

class NodeGetUpvalue: public NodeZeroOP
{
public:
			NodeGetUpvalue(FunctionInfo* functionInfo, int closureOffset, int closureElement, TypeInfo *retInfo);
	virtual ~NodeGetUpvalue();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	FunctionInfo	*parentFunc;
	int			closurePos, closureElem;
};

class NodeConvertPtr: public NodeOneOP
{
public:
			NodeConvertPtr(TypeInfo *dstType);
	virtual ~NodeConvertPtr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
};

class NodeVariableSet: public NodeTwoOP
{
public:
			NodeVariableSet(TypeInfo* targetType, bool firstDefinition, bool swapNodes);
	virtual ~NodeVariableSet();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	unsigned int	elemCount;	// If node sets all array, here is the element count
	int		addrShift;
	bool	absAddress, knownAddress, arrSetAll;
};

class NodeVariableModify: public NodeTwoOP
{
public:
			NodeVariableModify(TypeInfo* targetType, CmdID cmd);
	virtual ~NodeVariableModify();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	CmdID	cmdID;
	int		addrShift;
	bool	absAddress, knownAddress;
};

class NodeDereference: public NodeOneOP
{
public:
			NodeDereference(FunctionInfo* setClosure = NULL, unsigned int offsetToPrevClosure = 0, bool readonly = false);
	virtual ~NodeDereference();

	virtual void Neutralize();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
private:
	int		addrShift;
	bool	absAddress, knownAddress, neutralized, readonly;
	FunctionInfo	*closureFunc;
	unsigned int	offsetToPreviousClosure;
	NodeZeroOP		*originalNode;
};

class NodeArrayIndex: public NodeTwoOP
{
public:
			NodeArrayIndex(TypeInfo* parentType);
	virtual ~NodeArrayIndex();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
public:
	friend class NodeDereference;
	friend class NodeVariableSet;
	friend class NodeVariableModify;
	friend class NodePreOrPostOp;

	TypeInfo	*typeParent;
	bool		knownShift;
	int			shiftValue;
};

class NodeShiftAddress: public NodeOneOP
{
public:
			NodeShiftAddress(TypeInfo::MemberVariable *classMember);
	virtual ~NodeShiftAddress();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
	friend class NodeVariableModify;
	friend class NodePreOrPostOp;

	TypeInfo::MemberVariable	*member;
	unsigned int	memberShift;
};

class NodePreOrPostOp: public NodeOneOP
{
public:
			NodePreOrPostOp(bool isInc, bool preOp);
	virtual ~NodePreOrPostOp();

			void SetOptimised(bool doOptimisation);

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	bool	incOp;
	bool	optimised;

	bool	prefixOp;

	int		addrShift;
	bool	absAddress, knownAddress;
};

class NodeFunctionAddress: public NodeOneOP
{
public:
			NodeFunctionAddress(FunctionInfo* functionInfo);
	virtual ~NodeFunctionAddress();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());

	FunctionInfo	*funcInfo;
};
//////////////////////////////////////////////////////////////////////////

class NodeBinaryOp: public NodeTwoOP
{
public:
	NodeBinaryOp(CmdID cmd);
	virtual ~NodeBinaryOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	CmdID cmdID;
};

class NodeIfElseExpr: public NodeThreeOP
{
public:
	NodeIfElseExpr(bool haveElse, bool isTerm = false);
	virtual ~NodeIfElseExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
};

class NodeForExpr: public NodeThreeOP
{
public:
	NodeForExpr();
	virtual ~NodeForExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	NodeZeroOP*	fourth;
};

class NodeWhileExpr: public NodeTwoOP
{
public:
	NodeWhileExpr();
	virtual ~NodeWhileExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
};

class NodeDoWhileExpr: public NodeTwoOP
{
public:
	NodeDoWhileExpr();
	virtual ~NodeDoWhileExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
};

class NodeBreakOp: public NodeZeroOP
{
public:
	NodeBreakOp(unsigned int breakDepth);
	virtual ~NodeBreakOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());

	static	void SatisfyJumps(unsigned int pos);
	static FastVector<unsigned int>	fixQueue;
protected:
	unsigned int breakDepth;
};

class NodeContinueOp: public NodeZeroOP
{
public:
	NodeContinueOp(unsigned int continueDepth);
	virtual ~NodeContinueOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());

	static	void SatisfyJumps(unsigned int pos);
	static FastVector<unsigned int>	fixQueue;
protected:
	unsigned int continueDepth;
};

class NodeSwitchExpr: public NodeOneOP
{
public:
	NodeSwitchExpr();
	virtual ~NodeSwitchExpr();

			void AddCase();
			void AddDefault();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	COMPILE_LLVM(virtual void CompileLLVM());

	static FastVector<unsigned int>	fixQueue;
protected:
	NodeZeroOP	*conditionHead, *conditionTail;
	NodeZeroOP	*blockHead, *blockTail;
	NodeZeroOP	*defaultCase;

	unsigned int	caseCount;
};

class NodeExpressionList: public NodeOneOP
{
public:
	NodeExpressionList(TypeInfo *returnType = typeVoid);
	virtual ~NodeExpressionList();

			void AddNode(bool reverse = true);
			NodeZeroOP* GetFirstNode();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
protected:
	NodeZeroOP	*tail;
};

class NodeFuncCall: public NodeOneOP
{
public:
	NodeFuncCall(FunctionInfo *info, FunctionType *type);
	virtual ~NodeFuncCall();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());

	static unsigned int baseShift;
	static ChunkedStackPool<4092>	memoPool;
	struct CallMemo
	{
		FunctionInfo	*func;
		NodeNumber		*value;
		char			*arguments;
	};
	static FastVector<CallMemo>		memoList;
public:
	FunctionInfo	*funcInfo;
	FunctionType	*funcType;

	NodeZeroOP		*paramHead, *paramTail;
};

class NodeFunctionProxy: public NodeZeroOP
{
public:
	NodeFunctionProxy(FunctionInfo *info, const char *pos, bool silent = false);
	virtual ~NodeFunctionProxy();

			bool HasType(TypeInfo *type);

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	COMPILE_TRANSLATION(virtual void TranslateToC(FILE *fOut));
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
	COMPILE_LLVM(virtual void CompileLLVM());
public:
	FunctionInfo	*funcInfo;
	const char		*codePos;
	bool			noError;
};
