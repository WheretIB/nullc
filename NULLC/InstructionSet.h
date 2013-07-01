#pragma once
#ifndef NULLC_INSTRUCTIONSET_H
#define NULLC_INSTRUCTIONSET_H

#include "stdafx.h"

typedef unsigned char CmdID;

// Command definition file. For extended description, check out CmdRef.txt

// Virtual machine has different stacks for variables of different kind:
// 1) temporary stack. Literal values are placed in it, and various operations are performed on them
// 2) variable stack. Global variables and local function variables are placed in it
// 3) call stack. Contains pointers to code, used to return from functions

const unsigned int CALL_BY_POINTER = (unsigned int)-1;
const unsigned int EXTERNAL_FUNCTION = (unsigned int)-1;
const unsigned char ADDRESS_ABOLUTE = 0;
const unsigned char ADDRESS_RELATIVE = 1;

enum InstructionCode
{
	// No operation
	cmdNop = 0,

	// Push a number on top of temporary stack
	cmdPushChar,
	cmdPushShort,
	cmdPushInt,
	cmdPushFloat,
	cmdPushDorL,
	cmdPushCmplx,

	// Push a number on top of temporary stack using an address from the top of the temporary stack
	cmdPushCharStk,
	cmdPushShortStk,
	cmdPushIntStk,
	cmdPushFloatStk,
	cmdPushDorLStk,
	cmdPushCmplxStk,

	// Push an immediate dword on the temporary stack
	cmdPushImmt,

	// Copy number from top of the temporary stack to a location in variable stack
	cmdMovChar,
	cmdMovShort,
	cmdMovInt,
	cmdMovFloat,
	cmdMovDorL,
	cmdMovCmplx,

	// Copy a number from top of the temporary stack to the memory location from the top of the temporary stack
	cmdMovCharStk,
	cmdMovShortStk,
	cmdMovIntStk,
	cmdMovFloatStk,
	cmdMovDorLStk,
	cmdMovCmplxStk,

	// Removes a number of bytes from the top of temporary stack
	cmdPop,

	// Number conversion on the top of the stack
	cmdDtoI,	// double to int
	cmdDtoL,	// double to long
	cmdDtoF,	// double to float
	cmdItoD,	// int to double
	cmdLtoD,	// long to double
	cmdItoL,	// int to long
	cmdLtoI,	// long to int

	// Multiples integer on top of the temporary stack by some number (indexes array) and performs value bound check
	cmdIndex,
	// Same, but the array size is on the temporary stack
	cmdIndexStk,

	// Copy value on top of the temporary stack, and push it on the top again
	cmdCopyDorL,
	cmdCopyI,

	// Get variable address, shifted by the base of the current variable stack frame
	cmdGetAddr,

	// Get function address
	cmdFuncAddr,

	// Set value to a range of memory. Starting address is given, count pushed before
	cmdSetRange,

	// Unconditional jump
	cmdJmp,

	// Jump on zero
	cmdJmpZ,

	// Jump on not zero
	cmdJmpNZ,

	// Call function
	cmdCall,
	cmdCallPtr,

	// Return from function
	cmdReturn,
	// Yield from function
	cmdYield,

	// Commands for work with variable stack

	// Create variable stack frame of the specified size
	cmdPushVTop,

	// Binary commands remove two numbers of the same type from the top of the temporary stack, perform the required operation and place the result on top of the stack

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
	cmdBitAnd,	// a & b	binary AND
	cmdBitOr,	// a | b	binary OR
	cmdBitXor,	// a ^ b	binary XOR
	cmdLogAnd,	// a && b	logical AND
	cmdLogOr,	// a || b	logical OR
	cmdLogXor,	// a ^^ b	logical XOR

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
	cmdBitAndL,	// a & b	binary AND
	cmdBitOrL,	// a | b	binary OR
	cmdBitXorL,	// a ^ b	binary XOR
	cmdLogAndL,	// a && b	logical AND
	cmdLogOrL,	// a || b	logical OR
	cmdLogXorL,	// a ^^ b	logical XOR

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

	// Unary commands	[using operation flag. check CmdRef.txt]
	// they take one value from top of the stack, and replace it with resulting value
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

	cmdCreateClosure,
	cmdCloseUpvals,

	cmdPushTypeID,
	cmdConvertPtr,

	cmdPushPtr,
	cmdPushPtrStk,
	cmdPushPtrImmt,

	cmdCheckedRet,

	cmdEnumCount,
};

