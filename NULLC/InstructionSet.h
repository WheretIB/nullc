#pragma once
#include "stdafx.h"

typedef unsigned char CmdID;

// Command definition file. For extended description, check out CmdRef.txt
// ���� ����������� �������� ������. ��� ���������� �������� �� ������ �������� CmdRef.txt

// ����������� ������ ����� ��������� ������ ��� ���������� ���������� ����������:
// 1) �������� ����. ���� ���������� �����, ������ � ������������ �������� ��� ����.
// 1a) ���� ���� ���������� �������������� ���� ���������� � �������� �����. ������������ ��� �������,
//		� ����� ��� ����������� ���� ���������� ��� ������ ���������� �������.
// 2) ���� ����������. ���� ���������� �������� �������� ����������.
// 3) ���� ��� �������� ���������� ���������� � ����� ����������. ������������ ��� �������� ���������� ���
//		������ �� ������� ���������.
// 4) ���� �������. ��������� ��������� �� ���. ������������ ��� �������� �� �������.

const unsigned int CALL_BY_POINTER = (unsigned int)-1;

enum InstructionCode
{
	// no operation
	// ���������� ��������
	cmdNop = -1,

	// push a number on top of general stack
	// �������� �������� �� �������� �����
	cmdPushCharAbs,
	cmdPushShortAbs,
	cmdPushIntAbs,
	cmdPushFloatAbs,
	cmdPushDorLAbs,
	cmdPushCmplxAbs,

	cmdPushCharRel,
	cmdPushShortRel,
	cmdPushIntRel,
	cmdPushFloatRel,
	cmdPushDorLRel,
	cmdPushCmplxRel,

	cmdPushCharStk,
	cmdPushShortStk,
	cmdPushIntStk,
	cmdPushFloatStk,
	cmdPushDorLStk,
	cmdPushCmplxStk,

	// push an immediate dword on the stack
	cmdPushImmt,

	// copy's number from top of stack to value in value stack
	// ����������� �������� � �������� ����� � ������ ��� ������������� ����������
	cmdMovCharAbs,
	cmdMovShortAbs,
	cmdMovIntAbs,
	cmdMovFloatAbs,
	cmdMovDorLAbs,
	cmdMovCmplxAbs,

	cmdMovCharRel,
	cmdMovShortRel,
	cmdMovIntRel,
	cmdMovFloatRel,
	cmdMovDorLRel,
	cmdMovCmplxRel,

	cmdMovCharStk,
	cmdMovShortStk,
	cmdMovIntStk,
	cmdMovFloatStk,
	cmdMovDorLStk,
	cmdMovCmplxStk,

	// pop a value from top of the stack to [address + value_top]
	// ����������� ������� �� ������ [address + value_top] � ������ �� �����
	cmdReserveV,

	cmdPopCharTop,
	cmdPopShortTop,
	cmdPopIntTop,
	cmdPopFloatTop,
	cmdPopDorLTop,
	cmdPopCmplxTop,

	// removes a number of bytes from top
	// ������ �������� ���-�� ���� �� ����� � �������� �����
	cmdPop,

	// Number conversion on the top of the stack
	// �������������� ����� �� ������� �����
	cmdDtoI,	// double to int
	cmdDtoL,	// double to long
	cmdDtoF,	// double to float
	cmdItoD,	// int to double
	cmdLtoD,	// long to double
	cmdItoL,	// int to long
	cmdLtoI,	// long to int

	// converts number on top of the stack to integer and multiples it by some number [int after instruction]
	// ������������ ����� �� ������� ����� � int � �������� �� ��������� ����� [int �� �����������]
	cmdImmtMulD,	// double on top of the stack
	cmdImmtMulL,	// long on top of the stack
	cmdImmtMulI,	// int on top of the stack

	// copy value on top of the stack, and push it on the top again
	// ����������� �������� �� �������� ����� � �������� ��� � ����
	cmdCopyDorL,
	cmdCopyI,

