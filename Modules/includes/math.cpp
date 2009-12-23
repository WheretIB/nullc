#include "math.h"
#include "../../nullc/nullc.h"

namespace NULLCMath
{

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcAddModuleFunction("std.math", (void(*)())NULLCMath::funcPtr, name, index)) return false;
bool	nullcInitMathModule()
{
	return true;
}

void	nullcDeinitMathModule()
{

}
