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
//const UINT typeNodeNegOp		= 7;
const UINT typeNodeOneOp		= 8;
const UINT typeNodePopOp		= 9;
const UINT typeNodePreValOp		= 10;
//const UINT typeNodeRealNum		= 11;
const UINT typeNodeReturnOp		= 12;
const UINT typeNodeThreeOp		= 13;
const UINT typeNodeTwoAndCmdOp	= 14;
const UINT typeNodeTwoExpression= 15;
const UINT typeNodeTwoOp		= 16;
const UINT typeNodeVarDef		= 17;
const UINT typeNodeVarGet		= 18;
const UINT typeNodeVarSet		= 19;
const UINT typeNodeVarSetAndOp	= 20;
const UINT typeNodeZeroOp		= 21;
const UINT typeNodeWhileExpr	= 22;
const UINT typeNodeDoWhileExpr	= 22;
const UINT typeNodeBreakOp		= 23;
const UINT typeNodeCaseExpr		= 24;
const UINT typeNodeSwitchExpr	= 25;
//const UINT typeNodeNotOp		= 26;
const UINT typeNodeNumber		= 27;
const UINT typeNodeUnaryOp		= 28;
const UINT typeNodeFuncParam	= 29;
//////////////////////////////////////////////////////////////////////////

class NodeZeroOP
{
public:
	NodeZeroOP();
	explicit NodeZeroOP(TypeInfo* tinfo);
	virtual ~NodeZeroOP();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeZeroOp; }
	virtual TypeInfo*	typeInfo();
protected:
	TypeInfo*	m_typeInfo;
};

std::vector<shared_ptr<NodeZeroOP> >*	getList();
void	SetCommandList(CommandList* list);
void	SetFunctionList(std::vector<FunctionInfo*>* list);
void	SetLogStream(ostringstream* stream);
void	SetNodeList(std::vector<shared_ptr<NodeZeroOP> >* list);

CommandList*	GetCommandList();

void	goDown();
void	goUp();
void	drawLn(ostringstream& ostr);

//////////////////////////////////////////////////////////////////////////
class NodeOneOP: public NodeZeroOP
{
public:
	NodeOneOP();
	virtual ~NodeOneOP();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeOneOp; }
protected:
	shared_ptr<NodeZeroOP>	first;
};

class NodeTwoOP: public NodeOneOP
{
public:
	NodeTwoOP();
	virtual ~NodeTwoOP();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeTwoOp; }
protected:
	shared_ptr<NodeZeroOP>	second;
};

class NodeThreeOP: public NodeTwoOP
{
public:
	NodeThreeOP();
	virtual ~NodeThreeOP();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeThreeOp; }
protected:
	shared_ptr<NodeZeroOP>	third;
};

//Zero child operators
template<typename T>
class NodeNumber: public NodeZeroOP
{
	typedef T NumType;
public:
	NodeNumber(NumType num, TypeInfo* ptrType){ m_num = num; m_typeInfo = ptrType; }
	virtual ~NodeNumber(){}

	virtual void doAct()
	{
		GetCommandList()->AddData(cmdPush);
		GetCommandList()->AddData((USHORT)(GetAsmStackType<T>() | GetAsmDataType<T>()));
		GetCommandList()->AddData((T)m_num);
	}
	virtual void doLog(ostringstream& ostr){ drawLn(ostr); ostr << *m_typeInfo << "Number " << m_num << "\r\n"; }
	virtual UINT getSize()
	{
		return sizeof(CmdID) + sizeof(USHORT) + sizeof(T);
	}
	virtual UINT getType(){ return typeNodeNumber; }

	NumType		 getVal(){ return m_num; }
	NumType		 getLogNotVal(){ return !m_num; }
	NumType		 getBitNotVal(){ return ~m_num; }
protected:
	NumType		m_num;
private:
	template<typename N>	asmDataType	GetAsmDataType();
	template<>	asmDataType	GetAsmDataType<char>(){ return DTYPE_CHAR; }
	template<>	asmDataType	GetAsmDataType<short>(){ return DTYPE_SHORT; }
	template<>	asmDataType	GetAsmDataType<int>(){ return DTYPE_INT; }
	template<>	asmDataType	GetAsmDataType<long long>(){ return DTYPE_LONG; }
	template<>	asmDataType	GetAsmDataType<float>(){ return DTYPE_FLOAT; }
	template<>	asmDataType	GetAsmDataType<double>(){ return DTYPE_DOUBLE; }
	template<typename N>	asmStackType	GetAsmStackType();
	template<>	asmStackType	GetAsmStackType<char>(){ return STYPE_INT; }
	template<>	asmStackType	GetAsmStackType<short>(){ return STYPE_INT; }
	template<>	asmStackType	GetAsmStackType<int>(){ return STYPE_INT; }
	template<>	asmStackType	GetAsmStackType<long long>(){ return STYPE_LONG; }
	template<>	asmStackType	GetAsmStackType<float>(){ return STYPE_DOUBLE; }	// float expands to double
	template<>	asmStackType	GetAsmStackType<double>(){ return STYPE_DOUBLE; }
};

//One child operators
class NodePopOp: public NodeOneOP
{
public:
	NodePopOp();
	virtual ~NodePopOp();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodePopOp; }
protected:
};

class NodeUnaryOp: public NodeOneOP
{
public:
	NodeUnaryOp(CmdID op);
	virtual ~NodeUnaryOp();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeUnaryOp; }
protected:
	CmdID m_op;
};

class NodeReturnOp: public NodeOneOP
{
public:
	NodeReturnOp(UINT c, TypeInfo* tinfo);
	virtual ~NodeReturnOp();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeReturnOp; }
