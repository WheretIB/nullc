#include "stdafx.h"
#pragma once

typedef short int CmdID;

// Command definition file. For extended description, check out CmdRef.txt
// Файл определения значений команд. Для подробного описания их работы смотрите CmdRef.txt

// виртуальная машина имеет несколько стеков для переменных различного назначения:
// 1) основной стек. Сюда помещаются числа, адреса и производятся операции над ними.
// 1a) стек куда помещаются идентификаторы типа переменных в основном стеке. Используется для отладки,
//		а также для определения типа переменной при вызове встроенных функций.
// 2) стек переменных. Сюда помещаются значения активных переменных.
// 3) стек для хранения количества переменных в стеке переменных. Используется для удаления переменных при
//		выходе из области видимости.
// 4) стек вызовов. Сохраняет указатели на код. Используется для возврата из функций.

	// no operation
	// :)
const CmdID cmdNop		= 0;

// general commands [using command flag. check CmdRef.txt]
// основные команды [используются флаг команды, смотрите CmdRef.txt]
	// push's a number on top of general stack
	// положить значение на верхушку стека
const CmdID cmdPush		= 100;
	// remove's a number from top
	// убрать значение с верхушки стека
const CmdID cmdPop		= 101;
	// copy's number from top of stack to value in value stack
	// скопировать значение с верхушки стека в память где располагаются переменные
const CmdID cmdMov		= 102;
	// add's a shift value to address in general stack, checking the lower and upper bound
	// добавить значение к адресу на верхушке стека, проведя clamp(адрес, минимум, максимум) после сложения
const CmdID cmdShift	= 103;
	// convert's real numbers to integer
	// преобразовать действительное число в целое (на вершине стека)
const CmdID cmdRTOI		= 104;
	// convert's integer numbers to real
	// преобразовать целое число в действительное (на вершине стека)
const CmdID cmdITOR		= 105;
	// convert's integer numbers to long
	// преобразовать int в long (на вершине стека)
const CmdID cmdITOL		= 106;
	// convert's long numbers to integer
	// преобразовать long в int (на вершине стека)
const CmdID cmdLTOI		= 107;
	// swap's two values on top of the general stack
	// поменять местами два значения на верхушке стека
const CmdID cmdSwap		= 108;
	// copy value on top of the stack, and push it on the top again
	// скопировать значение на верхушке стека и добавить его в стек
const CmdID cmdCopy		= 109;

// conditional and unconditional jumps  [using operation flag. check CmdRef.txt]
// условные и безусловные переходы [используется флаг операции, смотрите CmdRef.txt]
	// unconditional jump
	// безусловный переход
const CmdID cmdJmp		= 110;
	// jump on zero
	// переход, если значение на вершине == 0
const CmdID cmdJmpZ		= 111;
	// jump on not zero
	// переход, если значение на вершине != 0
const CmdID cmdJmpNZ	= 112;

// commands for functions	[not using flags]
// команды для функций	[флаги не используются]
	// call script function
	// вызов функции, определённой в скрипте
const CmdID cmdCall		= 113;
	// call standard function
	// вызов стандартных (встроенных) функций
const CmdID cmdCallStd	= 114;
	// return from function
	// возврат из функции
const CmdID cmdReturn	= 115;

// commands for work with variable stack	[not using flags]
// команды для работы со стеком переменных	[флаги не используются]
	// save active variable count to "top value" stack
	// сохранить количество активных переменных в стек вершин стека переменных
const CmdID cmdPushVTop	= 116;
	// pop data from variable stack until last top position and remove last top position
	// убрать данные из стека переменных до предыдущего значения вершины и убрать значение вершины из стека значений вершин
const CmdID cmdPopVTop	= 117;
	// shift value stack top
	// добавить указанное в команде количество байт в стек переменных
const CmdID cmdPushV	= 118;

