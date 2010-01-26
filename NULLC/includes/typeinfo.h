#pragma once

#include "../Linker.h"

bool	nullcInitTypeinfoModule(Linker* linker);

void	nullcDeinitTypeinfoModule();

unsigned int	nullcGetTypeSize(unsigned int typeID);
