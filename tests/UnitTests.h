#pragma once

int RunTests(bool verbose, const void* (*fileLoadFunc)(const char*, unsigned int*, int*) = 0, bool runSpeedTests = false, bool testOutput = false, bool testTranslationSave = false, bool testTranslation = false);