// binary commands	[using operation flag. check CmdRef.txt]
// they take two top numbers (both the same type!), remove them from stack,
// perform operation and place result on top of stack (the same type as two values!)
// бинарные операции [используется флаг операции, смотрите CmdRef.txt]
// они берут два значения с вершины стека переменных (одинакового типа!), убирают их из стека
// производят операцию и помещают результат в стек переменных (такого-же типа, как исходные переменные)
	// a + b
const CmdID cmdAdd		= 130;
	// a - b
const CmdID cmdSub		= 131;
	// a * b
const CmdID cmdMul		= 132;
	// a / b 
const CmdID cmdDiv		= 133;
	// power(a, b) (**)
const CmdID cmdPow		= 134;
	// a % b
const CmdID cmdMod		= 135;
	// a < b
const CmdID cmdLess		= 136;
	// a > b
const CmdID cmdGreater	= 137;
	// a <= b
const CmdID cmdLEqual	= 138;
	// a >= b
const CmdID cmdGEqual	= 139;
	// a == b
const CmdID cmdEqual	= 140;
	// a != b
const CmdID cmdNEqual	= 141;

// the following commands work only with integer numbers
// следующие команды работают только с целочисленными переменными
	// a << b
const CmdID cmdShl		= 142;
	// a >> b
const CmdID cmdShr		= 143;
	// a & b	binary AND/бинарное И
const CmdID cmdBitAnd	= 144;
	// a | b	binary OR/бинарное ИЛИ
const CmdID cmdBitOr	= 145;
	// a ^ b	binary XOR/бинарное Исключающее ИЛИ
const CmdID cmdBitXor	= 146;
	// a && b	logical AND/логическое И
const CmdID cmdLogAnd	= 147;
	// a || b	logical OR/логическое ИЛИ
const CmdID cmdLogOr	= 148;
	// a logical_xor b	logical XOR/логическое Исключающее ИЛИ
const CmdID cmdLogXor	= 149;

// unary commands	[using operation flag. check CmdRef.txt]
// they take one value from top of the stack, and replace it with resulting value
// унарные операции [используется флаг операции, смотрите CmdRef.txt]
// они меняют значение на вершине стека
	// negation
	// отрицание
const CmdID cmdNeg		= 160;
	// increment
	// инкримент
const CmdID cmdInc		= 161;
	// decrement
	// декримент
const CmdID cmdDec		= 162;
	// ~	binary NOT | Only for integers! |
	// ~	бинарное отрицание | Только для целых чисел |
const CmdID cmdBitNot	= 163;
	// !	logical NOT
	// !	логическое НЕ
const CmdID cmdLogNot	= 164;

// Special unary commands	[using command flag. check CmdRef.txt]
// They work with variable at address
// Особые унарные операции	[используются флаг команды, смотрите CmdRef.txt]
// Изменяют значения сразу в стеке переменных
	// increment at address
	// инкримент значения по адресу
const CmdID cmdIncAt	= 171;
	// decrement at address
	// декримент значения по адресу
const CmdID cmdDecAt	= 172;

// Flag types
// Типы флагов
typedef unsigned short int CmdFlag;	// command flag
typedef unsigned char OperFlag;		// operation flag

// Types of values on stack
// Типы значений в стеке
enum asmStackType
{
	STYPE_INT,
	STYPE_LONG,
	STYPE_FLOAT_DEPRECATED,
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
	DTYPE_FORCE_DWORD = 1<<30,
};
// Type of operation (for operation flag)
// Типы операция (для флага операции)
enum asmOperType
{
	OTYPE_DOUBLE,
	OTYPE_FLOAT_DEPRECATED,
	OTYPE_LONG,
	OTYPE_INT,
};

// Conversion of asmStackType to appropriate asmOperType
// Преобразование asmStackType в подходящий asmOperType
static asmOperType operTypeForStackType[] = { OTYPE_INT, OTYPE_LONG, (asmOperType)0, OTYPE_DOUBLE };

// Conversion of asmStackType to appropriate asmDataType
// Преобразование asmStackType в подходящий asmDataType
static asmDataType dataTypeForStackType[] = { DTYPE_INT, DTYPE_LONG, (asmDataType)0, DTYPE_DOUBLE };

