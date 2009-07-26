#include "Translator_X86.h"

// Mapping from x86Reg to register code
char	regCode[] = { -1, 0, 3, 1, 2, 4, 7, 5, 6 };
// Segment codes
enum	segCode{ segES, segCS, segSS, segDS, segFS, segGS };
// x87Reg are mapped to FP register codes directly
//char	fpCode[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
// Mapping from x86Cond to x86 conditions
char	condCode[] = { 0, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15 };

// spareField can be found in nasmdoc as /0-7 or /r codes in instruction bytecode
// encode register as address
unsigned char	encodeRegister(x86Reg reg, char spareField)
{
	unsigned char mod = 3 << 6;
	unsigned char spare = spareField << 3;
	unsigned char RM = regCode[reg];
	return mod | spare | RM;
}

// encode [base], [base+displacement], [index*multiplier+displacement] and [index*multiplier+base+displacement]
unsigned int	encodeAddress(unsigned char* stream, x86Reg index, int multiplier, x86Reg base, int displacement, char spareField)
{
	assert(index != rESP);
	unsigned char* start = stream;

	bool dispImm8 = (char)(displacement) == displacement;

	unsigned char mod = 0;
	if(displacement)
	{
		if(dispImm8)
			mod = 1 << 6;
		else
			mod = 2 << 6;
	}

	// special case: [ebp] should be encoded as [ebp+0]
	if(displacement == 0 && base == rEBP)
		mod = 1 << 6;
	if(index == rNONE && base == rNONE)
		mod = 0;

	unsigned char spare = spareField << 3;

	unsigned char RM = regCode[rEBP]; // by default, it's simply [displacement]
	if(base != rNONE)
		RM = regCode[base];	// this is [base + displacement]
	if(index != rNONE)
		RM = regCode[rESP];	// this changes mode to [index*multiplier + base + displacement]

	*stream++ = mod | spare | RM;

	unsigned char sibScale = 0;
	if(multiplier == 2)
		sibScale = 1 << 6;
	else if(multiplier == 4)
		sibScale = 2 << 6;
	else if(multiplier == 8)
		sibScale = 3 << 6;
	assert(multiplier == 0 || multiplier == 1 || multiplier == 2 || multiplier == 4 || multiplier == 8);

	unsigned char sibIndex = (index != rNONE ? regCode[index] << 3 : regCode[rESP] << 3);
	unsigned char sibBase = regCode[base];

	if(index != rNONE && base == rNONE)
		sibBase = regCode[rEBP];
	if(index != rNONE || base == rESP)
		*stream++ = sibScale | sibIndex | sibBase;
	
	if(dispImm8)
	{
		*stream = (char)displacement;
		if(mod)
			stream++;
	}else{
		*(int*)stream = displacement;
		stream += 4;
	}
	return (int)(stream - start);
}

struct LabelInfo
{
	LabelInfo():pos(NULL){}
	LabelInfo(unsigned int LabelID, const unsigned char *newPos)
	{
		labelID = LabelID;
		pos = newPos;
	}

	unsigned int labelID;
	const unsigned char *pos;
};

struct UnsatisfiedJump
{
	UnsatisfiedJump():jmpPos(NULL){}
	UnsatisfiedJump(unsigned int LabelID, bool newIsNear, unsigned char *newJmpPos)
	{
		labelID = LabelID;
		isNear = newIsNear;
		jmpPos = newJmpPos;
	}

	unsigned int labelID;
	bool isNear;
	unsigned char *jmpPos;
};

FastVector<LabelInfo>	labels(16);
FastVector<UnsatisfiedJump> pendingJumps(16);

bool FindLabel(unsigned int labelID, LabelInfo& info)
{
	for(unsigned int i = 0; i < labels.size(); i++)
	{
		if(labels[i].labelID == labelID)
		{
			info = labels[i];
			return true;
		}
	}
	return false;
}

void x86ClearLabels()
{
	labels.clear();
	pendingJumps.clear();
}

int x86FLDZ(unsigned char* stream)
{
	stream[0] = 0xd9;
	stream[1] = 0xee;
	return 2;
}

int x86FLD1(unsigned char* stream)
{
	stream[0] = 0xd9;
	stream[1] = 0xe8;
	return 2;
}

