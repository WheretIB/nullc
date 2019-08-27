#pragma once

#include <assert.h>

#define RAPIDJSON_HAS_STDSTRING 1

#include "rapidjson/document.h"

#include <string>
#include <vector>

// Simple optional that doesn't avoid value initialization
template<typename T>
struct Optional
{
	Optional() = default;

	Optional(const T& value): active(true), value(value)
	{
	}

	Optional& operator=(const T& rhs)
	{
		active = true;
		value = rhs;

		return *this;
	}

	T* operator ->()
	{
		assert(active);

		return &value;
	}

	const T* operator ->() const
	{
		assert(active);

		return &value;
	}

	T& operator *()
	{
		assert(active);

		return value;
	}

	const T& operator *() const
	{
		assert(active);

		return value;
	}

	explicit operator bool() const
	{
		return active;
	}

	bool active = false;
	T value;
};

inline rapidjson::Value ToJson(const bool &el, rapidjson::Document &document)
{
	(void)document;

	rapidjson::Value result;

	result.SetBool(el);

	return result;
}

inline rapidjson::Value ToJson(const int &el, rapidjson::Document &document)
{
	(void)document;

	rapidjson::Value result;

	result.SetInt(el);

	return result;
}

inline rapidjson::Value ToJson(const std::string &el, rapidjson::Document &document)
{
	rapidjson::Value result;

	result.SetString(el, document.GetAllocator());

	return result;
}

template<typename T>
rapidjson::Value ToJson(const T &el, rapidjson::Document &document)
{
	rapidjson::Value result;

	el.SaveTo(result, document);

	return result;
}

template<typename T>
rapidjson::Value ToJson(const std::vector<T> &arr, rapidjson::Document &document)
{
	rapidjson::Value result;
	result.SetArray();

	for(auto &&el : arr)
		result.PushBack(ToJson(el, document), document.GetAllocator());

	return result;
}

inline void FromJson(bool &el, rapidjson::Value &source)
{
	el = source.GetBool();
}

inline void FromJson(int &el, rapidjson::Value &source)
{
	el = source.GetInt();
}

inline void FromJson(std::string &el, rapidjson::Value &source)
{
	el = source.GetString();
}

template<typename T>
void FromJson(T &el, rapidjson::Value &source)
{
	el = T(source);
}

template<typename T>
void FromJson(std::vector<T> &target, rapidjson::Value &source)
{
	target.clear();

	for(auto &&el : source.GetArray())
	{
		T elem;

		FromJson(elem, el);

		target.push_back(elem);
	}
}

template<typename T>
void FromJson(Optional<T> &target, rapidjson::Value &source)
{
	T elem;

	FromJson(elem, source);

	target = elem;
}

namespace ColumnDescriptorTypes
{
	static const char *string = "string";
	static const char *number = "number";
	static const char *boolean = "boolean";
	static const char *timestamp = "unixTimestampUTC";
}

using ColumnDescriptorType = std::string;

namespace ChecksumAlgorithms
{
	static const char *md5 = "MD5";
	static const char *sha1 = "SHA1";
	static const char *sha256 = "SHA256";
	static const char *timestamp = "timestamp";
}

using ChecksumAlgorithm = std::string;

namespace SourcePresentationHints
{
	static const char *normal = "normal";
	static const char *emphasize = "emphasize";
	static const char *deemphasize = "deemphasize";
}

using SourcePresentationHint = std::string;

namespace ExceptionBreakModes
{
	static const char *never = "never";
	static const char *always = "always";
	static const char *unhandled = "unhandled";
	static const char *userUnhandled = "userUnhandled";
}

using ExceptionBreakMode = std::string;

namespace StackFramePresentationHints
{
	static const char *normal = "normal";
	static const char *label = "label";
	static const char *subtle = "subtle";
}

using StackFramePresentationHint = std::string;

namespace VariablesArgumentsFilters
{
	static const char *indexed = "indexed";
	static const char *named = "named";
}

using VariablesArgumentsFilter = std::string;

namespace VariableKinds
{
	static const char *property_ = "property";
	static const char *method = "method";
	static const char *class_ = "class";
	static const char *data = "data";
	static const char *event = "event";
	static const char *baseClass = "baseClass";
	static const char *innerClass = "innerClass";
	static const char *interface_ = "interface";
	static const char *mostDerivedClass = "mostDerivedClass";
	static const char *virtual_ = "virtual";
	static const char *dataBreakpoint = "dataBreakpoint";
}

using VariableKind = std::string;

namespace VariableAttributes
{
	static const char *static_ = "static";
	static const char *constant_ = "constant";
	static const char *readOnly = "readOnly";
	static const char *rawString = "rawString";
	static const char *hasObjectId = "hasObjectId";
	static const char *canHaveObjectId = "canHaveObjectId";
	static const char *hasSideEffects = "hasSideEffects";
}

using VariableAttribute = std::string;

/**
* Visibility of variable. Before introducing additional values, try to use the listed values.
* Values: 'public', 'private', 'protected', 'internal', 'final', etc.
*/

namespace VariableVisibilities
{
	static const char *public_ = "public";
	static const char *private_ = "private";
	static const char *protected_ = "protected";
	static const char *internal_ = "internal";
	static const char *final_ = "final";
}

using VariableVisibility = std::string;

struct InitializeRequestArguments
{
	InitializeRequestArguments() = default;

	InitializeRequestArguments(rapidjson::Value &json)
	{
		if(json.HasMember("clientID"))
			clientID = json["clientID"].GetString();

		if(json.HasMember("clientName"))
			clientName = json["clientName"].GetString();

		adapterID = json["adapterID"].GetString();

		if(json.HasMember("locale"))
			locale = json["locale"].GetString();

		if(json.HasMember("linesStartAt1"))
			linesStartAt1 = json["linesStartAt1"].GetBool();

		if(json.HasMember("columnsStartAt1"))
			columnsStartAt1 = json["columnsStartAt1"].GetBool();

		if(json.HasMember("pathFormat"))
			pathFormat = json["pathFormat"].GetString();

		if(json.HasMember("supportsVariableType"))
			supportsVariableType = json["supportsVariableType"].GetBool();

		if(json.HasMember("supportsVariablePaging"))
			supportsVariablePaging = json["supportsVariablePaging"].GetBool();

		if(json.HasMember("supportsRunInTerminalRequest"))
			supportsRunInTerminalRequest = json["supportsRunInTerminalRequest"].GetBool();
	}

	/**
	* The ID of the (frontend) client using this adapter.
	*/
	Optional<std::string> clientID;

	/**
	* The human readable name of the (frontend) client using this adapter.
	*/
	Optional<std::string> clientName;

	/**
	* The ID of the debug adapter.
	*/
	std::string adapterID;

	/**
	* The ISO-639 locale of the (frontend) client using this adapter, e.g. en-US or de-CH.
	*/
	Optional<std::string> locale;

