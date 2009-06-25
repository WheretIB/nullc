#include "Translator_X86.h"
#include <vector>
#include <assert.h>

// Mapping from x86Reg to register code
char	regCode[] = { -1, 0, 3, 1, 2, 4, 7, 5, 6 };
// Segment codes
enum	segCode{ segES, segCS, segSS, segDS, segFS, segGS };
// x87Reg are mapped to FP register codes directly
//char	fpCode[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
// Mapping from x86Cond to x86 conditions
char	condCode[] = { 0, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15 };

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
unsigned int	encodeAddress(unsigned char* stream, x86Reg index, int multiplier, x86Reg base, unsigned int displacement, char spareField)
{
	assert(index != rESP);
	unsigned char* start = stream;

	unsigned char mod = 0;
	if(displacement)
	{
		if(displacement < 256)
			mod = 1 << 6;
		else
			mod = 2 << 6;
	}

	// special case: [ebp] should be encoded as [ebp+0]
	if(displacement == 0 && base == rEBP)
		mod = 1 << 6;

	unsigned char spare = spareField << 3;

	unsigned char RM = regCode[rEBP]; // by default, it's simply [displacement]
	if(base != rNONE)
		RM = regCode[base];	// this is [base + displacement]
	if(index != rNONE)
		RM = regCode[rESP];	// this changes mode to [index*multiplier + base + displacement]

	*stream++ = mod | spare | RM;

	unsigned char sibScale = 0;
	if(multiplier == 1)
		sibScale = 0 << 6;
	else if(multiplier == 2)
		sibScale = 1 << 6;
	else if(multiplier == 4)
		sibScale = 2 << 6;
	else if(multiplier == 8)
		sibScale = 3 << 6;
	else
		assert(!"scale must be 1, 2, 4 or 8");
	unsigned char sibIndex = (index != rNONE ? regCode[index] << 3 : regCode[rESP] << 3);
	unsigned char sibBase = regCode[base];

	if(index != rNONE && base == rNONE)
		sibBase = regCode[rEBP];
	if(index != rNONE || base == rESP)
		*stream++ = sibScale | sibIndex | sibBase;
	
	if(displacement < 256)
		*stream = (unsigned char)displacement;
	else
		*(int*)stream = displacement;
	return (int)(stream - start) + (mod == 0 ? 0 : (displacement < 256 ? 1 : 4));
}

struct LabelInfo
{
	LabelInfo():pos(NULL){}
	LabelInfo(const char *newLabel, const unsigned char *newPos)
	{
		strncpy(label, newLabel, 16);
		label[15] = 0;
		pos = newPos;
	}

	char label[16];
	const unsigned char *pos;
};

struct UnsatisfiedJump
{
	UnsatisfiedJump(const char *newLabel, bool newIsNear, unsigned char *newJmpPos)
	{
		strncpy(label, newLabel, 16);
		label[15] = 0;
		isNear = newIsNear;
		jmpPos = newJmpPos;
	}

	char label[16];
	bool isNear;
	unsigned char *jmpPos;
};

std::vector<LabelInfo>	labels;
std::vector<UnsatisfiedJump> pendingJumps;

