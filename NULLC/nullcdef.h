#ifndef NULLC_DEF_INCLUDED
#define NULLC_DEF_INCLUDED

#ifdef _MSC_VER
	#define NCDECL _cdecl
#else
	#define NCDECL
#endif

#pragma pack(push, 4)

// Wrapper over NULLC array, for use in external functions
struct NullCArray
{
	char			*ptr;
	unsigned int	len;
};

// Wrapper over NULLC auto ref class for use in external functions
struct NULLCRef
{
	unsigned int	typeID;
	char			*ptr;
};

// Wrapper over NULLC function pointer for use in external functions
struct NULLCFuncPtr
{
	void			*context;
	unsigned int	id;
};

#pragma pack(pop)

#define NULLC_MAX_VARIABLE_NAME_LENGTH 2048

//#define NULLC_VM_LOG_INSTRUCTION_EXECUTION
//#define NULLC_VM_PROFILE_INSTRUCTIONS
//#define NULLC_VM_DEBUG
#define NULLC_STACK_TRACE_WITH_LOCALS

//#define NULLC_LOG_FILES
#if defined(_MSC_VER) && defined(_DEBUG)
//#define VERBOSE_DEBUG_OUTPUT
//#define IMPORT_VERBOSE_DEBUG_OUTPUT
//#define LINK_VERBOSE_DEBUG_OUTPUT
#endif
#define ENABLE_GC
//#define NULLC_ENABLE_C_TRANSLATION

#if (defined(_MSC_VER) || defined(__DMC__)) && !defined(_M_X64) && !defined(NULLC_NO_EXECUTOR)
	#define NULLC_BUILD_X86_JIT
	#define NULLC_OPTIMIZE_X86
#endif

#if defined(NULLC_ENABLE_C_TRANSLATION) && defined(NULLC_OPTIMIZE_X86)
	#error "Cannot enable translation to C and x86 optimizer simultaneously"
#endif

typedef unsigned char nullres;

#define NULLC_VM	0
#define NULLC_X86	1

#ifdef _M_X64
	#define NULLC_PTR_TYPE TYPE_LONG
	#define NULLC_PTR_SIZE 8
#else
	#define NULLC_PTR_TYPE TYPE_INT
	#define NULLC_PTR_SIZE 4
#endif

#endif