protected:
	UINT	m_popCnt;
};

class NodeExpression: public NodeOneOP
{
public:
	NodeExpression();
	virtual ~NodeExpression();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeExpression; }
protected:
};

class NodeVarDef: public NodeZeroOP
{
public:
	NodeVarDef(UINT sh, std::string nm);
	virtual ~NodeVarDef();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeVarDef; }
protected:
	UINT shift;
	std::string name;
};

class NodeBlock: public NodeOneOP
{
public:
	NodeBlock();
	virtual ~NodeBlock();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeBlock; }
protected:
};

class NodeFuncDef: public NodeOneOP
{
public:
	NodeFuncDef(UINT id);
	virtual ~NodeFuncDef();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeFuncDef; }
protected:
	UINT	m_id;
};

class NodeFuncParam: public NodeOneOP
{
public:
	NodeFuncParam(TypeInfo* tinfo);
	virtual ~NodeFuncParam();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeFuncParam; }
protected:
};

class NodeFuncCall: public NodeOneOP
{
public:
	NodeFuncCall(std::string name, UINT id, UINT argCnt, TypeInfo* retType);
	virtual ~NodeFuncCall();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeFuncCall; }
protected:
	std::string	m_name;
	UINT		m_id;
};

class NodeVarSet: public NodeTwoOP
{
public:
	NodeVarSet(VariableInfo vInfo, UINT adrShift = 0, bool allSet = false, bool adrAbs = false);
	//NodeVarSet(TypeInfo* tinfo, UINT vpos, std::string name, bool arr, UINT size, bool allSet = false, bool adrAbs = false);
	virtual ~NodeVarSet();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeVarSet; }
protected:
	VariableInfo	m_varInfo;
	UINT		m_adrVar;
	bool		m_allSet, m_adrAbs;
	//std::string	m_name;
	
	//UINT		m_size;
};

class NodeVarGet: public NodeOneOP
{
public:
	NodeVarGet(VariableInfo vInfo, UINT adrShift = 0, bool adrAbs = false);
	//NodeVarGet(TypeInfo* tinfo, UINT vpos, std::string name, bool arr, UINT size, bool adrAbs = false);
	virtual ~NodeVarGet();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeVarGet; }
protected:
	UINT		m_vpos;
	std::string	m_name;
	bool		m_arr, m_absadr;
	UINT		m_size;
};

class NodeVarSetAndOp: public NodeTwoOP
{
public:
	NodeVarSetAndOp(TypeInfo* tinfo, UINT vpos, std::string name, bool arr, UINT size, CmdID cmd);
	virtual ~NodeVarSetAndOp();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeVarSetAndOp; }
protected:
	UINT		m_vpos;
	std::string	m_name;
	bool		m_arr;
	UINT		m_size;
	CmdID		m_cmd;
};

class NodePreValOp: public NodeOneOP
{
public:
	NodePreValOp(TypeInfo* tinfo, UINT vpos, std::string name, bool arr, UINT size, CmdID cmd, bool pre);
	virtual ~NodePreValOp();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodePreValOp; }

	void		 SetOptimised(bool optim){ m_optimised = optim; }
protected:
	UINT		m_vpos;
	std::string	m_name;
	bool		m_arr, m_pre;
	UINT		m_size;
	CmdID		m_cmd;
	bool		m_optimised;
};
//Two child operators

class NodeTwoAndCmdOp: public NodeTwoOP
{
public:
	NodeTwoAndCmdOp(CmdID cmd);
	virtual ~NodeTwoAndCmdOp();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeTwoAndCmdOp; }
protected:
	CmdID m_cmd;
};

class NodeTwoExpression: public NodeTwoOP
{
public:
	NodeTwoExpression();
	virtual ~NodeTwoExpression();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeTwoExpression; }
protected:
};

class NodeIfElseExpr: public NodeThreeOP
{
public:
	NodeIfElseExpr(bool haveElse, bool isTerm = false);
	virtual ~NodeIfElseExpr();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeIfElseExpr; }
protected:
};

class NodeForExpr: public NodeThreeOP
{
public:
	NodeForExpr();
	virtual ~NodeForExpr();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeForExpr; }
protected:
	shared_ptr<NodeZeroOP>	fourth;
};

class NodeWhileExpr: public NodeTwoOP
{
public:
	NodeWhileExpr();
	virtual ~NodeWhileExpr();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeWhileExpr; }
protected:
};

class NodeDoWhileExpr: public NodeTwoOP
{
public:
	NodeDoWhileExpr();
	virtual ~NodeDoWhileExpr();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeDoWhileExpr; }
protected:
};

class NodeBreakOp: public NodeZeroOP
{
public:
	NodeBreakOp(UINT c);
	virtual ~NodeBreakOp();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeBreakOp; }
protected:
	UINT	m_popCnt;
};

class NodeCaseExpr: public NodeTwoOP
{
public:
	NodeCaseExpr();
	virtual ~NodeCaseExpr();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeCaseExpr; }
protected:
};

class NodeSwitchExpr: public NodeThreeOP
{
public:
	NodeSwitchExpr();
	virtual ~NodeSwitchExpr();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNodeSwitchExpr; }
protected:
};

/*
void blockBegin(UNUSED);
void blockEnd(UNUSED);

void removeTop(UNUSED);
void popBackInIndexed(UNUSED);
*/

/*
class Node: public NodeOP
{
public:
	Node();
	virtual ~Node();

	virtual void doAct();
	virtual void doLog(ostringstream& ostr);
	virtual UINT getSize();
	virtual UINT getType(){ return typeNode; }
protected:
};*/