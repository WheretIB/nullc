#include "pugi.h"

#include "../nullc.h"
#include "../nullbind.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <new>
#include "../../external/pugixml/pugixml.hpp"

namespace
{
	uintptr_t PodWrap(pugi::xml_node &node)
	{
		assert(sizeof(node) == sizeof(uintptr_t));

		uintptr_t res;
		memcpy(&res, &node, sizeof(uintptr_t));
		return res;
	}

	uintptr_t PodWrap(pugi::xml_attribute& attribute)
	{
		assert(sizeof(attribute) == sizeof(uintptr_t));

		uintptr_t res;
		memcpy(&res, &attribute, sizeof(uintptr_t));
		return res;
	}
}

namespace NULLCPugiXML
{
	class xml_attribute
	{
	public:
		pugi::xml_attribute attribute;	// pointer sized
	};
	xml_attribute xml_attribute__next_attribute(xml_attribute* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->attribute.next_attribute();
		return ret;
	}
	xml_attribute xml_attribute__previous_attribute(xml_attribute* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->attribute.previous_attribute();
		return ret;
	}

	int		xml_attribute__as_int(xml_attribute* ptr)
	{
		return ptr->attribute.as_int();
	}
	double	xml_attribute__as_double(xml_attribute* ptr)
	{
		return ptr->attribute.as_double();
	}
	float	xml_attribute__as_float(xml_attribute* ptr)
	{
		return ptr->attribute.as_float();
	}
	int		xml_attribute__as_bool(xml_attribute* ptr)
	{
		return ptr->attribute.as_bool();
	}

	int		xml_attribute__set_name(NULLCArray rhs, xml_attribute* ptr)
	{
		return ptr->attribute.set_name(rhs.ptr);
	}

	int		xml_attribute__set_valueS(NULLCArray rhs, xml_attribute* ptr)
	{
		return ptr->attribute.set_value(rhs.ptr);
	}
	int		xml_attribute__set_valueI(int rhs, xml_attribute* ptr)
	{
		return ptr->attribute.set_value(rhs);
	}
	int		xml_attribute__set_valueD(double rhs, xml_attribute* ptr)
	{
		return ptr->attribute.set_value(rhs);
	}

	int		xml_attribute__empty(xml_attribute* ptr)
	{
		return ptr->attribute.empty();
	}

	NULLCArray	xml_attribute__name(xml_attribute* ptr)
	{
		NULLCArray ret;
		ret.ptr = (pugi::char_t*)ptr->attribute.name();
		ret.len = (unsigned)strlen(ret.ptr) + 1;
		return ret;
	}
	NULLCArray	xml_attribute__value(xml_attribute* ptr)
	{
		NULLCArray ret;
		ret.ptr = (pugi::char_t*)ptr->attribute.value();
		ret.len = (unsigned)strlen(ret.ptr) + 1;
		return ret;
	}

	xml_attribute* xml_attribute__operatorSetS(xml_attribute* att, NULLCArray rhs)
	{
		att->attribute = rhs.ptr;
		return att;
	}
	xml_attribute* xml_attribute__operatorSetI(xml_attribute* att, int rhs)
	{
		att->attribute = rhs;
		return att;
	}
	xml_attribute* xml_attribute__operatorSetD(xml_attribute* att, double rhs)
	{
		att->attribute = rhs;
		return att;
	}
		
	int xml_attribute__operatorNot(xml_attribute* att)
	{
		return !att->attribute;
	}
	int xml_attribute__operatorEqual(xml_attribute* left, xml_attribute* right)
	{
		return left->attribute == right->attribute;
	}
	int xml_attribute__operatorNEqual(xml_attribute* left, xml_attribute* right)
	{
		return left->attribute != right->attribute;
	}
	int xml_attribute__operatorLess(xml_attribute* left, xml_attribute* right)
	{
		return left->attribute < right->attribute;
	}
	int xml_attribute__operatorGreater(xml_attribute* left, xml_attribute* right)
	{
		return left->attribute > right->attribute;
	}
	int xml_attribute__operatorLEqual(xml_attribute* left, xml_attribute* right)
	{
		return left->attribute <= right->attribute;
	}
	int xml_attribute__operatorGEqual(xml_attribute* left, xml_attribute* right)
	{
		return left->attribute >= right->attribute;
	}

	class xml_node
	{
	public:
		pugi::xml_node node;	// pointer sized
	};
	int		xml_node__empty(xml_node* ptr)
	{
		return ptr->node.empty();
	}

