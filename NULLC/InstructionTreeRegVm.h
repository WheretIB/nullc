#pragma once

#include <stdint.h>

enum RegVmInstructionCode
{
	rviNop,

	rviLoadByte,
	rviLoadWord,
	rviLoadDword,
	rviLoadLong,
	rviLoadFloat,
	rviLoadDouble,

	rviLoadImm,
	rviLoadImmLong,
	rviLoadImmDouble,

	rviStoreByte,
	rviStoreWord,
	rviStoreDword,
	rviStoreLong,
	rviStoreFloat,
	rviStoreDouble,

	rviCombinedd,
	rviMov,

	rviDtoi,
	rviDtol,
	rviDtof,
	rviItod,
	rviLtod,
	rviItol,
	rviLtoi,

	rviIndex,

	rviGetAddr,

	rviSetRange,

	rviJmp,
	rviJmpz,
	rviJmpnz,

	rviPush,
	rviPushLong,
	rviPushDouble,

	rviPushImm,
	rviPushImmq,

	rviPop,
	rviPopq,

	rviCall,
	rviCallPtr,

	rviReturn,

	rviPushvtop,

	rviAdd,
	rviSub,
	rviMul,
	rviDiv,
	rviPow,
	rviMod,

	rviLess,
	rviGreater,
	rviLequal,
	rviGequal,
	rviEqual,
	rviNequal,

	rviShl,
	rviShr,
	
	rviBitAnd,
	rviBitOr,
	rviBitXor,

	rviLogXor,

	rviAddl,
	rviSubl,
	rviMull,
	rviDivl,
	rviPowl,
	rviModl,

	rviLessl,
	rviGreaterl,
	rviLequall,
	rviGequall,
	rviEquall,
	rviNequall,

	rviShll,
	rviShrl,

	rviBitAndl,
	rviBitOrl,
	rviBitXorl,

	rviLogXorl,

	rviAddd,
	rviSubd,
	rviMuld,
	rviDivd,
	rviPowd,
	rviModd,

	rviLessd,
	rviGreaterd,
	rviLequald,
	rviGequald,
	rviEquald,
	rviNequald,

	rviNeg,
	rviNegl,
	rviNegd,

	rviBitNot,
	rviBitNotl,

	rviLogNot,
	rviLogNotl,

	rviConvertPtr,

	rviCheckRet,

	// Temporary instructions, no execution
	rviFuncAddr,
	rviTypeid,
};

enum RegVmSetRangeType
{
	rvsrDouble,
	rvsrFloat,
	rvsrLong,
	rvsrInt,
	rvsrShort,
	rvsrChar,
};

enum RegVmReturnType
{
	rvrVoid,
	rvrDouble,
	rvrLong,
	rvrInt,
	rvrStruct,
	rvrError,
};

#if NULLC_PTR_SIZE == 4
#define rvrPointer rvrInt
#else
#define rvrPointer rvrLong
#endif

#define rvrrGlobals 0
#define rvrrFrame 1

#define rvrrCount 2

struct RegVmCmd
{
	RegVmCmd(): code(0), rA(0), rB(0), rC(0), argument(0)
	{
	}

	RegVmCmd(RegVmInstructionCode code, unsigned char rA, unsigned char rB, unsigned char rC, unsigned argument): code((unsigned char)code), rA(rA), rB(rB), rC(rC), argument(argument)
	{
	}

	unsigned char code;
	unsigned char rA;
	unsigned char rB;
	unsigned char rC;
	unsigned argument;
};

struct RegVmRegister
{
	// Debug testing only
	//RegVmReturnType activeType;

	union
	{
		int32_t	intValue;
		int64_t longValue;
		double doubleValue;
	};
};

#if NULLC_PTR_SIZE == 4
#define ptrValue intValue
#else
#define ptrValue longValue
#endif

struct RegVmCallFrame
{
	RegVmCallFrame() : instruction(0), dataSize(0), regFilePtr(0), resultReg(0)
	{
	}

	RegVmCallFrame(RegVmCmd *instruction, unsigned dataSize, RegVmRegister *regFilePtr, unsigned char resultReg) : instruction(instruction), dataSize(dataSize), regFilePtr(regFilePtr), resultReg(resultReg)
	{
	}

	RegVmCmd *instruction;
	unsigned dataSize;
	RegVmRegister *regFilePtr;
	unsigned char resultReg;
};

const char* GetInstructionName(RegVmInstructionCode code);
