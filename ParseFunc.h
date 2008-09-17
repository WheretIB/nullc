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
const UINT typeNodePreValOp		= 10;

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

const UINT typeNodeNumber		= 27;
const UINT typeNodeUnaryOp		= 28;
const UINT typeNodeFuncParam	= 29;
const UINT typeNodePushShift	= 30;
const UINT typeNodeGetAddress	= 31;
const UINT typeNodePushAddress	= 32;
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

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeOneOp; }
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
protected:
	shared_ptr<NodeZeroOP>	third;
};

//Zero child operators
template<typename T>
class NodeNumber: public NodeZeroOP
{
	typedef T NumType;
public:
	NodeNumber(NumType number, TypeInfo* ptrType){ num = number; typeInfo = ptrType; }
	virtual ~NodeNumber(){}

	virtual void Compile()
	{
		GetCommandList()->AddData(cmdPush);
		GetCommandList()->AddData((USHORT)(GetAsmStackType<T>() | GetAsmDataType<T>()));
		GetCommandList()->AddData((T)num);
	}
	virtual void LogToStream(ostringstream& ostr){ drawLn(ostr); ostr << *typeInfo << "Number " << num << "\r\n"; }
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

class NodeGetAddress: public NodeZeroOP
{
public:
	NodeGetAddress(VariableInfo vInfo, UINT varAddress);
	virtual ~NodeGetAddress();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeGetAddress; }
protected:
	VariableInfo	varInfo;
	UINT			varAddress;
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
	NodeFuncDef(UINT id);
	virtual ~NodeFuncDef();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeFuncDef; }
protected:
	UINT	funcID;
};

class NodeFuncParam: public NodeOneOP
{
public:
	NodeFuncParam(TypeInfo* tinfo, int paramIndex, bool funcStd);
	virtual ~NodeFuncParam();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeFuncParam; }
protected:
	UINT	idParam;
	bool	stdFunction;
};

class NodeFuncCall: public NodeOneOP
{
public:
	NodeFuncCall(std::string name, UINT id, UINT argCnt, TypeInfo* retType);
	virtual ~NodeFuncCall();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeFuncCall; }
protected:
	std::string	funcName;
	UINT		funcID;
};

class NodePushShift: public NodeOneOP
{
public:
	NodePushShift(int varSizeOf);
	virtual ~NodePushShift();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodePushShift; }
protected:
	int sizeOfType;
};

class NodeVarGet: public NodeOneOP
{
public:
	NodeVarGet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddress, bool shiftAddress, bool absAddress);
	virtual ~NodeVarGet();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeVarGet; }
protected:
	VariableInfo	varInfo;
	UINT			varAddress;
	bool			absAddress, shiftAddress, bakedShift;
};

class NodePreValOp: public NodeOneOP
{
public:
	NodePreValOp(VariableInfo vInfo, TypeInfo* targetType, UINT varAddress, bool shiftAddress, bool absAddress, CmdID cmd, bool preOp);
	virtual ~NodePreValOp();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodePreValOp; }

	void		 SetOptimised(bool doOptimisation);
protected:
	VariableInfo	varInfo;
	UINT			varAddress;
	bool			arrSetAll, absAddress, shiftAddress, prefixOperator, optimised, bakedShift;
	CmdID			cmdID;
};

//Two child operators
class NodeVarSet: public NodeTwoOP
{
public:
	NodeVarSet(VariableInfo vInfo, TypeInfo* targetType, UINT varAddress, bool shiftAddress, bool arrSetAll, bool absAddress, UINT pushBytes);
	virtual ~NodeVarSet();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeVarSet; }
protected:
	VariableInfo	varInfo;
	UINT			varAddress, bytesToPush;
	bool			arrSetAll, absAddress, shiftAddress, bakedShift;
};

class NodeVarSetAndOp: public NodeTwoOP
{
public:
	NodeVarSetAndOp(VariableInfo vInfo, TypeInfo* targetType, UINT varAddress, bool shiftAddress, bool absAddress, CmdID cmd);
	virtual ~NodeVarSetAndOp();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeVarSetAndOp; }
protected:
	VariableInfo	varInfo;
	UINT			varAddress;
	bool			absAddress, shiftAddress, bakedShift;
	CmdID			cmdID;
};

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
};

class NodeTwoExpression: public NodeTwoOP
{
public:
	NodeTwoExpression();
	virtual ~NodeTwoExpression();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeTwoExpression; }
protected:
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

class NodeCaseExpr: public NodeTwoOP
{
public:
	NodeCaseExpr();
	virtual ~NodeCaseExpr();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeCaseExpr; }
protected:
};

class NodeSwitchExpr: public NodeThreeOP
{
public:
	NodeSwitchExpr();
	virtual ~NodeSwitchExpr();

	virtual void Compile();
	virtual void LogToStream(ostringstream& ostr);
	virtual UINT GetSize();
	virtual UINT GetNodeType(){ return typeNodeSwitchExpr; }
protected:
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