// Conversion of asmDataType to appropriate asmStackType
// Преобразование asmDataType в подходящий asmStackType
static asmStackType stackTypeForDataType[] = { STYPE_INT, STYPE_INT, STYPE_INT, STYPE_LONG, STYPE_DOUBLE, STYPE_DOUBLE };

// Functions for extraction of different bits from flags
// Функции для извлечения значений отдельных битов из флагов
	// extract asmStackType
	// извлечь asmStackType
__forceinline asmStackType	flagStackType(const CmdFlag& flag){ return (asmStackType)(flag&0x00000003); }
	// extract asmDataType
	// извлечь asmDataType
__forceinline asmDataType	flagDataType(const CmdFlag& flag){ return (asmDataType)(((flag>>2)&0x00000007)<<2); }//flag&0x00000003C); }
	// addressing is performed according to top value of variable stack
	// адресация производится относительно вершины стека переменных
__forceinline UINT			flagAddrRel(const CmdFlag& flag){ return (flag>>5)&0x00000001; }
	// addressing is perform using absolute address
	// адресация производится по абсолютному адресу
__forceinline UINT			flagAddrAbs(const CmdFlag& flag){ return (flag>>6)&0x00000001; }
	// address is placed in stack
	// адрес находится в основном стеке
__forceinline UINT			flagAddrStk(const CmdFlag& flag){ return (flag>>7)&0x00000001; }
	// shift is performed to the address
	// к адресу применяется сдвиг
__forceinline UINT			flagShiftOn(const CmdFlag& flag){ return (flag>>8)&0x00000001; }
	// shift is placed in stack
	// сдвиг находится в основном стеке
__forceinline UINT			flagShiftStk(const CmdFlag& flag){ return (flag>>9)&0x00000001; }
	// address is clamped to the maximum
	// адрес не может превыщать некоторое значение
__forceinline UINT			flagSizeOn(const CmdFlag& flag){ return (flag>>10)&0x00000001; }
	// maximum is placed in stack
	// максимуи находится в основном стеке
__forceinline UINT			flagSizeStk(const CmdFlag& flag){ return (flag>>11)&0x00000001; }
	// addressing is not performed
	// адресация отсутствует
__forceinline UINT			flagNoAddr(const CmdFlag& flag){ return !(flag&0x00000060); }

// constants for flag creation from different bits
// константы для создания флага из отдельных битов
const UINT	bitAddrRel	= 1 << 5;
const UINT	bitAddrAbs	= 1 << 6;
const UINT	bitAddrStk	= 1 << 7;
const UINT	bitShiftOn	= 1 << 8;
const UINT	bitShiftStk	= 1 << 9;
const UINT	bitSizeOn	= 1 << 10;
const UINT	bitSizeStk	= 1 << 11;

// Command list (bytecode)
// Листинг команд (байткод)
class CommandList
{
public:
	// создаём сразу место для будующих команд
	CommandList(UINT count = 65000)
	{
		bytecode = new char[count];
		max = count;
		curr = 0;
	}
	CommandList(const CommandList& r)
	{
		bytecode = new char[r.max];
		max = r.max;
		curr = 0;
	}
	~CommandList()
	{
		delete[] bytecode;
	}

	// добавить данные (это могут быть команды, адреса, флаги и числа)
	// значение одного типа
	template<class T> void AddData(const T& data)
	{
		memcpy(bytecode+curr, &data, sizeof(T));
		curr += sizeof(T);
	}
	// несколько произвольных байт
	void AddData(const void* data, size_t size)
	{
		memcpy(bytecode+curr, data, size);
		curr += (UINT)size;
	}

