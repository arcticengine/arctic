/**
 * pugixml parser - version 1.11
 * --------------------------------------------------------
 * Copyright (C) 2006-2020, by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 * Report bugs and download new versions at https://pugixml.org/
 *
 * This library is distributed under the MIT License. See notice at the end
 * of this file.
 *
 * This work is based on the pugxml parser, which is:
 * Copyright (C) 2003, by Kristen Wegner (kristen@tima.net)
 */

// Tune these constants to adjust memory-related behavior
#define PUGIXML_MEMORY_PAGE_SIZE 32768
#define PUGIXML_MEMORY_OUTPUT_STACK 10240

#ifndef HEADER_PUGIXML_HPP
#define HEADER_PUGIXML_HPP

// Include stddef.h for size_t and ptrdiff_t
#include <stddef.h>
#include "engine/arctic_types.h"

// Include STL headers
#	include <iterator>
#	include <iosfwd>
#	include <string>

// Macro for deprecated features
#ifndef PUGIXML_DEPRECATED
#	if defined(__GNUC__)
#		define PUGIXML_DEPRECATED __attribute__((deprecated))
#	elif defined(_MSC_VER)
#		define PUGIXML_DEPRECATED __declspec(deprecated)
#	else
#		define PUGIXML_DEPRECATED
#	endif
#endif

namespace pugi
{
	// Character type used for all internal storage and operations;
	typedef char char_t;

	// String type used for operations that work with STL string;
	typedef std::basic_string<char, std::char_traits<char>, std::allocator<char> > string_t;
}

// The PugiXML namespace
namespace pugi
{
	// Tree node types
	enum xml_node_type
	{
		node_null,			// Empty (null) node handle
		node_document,		// A document tree's absolute root
		node_element,		// Element tag, i.e. '<node/>'
		node_pcdata,		// Plain character data, i.e. 'text'
		node_cdata,			// Character data, i.e. '<![CDATA[text]]>'
		node_comment,		// Comment tag, i.e. '<!-- text -->'
		node_pi,			// Processing instruction, i.e. '<?name?>'
		node_declaration,	// Document declaration, i.e. '<?xml version="1.0"?>'
		node_doctype		// Document type declaration, i.e. '<!DOCTYPE doc>'
	};

	// Parsing options

	// Minimal parsing mode (equivalent to turning all other flags off).
	// Only elements and PCDATA sections are added to the DOM tree, no text conversions are performed.
	const unsigned int parse_minimal = 0x0000;

	// This flag determines if processing instructions (node_pi) are added to the DOM tree. This flag is off by default.
	const unsigned int parse_pi = 0x0001;

	// This flag determines if comments (node_comment) are added to the DOM tree. This flag is off by default.
	const unsigned int parse_comments = 0x0002;

	// This flag determines if CDATA sections (node_cdata) are added to the DOM tree. This flag is on by default.
	const unsigned int parse_cdata = 0x0004;

	// This flag determines if plain character data (node_pcdata) that consist only of whitespace are added to the DOM tree.
	// This flag is off by default; turning it on usually results in slower parsing and more memory consumption.
	const unsigned int parse_ws_pcdata = 0x0008;

	// This flag determines if character and entity references are expanded during parsing. This flag is on by default.
	const unsigned int parse_escapes = 0x0010;

	// This flag determines if EOL characters are normalized (converted to #xA) during parsing. This flag is on by default.
	const unsigned int parse_eol = 0x0020;

	// This flag determines if attribute values are normalized using CDATA normalization rules during parsing. This flag is on by default.
	const unsigned int parse_wconv_attribute = 0x0040;

	// This flag determines if attribute values are normalized using NMTOKENS normalization rules during parsing. This flag is off by default.
	const unsigned int parse_wnorm_attribute = 0x0080;

	// This flag determines if document declaration (node_declaration) is added to the DOM tree. This flag is off by default.
	const unsigned int parse_declaration = 0x0100;

	// This flag determines if document type declaration (node_doctype) is added to the DOM tree. This flag is off by default.
	const unsigned int parse_doctype = 0x0200;

	// This flag determines if plain character data (node_pcdata) that is the only child of the parent node and that consists only
	// of whitespace is added to the DOM tree.
	// This flag is off by default; turning it on may result in slower parsing and more memory consumption.
	const unsigned int parse_ws_pcdata_single = 0x0400;