bool FindLabel(const char *name, LabelInfo& info)
{
	for(unsigned int i = 0; i < labels.size(); i++)
	{
		if(strcmp(labels[i].label, name) == 0)
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
	return 0;
}

int x86FLD1(unsigned char* stream)
{
	return 0;
}

// fld st*
int x86FLD(unsigned char *stream, x87Reg reg)
{
	return 0;
}

// fld *word [reg+shift]
int x86FLD(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	return 0;
}
// fld *word [regA+regB+shift]
int x86FLD(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	return 0;
}

// fild dword [reg]
int x86FILD(unsigned char *stream, x86Size, x86Reg reg)
{
	return 0;
}

// fst *word [reg+shift]
int x86FST(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	return 0;
}
// fst *word [regA+regB+shift]
int x86FST(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	return 0;
}

// fstp st*
int x86FSTP(unsigned char *stream, x87Reg dst)
{
	return 0;
}
// fstp *word [reg+shift]
int x86FSTP(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	return 0;
}
// fstp *word [regA+regB+shift]
int x86FSTP(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	return 0;
}

// fistp *word [reg+shift]
int x86FISTP(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	return 0;
}

// fadd *word [reg]
int x86FADD(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// faddp
int x86FADDP(unsigned char *stream)
{
	return 0;
}
// fsub *word [reg]
int x86FSUB(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fsubr *word [reg]
int x86FSUBR(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fsubp *word [reg]
int x86FSUBP(unsigned char *stream)
{
	return 0;
}
// fsubrp *word [reg]
int x86FSUBRP(unsigned char *stream)
{
	return 0;
}
// fmul *word [reg]
int x86FMUL(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fmulp
int x86FMULP(unsigned char *stream)
{
	return 0;
}
// fdiv *word [reg]
int x86FDIV(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fdivr *word [reg]
int x86FDIVR(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fdivrp
int x86FDIVRP(unsigned char *stream)
{
	return 0;
}

int x86FCHS(unsigned char *stream)
{
	return 0;
}

int x86FPREM(unsigned char *stream)
{
	return 0;
}

int x86FSQRT(unsigned char *stream)
{
	return 0;
}

int x86FSINCOS(unsigned char *stream)
{
	return 0;
}
int x86FPTAN(unsigned char *stream)
{
	return 0;
}

int x86FRNDINT(unsigned char *stream)
{
	return 0;
}

// fcomp *word [reg+shift]
int x86FCOMP(unsigned char *stream, x86Size size, x86Reg reg, int shift)
{
	return 0;
}

// fnstsw ax
int x86FNSTSW(unsigned char *stream)
{
	return 0;
}

// target - word [esp]
int x86FSTCW(unsigned char *stream)
{
	return 0;
}
int x86FLDCW(unsigned char *stream, int shift)
{
	return 0;
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
int x86POP(unsigned char *stream, x86Size, x86Reg regA, x86Reg regB, int shift)
{
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
	stream[0] = 0x8b;
	stream[1] = encodeRegister(src, regCode[dst]);
	return 2;
}
// mov dst, dword [src+shift]
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src, x86Size size, int shift)
{
	assert(size == sDWORD);
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
	*(int*)(stream+2+asize) = (int)(num);
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
		stream[0] = 0x88;
		unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, regCode[src]);
		return 1+asize;
	}else if(size == sWORD){
		stream[0] = 0x66;	// switch to word
		stream[1] = 0x89;
		unsigned int asize = encodeAddress(stream+2, regA, 1, regB, shift, regCode[src]);
		return 2+asize;
	}
	stream[0] = 0x89;
	unsigned int asize = encodeAddress(stream+1, regA, 1, regB, shift, regCode[src]);
	return 1+asize;
}

// movsx dst, *word [regA+regB+shift]
int x86MOVSX(unsigned char *stream, x86Reg dst, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	return 0;
}

// lea dst, [src+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Reg src, int shift)
{
	return 0;
}
// lea dst, [src*multiplier+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Reg src, int multiplier, int shift)
{
	return 0;
}


// neg dword [reg+shift]
int x86NEG(unsigned char *stream, x86Size, x86Reg reg, int shift)
{
	return 0;
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
// add dword [reg+shift], op2
int x86ADD(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	stream[0] = 0x01;
	unsigned int size = encodeAddress(stream+1, rNONE, 1, reg, shift, regCode[op2]);
	return 1 + size;
}

// adc dst, num
int x86ADC(unsigned char *stream, x86Reg dst, int num)
{
	return 0;
}
// adc dword [reg+shift], num
int x86ADC(unsigned char *stream, x86Size, x86Reg reg, int shift, int num)
{
	return 0;
}
// adc dword [reg+shift], op2
int x86ADC(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}

// sub dst, num
int x86SUB(unsigned char *stream, x86Reg dst, int num)
{
	return 0;
}
// sub dword [reg+shift], op2
int x86SUB(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}

// sbb dst, num
int x86SBB(unsigned char *stream, x86Reg dst, int num)
{
	return 0;
}
// sbb dword [reg+shift], op2
int x86SBB(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}

// imul dst, num
int x86IMUL(unsigned char *stream, x86Reg srcdst, int num)
{
	return 0;
}
// imul src
int x86IMUL(unsigned char *stream, x86Reg src)
{
	return 0;
}

// idiv dword [reg]
int x86IDIV(unsigned char *stream, x86Size, x86Reg reg)
{
	return 0;
}

// shl reg, shift
int x86SHL(unsigned char *stream, x86Reg reg, int shift)
{
	return 0;
}
// shl dword [reg], shift
int x86SHL(unsigned char *stream, x86Size, x86Reg reg, int shift)
{
	return 0;
}

// sal eax, cl
int x86SAL(unsigned char *stream)
{
	return 0;
}
// sar eax, cl
int x86SAR(unsigned char *stream)
{
	return 0;
}

// not dword [reg+shift]
int x86NOT(unsigned char *stream, x86Size, x86Reg reg, int shift)
{
	return 0;
}

// and dword [reg+shift], op2
int x86AND(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}

// or op1, op2
int x86OR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	return 0;
}
// or op1, dword [reg+shift]
int x86OR(unsigned char *stream, x86Reg op1, x86Size, x86Reg reg, int shift)
{
	return 0;
}

// xor op1, op2
int x86XOR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	stream[0] = 0x31;
	stream[1] = encodeRegister(op2, regCode[op1]);
	return 2;
}
// xor dword [reg+shift], op2
int x86XOR(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}

// cmp reg, num
int x86CMP(unsigned char *stream, x86Reg reg, int num)
{
	return 0;
}
// cmp reg1, reg2
int x86CMP(unsigned char *stream, x86Reg reg1, x86Reg reg2)
{
	return 0;
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
int x86CMP(unsigned char *stream, x86Size, x86Reg reg, int shift, int op2)
{
	return 0;
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
	return 0;
}

// xchg dword [reg], op2
int x86XCHG(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}

int x86CDQ(unsigned char *stream)
{
	return 0;
}

// setcc cl
int x86SETcc(unsigned char *stream, x86Cond cond)
{
	stream[0] = 0x0f;
	stream[1] = 0x90 + condCode[cond];
	stream[2] = encodeRegister(rECX, 0);
	return 3;
}

int x86CALL(unsigned char *stream, x86Reg address)
{
	return 0;
}
int x86CALL(unsigned char *stream, const char* label)
{
	return 0;
}
int x86RET(unsigned char *stream)
{
	return 0;
}

int x86REP_MOVSD(unsigned char *stream)
{
	return 0;
}

int x86INT(unsigned char *stream, int interrupt)
{
	return 0;
}

int x86NOP(unsigned char *stream)
{
	stream[0] = 0x90;
	return 1;
}

int x86Jcc(unsigned char *stream, const char* label, x86Cond cond, bool isNear)
{
	LabelInfo info;
	if(isNear)
		label += 5;

	if(isNear)
	{
		stream[0] = 0x0f;
		stream[1] = 0x80 + condCode[cond];
	}else{
		stream[0] = 0x70 + condCode[cond];
	}

	if(!FindLabel(label, info))
	{
		pendingJumps.push_back(UnsatisfiedJump(label, isNear, stream));
	}else{
		if(isNear)
		{
			*(int*)(stream+2) = (int)(info.pos-stream);
		}else{
			assert(info.pos-stream + 128 < 256);
			stream[1] = (char)(info.pos-stream);
		}
	}
	return (isNear ? 6 : 2);
}

int x86JMP(unsigned char *stream, const char* label, bool isNear)
{
	LabelInfo info;
	if(isNear)
		label += 5;

	if(isNear)
		stream[0] = 0xE9;
	else
		stream[0] = 0xEB;

	if(!FindLabel(label, info))
	{
		pendingJumps.push_back(UnsatisfiedJump(label, isNear, stream));
	}else{
		if(isNear)
		{
			*(int*)(stream+1) = (int)(info.pos-stream);
		}else{
			assert(info.pos-stream + 128 < 256);
			stream[1] = (char)(info.pos-stream);
		}
	}
	return (isNear ? 5 : 2);
}

void x86AddLabel(unsigned char *stream, const char* label)
{
	labels.push_back(LabelInfo(label, stream));
	for(unsigned int i = 0; i < pendingJumps.size(); i++)
	{
		UnsatisfiedJump& uJmp = pendingJumps[i];
		if(strcmp(uJmp.label, label) == 0)
		{
			if(uJmp.isNear)
			{
				if(*uJmp.jmpPos == 0x0f)
					*(int*)(uJmp.jmpPos+2) = (int)(stream-uJmp.jmpPos);
				else
					*(int*)(uJmp.jmpPos+1) = (int)(stream-uJmp.jmpPos);
			}else{
				assert(uJmp.jmpPos-stream + 128 < 256);
				*(char*)(uJmp.jmpPos+1) = (char)(stream-uJmp.jmpPos);
			}
			uJmp.label[0] = 0;
		}
	}
}