	// get variable address, shifted by the parameter stack base
	// �������� ����� ����������, ������������ ���� ����� ����������
	cmdGetAddr,

	// get function address
	// �������� ����� �������
	cmdFuncAddr,

	// set value to a range of memory. Starting address is given, count pushed before
	// ���������� �������� ������� ������. ��������� ������� ������, ���������� - �� ������� �����
	cmdSetRange,

	// unconditional jump
	// ����������� �������
	cmdJmp,

	// jump on zero
	// �������, ���� �������� �� ������� == 0
	cmdJmpZI,
	cmdJmpZD,
	cmdJmpZL,
	// jump on not zero
	// �������, ���� �������� �� ������� != 0
	cmdJmpNZI,
	cmdJmpNZD,
	cmdJmpNZL,

	// call script function
	// ����� �������, ����������� � �������
	cmdCall,
	// call standard function
	// ����� ����������� (����������) �������
	cmdCallStd,

	// return from function
	// ������� �� ������� � ���������� POPT n ���, ��� n ��� �� ��������
	cmdReturn,

	// commands for work with variable stack
	// ������� ��� ������ �� ������ ����������
		// save active variable count to "top value" stack
		// ��������� ���������� �������� ���������� � ���� ������ ����� ����������
	cmdPushVTop,
		// pop data from variable stack until last top position and remove last top position
		// ������ ������ �� ����� ���������� �� ����������� �������� ������� � ������ �������� ������� �� ����� �������� ������
	cmdPopVTop,
		// shift value stack top
		// �������� ��������� � ������� ���������� ���� � ���� ����������
	cmdPushV,

	// binary commands
	// they take two top numbers (both the same type!), remove them from stack,
	// perform operation and place result on top of stack (the same type as two values!)
	// �������� ��������
	// ��� ����� ��� �������� � ������� ����� ���������� (����������� ����!), ������� �� �� �����
	// ���������� �������� � �������� ��������� � ���� ���������� (������-�� ����, ��� �������� ����������)
		
	cmdAdd,		// a + b
	cmdSub,		// a - b
	cmdMul,		// a * b
	cmdDiv,		// a / b
	cmdPow,		// power(a, b) (**)
	cmdMod,		// a % b
	cmdLess,	// a < b
	cmdGreater,	// a > b
	cmdLEqual,	// a <= b
	cmdGEqual,	// a >= b
	cmdEqual,	// a == b
	cmdNEqual,	// a != b
	cmdShl,		// a << b
	cmdShr,		// a >> b
	cmdBitAnd,	// a & b	binary AND/�������� �
	cmdBitOr,	// a | b	binary OR/�������� ���
	cmdBitXor,	// a ^ b	binary XOR/�������� ����������� ���
	cmdLogAnd,	// a && b	logical AND/���������� �
	cmdLogOr,	// a || b	logical OR/���������� ���
	cmdLogXor,	// a logical_xor b	logical XOR/���������� ����������� ���

	cmdAddL,	// a + b
	cmdSubL,	// a - b
	cmdMulL,	// a * b
	cmdDivL,	// a / b
	cmdPowL,	// power(a, b) (**)
	cmdModL,	// a % b
	cmdLessL,	// a < b
	cmdGreaterL,// a > b
	cmdLEqualL,	// a <= b
	cmdGEqualL,	// a >= b
	cmdEqualL,	// a == b
	cmdNEqualL,	// a != b
	cmdShlL,	// a << b
	cmdShrL,	// a >> b
	cmdBitAndL,	// a & b	binary AND/�������� �
	cmdBitOrL,	// a | b	binary OR/�������� ���
	cmdBitXorL,	// a ^ b	binary XOR/�������� ����������� ���
	cmdLogAndL,	// a && b	logical AND/���������� �
	cmdLogOrL,	// a || b	logical OR/���������� ���
	cmdLogXorL,	// a logical_xor b	logical XOR/���������� ����������� ���

