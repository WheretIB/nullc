#pragma once
#include "stdafx.h"
#include "ParseCommand.h"
#include "ParseClass.h"

//////////////////////////////////////////////////////////////////////////
const UINT typeNodeBlock		= 1;
const UINT typeNodeExpression	= 2;
const UINT typeNodeForExpr		= 3;
const UINT typeNodeFuncCall		= 4;
const UINT typeNodeFuncDef		= 5;
const UINT typeNodeIfElseExpr	= 6;

const UINT typeNodeOneOp		= 8;
const UINT typeNodePopOp		= 9;


const UINT typeNodeReturnOp		= 12;
const UINT typeNodeThreeOp		= 13;
const UINT typeNodeTwoAndCmdOp	= 14;

const UINT typeNodeTwoOp		= 16;
const UINT typeNodeVarDef		= 17;



const UINT typeNodeZeroOp		= 21;
const UINT typeNodeWhileExpr	= 22;
const UINT typeNodeDoWhileExpr	= 22;
const UINT typeNodeBreakOp		= 23;
const UINT typeNodeCaseExpr		= 24;
const UINT typeNodeSwitchExpr	= 25;

const UINT typeNodeNumber		= 27;
const UINT typeNodeUnaryOp		= 28;
const UINT typeNodeFuncParam	= 29;


const UINT typeNodeExpressionList	= 32;
const UINT typeNodeArrayIndex	= 33;
const UINT typeNodeDereference	= 34;
const UINT typeNodeShiftAddress	= 35;
const UINT typeNodeVariableGet	= 36;
const UINT typeNodeVariableSet	= 37;
const UINT typeNodePreOrPostOp	= 38;
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
	virtual void LogToStream(ostringstream& ostr);
	// Получения размера кода, сгенерированного данным узлом
	virtual UINT GetSize();
	// Получение типа ячейки
	virtual UINT GetNodeType(){ return typeNodeZeroOp; }
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
void	DrawLine(ostringstream& ostr);

//////////////////////////////////////////////////////////////////////////
class NodeOneOP: public NodeZeroOP
{
public:
	NodeOneOP();
	virtual ~NodeOneOP();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeOneOp; }

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
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeTwoOp; }

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
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeThreeOp; }

	shared_ptr<NodeZeroOP>	GetTrirdNode(){ return third; }
protected:
	shared_ptr<NodeZeroOP>	third;
};

//Zero child operators
void NodeNumberPushCommand(USHORT cmdFlag, char* data, UINT dataSize);
template<typename T>
class NodeNumber: public NodeZeroOP
{
	typedef T NumType;
public:
	NodeNumber(NumType number, TypeInfo* ptrType){ num = number; typeInfo = ptrType; }
	virtual ~NodeNumber(){}

	virtual void Compile()
	{
		NodeNumberPushCommand((USHORT)(GetAsmStackType<T>() | GetAsmDataType<T>()), (char*)(&num), sizeof(T));
	}
	virtual void LogToStream(ostringstream& ostr)
	{
		DrawLine(ostr);
		ostr << *typeInfo << "Number " << num << "\r\n";
	}
	virtual UINT GetSize()
	{
		return sizeof(CmdID) + sizeof(USHORT) + sizeof(T);
	}
	virtual UINT GetNodeType(){ return typeNodeNumber; }

	NumType		 GetVal(){ return num; }
	NumType		 GetLogNotVal(){ return !num; }
	NumType		 GetBitNotVal(){ return ~num; }
protected:
	NumType		num;
private:
	template<typename N>	asmDataType	GetAsmDataType();
	template<>	asmDataType		GetAsmDataType<char>(){ return DTYPE_CHAR; }
	template<>	asmDataType		GetAsmDataType<short>(){ return DTYPE_SHORT; }
	template<>	asmDataType		GetAsmDataType<int>(){ return DTYPE_INT; }
	template<>	asmDataType		GetAsmDataType<long long>(){ return DTYPE_LONG; }
	template<>	asmDataType		GetAsmDataType<float>(){ return DTYPE_FLOAT; }
	template<>	asmDataType		GetAsmDataType<double>(){ return DTYPE_DOUBLE; }
	template<typename N>	asmStackType	GetAsmStackType();
	template<>	asmStackType	GetAsmStackType<char>(){ return STYPE_INT; }
	template<>	asmStackType	GetAsmStackType<short>(){ return STYPE_INT; }
	template<>	asmStackType	GetAsmStackType<int>(){ return STYPE_INT; }
	template<>	asmStackType	GetAsmStackType<long long>(){ return STYPE_LONG; }
	template<>	asmStackType	GetAsmStackType<float>(){ return STYPE_DOUBLE; }	// float expands to double
	template<>	asmStackType	GetAsmStackType<double>(){ return STYPE_DOUBLE; }
};

class NodeVarDef: public NodeZeroOP
{
public:
	NodeVarDef(UINT sh, std::string nm);
	virtual ~NodeVarDef();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeVarDef; }