	// This flag determines if leading and trailing whitespace is to be removed from plain character data. This flag is off by default.
	const unsigned int parse_trim_pcdata = 0x0800;

	// This flag determines if plain character data that does not have a parent node is added to the DOM tree, and if an empty document
	// is a valid document. This flag is off by default.
	const unsigned int parse_fragment = 0x1000;

	// This flag determines if plain character data is be stored in the parent element's value. This significantly changes the structure of
	// the document; this flag is only recommended for parsing documents with many PCDATA nodes in memory-constrained environments.
	// This flag is off by default.
	const unsigned int parse_embed_pcdata = 0x2000;

	// The default parsing mode.
	// Elements, PCDATA and CDATA sections are added to the DOM tree, character/reference entities are expanded,
	// End-of-Line characters are normalized, attribute values are normalized using CDATA normalization rules.
	const unsigned int parse_default = parse_cdata | parse_escapes | parse_wconv_attribute | parse_eol;

	// The full parsing mode.
	// Nodes of all types are added to the DOM tree, character/reference entities are expanded,
	// End-of-Line characters are normalized, attribute values are normalized using CDATA normalization rules.
	const unsigned int parse_full = parse_default | parse_pi | parse_comments | parse_declaration | parse_doctype;

	// These flags determine the encoding of input data for XML document
	enum xml_encoding
	{
		encoding_auto,		// Auto-detect input encoding using BOM or < / <? detection; use UTF8 if BOM is not found
		encoding_utf8,		// UTF8 encoding
		encoding_utf16_le,	// Little-endian UTF16
		encoding_utf16_be,	// Big-endian UTF16
		encoding_utf16,		// UTF16 with native endianness
		encoding_utf32_le,	// Little-endian UTF32
		encoding_utf32_be,	// Big-endian UTF32
		encoding_utf32,		// UTF32 with native endianness
		encoding_wchar,		// The same encoding wchar_t has (either UTF16 or UTF32)
		encoding_latin1
	};

	// Formatting flags

	// Indent the nodes that are written to output stream with as many indentation strings as deep the node is in DOM tree. This flag is on by default.
	const unsigned int format_indent = 0x01;

	// Write encoding-specific BOM to the output stream. This flag is off by default.
	const unsigned int format_write_bom = 0x02;

	// Use raw output mode (no indentation and no line breaks are written). This flag is off by default.
	const unsigned int format_raw = 0x04;

	// Omit default XML declaration even if there is no declaration in the document. This flag is off by default.
	const unsigned int format_no_declaration = 0x08;

	// Don't escape attribute values and PCDATA contents. This flag is off by default.
	const unsigned int format_no_escapes = 0x10;

	// Open file using text mode in xml_document::save_file. This enables special character (i.e. new-line) conversions on some systems. This flag is off by default.
	const unsigned int format_save_file_text = 0x20;

	// Write every attribute on a new line with appropriate indentation. This flag is off by default.
	const unsigned int format_indent_attributes = 0x40;

	// Don't output empty element tags, instead writing an explicit start and end tag even if there are no children. This flag is off by default.
	const unsigned int format_no_empty_element_tags = 0x80;

	// Skip characters belonging to range [0; 32) instead of "&#xNN;" encoding. This flag is off by default.
	const unsigned int format_skip_control_chars = 0x100;

	// Use single quotes ' instead of double quotes " for enclosing attribute values. This flag is off by default.
	const unsigned int format_attribute_single_quote = 0x200;

	// The default set of formatting flags.
	// Nodes are indented depending on their depth in DOM tree, a default declaration is output if document has none.
	const unsigned int format_default = format_indent;

	const int default_double_precision = 17;
	const int default_float_precision = 9;

	// Forward declarations
	struct xml_attribute_struct;
	struct xml_node_struct;

	class XmlNodeIterator;
	class XmlAttributeIterator;
	class XmlNamedNodeIterator;

	class XmlTreeWalker;

	struct XmlParseResult;

	class XmlNode;

	class XmlText;

	// Range-based for loop support
	template <typename It> class XmlObjectRange
	{
	public:
		typedef It const_iterator;
		typedef It iterator;

		XmlObjectRange(It b, It e): _begin(b), _end(e)
		{
		}

		It begin() const { return _begin; }
		It end() const { return _end; }

		bool empty() const { return _begin == _end; }

	private:
		It _begin, _end;
	};

