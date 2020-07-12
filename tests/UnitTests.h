#pragma once

int RunTests(bool verbose, const char* (*fileLoadFunc)(const char*, unsigned*) = 0, void (*fileFreeFunc)(const char*) = 0, bool runSpeedTests = false, bool testOutput = false, bool testTranslationSave = false, bool testTranslation = false, bool testTimeTrace = false);
