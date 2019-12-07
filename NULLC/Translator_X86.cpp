#include "Translator_X86.h"

// Mapping from x86Reg to register code
char regCode[] = { -1, 0, 3, 1, 2, 4, 7, 5, 6, 0, 1, 2, 3, 4, 5, 6, 7 };

// x87Reg are mapped to FP register codes directly
//char	fpCode[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

// Mapping from x86Cond to x86 conditions
//					   o  no b  c nae ae nb nc e  z ne nz be na  a nbe s ns  p   pe  np  po  l  nge  ge  nl  le  ng  g   nle
char	condCode[] = { 0, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15 };

// spareField can be found in nasmdoc as /0-7 or /r codes in instruction bytecode
// encode register as address
unsigned char encodeRegister(x86Reg reg, char spareField)
{
	assert(unsigned(spareField) <= 0x7);

	unsigned char mod = 3 << 6;
	unsigned char spare = spareField << 3;
	unsigned char RM = regCode[reg];
	return mod | spare | RM;
}

unsigned char encodeRegister(x86XmmReg dst, x86XmmReg src)
{
	unsigned char mod = 3 << 6;
	unsigned char spare = (unsigned char)(src & 0x7) << 3;
	unsigned char RM = (unsigned char)(dst & 0x7);
	return mod | spare | RM;
}

unsigned char encodeRegister(x86Reg dst, x86XmmReg src)
{
	unsigned char mod = 3 << 6;
	unsigned char spare = (unsigned char)(src & 0x7) << 3;
	unsigned char RM = regCode[dst];
	return mod | spare | RM;
}

unsigned char encodeRegister(x86XmmReg dst, char spareField)
{
	assert(unsigned(spareField) <= 0x7);
	assert(unsigned(dst) <= 0x7);

	unsigned char mod = 3 << 6;
	unsigned char spare = spareField << 3;
	unsigned char RM = (unsigned char)dst;
	return mod | spare | RM;
}

