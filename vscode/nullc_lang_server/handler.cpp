#include "handler.h"

#include <stdio.h>

#include <vector>

#include "external/rapidjson/document.h"
#include "external/rapidjson/prettywriter.h"
#include "external/rapidjson/error/en.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullc_internal.h"

#include "context.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

enum class ErrorCode
{
	ParseError = -32700,
	InvalidRequest = -32600,
	MethodNotFound = -32601,
	InvalidParams = -32602,
	InternalError = -32603,
	serverErrorStart = -32099,
	serverErrorEnd = -32000,
	ServerNotInitialized = -32002,
	UnknownErrorCode = -32001,

	// Defined by the protocol.
	RequestCancelled = -32800,
	ContentModified = -32801
};

std::string UrlDecode(const char *url)
{
	std::string result;

	result.reserve(strlen(url) + 1);

	const char *pos = url;

	while(*pos)
	{
		if(*pos == '%')
		{
			pos++;

			char buf[3];

			if(*pos)
				buf[0] = *pos++;

			if(*pos)
				buf[1] = *pos++;

			buf[2] = 0;

			result.append(1, (char)strtoul(buf, nullptr, 16));
		}
		else
		{
			result.append(1, *pos);

			pos++;
		}
	}

	return result;
}

bool IsInside(SynBase *syntax, unsigned line, unsigned column)
{
	if(line > syntax->begin->line || (line == syntax->begin->line && column >= syntax->begin->column))
	{
		if(line < syntax->end->line || (line == syntax->end->line && column < syntax->end->column + syntax->end->length))
		{
			return true;
		}
	}

	return false;
}

bool IsSmaller(SynBase *current, SynBase *next)
{
	if(!current)
		return true;

	if(next->begin->line > current->begin->line || (next->begin->line == current->begin->line && next->begin->column > current->begin->column))
	{
		if(next->end->line < current->end->line || (next->end->line == current->end->line && next->end->column + next->end->length < current->end->column + current->end->length))
		{
			return true;
		}
	}

	return false;
}

bool HandleMessage(Context& ctx, char *message, unsigned length)
{
	(void)length;

	rapidjson::Document doc;

	rapidjson::ParseResult ok = doc.ParseInsitu(message);

	if(!ok)
	{
		fprintf(stderr, "ERROR: Failed to parse message: %s (%d)\n", rapidjson::GetParseError_En(ok.Code()), (int)ok.Offset());
		return false;
	}

	if(!doc.HasMember("jsonrpc"))
	{
		fprintf(stderr, "ERROR: Message must have 'jsonrpc' member\n");
		return false;
	}

	auto rpcVersion = doc["jsonrpc"].GetString();
	(void)rpcVersion;

	if(!doc.HasMember("method"))
	{
		fprintf(stderr, "ERROR: Message must have 'method' member\n");
		return false;
	}

	auto method = doc["method"].GetString();

	if(doc.HasMember("id"))
	{
		// id can be a number or a string
		auto idNumber = doc["id"].IsUint() ? doc["id"].GetUint() : ~0u;
		auto strNumber = doc["id"].IsString() ? doc["id"].GetString() : nullptr;

		return HandleMessage(ctx, idNumber, strNumber, method, doc["params"]);
	}

	return HandleNotification(ctx, method, doc["params"]);
}

void PrepareResponse(rapidjson::Document &doc, unsigned idNumber, const char *idString)
{
	doc.SetObject();

	doc.AddMember("jsonrpc", "2.0", doc.GetAllocator());

	if(idNumber != ~0u)
		doc.AddMember("id", idNumber, doc.GetAllocator());
	else if(idString)
		doc.AddMember("id", rapidjson::StringRef(idString), doc.GetAllocator());
}

void SendResponse(Context& ctx, rapidjson::Document &doc)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	const char* output = buffer.GetString();
	unsigned length = (unsigned)strlen(output);

	if(ctx.debugMode)
		fprintf(stderr, "INFO: Sending message '%.*s%s'\n", (int)(length > 96 ? 96 : length), output, length > 96 ? "..." : "");

	fprintf(stdout, "Content-Length: %d\r\n", length);
	fprintf(stdout, "\r\n");
	fprintf(stdout, "%s", output);
}