	// Writer interface for node printing (see xml_node::print)
	class  XmlWriter
	{
	public:
		virtual ~XmlWriter() {}

		// Write memory chunk into stream/file/whatever
		virtual void write(const void* data, size_t size) = 0;
	};

	// xml_writer implementation for FILE*
	class  XmlWriterFile: public XmlWriter
	{
	public:
		// Construct writer from a FILE* object; void* is used to avoid header dependencies on stdio
		XmlWriterFile(void* file);

		virtual void write(const void* data, size_t size) override;

	private:
		void* file;
	};

	// xml_writer implementation for streams
	class  XmlWriterStream: public XmlWriter
	{
	public:
		// Construct writer from an output stream object
		XmlWriterStream(std::basic_ostream<char, std::char_traits<char> >& stream);
		XmlWriterStream(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& stream);

		virtual void write(const void* data, size_t size) override;

	private:
		std::basic_ostream<char, std::char_traits<char> >* narrow_stream;
		std::basic_ostream<wchar_t, std::char_traits<wchar_t> >* wide_stream;
	};

	// A light-weight handle for manipulating attributes in DOM tree
	class XmlAttribute
	{
		friend class XmlAttributeIterator;
		friend class XmlNode;

	private:
		xml_attribute_struct* _attr;

		typedef void (*unspecified_bool_type)(XmlAttribute***);

	public:
		// Default constructor. Constructs an empty attribute.
		XmlAttribute();

		// Constructs attribute from internal pointer
		explicit XmlAttribute(xml_attribute_struct* attr);

		// Safe bool conversion operator
		operator unspecified_bool_type() const;

		// Borland C++ workaround
		bool operator!() const;

		// Comparison operators (compares wrapped attribute pointers)
		bool operator==(const XmlAttribute& r) const;
		bool operator!=(const XmlAttribute& r) const;
		bool operator<(const XmlAttribute& r) const;
		bool operator>(const XmlAttribute& r) const;
		bool operator<=(const XmlAttribute& r) const;
		bool operator>=(const XmlAttribute& r) const;

		// Check if attribute is empty
		bool empty() const;

		// Get attribute name/value, or "" if attribute is empty
		const char_t* name() const;
		const char_t* value() const;

		// Get attribute value, or the default value if attribute is empty
		const char_t* as_string(const char_t* def = "") const;

		// Get attribute value as a number, or the default value if conversion did not succeed or attribute is empty
		int as_int(int def = 0) const;
		unsigned int as_uint(unsigned int def = 0) const;
		double as_double(double def = 0) const;
		float as_float(float def = 0) const;
		long long as_llong(long long def = 0) const;
		unsigned long long as_ullong(unsigned long long def = 0) const;

		// Get attribute value as bool (returns true if first character is in '1tTyY' set), or the default value if attribute is empty
		bool as_bool(bool def = false) const;

		// Set attribute name/value (returns false if attribute is empty or there is not enough memory)
		bool set_name(const char_t* rhs);
		bool set_value(const char_t* rhs);

		// Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		bool set_value(int rhs);
		bool set_value(unsigned int rhs);
		bool set_value(long rhs);
		bool set_value(unsigned long rhs);
		bool set_value(double rhs);
		bool set_value(double rhs, int precision);
		bool set_value(float rhs);
		bool set_value(float rhs, int precision);
		bool set_value(bool rhs);
		bool set_value(long long rhs);
		bool set_value(unsigned long long rhs);

		// Set attribute value (equivalent to set_value without error checking)
		XmlAttribute& operator=(const char_t* rhs);
		XmlAttribute& operator=(int rhs);
		XmlAttribute& operator=(unsigned int rhs);
		XmlAttribute& operator=(long rhs);
		XmlAttribute& operator=(unsigned long rhs);
		XmlAttribute& operator=(double rhs);
		XmlAttribute& operator=(float rhs);
		XmlAttribute& operator=(bool rhs);
		XmlAttribute& operator=(long long rhs);
		XmlAttribute& operator=(unsigned long long rhs);

		// Get next/previous attribute in the attribute list of the parent node
		XmlAttribute next_attribute() const;
		XmlAttribute previous_attribute() const;

		// Get hash value (unique for handles to the same object)
		size_t hash_value() const;

		// Get internal pointer
		xml_attribute_struct* internal_object() const;
	};

