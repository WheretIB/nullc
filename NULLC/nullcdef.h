#ifndef NULLC_DEF_INCLUDED
#define NULLC_DEF_INCLUDED

#ifdef _MSC_VER
	#define NCDECL _cdecl
#else
	#define NCDECL
#endif

#define NULLC_VM_LOG_INSTRUCTION_EXECUTION
#define NULLC_VM_PROFILE_INSTRUCTIONS
#define NULLC_VM_DEBUG
#define NULLC_X86_CMP_FASM

#define NULLC_LOG_FILES

#ifdef _MSC_VER
#define NULLC_BUILD_X86_JIT
#endif

#endif