	cmdAddD,	// a + b
	cmdSubD,	// a - b
	cmdMulD,	// a * b
	cmdDivD,	// a / b
	cmdPowD,	// power(a, b) (**)
	cmdModD,	// a % b
	cmdLessD,	// a < b
	cmdGreaterD,// a > b
	cmdLEqualD,	// a <= b
	cmdGEqualD,	// a >= b
	cmdEqualD,	// a == b
	cmdNEqualD,	// a != b

	// unary commands	[using operation flag. check CmdRef.txt]
	// they take one value from top of the stack, and replace it with resulting value
	// ������� �������� [������������ ���� ��������, �������� CmdRef.txt]
	// ��� ������ �������� �� ������� �����
	cmdNeg,		// negation
	cmdBitNot,	// ~	binary NOT
	cmdLogNot,	// !	logical NOT

	cmdNegL,	// negation
	cmdBitNotL,	// ~	binary NOT
	cmdLogNotL,	// !	logical NOT

	cmdNegD,	// negation
	noBitNotD,	// not available for double
	cmdLogNotD,	// !	logical NOT

	cmdIncI,
	cmdIncD,
	cmdIncL,

	cmdDecI,
	cmdDecD,
	cmdDecL,

	cmdAddAtCharStk,
	cmdAddAtShortStk,
	cmdAddAtIntStk,
	cmdAddAtLongStk,
	cmdAddAtFloatStk,
	cmdAddAtDoubleStk,
};

static char *vmInstructionText[] =
{
	"Nop",
	"PushCharAbs", "PushShortAbs", "PushIntAbs", "PushFloatAbs", "PushDorLAbs", "PushCmplxAbs",
	"PushCharRel", "PushShortRel", "PushIntRel", "PushFloatRel", "PushDorLRel", "PushCmplxRel",
	"PushCharStk", "PushShortStk", "PushIntStk", "PushFloatStk", "PushDorLStk", "PushCmplxStk",
	"PushImmt",
	"MovCharAbs", "MovShortAbs", "MovIntAbs", "MovFloatAbs", "MovDorLAbs", "MovCmplxAbs",
	"MovCharRel", "MovShortRel", "MovIntRel", "MovFloatRel", "MovDorLRel", "MovCmplxRel",
	"MovCharStk", "MovShortStk", "MovIntStk", "MovFloatStk", "MovDorLStk", "MovCmplxStk",
	"ReserveV",
	"PopCharTop", "PopShortTop", "PopIntTop", "PopFloatTop", "PopDorLTop", "PopCmplxTop",
	"Pop",
	"DtoI", "DtoL", "DtoF", "ItoD", "LtoD", "ItoL", "LtoI",
	"ImmtMulD", "ImmtMulL", "ImmtMulI",
	"CopyDorL", "CopyI",
	"GetAddr", "FuncAddr", "SetRange",
	"Jmp", "JmpZI", "JmpZD", "JmpZL", "JmpNZI", "JmpNZD", "JmpNZL",
	"Call", "CallStd", "Return",
	"PushVTop", "PopVTop", "PushV",
	"Add", "Sub", "Mul", "Div", "Pow", "Mod", "Less", "Greater", "LEqual", "GEqual", "Equal", "NEqual",
	"Shl", "Shr", "BitAnd", "BitOr", "BitXor", "LogAnd", "LogOr", "LogXor",
	"AddL", "SubL", "MulL", "DivL", "PowL", "ModL", "LessL", "GreaterL", "LEqualL", "GEqualL", "EqualL", "NEqualL",
	"ShlL", "ShrL", "BitAndL", "BitOrL", "BitXorL", "LogAndL", "LogOrL", "LogXorL",
	"AddD", "SubD", "MulD", "DivD", "PowD", "ModD", "LessD", "GreaterD", "LEqualD", "GEqualD", "EqualD", "NEqualD",
	"Neg", "BitNot", "LogNot",
	"NegL", "BitNotL", "LogNotL",
	"NegD", "noBitNotD", "LogNotD",
	"IncI", "IncD", "IncL",
	"DecI", "DecD", "DecL",
	"AddAtCharStk", "AddAtShortStk", "AddAtIntStk", "AddAtLongStk", "AddAtFloatStk", "AddAtDoubleStk"
};

