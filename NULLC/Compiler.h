#pragma once

#include "stdafx.h"

#include "ParseTree.h"
#include "ParseGraph.h"
#include "ExpressionTree.h"
#include "ExpressionGraph.h"
#include "ExpressionEval.h"
#include "ExpressionTranslate.h"
#include "InstructionTreeVm.h"
#include "InstructionTreeVmGraph.h"
#include "InstructionTreeVmEval.h"
#include "InstructionTreeRegVmLower.h"
#include "InstructionTreeRegVmLowerGraph.h"
#include "InstructionTreeLlvm.h"
#include "Output.h"

struct CompilerContext
{
	CompilerContext(Allocator *allocator, int optimizationLevel, ArrayView<InplaceStr> activeImports): allocator(allocator), parseCtx(allocator, optimizationLevel, activeImports), exprCtx(allocator, optimizationLevel), instRegVmFinalizeCtx(exprCtx, allocator), optimizationLevel(optimizationLevel)
	{
		code = 0;

		errorPos = 0;
		errorBuf = 0;
		errorBufSize = 0;

		synModule = 0;

		exprModule = 0;

		vmModule = 0;

		llvmModule = 0;

		regVmLoweredModule = 0;

		enableLogFiles = false;
	}

	Allocator *allocator;

	const char *code;

	const char *errorPos;
	char *errorBuf;
	unsigned errorBufSize;

	OutputContext outputCtx;

	ParseContext parseCtx;
	SynModule *synModule;

	ExpressionContext exprCtx;
	ExprModule *exprModule;

	VmModule *vmModule;

	LlvmModule *llvmModule;

	RegVmLoweredModule *regVmLoweredModule;

	InstructionRegVmFinalizeContext instRegVmFinalizeCtx;

	bool enableLogFiles;

	int optimizationLevel;
};

bool BuildBaseModule(Allocator *allocator, int optimizationLevel);

ExprModule* AnalyzeModuleFromSource(CompilerContext &ctx, const char *code);

bool CompileModuleFromSource(CompilerContext &ctx, const char *code);

unsigned GetBytecode(CompilerContext &ctx, char **bytecode);

bool SaveListing(CompilerContext &ctx, const char *fileName);

bool TranslateToC(CompilerContext &ctx, const char *fileName, const char *mainName, void (*addDependency)(const char *fileName));

char* BuildModuleFromSource(Allocator *allocator, const char *modulePath, const char *code, unsigned codeSize, const char **errorPos, char *errorBuf, unsigned errorBufSize, int optimizationLevel, ArrayView<InplaceStr> activeImports);
char* BuildModuleFromPath(Allocator *allocator, InplaceStr moduleName, bool addExtension, const char **errorPos, char *errorBuf, unsigned errorBufSize, int optimizationLevel, ArrayView<InplaceStr> activeImports);

bool AddModuleFunction(Allocator *allocator, const char* module, void (*ptrRaw)(), void *funcWrap, void (*ptrWrap)(void *func, char* retBuf, char* argBuf), const char* name, int index, const char **errorPos, char *errorBuf, unsigned errorBufSize, int optimizationLevel);
