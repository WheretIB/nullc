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
const unsigned int typeNodeVarDef		= 17;



const unsigned int typeNodeZeroOp		= 21;
const unsigned int typeNodeWhileExpr	= 22;
const unsigned int typeNodeDoWhileExpr	= 22;
const unsigned int typeNodeBreakOp		= 23;
const unsigned int typeNodeCaseExpr		= 24;
const unsigned int typeNodeSwitchExpr	= 25;

const unsigned int typeNodeNumber		= 27;
const unsigned int typeNodeUnaryOp		= 28;
const unsigned int typeNodeFuncParam	= 29;


const unsigned int typeNodeExpressionList	= 32;
const unsigned int typeNodeArrayIndex	= 33;
const unsigned int typeNodeDereference	= 34;
const unsigned int typeNodeShiftAddress	= 35;
const unsigned int typeNodeGetAddress	= 36;
const unsigned int typeNodeVariableSet	= 37;
const unsigned int typeNodePreOrPostOp	= 38;
const unsigned int typeNodeFunctionAddress	= 39;
const unsigned int typeNodeContinueOp	= 40;
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
	// Получения размера кода, сгенерированного данным узлом
	virtual unsigned int GetSize();
	// Получение типа ячейки
	virtual unsigned int GetNodeType(){ return typeNodeZeroOp; }
	// Получение типа результата, возвращаемого ячейкой
	virtual TypeInfo*	GetTypeInfo();
	// Установка строки кода, с которым связана ячейка
	virtual void SetCodeInfo(const char* start, const char* end);
protected:
	TypeInfo*	typeInfo;
	const char	*strBegin, *strEnd;
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeOneOp; }

	shared_ptr<NodeZeroOP>	GetFirstNode(){ return first; }
protected:
	shared_ptr<NodeZeroOP>	first;
};

class NodeTwoOP: public NodeOneOP
{
public:
	NodeTwoOP();
	virtual ~NodeTwoOP();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeTwoOp; }

	shared_ptr<NodeZeroOP>	GetSecondNode(){ return second; }
protected:
	shared_ptr<NodeZeroOP>	second;
};

class NodeThreeOP: public NodeTwoOP
{
public:
	NodeThreeOP();
	virtual ~NodeThreeOP();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeThreeOp; }

	shared_ptr<NodeZeroOP>	GetTrirdNode(){ return third; }
protected:
	shared_ptr<NodeZeroOP>	third;
};

// Assembly type traits
template <typename T> struct AsmTypeTraits;

#define ASM_TYPE_TRAITS(type, dataTypeValue, stackTypeValue) template <> struct AsmTypeTraits<type> { static const asmDataType dataType = dataTypeValue; static const asmStackType stackType = stackTypeValue; }

ASM_TYPE_TRAITS(char, DTYPE_CHAR, STYPE_INT);
ASM_TYPE_TRAITS(short, DTYPE_SHORT, STYPE_INT);
ASM_TYPE_TRAITS(int, DTYPE_INT, STYPE_INT);
ASM_TYPE_TRAITS(long long, DTYPE_LONG, STYPE_LONG);
ASM_TYPE_TRAITS(float, DTYPE_FLOAT, STYPE_DOUBLE); // float expands to double
ASM_TYPE_TRAITS(double, DTYPE_DOUBLE, STYPE_DOUBLE);

//Zero child operators
void NodeNumberPushCommand(asmDataType dt, char* data);
template<typename T>
class NodeNumber: public NodeZeroOP
{
	typedef T NumType;
public:
	NodeNumber(NumType number, TypeInfo* ptrType){ num = number; typeInfo = ptrType; }
	virtual ~NodeNumber(){}

	virtual void Compile()
	{
		typedef AsmTypeTraits<T> Traits;
		NodeNumberPushCommand(Traits::dataType, (char*)(&num));
	}
	virtual void LogToStream(FILE *fGraph)
	{
		DrawLine(fGraph);
		fprintf(fGraph, "%s Number\r\n", typeInfo->GetTypeName().c_str());
	}
	virtual unsigned int GetSize()
	{
		typedef AsmTypeTraits<T> Traits;
		return Traits::dataType == DTYPE_FLOAT ? 2 : (sizeof(T)/4);
	}
	virtual unsigned int GetNodeType(){ return typeNodeNumber; }

	NumType		 GetVal(){ return num; }
	NumType		 GetLogNotVal(){ return !num; }
	NumType		 GetBitNotVal(){ return ~num; }
protected:
	NumType		num;
};

