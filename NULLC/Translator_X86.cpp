#include "Translator_X86.h"

#include "CodeGen_X86.h"
#include "CodeGenRegVm_X86.h"
#include "Output.h"

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
	
	if(index != rNONE || base == rESP || base == rR12)
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
		assert(sizeof(void*) == 8);

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
		assert(sizeof(void*) == 8);

		*stream = 0x40 | code;
		return 1;
	}

	return 0;
}

unsigned encodeRex(unsigned char* stream, bool operand64Bit, x86Reg dst, x86Reg src)
{
	unsigned char code = (operand64Bit ? 0x08 : 0x00) | (src >= rR8 ? 0x04 : 0x00) | (dst >= rR8 ? 0x01 : 0x00); // Reverse for some reason

	if(code)
	{
		assert(sizeof(void*) == 8);

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
		assert(sizeof(void*) == 8);

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
		assert(sizeof(void*) == 8);

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
		assert(sizeof(void*) == 8);

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

unsigned encodeImmQword(unsigned char* stream, unsigned long long num)
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

// push dword [index*mult+base+shift]
int x86PUSH(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	if(base == rNONE && index != rNONE && multiplier == 1)	// swap so if there is only one register, it will be base
	{
		base = index;
		index = rNONE;
	}

	stream += encodeRex(stream, false, rNONE, index, base);
	*stream++ = 0xff;
	stream += encodeAddress(stream, index, multiplier, base, shift, 6);

	return int(stream - start);
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
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	if(base == rNONE && index != rNONE && multiplier == 1)	// swap so if there is only one register, it will be base
	{
		base = index;
		index = rNONE;
	}

	stream += encodeRex(stream, false, rNONE, index, base);
	*stream++ = 0x8f;
	stream += encodeAddress(stream, index, multiplier, base, shift, 0);

	return int(stream - start);
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
	unsigned char *start = stream;

	*stream++ = 0x60;

	return int(stream - start);
}

int x86POPAD(unsigned char *stream)
{
	unsigned char *start = stream;

	*stream++ = 0x61;

	return int(stream - start);
}

int x86MOV(unsigned char *stream, x86Reg dst, int src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, dst);
	*stream++ = 0xb8 + regCode[dst];
	stream += encodeImmDword(stream, src);

	return int(stream - start);
}

int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, src, rNONE, dst);
	*stream++ = 0x89;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// REX.W mov dst, num
int x64MOV(unsigned char *stream, x86Reg dst, unsigned long long num)
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

	stream += encodeRex(stream, true, src, rNONE, dst);
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
	unsigned char *start = stream;

	labelID &= 0x7FFFFFFF;
	(void)shift;

	*stream++ = 0x8d;
	
	pendingJumps.push_back(UnsatisfiedJump(labelID, true, stream));
	stream += encodeAddress(stream, rNONE, 1, rNONE, 0xcdcdcdcd, regCode[dst]);

	return int(stream - start);
}

// lea dst, [index*multiplier+base+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	(void)size;

	unsigned char *start = stream;

	stream += encodeRex(stream, size == sQWORD, dst, index, base);
	*stream++ = 0x8d;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// neg reg
int x86NEG(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, reg);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(reg, 3);

	return int(stream - start);
}

// REX.W neg reg
int x64NEG(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, reg);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(reg, 3);

	return int(stream - start);
}

// neg dword [index*mult+base+shift]
int x86NEG(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);
	*stream++ = 0xf7;
	stream += encodeAddress(stream, index, multiplier, base, shift, 3);

	return int(stream - start);
}

// add dst, num
int x86ADD(unsigned char *stream, x86Reg dst, int num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, dst);

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

	stream += encodeRex(stream, false, src, rNONE, dst);
	*stream++ = 0x01;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// REX.W add dst, num
int x64ADD(unsigned char *stream, x86Reg dst, int num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, dst);

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

	stream += encodeRex(stream, true, src, rNONE, dst);
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
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, dst);

	if((char)num == num)
	{
		*stream++ = 0x83;
		*stream++ = encodeRegister(dst, 2);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(dst, 2);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// adc dst, src
int x86ADC(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, src, rNONE, dst);
	*stream++ = 0x11;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// adc dword [index*mult+base+shift], num
int x86ADC(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	if((char)num == num)
	{
		*stream++ = 0x83;
		stream += encodeAddress(stream, index, multiplier, base, shift, 2);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	stream += encodeAddress(stream, index, multiplier, base, shift, 2);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// adc dword [index*mult+base+shift], op2
int x86ADC(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op2, index, base);
	*stream++ = 0x11;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op2]);

	return int(stream - start);
}

// sub dst, num
int x86SUB(unsigned char *stream, x86Reg dst, int num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, dst);

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

	stream += encodeRex(stream, false, src, rNONE, dst);
	*stream++ = 0x29;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// REX.W sub dst, num
int x64SUB(unsigned char *stream, x86Reg dst, int num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, dst);

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

// REX.W sub dst, src
int x64SUB(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, src, rNONE, dst);
	*stream++ = 0x29;
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
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, dst);

	if((char)num == num)
	{
		*stream++ = 0x83;
		*stream++ = encodeRegister(dst, 3);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(dst, 3);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// sbb dst, src
int x86SBB(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, src, rNONE, dst);
	*stream++ = 0x19;
	*stream++ = encodeRegister(dst, regCode[src]);

	return int(stream - start);
}

// sbb dword [index*mult+base+shift], num
int x86SBB(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		stream += encodeAddress(stream, index, multiplier, base, shift, 3);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	stream += encodeAddress(stream, index, multiplier, base, shift, 3);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// sbb dword [index*mult+base+shift], op2
int x86SBB(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op2, index, base);
	*stream++ = 0x19;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op2]);

	return int(stream - start);
}

// imul dst, num
int x86IMUL(unsigned char *stream, x86Reg srcdst, int num)
{
	unsigned char *start = stream;

	if((char)(num) == num)
	{
		stream += encodeRex(stream, false, rNONE, rNONE, srcdst);
		*stream++ = 0x6b;
		*stream++ = encodeRegister(srcdst, regCode[srcdst]);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	stream += encodeRex(stream, false, rNONE, rNONE, srcdst);
	*stream++ = 0x69;
	*stream++ = encodeRegister(srcdst, regCode[srcdst]);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// imul dst, src
int x86IMUL(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, src, dst);
	*stream++ = 0x0f;
	*stream++ = 0xaf;
	*stream++ = encodeRegister(src, regCode[dst]);

	return int(stream - start);
}

// REX.W imul dst, src
int x64IMUL(unsigned char *stream, x86Reg dst, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, src, dst);
	*stream++ = 0x0f;
	*stream++ = 0xaf;
	*stream++ = encodeRegister(src, regCode[dst]);

	return int(stream - start);
}

// imul dst, dword [index*mult+base+shift]
int x86IMUL(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, dst, index, base);
	*stream++ = 0x0f;
	*stream++ = 0xaf;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[dst]);

	return int(stream - start);
}

// imul src
int x86IMUL(unsigned char *stream, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, src);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(src, 5);

	return int(stream - start);
}

// idiv src
int x86IDIV(unsigned char *stream, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, src);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(src, 7);

	return int(stream - start);
}

// REX.W idiv src
int x64IDIV(unsigned char *stream, x86Reg src)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, src);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(src, 7);

	return int(stream - start);
}

