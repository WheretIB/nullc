#include "stdafx.h"
#include "Executor.h"

#include "nullc.h"
#include "nullc_debug.h"
#include "StdLib.h"

#if defined(NULLC_USE_DYNCALL)

	#define dcAllocMem NULLC::alloc
	#define dcFreeMem  NULLC::dealloc

	#include "../external/dyncall/dyncall.h"

#endif

#ifdef NULLC_VM_CALL_STACK_UNWRAP
#define NULLC_UNWRAP(x) x
#else
#define NULLC_UNWRAP(x)
#endif

int vmIntPow(int power, int number)
{
	if(power < 0)
		return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

	int result = 1;
	while(power)
	{
		if(power & 1)
		{
			result *= number;
			power--;
		}
		number *= number;
		power >>= 1;
	}
	return result;
}

long long vmLongPow(long long power, long long number)
{
	if(power < 0)
		return number == 1 ? 1 : (number == -1 ? ((power & 1) ? -1 : 1) : 0);

	long long result = 1;
	while(power)
	{
		if(power & 1)
		{
			result *= number;
			power--;
		}
		number *= number;
		power >>= 1;
	}
	return result;
}

long long vmLoadLong(void* target)
{
	long long value;
	memcpy(&value, target, sizeof(long long));
	return value;
}

void vmStoreLong(void* target, long long value)
{
	memcpy(target, &value, sizeof(long long));
}

double vmLoadDouble(void* target)
{
	double value;
	memcpy(&value, target, sizeof(double));
	return value;
}

void vmStoreDouble(void* target, double value)
{
	memcpy(target, &value, sizeof(double));
}

char* vmLoadPointer(void* target)
{
	char* value;
	memcpy(&value, target, sizeof(char*));
	return value;
}

void vmStorePointer(void* target, char* value)
{
	memcpy(target, &value, sizeof(char*));
}

#if defined(_M_X64)
#ifdef _WIN64
	#include <Windows.h>
	#define NULLC_X64_IREGARGS 4
	#define NULLC_X64_FREGARGS 4
#else
	#include <sys/mman.h>
	#include <limits.h>
	#ifndef PAGESIZE
		#define PAGESIZE 4096
	#endif
	#define NULLC_X64_IREGARGS 6
	#define NULLC_X64_FREGARGS 8
#endif

static const unsigned char gatePrologue[] =
{
#ifdef _WIN64
	0x4C, 0x89, 0x44, 0x24, 0x18,	// mov qword [rsp+18h], r8	// spill r8
	0x48, 0x89, 0x54, 0x24, 0x10,	// mov qword [rsp+10h], rdx	// spill RDX
	0x48, 0x89, 0x4C, 0x24, 0x08,	// mov qword [rsp+8h], rcx	// spill RCX
#endif
	0x57,	// push RDI
	0x50,	// push RAX
	0x53,	// push RBX
	0x56,	// push RSI
	0x49, 0x54,	// push R12
#ifdef _WIN64
	0x48, 0x8b, 0301,// mov RAX, RCX
	0x48, 0x8b, 0332,// mov RBX, RDX
	0x4d, 0x8b, 0330,// mov R11, R8
#else
	0x48, 0x8b, 0307,// mov RAX, RDI
	0x48, 0x8b, 0336,// mov RBX, RSI
	0x4c, 0x8b, 0332,// mov R11, RDX
#endif
};

bool AreMembersAligned(ExternTypeInfo *lType, Linker *exLinker)
{
	bool aligned = 1;
	//printf("checking class %s: ", exLinker->exSymbols.data + lType->offsetToName);
	for(unsigned m = 0; m < lType->memberCount; m++)
	{
		ExternMemberInfo &member = exLinker->exTypeExtra[lType->memberOffset + m];
		ExternTypeInfo &memberType = exLinker->exTypes[member.type];
		unsigned pos = member.offset;

		//printf("member %s; ", exLinker->exSymbols.data + memberType.offsetToName);
		switch(memberType.type)
		{
		case ExternTypeInfo::TYPE_COMPLEX:
			break;
		case ExternTypeInfo::TYPE_VOID:
			break;
		case ExternTypeInfo::TYPE_INT:
			if(pos % 4 != 0)
				aligned = 0;
			break;
		case ExternTypeInfo::TYPE_FLOAT:
			break;
		case ExternTypeInfo::TYPE_LONG:
			if(pos % 8 != 0)
				aligned = 0;
			break;
		case ExternTypeInfo::TYPE_DOUBLE:
			break;
		case ExternTypeInfo::TYPE_SHORT:
			if(pos % 2 != 0)
				aligned = 0;
			break;
		case ExternTypeInfo::TYPE_CHAR:
			break;
		}
		pos += memberType.size;
	}
	//printf("%s\n", aligned ? "aligned" : "unaligned");
	return aligned;
}

bool HasIntegerMembersInRange(ExternTypeInfo &type, unsigned fromOffset, unsigned toOffset, Linker *linker)
{
	for(unsigned m = 0; m < type.memberCount; m++)
	{
		ExternMemberInfo &member = linker->exTypeExtra[type.memberOffset + m];

		ExternTypeInfo &memberType = linker->exTypes[member.type];

		if(memberType.type == ExternTypeInfo::TYPE_COMPLEX)
		{
			// Handle opaque types
			bool opaqueType = memberType.subCat != ExternTypeInfo::CAT_CLASS || memberType.memberCount == 0;

			if(opaqueType)
			{
				if(member.offset + memberType.size > fromOffset && member.offset < toOffset)
					return true;
			}else{
				if(HasIntegerMembersInRange(memberType, fromOffset - member.offset, toOffset - member.offset, linker))
					return true;
			}
		}else if(memberType.type != ExternTypeInfo::TYPE_FLOAT && memberType.type != ExternTypeInfo::TYPE_DOUBLE){
			if(member.offset + memberType.size > fromOffset && member.offset < toOffset)
				return true;
		}
	}

	return false;
}

unsigned int Executor::CreateFunctionGateway(FastVector<unsigned char>& code, unsigned int funcID)
{
	unsigned int dwordsToPop = (exFunctions[funcID].bytesToPop >> 2);
	unsigned int tmp;

	/*printf("Creating gateway for function %s(", exLinker->exSymbols.data + exFunctions[funcID].offsetToName);
	// Check every function local
	for(unsigned int i = 0; i < exFunctions[funcID].paramCount; i++)
	{
		// Get information about local
		ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + i];
		printf("%s, ", exLinker->exSymbols.data + exTypes[lInfo.type].offsetToName);
	}
	printf(") arg count %d\n", exFunctions[funcID].paramCount + (exFunctions[funcID].funcCat == ExternFuncInfo::NORMAL ? 0 : 1));*/

	// Create function call code, clear it out, copy call prologue to the beginning
	code.push_back(gatePrologue, sizeof(gatePrologue) / sizeof(gatePrologue[0]));

	// complex type return is handled by passing pointer to a place, where the return value will be placed. This uses first register.
	unsigned int rvs = exFunctions[funcID].retType == ExternFuncInfo::RETURN_UNKNOWN ? 1 : 0;

#ifndef _WIN64
	ExternTypeInfo &funcType = exTypes[exFunctions[funcID].funcType];
	unsigned returnTypeID = exLinker->exTypeExtra[funcType.memberOffset].type;
	ExternTypeInfo &returnType = exTypes[returnTypeID];

	if(rvs && returnType.size <= 16 && returnTypeID != NULLC_TYPE_AUTO_REF)
		rvs = 0;
	if(returnType.subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(&returnType, exLinker))
		rvs = 1;

	// in AMD64, int and FP registers are independant, so we have to count, how much arguments to save in registers
	unsigned int usedIRegs = rvs, usedFRegs = 0;

	// This two variables will help later to know we've reached the boundary of arguments that are placed in registers
	unsigned int argsToIReg = 0, argsToFReg = 0;

	bool onStack[32] = { false };

	for(unsigned int k = 0; k < exFunctions[funcID].paramCount + (exFunctions[funcID].funcCat == ExternFuncInfo::NORMAL ? 0 : 1); k++)
	{
		unsigned int typeID = NULLC_TYPE_LONG;
		ExternTypeInfo *lType = NULL;

		ExternTypeInfo::TypeCategory typeCat = ExternTypeInfo::TYPE_LONG;
		unsigned int typeSize = 8;

		bool firstQwordInteger = true;
		bool secondQwordInteger = false;

		if((unsigned int)k != exFunctions[funcID].paramCount)
		{
			ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + k];

			typeID = lInfo.type;
			lType = &exLinker->exTypes[typeID];
			typeCat = lType->type;
			typeSize = lType->size;

			bool opaqueType = lType->subCat != ExternTypeInfo::CAT_CLASS || lType->memberCount == 0;

			firstQwordInteger = opaqueType || HasIntegerMembersInRange(*lType, 0, 8, exLinker);
			secondQwordInteger = opaqueType || HasIntegerMembersInRange(*lType, 8, 16, exLinker);
		}

		int requredIRegs = (typeSize > 0 ? (firstQwordInteger ? 1 : 0) : 0) + (typeSize > 8 ? (secondQwordInteger ? 1 : 0) : 0);
		int requredFRegs = (typeSize > 0 ? (firstQwordInteger ? 0 : 1) : 0) + (typeSize > 8 ? (secondQwordInteger ? 0 : 1) : 0);

		switch(typeCat)
		{
		case ExternTypeInfo::TYPE_FLOAT:
		case ExternTypeInfo::TYPE_DOUBLE:
			if(usedFRegs < NULLC_X64_FREGARGS)
			{
				usedFRegs++;
				argsToFReg = k + 1;
			}
			break;
		case ExternTypeInfo::TYPE_CHAR:
		case ExternTypeInfo::TYPE_SHORT:
		case ExternTypeInfo::TYPE_INT:
		case ExternTypeInfo::TYPE_LONG:
			if(typeSize && usedIRegs < NULLC_X64_IREGARGS)
			{
				usedIRegs++;
				argsToIReg = k + 1;
				if(k < 32)
					onStack[k] = true;
			}
			break;
		case ExternTypeInfo::TYPE_COMPLEX:
			// Parameters larger than 16 bytes are sent through stack
			// Unaligned types such as auto ref type are sent through stack
			if(typeSize > 16 || typeID == NULLC_TYPE_AUTO_REF)
				break;

			// Check class structure to see if it has some unaligned fields
			if(lType->subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(lType, exLinker))
				break;

			// If the class if being divided in the middle, between registers and stack, sent through stack
			if(usedIRegs + requredIRegs > NULLC_X64_IREGARGS || usedFRegs + requredFRegs > NULLC_X64_FREGARGS)
				break;

			// Request registers
			usedIRegs += requredIRegs;
			usedFRegs += requredFRegs;

			// Mark this argument as being sent through stack
			if(requredIRegs)
				argsToIReg = k + 1;

			if(requredFRegs)
				argsToFReg = k + 1;

			if(k < 32)
				onStack[k] = true;
			break;
		default:
			assert(!"parameter type unsupported");
		}
	}
	//printf("Used Iregs: %d Fregs: %d\n", usedIRegs, usedFRegs);
	//printf("Arguments to Ireg limit: %d Freg limit: %d\n", argsToIReg, argsToFReg);
#endif
	
	// compute how much we are going to pop
	unsigned int needPopPrev = 0;
	int i = exFunctions[funcID].paramCount - (exFunctions[funcID].funcCat == ExternFuncInfo::NORMAL ? 1 : 0);
	for(; i >= 0; i--)
	{
		// By default, suppose we have last hidden argument, that is a pointer, represented as long number of size 8
		ExternTypeInfo::TypeCategory typeCat = ExternTypeInfo::TYPE_LONG;
		ExternTypeInfo *lType = NULL;
		unsigned int typeSize = 8;
		
		// If this is not the last argument, update data above
		if((unsigned int)i != exFunctions[funcID].paramCount)
		{
			ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + i];

			lType = &exLinker->exTypes[lInfo.type];
			typeCat = lType->type;
			typeSize = lType->size;
		}

		// Aggregate types are passed in registers
#ifdef _WIN64
		if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize != 0 && typeSize <= 4)
#else
		if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize <= 4)
#endif
			typeCat = ExternTypeInfo::TYPE_INT;

#ifdef _WIN64
		if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize == 8)
			typeCat = ExternTypeInfo::TYPE_LONG;