// encode [base], [base+displacement], [index*multiplier+displacement] and [index*multiplier+base+displacement]
unsigned int	encodeAddress(unsigned char* stream, x86Reg index, int multiplier, x86Reg base, int displacement, unsigned spareField)
{
	assert(index != rESP);
	assert(spareField < 16);

	unsigned char* start = stream;

	bool dispImm8 = (char)(displacement) == displacement && base != rNONE;

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

	// special case: [r13] should be encoded as [r13+0]
	if(displacement == 0 && base == rR13)
		mod = 1 << 6;

	if(index == rNONE && base == rNONE)
		mod = 0;

	unsigned char spare = (unsigned char)(spareField & 0x7) << 3;

	unsigned char RM = regCode[rEBP]; // by default, it's simply [displacement]
	if(base != rNONE)
		RM = regCode[base];	// this is [base + displacement]
	if(index != rNONE)
		RM = regCode[rESP];	// this changes mode to [index*multiplier + base + displacement]

	unsigned char sibBase = regCode[base];

	if(index != rNONE && base == rNONE)
	{
		mod = 0;
		sibBase = regCode[rEBP];
	}

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

unsigned encodeRex(unsigned char* stream, bool operand64Bit, x86Reg reg, x86Reg index, x86Reg base)
{
	unsigned char code = (operand64Bit ? 0x08 : 0x00) | (reg >= rR8 ? 0x04 : 0x00) | (index >= rR8 ? 0x02 : 0x00) | (base >= rR8 ? 0x01 : 0x00);

	if(code)
	{
		*stream = 0x40 | code;
		return 1;
	}

	return 0;
}

unsigned encodeRex(unsigned char* stream, bool operand64Bit, x86XmmReg reg, x86Reg index, x86Reg base)
{
	unsigned char code = (operand64Bit ? 0x08 : 0x00) | (reg >= rXMM8 ? 0x04 : 0x00) | (index >= rR8 ? 0x02 : 0x00) | (base >= rR8 ? 0x01 : 0x00);

	if(code)
	{
		*stream = 0x40 | code;
		return 1;
	}

	return 0;
}

unsigned encodeRex(unsigned char* stream, bool operand64Bit, x86Reg dst, x86Reg src)
{
	unsigned char code = (operand64Bit ? 0x08 : 0x00) | (dst >= rR8 ? 0x04 : 0x00) | (src >= rR8 ? 0x01 : 0x00);

	if(code)
	{
		*stream = 0x40 | code;
		return 1;
	}

	return 0;
}

unsigned encodeRex(unsigned char* stream, bool operand64Bit, x86XmmReg dst, x86XmmReg src)
{
	unsigned char code = (operand64Bit ? 0x08 : 0x00) | (dst >= rXMM8 ? 0x04 : 0x00) | (src >= rXMM8 ? 0x01 : 0x00);

	if(code)
	{
		*stream = 0x40 | code;
		return 1;
	}

	return 0;
}

unsigned encodeRex(unsigned char* stream, bool operand64Bit, x86Reg dst, x86XmmReg src)
{
	unsigned char code = (operand64Bit ? 0x08 : 0x00) | (dst >= rR8 ? 0x04 : 0x00) | (src >= rXMM8 ? 0x01 : 0x00);

	if(code)
	{
		*stream = 0x40 | code;
		return 1;
	}

	return 0;
}

unsigned encodeRex(unsigned char* stream, bool operand64Bit, x86XmmReg dst, x86Reg src)
{
	unsigned char code = (operand64Bit ? 0x08 : 0x00) | (dst >= rXMM8 ? 0x04 : 0x00) | (src >= rR8 ? 0x01 : 0x00);

	if(code)
	{
		*stream = 0x40 | code;
		return 1;
	}

	return 0;
}

unsigned encodeImmByte(unsigned char* stream, char num)
{
	memcpy(stream, &num, sizeof(num));
	return sizeof(num);
}

unsigned encodeImmWord(unsigned char* stream, short num)
{
	memcpy(stream, &num, sizeof(num));
	return sizeof(num);
}

unsigned encodeImmDword(unsigned char* stream, int num)
{
	memcpy(stream, &num, sizeof(num));
	return sizeof(num);
}

unsigned encodeImmQword(unsigned char* stream, uintptr_t num)
{
	memcpy(stream, &num, sizeof(num));
	return sizeof(num);
}

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

FastVector<unsigned char*>	labels;
FastVector<UnsatisfiedJump> pendingJumps;

void x86ResetLabels()
{
	labels.reset();
	pendingJumps.reset();
}

void x86ClearLabels()
{
	labels.clear();
	pendingJumps.clear();
}

void x86ReserveLabels(unsigned int count)
{
	labels.resize(count);
}

// movss dword [index*mult+base+shift], xmm*
int x86MOVSS(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86XmmReg src)
{
	unsigned char *start = stream;

	assert(size == sDWORD);

	*stream++ = 0xf3;
	stream += encodeRex(stream, false, src, index, base);
	*stream++ = 0x0f;
	*stream++ = 0x11;
	stream += encodeAddress(stream, index, multiplier, base, shift, (char)src);

	return int(stream - start);
}

// movsd qword [index*mult+base+shift], xmm*
int x86MOVSD(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86XmmReg src)
{
	unsigned char *start = stream;

	assert(size == sQWORD);

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, src, index, base);
	*stream++ = 0x0f;
	*stream++ = 0x11;
	stream += encodeAddress(stream, index, multiplier, base, shift, (char)src);

	return int(stream - start);
}

// movsd xmm*, qword [index*mult+base+shift]
int x86MOVSD(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sQWORD);

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, index, base);
	*stream++ = 0x0f;
	*stream++ = 0x10;
	stream += encodeAddress(stream, index, multiplier, base, shift, (char)dst);

	return int(stream - start);
}

// movsd xmm*, xmm*
int x86MOVSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0x10;
	*stream++ = encodeRegister(src, dst);

	return int(stream - start);
}

// movd reg, xmm*
int x86MOVD(unsigned char *stream, x86Reg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0x66;
	stream += encodeRex(stream, false, src, dst);
	*stream++ = 0x0f;
	*stream++ = 0x7e;
	*stream++ = encodeRegister(dst, src);

	return int(stream - start);
}

