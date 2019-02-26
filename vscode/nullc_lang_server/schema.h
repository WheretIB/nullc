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
	/**
	* Folding range for a comment
	*/
	static const char *comment = "comment";

	/**
	* Folding range for a imports or includes
	*/
	static const char *imports = "imports";

	/**
	* Folding range for a region (e.g. `#region`)
	*/
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

	/**
	* Line position in a document (zero-based).
	*/
	int line = 0;

	/**
	* Character offset on a line in a document (zero-based). Assuming that the line is
	* represented as a string, the `character` value represents the gap between the
	* `character` and `character + 1`.
	*
	* If the character value is greater than the line length it defaults back to the
	* line length.
	*/
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

	/**
	* The range's start position.
	*/
	Position start;

	/**
	* The range's end position.
	*/
	Position end;
};

/**
* Represents a location inside a resource, such as a line inside a text file.
*/
struct Location
{
	Location() = default;

	Location(std::string uri, Range range): uri(uri), range(range)
	{
	}

	explicit Location(rapidjson::Value &source)
	{
		uri = source["uri"].GetString();
		range = Range(source["range"]);
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("uri", uri, document.GetAllocator());
		target.AddMember("range", range.ToJson(document), document.GetAllocator());
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	std::string uri;
	Range range;
};

/**
* Represents a folding range.
*/
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

	/**
	* The zero-based line number from where the folded range starts.
	*/
	int startLine = 0;

	/**
	* The zero-based character offset from where the folded range starts. If not defined, defaults to the length of the start line.
	*/
	int startCharacter = -1;

	/**
	* The zero-based line number where the folded range ends.
	*/
	int endLine = 0;

	/**
	* The zero-based character offset before the folded range ends. If not defined, defaults to the length of the end line.
	*/
	int endCharacter = -1;

	/**
	* Describes the kind of the folding range such as `comment' or 'region'. The kind
	* is used to categorize folding ranges and used by commands like 'Fold all comments'. See
	* [FoldingRangeKind](#FoldingRangeKind) for an enumeration of standardized kinds.
	*/
	const char *kind = nullptr;
};

/**
* Represents programming constructs like variables, classes, interfaces etc. that appear in a document. Document symbols can be
* hierarchical and they have two ranges: one that encloses its definition and one that points to its most interesting range,
* e.g. the range of an identifier.
*/
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

	/**
	* The name of this symbol. Will be displayed in the user interface and therefore must not be
	* an empty string or a string only consisting of white spaces.
	*/
	std::string name;

	/**
	* More detail for this symbol, e.g the signature of a function.
	*/
	std::string detail;

	/**
	* The kind of this symbol.
	*/
	SymbolKind kind;

	/**
	* Indicates if this symbol is deprecated.
	*/
	bool deprecated = false;

	/**
	* The range enclosing this symbol not including leading/trailing whitespace but everything else
	* like comments. This information is typically used to determine if the clients cursor is
	* inside the symbol to reveal in the symbol in the UI.
	*/
	Range range;

	/**
	* The range that should be selected and revealed when this symbol is being picked, e.g the name of a function.
	* Must be contained by the `range`.
	*/
	Range selectionRange;

	/**
	* Children of this symbol, e.g. properties of a class.
	*/
	std::vector<DocumentSymbol> children;
};

enum class DiagnosticSeverity
{
	None = 0,
	Error = 1,
	Warning = 2,
	Information = 3,
	Hint = 4,
};

/**
* Represents a related message and source code location for a diagnostic. This should be
* used to point to code locations that cause or related to a diagnostics, e.g when duplicating
* a symbol in a scope.
*/
struct DiagnosticRelatedInformation
{
	DiagnosticRelatedInformation() = default;

	explicit DiagnosticRelatedInformation(rapidjson::Value &source)
	{
		location = Location(source["location"]);
		message = source["message"].GetString();
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("location", location.ToJson(document), document.GetAllocator());
		target.AddMember("message", message, document.GetAllocator());
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	/**
	* The location of this related diagnostic information.
	*/
	Location location;

	/**
	* The message of this related diagnostic information.
	*/
	std::string message;
};

/**
* Represents a diagnostic, such as a compiler error or warning. Diagnostic objects are only valid in the scope of a resource.
*/
struct Diagnostic
{
	Diagnostic() = default;

	explicit Diagnostic(rapidjson::Value &json)
	{
		range = Range(json["range"]);
		severity = json.HasMember("severity") ? DiagnosticSeverity(json["severity"].GetUint()) : DiagnosticSeverity::None;
		code = json.HasMember("code") ? json["severity"].GetString() : "";
		source = json.HasMember("source") ? json["source"].GetString() : "";
		message = json["range"].GetString();

		if(json.HasMember("relatedInformation"))
		{
			for(auto &&el : json["relatedInformation"].GetArray())
				relatedInformation.push_back(DiagnosticRelatedInformation(el));
		}
	}

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("range", range.ToJson(document), document.GetAllocator());

		if(severity != DiagnosticSeverity::None)
			target.AddMember("severity", unsigned(severity), document.GetAllocator());

		if(!code.empty())
			target.AddMember("code", code, document.GetAllocator());

		if(!source.empty())
			target.AddMember("source", source, document.GetAllocator());

		target.AddMember("message", message, document.GetAllocator());

		if(!relatedInformation.empty())
		{
			rapidjson::Value arr;
			arr.SetArray();

			for(auto &&el : relatedInformation)
				arr.PushBack(el.ToJson(document), document.GetAllocator());

			target.AddMember("relatedInformation", arr, document.GetAllocator());
		}
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	/**
	* The range at which the message applies.
	*/
	Range range;

	/**
	* The diagnostic's severity. Can be omitted. If omitted it is up to the
	* client to interpret diagnostics as error, warning, info or hint.
	*/
	DiagnosticSeverity severity = DiagnosticSeverity::None;

	/**
	* The diagnostic's code, which might appear in the user interface.
	*/
	std::string code;

	/**
	* A human-readable string describing the source of this
	* diagnostic, e.g. 'typescript' or 'super lint'.
	*/
	std::string source;

	/**
	* The diagnostic's message.
	*/
	std::string message;

	/**
	* An array of related diagnostic information, e.g. when symbol-names within
	* a scope collide all definitions can be marked via this property.
	*/
	std::vector<DiagnosticRelatedInformation> relatedInformation;
};
