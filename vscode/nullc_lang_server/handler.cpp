#include "handler.h"

#include <stdio.h>

#include <vector>

#include "external/rapidjson/document.h"
#include "external/rapidjson/prettywriter.h"
#include "external/rapidjson/error/en.h"

#include "../../NULLC/nullc.h"
#include "../../NULLC/nullc_internal.h"

#include "context.h"
#include "schema.h"

#define FMT_ISTR(x) unsigned(x.end - x.begin), x.begin

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

	std::vector<FoldingRange> foldingRanges;

	if(nullcAnalyze(documentIt->second.code.c_str()))
	{
		if(ctx.debugMode)
			fprintf(stderr, "INFO: Successfully compiled\n");
	}
	else
	{
		if(ctx.debugMode)
			fprintf(stderr, "INFO: Failed to compile\n");
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

					foldingRanges.push_back(FoldingRange(trueBlock->begin->line, trueBlock->begin->column, trueBlock->end->line, trueBlock->end->column, FoldingRangeKind::region));

					if(SynBase *falseBlock = node->falseBlock)
						foldingRanges.push_back(FoldingRange(falseBlock->begin->line, falseBlock->begin->column, falseBlock->end->line, falseBlock->end->column, FoldingRangeKind::region));
				}
				else if(SynFor *node = getType<SynFor>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, FoldingRangeKind::region));
				}
				else if(SynWhile *node = getType<SynWhile>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, FoldingRangeKind::region));
				}
				else if(SynDoWhile *node = getType<SynDoWhile>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, FoldingRangeKind::region));
				}
				else if(SynFunctionDefinition *node = getType<SynFunctionDefinition>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, FoldingRangeKind::region));
				}
				else if(SynClassDefinition *node = getType<SynClassDefinition>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, FoldingRangeKind::region));
				}
				else if(SynEnumDefinition *node = getType<SynEnumDefinition>(child))
				{
					foldingRanges.push_back(FoldingRange(node->begin->line, node->begin->column, node->end->line, node->end->column + node->end->length, FoldingRangeKind::region));
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
			result.PushBack(el.ToJson(response), response.GetAllocator());
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

	auto position = Position(arguments["position"]);

	if(nullcAnalyze(documentIt->second.code.c_str()))
	{
		if(ctx.debugMode)
			fprintf(stderr, "INFO: Successfully compiled\n");
	}
	else
	{
		if(ctx.debugMode)
			fprintf(stderr, "INFO: Failed to compile\n");
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

	hoverInfo.line = position.line;
	hoverInfo.column = position.character;

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

bool HandleDocumentSymbol(Context& ctx, rapidjson::Value& arguments, rapidjson::Document &response)
{
	auto documentIt = ctx.documents.find(arguments["textDocument"]["uri"].GetString());

	if(documentIt == ctx.documents.end())
	{
		fprintf(stderr, "ERROR: Failed to find document '%s'\n", arguments["textDocument"]["uri"].GetString());

		return RespondWithError(ctx, response, "", ErrorCode::InvalidParams, "failed to find target document");
	}

	nullcAnalyze(documentIt->second.code.c_str());

	std::vector<DocumentSymbol> symbols;

	if(CompilerContext *context = nullcGetCompilerContext())
	{
		for(unsigned i = 0; i < context->exprCtx.namespaces.size(); i++)
		{
			auto ns = context->exprCtx.namespaces[i];

			// Filter functions with location information
			if(!ns->name.begin)
				continue;

			DocumentSymbol symbol;

			symbol.name = std::string(ns->name.name.begin, ns->name.name.end);
			symbol.kind = SymbolKind::Namespace;
			symbol.range = Range(Position(ns->source->begin->line, ns->source->begin->column), Position(ns->source->end->line, ns->source->end->column + ns->source->end->length));
			symbol.selectionRange = Range(Position(ns->name.begin->line, ns->name.begin->column), Position(ns->name.end->line, ns->name.end->column + ns->name.end->length));

			symbols.push_back(symbol);
		}

		for(unsigned i = 0; i < context->exprCtx.functions.size(); i++)
		{
			auto function = context->exprCtx.functions[i];

			// Filter functions with location information
			if(!function->name.begin)
				continue;

			DocumentSymbol symbol;

			symbol.name = std::string(function->name.name.begin, function->name.name.end);
			symbol.kind = SymbolKind::Function;
			symbol.range = Range(Position(function->source->begin->line, function->source->begin->column), Position(function->source->end->line, function->source->end->column + function->source->end->length));
			symbol.selectionRange = Range(Position(function->name.begin->line, function->name.begin->column), Position(function->name.end->line, function->name.end->column + function->name.end->length));

			// Format function signature
			const unsigned bufSize = 4096;
			char buf[bufSize];

			char *pos = buf;
			*pos = 0;

			pos += SafeSprintf(pos, bufSize - int(pos - buf), "%.*s %.*s", FMT_ISTR(function->type->returnType->name), FMT_ISTR(function->name.name));

			if(!function->generics.empty())
			{
				pos += SafeSprintf(pos, bufSize - int(pos - buf), "<");

				for(unsigned k = 0; k < function->generics.size(); k++)
				{
					MatchData *match = function->generics[k];

					pos += SafeSprintf(pos, bufSize - int(pos - buf), "%s%.*s", k != 0 ? ", " : "", FMT_ISTR(match->type->name));
				}

				pos += SafeSprintf(pos, bufSize - int(pos - buf), ">");
			}

			pos += SafeSprintf(pos, bufSize - int(pos - buf), "(");

			for(unsigned k = 0; k < function->arguments.size(); k++)
			{
				ArgumentData &argument = function->arguments[k];

				pos += SafeSprintf(pos, bufSize - int(pos - buf), "%s%s%.*s", k != 0 ? ", " : "", argument.isExplicit ? "explicit " : "", FMT_ISTR(argument.type->name));
			}

			pos += SafeSprintf(pos, bufSize - int(pos - buf), ")");

			symbol.detail = buf;

			symbols.push_back(symbol);
		}
	}

	rapidjson::Value result;

	if(symbols.empty())
	{
		result.SetNull();
	}
	else
	{
		result.SetArray();

		for(auto &&el : symbols)
		{
			rapidjson::Value symbol;

			el.SaveTo(symbol, response);

			result.PushBack(symbol, response.GetAllocator());
		}
	}

	response.AddMember("result", result, response.GetAllocator());

	SendResponse(ctx, response);

	nullcClean();

	return true;
}

void UpdateDiagnostics(Context& ctx, Document &document)
{
	rapidjson::Document response;
	response.SetObject();

	response.AddMember("jsonrpc", "2.0", response.GetAllocator());

	response.AddMember("method", "textDocument/publishDiagnostics", response.GetAllocator());

	rapidjson::Value params;
	params.SetObject();

	params.AddMember("uri", document.uri, response.GetAllocator());

	rapidjson::Value diagnostics;
	diagnostics.SetArray();

	if(!nullcAnalyze(document.code.c_str()))
	{
		if(CompilerContext *context = nullcGetCompilerContext())
		{
			for(auto &&el : context->parseCtx.errorInfo)
			{
				Diagnostic diagnostic;

				diagnostic.range = Range(Position(el->begin->line, el->begin->column), Position(el->end->line, el->end->column + el->end->length));

				diagnostic.severity = DiagnosticSeverity::Error;
				diagnostic.code = "parsing";
				diagnostic.source = "nullc";

				diagnostic.message = std::string(el->messageStart, el->messageEnd);

				for(auto &&extra : el->related)
				{
					DiagnosticRelatedInformation info;

					info.location = Location(document.uri, Range(Position(extra->begin->line, extra->begin->column), Position(extra->end->line, extra->end->column + extra->end->length)));
					info.message = std::string(extra->messageStart, extra->messageEnd);

					diagnostic.relatedInformation.push_back(info);
				}

				diagnostics.PushBack(diagnostic.ToJson(response), response.GetAllocator());
			}

			for(auto &&el : context->exprCtx.errorInfo)
			{
				Diagnostic diagnostic;

				diagnostic.range = Range(Position(el->begin->line, el->begin->column), Position(el->end->line, el->end->column + el->end->length));

				diagnostic.severity = DiagnosticSeverity::Error;
				diagnostic.code = "analysis";
				diagnostic.source = "nullc";

				diagnostic.message = std::string(el->messageStart, el->messageEnd);

				for(auto &&extra : el->related)
				{
					DiagnosticRelatedInformation info;

					info.location = Location(document.uri, Range(Position(extra->begin->line, extra->begin->column), Position(extra->end->line, extra->end->column + extra->end->length)));
					info.message = std::string(extra->messageStart, extra->messageEnd);

					diagnostic.relatedInformation.push_back(info);
				}

				diagnostics.PushBack(diagnostic.ToJson(response), response.GetAllocator());
			}
		}
	}

	params.AddMember("diagnostics", diagnostics, response.GetAllocator());

	response.AddMember("params", params, response.GetAllocator());

	SendResponse(ctx, response);

	nullcClean();
}

bool HandleDidOpen(Context& ctx, rapidjson::Value& arguments)
{
	auto uri = arguments["textDocument"]["uri"].GetString();

	auto &document = ctx.documents[uri];

	if(ctx.debugMode)
		fprintf(stderr, "INFO: Created document '%s'\n", uri);

	document.uri = uri;
	document.code = arguments["textDocument"]["text"].GetString();

	UpdateDiagnostics(ctx, document);

	return true;
}

bool HandleDidChange(Context& ctx, rapidjson::Value& arguments)
{
	auto uri = arguments["textDocument"]["uri"].GetString();

	auto &document = ctx.documents[uri];

	document.uri = uri;

	for(auto &&el : arguments["contentChanges"].GetArray())
	{
		if(el.HasMember("text"))
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Updated document '%s'\n", uri);

			document.code = el["text"].GetString();
		}
	}

	UpdateDiagnostics(ctx, document);

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
	else if(strcmp(method, "textDocument/documentSymbol") == 0)
		return HandleDocumentSymbol(ctx, arguments, response);
	
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
