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

// Wrapper over NULLC auto[] class for use in external functions
struct NULLCAutoArray
{
	unsigned int	typeID;
	char			*ptr;
	unsigned int	len;
};

#pragma pack(pop)

#define NULLC_MAX_VARIABLE_NAME_LENGTH 2048
#define NULLC_DEFAULT_GLOBAL_MEMORY_LIMIT 1024 * 1024 * 1024

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
#define NULLC_PURE_FUNCTIONS

#if (defined(_MSC_VER) || defined(__DMC__)) && !defined(_M_X64) && !defined(NULLC_NO_EXECUTOR)
	#define NULLC_BUILD_X86_JIT
	#define NULLC_OPTIMIZE_X86
#endif

#if defined(NULLC_ENABLE_C_TRANSLATION) && defined(NULLC_OPTIMIZE_X86)
	#error "Cannot enable translation to C and x86 optimizer simultaneously"
#endif

#if (defined(__linux) && !defined(__x86_64__)) || defined(__CELLOS_LV2__)
	#define NULLC_COMPLEX_RETURN
#endif

typedef unsigned char nullres;

#define NULLC_VM	0
#define NULLC_X86	1

#ifdef __x86_64__
	#define _M_X64
#endif

#ifdef _M_X64
	#define NULLC_PTR_TYPE TYPE_LONG
	#define NULLC_PTR_SIZE 8
#else
	#define NULLC_PTR_TYPE TYPE_INT
	#define NULLC_PTR_SIZE 4
#endif

#endif
