#include "Translator_X86.h"
#include <vector>

// Mapping from x86Reg to register code
char	regCode[] = { -1, 0, 3, 1, 2, 4, 7, 5, 6 };
// Segment codes
enum	segCode{ segES, segCS, segSS, segDS, segFS, segGS };
// x87Reg are mapped to FP register codes directly
//char	fpCode[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

// x86 conditions
enum	x86Cond{ condO, condNO, condB, condC, condAE, condNB, condNC, condE, condZ, condNE, condNZ,
				condBE, condNA, condA, condNBE, condS, condNS, condP, condPE, condNP, condPO,
				condL, condNGE, condGE, condNL, condLE, condNG, condG, condNLE };
char	condCode[] = { 0, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15 };

struct LabelInfo
{
	LabelInfo(const char *newLabel, const unsigned char *newPos)
	{
		strncpy(label, newLabel, 16);
		label[15] = 0;
		pos = newPos;
	}

	char label[16];
	const unsigned char *pos;
};

std::vector<LabelInfo>	labels;

void x86ClearLabels()
{
	labels.clear();
}

int x86FLDZ(unsigned char* stream)
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

// fadd *word [reg]
int x86FADD(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fadd *word [reg]
int x86FSUB(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fadd *word [reg]
int x86FSUBR(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}
// fmul *word [reg]
int x86FMUL(unsigned char *stream, x86Size size, x86Reg reg)
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

// fcomp *word [reg]
int x86FCOMP(unsigned char *stream, x86Size size, x86Reg reg)
{
	return 0;
}

// target - word [esp]
int x86FSTCW(unsigned char *stream)
{
	return 0;
}
int x86FLDCW(unsigned char *stream)
{
	return 0;
}

// push *word [regA+regB+shift]
int x86PUSH(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift)
{
	return 0;
}
int x86PUSH(unsigned char *stream, x86Reg reg)
{
	return 0;
}
int x86PUSH(unsigned char *stream, int num)
{
	return 0;
}

int x86POP(unsigned char *stream, x86Reg reg)
{
	return 0;
}

int x86MOV(unsigned char *stream, x86Reg dst, int src)
{
	return 0;
}
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src)
{
	return 0;
}
// mov dst, dword [src+shift]
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src, x86Size, int shift)
{
	return 0;
}
// mov *word [regA+regB+shift], src
int x86MOV(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift, x86Reg src)
{
	return 0;
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

int x86OR(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	return 0;
}

// add dst, num
int x86ADD(unsigned char *stream, x86Reg dst, int num)
{
	return 0;
}
// add dword [reg+shift], op2
int x86ADD(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2)
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
// imul dst, num
int x86IMUL(unsigned char *stream, x86Reg srcdst, int num)
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

// cmp reg, num
int x86CMP(unsigned char *stream, x86Reg reg, int num)
{
	return 0;
}
// cmp dword [reg], op2
int x86CMP(unsigned char *stream, x86Size, x86Reg reg, x86Reg op2)
{
	return 0;
}

int x86TEST(unsigned char *stream, x86Reg op1, x86Reg op2)
{
	return 0;
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

// setl cl
int x86SETL(unsigned char *stream)
{
	return 0;
}
// setg cl
int x86SETG(unsigned char *stream)
{
	return 0;
}
// setle cl
int x86SETLE(unsigned char *stream)
{
	return 0;
}
// setge cl
int x86SETGE(unsigned char *stream)
{
	return 0;
}
// sete cl
int x86SETE(unsigned char *stream)
{
	return 0;
}
// setne cl
int x86SETNE(unsigned char *stream)
{
	return 0;
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

int x86JA(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JAE(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JB(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JBE(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JG(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JL(unsigned char *stream, const char* label)
{
	return 0;
}

int x86JP(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JE(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JZ(unsigned char *stream, const char* label)
{
	return 0;
}

int x86JNP(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JNE(unsigned char *stream, const char* label)
{
	return 0;
}
int x86JNZ(unsigned char *stream, const char* label)
{
	return 0;
}

// short - 0-255 bytes AFAIK
int x86JMPshort(unsigned char *stream, const char* label)
{
	return 0;
}
// near - 0-2^32  AFAIK
int x86JMPnear(unsigned char *stream, const char* label)
{
	return 0;
}

void x86AddLabel(unsigned char *stream, const char* label)
{
	labels.push_back(LabelInfo(label, stream));
}