// movsxd reg, dword [index*mult+base+shift]
int x86MOVSXD(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	(void)size;

	unsigned char *start = stream;

	stream += encodeRex(stream, true, dst, index, base);
	*stream++ = 0x63;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// cvtss2sd xmm*, dword [index*mult+base+shift]
int x86CVTSS2SD(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD);

	*stream++ = 0xf3;
	stream += encodeRex(stream, false, dst, index, base);
	*stream++ = 0x0f;
	*stream++ = 0x5A;
	stream += encodeAddress(stream, index, multiplier, base, shift, (char)dst);

	return int(stream - start);
}

// cvtsd2ss xmm*, qword [index*mult+base+shift]
int x86CVTSD2SS(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sQWORD);

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, index, base);
	*stream++ = 0x0f;
	*stream++ = 0x5A;
	stream += encodeAddress(stream, index, multiplier, base, shift, (char)dst);

	return int(stream - start);
}

// cvttsd2si dst, qword [index*mult+base+shift]
int x86CVTTSD2SI(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sQWORD);

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, index, base);
	*stream++ = 0x0f;
	*stream++ = 0x2C;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// REX.W cvttsd2si dst, qword [index*mult+base+shift]
int x64CVTTSD2SI(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sQWORD);

	*stream++ = 0xf2;
	stream += encodeRex(stream, true, dst, index, base);
	*stream++ = 0x0f;
	*stream++ = 0x2C;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// cvtsi2sd xmm*, *word [index*mult+base+shift]
int x86CVTSI2SD(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	*stream++ = 0xf2;
	stream += encodeRex(stream, size == sQWORD, dst, index, base);
	*stream++ = 0x0f;
	*stream++ = 0x2A;
	stream += encodeAddress(stream, index, multiplier, base, shift, (char)dst);

	return int(stream - start);
}

int x86ADDSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0x58;
	*stream++ = encodeRegister(src, dst);

	return int(stream - start);
}

int x86SUBSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0x5c;
	*stream++ = encodeRegister(src, dst);

	return int(stream - start);
}

int x86MULSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0x59;
	*stream++ = encodeRegister(src, dst);

	return int(stream - start);
}

int x86DIVSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0x5e;
	*stream++ = encodeRegister(src, dst);

	return int(stream - start);
}

int x86CMPEQSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0xc2;
	*stream++ = encodeRegister(src, dst);
	*stream++ = 0;

	return int(stream - start);
}

int x86CMPLTSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0xc2;
	*stream++ = encodeRegister(src, dst);
	*stream++ = 1;

	return int(stream - start);
}

int x86CMPLESD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0xc2;
	*stream++ = encodeRegister(src, dst);
	*stream++ = 2;

	return int(stream - start);
}

int x86CMPNEQSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf2;
	stream += encodeRex(stream, false, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0xc2;
	*stream++ = encodeRegister(src, dst);
	*stream++ = 4;

	return int(stream - start);
}
// fcomp *word [index*mult+base+shift]
int x86FCOMP(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	assert(size != sBYTE && size != sWORD);
	stream[0] = size == sDWORD ? 0xd8 : 0xdc;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 3);
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

// push dword [index*mult+base+shift]
int x86PUSH(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	(void)size;
	assert(size == sDWORD);
	if(base == rNONE && index != rNONE && multiplier == 1)	// swap so if there is only one register, it will be base
	{
		base = index;
		index = rNONE;
	}
	stream[0] = 0xff;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 6);
	return 1+asize;
}

int x86PUSH(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, reg);
	*stream++ = 0x50 + regCode[reg];

	return int(stream - start);
}

int x86PUSH(unsigned char *stream, int num)
{
	unsigned char *start = stream;

	if((char)(num) == num)
	{
		*stream++ = 0x6a;
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x68;
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// pop dword [index*mult+base+shift]
int x86POP(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	(void)size;
	assert(size == sDWORD);
	if(base == rNONE && index != rNONE && multiplier == 1)	// swap so if there is only one register, it will be base
	{
		base = index;
		index = rNONE;
	}
	stream[0] = 0x8f;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 0);
	return 1+asize;
}

// pop reg
int x86POP(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, reg);
	*stream++ = 0x58 + regCode[reg];

	return int(stream - start);
}

int x86PUSHAD(unsigned char *stream)
{
	stream[0] = 0x60;
	return 1;
}