	// A light-weight handle for manipulating nodes in DOM tree
	class XmlNode
	{
		friend class XmlAttributeIterator;
		friend class XmlNodeIterator;
		friend class XmlNamedNodeIterator;

	protected:
		xml_node_struct* _root;

		typedef void (*unspecified_bool_type)(XmlNode***);

	public:
		// Default constructor. Constructs an empty node.
		XmlNode();

		// Constructs node from internal pointer
		explicit XmlNode(xml_node_struct* p);

		// Safe bool conversion operator
		operator unspecified_bool_type() const;

		// Borland C++ workaround
		bool operator!() const;

		// Comparison operators (compares wrapped node pointers)
		bool operator==(const XmlNode& r) const;
		bool operator!=(const XmlNode& r) const;
		bool operator<(const XmlNode& r) const;
		bool operator>(const XmlNode& r) const;
		bool operator<=(const XmlNode& r) const;
		bool operator>=(const XmlNode& r) const;

		// Check if node is empty.
		bool empty() const;

		// Get node type
		xml_node_type type() const;

		// Get node name, or "" if node is empty or it has no name
		const char_t* name() const;

		// Get node value, or "" if node is empty or it has no value
		// Note: For <node>text</node> node.value() does not return "text"! Use child_value() or text() methods to access text inside nodes.
		const char_t* value() const;

		// Get attribute list
		XmlAttribute first_attribute() const;
		XmlAttribute last_attribute() const;

		// Get children list
		XmlNode first_child() const;
		XmlNode last_child() const;

		// Get next/previous sibling in the children list of the parent node
		XmlNode next_sibling() const;
		XmlNode previous_sibling() const;

		// Get parent node
		XmlNode parent() const;

		// Get root of DOM tree this node belongs to
		XmlNode root() const;

		// Get text object for the current node
		XmlText text() const;

		// Get child, attribute or next/previous sibling with the specified name
		XmlNode child(const char_t* name) const;
		XmlAttribute attribute(const char_t* name) const;
		XmlNode next_sibling(const char_t* name) const;
		XmlNode previous_sibling(const char_t* name) const;

		// Get attribute, starting the search from a hint (and updating hint so that searching for a sequence of attributes is fast)
		XmlAttribute attribute(const char_t* name, XmlAttribute& hint) const;

		// Get child value of current node; that is, value of the first child node of type PCDATA/CDATA
		const char_t* child_value() const;

		// Get child value of child with specified name. Equivalent to child(name).child_value().
		const char_t* child_value(const char_t* name) const;

		// Set node name/value (returns false if node is empty, there is not enough memory, or node can not have name/value)
		bool set_name(const char_t* rhs);
		bool set_value(const char_t* rhs);

		// Add attribute with specified name. Returns added attribute, or empty attribute on errors.
		XmlAttribute append_attribute(const char_t* name);
		XmlAttribute prepend_attribute(const char_t* name);
		XmlAttribute insert_attribute_after(const char_t* name, const XmlAttribute& attr);
		XmlAttribute insert_attribute_before(const char_t* name, const XmlAttribute& attr);

		// Add a copy of the specified attribute. Returns added attribute, or empty attribute on errors.
		XmlAttribute append_copy(const XmlAttribute& proto);
		XmlAttribute prepend_copy(const XmlAttribute& proto);
		XmlAttribute insert_copy_after(const XmlAttribute& proto, const XmlAttribute& attr);
		XmlAttribute insert_copy_before(const XmlAttribute& proto, const XmlAttribute& attr);

		// Add child node with specified type. Returns added node, or empty node on errors.
		XmlNode append_child(xml_node_type type = node_element);
		XmlNode prepend_child(xml_node_type type = node_element);
		XmlNode insert_child_after(xml_node_type type, const XmlNode& node);
		XmlNode insert_child_before(xml_node_type type, const XmlNode& node);

		// Add child element with specified name. Returns added node, or empty node on errors.
		XmlNode append_child(const char_t* name);
		XmlNode prepend_child(const char_t* name);
		XmlNode insert_child_after(const char_t* name, const XmlNode& node);
		XmlNode insert_child_before(const char_t* name, const XmlNode& node);

