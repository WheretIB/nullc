#ifndef NULLC_DEBUG_INCLUDED
#define NULLC_DEBUG_INCLUDED

#include "nullcdef.h"
#include "Bytecode.h"
#include "InstructionSet.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************/
/*							Debug functions								*/

void*				nullcGetVariableData(unsigned int *count);

unsigned int		nullcGetCurrentExecutor(void **exec);
const void*			nullcGetModule(const char* path);

/*	Used to retrieve code information of linked code	*/

ExternTypeInfo*		nullcDebugTypeInfo(unsigned int *count);
unsigned int*		nullcDebugTypeExtraInfo(unsigned int *count);
ExternVarInfo*		nullcDebugVariableInfo(unsigned int *count);
ExternFuncInfo*		nullcDebugFunctionInfo(unsigned int *count);
ExternLocalInfo*	nullcDebugLocalInfo(unsigned int *count);
char*				nullcDebugSymbols(unsigned int *count);
char*				nullcDebugSource();

struct NULLCCodeInfo
{
	unsigned int byteCodePos;
	unsigned int sourceOffset;
};
NULLCCodeInfo*		nullcDebugCodeInfo(unsigned int *count);
VMCmd*				nullcDebugCode(unsigned int *count);
ExternModuleInfo*	nullcDebugModuleInfo(unsigned int *count);

void				nullcDebugBeginCallStack();
unsigned int		nullcDebugGetStackFrame();

#define	NULLC_BREAK_PROCEED		0
#define NULLC_BREAK_STEP		1
#define NULLC_BREAK_STEP_INTO	2
#define NULLC_BREAK_STEP_OUT	3
#define NULLC_BREAK_STOP		4

// A function that is called when breakpoint is hit. Function accepts instruction number and returns how the break should be handled (constant above)
nullres				nullcDebugSetBreakFunction(unsigned (*callback)(unsigned int));
// You can remove all breakpoints explicitly. nullcClean clears all breakpoints automatically
nullres				nullcDebugClearBreakpoints();
// Line number can be translated into instruction number by using nullcDebugCodeInfo and nullcDebugModuleInfo
nullres				nullcDebugAddBreakpoint(unsigned int instruction);
nullres				nullcDebugAddOneHitBreakpoint(unsigned int instruction);
nullres				nullcDebugRemoveBreakpoint(unsigned int instruction);

ExternFuncInfo*		nullcDebugConvertAddressToFunction(int instruction);

#ifdef __cplusplus
}
#endif

#endif
