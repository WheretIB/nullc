#pragma once

class Linker;

bool	nullcInitTypeinfoModule(Linker* linker);
void	nullcInitTypeinfoModuleLinkerOnly(Linker* linker);
void	nullcDeinitTypeinfoModule();

unsigned int	nullcGetTypeSize(unsigned int typeID);
const char*		nullcGetTypeName(unsigned int typeID);

unsigned int	nullcGetFunctionType(unsigned int funcID);
const char*		nullcGetFunctionName(unsigned int funcID);

unsigned int	nullcGetArraySize(unsigned int typeID);

unsigned int	nullcGetTypeCount();

int				nullcIsFunction(unsigned int id);
int				nullcIsClass(unsigned int id);
int				nullcIsSimple(unsigned int id);
int				nullcIsArray(unsigned int id);
int				nullcIsPointer(unsigned int id);

// pointer target type or array element type
unsigned int	nullcGetSubType(int id);
