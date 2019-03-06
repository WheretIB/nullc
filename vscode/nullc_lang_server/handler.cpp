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

bool IsInside(SynBase *syntax, unsigned line, unsigned column)
{
	if(syntax->isInternal)
		return false;

	if(line > syntax->begin->line || (line == syntax->begin->line && column >= syntax->begin->column))
	{
		if(line < syntax->end->line || (line == syntax->end->line && column < syntax->end->column + syntax->end->length))
		{
			return true;
		}
	}

	return false;
}

bool IsAtEnd(SynBase *syntax, unsigned line, unsigned column)
{
	if(syntax->isInternal)
		return false;

	if(line == syntax->end->line && column == syntax->end->column + syntax->end->length)
		return true;

	return false;
}

bool IsSmaller(SynBase *current, SynBase *next)
{
	if(!current)
		return true;

	if(next->begin->line > current->begin->line || (next->begin->line == current->begin->line && next->begin->column > current->begin->column))
	{
		if(next->end->line < current->end->line || (next->end->line == current->end->line && next->end->column + next->end->length <= current->end->column + current->end->length))
		{
			return true;
		}
	}

	return false;
}

std::string GetFunctionSignature(FunctionData *function)
{
	const unsigned bufSize = 8192;
	char buf[bufSize];

	char *pos = buf;
	*pos = 0;

	pos += SafeSprintf(pos, bufSize - int(pos - buf), "%.*s %.*s", FMT_ISTR(function->type->returnType->name), FMT_ISTR(function->name->name));

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

		pos += SafeSprintf(pos, bufSize - int(pos - buf), "%s%s%.*s %.*s", k != 0 ? ", " : "", argument.isExplicit ? "explicit " : "", FMT_ISTR(argument.type->name), FMT_ISTR(argument.name->name));
	}

	pos += SafeSprintf(pos, bufSize - int(pos - buf), ")");

	return buf;
}

std::string GetMemberSignature(TypeBase *type, VariableData *member)
{
	const unsigned bufSize = 8192;
	char buf[bufSize];

	char *pos = buf;
	*pos = 0;

	pos += SafeSprintf(pos, bufSize - int(pos - buf), "%.*s %.*s::%.*s", FMT_ISTR(member->type->name), FMT_ISTR(type->name), FMT_ISTR(member->name->name));

	return buf;
}

std::string GetMemberSignature(TypeBase *type, ConstantData *member)
{
	const unsigned bufSize = 8192;
	char buf[bufSize];

	char *pos = buf;
	*pos = 0;

	pos += SafeSprintf(pos, bufSize - int(pos - buf), "%.*s %.*s::%.*s", FMT_ISTR(member->value->type->name), FMT_ISTR(type->name), FMT_ISTR(member->name->name));

	return buf;
}

std::string GetMemberSignature(TypeBase *type, MatchData *member)
{
	const unsigned bufSize = 8192;
	char buf[bufSize];

	char *pos = buf;
	*pos = 0;

	pos += SafeSprintf(pos, bufSize - int(pos - buf), "%.*s %.*s::%.*s", FMT_ISTR(member->type->name), FMT_ISTR(type->name), FMT_ISTR(member->name->name));

	return buf;
}

struct FindEntityResponse
{
	explicit operator bool() const
	{
		return targetVariable || targetFunction || targetType;
	}

	SynBase *bestNode = nullptr;

	VariableData *targetVariable = nullptr;
	FunctionData *targetFunction = nullptr;
	TypeBase *targetType = nullptr;

	std::string debugScopes;
};