	pugi::xml_node_type xml_node__type(xml_node* ptr)
	{
		return ptr->node.type();
	}
	NULLCArray xml_node__name(xml_node* ptr)
	{
		NULLCArray ret;
		ret.ptr = (pugi::char_t*)ptr->node.name();
		ret.len = (unsigned)strlen(ret.ptr) + 1;
		return ret;
	}
	NULLCArray xml_node__value(xml_node* ptr)
	{
		NULLCArray ret;
		ret.ptr = (pugi::char_t*)ptr->node.value();
		ret.len = (unsigned)strlen(ret.ptr) + 1;
		return ret;
	}

	xml_node xml_node__child(NULLCArray name, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.child(name.ptr);
		return ret;
	}
	xml_attribute xml_node__attribute(NULLCArray name, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.attribute(name.ptr);
		return ret;
	}

	xml_node xml_node__next_sibling0(NULLCArray name, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.next_sibling(name.ptr);
		return ret;
	}
	xml_node xml_node__next_sibling1(xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.next_sibling();
		return ret;
	}

	xml_node xml_node__previous_sibling0(NULLCArray name, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.previous_sibling(name.ptr);
		return ret;
	}
	xml_node xml_node__previous_sibling1(xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.previous_sibling();
		return ret;
	}

	xml_node xml_node__parent(xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.parent();
		return ret;
	}
	xml_node xml_node__root(xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.root();
		return ret;
	}

	NULLCArray xml_node__child_value0(xml_node* ptr)
	{
		NULLCArray ret;
		ret.ptr = (pugi::char_t*)ptr->node.child_value();
		ret.len = (unsigned)strlen(ret.ptr) + 1;
		return ret;
	}
	NULLCArray xml_node__child_value1(NULLCArray name, xml_node* ptr)
	{
		NULLCArray ret;
		ret.ptr = (pugi::char_t*)ptr->node.child_value(name.ptr);
		ret.len = (unsigned)strlen(ret.ptr) + 1;
		return ret;
	}

	int xml_node__set_name(NULLCArray rhs, xml_node* ptr)
	{
		return ptr->node.set_name(rhs.ptr);
	}
	int xml_node__set_value(NULLCArray rhs, xml_node* ptr)
	{
		return ptr->node.set_value(rhs.ptr);
	}

	xml_attribute xml_node__append_attribute(NULLCArray name, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.append_attribute(name.ptr);
		return ret;
	}
	xml_attribute xml_node__prepend_attribute(NULLCArray name, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.prepend_attribute(name.ptr);
		return ret;
	}
	xml_attribute xml_node__insert_attribute_after(NULLCArray name, xml_attribute* attr, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.insert_attribute_after(name.ptr, attr->attribute);
		return ret;
	}
	xml_attribute xml_node__insert_attribute_before(NULLCArray name, xml_attribute* attr, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.insert_attribute_before(name.ptr, attr->attribute);
		return ret;
	}

	xml_attribute xml_node__append_copy0(xml_attribute* proto, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.append_copy(proto->attribute);
		return ret;
	}
	xml_attribute xml_node__prepend_copy0(xml_attribute* proto, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.prepend_copy(proto->attribute);
		return ret;
	}
	xml_attribute xml_node__insert_copy_after0(xml_attribute* proto, xml_attribute* attr, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.insert_copy_after(proto->attribute, attr->attribute);
		return ret;
	}
	xml_attribute xml_node__insert_copy_before0(xml_attribute* proto, xml_attribute* attr, xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.insert_copy_before(proto->attribute, attr->attribute);
		return ret;
	}

	xml_node xml_node__append_child(pugi::xml_node_type type, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.append_child(type);
		return ret;
	}
	xml_node xml_node__prepend_child(pugi::xml_node_type type, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.prepend_child(type);
		return ret;
	}
	xml_node xml_node__insert_child_after(pugi::xml_node_type type, xml_node* node, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.insert_child_after(type, node->node);
		return ret;
	}
	xml_node xml_node__insert_child_before(pugi::xml_node_type type, xml_node* node, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.insert_child_before(type, node->node);
		return ret;
	}

