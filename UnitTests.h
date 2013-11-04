#pragma once

#include "NULLC/nullcdef.h"

void	RunTests(bool verbose, const void* (NCDECL *fileLoadFunc)(const char*, unsigned int*, int*) = 0);