bool RespondWithError(Context& ctx, rapidjson::Document &doc, const char *method, ErrorCode errorCode, const char *message)
{
	if(ctx.debugMode)
		fprintf(stderr, "INFO: RespondWithError(%s, %d, %s)\n", method, int(errorCode), message);

	rapidjson::Value error;
	error.SetObject();

	error.AddMember("code", int(errorCode), doc.GetAllocator());
	error.AddMember("message", rapidjson::StringRef(message), doc.GetAllocator());

	doc.AddMember("error", error, doc.GetAllocator());

	SendResponse(ctx, doc);

	return true;
}

bool HandleInitialize(Context& ctx, rapidjson::Value& arguments, rapidjson::Document &response)
{
	if(!ctx.nullcInitialized)
	{
		if(arguments["rootPath"].IsString())
		{
			std::string modulePath = arguments["rootPath"].GetString();

			modulePath += "/Modules/";

			if(ctx.debugMode)
				fprintf(stderr, "INFO: Launching nullc with module path '%s'\n", modulePath.c_str());

			nullcInit(modulePath.c_str());
		}
		else
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Launching nullc without module path\n");

			nullcInit("");
		}
	}

	rapidjson::Value result;
	result.SetObject();

	rapidjson::Value capabilities;
	capabilities.SetObject();

	rapidjson::Value save;
	save.SetObject();

	save.AddMember("includeText", false, response.GetAllocator());

	rapidjson::Value textDocumentSync;
	textDocumentSync.SetObject();

	textDocumentSync.AddMember("openClose", true, response.GetAllocator());
	textDocumentSync.AddMember("change", 1, response.GetAllocator()); // full
	textDocumentSync.AddMember("willSave", false, response.GetAllocator());
	textDocumentSync.AddMember("willSaveWaitUntil", false, response.GetAllocator());
	textDocumentSync.AddMember("save", save, response.GetAllocator());

	capabilities.AddMember("textDocumentSync", textDocumentSync, response.GetAllocator());
	capabilities.AddMember("foldingRangeProvider", true, response.GetAllocator());
	capabilities.AddMember("documentSymbolProvider", true, response.GetAllocator());
	capabilities.AddMember("hoverProvider", true, response.GetAllocator());

	result.AddMember("capabilities", capabilities, response.GetAllocator());

	response.AddMember("result", result, response.GetAllocator());

	SendResponse(ctx, response);

	return true;
}

bool HandleFoldingRange(Context& ctx, rapidjson::Value& arguments, rapidjson::Document &response)
{
	auto documentIt = ctx.documents.find(arguments["textDocument"]["uri"].GetString());

	if(documentIt == ctx.documents.end())
	{
		fprintf(stderr, "ERROR: Failed to find document '%s'\n", arguments["textDocument"]["uri"].GetString());

		return RespondWithError(ctx, response, "", ErrorCode::InvalidParams, "failed to find target document");
	}

	static const char *foldingRangeComment = "comment";
	static const char *foldingRangeImports = "imports";
	static const char *foldingRangeRegion = "region";

	struct FoldingRange
	{
		FoldingRange(): startLine(0), startCharacter(0), endLine(0), endCharacter(0), kind(nullptr)
		{
		}

		FoldingRange(int startLine, int startCharacter, int endLine, int endCharacter, const char *kind): startLine(startLine), startCharacter(startCharacter), endLine(endLine), endCharacter(endCharacter), kind(kind)
		{
		}

		int startLine;
		int startCharacter;

		int endLine;
		int endCharacter;

		const char *kind;
	};

	std::vector<FoldingRange> foldingRanges;

	if(nullcAnalyze(documentIt->second.code.c_str()))
	{
		if(ctx.debugMode)
			fprintf(stderr, "INFO: Successfully compiled\n");
	}
	else
	{
		if(ctx.debugMode)
			fprintf(stderr, "INFO: Failed to compile: %s\n", nullcGetLastError());
	}

	if(CompilerContext *context = nullcGetCompilerContext())
	{
		if(context->synModule)
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Parse tree is available\n");

			nullcVisitParseTreeNodes(context->synModule, &foldingRanges, [](void *context, SynBase *child){
				auto &foldingRanges = *(std::vector<FoldingRange>*)context;

				if(SynIfElse *node = getType<SynIfElse>(child))
				{
					SynBase *trueBlock = node->trueBlock;

					foldingRanges.push_back(FoldingRange(trueBlock->begin->line, trueBlock->begin->column, trueBlock->end->line, trueBlock->end->column, foldingRangeRegion));

					if(SynBase *falseBlock = node->falseBlock)
						foldingRanges.push_back(FoldingRange(falseBlock->begin->line, falseBlock->begin->column, falseBlock->end->line, falseBlock->end->column, foldingRangeRegion));
				}
				else if(SynFor *node = getType<SynFor>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, foldingRangeRegion));
				}
				else if(SynWhile *node = getType<SynWhile>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, foldingRangeRegion));
				}
				else if(SynDoWhile *node = getType<SynDoWhile>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, foldingRangeRegion));
				}
				else if(SynFunctionDefinition *node = getType<SynFunctionDefinition>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, foldingRangeRegion));
				}
				else if(SynClassDefinition *node = getType<SynClassDefinition>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, foldingRangeRegion));
				}
				else if(SynEnumDefinition *node = getType<SynEnumDefinition>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, foldingRangeRegion));
				}
			});
		}
		else
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Parse tree unavailable\n");
		}
	}

	rapidjson::Value result;

	if(foldingRanges.empty())
	{
		result.SetNull();
	}
	else
	{
		result.SetArray();

		for(auto &&el : foldingRanges)
		{
			rapidjson::Value foldingRange;
			foldingRange.SetObject();

			foldingRange.AddMember("startLine", el.startLine, response.GetAllocator());
			foldingRange.AddMember("startCharacter", el.startCharacter, response.GetAllocator());
			foldingRange.AddMember("endLine", el.endLine, response.GetAllocator());
			foldingRange.AddMember("endCharacter", el.endCharacter, response.GetAllocator());
			foldingRange.AddMember("kind", rapidjson::StringRef(el.kind), response.GetAllocator());

			result.PushBack(foldingRange, response.GetAllocator());
		}
	}

	response.AddMember("result", result, response.GetAllocator());

	SendResponse(ctx, response);

	nullcClean();

	return true;
}