struct VMCmd
{
	VMCmd()
	{
		Set(0, 0, 0, 0);
	}
	explicit VMCmd(InstructionCode Cmd)
	{
		Set((CmdID)Cmd, 0, 0, 0);
	}
	VMCmd(InstructionCode Cmd, unsigned int Arg)
	{
		Set((CmdID)Cmd, 0, 0, Arg);
	}
	VMCmd(InstructionCode Cmd, unsigned short Helper, unsigned int Arg)
	{
		Set((CmdID)Cmd, 0, Helper, Arg);
	}
	VMCmd(InstructionCode Cmd, unsigned char Flag, unsigned short Helper, unsigned int Arg)
	{
		Set((CmdID)Cmd, Flag, Helper, Arg);
	}

	void Set(CmdID Cmd, unsigned char Flag, unsigned short Helper, unsigned int Arg)
	{
		cmd = Cmd;
		flag = Flag;
		helper = Helper;
		argument = Arg;
	}

	#ifdef NULLC_LOG_FILES
    int Decode(char *buf)
	{
		char *curr = buf;
		curr += sprintf(curr, "%s", vmInstructionText[cmd+1]);

		switch(cmd)
		{
		case cmdPushCharAbs:
		case cmdPushShortAbs:
		case cmdPushIntAbs:
		case cmdPushFloatAbs:
		case cmdPushDorLAbs:
		case cmdPushCmplxAbs:

		case cmdPushCharRel:
		case cmdPushShortRel:
		case cmdPushIntRel:
		case cmdPushFloatRel:
		case cmdPushDorLRel:
		case cmdPushCmplxRel:

		case cmdPushCharStk:
		case cmdPushShortStk:
		case cmdPushIntStk:
		case cmdPushFloatStk:
		case cmdPushDorLStk:
		case cmdPushCmplxStk:
			curr += sprintf(curr, " [%d] sizeof(%d)", argument, helper);
			break;

		case cmdPushImmt:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdMovCharAbs:
		case cmdMovShortAbs:
		case cmdMovIntAbs:
		case cmdMovFloatAbs:
		case cmdMovDorLAbs:
		case cmdMovCmplxAbs:

		case cmdMovCharRel:
		case cmdMovShortRel:
		case cmdMovIntRel:
		case cmdMovFloatRel:
		case cmdMovDorLRel:
		case cmdMovCmplxRel:

		case cmdMovCharStk:
		case cmdMovShortStk:
		case cmdMovIntStk:
		case cmdMovFloatStk:
		case cmdMovDorLStk:
		case cmdMovCmplxStk:
			curr += sprintf(curr, " [%d] sizeof(%d)", argument, helper);
			break;

		case cmdReserveV:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdPopCharTop:
		case cmdPopShortTop:
		case cmdPopIntTop:
		case cmdPopFloatTop:
		case cmdPopDorLTop:
		case cmdPopCmplxTop:
			curr += sprintf(curr, " [%d] sizeof(%d)", argument, helper);
			break;

		case cmdPop:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdImmtMulD:
		case cmdImmtMulL:
		case cmdImmtMulI:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdGetAddr:
			curr += sprintf(curr, " %d", argument);
			break;
		case cmdFuncAddr:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdSetRange:
			curr += sprintf(curr, " start: %d dtype: %d", argument, helper);
			break;

		case cmdJmp:
		case cmdJmpZI:
		case cmdJmpZD:
		case cmdJmpZL:
		case cmdJmpNZI:
		case cmdJmpNZD:
		case cmdJmpNZL:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdCall:
			curr += sprintf(curr, " id/address: %d helper: %d", argument, helper);
			break;

		case cmdCallStd:
			curr += sprintf(curr, " ID: %d", argument);
			break;

		case cmdReturn:
			curr += sprintf(curr, " flag: %d sizeof: %d popcnt: %d", (int)flag, helper, argument);
			break;

		case cmdPushV:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdAddAtCharStk:
		case cmdAddAtShortStk:
		case cmdAddAtIntStk:
		case cmdAddAtLongStk:
		case cmdAddAtFloatStk:
		case cmdAddAtDoubleStk:
			curr += sprintf(curr, " [stk + %d] flag: %d helper: %d", argument, (int)flag, helper);
			break;
		}
		return (int)(curr-buf);
	}
#endif