	xml_node xml_node__append_child1(NULLCArray name, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.append_child(name.ptr);
		return ret;
	}
	xml_node xml_node__prepend_child1(NULLCArray name, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.prepend_child(name.ptr);
		return ret;
	}
	xml_node xml_node__insert_child_after1(NULLCArray name, xml_node* node, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.insert_child_after(name.ptr, node->node);
		return ret;
	}
	xml_node xml_node__insert_child_before1(NULLCArray name, xml_node* node, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.insert_child_before(name.ptr, node->node);
		return ret;
	}

	xml_node xml_node__append_copy1(xml_node* proto, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.append_copy(proto->node);
		return ret;
	}
	xml_node xml_node__prepend_copy1(xml_node* proto, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.prepend_copy(proto->node);
		return ret;
	}
	xml_node xml_node__insert_copy_after1(xml_node* proto, xml_node* node, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.insert_copy_after(proto->node, node->node);
		return ret;
	}
	xml_node xml_node__insert_copy_before1(xml_node* proto, xml_node* node, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.insert_copy_before(proto->node, node->node);
		return ret;
	}

	void xml_node__remove_attribute0(xml_attribute* a, xml_node* ptr)
	{
		ptr->node.remove_attribute(a->attribute);
	}
	void xml_node__remove_attribute1(NULLCArray name, xml_node* ptr)
	{
		ptr->node.remove_attribute(name.ptr);
	}

	void xml_node__remove_child0(xml_node* n, xml_node* ptr)
	{
		ptr->node.remove_child(n->node);
	}
	void xml_node__remove_child1(NULLCArray name, xml_node* ptr)
	{
		ptr->node.remove_child(name.ptr);
	}

	xml_attribute xml_node__first_attribute(xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.first_attribute();
		return ret;
	}
    xml_attribute xml_node__last_attribute(xml_node* ptr)
	{
		xml_attribute ret;
		ret.attribute = ptr->node.last_attribute();
		return ret;
	}

	xml_node xml_node__first_child(xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.first_child();
		return ret;
	}
    xml_node xml_node__last_child(xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.last_child();
		return ret;
	}

	xml_node xml_node__find_child_by_attribute0(NULLCArray name, NULLCArray attr_name, NULLCArray attr_value, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.find_child_by_attribute(name.ptr, attr_name.ptr, attr_value.ptr);
		return ret;
	}
	xml_node xml_node__find_child_by_attribute1(NULLCArray attr_name, NULLCArray attr_value, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.find_child_by_attribute(attr_name.ptr, attr_value.ptr);
		return ret;
	}

	xml_node xml_node__first_element_by_path(NULLCArray path, char delimiter, xml_node* ptr)
	{
		xml_node ret;
		ret.node = ptr->node.first_element_by_path(path.ptr, delimiter);
		return ret;
	}

	class xml_tree_walker: public pugi::xml_tree_walker
	{
	public:
		xml_tree_walker(NULLCFuncPtr beginCB, NULLCFuncPtr for_eachCB, NULLCFuncPtr endCB)
		{
			this->beginCB = beginCB;
			this->for_eachCB = for_eachCB;
			this->endCB = endCB;
		}

		virtual ~xml_tree_walker(){}

		virtual bool begin(pugi::xml_node& node)
		{
			xml_node n;
			n.node = node;
			nullcCallFunction(beginCB, PodWrap(n.node));
			return !!nullcGetResultInt();
		}
		virtual bool for_each(pugi::xml_node& node)
		{
			xml_node n;
			n.node = node;
			nullcCallFunction(for_eachCB, PodWrap(n.node));
			return !!nullcGetResultInt();
		}
		virtual bool end(pugi::xml_node& node)
		{
			xml_node n;
			n.node = node;
			nullcCallFunction(endCB, PodWrap(n.node));
			return !!nullcGetResultInt();
		}
	private:
		NULLCFuncPtr beginCB, for_eachCB, endCB;
	};
	int xml_node__traverse(NULLCFuncPtr begin, NULLCFuncPtr for_each, NULLCFuncPtr end, xml_node* ptr)
	{
		xml_tree_walker walker = xml_tree_walker(begin, for_each, end);
		return ptr->node.traverse(walker);
	}
	class xml_writer_nullc: public pugi::xml_writer
	{
	public:
		xml_writer_nullc(NULLCFuncPtr writerCB)
		{
			this->writerCB = writerCB;
		}
		virtual ~xml_writer_nullc(){}

