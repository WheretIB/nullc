#ifndef NULLC_DEF_INCLUDED
#define NULLC_DEF_INCLUDED

#ifdef _MSC_VER
	#define NCDECL _cdecl
#else
	#define NCDECL
	typedef int intptr_t;
#endif

// Wrapper over NULLC array, for use in external functions
struct NullCArray
{
	char* ptr;
	unsigned int len;
};

// Wrapper over NULLC auto ref class for use in external functions
struct NULLCRef
{
	unsigned int typeID;
	char* ptr;
};

#define NULLC_MAX_VARIABLE_NAME_LENGTH 2048

//#define NULLC_VM_LOG_INSTRUCTION_EXECUTION
//#define NULLC_VM_PROFILE_INSTRUCTIONS
//#define NULLC_VM_DEBUG
#define NULLC_STACK_TRACE_WITH_LOCALS

#define NULLC_LOG_FILES
#if defined(_MSC_VER) && defined(_DEBUG)
//#define VERBOSE_DEBUG_OUTPUT
//#define IMPORT_VERBOSE_DEBUG_OUTPUT
//#define LINK_VERBOSE_DEBUG_OUTPUT
#endif
#define ENABLE_GC

#if defined(_MSC_VER) && !defined(_M_X64)
#define NULLC_BUILD_X86_JIT
#endif

#endif
