#pragma once
#include "stdafx.h"

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

// Special functions
// Особые функции [флаги не используются]
	// no operation
	// :)
const CmdID cmdNop		= 0;
	// converts number on top of the stack to integer and multiples it by some number [int after instruction]
	// конвентирует число на вершине стека в int и умножает на некоторое число [int за инструкцией]
const CmdID cmdCTI		= 103;
	// get variable address, shifted by the parameter stack base
	// получить адрес переменной, относительно базы стека переменных
const CmdID cmdGetAddr	= 201;
	// get function address
	// получить адрес функции
const CmdID cmdFuncAddr	= 202;

// general commands [using command flag. check CmdRef.txt]
// основные команды [используются флаг команды, смотрите CmdRef.txt]
	// pushes a number on top of general stack
	// положить значение на верхушку стека
const CmdID cmdPush		= 100;

// cmdPush specializations for VM
const CmdID cmdPushCharAbs = 70;
const CmdID cmdPushShortAbs = 71;
const CmdID cmdPushIntAbs = 72;
const CmdID cmdPushFloatAbs = 73;
const CmdID cmdPushDorLAbs = 74;
const CmdID cmdPushCmplxAbs = 75;

const CmdID cmdPushCharRel = 76;
const CmdID cmdPushShortRel = 77;
const CmdID cmdPushIntRel = 78;
const CmdID cmdPushFloatRel = 79;
const CmdID cmdPushDorLRel = 80;
const CmdID cmdPushCmplxRel = 81;

const CmdID cmdPushCharStk = 82;
const CmdID cmdPushShortStk = 83;
const CmdID cmdPushIntStk = 84;
const CmdID cmdPushFloatStk = 85;
const CmdID cmdPushDorLStk = 86;
const CmdID cmdPushCmplxStk = 87;

const CmdID cmdDTOF		= 96;	// double to float conversion
const CmdID cmdFEnter	= 97;	// only for VM - jmp before function is replaced by function call imitation
const CmdID cmdMovRTaP	= 98;	// cmdMov + (relative to top and pop)
const CmdID cmdPushImmt	= 99;

	// removes a number from top
	// убрать значение с верхушки стека
const CmdID cmdPop		= 101;
	// copy's number from top of stack to value in value stack
	// скопировать значение с верхушки стека в память где располагаются переменные
const CmdID cmdMov		= 102;
	// converts real numbers to integer
	// преобразовать действительное число в целое (на вершине стека)
const CmdID cmdRTOI		= 104;
	// converts integer numbers to real
	// преобразовать целое число в действительное (на вершине стека)
const CmdID cmdITOR		= 105;
	// converts integer numbers to long
	// преобразовать int в long (на вершине стека)
const CmdID cmdITOL		= 106;
	// converts long numbers to integer
	// преобразовать long в int (на вершине стека)
const CmdID cmdLTOI		= 107;
	// swaps two values on top of the general stack
	// поменять местами два значения на верхушке стека
const CmdID cmdSwap		= 108;
	// copy value on top of the stack, and push it on the top again
	// скопировать значение на верхушке стека и добавить его в стек
const CmdID cmdCopy		= 109;
	// set value to a range of memory. data type are provided, as well as starting address and count
	// установить значение участку памяти. Указаны тип данных, а также начальная позиция и количество
const CmdID cmdSetRange = 200;

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

const unsigned int CALL_BY_POINTER = (unsigned int)-1;
// commands for functions	[not using flags]
// команды для функций	[флаги не используются]
	// call script function
	// вызов функции, определённой в скрипте
const CmdID cmdCall		= 113;
	// call standard function
	// вызов стандартных (встроенных) функций
const CmdID cmdCallStd	= 114;
//							[using Operation Flag]
	// return from function
	// возврат из функции и выполнение POPT n раз, где n идёт за командой
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
	// инкремент значения по адресу
