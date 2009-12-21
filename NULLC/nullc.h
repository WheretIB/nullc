#ifndef NULLC_INCLUDED
#define NULLC_INCLUDED

#include "nullcdef.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned char nullres;

// Initialize NULLC
void	nullcInit();
void	nullcInitCustomAlloc(void* (NCDECL *allocFunc)(size_t), void (NCDECL *deallocFunc)(void*));

void	nullcSetImportPath(const char* path);

#define NULLC_VM	0
#define NULLC_X86	1
void	nullcSetExecutor(unsigned int id);

// prototype contains function prototype as if it was written in NULLC. It mush be followed by ';'
nullres	nullcAddExternalFunction(void (NCDECL *ptr)(), const char* prototype);

// compiles the code (!) and returns 1 on success
nullres	nullcCompile(const char* code);

// if compilation failed, this function will return compilation error
const char*	nullcGetCompilationError();

// compiled bytecode to be used for linking and executing can be retrieved with this function
// function returns bytecode size, and memory to which 'bytecode' points can be freed at any time
unsigned int nullcGetBytecode(char **bytecode);

// Function work only if NULLC_LOG_FILES is defined
// this function returns string with last bytecode disassembly
void	nullcSaveListing(const char *fileName);

// Clean all accumulated bytecode
void nullcClean();

// Link new chunk of code.
// If 'acceptRedefinitions' is 0, then error will be generated is function name collisions are found
// otherwise, old function code will be replaced with the new one.
// Type or redefinition always generates an error.
// If global variables with the same name are found, a warning is generated.
nullres nullcLinkCode(const char *bytecode, int acceptRedefinitions);
const char*	nullcGetLinkLog();

nullres	nullcRun();
nullres	nullcRunFunction(const char* funcName);
const char*	nullcGetRuntimeError();
void	nullcThrowError(const char* error);

const char*	nullcGetResult();
void*	nullcGetVariableData();

void	nullcDeinit();

void**	nullcGetVariableInfo(unsigned int* count);

#ifdef __cplusplus
}
#endif

#endif
