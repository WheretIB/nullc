#pragma once
#include "stdafx.h"
#include "ParseCommand.h"
#include "ParseClass.h"

//////////////////////////////////////////////////////////////////////////
const unsigned int typeNodeBlock		= 1;
const unsigned int typeNodeExpression	= 2;
const unsigned int typeNodeForExpr		= 3;
const unsigned int typeNodeFuncCall		= 4;
const unsigned int typeNodeFuncDef		= 5;
const unsigned int typeNodeIfElseExpr	= 6;

const unsigned int typeNodeOneOp		= 8;
const unsigned int typeNodePopOp		= 9;


const unsigned int typeNodeReturnOp		= 12;
const unsigned int typeNodeThreeOp		= 13;
const unsigned int typeNodeTwoAndCmdOp	= 14;

const unsigned int typeNodeTwoOp		= 16;




const unsigned int typeNodeZeroOp		= 21;
const unsigned int typeNodeWhileExpr	= 22;
const unsigned int typeNodeDoWhileExpr	= 22;
const unsigned int typeNodeBreakOp		= 23;

const unsigned int typeNodeSwitchExpr	= 25;

const unsigned int typeNodeNumber		= 27;
const unsigned int typeNodeUnaryOp		= 28;



const unsigned int typeNodeExpressionList	= 32;
const unsigned int typeNodeArrayIndex	= 33;
const unsigned int typeNodeDereference	= 34;
const unsigned int typeNodeShiftAddress	= 35;
const unsigned int typeNodeGetAddress	= 36;
const unsigned int typeNodeVariableSet	= 37;
const unsigned int typeNodePreOrPostOp	= 38;
const unsigned int typeNodeFunctionAddress	= 39;
const unsigned int typeNodeContinueOp		= 40;
const unsigned int typeNodeVariableModify	= 41;
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
	virtual void SetCodeInfo(const char* start, const char* end);

	void*		operator new(unsigned int size)
	{
		return nodePool.Allocate(size);
	}
	void		operator delete(void *ptr, unsigned int size)
	{
		(void)ptr; (void)size;
		assert(!"Cannot delete NodeZeroOp");
	}

	static	ChunkedStackPool<4092>	nodePool;
	static void	DeleteNodes(){ nodePool.Clear(); }
protected:
	const char	*strBegin, *strEnd;
public:
	TypeInfo	*typeInfo;
	unsigned int codeSize;
	unsigned int nodeType;
	NodeZeroOP	*prev, *next;	// For organizing intrusive node lists
};

void	GoDown();
void	GoUp();
void	DrawLine(FILE *fGraph);

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

// Assembly type traits
template <typename T> struct AsmTypeTraits;

#define ASM_TYPE_TRAITS(type, dataTypeValue, stackTypeValue, nodeCodeSize) template <> struct AsmTypeTraits<type>\
{\
	static const asmDataType dataType = dataTypeValue;\
	static const asmStackType stackType = stackTypeValue;\
	static const unsigned int codeSize = nodeCodeSize;\
}

ASM_TYPE_TRAITS(char, DTYPE_CHAR, STYPE_INT, 1);
ASM_TYPE_TRAITS(short, DTYPE_SHORT, STYPE_INT, 1);
ASM_TYPE_TRAITS(int, DTYPE_INT, STYPE_INT, 1);
ASM_TYPE_TRAITS(long long, DTYPE_LONG, STYPE_LONG, 2);
ASM_TYPE_TRAITS(float, DTYPE_FLOAT, STYPE_DOUBLE, 2); // float expands to double
ASM_TYPE_TRAITS(double, DTYPE_DOUBLE, STYPE_DOUBLE, 2);

//Zero child operators
void NodeNumberPushCommand(asmDataType dt, char* data);
template<typename T>
class NodeNumber: public NodeZeroOP
{
	typedef T NumType;
	typedef AsmTypeTraits<T> Traits;
public:
	NodeNumber(NumType number, TypeInfo* ptrType)
	{
		num = number;
		typeInfo = ptrType;
		codeSize = Traits::codeSize;
		nodeType = typeNodeNumber;
	}
	virtual ~NodeNumber(){}

	virtual void Compile()
	{
		NodeNumberPushCommand(Traits::dataType, (char*)(&num));
	}
	virtual void LogToStream(FILE *fGraph)
	{
		DrawLine(fGraph);
		fprintf(fGraph, "%s Number\r\n", typeInfo->GetFullTypeName());
	}

	NumType		 GetVal(){ return num; }
	NumType		 GetLogNotVal(){ return !num; }
	NumType		 GetBitNotVal(){ return ~num; }
protected:
	NumType		num;
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
	NodeReturnOp(unsigned int c, TypeInfo* tinfo);
	virtual ~NodeReturnOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	unsigned int	popCnt;
};

class NodeExpression: public NodeOneOP
{
public:
	NodeExpression(TypeInfo* realRetType = typeVoid);
	virtual ~NodeExpression();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
};

class NodeBlock: public NodeOneOP
{
public:
	NodeBlock(unsigned int varShift, bool postPop = true);
	virtual ~NodeBlock();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	unsigned int shift;
	bool popAfter;
};

class NodeFuncDef: public NodeOneOP
{
public:
	NodeFuncDef(FunctionInfo *info);
	virtual ~NodeFuncDef();

	virtual void Disable();
	virtual FunctionInfo*	GetFuncInfo(){ return funcInfo; }

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	FunctionInfo	*funcInfo;
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
			NodeVariableSet(TypeInfo* targetType, unsigned int pushVar, bool swapNodes);
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
			NodeDereference(TypeInfo* type);
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
			NodePreOrPostOp(TypeInfo* resType, bool isInc, bool preOp);
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

class NodeTwoAndCmdOp: public NodeTwoOP
{
public:
	NodeTwoAndCmdOp(CmdID cmd);
	virtual ~NodeTwoAndCmdOp();

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
	NodeBreakOp(unsigned int c);
	virtual ~NodeBreakOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	unsigned int	popCnt;
};

class NodeContinueOp: public NodeZeroOP
{
public:
	NodeContinueOp(unsigned int c);
	virtual ~NodeContinueOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	unsigned int	popCnt;
};

class NodeSwitchExpr: public NodeOneOP
{
public:
	NodeSwitchExpr();
	virtual ~NodeSwitchExpr();

			void AddCase();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
protected:
	NodeZeroOP	*conditionHead, *conditionTail;
	NodeZeroOP	*blockHead, *blockTail;

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