int x86POPAD(unsigned char *stream)
{
	stream[0] = 0x61;
	return 1;
}

int x86MOV(unsigned char *stream, x86Reg dst, int src)
{
	unsigned char *start = stream;

	*stream++ = 0xb8 + regCode[dst];
	stream += encodeImmDword(stream, src);

	return int(stream - start);
}

int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	*stream++ = 0x89;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// REX.W mov dst, num
int x64MOV(unsigned char *stream, x86Reg dst, uintptr_t num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, dst);
	*stream++ = 0xb8 + regCode[dst];
	stream += encodeImmQword(stream, num);

	return int(stream - start);
}

// REX.W mov dst, src
int x64MOV(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, dst, rNONE, src);
	*stream++ = 0x89;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// mov dst, *word [index*mult+base+shift]
int x86MOV(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	(void)size;
	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, dst, index, base);

	if(dst == rEAX && (char)(shift) != shift && index == rNONE && base == rNONE)
	{
		*stream++ = 0xa1;
		stream += encodeImmDword(stream, shift);

		return int(stream - start);
	}

	*stream++ = 0x8b;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// mov *word [index*mult+base+shift], num
int x86MOV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	if(size == sBYTE)
	{
		assert((char)(num) == num);

		stream += encodeRex(stream, false, rNONE, index, base);
		*stream++ = 0xc6;
		stream += encodeAddress(stream, index, multiplier, base, shift, 0);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}
	else if(size == sWORD)
	{
		assert((short int)(num) == num);

		*stream++ = 0x66;	// switch to word
		stream += encodeRex(stream, false, rNONE, index, base);
		*stream++ = 0xc7;

		stream += encodeAddress(stream, index, multiplier, base, shift, 0);
		stream += encodeImmWord(stream, (short)num);

		return int(stream - start);
	}

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	*stream++ = 0xc7;
	stream += encodeAddress(stream, index, multiplier, base, shift, 0);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// mov *word [index*mult+base+shift], src
int x86MOV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg src)
{
	unsigned char *start = stream;

	if(base == rNONE && index != rNONE && multiplier == 1)	// swap so if there is only one register, it will be base
	{
		base = index;
		index = rNONE;
	}

	if(size == sBYTE)
	{
		if(src == rEAX && index == rNONE && base == rNONE)
		{
			*stream++ = 0xa2;
			stream += encodeImmDword(stream, shift);

			return int(stream - start);
		}

		stream += encodeRex(stream, false, src, index, base);
		*stream++ = 0x88;
		stream += encodeAddress(stream, index, multiplier, base, shift, regCode[src]);

		return int(stream - start);
	}
	else if(size == sWORD)
	{
		*stream++ = 0x66;	// switch to word

		if(src == rEAX && index == rNONE && base == rNONE)
		{
			*stream++ = 0xa3;
			stream += encodeImmDword(stream, shift);

			return int(stream - start);
		}

		stream += encodeRex(stream, false, src, index, base);
		*stream++ = 0x89;
		stream += encodeAddress(stream, index, multiplier, base, shift, regCode[src]);

		return int(stream - start);
	}

	stream += encodeRex(stream, size == sQWORD, src, index, base);

	if(src == rEAX && index == rNONE && base == rNONE)
	{
		*stream++ = 0xa3;
		stream += encodeImmDword(stream, shift);

		return int(stream - start);
	}

	*stream++ = 0x89;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[src]);

	return int(stream - start);
}

// movsx dst, *word [index*mult+base+shift]
int x86MOVSX(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size != sDWORD && size != sQWORD);
	if(base == rNONE && index != rNONE && multiplier == 1)	// swap so if there is only one register, it will be base
	{
		base = index;
		index = rNONE;
	}

	stream += encodeRex(stream, false, dst, index, base);

	*stream++ = 0x0f;
	*stream++ = size == sBYTE ? 0xbe : 0xbf;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// lea dst, [label+shift]
int x86LEA(unsigned char *stream, x86Reg dst, unsigned int labelID, int shift)
{
	labelID &= 0x7FFFFFFF;
	(void)shift;

	stream[0] = 0x8d;
	
	pendingJumps.push_back(UnsatisfiedJump(labelID, true, stream));
	unsigned int asize = encodeAddress(stream+1, rNONE, 1, rNONE, 0xcdcdcdcd, regCode[dst]);
	assert(asize == 5);
	return 1 + asize;
}