		// Add a copy of the specified node as a child. Returns added node, or empty node on errors.
		XmlNode append_copy(const XmlNode& proto);
		XmlNode prepend_copy(const XmlNode& proto);
		XmlNode insert_copy_after(const XmlNode& proto, const XmlNode& node);
		XmlNode insert_copy_before(const XmlNode& proto, const XmlNode& node);

		// Move the specified node to become a child of this node. Returns moved node, or empty node on errors.
		XmlNode append_move(const XmlNode& moved);
		XmlNode prepend_move(const XmlNode& moved);
		XmlNode insert_move_after(const XmlNode& moved, const XmlNode& node);
		XmlNode insert_move_before(const XmlNode& moved, const XmlNode& node);

		// Remove specified attribute
		bool remove_attribute(const XmlAttribute& a);
		bool remove_attribute(const char_t* name);

		// Remove all attributes
		bool remove_attributes();

		// Remove specified child
		bool remove_child(const XmlNode& n);
		bool remove_child(const char_t* name);

		// Remove all children
		bool remove_children();

		// Parses buffer as an XML document fragment and appends all nodes as children of the current node.
		// Copies/converts the buffer, so it may be deleted or changed after the function returns.
		// Note: append_buffer allocates memory that has the lifetime of the owning document; removing the appended nodes does not immediately reclaim that memory.
		XmlParseResult append_buffer(const void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		// Find attribute using predicate. Returns first attribute for which predicate returned true.
		template <typename Predicate> XmlAttribute find_attribute(Predicate pred) const
		{
			if (!_root) return XmlAttribute();

			for (XmlAttribute attrib = first_attribute(); attrib; attrib = attrib.next_attribute())
				if (pred(attrib))
					return attrib;

			return XmlAttribute();
		}

		// Find child node using predicate. Returns first child for which predicate returned true.
		template <typename Predicate> XmlNode find_child(Predicate pred) const
		{
			if (!_root) return XmlNode();

			for (XmlNode node = first_child(); node; node = node.next_sibling())
				if (pred(node))
					return node;

			return XmlNode();
		}

		// Find node from subtree using predicate. Returns first node from subtree (depth-first), for which predicate returned true.
		template <typename Predicate> XmlNode find_node(Predicate pred) const
		{
			if (!_root) return XmlNode();

			XmlNode cur = first_child();

			while (cur._root && cur._root != _root)
			{
				if (pred(cur)) return cur;

				if (cur.first_child()) cur = cur.first_child();
				else if (cur.next_sibling()) cur = cur.next_sibling();
				else
				{
					while (!cur.next_sibling() && cur._root != _root) cur = cur.parent();

					if (cur._root != _root) cur = cur.next_sibling();
				}
			}

			return XmlNode();
		}

		// Find child node by attribute name/value
		XmlNode find_child_by_attribute(const char_t* name, const char_t* attr_name, const char_t* attr_value) const;
		XmlNode find_child_by_attribute(const char_t* attr_name, const char_t* attr_value) const;

		// Get the absolute node path from root as a text string.
		string_t path(char_t delimiter = '/') const;

		// Search for a node by path consisting of node names and . or .. elements.
		XmlNode first_element_by_path(const char_t* path, char_t delimiter = '/') const;

		// Recursively traverse subtree with xml_tree_walker
		bool traverse(XmlTreeWalker& walker);

		// Print subtree using a writer object
		void print(XmlWriter& writer, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto, unsigned int depth = 0) const;

		// Print subtree to stream
		void print(std::basic_ostream<char, std::char_traits<char> >& os, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto, unsigned int depth = 0) const;
		void print(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& os, const char_t* indent = "\t", unsigned int flags = format_default, unsigned int depth = 0) const;

		// Child nodes iterators
		typedef XmlNodeIterator iterator;

		iterator begin() const;
		iterator end() const;

		// Attribute iterators
		typedef XmlAttributeIterator attribute_iterator;

		attribute_iterator attributes_begin() const;
		attribute_iterator attributes_end() const;

		// Range-based for support
		XmlObjectRange<XmlNodeIterator> children() const;
		XmlObjectRange<XmlNamedNodeIterator> children(const char_t* name) const;
		XmlObjectRange<XmlAttributeIterator> attributes() const;

		// Get node offset in parsed file/string (in char_t units) for debugging purposes
		ptrdiff_t offset_debug() const;

		// Get hash value (unique for handles to the same object)
		size_t hash_value() const;

		// Get internal pointer
		xml_node_struct* internal_object() const;
	};