	/**
	* If true all line numbers are 1-based (default).
	*/
	Optional<bool> linesStartAt1;

	/**
	* If true all column numbers are 1-based (default).
	*/
	Optional<bool> columnsStartAt1;

	/**
	* Determines in what format paths are specified. The default is 'path', which is the native format.
	* Values: 'path', 'uri', etc.
	*/
	Optional<std::string> pathFormat;

	/**
	* Client supports the optional type attribute for variables.
	*/
	Optional<bool> supportsVariableType;

	/**
	* Client supports the paging of variables.
	*/
	Optional<bool> supportsVariablePaging;

	/**
	* Client supports the runInTerminal request.
	*/
	Optional<bool> supportsRunInTerminalRequest;
};

struct ExceptionBreakpointsFilter
{
	ExceptionBreakpointsFilter() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("filter", filter, document.GetAllocator());
		target.AddMember("label", label, document.GetAllocator());

		if(default_)
			target.AddMember("default", *default_, document.GetAllocator());
	}

	/**
	* The internal ID of the filter. This value is passed to the setExceptionBreakpoints request.
	*/
	std::string filter;

	/**
	* The name of the filter. This will be shown in the UI.
	*/
	std::string label;

	/**
	* Initial value of the filter. If not specified a value 'false' is assumed.
	*/
	Optional<bool> default_;
};

struct ColumnDescriptor
{
	ColumnDescriptor() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("attributeName", attributeName, document.GetAllocator());
		target.AddMember("label", label, document.GetAllocator());

		if(format)
			target.AddMember("format", *format, document.GetAllocator());

		if(type)
			target.AddMember("type", *type, document.GetAllocator());

		if(width)
			target.AddMember("width", *width, document.GetAllocator());
	}

	/**
	* Name of the attribute rendered in this column.
	*/
	std::string attributeName;

	/**
	* Header UI label of column.
	*/
	std::string label;

	/**
	* Format to use for the rendered values in this column. TBD how the format strings looks like.
	*/
	Optional<std::string> format;

	/**
	* Datatype of values in this column.  Defaults to 'string' if not specified.
	*/
	Optional<ColumnDescriptorType> type;

	/**
	* Width of this column in characters (hint only).
	*/
	Optional<int> width;
};

struct Capabilities
{
	Capabilities() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		if(supportsConfigurationDoneRequest)
			target.AddMember("supportsConfigurationDoneRequest", *supportsConfigurationDoneRequest, document.GetAllocator());

		if(supportsFunctionBreakpoints)
			target.AddMember("supportsFunctionBreakpoints", *supportsFunctionBreakpoints, document.GetAllocator());

		if(supportsConditionalBreakpoints)
			target.AddMember("supportsConditionalBreakpoints", *supportsConditionalBreakpoints, document.GetAllocator());

		if(supportsHitConditionalBreakpoints)
			target.AddMember("supportsHitConditionalBreakpoints", *supportsHitConditionalBreakpoints, document.GetAllocator());

		if(supportsEvaluateForHovers)
			target.AddMember("supportsEvaluateForHovers", *supportsEvaluateForHovers, document.GetAllocator());

		if(exceptionBreakpointFilters)
			target.AddMember("exceptionBreakpointFilters", ::ToJson(*exceptionBreakpointFilters, document), document.GetAllocator());

		if(supportsStepBack)
			target.AddMember("supportsStepBack", *supportsStepBack, document.GetAllocator());

		if(supportsSetVariable)
			target.AddMember("supportsSetVariable", *supportsSetVariable, document.GetAllocator());

		if(supportsRestartFrame)
			target.AddMember("supportsRestartFrame", *supportsRestartFrame, document.GetAllocator());

		if(supportsGotoTargetsRequest)
			target.AddMember("supportsGotoTargetsRequest", *supportsGotoTargetsRequest, document.GetAllocator());

		if(supportsStepInTargetsRequest)
			target.AddMember("supportsStepInTargetsRequest", *supportsStepInTargetsRequest, document.GetAllocator());

		if(supportsCompletionsRequest)
			target.AddMember("supportsCompletionsRequest", *supportsCompletionsRequest, document.GetAllocator());

		if(supportsModulesRequest)
			target.AddMember("supportsModulesRequest", *supportsModulesRequest, document.GetAllocator());

		if(additionalModuleColumns)
			target.AddMember("additionalModuleColumns", ::ToJson(*additionalModuleColumns, document), document.GetAllocator());

		if(supportedChecksumAlgorithms)
			target.AddMember("supportedChecksumAlgorithms", ::ToJson(*supportedChecksumAlgorithms, document), document.GetAllocator());

		if(supportsRestartRequest)
			target.AddMember("supportsRestartRequest", *supportsRestartRequest, document.GetAllocator());

		if(supportsExceptionOptions)
			target.AddMember("supportsExceptionOptions", *supportsExceptionOptions, document.GetAllocator());

		if(supportsValueFormattingOptions)
			target.AddMember("supportsValueFormattingOptions", *supportsValueFormattingOptions, document.GetAllocator());

		if(supportsExceptionInfoRequest)
			target.AddMember("supportsExceptionInfoRequest", *supportsExceptionInfoRequest, document.GetAllocator());

		if(supportTerminateDebuggee)
			target.AddMember("supportTerminateDebuggee", *supportTerminateDebuggee, document.GetAllocator());

		if(supportsDelayedStackTraceLoading)
			target.AddMember("supportsDelayedStackTraceLoading", *supportsDelayedStackTraceLoading, document.GetAllocator());

		if(supportsLoadedSourcesRequest)
			target.AddMember("supportsLoadedSourcesRequest", *supportsLoadedSourcesRequest, document.GetAllocator());

		if(supportsLogPoints)
			target.AddMember("supportsLogPoints", *supportsLogPoints, document.GetAllocator());

		if(supportsTerminateThreadsRequest)
			target.AddMember("supportsTerminateThreadsRequest", *supportsTerminateThreadsRequest, document.GetAllocator());

		if(supportsSetExpression)
			target.AddMember("supportsSetExpression", *supportsSetExpression, document.GetAllocator());

		if(supportsTerminateRequest)
			target.AddMember("supportsTerminateRequest", *supportsTerminateRequest, document.GetAllocator());