#endif

		switch(typeCat)
		{
		case ExternTypeInfo::TYPE_VOID:
			break;
		case ExternTypeInfo::TYPE_FLOAT:
		case ExternTypeInfo::TYPE_DOUBLE:
#ifdef _WIN64
			if(rvs + i > NULLC_X64_FREGARGS - 1)
#else
			if((unsigned)i >= argsToFReg)
#endif
				needPopPrev += 8;
			break;
		case ExternTypeInfo::TYPE_CHAR:
		case ExternTypeInfo::TYPE_SHORT:
		case ExternTypeInfo::TYPE_INT:
		case ExternTypeInfo::TYPE_LONG:
#ifdef _WIN64
			if(rvs + i > NULLC_X64_IREGARGS - 1)
#else
			if((unsigned)i >= argsToIReg || !typeSize)
#endif
				needPopPrev += 8;
			break;
		case ExternTypeInfo::TYPE_COMPLEX:
#ifdef _WIN64
			if(rvs + i > NULLC_X64_IREGARGS - 1)
				needPopPrev += 8;
#else
			if(!(typeSize <= 16 && exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + i].type != 7 &&
				!(lType->subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(lType, exLinker)) && i < 32 && onStack[i]))
				needPopPrev += (typeSize + 7) & ~7;
#endif
			break;
		}
	}

	// normal function doesn't accept context, so start of the stack skips those 2 dwords
	unsigned int currentShift = (dwordsToPop - (exFunctions[funcID].funcCat == ExternFuncInfo::NORMAL ? 2 : 0)) * 4;
	unsigned int needpop = 0;

	if(needPopPrev % 16 != 0)
	{
		code.push_back(0x56); // push RSI for alignment
		needpop += 8;
		needPopPrev += 8;
	}

	i = exFunctions[funcID].paramCount - (exFunctions[funcID].funcCat == ExternFuncInfo::NORMAL ? 1 : 0);

	for(; i >= 0; i--)
	{
		// By default, suppose we have last hidden argument, that is a pointer, represented as long number of size 8
		unsigned int typeID = NULLC_TYPE_LONG;
		ExternTypeInfo *lType = NULL;

		ExternTypeInfo::TypeCategory typeCat = ExternTypeInfo::TYPE_LONG;
		unsigned int typeSize = 8;

		bool firstQwordInteger = true;
		bool secondQwordInteger = false;

		// If this is not the last argument, update data above
		if((unsigned int)i != exFunctions[funcID].paramCount)
		{
			ExternLocalInfo &lInfo = exLinker->exLocals[exFunctions[funcID].offsetToFirstLocal + i];

			typeID = lInfo.type;
			lType = &exLinker->exTypes[typeID];
			typeCat = lType->type;
			typeSize = lType->size;

			bool opaqueType = lType->subCat != ExternTypeInfo::CAT_CLASS || lType->memberCount == 0;

			firstQwordInteger = opaqueType || HasIntegerMembersInRange(*lType, 0, 8, exLinker);
			secondQwordInteger = opaqueType || HasIntegerMembersInRange(*lType, 8, 16, exLinker);

			// Check for aggregate types that are passed in registers
#ifdef _WIN64
			if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize != 0 && typeSize <= 4)
				typeCat = ExternTypeInfo::TYPE_INT;

			if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize == 8)
				typeCat = ExternTypeInfo::TYPE_LONG;
#else
			if(typeCat == ExternTypeInfo::TYPE_COMPLEX && typeSize == 0)
				typeCat = ExternTypeInfo::TYPE_INT;
#endif
		}

		switch(typeCat)
		{
		case ExternTypeInfo::TYPE_FLOAT:
		case ExternTypeInfo::TYPE_DOUBLE:
#ifdef _WIN64
			if(rvs + i > NULLC_X64_FREGARGS - 1)
#else
			if((unsigned)i >= argsToFReg)
#endif
			{
				// mov rsi, [rax+shift]
				if(typeCat == ExternTypeInfo::TYPE_DOUBLE)
					code.push_back(0x48);	// 64bit mode
				code.push_back(0x8b);
				code.push_back(0264);	// modR/M mod = 10 (32-bit shift), spare = 6 (RSI is destination), r/m = 4 (SIB active)
				code.push_back(0040);	// sib	scale = 0 (1), index = 4 (NONE), base = 0 (EAX)
				tmp = currentShift - (typeCat == ExternTypeInfo::TYPE_DOUBLE ? 8 : 4);
				code.push_back((unsigned char*)&tmp, 4);

				// push rsi
				code.push_back(0x56);
				needpop += 8;
			}else{
				// movsd/movss xmm, [rax+shift]
				code.push_back(typeCat == ExternTypeInfo::TYPE_DOUBLE ? 0xf2 : 0xf3);
				code.push_back(0x0f);
				code.push_back(0x10);
#ifdef _WIN64
				code.push_back((unsigned char)(0200 | (i << 3)));	// modR/M mod = 10 (32-bit shift), spare = XMMn, r/m = 0 (RAX is base)
#else
				assert(usedFRegs - 1 < NULLC_X64_FREGARGS);
				code.push_back((unsigned char)(0200 | ((usedFRegs - 1) << 3)));
				usedFRegs--;
#endif
				tmp = currentShift - (typeCat == ExternTypeInfo::TYPE_DOUBLE ? 8 : 4);
				code.push_back((unsigned char*)&tmp, 4);
			}
			currentShift -= (typeCat == ExternTypeInfo::TYPE_DOUBLE ? 8 : 4);
			break;
		case ExternTypeInfo::TYPE_CHAR:
		case ExternTypeInfo::TYPE_SHORT:
		case ExternTypeInfo::TYPE_INT:
		case ExternTypeInfo::TYPE_LONG:
#ifdef _WIN64
			if(rvs + i > NULLC_X64_IREGARGS - 1)
#else
			if((unsigned)i >= argsToIReg || !typeSize)
#endif
			{
				// mov r12, [rax+shift]
				if(typeCat == ExternTypeInfo::TYPE_LONG)
					code.push_back(0x4c);	// 64bit mode
				else
					code.push_back(0x44);
				code.push_back(0x8b);
				code.push_back(0244);	// modR/M mod = 10 (32-bit shift), spare = 4 (R12 is destination), r/m = 4 (SIB active)
				code.push_back(0040);	// sib	scale = 0 (1), index = 4 (NONE), base = 0 (EAX)
				tmp = currentShift - (typeCat == ExternTypeInfo::TYPE_LONG ? 8 : 4);
				code.push_back((unsigned char*)&tmp, 4);

				// push r12
				code.push_back(0x49);
				code.push_back(0x54);
				needpop += 8;
			}else{
				unsigned int regID = rvs + i;
#ifndef _WIN64
				assert(usedIRegs - 1 < NULLC_X64_IREGARGS);
				regID = (usedIRegs - 1);
				usedIRegs--;
#endif
				if(regID == (NULLC_X64_IREGARGS - 2) || regID == (NULLC_X64_IREGARGS - 1))
					code.push_back(typeCat == ExternTypeInfo::TYPE_LONG ? 0x4c : 0x44);
				else if(typeCat == ExternTypeInfo::TYPE_LONG)
					code.push_back(0x48);	// 64bit mode
				code.push_back(0x8b);
#ifdef _WIN64
				unsigned regCodes[] = { 0214, 0224, 0204, 0214 }; // rcx, rdx, r8, r9
#else
				unsigned regCodes[] = { 0274, 0264, 0224, 0214, 0204, 0214 }; // rdi, rsi, rdx, rcx, r8, r9
#endif
				code.push_back((unsigned char)regCodes[regID]);
				code.push_back(0040);
				tmp = currentShift - (typeCat == ExternTypeInfo::TYPE_LONG ? 8 : 4);
				code.push_back((unsigned char*)&tmp, 4);
			}
			currentShift -= (typeCat == ExternTypeInfo::TYPE_LONG ? 8 : 4);
			break;
		case ExternTypeInfo::TYPE_COMPLEX:
#ifdef _WIN64
			if(typeSize != 0 && typeSize <= 8)
				assert(!"parameter type unsupported (small aggregate)");
			// under windows, a pointer to a structure is passed
			// lea r12, [rax+shift]
			code.push_back(0x4c);	// 64bit mode
			code.push_back(0x8d);
			code.push_back(0244);	// modR/M mod = 10 (32-bit shift), spare = 4 (R12 destination), r/m = 4 (SIB active)
			code.push_back(0040);	// sib	scale = 0 (1), index = 4 (NONE), base = 0 (EAX)
			
			tmp = currentShift - typeSize;
			code.push_back((unsigned char*)&tmp, 4);

			if(rvs + i > NULLC_X64_IREGARGS - 1)
			{
				// push r12
				code.push_back(0x41);	// 64bit mode
				code.push_back(0x54);
				needpop += 8;
			}else{
				// mov reg, r12
				if(rvs + i == (NULLC_X64_IREGARGS - 2) || rvs + i == (NULLC_X64_IREGARGS - 1))
					code.push_back(0x4d);	// 64bit mode
				else
					code.push_back(0x49);	// 64bit mode
				code.push_back(0x8b);

				unsigned regCodes[] = { 0314, 0324, 0304, 0314 }; // rcx, rdx, r8, r9
				code.push_back((unsigned char)regCodes[rvs + i]);
			}
#else
			// under linux, structure is passed through registers or stack
			if(typeSize <= 16 && typeID != NULLC_TYPE_AUTO_REF &&
				!(lType->subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(lType, exLinker)) && i < 32 && onStack[i])
			{
				unsigned regCodes[] = { 0274, 0264, 0224, 0214, 0204, 0214 }; // rdi, rsi, rdx, rcx, r8, r9

				unsigned int regID = 0;
				if(typeSize > 8)
				{
					// pass high dword/qword
					if(secondQwordInteger)
					{
						assert(usedIRegs - 1 < NULLC_X64_IREGARGS);
						regID = (usedIRegs - 1);
						usedIRegs--;
						if(regID == (NULLC_X64_IREGARGS - 2) || regID == (NULLC_X64_IREGARGS - 1))
							code.push_back(typeSize == 16 ? 0x4c : 0x44);	// 64bit mode
						else if(typeSize == 16)
							code.push_back(0x48);	// 64bit mode
						code.push_back(0x8b);
						code.push_back((unsigned char)regCodes[regID]);
						code.push_back(0040);
						tmp = currentShift - (typeSize == 16 ? 8 : 4);
						code.push_back((unsigned char*)&tmp, 4);
					}else{
						// movsd/movss xmm, [rax+shift]
						code.push_back(typeSize == 16 ? 0xf2 : 0xf3);
						code.push_back(0x0f);
						code.push_back(0x10);
						assert(usedFRegs - 1 < NULLC_X64_FREGARGS);
						code.push_back((unsigned char)(0200 | ((usedFRegs - 1) << 3)));
						usedFRegs--;
						tmp = currentShift - (typeSize == 16 ? 8 : 4);
						code.push_back((unsigned char*)&tmp, 4);
					}
				}

				// pass low qword
				if(firstQwordInteger)
				{
					assert(usedIRegs - 1 < NULLC_X64_IREGARGS);
					regID = (usedIRegs - 1);
					usedIRegs--;
					if(regID == (NULLC_X64_IREGARGS - 2) || regID == (NULLC_X64_IREGARGS - 1))
						code.push_back(0x4c);	// 64bit mode
					else
						code.push_back(0x48);	// 64bit mode
					code.push_back(0x8b);
					code.push_back((unsigned char)regCodes[regID]);
					code.push_back(0040);
					tmp = currentShift - typeSize;
					code.push_back((unsigned char*)&tmp, 4);
				}else{
					// movsd/movss xmm, [rax+shift]
					code.push_back(typeSize >= 8 ? 0xf2 : 0xf3);
					code.push_back(0x0f);
					code.push_back(0x10);
					assert(usedFRegs - 1 < NULLC_X64_FREGARGS);
					code.push_back((unsigned char)(0200 | ((usedFRegs - 1) << 3)));
					usedFRegs--;
					tmp = currentShift - typeSize;
					code.push_back((unsigned char*)&tmp, 4);
				}
			}else{
				for(unsigned p = 0; p < (typeSize + 7) / 8; p++)
				{
					// mov r12, [rax+shift]
					if(p || typeSize % 8 == 0)
						code.push_back(0x4c);	// 64bit mode
					else
						code.push_back(0x44);
					code.push_back(0x8b);
					code.push_back(0244);	// modR/M mod = 10 (32-bit shift), spare = 4 (R12 is destination), r/m = 4 (SIB active)
					code.push_back(0040);	// sib	scale = 0 (1), index = 4 (NONE), base = 0 (EAX)
					tmp = currentShift - p * 8 - (typeSize % 8 == 0 ? 8 : 4);
					code.push_back((unsigned char*)&tmp, 4);

					// push rsi
					code.push_back(0x41);	// 64bit mode
					code.push_back(0x54);
					needpop += 8;
				}
			}
#endif
			currentShift -= typeSize == 0 ? 4 : typeSize;
			break;
		default:
			assert(!"parameter type unsupported");
		}
	}
	// for complex return value, pass a pointer in first register
	if(rvs)
	{
		code.push_back(0x48);	// 64bit mode
		code.push_back(0x8b);
#ifdef _WIN64
		code.push_back(0313);	// modR/M mod = 11 (register), spare = 1 (RCX is a destination), r/m = 3 (RBX is source)
#else
		code.push_back(0373);	// modR/M mod = 11 (register), spare = 7 (RDI is a destination), r/m = 3 (RBX is source)
#endif
	}