protected:
	UINT shift;
	std::string name;
};

//One child operators
class NodePopOp: public NodeOneOP
{
public:
	NodePopOp();
	virtual ~NodePopOp();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodePopOp; }
protected:
};

class NodeUnaryOp: public NodeOneOP
{
public:
	NodeUnaryOp(CmdID cmd);
	virtual ~NodeUnaryOp();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeUnaryOp; }
protected:
	CmdID	cmdID;
};

class NodeReturnOp: public NodeOneOP
{
public:
	NodeReturnOp(UINT c, TypeInfo* tinfo);
	virtual ~NodeReturnOp();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeReturnOp; }
protected:
	UINT	popCnt;
};

class NodeExpression: public NodeOneOP
{
public:
	NodeExpression(TypeInfo* realRetType = typeVoid);
	virtual ~NodeExpression();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeExpression; }
protected:
};

class NodeBlock: public NodeOneOP
{
public:
	NodeBlock();
	virtual ~NodeBlock();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeBlock; }
protected:
};

class NodeFuncDef: public NodeOneOP
{
public:
	NodeFuncDef(FunctionInfo *info);
	virtual ~NodeFuncDef();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeFuncDef; }
protected:
	FunctionInfo	*funcInfo;
};

//////////////////////////////////////////////////////////////////////////
class NodeVariableGet: public NodeZeroOP
{
public:
			NodeVariableGet(VariableInfo* variableInfo, int variableAddress, bool absoluteAddressation);
	virtual ~NodeVariableGet();

			bool IsAbsoluteAddress();

			void IndexArray(int shift);
			void ShiftToMember(int member);

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeVariableGet; }
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
			NodeVariableSet(TypeInfo* targetType, UINT pushVar, bool swapNodes);
	virtual ~NodeVariableSet();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeVariableSet; }
	virtual TypeInfo*	GetTypeInfo();
protected:
	UINT	bytesToPush;
	int		addrShift;
	bool	absAddress, knownAddress, arrSetAll;
};

class NodeDereference: public NodeOneOP
{
public:
			NodeDereference(TypeInfo* type);
	virtual ~NodeDereference();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeDereference; }
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
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeArrayIndex; }
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
			NodeShiftAddress(UINT shift, TypeInfo* resType);
	virtual ~NodeShiftAddress();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeShiftAddress; }
	virtual TypeInfo*	GetTypeInfo();
protected:
	friend class NodeDereference;
	friend class NodeVariableSet;
	friend class NodePreOrPostOp;

	UINT	memberShift;
};

class NodePreOrPostOp: public NodeOneOP
{
public:
			NodePreOrPostOp(TypeInfo* resType, CmdID cmd, bool preOp);
	virtual ~NodePreOrPostOp();

			void SetOptimised(bool doOptimisation);

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodePreOrPostOp; }
	virtual TypeInfo*	GetTypeInfo();
protected:
	CmdID	cmdID;
	bool	optimised;

	bool	prefixOp;

	int		addrShift;
	bool	absAddress, knownAddress;
};

//////////////////////////////////////////////////////////////////////////

class NodeTwoAndCmdOp: public NodeTwoOP
{
public:
	NodeTwoAndCmdOp(CmdID cmd);
	virtual ~NodeTwoAndCmdOp();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeTwoAndCmdOp; }
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
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeIfElseExpr; }
protected:
};

class NodeForExpr: public NodeThreeOP
{
public:
	NodeForExpr();
	virtual ~NodeForExpr();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeForExpr; }
protected:
	shared_ptr<NodeZeroOP>	fourth;
};

class NodeWhileExpr: public NodeTwoOP
{
public:
	NodeWhileExpr();
	virtual ~NodeWhileExpr();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeWhileExpr; }
protected:
};

class NodeDoWhileExpr: public NodeTwoOP
{
public:
	NodeDoWhileExpr();
	virtual ~NodeDoWhileExpr();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeDoWhileExpr; }
protected:
};

class NodeBreakOp: public NodeZeroOP
{
public:
	NodeBreakOp(UINT c);
	virtual ~NodeBreakOp();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeBreakOp; }
protected:
	UINT	popCnt;
};

class NodeSwitchExpr: public NodeOneOP
{
public:
	NodeSwitchExpr();
	virtual ~NodeSwitchExpr();

			void AddCase();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeSwitchExpr; }
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

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeExpressionList; }
protected:
	std::list<shared_ptr<NodeZeroOP> >	exprList;
	typedef std::list<shared_ptr<NodeZeroOP> >::iterator listPtr;
};

class NodeFuncCall: public NodeZeroOP
{
public:
	NodeFuncCall(FunctionInfo *info);
	virtual ~NodeFuncCall();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeFuncCall; }
protected:
	FunctionInfo	*funcInfo;

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
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNode; }
	virtual TypeInfo*	GetTypeInfo();
protected:
};*/
