#pragma once
#include "stdafx.h"

typedef unsigned char CmdID;

// Command definition file. For extended description, check out CmdRef.txt
// ���� ����������� �������� ������. ��� ���������� �������� �� ������ �������� CmdRef.txt

// ����������� ������ ����� ��������� ������ ��� ���������� ���������� ����������:
// 1) �������� ����. ���� ���������� �����, ������ � ������������ �������� ��� ����.
// 2) ���� ����������. ���� ���������� �������� �������� ����������.
// 3) ���� �������. ��������� ��������� �� ���. ������������ ��� �������� �� �������.

const unsigned int CALL_BY_POINTER = (unsigned int)-1;
const unsigned char ADDRESS_ABOLUTE = 0;
const unsigned char ADDRESS_RELATIVE = 1;

enum InstructionCode
{
	// no operation
	// ���������� ��������
	cmdNop = 0,

	// push a number on top of general stack
	// �������� �������� �� �������� �����
	cmdPushChar,
	cmdPushShort,
	cmdPushInt,
	cmdPushFloat,
	cmdPushDorL,
	cmdPushCmplx,

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
	cmdMovChar,
	cmdMovShort,
	cmdMovInt,
	cmdMovFloat,
	cmdMovDorL,
	cmdMovCmplx,

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

	// multiples integer on top of stack by some number (indexes array) and performs value bound check
	cmdIndex,
	// array size is on stack
	cmdIndexStk,

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
	cmdJmpZ,

	// jump on not zero
	// �������, ���� �������� �� ������� != 0
	cmdJmpNZ,

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
	cmdLogXor,	// a ^^ b	logical XOR/���������� ����������� ���

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
	cmdLogXorL,	// a ^^ b	logical XOR/���������� ����������� ���

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
	cmdNegL,
	cmdNegD,

	cmdBitNot,	// ~	binary NOT
	cmdBitNotL,

	cmdLogNot,	// !	logical NOT
	cmdLogNotL,

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
	"PushChar", "PushShort", "PushInt", "PushFloat", "PushDorL", "PushCmplx",
	"PushCharStk", "PushShortStk", "PushIntStk", "PushFloatStk", "PushDorLStk", "PushCmplxStk",
	"PushImmt",
	"MovChar", "MovShort", "MovInt", "MovFloat", "MovDorL", "MovCmplx",
	"MovCharStk", "MovShortStk", "MovIntStk", "MovFloatStk", "MovDorLStk", "MovCmplxStk",
	"ReserveV",
	"PopCharTop", "PopShortTop", "PopIntTop", "PopFloatTop", "PopDorLTop", "PopCmplxTop",
	"Pop",
	"DtoI", "DtoL", "DtoF", "ItoD", "LtoD", "ItoL", "LtoI",
	"Index", "IndexStk",
	"CopyDorL", "CopyI",
	"GetAddr", "FuncAddr", "SetRange",
	"Jmp", "JmpZ", "JmpNZ",
	"Call", "CallStd", "Return",
	"PushVTop",
	"Add", "Sub", "Mul", "Div", "Pow", "Mod", "Less", "Greater", "LEqual", "GEqual", "Equal", "NEqual",
	"Shl", "Shr", "BitAnd", "BitOr", "BitXor", "LogAnd", "LogOr", "LogXor",
	"AddL", "SubL", "MulL", "DivL", "PowL", "ModL", "LessL", "GreaterL", "LEqualL", "GEqualL", "EqualL", "NEqualL",
	"ShlL", "ShrL", "BitAndL", "BitOrL", "BitXorL", "LogAndL", "LogOrL", "LogXorL",
	"AddD", "SubD", "MulD", "DivD", "PowD", "ModD", "LessD", "GreaterD", "LEqualD", "GEqualD", "EqualD", "NEqualD",
	"Neg", "NegL", "NegD",
	"BitNot", "BitNotL",
	"LogNot", "LogNotL",
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
		curr += sprintf(curr, "%s", vmInstructionText[cmd]);

