#include "TestBase.h"

#include "../NULLC/nullc_debug.h"
#include "../NULLC/nullc_internal.h"
#include "../NULLC/Trace.h"

#if defined(_MSC_VER)
#include <Windows.h>

double myGetPreciseTime()
{
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	double temp = double(count.QuadPart) / double(freq.QuadPart);
	return temp*1000.0;
}

#else

#include <time.h>
double myGetPreciseTime()
{
	return (clock() / double(CLOCKS_PER_SEC)) * 1000.0;
}

#endif

TestQueue* TestQueue::head = NULL;
TestQueue* TestQueue::tail = NULL;

int testsPassed[TEST_TYPE_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int testsCount[TEST_TYPE_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned int testTarget[TEST_TARGET_COUNT] = { NULLC_REG_VM, NULLC_X86, NULLC_LLVM };

namespace Tests
{
	bool messageVerbose = false;
	const char *lastMessage = NULL;

	const char *testMatch = NULL;
	bool testMatchLoop = false;

	double timeCompile = 0.0;
	double timeGetBytecode = 0.0;
	double timeVisit = 0.0;
	double timeTranslate = 0.0;
	double timeExprEvaluate = 0.0;
	double timeInstEvaluate = 0.0;
	double timeClean = 0.0;
	double timeLinkCode = 0.0;
	double timeRun = 0.0;

	long long totalOutput = 0;

	unsigned totalSyntaxNodes = 0;
	unsigned totalExpressionNodes = 0;

	unsigned totalRegVmInstructions = 0;

	unsigned totalPeepholeOptimizations = 0;
	unsigned totalConstantPropagations = 0;
	unsigned totalDeadCodeEliminations = 0;
	unsigned totalControlFlowSimplifications = 0;
	unsigned totalLoadStorePropagations = 0;
	unsigned totalCommonSubexprEliminations = 0;
	unsigned totalDeadAllocaStoreEliminations = 0;
	unsigned totalFunctionInlines = 0;

	unsigned totalDeltaPeepholeOptimizations = 0;
	unsigned totalDeltaConstantPropagations = 0;
	unsigned totalDeltaDeadCodeEliminations = 0;
	unsigned totalDeltaControlFlowSimplifications = 0;
	unsigned totalDeltaLoadStorePropagations = 0;
	unsigned totalDeltaCommonSubexprEliminations = 0;
	unsigned totalDeltaDeadAllocaStoreEliminations = 0;
	unsigned totalDeltaFunctionInlines = 0;

	const char		*varData = NULL;
	unsigned int	variableCount = 0;
	ExternVarInfo	*varInfo = NULL;
	const char		*symbols = NULL;

	bool doSaveTranslation = true;
	bool doTranslation = false;
	bool doExprEvaluation = true;
	bool doInstEvaluation = true;
	bool doVisit = true;

	bool compareOptimizations = false;
	bool enableDiffOptimization = true;
	bool enableTestOptimization = false;

	bool	testExecutor[TEST_TARGET_COUNT] = {
		true,
#ifdef NULLC_BUILD_X86_JIT
		true,
#else
		false,
#endif
#ifdef NULLC_LLVM_SUPPORT
		true,
#else
		false,
#endif
	};

	bool	testVmExecutor[TEST_TARGET_COUNT] = {
		true,
		false,
		false,
	};

	bool	testFailureExecutor[TEST_TARGET_COUNT] = {
		true,
#if defined(NULLC_BUILD_X86_JIT) && (_MSC_VER != 1700) // Broken unwind in v110 toolset
		true,
#else
		false,
#endif
		false
	};

	bool	testHardFailureExecutor[TEST_TARGET_COUNT] = {
		true,
#if defined(NULLC_BUILD_X86_JIT) && defined(NDEBUG)
		true,
#else
		false,
#endif
		false
	};

	const char* (*fileLoadFunc)(const char*, unsigned*) = 0;
	void (*fileFreeFunc)(const char*) = 0;

	void* OpenStream(const char *name)
	{
		(void)name;

		return new int;
	}

	void WriteStream(void *stream, const char *data, unsigned size)
	{
		(void)stream;
		(void)data;

		totalOutput += size;
	}

	void CloseStream(void* stream)
	{
		delete (int*)stream;
	}

	bool enableLogFiles = true;
	void* (*openStreamFunc)(const char* name) = OpenStream;
	void (*writeStreamFunc)(void *stream, const char *data, unsigned size) = WriteStream;
	void (*closeStreamFunc)(void* stream) = CloseStream;

	bool enableTimeTrace = true;

	unsigned testStackSize = 1024 * 1024;

	unsigned translationDependencyCount = 0;
	char *translationDependencies[128];

	void AddDependency(const char *fileName)
	{
		assert(translationDependencyCount < 128);

		translationDependencies[translationDependencyCount++] = strdup(fileName);
	}

	void AcceptSyntaxNode(void *context, SynBase *node)
	{
		(void)context;

		if(node && GetParseTreeNodeName(node))
			totalSyntaxNodes++;
	}

	void AcceptExpressionNode(void *context, ExprBase *node)
	{
		(void)context;

		if(node && GetExpressionTreeNodeName(node))
			totalExpressionNodes++;
	}

	struct EvaluationFailReson
	{
		char *reason;
		unsigned count;
	};

	unsigned expressionEvaluionFails = 0;
	EvaluationFailReson expressionEvaluionFailInfo[128];

	void AddExpressionEvaluionFail(const char *reason)
	{
		for(unsigned i = 0; i < expressionEvaluionFails; i++)
		{
			if(strcmp(expressionEvaluionFailInfo[i].reason, reason) == 0)
			{
				expressionEvaluionFailInfo[i].count++;
				return;
			}
		}
		assert(expressionEvaluionFails < 128);

		if(expressionEvaluionFails < 128)
		{
			expressionEvaluionFailInfo[expressionEvaluionFails].reason = strdup(reason);
			expressionEvaluionFailInfo[expressionEvaluionFails].count = 1;
			expressionEvaluionFails++;
		}
	}

	unsigned instructionEvaluionFails = 0;
	EvaluationFailReson instructionEvaluionFailInfo[128];

	void AddInstructionEvaluionFail(const char *reason)
	{
		for(unsigned i = 0; i < instructionEvaluionFails; i++)
		{
			if(strcmp(instructionEvaluionFailInfo[i].reason, reason) == 0)
			{
				instructionEvaluionFailInfo[i].count++;
				return;
			}
		}
		assert(instructionEvaluionFails < 128);

		if(instructionEvaluionFails < 128)
		{
			instructionEvaluionFailInfo[instructionEvaluionFails].reason = strdup(reason);
			instructionEvaluionFailInfo[instructionEvaluionFails].count = 1;
			instructionEvaluionFails++;
		}
	}
}

void* Tests::FindVar(const char* name)
{
	for(unsigned int i = 0; i < variableCount; i++)
	{
		if(strcmp(name, symbols + varInfo[i].offsetToName) == 0)
			return (void*)(varData + varInfo[i].offset);
	}
	return NULL;
}

bool Tests::RunCode(const char *code, unsigned int executor, const char* expected, const char* message, bool execShouldFail)
{
	TRACE_SCOPE("tests", "RunCode");
	TRACE_LABEL(message);

	if(testMatch && !strstr(message, testMatch))
		return false;

#ifndef NULLC_BUILD_X86_JIT
	if(executor == NULLC_X86)
		return false;
#endif

	if(!execShouldFail && strstr(code, "namespace") == NULL)
	{
		const unsigned extraCodeSize = 16 * 1024;
		static char extraCode[extraCodeSize];

		assert(strlen(code) < extraCodeSize - 128);

		const char *headerEnd = strstr(code, "import");

		while(headerEnd)
		{
			headerEnd = strchr(headerEnd, ';');

			if(const char *next = strstr(headerEnd, "import"))
				headerEnd = next;
			else
				break;
		}

		// Place code inside a function
		char *pos = extraCode;

		if(headerEnd)
		{
			memcpy(pos, code, unsigned(headerEnd - code) + 1);
			pos += unsigned(headerEnd - code) + 1;

			strcpy(pos, "\r\n");
			pos += strlen(pos);
		}

		strcpy(pos, "auto __runner(){\r\n");
		pos += strlen(pos);

		if(headerEnd)
		{
			strcpy(pos, headerEnd + 1);
			pos += strlen(pos);
		}
		else
		{
			strcpy(pos, code);
			pos += strlen(pos);
		}

		strcpy(pos, "\r\n}\r\nreturn __runner();");
		pos += strlen(pos);

		assert(pos < extraCode + extraCodeSize);

		if(!RunCodeSimple(extraCode, executor, expected, message, execShouldFail, "[function]"))
			return false;

		// Place code inside a coroutine
		pos = extraCode;

		if(headerEnd)
		{
			memcpy(pos, code, unsigned(headerEnd - code) + 1);
			pos += unsigned(headerEnd - code) + 1;

			strcpy(pos, "\r\n");
			pos += strlen(pos);
		}

		strcpy(pos, "coroutine auto __runner(){\r\n");
		pos += strlen(pos);

		if(headerEnd)
		{
			strcpy(pos, headerEnd + 1);
			pos += strlen(pos);
		}
		else
		{
			strcpy(pos, code);
			pos += strlen(pos);
		}

		strcpy(pos, "\r\n}\r\nreturn __runner();");
		pos += strlen(pos);

		assert(pos < extraCode + extraCodeSize);

		if(!RunCodeSimple(extraCode, executor, expected, message, execShouldFail, "[coroutine]"))
			return false;

		// Place code inside a namespace
		pos = extraCode;

		if(headerEnd)
		{
			memcpy(pos, code, unsigned(headerEnd - code) + 1);
			pos += unsigned(headerEnd - code) + 1;

			strcpy(pos, "\r\n");
			pos += strlen(pos);
		}

		strcpy(pos, "namespace Runner{\r\n");
		pos += strlen(pos);

		if(headerEnd)
		{
			strcpy(pos, headerEnd + 1);
			pos += strlen(pos);
		}
		else
		{
			strcpy(pos, code);
			pos += strlen(pos);
		}

		strcpy(pos, "\r\n}");
		pos += strlen(pos);

		assert(pos < extraCode + extraCodeSize);

		if(!RunCodeSimple(extraCode, executor, expected, message, execShouldFail, "[namespace]"))
			return false;
	}

	if(testMatch && testMatchLoop)
	{
		for(unsigned i = 0; i < 5000; i++)
		{
			if(!RunCodeSimple(code, executor, expected, message, execShouldFail, ""))
				return false;
		}
	}

	if(!RunCodeSimple(code, executor, expected, message, execShouldFail, ""))
		return false;

	return true;
}

bool Tests::RunCodeSimple(const char *code, unsigned int executor, const char* expected, const char* message, bool execShouldFail, const char *variant)
{
	TRACE_SCOPE("tests", "RunCodeSimple");
	TRACE_LABEL(message);
	TRACE_LABEL(variant);

	if(testMatch && !strstr(message, testMatch))
		return false;

#ifndef NULLC_BUILD_X86_JIT
	if(executor == NULLC_X86)
		return false;
#endif

	lastMessage = message;

	const char *executorName = executor == NULLC_X86 ? "X86" : (executor == NULLC_LLVM ? "LLVM" : "REGVM");

	if(message && messageVerbose)
		printf("%4d/%4d %s %s [%s]\n", testsPassed[executor], testsCount[executor] - 1, message, variant, executorName);

	nullcSetExecutor(executor);

	if(compareOptimizations)
		enableTestOptimization = false;

	double time = myGetPreciseTime();

	nullres good = nullcCompile(code);

	timeCompile += myGetPreciseTime() - time;
	time = myGetPreciseTime();

	if(!good)
	{
		if(message && !messageVerbose)
			printf("%s\n", message);
		printf("%s Compilation failed: %s\r\n", executorName, nullcGetLastError());
		return false;
	}
	else
	{
		unsigned optimizationsBefore = 0;

		unsigned peepholeOptimizations = 0;
		unsigned constantPropagations = 0;
		unsigned deadCodeEliminations = 0;
		unsigned controlFlowSimplifications = 0;
		unsigned loadStorePropagations = 0;
		unsigned commonSubexprEliminations = 0;
		unsigned deadAllocaStoreEliminations = 0;
		unsigned functionInlines = 0;

		if(CompilerContext *context = nullcGetCompilerContext())
		{
			totalRegVmInstructions += context->instRegVmFinalizeCtx.cmds.size();

			if(VmModule *vmModule = context->vmModule)
			{
				optimizationsBefore = vmModule->peepholeOptimizations + vmModule->constantPropagations + vmModule->deadCodeEliminations + vmModule->controlFlowSimplifications + vmModule->loadStorePropagations + vmModule->commonSubexprEliminations + vmModule->deadAllocaStoreEliminations + vmModule->functionInlines;

				peepholeOptimizations = vmModule->peepholeOptimizations;
				constantPropagations = vmModule->constantPropagations;
				deadCodeEliminations = vmModule->deadCodeEliminations;
				controlFlowSimplifications = vmModule->controlFlowSimplifications;
				loadStorePropagations = vmModule->loadStorePropagations;
				commonSubexprEliminations = vmModule->commonSubexprEliminations;
				deadAllocaStoreEliminations = vmModule->deadAllocaStoreEliminations;
				functionInlines = vmModule->functionInlines;

				totalPeepholeOptimizations += vmModule->peepholeOptimizations;
				totalConstantPropagations += vmModule->constantPropagations;
				totalDeadCodeEliminations += vmModule->deadCodeEliminations;
				totalControlFlowSimplifications += vmModule->controlFlowSimplifications;
				totalLoadStorePropagations += vmModule->loadStorePropagations;
				totalCommonSubexprEliminations += vmModule->commonSubexprEliminations;
				totalDeadAllocaStoreEliminations += vmModule->deadAllocaStoreEliminations;
				totalFunctionInlines += vmModule->functionInlines;
			}
		}

		if(compareOptimizations && executor == NULLC_REG_VM)
		{
			enableTestOptimization = true;

			nullcClean();

			nullcCompile(code);

			if(CompilerContext *context = nullcGetCompilerContext())
			{
				totalRegVmInstructions += context->instRegVmFinalizeCtx.cmds.size();

				if(VmModule *vmModule = context->vmModule)
				{
					unsigned optimizationsAfter = vmModule->peepholeOptimizations + vmModule->constantPropagations + vmModule->deadCodeEliminations + vmModule->controlFlowSimplifications + vmModule->loadStorePropagations + vmModule->commonSubexprEliminations + vmModule->deadAllocaStoreEliminations + vmModule->functionInlines;

					if(optimizationsAfter != optimizationsBefore)
					{
						int deltas[8] = {
							int(vmModule->peepholeOptimizations - peepholeOptimizations),
							int(vmModule->constantPropagations - constantPropagations),
							int(vmModule->deadCodeEliminations - deadCodeEliminations),
							int(vmModule->controlFlowSimplifications - controlFlowSimplifications),
							int(vmModule->loadStorePropagations - loadStorePropagations),
							int(vmModule->commonSubexprEliminations - commonSubexprEliminations),
							int(vmModule->deadAllocaStoreEliminations - deadAllocaStoreEliminations),
							int(vmModule->functionInlines - functionInlines)
						};

						totalDeltaPeepholeOptimizations += deltas[0];
						totalDeltaConstantPropagations += deltas[1];
						totalDeltaDeadCodeEliminations += deltas[2];
						totalDeltaControlFlowSimplifications += deltas[3];
						totalDeltaLoadStorePropagations += deltas[4];
						totalDeltaCommonSubexprEliminations += deltas[5];
						totalDeltaDeadAllocaStoreEliminations += deltas[6];
						totalDeltaFunctionInlines += deltas[7];

						if(message && !messageVerbose)
							printf("%s %s [%s]\n", message, variant, executorName);

						printf("Opt delta: peep %+d constprop %+d dce %+d cfsimp %+d lsprop %+d comsubexpr %+d deadstore %+d funcinline %+d\n", deltas[0], deltas[1], deltas[2], deltas[3], deltas[4], deltas[5], deltas[6], deltas[7]);

						if(enableDiffOptimization && Tests::enableLogFiles && !Tests::openStreamFunc)
						{
							(void)remove("inst_graph_opt_before.txt");
							(void)remove("inst_graph_opt_after.txt");
							(void)rename("inst_graph_opt.txt", "inst_graph_opt_after.txt");

							enableTestOptimization = false;

							nullcClean();

							nullcCompile(code);

							(void)rename("inst_graph_opt.txt", "inst_graph_opt_before.txt");
						}
					}
				}
			}
		}

		if(doVisit)
		{
			if(CompilerContext *context = nullcGetCompilerContext())
			{
				nullcVisitParseTreeNodes(context->synModule, NULL, AcceptSyntaxNode);

				nullcVisitExpressionTreeNodes(context->exprModule, NULL, AcceptExpressionNode);
			}

			timeVisit += myGetPreciseTime() - time;
			time = myGetPreciseTime();
		}

		char *bytecode = NULL;
		nullcGetBytecode(&bytecode);

		timeGetBytecode += myGetPreciseTime() - time;
		time = myGetPreciseTime();

		if(executor == NULLC_REG_VM && !execShouldFail && (doSaveTranslation || doTranslation))
		{
			for(unsigned i = 0; i < translationDependencyCount; i++)
				free(translationDependencies[i]);

			translationDependencyCount = 0;

			if(!nullcTranslateToC("1test.cpp", "main", AddDependency))
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("%s Translation failed: %s\r\n", executorName, nullcGetLastError());
				return false;
			}
		}

		timeTranslate += myGetPreciseTime() - time;
		time = myGetPreciseTime();

		char exprEvalBuf[256];
		bool exprCheckResult = false;

		if(executor == NULLC_REG_VM && !execShouldFail && doExprEvaluation)
		{
			exprCheckResult = nullcTestEvaluateExpressionTree(exprEvalBuf, 256) != 0;

			if(!exprCheckResult)
				AddExpressionEvaluionFail(nullcGetLastError());
		}

		timeExprEvaluate += myGetPreciseTime() - time;
		time = myGetPreciseTime();

		char instEvalBuf[256];
		bool instCheckResult = false;

		if(executor == NULLC_REG_VM && !execShouldFail && doInstEvaluation)
		{
			instCheckResult = nullcTestEvaluateInstructionTree(instEvalBuf, 256) != 0;

			if(!instCheckResult)
				AddInstructionEvaluionFail(nullcGetLastError());
		}

		timeInstEvaluate += myGetPreciseTime() - time;
		time = myGetPreciseTime();

		nullcClean();

		timeClean += myGetPreciseTime() - time;
		time = myGetPreciseTime();

		int linkgood = nullcLinkCode(bytecode);

		timeLinkCode += myGetPreciseTime() - time;
		time = myGetPreciseTime();

		delete[] bytecode;

		if(!linkgood)
		{
			if(message && !messageVerbose)
				printf("%s\n", message);
			printf("%s Link failed: %s\r\n", executorName, nullcGetLastError());
			return false;
		}

		nullres goodRun = nullcRun();

		timeRun += myGetPreciseTime() - time;

		if(goodRun)
		{
			if(execShouldFail)
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("%s Execution should have failed with %s\r\n", executorName, expected);
				return false;
			}

			const char* val = nullcGetResult();
			varData = (char*)nullcGetVariableData(NULL);

			nullcFinalize();

			if(expected && strcmp(val, expected) != 0)
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("%s Failed (%s != %s)\r\n", executorName, val, expected);
				return false;
			}

			if(exprCheckResult)
			{
				testsCount[TEST_TYPE_EXPR_EVALUATION]++;

				if(expected && strcmp(expected, exprEvalBuf) != 0)
				{
					if(message && !messageVerbose)
						printf("%s\n", message);
					printf("%s Failed (%s != %s)\r\n", "Expression evaluation", exprEvalBuf, expected);
					return false;
				}

				testsPassed[TEST_TYPE_EXPR_EVALUATION]++;
			}

			if(instCheckResult)
			{
				testsCount[TEST_TYPE_INST_EVALUATION]++;

				if(expected && strcmp(expected, instEvalBuf) != 0)
				{
					if(message && !messageVerbose)
						printf("%s\n", message);
					printf("%s Failed (%s != %s)\r\n", "Instruction evaluation", instEvalBuf, expected);
					return false;
				}

				testsPassed[TEST_TYPE_INST_EVALUATION]++;
			}
		}
		else if(execShouldFail)
		{
			char buf[512];

			if(const char *pos = strstr(nullcGetLastError(), "ERROR:"))
				strncpy(buf, pos, 511);
			else
				strncpy(buf, nullcGetLastError(), 511);

			buf[511] = 0;

			if(char *lineEnd = strchr(buf, '\r'))
				*lineEnd = 0;

			if(strcmp(expected, buf) != 0)
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("Failed but for wrong reason:\r\n    %s\r\nexpected:\r\n    %s\r\n", buf, expected);
				return false;
			}
		}
		else
		{
			if(message && !messageVerbose)
				printf("%s\n", message);
			printf("%s Execution failed: %s\r\n", executorName, nullcGetLastError());
			return false;
		}
	}

	varData = (char*)nullcGetVariableData(NULL);
	varInfo = nullcDebugVariableInfo(&variableCount);
	symbols = nullcDebugSymbols(NULL);

	if(executor == NULLC_REG_VM && !execShouldFail && doTranslation && !(message && strstr(message, "skip_c")))
	{
		testsCount[TEST_TYPE_TRANSLATION]++;

		char cmdLine[1024];
		char *pos = cmdLine;

#if defined(_MSC_VER)
		STARTUPINFO stInfo;
		PROCESS_INFORMATION prInfo;
		memset(&stInfo, 0, sizeof(stInfo));
		stInfo.cb = sizeof(stInfo);
		memset(&prInfo, 0, sizeof(prInfo));

		NULLC::SafeSprintf(pos, 1024, "gcc.exe -o runnable.exe");
		pos += strlen(pos);
#else
		NULLC::SafeSprintf(pos, 1024, "gcc -o runnable");
		pos += strlen(pos);
#endif

		NULLC::SafeSprintf(pos, 1024 - unsigned(pos - cmdLine), " 1test.cpp");
		pos += strlen(pos);

		NULLC::SafeSprintf(pos, 1024 - unsigned(pos - cmdLine), " -lstdc++");
		pos += strlen(pos);

		NULLC::SafeSprintf(pos, 1024 - unsigned(pos - cmdLine), " -Itranslation");
		pos += strlen(pos);

		NULLC::SafeSprintf(pos, 1024 - unsigned(pos - cmdLine), " -I../NULLC/translation");
		pos += strlen(pos);

		NULLC::SafeSprintf(pos, 1024 - unsigned(pos - cmdLine), " ../NULLC/translation/runtime.cpp -lstdc++ -lm");
		pos += strlen(pos);

		for(unsigned i = 0; i < translationDependencyCount; i++)
		{
			const char *dependency = translationDependencies[i];

			NULLC::SafeSprintf(pos, 1024 - unsigned(pos - cmdLine), " %s", dependency);
			pos += strlen(pos);

			if(strstr(dependency, "import_"))
			{
				char tmp[256];
				NULLC::SafeSprintf(tmp, 256 - strlen("_bind"), "../NULLC/translation/%s", dependency + strlen("import_"));

				if(char *pos = strstr(tmp, "_nc.cpp"))
					strcpy(pos, "_bind.cpp");

				if(FILE *file = fopen(tmp, "r"))
				{
					NULLC::SafeSprintf(pos, 1024 - unsigned(pos - cmdLine), " %s", tmp);
					pos += strlen(pos);

					fclose(file);
				}

				NULLC::SafeSprintf(tmp, 256 - strlen("_bind"), "translation/%s", dependency + strlen("import_"));

				if(char *pos = strstr(tmp, "_nc.cpp"))
					strcpy(pos, "_bind.cpp");

				if(FILE *file = fopen(tmp, "r"))
				{
					NULLC::SafeSprintf(pos, 1024 - unsigned(pos - cmdLine), " %s", tmp);
					pos += strlen(pos);

					fclose(file);
				}
			}
		}

		for(unsigned i = 0; i < translationDependencyCount; i++)
			free(translationDependencies[i]);

		translationDependencyCount = 0;

#if defined(_MSC_VER)
		DWORD res = CreateProcess(NULL, cmdLine, NULL, NULL, false, 0, NULL, ".\\", &stInfo, &prInfo);
		res = GetLastError();
		WaitForSingleObject(prInfo.hProcess, 10000);
		DWORD retCode;
		GetExitCodeProcess(prInfo.hProcess, &retCode);
		CloseHandle(prInfo.hProcess);
		CloseHandle(prInfo.hThread);
#else
		int retCode = system(cmdLine);
#endif
		if(!retCode)
		{
#if defined(_MSC_VER)
			FILE *resPipe = _popen("runnable.exe", "rb");
#else
			FILE *resPipe = popen("./runnable", "r");
#endif
			char buf[256];
			strcpy(buf, "no return value");
			fgets(buf, 256, resPipe);

#if defined(_MSC_VER)
			_pclose(resPipe);
#else
			pclose(resPipe);
#endif

			char expectedCopy[256];
			strcpy(expectedCopy, expected);
			if(strchr(expectedCopy, 'L'))
				*strchr(expectedCopy, 'L') = 0;
			if(strcmp(buf, expectedCopy))
			{
				if(message && !messageVerbose)
					printf("%s\n", message);
				printf("C++: failed, expected %s, got %s\r\n", expected, buf);
			}else{
				testsPassed[TEST_TYPE_TRANSLATION]++;
			}
		}else{
			if(message && !messageVerbose)
				printf("%s\n", message);
			printf("C++ compilation error (%d)\r\n", retCode);
		}
	}

	return true;
}

