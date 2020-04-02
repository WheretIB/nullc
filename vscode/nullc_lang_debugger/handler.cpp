#include "handler.h"

#pragma warning(disable: 4996)

#include <algorithm>

#include <stdio.h>
#include <stdarg.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullc_debug.h"

// NULLC modules
#include "../../NULLC/includes/file.h"
#include "../../NULLC/includes/io.h"
#include "../../NULLC/includes/math.h"
#include "../../NULLC/includes/string.h"
#include "../../NULLC/includes/vector.h"
#include "../../NULLC/includes/random.h"
#include "../../NULLC/includes/time.h"
#include "../../NULLC/includes/gc.h"

#include "../../NULLC/includes/window.h"
#include "../../NULLC/includes/canvas.h"

#include "../../NULLC/includes/pugi.h"

#include "context.h"
#include "debug.h"
#include "schema.h"

NULLC_PRINT_FORMAT_CHECK(1, 2) std::string ToString(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	static char buf[4096];

	vsnprintf(buf, 4096, format, args);
	buf[4095] = '\0';

	va_end(args);

	return buf;
}

void PrepareResponse(Context& ctx, rapidjson::Document &doc, int requestSeq, const char *command)
{
	doc.SetObject();

	doc.AddMember("seq", ctx.seq++, doc.GetAllocator());
	doc.AddMember("type", std::string("response"), doc.GetAllocator());

	doc.AddMember("request_seq", requestSeq, doc.GetAllocator());
	doc.AddMember("command", std::string(command), doc.GetAllocator());
}

void SendResponse(Context& ctx, rapidjson::Document &doc)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	const char* output = buffer.GetString();
	unsigned length = (unsigned)strlen(output);

	if(ctx.debugMode)
		fprintf(stderr, "DEBUG: Sending message '%s'\r\n", output);

	std::lock_guard<std::mutex> lock(ctx.outputMutex);

	fprintf(stdout, "Content-Length: %d\r\n", length);
	fprintf(stdout, "\r\n");
	fprintf(stdout, "%s", output);
	fflush(stdout);
}

bool RespondWithError(Context& ctx, rapidjson::Document &doc, const char *message)
{
	if(ctx.infoMode)
		fprintf(stderr, "INFO: RespondWithError('%s')\r\n", message);

	doc.AddMember("success", false, doc.GetAllocator());
	doc.AddMember("message", std::string(message), doc.GetAllocator());

	SendResponse(ctx, doc);

	return true;
}

void SendEventInitialized(Context& ctx)
{
	rapidjson::Document response;
	response.SetObject();

	response.AddMember("seq", ctx.seq++, response.GetAllocator());
	response.AddMember("type", std::string("event"), response.GetAllocator());

	response.AddMember("event", std::string("initialized"), response.GetAllocator());

	SendResponse(ctx, response);
}

void SendEventProcess(Context& ctx)
{
	rapidjson::Document response;
	response.SetObject();

	response.AddMember("seq", ctx.seq++, response.GetAllocator());
	response.AddMember("type", std::string("event"), response.GetAllocator());

	response.AddMember("event", std::string("process"), response.GetAllocator());

	rapidjson::Document body;
	body.SetObject();

	body.AddMember("name", "nullc_application", response.GetAllocator());
	body.AddMember("isLocalProcess", false, response.GetAllocator());
	body.AddMember("startMethod", "launch", response.GetAllocator());
	
	response.AddMember("body", body, response.GetAllocator());

	SendResponse(ctx, response);
}

void SendEventThread(Context& ctx, const ThreadEventData& data)
{
	rapidjson::Document response;
	response.SetObject();

	response.AddMember("seq", ctx.seq++, response.GetAllocator());
	response.AddMember("type", std::string("event"), response.GetAllocator());

	response.AddMember("event", std::string("thread"), response.GetAllocator());

	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);
}

void SendEventTerminated(Context& ctx)
{
	rapidjson::Document response;
	response.SetObject();

	response.AddMember("seq", ctx.seq++, response.GetAllocator());
	response.AddMember("type", std::string("event"), response.GetAllocator());

	response.AddMember("event", std::string("terminated"), response.GetAllocator());

	SendResponse(ctx, response);
}

void SendEventExited(Context& ctx, int exitCode)
{
	rapidjson::Document response;
	response.SetObject();

	response.AddMember("seq", ctx.seq++, response.GetAllocator());
	response.AddMember("type", std::string("event"), response.GetAllocator());

	response.AddMember("event", std::string("exited"), response.GetAllocator());

	rapidjson::Document body;
	body.SetObject();

	body.AddMember("exitCode", exitCode, response.GetAllocator());

	response.AddMember("body", body, response.GetAllocator());

	SendResponse(ctx, response);
}

void SendEventOutput(Context& ctx, const OutputEventData& data)
{
	rapidjson::Document response;
	response.SetObject();

	response.AddMember("seq", ctx.seq++, response.GetAllocator());
	response.AddMember("type", std::string("event"), response.GetAllocator());

	response.AddMember("event", std::string("output"), response.GetAllocator());

	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);
}

void SendEventStopped(Context& ctx, const StoppedEventData& data)
{
	rapidjson::Document response;
	response.SetObject();

	response.AddMember("seq", ctx.seq++, response.GetAllocator());
	response.AddMember("type", std::string("event"), response.GetAllocator());

	response.AddMember("event", std::string("stopped"), response.GetAllocator());

	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);
}