#ifdef _WIN64
	// sub rsp, 32
	code.push_back(0x48);
	code.push_back(0x81);
	code.push_back(0354);	// modR/M mod = 11 (register), spare = 5 (sub), r/m = 4 (RSP is changed)
	tmp = 32;
	code.push_back((unsigned char*)&tmp, 4);
#endif

	// call r11
	code.push_back(0x49);
	code.push_back(0xff);
	code.push_back(0323);	// modR/M mod = 11 (register), spare = 2, r/m = 3 (R11 is source)

#ifdef _WIN64
	tmp = 32 + needpop;
#else
	tmp = needpop;
#endif
	if(tmp)
	{
		// add rsp, 32 + needpop
		code.push_back(0x48);
		code.push_back(0x81);
		code.push_back(0304);	// modR/M mod = 11 (register), spare = 0 (add), r/m = 4 (RSP is changed)
		code.push_back((unsigned char*)&tmp, 4);
	}

	// handle return value
	ExternFuncInfo::ReturnType retType = (ExternFuncInfo::ReturnType)exFunctions[funcID].retType;
	unsigned int retTypeID = exLinker->exTypeExtra[exTypes[exFunctions[funcID].funcType].memberOffset].type;
	unsigned int retTypeSize = exTypes[retTypeID].size;

	if(retType == ExternFuncInfo::RETURN_UNKNOWN)
	{
		if(retTypeSize == 0)
			retType = ExternFuncInfo::RETURN_VOID;
#ifdef _WIN64
		else if(retTypeSize <= 4)
			retType = ExternFuncInfo::RETURN_INT;
		else if(retTypeSize == 8)
			retType = ExternFuncInfo::RETURN_LONG;
#endif
	}

	if(rvs)
		retType = ExternFuncInfo::RETURN_UNKNOWN;

	unsigned int returnShift = 0;
	switch(retType)
	{
	case ExternFuncInfo::RETURN_DOUBLE:
		if(retTypeID == NULLC_TYPE_FLOAT)
		{
			// cvtss2sd xmm0, xmm0
			code.push_back(0xf3);
			code.push_back(0x0f);
			code.push_back(0x5a);
			code.push_back(0xc0);
		}

		returnShift = 2;

		// movsd qword [rbx], xmm0
		code.push_back(0xf2);
		code.push_back(0x0f);
		code.push_back(0x11);
		code.push_back(0003);	// modR/M mod = 00 (no shift), spare = XMM0, r/m = 3 (RBX is base)
		break;
	case ExternFuncInfo::RETURN_LONG:
		returnShift = 1;

		// mov qword [rbx], rax
		code.push_back(0x48);	// 64bit mode
	case ExternFuncInfo::RETURN_INT:
		returnShift += 1;

		// mov dword [rbx], eax
		code.push_back(0x89);
		code.push_back(0003);	// modR/M mod = 00 (no shift), spare = 0 (RAX is source), r/m = 3 (RBX is base)
		break;
	case ExternFuncInfo::RETURN_VOID:
		break;
	case ExternFuncInfo::RETURN_UNKNOWN:
#ifndef _WIN64
		if(!rvs)
		{
			ExternTypeInfo &retType = exLinker->exTypes[retTypeID];

			bool opaqueType = retType.subCat != ExternTypeInfo::CAT_CLASS || retType.memberCount == 0;

			bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(retType, 0, 8, exLinker);
			bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(retType, 8, 16, exLinker);

			if(firstQwordInteger)
			{
				// mov qword [rbx], rax
				code.push_back(0x48);	// 64bit mode
				code.push_back(0x89);
				code.push_back(0003);	// modR/M mod = 00 (no shift), spare = 0 (RAX is source), r/m = 3 (RBX is base)
			}else{
				// movsd qword [rbx], xmm0
				code.push_back(0xf2);
				code.push_back(0x0f);
				code.push_back(0x11);
				code.push_back(0003);	// modR/M mod = 00 (no shift), spare = XMM0, r/m = 3 (RBX is base)
			}

			if(retTypeSize > 8)
			{
				if(secondQwordInteger)
				{
					// mov qword [rbx+8], rax/rdx
					code.push_back(0x48);	// 64bit mode
					code.push_back(0x89);
					if(firstQwordInteger)
						code.push_back(0123);	// modR/M mod = 01 (8bit shift), spare = 2 (RDX is source), r/m = 3 (RBX is base)
					else
						code.push_back(0103);	// modR/M mod = 01 (8bit shift), spare = 0 (RAX is source), r/m = 3 (RBX is base)
					code.push_back(8);
				}else{
					// movsd qword [rbx + 8], xmm0/xmm1
					code.push_back(0xf2);
					code.push_back(0x0f);
					code.push_back(0x11);
					if(firstQwordInteger)
						code.push_back(0103);	// modR/M mod = 01 (8bit shift), spare = XMM0, r/m = 3 (RBX is base)
					else
						code.push_back(0113);	// modR/M mod = 01 (8bit shift), spare = XMM1, r/m = 3 (RBX is base)
					code.push_back(8);
				}
			}
		}
#else
		if(retTypeSize <= 8)
			assert(!"return type unsupported (small aggregate)");
#endif
		returnShift = retTypeSize / 4;
		break;
	}

	code.push_back(0x49);
	code.push_back(0x5c);	// pop R12
	code.push_back(0x5e);	// pop RSI
	code.push_back(0x5b);	// pop RBX
	code.push_back(0x58);	// pop RAX
	code.push_back(0x5f);	// pop RDI
	code.push_back(0xc3);	// ret

	return returnShift;
}

#endif

Executor::Executor(Linker* linker): exLinker(linker), exTypes(linker->exTypes), exFunctions(linker->exFunctions), gateCode(4096), breakCode(128)
{
	genStackBase = NULL;
	genStackPtr = NULL;
	genStackTop = NULL;

	callContinue = true;

	paramBase = 0;
	currentFrame = 0;
	cmdBase = NULL;

	symbols = NULL;

	codeRunning = false;

	lastResultType = OTYPE_COMPLEX;
	lastResultInt = 0;
	lastResultLong = 0ll;
	lastResultDouble = 0.0;

	breakFunctionContext = NULL;
	breakFunction = NULL;

	dcCallVM = NULL;
}

Executor::~Executor()
{
	NULLC::dealloc(genStackBase);
	genStackBase = NULL;

#if defined(NULLC_USE_DYNCALL)
	if(dcCallVM)
		dcFree(dcCallVM);
	dcCallVM = NULL;
#endif
}

#define RUNTIME_ERROR(test, desc)	if(test){ fcallStack.push_back(cmdStream); cmdStream = NULL; strcpy(execError, desc); break; }

