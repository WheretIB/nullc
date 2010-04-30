#pragma once

#include "Instruction_X86.h"

void x86ResetLabels();
void x86ClearLabels();
void x86ReserveLabels(unsigned int count);

int x86FLDZ(unsigned char* stream);
int x86FLD1(unsigned char* stream);

// fld st*
int x86FLD(unsigned char *stream, x87Reg reg);
// fld *word [index*mult+base+shift]
int x86FLD(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// fild *word [index*mult+base+shift]
int x86FILD(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// fst *word [index*mult+base+shift]
int x86FST(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// fstp st*
int x86FSTP(unsigned char *stream, x87Reg dst);
// fstp *word [index*mult+base+shift]
int x86FSTP(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// fistp *word [index*mult+base+shift]
int x86FISTP(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// fadd *word [index*mult+base+shift]
int x86FADD(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);
// faddp
int x86FADDP(unsigned char *stream);

// fsub *word [index*mult+base+shift]
int x86FSUB(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);
// fsubr *word [index*mult+base+shift]
int x86FSUBR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);
// fsubp
int x86FSUBP(unsigned char *stream);
// fsubrp
int x86FSUBRP(unsigned char *stream);

// fmul *word [index*mult+base+shift]
int x86FMUL(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);
// fmulp
int x86FMULP(unsigned char *stream);

// fdiv *word [index*mult+base+shift]
int x86FDIV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);
// fdivr *word [index*mult+base+shift]
int x86FDIVR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);
// fdivrp
int x86FDIVRP(unsigned char *stream);

int x86FCHS(unsigned char *stream);

int x86FPREM(unsigned char *stream);

int x86FSQRT(unsigned char *stream);

int x86FSINCOS(unsigned char *stream);
int x86FPTAN(unsigned char *stream);

int x86FRNDINT(unsigned char *stream);

// fcomp *word [index*mult+base+shift]
int x86FCOMP(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// fnstsw ax
int x86FNSTSW(unsigned char *stream);

// fstcw word [esp]
int x86FSTCW(unsigned char *stream);
// fldcw word [esp+shift]
int x86FLDCW(unsigned char *stream, int shift);

// push dword [index*mult+base+shift]
int x86PUSH(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);
// push reg
int x86PUSH(unsigned char *stream, x86Reg reg);
// push num
int x86PUSH(unsigned char *stream, int num);

// pop dword [index*mult+base+shift]
int x86POP(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);
// pop reg
int x86POP(unsigned char *stream, x86Reg reg);

// mov dst, num
int x86MOV(unsigned char *stream, x86Reg dst, int num);
// mov dst, src
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src);
// mov dst, dword [index*mult+base+shift]
int x86MOV(unsigned char *stream, x86Reg dst, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// mov *word [index*mult+base+shift], num
int x86MOV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// mov *word [index*mult+base+shift], src
int x86MOV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg src);

// movsx dst, *word [index*mult+base+shift]
int x86MOVSX(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// lea dst, [label+shift]
int x86LEA(unsigned char *stream, x86Reg dst, unsigned int labelID, int shift);
// lea dst, [index*mult+base+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Reg index, int multiplier, x86Reg base, int shift);

// neg reg
int x86NEG(unsigned char *stream, x86Reg reg);
// neg dword [index*mult+base+shift]
int x86NEG(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// add dst, num
int x86ADD(unsigned char *stream, x86Reg dst, int num);
// add dst, src
int x86ADD(unsigned char *stream, x86Reg dst, x86Reg src);
// add dst, dword [index*mult+base+shift]
int x86ADD(unsigned char *stream, x86Reg dst, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);
// add dword [index*mult+base+shift], num
int x86ADD(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// add dword [index*mult+base+shift], op2
int x86ADD(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);

// adc dst, num
int x86ADC(unsigned char *stream, x86Reg dst, int num);
// adc dst, src
int x86ADC(unsigned char *stream, x86Reg dst, x86Reg src);
// adc dword [index*mult+base+shift], num
int x86ADC(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// adc dword [index*mult+base+shift], op2
int x86ADC(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);

// sub dst, num
int x86SUB(unsigned char *stream, x86Reg dst, int num);
// sub dst, src
int x86SUB(unsigned char *stream, x86Reg dst, x86Reg src);
// sub dword [index*mult+base+shift], num
int x86SUB(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// sub dword [index*mult+base+shift], op2
int x86SUB(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);

// sbb dst, num
int x86SBB(unsigned char *stream, x86Reg dst, int num);
// sbb dword [index*mult+base+shift], num
int x86SBB(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// sbb dword [index*mult+base+shift], op2
int x86SBB(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);

// imul srcdst, num
int x86IMUL(unsigned char *stream, x86Reg srcdst, int num);
// imul dst, src
int x86IMUL(unsigned char *stream, x86Reg dst, x86Reg src);
// imul dst, dword [index*mult+base+shift]
int x86IMUL(unsigned char *stream, x86Reg dst, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);
// imul src
int x86IMUL(unsigned char *stream, x86Reg src);

// idiv src
int x86IDIV(unsigned char *stream, x86Reg src);
// idiv dword [index*mult+base+shift]
int x86IDIV(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// shl reg, val
int x86SHL(unsigned char *stream, x86Reg reg, int val);
// shl dword [index*mult+base+shift], val
int x86SHL(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int val);

// sal eax, cl
int x86SAL(unsigned char *stream);
// sar eax, cl
int x86SAR(unsigned char *stream);

// not reg
int x86NOT(unsigned char *stream, x86Reg reg);
// not dword [index*mult+base+shift]
int x86NOT(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// and op1, op2
int x86AND(unsigned char *stream, x86Reg op1, x86Reg op2);
// and dword [index*mult+base+shift], op2
int x86AND(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);
// and dword [index*mult+base+shift], num
int x86AND(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int num);

// or op1, op2
int x86OR(unsigned char *stream, x86Reg op1, x86Reg op2);
// or dword [index*mult+base+shift], op2
int x86OR(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);
// or dword [index*mult+base+shift], num
int x86OR(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int op2);
// or op1, dword [index*mult+base+shift]
int x86OR(unsigned char *stream, x86Reg op1, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// xor op1, op2
int x86XOR(unsigned char *stream, x86Reg op1, x86Reg op2);
// xor dword [index*mult+base+shift], op2
int x86XOR(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);
// xor dword [index*mult+base+shift], num
int x86XOR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num);

// cmp reg, num
int x86CMP(unsigned char *stream, x86Reg reg, int num);
// cmp reg1, reg2
int x86CMP(unsigned char *stream, x86Reg reg1, x86Reg reg2);
// cmp dword [index*mult+base+shift], op2
int x86CMP(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);
// cmp op1, dword [index*mult+base+shift]
int x86CMP(unsigned char *stream, x86Reg op1, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);
// cmp dword [index*mult+base+shift], num
int x86CMP(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int op2);

int x86TEST(unsigned char *stream, x86Reg op1, x86Reg op2);
// test ah, num
int x86TESTah(unsigned char* stream, char num);

// xchg dword [reg], op2
int x86XCHG(unsigned char *stream, x86Size, x86Reg reg, int shift, x86Reg op2);
// xchg regA, regB
int x86XCHG(unsigned char *stream, x86Reg regA, x86Reg regB);

int x86CDQ(unsigned char *stream);

// setcc *l
int x86SETcc(unsigned char *stream, x86Cond cond, x86Reg reg);

// call reg
int x86CALL(unsigned char *stream, x86Reg address);
// call [index*mult+base+shift]
int x86CALL(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, unsigned int shift);
// call label
int x86CALL(unsigned char *stream, unsigned int labelID);
// ret
int x86RET(unsigned char *stream);

// rep movsd
int x86REP_MOVSD(unsigned char *stream);

// int num
int x86INT(unsigned char *stream, int interrupt);

// nop
int x86NOP(unsigned char *stream);

int x86Jcc(unsigned char *stream, unsigned int labelID, x86Cond cond, bool isNear);
int x86JMP(unsigned char *stream, unsigned int labelID, bool isNear);

void x86AddLabel(unsigned char *stream, unsigned int labelID);
void x86SatisfyJumps(FastVector<unsigned char*>& instPos);

//int x86(unsigned char *stream);