FindEntityResponse FindEntityAtLocation(CompilerContext *context, Position position, bool captureScopes)
{
	struct Data
	{
		Data(CompilerContext *context, Position &position, bool captureScopes): context(context), position(position), captureScopes(captureScopes)
		{
		}

		CompilerContext *context;

		Position &position;

		bool captureScopes;

		FindEntityResponse response;
	};

	Data data(context, position, captureScopes);

	nullcVisitExpressionTreeNodes(context->exprModule, &data, [](void *context, ExprBase *child){
		Data &data = *(Data*)context;
		FindEntityResponse &response = data.response;

		// Imported
		if(data.context->exprCtx.GetSourceOwner(child->source->begin))
			return;

		if(!IsInside(child->source, data.position.line, data.position.character))
			return;

		if(data.captureScopes)
		{
			response.debugScopes += GetExpressionTreeNodeName(child);
			response.debugScopes += ToString(" (%d:%d-%d:%d)", child->source->begin->line + 1, child->source->begin->column, child->source->end->line + 1, child->source->end->column + child->source->end->length);
		}

		if(ExprVariableAccess *node = getType<ExprVariableAccess>(child))
		{
			if(!IsSmaller(response.bestNode, node->source))
			{
				if(data.captureScopes)
					response.debugScopes += " <- skipped[larger]  \n";

				return;
			}

			response.bestNode = node->source;

			response.targetVariable = node->variable;
			response.targetFunction = nullptr;
			response.targetType = nullptr;

			if(data.captureScopes)
				response.debugScopes += " <- selected";
		}
		else if(ExprGetAddress *node = getType<ExprGetAddress>(child))
		{
			SynBase *nameSource = node->variable->source;

			if(data.captureScopes)
			{
				response.debugScopes += ToString(" name (%d:%d-%d:%d)", nameSource->begin->line + 1, nameSource->begin->column, nameSource->end->line + 1, nameSource->end->column + nameSource->end->length);
			}

			if(!nameSource)
			{
				if(data.captureScopes)
					response.debugScopes += " <- skipped[no name source]  \n";

				return;
			}

			if(!IsInside(nameSource, data.position.line, data.position.character))
			{
				if(data.captureScopes)
					response.debugScopes += " <- skipped[outside name]  \n";

				return;
			}

			if(!IsSmaller(response.bestNode, nameSource))
			{
				if(data.captureScopes)
					response.debugScopes += " <- skipped[larger]  \n";

				return;
			}

			response.bestNode = nameSource;

			response.targetVariable = node->variable->variable;
			response.targetFunction = nullptr;
			response.targetType = nullptr;

			if(data.captureScopes)
				response.debugScopes += " <- selected";
		}
		else if(ExprMemberAccess *node = getType<ExprMemberAccess>(child))
		{
			if(!IsSmaller(response.bestNode, node->source))
			{
				if(data.captureScopes)
					response.debugScopes += " <- skipped[larger]  \n";

				return;
			}

			if(!node->member)
			{
				if(data.captureScopes)
					response.debugScopes += " <- skipped[no member]  \n";

				return;
			}

			if(TypeRef *typeRef = getType<TypeRef>(node->value->type))
			{
				response.bestNode = node->source;

				response.targetVariable = node->member->variable;
				response.targetFunction = nullptr;
				response.targetType = nullptr;

				if(data.captureScopes)
					response.debugScopes += " <- selected";
			}
		}
		else if(ExprFunctionAccess *node = getType<ExprFunctionAccess>(child))
		{
			if(!IsSmaller(response.bestNode, node->source))
			{
				if(data.captureScopes)
					response.debugScopes += " <- skipped[larger]  \n";

				return;
			}

			if(isType<SynFunctionDefinition>(node->source))
			{
				if(data.captureScopes)
					response.debugScopes += " <- skipped[definition]  \n";

				return;
			}

			response.bestNode = node->source;

			response.targetVariable = nullptr;
			response.targetFunction = node->function;
			response.targetType = nullptr;

			if(data.captureScopes)
				response.debugScopes += " <- selected";
		}
		else if(ExprFunctionDefinition *node = getType<ExprFunctionDefinition>(child))
		{
			if(SynFunctionDefinition *source = getType<SynFunctionDefinition>(node->source))
			{
				if(IsInside(source->returnType, data.position.line, data.position.character))
				{
					if(!IsSmaller(response.bestNode, source->returnType))
					{
						if(data.captureScopes)
							response.debugScopes += " <- skipped[larger]  \n";

						return;
					}

					response.bestNode = source->returnType;

					response.targetVariable = nullptr;
					response.targetFunction = nullptr;
					response.targetType = node->function->type->returnType;

					if(data.captureScopes)
						response.debugScopes += " <- selected[returnType]";
				}
			}
		}
		else if(ExprVariableDefinitions *node = getType<ExprVariableDefinitions>(child))
		{
			if(SynVariableDefinitions *source = getType<SynVariableDefinitions>(node->source))
			{
				if(IsInside(source->type, data.position.line, data.position.character))
				{
					if(!IsSmaller(response.bestNode, source->type))
					{
						if(data.captureScopes)
							response.debugScopes += " <- skipped[larger]  \n";

						return;
					}

					TypeBase *definitionType = node->definitionType;

					if(definitionType == data.context->exprCtx.typeAuto && !node->definitions.empty())
					{
						if(ExprVariableDefinition *definition = getType<ExprVariableDefinition>(node->definitions.head))
							definitionType = definition->variable->variable->type;
					}

					if(definitionType != data.context->exprCtx.typeAuto)
					{
						response.bestNode = source->type;

						response.targetVariable = nullptr;
						response.targetFunction = nullptr;
						response.targetType = definitionType;

						if(data.captureScopes)
							response.debugScopes += " <- selected[definitionType]";
					}
				}
			}
		}


		if(data.captureScopes)
			response.debugScopes += "  \n";
	});

	return data.response;
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

	rapidjson::Value completionProvider;
	completionProvider.SetObject();

	completionProvider.AddMember("resolveProvider", false, response.GetAllocator());

	rapidjson::Value triggerCharacters;
	triggerCharacters.SetArray();

	triggerCharacters.PushBack(".", response.GetAllocator());

	completionProvider.AddMember("triggerCharacters", triggerCharacters, response.GetAllocator());

	capabilities.AddMember("textDocumentSync", textDocumentSync, response.GetAllocator());
	capabilities.AddMember("foldingRangeProvider", true, response.GetAllocator());
	capabilities.AddMember("documentSymbolProvider", true, response.GetAllocator());
	capabilities.AddMember("hoverProvider", true, response.GetAllocator());
	capabilities.AddMember("completionProvider", completionProvider, response.GetAllocator());
	capabilities.AddMember("definitionProvider", true, response.GetAllocator());

	//signatureHelpProvider
	//referencesProvider
	//documentHighlightProvider

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

	nullcAnalyze(documentIt->second.code.c_str());

	Hover hover;

	if(CompilerContext *context = nullcGetCompilerContext())
	{
		if(context->exprModule)
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Expression tree is available\n");

			FindEntityResponse result = FindEntityAtLocation(context, position, ctx.debugMode);

			if(result)
			{
				if(VariableData *variable = result.targetVariable)
				{
					hover.range = Range(Position(result.bestNode->begin->line, result.bestNode->begin->column), Position(result.bestNode->begin->line, result.bestNode->begin->column + result.bestNode->begin->length));

					hover.contents.kind = MarkupKind::Markdown;

					if(TypeBase *owner = variable->scope->ownerType)
						hover.contents.value = GetMemberSignature(owner, variable);
					else
						hover.contents.value = ToString("Variable '%.*s %.*s'", FMT_ISTR(variable->type->name), FMT_ISTR(variable->name->name));
				}
				else if(FunctionData *function = result.targetFunction)
				{
					hover.range = Range(Position(result.bestNode->begin->line, result.bestNode->begin->column), Position(result.bestNode->begin->line, result.bestNode->begin->column + result.bestNode->begin->length));

					hover.contents.kind = MarkupKind::Markdown;
					hover.contents.value = "Function \'" + GetFunctionSignature(function) + "\'";
				}
				else if(TypeBase *type = result.targetType)
				{
					hover.range = Range(Position(result.bestNode->begin->line, result.bestNode->begin->column), Position(result.bestNode->begin->line, result.bestNode->begin->column + result.bestNode->begin->length));

					hover.contents.kind = MarkupKind::Markdown;
					hover.contents.value = ToString("Type '%.*s'", FMT_ISTR(type->name));
				}
			}

			if(!result.debugScopes.empty())
			{
				if(hover.contents.value.empty())
					hover.contents.value = "No info";

				hover.contents.value += "  \n***  \n" + result.debugScopes;
			}
		}
		else
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Expression tree unavailable\n");
		}
	}

	rapidjson::Value result;

	if(hover.contents.value.empty())
	{
		result.SetNull();
	}
	else
	{
		hover.SaveTo(result, response);
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
			if(!function->name->begin)
				continue;

			if(context->exprCtx.IsGenericInstance(function))
				continue;

			if(context->exprCtx.GetSourceOwner(function->name->begin))
				continue;

			DocumentSymbol symbol;

			symbol.name = std::string(function->name->name.begin, function->name->name.end);
			symbol.detail = GetFunctionSignature(function);
			symbol.kind = SymbolKind::Function;
			symbol.range = Range(Position(function->source->begin->line, function->source->begin->column), Position(function->source->end->line, function->source->end->column + function->source->end->length));
			symbol.selectionRange = Range(Position(function->name->begin->line, function->name->begin->column), Position(function->name->end->line, function->name->end->column + function->name->end->length));

			symbols.push_back(symbol);
		}

		for(unsigned i = 0; i < context->exprCtx.types.size(); i++)
		{
			auto type = context->exprCtx.types[i];

			if(TypeClass *typeClass = getType<TypeClass>(type))
			{
				// Filter types with location information
				if(!typeClass->identifier.begin || context->exprCtx.GetSourceOwner(typeClass->identifier.begin))
					continue;

				Lexeme *sourceBegin = typeClass->source->begin, *sourceEnd = typeClass->source->end;
				Lexeme *nameBegin = typeClass->identifier.begin, *nameEnd = typeClass->identifier.end;

				DocumentSymbol symbol;

				symbol.name = std::string(typeClass->name.begin, typeClass->name.end);
				symbol.kind = SymbolKind::Class;
				symbol.range = Range(Position(sourceBegin->line, sourceBegin->column), Position(sourceEnd->line, sourceEnd->column + sourceEnd->length));
				symbol.selectionRange = Range(Position(nameBegin->line, nameBegin->column), Position(nameEnd->line, nameEnd->column + nameEnd->length));

				auto hasMember = [](TypeClass *type, VariableData *member) -> bool {
					for(VariableHandle *curr = type->members.head; curr; curr = curr->next)
					{
						if(curr->variable->name == member->name)
							return true;
					}

					return false;
				};

				for(VariableHandle *curr = typeClass->members.head; curr; curr = curr->next)
				{
					if(!curr->source)
						continue;

					if(typeClass->baseClass && hasMember(typeClass->baseClass, curr->variable))
						continue;

					Lexeme *itemBegin = curr->source->begin, *itemEnd = curr->source->end;

					DocumentSymbol item;

					item.name = std::string(curr->variable->name->name.begin, curr->variable->name->name.end);
					item.kind = SymbolKind::Variable;
					item.range = Range(Position(itemBegin->line, itemBegin->column), Position(itemEnd->line, itemEnd->column + itemEnd->length));
					item.selectionRange = Range(Position(itemBegin->line, itemBegin->column), Position(itemEnd->line, itemEnd->column + itemEnd->length));
					//item.detail = ToString("%.*s %.*s::%.*s", FMT_ISTR(curr->variable->type->name), FMT_ISTR(typeClass->name), FMT_ISTR(curr->variable->name->name));

					symbol.children.push_back(item);
				}

				auto hasConstant = [](TypeClass *type, ConstantData *member) -> bool {
					for(ConstantData *curr = type->constants.head; curr; curr = curr->next)
					{
						if(curr->name == member->name)
							return true;
					}

					return false;
				};

				for(ConstantData *curr = typeClass->constants.head; curr; curr = curr->next)
				{
					if(!curr->name)
						continue;

					if(typeClass->baseClass && hasConstant(typeClass->baseClass, curr))
						continue;

					Lexeme *itemBegin = curr->name->begin, *itemEnd = curr->name->end;

					DocumentSymbol item;

					item.name = std::string(curr->name->name.begin, curr->name->name.end);
					item.kind = SymbolKind::Constant;
					item.range = Range(Position(itemBegin->line, itemBegin->column), Position(itemEnd->line, itemEnd->column + itemEnd->length));
					item.selectionRange = Range(Position(itemBegin->line, itemBegin->column), Position(itemEnd->line, itemEnd->column + itemEnd->length));
					//item.detail = ToString("%.*s %.*s::%.*s", FMT_ISTR(curr->value->type->name), FMT_ISTR(type->name), FMT_ISTR(curr->name->name));

					symbol.children.push_back(item);
				}

				symbols.push_back(symbol);
			}
			else if(TypeEnum *typeEnum = getType<TypeEnum>(type))
			{
				// Filter types with location information
				if(!typeEnum->identifier.begin || context->exprCtx.GetSourceOwner(typeEnum->identifier.begin))
					continue;

				Lexeme *sourceBegin = typeEnum->source->begin, *sourceEnd = typeEnum->source->end;
				Lexeme *nameBegin = typeEnum->identifier.begin, *nameEnd = typeEnum->identifier.end;

				DocumentSymbol symbol;

				symbol.name = std::string(typeEnum->name.begin, typeEnum->name.end);
				symbol.kind = SymbolKind::Enum;
				symbol.range = Range(Position(sourceBegin->line, sourceBegin->column), Position(sourceEnd->line, sourceEnd->column + sourceEnd->length));
				symbol.selectionRange = Range(Position(nameBegin->line, nameBegin->column), Position(nameEnd->line, nameEnd->column + nameEnd->length));

				for(ConstantData *curr = typeEnum->constants.head; curr; curr = curr->next)
				{
					if(!curr->name)
						continue;

					Lexeme *itemBegin = curr->name->begin, *itemEnd = curr->name->end;

					DocumentSymbol item;

					item.name = std::string(curr->name->name.begin, curr->name->name.end);
					item.kind = SymbolKind::Constant;
					item.range = Range(Position(itemBegin->line, itemBegin->column), Position(itemEnd->line, itemEnd->column + itemEnd->length));
					item.selectionRange = Range(Position(itemBegin->line, itemBegin->column), Position(itemEnd->line, itemEnd->column + itemEnd->length));
					//item.detail = ToString("%.*s %.*s::%.*s", FMT_ISTR(curr->value->type->name), FMT_ISTR(type->name), FMT_ISTR(curr->name->name));

					symbol.children.push_back(item);
				}

				symbols.push_back(symbol);
			}
			else if(TypeGenericClassProto *typeGenericClassProto = getType<TypeGenericClassProto>(type))
			{
				// Filter types with location information
				if(!typeGenericClassProto->identifier.begin || context->exprCtx.GetSourceOwner(typeGenericClassProto->identifier.begin))
					continue;

				Lexeme *sourceBegin = typeGenericClassProto->source->begin, *sourceEnd = typeGenericClassProto->source->end;
				Lexeme *nameBegin = typeGenericClassProto->identifier.begin, *nameEnd = typeGenericClassProto->identifier.end;

				DocumentSymbol symbol;

				symbol.name = std::string(typeGenericClassProto->name.begin, typeGenericClassProto->name.end);
				symbol.kind = SymbolKind::Class;
				symbol.range = Range(Position(sourceBegin->line, sourceBegin->column), Position(sourceEnd->line, sourceEnd->column + sourceEnd->length));
				symbol.selectionRange = Range(Position(nameBegin->line, nameBegin->column), Position(nameEnd->line, nameEnd->column + nameEnd->length));

				symbols.push_back(symbol);
			}
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

bool HandleCompletion(Context& ctx, rapidjson::Value& arguments, rapidjson::Document &response)
{
	auto documentIt = ctx.documents.find(arguments["textDocument"]["uri"].GetString());

	if(documentIt == ctx.documents.end())
	{
		fprintf(stderr, "ERROR: Failed to find document '%s'\n", arguments["textDocument"]["uri"].GetString());

		return RespondWithError(ctx, response, "", ErrorCode::InvalidParams, "failed to find target document");
	}

	Position position = Position(arguments["position"]);

	CompletionContext completionContext;

	if(arguments.HasMember("context"))
		completionContext = CompletionContext(arguments["context"]);

	struct Data
	{
		Data(Context &ctx, Position &position, CompletionContext &completionContext, CompletionList &completions): ctx(ctx), position(position), completionContext(completionContext), completions(completions)
		{
		}

		Context &ctx;

		Position &position;
		CompletionContext &completionContext;

		CompletionList &completions;

		CompilerContext *context = nullptr;
	};

	nullcAnalyze(documentIt->second.code.c_str());

	CompletionList completions;

	Data data(ctx, position, completionContext, completions);

	if(CompilerContext *context = nullcGetCompilerContext())
	{
		data.context = context;

		if(context->exprModule)
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Expression tree is available\n");

			nullcVisitExpressionTreeNodes(context->exprModule, &data, [](void *context, ExprBase *child){
				Data &data = *(Data*)context;

				if(!IsAtEnd(child->source, data.position.line, data.position.character))
					return;

				auto addTypeidCompletionOptions = [&data](TypeBase *type){
					{
						CompletionItem item;

						item.label = "isReference";
						item.kind = CompletionItemKind::Value;
						item.detail = ToString("bool %.*s::isReference = %s", FMT_ISTR(type->name), isType<TypeRef>(type) ? "true" : "false");

						data.completions.items.push_back(item);
					}

					{
						CompletionItem item;

						item.label = "isArray";
						item.kind = CompletionItemKind::Value;
						item.detail = ToString("bool %.*s::isArray = %s", FMT_ISTR(type->name), isType<TypeArray>(type) || isType<TypeUnsizedArray>(type) ? "true" : "false");

						data.completions.items.push_back(item);
					}

					{
						CompletionItem item;

						item.label = "isFunction";
						item.kind = CompletionItemKind::Value;
						item.detail = ToString("bool %.*s::isFunction = %s", FMT_ISTR(type->name), isType<TypeFunction>(type) ? "true" : "false");

						data.completions.items.push_back(item);
					}

					if(TypeArray *typeArray = getType<TypeArray>(type))
					{
						CompletionItem item;

						item.label = "arraySize";
						item.kind = CompletionItemKind::Value;
						item.detail = ToString("int %.*s::arraySize = %lld", FMT_ISTR(type->name), typeArray->length);

						data.completions.items.push_back(item);
					}

					if(isType<TypeUnsizedArray>(type))
					{
						CompletionItem item;

						item.label = "arraySize";
						item.kind = CompletionItemKind::Value;
						item.detail = ToString("int %.*s::arraySize = -1 (dynamic)", FMT_ISTR(type->name));

						data.completions.items.push_back(item);
					}

					if(TypeArgumentSet *typeArgumentSet = getType<TypeArgumentSet>(type))
					{
						CompletionItem item;

						item.label = "size";
						item.kind = CompletionItemKind::Value;
						item.detail = ToString("int %.*s::size = %u", FMT_ISTR(type->name), typeArgumentSet->types.size());

						data.completions.items.push_back(item);
					}

					if(isType<TypeFunction>(type))
					{
						CompletionItem item;

						item.label = "argument";
						item.kind = CompletionItemKind::TypeParameter;

						data.completions.items.push_back(item);
					}

					if(TypeFunction *typeFunction = getType<TypeFunction>(type))
					{
						CompletionItem item;

						item.label = "return";
						item.kind = CompletionItemKind::TypeParameter;
						item.detail = ToString("typeid %.*s::return = %.*s", FMT_ISTR(type->name), FMT_ISTR(typeFunction->returnType->name));

						data.completions.items.push_back(item);
					}

					if(TypeRef *typeRef = getType<TypeRef>(type))
					{
						CompletionItem item;

						item.label = "target";
						item.kind = CompletionItemKind::TypeParameter;
						item.detail = ToString("typeid %.*s::target = %.*s", FMT_ISTR(type->name), FMT_ISTR(typeRef->subType->name));

						data.completions.items.push_back(item);
					}

					if(TypeArray *typeArray = getType<TypeArray>(type))
					{
						CompletionItem item;

						item.label = "target";
						item.kind = CompletionItemKind::TypeParameter;
						item.detail = ToString("typeid %.*s::target = %.*s", FMT_ISTR(type->name), FMT_ISTR(typeArray->subType->name));

						data.completions.items.push_back(item);
					}

					if(TypeUnsizedArray *typeUnsizedArray = getType<TypeUnsizedArray>(type))
					{
						CompletionItem item;

						item.label = "target";
						item.kind = CompletionItemKind::TypeParameter;
						item.detail = ToString("typeid %.*s::target = %.*s", FMT_ISTR(type->name), FMT_ISTR(typeUnsizedArray->subType->name));

						data.completions.items.push_back(item);
					}

					if(TypeArgumentSet *typeArgumentSet = getType<TypeArgumentSet>(type))
					{
						if(!typeArgumentSet->types.empty())
						{
							CompletionItem item;

							item.label = "first";
							item.kind = CompletionItemKind::Value;
							item.detail = ToString("typeid %.*s::first = %.*s", FMT_ISTR(type->name), FMT_ISTR(typeArgumentSet->types.head->type->name));

							data.completions.items.push_back(item);
						}
					}

					if(TypeArgumentSet *typeArgumentSet = getType<TypeArgumentSet>(type))
					{
						if(!typeArgumentSet->types.empty())
						{
							CompletionItem item;

							item.label = "last";
							item.kind = CompletionItemKind::Value;
							item.detail = ToString("typeid %.*s::last = %.*s", FMT_ISTR(type->name), FMT_ISTR(typeArgumentSet->types.tail->type->name));

							data.completions.items.push_back(item);
						}
					}

					if(TypeClass *classType = getType<TypeClass>(type))
					{
						for(MatchData *curr = classType->aliases.head; curr; curr = curr->next)
						{
							CompletionItem item;

							item.label = std::string(curr->name->name.begin, curr->name->name.end);
							item.kind = CompletionItemKind::TypeParameter;
							item.detail = ToString("typeid %.*s::%.*s = %.*s", FMT_ISTR(type->name), FMT_ISTR(curr->name->name), FMT_ISTR(curr->type->name));

							data.completions.items.push_back(item);
						}

						for(MatchData *curr = classType->generics.head; curr; curr = curr->next)
						{
							CompletionItem item;

							item.label = std::string(curr->name->name.begin, curr->name->name.end);
							item.kind = CompletionItemKind::TypeParameter;
							item.detail = ToString("typeid %.*s::%.*s = %.*s", FMT_ISTR(type->name), FMT_ISTR(curr->name->name), FMT_ISTR(curr->type->name));

							data.completions.items.push_back(item);
						}
					}

					if(TypeStruct *structType = getType<TypeStruct>(type))
					{
						for(VariableHandle *curr = structType->members.head; curr; curr = curr->next)
						{
							CompletionItem item;

							item.label = std::string(curr->variable->name->name.begin, curr->variable->name->name.end);
							item.kind = CompletionItemKind::TypeParameter;
							item.detail = ToString("typeid %.*s::%.*s = %.*s", FMT_ISTR(type->name), FMT_ISTR(curr->variable->name->name), FMT_ISTR(curr->variable->type->name));

							data.completions.items.push_back(item);
						}

						for(ConstantData *curr = structType->constants.head; curr; curr = curr->next)
						{
							CompletionItem item;

							item.label = std::string(curr->name->name.begin, curr->name->name.end);
							item.kind = CompletionItemKind::Constant;
							item.detail = ToString("%.*s %.*s::%.*s", FMT_ISTR(curr->value->type->name), FMT_ISTR(type->name), FMT_ISTR(curr->name->name));

							data.completions.items.push_back(item);
						}

						{
							CompletionItem item;

							item.label = "hasMember";
							item.kind = CompletionItemKind::Function;
							item.detail = ToString("bool %.*s::hasMember(name)", FMT_ISTR(type->name));

							data.completions.items.push_back(item);
						}
					}

					if(TypeGenericClass *typeGenericClass = getType<TypeGenericClass>(type))
					{
						for(SynIdentifier *curr = typeGenericClass->proto->definition->aliases.head; curr; curr = getType<SynIdentifier>(curr->next))
						{
							CompletionItem item;

							item.label = std::string(curr->name.begin, curr->name.end);
							item.kind = CompletionItemKind::TypeParameter;
							item.detail = ToString("typeid %.*s::%.*s", FMT_ISTR(type->name), FMT_ISTR(curr->name));

							data.completions.items.push_back(item);
						}
					}
				};

				if(ExprMemberAccess *node = getType<ExprMemberAccess>(child))
				{
					if(data.ctx.debugMode)
					{
						fprintf(stderr, "INFO: Found ExprMemberAccess at position (%d:%d)\n", data.position.line, data.position.character);
						fprintf(stderr, "INFO: ExprMemberAccess location (%d:%d - %d:%d)\n", node->source->begin->line, node->source->begin->column, node->source->end->line, node->source->end->column + node->source->end->length);
						fprintf(stderr, "INFO: ExprMemberAccess value type '%.*s'\n", FMT_ISTR(node->value->type->name));
					}

					if(isType<TypeArray>(node->value->type) || isType<TypeUnsizedArray>(node->value->type))
					{
						CompletionItem item;

						item.label = "size";
						item.kind = CompletionItemKind::Field;

						item.detail = ToString("int %.*s::size", FMT_ISTR(node->value->type->name));

						item.preselect = true;

						data.completions.items.push_back(item);
					}
					else if(TypeClass *typeClass = getType<TypeClass>(node->value->type))
					{
						for(unsigned i = 0; i < data.context->exprCtx.functions.size(); i++)
						{
							auto function = data.context->exprCtx.functions[i];

							// Filter functions with location information
							if(function->scope->ownerType != node->value->type)
								continue;

							const char *start = function->name->name.begin;

							while(start < function->name->name.end)
							{
								if(start[0] == ':' && start[1] == ':')
								{
									start += 2;
									break;
								}

								start++;
							}

							if(start == function->name->name.end)
								start = function->name->name.begin;

							CompletionItem item;

							item.label = std::string(start, function->name->name.end);
							item.kind = CompletionItemKind::Method;
							item.detail = GetFunctionSignature(function);

							data.completions.items.push_back(item);
						}

						for(auto curr = typeClass->members.head; curr; curr = curr->next)
						{
							auto variable = curr->variable;

							CompletionItem item;

							item.label = std::string(variable->name->name.begin, variable->name->name.end);
							item.kind = CompletionItemKind::Field;
							item.detail = GetMemberSignature(node->value->type, variable);

							data.completions.items.push_back(item);
						}

						for(auto curr = typeClass->constants.head; curr; curr = curr->next)
						{
							CompletionItem item;

							item.label = std::string(curr->name->name.begin, curr->name->name.end);
							item.kind = CompletionItemKind::Constant;
							item.detail = GetMemberSignature(node->value->type, curr);

							data.completions.items.push_back(item);
						}

						for(auto curr = typeClass->aliases.head; curr; curr = curr->next)
						{
							CompletionItem item;

							item.label = std::string(curr->name->name.begin, curr->name->name.end);
							item.kind = CompletionItemKind::TypeParameter;
							item.detail = GetMemberSignature(node->value->type, curr);

							data.completions.items.push_back(item);
						}
					}

					if(ExprTypeLiteral *typeLiteral = getType<ExprTypeLiteral>(node->value))
					{
						addTypeidCompletionOptions(typeLiteral->value);
					}
				}
				else if(ExprErrorTypeMemberAccess *node = getType<ExprErrorTypeMemberAccess>(child))
				{
					if(data.ctx.debugMode)
					{
						fprintf(stderr, "INFO: Found ExprErrorTypeMemberAccess at position (%d:%d)\n", data.position.line, data.position.character);
						fprintf(stderr, "INFO: ExprErrorTypeMemberAccess location (%d:%d - %d:%d)\n", node->source->begin->line, node->source->begin->column, node->source->end->line, node->source->end->column + node->source->end->length);
						fprintf(stderr, "INFO: ExprErrorTypeMemberAccess value type '%.*s'\n", FMT_ISTR(node->value->name));
					}

					addTypeidCompletionOptions(node->value);
				}
			});
		}
		else
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Expression tree unavailable\n");
		}
	}

	rapidjson::Value result;

	if(completions.items.empty())
	{
		result.SetNull();
	}
	else
	{
		completions.SaveTo(result, response);
	}

	response.AddMember("result", result, response.GetAllocator());

	SendResponse(ctx, response);

	nullcClean();

	return true;
}

