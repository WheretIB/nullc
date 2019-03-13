#pragma once

struct Context;

unsigned OnDebugBreak(void *context, unsigned instruction);

void LaunchApplicationThread(Context &ctx);

struct Source;
const char* GetModuleSourceCode(Context &ctx, const Source& source);

const char* GetLineStart(const char *sourceCode, int line);
const char* GetLineEnd(const char *lineStart);
unsigned ConvertPositionToInstruction(unsigned lineStartOffset, unsigned lineEndOffset);
unsigned ConvertLineToInstruction(const char *sourceCode, int line);
