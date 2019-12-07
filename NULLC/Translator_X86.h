#pragma once

#include "Instruction_X86.h"

void x86ResetLabels();
void x86ClearLabels();
void x86ReserveLabels(unsigned int count);

// movss dword [index*mult+base+shift], xmm*
int x86MOVSS(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86XmmReg src);

// movsd qword [index*mult+base+shift], xmm*
int x86MOVSD(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86XmmReg src);

// movsd xmm*, qword [index*mult+base+shift]
int x86MOVSD(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// movsd xmm*, xmm*
int x86MOVSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);

// movd reg, xmm*
int x86MOVD(unsigned char *stream, x86Reg dst, x86XmmReg src);

// movsxd reg, dword [index*mult+base+shift]
int x86MOVSXD(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// cvtss2sd xmm*, dword [index*mult+base+shift]
int x86CVTSS2SD(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// cvtsd2ss xmm*, qword [index*mult+base+shift]
int x86CVTSD2SS(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// cvttsd2si dst, qword [index*mult+base+shift]
int x86CVTTSD2SI(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// REX.W cvttsd2si dst, qword [index*mult+base+shift]
int x64CVTTSD2SI(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// cvtsi2sd xmm*, *word [index*mult+base+shift]
int x86CVTSI2SD(unsigned char *stream, x86XmmReg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

int x86ADDSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);
int x86SUBSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);
int x86MULSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);
int x86DIVSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);

int x86CMPEQSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);
int x86CMPLTSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);
int x86CMPLESD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);
int x86CMPNEQSD(unsigned char *stream, x86XmmReg dst, x86XmmReg src);

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

// pushad
int x86PUSHAD(unsigned char *stream);

// popad
int x86POPAD(unsigned char *stream);

// mov dst, num
int x86MOV(unsigned char *stream, x86Reg dst, int num);
// mov dst, src
int x86MOV(unsigned char *stream, x86Reg dst, x86Reg src);

// REX.W mov dst, num
int x64MOV(unsigned char *stream, x86Reg dst, uintptr_t num);
// REX.W mov dst, src
int x64MOV(unsigned char *stream, x86Reg dst, x86Reg src);

// mov dst, *word [index*mult+base+shift]
int x86MOV(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);
// mov *word [index*mult+base+shift], num
int x86MOV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// mov *word [index*mult+base+shift], src
int x86MOV(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg src);

