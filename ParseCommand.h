#include "stdafx.h"
#pragma once

typedef short int CmdID;

const CmdID cmdNop		= 0;	// no operation

//general commands [using command flag. check CmdRef.txt]
const CmdID cmdPush	= 100;	// pushe's a number on top of general stack
const CmdID cmdPop		= 101;	// remove's a number from top
const CmdID cmdMov		= 102;	// copy's number from top of stack to value in value stack
const CmdID cmdShift	= 103;	// add's a shift value to address in general stack, checking the lower and upper bound
const CmdID cmdRTOI		= 104;	// convert's real numbers to integer
const CmdID cmdITOR		= 105;	// convert's integer numbers to real
const CmdID cmdITOL		= 106;	// convert's integer numbers to long
const CmdID cmdLTOI		= 107;	// convert's long numbers to integer
const CmdID cmdSwap		= 108;	// swap's two values on top of the general stack
const CmdID cmdCopy		= 109;	// copy value on top of the stack, and push it on the top again

//conditional and unconditional jumps  [using operation flag. check CmdRef.txt]
const CmdID cmdJmp		= 110;	// unconditional jump
const CmdID cmdJmpZ		= 111;	// jump on zero
const CmdID cmdJmpNZ	= 112;	// jump on not zero

//commands for functions	[not using flags]
const CmdID cmdCall		= 113;	// call script function
const CmdID cmdCallStd	= 114;	// call standard function
const CmdID cmdReturn	= 115;

//commands for work with value stack	[not using flags]
const CmdID cmdPushVTop	= 116;	// save value stack top position
const CmdID cmdPopVTop	= 117;	// pop data from value stack until last top position and remove last top position
const CmdID cmdPushV	= 118;	// shift value stack top

//binary commands	[using operation flag. check CmdRef.txt]
//they take two top numbers (both the same type!), remove them from stack,
//perform operation and place result on top of stack (the same type as two values!)
const CmdID cmdAdd		= 130;
const CmdID cmdSub		= 131;
const CmdID cmdMul		= 132;
const CmdID cmdDiv		= 133;
const CmdID cmdPow		= 134;
const CmdID cmdMod		= 135;	// a % b
const CmdID cmdLess		= 136;	// <
const CmdID cmdGreater	= 137;	// >
const CmdID cmdLEqual	= 138;	// <=
const CmdID cmdGEqual	= 139;	// >=
const CmdID cmdEqual	= 140;	// ==
const CmdID cmdNEqual	= 141;	// !=
const CmdID cmdShl		= 142;	// << shift left
const CmdID cmdShr		= 143;	// >> shift right
const CmdID cmdBitAnd	= 144;	// & binary AND	| Doesn't work for double and float numbers! |
const CmdID cmdBitOr	= 145;	// | binary OR	| Doesn't work for double and float numbers! |
const CmdID cmdBitXor	= 146;	//   binary XOR	| Doesn't work for double and float numbers! |
const CmdID cmdLogAnd	= 147;	// && logical AND
const CmdID cmdLogOr	= 148;	// || logical OR
const CmdID cmdLogXor	= 149;	//    logical XOR

//unary commands	[using operation flag. check CmdRef.txt]
//they take one value from top of the stack, and replace it with resulting value
const CmdID cmdNeg		= 160;	// negation
const CmdID cmdInc		= 161;	// increment
const CmdID cmdDec		= 162;	// decrement
const CmdID cmdBitNot	= 163;	// ~ binary NOT | Doesn't work for double and float numbers! |
const CmdID cmdLogNot	= 164;	// ! logical NOT

//Special unary commands	[using command flag. check CmdRef.txt]
//They work with variable at address
const CmdID cmdIncAt	= 171;	// increment at address
const CmdID cmdDecAt	= 172;	// decrement at address

//Flag types
typedef unsigned short int CmdFlag;	// command flag
typedef unsigned char OperFlag;		// operation flag