		if(supportsDataBreakpoints)
			target.AddMember("supportsDataBreakpoints", *supportsDataBreakpoints, document.GetAllocator());
	}

	/**
	* The debug adapter supports the 'configurationDone' request.
	*/
	Optional<bool> supportsConfigurationDoneRequest;

	/**
	* The debug adapter supports function breakpoints.
	*/
	Optional<bool> supportsFunctionBreakpoints;

	/**
	* The debug adapter supports conditional breakpoints.
	*/
	Optional<bool> supportsConditionalBreakpoints;

	/**
	* The debug adapter supports breakpoints that break execution after a specified number of hits.
	*/
	Optional<bool> supportsHitConditionalBreakpoints;

	/**
	* The debug adapter supports a (side effect free) evaluate request for data hovers.
	*/
	Optional<bool> supportsEvaluateForHovers;

	/**
	* Available filters or options for the setExceptionBreakpoints request.
	*/
	Optional<std::vector<ExceptionBreakpointsFilter>> exceptionBreakpointFilters;

	/**
	* The debug adapter supports stepping back via the 'stepBack' and 'reverseContinue' requests.
	*/
	Optional<bool> supportsStepBack;

	/**
	* The debug adapter supports setting a variable to a value.
	*/
	Optional<bool> supportsSetVariable;

	/**
	* The debug adapter supports restarting a frame.
	*/
	Optional<bool> supportsRestartFrame;

	/**
	* The debug adapter supports the 'gotoTargets' request.
	*/
	Optional<bool> supportsGotoTargetsRequest;

	/**
	* The debug adapter supports the 'stepInTargets' request.
	*/
	Optional<bool> supportsStepInTargetsRequest;

	/**
	* The debug adapter supports the 'completions' request.
	*/
	Optional<bool> supportsCompletionsRequest;

	/**
	* The debug adapter supports the 'modules' request.
	*/
	Optional<bool> supportsModulesRequest;

	/**
	* The set of additional module information exposed by the debug adapter.
	*/
	Optional<std::vector<ColumnDescriptor>> additionalModuleColumns;

	/**
	* Checksum algorithms supported by the debug adapter.
	*/
	Optional<std::vector<ChecksumAlgorithm>> supportedChecksumAlgorithms;

	/**
	* The debug adapter supports the 'restart' request. In this case a client should not implement 'restart' by terminating and relaunching the adapter but by calling the RestartRequest.
	*/
	Optional<bool> supportsRestartRequest;

	/**
	* The debug adapter supports 'exceptionOptions' on the setExceptionBreakpoints request.
	*/
	Optional<bool> supportsExceptionOptions;

	/**
	* The debug adapter supports a 'format' attribute on the stackTraceRequest, variablesRequest, and evaluateRequest.
	*/
	Optional<bool> supportsValueFormattingOptions;

	/**
	* The debug adapter supports the 'exceptionInfo' request.
	*/
	Optional<bool> supportsExceptionInfoRequest;

	/**
	* The debug adapter supports the 'terminateDebuggee' attribute on the 'disconnect' request.
	*/
	Optional<bool> supportTerminateDebuggee;

	/**
	* The debug adapter supports the delayed loading of parts of the stack, which requires that both the 'startFrame' and 'levels' arguments and the 'totalFrames' result of the 'StackTrace' request are supported.
	*/
	Optional<bool> supportsDelayedStackTraceLoading;

	/**
	* The debug adapter supports the 'loadedSources' request.
	*/
	Optional<bool> supportsLoadedSourcesRequest;

	/**
	* The debug adapter supports logpoints by interpreting the 'logMessage' attribute of the SourceBreakpoint.
	*/
	Optional<bool> supportsLogPoints;

	/**
	* The debug adapter supports the 'terminateThreads' request.
	*/
	Optional<bool> supportsTerminateThreadsRequest;

	/**
	* The debug adapter supports the 'setExpression' request.
	*/
	Optional<bool> supportsSetExpression;

	/**
	* The debug adapter supports the 'terminate' request.
	*/
	Optional<bool> supportsTerminateRequest;

	/**
	* The debug adapter supports data breakpoints.
	*/
	Optional<bool> supportsDataBreakpoints;
};

struct NullcRestartData
{
	NullcRestartData() = default;

	NullcRestartData(rapidjson::Value &json)
	{
		(void)json;
	}
};

struct LaunchRequestArguments
{
	LaunchRequestArguments() = default;

	LaunchRequestArguments(rapidjson::Value &json)
	{
		if(json.HasMember("noDebug"))
			noDebug = json["noDebug"].GetBool();

		if(json.HasMember("type"))
			type = json["type"].GetString();

		if(json.HasMember("name"))
			name = json["name"].GetString();

		if(json.HasMember("request"))
			request = json["request"].GetString();

		if(json.HasMember("program"))
			program = json["program"].GetString();

		if(json.HasMember("trace"))
			trace = json["trace"].GetString();

		if(json.HasMember("workspaceFolder"))
			workspaceFolder = json["workspaceFolder"].GetString();

		if(json.HasMember("modulePath"))
			modulePath = json["modulePath"].GetString();

		if(json.HasMember("__sessionId"))
			xxsessionId = json["__sessionId"].GetString();

		if(json.HasMember("__restart"))
			xxrestart = NullcRestartData(json["__restart"]);
	}

	/**
	* If noDebug is true the launch request should launch the program without enabling debugging.
	*/
	Optional<bool> noDebug;

	Optional<std::string> type;

	Optional<std::string> name;

	Optional<std::string> request;

	Optional<std::string> program;

	Optional<std::string> trace;

	Optional<std::string> workspaceFolder;

	Optional<std::string> modulePath;

	Optional<std::string> xxsessionId;

	/**
	* Optional data from the previous, restarted session.
	* The data is sent as the 'restart' attribute of the 'terminated' event.
	* The client should leave the data intact.
	*/
	Optional<NullcRestartData> xxrestart;
};

struct NullcSourceData
{
	NullcSourceData() = default;

	NullcSourceData(rapidjson::Value &json)
	{
		(void)json;
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		(void)document;
	}
};

struct Checksum
{
	Checksum() = default;

	Checksum(rapidjson::Value &json)
	{
		FromJson(algorithm, json["algorithm"]);
		FromJson(checksum, json["checksum"]);
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("algorithm", algorithm, document.GetAllocator());
		target.AddMember("checksum", checksum, document.GetAllocator());
	}

	/**
	* The algorithm used to calculate this checksum.
	*/
	ChecksumAlgorithm algorithm;

	/**
	* Value of the checksum.
	*/
	std::string checksum;
};

struct Source
{
	Source() = default;

	Source(rapidjson::Value &json)
	{
		if(json.HasMember("name"))
			FromJson(name, json["name"]);

		if(json.HasMember("path"))
			FromJson(path, json["path"]);

		if(json.HasMember("sourceReference"))
			FromJson(sourceReference, json["sourceReference"]);

		if(json.HasMember("presentationHint"))
			FromJson(presentationHint, json["presentationHint"]);

		if(json.HasMember("origin"))
			FromJson(origin, json["origin"]);

		if(json.HasMember("sources"))
			FromJson(sources, json["sources"]);

		if(json.HasMember("adapterData"))
			FromJson(adapterData, json["adapterData"]);

		if(json.HasMember("checksums"))
			FromJson(checksums, json["checksums"]);
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		if(name)
			target.AddMember("name", ::ToJson(*name, document), document.GetAllocator());

		if(path)
			target.AddMember("path", ::ToJson(*path, document), document.GetAllocator());

		if(sourceReference)
			target.AddMember("sourceReference", ::ToJson(*sourceReference, document), document.GetAllocator());

		if(presentationHint)
			target.AddMember("presentationHint", ::ToJson(*presentationHint, document), document.GetAllocator());

		if(origin)
			target.AddMember("origin", ::ToJson(*origin, document), document.GetAllocator());

		if(sources)
			target.AddMember("sources", ::ToJson(*sources, document), document.GetAllocator());

		if(adapterData)
			target.AddMember("adapterData", ::ToJson(*adapterData, document), document.GetAllocator());

		if(checksums)
			target.AddMember("checksums", ::ToJson(*checksums, document), document.GetAllocator());
	}