void Executor::InitExecution()
{
	if(!exLinker->exCode.size())
	{
		strcpy(execError, "ERROR: no code to run");
		return;
	}
	fcallStack.clear();
	NULLC_UNWRAP(funcIDStack.clear());

	CommonSetLinker(exLinker);

	genParams.reserve(4096);
	genParams.clear();
	genParams.resize((exLinker->globalVarSize + 0xf) & ~0xf);

	SetUnmanagableRange(genParams.data, genParams.max);

	execError[0] = 0;
	callContinue = true;

	// Add return after the last instruction to end execution of code with no return at the end
	exLinker->exCode.push_back(VMCmd(cmdReturn, bitRetError, 0, 1));

	// General stack
	if(!genStackBase)
	{
		genStackBase = (unsigned int*)NULLC::alloc(sizeof(unsigned int) * 8192 * 2);	// Should be enough, but can grow
		genStackTop = genStackBase + 8192 * 2;
	}
	genStackPtr = genStackTop - 1;

	paramBase = 0;

#if defined(NULLC_USE_DYNCALL)
	if(!dcCallVM)
	{
		dcCallVM = dcNewCallVM(4096);
		dcMode(dcCallVM, DC_CALL_C_DEFAULT);
	}
#endif

#if defined(_M_X64) && !defined(NULLC_USE_DYNCALL)
	// Generate gateway code for all functions
	gateCode.clear();
	for(unsigned int i = 0; i < exFunctions.size(); i++)
	{
		if(!exFunctions[i].funcType)
		{
			exFunctions[i].startInByteCode = 0;
			exFunctions[i].returnShift = 0;
		}else{
			exFunctions[i].startInByteCode = gateCode.size();
			exFunctions[i].returnShift = (unsigned char)CreateFunctionGateway(gateCode, i);
		}
	}
#ifdef _WIN64
	DWORD old;
	VirtualProtect(gateCode.data, gateCode.size(), PAGE_EXECUTE_READWRITE, &old);
#else
	char *alignedAddr = (char*)((intptr_t)((char*)gateCode.data + PAGESIZE - 1) & ~(PAGESIZE - 1)) - PAGESIZE;
	char *alignedEnd = (char*)((intptr_t)((char*)gateCode.data + gateCode.size() + PAGESIZE - 1) & ~(PAGESIZE - 1));

	mprotect(alignedAddr, alignedEnd - alignedAddr, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

#endif
}

void Executor::Run(unsigned int functionID, const char *arguments)
{
	if(!codeRunning || functionID == ~0u)
		InitExecution();
	codeRunning = true;

	asmOperType retType = (asmOperType)-1;

	cmdBase = &exLinker->exCode[0];
	VMCmd *cmdStream = &exLinker->exCode[exLinker->offsetToGlobalCode];

	// By default error is flagged, normal return will clear it
	bool	errorState = true;
	// We will know that return is global if call stack size is equal to current
	unsigned int	finalReturn = fcallStack.size();

	if(functionID != ~0u)
	{
		unsigned int funcPos = ~0u;
		funcPos = exFunctions[functionID].address;
		if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_VOID)
		{
			retType = OTYPE_COMPLEX;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_INT){
			retType = OTYPE_INT;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_DOUBLE){
			retType = OTYPE_DOUBLE;
		}else if(exFunctions[functionID].retType == ExternFuncInfo::RETURN_LONG){
			retType = OTYPE_LONG;
		}
		if(funcPos == ~0u)
		{
			// Copy all arguments
			memcpy(genStackPtr - (exFunctions[functionID].bytesToPop >> 2), arguments, exFunctions[functionID].bytesToPop);
			genStackPtr -= (exFunctions[functionID].bytesToPop >> 2);
			// Call function
			if(RunExternalFunction(functionID, 0))
				errorState = false;
			// This will disable NULLC code execution while leaving error check and result retrieval
			cmdStream = NULL;
		}else{
			cmdStream = &exLinker->exCode[funcPos];

			// Copy from argument buffer to next stack frame
			char* oldBase = genParams.data;
			unsigned int oldSize = genParams.max;

			unsigned int paramSize = exFunctions[functionID].bytesToPop;
			// Keep stack frames aligned to 16 byte boundary
			unsigned int alignOffset = (genParams.size() % 16 != 0) ? (16 - (genParams.size() % 16)) : 0;
			// Reserve new stack frame
			genParams.reserve(genParams.size() + alignOffset + paramSize);
			// Copy arguments to new stack frame
			memcpy((char*)(genParams.data + genParams.size() + alignOffset), arguments, paramSize);

			// Ensure that stack is resized, if needed
			if(genParams.size() + alignOffset + paramSize >= oldSize)
				ExtendParameterStack(oldBase, oldSize, cmdStream);
		}
	}else{
		// If global code is executed, reset all global variables
		assert(genParams.size() >= exLinker->globalVarSize);
		memset(genParams.data, 0, exLinker->globalVarSize);
	}

#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
	unsigned int insCallCount[255];
	memset(insCallCount, 0, 255*4);
	unsigned int insExecuted = 0;
#endif

	while(cmdStream)
	{
		const VMCmd cmd = *cmdStream;
		cmdStream++;

		#ifdef NULLC_VM_PROFILE_INSTRUCTIONS
			insCallCount[cmd.cmd]++;
			insExecuted++;
		#endif

		switch(cmd.cmd)
		{
		case cmdNop:
			if(cmd.flag == EXEC_BREAK_SIGNAL || cmd.flag == EXEC_BREAK_ONCE)
			{
				RUNTIME_ERROR(breakFunction == NULL, "ERROR: break function isn't set");
				unsigned int target = cmd.argument;
				fcallStack.push_back(cmdStream);
				RUNTIME_ERROR(cmdStream < cmdBase || cmdStream > exLinker->exCode.data + exLinker->exCode.size() + 1, "ERROR: break position is out of range");
				unsigned int instruction = unsigned(cmdStream - cmdBase - 1);
				unsigned int response = breakFunction(breakFunctionContext, instruction);
				fcallStack.pop_back();

				RUNTIME_ERROR(response == NULLC_BREAK_STOP, "ERROR: execution was stopped after breakpoint");

				// Step command - set breakpoint on the next instruction, if there is no breakpoint already
				if(response)
				{
					// Next instruction for step command
					VMCmd *nextCommand = cmdStream;
					// Step command - handle unconditional jump step
					if(breakCode[target].cmd == cmdJmp)
						nextCommand = cmdBase + breakCode[target].argument;
					// Step command - handle conditional "jump on false" step
					if(breakCode[target].cmd == cmdJmpZ && *genStackPtr == 0)
						nextCommand = cmdBase + breakCode[target].argument;
					// Step command - handle conditional "jump on true" step
					if(breakCode[target].cmd == cmdJmpNZ && *genStackPtr != 0)
						nextCommand = cmdBase + breakCode[target].argument;
					if(breakCode[target].cmd == cmdYield && breakCode[target].flag)
					{
						ExternFuncInfo::Upvalue *closurePtr = *((ExternFuncInfo::Upvalue**)(&genParams[breakCode[target].argument + paramBase]));
						unsigned int offset = *closurePtr->ptr;
						if(offset != 0)
							nextCommand = cmdBase + offset;
					}
					// Step command - handle "return" step
					if(breakCode[target].cmd == cmdReturn && fcallStack.size() != finalReturn)
						nextCommand = fcallStack.back();
					if(response == NULLC_BREAK_STEP_INTO && breakCode[target].cmd == cmdCall && exFunctions[breakCode[target].argument].address != -1)
						nextCommand = cmdBase + exFunctions[breakCode[target].argument].address;
					if(response == NULLC_BREAK_STEP_INTO && breakCode[target].cmd == cmdCallPtr && genStackPtr[breakCode[target].argument >> 2] && exFunctions[genStackPtr[breakCode[target].argument >> 2]].address != -1)
						nextCommand = cmdBase + exFunctions[genStackPtr[breakCode[target].argument >> 2]].address;

					if(response == NULLC_BREAK_STEP_OUT && fcallStack.size() != finalReturn)
						nextCommand = fcallStack.back();

					if(nextCommand->cmd != cmdNop)
					{
						unsigned int pos = breakCode.size();
						breakCode.push_back(*nextCommand);
						nextCommand->cmd = cmdNop;
						nextCommand->flag = EXEC_BREAK_ONCE;
						nextCommand->argument = pos;
					}
				}
				// This flag means that breakpoint works only once
				if(cmd.flag == EXEC_BREAK_ONCE)
				{
					cmdStream[-1] = breakCode[target];
					cmdStream--;
					break;
				}
				// Jump to external code
				cmdStream = &breakCode[target];
				break;
			}
			cmdStream = cmdBase + cmd.argument;
			break;
		case cmdPushChar:
			genStackPtr--;
			*genStackPtr = genParams[cmd.argument + (paramBase * cmd.flag)];
			break;
		case cmdPushShort:
			genStackPtr--;
			*genStackPtr = *((short*)(&genParams[cmd.argument + (paramBase * cmd.flag)]));
			break;
		case cmdPushInt:
			genStackPtr--;
			*genStackPtr = *((int*)(&genParams[cmd.argument + (paramBase * cmd.flag)]));
			break;
		case cmdPushFloat:
			genStackPtr -= 2;
			vmStoreDouble(genStackPtr, (double)*((float*)(&genParams[cmd.argument + (paramBase * cmd.flag)])));
			break;
		case cmdPushDorL:
			genStackPtr -= 2;
			vmStoreLong(genStackPtr, vmLoadLong(&genParams[cmd.argument + (paramBase * cmd.flag)]));
			break;
		case cmdPushCmplx:
		{
			int valind = cmd.argument + (paramBase * cmd.flag);
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				genStackPtr--;
				*genStackPtr = *((unsigned int*)(&genParams[valind + currShift]));
			}
		}
			break;

		case cmdPushCharStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			genStackPtr++;
			*genStackPtr = *(char*)(cmd.argument + vmLoadPointer(genStackPtr - 1));
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			*genStackPtr = *((char*)NULL + cmd.argument + *genStackPtr);
#endif
			break;
		case cmdPushShortStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			genStackPtr++;
			*genStackPtr = *(short*)(cmd.argument + vmLoadPointer(genStackPtr - 1));
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			*genStackPtr = *(short*)((char*)NULL + cmd.argument + *genStackPtr);
#endif
			break;
		case cmdPushIntStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			genStackPtr++;
			*genStackPtr = *(int*)(cmd.argument + vmLoadPointer(genStackPtr - 1));
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			*genStackPtr = *(int*)((char*)NULL + cmd.argument + *genStackPtr);
#endif
			break;
		case cmdPushFloatStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			vmStoreDouble(genStackPtr, (double)*(float*)(cmd.argument + vmLoadPointer(genStackPtr)));
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			genStackPtr--;
			vmStoreDouble(genStackPtr, (double)*(float*)((char*)NULL + cmd.argument + *(genStackPtr + 1)));
#endif
			break;
		case cmdPushDorLStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			vmStoreLong(genStackPtr, vmLoadLong(cmd.argument + vmLoadPointer(genStackPtr)));
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			genStackPtr--;
			vmStoreLong(genStackPtr, vmLoadLong((char*)NULL + cmd.argument + *(genStackPtr + 1)));
#endif
			break;
		case cmdPushCmplxStk:
		{
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			char *start = cmd.argument + vmLoadPointer(genStackPtr);
			genStackPtr += 2;
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			char *start = (char*)NULL + cmd.argument + *genStackPtr;
			genStackPtr++;
#endif
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				genStackPtr--;
				*genStackPtr = *(unsigned int*)(start + currShift);
			}
		}
			break;

		case cmdPushImmt:
			genStackPtr--;
			*genStackPtr = cmd.argument;
			break;

		case cmdMovChar:
			genParams[cmd.argument + (paramBase * cmd.flag)] = (unsigned char)(*genStackPtr);
			break;
		case cmdMovShort:
			*((unsigned short*)(&genParams[cmd.argument + (paramBase * cmd.flag)])) = (unsigned short)(*genStackPtr);
			break;
		case cmdMovInt:
			*((int*)(&genParams[cmd.argument + (paramBase * cmd.flag)])) = (int)(*genStackPtr);
			break;
		case cmdMovFloat:
			*((float*)(&genParams[cmd.argument + (paramBase * cmd.flag)])) = (float)vmLoadDouble(genStackPtr);
			break;
		case cmdMovDorL:
			vmStoreLong(&genParams[cmd.argument + (paramBase * cmd.flag)], vmLoadLong(genStackPtr));
			break;
		case cmdMovCmplx:
		{
			int valind = cmd.argument + (paramBase * cmd.flag);
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				*((unsigned int*)(&genParams[valind + currShift])) = *(genStackPtr+(currShift>>2));
			}
			assert(currShift == 0);
		}
			break;

		case cmdMovCharStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			genStackPtr += 2;
			*(cmd.argument + vmLoadPointer(genStackPtr - 2)) = (unsigned char)(*genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			genStackPtr++;
			*((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned char)(*genStackPtr);
#endif
			break;
		case cmdMovShortStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			genStackPtr += 2;
			*(unsigned short*)(cmd.argument + vmLoadPointer(genStackPtr - 2)) = (unsigned short)(*genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			genStackPtr++;
			*(unsigned short*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (unsigned short)(*genStackPtr);
#endif
			break;
		case cmdMovIntStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			genStackPtr += 2;
			*(int*)(cmd.argument + vmLoadPointer(genStackPtr - 2)) = (int)(*genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			genStackPtr++;
			*(int*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (int)(*genStackPtr);
#endif
			break;

		case cmdMovFloatStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			genStackPtr += 2;
			*(float*)(cmd.argument + vmLoadPointer(genStackPtr - 2)) = (float)vmLoadDouble(genStackPtr);
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			genStackPtr++;
			*(float*)((char*)NULL + cmd.argument + *(genStackPtr-1)) = (float)vmLoadDouble(genStackPtr);
#endif
			break;
		case cmdMovDorLStk:
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			genStackPtr += 2;
			vmStoreLong(cmd.argument + vmLoadPointer(genStackPtr - 2), vmLoadLong(genStackPtr));
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			genStackPtr++;
			vmStoreLong((char*)NULL + cmd.argument + *(genStackPtr-1), vmLoadLong(genStackPtr));
#endif
			break;
		case cmdMovCmplxStk:
		{
#ifdef _M_X64
			RUNTIME_ERROR(uintptr_t(vmLoadPointer(genStackPtr)) < 0x00010000, "ERROR: null pointer access");
			char *start = cmd.argument + vmLoadPointer(genStackPtr);
			genStackPtr += 2;
#else
			RUNTIME_ERROR(*genStackPtr < 0x00010000, "ERROR: null pointer access");
			char *start = (char*)NULL + cmd.argument + *genStackPtr;
			genStackPtr++;
#endif
			unsigned int currShift = cmd.helper;
			while(currShift >= 4)
			{
				currShift -= 4;
				*(unsigned int*)(start + currShift) = *(genStackPtr+(currShift>>2));
			}
			assert(currShift == 0);
		}
			break;

		case cmdPop:
			genStackPtr = (unsigned int*)((char*)(genStackPtr) + cmd.argument);

			break;

		case cmdDtoI:
			*(genStackPtr + 1) = int(vmLoadDouble(genStackPtr));
			genStackPtr++;
			break;
		case cmdDtoL:
			vmStoreLong(genStackPtr, (long long)vmLoadDouble(genStackPtr));
			break;
		case cmdDtoF:
			*((float*)(genStackPtr + 1)) = float(vmLoadDouble(genStackPtr));
			genStackPtr++;
			break;
		case cmdItoD:
			genStackPtr--;
			vmStoreDouble(genStackPtr, double(*(int*)(genStackPtr + 1)));
			break;
		case cmdLtoD:
			vmStoreDouble(genStackPtr, double(vmLoadLong(genStackPtr)));
			break;
		case cmdItoL:
			genStackPtr--;
			vmStoreLong(genStackPtr, (long long)(*(int*)(genStackPtr + 1)));
			break;
		case cmdLtoI:
			genStackPtr++;
			*genStackPtr = (int)vmLoadLong(genStackPtr - 1);
			break;

		case cmdIndex:
			RUNTIME_ERROR(*genStackPtr >= (unsigned int)cmd.argument, "ERROR: array index out of bounds");
			vmStorePointer(genStackPtr + 1, vmLoadPointer(genStackPtr + 1) + cmd.helper * (*genStackPtr));
			genStackPtr++;
			break;
		case cmdIndexStk:
#ifdef _M_X64
			RUNTIME_ERROR(*genStackPtr >= *(genStackPtr + 3), "ERROR: array index out of bounds");
			vmStorePointer(genStackPtr + 2, vmLoadPointer(genStackPtr + 1) + cmd.helper * (*genStackPtr));
			genStackPtr += 2;
#else
			RUNTIME_ERROR(*genStackPtr >= *(genStackPtr + 2), "ERROR: array index out of bounds");
			*(int*)(genStackPtr + 2) = *(genStackPtr + 1) + cmd.helper * (*genStackPtr);
			genStackPtr += 2;
#endif
			break;

		case cmdCopyDorL:
			genStackPtr -= 2;
			*genStackPtr = *(genStackPtr + 2);
			*(genStackPtr + 1) = *(genStackPtr + 3);
			break;
		case cmdCopyI:
			genStackPtr--;
			*genStackPtr = *(genStackPtr + 1);
			break;

		case cmdGetAddr:
#ifdef _M_X64
			genStackPtr -= 2;
			vmStorePointer(genStackPtr, cmd.argument + paramBase * cmd.helper + genParams.data);
#else
			genStackPtr--;
			*genStackPtr = cmd.argument + paramBase * cmd.helper + (int)(intptr_t)genParams.data;
#endif
			break;
		case cmdFuncAddr:
			break;

		case cmdSetRangeStk:
		{
			unsigned int count = cmd.argument;

#ifdef _M_X64
			char *start =  vmLoadPointer(genStackPtr);
			genStackPtr += 2;
#else
			char *start = vmLoadPointer(genStackPtr);
			genStackPtr++;
#endif

			for(unsigned int varNum = 0; varNum < count; varNum++)
			{
				switch(cmd.helper)
				{
				case DTYPE_DOUBLE:
					*((double*)start) = vmLoadDouble(genStackPtr);
					start += 8;
					break;
				case DTYPE_FLOAT:
					*((float*)start) = float(vmLoadDouble(genStackPtr));
					start += 4;
					break;
				case DTYPE_LONG:
					vmStoreLong(start, vmLoadLong(genStackPtr));
					start += 8;
					break;
				case DTYPE_INT:
					*((int*)start) = int(*genStackPtr);
					start += 4;
					break;
				case DTYPE_SHORT:
					*((short*)start) = short(*genStackPtr);
					start += 2;
					break;
				case DTYPE_CHAR:
					*start = char(*genStackPtr);
					start += 1;
					break;
				}
			}
		}
			break;

		case cmdJmp:
			cmdStream = cmdBase + cmd.argument;
			break;

		case cmdJmpZ:
			if(*genStackPtr == 0)
				cmdStream = cmdBase + cmd.argument;
			genStackPtr++;
			break;

		case cmdJmpNZ:
			if(*genStackPtr != 0)
				cmdStream = cmdBase + cmd.argument;
			genStackPtr++;
			break;

		case cmdCall:
		{
			RUNTIME_ERROR(genStackPtr <= genStackBase+8, "ERROR: stack overflow");
			unsigned int fAddress = exFunctions[cmd.argument].address;

			if(fAddress == EXTERNAL_FUNCTION)
			{
				fcallStack.push_back(cmdStream);
#ifdef NULLC_VM_CALL_STACK_UNWRAP
				funcIDStack.push_back(cmd.argument);
				if(!RunCallStackHelper(cmd.argument, 0, finalReturn))
#else
				if(!RunExternalFunction(cmd.argument, 0))
#endif
				{
					cmdStream = NULL;
				}else{
					cmdStream = fcallStack.back();
					fcallStack.pop_back();
					NULLC_UNWRAP(funcIDStack.pop_back());
				}
			}else{
				fcallStack.push_back(cmdStream);
				NULLC_UNWRAP(funcIDStack.push_back(cmd.argument));
				cmdStream = cmdBase + fAddress;

				char* oldBase = genParams.data;
				unsigned int oldSize = genParams.max;

				unsigned int paramSize = exFunctions[cmd.argument].bytesToPop;

				// Parameter stack is always aligned to 16 bytes
				assert(genParams.size() % 16 == 0);

				// Reserve place for new stack frame (cmdPushVTop will resize)
				genParams.reserve(genParams.size() + paramSize);

				// Copy function arguments to new stack frame
				memcpy((char*)(genParams.data + genParams.size()), genStackPtr, paramSize);

				// Pop arguments from stack
				genStackPtr += paramSize >> 2;

				// If parameter stack was reallocated
				if(genParams.size() + paramSize >= oldSize)
					ExtendParameterStack(oldBase, oldSize, cmdStream);
			}
		}
			break;

		case cmdCallPtr:
		{
			unsigned int paramSize = cmd.argument;
			unsigned int fID = genStackPtr[paramSize >> 2];
			RUNTIME_ERROR(fID == 0, "ERROR: invalid function pointer");
			unsigned int fAddress = exFunctions[fID].address;

			if(fAddress == EXTERNAL_FUNCTION)
			{
				fcallStack.push_back(cmdStream);
#ifdef NULLC_VM_CALL_STACK_UNWRAP
				funcIDStack.push_back(fID);
				if(!RunCallStackHelper(fID, 1, finalReturn))
#else
				if(!RunExternalFunction(fID, 1))
#endif
				{
					cmdStream = NULL;
				}else{
					cmdStream = fcallStack.back();
					fcallStack.pop_back();
					NULLC_UNWRAP(funcIDStack.pop_back());
				}
			}else{
				fcallStack.push_back(cmdStream);
				NULLC_UNWRAP(funcIDStack.push_back(fID));
				cmdStream = cmdBase + fAddress;

				char* oldBase = genParams.data;
				unsigned int oldSize = genParams.max;

				// Parameter stack is always aligned to 16 bytes
				assert(genParams.size() % 16 == 0);

				// Reserve place for new stack frame (cmdPushVTop will resize)
				genParams.reserve(genParams.size() + paramSize);

				// Copy function arguments to new stack frame
				memcpy((char*)(genParams.data + genParams.size()), genStackPtr, paramSize);

				// Pop arguments from stack
				genStackPtr += (paramSize >> 2) + 1;
				RUNTIME_ERROR(genStackPtr <= genStackBase + 8, "ERROR: stack overflow");

				// If parameter stack was reallocated
				if(genParams.size() + paramSize >= oldSize)
					ExtendParameterStack(oldBase, oldSize, cmdStream);
			}
		}
			break;

		case cmdReturn:
			if(cmd.flag & bitRetError)
			{
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
				codeRunning = false;
				errorState = !cmd.argument;
				if(errorState)
					strcpy(execError, "ERROR: function didn't return a value");
				break;
			}
			{
				unsigned int *retValue = genStackPtr;
				genStackPtr = (unsigned int*)((char*)(genStackPtr) + cmd.argument);

				genParams.shrink(paramBase);
				paramBase = *genStackPtr;
				genStackPtr++;

				genStackPtr = (unsigned int*)((char*)(genStackPtr) - cmd.argument);
				memmove(genStackPtr, retValue, cmd.argument);
			}
			if(fcallStack.size() == finalReturn)
			{
				if(int(retType) == -1)
					retType = (asmOperType)(int)cmd.flag;
				cmdStream = NULL;
				errorState = false;
				if(finalReturn == 0)
					codeRunning = false;
				break;
			}
			cmdStream = fcallStack.back();
			fcallStack.pop_back();
			NULLC_UNWRAP(funcIDStack.pop_back());
			break;

		case cmdPushVTop:
			genStackPtr--;
			*genStackPtr = paramBase;
			paramBase = genParams.size();
			assert(paramBase % 16 == 0);
			if(paramBase + cmd.argument >= genParams.max)
			{
				char* oldBase = genParams.data;
				unsigned int oldSize = genParams.max;

				genParams.resize(paramBase + cmd.argument);
				assert(cmd.helper <= cmd.argument);
				if(cmd.argument - cmd.helper)
					memset(genParams.data + paramBase + cmd.helper, 0, cmd.argument - cmd.helper);

				ExtendParameterStack(oldBase, oldSize, cmdStream);
			}else{
				genParams.resize(paramBase + cmd.argument);
				assert(cmd.helper <= cmd.argument);
				if(cmd.argument - cmd.helper)
					memset(genParams.data + paramBase + cmd.helper, 0, cmd.argument - cmd.helper);
			}
			break;

		case cmdAdd:
			*(int*)(genStackPtr + 1) += *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdSub:
			*(int*)(genStackPtr + 1) -= *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdMul:
			*(int*)(genStackPtr + 1) *= *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdDiv:
			if(*(int*)(genStackPtr))
			{
				*(int*)(genStackPtr + 1) /= *(int*)(genStackPtr);
			}else{
				strcpy(execError, "ERROR: integer division by zero");
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr++;
			break;
		case cmdPow:
			*(int*)(genStackPtr + 1) = vmIntPow(*(int*)(genStackPtr), *(int*)(genStackPtr + 1));
			genStackPtr++;
			break;
		case cmdMod:
			if(*(int*)(genStackPtr))
			{
				*(int*)(genStackPtr + 1) %= *(int*)(genStackPtr);
			}else{
				strcpy(execError, "ERROR: integer division by zero");
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr++;
			break;
		case cmdLess:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) < *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdGreater:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) > *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdLEqual:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) <= *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdGEqual:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) >= *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdEqual:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) == *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdNEqual:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) != *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdShl:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) << *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdShr:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) >> *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdBitAnd:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) & *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdBitOr:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) | *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdBitXor:
			*(int*)(genStackPtr + 1) = *(int*)(genStackPtr + 1) ^ *(int*)(genStackPtr);
			genStackPtr++;
			break;
		case cmdLogAnd:
		case cmdLogOr:
			break;
		case cmdLogXor:
			*(int*)(genStackPtr + 1) = !!(*(int*)(genStackPtr + 1)) ^ !!(*(int*)(genStackPtr));
			genStackPtr++;
			break;

		case cmdAddL:
			vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) + vmLoadLong(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdSubL:
			vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) - vmLoadLong(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdMulL:
			vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) * vmLoadLong(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdDivL:
			if(long long value = vmLoadLong(genStackPtr))
			{
				vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) / value);
			}
			else
			{
				strcpy(execError, "ERROR: integer division by zero");
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr += 2;
			break;
		case cmdPowL:
			vmStoreLong(genStackPtr + 2, vmLongPow(vmLoadLong(genStackPtr), vmLoadLong(genStackPtr + 2)));
			genStackPtr += 2;
			break;
		case cmdModL:
			if(long long value = vmLoadLong(genStackPtr))
			{
				vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) % value);
			}else{
				strcpy(execError, "ERROR: integer division by zero");
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr += 2;
			break;
		case cmdLessL:
			*(int*)(genStackPtr + 3) = vmLoadLong(genStackPtr + 2) < vmLoadLong(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdGreaterL:
			*(int*)(genStackPtr + 3) = vmLoadLong(genStackPtr + 2) > vmLoadLong(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdLEqualL:
			*(int*)(genStackPtr + 3) = vmLoadLong(genStackPtr + 2) <= vmLoadLong(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdGEqualL:
			*(int*)(genStackPtr + 3) = vmLoadLong(genStackPtr + 2) >= vmLoadLong(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdEqualL:
			*(int*)(genStackPtr + 3) = vmLoadLong(genStackPtr + 2) == vmLoadLong(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdNEqualL:
			*(int*)(genStackPtr + 3) = vmLoadLong(genStackPtr + 2) != vmLoadLong(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdShlL:
			vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) << vmLoadLong(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdShrL:
			vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) >> vmLoadLong(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdBitAndL:
			vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) & vmLoadLong(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdBitOrL:
			vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) | vmLoadLong(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdBitXorL:
			vmStoreLong(genStackPtr + 2, vmLoadLong(genStackPtr + 2) ^ vmLoadLong(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdLogAndL:
		case cmdLogOrL:
			break;
		case cmdLogXorL:
			*(int*)(genStackPtr + 3) = !!vmLoadLong(genStackPtr + 2) ^ !!vmLoadLong(genStackPtr);
			genStackPtr += 3;
			break;

		case cmdAddD:
			vmStoreDouble(genStackPtr + 2, vmLoadDouble(genStackPtr + 2) + vmLoadDouble(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdSubD:
			vmStoreDouble(genStackPtr + 2, vmLoadDouble(genStackPtr + 2) - vmLoadDouble(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdMulD:
			vmStoreDouble(genStackPtr + 2, vmLoadDouble(genStackPtr + 2) * vmLoadDouble(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdDivD:
			vmStoreDouble(genStackPtr + 2, vmLoadDouble(genStackPtr + 2) / vmLoadDouble(genStackPtr));
			genStackPtr += 2;
			break;
		case cmdPowD:
			vmStoreDouble(genStackPtr + 2, pow(vmLoadDouble(genStackPtr + 2), vmLoadDouble(genStackPtr)));
			genStackPtr += 2;
			break;
		case cmdModD:
			vmStoreDouble(genStackPtr + 2, fmod(vmLoadDouble(genStackPtr + 2), vmLoadDouble(genStackPtr)));
			genStackPtr += 2;
			break;
		case cmdLessD:
			*(int*)(genStackPtr + 3) = vmLoadDouble(genStackPtr + 2) < vmLoadDouble(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdGreaterD:
			*(int*)(genStackPtr + 3) = vmLoadDouble(genStackPtr + 2) > vmLoadDouble(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdLEqualD:
			*(int*)(genStackPtr + 3) = vmLoadDouble(genStackPtr + 2) <= vmLoadDouble(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdGEqualD:
			*(int*)(genStackPtr + 3) = vmLoadDouble(genStackPtr + 2) >= vmLoadDouble(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdEqualD:
			*(int*)(genStackPtr + 3) = vmLoadDouble(genStackPtr + 2) == vmLoadDouble(genStackPtr);
			genStackPtr += 3;
			break;
		case cmdNEqualD:
			*(int*)(genStackPtr + 3) = vmLoadDouble(genStackPtr + 2) != vmLoadDouble(genStackPtr);
			genStackPtr += 3;
			break;

		case cmdNeg:
			*(int*)(genStackPtr) = -*(int*)(genStackPtr);
			break;
		case cmdNegL:
			vmStoreLong(genStackPtr, -vmLoadLong(genStackPtr));
			break;
		case cmdNegD:
			vmStoreDouble(genStackPtr, -vmLoadDouble(genStackPtr));
			break;

		case cmdBitNot:
			*(int*)(genStackPtr) = ~*(int*)(genStackPtr);
			break;
		case cmdBitNotL:
			vmStoreLong(genStackPtr, ~vmLoadLong(genStackPtr));
			break;

		case cmdLogNot:
			*(int*)(genStackPtr) = !*(int*)(genStackPtr);
			break;
		case cmdLogNotL:
			*(int*)(genStackPtr + 1) = !vmLoadLong(genStackPtr);
			genStackPtr++;
			break;
		
		case cmdIncI:
			(*(int*)(genStackPtr))++;
			break;
		case cmdIncD:
			vmStoreDouble(genStackPtr, vmLoadDouble(genStackPtr) + 1.0);
			break;
		case cmdIncL:
			vmStoreLong(genStackPtr, vmLoadLong(genStackPtr) + 1);
			break;

		case cmdDecI:
			(*(int*)(genStackPtr))--;
			break;
		case cmdDecD:
			vmStoreDouble(genStackPtr, vmLoadDouble(genStackPtr) - 1.0);
			break;
		case cmdDecL:
			vmStoreLong(genStackPtr, vmLoadLong(genStackPtr) - 1);
			break;

		case cmdConvertPtr:
			if(!ConvertFromAutoRef(cmd.argument, *genStackPtr))
			{
				NULLC::SafeSprintf(execError, 1024, "ERROR: cannot convert from %s ref to %s ref", &exLinker->exSymbols[exLinker->exTypes[*genStackPtr].offsetToName], &exLinker->exSymbols[exLinker->exTypes[cmd.argument].offsetToName]);
				fcallStack.push_back(cmdStream); 
				cmdStream = NULL;
			}
			genStackPtr++;
			break;
#ifdef _M_X64
		case cmdPushPtrImmt:
			genStackPtr--;
			*genStackPtr = cmd.argument;
			genStackPtr--;
			*genStackPtr = cmd.argument;
			break;
#endif
		case cmdYield:
			// If flag is set, jump to saved offset
			if(cmd.flag)
			{
				ExternFuncInfo::Upvalue *closurePtr = *((ExternFuncInfo::Upvalue**)(&genParams[cmd.argument + paramBase]));
				RUNTIME_ERROR(closurePtr == NULL, "ERROR: null pointer access");
				unsigned int offset = *closurePtr->ptr;
				if(offset != 0)
					cmdStream = cmdBase + offset;
			}else{
				ExternFuncInfo::Upvalue *closurePtr = *((ExternFuncInfo::Upvalue**)(&genParams[cmd.argument + paramBase]));
				RUNTIME_ERROR(closurePtr == NULL, "ERROR: null pointer access");
				// If helper is set, it's yield
				if(cmd.helper)
				{
					// When function is called again, it will continue from instruction after value return.
					*closurePtr->ptr = int(cmdStream - cmdBase) + 1;
				}else{	// otherwise, it's return
					// When function is called again, start from beginning
					*closurePtr->ptr = 0;
				}
			}
			break;
		case cmdCheckedRet:
			if(vmLoadPointer(genStackPtr) >= &genParams[paramBase] && vmLoadPointer(genStackPtr) <= genParams.data + genParams.size())
			{
				ExternTypeInfo &type = exLinker->exTypes[cmd.argument];
				if(type.arrSize == ~0u)
				{
					unsigned length = *(int*)(((char*)genStackPtr) + sizeof(void*));
					char *copy = (char*)NULLC::AllocObject(exLinker->exTypes[type.subType].size * length);
					memcpy(copy, vmLoadPointer(genStackPtr), unsigned(exLinker->exTypes[type.subType].size * length));
					vmStorePointer(genStackPtr, copy);
				}else{
					unsigned int objSize = type.size;
					char *copy = (char*)NULLC::AllocObject(objSize);
					memcpy(copy, vmLoadPointer(genStackPtr), objSize);
					vmStorePointer(genStackPtr, copy);
				}
			}
			break;
		}
	}
	// If there was an execution error
	if(errorState)
	{
		// Print call stack on error, when we get to the first function
		if(!finalReturn)
		{
			char *currPos = execError + strlen(execError);
			currPos += NULLC::SafeSprintf(currPos, ERROR_BUFFER_SIZE - int(currPos - execError), "\r\nCall stack:\r\n");

			BeginCallStack();
			while(unsigned int address = GetNextAddress())
				currPos += PrintStackFrame(address, currPos, ERROR_BUFFER_SIZE - int(currPos - execError), false);
		}
		// Ascertain that execution stops when there is a chain of nullcRunFunction
		callContinue = false;
		codeRunning = false;
		return;
	}
	
	lastResultType = retType;
	lastResultInt = *(int*)genStackPtr;
	if(genStackTop - genStackPtr > 1)
	{
		lastResultLong = vmLoadLong(genStackPtr);
		lastResultDouble = vmLoadDouble(genStackPtr);
	}

	switch(retType)
	{
	case OTYPE_DOUBLE:
	case OTYPE_LONG:
		genStackPtr += 2;
		break;
	case OTYPE_INT:
		genStackPtr++;
		break;
	case OTYPE_COMPLEX:
		break;
	}
	// If the call was started from an internal function call, a value pushed on stack for correct global return is still on stack
	if(!codeRunning && functionID != ~0u)
		genStackPtr++;
}

void Executor::Stop(const char* error)
{
	codeRunning = false;

	callContinue = false;
	NULLC::SafeSprintf(execError, ERROR_BUFFER_SIZE, "%s", error);
}

#ifdef NULLC_VM_CALL_STACK_UNWRAP
bool Executor::RunCallStackHelper(unsigned funcID, unsigned extraPopDW, unsigned callStackPos)
{
	const char *fName = exLinker->exSymbols.data + exFunctions[funcIDStack[callStackPos]].offsetToName;
	(void)fName;
	if(callStackPos + 1 < funcIDStack.size())
	{
		return RunCallStackHelper(funcID, extraPopDW, callStackPos + 1);
	}else{
		return RunExternalFunction(funcID, extraPopDW);
	}
}
#endif

#if !defined(_M_X64) && !defined(NULLC_USE_DYNCALL)

// X86 implementation
bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	unsigned int dwordsToPop = (exFunctions[funcID].bytesToPop >> 2);

	void* fPtr = exFunctions[funcID].funcPtr;
	unsigned int retType = exFunctions[funcID].retType;

	unsigned int *stackStart = (genStackPtr + dwordsToPop - 1);
	for(unsigned int i = 0; i < dwordsToPop; i++)
	{
#ifdef __GNUC__
		asm("movl %0, %%eax"::"r"(stackStart):"%eax");
		asm("pushl (%eax)");
#else
	__asm{ mov eax, dword ptr[stackStart] }
	__asm{ push dword ptr[eax] }
#endif
		stackStart--;
	}
	unsigned int *newStackPtr = genStackPtr + dwordsToPop + extraPopDW;

	switch(retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		((void (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_INT:
		newStackPtr -= 1;
		*newStackPtr = ((int (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		newStackPtr -= 2;
		*(double*)newStackPtr = ((double (*)())fPtr)();
		break;
	case ExternFuncInfo::RETURN_LONG:
		newStackPtr -= 2;
		*(long long*)newStackPtr = ((long long (*)())fPtr)();
		break;
#ifdef NULLC_COMPLEX_RETURN
	case ExternFuncInfo::RETURN_UNKNOWN:
	{
		unsigned int ret[128];
		unsigned int *ptr_ = &ret[0];

#ifdef __GNUC__
		asm("movl %0, %%eax"::"r"(ptr_):"%eax");
		asm("pushl %eax");
#else
		__asm mov eax, ptr_
		__asm push eax
#endif
		((void (*)())fPtr)();
		
#ifdef _MSC_VER
		__asm add esp, 4
#endif

		// adjust new stack top
		newStackPtr -= exFunctions[funcID].returnShift;
		// copy return value on top of the stack
		memcpy(newStackPtr, ret, exFunctions[funcID].returnShift * 4);
	}
		break;
#endif
	}
	genStackPtr = newStackPtr;
#ifdef __GNUC__
	asm("movl %0, %%eax"::"r"(dwordsToPop):"%eax");
	asm("leal (%esp, %eax, 0x4), %esp");
#else
	__asm mov eax, dwordsToPop
	__asm lea esp, [eax * 4 + esp]
#endif
	return callContinue;
}

#elif defined(_M_X64) && !defined(NULLC_USE_DYNCALL)

// X64 implementation
bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	unsigned int dwordsToPop = (exFunctions[funcID].bytesToPop >> 2);
	void* fPtr = exFunctions[funcID].funcPtr;

	assert(fPtr);

	unsigned int *stackStart = genStackPtr;
	unsigned int *newStackPtr = genStackPtr + dwordsToPop + extraPopDW;

	// buffer for return value
	unsigned int ret[128];
	unsigned int returnShift = exFunctions[funcID].returnShift;

	assert(exFunctions[funcID].returnShift < 128);	// maximum supported return type size is 512 bytes

	// call external function through gateway
	void (*gate)(unsigned int*, unsigned int*, void*) = (void (*)(unsigned int*, unsigned int*, void*))(void*)(gateCode.data + exFunctions[funcID].startInByteCode);
	gate(stackStart, ret, fPtr);

	// adjust new stack top
	newStackPtr -= returnShift;

	// copy return value on top of the stack
	memcpy(newStackPtr, ret, returnShift * 4);

	// set new stack top
	genStackPtr = newStackPtr;

	return callContinue;
}

#elif defined(NULLC_USE_DYNCALL)

DCcomplexdd dcCallComplexDD(DCCallVM* vm, DCpointer funcptr);
DCcomplexdl dcCallComplexDL(DCCallVM* vm, DCpointer funcptr);
DCcomplexld dcCallComplexLD(DCCallVM* vm, DCpointer funcptr);
DCcomplexll dcCallComplexLL(DCCallVM* vm, DCpointer funcptr);

void dcArgStack(DCCallVM* in_self, void *ptr, unsigned size);

int dcFreeIRegs(DCCallVM* in_self);
int dcFreeFRegs(DCCallVM* in_self);

bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	ExternFuncInfo &func = exFunctions[funcID];

	unsigned int dwordsToPop = (func.bytesToPop >> 2);

	void* fPtr = func.funcPtr;
	unsigned int retType = func.retType;

	unsigned int *stackStart = genStackPtr;
	unsigned int *newStackPtr = genStackPtr + dwordsToPop + extraPopDW;

	dcReset(dcCallVM);

#if defined(_WIN64)
	bool returnByPointer = func.returnShift > 1;
#elif !defined(_M_X64)
	bool returnByPointer = true;
#else
	ExternTypeInfo &funcType = exTypes[func.funcType];

	ExternMemberInfo &member = exLinker->exTypeExtra[funcType.memberOffset];
	ExternTypeInfo &returnType = exLinker->exTypes[member.type];

	bool returnByPointer = func.returnShift > 4 || member.type == NULLC_TYPE_AUTO_REF || (returnType.subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(&returnType, exLinker));

	bool opaqueType = returnType.subCat != ExternTypeInfo::CAT_CLASS || returnType.memberCount == 0;

	bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 0, 8, exLinker);
	bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(returnType, 8, 16, exLinker);
#endif

	unsigned int ret[128];

	if(retType == ExternFuncInfo::RETURN_UNKNOWN && returnByPointer)
		dcArgPointer(dcCallVM, ret);

	for(unsigned int i = 0; i < func.paramCount; i++)
	{
		// Get information about local
		ExternLocalInfo &lInfo = exLinker->exLocals[func.offsetToFirstLocal + i];

		ExternTypeInfo &tInfo = exTypes[lInfo.type];

		switch(tInfo.type)
		{
		case ExternTypeInfo::TYPE_COMPLEX:
#if defined(_WIN64)
			if(tInfo.size <= 4)
			{
				// This branch also handles 0 byte structs
				dcArgInt(dcCallVM, *(int*)stackStart);
				stackStart += 1;
			}else if(tInfo.size <= 8){
				dcArgLongLong(dcCallVM, *(long long*)stackStart);
				stackStart += 2;
			}else{
				dcArgPointer(dcCallVM, stackStart);
				stackStart += tInfo.size / 4;
			}
#elif defined(_M_X64)
			if(tInfo.size > 16 || lInfo.type == NULLC_TYPE_AUTO_REF || (tInfo.subCat == ExternTypeInfo::CAT_CLASS && !AreMembersAligned(&tInfo, exLinker)))
			{
				dcArgStack(dcCallVM, stackStart, (tInfo.size + 7) & ~7);
				stackStart += tInfo.size / 4;
			}else{
				bool opaqueType = tInfo.subCat != ExternTypeInfo::CAT_CLASS || tInfo.memberCount == 0;

				bool firstQwordInteger = opaqueType || HasIntegerMembersInRange(tInfo, 0, 8, exLinker);
				bool secondQwordInteger = opaqueType || HasIntegerMembersInRange(tInfo, 8, 16, exLinker);

				if(tInfo.size <= 4)
				{
					if(tInfo.size != 0)
					{
						if(firstQwordInteger)
							dcArgInt(dcCallVM, *(int*)stackStart);
						else
							dcArgFloat(dcCallVM, *(float*)stackStart);
					}else{
						stackStart += 1;
					}
				}else if(tInfo.size <= 8){
					if(firstQwordInteger)
						dcArgLongLong(dcCallVM, *(long long*)stackStart);
					else
						dcArgDouble(dcCallVM, *(double*)stackStart);
				}else{
					int requredIRegs = (firstQwordInteger ? 1 : 0) + (secondQwordInteger ? 1 : 0);

					if(dcFreeIRegs(dcCallVM) < requredIRegs || dcFreeFRegs(dcCallVM) < (2 - requredIRegs))
					{
						dcArgStack(dcCallVM, stackStart, (tInfo.size + 7) & ~7);
					}else{
						if(firstQwordInteger)
							dcArgLongLong(dcCallVM, *(long long*)stackStart);
						else
							dcArgDouble(dcCallVM, *(double*)stackStart);

						if(secondQwordInteger)
							dcArgLongLong(dcCallVM, *(long long*)(stackStart + 2));
						else
							dcArgDouble(dcCallVM, *(double*)(stackStart + 2));
					}
				}
				
				stackStart += tInfo.size / 4;
			}
#else
			if(tInfo.size <= 4)
			{
				// This branch also handles 0 byte structs
				dcArgInt(dcCallVM, *(int*)stackStart);
				stackStart += 1;
			}else{
				for(unsigned int k = 0; k < tInfo.size / 4; k++)
				{
					dcArgInt(dcCallVM, *(int*)stackStart);
					stackStart += 1;
				}
			}
#endif
			break;
		case ExternTypeInfo::TYPE_VOID:
			return false;
		case ExternTypeInfo::TYPE_INT:
			dcArgInt(dcCallVM, *(int*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_FLOAT:
			dcArgFloat(dcCallVM, *(float*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_LONG:
			dcArgLongLong(dcCallVM, *(long long*)stackStart);
			stackStart += 2;
			break;
		case ExternTypeInfo::TYPE_DOUBLE:
			dcArgDouble(dcCallVM, *(double*)stackStart);
			stackStart += 2;
			break;
		case ExternTypeInfo::TYPE_SHORT:
			dcArgShort(dcCallVM, *(short*)stackStart);
			stackStart += 1;
			break;
		case ExternTypeInfo::TYPE_CHAR:
			dcArgChar(dcCallVM, *(char*)stackStart);
			stackStart += 1;
			break;
		}
	}

	dcArgPointer(dcCallVM, *(DCpointer*)stackStart);

	switch(retType)
	{
	case ExternFuncInfo::RETURN_VOID:
		dcCallVoid(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_INT:
		newStackPtr -= 1;
		*newStackPtr = dcCallInt(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_DOUBLE:
		newStackPtr -= 2;
		if(func.returnShift == 1)
			*(double*)newStackPtr = dcCallFloat(dcCallVM, fPtr);
		else
			*(double*)newStackPtr = dcCallDouble(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_LONG:
		newStackPtr -= 2;
		*(long long*)newStackPtr = dcCallLongLong(dcCallVM, fPtr);
		break;
	case ExternFuncInfo::RETURN_UNKNOWN:
#if defined(_WIN64)
		if(func.returnShift == 1)
		{
			newStackPtr -= 1;
			*newStackPtr = dcCallInt(dcCallVM, fPtr);
		}else{
			dcCallVoid(dcCallVM, fPtr);

			newStackPtr -= exFunctions[funcID].returnShift;
			// copy return value on top of the stack
			memcpy(newStackPtr, ret, exFunctions[funcID].returnShift * 4);
		}
#elif !defined(_M_X64)
		dcCallPointer(dcCallVM, fPtr);

		newStackPtr -= exFunctions[funcID].returnShift;
		// copy return value on top of the stack
		memcpy(newStackPtr, ret, exFunctions[funcID].returnShift * 4);
#else
		if(returnByPointer)
		{
			dcCallPointer(dcCallVM, fPtr);

			newStackPtr -= exFunctions[funcID].returnShift;
			// copy return value on top of the stack
			memcpy(newStackPtr, ret, exFunctions[funcID].returnShift * 4);
		}else{
			newStackPtr -= exFunctions[funcID].returnShift;

			if(!firstQwordInteger && !secondQwordInteger)
			{
				DCcomplexdd res = dcCallComplexDD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, exFunctions[funcID].returnShift * 4); // copy return value on top of the stack
			}else if(firstQwordInteger && !secondQwordInteger){
				DCcomplexld res = dcCallComplexLD(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, exFunctions[funcID].returnShift * 4); // copy return value on top of the stack
			}else if(!firstQwordInteger && secondQwordInteger){
				DCcomplexdl res = dcCallComplexDL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, exFunctions[funcID].returnShift * 4); // copy return value on top of the stack
			}else{
				DCcomplexll res = dcCallComplexLL(dcCallVM, fPtr);

				memcpy(newStackPtr, &res, exFunctions[funcID].returnShift * 4); // copy return value on top of the stack
			}
		}
#endif
		break;
	}
	genStackPtr = newStackPtr;

	return callContinue;
}
#else

bool Executor::RunExternalFunction(unsigned int funcID, unsigned int extraPopDW)
{
	strcpy(execError, "ERROR: external function call failed");
	return false;
}

#endif

namespace ExPriv
{
	char *oldBase;
	char *newBase;
	unsigned int oldSize;
	unsigned int newSize;
	unsigned int objectName = NULLC::GetStringHash("auto ref");
	unsigned int autoArrayName = NULLC::GetStringHash("auto[]");
}

#define RELOCATE_DEBUG_PRINT(...) (void)0
//#define RELOCATE_DEBUG_PRINT printf

void Executor::FixupPointer(char* ptr, const ExternTypeInfo& type, bool takeSubType)
{
	char *target = vmLoadPointer(ptr);

	if(target > (char*)0x00010000)
	{
		if(target >= ExPriv::oldBase && target < (ExPriv::oldBase + ExPriv::oldSize))
		{
			RELOCATE_DEBUG_PRINT("\tFixing from %p to %p\r\n", ptr, ptr - ExPriv::oldBase + ExPriv::newBase);

			vmStorePointer(ptr, target - ExPriv::oldBase + ExPriv::newBase);
		}
		else if(target >= ExPriv::newBase && target < (ExPriv::newBase + ExPriv::newSize))
		{
			const ExternTypeInfo &subType = takeSubType ? exTypes[type.subType] : type;
			(void)subType;
			RELOCATE_DEBUG_PRINT("\tStack%s pointer %s %p (at %p)\r\n", type.subType == 0 ? " opaque" : "", symbols + subType.offsetToName, target, ptr);
		}
		else
		{
			const ExternTypeInfo &subType = takeSubType ? exTypes[type.subType] : type;
			RELOCATE_DEBUG_PRINT("\tGlobal%s pointer %s %p (at %p) base %p\r\n", type.subType == 0 ? " opaque" : "", symbols + subType.offsetToName, target, ptr, NULLC::GetBasePointer(target));

			if(type.subType != 0 && NULLC::IsBasePointer(target))
			{
				markerType *marker = (markerType*)((char*)target - sizeof(markerType));
				RELOCATE_DEBUG_PRINT("\tMarker is %d", *marker);

				const uintptr_t OBJECT_VISIBLE		= 1 << 0;
				const uintptr_t OBJECT_FREED		= 1 << 1;
				const uintptr_t OBJECT_FINALIZABLE	= 1 << 2;
				const uintptr_t OBJECT_FINALIZED	= 1 << 3;
				const uintptr_t OBJECT_ARRAY		= 1 << 4;

				if(*marker & OBJECT_VISIBLE)
					RELOCATE_DEBUG_PRINT(" visible");
				if(*marker & OBJECT_FREED)
					RELOCATE_DEBUG_PRINT(" freed");
				if(*marker & OBJECT_FINALIZABLE)
					RELOCATE_DEBUG_PRINT(" finalizable");
				if(*marker & OBJECT_FINALIZED)
					RELOCATE_DEBUG_PRINT(" finalized");
				if(*marker & OBJECT_ARRAY)
					RELOCATE_DEBUG_PRINT(" array");

				RELOCATE_DEBUG_PRINT(" %s\r\n", symbols + exTypes[unsigned(*marker >> 8)].offsetToName);

				if(*marker & 1)
				{
					*marker &= ~1;
					if(type.subCat != ExternTypeInfo::CAT_NONE)
						FixupVariable(target, subType);
				}
			}
		}
	}
}

void Executor::FixupArray(char* ptr, const ExternTypeInfo& type)
{
	ExternTypeInfo *subType = type.nameHash == ExPriv::autoArrayName ? NULL : &exTypes[type.subType];
	unsigned int size = type.arrSize;
	if(type.arrSize == ~0u)
	{
		// Get real array size
		size = *(int*)(ptr + NULLC_PTR_SIZE);

		// Switch pointer to array data
		char *target = vmLoadPointer(ptr);

		// If it points to stack, fix it and return
		if(target >= ExPriv::oldBase && target < (ExPriv::oldBase + ExPriv::oldSize))
		{
			vmStorePointer(ptr, target - ExPriv::oldBase + ExPriv::newBase);
			return;
		}
		ptr = target;

		// If uninitialized, return
		if(!ptr || ptr <= (char*)0x00010000)
			return;

		// Get base pointer
		unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(ptr);
		markerType *marker = (markerType*)((char*)basePtr - sizeof(markerType));

		// If there is no base pointer or memory already marked, exit
		if(!basePtr || !(*marker & 1))
			return;

		// Mark memory as used
		*marker &= ~1;
	}else if(type.nameHash == ExPriv::autoArrayName){
		NULLCAutoArray *data = (NULLCAutoArray*)ptr;

		// Get real variable type
		subType = &exTypes[data->typeID];

		// Skip uninitialized array
		if(!data->ptr)
			return;

		// If it points to stack, fix it
		if(data->ptr >= ExPriv::oldBase && data->ptr < (ExPriv::oldBase + ExPriv::oldSize))
			data->ptr = data->ptr - ExPriv::oldBase + ExPriv::newBase;

		// Mark target data
		FixupPointer(data->ptr, *subType, false);

		// Switch pointer to target
		ptr = data->ptr;

		// Get array size
		size = data->len;
	}

	if(!subType->pointerCount)
		return;

	switch(subType->subCat)
	{
	case ExternTypeInfo::CAT_NONE:
		break;
	case ExternTypeInfo::CAT_ARRAY:
		for(unsigned int i = 0; i < size; i++, ptr += subType->size)
			FixupArray(ptr, *subType);
		break;
	case ExternTypeInfo::CAT_POINTER:
		for(unsigned int i = 0; i < size; i++, ptr += subType->size)
			FixupPointer(ptr, *subType, true);
		break;
	case ExternTypeInfo::CAT_FUNCTION:
		for(unsigned int i = 0; i < size; i++, ptr += subType->size)
			FixupFunction(ptr);
		break;
	case ExternTypeInfo::CAT_CLASS:
		for(unsigned int i = 0; i < size; i++, ptr += subType->size)
			FixupClass(ptr, *subType);
		break;
	}
}

void Executor::FixupClass(char* ptr, const ExternTypeInfo& type)
{
	const ExternTypeInfo *realType = &type;
	if(type.nameHash == ExPriv::objectName)
	{
		// Get real variable type
		realType = &exTypes[*(int*)ptr];

		// Switch pointer to target
		char *target = vmLoadPointer(ptr + 4);

		// If it points to stack, fix it and return
		if(target >= ExPriv::oldBase && target < (ExPriv::oldBase + ExPriv::oldSize))
		{
			vmStorePointer(ptr + 4, target - ExPriv::oldBase + ExPriv::newBase);
			return;
		}
		ptr = target;

		// If uninitialized, return
		if(!ptr || ptr <= (char*)0x00010000)
			return;
		// Get base pointer
		unsigned int *basePtr = (unsigned int*)NULLC::GetBasePointer(ptr);
		markerType *marker = (markerType*)((char*)basePtr - sizeof(markerType));
		// If there is no base pointer or memory already marked, exit
		if(!basePtr || !(*marker & 1))
			return;
		// Mark memory as used
		*marker &= ~1;
		// Fixup target
		FixupVariable(target, *realType);
		// Exit
		return;
	}else if(type.nameHash == ExPriv::autoArrayName){
		FixupArray(ptr, type);
		// Exit
		return;
	}

	// Get class member type list
	ExternMemberInfo *memberList = &exLinker->exTypeExtra[realType->memberOffset + realType->memberCount];
	char *str = symbols + type.offsetToName;
	const char *memberName = symbols + type.offsetToName + strlen(str) + 1;
	// Check pointer members
	for(unsigned int n = 0; n < realType->pointerCount; n++)
	{
		// Get member type
		ExternTypeInfo &subType = exTypes[memberList[n].type];
		unsigned int pos = memberList[n].offset;

		RELOCATE_DEBUG_PRINT("\tChecking member %s at offset %d\r\n", memberName, pos);

		// Check member
		FixupVariable(ptr + pos, subType);
		unsigned int strLength = (unsigned int)strlen(memberName) + 1;
		memberName += strLength;
	}
}

void Executor::FixupFunction(char* ptr)
{
	NULLCFuncPtr *fPtr = (NULLCFuncPtr*)ptr;

	// If there's no context, there's nothing to check
	if(!fPtr->context)
		return;

	const ExternFuncInfo &func = exFunctions[fPtr->id];

	// If function context type is valid
	if(func.contextType != ~0u)
		FixupPointer((char*)&fPtr->context, exTypes[func.contextType], true);
}

void Executor::FixupVariable(char* ptr, const ExternTypeInfo& type)
{
	if(!type.pointerCount)
		return;

	switch(type.subCat)
	{
	case ExternTypeInfo::CAT_NONE:
		break;
	case ExternTypeInfo::CAT_ARRAY:
		FixupArray(ptr, type);
		break;
	case ExternTypeInfo::CAT_POINTER:
		FixupPointer(ptr, type, true);
		break;
	case ExternTypeInfo::CAT_FUNCTION:
		FixupFunction(ptr);
		break;
	case ExternTypeInfo::CAT_CLASS:
		FixupClass(ptr, type);
		break;
	}
}

bool Executor::ExtendParameterStack(char* oldBase, unsigned int oldSize, VMCmd *current)
{
	RELOCATE_DEBUG_PRINT("Old base: %p-%p\r\n", oldBase, oldBase + oldSize);
	RELOCATE_DEBUG_PRINT("New base: %p-%p\r\n", genParams.data, genParams.data + genParams.max);

	SetUnmanagableRange(genParams.data, genParams.max);

	NULLC::MarkMemory(1);

	ExPriv::oldBase = oldBase;
	ExPriv::newBase = genParams.data;
	ExPriv::oldSize = oldSize;
	ExPriv::newSize = genParams.max;

	symbols = exLinker->exSymbols.data;

	ExternVarInfo *vars = exLinker->exVariables.data;
	ExternTypeInfo *types = exLinker->exTypes.data;
	// Fix global variables
	for(unsigned int i = 0; i < exLinker->exVariables.size(); i++)
	{
		ExternVarInfo &varInfo = vars[i];

		RELOCATE_DEBUG_PRINT("Global variable %s (with offset of %d)\r\n", symbols + varInfo.offsetToName, varInfo.offset);

		FixupVariable(genParams.data + varInfo.offset, types[varInfo.type]);
	}

	int offset = exLinker->globalVarSize;
	int n = 0;
	fcallStack.push_back(current);
	// Fixup local variables
	for(; n < (int)fcallStack.size(); n++)
	{
		int address = int(fcallStack[n]-cmdBase);
		int funcID = -1;

		for(unsigned int i = 0; i < exFunctions.size(); i++)
		{
			if(address >= exFunctions[i].address && address < (exFunctions[i].address + exFunctions[i].codeSize))
				funcID = i;
		}

		if(funcID != -1)
		{
			ExternFuncInfo &funcInfo = exFunctions[funcID];

			int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;
			RELOCATE_DEBUG_PRINT("In function %s (with offset of %d)\r\n", symbols + funcInfo.offsetToName, alignOffset);
			offset += alignOffset;

			unsigned int offsetToNextFrame = funcInfo.bytesToPop;
			// Check every function local
			for(unsigned int i = 0; i < funcInfo.localCount; i++)
			{
				// Get information about local
				ExternLocalInfo &lInfo = exLinker->exLocals[funcInfo.offsetToFirstLocal + i];

				RELOCATE_DEBUG_PRINT("Local %s %s (with offset of %d+%d)\r\n", symbols + types[lInfo.type].offsetToName, symbols + lInfo.offsetToName, offset, lInfo.offset);
				FixupVariable(genParams.data + offset + lInfo.offset, types[lInfo.type]);
				if(lInfo.offset + lInfo.size > offsetToNextFrame)
					offsetToNextFrame = lInfo.offset + lInfo.size;
			}
			if(funcInfo.contextType != ~0u)
			{
				RELOCATE_DEBUG_PRINT("Local %s $context (with offset of %d+%d)\r\n", symbols + types[funcInfo.contextType].offsetToName, offset, funcInfo.bytesToPop - NULLC_PTR_SIZE);
				char *ptr = genParams.data + offset + funcInfo.bytesToPop - NULLC_PTR_SIZE;

				// Fixup pointer itself
				char *target = vmLoadPointer(ptr);

				if(target >= ExPriv::oldBase && target < (ExPriv::oldBase + ExPriv::oldSize))
				{
					RELOCATE_DEBUG_PRINT("\tFixing from %p to %p\r\n", ptr, ptr - ExPriv::oldBase + ExPriv::newBase);
					vmStorePointer(ptr, target - ExPriv::oldBase + ExPriv::newBase);
				}

				// Fixup what it was pointing to
				if(char *fixedTarget = vmLoadPointer(ptr))
					FixupVariable(fixedTarget, types[funcInfo.contextType]);
			}
			offset += offsetToNextFrame;
			RELOCATE_DEBUG_PRINT("Moving offset to next frame by %d bytes\r\n", offsetToNextFrame);
		}
	}
	fcallStack.pop_back();

	return true;
}

const char* Executor::GetResult()
{
	if(!codeRunning && genStackTop - genStackPtr > (int(lastResultType) == -1 ? 1 : 0))
	{
		NULLC::SafeSprintf(execResult, 64, "There is more than one value on the stack (%d)", int(genStackTop - genStackPtr));
		return execResult;
	}

	switch(lastResultType)
	{
	case OTYPE_DOUBLE:
		NULLC::SafeSprintf(execResult, 64, "%f", lastResultDouble);
		break;
	case OTYPE_LONG:
		NULLC::SafeSprintf(execResult, 64, "%lldL", lastResultLong);
		break;
	case OTYPE_INT:
		NULLC::SafeSprintf(execResult, 64, "%d", lastResultInt);
		break;
	default:
		NULLC::SafeSprintf(execResult, 64, "no return value");
		break;
	}
	return execResult;
}
int Executor::GetResultInt()
{
	assert(lastResultType == OTYPE_INT);
	return lastResultInt;
}
double Executor::GetResultDouble()
{
	assert(lastResultType == OTYPE_DOUBLE);
	return lastResultDouble;
}
long long Executor::GetResultLong()
{
	assert(lastResultType == OTYPE_LONG);
	return lastResultLong;
}

const char*	Executor::GetExecError()
{
	return execError;
}

char* Executor::GetVariableData(unsigned int *count)
{
	if(count)
		*count = genParams.size();
	return genParams.data;
}

void Executor::BeginCallStack()
{
	currentFrame = 0;
}
unsigned int Executor::GetNextAddress()
{
	return currentFrame == fcallStack.size() ? 0 : (unsigned int)(fcallStack[currentFrame++] - cmdBase);
}

void* Executor::GetStackStart()
{
	return genStackPtr;
}
void* Executor::GetStackEnd()
{
	return genStackTop;
}

void Executor::SetBreakFunction(void *context, unsigned (*callback)(void*, unsigned))
{
	breakFunctionContext = context;
	breakFunction = callback;
}

void Executor::ClearBreakpoints()
{
	// Check all instructions for break instructions
	for(unsigned int i = 0; i < exLinker->exCode.size(); i++)
	{
		// nop instruction is used for breaks
		// break structure: cmdOriginal, cmdNop
		if(exLinker->exCode[i].cmd == cmdNop)
			exLinker->exCode[i] = breakCode[exLinker->exCode[i].argument];	// replace it with original instruction
	}
	breakCode.clear();
}

bool Executor::AddBreakpoint(unsigned int instruction, bool oneHit)
{
	if(instruction >= exLinker->exCode.size())
	{
		NULLC::SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		return false;
	}
	unsigned int pos = breakCode.size();
	if(exLinker->exCode[instruction].cmd == cmdNop)
	{
		NULLC::SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: cannot set breakpoint on breakpoint");
		return false;
	}
	if(oneHit)
	{
		breakCode.push_back(exLinker->exCode[instruction]);
		exLinker->exCode[instruction].cmd = cmdNop;
		exLinker->exCode[instruction].flag = EXEC_BREAK_ONCE;
		exLinker->exCode[instruction].argument = pos;
	}else{
		breakCode.push_back(exLinker->exCode[instruction]);
		breakCode.push_back(VMCmd(cmdNop, EXEC_BREAK_RETURN, 0, instruction + 1));
		exLinker->exCode[instruction].cmd = cmdNop;
		exLinker->exCode[instruction].flag = EXEC_BREAK_SIGNAL;
		exLinker->exCode[instruction].argument = pos;
	}
	return true;
}

bool Executor::RemoveBreakpoint(unsigned int instruction)
{
	if(instruction > exLinker->exCode.size())
	{
		NULLC::SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: break position out of code range");
		return false;
	}
	if(exLinker->exCode[instruction].cmd != cmdNop)
	{
		NULLC::SafeSprintf(execError, ERROR_BUFFER_SIZE, "ERROR: there is no breakpoint at instruction %d", instruction);
		return false;
	}
	exLinker->exCode[instruction] = breakCode[exLinker->exCode[instruction].argument];
	return true;
}

void Executor::UpdateInstructionPointer()
{
	if(!cmdBase || !fcallStack.size() || cmdBase == &exLinker->exCode[0])
		return;
	for(unsigned int i = 0; i < fcallStack.size(); i++)
	{
		int currentPos = int(fcallStack[i] - cmdBase);
		assert(currentPos >= 0);
		fcallStack[i] = &exLinker->exCode[0] + currentPos;
	}
	cmdBase = &exLinker->exCode[0];
}
