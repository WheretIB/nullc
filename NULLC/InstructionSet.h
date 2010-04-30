#pragma once
#ifndef NULLC_INSTRUCTIONSET_H
#define NULLC_INSTRUCTIONSET_H

#include "stdafx.h"

typedef unsigned char CmdID;

// Command definition file. For extended description, check out CmdRef.txt
// Файл определения значений команд. Для подробного описания их работы смотрите CmdRef.txt

// виртуальная машина имеет несколько стеков для переменных различного назначения:
// 1) основной стек. Сюда помещаются числа, адреса и производятся операции над ними.
// 2) стек переменных. Сюда помещаются значения активных переменных.
// 3) стек вызовов. Сохраняет указатели на код. Используется для возврата из функций.

const unsigned int CALL_BY_POINTER = (unsigned int)-1;
const unsigned char ADDRESS_ABOLUTE = 0;
const unsigned char ADDRESS_RELATIVE = 1;

enum InstructionCode
{
	// no operation
	// отсутствие операции
	cmdNop = 0,

	// push a number on top of general stack
	// положить значение на верхушку стека
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
	// скопировать значение с верхушки стека в память где располагаются переменные
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

	// removes a number of bytes from top
	// убрать заданное кол-во байт со стека с верхушки стека
	cmdPop,

	// Number conversion on the top of the stack
	// Преобразование чисел на вершине стека
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
	// скопировать значение на верхушке стека и добавить его в стек
	cmdCopyDorL,
	cmdCopyI,

	// get variable address, shifted by the parameter stack base
	// получить адрес переменной, относительно базы стека переменных
	cmdGetAddr,

	// get function address
	// получить адрес функции
	cmdFuncAddr,

	// set value to a range of memory. Starting address is given, count pushed before
	// установить значение участку памяти. Начальная позиция задана, количество - на вершине стека
	cmdSetRange,

	// unconditional jump
	// безусловный переход
	cmdJmp,

	// jump on zero
	// переход, если значение на вершине == 0
	cmdJmpZ,

	// jump on not zero
	// переход, если значение на вершине != 0
	cmdJmpNZ,

	// call function
	// вызов функции
	cmdCall,
	cmdCallPtr,

	// return from function
	// возврат из функции и выполнение POPT n раз, где n идёт за командой
	cmdReturn,

	// commands for work with variable stack
	// команды для работы со стеком переменных
		// save active variable count to "top value" stack
		// сохранить количество активных переменных в стек вершин стека переменных
	cmdPushVTop,