// fld st*
int x86FLD(unsigned char *stream, x87Reg reg)
{
	stream[0] = 0xd9;
	stream[1] = (unsigned char)(0xc0 + reg);
	return 2;
}

// fld *word [reg+shift]
int x86FLD(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd9 : 0xdd;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 0);
	return 1+asize;
}
// fld *word [regA+regB+shift]
int x86FLD(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	assert(regB != rNONE);
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd9 : 0xdd;
	unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, 0);
	return 1+asize;
}

// fild *word [reg]
int x86FILD(unsigned char *stream, x86Size size, x86Reg reg)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xdb : 0xdf;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, 0, size == sDWORD ?  0 : 5);
	return 1+asize;
}

// fst *word [reg+shift]
int x86FST(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd9 : 0xdd;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 2);
	return 1+asize;
}
// fst *word [regA+regB+shift]
int x86FST(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	assert(regB != rNONE);
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd9 : 0xdd;
	unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, 2);
	return 1+asize;
}

// fstp st*
int x86FSTP(unsigned char *stream, x87Reg dst)
{
	stream[0] = 0xdd;
	stream[1] = (unsigned char)(0xd8 + dst);
	return 2;
}
// fstp *word [reg+shift]
int x86FSTP(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd9 : 0xdd;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 3);
	return 1+asize;
}
// fstp *word [regA+regB+shift]
int x86FSTP(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	assert(regB != rNONE);
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd9 : 0xdd;
	unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, 3);
	return 1+asize;
}

// fistp *word [reg+shift]
int x86FISTP(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE);
	if(size == sWORD)
	{
		stream[0] = 0xdf;
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 3);
		return 1+asize;
	}else if(size == sDWORD){
		stream[0] = 0xdb;
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 3);
		return 1+asize;
	}
	stream[0] = 0xdf;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 7);
	return 1+asize;
}

// fadd *word [reg+shift]
int x86FADD(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd8 : 0xdc;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 0);
	return 1+asize;
}
// faddp
int x86FADDP(unsigned char *stream)
{
	stream[0] = 0xde;
	stream[1] = 0xc1;
	return 2;
}
// fsub *word [reg+shift]
int x86FSUB(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd8 : 0xdc;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 4);
	return 1+asize;
}
// fsubr *word [reg+shift]
int x86FSUBR(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd8 : 0xdc;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 5);
	return 1+asize;
}
// fsubp
int x86FSUBP(unsigned char *stream)
{
	stream[0] = 0xde;
	stream[1] = 0xe9;
	return 2;
}
// fsubrp
int x86FSUBRP(unsigned char *stream)
{
	stream[0] = 0xde;
	stream[1] = 0xe1;
	return 2;
}
// fmul *word [reg+shift]
int x86FMUL(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd8: 0xdc;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 1);
	return 1+asize;
}
// fmulp
int x86FMULP(unsigned char *stream)
{
	stream[0] = 0xde;
	stream[1] = 0xc9;
	return 2;
}
// fdiv *word [reg+shift]
int x86FDIV(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd8 : 0xdc;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 6);
	return 1+asize;
}
// fdivr *word [reg+shift]
int x86FDIVR(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd8 : 0xdc;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 7);
	return 1+asize;
}
// fdivrp
int x86FDIVRP(unsigned char *stream)
{
	stream[0] = 0xde;
	stream[1] = 0xf1;
	return 2;
}

int x86FCHS(unsigned char *stream)
{
	stream[0] = 0xd9;
	stream[1] = 0xe0;
	return 2;
}

int x86FPREM(unsigned char *stream)
{
	stream[0] = 0xd9;
	stream[1] = 0xf8;
	return 2;
}

int x86FSQRT(unsigned char *stream)
{
	stream[0] = 0xd9;
	stream[1] = 0xfa;
	return 2;
}

int x86FSINCOS(unsigned char *stream)
{
	stream[0] = 0xd9;
	stream[1] = 0xfb;
	return 2;
}
int x86FPTAN(unsigned char *stream)
{
	stream[0] = 0xd9;
	stream[1] = 0xf2;
	return 2;
}

int x86FRNDINT(unsigned char *stream)
{
	stream[0] = 0xd9;
	stream[1] = 0xfc;
	return 2;
}