	/**
	* The short name of the source. Every source returned from the debug adapter has a name. When sending a source to the debug adapter this name is optional.
	*/
	Optional<std::string> name;

	/**
	* The path of the source to be shown in the UI. It is only used to locate and load the content of the source if no sourceReference is specified (or its value is 0).
	*/
	Optional<std::string> path;

	/**
	* If sourceReference > 0 the contents of the source must be retrieved through the SourceRequest (even if a path is specified). A sourceReference is only valid for a session, so it must not be used to persist a source.
	*/
	Optional<int> sourceReference;

	/**
	* An optional hint for how to present the source in the UI. A value of 'deemphasize' can be used to indicate that the source is not available or that it is skipped on stepping.
	*/
	Optional<SourcePresentationHint> presentationHint;

	/**
	* The (optional) origin of this source: possible values 'internal module', 'inlined content from source map', etc.
	*/
	Optional<std::string> origin;

	/**
	* An optional list of sources that are related to this source. These may be the source that generated this source.
	*/
	Optional<std::vector<Source>> sources;

	/**
	* Optional data that a debug adapter might want to loop through the client. The client should leave the data intact and persist it across sessions. The client should not interpret the data.
	*/
	Optional<NullcSourceData> adapterData;

	/**
	* The checksums associated with this file.
	*/
	Optional<std::vector<Checksum>> checksums;
};

struct TerminateArguments
{
	TerminateArguments() = default;

	TerminateArguments(rapidjson::Value &json)
	{
		if(json.HasMember("restart"))
			restart = json["restart"].GetBool();
	}

	/**
	* A value of true indicates that this 'terminate' request is part of a restart sequence.
	*/
	Optional<bool> restart;
};

struct DisconnectArguments
{
	DisconnectArguments() = default;

	DisconnectArguments(rapidjson::Value &json)
	{
		if(json.HasMember("restart"))
			restart = json["restart"].GetBool();

		if(json.HasMember("terminateDebuggee"))
			terminateDebuggee = json["terminateDebuggee"].GetBool();
	}

	/**
	* A value of true indicates that this 'disconnect' request is part of a restart sequence.
	*/
	Optional<bool> restart;

	/**
	* Indicates whether the debuggee should be terminated when the debugger is disconnected.
	* If unspecified, the debug adapter is free to do whatever it thinks is best.
	* A client can only rely on this attribute being properly honored if a debug adapter returns true for the 'supportTerminateDebuggee' capability.
	*/
	Optional<bool> terminateDebuggee;
};

struct Thread
{
	Thread() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("id", ::ToJson(id, document), document.GetAllocator());
		target.AddMember("name", ::ToJson(name, document), document.GetAllocator());
	}

	/**
	* Unique identifier for the thread.
	*/
	int id;

	/**
	* A name of the thread.
	*/
	std::string name;
};

struct SourceBreakpoint
{
	SourceBreakpoint() = default;

	SourceBreakpoint(rapidjson::Value &json)
	{
		line = json["line"].GetInt();

		if(json.HasMember("column"))
			column = json["column"].GetInt();

		if(json.HasMember("condition"))
			condition = json["condition"].GetString();

		if(json.HasMember("hitCondition"))
			hitCondition = json["hitCondition"].GetString();

		if(json.HasMember("logMessage"))
			logMessage = json["logMessage"].GetString();
	}

	/**
	* The source line of the breakpoint or logpoint.
	*/
	int line;

	/**
	* An optional source column of the breakpoint.
	*/
	Optional<int> column;

	/**
	* An optional expression for conditional breakpoints.
	*/
	Optional<std::string> condition;

	/**
	* An optional expression that controls how many hits of the breakpoint are ignored. The backend is expected to interpret the expression as needed.
	*/
	Optional<std::string> hitCondition;

	/**
	* If this attribute exists and is non-empty, the backend must not 'break' (stop) but log the message instead. Expressions within {} are interpolated.
	*/
	Optional<std::string> logMessage;
};

struct SetBreakpointsArguments
{
	SetBreakpointsArguments() = default;

	SetBreakpointsArguments(rapidjson::Value &json)
	{
		FromJson(source, json["source"]);

		if(json.HasMember("breakpoints"))
			FromJson(breakpoints, json["breakpoints"]);

		if(json.HasMember("lines"))
			FromJson(lines, json["lines"]);

		if(json.HasMember("sourceModified"))
			FromJson(sourceModified, json["sourceModified"]);
	}

	/**
	* The source location of the breakpoints; either 'source.path' or 'source.reference' must be specified.
	*/
	Source source;

	/**
	* The code locations of the breakpoints.
	*/
	Optional<std::vector<SourceBreakpoint>> breakpoints;

	/**
	* Deprecated: The code locations of the breakpoints.
	*/
	Optional<std::vector<int>> lines;

	/**
	* A value of true indicates that the underlying source has been modified which results in new breakpoint locations.
	*/
	Optional<bool> sourceModified;
};

struct Breakpoint
{
	Breakpoint() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		if(id)
			target.AddMember("id", ::ToJson(*id, document), document.GetAllocator());

		target.AddMember("verified", ::ToJson(verified, document), document.GetAllocator());

		if(message)
			target.AddMember("message", ::ToJson(*message, document), document.GetAllocator());

		if(source)
			target.AddMember("source", ::ToJson(*source, document), document.GetAllocator());

		if(line)
			target.AddMember("line", ::ToJson(*line, document), document.GetAllocator());

		if(column)
			target.AddMember("column", ::ToJson(*column, document), document.GetAllocator());

		if(endLine)
			target.AddMember("endLine", ::ToJson(*endLine, document), document.GetAllocator());

