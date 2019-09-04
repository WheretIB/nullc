#include "list.h"
#include "../nullc.h"
#include "../nullbind.h"

namespace NULLCList
{

}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("old.list", NULLCList::funcPtr, name, index)) return false;
bool	nullcInitListModule()
{
	return true;
}