// fcomp *word [reg+shift]
int x86FCOMP(unsigned char *stream, x86Size size, x86Reg reg, int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd8 : 0xdc;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 3);
	return 1+asize;
}

// fnstsw ax
int x86FNSTSW(unsigned char *stream)
{
	stream[0] = 0xdf;
	stream[1] = 0xe0;
	return 2;
}

// target - word [esp]
int x86FSTCW(unsigned char *stream)
{
	stream[0] = 0x9b;
	stream[1] = 0xd9;
	unsigned int asize = encodeAddress(stream+2, rNONE, 1, rESP, 0, 7);
	return 2+asize;
}
int x86FLDCW(unsigned char *stream, int shift)
{
	stream[0] = 0xd9;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, rESP, shift, 5);
	return 1+asize;
}

// push dword [regA+regB+shift]
int x86PUSH(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	assert(size == sDWORD);
	if(regB == rNONE && regA != rNONE)	// swap so if there is only one register, it will be base
	{
		regB = regA;
		regA = rNONE;
	}
	stream[0] = 0xff;
	unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, 6);
	return 1+asize;
}
int x86PUSH(unsigned char *stream, x86Reg reg)
{
	stream[0] = 0x50 + regCode[reg];
	return 1;
}
int x86PUSH(unsigned char *stream, int num)
{
	if((char)(num) == num)
	{
		stream[0] = 0x6a;
		stream[1] = (char)(num);
		return 2;
	}
	stream[0] = 0x68;
	*(int*)(stream+1) = num;
	return 5;
}

// pop dword [regA+regB+shift]
int x86POP(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	assert(size == sDWORD);
	if(regB == rNONE && regA != rNONE)	// swap so if there is only one register, it will be base
	{
		regB = regA;
		regA = rNONE;
	}
	stream[0] = 0x8f;
	unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, 0);
	return 1+asize;
}
// pop reg
int x86POP(unsigned char *stream, x86Reg reg)
{
	stream[0] = 0x58 + regCode[reg];
	return 1;
}

int x86MOV(unsigned char *stream, x86Reg dst, int src)
{
	stream[0] = 0xb8 + regCode[dst];
	*(int*)(stream+1) = src;
	return 5;
}
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src)
{
	stream[0] = 0x89;
	stream[1] = encodeRegister(dst, regCode[src]);
	return 2;
}
// mov dst, dword [src+shift]
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src, x86Size size, int shift)
{
	assert(size == sDWORD);
	if(dst == rEAX && (char)(shift) != shift && src == rNONE)
	{
		stream[0] = 0xa1;
		*(int*)(stream+1) = shift;
		return 5;
	}
	stream[0] = 0x8b;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, src, shift, regCode[dst]);
	return 1 + asize;
}

// mov *word [regA+shift], num
int x86MOV(unsigned char *stream, x86Size size, x86Reg regA, int shift, int num)
{
	if(size == sBYTE)
	{
		assert((char)(num) == num);
		stream[0] = 0xc6;
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, regA, shift, 0);
		stream[1+asize] = (char)(num);
		return 2+asize;
	}else if(size == sWORD){
		assert((short int)(num) == num);
		stream[0] = 0x66;	// switch to word
		stream[1] = 0xc7;
		unsigned int asize = encodeAddress(stream+2, rNONE, 1, regA, shift, 0);
		*(short int*)(stream+2+asize) = (short int)(num);
		return 4+asize;
	}
	stream[0] = 0xc7;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, regA, shift, 0);
	*(int*)(stream+1+asize) = (int)(num);
	return 5+asize;
}

// mov *word [regA+regB+shift], src
int x86MOV(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift, x86Reg src)
{
	if(regB == rNONE && regA != rNONE)	// swap so if there is only one register, it will be base
	{
		regB = regA;
		regA = rNONE;
	}
	if(size == sBYTE)
	{
		if(src == rEAX && regA == rNONE && regB == rNONE)
		{
			stream[0] = 0xa2;
			*(int*)(stream+1) = shift;
			return 5;
		}
		stream[0] = 0x88;
		unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, regCode[src]);
		return 1+asize;
	}else if(size == sWORD){
		stream[0] = 0x66;	// switch to word
		if(src == rEAX && regA == rNONE && regB == rNONE)
		{
			stream[1] = 0xa3;
			*(int*)(stream+2) = shift;
			return 6;
		}
		stream[1] = 0x89;
		unsigned int asize = encodeAddress(stream+2, regA, 1, regB, shift, regCode[src]);
		return 2+asize;
	}
	if(src == rEAX && regA == rNONE && regB == rNONE)
	{
		stream[0] = 0xa3;
		*(int*)(stream+1) = shift;
		return 5;
	}
	stream[0] = 0x89;
	unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, regCode[src]);
	return 1+asize;
}