	CmdID	cmd;
	unsigned char	flag;		// rarely used (cmdIncAt, cmdDecAt)
	unsigned short	helper;		// rarely used (cmdPushCmplx*, cmdMovCmplx*, cmdPopCmplxTop)
	unsigned int	argument;
};

// Types of values on stack
// ���� �������� � �����
enum asmStackType
{
	STYPE_INT,
	STYPE_LONG,
	STYPE_COMPLEX_TYPE,
	STYPE_DOUBLE,
	STYPE_FORCE_DWORD = 1<<30,
};
// Types of values on variable stack
// ���� �������� � ����� ����������
enum asmDataType
{
	DTYPE_CHAR=0,
	DTYPE_SHORT=4,
	DTYPE_INT=8,
	DTYPE_LONG=12,
	DTYPE_FLOAT=16,
	DTYPE_DOUBLE=20,
	DTYPE_COMPLEX_TYPE=24,
	DTYPE_FORCE_DWORD = 1<<30,
};
// Type of operation (for operation flag)
// ���� �������� (��� ����� ��������)
enum asmOperType
{
	OTYPE_DOUBLE,
	OTYPE_FLOAT_DEPRECATED,
	OTYPE_LONG,
	OTYPE_INT,
};

static int		stackTypeSize[] = { 4, 8, -1, 8 };
// Conversion of asmStackType to appropriate asmOperType
// �������������� asmStackType � ���������� asmOperType
static asmOperType operTypeForStackType[] = { OTYPE_INT, OTYPE_LONG, (asmOperType)0, OTYPE_DOUBLE };

// Conversion of asmStackType to appropriate asmDataType
// �������������� asmStackType � ���������� asmDataType
static asmDataType dataTypeForStackType[] = { DTYPE_INT, DTYPE_LONG, (asmDataType)0, DTYPE_DOUBLE };

// Conversion of asmDataType to appropriate asmStackType
// �������������� asmDataType � ���������� asmStackType
static asmStackType stackTypeForDataTypeArr[] = { STYPE_INT, STYPE_INT, STYPE_INT, STYPE_LONG, STYPE_DOUBLE, STYPE_DOUBLE, STYPE_COMPLEX_TYPE };
__forceinline asmStackType stackTypeForDataType(asmDataType dt){ return stackTypeForDataTypeArr[dt/4]; }

static InstructionCode cmdPushTypeAbs[] = { cmdPushCharAbs, cmdPushShortAbs, cmdPushIntAbs, cmdPushDorLAbs, cmdPushFloatAbs, cmdPushDorLAbs, cmdPushCmplxAbs };
static InstructionCode cmdPushTypeRel[] = { cmdPushCharRel, cmdPushShortRel, cmdPushIntRel, cmdPushDorLRel, cmdPushFloatRel, cmdPushDorLRel, cmdPushCmplxRel };
static InstructionCode cmdPushTypeStk[] = { cmdPushCharStk, cmdPushShortStk, cmdPushIntStk, cmdPushDorLStk, cmdPushFloatStk, cmdPushDorLStk, cmdPushCmplxStk };

