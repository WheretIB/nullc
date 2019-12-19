#ifndef NULLC_INTERNAL_INCLUDED
#define NULLC_INTERNAL_INCLUDED

#include "nullcdef.h"
#include "Compiler.h"

/************************************************************************/
/*							Internal functions							*/

CompilerContext* nullcGetCompilerContext();

nullres nullcBindModuleFunctionBuiltin(const char* module, unsigned builtinIndex, const char* name, int index);

void nullcVisitParseTreeNodes(SynBase *syntax, void *context, void(*accept)(void *context, SynBase *child));
void nullcVisitExpressionTreeNodes(ExprBase *expression, void *context, void(*accept)(void *context, ExprBase *child));

#endif