// movsx dst, *word [regA+regB+shift]
int x86MOVSX(unsigned char *stream, x86Reg dst, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	assert(size != sDWORD && size != sQWORD);
	if(regB == rNONE && regA != rNONE)	// swap so if there is only one register, it will be base
	{
		regB = regA;
		regA = rNONE;
	}
	stream[0] = 0x0f;
	stream[1] = size == sBYTE ? 0xbe : 0xbf;
	unsigned int asize = encodeAddress(stream+2, regA, 1, regB, shift, regCode[dst]);
	return 2+asize;
}

// lea dst, [label+shift]
int x86LEA(unsigned char *stream, x86Reg dst, unsigned int labelID, int shift)
{
	labelID &= 0x7FFFFFFF;
	(void)shift;
	LabelInfo info;
	stream[0] = 0x8d;
	if(!FindLabel(labelID, info))
	{
		pendingJumps.push_back(UnsatisfiedJump(labelID, false, stream));
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, rNONE, 0xcdcdcdcd, regCode[dst]);
		assert(asize == 5);
		return 1 + asize;
	}
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, rNONE, (int)(long long)info.pos, regCode[dst]);
	return 1 + asize;
}
// lea dst, [src+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Reg src, int shift)
{
	stream[0] = 0x8d;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, src, shift, regCode[dst]);
	return 1 + asize;
}
// lea dst, [src*multiplier+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Reg src, int multiplier, int shift)
{
	stream[0] = 0x8d;
	unsigned int asize = encodeAddress(stream+1, src, multiplier, rNONE, shift, regCode[dst]);
	return 1 + asize;
}


// neg dword [reg+shift]
int x86NEG(unsigned char *stream, x86Size size, x86Reg reg, int shift)
{
	assert(size == sDWORD);
	stream[0] = 0xf7;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 3);
	return 1 + asize;
}

// add dst, num
int x86ADD(unsigned char *stream, x86Reg dst, int num)
{
	if((char)(num) == num)
	{
		stream[0] = 0x83;
		stream[1] = encodeRegister(dst, 0);
		stream[2] = (char)(num);
		return 3;
	}
	// else
	stream[0] = 0x81;
	stream[1] = encodeRegister(dst, 0);
	*(int*)(stream+2) = num;
	return 6;
}
// add dword [reg+shift], num
int x86ADD(unsigned char *stream, x86Size size, x86Reg reg, int shift, int num)
{
	assert(size == sDWORD);
	if((char)(num) == num)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 0);
		stream[1+asize] = (char)(num);
		return asize + 2;
	}
	// else
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 0);
	*(int*)(stream+1+asize) = num;
	return asize + 5;
}
// add dword [reg+shift], op2
int x86ADD(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x01;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1 + asize;
}

// adc dst, num
int x86ADC(unsigned char *stream, x86Reg dst, int num)
{
	if((char)num == num)
	{
		stream[0] = 0x83;
		stream[1] = encodeRegister(dst, 2);
		stream[2] = (char)num;
		return 3;
	}
	stream[0] = 0x81;
	stream[1] = encodeRegister(dst, 2);
	*(int*)(stream+2) = num;
	return 6;
}
// adc dword [reg+shift], num
int x86ADC(unsigned char *stream, x86Size size, x86Reg reg, int shift, int num)
{
	assert(size == sDWORD);
	if((char)num == num)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 2);
		stream[1+asize] = (char)num;
		return 2 + asize;
	}
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 2);
	*(int*)(stream+1+asize) = num;
	return 5 + asize;
}
// adc dword [reg+shift], op2
int x86ADC(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x11;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1 + asize;
}