// lea dst, [index*multiplier+base+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, dst, index, base);

	*stream++ = 0x8d;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// neg reg
int x86NEG(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	*stream++ = 0xf7;
	*stream++ = encodeRegister(reg, 3);

	return int(stream - start);
}

// REX.W neg reg
int x64NEG(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, reg, rNONE, rNONE);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(reg, 3);

	return int(stream - start);
}

// neg dword [index*mult+base+shift]
int x86NEG(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0xf7;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 3);
	return 1 + asize;
}

// add dst, num
int x86ADD(unsigned char *stream, x86Reg dst, int num)
{
	unsigned char *start = stream;

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		*stream++ = encodeRegister(dst, 0);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(dst, 0);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// add dst, src
int x86ADD(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	*stream++ = 0x01;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// REX.W add dst, num
int x64ADD(unsigned char *stream, x86Reg dst, int num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, dst, rNONE, rNONE);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		*stream++ = encodeRegister(dst, 0);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(dst, 0);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// REX.W add dst, src
int x64ADD(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, dst, rNONE, src);

	*stream++ = 0x01;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// add dst, *word [index*mult+base+shift]
int x86ADD(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, dst, index, base);

	*stream++ = 0x03;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// add *word [index*mult+base+shift], num
int x86ADD(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		stream += encodeAddress(stream, index, multiplier, base, shift, 0);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	stream += encodeAddress(stream, index, multiplier, base, shift, 0);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// add *word [index*mult+base+shift], op2
int x86ADD(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op2, index, base);

	*stream++ = 0x01;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op2]);

	return int(stream - start);
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
// adc dst, src
int x86ADC(unsigned char *stream, x86Reg dst, x86Reg src)
{
	stream[0] = 0x11;
	stream[1] = encodeRegister(dst, regCode[src]);
	return 2;
}
// adc dword [index*mult+base+shift], num
int x86ADC(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	(void)size;
	assert(size == sDWORD);
	if((char)num == num)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 2);
		stream[1+asize] = (char)num;
		return 2 + asize;
	}
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 2);
	*(int*)(stream+1+asize) = num;
	return 5 + asize;
}
// adc dword [index*mult+base+shift], op2
int x86ADC(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0x11;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, regCode[op2]);
	return 1 + asize;
}

// sub dst, num
int x86SUB(unsigned char *stream, x86Reg dst, int num)
{
	unsigned char *start = stream;

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		*stream++ = encodeRegister(dst, 5);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	if(dst == rEAX)
	{
		*stream++ = 0x2d;
		stream += encodeImmDword(stream, num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(dst, 5);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// sub dst, src
int x86SUB(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	*stream++ = 0x2B;
	*stream++ = encodeRegister(src, regCode[dst]);

	return int(stream - start);
}

// REX.W add dst, num
int x64SUB(unsigned char *stream, x86Reg dst, int num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, dst, rNONE, rNONE);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		*stream++ = encodeRegister(dst, 5);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(dst, 5);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// REX.W add dst, src
int x64SUB(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, dst, rNONE, src);

	*stream++ = 0x2B;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// sub *word [index*mult+base+shift], num
int x86SUB(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		stream += encodeAddress(stream, index, multiplier, base, shift, 5);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	stream += encodeAddress(stream, index, multiplier, base, shift, 5);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// sub *word [index*mult+base+shift], op2
int x86SUB(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op2, index, base);

	*stream++ = 0x29;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op2]);

	return int(stream - start);
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
// sbb dword [index*mult+base+shift], num
int x86SBB(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	(void)size;
	assert(size == sDWORD);
	if((char)(num) == num)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 3);
		stream[1+asize] = (char)(num);
		return asize + 2;
	}
	// else
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 3);
	*(int*)(stream+1+asize) = num;
	return asize + 5;
}
// sbb dword [index*mult+base+shift], op2
int x86SBB(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0x19;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, regCode[op2]);
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

// imul dst, src
int x86IMUL(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	*stream++ = 0x0f;
	*stream++ = 0xaf;
	*stream++ = encodeRegister(src, regCode[dst]);

	return int(stream - start);
}

