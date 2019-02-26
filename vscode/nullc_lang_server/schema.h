#pragma once

#define RAPIDJSON_HAS_STDSTRING 1

#include "external/rapidjson/document.h"

#include <string>
#include <vector>

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

enum class SymbolKind
{
	File = 1,
	Module = 2,
	Namespace = 3,
	Package = 4,
	Class = 5,
	Method = 6,
	Property = 7,
	Field = 8,
	Constructor = 9,
	Enum = 10,
	Interface = 11,
	Function = 12,
	Variable = 13,
	Constant = 14,
	String = 15,
	Number = 16,
	Boolean = 17,
	Array = 18,
	Object = 19,
	Key = 20,
	Null = 21,
	EnumMember = 22,
	Struct = 23,
	Event = 24,
	Operator = 25,
	TypeParameter = 26,
};

namespace FoldingRangeKind
{
	static const char *comment = "comment";
	static const char *imports = "imports";
	static const char *region = "region";
};

struct Position
{
	Position() = default;

	Position(int line, int character): line(line), character(character)
	{
	}

	explicit Position(rapidjson::Value &source)
	{
		if(source.IsNull())
			return;

		line = source["line"].GetUint();
		character = source["character"].GetUint();
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("line", line, document.GetAllocator());
		target.AddMember("character", character, document.GetAllocator());
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	int line = 0;
	int character = 0;
};

struct Range
{
	Range() = default;

	Range(Position start, Position end): start(start), end(end)
	{
	}

	explicit Range(rapidjson::Value &source)
	{
		if(source.IsNull())
			return;

		start = Position(source["start"]);
		end = Position(source["end"]);
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("start", start.ToJson(document), document.GetAllocator());
		target.AddMember("end", end.ToJson(document), document.GetAllocator());
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	Position start;
	Position end;
};

struct FoldingRange
{
	FoldingRange() = default;

	FoldingRange(int startLine, int startCharacter, int endLine, int endCharacter, const char *kind): startLine(startLine), startCharacter(startCharacter), endLine(endLine), endCharacter(endCharacter), kind(kind)
	{
	}

	explicit FoldingRange(rapidjson::Value &source)
	{
		if(source.IsNull())
			return;

		startLine = source["startLine"].GetInt();

		if(source.HasMember("startCharacter"))
			startCharacter = source["startCharacter"].GetInt();

		endLine = source["endLine"].GetInt();

		if(source.HasMember("endCharacter"))
			endCharacter = source["endCharacter"].GetInt();

		if(source.HasMember("kind"))
			kind = source["kind"].GetString();
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("startLine", startLine, document.GetAllocator());

		if(startCharacter != -1)
			target.AddMember("startCharacter", startCharacter, document.GetAllocator());

		target.AddMember("endLine", endLine, document.GetAllocator());

		if(endCharacter != -1)
			target.AddMember("endCharacter", endCharacter, document.GetAllocator());

		if(kind)
			target.AddMember("kind", rapidjson::StringRef(kind), document.GetAllocator());
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	int startLine = 0;
	int startCharacter = -1;

	int endLine = 0;
	int endCharacter = -1;

	const char *kind = nullptr;
};

struct DocumentSymbol
{
	DocumentSymbol() = default;

	explicit DocumentSymbol(rapidjson::Value &source)
	{
		name = source["name"].GetString();
		detail = source.HasMember("detail") ? source["detail"].GetString() : "";
		kind = SymbolKind(source["kind"].GetUint());
		deprecated = source.HasMember("deprecated") ? source["deprecated"].GetBool() : false;
		range = Range(source["range"]);
		selectionRange = Range(source["selectionRange"]);

		if(source.HasMember("children"))
		{
			for(auto &&el : source["children"].GetArray())
				children.push_back(DocumentSymbol(el));
		}
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("name", name, document.GetAllocator());

		if(!detail.empty())
			target.AddMember("detail", detail, document.GetAllocator());

		target.AddMember("kind", unsigned(kind), document.GetAllocator());

		if(deprecated)
			target.AddMember("deprecated", deprecated, document.GetAllocator());

		target.AddMember("range", range.ToJson(document), document.GetAllocator());

		target.AddMember("selectionRange", selectionRange.ToJson(document), document.GetAllocator());

		if(!children.empty())
		{
			rapidjson::Value arr;
			arr.SetArray();

			for(auto &&el : children)
				arr.PushBack(el.ToJson(document), document.GetAllocator());

			target.AddMember("children", arr, document.GetAllocator());
		}
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	std::string name;
	std::string detail;
	SymbolKind kind;
	bool deprecated = false;
	Range range;
	Range selectionRange;
	std::vector<DocumentSymbol> children;
};