// sub dst, num
int x86SUB(unsigned char *stream, x86Reg dst, int num)
{
	if((char)(num) == num)
	{
		stream[0] = 0x83;
		stream[1] = encodeRegister(dst, 5);
		stream[2] = (char)(num);
		return 3;
	}
	if(dst == rEAX)
	{
		stream[0] = 0x2d;
		*(int*)(stream+1) = num;
		return 5;
	}
	stream[0] = 0x81;
	stream[1] = encodeRegister(dst, 5);
	*(int*)(stream+2) = num;
	return 6;
}
// sub dword [reg+shift], num
int x86SUB(unsigned char *stream, x86Size size, x86Reg reg, int shift, int num)
{
	assert(size == sDWORD);
	if((char)(num) == num)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 5);
		stream[1+asize] = (char)(num);
		return asize + 2;
	}
	// else
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 5);
	*(int*)(stream+1+asize) = num;
	return asize + 5;
}
// sub dword [reg+shift], op2
int x86SUB(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x29;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1 + asize;
}

// sbb dst, num
int x86SBB(unsigned char *stream, x86Reg dst, int num)
{
	if((char)num == num)
	{
		stream[0] = 0x83;
		stream[1] = encodeRegister(dst, 3);
		stream[2] = (char)num;
		return 3;
	}
	stream[0] = 0x81;
	stream[1] = encodeRegister(dst, 3);
	*(int*)(stream+2) = num;
	return 6;
}
// sbb dword [reg+shift], num
int x86SBB(unsigned char *stream, x86Size size, x86Reg reg, int shift, int num)
{
	assert(size == sDWORD);
	if((char)(num) == num)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 3);
		stream[1+asize] = (char)(num);
		return asize + 2;
	}
	// else
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 3);
	*(int*)(stream+1+asize) = num;
	return asize + 5;
}
// sbb dword [reg+shift], op2
int x86SBB(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x19;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1 + asize;
}

// imul dst, num
int x86IMUL(unsigned char *stream, x86Reg srcdst, int num)
{
	if((char)(num) == num)
	{
		stream[0] = 0x6b;
		stream[1] = encodeRegister(srcdst, regCode[srcdst]);
		stream[2] = (char)(num);
		return 3;
	}
	stream[0] = 0x69;
	stream[1] = encodeRegister(srcdst, regCode[srcdst]);
	*(int*)(stream+2) = num;
	return 6;
}
// imul src
int x86IMUL(unsigned char *stream, x86Reg src)
{
	stream[0] = 0xf7;
	stream[1] = encodeRegister(src, 5);
	return 2;
}

// idiv dword [reg]
int x86IDIV(unsigned char *stream, x86Size size, x86Reg reg)
{
	assert(size == sDWORD);
	stream[0] = 0xf7;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, 0, 7);
	return 1+asize;
}

// shl reg, shift
int x86SHL(unsigned char *stream, x86Reg reg, int shift)
{
	assert((char)(shift) == shift);
	if(shift == 1)
		stream[0] = 0xd1;
	else
		stream[0] = 0xc1;
	stream[1] = encodeRegister(reg, 4);
	if(shift != 1)
		stream[2] = (char)(shift);
	return (shift == 1 ? 2 : 3);
}
// shl dword [reg], shift
int x86SHL(unsigned char *stream, x86Size size, x86Reg reg, int shift)
{
	assert(size == sDWORD);
	if(shift == 1)
		stream[0] = 0xd1;
	else
		stream[0] = 0xc1;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, 0, 4);
	if(shift != 1)
		stream[1+asize] = (char)(shift);
	return (shift == 1 ? 1 : 2) + asize;
}

// sal eax, cl
int x86SAL(unsigned char *stream)
{
	stream[0] = 0xd3;
	stream[1] = encodeRegister(rEAX, 4);
	return 2;
}
// sar eax, cl
int x86SAR(unsigned char *stream)
{
	stream[0] = 0xd3;
	stream[1] = encodeRegister(rEAX, 7);
	return 2;
}

// not dword [reg+shift]
int x86NOT(unsigned char *stream, x86Size size, x86Reg reg, int shift)
{
	assert(size == sDWORD);
	stream[0] = 0xf7;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 2);
	return 1 + asize;
}

// and dword [reg+shift], op2
int x86AND(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x21;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1 + asize;
}