		if(endColumn)
			target.AddMember("endColumn", ::ToJson(*endColumn, document), document.GetAllocator());
	}

	/**
	* An optional identifier for the breakpoint. It is needed if breakpoint events are used to update or remove breakpoints.
	*/
	Optional<int> id;

	/**
	* If true breakpoint could be set (but not necessarily at the desired location).
	*/
	bool verified = false;

	/**
	* An optional message about the state of the breakpoint. This is shown to the user and can be used to explain why a breakpoint could not be verified.
	*/
	Optional<std::string> message;

	/**
	* The source where the breakpoint is located.
	*/
	Optional<Source> source;

	/**
	* The start line of the actual range covered by the breakpoint.
	*/
	Optional<int> line;

	/**
	* An optional start column of the actual range covered by the breakpoint.
	*/
	Optional<int> column;

	/**
	* An optional end line of the actual range covered by the breakpoint.
	*/
	Optional<int> endLine;

	/**
	* An optional end column of the actual range covered by the breakpoint. If no end line is given, then the end column is assumed to be in the start line.
	*/
	Optional<int> endColumn;
};

struct ThreadEventData
{
	ThreadEventData() = default;

	ThreadEventData(std::string reason, int threadId): reason(reason), threadId(threadId)
	{
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("reason", ::ToJson(reason, document), document.GetAllocator());
		target.AddMember("threadId", ::ToJson(threadId, document), document.GetAllocator());
	}

	/**
	* The reason for the event.
	* Values: 'started', 'exited', etc.
	*/
	std::string reason;

	/**
	* The identifier of the thread.
	*/
	int threadId;
};

struct OutputEventData
{
	OutputEventData() = default;

	OutputEventData(std::string category, std::string output): category(category), output(output)
	{
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		if(category)
			target.AddMember("category", ::ToJson(*category, document), document.GetAllocator());

		target.AddMember("output", ::ToJson(output, document), document.GetAllocator());

		if(variablesReference)
			target.AddMember("variablesReference", ::ToJson(*variablesReference, document), document.GetAllocator());

		if(source)
			target.AddMember("source", ::ToJson(*source, document), document.GetAllocator());

		if(line)
			target.AddMember("line", ::ToJson(*line, document), document.GetAllocator());

		if(column)
			target.AddMember("column", ::ToJson(*column, document), document.GetAllocator());
	}

	/**
	* The output category. If not specified, 'console' is assumed.
	* Values: 'console', 'stdout', 'stderr', 'telemetry', etc.
	*/
	Optional<std::string> category;

	/**
	* The output to report.
	*/
	std::string output;

	/**
	* If an attribute 'variablesReference' exists and its value is > 0, the output contains objects which can be retrieved by passing 'variablesReference' to the 'variables' request.
	*/
	Optional<int> variablesReference;

	/**
	* An optional source location where the output was produced.
	*/
	Optional<Source> source;

	/**
	* An optional source location line where the output was produced.
	*/
	Optional<int> line;

	/**
	* An optional source location column where the output was produced.
	*/
	Optional<int> column;

	/**
	* Optional data to report. For the 'telemetry' category the data will be sent to telemetry, for the other categories the data is shown in JSON format.
	*/
	//Optional<any> data;
};

struct StoppedEventData
{
	StoppedEventData() = default;

	StoppedEventData(std::string reason, int threadId): reason(reason), threadId(threadId)
	{
	}

	StoppedEventData(std::string reason, std::string description, int threadId, bool preserveFocusHint, std::string text, bool allThreadsStopped): reason(reason), description(description), threadId(threadId), preserveFocusHint(preserveFocusHint), text(text), allThreadsStopped(allThreadsStopped)
	{
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("reason", ::ToJson(reason, document), document.GetAllocator());

		if(description)
			target.AddMember("description", ::ToJson(*description, document), document.GetAllocator());

		if(threadId)
			target.AddMember("threadId", ::ToJson(*threadId, document), document.GetAllocator());

		if(preserveFocusHint)
			target.AddMember("preserveFocusHint", ::ToJson(*preserveFocusHint, document), document.GetAllocator());

		if(text)
			target.AddMember("text", ::ToJson(*text, document), document.GetAllocator());

		if(allThreadsStopped)
			target.AddMember("allThreadsStopped", ::ToJson(*allThreadsStopped, document), document.GetAllocator());
	}

	/**
	* The reason for the event.
	* For backward compatibility this string is shown in the UI if the 'description' attribute is missing (but it must not be translated).
	* Values: 'step', 'breakpoint', 'exception', 'pause', 'entry', 'goto', 'function breakpoint', 'data breakpoint', etc.
	*/
	std::string reason;

	/**
	* The full reason for the event, e.g. 'Paused on exception'. This string is shown in the UI as is and must be translated.
	*/
	Optional<std::string> description;

	/**
	* The thread which was stopped.
	*/
	Optional<int> threadId;

	/**
	* A value of true hints to the frontend that this event should not change the focus.
	*/
	Optional<bool> preserveFocusHint;

	/**
	* Additional information. E.g. if reason is 'exception', text contains the exception name. This string is shown in the UI.
	*/
	Optional<std::string> text;

	/**
	* If 'allThreadsStopped' is true, a debug adapter can announce that all threads have stopped.
	* - The client should use this information to enable that all threads can be expanded to access their stacktraces.
	* - If the attribute is missing or false, only the thread with the given threadId can be expanded.
	*/
	Optional<bool> allThreadsStopped;
};

struct FunctionBreakpoint
{
	FunctionBreakpoint() = default;

	FunctionBreakpoint(rapidjson::Value &json)
	{
		FromJson(name, json["name"]);

		if(json.HasMember("condition"))
			FromJson(condition, json["condition"]);

		if(json.HasMember("hitCondition"))
			FromJson(hitCondition, json["hitCondition"]);
	}

	/**
	* The name of the function.
	*/
	std::string name;

	/**
	* An optional expression for conditional breakpoints.
	*/
	Optional<std::string> condition;

	/**
	* An optional expression that controls how many hits of the breakpoint are ignored. The backend is expected to interpret the expression as needed.
	*/
	Optional<std::string> hitCondition;
};

struct SetFunctionBreakpointsArguments
{
	SetFunctionBreakpointsArguments() = default;

	SetFunctionBreakpointsArguments(rapidjson::Value &json)
	{
		FromJson(breakpoints, json["breakpoints"]);
	}

	/**
	* The function names of the breakpoints.
	*/
	std::vector<FunctionBreakpoint> breakpoints;
};

struct ExceptionPathSegment
{
	ExceptionPathSegment() = default;

	ExceptionPathSegment(rapidjson::Value &json)
	{
		if(json.HasMember("negate"))
			FromJson(negate, json["negate"]);

		FromJson(names, json["names"]);
	}

	/**
	* If false or missing this segment matches the names provided, otherwise it matches anything except the names provided.
	*/
	Optional<bool> negate;

	/**
	* Depending on the value of 'negate' the names that should match or not match.
	*/
	std::vector<std::string> names;
};

struct ExceptionOptions
{
	ExceptionOptions() = default;

	ExceptionOptions(rapidjson::Value &json)
	{
		if(json.HasMember("path"))
			FromJson(path, json["path"]);

		FromJson(breakMode, json["breakMode"]);
	}