bool HandleDefinition(Context& ctx, rapidjson::Value& arguments, rapidjson::Document &response)
{
	auto documentIt = ctx.documents.find(arguments["textDocument"]["uri"].GetString());

	if(documentIt == ctx.documents.end())
	{
		fprintf(stderr, "ERROR: Failed to find document '%s'\n", arguments["textDocument"]["uri"].GetString());

		return RespondWithError(ctx, response, "", ErrorCode::InvalidParams, "failed to find target document");
	}

	auto position = Position(arguments["position"]);

	nullcAnalyze(documentIt->second.code.c_str());

	std::vector<LocationLink> locations;

	if(CompilerContext *context = nullcGetCompilerContext())
	{
		if(context->exprModule)
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Expression tree is available\n");

			FindEntityResponse result = FindEntityAtLocation(context, position, false);

			if(result)
			{
				if(VariableData *variable = result.targetVariable)
				{
					if(!variable->importModule && variable->source && variable->name)
					{
						LocationLink location;

						location.originSelectionRange = Range(Position(result.bestNode->begin->line, result.bestNode->begin->column), Position(result.bestNode->begin->line, result.bestNode->begin->column + result.bestNode->begin->length));

						location.targetUri = documentIt->first;

						location.targetRange = Range(Position(variable->source->begin->line, variable->source->begin->column), Position(variable->source->begin->line, variable->source->begin->column + variable->source->begin->length));

						location.targetSelectionRange = Range(Position(variable->name->begin->line, variable->name->begin->column), Position(variable->name->begin->line, variable->name->begin->column + variable->name->begin->length));

						locations.push_back(location);
					}
				}
				else if(FunctionData *function = result.targetFunction)
				{
					if(!function->importModule && function->source && function->name)
					{
						LocationLink location;

						location.originSelectionRange = Range(Position(result.bestNode->begin->line, result.bestNode->begin->column), Position(result.bestNode->begin->line, result.bestNode->begin->column + result.bestNode->begin->length));

						location.targetUri = documentIt->first;

						location.targetRange = Range(Position(function->source->begin->line, function->source->begin->column), Position(function->source->begin->line, function->source->begin->column + function->source->begin->length));

						location.targetSelectionRange = Range(Position(function->name->begin->line, function->name->begin->column), Position(function->name->begin->line, function->name->begin->column + function->name->begin->length));

						locations.push_back(location);
					}
				}
				else if(TypeBase *type = result.targetType)
				{
					SynBase *source = nullptr;
					SynIdentifier identifier = SynIdentifier(InplaceStr());

					if(TypeClass *typeClass = getType<TypeClass>(type))
					{
						source = typeClass->source;
						identifier = typeClass->identifier;
					}
					else if(TypeEnum *typeEnum = getType<TypeEnum>(type))
					{
						source = typeEnum->source;
						identifier = typeEnum->identifier;
					}
					else if(TypeGenericClassProto *typeGenericClassProto = getType<TypeGenericClassProto>(type))
					{
						source = typeGenericClassProto->source;
						identifier = typeGenericClassProto->identifier;
					}

					if(!type->importModule && source)
					{
						LocationLink location;

						location.originSelectionRange = Range(Position(result.bestNode->begin->line, result.bestNode->begin->column), Position(result.bestNode->begin->line, result.bestNode->begin->column + result.bestNode->begin->length));

						location.targetUri = documentIt->first;

						location.targetRange = Range(Position(source->begin->line, source->begin->column), Position(source->begin->line, source->begin->column + source->begin->length));

						location.targetSelectionRange = Range(Position(identifier.begin->line, identifier.begin->column), Position(identifier.begin->line, identifier.begin->column + identifier.begin->length));

						locations.push_back(location);
					}
				}
			}
		}
		else
		{
			if(ctx.debugMode)
				fprintf(stderr, "INFO: Expression tree unavailable\n");
		}
	}

	rapidjson::Value result;

	if(locations.empty())
	{
		result.SetNull();
	}
	else
	{
		result.SetArray();

		for(auto &&el : locations)
			result.PushBack(el.ToJson(response), response.GetAllocator());
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
	else if(strcmp(method, "textDocument/completion") == 0)
		return HandleCompletion(ctx, arguments, response);
	else if(strcmp(method, "textDocument/definition") == 0)
		return HandleDefinition(ctx, arguments, response);
	
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