// or op1, op2
int x86OR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	stream[0] = 0x09;
	stream[1] = encodeRegister(op1, regCode[op2]);
	return 2;
}
// or dword [reg+shift], op2
int x86OR(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x09;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1 + asize;
}
// or op1, dword [reg+shift]
int x86OR(unsigned char *stream, x86Reg op1, x86Size size, x86Reg reg, int shift)
{
	assert(size == sDWORD);
	stream[0] = 0x0B;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op1]);
	return 1 + asize;
}

// xor op1, op2
int x86XOR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	stream[0] = 0x31;
	stream[1] = encodeRegister(op1, regCode[op2]);
	return 2;
}
// xor dword [reg+shift], op2
int x86XOR(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x31;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1 + asize;
}

// cmp reg, num
int x86CMP(unsigned char *stream, x86Reg reg, int num)
{
	if((char)(num) == num)
	{
		stream[0] = 0x83;
		stream[1] = encodeRegister(reg, 7);
		stream[2] = (char)num;
		return 3;
	}
	stream[0] = 0x81;
	stream[1] = encodeRegister(reg, 7);
	*(int*)(stream+2) = num;
	return 6;
}
// cmp reg1, reg2
int x86CMP(unsigned char *stream, x86Reg reg1, x86Reg reg2)
{
	stream[0] = 0x39;
	stream[1] = encodeRegister(reg1, regCode[reg2]);
	return 2;
}
// cmp dword [reg], op2
int x86CMP(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x39;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1+asize;
}
// cmp dword [reg+shift], num
int x86CMP(unsigned char *stream, x86Size size, x86Reg reg, int shift, int op2)
{
	assert(size == sDWORD);
	if((char)(op2) == op2)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 7);
		stream[1+asize] = (char)op2;
		return 2+asize;
	}
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, 7);
	*(int*)(stream+1+asize) = op2;
	return 5+asize;
}

int x86TEST(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	stream[0] = 0x85;
	stream[1] = encodeRegister(op1, regCode[op2]);
	return 2;
}
// test ah, num
int x86TESTah(unsigned char* stream, char num)
{
	stream[0] = 0xf6;
	stream[1] = encodeRegister(rESP, 0);
	stream[2] = num;
	return 3;
}

// xchg dword [reg], op2
int x86XCHG(unsigned char *stream, x86Size size, x86Reg reg, int shift, x86Reg op2)
{
	assert(size == sDWORD);
	stream[0] = 0x87;
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1+asize;
}
// xchg regA, regB
int x86XCHG(unsigned char *stream, x86Reg regA, x86Reg regB)
{
	if(regB == rEAX)
	{
		regB = regA;
		regA = rEAX;
	}
	if(regA == rEAX)
	{
		stream[0] = 0x90 + regCode[regB];
		return 1;
	}
	stream[0] = 0x87;
	stream[1] = encodeRegister(regA, regCode[regB]);
	return 2;
}

int x86CDQ(unsigned char *stream)
{
	stream[0] = 0x99;
	return 1;
}

// setcc *l
int x86SETcc(unsigned char *stream, x86Cond cond, x86Reg reg)
{
	stream[0] = 0x0f;
	stream[1] = 0x90 + condCode[cond];
	stream[2] = encodeRegister(reg, 0);
	return 3;
}

int x86CALL(unsigned char *stream, x86Reg address)
{
	stream[0] = 0xff;
	stream[1] = encodeRegister(address, 2);
	return 2;
}
int x86CALL(unsigned char *stream, unsigned int labelID)
{
	labelID &= 0x7FFFFFFF;
	stream[0] = 0xe8;

	LabelInfo info;
	if(!FindLabel(labelID, info))
	{
		pendingJumps.push_back(UnsatisfiedJump(labelID, true, stream));
	}else{
		*(int*)(stream+1) = (int)(info.pos-stream-5);
	}
	return 5;
}
int x86RET(unsigned char *stream)
{
	stream[0] = 0xc3;
	return 1;
}

int x86REP_MOVSD(unsigned char *stream)
{
	stream[0] = 0xf3;
	stream[1] = 0xa5;
	return 2;
}

int x86INT(unsigned char *stream, int interrupt)
{
	stream[0] = 0xcd;
	stream[1] = (unsigned char)interrupt;
	return 2;
}

int x86NOP(unsigned char *stream)
{
	stream[0] = 0x90;
	return 1;
}