	/**
	* A path that selects a single or multiple exceptions in a tree. If 'path' is missing, the whole tree is selected. By convention the first segment of the path is a category that is used to group exceptions in the UI.
	*/
	Optional<std::vector<ExceptionPathSegment>> path;

	/**
	* Condition when a thrown exception should result in a break.
	*/
	ExceptionBreakMode breakMode;
};

struct SetExceptionBreakpointsArguments
{
	SetExceptionBreakpointsArguments() = default;

	SetExceptionBreakpointsArguments(rapidjson::Value &json)
	{
		FromJson(filters, json["filters"]);

		if(json.HasMember("exceptionOptions"))
			FromJson(exceptionOptions, json["exceptionOptions"]);
	}

	/**
	* IDs of checked exception options. The set of IDs is returned via the 'exceptionBreakpointFilters' capability.
	*/
	std::vector<std::string> filters;

	/**
	* Configuration options for selected exceptions.
	*/
	Optional<std::vector<ExceptionOptions>> exceptionOptions;
};

struct ValueFormat
{
	ValueFormat() = default;

	ValueFormat(rapidjson::Value &json)
	{
		if(json.HasMember("hex"))
			FromJson(hex, json["hex"]);
	}

	/**
	* Display the value in hex.
	*/
	Optional<bool> hex;
};

struct StackFrameFormat: ValueFormat
{
	StackFrameFormat() = default;

	StackFrameFormat(rapidjson::Value &json): ValueFormat(json)
	{
		if(json.HasMember("parameters"))
			FromJson(parameters, json["parameters"]);

		if(json.HasMember("parameterTypes"))
			FromJson(parameterTypes, json["parameterTypes"]);

		if(json.HasMember("parameterNames"))
			FromJson(parameterNames, json["parameterNames"]);

		if(json.HasMember("parameterValues"))
			FromJson(parameterValues, json["parameterValues"]);

		if(json.HasMember("line"))
			FromJson(line, json["line"]);

		if(json.HasMember("module"))
			FromJson(module, json["module"]);

		if(json.HasMember("includeAll"))
			FromJson(includeAll, json["includeAll"]);
	}

	/**
	* Displays parameters for the stack frame.
	*/
	Optional<bool> parameters;

	/**
	* Displays the types of parameters for the stack frame.
	*/
	Optional<bool> parameterTypes;

	/**
	* Displays the names of parameters for the stack frame.
	*/
	Optional<bool> parameterNames;

	/**
	* Displays the values of parameters for the stack frame.
	*/
	Optional<bool> parameterValues;

	/**
	* Displays the line number of the stack frame.
	*/
	Optional<bool> line;

	/**
	* Displays the module of the stack frame.
	*/
	Optional<bool> module;

	/**
	* Includes all stack frames, including those the debug adapter might otherwise hide.
	*/
	Optional<bool> includeAll;
};

struct StackTraceArguments
{
	StackTraceArguments() = default;

	StackTraceArguments(rapidjson::Value &json)
	{
		FromJson(threadId, json["threadId"]);

		if(json.HasMember("startFrame"))
			FromJson(startFrame, json["startFrame"]);

		if(json.HasMember("levels"))
			FromJson(levels, json["levels"]);

		if(json.HasMember("format"))
			FromJson(format, json["format"]);
	}

	/**
	* Retrieve the stacktrace for this thread.
	*/
	int threadId;

	/**
	* The index of the first frame to return; if omitted frames start at 0.
	*/
	Optional<int> startFrame;

	/**
	* The maximum number of frames to return. If levels is not specified or 0, all frames are returned.
	*/
	Optional<int> levels;

	/**
	* Specifies details on how to format the stack frames.
	*/
	Optional<StackFrameFormat> format;
};

struct StackFrame
{
	StackFrame() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("id", ::ToJson(id, document), document.GetAllocator());
		target.AddMember("name", ::ToJson(name, document), document.GetAllocator());

		if(source)
			target.AddMember("source", ::ToJson(*source, document), document.GetAllocator());

		target.AddMember("line", ::ToJson(line, document), document.GetAllocator());
		target.AddMember("column", ::ToJson(column, document), document.GetAllocator());

		if(endLine)
			target.AddMember("endLine", ::ToJson(*endLine, document), document.GetAllocator());

		if(endColumn)
			target.AddMember("endColumn", ::ToJson(*endColumn, document), document.GetAllocator());

		if(moduleId)
			target.AddMember("moduleId", ::ToJson(*moduleId, document), document.GetAllocator());

		if(presentationHint)
			target.AddMember("presentationHint", ::ToJson(*presentationHint, document), document.GetAllocator());
	}

	/**
	* An identifier for the stack frame. It must be unique across all threads. This id can be used to retrieve the scopes of the frame with the 'scopesRequest' or to restart the execution of a stackframe.
	*/
	int id;

	/**
	* The name of the stack frame, typically a method name.
	*/
	std::string name;

	/**
	* The optional source of the frame.
	*/
	Optional<Source> source;

	/**
	* The line within the file of the frame. If source is null or doesn't exist, line is 0 and must be ignored.
	*/
	int line;

	/**
	* The column within the line. If source is null or doesn't exist, column is 0 and must be ignored.
	*/
	int column;

	/**
	* An optional end line of the range covered by the stack frame.
	*/
	Optional<int> endLine;

	/**
	* An optional end column of the range covered by the stack frame.
	*/
	Optional<int> endColumn;

	/**
	* The module associated with this frame, if any.
	*/
	Optional<int> moduleId;

	/**
	* An optional hint for how to present this frame in the UI. A value of 'label' can be used to indicate that the frame is an artificial frame that is used as a visual label or separator. A value of 'subtle' can be used to change the appearance of a frame in a 'subtle' way.
	*/
	Optional<StackFramePresentationHint> presentationHint;
};

struct StackTraceResponseData
{
	StackTraceResponseData() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("stackFrames", ::ToJson(stackFrames, document), document.GetAllocator());

		if(totalFrames)
			target.AddMember("totalFrames", ::ToJson(*totalFrames, document), document.GetAllocator());
	}

	/**
	* The frames of the stackframe. If the array has length zero, there are no stackframes available.
	* This means that there is no location information available.
	*/
	std::vector<StackFrame> stackFrames;

	/**
	* The total number of frames available.
	*/
	Optional<int> totalFrames;
};

struct ScopesArguments
{
	ScopesArguments() = default;

	ScopesArguments(rapidjson::Value &json)
	{
		FromJson(frameId, json["frameId"]);
	}

	/**
	* Retrieve the scopes for this stackframe.
	*/
	int frameId;
};

struct Scope
{
	Scope() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("name", ::ToJson(name, document), document.GetAllocator());
		target.AddMember("variablesReference", ::ToJson(variablesReference, document), document.GetAllocator());

		if(namedVariables)
			target.AddMember("namedVariables", ::ToJson(*namedVariables, document), document.GetAllocator());