	// binary commands
	// they take two top numbers (both the same type!), remove them from stack,
	// perform operation and place result on top of stack (the same type as two values!)
	// бинарные операции
	// они берут два значения с вершины стека переменных (одинакового типа!), убирают их из стека
	// производят операцию и помещают результат в стек переменных (такого-же типа, как исходные переменные)
		
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
	cmdBitAnd,	// a & b	binary AND/бинарное И
	cmdBitOr,	// a | b	binary OR/бинарное ИЛИ
	cmdBitXor,	// a ^ b	binary XOR/бинарное Исключающее ИЛИ
	cmdLogAnd,	// a && b	logical AND/логическое И
	cmdLogOr,	// a || b	logical OR/логическое ИЛИ
	cmdLogXor,	// a ^^ b	logical XOR/логическое Исключающее ИЛИ

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
	cmdBitAndL,	// a & b	binary AND/бинарное И
	cmdBitOrL,	// a | b	binary OR/бинарное ИЛИ
	cmdBitXorL,	// a ^ b	binary XOR/бинарное Исключающее ИЛИ
	cmdLogAndL,	// a && b	logical AND/логическое И
	cmdLogOrL,	// a || b	logical OR/логическое ИЛИ
	cmdLogXorL,	// a ^^ b	logical XOR/логическое Исключающее ИЛИ

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
	// унарные операции [используется флаг операции, смотрите CmdRef.txt]
	// они меняют значение на вершине стека
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
	"Call", "CallPtr", "Return",
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
	"PushPtr", "PushPtrStk", "PushPtrImmt"
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
			curr += sprintf(curr, " Function id: %d", argument);
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
// Типы значений в стеке
enum asmStackType
{
	STYPE_INT,
	STYPE_LONG,
	STYPE_COMPLEX_TYPE,
	STYPE_DOUBLE,
	STYPE_FORCE_DWORD = 1<<30,
};
// Types of values on variable stack
// Типы значений в стеке переменных
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
// Типы операция (для флага операции)
enum asmOperType
{
	OTYPE_DOUBLE,
	OTYPE_COMPLEX,
	OTYPE_LONG,
	OTYPE_INT,
};

static int		stackTypeSize[] = { 4, 8, -1, 8 };
// Conversion of asmStackType to appropriate asmOperType
// Преобразование asmStackType в подходящий asmOperType
static asmOperType operTypeForStackType[] = { OTYPE_INT, OTYPE_LONG, OTYPE_COMPLEX, OTYPE_DOUBLE };

// Conversion of asmStackType to appropriate asmDataType
// Преобразование asmStackType в подходящий asmDataType
static asmDataType dataTypeForStackType[] = { DTYPE_INT, DTYPE_LONG, (asmDataType)0, DTYPE_DOUBLE };

// Conversion of asmDataType to appropriate asmStackType
// Преобразование asmDataType в подходящий asmStackType
static asmStackType stackTypeForDataTypeArr[] = { STYPE_INT, STYPE_INT, STYPE_INT, STYPE_LONG, STYPE_DOUBLE, STYPE_DOUBLE, STYPE_COMPLEX_TYPE };
__forceinline asmStackType stackTypeForDataType(asmDataType dt){ return stackTypeForDataTypeArr[dt/4]; }

static InstructionCode cmdPushType[] = { cmdPushChar, cmdPushShort, cmdPushInt, cmdPushDorL, cmdPushFloat, cmdPushDorL, cmdPushCmplx };
static InstructionCode cmdPushTypeStk[] = { cmdPushCharStk, cmdPushShortStk, cmdPushIntStk, cmdPushDorLStk, cmdPushFloatStk, cmdPushDorLStk, cmdPushCmplxStk };

static InstructionCode cmdMovType[] = { cmdMovChar, cmdMovShort, cmdMovInt, cmdMovDorL, cmdMovFloat, cmdMovDorL, cmdMovCmplx };
static InstructionCode cmdMovTypeStk[] = { cmdMovCharStk, cmdMovShortStk, cmdMovIntStk, cmdMovDorLStk, cmdMovFloatStk, cmdMovDorLStk, cmdMovCmplxStk };

static InstructionCode cmdIncType[] = { cmdIncD, cmdNop, cmdIncL, cmdIncI };
static InstructionCode cmdDecType[] = { cmdDecD, cmdNop, cmdDecL, cmdDecI };

// Для cmdIncAt и cmdDecAt
const unsigned int	bitPushBefore = 1;	// положить значение в стек до изменения
const unsigned int	bitPushAfter = 2;	// положить значение в стек после изменения

// constants for RetFlag creation from different bits
// константы для создания флага возврата из отдельных битов
const unsigned int	bitRetError		= 1 << 7;	// пользователь забыл возвратить значение, остановить выполнение
const unsigned int	bitRetSimple	= 1 << 15;	// пользователь забыл возвратить значение, остановить выполнение

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

		unsigned int	byteCodePos;	// Позиция в байткоде, к которой относится строка
		const char		*sourcePos;
	};
public:
	SourceInfo()
	{
		sourceStart = NULL;
	}

	void	Reset()
	{
		sourceInfo.reset();
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

	const char *sourceStart;
	FastVector<SourceLine>	sourceInfo;	// Список строк к коду
};

#endif