		virtual void write(const void* data, size_t size)
		{
			NULLCArray arr;
			arr.ptr = (char*)data;
			arr.len = (unsigned)size;
			nullcCallFunction(writerCB, arr);
		}
	private:
		NULLCFuncPtr writerCB;
	};
	void xml_node__print(NULLCFuncPtr writer, NULLCArray indent, int flags, pugi::xml_encoding encoding, int depth, xml_node* ptr)
	{
		xml_writer_nullc writer_nullc = xml_writer_nullc(writer);
		ptr->node.print(writer_nullc, indent.ptr, flags, encoding, depth);
	}

	int xml_node__offset_debug(xml_node* ptr)
	{
		return (int)ptr->node.offset_debug();
	}

	class inserter_nullc
	{
	public:
		inserter_nullc(NULLCFuncPtr callback)
		{
			this->callback = callback;
		}

		inserter_nullc& operator*(){ return *this; }
		void operator=(pugi::xml_node& node)
		{
			xml_node n;
			n.node = node;
			nullcCallFunction(callback, PodWrap(n.node));
		}
		void operator++()
		{
		}
	private:
		NULLCFuncPtr callback;
	};
	class predicate_nullc_attribute
	{
	public:
		predicate_nullc_attribute(NULLCFuncPtr callback)
		{
			this->callback = callback;
		}

		bool operator()(pugi::xml_attribute& attribute)
		{
			xml_attribute att;
			att.attribute = attribute;
			nullcCallFunction(callback, PodWrap(att.attribute));
			return !!nullcGetResultInt();
		}
		
	private:
		NULLCFuncPtr callback;
	};
	class predicate_nullc_node
	{
	public:
		predicate_nullc_node(NULLCFuncPtr callback)
		{
			this->callback = callback;
		}

		bool operator()(pugi::xml_node& node)
		{
			xml_node n;
			n.node = node;
			nullcCallFunction(callback, PodWrap(n.node));
			return !!nullcGetResultInt();
		}
	private:
		NULLCFuncPtr callback;
	};
	xml_attribute xml_node__find_attribute(NULLCFuncPtr pred, xml_node* ptr)
	{
		xml_attribute att;
		att.attribute = ptr->node.find_attribute(predicate_nullc_attribute(pred));
		return att;
	}
	xml_node xml_node__find_child(NULLCFuncPtr pred, xml_node* ptr)
	{
		xml_node n;
		n.node = ptr->node.find_child(predicate_nullc_node(pred));
		return n;
	}
	xml_node xml_node__find_node(NULLCFuncPtr pred, xml_node* ptr)
	{
		xml_node n;
		n.node = ptr->node.find_node(predicate_nullc_node(pred));
		return n;
	}

	int xml_node__operatorNot(xml_node* att)
	{
		return !att->node;
	}
	int xml_node__operatorEqual(xml_node* left, xml_node* right)
	{
		return left->node == right->node;
	}
	int xml_node__operatorNEqual(xml_node* left, xml_node* right)
	{
		return left->node != right->node;
	}
	int xml_node__operatorLess(xml_node* left, xml_node* right)
	{
		return left->node < right->node;
	}
	int xml_node__operatorGreater(xml_node* left, xml_node* right)
	{
		return left->node > right->node;
	}
	int xml_node__operatorLEqual(xml_node* left, xml_node* right)
	{
		return left->node <= right->node;
	}
	int xml_node__operatorGEqual(xml_node* left, xml_node* right)
	{
		return left->node >= right->node;
	}

	class xml_parse_result
	{
	public:
		int status;
		int offset;
		int encoding;
		void operator=(const pugi::xml_parse_result& r)
		{
			status = r.status;
			offset = (int)r.offset;
			encoding = r.encoding;
		}
	};

	class xml_document_impl
	{
	public:
		pugi::xml_document	*document;
	};
	class xml_document
	{
	public:
		xml_document_impl	*impl;
	};
	
	NULLCArray description(xml_parse_result* desc)
	{
		NULLCArray ret;
		pugi::xml_parse_result res;
		res.status = (pugi::xml_parse_status)desc->status;
		res.offset = desc->offset;
		res.encoding = (pugi::xml_encoding)desc->encoding;
		ret.ptr = (char*)res.description();
		ret.len = (unsigned)strlen(ret.ptr) + 1;
		return ret;
	}