//
enum asmStackType
{
	STYPE_INT,
	STYPE_LONG,
	STYPE_FLOAT_DEPRECATED,
	STYPE_DOUBLE,
	STYPE_FORCE_DWORD = 1<<30,
};
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
enum asmOperType
{
	OTYPE_DOUBLE,
	OTYPE_FLOAT_DEPRECATED,
	OTYPE_LONG,
	OTYPE_INT,
};
static asmOperType operTypeForStackType[] = { OTYPE_INT, OTYPE_LONG, (asmOperType)0, OTYPE_DOUBLE };
static asmDataType dataTypeForStackType[] = { DTYPE_INT, DTYPE_LONG, (asmDataType)0, DTYPE_DOUBLE };
//Don't forget the shift, before addressing!
static asmStackType stackTypeForDataType[] = { STYPE_INT, STYPE_INT, STYPE_INT, STYPE_LONG, STYPE_DOUBLE, STYPE_DOUBLE };
//Functions for extraction of different bits from flags
__forceinline asmOperType	flagOperandA(const OperFlag& flag){ return (asmOperType)(flag&0x00000003); }
__forceinline asmOperType	flagOperandB(const OperFlag& flag){ return (asmOperType)((flag>>2)&0x00000003); }
__forceinline asmStackType	flagStackType(const CmdFlag& flag){ return (asmStackType)(flag&0x00000003); }
__forceinline asmDataType	flagDataType(const CmdFlag& flag){ return (asmDataType)(((flag>>2)&0x00000007)<<2); }//flag&0x00000003C); }
__forceinline UINT			flagAddrRel(const CmdFlag& flag){ return (flag>>5)&0x00000001; }
__forceinline UINT			flagAddrAbs(const CmdFlag& flag){ return (flag>>6)&0x00000001; }
__forceinline UINT			flagAddrStk(const CmdFlag& flag){ return (flag>>7)&0x00000001; }
__forceinline UINT			flagShiftOn(const CmdFlag& flag){ return (flag>>8)&0x00000001; }
__forceinline UINT			flagShiftStk(const CmdFlag& flag){ return (flag>>9)&0x00000001; }
__forceinline UINT			flagSizeOn(const CmdFlag& flag){ return (flag>>10)&0x00000001; }
__forceinline UINT			flagSizeStk(const CmdFlag& flag){ return (flag>>11)&0x00000001; }
__forceinline UINT			flagNoAddr(const CmdFlag& flag){ return !(flag&0x00000060); }

//constants for flag creation from different bits
const UINT	bitAddrRel	= 1<<5;
const UINT	bitAddrAbs	= 1<<6;
const UINT	bitAddrStk	= 1<<7;
const UINT	bitShiftOn	= 1<<8;
const UINT	bitShiftStk	= 1<<9;
const UINT	bitSizeOn	= 1<<10;
const UINT	bitSizeStk	= 1<<11;

const UINT	operAdouble	= 0;
const UINT	operAfloat	= 1;
const UINT	operAlong	= 2;
const UINT	operAint	= 3;
/*const UINT	operBdouble	= 0;
const UINT	operBfloat	= 4;
const UINT	operBlong	= 8;
const UINT	operBint	= 12;*/

//Command list (bytecode)
class CommandList
{
public:
	CommandList(UINT count = 65000)
	{
		bytecode = new char[count];
		max = count;
		curr = 0;
	}
	CommandList(const CommandList& r)	//Copy constructor
	{
		bytecode = new char[r.max];
		max = r.max;
		curr = 0;
	}
	~CommandList()
	{
		delete[] bytecode;
	}

	template<class T> void AddData(const T& data)
	{
		memcpy(bytecode+curr, &data, sizeof(T));
		curr += sizeof(T);
	}
	void AddData(const void* data, size_t size){ memcpy(bytecode+curr, data, size); curr += (UINT)size; }

	__inline	bool GetINT(UINT pos, int& ret){ ret = *(reinterpret_cast<int*>(bytecode+pos)); return true; }
	__inline	bool GetSHORT(UINT pos, short int& ret){ if(pos+2 > curr)return false; ret = *(reinterpret_cast<short int*>(bytecode+pos)); return true; }
	__inline	bool GetUSHORT(UINT pos, unsigned short& ret){ ret = *(reinterpret_cast<unsigned short*>(bytecode+pos)); return true; }
	__inline	bool GetUINT(UINT pos, UINT& ret){ ret = *(reinterpret_cast<UINT*>(bytecode+pos)); return true; }
	__inline	bool GetUCHAR(UINT pos, unsigned char& ret){ ret = *(reinterpret_cast<unsigned char*>(bytecode+pos)); return true; }

	template<class T> __inline bool GetData(UINT pos, T& ret)
	{
		if(pos+sizeof(T) > curr)
			return false;
		memcpy(&ret, bytecode+pos, sizeof(T));
		return true;
	}
	__inline bool	GetData(UINT pos, void* data, size_t size)
	{
		if(pos+size > curr)
			return false;
		memcpy(data, bytecode+pos, size);
		return true;
	}

	__inline void	SetData(UINT pos, const void* data, size_t size)
	{
		memcpy(bytecode+pos, data, size);
	}

	__inline const UINT&	GetCurrPos(){ return curr; }

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
			if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
				throw string("Invalid operation: SHL used on float");
			break;
		case cmdShr:
			(*stream) << "SHR";
			if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
				throw string("Invalid operation: SHR used on float");
			break;
		case cmdBitAnd:
			(*stream) << "BAND";
			if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
				throw string("Invalid operation: BAND used on float");
			break;
		case cmdBitOr:
			(*stream) << "BOR";
			if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
				throw string("Invalid operation: BOR used on float");
			break;
		case cmdBitXor:
			(*stream) << "BXOR";
			if(oFlag == OTYPE_DOUBLE)// || oFlag == OTYPE_FLOAT)
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
	
	//Add end alignment
	size_t endPos = stream->tellp();
	int putSize = (int)(endPos - beginPos);
	int alignLen = 55-putSize;
	if(alignLen > 0)
		for(int i = 0; i < alignLen; i++)
			(*stream) << ' ';
	
}