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
__forceinline UINT			flagAddrRel(const CmdFlag& flag){ return (flag>>5)&0x00000001; }
	// addressing is perform using absolute address
	// адресация производится по абсолютному адресу
__forceinline UINT			flagAddrAbs(const CmdFlag& flag){ return (flag>>6)&0x00000001; }
	// addressing is performed relatively to the top of variable stack
	// адресация производится относительно вершины стека переменных
__forceinline UINT			flagAddrRelTop(const CmdFlag& flag){ return (flag>>7)&0x00000001; }

	// shift is placed in stack
	// сдвиг находится в основном стеке
__forceinline UINT			flagShiftStk(const CmdFlag& flag){ return (flag>>9)&0x00000001; }
	// address is clamped to the maximum
	// адрес не может превыщать некоторое значение
__forceinline UINT			flagSizeOn(const CmdFlag& flag){ return (flag>>10)&0x00000001; }
	// maximum is placed in stack
	// максимум находится в основном стеке
__forceinline UINT			flagSizeStk(const CmdFlag& flag){ return (flag>>11)&0x00000001; }

	// push value on stack before modifying
	// положить значение в стек до изменения
__forceinline UINT			flagPushBefore(const CmdFlag& flag){ return (flag>>12)&0x00000001; }
	// push value on stack after modifying
	// положить значение в стек после изменения
__forceinline UINT			flagPushAfter(const CmdFlag& flag){ return (flag>>13)&0x00000001; }

	// addressing is not performed
	// адресация отсутствует
__forceinline UINT			flagNoAddr(const CmdFlag& flag){ return !(flag&0x00000060); }

// constants for CmdFlag creation from different bits
// константы для создания флага комманды из отдельных битов
const UINT	bitAddrRel	= 1 << 5;
const UINT	bitAddrAbs	= 1 << 6;
const UINT	bitAddrRelTop= 1 << 7;

const UINT	bitShiftStk	= 1 << 9;
const UINT	bitSizeOn	= 1 << 10;
const UINT	bitSizeStk	= 1 << 11;

// Для cmdIncAt и cmdDecAt
const UINT	bitPushBefore = 1 << 12;	// положить значение в стек до изменения
const UINT	bitPushAfter = 1 << 13;		// положить значение в стек после изменения

// constants for RetFlag creation from different bits
// константы для создания флага возврата из отдельных битов
const UINT	bitRetError	= 1 << 15;	// пользователь забыл возвратить значение, остановить выполнение
const UINT	bitRetSimple= 1 << 14;	// функция возвращает базовый тип

// Command list (bytecode)
// Листинг команд (байткод)
class CommandList
{
	struct CodeInfo
	{
		CodeInfo(UINT position, const char* beginPos, const char* endPos)
		{
			byteCodePos = position;
			memcpy(info, beginPos, (endPos-beginPos < 128 ? endPos-beginPos : 127));
			info[(endPos-beginPos < 128 ? endPos-beginPos : 127)] = '\0';
			for(int i = 0; i < 128; i++)
				if(info[i] == '\n' || info[i] == '\r')
					info[i] = ' ';
		}

		UINT	byteCodePos;	// Позиция в байткоде, к которой относится строка
		char	info[128];		// Указатели на начало и конец строки
	};
public:
	// создаём сразу место для будущих команд
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
		codeInfo.clear();
	}

	void		AddDescription(UINT position, const char* start, const char* end)
	{
		codeInfo.push_back(CodeInfo(position, start, end));
	}
	const char*	GetDescription(UINT position)
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
//private:
	char*	bytecode;
	UINT	curr;
	UINT	max;

	std::vector<CodeInfo>	codeInfo;	// Список строк к коду
};
