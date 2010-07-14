// ext.pugixml
import std.file;

class PugiNamespace
{
	int parse_minimal;
	int parse_pi;
	int parse_comments;
	int parse_cdata;
	int parse_ws_pcdata;
	int parse_escapes;
	int parse_eol;
	int parse_wconv_attribute;
	int parse_declaration;
	int parse_default;
	
	int encoding_auto,      //!< Auto-detect input encoding using BOM or </<? detection; use UTF8 if BOM is not found
	encoding_utf8,      //!< UTF8 encoding
	encoding_utf16_le,  //!< Little-endian UTF16
	encoding_utf16_be,  //!< Big-endian UTF16
	encoding_utf16,     //!< UTF16 with native endianness
	encoding_utf32_le,  //!< Little-endian UTF32
	encoding_utf32_be,  //!< Big-endian UTF32
	encoding_utf32,     //!< UTF32 with native endianness
	encoding_wchar;      //!< The same encoding wchar_t has (either UTF16 or UTF32)
	
	int status_ok, ///< No error

	status_file_not_found, ///< File was not found during load_file()
	status_io_error, ///< Error reading from file/stream
	status_out_of_memory, ///< Could not allocate memory
	status_internal_error, ///< Internal error occured
	status_unrecognized_tag, ///< Parser could not determine tag type
	status_bad_pi, ///< Parsing error occured while parsing document declaration/processing instruction (<?...?>)
	status_bad_comment, ///< Parsing error occured while parsing comment (<!--...-->)
	status_bad_cdata, ///< Parsing error occured while parsing CDATA section (<![CDATA[...]]>)
	status_bad_doctype, ///< Parsing error occured while parsing document type declaration
	status_bad_pcdata, ///< Parsing error occured while parsing PCDATA section (>...<)
	status_bad_start_element, ///< Parsing error occured while parsing start element tag (<name ...>)
	status_bad_attribute, ///< Parsing error occured while parsing element attribute
	status_bad_end_element, ///< Parsing error occured while parsing end element tag (</name>)
	status_end_element_mismatch; ///< There was a mismatch of start-end tags (closing tag had incorrect name, some tag was not closed or there was an excessive closing tag)
	
	int format_indent;
	int format_write_bom;
	int format_raw;
	int format_no_declaration;
	int format_default;
	
	int node_null,			///< Undifferentiated entity
	node_document,		///< A document tree's absolute root.
	node_element,		///< E.g. '<...>'
	node_pcdata,		///< E.g. '>...<'
	node_cdata,			///< E.g. '<![CDATA[...]]>'
	node_comment,		///< E.g. '<!--...-->'
	node_pi,			///< E.g. '<?...?>'
	node_declaration;	///< E.g. '<?xml ...?>'
}
PugiNamespace pugi;

pugi.parse_minimal			= 0x0000;
pugi.parse_pi				= 0x0001;
pugi.parse_comments			= 0x0002;
pugi.parse_cdata			= 0x0004;
pugi.parse_ws_pcdata		= 0x0008;
pugi.parse_escapes			= 0x0010;
pugi.parse_eol				= 0x0020;
pugi.parse_wconv_attribute	= 0x0040;
pugi.parse_declaration		= 0x0100;
pugi.parse_default			= pugi.parse_cdata | pugi.parse_escapes | pugi.parse_wconv_attribute | pugi.parse_eol;

pugi.status_ok					= 0;
pugi.status_file_not_found		= 1;
pugi.status_io_error			= 2;
pugi.status_out_of_memory		= 3;
pugi.status_internal_error		= 4;
pugi.status_unrecognized_tag	= 5;
pugi.status_bad_pi				= 6;
pugi.status_bad_comment			= 7;
pugi.status_bad_cdata			= 8;
pugi.status_bad_doctype			= 9;
pugi.status_bad_pcdata			= 10;
pugi.status_bad_start_element	= 11;
pugi.status_bad_attribute		= 12;
pugi.status_bad_end_element		= 13;
pugi.status_end_element_mismatch= 14;

pugi.encoding_auto			= 0;
pugi.encoding_utf8			= 1;
pugi.encoding_utf16_le		= 2;
pugi.encoding_utf16_be		= 3;
pugi.encoding_utf16			= 4;
pugi.encoding_utf32_le		= 5;
pugi.encoding_utf32_be		= 6;
pugi.encoding_utf32			= 7;
pugi.encoding_wchar			= 8;

pugi.format_indent			= 0x01;
pugi.format_write_bom		= 0x02;
pugi.format_raw				= 0x04;
pugi.format_no_declaration	= 0x08;
pugi.format_default			= pugi.format_indent;

pugi.node_null			= 0;
pugi.node_document		= 1;
pugi.node_element		= 2;
pugi.node_pcdata		= 3;
pugi.node_cdata			= 4;
pugi.node_comment		= 5;
pugi.node_pi			= 6;
pugi.node_declaration	= 7;

typedef int xml_parse_status;
typedef int encoding_t;
typedef int xml_node_type;

typedef void ref(char[]) xml_writer;
// Function returns writer to a File
auto xml_writer_file(File file)
{
	return auto(char[] data)
	{
		file.Write(data);
	};
}

// Function returns writer to a string
auto xml_writer_string(char[] ref str)
{
	return auto(char[] data)
	{
		*str += data;
	};
}

class xml_attribute
{
	void ref attribute;
	void xml_attribute(){ attribute = nullptr; }

	xml_attribute next_attribute();
	xml_attribute previous_attribute();

