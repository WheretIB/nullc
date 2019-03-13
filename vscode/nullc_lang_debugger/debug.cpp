#include "debug.h"

#include <thread>

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullc_debug.h"

#include "context.h"
#include "handler.h"
#include "schema.h"

unsigned OnDebugBreak(void *context, unsigned instruction)
{
	Context &ctx = *(Context*)context;

	//SendEventOutput(ctx,  OutputEventData("stdout", "before breakpoint\n"));
	//SendEventStopped(ctx, StoppedEventData("breakpoint", "Breakpoint Hit", 1, false, "Manual Breakpoint", true));
	SendEventStopped(ctx, StoppedEventData("breakpoint", 1));

	/*StoppedEventData data;
	data.reason = "breakpoint";
	data.allThreadsStopped = true;
	SendEventStopped(ctx, data);*/

	//SendEventOutput(ctx,  OutputEventData("stdout", "after breakpoint\n"));

	{
		std::unique_lock<std::mutex> lock(ctx.breakpointMutex);

		ctx.breakpointWait.wait(lock);
	}

	(void)instruction;

	return NULLC_BREAK_PROCEED;
}

void ApplicationThread(Context &ctx)
{
	if(nullcRunFunction(NULL))
	{
		const char *val = nullcGetResult();

		nullcFinalize();

		SendEventOutput(ctx, OutputEventData("console", val));

		ctx.running.store(false);

		/*SendEventThread(ctx, ThreadEventData("exited", 1));
		SendEventExited(ctx, atoi(val));
		SendEventTerminated(ctx);*/
	}
	else
	{
		SendEventOutput(ctx, OutputEventData("stderr", nullcGetLastError()));

		ctx.running.store(false);

		SendEventThread(ctx, ThreadEventData("exited", 1));
		SendEventExited(ctx, -1);
		SendEventTerminated(ctx);
	}
}

void LaunchApplicationThread(Context &ctx)
{
	ctx.applicationThread = std::thread([&ctx]{
		ApplicationThread(ctx);
	});
}

std::string NormalizePath(std::string path)
{
	// Lowercase and transform folder slashes in a consistent way
	for(auto &&el : path)
	{
		if(el == '\\')
			el = '/';
		else if(isalpha(el))
			el = (char)tolower(el);
	}

	return path;
}

const char* GetModuleSourceCode(Context &ctx, const Source& source)
{
	// Name is required
	if(!source.name)
		return nullptr;

	std::string name = NormalizePath(*source.name);

	unsigned symbolCount = 0;
	auto symbols = nullcDebugSymbols(&symbolCount);

	unsigned moduleCount = 0;
	auto modules = nullcDebugModuleInfo(&moduleCount);

	auto fullSource = nullcDebugSource();

	for(unsigned i = 0; i < moduleCount; i++)
	{
		ExternModuleInfo &moduleInfo = modules[i];

		std::string moduleName = NormalizePath(symbols + moduleInfo.nameOffset);

		if(const char *pos = strstr(moduleName.c_str(), name.c_str()))
		{
			if(strlen(pos) == name.length())
				return fullSource + modules[i].sourceOffset;
		}
	}

	const char *mainModuleSource = fullSource + modules[moduleCount - 1].sourceOffset + modules[moduleCount - 1].sourceSize;

	std::string program = NormalizePath(*ctx.launchArgs.program);

	if(const char *pos = strstr(program.c_str(), name.c_str()))
	{
		if(strlen(pos) == name.length())
			return mainModuleSource;
	}

	return nullptr;
}

const char* GetLineStart(const char *sourceCode, int line)
{
	const char *start = sourceCode;
	int startLine = 0;

	while(*start && startLine < line)
	{
		if(*start == '\r')
		{
			start++;

			if(*start == '\n')
				start++;

			startLine++;
		}
		else if(*start == '\n')
		{
			start++;

			startLine++;
		}
		else
		{
			start++;
		}
	}

	return start;
}

const char* GetLineEnd(const char *lineStart)
{
	const char *pos = lineStart;

	while(*pos)
	{
		if(*pos == '\r')
			return pos;
		
		if(*pos == '\n')
			return pos;

		pos++;
	}

	return pos;
}

unsigned ConvertPositionToInstruction(unsigned lineStartOffset, unsigned lineEndOffset)
{
	unsigned infoSize = 0;
	auto codeInfo = nullcDebugCodeInfo(&infoSize);

	// Find instruction
	for(unsigned i = 0; i < infoSize; i++)
	{
		if(codeInfo[i].sourceOffset >= lineStartOffset && codeInfo[i].sourceOffset <= lineEndOffset)
			return codeInfo[i].byteCodePos;
	}

	return 0;
}

unsigned ConvertLineToInstruction(const char *sourceCode, int line)
{
	const char *lineStart = GetLineStart(sourceCode, line);
	
	if(!*lineStart)
		return 0;

	const char *lineEnd = GetLineEnd(lineStart);

	auto fullSource = nullcDebugSource();

	unsigned lineStartOffset = unsigned(lineStart - fullSource);
	unsigned lineEndOffset = unsigned(lineEnd - fullSource);

	return ConvertPositionToInstruction(lineStartOffset, lineEndOffset);
}