const CmdID cmdIncAt	= 171;
	// decrement at address
	// декремент значения по адресу
const CmdID cmdDecAt	= 172;

// Flag types
// Типы флагов
typedef unsigned short int CmdFlag;	// command flag
typedef unsigned char OperFlag;		// operation flag
typedef unsigned short int RetFlag; // return flag

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
	OTYPE_FLOAT_DEPRECATED,
	OTYPE_LONG,
	OTYPE_INT,
};

static int		stackTypeSize[] = { 4, 8, -1, 8 };
// Conversion of asmStackType to appropriate asmOperType
// Преобразование asmStackType в подходящий asmOperType
static asmOperType operTypeForStackType[] = { OTYPE_INT, OTYPE_LONG, (asmOperType)0, OTYPE_DOUBLE };

// Conversion of asmStackType to appropriate asmDataType
// Преобразование asmStackType в подходящий asmDataType
static asmDataType dataTypeForStackType[] = { DTYPE_INT, DTYPE_LONG, (asmDataType)0, DTYPE_DOUBLE };

// Conversion of asmDataType to appropriate asmStackType
// Преобразование asmDataType в подходящий asmStackType
static asmStackType stackTypeForDataTypeArr[] = { STYPE_INT, STYPE_INT, STYPE_INT, STYPE_LONG, STYPE_DOUBLE, STYPE_DOUBLE, STYPE_COMPLEX_TYPE };
__forceinline asmStackType stackTypeForDataType(asmDataType dt){ return stackTypeForDataTypeArr[dt/4]; }

// Functions for extraction of different bits from flags
// Функции для извлечения значений отдельных битов из флагов
	// extract asmStackType
	// извлечь asmStackType
__forceinline asmStackType	flagStackType(const CmdFlag& flag){ return (asmStackType)(flag&0x00000003); }
	// extract asmDataType
	// извлечь asmDataType
__forceinline asmDataType	flagDataType(const CmdFlag& flag){ return (asmDataType)(((flag>>2)&0x00000007)<<2); }
	// addressing is performed relatively to the base of variable stack
	// адресация производится относительно базы стека переменных
__forceinline unsigned int	flagAddrRel(const CmdFlag& flag){ return (flag>>5)&0x00000001; }
	// addressing is perform using absolute address
	// адресация производится по абсолютному адресу
__forceinline unsigned int	flagAddrAbs(const CmdFlag& flag){ return (flag>>6)&0x00000001; }
	// addressing is performed relatively to the top of variable stack
	// адресация производится относительно вершины стека переменных
__forceinline unsigned int	flagAddrRelTop(const CmdFlag& flag){ return (flag>>7)&0x00000001; }

	// shift is placed in stack
	// сдвиг находится в основном стеке
__forceinline unsigned int	flagShiftStk(const CmdFlag& flag){ return (flag>>9)&0x00000001; }
	// address is clamped to the maximum
	// адрес не может превыщать некоторое значение
__forceinline unsigned int	flagSizeOn(const CmdFlag& flag){ return (flag>>10)&0x00000001; }
	// maximum is placed in stack
	// максимум находится в основном стеке
__forceinline unsigned int	flagSizeStk(const CmdFlag& flag){ return (flag>>11)&0x00000001; }

	// push value on stack before modifying
	// положить значение в стек до изменения
__forceinline unsigned int	flagPushBefore(const CmdFlag& flag){ return (flag>>12)&0x00000001; }
	// push value on stack after modifying
	// положить значение в стек после изменения
__forceinline unsigned int	flagPushAfter(const CmdFlag& flag){ return (flag>>13)&0x00000001; }

	// addressing is not performed
	// адресация отсутствует
__forceinline unsigned int	flagNoAddr(const CmdFlag& flag){ return !(flag&0x00000060); }

