#include "handler.h"

#include <stdio.h>

#include "external/rapidjson/document.h"
#include "external/rapidjson/prettywriter.h"
#include "external/rapidjson/error/en.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullc_debug.h"

// NULLC modules
#include "../../NULLC/includes/file.h"
#include "../../NULLC/includes/io.h"
#include "../../NULLC/includes/math.h"
#include "../../NULLC/includes/string.h"
#include "../../NULLC/includes/vector.h"
#include "../../NULLC/includes/list.h"
#include "../../NULLC/includes/map.h"
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
	capabilities.supportsTerminateThreadsRequest = true;
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

	// TODO: create breakpoint objects in context

	const char *moduleSource = GetModuleSourceCode(ctx, args.source);

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

	// Initialize nullc with target module path
	if(ctx.launchArgs.modulePath)
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
	}
	else
	{
		ctx.modulePath = "";

		fprintf(stderr, "WARNING: Launching nullc without module path\r\n");

		if(!nullcInit())
			return RespondWithError(ctx, response, "failed to launch nullc");
	}

	if(ctx.debugMode)
		nullcSetEnableLogFiles(1, NULL, NULL, NULL);
	else
		nullcSetEnableLogFiles(0, NULL, NULL, NULL);

	nullcSetExecutor(NULLC_VM);

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
	if(!nullcInitIOModule())
		return RespondWithError(ctx, response, "failed to init std.io module");
	if(!nullcInitMathModule())
		return RespondWithError(ctx, response, "failed to init std.math module");
	if(!nullcInitStringModule())
		return RespondWithError(ctx, response, "failed to init std.string module");
	if(!nullcInitMapModule())
		return RespondWithError(ctx, response, "failed to init std.map module");
	if(!nullcInitRandomModule())
		return RespondWithError(ctx, response, "failed to init std.random module");
	if(!nullcInitTimeModule())
		return RespondWithError(ctx, response, "failed to init std.time module");
	if(!nullcInitGCModule())
		return RespondWithError(ctx, response, "failed to init std.gc module");

	if(!nullcInitCanvasModule())
		return RespondWithError(ctx, response, "failed to init img.canvas module");
	if(!nullcInitWindowModule())
		return RespondWithError(ctx, response, "failed to init win.window module");

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

		if(FILE *fIn = fopen(moduleSourcePath.c_str(), "rb"))
		{
			source.path = moduleSourcePath;

			source.presentationHint = SourcePresentationHints::deemphasize;

			source.origin = "module path";

			fclose(fIn);
		}
		else if(ctx.launchArgs.workspaceFolder)
		{
			std::string rootSourcePath = ToString("%s%s", ctx.launchArgs.workspaceFolder->c_str(), name.c_str());

			if(FILE *fIn = fopen(rootSourcePath.c_str(), "rb"))
			{
				source.path = moduleSourcePath;

				source.presentationHint = SourcePresentationHints::emphasize;

				source.origin = "workspace";

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

	std::vector<Source> sources;

	unsigned moduleCount = 0;
	nullcDebugModuleInfo(&moduleCount);

	for(unsigned i = 0; i < moduleCount; i++)
		sources.push_back(GetModuleSourceInfo(ctx, i));

	sources.push_back(GetModuleSourceInfo(ctx, ~0u));

	rapidjson::Value body;
	body.SetObject();

	{
		rapidjson::Value jsonSources;
		jsonSources.SetArray();

		for(auto &&el : sources)
			jsonSources.PushBack(::ToJson(el, response), response.GetAllocator());

		body.AddMember("sources", jsonSources, response.GetAllocator());
	}

	response.AddMember("success", true, response.GetAllocator());
	response.AddMember("body", body, response.GetAllocator());

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
	nullcDebugVariableInfo(&variableCount);

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
			if(function->paramCount != 0)
			{
				Scope scope;

				scope.name = "Arguments";

				scope.variablesReference = args.frameId * 10 + 2;

				scope.namedVariables = function->paramCount + 1;

				data.scopes.push_back(scope);
			}

			if(function->localCount - (function->paramCount + 1) - function->externCount != 0)
			{
				Scope scope;

				scope.name = "Locals";

				scope.variablesReference = args.frameId * 10 + 3;

				scope.namedVariables = function->localCount - (function->paramCount + 1) - function->externCount;

				data.scopes.push_back(scope);
			}

			if(function->externCount != 0)
			{
				Scope scope;

				scope.name = "Externals";

				scope.variablesReference = args.frameId * 10 + 4;

				scope.namedVariables = function->externCount;

				data.scopes.push_back(scope);
			}
		}
		else
		{
			Scope scope;

			scope.name = "Globals";

			scope.variablesReference = args.frameId * 10 + 1;

			scope.namedVariables = variableCount;

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

Variable GetVariableInfo(unsigned typeIndex, const char *name, char *ptr, bool showHex)
{
	unsigned typeCount = 0;
	auto types = nullcDebugTypeInfo(&typeCount);

	auto symbols = nullcDebugSymbols(nullptr);

	Variable variable;

	variable.name = name;

	variable.value = GetBasicVariableInfo(typeIndex, ptr, showHex);

	variable.type = symbols + types[typeIndex].offsetToName;

	variable.evaluateName = name;

	// TODO: nested types
	variable.variablesReference = 0;

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

	auto symbols = nullcDebugSymbols(nullptr);

	// Stack frame references don't have their high bit set
	if((args.variablesReference & 0x80000000) == 0)
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

			unsigned start = args.start ? *args.start : 0;

			bool showHex = args.format && args.format->hex && *args.format->hex;

			if(auto function = nullcDebugConvertAddressToFunction(nextAddress, functions, functionCount))
			{
				// Align offset to the first variable (by 16 byte boundary)
				int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;

				offset += alignOffset;

				if(scopeKind == 2) // Arguments
				{
					unsigned count = function->paramCount + 1;

					for(unsigned i = start; i < count && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
					{
						auto localinfo = locals[function->offsetToFirstLocal + i];

						data.variables.push_back(GetVariableInfo(localinfo.type, symbols + localinfo.offsetToName, stack + offset + localinfo.offset, showHex));
					}
				}
				else if(scopeKind == 3) // Locals
				{
					unsigned count = function->localCount - (function->paramCount + 1) - function->externCount;

					for(unsigned i = start; i < count && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
					{
						auto localinfo = locals[function->offsetToFirstLocal + function->paramCount + 1 + i];

						data.variables.push_back(GetVariableInfo(localinfo.type, symbols + localinfo.offsetToName, stack + offset + localinfo.offset, showHex));
					}
				}
				else if(scopeKind == 4) // Externals
				{
					unsigned count = function->externCount;

					for(unsigned i = start; i < count && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
					{
						auto localinfo = locals[function->offsetToFirstLocal + function->localCount - function->externCount + i];

						data.variables.push_back(GetVariableInfo(localinfo.type, symbols + localinfo.offsetToName, stack + offset + localinfo.offset, showHex));
					}
				}
			}
			else if(scopeKind == 1) // Globals
			{
				for(unsigned i = args.start ? *args.start : 0; i < variableCount && (!args.count || data.variables.size() < (unsigned)*args.count); i++)
				{
					auto variableInfo = variables[i];

					data.variables.push_back(GetVariableInfo(variableInfo.type, symbols + variableInfo.offsetToName, stack + variableInfo.offset, showHex));
				}
			}

			break;
		}
	}
	else
	{
	}

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

	if(strcmp(command, "threads") == 0)
		return HandleRequestThreads(ctx, response, arguments);

	if(strcmp(command, "stackTrace") == 0)
		return HandleRequestStackTrace(ctx, response, arguments);

	if(strcmp(command, "scopes") == 0)
		return HandleRequestScopes(ctx, response, arguments);

	if(strcmp(command, "variables") == 0)
		return HandleRequestVariables(ctx, response, arguments);

	if(strcmp(command, "continue") == 0)
		return HandleRequestContinue(ctx, response, arguments);

	if(strcmp(command, "next") == 0)
		return HandleRequestNext(ctx, response, arguments);

	if(strcmp(command, "stepIn") == 0)
		return HandleRequestStepIn(ctx, response, arguments);

	if(strcmp(command, "stepOut") == 0)
		return HandleRequestStepOut(ctx, response, arguments);

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
