#include "Translator_X86.h"
#include <vector>

struct LabelInfo
{
	LabelInfo(const char *newLabel, const char *newPos)
	{
		strncpy(label, newLabel, 16);
		label[15] = 0;
		pos = newPos;
	}

	char label[16];
	const char *pos;
};

std::vector<LabelInfo>	labels;

void x86ClearLabels()
{
	labels.clear();
}

int x86FLDZ(char* stream)
{
	return 0;
}

// fld *word [reg+shift]
int x86FLD(char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	return 0;
}
// fld *word [regA+regB+shift]
int x86FLD(char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	return 0;
}

// fild dword [reg]
int x86FILD(char *stream, x86Size, x86Reg reg)
{
	return 0;
}

// fst st*
int x86FSTP(char *stream, x87Reg dst)
{
	return 0;
}
// fst *word [reg+shift]
int x86FSTP(char *stream, x86Size size, x86Reg reg, unsigned int shift)
{
	return 0;
}

// fadd *word [reg]
int x86FADD(char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fadd *word [reg]
int x86FSUB(char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fadd *word [reg]
int x86FSUBR(char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fmul *word [reg]
int x86FMUL(char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fdiv *word [reg]
int x86FDIV(char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fdivr *word [reg]
int x86FDIVR(char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fdivrp
int x86FDIVRP(char *stream)
{
	return 0;
}

int x86FPREM(char *stream)
{
	return 0;
}

int x86FSQRT(char *stream)
{
	return 0;
}

int x86FSINCOS(char *stream)
{
	return 0;
}
int x86FPTAN(char *stream)
{
	return 0;
}

int x86FRNDINT(char *stream)
{
	return 0;
}

// fcomp *word [reg]
int x86FCOMP(char *stream, x86Size size, x86Reg reg)
{
	return 0;
}

// target - word [esp]
int x86FSTCW(char *stream)
{
	return 0;
}
int x86FLDCW(char *stream)
{
	return 0;
}

// push *word [regA+regB+shift]
int x86PUSH(char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	return 0;
}
int x86PUSH(char *stream, x86Reg reg)
{
	return 0;
}
int x86PUSH(char *stream, int num)
{
	return 0;
}

int x86POP(char *stream, x86Reg reg)
{
	return 0;
}

int x86MOV(char *stream, x86Reg dst, int src)
{
	return 0;
}
int x86MOV(char *stream, x86Reg dst, x86Reg src)
{
	return 0;
}
// mov dst, dword [src+shift]
int x86MOV(char *stream, x86Reg dst, x86Reg src, x86Size, int shift)
{
	return 0;
}
// mov *word [regA+regB+shift], src
int x86MOV(char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift, x86Reg src)
{
	return 0;
}

// movsx dst, *word [regA+regB+shift]
int x86MOVSX(char *stream, x86Reg dst, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	return 0;
}

// lea dst, [src+shift]
int x86LEA(char *stream, x86Reg dst, x86Reg src, int shift)
{
	return 0;
}
// lea dst, [src*multiplier+shift]
int x86LEA(char *stream, x86Reg dst, x86Reg src, int multiplier, int shift)
{
	return 0;
}

int x86OR(char *stream, x86Reg op1, x86Reg op2)
{
	return 0;
}

// add dst, num
int x86ADD(char *stream, x86Reg dst, int num)
{
	return 0;
}
// add dword [reg+shift], op2
int x86ADD(char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}
// adc dword [reg+shift], op2
int x86ADC(char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}
// sub dst, num
int x86SUB(char *stream, x86Reg dst, int num)
{
	return 0;
}
// imul dst, num
int x86IMUL(char *stream, x86Reg srcdst, int num)
{
	return 0;
}
// idiv dword [reg]
int x86IDIV(char *stream, x86Size, x86Reg reg)
{
	return 0;
}

// shl reg, shift
int x86SHL(char *stream, x86Reg reg, int shift)
{
	return 0;
}
// shl dword [reg], shift
int x86SHL(char *stream, x86Size, x86Reg reg, int shift)
{
	return 0;
}

// cmp reg, num
int x86CMP(char *stream, x86Reg reg, int num)
{
	return 0;
}
// cmp dword [reg], op2
int x86CMP(char *stream, x86Size, x86Reg reg, x86Reg op2)
{
	return 0;
}

int x86TEST(char *stream, x86Reg op1, x86Reg op2)
{
	return 0;
}
// test ah, num
int x86TESTah(char* stream, char num)
{
	return 0;
}

// xchg dword [reg], op2
int x86XCHG(char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
{
	return 0;
}

int x86CDQ(char *stream)
{
	return 0;
}

// setl cl
int x86SETL(char *stream)
{
	return 0;
}
// setg cl
int x86SETG(char *stream)
{
	return 0;
}
// setle cl
int x86SETLE(char *stream)
{
	return 0;
}
// setge cl
int x86SETGE(char *stream)
{
	return 0;
}
// sete cl
int x86SETE(char *stream)
{
	return 0;
}
// setne cl
int x86SETNE(char *stream)
{
	return 0;
}

int x86CALL(char *stream, x86Reg address)
{
	return 0;
}
int x86CALL(char *stream, const char* label)
{
	return 0;
}
int x86RET(char *stream)
{
	return 0;
}

int x86REP_MOVSD(char *stream)
{
	return 0;
}

int x86INT(char *stream, int interrupt)
{
	return 0;
}

int x86NOP(unsigned char *stream)
{
	stream[0] = 0x90;
	return 1;
}

int x86JA(char *stream, const char* label)
{
	return 0;
}
int x86JAE(char *stream, const char* label)
{
	return 0;
}
int x86JB(char *stream, const char* label)
{
	return 0;
}
int x86JBE(char *stream, const char* label)
{
	return 0;
}
int x86JG(char *stream, const char* label)
{
	return 0;
}
int x86JL(char *stream, const char* label)
{
	return 0;
}

int x86JP(char *stream, const char* label)
{
	return 0;
}
int x86JE(char *stream, const char* label)
{
	return 0;
}
int x86JZ(char *stream, const char* label)
{
	return 0;
}

int x86JNP(char *stream, const char* label)
{
	return 0;
}
int x86JNE(char *stream, const char* label)
{
	return 0;
}
int x86JNZ(char *stream, const char* label)
{
	return 0;
}

// short - 0-255 bytes AFAIK
int x86JMPshort(char *stream, const char* label)
{
	return 0;
}
// near - 0-2^32  AFAIK
int x86JMPnear(char *stream, const char* label)
{
	return 0;
}

void x86AddLabel(char *stream, const char* label)
{
	labels.push_back(LabelInfo(label, stream));
}