	// получить int по адресу
	__inline	bool GetINT(UINT pos, int& ret)
	{
		ret = *(reinterpret_cast<int*>(bytecode+pos));
		return true;
	}
	// получить short int по адресу
	__inline	bool GetSHORT(UINT pos, short int& ret)
	{
		if(pos+2 > curr)
			return false;
		ret = *(reinterpret_cast<short int*>(bytecode+pos));
		return true;
	}
	// получить unsigned short int по адресу
	__inline	bool GetUSHORT(UINT pos, unsigned short& ret)
	{
		ret = *(reinterpret_cast<unsigned short*>(bytecode+pos));
		return true;
	}
	// получить unsinged int по адресу
	__inline	bool GetUINT(UINT pos, UINT& ret)
	{
		ret = *(reinterpret_cast<UINT*>(bytecode+pos));
		return true;
	}
	// получить unsigned char по адресу
	__inline	bool GetUCHAR(UINT pos, unsigned char& ret)
	{
		ret = *(reinterpret_cast<unsigned char*>(bytecode+pos));
		return true;
	}

	// получить значение произвольного типа по адресу
	template<class T> __inline bool GetData(UINT pos, T& ret)
	{
		if(pos+sizeof(T) > curr)
			return false;
		memcpy(&ret, bytecode+pos, sizeof(T));
		return true;
	}
	// получить произвольное кол-во байт по адресу
	__inline bool	GetData(UINT pos, void* data, size_t size)
	{
		if(pos+size > curr)
			return false;
		memcpy(data, bytecode+pos, size);
		return true;
	}

	// заменить произвольное количество байт по адресу
	__inline void	SetData(UINT pos, const void* data, size_t size)
	{
		memcpy(bytecode+pos, data, size);
	}

	// получить текущий размер байткода
	__inline const UINT&	GetCurrPos()
	{
		return curr;
	}

	// удалить байткод
	__inline void	Clear()
	{
		curr = 0;
		memset(bytecode, 0, max);
	}
private:
	char*	bytecode;
	UINT	curr;
	UINT	max;
};

