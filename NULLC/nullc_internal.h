#ifndef NULLC_INTERNAL_INCLUDED
#define NULLC_INTERNAL_INCLUDED

#include "nullcdef.h"
#include "Compiler.h"

/************************************************************************/
/*							Internal functions							*/

CompilerContext* nullcGetCompilerContext();

#define NULLC_BUILTIN_SQRT 1

nullres nullcBindModuleFunctionBuiltin(const char* module, const char* name, int index, unsigned builtinIndex);

#define NULLC_ATTRIBUTE_NO_MEMORY_WRITE 0

nullres nullcSetModuleFunctionAttribute(const char* module, const char* name, int index, unsigned attribute, unsigned value);

void nullcVisitParseTreeNodes(SynBase *syntax, void *context, void(*accept)(void *context, SynBase *child));
void nullcVisitExpressionTreeNodes(ExprBase *expression, void *context, void(*accept)(void *context, ExprBase *child));

#endif