bool HandleHover(Context& ctx, rapidjson::Value& arguments, rapidjson::Document &response)
{
	auto documentIt = ctx.documents.find(arguments["textDocument"]["uri"].GetString());

	if(documentIt == ctx.documents.end())
	{
		fprintf(stderr, "ERROR: Failed to find document '%s'\n", arguments["textDocument"]["uri"].GetString());

		return RespondWithError(ctx, response, "", ErrorCode::InvalidParams, "failed to find target document");
	}

	auto positionLine = arguments["position"]["line"].GetUint();
	auto positionColumn = arguments["position"]["character"].GetUint();

	if(nullcAnalyze(documentIt->second.code.c_str()))
	{
		if(ctx.debugMode)
			fprintf(stderr, "INFO: Successfully compiled\n");
	}
	else
	{
		if(ctx.debugMode)
			fprintf(stderr, "INFO: Failed to compile: %s\n", nullcGetLastError());
	}

	struct HoverInfo
	{
		HoverInfo(): identifier(nullptr)
		{
		}

		unsigned line;
		unsigned column;

		SynBase *identifier;
		std::string description;
	};

	HoverInfo hoverInfo;

	hoverInfo.line = positionLine;
	hoverInfo.column = positionColumn;

	if(CompilerContext *context = nullcGetCompilerContext())
	{
		if(context->exprModule)
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Expression tree is available\n");

			nullcVisitExpressionTreeNodes(context->exprModule, &hoverInfo, [](void *context, ExprBase *child){
				HoverInfo &hoverInfo = *(HoverInfo*)context;

				if(!IsInside(child->source, hoverInfo.line, hoverInfo.column))
					return;

				if(ExprVariableAccess *node = getType<ExprVariableAccess>(child))
				{
					if(!IsSmaller(hoverInfo.identifier, node->source))
						return;

					hoverInfo.identifier = node->source;

					char buf[256];
					SafeSprintf(buf, 256, "Variable '%.*s %.*s'", FMT_ISTR(node->variable->type->name), FMT_ISTR(node->variable->name));
					hoverInfo.description = buf;
				}
				else if(ExprGetAddress *node = getType<ExprGetAddress>(child))
				{
					if(!IsSmaller(hoverInfo.identifier, node->source))
						return;

					hoverInfo.identifier = node->source;

					char buf[256];
					SafeSprintf(buf, 256, "Variable '%.*s %.*s'", FMT_ISTR(node->variable->type->name), FMT_ISTR(node->variable->name));
					hoverInfo.description = buf;
				}
				else if(ExprMemberAccess *node = getType<ExprMemberAccess>(child))
				{
					if(!IsSmaller(hoverInfo.identifier, node->source))
						return;

					hoverInfo.identifier = node->source;

					char buf[256];
					SafeSprintf(buf, 256, "Member '%.*s %.*s'", FMT_ISTR(node->member->type->name), FMT_ISTR(node->member->name));
					hoverInfo.description = buf;
				}
				else if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(child))
				{
					if(!IsSmaller(hoverInfo.identifier, node->source))
						return;

					hoverInfo.identifier = node->source;

					char buf[256];
					SafeSprintf(buf, 256, "Function '%.*s %.*s'", FMT_ISTR(node->function->type->name), FMT_ISTR(node->function->name));
					hoverInfo.description = buf;
				}
				else if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(child))
				{
					if(SynFunctionDefinition *source = getType<SynFunctionDefinition>(node->source))
					{
						if(IsInside(source->returnType, hoverInfo.line, hoverInfo.column))
						{
							if(!IsSmaller(hoverInfo.identifier, source->returnType))
								return;

							hoverInfo.identifier = source->returnType;

							char buf[256];
							SafeSprintf(buf, 256, "Function '%.*s %.*s' return type", FMT_ISTR(node->function->type->name), FMT_ISTR(node->function->name));
							hoverInfo.description = buf;
						}
					}
				}
			});
		}
		/*else if(context->synModule)
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Parse tree is available\n");

			nullcVisitParseTreeNodes(context->synModule, &hoverInfo, [](void *context, SynBase *child){
				HoverInfo &hoverInfo = *(HoverInfo*)context;
			});
		}*/
		else
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Parse tree unavailable\n");
		}
	}

	rapidjson::Value result;

	if(hoverInfo.identifier)
	{
		result.SetObject();

		rapidjson::Value range;
		range.SetObject();

		rapidjson::Value start;
		start.SetObject();

		start.AddMember("line", hoverInfo.identifier->begin->line, response.GetAllocator());
		start.AddMember("character", hoverInfo.identifier->begin->column, response.GetAllocator());

		rapidjson::Value end;
		end.SetObject();

		end.AddMember("line", hoverInfo.identifier->begin->line, response.GetAllocator());
		end.AddMember("character", hoverInfo.identifier->begin->column + hoverInfo.identifier->begin->length, response.GetAllocator());

		range.AddMember("start", start, response.GetAllocator());
		range.AddMember("end", end, response.GetAllocator());

		result.AddMember("contents", rapidjson::StringRef(hoverInfo.description.c_str()), response.GetAllocator());
		result.AddMember("range", range, response.GetAllocator());
	}
	else
	{
		result.SetNull();
	}

	response.AddMember("result", result, response.GetAllocator());

	SendResponse(ctx, response);

	nullcClean();

	return true;
}