	xml_parse_result* xml_document__load(NULLCArray contents, unsigned int options, xml_document* document)
	{
		if(!document->impl){ nullcThrowError("xml_document constructor wasn't called"); return NULL; }
		pugi::xml_document *doc = document->impl->document;
		if(!doc)
		{
			doc = document->impl->document = (pugi::xml_document*)nullcAllocate(sizeof(pugi::xml_document));
			::new(doc) pugi::xml_document();
		}
		xml_parse_result *res = (xml_parse_result*)nullcAllocate(sizeof(xml_parse_result));
		*res = doc->load(contents.ptr, options);
		return res;
	}

	xml_parse_result* xml_document__load_file(NULLCArray name, unsigned int options, pugi::xml_encoding encoding, xml_document* document)
	{
		if(!document->impl){ nullcThrowError("xml_document constructor wasn't called"); return NULL; }
		pugi::xml_document *doc = document->impl->document;
		if(!doc)
		{
			doc = document->impl->document = (pugi::xml_document*)nullcAllocate(sizeof(pugi::xml_document));
			::new(doc) pugi::xml_document();
		}
		xml_parse_result *res = (xml_parse_result*)nullcAllocate(sizeof(xml_parse_result));
		*res = doc->load_file(name.ptr, options, encoding);
		return res;
	}

	xml_parse_result* xml_document__load_buffer(NULLCArray contents, int size, unsigned int options, pugi::xml_encoding encoding, xml_document* document)
	{
		if(!document->impl){ nullcThrowError("xml_document constructor wasn't called"); return NULL; }
		pugi::xml_document *doc = document->impl->document;
		if(!doc)
		{
			doc = document->impl->document = (pugi::xml_document*)nullcAllocate(sizeof(pugi::xml_document));
			::new(doc) pugi::xml_document();
		}
		xml_parse_result *res = (xml_parse_result*)nullcAllocate(sizeof(xml_parse_result));
		*res = doc->load_buffer(contents.ptr, size, options, encoding);
		return res;
	}

	xml_parse_result* xml_document__load_buffer_inplace(NULLCArray contents, int size, unsigned int options, pugi::xml_encoding encoding, xml_document* document)
	{
		if(!document->impl){ nullcThrowError("xml_document constructor wasn't called"); return NULL; }
		pugi::xml_document *doc = document->impl->document;
		if(!doc)
		{
			doc = document->impl->document = (pugi::xml_document*)nullcAllocate(sizeof(pugi::xml_document));
			::new(doc) pugi::xml_document();
		}
		xml_parse_result *res = (xml_parse_result*)nullcAllocate(sizeof(xml_parse_result));
		*res = doc->load_buffer_inplace(contents.ptr, size, options, encoding);
		return res;
	}

	void xml_document__save(NULLCFuncPtr writer, NULLCArray indent, int flags, pugi::xml_encoding encoding, xml_document* document)
	{
		if(!document->impl){ nullcThrowError("xml_document constructor wasn't called"); return; }
		pugi::xml_document *doc = document->impl->document;
		if(!doc)
		{
			nullcThrowError("xml_document::save document is empty, you must create document first");
			return;
		}
		xml_writer_nullc writer_nullc = xml_writer_nullc(writer);
		doc->save(writer_nullc, indent.ptr, flags, encoding);
	}

	int xml_document__save_file(NULLCArray name, NULLCArray indent, int flags, pugi::xml_encoding encoding, xml_document* document)
	{
		if(!document->impl){ nullcThrowError("xml_document constructor wasn't called"); return 0; }
		pugi::xml_document *doc = document->impl->document;
		if(!doc)
		{
			nullcThrowError("xml_document::save document is empty, you must create document first");
			return 0;
		}
		return doc->save_file(name.ptr, indent.ptr, flags, encoding);
	}

	xml_node	xml_document__root(xml_document* document)
	{
		if(!document->impl){ nullcThrowError("xml_document constructor wasn't called"); return xml_node(); }
		pugi::xml_document *doc = document->impl->document;
		if(!doc)
		{
			doc = document->impl->document = (pugi::xml_document*)nullcAllocate(sizeof(pugi::xml_document));
			::new(doc) pugi::xml_document();
		}
		xml_node ret;
		ret.node = doc->root();
		return ret;
	}

	void	xml_document__finalize(xml_document_impl* document)
	{
		pugi::xml_document *doc = document->document;
		if(doc)
		{
			doc->~xml_document();
			document->document = NULL;
		}
	}
}

