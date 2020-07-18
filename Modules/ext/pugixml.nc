// ext.pugixml
import std.file;

class PugiNamespace
{
	const int	parse_minimal			= 0x0000,
				parse_pi				= 0x0001,
				parse_comments			= 0x0002,
				parse_cdata				= 0x0004,
				parse_ws_pcdata			= 0x0008,
				parse_escapes			= 0x0010,
				parse_eol				= 0x0020,
				parse_wconv_attribute	= 0x0040,
				parse_declaration		= 0x0100,
				parse_default			= parse_cdata | parse_escapes | parse_wconv_attribute | parse_eol;
	
	const int	encoding_auto = 0,
				encoding_utf8,
				encoding_utf16_le,
				encoding_utf16_be,
				encoding_utf16,
				encoding_utf32_le,
				encoding_utf32_be,
				encoding_utf32,
				encoding_wchar;

	const int	status_ok = 0,
				status_file_not_found,
				status_io_error,
				status_out_of_memory,
				status_internal_error,
				status_unrecognized_tag,
				status_bad_pi,
				status_bad_comment,
				status_bad_cdata,
				status_bad_doctype,
				status_bad_pcdata,
				status_bad_start_element,
				status_bad_attribute,
				status_bad_end_element,
				status_end_element_mismatch;

	const int	format_indent			= 0x01,
				format_write_bom		= 0x02,
				format_raw				= 0x04,
				format_no_declaration	= 0x08,
				format_default			= format_indent;
	
	const int	node_null = 0,		///< Undifferentiated entity
				node_document,		///< A document tree's absolute root.
				node_element,		///< E.g. '<...>'
				node_pcdata,		///< E.g. '>...<'
				node_cdata,			///< E.g. '<![CDATA[...]]>'
				node_comment,		///< E.g. '<!--...-->'
				node_pi,			///< E.g. '<?...?>'
				node_declaration;	///< E.g. '<?xml ...?>'
}
PugiNamespace pugi;

// const string implementation
class const_string
{
	char[] arr;
	int size{ get{ return arr.size; } };
}
auto operator=(const_string ref l, char[] arr)
{
	l.arr = arr;
	return l;
}
const_string const_string(char[] arr)
{
	const_string ret = arr;
	return ret;
}
char operator[](const_string ref l, int index)
{
	return l.arr[index];
}
int operator==(const_string a, b)
{
	return a.arr == b.arr;
}
int operator==(const_string a, char[] b)
{
	return a.arr == b;
}
int operator==(char[] a, const_string b)
{
	return a == b.arr;
}
int operator!=(const_string a, b)
{
	return a.arr != b.arr;
}
int operator!=(const_string a, char[] b)
{
	return a.arr != b;
}
int operator!=(char[] a, const_string b)
{
	return a != b.arr;
}
const_string operator+(const_string a, b)
{
	return const_string(a.arr + b.arr);
}
const_string operator+(const_string a, char[] b)
{
	return const_string(a.arr + b);
}
const_string operator+(char[] a, const_string b)
{
	return const_string(a + b.arr);
}

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
	xml_attribute prepend_attribute(char[] name);
	xml_attribute insert_attribute_after(char[] name, xml_attribute ref attr);
	xml_attribute insert_attribute_before(char[] name, xml_attribute ref attr);

	xml_attribute append_copy(xml_attribute ref proto);
	xml_attribute prepend_copy(xml_attribute ref proto);
	xml_attribute insert_copy_after(xml_attribute ref proto, xml_attribute ref attr);
	xml_attribute insert_copy_before(xml_attribute ref proto, xml_attribute ref attr);

	xml_node append_child(xml_node_type type = pugi.node_element);
	xml_node prepend_child(xml_node_type type = pugi.node_element);
	xml_node insert_child_after(xml_node_type type, xml_node ref node);
	xml_node insert_child_before(xml_node_type type, xml_node ref node);
	
	xml_node append_child(char[] name);
	xml_node prepend_child(char[] name);
	xml_node insert_child_after(char[] name, xml_node ref node);
	xml_node insert_child_before(char[] name, xml_node ref node);

	xml_node append_copy(xml_node ref proto);
	xml_node prepend_copy(xml_node ref proto);
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

class xml_document_impl
{
	void ref	document;
	void		finalize();
}

class xml_document
{
	xml_document_impl ref impl;

	/**
	 * Default ctor, makes empty document
	 */
	void xml_document(){ impl = new xml_document_impl; }

	xml_parse_result ref load(char[] contents, int options = pugi.parse_default);

	xml_parse_result ref load_file(char[] name, int options = pugi.parse_default, encoding_t encoding = pugi.encoding_auto);

	xml_parse_result ref load_buffer(char[] contents, int size, int options = pugi.parse_default, encoding_t encoding = pugi.encoding_auto);

	xml_parse_result ref load_buffer_inplace(char[] contents, int size, int options = pugi.parse_default, encoding_t encoding = pugi.encoding_auto);

	void save(xml_writer writer, char[] indent = "\t", int flags = pugi.format_default, encoding_t encoding = pugi.encoding_auto);

	int save_file(char[] name, char[] indent = "\t", int flags = pugi.format_default, encoding_t encoding = pugi.encoding_auto);
	
	xml_node	root();
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

bool bool(xml_node x){ return !!x; }
bool bool(xml_attribute x){ return !!x; }
