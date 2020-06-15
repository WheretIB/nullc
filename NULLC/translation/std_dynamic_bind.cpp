#include "runtime.h"

#undef assert

void  assert(int val, const char* message, void* unused);

void override_void_ref_auto_ref_auto_ref_(NULLCRef a, NULLCRef b, void* __context)
{
	assert(__nullcGetTypeInfo(a.typeID)->category == NULLC_FUNCTION, "ERROR: first argument to 'override' is not a function", 0);
	assert(__nullcGetTypeInfo(b.typeID)->category == NULLC_FUNCTION, "ERROR: second argument to 'override' is not a function", 0);
	__nullcFunctionArray* fTable = __nullcGetFunctionTable();
	NULLCFuncPtr<> *aFunc = (NULLCFuncPtr<>*)a.ptr;
	NULLCFuncPtr<> *bFunc = (NULLCFuncPtr<>*)b.ptr;
	(*fTable)[aFunc->id] = (*fTable)[bFunc->id];
}
void override_void_ref_auto_ref_char___(NULLCRef function, NULLCArray< char > code, void* __context)
{
	assert(0, "ERROR: override with source code is unsupported in translated code", 0);
}
