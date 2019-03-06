#pragma once

#include <assert.h>

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

/**
* How a completion was triggered
*/
enum class CompletionTriggerKind
{
	Unknown = 0,

	/**
	* Completion was triggered by typing an identifier (24x7 code
	* complete), manual invocation (e.g Ctrl+Space) or via API.
	*/
	Invoked = 1,

	/**
	* Completion was triggered by a trigger character specified by
	* the `triggerCharacters` properties of the `CompletionRegistrationOptions`.
	*/
	TriggerCharacter = 2,

	/**
	* Completion was re-triggered as the current completion list is incomplete.
	*/
	TriggerForIncompleteCompletions = 3,
};

/**
* Defines whether the insert text in a completion item should be interpreted as
* plain text or a snippet.
*/
enum class InsertTextFormat
{
	None = 0,

	/**
	* The primary text to be inserted is treated as a plain string.
	*/
	PlainText = 1,

	/**
	* The primary text to be inserted is treated as a snippet.
	*
	* A snippet can define tab stops and placeholders with `$1`, `$2`
	* and `${3:foo}`. `$0` defines the final tab stop, it defaults to
	* the end of the snippet. Placeholders with equal identifiers are linked,
	* that is typing in one will update others too.
	*/
	Snippet = 2,
};

enum class CompletionItemKind
{
	None = 0,
	Text = 1,
	Method = 2,
	Function = 3,
	Constructor = 4,
	Field = 5,
	Variable = 6,
	Class = 7,
	Interface = 8,
	Module = 9,
	Property = 10,
	Unit = 11,
	Value = 12,
	Enum = 13,
	Keyword = 14,
	Snippet = 15,
	Color = 16,
	File = 17,
	Reference = 18,
	Folder = 19,
	EnumMember = 20,
	Constant = 21,
	Struct = 22,
	Event = 23,
	Operator = 24,
	TypeParameter = 25,
};

/**
* Defines how the host (editor) should sync document changes to the language server.
*/
enum class TextDocumentSyncKind
{
	/**
	* Documents should not be synced at all.
	*/
	None = 0,

	/**
	* Documents are synced by always sending the full content
	* of the document.
	*/
	Full = 1,

	/**
	* Documents are synced by sending the full content on open.
	* After that only incremental updates to the document are
	* send.
	*/
	Incremental = 2,
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
}

/**
* Describes the content type that a client supports in various
* result literals like `Hover`, `ParameterInfo` or `CompletionItem`.
*
* Please note that `MarkupKinds` must not start with a `$`. This kinds
* are reserved for internal usage.
*/
namespace MarkupKind
{
	/**
	* Plain text is supported as a content format
	*/
	static const char *PlainText = "plaintext";

	/**
	* Markdown is supported as a content format
	*/
	static const char *Markdown = "markdown";
};

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

	T& operator *()
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

struct LocationLink
{
	LocationLink() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		if(originSelectionRange)
			target.AddMember("originSelectionRange", originSelectionRange->ToJson(document), document.GetAllocator());

		target.AddMember("targetUri", targetUri, document.GetAllocator());
		target.AddMember("targetRange", targetRange.ToJson(document), document.GetAllocator());
		target.AddMember("targetSelectionRange", targetSelectionRange.ToJson(document), document.GetAllocator());
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	/**
	* Span of the origin of this link.
	*
	* Used as the underlined span for mouse interaction. Defaults to the word range at
	* the mouse position.
	*/
	Optional<Range> originSelectionRange;

	/**
	* The target resource identifier of this link.
	*/
	std::string targetUri;

	/**
	* The full target range of this link. If the target for example is a symbol then target range is the
	* range enclosing this symbol not including leading/trailing whitespace but everything else
	* like comments. This information is typically used to highlight the range in the editor.
	*/
	Range targetRange;

	/**
	* The range that should be selected and revealed when this link is being followed, e.g the name of a function.
	* Must be contained by the the `targetRange`. See also `DocumentSymbol#range`
	*/
	Range targetSelectionRange;
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

struct CompletionContext
{
	CompletionContext() = default;

	explicit CompletionContext(rapidjson::Value &source)
	{
		triggerKind = CompletionTriggerKind(source["triggerKind"].GetUint());

		if(source.HasMember("triggerCharacter"))
			triggerCharacter = source["triggerCharacter"].GetString();
	}

	/**
	* The location of this related diagnostic information.
	*/
	CompletionTriggerKind triggerKind;

	/**
	* The message of this related diagnostic information.
	*/
	std::string triggerCharacter;
};

struct CompletionItem
{
	CompletionItem() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("label", label, document.GetAllocator());

		if(kind != CompletionItemKind::None)
			target.AddMember("kind", int(kind), document.GetAllocator());

		if(!detail.empty())
			target.AddMember("detail", detail, document.GetAllocator());

		if(deprecated)
			target.AddMember("deprecated", deprecated, document.GetAllocator());

		if(preselect)
			target.AddMember("preselect", preselect, document.GetAllocator());

		if(!sortText.empty())
			target.AddMember("sortText", sortText, document.GetAllocator());

		if(!filterText.empty())
			target.AddMember("filterText", filterText, document.GetAllocator());

		if(insertTextFormat != InsertTextFormat::None)
			target.AddMember("insertTextFormat", int(insertTextFormat), document.GetAllocator());