static InstructionCode cmdMovTypeAbs[] = { cmdMovCharAbs, cmdMovShortAbs, cmdMovIntAbs, cmdMovDorLAbs, cmdMovFloatAbs, cmdMovDorLAbs, cmdMovCmplxAbs };
static InstructionCode cmdMovTypeRel[] = { cmdMovCharRel, cmdMovShortRel, cmdMovIntRel, cmdMovDorLRel, cmdMovFloatRel, cmdMovDorLRel, cmdMovCmplxRel };
static InstructionCode cmdMovTypeStk[] = { cmdMovCharStk, cmdMovShortStk, cmdMovIntStk, cmdMovDorLStk, cmdMovFloatStk, cmdMovDorLStk, cmdMovCmplxStk };

static InstructionCode cmdPopTypeTop[] = { cmdPopCharTop, cmdPopShortTop, cmdPopIntTop, cmdPopDorLTop, cmdPopFloatTop, cmdPopDorLTop, cmdPopCmplxTop };

static InstructionCode cmdImmtMulType[] = { cmdImmtMulD, cmdNop, cmdImmtMulL, cmdImmtMulI };
static InstructionCode cmdJmpZType[] = { cmdJmpZD, cmdNop, cmdJmpZL, cmdJmpZI };
static InstructionCode cmdJmpNZType[] = { cmdJmpNZD, cmdNop, cmdJmpNZL, cmdJmpNZI };

static InstructionCode cmdIncType[] = { cmdIncD, cmdNop, cmdIncL, cmdIncI };
static InstructionCode cmdDecType[] = { cmdDecD, cmdNop, cmdDecL, cmdDecI };

static InstructionCode cmdAddAtTypeStk[] = { cmdAddAtCharStk, cmdAddAtShortStk, cmdAddAtIntStk, cmdAddAtLongStk, cmdAddAtFloatStk, cmdAddAtDoubleStk };

// ��� cmdIncAt � cmdDecAt
const unsigned int	bitPushBefore = 1;	// �������� �������� � ���� �� ���������
const unsigned int	bitPushAfter = 2;	// �������� �������� � ���� ����� ���������

// constants for RetFlag creation from different bits
// ��������� ��� �������� ����� �������� �� ��������� �����
const unsigned int	bitRetError		= 1;	// ������������ ����� ���������� ��������, ���������� ����������
const unsigned int	bitRetSimple	= 1 << 15;	// ������������ ����� ���������� ��������, ���������� ����������

const int	COMMANDE_LENGTH = 8;

class SourceInfo
{
	struct SourceLine
	{
		SourceLine(){}
		SourceLine(unsigned int position, unsigned int beginPos, unsigned int endPos)
		{
			byteCodePos = position;
			beginOffset = beginPos;
			endOffset = endPos;
		}

		unsigned int	byteCodePos;	// ������� � ��������, � ������� ��������� ������
		unsigned int	beginOffset, endOffset;
	};
public:
	SourceInfo(){ sourceStart = NULL; }

	void	SetSourceStart(const char *start)
	{
		sourceStart = start;
	}
	void	Clear()
	{
		sourceInfo.clear();
	}
	void		AddDescription(unsigned int position, const char* start, const char* end)
	{
		assert(sourceStart != NULL);
		sourceInfo.push_back(SourceLine(position, (unsigned int)(start - sourceStart), (unsigned int)(end - sourceStart)));
	}
	SourceLine*	GetDescription(unsigned int position)
	{
		for(int s = 0, e = (int)sourceInfo.size(); s != e; s++)
		{
			if(sourceInfo[s].byteCodePos == position)
				return &sourceInfo[s];
			if(sourceInfo[s].byteCodePos > position)
				return NULL;
		}
		return NULL;
	}
private:
	const char *sourceStart;
	FastVector<SourceLine>	sourceInfo;	// ������ ����� � ����
};