		switch(cmd)
		{
		case cmdPushChar:
		case cmdPushShort:
		case cmdPushInt:
		case cmdPushFloat:
		case cmdPushDorL:
		case cmdPushCmplx:

		case cmdPushCharStk:
		case cmdPushShortStk:
		case cmdPushIntStk:
		case cmdPushFloatStk:
		case cmdPushDorLStk:
		case cmdPushCmplxStk:
			curr += sprintf(curr, " [%d%s] sizeof(%d)", argument, flag ? " + base" : "", helper);
			break;

		case cmdPushImmt:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdMovChar:
		case cmdMovShort:
		case cmdMovInt:
		case cmdMovFloat:
		case cmdMovDorL:
		case cmdMovCmplx:

		case cmdMovCharStk:
		case cmdMovShortStk:
		case cmdMovIntStk:
		case cmdMovFloatStk:
		case cmdMovDorLStk:
		case cmdMovCmplxStk:
			curr += sprintf(curr, " [%d%s] sizeof(%d)", argument, flag ? " + base" : "", helper);
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
		case cmdIndex:
		case cmdIndexStk:
		case cmdGetAddr:
		case cmdFuncAddr:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdSetRange:
			curr += sprintf(curr, " start: %d dtype: %d", argument, helper);
			break;

		case cmdJmp:
		case cmdJmpZ:
		case cmdJmpNZ:
			curr += sprintf(curr, " %d", argument);
			break;

		case cmdCall:
			curr += sprintf(curr, " id/address: %d helper: %d", argument, helper);
			break;

		case cmdCallStd:
			curr += sprintf(curr, " ID: %d", argument);
			break;

		case cmdReturn:
			curr += sprintf(curr, " %s flag: %d sizeof: %d", helper ? "local" : "global", (int)flag, argument);
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
	OTYPE_COMPLEX,
	OTYPE_LONG,
	OTYPE_INT,
};

static int		stackTypeSize[] = { 4, 8, -1, 8 };
// Conversion of asmStackType to appropriate asmOperType
// �������������� asmStackType � ���������� asmOperType
static asmOperType operTypeForStackType[] = { OTYPE_INT, OTYPE_LONG, OTYPE_COMPLEX, OTYPE_DOUBLE };

// Conversion of asmStackType to appropriate asmDataType
// �������������� asmStackType � ���������� asmDataType
static asmDataType dataTypeForStackType[] = { DTYPE_INT, DTYPE_LONG, (asmDataType)0, DTYPE_DOUBLE };

// Conversion of asmDataType to appropriate asmStackType
// �������������� asmDataType � ���������� asmStackType
static asmStackType stackTypeForDataTypeArr[] = { STYPE_INT, STYPE_INT, STYPE_INT, STYPE_LONG, STYPE_DOUBLE, STYPE_DOUBLE, STYPE_COMPLEX_TYPE };
__forceinline asmStackType stackTypeForDataType(asmDataType dt){ return stackTypeForDataTypeArr[dt/4]; }

static InstructionCode cmdPushType[] = { cmdPushChar, cmdPushShort, cmdPushInt, cmdPushDorL, cmdPushFloat, cmdPushDorL, cmdPushCmplx };
static InstructionCode cmdPushTypeStk[] = { cmdPushCharStk, cmdPushShortStk, cmdPushIntStk, cmdPushDorLStk, cmdPushFloatStk, cmdPushDorLStk, cmdPushCmplxStk };

static InstructionCode cmdMovType[] = { cmdMovChar, cmdMovShort, cmdMovInt, cmdMovDorL, cmdMovFloat, cmdMovDorL, cmdMovCmplx };
static InstructionCode cmdMovTypeStk[] = { cmdMovCharStk, cmdMovShortStk, cmdMovIntStk, cmdMovDorLStk, cmdMovFloatStk, cmdMovDorLStk, cmdMovCmplxStk };

static InstructionCode cmdPopTypeTop[] = { cmdPopCharTop, cmdPopShortTop, cmdPopIntTop, cmdPopDorLTop, cmdPopFloatTop, cmdPopDorLTop, cmdPopCmplxTop };

static InstructionCode cmdIncType[] = { cmdIncD, cmdNop, cmdIncL, cmdIncI };
static InstructionCode cmdDecType[] = { cmdDecD, cmdNop, cmdDecL, cmdDecI };

static InstructionCode cmdAddAtTypeStk[] = { cmdAddAtCharStk, cmdAddAtShortStk, cmdAddAtIntStk, cmdAddAtLongStk, cmdAddAtFloatStk, cmdAddAtDoubleStk };

// ��� cmdIncAt � cmdDecAt
const unsigned int	bitPushBefore = 1;	// �������� �������� � ���� �� ���������
const unsigned int	bitPushAfter = 2;	// �������� �������� � ���� ����� ���������

// constants for RetFlag creation from different bits
// ��������� ��� �������� ����� �������� �� ��������� �����
const unsigned int	bitRetError		= 1 << 7;	// ������������ ����� ���������� ��������, ���������� ����������
const unsigned int	bitRetSimple	= 1 << 15;	// ������������ ����� ���������� ��������, ���������� ����������

const int	COMMANDE_LENGTH = 8;

class SourceInfo
{
	struct SourceLine
	{
		SourceLine(){}
		SourceLine(unsigned int position, const char *pos)
		{
			byteCodePos = position;
			sourcePos = pos;
		}

		unsigned int	byteCodePos;	// ������� � ��������, � ������� ��������� ������
		const char		*sourcePos, *sourceEnd;
		unsigned int	sourceLine;
	};
public:
	SourceInfo()
	{
		sourceStart = NULL;
	}

	void	SetSourceStart(const char *start)
	{
		sourceStart = start;
	}
	void	Clear()
	{
		sourceInfo.clear();
	}
	void		AddDescription(unsigned int instructionNum, const char* pos)
	{
		sourceInfo.push_back(SourceLine(instructionNum, pos));
	}
	void		FindLineNumbers()
	{
		for(unsigned int i = 0; i < sourceInfo.size(); i++)
		{
			SourceLine &curr = sourceInfo[i];
			curr.sourceEnd = curr.sourcePos;
			while(curr.sourcePos != sourceStart && *(curr.sourcePos-1) != '\n')
				curr.sourcePos--;
			while(*(curr.sourceEnd+1) != '\n' && *(curr.sourceEnd+1) != '\0')
				curr.sourceEnd++;
			if(*(curr.sourceEnd+1) != '\0')
				curr.sourceEnd++;
		}
	}

	const char *sourceStart;
	FastVector<SourceLine>	sourceInfo;	// ������ ����� � ����
};
