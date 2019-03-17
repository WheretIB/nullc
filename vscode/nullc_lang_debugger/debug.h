#pragma once

#include <string>

struct Context;

unsigned OnDebugBreak(void *context, unsigned instruction);

void LaunchApplicationThread(Context &ctx);

struct Source;
const char* GetModuleSourceCode(Context &ctx, const Source& source);

const char* GetLineStart(const char *sourceCode, int line);
const char* GetLineEnd(const char *lineStart);
unsigned ConvertPositionToInstruction(unsigned lineStartOffset, unsigned lineEndOffset);
unsigned ConvertLineToInstruction(const char *sourceCode, int line);

const char* GetInstructionSourceLocation(unsigned instruction);
unsigned GetSourceLocationModuleIndex(const char *sourceLocation);
unsigned ConvertSourceLocationToLine(const char *sourceLocation, unsigned moduleIndex, unsigned &column);
unsigned ConvertInstructionToLineAndModule(unsigned instruction, unsigned &moduleIndex);

std::string GetBasicVariableInfo(unsigned typeIndex, char* ptr, bool hex);
bool SetBasicVariableValue(unsigned typeIndex, char* ptr, const std::string& value);
