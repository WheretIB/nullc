#pragma once

#include "NULLC/nullcdef.h"

int RunTests(bool verbose, const void* (NCDECL *fileLoadFunc)(const char*, unsigned int*, int*) = 0, bool runSpeedTests = false);