#define REGISTER_FUNC(funcPtr, name, index) if(!nullcBindModuleFunctionHelper("ext.pugixml", NULLCPugiXML::funcPtr, name, index)) return false;
bool	nullcInitPugiXMLModule()
{
	REGISTER_FUNC(description, "xml_parse_result::description", 0);

	REGISTER_FUNC(xml_document__load, "xml_document::load", 0);
	REGISTER_FUNC(xml_document__load_file, "xml_document::load_file", 0);
	REGISTER_FUNC(xml_document__load_buffer, "xml_document::load_buffer", 0);
	REGISTER_FUNC(xml_document__load_buffer_inplace, "xml_document::load_buffer_inplace", 0);
	REGISTER_FUNC(xml_document__save, "xml_document::save", 0);
	REGISTER_FUNC(xml_document__save_file, "xml_document::save_file", 0);

	REGISTER_FUNC(xml_document__root, "xml_document::root", 0);

	REGISTER_FUNC(xml_document__finalize, "xml_document_impl::finalize", 0);

	REGISTER_FUNC(xml_attribute__next_attribute, "xml_attribute::next_attribute", 0);
	REGISTER_FUNC(xml_attribute__previous_attribute, "xml_attribute::previous_attribute", 0);
	REGISTER_FUNC(xml_attribute__as_int, "xml_attribute::as_int", 0);
	REGISTER_FUNC(xml_attribute__as_double, "xml_attribute::as_double", 0);
	REGISTER_FUNC(xml_attribute__as_float, "xml_attribute::as_float", 0);
	REGISTER_FUNC(xml_attribute__as_bool, "xml_attribute::as_bool", 0);
	REGISTER_FUNC(xml_attribute__set_name, "xml_attribute::set_name", 0);
	REGISTER_FUNC(xml_attribute__set_valueS, "xml_attribute::set_value", 0);
	REGISTER_FUNC(xml_attribute__set_valueI, "xml_attribute::set_value", 1);
	REGISTER_FUNC(xml_attribute__set_valueD, "xml_attribute::set_value", 2);
	REGISTER_FUNC(xml_attribute__empty, "xml_attribute::empty", 0);
	REGISTER_FUNC(xml_attribute__name, "xml_attribute::name", 0);
	REGISTER_FUNC(xml_attribute__value, "xml_attribute::value", 0);

	REGISTER_FUNC(xml_attribute__operatorSetS, "=", 0);
	REGISTER_FUNC(xml_attribute__operatorSetI, "=", 1);
	REGISTER_FUNC(xml_attribute__operatorSetD, "=", 2);
	
	REGISTER_FUNC(xml_attribute__operatorNot, "!", 0);
	REGISTER_FUNC(xml_attribute__operatorEqual, "==", 0);
	REGISTER_FUNC(xml_attribute__operatorNEqual, "!=", 0);
	REGISTER_FUNC(xml_attribute__operatorLess, "<", 0);
	REGISTER_FUNC(xml_attribute__operatorGreater, ">", 0);
	REGISTER_FUNC(xml_attribute__operatorLEqual, "<=", 0);
	REGISTER_FUNC(xml_attribute__operatorGEqual, ">=", 0);

	REGISTER_FUNC(xml_node__empty, "xml_node::empty", 0);
	REGISTER_FUNC(xml_node__type, "xml_node::type", 0);
	REGISTER_FUNC(xml_node__name, "xml_node::name", 0);
	REGISTER_FUNC(xml_node__value, "xml_node::value", 0);
	REGISTER_FUNC(xml_node__child, "xml_node::child", 0);
	REGISTER_FUNC(xml_node__attribute, "xml_node::attribute", 0);
	REGISTER_FUNC(xml_node__next_sibling0, "xml_node::next_sibling", 0);
	REGISTER_FUNC(xml_node__next_sibling1, "xml_node::next_sibling", 1);
	REGISTER_FUNC(xml_node__previous_sibling0, "xml_node::previous_sibling", 0);
	REGISTER_FUNC(xml_node__previous_sibling1, "xml_node::previous_sibling", 1);
	REGISTER_FUNC(xml_node__parent, "xml_node::parent", 0);
	REGISTER_FUNC(xml_node__root, "xml_node::root", 0);
	REGISTER_FUNC(xml_node__child_value0, "xml_node::child_value", 0);
	REGISTER_FUNC(xml_node__child_value1, "xml_node::child_value", 1);
	REGISTER_FUNC(xml_node__set_name, "xml_node::set_name", 0);
	REGISTER_FUNC(xml_node__set_value, "xml_node::set_value", 0);

	REGISTER_FUNC(xml_node__append_attribute, "xml_node::append_attribute", 0);
	REGISTER_FUNC(xml_node__prepend_attribute, "xml_node::prepend_attribute", 0);
	REGISTER_FUNC(xml_node__insert_attribute_after, "xml_node::insert_attribute_after", 0);
	REGISTER_FUNC(xml_node__insert_attribute_before, "xml_node::insert_attribute_before", 0);

	REGISTER_FUNC(xml_node__append_copy0, "xml_node::append_copy", 0);
	REGISTER_FUNC(xml_node__prepend_copy0, "xml_node::prepend_copy", 0);
	REGISTER_FUNC(xml_node__insert_copy_after0, "xml_node::insert_copy_after", 0);
	REGISTER_FUNC(xml_node__insert_copy_before0, "xml_node::insert_copy_before", 0);

	REGISTER_FUNC(xml_node__append_child, "xml_node::append_child", 0);
	REGISTER_FUNC(xml_node__prepend_child, "xml_node::prepend_child", 0);
	REGISTER_FUNC(xml_node__insert_child_after, "xml_node::insert_child_after", 0);
	REGISTER_FUNC(xml_node__insert_child_before, "xml_node::insert_child_before", 0);

	REGISTER_FUNC(xml_node__append_child1, "xml_node::append_child", 1);
	REGISTER_FUNC(xml_node__prepend_child1, "xml_node::prepend_child", 1);
	REGISTER_FUNC(xml_node__insert_child_after1, "xml_node::insert_child_after", 1);
	REGISTER_FUNC(xml_node__insert_child_before1, "xml_node::insert_child_before", 1);

	REGISTER_FUNC(xml_node__append_copy1, "xml_node::append_copy", 1);
	REGISTER_FUNC(xml_node__prepend_copy1, "xml_node::prepend_copy", 1);
	REGISTER_FUNC(xml_node__insert_copy_after1, "xml_node::insert_copy_after", 1);
	REGISTER_FUNC(xml_node__insert_copy_before1, "xml_node::insert_copy_before", 1);

	REGISTER_FUNC(xml_node__remove_attribute0, "xml_node::remove_attribute", 0);
	REGISTER_FUNC(xml_node__remove_attribute1, "xml_node::remove_attribute", 1);
	REGISTER_FUNC(xml_node__remove_child0, "xml_node::remove_child", 0);
	REGISTER_FUNC(xml_node__remove_child1, "xml_node::remove_child", 1);
	REGISTER_FUNC(xml_node__first_attribute, "xml_node::first_attribute", 0);
    REGISTER_FUNC(xml_node__last_attribute, "xml_node::last_attribute", 0);
	REGISTER_FUNC(xml_node__first_child, "xml_node::first_child", 0);
    REGISTER_FUNC(xml_node__last_child, "xml_node::last_child", 0);
	REGISTER_FUNC(xml_node__find_child_by_attribute0, "xml_node::find_child_by_attribute", 0);
	REGISTER_FUNC(xml_node__find_child_by_attribute1, "xml_node::find_child_by_attribute", 1);
	REGISTER_FUNC(xml_node__first_element_by_path, "xml_node::first_element_by_path", 0);
	REGISTER_FUNC(xml_node__offset_debug, "xml_node::offset_debug", 0);

	REGISTER_FUNC(xml_node__traverse, "xml_node::traverse", 0);
	REGISTER_FUNC(xml_node__print, "xml_node::print", 0);

	REGISTER_FUNC(xml_node__find_attribute, "xml_node::find_attribute", 0);
	REGISTER_FUNC(xml_node__find_child, "xml_node::find_child", 0);
	REGISTER_FUNC(xml_node__find_node, "xml_node::find_node", 0);

	REGISTER_FUNC(xml_node__operatorNot, "!", 1);
	REGISTER_FUNC(xml_node__operatorEqual, "==", 1);
	REGISTER_FUNC(xml_node__operatorNEqual, "!=", 1);
	REGISTER_FUNC(xml_node__operatorLess, "<", 1);
	REGISTER_FUNC(xml_node__operatorGreater, ">", 1);
	REGISTER_FUNC(xml_node__operatorLEqual, "<=", 1);
	REGISTER_FUNC(xml_node__operatorGEqual, ">=", 1);

	return true;
}