bool HandleRequestInitialize(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	ctx.initArgs = InitializeRequestArguments(arguments);

	Capabilities capabilities;

	capabilities.supportsConfigurationDoneRequest = true;
	capabilities.supportsFunctionBreakpoints = true;
	capabilities.supportsConditionalBreakpoints = false;
	capabilities.supportsHitConditionalBreakpoints = false;
	capabilities.supportsEvaluateForHovers = false;
	//capabilities.exceptionBreakpointFilters;
	capabilities.supportsStepBack = false;
	capabilities.supportsSetVariable = true;
	capabilities.supportsRestartFrame = false;
	capabilities.supportsGotoTargetsRequest = false;
	capabilities.supportsStepInTargetsRequest = false;
	capabilities.supportsCompletionsRequest = false;
	capabilities.supportsModulesRequest = false;
	//capabilities.additionalModuleColumns;
	//capabilities.supportedChecksumAlgorithms;
	capabilities.supportsRestartRequest = true;
	capabilities.supportsExceptionOptions = false;
	capabilities.supportsValueFormattingOptions = true;
	capabilities.supportsExceptionInfoRequest = false;
	capabilities.supportTerminateDebuggee = true;
	capabilities.supportsDelayedStackTraceLoading = true;
	capabilities.supportsLoadedSourcesRequest = true;
	capabilities.supportsLogPoints = false;
	capabilities.supportsTerminateThreadsRequest = false;
	capabilities.supportsSetExpression = false;
	capabilities.supportsTerminateRequest = true;
	capabilities.supportsDataBreakpoints = false;

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", ::ToJson(capabilities, response), response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestSetBreakpoints(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	SetBreakpointsArguments args{arguments};

	std::vector<Breakpoint> breakpoints;

	const char *moduleSource = GetModuleSourceCode(ctx, args.source);

	auto &activeBreakpoints = ctx.sourceBreakpoints[uintptr_t(moduleSource)];

	for(auto &&el : activeBreakpoints)
		nullcDebugRemoveBreakpoint(el);

	activeBreakpoints.clear();

	if(args.breakpoints)
	{
		for(auto &el : *args.breakpoints)
		{
			Breakpoint breakpoint;

			breakpoint.id = (int)breakpoints.size() + 1;

			breakpoint.verified = false;

			if(!el.condition && !el.hitCondition && !el.logMessage)
			{
				if(moduleSource)
				{
					unsigned zeroBasedLine = el.line - (ctx.initArgs.linesStartAt1 && *ctx.initArgs.linesStartAt1 ? 1 : 0);

					if(unsigned instruction = ConvertLineToInstruction(moduleSource, zeroBasedLine))
					{
						if(ctx.debugMode)
							fprintf(stderr, "INFO: Placing file '%s' breakpoint line %d at instruction %d\r\n", args.source.name->c_str(), zeroBasedLine, instruction);

						if(nullcDebugAddBreakpoint(instruction))
						{
							activeBreakpoints.push_back(instruction);

							breakpoint.line = el.line;

							breakpoint.verified = true;
						}
						else
						{
							breakpoint.message = nullcGetLastError();
						}
					}
					else
					{
						breakpoint.message = "can't find instruction at the target location";
					}
				}
				else
				{
					breakpoint.message = "target source file not found in linked program";
				}
			}
			else
			{
				breakpoint.message = "unsupported field condition/hitCondition/logMessage";
			}

			breakpoints.push_back(breakpoint);
		}
	}
	else if(args.lines)
	{
		for(auto &el : *args.lines)
		{
			Breakpoint breakpoint;

			breakpoint.id = (int)breakpoints.size() + 1;

			breakpoint.verified = false;

			if(moduleSource)
			{
				if(unsigned instruction = ConvertLineToInstruction(moduleSource, el))
				{
					if(nullcDebugAddBreakpoint(instruction))
					{
						breakpoint.line = el - (ctx.initArgs.linesStartAt1 && *ctx.initArgs.linesStartAt1 ? 1 : 0);

						breakpoint.verified = true;
					}
					else
					{
						breakpoint.message = nullcGetLastError();
					}
				}
				else
				{
					breakpoint.message = "can't find instruction at the target location";
				}
			}
			else
			{
				breakpoint.message = "target source file not found in linked program";
			}

			breakpoints.push_back(breakpoint);
		}
	}

	rapidjson::Value body;
	body.SetObject();

	{
		rapidjson::Value jsonSources;
		jsonSources.SetArray();

		for(auto &&el : breakpoints)
			jsonSources.PushBack(::ToJson(el, response), response.GetAllocator());

		body.AddMember("breakpoints", jsonSources, response.GetAllocator());
	}

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", body, response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestSetFunctionBreakpoints(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	SetFunctionBreakpointsArguments args{arguments};

	std::vector<Breakpoint> breakpoints;

	// TODO: create breakpoint objects in context

	for(auto &el : args.breakpoints)
	{
		Breakpoint breakpoint;

		breakpoint.id = (int)breakpoints.size() + 1;

		breakpoint.verified = false;

		if(!el.condition && !el.hitCondition)
		{
			breakpoint.message = "not implemented";
		}
		else
		{
			breakpoint.message = "unsupported field condition/hitCondition/logMessage";
		}

		breakpoints.push_back(breakpoint);
	}

	rapidjson::Value body;
	body.SetObject();

	{
		rapidjson::Value jsonSources;
		jsonSources.SetArray();

		for(auto &&el : breakpoints)
			jsonSources.PushBack(::ToJson(el, response), response.GetAllocator());

		body.AddMember("breakpoints", jsonSources, response.GetAllocator());
	}

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", body, response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestSetExceptionBreakpoints(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	SetExceptionBreakpointsArguments args{arguments};

	response.AddMember("success", true, response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestConfigurationDone(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	(void)arguments;

	response.AddMember("success", true, response.GetAllocator());

	SendResponse(ctx, response);

	SendEventProcess(ctx);
	SendEventThread(ctx, ThreadEventData("started", 1));

	ctx.running.store(true);

	LaunchApplicationThread(ctx);

	return true;
}

bool HandleRequestLaunch(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	ctx.launchArgs = LaunchRequestArguments(arguments);

	if(ctx.nullcInitialized)
		return RespondWithError(ctx, response, "can't launch, nullc is already initialized");

	if(!ctx.launchArgs.program)
		return RespondWithError(ctx, response, "launch program argument required");

	if(ctx.launchArgs.trace)
	{
		if(*ctx.launchArgs.trace == "off")
		{
			ctx.infoMode = false;
			ctx.debugMode = false;
		}
		else if(*ctx.launchArgs.trace == "info")
		{
			ctx.infoMode = true;
			ctx.debugMode = false;
		}
		else if(*ctx.launchArgs.trace == "debug")
		{
			ctx.infoMode = true;
			ctx.debugMode = true;
		}
	}

	if(ctx.launchArgs.program && (ctx.launchArgs.program->find_last_of('/') != std::string::npos || ctx.launchArgs.program->find_last_of('\\') != std::string::npos))
	{
		auto pos = ctx.launchArgs.program->find_last_of('/') != std::string::npos ? ctx.launchArgs.program->find_last_of('/') : ctx.launchArgs.program->find_last_of('\\');

		ctx.rootPath = ctx.launchArgs.program->substr(0, pos + 1);
	}

	// Initialize nullc with target module path
	if(ctx.launchArgs.modulePath && !ctx.launchArgs.modulePath->empty())
	{
		ctx.modulePath = *ctx.launchArgs.modulePath;

		if(ctx.modulePath.back() != '/' && ctx.modulePath.back() != '\\')
			ctx.modulePath.push_back('/');

		if(ctx.debugMode)
			fprintf(stderr, "DEBUG: Launching nullc with module path '%s'\r\n", ctx.modulePath.c_str());

		if(!nullcInit())
			return RespondWithError(ctx, response, "failed to launch nullc");

		nullcAddImportPath(ctx.modulePath.c_str());

		if(ctx.launchArgs.workspaceFolder)
		{
			std::string workspacePath = *ctx.launchArgs.workspaceFolder;

			workspacePath += "/";

			nullcAddImportPath(workspacePath.c_str());
		}
	}
	else if(ctx.launchArgs.workspaceFolder)
	{
		ctx.modulePath = *ctx.launchArgs.workspaceFolder;

		ctx.modulePath += "/Modules/";

		if(ctx.debugMode)
			fprintf(stderr, "DEBUG: Launching nullc with module path '%s'\r\n", ctx.modulePath.c_str());

		if(!nullcInit())
			return RespondWithError(ctx, response, "failed to launch nullc");

		nullcAddImportPath(ctx.modulePath.c_str());

		{
			std::string workspacePath = *ctx.launchArgs.workspaceFolder;

			workspacePath += "/";

			nullcAddImportPath(workspacePath.c_str());
		}
	}
	else if(!ctx.rootPath.empty())
	{
		ctx.modulePath = ctx.rootPath;

		ctx.modulePath += "Modules/";

		if(ctx.debugMode)
			fprintf(stderr, "DEBUG: Launching nullc with module path '%s'\r\n", ctx.modulePath.c_str());

		if(!nullcInit())
			return RespondWithError(ctx, response, "failed to launch nullc");

		nullcAddImportPath(ctx.modulePath.c_str());
		nullcAddImportPath(ctx.rootPath.c_str());
	}
	else
	{
		ctx.modulePath = "";

		fprintf(stderr, "WARNING: Launching nullc without module path\r\n");

		if(!nullcInit())
			return RespondWithError(ctx, response, "failed to launch nullc");
	}

	if(!ctx.defaultModulePath.empty() && ctx.defaultModulePath != ctx.modulePath)
		nullcAddImportPath(ctx.defaultModulePath.c_str());

	if(ctx.debugMode)
		nullcSetEnableLogFiles(1, NULL, NULL, NULL);
	else
		nullcSetEnableLogFiles(0, NULL, NULL, NULL);

	nullcSetOptimizationLevel(0);

	nullcSetExecutor(NULLC_REG_VM);

	if(!nullcDebugSetBreakFunction(&ctx, OnDebugBreak))
	{
		nullcTerminate();

		return RespondWithError(ctx, response, "failed to set null debug break callback");
	}

	if(!nullcInitTypeinfoModule())
		return RespondWithError(ctx, response, "failed to init std.typeinfo module");
	if(!nullcInitDynamicModule())
		return RespondWithError(ctx, response, "failed to init std.dynamic module");

	if(!nullcInitFileModule())
		return RespondWithError(ctx, response, "failed to init std.file module");
	if(!nullcInitIOModule(&ctx, OnIoWrite, NULL))
		return RespondWithError(ctx, response, "failed to init std.io module");
	if(!nullcInitMathModule())
		return RespondWithError(ctx, response, "failed to init std.math module");
	if(!nullcInitStringModule())
		return RespondWithError(ctx, response, "failed to init std.string module");
	if(!nullcInitRandomModule())
		return RespondWithError(ctx, response, "failed to init std.random module");
	if(!nullcInitTimeModule())
		return RespondWithError(ctx, response, "failed to init std.time module");
	if(!nullcInitGCModule())
		return RespondWithError(ctx, response, "failed to init std.gc module");

	if(!nullcInitCanvasModule())
		return RespondWithError(ctx, response, "failed to init img.canvas module");

#if defined(_WIN32)
	if(!nullcInitWindowModule())
		return RespondWithError(ctx, response, "failed to init win.window module");
#endif

	if(!nullcInitPugiXMLModule())
		return RespondWithError(ctx, response, "failed to init ext.pugixml module");

	// Get program source
	FILE *fIn = fopen(ctx.launchArgs.program->c_str(), "rb");

	if(!fIn)
	{
		nullcTerminate();

		return RespondWithError(ctx, response, "failed to open source file");
	}

	fseek(fIn, 0, SEEK_END);
	unsigned textSize = ftell(fIn);
	fseek(fIn, 0, SEEK_SET);
	char *source = new char[textSize + 1];
	fread(source, 1, textSize, fIn);
	source[textSize] = 0;
	fclose(fIn);

	// Compile nullc program
	if(!nullcBuild(source))
	{
		nullcTerminate();

		return RespondWithError(ctx, response, "failed to compile source file");
	}

	response.AddMember("success", true, response.GetAllocator());

	SendResponse(ctx, response);

	ctx.nullcInitialized = true;

	// Notify that we are ready to receive configuration requests
	SendEventInitialized(ctx);

	return true;
}

Source GetModuleSourceInfo(Context& ctx, unsigned moduleIndex)
{
	unsigned moduleCount = 0;
	auto modules = nullcDebugModuleInfo(&moduleCount);

	auto symbols = nullcDebugSymbols(nullptr);

	if(moduleIndex < moduleCount)
	{
		auto &moduleInfo = modules[moduleIndex];

		Source source;

		std::string name = symbols + moduleInfo.nameOffset;

		source.name = name;

		std::string moduleSourcePath = ToString("%s%s", ctx.modulePath.c_str(), name.c_str());

		if(FILE *fInModule = fopen(moduleSourcePath.c_str(), "rb"))
		{
			source.path = moduleSourcePath;

			source.presentationHint = SourcePresentationHints::deemphasize;

			source.origin = "module path";

			fclose(fInModule);
		}
		else if(ctx.launchArgs.workspaceFolder)
		{
			std::string rootSourcePath = ToString("%s/%s", ctx.launchArgs.workspaceFolder->c_str(), name.c_str());

			if(FILE *fInRoot = fopen(rootSourcePath.c_str(), "rb"))
			{
				source.path = rootSourcePath;

				source.presentationHint = SourcePresentationHints::emphasize;

				source.origin = "workspace";

				fclose(fInRoot);
			}
			else
			{
				source.sourceReference = moduleIndex;

				source.presentationHint = name[0] == '$' ? SourcePresentationHints::deemphasize : SourcePresentationHints::normal;
			}
		}
		else if(!ctx.rootPath.empty())
		{
			std::string rootSourcePath = ToString("%s%s", ctx.rootPath.c_str(), name.c_str());

			if(FILE * fIn = fopen(rootSourcePath.c_str(), "rb"))
			{
				source.path = rootSourcePath;

				source.presentationHint = SourcePresentationHints::emphasize;

				source.origin = "root";

				fclose(fIn);
			}
			else
			{
				source.sourceReference = moduleIndex;

				source.presentationHint = name[0] == '$' ? SourcePresentationHints::deemphasize : SourcePresentationHints::normal;
			}
		}
		else
		{
			source.sourceReference = moduleIndex;

			source.presentationHint = name[0] == '$' ? SourcePresentationHints::deemphasize : SourcePresentationHints::normal;
		}

		return source;
	}

	Source source;

	auto folderPos = ctx.launchArgs.program->find_last_of("/\\");

	if(folderPos != std::string::npos)
		source.name = ctx.launchArgs.program->substr(folderPos + 1);
	else
		source.name = "main.nc";

	source.path = ctx.launchArgs.program;

	source.presentationHint = SourcePresentationHints::emphasize;

	source.origin = "workspace";

	return source;
}

bool HandleRequestLoadedSources(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	(void)arguments;

	LoadedSourcesResponseData data;

	unsigned moduleCount = 0;
	nullcDebugModuleInfo(&moduleCount);

	for(unsigned i = 0; i < moduleCount; i++)
		data.sources.push_back(GetModuleSourceInfo(ctx, i));

	data.sources.push_back(GetModuleSourceInfo(ctx, ~0u));

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestSource(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	SourceArguments args{arguments};

	SourceResponseData data;

	unsigned moduleCount = 0;
	auto modules = nullcDebugModuleInfo(&moduleCount);

	auto fullSource = nullcDebugSource();

	if(args.source)
	{
		if(args.source->sourceReference)
		{
			unsigned moduleIndex = *args.source->sourceReference;

			if(moduleIndex < moduleCount)
				data.content = fullSource + modules[moduleIndex].sourceOffset;
		}
	}
	else if(unsigned(args.sourceReference) < moduleCount)
	{
		data.content = fullSource + modules[args.sourceReference].sourceOffset;
	}

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestThreads(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	(void)arguments;

	std::vector<Thread> threads;

	if(ctx.running.load())
	{
		Thread thread;

		thread.id = 1;
		thread.name = "main";

		threads.push_back(thread);
	}

	rapidjson::Value body;
	body.SetObject();

	{
		rapidjson::Value jsonSources;
		jsonSources.SetArray();

		for(auto &&el : threads)
			jsonSources.PushBack(::ToJson(el, response), response.GetAllocator());

		body.AddMember("threads", jsonSources, response.GetAllocator());
	}

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", body, response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestStackTrace(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	StackTraceArguments args{arguments};

	StackTraceResponseData data;

	auto symbols = nullcDebugSymbols(nullptr);

	unsigned functionCount = 0;
	auto functions = nullcDebugFunctionInfo(&functionCount);

	unsigned localCount = 0;
	auto locals = nullcDebugLocalInfo(&localCount);

	unsigned typeCount = 0;
	auto types = nullcDebugTypeInfo(&typeCount);

	unsigned skipFrames = args.startFrame ? *args.startFrame : 0;

	std::vector<int> stackFrames;

	nullcDebugBeginCallStack();

	while(int nextAddress = nullcDebugGetStackFrame())
		stackFrames.push_back(nextAddress - 1);

	std::reverse(stackFrames.begin(), stackFrames.end());

	for(unsigned i = 0; i < stackFrames.size(); i++)
	{
		if(skipFrames)
		{
			skipFrames--;

			continue;
		}

		unsigned frameId = (int)stackFrames.size() - i - 1;

		unsigned instruction = stackFrames[i];

		auto sourceLocation = GetInstructionSourceLocation(instruction);

		if(!sourceLocation)
		{
			fprintf(stderr, "ERROR: Failed to find location for stack frame %d\r\n", i);

			continue;
		}

		unsigned moduleIndex = GetSourceLocationModuleIndex(sourceLocation);

		auto function = nullcDebugConvertAddressToFunction(instruction, functions, functionCount);

		auto functionName = function ? symbols + function->offsetToName : "global";

		if(ctx.debugMode)
			fprintf(stderr, "INFO: Stack trace frame %d on instruction %d in function '%s'\r\n", frameId, instruction, functionName);

		StackFrame stackFrame;

		stackFrame.id = frameId;

		stackFrame.name = functionName;

		stackFrame.source = GetModuleSourceInfo(ctx, moduleIndex);

		unsigned column = 0;
		unsigned line = ConvertSourceLocationToLine(sourceLocation, moduleIndex, column);

		if(!ctx.initArgs.linesStartAt1 || *ctx.initArgs.linesStartAt1)
			line++;

		if(!ctx.initArgs.columnsStartAt1 || *ctx.initArgs.columnsStartAt1)
			column++;

		stackFrame.line = line;
		stackFrame.column = column;

		if(args.format)
		{
			if(function && args.format->parameters && *args.format->parameters)
			{
				stackFrame.name += "(";

				for(unsigned k = 0; k < function->paramCount + 1; k++)
				{
					auto& localInfo = locals[function->offsetToFirstLocal + k];

					const char* name = symbols + localInfo.offsetToName;

					if(*name == '$')
						continue;

					if(k != 0)
						stackFrame.name += ", ";

					if(args.format->parameterTypes && *args.format->parameterTypes)
					{
						const char* type = symbols + types[localInfo.type].offsetToName;

						stackFrame.name += type;

						if(args.format->parameterNames && *args.format->parameterNames)
						{
							stackFrame.name += " ";

							stackFrame.name += name;
						}
					}
					else if(args.format->parameterNames && *args.format->parameterNames)
					{
						stackFrame.name += name;
					}
				}

				stackFrame.name += ")";
			}

			if(args.format->line && *args.format->line)
			{
				stackFrame.name += ToString(" Line %u", line);
			}
		}

		if(!args.levels || *args.levels == 0 || data.stackFrames.size() < (unsigned)*args.levels)
			data.stackFrames.push_back(stackFrame);
	}

	data.totalFrames = (int)stackFrames.size();

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestScopes(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	ScopesArguments args{arguments};

	ScopesResponseData data;

	unsigned functionCount = 0;
	auto functions = nullcDebugFunctionInfo(&functionCount);

	unsigned variableCount = 0;
	auto variables = nullcDebugVariableInfo(&variableCount);

	unsigned localCount = 0;
	auto locals = nullcDebugLocalInfo(&localCount);

	auto symbols = nullcDebugSymbols(nullptr);

	unsigned skipFrames = args.frameId;

	nullcDebugBeginCallStack();

	while(int nextAddress = nullcDebugGetStackFrame())
	{
		if(skipFrames)
		{
			skipFrames--;
			continue;
		}

		auto sourceLocation = GetInstructionSourceLocation(nextAddress - 1);

		if(!sourceLocation)
		{
			fprintf(stderr, "ERROR: Failed to find location for stack frame %d\r\n", args.frameId);
			continue;
		}

		unsigned moduleIndex = GetSourceLocationModuleIndex(sourceLocation);

		if(auto function = nullcDebugConvertAddressToFunction(nextAddress, functions, functionCount))
		{
			if(unsigned count = function->paramCount + 1)
			{
				unsigned activeCount = 0;

				for(unsigned i = 0; i < count; i++)
				{
					auto &localInfo = locals[function->offsetToFirstLocal + i];

					const char *name = symbols + localInfo.offsetToName;

					if(*name != '$')
						activeCount++;
				}

				if(activeCount)
				{
					Scope scope;

					scope.name = "Arguments";

					scope.variablesReference = args.frameId * 10 + 2;

					scope.namedVariables = activeCount;

					data.scopes.push_back(scope);
				}
			}

			if(unsigned count = function->localCount - (function->paramCount + 1))
			{
				unsigned activeCount = 0;

				for(unsigned i = 0; i < count; i++)
				{
					auto &localInfo = locals[function->offsetToFirstLocal + function->paramCount + 1 + i];

					const char *name = symbols + localInfo.offsetToName;

					if(*name != '$')
						activeCount++;
				}

				if(activeCount)
				{
					Scope scope;

					scope.name = "Locals";

					scope.variablesReference = args.frameId * 10 + 3;

					scope.namedVariables = activeCount;

					data.scopes.push_back(scope);
				}
			}

			if(unsigned count = function->externCount)
			{
				unsigned activeCount = 0;

				for(unsigned i = 0; i < count; i++)
				{
					auto &localInfo = locals[function->offsetToFirstLocal + function->localCount + i];

					const char *name = symbols + localInfo.offsetToName;

					if(*name != '$')
						activeCount++;
				}

				if(activeCount)
				{
					Scope scope;

					scope.name = "Externals";

					scope.variablesReference = args.frameId * 10 + 4;

					scope.namedVariables = activeCount;

					data.scopes.push_back(scope);
				}
			}
		}
		else
		{
			unsigned activeCount = 0;

			for(unsigned i = 0; i < variableCount; i++)
			{
				const char *name = symbols + variables[i].offsetToName;

				if(*name != '$')
					activeCount++;
			}

			Scope scope;

			scope.name = "Globals";

			scope.variablesReference = args.frameId * 10 + 1;

			scope.namedVariables = activeCount;

			scope.source = GetModuleSourceInfo(ctx, moduleIndex);

			data.scopes.push_back(scope);
		}

		break;
	}

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

Variable GetVariableInfo(Context& ctx, unsigned typeIndex, const char *name, char *ptr, bool showHex)
{
	unsigned typeCount = 0;
	auto types = nullcDebugTypeInfo(&typeCount);

	auto symbols = nullcDebugSymbols(nullptr);

	Variable variable;

	variable.name = name;

	variable.value = GetBasicVariableInfo(typeIndex, ptr, showHex);

	auto &type = types[typeIndex];

	variable.type = symbols + type.offsetToName;

	variable.evaluateName = name;

	if(type.subCat == ExternTypeInfo::CAT_POINTER)
	{
		if(char *subPtr = *(char**)ptr)
		{
			variable.variablesReference = unsigned(0x40000000 | ctx.variableReferences.size());

			ctx.variableReferences.push_back(Context::VariableReference(subPtr, type.subType));

			auto &subType = types[type.subType];

			if(subType.subCat == ExternTypeInfo::CAT_CLASS && subType.type != ExternTypeInfo::TYPE_INT)
			{
				bool isExtendable = (subType.typeFlags & ExternTypeInfo::TYPE_IS_EXTENDABLE) != 0;

				variable.namedVariables = subType.memberCount + (isExtendable ? 1 : 0);
				variable.indexedVariables = 0;
			}
			else if(subType.subCat == ExternTypeInfo::CAT_ARRAY)
			{
				if(subType.arrSize == ~0u)
				{
					NULLCArray &arr = *(NULLCArray*)subPtr;

					variable.namedVariables = 1;
					variable.indexedVariables = arr.len;
				}
				else
				{
					variable.namedVariables = 1;
					variable.indexedVariables = subType.arrSize;
				}
			}
			else if(subType.subCat == ExternTypeInfo::CAT_FUNCTION)
			{
				variable.namedVariables = 2;
				variable.indexedVariables = 0;
			}
			else if(type.subCat == ExternTypeInfo::CAT_POINTER)
			{
				variable.namedVariables = 0;
				variable.indexedVariables = 0;
			}
			else
			{
				variable.namedVariables = 0;
				variable.indexedVariables = 0;
			}
		}
	}
	else if(type.subCat == ExternTypeInfo::CAT_CLASS && type.type != ExternTypeInfo::TYPE_INT)
	{
		variable.variablesReference = unsigned(0x40000000 | ctx.variableReferences.size());

		ctx.variableReferences.push_back(Context::VariableReference(ptr, typeIndex));

		bool isExtendable = (type.typeFlags & ExternTypeInfo::TYPE_IS_EXTENDABLE) != 0;

		variable.namedVariables = type.memberCount + (isExtendable ? 1 : 0);
		variable.indexedVariables = 0;
	}
	else if(type.subCat == ExternTypeInfo::CAT_ARRAY)
	{
		if(type.arrSize == ~0u)
		{
			NULLCArray &arr = *(NULLCArray*)ptr;

			variable.variablesReference = unsigned(0x40000000 | ctx.variableReferences.size());

			ctx.variableReferences.push_back(Context::VariableReference(ptr, typeIndex));

			variable.namedVariables = 1;
			variable.indexedVariables = arr.len;
		}
		else
		{
			variable.variablesReference = unsigned(0x40000000 | ctx.variableReferences.size());

			ctx.variableReferences.push_back(Context::VariableReference(ptr, typeIndex));

			variable.namedVariables = 1;
			variable.indexedVariables = type.arrSize;
		}
	}
	else if(type.subCat == ExternTypeInfo::CAT_FUNCTION)
	{
		variable.variablesReference = unsigned(0x40000000 | ctx.variableReferences.size());

		ctx.variableReferences.push_back(Context::VariableReference(ptr, typeIndex));

		variable.namedVariables = 2;
		variable.indexedVariables = 0;
	}
	else
	{
		variable.variablesReference = 0;
	}

	return variable;
}

bool HandleRequestVariables(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	VariablesArguments args{arguments};

	VariablesResponseData data;

	unsigned typeCount = 0;
	auto types = nullcDebugTypeInfo(&typeCount);

	unsigned functionCount = 0;
	auto functions = nullcDebugFunctionInfo(&functionCount);

	unsigned variableCount = 0;
	auto variables = nullcDebugVariableInfo(&variableCount);

	unsigned localCount = 0;
	auto locals = nullcDebugLocalInfo(&localCount);

	unsigned stackSize = 0;
	char *stack = (char*)nullcGetVariableData(&stackSize);

	unsigned typeExtraCount = 0;
	auto typeExtras = nullcDebugTypeExtraInfo(&typeExtraCount);

	auto symbols = nullcDebugSymbols(nullptr);

	unsigned start = args.start ? *args.start : 0;

	bool showHex = args.format && args.format->hex && *args.format->hex;

	// Stack frame references don't have their high bit set
	if((args.variablesReference & 0x40000000) == 0)
	{
		unsigned frameId = args.variablesReference / 10;
		unsigned scopeKind = args.variablesReference % 10;

		unsigned skipFrames = frameId;

		unsigned offset = 0;

		for(unsigned i = 0; i < variableCount; i++)
		{
			auto variableInfo = variables[i];

			auto &type = types[variableInfo.type];

			if(variableInfo.offset + type.size > offset)
				offset = variableInfo.offset + type.size;
		}

		nullcDebugBeginCallStack();

		while(int nextAddress = nullcDebugGetStackFrame())
		{
			if(skipFrames)
			{
				skipFrames--;

				if(auto function = nullcDebugConvertAddressToFunction(nextAddress, functions, functionCount))
				{
					// Align offset to the first variable (by 16 byte boundary)
					int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;

					offset += alignOffset;

					unsigned offsetToNextFrame = function->bytesToPop;

					for(unsigned int i = 0; i < function->localCount; i++)
					{
						auto &localInfo = locals[function->offsetToFirstLocal + i];

						if(localInfo.offset + localInfo.size > offsetToNextFrame)
							offsetToNextFrame = localInfo.offset + localInfo.size;
					}

					offset += offsetToNextFrame;
				}

				continue;
			}

			auto sourceLocation = GetInstructionSourceLocation(nextAddress - 1);

			if(!sourceLocation)
			{
				fprintf(stderr, "ERROR: Failed to find location for stack frame %d\r\n", frameId);
				continue;
			}

			if(args.filter && *args.filter == VariablesArgumentsFilters::indexed)
				break;

			if(auto function = nullcDebugConvertAddressToFunction(nextAddress, functions, functionCount))
			{
				// Align offset to the first variable (by 16 byte boundary)
				int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;

				offset += alignOffset;

				if(scopeKind == 2) // Arguments
				{
					unsigned count = function->paramCount + 1;

					std::vector<ExternLocalInfo*> filtered;

					for(unsigned i = 0; i < count; i++)
					{
						auto &localInfo = locals[function->offsetToFirstLocal + i];

						const char *name = symbols + localInfo.offsetToName;

						if(*name != '$')
							filtered.push_back(&localInfo);
					}

					for(unsigned i = start; i < filtered.size() && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
					{
						auto &localInfo = *filtered[i];

						data.variables.push_back(GetVariableInfo(ctx, localInfo.type, symbols + localInfo.offsetToName, stack + offset + localInfo.offset, showHex));
					}
				}
				else if(scopeKind == 3) // Locals
				{
					unsigned count = function->localCount - (function->paramCount + 1);

					std::vector<ExternLocalInfo*> filtered;

					for(unsigned i = 0; i < count; i++)
					{
						auto &localInfo = locals[function->offsetToFirstLocal + function->paramCount + 1 + i];

						const char *name = symbols + localInfo.offsetToName;

						if(*name != '$')
							filtered.push_back(&localInfo);
					}

					for(unsigned i = start; i < filtered.size() && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
					{
						auto &localInfo = *filtered[i];

						data.variables.push_back(GetVariableInfo(ctx, localInfo.type, symbols + localInfo.offsetToName, stack + offset + localInfo.offset, showHex));
					}
				}
				else if(scopeKind == 4) // Externals
				{
					unsigned count = function->externCount;

					std::vector<ExternLocalInfo*> filtered;

					for(unsigned i = 0; i < count; i++)
					{
						auto &localInfo = locals[function->offsetToFirstLocal + function->localCount + i];

						const char *name = symbols + localInfo.offsetToName;

						if(*name != '$')
							filtered.push_back(&localInfo);
					}

					auto &contextLocalInfo = locals[function->offsetToFirstLocal + function->paramCount];

					char *contextPtr = *(char**)(stack + offset + contextLocalInfo.offset);

					for(unsigned i = start; i < filtered.size() && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
					{
						auto &localInfo = *filtered[i];

						data.variables.push_back(GetVariableInfo(ctx, localInfo.type, symbols + localInfo.offsetToName, *(char**)(contextPtr + localInfo.offset), showHex));
					}
				}
			}
			else if(scopeKind == 1) // Globals
			{
				std::vector<ExternVarInfo*> filtered;

				for(unsigned i = 0; i < variableCount; i++)
				{
					ExternVarInfo &variableInfo = variables[i];

					const char *name = symbols + variableInfo.offsetToName;

					if(*name != '$')
						filtered.push_back(&variableInfo);
				}

				for(unsigned i = start; i < filtered.size() && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
				{
					auto variableInfo = *filtered[i];

					data.variables.push_back(GetVariableInfo(ctx, variableInfo.type, symbols + variableInfo.offsetToName, stack + variableInfo.offset, showHex));
				}
			}

			break;
		}
	}
	else
	{
		auto reference = ctx.variableReferences[args.variablesReference & ~0x40000000];

		auto &type = types[reference.type];

		if(type.subCat == ExternTypeInfo::CAT_CLASS)
		{
			bool isExtendable = (type.typeFlags & ExternTypeInfo::TYPE_IS_EXTENDABLE) != 0;

			auto &realType = isExtendable ? types[*(int*)(reference.ptr)] : type;

			const char *memberName = symbols + realType.offsetToName + (unsigned int)strlen(symbols + realType.offsetToName) + 1;

			if(!args.filter || *args.filter == VariablesArgumentsFilters::named)
			{
				unsigned memberCount = realType.memberCount + (isExtendable ? 1 : 0);

				for(unsigned i = start; i < memberCount && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
				{
					if(isExtendable && i == 0)
					{
						data.variables.push_back(GetVariableInfo(ctx, NULLC_TYPE_TYPEID, "typeid", reference.ptr, showHex));
					}
					else
					{
						auto &member = typeExtras[realType.memberOffset + i - (isExtendable ? 1 : 0)];

						data.variables.push_back(GetVariableInfo(ctx, member.type, memberName, reference.ptr + member.offset, showHex));

						memberName += (unsigned int)strlen(memberName) + 1;
					}
				}
			}
		}
		else if(type.subCat == ExternTypeInfo::CAT_ARRAY)
		{
			auto &subType = types[type.subType];

			if(type.arrSize == ~0u)
			{
				NULLCArray &arr = *(NULLCArray*)reference.ptr;

				if(!args.filter || *args.filter == VariablesArgumentsFilters::named)
					data.variables.push_back(GetVariableInfo(ctx, NULLC_TYPE_INT, "size", (char*)&arr.len, showHex));

				if(!args.filter || *args.filter == VariablesArgumentsFilters::indexed)
				{
					for(unsigned i = start; i < arr.len && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
						data.variables.push_back(GetVariableInfo(ctx, type.subType, ToString("%d", i).c_str(), arr.ptr + i * subType.size, showHex));
				}
			}
			else
			{
				if(!args.filter || *args.filter == VariablesArgumentsFilters::named)
					data.variables.push_back(GetVariableInfo(ctx, NULLC_TYPE_INT, "size", (char*)&type.arrSize, showHex));

				if(!args.filter || *args.filter == VariablesArgumentsFilters::indexed)
				{
					for(unsigned i = start; i < type.arrSize && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
						data.variables.push_back(GetVariableInfo(ctx, type.subType, ToString("%d", i).c_str(), reference.ptr + i * subType.size, showHex));
				}
			}
		}
		else if(type.subCat == ExternTypeInfo::CAT_FUNCTION)
		{
			NULLCFuncPtr &funcPtr = *(NULLCFuncPtr*)reference.ptr;

			auto &function = functions[funcPtr.id];

			if(!args.filter || *args.filter == VariablesArgumentsFilters::named)
			{
				data.variables.push_back(GetVariableInfo(ctx, NULLC_TYPE_INT, "id", (char*)&funcPtr.id, showHex));

				if(function.contextType == ~0u)
					data.variables.push_back(GetVariableInfo(ctx, NULLC_TYPE_VOID_REF, "context", (char*)&funcPtr.context, showHex));
				else
					data.variables.push_back(GetVariableInfo(ctx, function.contextType, "context", (char*)&funcPtr.context, showHex));
			}
		}
		else if(type.subCat == ExternTypeInfo::CAT_POINTER)
		{
			if(!args.filter || *args.filter == VariablesArgumentsFilters::named)
			{
				data.variables.push_back(GetVariableInfo(ctx, reference.type, "[ptr]", (char*)reference.ptr, showHex));
			}
		}
		else
		{
			if(!args.filter || *args.filter == VariablesArgumentsFilters::named)
			{
				data.variables.push_back(GetVariableInfo(ctx, reference.type, "[value]", (char*)reference.ptr, showHex));
			}
		}
	}

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestSetVariable(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	SetVariableArguments args{arguments};

	SetVariableResponseData data;

	unsigned typeCount = 0;
	auto types = nullcDebugTypeInfo(&typeCount);

	unsigned functionCount = 0;
	auto functions = nullcDebugFunctionInfo(&functionCount);

	unsigned variableCount = 0;
	auto variables = nullcDebugVariableInfo(&variableCount);

	unsigned localCount = 0;
	auto locals = nullcDebugLocalInfo(&localCount);

	unsigned stackSize = 0;
	char *stack = (char*)nullcGetVariableData(&stackSize);

	unsigned typeExtraCount = 0;
	auto typeExtras = nullcDebugTypeExtraInfo(&typeExtraCount);

	auto symbols = nullcDebugSymbols(nullptr);

	bool showHex = args.format && args.format->hex && *args.format->hex;

	bool variableSet = false;

	// Stack frame references don't have their high bit set
	if((args.variablesReference & 0x40000000) == 0)
	{
		unsigned frameId = args.variablesReference / 10;
		unsigned scopeKind = args.variablesReference % 10;

		unsigned skipFrames = frameId;

		unsigned offset = 0;

		for(unsigned i = 0; i < variableCount; i++)
		{
			auto variableInfo = variables[i];

			auto &type = types[variableInfo.type];

			if(variableInfo.offset + type.size > offset)
				offset = variableInfo.offset + type.size;
		}

		nullcDebugBeginCallStack();

		while(int nextAddress = nullcDebugGetStackFrame())
		{
			if(skipFrames)
			{
				skipFrames--;

				if(auto function = nullcDebugConvertAddressToFunction(nextAddress, functions, functionCount))
				{
					// Align offset to the first variable (by 16 byte boundary)
					int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;

					offset += alignOffset;

					unsigned offsetToNextFrame = function->bytesToPop;

					for(unsigned int i = 0; i < function->localCount; i++)
					{
						auto &localInfo = locals[function->offsetToFirstLocal + i];

						if(localInfo.offset + localInfo.size > offsetToNextFrame)
							offsetToNextFrame = localInfo.offset + localInfo.size;
					}

					offset += offsetToNextFrame;
				}

				continue;
			}

			auto sourceLocation = GetInstructionSourceLocation(nextAddress - 1);

			if(!sourceLocation)
			{
				fprintf(stderr, "ERROR: Failed to find location for stack frame %d\r\n", frameId);
				continue;
			}

			if(auto function = nullcDebugConvertAddressToFunction(nextAddress, functions, functionCount))
			{
				// Align offset to the first variable (by 16 byte boundary)
				int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;

				offset += alignOffset;

				if(scopeKind == 2) // Arguments
				{
					unsigned count = function->paramCount + 1;

					for(unsigned i = 0; i < count; i++)
					{
						auto &localInfo = locals[function->offsetToFirstLocal + i];

						if(args.name == symbols + localInfo.offsetToName)
						{
							if(SetBasicVariableValue(localInfo.type, stack + offset + localInfo.offset, args.value))
								variableSet = true;

							data.value = GetBasicVariableInfo(localInfo.type, stack + offset + localInfo.offset, showHex);
						}
					}
				}
				else if(scopeKind == 3) // Locals
				{
					unsigned count = function->localCount - (function->paramCount + 1);

					for(unsigned i = 0; i < count; i++)
					{
						auto &localInfo = locals[function->offsetToFirstLocal + function->paramCount + 1 + i];

						if(args.name == symbols + localInfo.offsetToName)
						{
							if(SetBasicVariableValue(localInfo.type, stack + offset + localInfo.offset, args.value))
								variableSet = true;

							data.value = GetBasicVariableInfo(localInfo.type, stack + offset + localInfo.offset, showHex);
						}
					}
				}
				else if(scopeKind == 4) // Externals
				{
					auto &contextLocalInfo = locals[function->offsetToFirstLocal + function->paramCount];

					char *contextPtr = *(char**)(stack + offset + contextLocalInfo.offset);

					unsigned count = function->externCount;

					for(unsigned i = 0; i < count; i++)
					{
						auto &localInfo = locals[function->offsetToFirstLocal + function->localCount + i];

						if(args.name == symbols + localInfo.offsetToName)
						{
							if(SetBasicVariableValue(localInfo.type, *(char**)(contextPtr + localInfo.offset), args.value))
								variableSet = true;

							data.value = GetBasicVariableInfo(localInfo.type, *(char**)(contextPtr + localInfo.offset), showHex);
						}
					}
				}
			}
			else if(scopeKind == 1) // Globals
			{
				for(unsigned i = 0; i < variableCount; i++)
				{
					auto &variableInfo = variables[i];

					if(args.name == symbols + variableInfo.offsetToName)
					{
						if(SetBasicVariableValue(variableInfo.type, stack + variableInfo.offset, args.value))
							variableSet = true;

						data.value = GetBasicVariableInfo(variableInfo.type, stack + variableInfo.offset, showHex);
					}
				}
			}

			break;
		}
	}
	else if(unsigned(args.variablesReference & ~0x40000000) < ctx.variableReferences.size())
	{
		auto reference = ctx.variableReferences[args.variablesReference & ~0x40000000];

		auto &type = types[reference.type];

		if(type.subCat == ExternTypeInfo::CAT_CLASS)
		{
			const char *memberName = symbols + type.offsetToName + (unsigned int)strlen(symbols + type.offsetToName) + 1;

			for(unsigned i = 0; i < type.memberCount; i++)
			{
				auto &member = typeExtras[type.memberOffset + i];

				if(args.name == memberName)
				{
					if(SetBasicVariableValue(member.type, reference.ptr + member.offset, args.value))
						variableSet = true;

					data.value = GetBasicVariableInfo(member.type, reference.ptr + member.offset, showHex);
				}

				memberName += (unsigned int)strlen(memberName) + 1;
			}
		}
		else if(type.subCat == ExternTypeInfo::CAT_ARRAY)
		{
			auto &subType = types[type.subType];

			if(type.arrSize == ~0u)
			{
				NULLCArray &arr = *(NULLCArray*)reference.ptr;

				unsigned index = strtoul(args.name.c_str(), nullptr, 10);

				if(index < unsigned(arr.len))
				{
					if(SetBasicVariableValue(type.subType, arr.ptr + index * subType.size, args.value))
						variableSet = true;

					data.value = GetBasicVariableInfo(type.subType, arr.ptr + index * subType.size, showHex);
				}
			}
			else
			{
				unsigned index = strtoul(args.name.c_str(), nullptr, 10);

				if(index < type.arrSize)
				{
					if(SetBasicVariableValue(type.subType, reference.ptr + index * subType.size, args.value))
						variableSet = true;

					data.value = GetBasicVariableInfo(type.subType, reference.ptr + index * subType.size, showHex);
				}
			}
		}
		else
		{
			if(SetBasicVariableValue(reference.type, (char*)reference.ptr, args.value))
				variableSet = true;

			data.value = GetBasicVariableInfo(reference.type, (char*)reference.ptr, showHex);
		}
	}
	else
	{
		return RespondWithError(ctx, response, "Unknown variables reference");
	}

	if(!variableSet)
		return RespondWithError(ctx, response, "Failed to set variable value");

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestContinue(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	ContinueArguments args{arguments};

	ContinueResponseData data;

	if(ctx.breakpointActive.load())
	{
		ctx.breakpointAction = NULLC_BREAK_PROCEED;

		ctx.breakpointWait.notify_one();
	}

	data.allThreadsContinued = true;

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", ::ToJson(data, response), response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestNext(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	NextArguments args{arguments};

	if(ctx.breakpointActive.load())
	{
		ctx.breakpointAction = NULLC_BREAK_STEP;

		ctx.breakpointWait.notify_one();
	}

	response.AddMember("success", true, response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestStepIn(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	StepInArguments args{arguments};

	if(ctx.breakpointActive.load())
	{
		ctx.breakpointAction = NULLC_BREAK_STEP_INTO;

		ctx.breakpointWait.notify_one();
	}

	response.AddMember("success", true, response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestStepOut(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	StepOutArguments args{arguments};

	if(ctx.breakpointActive.load())
	{
		ctx.breakpointAction = NULLC_BREAK_STEP_OUT;

		ctx.breakpointWait.notify_one();
	}

	response.AddMember("success", true, response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestRestart(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	(void)arguments;

	response.AddMember("success", true, response.GetAllocator());

	if(ctx.applicationThread.joinable())
	{
		if(ctx.debugMode)
			fprintf(stderr, "DEBUG: Stopping application\r\n");

		ctx.pendingRestart.store(true);

		if(ctx.breakpointActive.load())
		{
			ctx.breakpointAction = NULLC_BREAK_STOP;

			ctx.breakpointWait.notify_one();
		}
		else
		{
			// TODO: stop application thread if it still exists
		}

		ctx.applicationThread.join();
	}

	ctx.variableReferences.clear();
	ctx.sourceBreakpoints.clear();

	ctx.pendingRestart.store(false);

	SendEventProcess(ctx);
	SendEventThread(ctx, ThreadEventData("started", 1));

	ctx.running.store(true);

	LaunchApplicationThread(ctx);

	SendResponse(ctx, response);

	return true;
}

bool HandleRequestTerminate(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	TerminateArguments args{arguments};

	if(ctx.applicationThread.joinable())
	{
		if(ctx.debugMode)
			fprintf(stderr, "DEBUG: Stopping application\r\n");

		if(ctx.breakpointActive.load())
		{
			ctx.breakpointAction = NULLC_BREAK_STOP;

			ctx.breakpointWait.notify_one();
		}
		else
		{
			// TODO: stop application thread if it still exists
		}

		ctx.applicationThread.join();
	}

	if(ctx.nullcInitialized)
	{
		nullcTerminate();

		ctx.nullcInitialized = false;
	}

	response.AddMember("success", true, response.GetAllocator());

	SendResponse(ctx, response);

	SendEventTerminated(ctx);

	return true;
}

bool HandleRequestDisconnect(Context& ctx, rapidjson::Document &response, rapidjson::Value &arguments)
{
	DisconnectArguments args{arguments};

	if(ctx.applicationThread.joinable())
	{
		if(ctx.debugMode)
			fprintf(stderr, "DEBUG: Stopping application\r\n");

		if(ctx.breakpointActive.load())
		{
			ctx.breakpointAction = NULLC_BREAK_STOP;

			ctx.breakpointWait.notify_one();
		}
		else
		{
			// TODO: stop application thread if it still exists
		}

		ctx.applicationThread.join();
	}

	if(ctx.nullcInitialized)
	{
		nullcTerminate();

		ctx.nullcInitialized = false;
	}

	response.AddMember("success", true, response.GetAllocator());

	SendResponse(ctx, response);

	SendEventTerminated(ctx);

	// Client asked us to terminate
	return false;
}

bool HandleRequest(Context& ctx, int seq, const char *command, rapidjson::Value &arguments)
{
	rapidjson::Document response;

	PrepareResponse(ctx, response, seq, command);

	if(ctx.debugMode)
		fprintf(stderr, "DEBUG: HandleRequest(%d, '%s')\r\n", seq, command);

	if(strcmp(command, "initialize") == 0)
		return HandleRequestInitialize(ctx, response, arguments);

	if(strcmp(command, "setBreakpoints") == 0)
		return HandleRequestSetBreakpoints(ctx, response, arguments);

	if(strcmp(command, "setFunctionBreakpoints") == 0)
		return HandleRequestSetFunctionBreakpoints(ctx, response, arguments);

	if(strcmp(command, "setExceptionBreakpoints") == 0)
		return HandleRequestSetExceptionBreakpoints(ctx, response, arguments);

	if(strcmp(command, "configurationDone") == 0)
		return HandleRequestConfigurationDone(ctx, response, arguments);

	if(strcmp(command, "launch") == 0)
		return HandleRequestLaunch(ctx, response, arguments);

	if(strcmp(command, "loadedSources") == 0)
		return HandleRequestLoadedSources(ctx, response, arguments);

	if(strcmp(command, "source") == 0)
		return HandleRequestSource(ctx, response, arguments);

	if(strcmp(command, "threads") == 0)
		return HandleRequestThreads(ctx, response, arguments);

	if(strcmp(command, "stackTrace") == 0)
		return HandleRequestStackTrace(ctx, response, arguments);

	if(strcmp(command, "scopes") == 0)
		return HandleRequestScopes(ctx, response, arguments);

	if(strcmp(command, "variables") == 0)
		return HandleRequestVariables(ctx, response, arguments);

	if(strcmp(command, "setVariable") == 0)
		return HandleRequestSetVariable(ctx, response, arguments);

	if(strcmp(command, "continue") == 0)
		return HandleRequestContinue(ctx, response, arguments);

	if(strcmp(command, "next") == 0)
		return HandleRequestNext(ctx, response, arguments);

	if(strcmp(command, "stepIn") == 0)
		return HandleRequestStepIn(ctx, response, arguments);

	if(strcmp(command, "stepOut") == 0)
		return HandleRequestStepOut(ctx, response, arguments);

	if(strcmp(command, "restart") == 0)
		return HandleRequestRestart(ctx, response, arguments);

	if(strcmp(command, "terminate") == 0)
		return HandleRequestTerminate(ctx, response, arguments);

	if(strcmp(command, "disconnect") == 0)
		return HandleRequestDisconnect(ctx, response, arguments);

	return RespondWithError(ctx, response, "not implemented");
}

bool HandleResponseSuccess(Context& ctx, int seq, int requestSeq, const char *command, rapidjson::Value &body)
{
	(void)body;

	if(ctx.debugMode)
		fprintf(stderr, "DEBUG: HandleResponseSuccess(%d, %d, '%s')\r\n", seq, requestSeq, command);

	return true;
}

bool HandleResponseFailure(Context& ctx, int seq, int requestSeq, const char *command, rapidjson::Value &message)
{
	if(ctx.debugMode)
		fprintf(stderr, "DEBUG: HandleResponseFailure(%d, %d, '%s', '%s')\r\n", seq, requestSeq, command, message.IsString() ? message.GetString() : "no info");

	return true;
}

bool HandleMessage(Context& ctx, char *json, unsigned length)
{
	(void)length;

	rapidjson::Document doc;

	rapidjson::ParseResult ok = doc.ParseInsitu(json);

	if(!ok)
	{
		fprintf(stderr, "ERROR: Failed to parse message: %s (%d)\r\n", rapidjson::GetParseError_En(ok.Code()), (int)ok.Offset());
		return false;
	}

	if(!doc.HasMember("seq"))
	{
		fprintf(stderr, "ERROR: Message must have 'seq' member\r\n");
		return false;
	}

	auto seq = doc["seq"].GetInt();

	if(!doc.HasMember("type"))
	{
		fprintf(stderr, "ERROR: Message must have 'type' member\r\n");
		return false;
	}

	auto type = doc["type"].GetString();

	rapidjson::Value null;

	if(strcmp(type, "request") == 0)
	{
		if(!doc.HasMember("command"))
		{
			fprintf(stderr, "ERROR: Request must have 'command' member\r\n");
			return false;
		}
		
		auto command = doc["command"].GetString();
		auto &arguments = doc.HasMember("arguments") ? doc["arguments"] : null;

		return HandleRequest(ctx, seq, command, arguments);
	}

	if(strcmp(type, "response") == 0)
	{
		if(!doc.HasMember("request_seq"))
		{
			fprintf(stderr, "ERROR: Request must have 'request_seq' member\r\n");
			return false;
		}

		if(!doc.HasMember("success"))
		{
			fprintf(stderr, "ERROR: Request must have 'success' member\r\n");
			return false;
		}

		if(!doc.HasMember("command"))
		{
			fprintf(stderr, "ERROR: Request must have 'command' member\r\n");
			return false;
		}

		auto requestSeq = doc["request_seq"].GetInt();
		auto command = doc["command"].GetString();

		if(doc["success"].GetBool())
		{
			rapidjson::Value &body = doc.HasMember("body") ? doc["body"] : null;

			return HandleResponseSuccess(ctx, seq, requestSeq, command, body);
		}

		rapidjson::Value &message = doc.HasMember("message") ? doc["message"] : null;

		return HandleResponseFailure(ctx, seq, requestSeq, command, message);
	}

	fprintf(stderr, "ERROR: Unknown message '%d' type '%s'\r\n", seq, type);
	return false;
}