// constants for CmdFlag creation from different bits
// константы для создания флага комманды из отдельных битов
const unsigned int	bitAddrRel	= 1 << 5;
const unsigned int	bitAddrAbs	= 1 << 6;
const unsigned int	bitAddrRelTop= 1 << 7;

const unsigned int	bitShiftStk	= 1 << 9;
const unsigned int	bitSizeOn	= 1 << 10;
const unsigned int	bitSizeStk	= 1 << 11;

// Для cmdIncAt и cmdDecAt
const unsigned int	bitPushBefore = 1 << 12;	// положить значение в стек до изменения
const unsigned int	bitPushAfter = 1 << 13;		// положить значение в стек после изменения

// constants for RetFlag creation from different bits
// константы для создания флага возврата из отдельных битов
const unsigned int	bitRetError	= 1 << 15;	// пользователь забыл возвратить значение, остановить выполнение
const unsigned int	bitRetSimple= 1 << 14;	// функция возвращает базовый тип

// Command list (bytecode)
// Листинг команд (байткод)
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

		unsigned int	byteCodePos;	// Позиция в байткоде, к которой относится строка
		char	info[128];		// Указатели на начало и конец строки
	};
public:
	// создаём сразу место для будущих команд
	CommandList(unsigned int count = 65000)
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
		curr += (unsigned int)size;
	}

	// получить int по адресу
	__inline	bool GetINT(unsigned int pos, int& ret)
	{
		ret = *(reinterpret_cast<int*>(bytecode+pos));
		return true;
	}
	// получить short int по адресу
	__inline	bool GetSHORT(unsigned int pos, short int& ret)
	{
		if(pos+2 > curr)
			return false;
		ret = *(reinterpret_cast<short int*>(bytecode+pos));
		return true;
	}
	// получить unsigned short int по адресу
	__inline	bool GetUSHORT(unsigned int pos, unsigned short& ret)
	{
		ret = *(reinterpret_cast<unsigned short*>(bytecode+pos));
		return true;
	}
	// получить unsigned int по адресу
	__inline	bool GetUINT(unsigned int pos, unsigned int& ret)
	{
		ret = *(reinterpret_cast<unsigned int*>(bytecode+pos));
		return true;
	}
	// получить unsigned char по адресу
	__inline	bool GetUCHAR(unsigned int pos, unsigned char& ret)
	{
		ret = *(reinterpret_cast<unsigned char*>(bytecode+pos));
		return true;
	}

	// получить значение произвольного типа по адресу
	template<class T> __inline bool GetData(unsigned int pos, T& ret)
	{
		if(pos+sizeof(T) > curr)
			return false;
		memcpy(&ret, bytecode+pos, sizeof(T));
		return true;
	}
	// получить произвольное кол-во байт по адресу
	__inline bool	GetData(unsigned int pos, void* data, size_t size)
	{
		if(pos+size > curr)
			return false;
		memcpy(data, bytecode+pos, size);
		return true;
	}

	// заменить произвольное количество байт по адресу
	__inline void	SetData(unsigned int pos, const void* data, size_t size)
	{
		memcpy(bytecode+pos, data, size);
	}

	// получить текущий размер байткода
	__inline const unsigned int&	GetCurrPos()
	{
		return curr;
	}

	// удалить байткод
	__inline void	Clear()
	{
		curr = 0;
		memset(bytecode, 0, max);
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
	static int		GetCommandLength(CmdID cmd, CmdFlag cFlag)
	{
		asmDataType dt;
		int size = 2;
		switch(cmd)
		{
		case cmdMovRTaP:
			size += 2;
			dt = flagDataType(cFlag);

			size += 4;

			if(dt == DTYPE_COMPLEX_TYPE)
				size += 4;
			break;
		case cmdCallStd:
			size += 4;
			break;
		case cmdCall:
			size += 4;
			size += 2;
			break;
		case cmdFuncAddr:
			size += 4;
			break;
		case cmdReturn:
			size += 4;
			break;
		case cmdPushV:
			size += 4;
			break;
		case cmdCTI:
			size += 5;
			break;
		case cmdPushImmt:
			size += 2;
			dt = flagDataType(cFlag);

			if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG)
				size += 8;
			if(dt == DTYPE_FLOAT || dt == DTYPE_INT)
				size += 4;
			if(dt == DTYPE_SHORT)
				size += 2;
			if(dt == DTYPE_CHAR)
				size += 1;
			break;
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
		case cmdPush:
			{
				size += 2;
				dt = flagDataType(cFlag);

				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)))
					size += 4;
				if(flagSizeOn(cFlag))
					size += 4;

				if(dt == DTYPE_COMPLEX_TYPE)
					size += 4;
			}
			break;
		case cmdMov:
			{
				size += 2;
				if((flagAddrAbs(cFlag) || flagAddrRel(cFlag)))
					size += 4;
				if(flagSizeOn(cFlag))
					size += 4;
				if(flagDataType(cFlag) == DTYPE_COMPLEX_TYPE)
					size += 4;
			}
			break;
		case cmdPop:
			size += 4;
			break;
		case cmdRTOI:
			size += 2;
			break;
		case cmdITOR:
			size += 2;
			break;
		case cmdSwap:
			size += 2;
			break;
		case cmdCopy:
			size += 1;
			break;
		case cmdJmp:
			size += 4;
			break;
		case cmdJmpZ:
			size += 1;
			size += 4;
			break;
		case cmdJmpNZ:
			size += 1;
			size += 4;
			break;
		case cmdSetRange:
			size += 10;
			break;
		case cmdGetAddr:
			size += 4;
			break;
		}
		if(cmd >= cmdAdd && cmd <= cmdLogXor)
			size += 1;
		if(cmd >= cmdNeg && cmd <= cmdLogNot)
			size += 1;
		if(cmd >= cmdIncAt && cmd <= cmdDecAt)
		{
			size += 2;

			if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
				size += 4;
			if(flagSizeOn(cFlag))
				size += 4;
		}
		return size;
	}
	