	// A helper for working with text inside PCDATA nodes
	class XmlText
	{
		friend class XmlNode;

		xml_node_struct* _root;

		typedef void (*unspecified_bool_type)(XmlText***);

		explicit XmlText(xml_node_struct* root);

		xml_node_struct* _data_new();
		xml_node_struct* _data() const;

	public:
		// Default constructor. Constructs an empty object.
		XmlText();

		// Safe bool conversion operator
		operator unspecified_bool_type() const;

		// Borland C++ workaround
		bool operator!() const;

		// Check if text object is empty
		bool empty() const;

		// Get text, or "" if object is empty
		const char_t* get() const;

		// Get text, or the default value if object is empty
		const char_t* as_string(const char_t* def = "") const;

		// Get text as a number, or the default value if conversion did not succeed or object is empty
		int as_int(int def = 0) const;
		unsigned int as_uint(unsigned int def = 0) const;
		double as_double(double def = 0) const;
		float as_float(float def = 0) const;
		long long as_llong(long long def = 0) const;
		unsigned long long as_ullong(unsigned long long def = 0) const;

		// Get text as bool (returns true if first character is in '1tTyY' set), or the default value if object is empty
		bool as_bool(bool def = false) const;

		// Set text (returns false if object is empty or there is not enough memory)
		bool set(const char_t* rhs);

		// Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		bool set(int rhs);
		bool set(unsigned int rhs);
		bool set(long rhs);
		bool set(unsigned long rhs);
		bool set(double rhs);
		bool set(double rhs, int precision);
		bool set(float rhs);
		bool set(float rhs, int precision);
		bool set(bool rhs);
		bool set(long long rhs);
		bool set(unsigned long long rhs);

		// Set text (equivalent to set without error checking)
		XmlText& operator=(const char_t* rhs);
		XmlText& operator=(int rhs);
		XmlText& operator=(unsigned int rhs);
		XmlText& operator=(long rhs);
		XmlText& operator=(unsigned long rhs);
		XmlText& operator=(double rhs);
		XmlText& operator=(float rhs);
		XmlText& operator=(bool rhs);
		XmlText& operator=(long long rhs);
		XmlText& operator=(unsigned long long rhs);

		// Get the data node (node_pcdata or node_cdata) for this object
		XmlNode data() const;
	};

	// Child node iterator (a bidirectional iterator over a collection of xml_node)
	class XmlNodeIterator
	{
		friend class XmlNode;

	private:
		mutable XmlNode _wrap;
		XmlNode _parent;

		XmlNodeIterator(xml_node_struct* ref, xml_node_struct* parent);

	public:
		// Iterator traits
		typedef ptrdiff_t difference_type;
		typedef XmlNode value_type;
		typedef XmlNode* pointer;
		typedef XmlNode& reference;
		typedef std::bidirectional_iterator_tag iterator_category;

		// Default constructor
		XmlNodeIterator();

		// Construct an iterator which points to the specified node
		XmlNodeIterator(const XmlNode& node);

		// Iterator operators
		bool operator==(const XmlNodeIterator& rhs) const;
		bool operator!=(const XmlNodeIterator& rhs) const;

		XmlNode& operator*() const;
		XmlNode* operator->() const;

		XmlNodeIterator& operator++();
		XmlNodeIterator operator++(int);

		XmlNodeIterator& operator--();
		XmlNodeIterator operator--(int);
	};

	// Attribute iterator (a bidirectional iterator over a collection of xml_attribute)
	class  XmlAttributeIterator
	{
		friend class XmlNode;

	private:
		mutable XmlAttribute _wrap;
		XmlNode _parent;

		XmlAttributeIterator(xml_attribute_struct* ref, xml_node_struct* parent);

	public:
		// Iterator traits
		typedef ptrdiff_t difference_type;
		typedef XmlAttribute value_type;
		typedef XmlAttribute* pointer;
		typedef XmlAttribute& reference;
		typedef std::bidirectional_iterator_tag iterator_category;

		// Default constructor
		XmlAttributeIterator();

		// Construct an iterator which points to the specified attribute
		XmlAttributeIterator(const XmlAttribute& attr, const XmlNode& parent);

		// Iterator operators
		bool operator==(const XmlAttributeIterator& rhs) const;
		bool operator!=(const XmlAttributeIterator& rhs) const;