// idiv dword [index*mult+base+shift]
int x86IDIV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);
	*stream++ = 0xf7;
	stream += encodeAddress(stream, index, multiplier, base, shift, 7);

	return int(stream - start);
}

// shl reg, shift
int x86SHL(unsigned char *stream, x86Reg reg, int num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, reg);

	assert((char)(num) == num);

	if(num == 1)
		*stream++ = 0xd1;
	else
		*stream++ = 0xc1;

	*stream++ = encodeRegister(reg, 4);

	if(num != 1)
		stream += encodeImmByte(stream, (char)num);

	return int(stream - start);
}

// shl dword [index*mult+base+shift], shift
int x86SHL(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	if(num == 1)
		*stream++ = 0xd1;
	else
		*stream++ = 0xc1;

	stream += encodeAddress(stream, index, multiplier, base, shift, 4);

	if(num != 1)
		stream += encodeImmByte(stream, (char)num);

	return int(stream - start);
}

// sal reg, cl
int x86SAL(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, reg);
	*stream++ = 0xd3;
	*stream++ = encodeRegister(reg, 4);

	return int(stream - start);
}

// REX.W sal reg, cl
int x64SAL(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, reg);
	*stream++ = 0xd3;
	*stream++ = encodeRegister(reg, 4);

	return int(stream - start);
}

// sar reg, cl
int x86SAR(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, reg);
	*stream++ = 0xd3;
	*stream++ = encodeRegister(reg, 7);

	return int(stream - start);
}

// REX.W sar reg, cl
int x64SAR(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, reg);
	*stream++ = 0xd3;
	*stream++ = encodeRegister(reg, 7);

	return int(stream - start);
}

// not reg
int x86NOT(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, reg);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(reg, 2);

	return int(stream - start);
}

// not reg
int x64NOT(unsigned char *stream, x86Reg reg)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, true, rNONE, rNONE, reg);
	*stream++ = 0xf7;
	*stream++ = encodeRegister(reg, 2);

	return int(stream - start);
}

// not dword [index*mult+base+shift]
int x86NOT(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);
	*stream++ = 0xf7;
	stream += encodeAddress(stream, index, multiplier, base, shift, 2);

	return int(stream - start);
}

// and op1, op2
int x86AND(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, op1, op2);
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
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, op1);

	if(op1 == rEAX)
	{
		*stream++ = 0x25;
		stream += encodeImmDword(stream, num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(op1, 4);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// and dword [index*mult+base+shift], op2
int x86AND(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op2, index, base);
	*stream++ = 0x21;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op2]);

	return int(stream - start);
}

// and dword [index*mult+base+shift], num
int x86AND(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		stream += encodeAddress(stream, index, multiplier, base, shift, 4);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	stream += encodeAddress(stream, index, multiplier, base, shift, 4);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
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

	stream += encodeRex(stream, false, op1, op2);
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

// or op1, num
int x86OR(unsigned char *stream, x86Reg op1, int num)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, op1);

	if(op1 == rEAX)
	{
		*stream++ = 0x0d;
		stream += encodeImmDword(stream, num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(op1, 1);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// or dword [index*mult+base+shift], op2
int x86OR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op2, index, base);
	*stream++ = 0x09;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op2]);

	return int(stream - start);
}

// or dword [index*mult+base+shift], num
int x86OR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		stream += encodeAddress(stream, index, multiplier, base, shift, 1);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	stream += encodeAddress(stream, index, multiplier, base, shift, 1);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
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

	stream += encodeRex(stream, false, op1, op2);
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

// xor op1, num
int x86XOR(unsigned char *stream, x86Reg op1, int num)
{
	unsigned char *start = stream;

	if(op1 == rEAX)
	{
		stream += encodeRex(stream, false, rNONE, rNONE, op1);
		*stream++ = 0x35;
		stream += encodeImmDword(stream, num);

		return int(stream - start);
	}

	stream += encodeRex(stream, false, rNONE, rNONE, op1);
	*stream++ = 0x81;
	*stream++ = encodeRegister(op1, 6);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// xor dword [index*mult+base+shift], op2
int x86XOR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, size == sQWORD, op2, index, base);
	*stream++ = 0x31;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op2]);

	return int(stream - start);
}

// xor dword [index*mult+base+shift], num
int x86XOR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);
	*stream++ = 0x81;
	stream += encodeAddress(stream, index, multiplier, base, shift, 6);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
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
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, reg);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		*stream++ = encodeRegister(reg, 7);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	*stream++ = encodeRegister(reg, 7);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

// cmp reg1, reg2
int x86CMP(unsigned char *stream, x86Reg reg1, x86Reg reg2)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, reg1, reg2);
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
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op2, index, base);
	*stream++ = 0x39;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op2]);

	return int(stream - start);
}

// cmp op1, dword [index*mult+base+shift]
int x86CMP(unsigned char *stream, x86Reg op1, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, op1, index, base);
	*stream++ = 0x3b;
	stream += encodeAddress(stream, index, multiplier, base, shift, regCode[op1]);

	return int(stream - start);
}

// cmp dword [index*mult+base+shift], num
int x86CMP(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num)
{
	unsigned char *start = stream;

	assert(size == sDWORD || size == sQWORD);

	stream += encodeRex(stream, size == sQWORD, rNONE, index, base);

	if((char)(num) == num)
	{
		*stream++ = 0x83;
		stream += encodeAddress(stream, index, multiplier, base, shift, 7);
		stream += encodeImmByte(stream, (char)num);

		return int(stream - start);
	}

	*stream++ = 0x81;
	stream += encodeAddress(stream, index, multiplier, base, shift, 7);
	stream += encodeImmDword(stream, num);

	return int(stream - start);
}

int x86TEST(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, op1, op2);
	*stream++ = 0x85;
	*stream++ = encodeRegister(op1, regCode[op2]);

	return int(stream - start);
}

// test ah, num
int x86TESTah(unsigned char* stream, char num)
{
	unsigned char *start = stream;

	*stream++ = 0xf6;
	*stream++ = encodeRegister(rESP, 0);
	stream += encodeImmByte(stream, num);

	return int(stream - start);
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
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, rNONE, address);
	*stream++ = 0xff;
	*stream++ = encodeRegister(address, 2);

	return int(stream - start);
}