int x86Jcc(unsigned char *stream, unsigned int labelID, x86Cond cond, bool isNear)
{
	labelID &= 0x7FFFFFFF;
	LabelInfo info;

	if(isNear)
	{
		stream[0] = 0x0f;
		stream[1] = 0x80 + condCode[cond];
	}else{
		stream[0] = 0x70 + condCode[cond];
	}

	if(!FindLabel(labelID, info))
	{
		pendingJumps.push_back(UnsatisfiedJump(labelID, isNear, stream));
	}else{
		if(isNear)
		{
			*(int*)(stream+2) = (int)(info.pos-stream-6);
		}else{
			assert(info.pos-stream + 128 < 256);
			stream[1] = (char)(info.pos-stream-2);
		}
	}
	return (isNear ? 6 : 2);
}

int x86JMP(unsigned char *stream, unsigned int labelID, bool isNear)
{
	labelID &= 0x7FFFFFFF;
	LabelInfo info;

	if(isNear)
		stream[0] = 0xE9;
	else
		stream[0] = 0xEB;

	if(!FindLabel(labelID, info))
	{
		pendingJumps.push_back(UnsatisfiedJump(labelID, isNear, stream));
	}else{
		if(isNear)
		{
			*(int*)(stream+1) = (int)(info.pos-stream-5);
		}else{
			assert(info.pos-stream + 128 < 256);
			stream[1] = (char)(info.pos-stream-2);
		}
	}
	return (isNear ? 5 : 2);
}

void x86AddLabel(unsigned char *stream, unsigned int labelID)
{
	labelID &= 0x7FFFFFFF;
	labels.push_back(LabelInfo(labelID, stream));
	for(unsigned int i = 0; i < pendingJumps.size(); i++)
	{
		UnsatisfiedJump& uJmp = pendingJumps[i];
		if(uJmp.labelID == labelID)
		{
			if(*uJmp.jmpPos == 0xe8)	// This one is for call label
			{
				*(int*)(uJmp.jmpPos+1) = (int)(stream-uJmp.jmpPos-5);
			}else if(*uJmp.jmpPos == 0x8d){	// This one is for lea reg, [label+offset]
				*(int*)(uJmp.jmpPos+2) = (int)(long long)(stream);
			}else{
				if(uJmp.isNear)
				{
					if(*uJmp.jmpPos == 0x0f)
						*(int*)(uJmp.jmpPos+2) = (int)(stream-uJmp.jmpPos-6);
					else
						*(int*)(uJmp.jmpPos+1) = (int)(stream-uJmp.jmpPos-5);
				}else{
					assert(uJmp.jmpPos-stream + 128 < 256);
					*(char*)(uJmp.jmpPos+1) = (char)(stream-uJmp.jmpPos-2);
				}
			}
			uJmp.labelID = (unsigned int)-1;
		}
	}
}

void x86SatisfyJumps(FastVector<unsigned char*>& instPos)
{
	for(unsigned int i = 0; i < pendingJumps.size(); i++)
	{
		UnsatisfiedJump& uJmp = pendingJumps[i];
		if(uJmp.labelID == -1)
			continue;
		if(*uJmp.jmpPos == 0xe8)	// This one is for call label
		{
			*(int*)(uJmp.jmpPos+1) = (int)(instPos[uJmp.labelID&0x00ffffff] - uJmp.jmpPos-5);
		}else if(*uJmp.jmpPos == 0x8d){	// This one is for lea reg, [label+offset]
			*(int*)(uJmp.jmpPos+2) = (int)(long long)(instPos[uJmp.labelID&0x00ffffff]);
		}else{
			if(uJmp.isNear)
			{
				if(*uJmp.jmpPos == 0x0f)
					*(int*)(uJmp.jmpPos+2) = (int)(instPos[uJmp.labelID&0x00ffffff] - uJmp.jmpPos-6);
				else
					*(int*)(uJmp.jmpPos+1) = (int)(instPos[uJmp.labelID&0x00ffffff] - uJmp.jmpPos-5);
			}else{
				assert(uJmp.jmpPos - instPos[uJmp.labelID&0x00ffffff] + 128 < 256);
				*(char*)(uJmp.jmpPos+1) = (char)(instPos[uJmp.labelID&0x00ffffff] - uJmp.jmpPos-2);
			}
		}
	}
	pendingJumps.clear();
}