char*	Tests::Format(const char *str, ...)
{
	static char text[4096*16];
	static unsigned int section = 0;

	char* ptr = text + (section++ % 16) * 4096;

	va_list args;
	va_start(args, str);
	vsnprintf(ptr, 1023, str, args);
	va_end(args);
	return ptr;
}

void Tests::Cleanup()
{
	for(unsigned i = 0; i < expressionEvaluionFails; i++)
		free(expressionEvaluionFailInfo[i].reason);
	expressionEvaluionFails = 0;

	for(unsigned i = 0; i < instructionEvaluionFails; i++)
		free(instructionEvaluionFailInfo[i].reason);
	instructionEvaluionFails = 0;
}

void TEST_FOR_FAIL(const char* name, const char* str, const char* error)
{
	TRACE_SCOPE("tests", "TEST_FOR_FAIL");
	TRACE_LABEL(name);

	testsCount[TEST_TYPE_FAILURE]++;
	nullres good = nullcCompile(str);

	if(!good)
	{
		char buf[1024];

		if(const char *pos = strstr(nullcGetLastError(), "ERROR:"))
			strncpy(buf, pos, 1023);
		else
			strncpy(buf, nullcGetLastError(), 1023);

		buf[1023] = 0;

		if(char *lineEnd = strchr(buf, '\n'))
			*lineEnd = 0;

		if(strcmp(error, buf) != 0)
		{
			printf("Failed %s but for wrong reason:\n    %s\nexpected:\n    %s\n", name, buf, error);
		}
		else
		{
			// Check that evaluation is ok on an ill-formed program
			char exprResult[256];
			nullcTestEvaluateExpressionTree(exprResult, 256);

			testsPassed[TEST_TYPE_FAILURE]++;
		}
	}
	else
	{
		printf("Test \"%s\" failed to fail.\n", name);
	}
}