// call [index*mult+base+shift]
int x86CALL(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift)
{
	unsigned char *start = stream;

	stream += encodeRex(stream, false, rNONE, index, base);
	*stream++ = 0xff;
	stream += encodeAddress(stream, index, multiplier, base, shift, 2);

	return int(stream - start);
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

unsigned char* x86TranslateInstructionList(unsigned char *code, unsigned char *codeEnd, x86Instruction *start, unsigned instCount, unsigned char **instAddress)
{
	x86Instruction *curr = start;

	for(unsigned int i = 0, e = instCount; i != e; i++)
	{
		x86Instruction &cmd = *curr;

		if(cmd.instID)
			instAddress[cmd.instID - 1] = code;	// Save VM instruction address in x86 bytecode

#if !defined(_M_X64)
		assert(cmd.name < o_mov64);
#endif

		switch(cmd.name)
		{
		case o_none:
		case o_nop:
			break;
		case o_mov:
			if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argNumber)
				{
					code += x86MOV(code, cmd.argA.reg, cmd.argB.num);
				}
				else if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sDWORD);
					code += x86MOV(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argReg)
				{
					code += x86MOV(code, cmd.argA.reg, cmd.argB.reg);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			else if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize != sQWORD);

				if(cmd.argB.type == x86Argument::argNumber)
					code += x86MOV(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else if(cmd.argB.type == x86Argument::argReg)
					code += x86MOV(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					assert(!"unknown argument");
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_movsx:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.type == x86Argument::argPtr);
			code += x86MOVSX(code, cmd.argA.reg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;
		case o_push:
			if(cmd.argA.type == x86Argument::argNumber)
				code += x86PUSH(code, cmd.argA.num);
			else if(cmd.argA.type == x86Argument::argPtr)
				code += x86PUSH(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else if(cmd.argA.type == x86Argument::argReg)
				code += x86PUSH(code, cmd.argA.reg);
			else
				assert(!"unknown argument");
			break;
		case o_pop:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86POP(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else if(cmd.argA.type == x86Argument::argReg)
				code += x86POP(code, cmd.argA.reg);
			else
				assert(!"unknown argument");
			break;
		case o_lea:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.type == x86Argument::argPtr);

			code += x86LEA(code, cmd.argA.reg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;
		case o_cdq:
			code += x86CDQ(code);
			break;
		case o_cqo:
			code += x86CQO(code);
			break;
		case o_rep_movsd:
			code += x86REP_MOVSD(code);
			break;
		case o_rep_stosb:
			code += x86REP_STOSB(code);
			break;
		case o_rep_stosw:
			code += x86REP_STOSW(code);
			break;
		case o_rep_stosd:
			code += x86REP_STOSD(code);
			break;
		case o_rep_stosq:
			code += x86REP_STOSQ(code);
			break;

		case o_jmp:
			if(cmd.argA.type == x86Argument::argPtr)
				code += x86JMP(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else
				code += x86JMP(code, cmd.argA.labelID, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_ja:
			code += x86Jcc(code, cmd.argA.labelID, condA, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jae:
			code += x86Jcc(code, cmd.argA.labelID, condAE, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jb:
			code += x86Jcc(code, cmd.argA.labelID, condB, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jbe:
			code += x86Jcc(code, cmd.argA.labelID, condBE, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_je:
			code += x86Jcc(code, cmd.argA.labelID, condE, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jg:
			code += x86Jcc(code, cmd.argA.labelID, condG, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jl:
			code += x86Jcc(code, cmd.argA.labelID, condL, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jne:
			code += x86Jcc(code, cmd.argA.labelID, condNE, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jnp:
			code += x86Jcc(code, cmd.argA.labelID, condNP, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jp:
			code += x86Jcc(code, cmd.argA.labelID, condP, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jge:
			code += x86Jcc(code, cmd.argA.labelID, condGE, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_jle:
			code += x86Jcc(code, cmd.argA.labelID, condLE, (cmd.argA.labelID & JUMP_NEAR) != 0);
			break;
		case o_call:
			if(cmd.argA.type == x86Argument::argLabel)
				code += x86CALL(code, cmd.argA.labelID);
			else if(cmd.argA.type == x86Argument::argPtr)
				code += x86CALL(code, cmd.argA.ptrSize, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			else if(cmd.argA.type == x86Argument::argReg)
				code += x86CALL(code, cmd.argA.reg);
			break;
		case o_ret:
			code += x86RET(code);
			break;

		case o_neg:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				code += x86NEG(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				code += x86NEG(code, cmd.argA.reg);
			}
			break;
		case o_add:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADD(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86ADD(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}
			else
			{
				if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sDWORD);

					code += x86ADD(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argReg)
				{
					code += x86ADD(code, cmd.argA.reg, cmd.argB.reg);
				}
				else
				{
					code += x86ADD(code, cmd.argA.reg, cmd.argB.num);
				}
			}
			break;
		case o_adc:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADC(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86ADC(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}
			else
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADC(code, cmd.argA.reg, cmd.argB.reg);
				else
					code += x86ADC(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_sub:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86SUB(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86SUB(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}
			else
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86SUB(code, cmd.argA.reg, cmd.argB.reg);
				else
					code += x86SUB(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_sbb:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86SBB(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86SBB(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}
			else
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x86SBB(code, cmd.argA.reg, cmd.argB.reg);
				else
					code += x86SBB(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_imul:
			if(cmd.argB.type == x86Argument::argNumber)
			{
				code += x86IMUL(code, cmd.argA.reg, cmd.argB.num);
			}
			else if(cmd.argB.type == x86Argument::argReg)
			{
				code += x86IMUL(code, cmd.argA.reg, cmd.argB.reg);
			}
			else if(cmd.argB.type == x86Argument::argPtr)
			{
				assert(cmd.argB.ptrSize == sDWORD);

				code += x86IMUL(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			}
			else
			{
				code += x86IMUL(code, cmd.argA.reg);
			}
			break;
		case o_idiv:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				code += x86IDIV(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			}
			else
			{
				code += x86IDIV(code, cmd.argA.reg);
			}
			break;
		case o_shl:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				code += x86SHL(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}
			else
			{
				code += x86SHL(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_sal:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.reg == rECX || cmd.argB.type == x86Argument::argNone);
			code += x86SAL(code, cmd.argA.reg);
			break;
		case o_sar:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.reg == rECX || cmd.argB.type == x86Argument::argNone);
			code += x86SAR(code, cmd.argA.reg);
			break;
		case o_not:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				code += x86NOT(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			}
			else
			{
				code += x86NOT(code, cmd.argA.reg);
			}
			break;
		case o_and:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86AND(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else if(cmd.argB.type == x86Argument::argNumber)
					code += x86AND(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					assert(!"unknown argument");
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argReg)
				{
					code += x86AND(code, cmd.argA.reg, cmd.argB.reg);
				}
				else if(cmd.argB.type == x86Argument::argNumber)
				{
					code += x86AND(code, cmd.argA.reg, cmd.argB.num);
				}
				else if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sDWORD);

					code += x86AND(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_or:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86OR(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else if(cmd.argB.type == x86Argument::argNumber)
					code += x86OR(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					assert(!"unknown argument");
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argReg)
				{
					code += x86OR(code, cmd.argA.reg, cmd.argB.reg);
				}
				else if(cmd.argB.type == x86Argument::argNumber)
				{
					code += x86OR(code, cmd.argA.reg, cmd.argB.num);
				}
				else if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sDWORD);

					code += x86OR(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_xor:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86XOR(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else if(cmd.argB.type == x86Argument::argNumber)
					code += x86XOR(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					assert(!"unknown argument");
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argReg)
				{
					code += x86XOR(code, cmd.argA.reg, cmd.argB.reg);
				}
				else if(cmd.argB.type == x86Argument::argNumber)
				{
					code += x86XOR(code, cmd.argA.reg, cmd.argB.num);
				}
				else if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sDWORD);

					code += x86XOR(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_cmp:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sDWORD);

				if(cmd.argB.type == x86Argument::argNumber)
					code += x86CMP(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else if(cmd.argB.type == x86Argument::argReg)
					code += x86CMP(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					assert(!"unknown argument");
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sDWORD);

					code += x86CMP(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argNumber)
				{
					code += x86CMP(code, cmd.argA.reg, cmd.argB.num);
				}
				else
				{
					code += x86CMP(code, cmd.argA.reg, cmd.argB.reg);
				}
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_test:
			if(cmd.argB.type == x86Argument::argNumber)
				code += x86TESTah(code, (char)cmd.argB.num);
			else
				code += x86TEST(code, cmd.argA.reg, cmd.argB.reg);
			break;

		case o_setl:
			code += x86SETcc(code, condL, cmd.argA.reg);
			break;
		case o_setg:
			code += x86SETcc(code, condG, cmd.argA.reg);
			break;
		case o_setle:
			code += x86SETcc(code, condLE, cmd.argA.reg);
			break;
		case o_setge:
			code += x86SETcc(code, condGE, cmd.argA.reg);
			break;
		case o_sete:
			code += x86SETcc(code, condE, cmd.argA.reg);
			break;
		case o_setne:
			code += x86SETcc(code, condNE, cmd.argA.reg);
			break;
		case o_setz:
			code += x86SETcc(code, condZ, cmd.argA.reg);
			break;
		case o_setnz:
			code += x86SETcc(code, condNZ, cmd.argA.reg);
			break;

		case o_movss:
			assert(cmd.argA.type == x86Argument::argPtr);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			assert(cmd.argA.ptrSize == sDWORD);
			code += x86MOVSS(code, sDWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.xmmArg);
			break;
		case o_movsd:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argB.type == x86Argument::argXmmReg);
				assert(cmd.argA.ptrSize == sQWORD);
				code += x86MOVSD(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.xmmArg);
			}
			else
			{
				assert(cmd.argA.type == x86Argument::argXmmReg);

				if(cmd.argB.type == x86Argument::argPtr)
					code += x86MOVSD(code, cmd.argA.xmmArg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				else if(cmd.argB.type == x86Argument::argXmmReg)
					code += x86MOVSD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
				else
					assert(!"unknown argument");
			}
			break;
		case o_movd:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86MOVD(code, cmd.argA.reg, cmd.argB.xmmArg);
			break;
		case o_cvtss2sd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argPtr);
			code += x86CVTSS2SD(code, cmd.argA.xmmArg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;
		case o_cvtsd2ss:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argPtr);
			code += x86CVTSD2SS(code, cmd.argA.xmmArg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;
		case o_cvttsd2si:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.type == x86Argument::argPtr);
			code += x86CVTTSD2SI(code, cmd.argA.reg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;
		case o_cvtsi2sd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argPtr);
			code += x86CVTSI2SD(code, cmd.argA.xmmArg, cmd.argB.ptrSize, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;
		case o_addsd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86ADDSD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
			break;
		case o_subsd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86SUBSD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
			break;
		case o_mulsd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86MULSD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
			break;
		case o_divsd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86DIVSD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
			break;
		case o_cmpeqsd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86CMPEQSD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
			break;
		case o_cmpltsd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86CMPLTSD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
			break;
		case o_cmplesd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86CMPLESD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
			break;
		case o_cmpneqsd:
			assert(cmd.argA.type == x86Argument::argXmmReg);
			assert(cmd.argB.type == x86Argument::argXmmReg);
			code += x86CMPNEQSD(code, cmd.argA.xmmArg, cmd.argB.xmmArg);
			break;

		case o_int:
			code += x86INT(code, 3);
			break;
		case o_label:
			x86AddLabel(code, cmd.labelID);
			break;
		case o_use32:
			break;
		case o_other:
			break;
		case o_mov64:
			if(cmd.argA.type != x86Argument::argPtr)
			{
				if(cmd.argB.type == x86Argument::argNumber)
				{
					code += x64MOV(code, cmd.argA.reg, cmd.argB.num);
				}
				else if(cmd.argB.type == x86Argument::argImm64)
				{
					code += x64MOV(code, cmd.argA.reg, cmd.argB.imm64Arg);
				}
				else if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sQWORD);

					code += x86MOV(code, cmd.argA.reg, sQWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argReg)
				{
					code += x64MOV(code, cmd.argA.reg, cmd.argB.reg);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			else
			{
				assert(cmd.argA.ptrSize == sQWORD);

				if(cmd.argB.type == x86Argument::argNumber)
					code += x86MOV(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					code += x86MOV(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
			}
			break;
		case o_movsxd:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.type == x86Argument::argPtr);
			assert(cmd.argB.ptrSize == sDWORD);
			code += x86MOVSXD(code, cmd.argA.reg, sDWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;

		case o_neg64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				code += x86NEG(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				code += x64NEG(code, cmd.argA.reg);
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_add64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86ADD(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86ADD(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}
			else
			{
				if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sQWORD);

					code += x86ADD(code, cmd.argA.reg, sQWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argReg)
				{
					code += x64ADD(code, cmd.argA.reg, cmd.argB.reg);
				}
				else
				{
					code += x64ADD(code, cmd.argA.reg, cmd.argB.num);
				}
			}
			break;
		case o_sub64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86SUB(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					code += x86SUB(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
			}
			else
			{
				if(cmd.argB.type == x86Argument::argReg)
					code += x64SUB(code, cmd.argA.reg, cmd.argB.reg);
				else
					code += x64SUB(code, cmd.argA.reg, cmd.argB.num);
			}
			break;
		case o_imul64:
			assert(cmd.argA.type == x86Argument::argReg);

			if(cmd.argB.type == x86Argument::argPtr)
			{
				assert(cmd.argB.ptrSize == sQWORD);

				code += x86IMUL(code, cmd.argA.reg, sQWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			}
			else if(cmd.argB.type == x86Argument::argReg)
			{
				code += x64IMUL(code, cmd.argA.reg, cmd.argB.reg);
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_idiv64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				code += x86IDIV(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				code += x64IDIV(code, cmd.argA.reg);
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_sal64:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.reg == rECX || cmd.argB.type == x86Argument::argNone);
			code += x64SAL(code, cmd.argA.reg);
			break;
		case o_sar64:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.reg == rECX || cmd.argB.type == x86Argument::argNone);
			code += x64SAR(code, cmd.argA.reg);
			break;
		case o_not64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				code += x86NOT(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum);
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				code += x64NOT(code, cmd.argA.reg);
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_and64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86AND(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else if(cmd.argB.type == x86Argument::argNumber)
					code += x86AND(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					assert(!"unknown argument");
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sQWORD);

					code += x86AND(code, cmd.argA.reg, sQWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argReg)
				{
					code += x64AND(code, cmd.argA.reg, cmd.argB.reg);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_or64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86OR(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else if(cmd.argB.type == x86Argument::argNumber)
					code += x86OR(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					assert(!"unknown argument");
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sQWORD);

					code += x86OR(code, cmd.argA.reg, sQWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argReg)
				{
					code += x64OR(code, cmd.argA.reg, cmd.argB.reg);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_xor64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				if(cmd.argB.type == x86Argument::argReg)
					code += x86XOR(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else if(cmd.argB.type == x86Argument::argNumber)
					code += x86XOR(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else
					assert(!"unknown argument");
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sQWORD);

					code += x86XOR(code, cmd.argA.reg, sQWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argReg)
				{
					code += x64XOR(code, cmd.argA.reg, cmd.argB.reg);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			else
			{
				assert(!"unknown argument");
			}
			break;
		case o_cmp64:
			if(cmd.argA.type == x86Argument::argPtr)
			{
				assert(cmd.argA.ptrSize == sQWORD);

				if(cmd.argB.type == x86Argument::argNumber)
					code += x86CMP(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.num);
				else if(cmd.argB.type == x86Argument::argReg)
					code += x86CMP(code, sQWORD, cmd.argA.ptrIndex, cmd.argA.ptrMult, cmd.argA.ptrBase, cmd.argA.ptrNum, cmd.argB.reg);
				else
					assert(!"unknown argument");
			}
			else if(cmd.argA.type == x86Argument::argReg)
			{
				if(cmd.argB.type == x86Argument::argPtr)
				{
					assert(cmd.argB.ptrSize == sQWORD);

					code += x86CMP(code, cmd.argA.reg, sQWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
				}
				else if(cmd.argB.type == x86Argument::argReg)
				{
					code += x64CMP(code, cmd.argA.reg, cmd.argB.reg);
				}
				else
				{
					assert(!"unknown argument");
				}
			}
			break;
		case o_cvttsd2si64:
			assert(cmd.argA.type == x86Argument::argReg);
			assert(cmd.argB.type == x86Argument::argPtr);
			assert(cmd.argB.ptrSize == sQWORD);
			code += x64CVTTSD2SI(code, cmd.argA.reg, sQWORD, cmd.argB.ptrIndex, cmd.argB.ptrMult, cmd.argB.ptrBase, cmd.argB.ptrNum);
			break;
		default:
			assert(!"unknown instruction");
		}

		assert(code < codeEnd);

		curr++;
	}

	return code;
}


static const unsigned testSizeDword = 1;
static const unsigned testSizeQword = 2;
static const x86Reg testIndexRegs[] = { rNONE, rEAX, rEDX, rEDI, rEBP, rR8, rR12, rR13, rR15 };
static const x86Reg testBaseRegs[] = { rNONE, rEAX, rEDX, rEDI, rESP, rEBP, rR8, rR12, rR13, rR15 };
static const x86Reg testRegs[] = { rEAX, rEDX, rEDI, rESP, rEBP, rR8, rR12, rR13, rR15 };
static const x86XmmReg testXmmRegs[] = { rXMM0, rXMM7, rXMM8, rXMM12, rXMM13, rXMM15 };

int TestRptrEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift), unsigned testSize = testSizeDword | testSizeQword)
{
	unsigned char *start = stream;

	for(unsigned index = 0; index < sizeof(testIndexRegs) / sizeof(testIndexRegs[0]); index++)
	{
		for(unsigned base = 0; base < sizeof(testBaseRegs) / sizeof(testBaseRegs[0]); base++)
		{
			if(testSize & testSizeDword)
			{
				if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
				{
					EMIT_OP_RPTR(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
					stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
					EMIT_OP_RPTR(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
					stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
					EMIT_OP_RPTR(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
					stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
				}

				if(testIndexRegs[index] != rNONE)
				{
					EMIT_OP_RPTR(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
					stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
					EMIT_OP_RPTR(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
					stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
					EMIT_OP_RPTR(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
					stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
				}
			}

			if(testSize & testSizeQword)
			{
				// skip RDI-based addressing
				if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
				{
					EMIT_OP_RPTR(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
					stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
					EMIT_OP_RPTR(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
					stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
					EMIT_OP_RPTR(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
					stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
				}

				if(testIndexRegs[index] != rNONE)
				{
					EMIT_OP_RPTR(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
					stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
					EMIT_OP_RPTR(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
					stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
					EMIT_OP_RPTR(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
					stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
				}
			}
		}
	}

	return int(stream - start);
}

int TestRegEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Reg reg))
{
	unsigned char *start = stream;

	for(unsigned reg = 0; reg < sizeof(testRegs) / sizeof(testRegs[0]); reg++)
	{
		EMIT_OP_REG(ctx, op, testRegs[reg]);
		stream += fun(stream, testRegs[reg]);
	}

	return int(stream - start);
}

int TestNumEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, int num))
{
	unsigned char *start = stream;

	EMIT_OP_NUM(ctx, op, 4);
	stream += fun(stream, 4);

	EMIT_OP_NUM(ctx, op, 1024);
	stream += fun(stream, 1024);

	return int(stream - start);
}

int TestRegRegEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Reg dst, x86Reg src))
{
	unsigned char *start = stream;

	for(unsigned reg1 = 0; reg1 < sizeof(testRegs) / sizeof(testRegs[0]); reg1++)
	{
		for(unsigned reg2 = 0; reg2 < sizeof(testRegs) / sizeof(testRegs[0]); reg2++)
		{
			EMIT_OP_REG_REG(ctx, op, testRegs[reg1], testRegs[reg2]);
			stream += fun(stream, testRegs[reg1], testRegs[reg2]);
		}
	}

	return int(stream - start);
}

int TestRegNumEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Reg dst, int num), bool onlySmall = false)
{
	unsigned char *start = stream;

	for(unsigned reg = 0; reg < sizeof(testRegs) / sizeof(testRegs[0]); reg++)
	{
		EMIT_OP_REG_NUM(ctx, op, testRegs[reg], 4);
		stream += fun(stream, testRegs[reg], 4);

		if(!onlySmall)
		{
			EMIT_OP_REG_NUM(ctx, op, testRegs[reg], 1024);
			stream += fun(stream, testRegs[reg], 1024);
		}
	}

	return int(stream - start);
}

int TestRegNum64Encoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Reg dst, unsigned long long num))
{
	unsigned char *start = stream;

	for(unsigned reg = 0; reg < sizeof(testRegs) / sizeof(testRegs[0]); reg++)
	{
		EMIT_OP_REG_NUM64(ctx, op, testRegs[reg], 0x1122aabbccddeeffll);
		stream += fun(stream, testRegs[reg], 0x1122aabbccddeeffll);
	}

	return int(stream - start);
}

int TestRegRptrEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift), unsigned testSize = testSizeDword | testSizeQword)
{
	unsigned char *start = stream;

	for(unsigned reg = 0; reg < sizeof(testRegs) / sizeof(testRegs[0]); reg++)
	{
		for(unsigned index = 0; index < sizeof(testIndexRegs) / sizeof(testIndexRegs[0]); index++)
		{
			for(unsigned base = 0; base < sizeof(testBaseRegs) / sizeof(testBaseRegs[0]); base++)
			{
				if(testSize & testSizeDword)
				{
					if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
					{
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
						stream += fun(stream, testRegs[reg], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
						stream += fun(stream, testRegs[reg], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
						stream += fun(stream, testRegs[reg], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
					}

					if(testIndexRegs[index] != rNONE)
					{
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
						stream += fun(stream, testRegs[reg], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
						stream += fun(stream, testRegs[reg], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
						stream += fun(stream, testRegs[reg], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
					}
				}

				if(testSize & testSizeQword)
				{
					// skip RDI-based addressing
					if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
					{
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
						stream += fun(stream, testRegs[reg], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
						stream += fun(stream, testRegs[reg], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
						stream += fun(stream, testRegs[reg], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
					}

					if(testIndexRegs[index] != rNONE)
					{
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
						stream += fun(stream, testRegs[reg], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
						stream += fun(stream, testRegs[reg], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
						EMIT_OP_REG_RPTR(ctx, op, testRegs[reg], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
						stream += fun(stream, testRegs[reg], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
					}
				}
			}
		}
	}

	return int(stream - start);
}

int TestRptrRegEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg src), unsigned testSize = testSizeDword | testSizeQword)
{
	unsigned char *start = stream;

	for(unsigned reg = 0; reg < sizeof(testRegs) / sizeof(testRegs[0]); reg++)
	{
		for(unsigned index = 0; index < sizeof(testIndexRegs) / sizeof(testIndexRegs[0]); index++)
		{
			for(unsigned base = 0; base < sizeof(testBaseRegs) / sizeof(testBaseRegs[0]); base++)
			{
				if(testSize & testSizeDword)
				{
					if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
					{
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, testRegs[reg]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, testRegs[reg]);
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, testRegs[reg]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, testRegs[reg]);
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, testRegs[reg]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, testRegs[reg]);
					}

					if(testIndexRegs[index] != rNONE)
					{
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, testRegs[reg]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, testRegs[reg]);
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, testRegs[reg]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, testRegs[reg]);
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, testRegs[reg]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, testRegs[reg]);
					}
				}

				if(testSize & testSizeQword)
				{
					// skip RDI-based addressing
					if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
					{
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, testRegs[reg]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, testRegs[reg]);
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, testRegs[reg]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, testRegs[reg]);
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, testRegs[reg]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, testRegs[reg]);
					}

					if(testIndexRegs[index] != rNONE)
					{
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, testRegs[reg]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, testRegs[reg]);
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, testRegs[reg]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, testRegs[reg]);
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, testRegs[reg]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, testRegs[reg]);
					}
				}
			}
		}
	}

	return int(stream - start);
}

int TestRptrNumEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num), unsigned testSize = testSizeDword | testSizeQword)
{
	unsigned char *start = stream;

	for(unsigned index = 0; index < sizeof(testIndexRegs) / sizeof(testIndexRegs[0]); index++)
	{
		for(unsigned base = 0; base < sizeof(testBaseRegs) / sizeof(testBaseRegs[0]); base++)
		{
			if(testSize & testSizeDword)
			{
				if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
				{
					EMIT_OP_RPTR_NUM(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, 8);
					stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, 8);
					EMIT_OP_RPTR_NUM(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, 8);
					stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, 8);
					EMIT_OP_RPTR_NUM(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, 8);
					stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, 8);
				}

				if(testIndexRegs[index] != rNONE)
				{
					EMIT_OP_RPTR_NUM(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, 8);
					stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, 8);
					EMIT_OP_RPTR_NUM(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, 8);
					stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, 8);
					EMIT_OP_RPTR_NUM(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, 8);
					stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, 8);
				}
			}

			if(testSize & testSizeQword)
			{
				// skip RDI-based addressing
				if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
				{
					EMIT_OP_RPTR_NUM(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, 8);
					stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, 8);
					EMIT_OP_RPTR_NUM(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, 8);
					stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, 8);
					EMIT_OP_RPTR_NUM(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, 8);
					stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, 8);
				}

				if(testIndexRegs[index] != rNONE)
				{
					EMIT_OP_RPTR_NUM(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, 8);
					stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, 8);
					EMIT_OP_RPTR_NUM(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, 8);
					stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, 8);
					EMIT_OP_RPTR_NUM(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, 8);
					stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, 8);
				}
			}
		}
	}

	return int(stream - start);
}

int TestXmmXmmEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86XmmReg dst, x86XmmReg src))
{
	unsigned char *start = stream;

	for(unsigned xmm1 = 0; xmm1 < sizeof(testXmmRegs) / sizeof(testXmmRegs[0]); xmm1++)
	{
		for(unsigned xmm2 = 0; xmm2 < sizeof(testXmmRegs) / sizeof(testXmmRegs[0]); xmm2++)
		{
			EMIT_OP_REG_REG(ctx, op, testXmmRegs[xmm1], testXmmRegs[xmm2]);
			stream += fun(stream, testXmmRegs[xmm1], testXmmRegs[xmm2]);
		}
	}

	return int(stream - start);
}

int TestRegXmmEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Reg dst, x86XmmReg src))
{
	unsigned char *start = stream;

	for(unsigned reg = 0; reg < sizeof(testRegs) / sizeof(testRegs[0]); reg++)
	{
		for(unsigned xmm = 0; xmm < sizeof(testXmmRegs) / sizeof(testXmmRegs[0]); xmm++)
		{
			EMIT_OP_REG_REG(ctx, op, testRegs[reg], testXmmRegs[xmm]);
			stream += fun(stream, testRegs[reg], testXmmRegs[xmm]);
		}
	}

	return int(stream - start);
}

int TestRptrXmmEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86XmmReg src), unsigned testSize = testSizeDword | testSizeQword)
{
	unsigned char *start = stream;

	for(unsigned xmm = 0; xmm < sizeof(testXmmRegs) / sizeof(testXmmRegs[0]); xmm++)
	{
		for(unsigned index = 0; index < sizeof(testIndexRegs) / sizeof(testIndexRegs[0]); index++)
		{
			for(unsigned base = 0; base < sizeof(testBaseRegs) / sizeof(testBaseRegs[0]); base++)
			{
				if(testSize & testSizeDword)
				{
					// skip RDI-based addressing
					if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
					{
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, testXmmRegs[xmm]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, testXmmRegs[xmm]);
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, testXmmRegs[xmm]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, testXmmRegs[xmm]);
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, testXmmRegs[xmm]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, testXmmRegs[xmm]);
					}

					if(testIndexRegs[index] != rNONE)
					{
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, testXmmRegs[xmm]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, testXmmRegs[xmm]);
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, testXmmRegs[xmm]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, testXmmRegs[xmm]);
						EMIT_OP_RPTR_REG(ctx, op, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, testXmmRegs[xmm]);
						stream += fun(stream, sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, testXmmRegs[xmm]);
					}
				}

				if(testSize & testSizeQword)
				{
					// skip RDI-based addressing
					if(testIndexRegs[index] != rNONE || testBaseRegs[base] != rNONE)
					{
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, testXmmRegs[xmm]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0, testXmmRegs[xmm]);
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, testXmmRegs[xmm]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4, testXmmRegs[xmm]);
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, testXmmRegs[xmm]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024, testXmmRegs[xmm]);
					}

					if(testIndexRegs[index] != rNONE)
					{
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, testXmmRegs[xmm]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0, testXmmRegs[xmm]);
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, testXmmRegs[xmm]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4, testXmmRegs[xmm]);
						EMIT_OP_RPTR_REG(ctx, op, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, testXmmRegs[xmm]);
						stream += fun(stream, sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024, testXmmRegs[xmm]);
					}
				}
			}
		}
	}

	return int(stream - start);
}

int TestXmmRptrEncoding(CodeGenGenericContext &ctx, unsigned char *stream, x86Command op, int (*fun)(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift), unsigned testSize = testSizeDword | testSizeQword)
{
	unsigned char *start = stream;

	for(unsigned xmm = 0; xmm < sizeof(testXmmRegs) / sizeof(testXmmRegs[0]); xmm++)
	{
		for(unsigned index = 0; index < sizeof(testIndexRegs) / sizeof(testIndexRegs[0]); index++)
		{
			for(unsigned base = 0; base < sizeof(testBaseRegs) / sizeof(testBaseRegs[0]); base++)
			{
				if(testSize & testSizeDword)
				{
					if(testIndexRegs[base] != rNONE || testBaseRegs[base] != rNONE)
					{
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
						stream += fun(stream, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
						stream += fun(stream, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
						stream += fun(stream, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
					}

					if(testIndexRegs[index] != rNONE)
					{
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
						stream += fun(stream, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
						stream += fun(stream, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
						stream += fun(stream, testXmmRegs[xmm], sDWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
					}
				}

				if(testSize & testSizeQword)
				{
					// skip RDI-based addressing
					if(testIndexRegs[base] != rNONE || testBaseRegs[base] != rNONE)
					{
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
						stream += fun(stream, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 0);
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
						stream += fun(stream, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 4);
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
						stream += fun(stream, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 1, testBaseRegs[base], 1024);
					}

					if(testIndexRegs[index] != rNONE)
					{
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
						stream += fun(stream, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 0);
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
						stream += fun(stream, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 4);
						EMIT_OP_REG_RPTR(ctx, op, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
						stream += fun(stream, testXmmRegs[xmm], sQWORD, testIndexRegs[index], 2, testBaseRegs[base], 1024);
					}
				}
			}
		}
	}

	return int(stream - start);
}

void x86TestEncoding(unsigned char *codeLaunchHeader)
{
	(void)codeLaunchHeader;

	unsigned bufSize = 2048 * 1024;
	unsigned char *buf = new unsigned char[bufSize];

	CodeGenGenericContext ctx;
	unsigned char *stream = buf;

	unsigned instSize = 256 * 1024;
	x86Instruction *instList = new x86Instruction[instSize];
	memset(instList, 0, instSize * sizeof(x86Instruction));
	ctx.SetLastInstruction(instList, instList);

	stream += TestRptrXmmEncoding(ctx, stream, o_movss, x86MOVSS, testSizeDword);

	stream += TestXmmXmmEncoding(ctx, stream, o_movsd, x86MOVSD);
	stream += TestRptrXmmEncoding(ctx, stream, o_movsd, x86MOVSD, testSizeQword);
	stream += TestXmmRptrEncoding(ctx, stream, o_movsd, x86MOVSD, testSizeQword);

	stream += TestRegXmmEncoding(ctx, stream, o_movd, x86MOVD);

	stream += TestRegRptrEncoding(ctx, stream, o_movsxd, x86MOVSXD, testSizeDword);

	stream += TestXmmRptrEncoding(ctx, stream, o_cvtss2sd, x86CVTSS2SD, testSizeDword);

	stream += TestXmmRptrEncoding(ctx, stream, o_cvtsd2ss, x86CVTSD2SS, testSizeQword);

	stream += TestRegRptrEncoding(ctx, stream, o_cvttsd2si, x86CVTTSD2SI, testSizeQword);

	stream += TestRegRptrEncoding(ctx, stream, o_cvttsd2si64, x64CVTTSD2SI, testSizeQword);

	stream += TestXmmRptrEncoding(ctx, stream, o_cvtsi2sd, x86CVTSI2SD, testSizeDword);

	stream += TestXmmXmmEncoding(ctx, stream, o_addsd, x86ADDSD);
	stream += TestXmmXmmEncoding(ctx, stream, o_subsd, x86SUBSD);
	stream += TestXmmXmmEncoding(ctx, stream, o_mulsd, x86MULSD);
	stream += TestXmmXmmEncoding(ctx, stream, o_divsd, x86DIVSD);

	stream += TestXmmXmmEncoding(ctx, stream, o_cmpeqsd, x86CMPEQSD);
	stream += TestXmmXmmEncoding(ctx, stream, o_cmpltsd, x86CMPLTSD);
	stream += TestXmmXmmEncoding(ctx, stream, o_cmplesd, x86CMPLESD);
	stream += TestXmmXmmEncoding(ctx, stream, o_cmpneqsd, x86CMPNEQSD);

#if defined(_M_X64)
	stream += TestRptrEncoding(ctx, stream, o_push, x86PUSH, testSizeQword);
	stream += TestRegEncoding(ctx, stream, o_push, x86PUSH);
	stream += TestNumEncoding(ctx, stream, o_push, x86PUSH);

	stream += TestRptrEncoding(ctx, stream, o_pop, x86POP, testSizeQword);
	stream += TestRegEncoding(ctx, stream, o_pop, x86POP);
#else
	stream += TestRptrEncoding(ctx, stream, o_push, x86PUSH, testSizeDword);
	stream += TestRegEncoding(ctx, stream, o_push, x86PUSH);
	stream += TestNumEncoding(ctx, stream, o_push, x86PUSH);

	stream += TestRptrEncoding(ctx, stream, o_pop, x86POP, testSizeDword);
	stream += TestRegEncoding(ctx, stream, o_pop, x86POP);
#endif

	stream += TestRegNumEncoding(ctx, stream, o_mov, x86MOV);
	stream += TestRegRegEncoding(ctx, stream, o_mov, x86MOV);
	stream += TestRegRptrEncoding(ctx, stream, o_mov, x86MOV, testSizeDword);
	stream += TestRptrRegEncoding(ctx, stream, o_mov, x86MOV, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_mov, x86MOV, testSizeDword);

	stream += TestRegNum64Encoding(ctx, stream, o_mov64, x64MOV);
	stream += TestRegRegEncoding(ctx, stream, o_mov64, x64MOV);
	stream += TestRegRptrEncoding(ctx, stream, o_mov64, x86MOV, testSizeQword);
	stream += TestRptrRegEncoding(ctx, stream, o_mov64, x86MOV, testSizeQword);
	stream += TestRptrNumEncoding(ctx, stream, o_mov64, x86MOV, testSizeQword);

	//stream += TestRegRptrEncoding(ctx, stream, o_movsx, x86MOVSX);

	stream += TestRegRptrEncoding(ctx, stream, o_lea, x86LEA);

	stream += TestRptrEncoding(ctx, stream, o_neg, x86NEG, testSizeDword);
	stream += TestRegEncoding(ctx, stream, o_neg, x86NEG);

	stream += TestRptrEncoding(ctx, stream, o_neg64, x86NEG, testSizeQword);
	stream += TestRegEncoding(ctx, stream, o_neg64, x64NEG);

	stream += TestRegRegEncoding(ctx, stream, o_add, x86ADD);
	stream += TestRegNumEncoding(ctx, stream, o_add, x86ADD);
	stream += TestRegRptrEncoding(ctx, stream, o_add, x86ADD, testSizeDword);
	stream += TestRptrRegEncoding(ctx, stream, o_add, x86ADD, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_add, x86ADD, testSizeDword);

	stream += TestRegRegEncoding(ctx, stream, o_add64, x64ADD);
	stream += TestRegNumEncoding(ctx, stream, o_add64, x64ADD);
	stream += TestRegRptrEncoding(ctx, stream, o_add64, x86ADD, testSizeQword);
	stream += TestRptrRegEncoding(ctx, stream, o_add64, x86ADD, testSizeQword);
	stream += TestRptrNumEncoding(ctx, stream, o_add64, x86ADD, testSizeQword);

	stream += TestRegRegEncoding(ctx, stream, o_adc, x86ADC);
	stream += TestRegNumEncoding(ctx, stream, o_adc, x86ADC);
	stream += TestRptrRegEncoding(ctx, stream, o_adc, x86ADC, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_adc, x86ADC, testSizeDword);

	stream += TestRegRegEncoding(ctx, stream, o_sub, x86SUB);
	stream += TestRegNumEncoding(ctx, stream, o_sub, x86SUB);
	stream += TestRptrRegEncoding(ctx, stream, o_sub, x86SUB, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_sub, x86SUB, testSizeDword);

	stream += TestRegRegEncoding(ctx, stream, o_sub64, x64SUB);
	stream += TestRegNumEncoding(ctx, stream, o_sub64, x64SUB);
	stream += TestRptrRegEncoding(ctx, stream, o_sub64, x86SUB, testSizeQword);
	stream += TestRptrNumEncoding(ctx, stream, o_sub64, x86SUB, testSizeQword);

	stream += TestRegNumEncoding(ctx, stream, o_sbb, x86SBB);
	stream += TestRegRegEncoding(ctx, stream, o_sbb, x86SBB);
	stream += TestRptrRegEncoding(ctx, stream, o_sbb, x86SBB, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_sbb, x86SBB, testSizeDword);

	stream += TestRegEncoding(ctx, stream, o_imul, x86IMUL);
	stream += TestRegRegEncoding(ctx, stream, o_imul, x86IMUL);
	stream += TestRegNumEncoding(ctx, stream, o_imul, x86IMUL);
	stream += TestRegRptrEncoding(ctx, stream, o_imul, x86IMUL, testSizeDword);

	stream += TestRegRegEncoding(ctx, stream, o_imul64, x64IMUL);
	stream += TestRegRptrEncoding(ctx, stream, o_imul64, x86IMUL, testSizeQword);

	stream += TestRptrEncoding(ctx, stream, o_idiv, x86IDIV, testSizeDword);
	stream += TestRegEncoding(ctx, stream, o_idiv, x86IDIV);

	stream += TestRptrEncoding(ctx, stream, o_idiv64, x86IDIV, testSizeQword);
	stream += TestRegEncoding(ctx, stream, o_idiv64, x64IDIV);

	stream += TestRegNumEncoding(ctx, stream, o_shl, x86SHL, true);
	stream += TestRptrNumEncoding(ctx, stream, o_shl, x86SHL, testSizeDword);

	stream += TestRegEncoding(ctx, stream, o_sal, x86SAL);

	stream += TestRegEncoding(ctx, stream, o_sal64, x64SAL);

	stream += TestRegEncoding(ctx, stream, o_sar, x86SAR);

	stream += TestRegEncoding(ctx, stream, o_sar64, x64SAR);

	stream += TestRptrEncoding(ctx, stream, o_not, x86NOT, testSizeDword);
	stream += TestRegEncoding(ctx, stream, o_not, x86NOT);

	stream += TestRptrEncoding(ctx, stream, o_not64, x86NOT, testSizeQword);
	stream += TestRegEncoding(ctx, stream, o_not64, x64NOT);

	stream += TestRegRegEncoding(ctx, stream, o_and, x86AND);
	stream += TestRegNumEncoding(ctx, stream, o_and, x86AND);
	stream += TestRegRptrEncoding(ctx, stream, o_and, x86AND, testSizeDword);
	stream += TestRptrRegEncoding(ctx, stream, o_and, x86AND, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_and, x86AND, testSizeDword);

	stream += TestRegRegEncoding(ctx, stream, o_and64, x64AND);
	stream += TestRegRptrEncoding(ctx, stream, o_and64, x86AND, testSizeQword);
	stream += TestRptrRegEncoding(ctx, stream, o_and64, x86AND, testSizeQword);
	stream += TestRptrNumEncoding(ctx, stream, o_and64, x86AND, testSizeQword);

	stream += TestRegRegEncoding(ctx, stream, o_or, x86OR);
	stream += TestRegNumEncoding(ctx, stream, o_or, x86OR);
	stream += TestRegRptrEncoding(ctx, stream, o_or, x86OR, testSizeDword);
	stream += TestRptrRegEncoding(ctx, stream, o_or, x86OR, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_or, x86OR, testSizeDword);

	stream += TestRegRegEncoding(ctx, stream, o_or64, x64OR);
	stream += TestRegRptrEncoding(ctx, stream, o_or64, x86OR, testSizeQword);
	stream += TestRptrRegEncoding(ctx, stream, o_or64, x86OR, testSizeQword);
	stream += TestRptrNumEncoding(ctx, stream, o_or64, x86OR, testSizeQword);

	stream += TestRegRegEncoding(ctx, stream, o_xor, x86XOR);
	stream += TestRegNumEncoding(ctx, stream, o_xor, x86XOR);
	stream += TestRegRptrEncoding(ctx, stream, o_xor, x86XOR, testSizeDword);
	stream += TestRptrRegEncoding(ctx, stream, o_xor, x86XOR, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_xor, x86XOR, testSizeDword);

	stream += TestRegRegEncoding(ctx, stream, o_xor64, x64XOR);
	stream += TestRegRptrEncoding(ctx, stream, o_xor64, x86XOR, testSizeQword);
	stream += TestRptrRegEncoding(ctx, stream, o_xor64, x86XOR, testSizeQword);
	stream += TestRptrNumEncoding(ctx, stream, o_xor64, x86XOR, testSizeQword);

	stream += TestRegRegEncoding(ctx, stream, o_cmp, x86CMP);
	stream += TestRegNumEncoding(ctx, stream, o_cmp, x86CMP);
	stream += TestRegRptrEncoding(ctx, stream, o_cmp, x86CMP, testSizeDword);
	stream += TestRptrRegEncoding(ctx, stream, o_cmp, x86CMP, testSizeDword);
	stream += TestRptrNumEncoding(ctx, stream, o_cmp, x86CMP, testSizeDword);

	stream += TestRegRegEncoding(ctx, stream, o_cmp64, x64CMP);
	stream += TestRegRptrEncoding(ctx, stream, o_cmp64, x86CMP, testSizeQword);
	stream += TestRptrRegEncoding(ctx, stream, o_cmp64, x86CMP, testSizeQword);
	stream += TestRptrNumEncoding(ctx, stream, o_cmp64, x86CMP, testSizeQword);

	stream += TestRegRegEncoding(ctx, stream, o_test, x86TEST);

	stream += TestRegEncoding(ctx, stream, o_call, x86CALL);
	stream += TestRptrEncoding(ctx, stream, o_call, x86CALL, testSizeQword);

	assert(stream < buf + bufSize);

	char instBuf[128];

	unsigned instCount = unsigned(ctx.GetLastInstruction() - instList);

	CodeGenRegVmStateContext vmState;

	vmState.vsAsmStyle = true;

	OutputContext output;
	output.stream = output.openStream("asmX86_test.txt");

	for(unsigned i = 0; i < instCount; i++)
	{
		instList[i].Decode(vmState, instBuf);

		output.Print(instBuf);
		output.Print('\n');
	}

	output.Flush();
	output.closeStream(output.stream);
	output.stream = NULL;

	unsigned char *buf2 = new unsigned char[bufSize];

	unsigned char *stream2 = x86TranslateInstructionList(buf2, buf2 + bufSize, instList, instCount, NULL);

	assert(unsigned(stream2 - buf2) == unsigned(stream - buf));
	assert(memcmp(buf, buf2, unsigned(stream - buf)) == 0);

	/*DWORD oldProtect;
	VirtualProtect((void*)buf, unsigned(stream - buf), PAGE_EXECUTE_READWRITE, (DWORD*)&oldProtect);

	uintptr_t regFilePtr[8];

	typedef	uintptr_t(*nullcFunc)(unsigned char *codeStart, uintptr_t *regFilePtr);
	nullcFunc gate = (nullcFunc)(uintptr_t)codeLaunchHeader;
	gate(buf, regFilePtr);*/
}