		if(indexedVariables)
			target.AddMember("indexedVariables", ::ToJson(*indexedVariables, document), document.GetAllocator());

		if(expensive)
			target.AddMember("expensive", ::ToJson(*expensive, document), document.GetAllocator());

		if(source)
			target.AddMember("source", ::ToJson(*source, document), document.GetAllocator());

		if(line)
			target.AddMember("line", ::ToJson(*line, document), document.GetAllocator());

		if(column)
			target.AddMember("column", ::ToJson(*column, document), document.GetAllocator());

		if(endLine)
			target.AddMember("endLine", ::ToJson(*endLine, document), document.GetAllocator());

		if(endColumn)
			target.AddMember("endColumn", ::ToJson(*endColumn, document), document.GetAllocator());
	}

	/**
	* Name of the scope such as 'Arguments', 'Locals'.
	*/
	std::string name;

	/**
	* The variables of this scope can be retrieved by passing the value of variablesReference to the VariablesRequest.
	*/
	int variablesReference = 0;

	/**
	* The number of named variables in this scope.
	* The client can use this optional information to present the variables in a paged UI and fetch them in chunks.
	*/
	Optional<int> namedVariables;

	/**
	* The number of indexed variables in this scope.
	* The client can use this optional information to present the variables in a paged UI and fetch them in chunks.
	*/
	Optional<int> indexedVariables;

	/**
	* If true, the number of variables in this scope is large or expensive to retrieve.
	*/
	Optional<bool> expensive;

	/**
	* Optional source for this scope.
	*/
	Optional<Source> source;

	/**
	* Optional start line of the range covered by this scope.
	*/
	Optional<int> line;

	/**
	* Optional start column of the range covered by this scope.
	*/
	Optional<int> column;

	/**
	* Optional end line of the range covered by this scope.
	*/
	Optional<int> endLine;

	/**
	* Optional end column of the range covered by this scope.
	*/
	Optional<int> endColumn;
};

struct ScopesResponseData
{
	ScopesResponseData() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("scopes", ::ToJson(scopes, document), document.GetAllocator());
	}

	/**
	* The scopes of the stackframe. If the array has length zero, there are no scopes available.
	*/
	std::vector<Scope> scopes;
};

struct VariablesArguments
{
	VariablesArguments() = default;

	VariablesArguments(rapidjson::Value &json)
	{
		FromJson(variablesReference, json["variablesReference"]);

		if(json.HasMember("filter"))
			FromJson(filter, json["filter"]);

		if(json.HasMember("start"))
			FromJson(start, json["start"]);

		if(json.HasMember("count"))
			FromJson(count, json["count"]);

		if(json.HasMember("format"))
			FromJson(format, json["format"]);
	}

	/**
	* The Variable reference.
	*/
	int variablesReference = 0;

	/**
	* Optional filter to limit the child variables to either named or indexed. If ommited, both types are fetched.
	*/
	Optional<VariablesArgumentsFilter> filter;

	/**
	* The index of the first variable to return; if omitted children start at 0.
	*/
	Optional<int> start;

	/**
	* The number of variables to return. If count is missing or 0, all variables are returned.
	*/
	Optional<int> count;

	/**
	* Specifies details on how to format the Variable values.
	*/
	Optional<ValueFormat> format;
};

struct VariablePresentationHint
{
	VariablePresentationHint() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		if(kind)
			target.AddMember("kind", ::ToJson(*kind, document), document.GetAllocator());

		if(attributes)
			target.AddMember("attributes", ::ToJson(*attributes, document), document.GetAllocator());

		if(visibility)
			target.AddMember("visibility", ::ToJson(*visibility, document), document.GetAllocator());
	}

	/**
	* The kind of variable.
	*/
	Optional<VariableKind> kind;

	/**
	* Set of attributes represented as an array of strings.
	*/
	Optional<std::vector<VariableAttribute>> attributes;

	/**
	* Visibility of variable.
	*/
	Optional<VariableVisibility> visibility;
};

struct Variable
{
	Variable() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("name", ::ToJson(name, document), document.GetAllocator());
		target.AddMember("value", ::ToJson(value, document), document.GetAllocator());

		if(type)
			target.AddMember("type", ::ToJson(*type, document), document.GetAllocator());

		if(presentationHint)
			target.AddMember("presentationHint", ::ToJson(*presentationHint, document), document.GetAllocator());

		if(evaluateName)
			target.AddMember("evaluateName", ::ToJson(*evaluateName, document), document.GetAllocator());

		target.AddMember("variablesReference", ::ToJson(variablesReference, document), document.GetAllocator());

		if(namedVariables)
			target.AddMember("namedVariables", ::ToJson(*namedVariables, document), document.GetAllocator());

		if(indexedVariables)
			target.AddMember("indexedVariables", ::ToJson(*indexedVariables, document), document.GetAllocator());
	}

	/**
	* The variable's name.
	*/
	std::string name;

	/**
	* The variable's value. This can be a multi-line text, e.g. for a function the body of a function.
	*/
	std::string value;

	/**
	* The type of the variable's value. Typically shown in the UI when hovering over the value.
	*/
	Optional<std::string> type;

	/**
	* Properties of a variable that can be used to determine how to render the variable in the UI.
	*/
	Optional<VariablePresentationHint> presentationHint;

	/**
	* Optional evaluatable name of this variable which can be passed to the 'EvaluateRequest' to fetch the variable's value.
	*/
	Optional<std::string> evaluateName;

	/**
	* If variablesReference is > 0, the variable is structured and its children can be retrieved by passing variablesReference to the VariablesRequest.
	*/
	int variablesReference = 0;

	/**
	* The number of named child variables.
	* The client can use this optional information to present the children in a paged UI and fetch them in chunks.
	*/
	Optional<int> namedVariables;

	/**
	* The number of indexed child variables.
	* The client can use this optional information to present the children in a paged UI and fetch them in chunks.
	*/
	Optional<int> indexedVariables;
};

struct VariablesResponseData
{
	VariablesResponseData() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("variables", ::ToJson(variables, document), document.GetAllocator());
	}

	/**
	* All (or a range) of variables for the given variable reference.
	*/
	std::vector<Variable> variables;
};

struct ContinueArguments
{
	ContinueArguments() = default;

	ContinueArguments(rapidjson::Value &json)
	{
		FromJson(threadId, json["threadId"]);
	}

	/**
	* Continue execution for the specified thread (if possible). If the backend cannot continue on a single thread but will continue on all threads, it should set the 'allThreadsContinued' attribute in the response to true.
	*/
	int threadId;
};

struct ContinueResponseData
{
	ContinueResponseData() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		if(allThreadsContinued)
			target.AddMember("allThreadsContinued", ::ToJson(*allThreadsContinued, document), document.GetAllocator());
	}

	/**
	* If true, the 'continue' request has ignored the specified thread and continued all threads instead. If this attribute is missing a value of 'true' is assumed for backward compatibility.
	*/
	Optional<bool> allThreadsContinued;
};