// REX.W imul dst, src
int x64IMUL(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, dst, src);
	*stream++ = 0x0f;
	*stream++ = 0xaf;
	*stream++ = encodeRegister(src, regCode[dst]);

	return int(stream - start);
}

// imul dst, dword [index*mult+base+shift]
int x86IMUL(unsigned char *stream, x86Reg dst, x86Size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	stream[0] = 0x0f;
	stream[1] = 0xaf;
	unsigned int asize = encodeAddress(stream+2, index, multiplier, base, shift, regCode[dst]);
	return 2+asize;
}

// imul src
int x86IMUL(unsigned char *stream, x86Reg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf7;
	*stream++ = encodeRegister(src, 5);

	return int(stream - start);
}

// idiv src
int x86IDIV(unsigned char *stream, x86Reg src)
{
	unsigned char *start = stream;

	*stream++ = 0xf7;
	*stream++ = encodeRegister(src, 7);

	return int(stream - start);
}

// REX.W idiv src
int x64IDIV(unsigned char *stream, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, src, rNONE, rNONE);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(src, 7);

	return int(stream - start);
}

// idiv dword [index*mult+base+shift]
int x86IDIV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0xf7;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 7);
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
// shl dword [index*mult+base+shift], shift
int x86SHL(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int val)
{
	(void)size;
	assert(size == sDWORD);
	if(val == 1)
		stream[0] = 0xd1;
	else
		stream[0] = 0xc1;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 4);
	if(val != 1)
		stream[1+asize] = (char)(val);
	return (val == 1 ? 1 : 2) + asize;
}

// sal reg, cl
int x86SAL(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	*stream++ = 0xd3;
	*stream++ = encodeRegister(reg, 4);

	return int(stream - start);
}

// REX.W sal reg, cl
int x64SAL(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, reg, rNONE, rNONE);
	*stream++ = 0xd3;
	*stream++ = encodeRegister(reg, 4);

	return int(stream - start);
}

// sar reg, cl
int x86SAR(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	*stream++ = 0xd3;
	*stream++ = encodeRegister(reg, 7);

	return int(stream - start);
}

// REX.W sar reg, cl
int x64SAR(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, reg, rNONE, rNONE);
	*stream++ = 0xd3;
	*stream++ = encodeRegister(reg, 7);

	return int(stream - start);
}

// not reg
int x86NOT(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	*stream++ = 0xf7;
	*stream++ = encodeRegister(reg, 2);

	return int(stream - start);
}

// not reg
int x64NOT(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, reg, rNONE, rNONE);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(reg, 2);

	return int(stream - start);
}

// not dword [index*mult+base+shift]
int x86NOT(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0xf7;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 2);
	return 1 + asize;
}

// and op1, op2
int x86AND(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	unsigned char *start = stream;

	*stream++ = 0x21;
	*stream++ = encodeRegister(op1, regCode[op2]);

	return int(stream - start);
}

// REX.W and op1, op2
int x64AND(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, op1, op2);
	*stream++ = 0x21;
	*stream++ = encodeRegister(op1, regCode[op2]);

	return int(stream - start);
}

// and op1, num
int x86AND(unsigned char *stream, x86Reg op1, int num)
{
	if(op1 == rEAX)
	{
		stream[0] = 0x25;
		memcpy(stream + 1, &num, sizeof(num));
		return 5;
	}

	stream[0] = 0x81;
	stream[1] = encodeRegister(op1, 4);
	memcpy(stream + 2, &num, sizeof(num));
	return 6;
}

// and dword [index*mult+base+shift], op2
int x86AND(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0x21;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, regCode[op2]);
	return 1 + asize;
}

// and dword [index*mult+base+shift], num
int x86AND(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	if((char)(num) == num)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 4);
		stream[1+asize] = (char)(num);
		return asize + 2;
	}
	// else
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 4);
	*(int*)(stream+1+asize) = num;
	return asize + 5;
}

// and op1, *word [index*mult+base+shift]
int x86AND(unsigned char *stream, x86Reg op1, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op1, index, base);
	*stream++ = 0x23;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op1]);

	return int(stream - start);
}

