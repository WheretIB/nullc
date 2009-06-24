#pragma once

enum x86Reg{ rNONE, rEAX, rEBX, rECX, rEDX, rESP, rEDI, rEBP, };
enum x87Reg{ rST0, rST1, rST2, rST3, rST4, rST5, rST6, rST7, };
enum x86Size{ sNONE, sBYTE, sWORD, sDWORD, sQWORD, };

void x86ClearLabels();

int x86FLDZ(char* stream);

// fld *word [reg+shift]
int x86FLD(char *stream, x86Size size, x86Reg reg, unsigned int shift);
// fld *word [regA+regB+shift]
int x86FLD(char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift);

// fild dword [reg]
int x86FILD(char *stream, x86Size, x86Reg reg);

// fst st*
int x86FSTP(char *stream, x87Reg dst);
// fst *word [reg+shift]
int x86FSTP(char *stream, x86Size size, x86Reg reg, unsigned int shift);

// fadd *word [reg]
int x86FADD(char *stream, x86Size size, x86Reg reg);
// fadd *word [reg]
int x86FSUB(char *stream, x86Size size, x86Reg reg);
// fadd *word [reg]
int x86FSUBR(char *stream, x86Size size, x86Reg reg);
// fmul *word [reg]
int x86FMUL(char *stream, x86Size size, x86Reg reg);
// fdiv *word [reg]
int x86FDIV(char *stream, x86Size size, x86Reg reg);
// fdivr *word [reg]
int x86FDIVR(char *stream, x86Size size, x86Reg reg);
// fdivrp
int x86FDIVRP(char *stream);

int x86FPREM(char *stream);

int x86FSQRT(char *stream);

int x86FSINCOS(char *stream);
int x86FPTAN(char *stream);

int x86FRNDINT(char *stream);

// fcomp *word [reg]
int x86FCOMP(char *stream, x86Size size, x86Reg reg);

// target - word [esp]
int x86FSTCW(char *stream);
int x86FLDCW(char *stream);

// push *word [regA+regB+shift]
int x86PUSH(char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift);
int x86PUSH(char *stream, x86Reg reg);
int x86PUSH(char *stream, int num);

int x86POP(char *stream, x86Reg reg);

int x86MOV(char *stream, x86Reg dst, int src);
int x86MOV(char *stream, x86Reg dst, x86Reg src);
// mov dst, dword [src+shift]
int x86MOV(char *stream, x86Reg dst, x86Reg src, x86Size, int shift);
// mov *word [regA+regB+shift], src
int x86MOV(char *stream, x86Size size, x86Reg regA, x86Reg regB, int shift, x86Reg src);

// movsx dst, *word [regA+regB+shift]
int x86MOVSX(char *stream, x86Reg dst, x86Size size, x86Reg regA, x86Reg regB, int shift);

// lea dst, [src+shift]
int x86LEA(char *stream, x86Reg dst, x86Reg src, int shift);
// lea dst, [src*multiplier+shift]
int x86LEA(char *stream, x86Reg dst, x86Reg src, int multiplier, int shift);

int x86OR(char *stream, x86Reg op1, x86Reg op2);

// add dst, num
int x86ADD(char *stream, x86Reg dst, int num);
// add dword [reg+shift], op2
int x86ADD(char *stream, x86Size, x86Reg reg, int shift, x86Reg op2);
// adc dword [reg+shift], op2
int x86ADC(char *stream, x86Size, x86Reg reg, int shift, x86Reg op2);
// sub dst, num
int x86SUB(char *stream, x86Reg dst, int num);
// imul dst, num
int x86IMUL(char *stream, x86Reg srcdst, int num);
// idiv dword [reg]
int x86IDIV(char *stream, x86Size, x86Reg reg);

// shl reg, shift
int x86SHL(char *stream, x86Reg reg, int shift);
// shl dword [reg], shift
int x86SHL(char *stream, x86Size, x86Reg reg, int shift);

// cmp reg, num
int x86CMP(char *stream, x86Reg reg, int num);
// cmp dword [reg], op2
int x86CMP(char *stream, x86Size, x86Reg reg, x86Reg op2);

int x86TEST(char *stream, x86Reg op1, x86Reg op2);
// test ah, num
int x86TESTah(char* stream, char num);

// xchg dword [reg], op2
int x86XCHG(char *stream, x86Size, x86Reg reg, int shift, x86Reg op2);

int x86CDQ(char *stream);

// setl cl
int x86SETL(char *stream);
// setg cl
int x86SETG(char *stream);
// setle cl
int x86SETLE(char *stream);
// setge cl
int x86SETGE(char *stream);
// sete cl
int x86SETE(char *stream);
// setne cl
int x86SETNE(char *stream);

int x86CALL(char *stream, x86Reg address);
int x86CALL(char *stream, const char* label);
int x86RET(char *stream);

int x86REP_MOVSD(char *stream);

int x86INT(char *stream, int interrupt);

int x86NOP(unsigned char *stream);

int x86JA(char *stream, const char* label);
int x86JAE(char *stream, const char* label);
int x86JB(char *stream, const char* label);
int x86JBE(char *stream, const char* label);
int x86JG(char *stream, const char* label);
int x86JL(char *stream, const char* label);

int x86JP(char *stream, const char* label);
int x86JE(char *stream, const char* label);
int x86JZ(char *stream, const char* label);

int x86JNP(char *stream, const char* label);
int x86JNE(char *stream, const char* label);
int x86JNZ(char *stream, const char* label);

// short - 0-255 bytes AFAIK
int x86JMPshort(char *stream, const char* label);
// near - 0-2^32  AFAIK
int x86JMPnear(char *stream, const char* label);

void x86AddLabel(char *stream, const char* label);

//int x86(char *stream);