void TEST_FOR_FAIL_GENERIC(const char* name, const char* str, const char* error1, const char* error2)
{
	testsCount[TEST_TYPE_FAILURE]++;
	nullres good = nullcCompile(str);
	if(!good)
	{
		char buf[1024];

		if(const char *pos = strstr(nullcGetLastError(), "ERROR:"))
			strncpy(buf, pos, 1023);
		else
			strncpy(buf, nullcGetLastError(), 1023);

		buf[1023] = 0;


		if(memcmp(buf, error1, strlen(error1)) != 0)
		{
			printf("Failed %s but for wrong reason:\n    %s\nexpected:\n    %s\n", name, buf, error1);
		}
		else
		{
			char *bufNext = strstr(buf + 1, "while instantiating");

			if(!bufNext)
			{
				printf("Failed %s but with no second error:\nexpected:\n    %s\n", name, error2);
			}
			else
			{
				if(char *lineEnd = strchr(bufNext, '\n'))
					*lineEnd = 0;

				if(strcmp(error2, bufNext) != 0)
				{
					printf("Failed %s but for wrong reason:\n    %s\nexpected:\n    %s\n", name, bufNext, error2);
				}
				else
				{
					testsPassed[TEST_TYPE_FAILURE]++;
				}
			}
		}
	}
	else
	{
		printf("Test \"%s\" failed to fail.\n", name);
	}
}
