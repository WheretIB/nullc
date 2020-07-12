#pragma once

#include <stdint.h>

#include "nullcdef.h"

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
	rviBreakupdd,
	rviMov,
	rviMovMult,

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
	rviMemCopy,

	rviJmp,
	rviJmpz,
	rviJmpnz,

	rviCall,
	rviCallPtr,

	rviReturn,

	rviAddImm,

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

	rviAddImml,

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

	rviAddd,
	rviSubd,
	rviMuld,
	rviDivd,

	rviAddf,
	rviSubf,
	rviMulf,
	rviDivf,

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

enum RegVmCopyType
{
	rvcFull,
	rvcInt,
	rvcDouble,
	rvcLong,
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

enum RegVmMicrocodeInstruction
{
	rmviNop,

	rvmiCall,

	rvmiPush,
	rvmiPushQword,

	rvmiPushImm,
	rvmiPushImmq,
	rvmiPushMem,

	rvmiReturn,

	rvmiPop,
	rvmiPopq,
	rvmiPopMem,
};

#if NULLC_PTR_SIZE == 4
#define rvrPointer rvrInt
#else
#define rvrPointer rvrLong
#endif

#define rvrrGlobals 0
#define rvrrFrame 1
#define rvrrConstants 2
#define rvrrRegisters 3

#define rvrrCount 4

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

const char* GetInstructionName(RegVmInstructionCode code);
