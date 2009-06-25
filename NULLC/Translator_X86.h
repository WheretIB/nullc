#pragma once

enum x86Reg{ rNONE, rEAX, rEBX, rECX, rEDX, rESP, rEDI, rEBP, rESI, };
enum x87Reg{ rST0, rST1, rST2, rST3, rST4, rST5, rST6, rST7, };
enum x86Size{ sNONE, sBYTE, sWORD, sDWORD, sQWORD, };
enum x86Cond{ condO, condNO, condB, condC, condAE, condNB, condNC, condE, condZ, condNE, condNZ,
				condBE, condNA, condA, condNBE, condS, condNS, condP, condPE, condNP, condPO,
				condL, condNGE, condGE, condNL, condLE, condNG, condG, condNLE };

const int rAX = rEAX;
const int rAL = rEAX;
const int rBX = rEBX;
const int rBL = rEBX;

void x86ClearLabels();

int x86FLDZ(unsigned char* stream);

// fld st*
int x86FLD(unsigned char *stream, x87Reg reg);
// fld *word [reg+shift]
int x86FLD(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift);
// fld *word [regA+regB+shift]
int x86FLD(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift);

// fild dword [reg]
int x86FILD(unsigned char *stream, x86Size, x86Reg reg);

// fstp st*
int x86FSTP(unsigned char *stream, x87Reg dst);
// fstp *word [reg+shift]
int x86FSTP(unsigned char *stream, x86Size size, x86Reg reg, unsigned int shift);
// fstp *word [regA+regB+shift]
int x86FSTP(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift);

// fadd *word [reg]
int x86FADD(unsigned char *stream, x86Size size, x86Reg reg);
// fadd *word [reg]
int x86FSUB(unsigned char *stream, x86Size size, x86Reg reg);
// fadd *word [reg]
int x86FSUBR(unsigned char *stream, x86Size size, x86Reg reg);
// fmul *word [reg]
int x86FMUL(unsigned char *stream, x86Size size, x86Reg reg);
// fdiv *word [reg]
int x86FDIV(unsigned char *stream, x86Size size, x86Reg reg);
// fdivr *word [reg]
int x86FDIVR(unsigned char *stream, x86Size size, x86Reg reg);
// fdivrp
int x86FDIVRP(unsigned char *stream);

int x86FPREM(unsigned char *stream);

int x86FSQRT(unsigned char *stream);

int x86FSINCOS(unsigned char *stream);
int x86FPTAN(unsigned char *stream);

int x86FRNDINT(unsigned char *stream);

// fcomp *word [reg]
int x86FCOMP(unsigned char *stream, x86Size size, x86Reg reg);

// fstcw word [esp]
int x86FSTCW(unsigned char *stream);
// fldcw word [esp+shift]
int x86FLDCW(unsigned char *stream, int shift);

// push *word [regA+regB+shift]
int x86PUSH(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift);
int x86PUSH(unsigned char *stream, x86Reg reg);
int x86PUSH(unsigned char *stream, int num);

int x86POP(unsigned char *stream, x86Reg reg);

// mov dst, num
int x86MOV(unsigned char *stream, x86Reg dst, int num);
// mov dst, src
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src);
// mov dst, dword [src+shift]
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src, x86Size, int shift);

// mov *word [regA+shift], num
int x86MOV(unsigned char *stream, x86Size size, x86Reg regA, int shift, int num);
// mov *word [regA+regB+shift], src
int x86MOV(unsigned char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift, x86Reg src);

// movsx dst, *word [regA+regB+shift]
int x86MOVSX(unsigned char *stream, x86Reg dst, x86Size size, x86Reg regA, x86Reg regB, int shift);

// lea dst, [src+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Reg src, int shift);
// lea dst, [src*multiplier+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Reg src, int multiplier, int shift);

int x86OR(unsigned char *stream, x86Reg op1, x86Reg op2);

// add dst, num
int x86ADD(unsigned char *stream, x86Reg dst, int num);
// add dword [reg+shift], op2
int x86ADD(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2);
// adc dword [reg+shift], op2
int x86ADC(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2);
// sub dst, num
int x86SUB(unsigned char *stream, x86Reg dst, int num);
// imul dst, num
int x86IMUL(unsigned char *stream, x86Reg srcdst, int num);
// idiv dword [reg]
int x86IDIV(unsigned char *stream, x86Size, x86Reg reg);

// shl reg, shift
int x86SHL(unsigned char *stream, x86Reg reg, int shift);
// shl dword [reg], shift
int x86SHL(unsigned char *stream, x86Size, x86Reg reg, int shift);

// cmp reg, num
int x86CMP(unsigned char *stream, x86Reg reg, int num);
// cmp dword [reg], op2
int x86CMP(unsigned char *stream, x86Size, x86Reg reg, x86Reg op2);

int x86TEST(unsigned char *stream, x86Reg op1, x86Reg op2);
// test ah, num
int x86TESTah(unsigned char* stream, char num);

// xchg dword [reg], op2
int x86XCHG(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2);

int x86CDQ(unsigned char *stream);

// setcc cl
int x86SETcc(unsigned char *stream, x86Cond cond);

int x86CALL(unsigned char *stream, x86Reg address);
int x86CALL(unsigned char *stream, const char* label);
int x86RET(unsigned char *stream);

int x86REP_MOVSD(unsigned char *stream);

int x86INT(unsigned char *stream, int interrupt);

int x86NOP(unsigned char *stream);

int x86Jcc(unsigned char *stream, const char* label, x86Cond cond);

// short - 0-255 bytes AFAIK
int x86JMPshort(unsigned char *stream, const char* label);
// near - 0-2^32  AFAIK
int x86JMPnear(unsigned char *stream, const char* label);

void x86AddLabel(unsigned char *stream, const char* label);

//int x86(unsigned char *stream);