// movsx dst, *word [index*mult+base+shift]
int x86MOVSX(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// lea dst, [label+shift]
int x86LEA(unsigned char *stream, x86Reg dst, unsigned int labelID, int shift);
// lea dst, [index*mult+base+shift]
int x86LEA(unsigned char *stream, x86Reg dst, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// neg reg
int x86NEG(unsigned char *stream, x86Reg reg);

// REX.W neg reg
int x64NEG(unsigned char *stream, x86Reg reg);

// neg dword [index*mult+base+shift]
int x86NEG(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// add dst, num
int x86ADD(unsigned char *stream, x86Reg dst, int num);
// add dst, src
int x86ADD(unsigned char *stream, x86Reg dst, x86Reg src);

// REX.W add dst, num
int x64ADD(unsigned char *stream, x86Reg dst, int num);
// REX.W add dst, src
int x64ADD(unsigned char *stream, x86Reg dst, x86Reg src);

// add dst, *word [index*mult+base+shift]
int x86ADD(unsigned char *stream, x86Reg dst, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);
// add *word [index*mult+base+shift], num
int x86ADD(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// add *word [index*mult+base+shift], op2
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

// REX.W add dst, num
int x64SUB(unsigned char *stream, x86Reg dst, int num);
// REX.W add dst, src
int x64SUB(unsigned char *stream, x86Reg dst, x86Reg src);

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

// REX.W imul dst, src
int x64IMUL(unsigned char *stream, x86Reg dst, x86Reg src);

// imul dst, dword [index*mult+base+shift]
int x86IMUL(unsigned char *stream, x86Reg dst, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// imul src
int x86IMUL(unsigned char *stream, x86Reg src);

// idiv src
int x86IDIV(unsigned char *stream, x86Reg src);

// REX.W idiv src
int x64IDIV(unsigned char *stream, x86Reg src);

// idiv dword [index*mult+base+shift]
int x86IDIV(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// shl reg, val
int x86SHL(unsigned char *stream, x86Reg reg, int val);
// shl dword [index*mult+base+shift], val
int x86SHL(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int val);

// sal reg, cl
int x86SAL(unsigned char *stream, x86Reg reg);

// REX.W sal reg, cl
int x64SAL(unsigned char *stream, x86Reg reg);

// sar reg, cl
int x86SAR(unsigned char *stream, x86Reg reg);

// REX.W sar reg, cl
int x64SAR(unsigned char *stream, x86Reg reg);

// not reg
int x86NOT(unsigned char *stream, x86Reg reg);

// REX.W not reg
int x64NOT(unsigned char *stream, x86Reg reg);

// not dword [index*mult+base+shift]
int x86NOT(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift);

// and op1, op2
int x86AND(unsigned char *stream, x86Reg op1, x86Reg op2);

// REX.W and op1, op2
int x64AND(unsigned char *stream, x86Reg op1, x86Reg op2);

// and op1, num
int x86AND(unsigned char *stream, x86Reg op1, int num);

// and dword [index*mult+base+shift], op2
int x86AND(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);
// and dword [index*mult+base+shift], num
int x86AND(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// and op1, *word [index*mult+base+shift]
int x86AND(unsigned char *stream, x86Reg op1, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// or op1, op2
int x86OR(unsigned char *stream, x86Reg op1, x86Reg op2);

// REX.W or op1, op2
int x64OR(unsigned char *stream, x86Reg op1, x86Reg op2);

// or op1, num
int x86OR(unsigned char *stream, x86Reg op1, int num);

// or dword [index*mult+base+shift], op2
int x86OR(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);
// or dword [index*mult+base+shift], num
int x86OR(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, int op2);
// or op1, *word [index*mult+base+shift]
int x86OR(unsigned char *stream, x86Reg op1, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// xor op1, op2
int x86XOR(unsigned char *stream, x86Reg op1, x86Reg op2);

// REX.W xor op1, op2
int x64XOR(unsigned char *stream, x86Reg op1, x86Reg op2);

// xor op1, num
int x86XOR(unsigned char *stream, x86Reg op1, int num);

// xor dword [index*mult+base+shift], op2
int x86XOR(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, int shift, x86Reg op2);
// xor dword [index*mult+base+shift], num
int x86XOR(unsigned char *stream, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift, int num);
// xor op1, *word [index*mult+base+shift]
int x86XOR(unsigned char *stream, x86Reg op1, x86Size size, x86Reg index, int multiplier, x86Reg base, int shift);

// cmp reg, num
int x86CMP(unsigned char *stream, x86Reg reg, int num);

// cmp reg1, reg2
int x86CMP(unsigned char *stream, x86Reg reg1, x86Reg reg2);

// REX.W cmp reg1, reg2
int x64CMP(unsigned char *stream, x86Reg reg1, x86Reg reg2);

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

// cdq
int x86CDQ(unsigned char *stream);

// cqo
int x86CQO(unsigned char *stream);

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

// rep stosb
int x86REP_STOSB(unsigned char *stream);

// rep stosw
int x86REP_STOSW(unsigned char *stream);

// rep stosd
int x86REP_STOSD(unsigned char *stream);

// rep stosq
int x86REP_STOSQ(unsigned char *stream);

// int num
int x86INT(unsigned char *stream, int interrupt);

// nop
int x86NOP(unsigned char *stream);

int x86Jcc(unsigned char *stream, unsigned int labelID, x86Cond cond, bool isNear);
int x86JMP(unsigned char *stream, x86Size, x86Reg index, int multiplier, x86Reg base, unsigned int shift);
int x86JMP(unsigned char *stream, unsigned int labelID, bool isNear);

void x86AddLabel(unsigned char *stream, unsigned int labelID);
void x86SatisfyJumps(FastVector<unsigned char*>& instPos);

//int x86(unsigned char *stream);
