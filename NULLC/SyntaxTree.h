#pragma once
#include "stdafx.h"
#include "InstructionSet.h"
#include "ParseClass.h"

void	ResetTreeGlobals();
void	OutputCFunctionName(FILE *fOut, FunctionInfo *funcInfo);

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
	virtual void TranslateToC(FILE *fOut);
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
	virtual void TranslateToCExtra(FILE *fOut);

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
	virtual void TranslateToC(FILE *fOut);

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
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);

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
	virtual void TranslateToC(FILE *fOut);
protected:
};

class NodeUnaryOp: public NodeOneOP
{
public:
	NodeUnaryOp(CmdID cmd, unsigned int argument = 0);
	virtual ~NodeUnaryOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
protected:
	VMCmd vmCmd;
};

class NodeReturnOp: public NodeOneOP
{
public:
	NodeReturnOp(bool localRet, TypeInfo* tinfo, FunctionInfo* parentFunc = NULL);
	virtual ~NodeReturnOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
protected:
	bool			localReturn;
	FunctionInfo	*parentFunction;
};

class NodeBlock: public NodeOneOP
{
public:
	NodeBlock(FunctionInfo* parentFunc, unsigned int shift);
	virtual ~NodeBlock();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
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
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
protected:
	FunctionInfo	*funcInfo;
	unsigned int shift;
	bool disabled;
};

//////////////////////////////////////////////////////////////////////////
class NodeGetAddress: public NodeZeroOP
{
public:
			NodeGetAddress(VariableInfo* variableInfo, int variableAddress, bool absoluteAddressation, TypeInfo *retInfo = NULL);
	virtual ~NodeGetAddress();

			bool IsAbsoluteAddress();
			void SetAddressTracking();

			void IndexArray(int shift);
			void ShiftToMember(TypeInfo::MemberVariable *member);

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
			void TranslateToCEx(FILE *fOut, bool takeAddress);
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
	friend class NodeVariableModify;
	friend class NodePreOrPostOp;
	friend class NodeFunctionAddress;

	TypeInfo		*typeOrig;
	VariableInfo	*varInfo;
	int				varAddress, addressOriginal;
	bool			absAddress, trackAddress;
};

class NodeGetUpvalue: public NodeZeroOP
{
public:
			NodeGetUpvalue(FunctionInfo* functionInfo, int closureOffset, int closureElement, TypeInfo *retInfo);
	virtual ~NodeGetUpvalue();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
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
	virtual void TranslateToC(FILE *fOut);
protected:
};

class NodeVariableSet: public NodeTwoOP
{
public:
			NodeVariableSet(TypeInfo* targetType, bool firstDefinition, bool swapNodes);
	virtual ~NodeVariableSet();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
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
	virtual void TranslateToC(FILE *fOut);
protected:
	CmdID	cmdID;
	int		addrShift;
	bool	absAddress, knownAddress;
};

class NodeDereference: public NodeOneOP
{
public:
			NodeDereference(FunctionInfo* setClosure = NULL, unsigned int offsetToPrevClosure = 0);
	virtual ~NodeDereference();

	virtual void Neutralize();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
private:
	int		addrShift;
	bool	absAddress, knownAddress, neutralized;
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
	virtual void TranslateToC(FILE *fOut);
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
	virtual void TranslateToC(FILE *fOut);
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
	virtual void TranslateToC(FILE *fOut);
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
	virtual void TranslateToC(FILE *fOut);

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
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
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
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
protected:
};

class NodeForExpr: public NodeThreeOP
{
public:
	NodeForExpr();
	virtual ~NodeForExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
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
	virtual void TranslateToC(FILE *fOut);
protected:
};

class NodeDoWhileExpr: public NodeTwoOP
{
public:
	NodeDoWhileExpr();
	virtual ~NodeDoWhileExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);
protected:
};

class NodeBreakOp: public NodeZeroOP
{
public:
	NodeBreakOp(unsigned int breakDepth);
	virtual ~NodeBreakOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual void TranslateToC(FILE *fOut);

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
	virtual void TranslateToC(FILE *fOut);

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
	virtual void TranslateToC(FILE *fOut);

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
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
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
	virtual void TranslateToC(FILE *fOut);
	virtual NodeNumber*	Evaluate(char *memory, unsigned int size);
public:
	FunctionInfo	*funcInfo;
	FunctionType	*funcType;

	NodeZeroOP		*paramHead, *paramTail;
};

/*
class Node: public NodeOP
{
public:
	Node();
	virtual ~Node();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
};*/