struct NextArguments
{
	NextArguments() = default;

	NextArguments(rapidjson::Value &json)
	{
		FromJson(threadId, json["threadId"]);
	}

	/**
	* Execute 'next' for this thread.
	*/
	int threadId;
};

struct StepInArguments
{
	StepInArguments() = default;

	StepInArguments(rapidjson::Value &json)
	{
		FromJson(threadId, json["threadId"]);

		if(json.HasMember("targetId"))
			FromJson(targetId, json["targetId"]);
	}

	/**
	* Execute 'stepIn' for this thread.
	*/
	int threadId;

	/**
	* Optional id of the target to step into.
	*/
	Optional<int> targetId;
};

struct StepOutArguments
{
	StepOutArguments() = default;

	StepOutArguments(rapidjson::Value &json)
	{
		FromJson(threadId, json["threadId"]);
	}

	/**
	* Execute 'stepOut' for this thread.
	*/
	int threadId;
};

struct LoadedSourcesResponseData
{
	LoadedSourcesResponseData() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("sources", ::ToJson(sources, document), document.GetAllocator());
	}

	/**
	* Set of loaded sources.
	*/
	std::vector<Source> sources;
};

struct SourceArguments
{
	SourceArguments() = default;

	SourceArguments(rapidjson::Value &json)
	{
		if(json.HasMember("source"))
			FromJson(source, json["source"]);

		FromJson(sourceReference, json["sourceReference"]);
	}

	/**
	* Specifies the source content to load. Either source.path or source.sourceReference must be specified.
	*/
	Optional<Source> source;

	/**
	* The reference to the source. This is the same as source.sourceReference. This is provided for backward compatibility since old backends do not understand the 'source' attribute.
	*/
	int sourceReference;
};

struct SourceResponseData
{
	SourceResponseData() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("content", ::ToJson(content, document), document.GetAllocator());

		if(mimeType)
			target.AddMember("mimeType", ::ToJson(*mimeType, document), document.GetAllocator());
	}

	/**
	* Content of the source reference.
	*/
	std::string content;

	/**
	* Optional content type (mime type) of the source.
	*/
	Optional<std::string> mimeType;
};

struct SetVariableArguments
{
	SetVariableArguments() = default;

	SetVariableArguments(rapidjson::Value &json)
	{
		FromJson(variablesReference, json["variablesReference"]);
		FromJson(name, json["name"]);
		FromJson(value, json["value"]);

		if(json.HasMember("format"))
			FromJson(format, json["format"]);
	}

	/**
	* The reference of the variable container.
	*/
	int variablesReference;

	/**
	* The name of the variable in the container.
	*/
	std::string name;

	/**
	* The value of the variable.
	*/
	std::string value;

	/**
	* Specifies details on how to format the response value.
	*/
	Optional<ValueFormat> format;
};

struct SetVariableResponseData
{
	SetVariableResponseData() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("value", ::ToJson(value, document), document.GetAllocator());

		if(type)
			target.AddMember("type", ::ToJson(*type, document), document.GetAllocator());

		if(variablesReference)
			target.AddMember("variablesReference", ::ToJson(*variablesReference, document), document.GetAllocator());

		if(namedVariables)
			target.AddMember("namedVariables", ::ToJson(*namedVariables, document), document.GetAllocator());

		if(indexedVariables)
			target.AddMember("indexedVariables", ::ToJson(*indexedVariables, document), document.GetAllocator());
	}

	/**
	* The new value of the variable.
	*/
	std::string value;

	/**
	* The type of the new value. Typically shown in the UI when hovering over the value.
	*/
	Optional<std::string> type;

	/**
	* If variablesReference is > 0, the new value is structured and its children can be retrieved by passing variablesReference to the VariablesRequest.
	*/
	Optional<int> variablesReference;

	/**
	* The number of named child variables.
	* The client can use this optional information to present the variables in a paged UI and fetch them in chunks.
	*/
	Optional<int> namedVariables;

	/**
	* The number of indexed child variables.
	* The client can use this optional information to present the variables in a paged UI and fetch them in chunks.
	*/
	Optional<int> indexedVariables;
};

struct EvaluateArguments
{
	EvaluateArguments() = default;

	EvaluateArguments(rapidjson::Value &json)
	{
		FromJson(expression, json["expression"]);

		if(json.HasMember("frameId"))
			FromJson(frameId, json["frameId"]);

		if(json.HasMember("context"))
			FromJson(context, json["context"]);

		if(json.HasMember("format"))
			FromJson(format, json["format"]);
	}

	/**
	* The expression to evaluate.
	*/
	std::string expression;

	/**
	* Evaluate the expression in the scope of this stack frame. If not specified, the expression is evaluated in the global scope.
	*/
	Optional<int> frameId;

	/**
	* The context in which the evaluate request is run.
	* Values: 
	* 'watch': evaluate is run in a watch.
	* 'repl': evaluate is run from REPL console.
	* 'hover': evaluate is run from a data hover.
	* etc.
	*/
	Optional<std::string> context;

	/**
	* Specifies details on how to format the Evaluate result.
	*/
	Optional<ValueFormat> format;
};

struct EvaluateResponseData
{
	EvaluateResponseData() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document) const
	{
		target.SetObject();

		target.AddMember("result", ::ToJson(result, document), document.GetAllocator());

		if(type)
			target.AddMember("type", ::ToJson(*type, document), document.GetAllocator());

		if(presentationHint)
			target.AddMember("presentationHint", ::ToJson(*presentationHint, document), document.GetAllocator());

		if(variablesReference)
			target.AddMember("variablesReference", ::ToJson(*variablesReference, document), document.GetAllocator());

		if(namedVariables)
			target.AddMember("namedVariables", ::ToJson(*namedVariables, document), document.GetAllocator());

		if(indexedVariables)
			target.AddMember("indexedVariables", ::ToJson(*indexedVariables, document), document.GetAllocator());
	}

	/**
	* The result of the evaluate request.
	*/
	std::string result;

	/**
	* The optional type of the evaluate result.
	*/
	Optional<std::string> type;

	/**
	* Properties of a evaluate result that can be used to determine how to render the result in the UI.
	*/
	Optional<VariablePresentationHint> presentationHint;

	/**
	* If variablesReference is > 0, the evaluate result is structured and its children can be retrieved by passing variablesReference to the VariablesRequest.
	*/
	Optional<int> variablesReference;

	/**
	* The number of named child variables.
	* The client can use this optional information to present the variables in a paged UI and fetch them in chunks.
	*/
	Optional<int> namedVariables;

	/**
	* The number of indexed child variables.
	* The client can use this optional information to present the variables in a paged UI and fetch them in chunks.
	*/
	Optional<int> indexedVariables;
};