static const char *vmInstructionText[] =
{
	"Nop",
	"PushChar", "PushShort", "PushInt", "PushFloat", "PushDorL", "PushCmplx",
	"PushCharStk", "PushShortStk", "PushIntStk", "PushFloatStk", "PushDorLStk", "PushCmplxStk",
	"PushImmt",
	"MovChar", "MovShort", "MovInt", "MovFloat", "MovDorL", "MovCmplx",
	"MovCharStk", "MovShortStk", "MovIntStk", "MovFloatStk", "MovDorLStk", "MovCmplxStk",
	"Pop",
	"DtoI", "DtoL", "DtoF", "ItoD", "LtoD", "ItoL", "LtoI",
	"Index", "IndexStk",
	"CopyDorL", "CopyI",
	"GetAddr", "FuncAddr", "SetRange",
	"Jmp", "JmpZ", "JmpNZ",
	"Call", "CallPtr", "Return", "Yield",
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
	"CreateClosure", "CloseUpvals",
	"PushTypeID", "ConvertPtr",
	"PushPtr", "PushPtrStk", "PushPtrImmt",
	"CheckedRet"
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
		case cmdPushPtr:

		case cmdPushCharStk:
		case cmdPushShortStk:
		case cmdPushIntStk:
		case cmdPushFloatStk:
		case cmdPushDorLStk:
		case cmdPushCmplxStk:
		case cmdPushPtrStk:
			curr += sprintf(curr, " [%d%s] sizeof(%d)", argument, flag ? " + base" : "", helper);
			break;

		case cmdPushImmt:
		case cmdPushPtrImmt:
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

		case cmdPop:
		case cmdIndex:
		case cmdIndexStk:
		case cmdGetAddr:
		case cmdFuncAddr:
		case cmdPushVTop:
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
			curr += sprintf(curr, " Function id: %d ret size: %d", argument, helper);
			break;

		case cmdCallPtr:
			curr += sprintf(curr, " Param size: %d ret size: %d", argument, helper);
			break;

		case cmdReturn:
			curr += sprintf(curr, " %s flag: %d sizeof: %d", helper ? "local" : "global", (int)flag, argument);
			break;

		case cmdCreateClosure:
			curr += sprintf(curr, " Function id: %d Previous closure at %d", argument, helper);
			break;
		case cmdCloseUpvals:
			curr += sprintf(curr, " Function id: %d Stack offset: %d", helper, argument);
			break;

		case cmdPushTypeID:
			curr += sprintf(curr, " %d", argument);
			break;
		case cmdConvertPtr:
			curr += sprintf(curr, " Type id: %d", argument);
			break;

		case cmdYield:
			curr += sprintf(curr, " %s %s (context at %d)", flag ? "Restore" : "Save", flag ? " " : (helper ? "Yield" : "Return"), argument);
			break;
		}
		return (int)(curr-buf);
	}

	CmdID	cmd;
	unsigned char	flag;		// rarely used (cmdIncAt, cmdDecAt)
	unsigned short	helper;		// rarely used (cmdPushCmplx*, cmdMovCmplx*, cmdPopCmplxTop)
	unsigned int	argument;
};

// Types of values on temporary stack
enum asmStackType
{
	STYPE_INT,
	STYPE_LONG,
	STYPE_COMPLEX_TYPE,
	STYPE_DOUBLE,
	STYPE_FORCE_DWORD = 1<<30,
};

// Types of values on variable stack
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
enum asmOperType
{
	OTYPE_DOUBLE,
	OTYPE_COMPLEX,
	OTYPE_LONG,
	OTYPE_INT,
};

static int		stackTypeSize[] = { 4, 8, -1, 8 };

// Conversion of asmStackType to appropriate asmOperType
static asmOperType operTypeForStackType[] = { OTYPE_INT, OTYPE_LONG, OTYPE_COMPLEX, OTYPE_DOUBLE };

// Conversion of asmStackType to appropriate asmDataType
static asmDataType dataTypeForStackType[] = { DTYPE_INT, DTYPE_LONG, (asmDataType)0, DTYPE_DOUBLE };

// Conversion of asmDataType to appropriate asmStackType
static asmStackType stackTypeForDataTypeArr[] = { STYPE_INT, STYPE_INT, STYPE_INT, STYPE_LONG, STYPE_DOUBLE, STYPE_DOUBLE, STYPE_COMPLEX_TYPE };
__forceinline asmStackType stackTypeForDataType(asmDataType dt){ return stackTypeForDataTypeArr[dt / 4]; }

static InstructionCode cmdPushType[] = { cmdPushChar, cmdPushShort, cmdPushInt, cmdPushDorL, cmdPushFloat, cmdPushDorL, cmdPushCmplx };
static InstructionCode cmdPushTypeStk[] = { cmdPushCharStk, cmdPushShortStk, cmdPushIntStk, cmdPushDorLStk, cmdPushFloatStk, cmdPushDorLStk, cmdPushCmplxStk };

static InstructionCode cmdMovType[] = { cmdMovChar, cmdMovShort, cmdMovInt, cmdMovDorL, cmdMovFloat, cmdMovDorL, cmdMovCmplx };
static InstructionCode cmdMovTypeStk[] = { cmdMovCharStk, cmdMovShortStk, cmdMovIntStk, cmdMovDorLStk, cmdMovFloatStk, cmdMovDorLStk, cmdMovCmplxStk };

static InstructionCode cmdIncType[] = { cmdIncD, cmdNop, cmdIncL, cmdIncI };
static InstructionCode cmdDecType[] = { cmdDecD, cmdNop, cmdDecL, cmdDecI };

// Constants for RetFlag creation from different bits
const unsigned int	bitRetError		= 1 << 7;	// User forgot to return a value, abort execution
const unsigned int	bitRetSimple	= 1 << 15;	// Function returns a simple value

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

		unsigned int	byteCodePos;	// Position in bytecode that corresponds with the source line
		const char		*sourcePos;
	};
public:
	SourceInfo()
	{
		sourceStart = NULL;
		sourceEnd = NULL;
	}

	void	Reset()
	{
		sourceInfo.reset();
	}

	void	SetSourceStart(const char* start, const char* end)
	{
		sourceStart = start;
		sourceEnd = end;
	}
	void	Clear()
	{
		sourceInfo.clear();
	}
	void		AddDescription(unsigned int instructionNum, const char* pos)
	{
		if(pos >= sourceStart && pos <= sourceEnd)
			sourceInfo.push_back(SourceLine(instructionNum, pos));
	}

	const char *sourceStart;
	const char *sourceEnd;
	FastVector<SourceLine>	sourceInfo;	// Array of source code lines
};

#endif