// or op1, op2
int x86OR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	unsigned char *start = stream;

	*stream++ = 0x09;
	*stream++ = encodeRegister(op1, regCode[op2]);

	return int(stream - start);
}

// REX.W or op1, op2
int x64OR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, op1, op2);
	*stream++ = 0x09;
	*stream++ = encodeRegister(op1, regCode[op2]);

	return int(stream - start);
}

// or dword [index*mult+base+shift], op2
int x86OR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0x09;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, regCode[op2]);
	return 1 + asize;
}

// or dword [index*mult+base+shift], num
int x86OR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int op2)
{
	(void)size;
	assert(size == sDWORD);
	if((char)(op2) == op2)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 1);
		stream[1+asize] = (char)op2;
		return 2+asize;
	}
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 1);
	*(int*)(stream+1+asize) = op2;
	return 5+asize;
}

// or op1, dword [index*mult+base+shift]
int x86OR(unsigned char *stream, x86Reg op1, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op1, index, base);
	*stream++ = 0x0B;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op1]);

	return int(stream - start);
}

// xor op1, op2
int x86XOR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	unsigned char *start = stream;

	*stream++ = 0x31;
	*stream++ = encodeRegister(op1, regCode[op2]);

	return int(stream - start);
}

// REX.W xor op1, op2
int x64XOR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, op1, op2);
	*stream++ = 0x31;
	*stream++ = encodeRegister(op1, regCode[op2]);

	return int(stream - start);
}

// xor dword [index*mult+base+shift], op2
int x86XOR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0x31;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, regCode[op2]);
	return 1 + asize;
}

// xor dword [index*mult+base+shift], num
int x86XOR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 6);
	*(int*)(stream+1+asize) = num;
	return 5 + asize;
}

// xor op1, dword [index*mult+base+shift]
int x86XOR(unsigned char *stream, x86Reg op1, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op1, index, base);
	*stream++ = 0x33;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op1]);

	return int(stream - start);
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
	unsigned char *start = stream;

	*stream++ = 0x39;
	*stream++ = encodeRegister(reg1, regCode[reg2]);

	return int(stream - start);
}

// REX.W cmp reg1, reg2
int x64CMP(unsigned char *stream, x86Reg reg1, x86Reg reg2)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, reg1, reg2);
	*stream++ = 0x39;
	*stream++ = encodeRegister(reg1, regCode[reg2]);

	return int(stream - start);
}

// cmp dword [reg], op2
int x86CMP(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0x39;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, regCode[op2]);
	return 1+asize;
}
// cmp op1, dword [index*mult+base+shift]
int x86CMP(unsigned char *stream, x86Reg op1, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	(void)size;
	assert(size == sDWORD);
	stream[0] = 0x3b;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, regCode[op1]);
	return 1+asize;
}
// cmp dword [index*mult+base+shift], num
int x86CMP(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int op2)
{
	(void)size;
	assert(size == sDWORD);
	if((char)(op2) == op2)
	{
		stream[0] = 0x83;
		unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 7);
		stream[1+asize] = (char)op2;
		return 2+asize;
	}
	stream[0] = 0x81;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 7);
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
	(void)size;
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

// cdq
int x86CDQ(unsigned char *stream)
{
	unsigned char *start = stream;

	*stream++ = 0x99;

	return int(stream - start);
}

// cqo
int x86CQO(unsigned char *stream)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, rNONE);
	*stream++ = 0x99;

	return int(stream - start);
}

// setcc *l
int x86SETcc(unsigned char *stream, x86Cond cond, x86Reg reg)
{
	stream[0] = 0x0f;
	stream[1] = 0x90 + condCode[cond];
	stream[2] = encodeRegister(reg, 0);
	return 3;
}

// call reg
int x86CALL(unsigned char *stream, x86Reg address)
{
	stream[0] = 0xff;
	stream[1] = encodeRegister(address, 2);
	return 2;
}
// call [index*mult+base+shift]
int x86CALL(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, unsigned int shift)
{
	stream[0] = 0xff;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 2);
	return 1+asize;
}
int x86CALL(unsigned char *stream, unsigned int labelID)
{
	labelID &= 0x7FFFFFFF;
	stream[0] = 0xe8;

	pendingJumps.push_back(UnsatisfiedJump(labelID, true, stream));
	return 5;
}
int x86RET(unsigned char *stream)
{
	stream[0] = 0xc3;
	return 1;
}