// распечатать инструкцию в читабельном виде в поток
static void PrintInstructionText(ostream* stream, CmdID cmd, UINT pos2, UINT valind, const CmdFlag cFlag, const OperFlag oFlag, UINT dw0=0, UINT dw1=0)
{
	asmStackType st = flagStackType(cFlag);
	asmDataType dt = flagDataType(cFlag);
	char*	typeInfoS[] = { "int", "long", "float", "double" };
	char*	typeInfoD[] = { "char", "short", "int", "long", "float", "double" };
	UINT	typeSizeS[] = { 4, 8, 4, 8 };
	UINT	typeSizeD[] = { 1, 2, 4, 8, 4, 8 };
	UINT	DWords[] = { dw0, dw1 };

	size_t beginPos = stream->tellp();
	(*stream) << pos2;
	char temp[32];
	UINT addSp = 5 - (UINT)strlen(_itoa(pos2, temp, 10));
	for(UINT i = 0; i < addSp; i++)
		(*stream) << ' ';
	switch(cmd)
	{
	case cmdPushVTop:
		(*stream) << " PUSHT " << valind << ";";
		break;
	case cmdPopVTop:
		(*stream) << " POPT " << valind << ";";
		break;
	case cmdCall:
		(*stream) << " CALL " << valind << ";";
		break;
	case cmdReturn:
		(*stream) << " RET " << ";";
		break;
	case cmdPushV:
		(*stream) << " PUSHV " << valind << ";";
		break;
	case cmdNop:
		(*stream) << " NOP;";
		break;
	case cmdPop:
		(*stream) << " POP ";
		(*stream) << typeInfoS[cFlag&0x00000003];
		break;
	case cmdRTOI:
		(*stream) << " RTOI ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
		break;
	case cmdITOR:
		(*stream) << " ITOR ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
		break;
	case cmdITOL:
		(*stream) << " ITOL";
		break;
	case cmdLTOI:
		(*stream) << " LTOI";
		break;
	case cmdSwap:
		(*stream) << " SWAP ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "<->";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007];
		break;
	case cmdCopy:
		(*stream) << " COPY ";
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double;";
			break;
		case OTYPE_LONG:
			(*stream) << " long;";
			break;
		case OTYPE_INT:
			(*stream) << " int;";
			break;
		}
		break;
	case cmdJmp:
		(*stream) << " JMP " << valind;
		break;
	case cmdJmpZ:
		(*stream) << " JMPZ";
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double ";
			break;
		case OTYPE_LONG:
			(*stream) << " long ";
			break;
		case OTYPE_INT:
			(*stream) << " int ";
			break;
		}
		(*stream) << valind << ';';
		break;
	case cmdJmpNZ:
		(*stream) << " JMPNZ";
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double ";
			break;
		case OTYPE_LONG:
			(*stream) << " long ";
			break;
		case OTYPE_INT:
			(*stream) << " int ";
			break;
		}
		(*stream) << valind << ';';
		break;
	case cmdMov:
		(*stream) << " MOV ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "->";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";

		(*stream) << valind << "] //";
		if(flagAddrStk(cFlag)){
			(*stream) << "stack";
			if(flagAddrRel(cFlag))
				(*stream) << "+top";
		}else{
			if(flagAddrRel(cFlag))
				(*stream) << "rel+top";

			if(flagShiftStk(cFlag)){
				(*stream) << "+shiftstk";
			}
			if(flagShiftOn(cFlag)){
				(*stream) << "+shifton";
			}
			if(flagShiftStk(cFlag) || flagShiftOn(cFlag))
			{
				(*stream) << "*" << typeSizeD[(cFlag>>2)&0x00000007];
			}
			//if(flagSizeStk(cFlag))
			//	(*stream) << " size: stack";
			//if(flagSizeOn(cFlag))
			//	(*stream) << " size: instr";
			//if(flagSizeStk(cFlag) || flagSizeOn(cFlag))
			//	(*stream) << "*" << typeSizeD[(cFlag>>2)&0x00000007];
		}
		break;
	case cmdPush:
		(*stream) << " PUSH ";
		(*stream) << typeInfoS[cFlag&0x00000003] << "<-";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007];

		if(flagNoAddr(cFlag))
		{
			//(*stream) << " (known number)";
			if(dt == DTYPE_DOUBLE)
				(*stream) << " (" << *((double*)(&DWords[0])) << ')';
			if(dt == DTYPE_LONG)
				(*stream) << " (" << *((long*)(&DWords[0])) << ')';
			if(dt == DTYPE_FLOAT)
				(*stream) << " (" << *((float*)(&DWords[1])) << ')';
			if(dt == DTYPE_INT)
				(*stream) << " (" << *((int*)(&DWords[1])) << ')';
			if(dt == DTYPE_SHORT)
				(*stream) << " (" << *((short*)(&DWords[1])) << ')';
			if(dt == DTYPE_CHAR)
				(*stream) << " (" << *((char*)(&DWords[1])) << ')';
		}else{
			(*stream) << " PTR[";
			(*stream) << valind << "] //";
			if(flagAddrStk(cFlag)){
				(*stream) << "stack";
				if(flagAddrRel(cFlag))
					(*stream) << "+top";
			}else{
				if(flagAddrRel(cFlag))
					(*stream) << "rel+top";
			}
			if(flagShiftStk(cFlag))
				(*stream) << "+shiftstk";
			if(flagShiftOn(cFlag))
				(*stream) << "+shifton";
			if(flagShiftStk(cFlag) || flagShiftOn(cFlag))
				(*stream) << "*" << typeSizeD[(cFlag>>2)&0x00000007];
			//if(flagSizeStk(cFlag))
			//	(*stream) << " size: stack";
			//if(flagSizeOn(cFlag))
			//	(*stream) << " size: instr";
			//if(flagSizeStk(cFlag) || flagSizeOn(cFlag))
			//	(*stream) << "*" << typeSizeD[(cFlag>>2)&0x00000007];
		}
		break;
	}
	if(cmd >= cmdAdd && cmd <= cmdLogXor)
	{
		(*stream) << ' ';
		switch(cmd)
		{
		case cmdAdd:
			(*stream) << "ADD";
			break;
		case cmdSub:
			(*stream) << "SUB";
			break;
		case cmdMul:
			(*stream) << "MUL";
			break;
		case cmdDiv:
			(*stream) << "DIV";
			break;
		case cmdPow:
			(*stream) << "POW";
			break;
		case cmdMod:
			(*stream) << "MOD";
			break;
		case cmdLess:
			(*stream) << "LES";
			break;
		case cmdGreater:
			(*stream) << "GRT";
			break;
		case cmdLEqual:
			(*stream) << "LEQL";
			break;
		case cmdGEqual:
			(*stream) << "GEQL";
			break;
		case cmdEqual:
			(*stream) << "EQL";
			break;
		case cmdNEqual:
			(*stream) << "NEQL";
			break;
		case cmdShl:
			(*stream) << "SHL";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: SHL used on float");
			break;
		case cmdShr:
			(*stream) << "SHR";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: SHR used on float");
			break;
		case cmdBitAnd:
			(*stream) << "BAND";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: BAND used on float");
			break;
		case cmdBitOr:
			(*stream) << "BOR";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: BOR used on float");
			break;
		case cmdBitXor:
			(*stream) << "BXOR";
			if(oFlag == OTYPE_DOUBLE)
				throw string("Invalid operation: BXOR used on float");
			break;
		case cmdLogAnd:
			(*stream) << "LAND";
			break;
		case cmdLogOr:
			(*stream) << "LOR";
			break;
		case cmdLogXor:
			(*stream) << "LXOR";
			break;
		}
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double;";
			break;
		case OTYPE_LONG:
			(*stream) << " long;";
			break;
		case OTYPE_INT:
			(*stream) << " int;";
			break;
		default:
			(*stream) << "ERROR: OperFlag expected after instruction";
		}
	}
	if(cmd >= cmdNeg && cmd <= cmdLogNot)
	{
		(*stream) << ' ';
		switch(cmd)
		{
		case cmdNeg:
			(*stream) << "NEG";
			break;
		case cmdInc:
			(*stream) << "INC";
			break;
		case cmdDec:
			(*stream) << "DEC";
			break;
		case cmdBitNot:
			(*stream) << "BNOT";
			if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
				throw string("Invalid operation: BNOT used on float");
			break;
		case cmdLogNot:
			(*stream) << "LNOT;";
			break;
		}
		switch(oFlag)
		{
		case OTYPE_DOUBLE:
			(*stream) << " double;";
			break;
		case OTYPE_LONG:
			(*stream) << " long;";
			break;
		case OTYPE_INT:
			(*stream) << " int;";
			break;
		default:
			(*stream) << "ERROR: OperFlag expected after ";
		}
	}
	if(cmd >= cmdIncAt && cmd < cmdDecAt)
	{
		if(cmd == cmdIncAt)
			(*stream) << " INCAT ";
		if(cmd == cmdDecAt)
			(*stream) << " DECAT ";
		(*stream) << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
		
		(*stream) << valind << "] //";
		if(flagAddrStk(cFlag)){
			(*stream) << "stack";
			if(flagAddrRel(cFlag))
				(*stream) << "+top";
		}else{
			if(flagAddrRel(cFlag))
				(*stream) << "rel+top";
			if(flagShiftStk(cFlag))
				(*stream) << "+shiftstk";
			if(flagShiftOn(cFlag))
				(*stream) << "+shifton";
			if(flagShiftStk(cFlag) || flagShiftOn(cFlag))
				(*stream) << "*" << typeSizeD[(cFlag>>2)&0x00000007];
			if(flagSizeStk(cFlag))
				(*stream) << " size: stack";
			if(flagSizeOn(cFlag))
				(*stream) << " size: instr";
			if(flagSizeStk(cFlag) || flagSizeOn(cFlag))
				(*stream) << "*" << typeSizeD[(cFlag>>2)&0x00000007];
		}
	}
	
	// Add end alignment
	// Добавить выравнивание
	size_t endPos = stream->tellp();
	int putSize = (int)(endPos - beginPos);
	int alignLen = 55-putSize;
	if(alignLen > 0)
		for(int i = 0; i < alignLen; i++)
			(*stream) << ' ';
	
}