bool HandleDidOpen(Context& ctx, rapidjson::Value& arguments)
{
	auto uri = arguments["textDocument"]["uri"].GetString();

	auto &document = ctx.documents[uri];

	if(ctx.debugMode)
		fprintf(stderr, "INFO: Created document '%s'\n", uri);

	document.code = arguments["textDocument"]["text"].GetString();

	return true;
}

bool HandleDidChange(Context& ctx, rapidjson::Value& arguments)
{
	auto uri = arguments["textDocument"]["uri"].GetString();

	auto &document = ctx.documents[uri];

	for(auto &&el : arguments["contentChanges"].GetArray())
	{
		if(el.HasMember("text"))
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Updated document '%s'\n", uri);

			document.code = el["text"].GetString();
		}
	}

	return true;
}

bool HandleMessage(Context& ctx, unsigned idNumber, const char *idString, const char *method, rapidjson::Value& arguments)
{
	rapidjson::Document response;

	PrepareResponse(response, idNumber, idString);

	if(ctx.debugMode)
		fprintf(stderr, "INFO: HandleMessage(%s)\n", method);

	if(strcmp(method, "initialize") == 0)
		return HandleInitialize(ctx, arguments, response);
	else if(strcmp(method, "textDocument/foldingRange") == 0)
		return HandleFoldingRange(ctx, arguments, response);
	else if(strcmp(method, "textDocument/hover") == 0)
		return HandleHover(ctx, arguments, response);
	
	return RespondWithError(ctx, response, method, ErrorCode::MethodNotFound, "not implemented");
}

bool HandleNotification(Context& ctx, const char *method, rapidjson::Value& arguments)
{
	(void)arguments;

	if(ctx.debugMode)
		fprintf(stderr, "INFO: HandleNotification(%s)\n", method);

	if(strcmp(method, "textDocument/didOpen") == 0)
		return HandleDidOpen(ctx, arguments);
	else if(strcmp(method, "textDocument/didChange") == 0)
		return HandleDidChange(ctx, arguments);

	return true;
}