class NodeVarDef: public NodeZeroOP
{
public:
	NodeVarDef(std::string nm);
	virtual ~NodeVarDef();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeVarDef; }
protected:
	std::string name;
};

//One child operators
class NodePopOp: public NodeOneOP
{
public:
	NodePopOp();
	virtual ~NodePopOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodePopOp; }
protected:
};

class NodeUnaryOp: public NodeOneOP
{
public:
	NodeUnaryOp(CmdID cmd);
	virtual ~NodeUnaryOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeUnaryOp; }
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeReturnOp; }
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeExpression; }
protected:
};

class NodeBlock: public NodeOneOP
{
public:
	NodeBlock(unsigned int varShift, bool postPop = true);
	virtual ~NodeBlock();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeBlock; }
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

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeFuncDef; }
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
			void ShiftToMember(int member);

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeGetAddress; }
	virtual TypeInfo*	GetTypeInfo();
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
	friend class NodePreOrPostOp;

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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeVariableSet; }
	virtual TypeInfo*	GetTypeInfo();
protected:
	unsigned int	bytesToPush;
	int		addrShift;
	bool	absAddress, knownAddress, arrSetAll;
};

class NodeDereference: public NodeOneOP
{
public:
			NodeDereference(TypeInfo* type);
	virtual ~NodeDereference();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeDereference; }
	virtual TypeInfo*	GetTypeInfo();
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeArrayIndex; }
	virtual TypeInfo*	GetTypeInfo();
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeShiftAddress; }
	virtual TypeInfo*	GetTypeInfo();
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodePreOrPostOp; }
	virtual TypeInfo*	GetTypeInfo();
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeFunctionAddress; }
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeTwoAndCmdOp; }
protected:
	CmdID cmdID;

	friend class NodeVarGet;
	friend class NodeVarSet;
	friend class NodeVarSetAndOp;
	friend class NodePreValOp;
};

class NodeIfElseExpr: public NodeThreeOP
{
public:
	NodeIfElseExpr(bool haveElse, bool isTerm = false);
	virtual ~NodeIfElseExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeIfElseExpr; }
protected:
};

class NodeForExpr: public NodeThreeOP
{
public:
	NodeForExpr();
	virtual ~NodeForExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeForExpr; }
protected:
	shared_ptr<NodeZeroOP>	fourth;
};

class NodeWhileExpr: public NodeTwoOP
{
public:
	NodeWhileExpr();
	virtual ~NodeWhileExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeWhileExpr; }
protected:
};

class NodeDoWhileExpr: public NodeTwoOP
{
public:
	NodeDoWhileExpr();
	virtual ~NodeDoWhileExpr();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeDoWhileExpr; }
protected:
};

class NodeBreakOp: public NodeZeroOP
{
public:
	NodeBreakOp(unsigned int c);
	virtual ~NodeBreakOp();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeBreakOp; }
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeContinueOp; }
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
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeSwitchExpr; }
protected:
	std::list<shared_ptr<NodeZeroOP> >	caseCondList;
	std::list<shared_ptr<NodeZeroOP> >	caseBlockList;
	typedef std::list<shared_ptr<NodeZeroOP> >::iterator casePtr;
};

class NodeExpressionList: public NodeZeroOP
{
public:
	NodeExpressionList(TypeInfo *returnType = typeVoid);
	virtual ~NodeExpressionList();

			void AddNode(bool reverse = true);
			shared_ptr<NodeZeroOP> GetFirstNode();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeExpressionList; }
protected:
	std::list<shared_ptr<NodeZeroOP> >	exprList;
	typedef std::list<shared_ptr<NodeZeroOP> >::iterator listPtr;
};

class NodeFuncCall: public NodeTwoOP
{
public:
	NodeFuncCall(FunctionInfo *info, FunctionType *type);
	virtual ~NodeFuncCall();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNodeFuncCall; }
protected:
	FunctionInfo	*funcInfo;
	FunctionType	*funcType;

	std::list<shared_ptr<NodeZeroOP> >	paramList;
	typedef std::list<shared_ptr<NodeZeroOP> >::reverse_iterator paramPtr;
};

/*
class Node: public NodeOP
{
public:
	Node();
	virtual ~Node();

	virtual void Compile();
	virtual void LogToStream(FILE *fGraph);
	virtual unsigned int GetSize();
	virtual unsigned int GetNodeType(){ return typeNode; }
	virtual TypeInfo*	GetTypeInfo();
protected:
};*/