int x86REP_MOVSD(unsigned char *stream)
{
	unsigned char *start = stream;

	*stream++ = 0xf3;
	*stream++ = 0xa5;

	return int(stream - start);
}

int x86REP_STOSB(unsigned char *stream)
{
	unsigned char *start = stream;

	*stream++ = 0xf3;
	*stream++ = 0xaa;

	return int(stream - start);
}

int x86REP_STOSW(unsigned char *stream)
{
	unsigned char *start = stream;

	*stream++ = 0xf3;
	*stream++ = 0x66;
	*stream++ = 0xab;

	return int(stream - start);
}

int x86REP_STOSD(unsigned char *stream)
{
	unsigned char *start = stream;

	*stream++ = 0xf3;
	*stream++ = 0xab;

	return int(stream - start);
}

int x86REP_STOSQ(unsigned char *stream)
{
	unsigned char *start = stream;

	*stream++ = 0xf3;
	stream += encodeRex(stream, true, rNONE, rNONE, rNONE);
	*stream++ = 0xab;

	return int(stream - start);
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

	if(isNear)
	{
		stream[0] = 0x0f;
		stream[1] = 0x80 + condCode[cond];
	}else{
		stream[0] = 0x70 + condCode[cond];
	}

	pendingJumps.push_back(UnsatisfiedJump(labelID, isNear, stream));
	return (isNear ? 6 : 2);
}

// jmp [index*mult+base+shift]
int x86JMP(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, unsigned int shift)
{
	stream[0] = 0xff;
	unsigned int asize = encodeAddress(stream+1, index, multiplier, base, shift, 4);
	return 1+asize;
}

int x86JMP(unsigned char *stream, unsigned int labelID, bool isNear)
{
	labelID &= 0x7FFFFFFF;

	if(isNear)
		stream[0] = 0xE9;
	else
		stream[0] = 0xEB;

	pendingJumps.push_back(UnsatisfiedJump(labelID, isNear, stream));
	return (isNear ? 5 : 2);
}

void x86AddLabel(unsigned char *stream, unsigned int labelID)
{
	assert(labelID < labels.size());
	labels[labelID] = stream;
}

void x86SatisfyJumps(FastVector<unsigned char*>& instPos)
{
	for(unsigned int i = 0; i < pendingJumps.size(); i++)
	{
		UnsatisfiedJump& uJmp = pendingJumps[i];
		if(uJmp.isNear)
		{
			if(*uJmp.jmpPos == 0xe8)	// This one is for call label
			{
				*(int*)(uJmp.jmpPos+1) = (int)(instPos[uJmp.labelID & 0x00ffffff] - uJmp.jmpPos-5);
			}else if(*uJmp.jmpPos == 0x8d){	// This one is for lea reg, [label+offset]
				*(int*)(uJmp.jmpPos+2) = (int)(intptr_t)(instPos[uJmp.labelID & 0x00ffffff]);
			}else{
				if(*uJmp.jmpPos == 0x0f)
					*(int*)(uJmp.jmpPos+2) = (int)(instPos[uJmp.labelID & 0x00ffffff] - uJmp.jmpPos-6);
				else
					*(int*)(uJmp.jmpPos+1) = (int)(instPos[uJmp.labelID & 0x00ffffff] - uJmp.jmpPos-5);
			}
		}else{
			if(*uJmp.jmpPos == 0xe8)	// This one is for call label
			{
				*(int*)(uJmp.jmpPos+1) = (int)(labels[uJmp.labelID] - uJmp.jmpPos-5);
			}else if(*uJmp.jmpPos == 0x8d){	// This one is for lea reg, [label+offset]
				*(int*)(uJmp.jmpPos+2) = (int)(intptr_t)(labels[uJmp.labelID]);
			}else{
				assert(uJmp.jmpPos - labels[uJmp.labelID] + 128 < 256);
				*(char*)(uJmp.jmpPos+1) = (char)(labels[uJmp.labelID] - uJmp.jmpPos-2);
			}
		}
	}
	pendingJumps.clear();
}