		XmlAttribute& operator*() const;
		XmlAttribute* operator->() const;

		XmlAttributeIterator& operator++();
		XmlAttributeIterator operator++(int);

		XmlAttributeIterator& operator--();
		XmlAttributeIterator operator--(int);
	};

	// Named node range helper
	class XmlNamedNodeIterator
	{
		friend class XmlNode;

	public:
		// Iterator traits
		typedef ptrdiff_t difference_type;
		typedef XmlNode value_type;
		typedef XmlNode* pointer;
		typedef XmlNode& reference;
		typedef std::bidirectional_iterator_tag iterator_category;

		// Default constructor
		XmlNamedNodeIterator();

		// Construct an iterator which points to the specified node
		XmlNamedNodeIterator(const XmlNode& node, const char_t* name);

		// Iterator operators
		bool operator==(const XmlNamedNodeIterator& rhs) const;
		bool operator!=(const XmlNamedNodeIterator& rhs) const;

		XmlNode& operator*() const;
		XmlNode* operator->() const;

		XmlNamedNodeIterator& operator++();
		XmlNamedNodeIterator operator++(int);

		XmlNamedNodeIterator& operator--();
		XmlNamedNodeIterator operator--(int);

	private:
		mutable XmlNode _wrap;
		XmlNode _parent;
		const char_t* _name;

		XmlNamedNodeIterator(xml_node_struct* ref, xml_node_struct* parent, const char_t* name);
	};

	// Abstract tree walker class (see xml_node::traverse)
	class XmlTreeWalker
	{
		friend class XmlNode;

	private:
		int _depth;

	protected:
		// Get current traversal depth
		int depth() const;

	public:
		XmlTreeWalker();
		virtual ~XmlTreeWalker();

		// Callback that is called when traversal begins
		virtual bool begin(XmlNode& node);

		// Callback that is called for each node traversed
		virtual bool for_each(XmlNode& node) = 0;

		// Callback that is called when traversal ends
		virtual bool end(XmlNode& node);
	};

	// Parsing status, returned as part of xml_parse_result object
	enum XmlParseStatus
	{
		status_ok = 0,				// No error

		status_file_not_found,		// File was not found during load_file()
		status_io_error,			// Error reading from file/stream
		status_out_of_memory,		// Could not allocate memory
		status_internal_error,		// Internal error occurred

		status_unrecognized_tag,	// Parser could not determine tag type

		status_bad_pi,				// Parsing error occurred while parsing document declaration/processing instruction
		status_bad_comment,			// Parsing error occurred while parsing comment
		status_bad_cdata,			// Parsing error occurred while parsing CDATA section
		status_bad_doctype,			// Parsing error occurred while parsing document type declaration
		status_bad_pcdata,			// Parsing error occurred while parsing PCDATA section
		status_bad_start_element,	// Parsing error occurred while parsing start element tag
		status_bad_attribute,		// Parsing error occurred while parsing element attribute
		status_bad_end_element,		// Parsing error occurred while parsing end element tag
		status_end_element_mismatch,// There was a mismatch of start-end tags (closing tag had incorrect name, some tag was not closed or there was an excessive closing tag)

		status_append_invalid_root,	// Unable to append nodes since root type is not node_element or node_document (exclusive to xml_node::append_buffer)

		status_no_document_element	// Parsing resulted in a document without element nodes
	};

	// Parsing result
	struct XmlParseResult
	{
		// Parsing status (see xml_parse_status)
		XmlParseStatus status;

		// Last parsed offset (in char_t units from start of input data)
		ptrdiff_t offset;
    arctic::Ui64 line = 0;
    arctic::Ui64 column = 0;

		// Source document encoding
		xml_encoding encoding;

		// Default constructor, initializes object to failed state
		XmlParseResult();

		// Cast to bool operator
		operator bool() const;

		// Get error description
		const char* description() const;
	};

	// Document class (DOM tree root)
	class  XmlDocument: public XmlNode
	{
	private:
		char_t* _buffer;

		char _memory[192];

		// Non-copyable semantics
		XmlDocument(const XmlDocument&);
		XmlDocument& operator=(const XmlDocument&);

		void _create();
		void _destroy();
		void _move(XmlDocument& rhs) noexcept;

	public:
		// Default constructor, makes empty document
		XmlDocument();

		// Destructor, invalidates all node/attribute handles to this document
		~XmlDocument();

