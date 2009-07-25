#include "Optimizer_x86.h"

unsigned int OptimizerX86::Optimize(FastVector<x86Instruction, true, true>& instList)
{
	unsigned int opti = 0, lastPass;
	start = &instList[0];
	end = &instList[instList.size()];

	lastPass = OptimizationPass(instList);
	opti += lastPass;
	if(lastPass)
	{
		lastPass = OptimizationPass(instList);
		opti += lastPass;
	}
	if(lastPass)
	{
		lastPass = OptimizationPass(instList);
		opti += lastPass;
	}
	return opti;
}

unsigned int OptimizerX86::SearchUp(unsigned int from)
{
	from--;
	while((start[from].name == o_other || start[from].name == o_none) && from != 0)
		from--;
	return from;
}

unsigned int OptimizerX86::SearchDown(unsigned int from)
{
	from++;
	while((start[from].name == o_other || start[from].name == o_none) && from != (unsigned int)(end-start))
		from++;
	return from;
}

unsigned int OptimizerX86::OptimizationPass(FastVector<x86Instruction, true, true>& instList)
{
	unsigned int optimizeCount = 0;

	for(unsigned int i = 1; i < instList.size()-1; i++)
	{
		x86Instruction &curr = instList[i];
		unsigned int prevIndex = SearchUp(i), nextIndex = SearchDown(i);
		x86Instruction &prev = instList[prevIndex];
		x86Instruction &next = instList[nextIndex];
		// TODO: possible to look further than 1 instruction up?
		if(curr.name == o_pop && curr.argA.type == x86Argument::argPtr)
		{
			// Optimizations for "push num ... pop [location]" and "push register ... pop location"
			if(prev.name == o_push && (prev.argA.type == x86Argument::argNumber || prev.argA.type == x86Argument::argReg))
			{
				curr = x86Instruction(o_mov, curr.argA, prev.argA);
				prev = x86Instruction(o_none);
				optimizeCount++;
			}
		}
		// TODO: possible to look further than 1 instruction up?
		if(curr.name == o_pop && curr.argA.type == x86Argument::argReg)
		{
			// Optimizations for "push num ... pop reg", "push [location] ... pop reg" and "push regA ... pop regB"
			if(prev.name == o_push && (prev.argA.type == x86Argument::argNumber || prev.argA.type == x86Argument::argReg || prev.argA.type == x86Argument::argPtr))
			{
				curr = x86Instruction(o_mov, curr.argA, prev.argA);
				prev = x86Instruction(o_none);
				optimizeCount++;
			}
		}
		if(curr.name == o_fld && curr.argA == x86Argument(sQWORD, rESP, 0))
		{
			// push dword [a+4], push dword[a], fld qword [esp]
			x86Instruction &pPrev = instList[SearchUp(prevIndex)];
			if(prev.name == o_push && prev.argA.type == x86Argument::argPtr && pPrev.name == o_push && pPrev.argA.type == x86Argument::argPtr &&
				prev.argA == x86Argument(pPrev.argA.ptrSize, pPrev.argA.ptrReg[0], pPrev.argA.ptrMult, pPrev.argA.ptrReg[1], pPrev.argA.ptrNum-4))
			{
				curr = x86Instruction(o_fld, prev.argA);
				curr.argA.ptrSize = sQWORD;
				prev = x86Instruction(o_sub, x86Argument(rESP), x86Argument(8));
				pPrev = x86Instruction(o_none);
				optimizeCount++;
			}
			// fstp qword [esp+8], add esp, 8, fld qword [esp]
			if(prev.name == o_add && prev.argA == x86Argument(rESP) && prev.argB == x86Argument(8) &&
				pPrev.name == o_fstp && pPrev.argA == x86Argument(sQWORD, rESP, 8))
			{
				curr = x86Instruction(o_none);
				pPrev = x86Instruction(o_fst, pPrev.argA);
				optimizeCount++;
			}
			// fstp qword [esp], fld qword [esp]
			if(prev.name == o_fstp && prev.argA == x86Argument(sQWORD, rESP, 0))
			{
				curr = x86Instruction(o_none);
				prev = x86Instruction(o_fst, prev.argA);
				optimizeCount++;
			}
			// push 0, push 0, fld qword [esp]	// push 0.0, load
			if(prev.name == o_push && prev.argA == x86Argument(0) &&
				pPrev.name == o_push && pPrev.argA == x86Argument(0))
			{
				curr = x86Instruction(o_fldz);
				prev = x86Instruction(o_sub, x86Argument(rESP), x86Argument(8));
				pPrev = x86Instruction(o_none);
				optimizeCount++;
			}
			// push 0x3ff00000, push 0, fld qword [esp]	// push 1.0, load
			if(prev.name == o_push && prev.argA == x86Argument(0) &&
				pPrev.name == o_push && pPrev.argA == x86Argument(0x3ff00000))
			{
				curr = x86Instruction(o_fld1);
				prev = x86Instruction(o_sub, x86Argument(rESP), x86Argument(8));
				pPrev = x86Instruction(o_none);
				optimizeCount++;
			}
		}
		// push num, mov reg, dword [esp]
		if(curr.name == o_push && curr.argA.type == x86Argument::argNumber)
		{
			// Optimizations for "push num ... pop [location]" and "push register ... pop location"
			if(next.name == o_mov && next.argB == x86Argument(sDWORD, rESP, 0))
			{
				next = x86Instruction(o_mov, next.argA, curr.argA);
				curr = x86Instruction(o_sub, x86Argument(rESP), x86Argument(4));
				optimizeCount++;
			}
		}
		// add reg, num; sub reg, num
		if(curr.name == o_add && curr.argA.type == x86Argument::argReg && curr.argB.type == x86Argument::argNumber)
		{
			// Optimizations for "push num ... pop [location]" and "push register ... pop location"
			if(next.name == o_sub && next.argA.type == x86Argument::argReg && next.argB.type == x86Argument::argNumber && next.argA.reg == curr.argA.reg)
			{
				int res = curr.argB.num - next.argB.num;
				if(res == 0)
				{
					next = x86Instruction(o_none);
					curr = x86Instruction(o_none);
					optimizeCount += 2;
				}else{
					curr = x86Instruction(o_none);
					next = x86Instruction(o_add, next.argA, x86Argument(res));
					optimizeCount++;
				}
			}
		}
		// add reg, num; sub reg, num
		if(curr.name == o_sub && curr.argA.type == x86Argument::argReg && curr.argB.type == x86Argument::argNumber)
		{
			// Optimizations for "push num ... pop [location]" and "push register ... pop location"
			if(next.name == o_add && next.argA.type == x86Argument::argReg && next.argB.type == x86Argument::argNumber && next.argA.reg == curr.argA.reg)
			{
				int res = -curr.argB.num + next.argB.num;
				if(res == 0)
				{
					next = x86Instruction(o_none);
					curr = x86Instruction(o_none);
					optimizeCount += 2;
				}else{
					curr = x86Instruction(o_none);
					next = x86Instruction(o_add, next.argA, x86Argument(res));
					optimizeCount++;
				}
			}
		}
		// mov ebx, num; mov dword [address], ebx
		if(curr.name == o_mov && curr.argA.reg == rEBX && curr.argB.type == x86Argument::argNumber && next.argB.reg == rEBX)
		{
			next = x86Instruction(o_mov, next.argA, curr.argB);
			curr = x86Instruction(o_none);
			optimizeCount++;
		}
		// mov reg, reg
		if(curr.name == o_mov && curr.argA == curr.argB)
		{
			curr = x86Instruction(o_none);
			optimizeCount++;
		}
		// mov reg, 0
		if(curr.name == o_mov && curr.argA.type == x86Argument::argReg && curr.argB.type == x86Argument::argNumber && curr.argB.num == 0)
			curr = x86Instruction(o_xor, curr.argA, curr.argA);
/*
push dword [ebp+536870924]
push dword [ebp+536870920]
fld qword [esp+8]
fmul qword [esp]
fstp qword [esp+8]
add esp, 8

fld qword [ebp+536870920]
fmul qword [esp]
fstp qword [esp]
*/
		if(curr.name == o_fld && curr.argA == x86Argument(sQWORD, rESP, 8))
		{
			x86Instruction &pPrev = instList[SearchUp(prevIndex)];
			x86Instruction &next2 = instList[SearchDown(nextIndex)];
			x86Instruction &next3 = instList[SearchDown(SearchDown(nextIndex))];
			if(prev.name == o_push && prev.argA.type == x86Argument::argPtr && pPrev.name == o_push && pPrev.argA.type == x86Argument::argPtr &&
				prev.argA == x86Argument(pPrev.argA.ptrSize, pPrev.argA.ptrReg[0], pPrev.argA.ptrMult, pPrev.argA.ptrReg[1], pPrev.argA.ptrNum-4) &&
				next.argA == x86Argument(sQWORD, rESP, 0) &&
				next2.name == o_fstp && next2.argA == x86Argument(sQWORD, rESP, 8) &&
				next3.name == o_add && next3.argA == x86Argument(rESP) && next3.argB == x86Argument(8))
			{
				if(next.name == o_fadd || next.name == o_fmul)
				{
					curr = x86Instruction(o_fld, prev.argA);
					curr.argA.ptrSize = sQWORD;
					prev = x86Instruction(o_none);
					pPrev = x86Instruction(o_none);
					next = x86Instruction(next.name, x86Argument(sQWORD, rESP, 0));
					next2 = x86Instruction(o_fstp, x86Argument(sQWORD, rESP, 0));
					next3 = x86Instruction(o_none);
					optimizeCount += 3;
				}else if(next.name == o_fsub || next.name == o_fdiv){
					curr = x86Instruction(o_fld, prev.argA);
					curr.argA.ptrSize = sQWORD;
					prev = x86Instruction(o_none);
					pPrev = x86Instruction(o_none);
					next = x86Instruction(next.name == o_fsub ? o_fsubr : o_fdivr, x86Argument(sQWORD, rESP, 0));
					next2 = x86Instruction(o_fstp, x86Argument(sQWORD, rESP, 0));
					next3 = x86Instruction(o_none);
					optimizeCount += 3;
				}
			}
		}
		// fst qword [esp] fstp qword [537133128] add esp, 8
		if(curr.name == o_fst && curr.argA == x86Argument(sQWORD, rESP, 0))
		{
			x86Instruction &next2 = instList[SearchDown(nextIndex)];
			if(next.name == o_fstp && next.argA.type == x86Argument::argPtr && next.argA.ptrReg[0] == rNONE && next.argA.ptrReg[1] == rNONE &&
				next2.name == o_add && next2.argA == x86Argument(rESP) && next2.argB == x86Argument(8))
			{
				curr = x86Instruction(o_none);
				optimizeCount++;
			}
		}
		// push dword [a+4] push dword [a] fld qword [address] fmul qword [esp]
		if(curr.name == o_fld && curr.argA.type == x86Argument::argPtr && curr.argA.ptrSize == sQWORD && curr.argA.ptrReg[0] != rESP && curr.argA.ptrReg[1] == rNONE)
		{
			x86Instruction &pPrev = instList[SearchUp(prevIndex)];
			if(prev.name == o_push && prev.argA.type == x86Argument::argPtr && pPrev.name == o_push && pPrev.argA.type == x86Argument::argPtr &&
				prev.argA == x86Argument(pPrev.argA.ptrSize, pPrev.argA.ptrReg[0], pPrev.argA.ptrMult, pPrev.argA.ptrReg[1], pPrev.argA.ptrNum-4) &&
				next.argA == x86Argument(sQWORD, rESP, 0))
			{
				if(next.name == o_fadd || next.name == o_fmul || next.name == o_fsubr || next.name == o_fdivr)
				{
					next.argA = prev.argA;
					next.argA.ptrSize = sQWORD;
					prev = x86Instruction(o_sub, x86Argument(rESP), x86Argument(8));
					pPrev = x86Instruction(o_none);
					optimizeCount++;
				}
			}
		}
		// fstp qword [esp+8] add esp, 8 pop dword [a] pop dword [a+4]
		if(curr.name == o_fstp && curr.argA == x86Argument(sQWORD, rESP, 8))
		{
			x86Instruction &next2 = instList[SearchDown(nextIndex)];
			x86Instruction &next3 = instList[SearchDown(SearchDown(nextIndex))];
			if(next2.name == o_pop && next2.argA.type == x86Argument::argPtr && next3.name == o_pop && next3.argA.type == x86Argument::argPtr &&
				next2.argA == x86Argument(next3.argA.ptrSize, next3.argA.ptrReg[0], next3.argA.ptrMult, next3.argA.ptrReg[1], next3.argA.ptrNum-4) &&
				next.name == o_add && next.argA == x86Argument(rESP) && next.argB == x86Argument(8))
			{
				curr.argA = next2.argA;
				curr.argA.ptrSize = sQWORD;
				next.argB = x86Argument(16);
				next2 = x86Instruction(o_none);
				next3 = x86Instruction(o_none);
				optimizeCount += 2;
			}
		}

	}
	return optimizeCount;
}
