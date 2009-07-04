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
	cmdNop = 0,

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

struct VMCmd
{
	VMCmd()
	{
		Set(0, 0, 0, 0);
	}
	VMCmd(InstructionCode Cmd)
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

// Command list (bytecode)
// ������� ������ (�������)
class CommandList
{
	struct CodeInfo
	{
		CodeInfo(unsigned int position, const char* beginPos, const char* endPos)
		{
			byteCodePos = position;
			memcpy(info, beginPos, (endPos-beginPos < 128 ? endPos-beginPos : 127));
			info[(endPos-beginPos < 128 ? endPos-beginPos : 127)] = '\0';
			for(int i = 0; i < 128; i++)
				if(info[i] == '\n' || info[i] == '\r')
					info[i] = ' ';
		}

		unsigned int	byteCodePos;	// ������� � ��������, � ������� ��������� ������
		char	info[128];		// ��������� �� ������ � ����� ������
	};
public:
	CommandList()
	{

	}

	void		Clear()
	{
		codeInfo.clear();
	}
	void		AddDescription(unsigned int position, const char* start, const char* end)
	{
		codeInfo.push_back(CodeInfo(position, start, end));
	}
	const char*	GetDescription(unsigned int position)
	{
		for(int s = 0, e = (int)codeInfo.size(); s != e; s++)
		{
			if(codeInfo[s].byteCodePos == position)
				return codeInfo[s].info;
			if(codeInfo[s].byteCodePos > position)
				return NULL;
		}
		return NULL;
	}
	
#ifdef NULLC_LOG_FILES
    static void PrintCommandListing(ostream *logASM, VMCmd *cmdStream, VMCmd *cmdStreamEnd)
	{
		const char	*typeName[] = { "char", "short", "int", "float", "qword", "complex" };
		// different for cmdAddAt**
		const char	*typeNameAA[] = { "char", "short", "int", "long", "float", "double" };

		VMCmd *cmdStreamBase = cmdStream;
		while(cmdStream < cmdStreamEnd)
		{
			const VMCmd &cmd = *cmdStream;
			*logASM << cmdStream - cmdStreamBase << " ";
			switch(cmd.cmd)
			{
			case cmdNop:
				*logASM << "NOP\r\n";
				break;
			case cmdPushCharAbs:
			case cmdPushShortAbs:
			case cmdPushIntAbs:
			case cmdPushFloatAbs:
			case cmdPushDorLAbs:
				*logASM << "PUSH " << typeName[cmd.cmd-cmdPushCharAbs] << " [" << cmd.argument << "]";
				break;
			case cmdPushCmplxAbs:
				*logASM << "PUSH complex [" << cmd.argument << "] sizeof(" << cmd.helper << ")";
				break;

			case cmdPushCharRel:
			case cmdPushShortRel:
			case cmdPushIntRel:
			case cmdPushFloatRel:
			case cmdPushDorLRel:
				*logASM << "PUSH " << typeName[cmd.cmd-cmdPushCharRel] << " [rel + " << (int)cmd.argument << "]";
				break;
			case cmdPushCmplxRel:
				*logASM << "PUSH complex [rel + " << (int)cmd.argument << "] sizeof(" << cmd.helper << ")";
				break;

			case cmdPushCharStk:
			case cmdPushShortStk:
			case cmdPushIntStk:
			case cmdPushFloatStk:
			case cmdPushDorLStk:
				*logASM << "PUSH " << typeName[cmd.cmd-cmdPushCharStk] << " [stack + " << cmd.argument << "]";
				break;
			case cmdPushCmplxStk:
				*logASM << "PUSH complex [stack + " << cmd.argument << "] sizeof(" << cmd.helper << ")";
				break;

			case cmdPushImmt:
				*logASM << "PUSHIMMT " << cmd.argument;
				break;

			case cmdMovCharAbs:
			case cmdMovShortAbs:
			case cmdMovIntAbs:
			case cmdMovFloatAbs:
			case cmdMovDorLAbs:
				*logASM << "MOV " << typeName[cmd.cmd-cmdMovCharAbs] << " [" << cmd.argument << "]";
				break;
			case cmdMovCmplxAbs:
				*logASM << "MOV complex [" << cmd.argument << "] sizeof(" << cmd.helper << ")";
				break;

			case cmdMovCharRel:
			case cmdMovShortRel:
			case cmdMovIntRel:
			case cmdMovFloatRel:
			case cmdMovDorLRel:
				*logASM << "MOV " << typeName[cmd.cmd-cmdMovCharRel] << " [rel + " << (int)cmd.argument << "]";
				break;
			case cmdMovCmplxRel:
				*logASM << "MOV complex [rel + " << (int)cmd.argument << "] sizeof(" << cmd.helper << ")";
				break;

			case cmdMovCharStk:
			case cmdMovShortStk:
			case cmdMovIntStk:
			case cmdMovFloatStk:
			case cmdMovDorLStk:
				*logASM << "MOV " << typeName[cmd.cmd-cmdMovCharStk] << " [stack + " << cmd.argument << "]";
				break;
			case cmdMovCmplxStk:
				*logASM << "MOV complex [rel + " << cmd.argument << "] sizeof(" << cmd.helper << ")";
				break;

			case cmdReserveV:
				*logASM << "RESERVE " << cmd.argument;
				break;

			case cmdPopCharTop:
			case cmdPopShortTop:
			case cmdPopIntTop:
			case cmdPopFloatTop:
			case cmdPopDorLTop:
				*logASM << "POPTOP " << typeName[cmd.cmd-cmdPopCharTop] << " [top + " << cmd.argument << "]";
				break;
			case cmdPopCmplxTop:
				*logASM << "POPTOP complex [top + " << cmd.argument << "] sizeof(" << cmd.helper << ")";
				break;

			case cmdPop:
				*logASM << "POP " << cmd.argument;
				break;

			case cmdDtoI:
				*logASM << "DTOI";
				break;
			case cmdDtoL:
				*logASM << "DTOL";
				break;
			case cmdDtoF:
				*logASM << "DTOF";
				break;
			case cmdItoD:
				*logASM << "ITOD";
				break;
			case cmdLtoD:
				*logASM << "LTOD";
				break;
			case cmdItoL:
				*logASM << "ITOL";
				break;
			case cmdLtoI:
				*logASM << "LTOI";
				break;

			case cmdImmtMulD:
				*logASM << "IMMTMUL double " << cmd.argument;
				break;
			case cmdImmtMulL:
				*logASM << "IMMTMUL long " << cmd.argument;
				break;
			case cmdImmtMulI:
				*logASM << "IMMTMUL int " << cmd.argument;
				break;

			case cmdCopyDorL:
				*logASM << "COPY qword";
				break;
			case cmdCopyI:
				*logASM << "COPY dword";
				break;

			case cmdGetAddr:
				*logASM << "GETADDR " << cmd.argument;
				break;
			case cmdFuncAddr:
				*logASM << "FUNCADDR " << cmd.argument;
				break;

			case cmdSetRange:
				*logASM << "SETRANGE start: " << cmd.argument << " dtype: " << cmd.helper;
				break;

			case cmdJmp:
				*logASM << "JMP " << cmd.argument;
				break;

			case cmdJmpZI:
				*logASM << "JMPZ int " << cmd.argument;
				break;
			case cmdJmpZD:
				*logASM << "JMPZ double " << cmd.argument;
				break;
			case cmdJmpZL:
				*logASM << "JMPZ long " << cmd.argument;
				break;

			case cmdJmpNZI:
				*logASM << "JMPNZ int " << cmd.argument;
				break;
			case cmdJmpNZD:
				*logASM << "JMPNZ double " << cmd.argument;
				break;
			case cmdJmpNZL:
				*logASM << "JMPNZ long " << cmd.argument;
				break;

			case cmdCall:
				*logASM << "CALL ID/address: " << cmd.argument << " helper: " << cmd.helper;
				break;

			case cmdCallStd:
				*logASM << "CALLSTD ID: " << cmd.argument;
				break;

			case cmdReturn:
				*logASM << "RET flag: " << (int)cmd.flag << " sizeof: " << cmd.helper << " popcnt: " << cmd.argument;
				break;

			case cmdPushVTop:
				*logASM << "PUSHT";
				break;
			case cmdPopVTop:
				*logASM << "POPT";
				break;

			case cmdPushV:
				*logASM << "PUSHV " << cmd.argument;
				break;

			case cmdAdd:
				*logASM << "ADD int";
				break;
			case cmdSub:
				*logASM << "SUB int";
				break;
			case cmdMul:
				*logASM << "MUL int";
				break;
			case cmdDiv:
				*logASM << "DIV int";
				break;
			case cmdPow:
				*logASM << "POW int";
				break;
			case cmdMod:
				*logASM << "MOD int";
				break;
			case cmdLess:
				*logASM << "LESS int";
				break;
			case cmdGreater:
				*logASM << "GREATER int";
				break;
			case cmdLEqual:
				*logASM << "LEQUAL int";
				break;
			case cmdGEqual:
				*logASM << "GEQUAL int";
				break;
			case cmdEqual:
				*logASM << "EQUAL int";
				break;
			case cmdNEqual:
				*logASM << "NEQUAL int";
				break;
			case cmdShl:
				*logASM << "SHL int";
				break;
			case cmdShr:
				*logASM << "SHR int";
				break;
			case cmdBitAnd:
				*logASM << "BAND int";
				break;
			case cmdBitOr:
				*logASM << "BOR int";
				break;
			case cmdBitXor:
				*logASM << "BXOR int";
				break;
			case cmdLogAnd:
				*logASM << "LAND int";
				break;
			case cmdLogOr:
				*logASM << "LOR int";
				break;
			case cmdLogXor:
				*logASM << "LXOR int";
				break;

			case cmdAddL:
				*logASM << "ADD long";
				break;
			case cmdSubL:
				*logASM << "SUB long";
				break;
			case cmdMulL:
				*logASM << "MUL long";
				break;
			case cmdDivL:
				*logASM << "DIV long";
				break;
			case cmdPowL:
				*logASM << "POW long";
				break;
			case cmdModL:
				*logASM << "MOD long";
				break;
			case cmdLessL:
				*logASM << "LESS long";
				break;
			case cmdGreaterL:
				*logASM << "GREATER long";
				break;
			case cmdLEqualL:
				*logASM << "LEQUAL long";
				break;
			case cmdGEqualL:
				*logASM << "GEQUAL long";
				break;
			case cmdEqualL:
				*logASM << "EQUAL long";
				break;
			case cmdNEqualL:
				*logASM << "NEQUAL long";
				break;
			case cmdShlL:
				*logASM << "SHL long";
				break;
			case cmdShrL:
				*logASM << "SHR long";
				break;
			case cmdBitAndL:
				*logASM << "BAND long";
				break;
			case cmdBitOrL:
				*logASM << "BOR long";
				break;
			case cmdBitXorL:
				*logASM << "BXOR long";
				break;
			case cmdLogAndL:
				*logASM << "LAND long";
				break;
			case cmdLogOrL:
				*logASM << "LOR long";
				break;
			case cmdLogXorL:
				*logASM << "LXOR long";
				break;

			case cmdAddD:
				*logASM << "ADD double";
				break;
			case cmdSubD:
				*logASM << "SUB double";
				break;
			case cmdMulD:
				*logASM << "MUL double";
				break;
			case cmdDivD:
				*logASM << "DIV double";
				break;
			case cmdPowD:
				*logASM << "POW double";
				break;
			case cmdModD:
				*logASM << "MOV double";
				break;
			case cmdLessD:
				*logASM << "LESS double";
				break;
			case cmdGreaterD:
				*logASM << "GREATER double";
				break;
			case cmdLEqualD:
				*logASM << "LEQUAL double";
				break;
			case cmdGEqualD:
				*logASM << "GEQUAL double";
				break;
			case cmdEqualD:
				*logASM << "EQUAL double";
				break;
			case cmdNEqualD:
				*logASM << "NEQUAL double";
				break;

			case cmdNeg:
				*logASM << "NEG int";
				break;
			case cmdBitNot:
				*logASM << "BNOT int";
				break;
			case cmdLogNot:
				*logASM << "LNOT int";
				break;

			case cmdNegL:
				*logASM << "NEG long";
				break;
			case cmdBitNotL:
				*logASM << "BNOT long";
				break;
			case cmdLogNotL:
				*logASM << "LNOT long";
				break;

			case cmdNegD:
				*logASM << "NEG double";
				break;
			case cmdLogNotD:
				*logASM << "LNOT double";
				break;
			
			case cmdIncI:
				*logASM << "INC int";
				break;
			case cmdIncD:
				*logASM << "INC double";
				break;
			case cmdIncL:
				*logASM << "INC long";
				break;

			case cmdDecI:
				*logASM << "DEC int";
				break;
			case cmdDecD:
				*logASM << "DEC double";
				break;
			case cmdDecL:
				*logASM << "DEC long";
				break;

			case cmdAddAtCharStk:
			case cmdAddAtShortStk:
			case cmdAddAtIntStk:
			case cmdAddAtLongStk:
			case cmdAddAtFloatStk:
			case cmdAddAtDoubleStk:
				*logASM << "ADDAT " << typeNameAA[cmd.cmd-cmdAddAtCharStk] << " [stk + " << cmd.argument << "] flag: " << (int)cmd.flag << " helper: " << cmd.helper;
				break;
			}
			*logASM << "\r\n";
			cmdStream++;
		}
	}
#endif
private:
	std::vector<CodeInfo>	codeInfo;	// ������ ����� � ����
};
