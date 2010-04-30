#ifndef NULLC_DEBUG_INCLUDED
#define NULLC_DEBUG_INCLUDED

#include "nullcdef.h"
#include "Bytecode.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************/
/*							Debug functions								*/

void*				nullcGetVariableData();

unsigned int		nullcGetCurrentExecutor(void **exec);
const void*			nullcGetModule(const char* path);

/*	Used to retrieve code information of linked code	*/

ExternTypeInfo*		nullcDebugTypeInfo(unsigned int *count);
unsigned int*		nullcDebugTypeExtraInfo(unsigned int *count);
ExternVarInfo*		nullcDebugVariableInfo(unsigned int *count);
ExternFuncInfo*		nullcDebugFunctionInfo(unsigned int *count);
ExternLocalInfo*	nullcDebugLocalInfo(unsigned int *count);
char*				nullcDebugSymbols();

struct NULLCCodeInfo
{
	unsigned int byteCodePos;
	unsigned int sourceOffset;
};
NULLCCodeInfo*		nullcDebugCodeInfo(unsigned int *count);
ExternModuleInfo*	nullcDebugModuleInfo(unsigned int *count);

void				nullcDebugBeginCallStack();
unsigned int		nullcDebugGetStackFrame();

// A function that is called when breakpoint is hit. Function accepts instruction number
nullres				nullcDebugSetBreakFunction(void (*callback)(unsigned int));
// You can remove all breakpoints explicitly. nullcClean clears all breakpoints automatically
nullres				nullcDebugClearBreakpoints();
// Line number can be translated into instruction number by using nullcDebugCodeInfo and nullcDebugModuleInfo
nullres				nullcDebugAddBreakpoint(unsigned int instruction);

//void				nullcDebugRemoveBreakpoint(unsigned int line);

#ifdef __cplusplus
}
#endif

#endif
