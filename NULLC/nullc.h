#ifndef NULLC_INCLUDED
#define NULLC_INCLUDED

#include "nullcdef.h"

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************/
/*				NULLC initialization and termination					*/

void		nullcInit(const char* importPath);
void		nullcInitCustomAlloc(void* (NCDECL *allocFunc)(int), void (NCDECL *deallocFunc)(void*), const char* importPath);

void		nullcSetImportPath(const char* importPath);
void		nullcSetFileReadHandler(const void* (NCDECL *fileLoadFunc)(const char* name, unsigned int* size, int* nullcShouldFreePtr));
void		nullcSetGlobalMemoryLimit(unsigned int limit);

void		nullcTerminate();

/************************************************************************/
/*				NULLC execution settings and environment				*/

/*	Change current executor to either NULLC_VM or NULLC_X86	*/
void		nullcSetExecutor(unsigned int id);
#ifdef NULLC_BUILD_X86_JIT
/*	Set memory range where JiT parameter stack will be placed.
	If flagMemoryAllocated is not set, executor will allocate memory itself using VirtualAlloc with base == start.
	When flagMemoryAllocated is not set, end can be set to NULL, meaning that x86 parameter stack can grow indefinitely.
	Default mode: start = 0x20000000, end = NULL, flagMemoryAllocated = false	*/
nullres		nullcSetJiTStack(void* start, void* end, unsigned int flagMemoryAllocated);
#endif

/*	Used to bind unresolved module functions to external C functions. Function index is the number of a function overload	*/
nullres		nullcBindModuleFunction(const char* module, void (NCDECL *ptr)(), const char* name, int index);

/*	Builds module and saves its binary into binary cache	*/
nullres		nullcLoadModuleBySource(const char* module, const char* code);

/*	Loads module into binary cache	*/
nullres		nullcLoadModuleByBinary(const char* module, const char* binary);

/* Removes module from binary cache	*/
void		nullcRemoveModule(const char* module);

/*	Returns name of a module at index 'id'. Null pointer is returned if a module at index 'id' doesn't exist.
	To get all module names, start with 'id' = 0 and go up until null pointer is returned	*/
const char*	nullcEnumerateModules(unsigned id);

/************************************************************************/
/*							Basic functions								*/

/*	Compiles and links code	*/
nullres		nullcBuild(const char* code);

/*	Run global code	*/
nullres		nullcRun();
/*	Run function code	*/
nullres		nullcRunFunction(const char* funcName, ...);

/*	Retrieve result	*/
const char*	nullcGetResult();
int			nullcGetResultInt();
double		nullcGetResultDouble();
long long	nullcGetResultLong();

/*	Returns last error description	*/
const char*	nullcGetLastError();

#ifndef NULLC_NO_EXECUTOR
nullres		nullcFinalize();
#endif

/************************************************************************/
/*							Interaction functions						*/

#ifndef NULLC_NO_EXECUTOR

// A list of indexes for NULLC build-in types
#define	NULLC_TYPE_VOID			0
#define	NULLC_TYPE_DOUBLE		1
#define	NULLC_TYPE_FLOAT		2
#define	NULLC_TYPE_LONG			3
#define	NULLC_TYPE_INT			4
#define	NULLC_TYPE_SHORT		5
#define	NULLC_TYPE_CHAR			6
#define	NULLC_TYPE_AUTO_REF		7
#define	NULLC_TYPE_TYPEID		8
#define	NULLC_TYPE_AUTO_ARRAY	10
#define	NULLC_TYPE_FUNCTION		11
#define	NULLC_TYPE_GENERIC		12
#define	NULLC_TYPE_BOOL			13

/*	Allocates memory block that is managed by GC	*/
void*		nullcAllocate(unsigned int size);
void*		nullcAllocateTyped(unsigned int typeID);
NULLCArray	nullcAllocateArrayTyped(unsigned int typeID, unsigned int count);

/*	Abort NULLC program execution with specified error code	*/
void		nullcThrowError(const char* error, ...);

/*	Call function using NULLCFuncPtr	*/
nullres		nullcCallFunction(NULLCFuncPtr ptr, ...);

/*	Get global variable value	*/
void*		nullcGetGlobal(const char* name);

/*	Set global variable value	*/
nullres		nullcSetGlobal(const char* name, void* data);

/*	Get function pointer	*/
nullres		nullcGetFunction(const char* name, NULLCFuncPtr* func);

/*	Set function using function pointer	*/
nullres		nullcSetFunction(const char* name, NULLCFuncPtr func);

/*	Function returns 1 if passed pointer points to NULLC stack; otherwise, the return value is 0	*/
nullres		nullcIsStackPointer(void* ptr);

/*	Function returns 1 if passed pointer points to a memory managed by NULLC GC; otherwise, the return value is 0	*/
nullres		nullcIsManagedPointer(void* ptr);

#endif

/************************************************************************/
/*							Special modules								*/

int			nullcInitTypeinfoModule();
int			nullcInitDynamicModule();

/************************************************************************/
/*							Extended functions							*/

/*	Compiles the code and returns 1 on success	*/
nullres			nullcCompile(const char* code);

/*	compiled bytecode to be used for linking and executing can be retrieved with this function
	function returns bytecode size, and memory to which 'bytecode' points can be freed at any time	*/
unsigned int	nullcGetBytecode(char **bytecode);
unsigned int	nullcGetBytecodeNoCache(char **bytecode);

/*	Function works only if NULLC_LOG_FILES is defined.
	This function saves disassembly of last compiled code into file	*/
void			nullcSaveListing(const char *fileName);

/*	Function works only if NULLC_ENABLE_C_TRANSLATION is defined.
	This function saved analog of C++ code of last compiled code into file	*/
void			nullcTranslateToC(const char *fileName, const char *mainName);

/*	Clean all accumulated bytecode	*/
void			nullcClean();

/*	Link new chunk of code.
	Type or function redefinition generates an error.
	Global variables with the same name are ok. */
nullres			nullcLinkCode(const char *bytecode);

#ifdef __cplusplus
}
#endif

#endif