		if(!commitCharacters.empty())
		{
			rapidjson::Value arr;
			arr.SetArray();

			for(auto &&el : commitCharacters)
				arr.PushBack(rapidjson::Value(el, document.GetAllocator()), document.GetAllocator());

			target.AddMember("commitCharacters", arr, document.GetAllocator());
		}
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	/**
	* The label of this completion item. By default
	* also the text that is inserted when selecting
	* this completion.
	*/
	std::string label;

	/**
	* The kind of this completion item. Based of the kind
	* an icon is chosen by the editor.
	* Optional.
	*/
	CompletionItemKind kind = CompletionItemKind::None;

	/**
	* A human-readable string with additional information
	* about this item, like type or symbol information.
	* Optional.
	*/
	std::string detail;

	/**
	* A human-readable string that represents a doc-comment.
	*/
	//documentation

	/**
	* Indicates if this item is deprecated.
	* Optional.
	*/
	bool deprecated = false;

	/**
	* Select this item when showing.
	*
	* *Note* that only one completion item can be selected and that the
	* tool / client decides which item that is. The rule is that the *first*
	* item of those that match best is selected.
	* Optional
	*/
	bool preselect = false;

	/**
	* A string that should be used when comparing this item
	* with other items. When `falsy` the label is used.
	* Optional.
	*/
	std::string sortText;

	/**
	* A string that should be used when filtering a set of
	* completion items. When `falsy` the label is used.
	* Optional.
	*/
	std::string filterText;

	/**
	* The format of the insert text. The format applies to both the `insertText` property
	* and the `newText` property of a provided `textEdit`.
	* Optional.
	*/
	InsertTextFormat insertTextFormat = InsertTextFormat::None;

	/**
	* An edit which is applied to a document when selecting this completion. When an edit is provided the value of
	* `insertText` is ignored.
	*
	* *Note:* The range of the edit must be a single line range and it must contain the position at which completion
	* has been requested.
	* Optional
	*/
	//TextEdit textEdit;

	/**
	* An optional array of additional text edits that are applied when
	* selecting this completion. Edits must not overlap (including the same insert position)
	* with the main edit nor with themselves.
	*
	* Additional text edits should be used to change text unrelated to the current cursor position
	* (for example adding an import statement at the top of the file if the completion item will
	* insert an unqualified type).
	* Optional
	*/
	//std::vector<TextEdit> additionalTextEdits;

	/**
	* An optional set of characters that when pressed while this completion is active will accept it first and
	* then type that character. *Note* that all commit characters should have `length=1` and that superfluous
	* characters will be ignored.
	* Optional.
	*/
	std::vector<std::string> commitCharacters;

	/**
	* An optional command that is executed *after* inserting this completion. *Note* that
	* additional modifications to the current document should be described with the
	* additionalTextEdits-property.
	* Optional
	*/
	//Command command;
};

/**
* Represents a collection of [completion items](#CompletionItem) to be presented
* in the editor.
*/
struct CompletionList
{
	CompletionList() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("isIncomplete", isIncomplete, document.GetAllocator());

		if(!items.empty())
		{
			rapidjson::Value arr;
			arr.SetArray();

			for(auto &&el : items)
				arr.PushBack(el.ToJson(document), document.GetAllocator());

			target.AddMember("items", arr, document.GetAllocator());
		}
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	/**
	* This list it not complete. Further typing should result in recomputing
	* this list.
	*/
	bool isIncomplete = false;

	/**
	* The completion items.
	*/
	std::vector<CompletionItem> items;
};

/**
* A `MarkupContent` literal represents a string value which content is interpreted base on its
* kind flag. Currently the protocol supports `plaintext` and `markdown` as markup kinds.
*
* If the kind is `markdown` then the value can contain fenced code blocks like in GitHub issues.
* See https://help.github.com/articles/creating-and-highlighting-code-blocks/#syntax-highlighting
*
* Here is an example how such a string can be constructed using JavaScript / TypeScript:
* ```typescript
* let markdown: MarkdownContent = {
*  kind: MarkupKind.Markdown,
*	value: [
*		'# Header',
*		'Some text',
*		'```typescript',
*		'someCode();',
*		'```'
*	].join('\n')
* };
* ```
*
* *Please Note* that clients might sanitize the return markdown. A client could decide to
* remove HTML from the markdown to avoid script execution.
*/
struct MarkupContent
{
	MarkupContent() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("kind", rapidjson::Value(kind, document.GetAllocator()), document.GetAllocator());
		target.AddMember("value", value, document.GetAllocator());
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	/**
	* The type of the Markup
	*/
	const char *kind = MarkupKind::PlainText;

	/**
	* The content itself
	*/
	std::string value;
};

/**
* The result of a hover request.
*/
struct Hover
{
	Hover() = default;

	void SaveTo(rapidjson::Value &target, rapidjson::Document &document)
	{
		target.SetObject();

		target.AddMember("contents", contents.ToJson(document), document.GetAllocator());

		if(range)
			target.AddMember("range", range->ToJson(document), document.GetAllocator());
	}

	rapidjson::Value ToJson(rapidjson::Document &document)
	{
		rapidjson::Value result;
		SaveTo(result, document);
		return result;
	}

	/**
	* The hover's content
	*/
	MarkupContent contents;

	/**
	* An optional range is a range inside a text document
	* that is used to visualize a hover, e.g. by changing the background color.
	*/
	Optional<Range> range;
};