	int as_int();
	double as_double();
	float as_float();
	int as_bool();

	int set_name(char[] rhs);

	int set_value(char[] rhs);
	int set_value(int rhs);
	int set_value(double rhs);

	int empty();

	const_string name();
	const_string value();
}
xml_attribute ref operator=(xml_attribute ref att, char[] rhs);
xml_attribute ref operator=(xml_attribute ref att, int rhs);
xml_attribute ref operator=(xml_attribute ref att, double rhs);
	
int operator!(xml_attribute ref att);
int operator==(xml_attribute ref left, right);
int operator!=(xml_attribute ref left, right);
int operator<(xml_attribute ref left, right);
int operator>(xml_attribute ref left, right);
int operator<=(xml_attribute ref left, right);
int operator>=(xml_attribute ref left, right);

class xml_node
{
	void ref	node;

	/**
	 * Default ctor. Constructs an empty node.
	 */
	void xml_node(){ node = nullptr; }

	/*typedef xml_node_iterator iterator;
	typedef xml_attribute_iterator attribute_iterator;

	iterator begin() const;
	iterator end() const;

	attribute_iterator attributes_begin();
	attribute_iterator attributes_end();*/

	int empty();

	xml_node_type type();
	const_string name();
	const_string value();

	xml_node child(char[] name);
	xml_attribute attribute(char[] name);

	xml_node next_sibling(char[] name);
	xml_node next_sibling();

	xml_node previous_sibling(char[] name);
	xml_node previous_sibling();

	xml_node parent();
	xml_node root();

	const_string child_value();
	const_string child_value(char[] name);
	
	int set_name(char[] rhs);
	int set_value(char[] rhs);

	xml_attribute append_attribute(char[] name);
	xml_attribute insert_attribute_after(char[] name, xml_attribute ref attr);
	xml_attribute insert_attribute_before(char[] name, xml_attribute ref attr);
	xml_attribute append_copy(xml_attribute ref proto);
	xml_attribute insert_copy_after(xml_attribute ref proto, xml_attribute ref attr);
	xml_attribute insert_copy_before(xml_attribute ref proto, xml_attribute ref attr);

	xml_node append_child(xml_node_type type = pugi.node_element);
	xml_node insert_child_after(xml_node_type type, xml_node ref node);
	xml_node insert_child_before(xml_node_type type, xml_node ref node);
	xml_node append_copy(xml_node ref proto);
	xml_node insert_copy_after(xml_node ref proto, xml_node ref node);
	xml_node insert_copy_before(xml_node ref proto, xml_node ref node);

	void remove_attribute(xml_attribute ref a);
	void remove_attribute(char[] name);

	void remove_child(xml_node ref n);
	void remove_child(char[] name);

	xml_attribute first_attribute();
    xml_attribute last_attribute();

	xml_node first_child();
    xml_node last_child();

	xml_node find_child_by_attribute(char[] name, char[] attr_name, char[] attr_value);
	xml_node find_child_by_attribute(char[] attr_name, char[] attr_value);

	xml_node first_element_by_path(char[] path, char delimiter = '/');

	int traverse(int ref(xml_node) begin, for_each, end);
	void print(xml_writer writer, char[] indent = "\t", int flags = pugi.format_default, encoding_t encoding = pugi.encoding_auto, int depth = 0);

	int offset_debug();

	xml_attribute find_attribute(void ref(xml_attribute) pred);
	xml_node find_child(void ref(xml_node) pred);
	xml_node find_node(void ref(xml_node) pred);
}

int operator!(xml_node ref node);
int operator==(xml_node ref left, right);
int operator!=(xml_node ref left, right);
int operator<(xml_node ref left, right);
int operator>(xml_node ref left, right);
int operator<=(xml_node ref left, right);
int operator>=(xml_node ref left, right);

class xml_parse_result
{
	/// Parsing status (\see xml_parse_status)
	xml_parse_status status;

	/// Last parsed offset (in bytes from file/string start)
	int offset;

	/// Source document encoding
	encoding_t encoding;

	/// Get error description
	const_string description();
}

class xml_document
{
	void ref	document;

	/**
	 * Default ctor, makes empty document
	 */
	void xml_document(){ }

	xml_parse_result ref load(char[] contents, int options = pugi.parse_default);

	xml_parse_result ref load_file(char[] name, int options = pugi.parse_default, encoding_t encoding = pugi.encoding_auto);

	xml_parse_result ref load_buffer(char[] contents, int size, int options = pugi.parse_default, encoding_t encoding = pugi.encoding_auto);

	xml_parse_result ref load_buffer_inplace(char[] contents, int size, int options = pugi.parse_default, encoding_t encoding = pugi.encoding_auto);

	void save(xml_writer writer, char[] indent = "\t", int flags = pugi.format_default, encoding_t encoding = pugi.encoding_auto);

	int save_file(char[] name, char[] indent = "\t", int flags = pugi.format_default, encoding_t encoding = pugi.encoding_auto);
	
	xml_node	root();
}
auto xml_document()
{
	xml_document r; return r;
}

auto xml_node()
{
	xml_node l;
	return l;
}
auto xml_attribute()
{
	xml_attribute a;
	return a;
}
// xml_node constror from xml_document
auto xml_node(xml_document ref r)
{
	xml_node l;
	l = r.root();
	return l;
}
// operator for the same thing
auto operator=(xml_node ref l, xml_document ref r)
{
	*l = r.root();
	return l;
}
