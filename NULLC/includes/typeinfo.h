#pragma once

#include "../Linker.h"

bool	nullcInitTypeinfoModule(Linker* linker);

unsigned int	nullcGetTypeSize(unsigned int typeID);
const char*		nullcGetTypeName(unsigned int typeID);
