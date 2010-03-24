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

void		nullcTerminate();

/************************************************************************/
/*				NULLC execution settings and environment				*/

void		nullcSetExecutor(unsigned int id);

/*	Used to bind unresolved module functions to external C functions. Function index is the number of a function overload	*/
nullres		nullcAddModuleFunction(const char* module, void (NCDECL *ptr)(), const char* name, int index);

/*	Builds module and saves it's binary into binary cache	*/
nullres		nullcLoadModuleBySource(const char* module, const char* code);

/*	Loads module into binary cache	*/
nullres		nullcLoadModuleByBinary(const char* module, const char* binary);

/************************************************************************/
/*							Basic functions								*/

/*	Compiles and links code	*/
nullres		nullcBuild(const char* code);

/*	Run global code	*/
nullres		nullcRun();
/*	Run function code	*/
nullres		nullcRunFunction(const char* funcName);

/*	Retrieve result	*/
const char*	nullcGetResult();

/*	Returns last error description	*/
const char*	nullcGetLastError();

/************************************************************************/
/*							Interaction functions						*/

/*	Allocates memory block that is managed by GC	*/
void*		nullcAllocate(unsigned int size);

/*	Abort NULLC program execution with specified error code	*/
void		nullcThrowError(const char* error, ...);

/************************************************************************/
/*							Special modules								*/

int			nullcInitTypeinfoModule();
int			nullcInitDynamicModule();

/************************************************************************/
/*							Extended functions							*/

/*	Compiles the code (!) and returns 1 on success	*/
nullres			nullcCompile(const char* code);

/*	compiled bytecode to be used for linking and executing can be retrieved with this function
	function returns bytecode size, and memory to which 'bytecode' points can be freed at any time	*/
unsigned int	nullcGetBytecode(char **bytecode);
unsigned int	nullcGetBytecodeNoCache(char **bytecode);

/*	Function work only if NULLC_LOG_FILES is defined
	this function returns string with last bytecode disassembly	*/
void			nullcSaveListing(const char *fileName);

/*	Clean all accumulated bytecode	*/
void			nullcClean();

/*	Link new chunk of code.
	If 'acceptRedefinitions' is 0, then error will be generated is function name collisions are found
	otherwise, old function code will be replaced with the new one.
	Type or redefinition always generates an error.
	If global variables with the same name are found, a warning is generated. */
nullres			nullcLinkCode(const char *bytecode, int acceptRedefinitions);

#ifdef __cplusplus
}
#endif

#endif