#ifdef NULLC_LOG_FILES
    static void PrintCommandListing(ostream *logASM, char *cmdStream, char *cmdStreamEnd)
	{
		unsigned int pos = 0, pos2 = 0;
		CmdID	cmd;

		unsigned int	valind, valind2;
		unsigned short	shVal1, shVal2;

		char* typeInfoS[] = { "int", "long", "complex", "double" };
		char* typeInfoD[] = { "char", "short", "int", "long", "float", "double", "complex" };

		CmdFlag cFlag;
		OperFlag oFlag;
		while(cmdStream+pos+2 < cmdStreamEnd)
		{
			cmd = *(CmdID*)(&cmdStream[pos]);

			pos2 = pos;
			pos += 2;

			*logASM << pos2;
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
			case cmdPush:
				{
					cFlag = *(CmdFlag*)(&cmdStream[pos]);
					pos += 2;
					if(cmd == cmdPush)
						*logASM << " PUSH ";
					else
						*logASM << " ***PUSH ";
					*logASM << typeInfoS[cFlag&0x00000003] << "<-";
					*logASM << typeInfoD[(cFlag>>2)&0x00000007];

					asmStackType st = flagStackType(cFlag);
					asmDataType dt = flagDataType(cFlag);
					unsigned int	DWords[2];
					unsigned short sdata;
					unsigned char cdata;
					int valind;
					if(flagNoAddr(cFlag)){
						if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG){
							DWords[0] = *(unsigned int*)(&cmdStream[pos]); pos += 4;
							DWords[1] = *(unsigned int*)(&cmdStream[pos]); pos += 4;
						}
						if(dt == DTYPE_FLOAT || dt == DTYPE_INT){ DWords[0] = *(unsigned int*)(&cmdStream[pos]); pos += 4; }
						if(dt == DTYPE_SHORT){ sdata = *(unsigned short*)(&cmdStream[pos]); pos += 2; DWords[0] = sdata; }
						if(dt == DTYPE_CHAR){ cdata = *(unsigned char*)(&cmdStream[pos]); pos += 1; DWords[0] = cdata; }

						if(dt == DTYPE_DOUBLE)
							*logASM << " (" << *((double*)(&DWords[0])) << ')';
						if(dt == DTYPE_LONG)
							*logASM << " (" << *((long long*)(&DWords[0])) << ')';
						if(dt == DTYPE_FLOAT)
							*logASM << " (" << *((float*)(&DWords[0])) << dec << ')';
						if(dt == DTYPE_INT)
							*logASM << " (" << *((int*)(&DWords[0])) << dec << ')';
						if(dt == DTYPE_SHORT)
							*logASM << " (" << *((short*)(&DWords[0])) << dec << ')';
						if(dt == DTYPE_CHAR)
							*logASM << " (" << *((char*)(&DWords[0])) << ')';
					}else{
						*logASM << " PTR[";
						if(flagAddrRel(cFlag) || flagAddrAbs(cFlag))
						{
							valind = *(int*)(&cmdStream[pos]);
							pos += 4;
							*logASM << valind;
						}
						if(flagAddrRel(cFlag))
							*logASM << "+top";
						if(flagAddrRelTop(cFlag))
							*logASM << "+max";
						if(flagShiftStk(cFlag))
							*logASM << "+shift(stack)";
						
						*logASM << "] ";
						if(flagSizeStk(cFlag))
							*logASM << "size(stack)";
						if(flagSizeOn(cFlag))
						{
							valind = *(int*)(&cmdStream[pos]);
							pos += 4;
							*logASM << " max size(" << valind << ") ";
						}
						if(st == STYPE_COMPLEX_TYPE)
						{
							valind = *(int*)(&cmdStream[pos]);
							pos += 4;
							*logASM << "sizeof(" << valind << ")";
						}
					}
				}
				break;
			case cmdDTOF:
				*logASM << " DTOF;";
				break;
			case cmdCallStd:
				valind = *(int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " CALLS " << valind << ";";
				break;
			case cmdPushVTop:
				*logASM << " PUSHT;";
				break;
			case cmdPopVTop:
				*logASM << " POPT;";
				break;
			case cmdCall:
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += sizeof(unsigned int);
				shVal1 = *(unsigned short*)(&cmdStream[pos]);
				pos += 2;
				*logASM << " CALL " << valind;
				if(shVal1 & bitRetSimple)
					*logASM << " simple";
				*logASM << " size:" << (shVal1&0x0FFF) << ";";
				break;
			case cmdReturn:
				shVal1 = *(unsigned short*)(&cmdStream[pos]);
				pos += 2;
				shVal2 = *(unsigned short*)(&cmdStream[pos]);
				pos += 2;
				*logASM << " RET " << shVal2;
				if(shVal1 & bitRetError)
				{
					*logASM << " ERROR;";
					break;
				}
				if(shVal1 & bitRetSimple)
				{
					switch(shVal1 & 0x0FFF)
					{
					case OTYPE_DOUBLE:
						*logASM << " double;";
						break;
					case OTYPE_LONG:
						*logASM << " long;";
						break;
					case OTYPE_INT:
						*logASM << " int;";
						break;
					}
				}else{
					*logASM << " bytes: " << shVal1;
				}
				break;
			case cmdPushV:
				valind = *(int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " PUSHV " << valind << dec << ";";
				break;
			case cmdNop:
				*logASM << dec << " NOP;";
				break;
			case cmdCTI:
				*logASM << dec << " CTI addr*";
				oFlag = *(unsigned char*)(&cmdStream[pos]);
				pos += 1;
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += sizeof(unsigned int);
				*logASM << valind;

				switch(oFlag)
				{
				case OTYPE_DOUBLE:
					*logASM << " double;";
					break;
				case OTYPE_LONG:
					*logASM << " long;";
					break;
				case OTYPE_INT:
					*logASM << " int;";
					break;
				default:
					*logASM << "ERROR: OperFlag expected after ";
				}
				break;
			case cmdPushImmt:
				{
					cFlag = *(CmdFlag*)(&cmdStream[pos]);
					pos += 2;
					*logASM << " PUSHIMMT ";
					*logASM << typeInfoS[cFlag&0x00000003] << "<-";
					*logASM << typeInfoD[(cFlag>>2)&0x00000007];

					asmDataType dt = flagDataType(cFlag);
					unsigned int	DWords[2];
					unsigned short sdata;
					unsigned char cdata;
					
					if(dt == DTYPE_DOUBLE || dt == DTYPE_LONG){
						DWords[0] = *(unsigned int*)(&cmdStream[pos]); pos += 4;
						DWords[1] = *(unsigned int*)(&cmdStream[pos]); pos += 4;
					}
					if(dt == DTYPE_FLOAT || dt == DTYPE_INT){ DWords[0] = *(unsigned int*)(&cmdStream[pos]); pos += 4; }
					if(dt == DTYPE_SHORT){ sdata = *(unsigned short*)(&cmdStream[pos]); pos += 2; DWords[0] = sdata; }
					if(dt == DTYPE_CHAR){ cdata = *(unsigned char*)(&cmdStream[pos]); pos += 1; DWords[0] = cdata; }

					if(dt == DTYPE_DOUBLE)
						*logASM << " (" << *((double*)(&DWords[0])) << ')';
					if(dt == DTYPE_LONG)
						*logASM << " (" << *((long long*)(&DWords[0])) << ')';
					if(dt == DTYPE_FLOAT)
						*logASM << " (" << *((float*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_INT)
						*logASM << " (" << *((int*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_SHORT)
						*logASM << " (" << *((short*)(&DWords[0])) << dec << ')';
					if(dt == DTYPE_CHAR)
						*logASM << " (" << *((char*)(&DWords[0])) << ')';
				}
				break;
			case cmdMovRTaP:
				{
					cFlag = *(CmdFlag*)(&cmdStream[pos]);
					pos += 2;

					*logASM << " MOVRTAP ";
					*logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
					int valind;
					valind = *(int*)(&cmdStream[pos]);
					pos += 4;
					*logASM << valind;
					*logASM << "+max] ";
					if(flagDataType(cFlag) == DTYPE_COMPLEX_TYPE)
					{
						valind = *(int*)(&cmdStream[pos]);
						pos += 4;
						*logASM << "sizeof(" << valind << ")";
					}
				}
				break;
			case cmdMov:
				{
					cFlag = *(CmdFlag*)(&cmdStream[pos]);
					pos += 2;
					*logASM << " MOV ";
					*logASM << typeInfoS[cFlag&0x00000003] << "->";
					*logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";
					asmStackType st = flagStackType(cFlag);
					int valind;
					
					if(flagAddrRel(cFlag) || flagAddrAbs(cFlag) || flagAddrRelTop(cFlag))
					{
						valind = *(int*)(&cmdStream[pos]);
						pos += 4;
						*logASM << valind;
					}
					if(flagAddrRel(cFlag))
						*logASM << "+top";

					if(flagAddrRelTop(cFlag))
						*logASM << "+max";

					if(flagShiftStk(cFlag))
						*logASM << "+shift(stack)";

					*logASM << "] ";
					if(flagSizeStk(cFlag))
						*logASM << "size(stack)";
					if(flagSizeOn(cFlag))
					{
						valind = *(int*)(&cmdStream[pos]);
						pos += 4;
						*logASM << "size: " << valind;
					}
					if(st == STYPE_COMPLEX_TYPE)
					{
						valind = *(int*)(&cmdStream[pos]);
						pos += 4;
						*logASM << "sizeof(" << valind << ")";
					}
				}
				break;
			case cmdPop:
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " POP" << " sizeof(" << valind << ")";
				break;
			case cmdRTOI:
				cFlag = *(CmdFlag*)(&cmdStream[pos]);
				pos += 2;
				*logASM << " RTOI ";
				*logASM << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
				break;
			case cmdITOR:
				cFlag = *(CmdFlag*)(&cmdStream[pos]);
				pos += 2;
				*logASM << " ITOR ";
				*logASM << typeInfoS[cFlag&0x00000003] << "->" << typeInfoD[(cFlag>>2)&0x00000007];
				break;
			case cmdITOL:
				*logASM << " ITOL";
				break;
			case cmdLTOI:
				*logASM << " LTOI";
				break;
			case cmdSwap:
				cFlag = *(CmdFlag*)(&cmdStream[pos]);
				pos += 2;
				*logASM << " SWAP ";
				*logASM << typeInfoS[cFlag&0x00000003] << "<->";
				*logASM << typeInfoD[(cFlag>>2)&0x00000007];
				break;
			case cmdCopy:
				oFlag = *(unsigned char*)(&cmdStream[pos]);
				pos += 1;
				*logASM << " COPY ";
				switch(oFlag)
				{
				case OTYPE_DOUBLE:
					*logASM << " double;";
					break;
				case OTYPE_LONG:
					*logASM << " long;";
					break;
				case OTYPE_INT:
					*logASM << " int;";
					break;
				}
				break;
			case cmdJmp:
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " JMP " << valind;
				break;
			case cmdJmpZ:
				oFlag = *(unsigned char*)(&cmdStream[pos]);
				pos += 1;
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " JMPZ";
				switch(oFlag)
				{
				case OTYPE_DOUBLE:
					*logASM << " double";
					break;
				case OTYPE_LONG:
					*logASM << " long";
					break;
				case OTYPE_INT:
					*logASM << " int";
					break;
				}
				*logASM << ' ' << valind << ';';
				break;
			case cmdJmpNZ:
				oFlag = *(unsigned char*)(&cmdStream[pos]);
				pos += 1;
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " JMPNZ";
				switch(oFlag)
				{
				case OTYPE_DOUBLE:
					*logASM << " double";
					break;
				case OTYPE_LONG:
					*logASM << " long";
					break;
				case OTYPE_INT:
					*logASM << " int";
					break;
				}
				*logASM << ' ' << valind << ';';
				break;
			case cmdSetRange:
				cFlag = *(CmdFlag*)(&cmdStream[pos]);
				pos += 2;
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += 4;
				valind2 = *(unsigned int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " SETRANGE " << typeInfoD[(cFlag>>2)&0x00000007] << " " << valind << " " << valind2 << ';';
				break;
			case cmdGetAddr:
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " GETADDR " << (int)valind << ';';
				break;
			case cmdFuncAddr:
				valind = *(unsigned int*)(&cmdStream[pos]);
				pos += 4;
				*logASM << " FUNCADDR " << valind;
				break;
			}
			if(cmd >= cmdAdd && cmd <= cmdLogXor)
			{
				oFlag = *(unsigned char*)(&cmdStream[pos]);
				pos += 1;
				*logASM << ' ';
				switch(cmd)
				{
				case cmdAdd:
					*logASM << "ADD";
					break;
				case cmdSub:
					*logASM << "SUB";
					break;
				case cmdMul:
					*logASM << "MUL";
					break;
				case cmdDiv:
					*logASM << "DIV";
					break;
				case cmdPow:
					*logASM << "POW";
					break;
				case cmdMod:
					*logASM << "MOD";
					break;
				case cmdLess:
					*logASM << "LES";
					break;
				case cmdGreater:
					*logASM << "GRT";
					break;
				case cmdLEqual:
					*logASM << "LEQL";
					break;
				case cmdGEqual:
					*logASM << "GEQL";
					break;
				case cmdEqual:
					*logASM << "EQL";
					break;
				case cmdNEqual:
					*logASM << "NEQL";
					break;
				case cmdShl:
					*logASM << "SHL";
					if(oFlag == OTYPE_DOUBLE)
						throw string("Invalid operation: SHL used on float");
					break;
				case cmdShr:
					*logASM << "SHR";
					if(oFlag == OTYPE_DOUBLE)
						throw string("Invalid operation: SHR used on float");
					break;
				case cmdBitAnd:
					*logASM << "BAND";
					if(oFlag == OTYPE_DOUBLE)
						throw string("Invalid operation: BAND used on float");
					break;
				case cmdBitOr:
					*logASM << "BOR";
					if(oFlag == OTYPE_DOUBLE)
						throw string("Invalid operation: BOR used on float");
					break;
				case cmdBitXor:
					*logASM << "BXOR";
					if(oFlag == OTYPE_DOUBLE)
						throw string("Invalid operation: BXOR used on float");
					break;
				case cmdLogAnd:
					*logASM << "LAND";
					break;
				case cmdLogOr:
					*logASM << "LOR";
					break;
				case cmdLogXor:
					*logASM << "LXOR";
					break;
				}
				switch(oFlag)
				{
				case OTYPE_DOUBLE:
					*logASM << " double;";
					break;
				case OTYPE_LONG:
					*logASM << " long;";
					break;
				case OTYPE_INT:
					*logASM << " int;";
					break;
				default:
					*logASM << "ERROR: OperFlag expected after instruction";
				}
			}
			if(cmd >= cmdNeg && cmd <= cmdLogNot)
			{
				oFlag = *(unsigned char*)(&cmdStream[pos]);
				pos += 1;
				*logASM << ' ';
				switch(cmd)
				{
				case cmdNeg:
					*logASM << "NEG";
					break;
				case cmdBitNot:
					*logASM << "BNOT";
					if(oFlag == OTYPE_DOUBLE)
						throw string("Invalid operation: BNOT used on float");
					break;
				case cmdLogNot:
					*logASM << "LNOT;";
					break;
				}
				switch(oFlag)
				{
				case OTYPE_DOUBLE:
					*logASM << " double;";
					break;
				case OTYPE_LONG:
					*logASM << " long;";
					break;
				case OTYPE_INT:
					*logASM << " int;";
					break;
				default:
					*logASM << "ERROR: OperFlag expected after ";
				}
			}
			if(cmd >= cmdIncAt && cmd <= cmdDecAt)
			{
				cFlag = *(CmdFlag*)(&cmdStream[pos]);
				pos += 2;
				if(cmd == cmdIncAt)
					*logASM << " INCAT ";
				if(cmd == cmdDecAt)
					*logASM << " DECAT ";
				*logASM << typeInfoD[(cFlag>>2)&0x00000007] << " PTR[";

				int valind;

				if(flagAddrRel(cFlag) || flagAddrAbs(cFlag)){
					valind = *(int*)(&cmdStream[pos]);
					pos += 4;
					*logASM << valind;
				}
				if(flagAddrRel(cFlag))
					*logASM << "+top";

				if(flagShiftStk(cFlag)){
					*logASM << "+shift";
				}
				*logASM << "] ";
				if(flagSizeStk(cFlag)){
					*logASM << "size: stack";
				}
				if(flagSizeOn(cFlag)){
					valind = *(int*)(&cmdStream[pos]);
					pos += 4;
					*logASM << "size: " << valind;
				}
			}
			*logASM << "\r\n";
		}
	}
#endif
//private:
	char*	bytecode;
	unsigned int	curr;
	unsigned int	max;

	std::vector<CodeInfo>	codeInfo;	// Список строк к коду
};
