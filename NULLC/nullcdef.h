#ifndef NULLC_DEF_INCLUDED
#define NULLC_DEF_INCLUDED

#ifdef _MSC_VER
	#define NCDECL _cdecl
#else
	#define NCDECL
	typedef int intptr_t;
#endif

#define NULLC_MAX_VARIABLE_NAME_LENGTH 2048

//#define NULLC_VM_LOG_INSTRUCTION_EXECUTION
//#define NULLC_VM_PROFILE_INSTRUCTIONS
//#define NULLC_VM_DEBUG

#define NULLC_LOG_FILES

#ifdef _MSC_VER
#define NULLC_BUILD_X86_JIT
#endif

#endif