		// Move semantics support
		XmlDocument(XmlDocument&& rhs) noexcept;
		XmlDocument& operator=(XmlDocument&& rhs) noexcept;

		// Removes all nodes, leaving the empty document
		void reset();

		// Removes all nodes, then copies the entire contents of the specified document
		void reset(const XmlDocument& proto);

		// Load document from stream.
		XmlParseResult load(std::basic_istream<char, std::char_traits<char> >& stream, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);
		XmlParseResult load(std::basic_istream<wchar_t, std::char_traits<wchar_t> >& stream, unsigned int options = parse_default);

		// (deprecated: use load_string instead) Load document from zero-terminated string. No encoding conversions are applied.
		PUGIXML_DEPRECATED XmlParseResult load(const char_t* contents, unsigned int options = parse_default);

		// Load document from zero-terminated string. No encoding conversions are applied.
		XmlParseResult load_string(const char_t* contents, unsigned int options = parse_default);

		// Load document from file
		XmlParseResult load_file(const char* path, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);
		XmlParseResult load_file(const wchar_t* path, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		// Load document from buffer. Copies/converts the buffer, so it may be deleted or changed after the function returns.
		XmlParseResult load_buffer(const void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		// Load document from buffer, using the buffer for in-place parsing (the buffer is modified and used for storage of document data).
		// You should ensure that buffer data will persist throughout the document's lifetime, and free the buffer memory manually once document is destroyed.
		XmlParseResult load_buffer_inplace(void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		// Load document from buffer, using the buffer for in-place parsing (the buffer is modified and used for storage of document data).
		// You should allocate the buffer with pugixml allocation function; document will free the buffer when it is no longer needed (you can't use it anymore).
		XmlParseResult load_buffer_inplace_own(void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		// Save XML document to writer (semantics is slightly different from xml_node::print, see documentation for details).
		void save(XmlWriter& writer, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto) const;

		// Save XML document to stream (semantics is slightly different from xml_node::print, see documentation for details).
		void save(std::basic_ostream<char, std::char_traits<char> >& stream, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto) const;
		void save(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& stream, const char_t* indent = "\t", unsigned int flags = format_default) const;

		// Save XML to file
		bool save_file(const char* path, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto) const;
		bool save_file(const wchar_t* path, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto) const;

		// Get document element
		XmlNode document_element() const;
	};

	// Convert wide string to UTF8
	std::basic_string<char, std::char_traits<char>, std::allocator<char> >  as_utf8(const wchar_t* str);
	std::basic_string<char, std::char_traits<char>, std::allocator<char> >  as_utf8(const std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >& str);

	// Convert UTF8 to wide string
	std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >  as_wide(const char* str);
	std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >  as_wide(const std::basic_string<char, std::char_traits<char>, std::allocator<char> >& str);

	// Memory allocation function interface; returns pointer to allocated memory or NULL on failure
	typedef void* (*allocation_function)(size_t size);

	// Memory deallocation function interface
	typedef void (*deallocation_function)(void* ptr);

	// Override default memory management functions. All subsequent allocations/deallocations will be performed via supplied functions.
	void  set_memory_management_functions(allocation_function allocate, deallocation_function deallocate);

	// Get current memory management functions
	allocation_function  get_memory_allocation_function();
	deallocation_function  get_memory_deallocation_function();
}

#if (defined(_MSC_VER) || defined(__ICC))
namespace std
{
	// Workarounds for (non-standard) iterator category detection for older versions (MSVC7/IC8 and earlier)
	std::bidirectional_iterator_tag  _Iter_cat(const pugi::xml_node_iterator&);
	std::bidirectional_iterator_tag  _Iter_cat(const pugi::xml_attribute_iterator&);
	std::bidirectional_iterator_tag  _Iter_cat(const pugi::xml_named_node_iterator&);
}
#endif

#if defined(__SUNPRO_CC)
namespace std
{
	// Workarounds for (non-standard) iterator category detection
	std::bidirectional_iterator_tag  __iterator_category(const pugi::xml_node_iterator&);
	std::bidirectional_iterator_tag  __iterator_category(const pugi::xml_attribute_iterator&);
	std::bidirectional_iterator_tag  __iterator_category(const pugi::xml_named_node_iterator&);
}
#endif

#endif

/**
 * Copyright (c) 2006-2020 Arseny Kapoulkine
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
