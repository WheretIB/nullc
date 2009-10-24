#pragma once
#include "stdafx.h"
#include "InstructionSet.h"
#include "ParseClass.h"

//////////////////////////////////////////////////////////////////////////
enum NodeType
{
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
	typeNodeZeroOp,
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
};
//////////////////////////////////////////////////////////////////////////

class NodeZeroOP
{
public:
	NodeZeroOP();
	explicit NodeZeroOP(TypeInfo* tinfo);
	virtual ~NodeZeroOP();

	// Генерация кода
	virtual void Compile();
	// Вывод в лог параметров узла
	virtual void LogToStream(FILE *fGraph);
	// Установка строки кода, с которым связана ячейка
	virtual void SetCodeInfo(const char* newSourcePos);

	void*		operator new(size_t size)
	{
		return nodePool.Allocate((unsigned int)size);
	}
	void		operator delete(void *ptr, size_t size)
	{
		(void)ptr; (void)size;
		assert(!"Cannot delete NodeZeroOp");
	}

	static	ChunkedStackPool<4092>	nodePool;
	static void	DeleteNodes(){ nodePool.Clear(); }
protected:
	const char		*sourcePos;
public:
	TypeInfo		*typeInfo;
	NodeType		nodeType;
	NodeZeroOP		*prev, *next;	// For organizing intrusive node lists
};

//////////////////////////////////////////////////////////////////////////
class NodeOneOP: public NodeZeroOP
{
public:
	NodeOneOP();
	virtual ~NodeOneOP();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);

	NodeZeroOP*	GetFirstNode(){ return first; }
	void		SetFirstNode(NodeZeroOP* node){ first = node; }
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
protected:
};

class NodeUnaryOp: public NodeOneOP
{
public:
	NodeUnaryOp(CmdID cmd);
	virtual ~NodeUnaryOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	CmdID	cmdID;
};

class NodeReturnOp: public NodeOneOP
{
public:
	NodeReturnOp(int localRet, TypeInfo* tinfo);
	virtual ~NodeReturnOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	int	localReturn;
};

class NodeFuncDef: public NodeOneOP
{
public:
	NodeFuncDef(FunctionInfo *info, unsigned int varShift);
	virtual ~NodeFuncDef();

	virtual void Disable();
	virtual FunctionInfo*	GetFuncInfo(){ return funcInfo; }

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
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

			void IndexArray(int shift);
			void ShiftToMember(TypeInfo::MemberVariable *member);

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
	friend class NodeVariableModify;
	friend class NodePreOrPostOp;

	TypeInfo		*typeOrig;
	VariableInfo	*varInfo;
	int				varAddress;
	bool			absAddress;
};

class NodeVariableSet: public NodeTwoOP
{
public:
			NodeVariableSet(TypeInfo* targetType, bool firstDefinition, bool swapNodes);
	virtual ~NodeVariableSet();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
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
protected:
	CmdID	cmdID;
	int		addrShift;
	bool	absAddress, knownAddress;
};

class NodeDereference: public NodeOneOP
{
public:
			NodeDereference();
	virtual ~NodeDereference();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	int		addrShift;
	bool	absAddress, knownAddress;
};

class NodeArrayIndex: public NodeTwoOP
{
public:
			NodeArrayIndex(TypeInfo* parentType);
	virtual ~NodeArrayIndex();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
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
			NodeShiftAddress(unsigned int shift, TypeInfo* resType);
	virtual ~NodeShiftAddress();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
	friend class NodeVariableModify;
	friend class NodePreOrPostOp;

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
protected:
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
protected:
};

class NodeForExpr: public NodeThreeOP
{
public:
	NodeForExpr();
	virtual ~NodeForExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
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
protected:
};

class NodeDoWhileExpr: public NodeTwoOP
{
public:
	NodeDoWhileExpr();
	virtual ~NodeDoWhileExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
};

class NodeBreakOp: public NodeZeroOP
{
public:
	NodeBreakOp(unsigned int breakDepth);
	virtual ~NodeBreakOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);

	static	void SatisfyJumps(unsigned int pos);
	static FastVector<VMCmd*>	fixQueue;
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

	static	void SatisfyJumps(unsigned int pos);
	static FastVector<VMCmd*>	fixQueue;
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
protected:
	NodeZeroOP	*conditionHead, *conditionTail;
	NodeZeroOP	*blockHead, *blockTail;
	NodeZeroOP	*defaultCase;

	unsigned int	caseCount;

	static FastVector<VMCmd*>	fixQueue;
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
protected:
	NodeZeroOP	*tail;
};

class NodeFuncCall: public NodeTwoOP
{
public:
	NodeFuncCall(FunctionInfo *info, FunctionType *type);
	virtual ~NodeFuncCall();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
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
