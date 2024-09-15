// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// This work is based on the pugxml parser, which is:
// Copyright (C) 2003, by Kristen Wegner (kristen@tima.net)

// The MIT License (MIT)
//
// Copyright (c) 2023 - 2024 Huldra
// Copyright (c) 2006 - 2020 Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.


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
	/// @brief Character type used for all internal storage and operations
	typedef char char_t;

	/// @brief String type used for operations that work with STL string
	typedef std::basic_string<char, std::char_traits<char>, std::allocator<char> > string_t;
}

// The PugiXML namespace
namespace pugi
{
	/// @brief Tree node types
	enum xml_node_type
	{
		node_null,			///< Empty (null) node handle
		node_document,		///< A document tree's absolute root
		node_element,		///< Element tag, i.e. '<node/>'
		node_pcdata,		///< Plain character data, i.e. 'text'
		node_cdata,			///< Character data, i.e. '<![CDATA[text]]>'
		node_comment,		///< Comment tag, i.e. '<!-- text -->'
		node_pi,			///< Processing instruction, i.e. '<?name?>'
		node_declaration,	///< Document declaration, i.e. '<?xml version="1.0"?>'
		node_doctype		///< Document type declaration, i.e. '<!DOCTYPE doc>'
	};

	// Parsing options

	/// @brief Minimal parsing mode (equivalent to turning all other flags off).
	/// Only elements and PCDATA sections are added to the DOM tree, no text conversions are performed.
	const unsigned int parse_minimal = 0x0000;

	/// @brief This flag determines if processing instructions (node_pi) are added to the DOM tree. This flag is off by default.
	const unsigned int parse_pi = 0x0001;

	/// @brief This flag determines if comments (node_comment) are added to the DOM tree. This flag is off by default.
	const unsigned int parse_comments = 0x0002;

	/// @brief This flag determines if CDATA sections (node_cdata) are added to the DOM tree. This flag is on by default.
	const unsigned int parse_cdata = 0x0004;

	/// @brief This flag determines if plain character data (node_pcdata) that consist only of whitespace are added to the DOM tree.
	/// This flag is off by default; turning it on usually results in slower parsing and more memory consumption.
	const unsigned int parse_ws_pcdata = 0x0008;

	/// @brief This flag determines if character and entity references are expanded during parsing. This flag is on by default.
	const unsigned int parse_escapes = 0x0010;

	/// @brief This flag determines if EOL characters are normalized (converted to #xA) during parsing. This flag is on by default.
	const unsigned int parse_eol = 0x0020;

	/// @brief This flag determines if attribute values are normalized using CDATA normalization rules during parsing. This flag is on by default.
	const unsigned int parse_wconv_attribute = 0x0040;

	/// @brief This flag determines if attribute values are normalized using NMTOKENS normalization rules during parsing. This flag is off by default.
	const unsigned int parse_wnorm_attribute = 0x0080;

	/// @brief This flag determines if document declaration (node_declaration) is added to the DOM tree. This flag is off by default.
	const unsigned int parse_declaration = 0x0100;

	/// @brief This flag determines if document type declaration (node_doctype) is added to the DOM tree. This flag is off by default.
	const unsigned int parse_doctype = 0x0200;

	/// @brief This flag determines if plain character data (node_pcdata) that is the only child of the parent node and that consists only
	/// of whitespace is added to the DOM tree.
	/// This flag is off by default; turning it on may result in slower parsing and more memory consumption.
	const unsigned int parse_ws_pcdata_single = 0x0400;

	/// @brief This flag determines if leading and trailing whitespace is to be removed from plain character data. This flag is off by default.
	const unsigned int parse_trim_pcdata = 0x0800;

	/// @brief This flag determines if plain character data that does not have a parent node is added to the DOM tree, and if an empty document
	/// is a valid document. This flag is off by default.
	const unsigned int parse_fragment = 0x1000;

	/// @brief This flag determines if plain character data is be stored in the parent element's value. This significantly changes the structure of
	/// the document; this flag is only recommended for parsing documents with many PCDATA nodes in memory-constrained environments.
	/// This flag is off by default.
	const unsigned int parse_embed_pcdata = 0x2000;

	/// @brief The default parsing mode.
	/// Elements, PCDATA and CDATA sections are added to the DOM tree, character/reference entities are expanded,
	/// End-of-Line characters are normalized, attribute values are normalized using CDATA normalization rules.
	const unsigned int parse_default = parse_cdata | parse_escapes | parse_wconv_attribute | parse_eol;

	/// @brief The full parsing mode.
	/// Nodes of all types are added to the DOM tree, character/reference entities are expanded,
	/// End-of-Line characters are normalized, attribute values are normalized using CDATA normalization rules.
	const unsigned int parse_full = parse_default | parse_pi | parse_comments | parse_declaration | parse_doctype;

	/// @brief These flags determine the encoding of input data for XML document
	enum xml_encoding
	{
		encoding_auto,		///< Auto-detect input encoding using BOM or < / <? detection; use UTF8 if BOM is not found
		encoding_utf8,		///< UTF8 encoding
		encoding_utf16_le,	///< Little-endian UTF16
		encoding_utf16_be,	///< Big-endian UTF16
		encoding_utf16,		///< UTF16 with native endianness
		encoding_utf32_le,	///< Little-endian UTF32
		encoding_utf32_be,	///< Big-endian UTF32
		encoding_utf32,		///< UTF32 with native endianness
		encoding_wchar,		///< The same encoding wchar_t has (either UTF16 or UTF32)
		encoding_latin1
	};

	// Formatting flags

	/// @brief Indent the nodes that are written to output stream with as many indentation strings as deep the node is in DOM tree. This flag is on by default.
	const unsigned int format_indent = 0x01;

	/// @brief Write encoding-specific BOM to the output stream. This flag is off by default.
	const unsigned int format_write_bom = 0x02;

	/// @brief Use raw output mode (no indentation and no line breaks are written). This flag is off by default.
	const unsigned int format_raw = 0x04;

	/// @brief Omit default XML declaration even if there is no declaration in the document. This flag is off by default.
	const unsigned int format_no_declaration = 0x08;

	/// @brief Don't escape attribute values and PCDATA contents. This flag is off by default.
	const unsigned int format_no_escapes = 0x10;

	/// @brief Open file using text mode in xml_document::save_file. This enables special character (i.e. new-line) conversions on some systems. This flag is off by default.
	const unsigned int format_save_file_text = 0x20;

	/// @brief Write every attribute on a new line with appropriate indentation. This flag is off by default.
	const unsigned int format_indent_attributes = 0x40;

	/// @brief Don't output empty element tags, instead writing an explicit start and end tag even if there are no children. This flag is off by default.
	const unsigned int format_no_empty_element_tags = 0x80;

	/// @brief Skip characters belonging to range [0; 32) instead of "&#xNN;" encoding. This flag is off by default.
	const unsigned int format_skip_control_chars = 0x100;

	/// @brief Use single quotes ' instead of double quotes " for enclosing attribute values. This flag is off by default.
	const unsigned int format_attribute_single_quote = 0x200;

	/// @brief The default set of formatting flags.
	/// Nodes are indented depending on their depth in DOM tree, a default declaration is output if document has none.
	const unsigned int format_default = format_indent;

	/// @brief Default precision for double values
	const int default_double_precision = 17;

	/// @brief Default precision for float values
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

	/// @brief Range-based for loop support
	template <typename It> class XmlObjectRange
	{
	public:
		typedef It const_iterator;
		typedef It iterator;

		/// @brief Constructor
		/// @param b Beginning iterator
		/// @param e Ending iterator
		XmlObjectRange(It b, It e): _begin(b), _end(e)
		{
		}

		/// @brief Get the beginning iterator
		/// @return Beginning iterator
		It begin() const { return _begin; }

		/// @brief Get the ending iterator
		/// @return Ending iterator
		It end() const { return _end; }

		/// @brief Check if the range is empty
		/// @return True if empty, false otherwise
		bool empty() const { return _begin == _end; }

	private:
		It _begin, _end;
	};

	/// @brief Writer interface for node printing (see xml_node::print)
	class  XmlWriter
	{
	public:
		virtual ~XmlWriter() {}

		/// @brief Write memory chunk into stream/file/whatever
		/// @param data Pointer to the data
		/// @param size Size of the data
		virtual void write(const void* data, size_t size) = 0;
	};

	/// @brief xml_writer implementation for FILE*
	class  XmlWriterFile: public XmlWriter
	{
	public:
		/// @brief Construct writer from a FILE* object
		/// @param file Pointer to FILE object
		XmlWriterFile(void* file);

		/// @brief Write data to the file
		/// @param data Pointer to the data
		/// @param size Size of the data
		virtual void write(const void* data, size_t size) override;

	private:
		void* file;
	};

	/// @brief xml_writer implementation for streams
	class  XmlWriterStream: public XmlWriter
	{
	public:
		/// @brief Construct writer from an output stream object
		/// @param stream Reference to the output stream
		XmlWriterStream(std::basic_ostream<char, std::char_traits<char> >& stream);

		/// @brief Construct writer from an output stream object
		/// @param stream Reference to the output stream
		XmlWriterStream(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& stream);

		/// @brief Write data to the stream
		/// @param data Pointer to the data
		/// @param size Size of the data
		virtual void write(const void* data, size_t size) override;

	private:
		std::basic_ostream<char, std::char_traits<char> >* narrow_stream;
		std::basic_ostream<wchar_t, std::char_traits<wchar_t> >* wide_stream;
	};

	/// @brief A light-weight handle for manipulating attributes in DOM tree
	class XmlAttribute
	{
		friend class XmlAttributeIterator;
		friend class XmlNode;

	private:
		xml_attribute_struct* _attr;

		typedef void (*unspecified_bool_type)(XmlAttribute***);

	public:
		/// @brief Default constructor. Constructs an empty attribute
		XmlAttribute();

		/// @brief Constructs attribute from internal pointer
		/// @param attr Pointer to the internal attribute structure
		explicit XmlAttribute(xml_attribute_struct* attr);

		/// @brief Safe bool conversion operator
		operator unspecified_bool_type() const;

		/// @brief Borland C++ workaround
		bool operator!() const;

		/// @brief Comparison operators (compares wrapped attribute pointers)
		bool operator==(const XmlAttribute& r) const;
		bool operator!=(const XmlAttribute& r) const;
		bool operator<(const XmlAttribute& r) const;
		bool operator>(const XmlAttribute& r) const;
		bool operator<=(const XmlAttribute& r) const;
		bool operator>=(const XmlAttribute& r) const;

		/// @brief Check if attribute is empty
		/// @return True if empty, false otherwise
		bool empty() const;

		/// @brief Get attribute name, or "" if attribute is empty
		/// @return Attribute name
		const char_t* name() const;

		/// @brief Get attribute value, or "" if attribute is empty
		/// @return Attribute value
		const char_t* value() const;

		/// @brief Get attribute value, or the default value if attribute is empty
		/// @param def Default value
		/// @return Attribute value or default value
		const char_t* as_string(const char_t* def = "") const;

		/// @brief Get attribute value as a number, or the default value if conversion did not succeed or attribute is empty
		/// @param def Default value
		/// @return Attribute value as int or default value
		int as_int(int def = 0) const;

		/// @brief Get attribute value as a number, or the default value if conversion did not succeed or attribute is empty
		/// @param def Default value
		/// @return Attribute value as unsigned int or default
		unsigned int as_uint(unsigned int def = 0) const;

		/// @brief Get attribute value as a number, or the default value if conversion did not succeed or attribute is empty
		/// @param def Default value
		/// @return Attribute value as double or default value
		double as_double(double def = 0) const;

		/// @brief Get attribute value as a number, or the default value if conversion did not succeed or attribute is empty
		/// @param def Default value
		/// @return Attribute value as float or default value
		float as_float(float def = 0) const;

		/// @brief Get attribute value as a number, or the default value if conversion did not succeed or attribute is empty
		/// @param def Default value
		/// @return Attribute value as long long or default value
		long long as_llong(long long def = 0) const;

		/// @brief Get attribute value as a number, or the default value if conversion did not succeed or attribute is empty
		/// @param def Default value
		/// @return Attribute value as unsigned long long or default value
		unsigned long long as_ullong(unsigned long long def = 0) const;

		/// @brief Get attribute value as bool (returns true if first character is in '1tTyY' set), or the default value if attribute is empty
		/// @param def Default value
		/// @return Attribute value as bool or default value
		bool as_bool(bool def = false) const;

		/// @brief Set attribute name/value (returns false if attribute is empty or there is not enough memory)
		/// @param rhs New attribute name
		/// @return True if successful, false otherwise
		bool set_name(const char_t* rhs);

		/// @brief Set attribute name/value (returns false if attribute is empty or there is not enough memory)
		/// @param rhs New attribute value
		/// @return True if successful, false otherwise
		bool set_value(const char_t* rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(int rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(unsigned int rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(long rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(unsigned long rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(double rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @param precision Precision for floating-point numbers
		/// @return True if successful, false otherwise
		bool set_value(double rhs, int precision);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(float rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @param precision Precision for floating-point numbers
		/// @return True if successful, false otherwise
		bool set_value(float rhs, int precision);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(bool rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(long long rhs);

		/// @brief Set attribute value with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(unsigned long long rhs);

		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(const char_t* rhs);

		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(int rhs);

		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(unsigned int rhs);

		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(long rhs);
		
		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(unsigned long rhs);
		
		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(double rhs);
		
		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(float rhs);
		
		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(bool rhs);
		
		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(long long rhs);
		
		/// @brief Set attribute value (equivalent to set_value without error checking)
		/// @param rhs Value to set
		/// @return Reference to the attribute
		XmlAttribute& operator=(unsigned long long rhs);

		/// @brief Get next attribute in the attribute list of the parent node
		/// @return Next attribute
		XmlAttribute next_attribute() const;
		
		/// @brief Get previous attribute in the attribute list of the parent node
		/// @return Previous attribute
		XmlAttribute previous_attribute() const;

		/// @brief Get hash value (unique for handles to the same object)
		/// @return Hash value
		size_t hash_value() const;

		/// @brief Get internal pointer
		xml_attribute_struct* internal_object() const;
	};

	/// @brief A light-weight handle for manipulating nodes in DOM tree
	class XmlNode
	{
		friend class XmlAttributeIterator;
		friend class XmlNodeIterator;
		friend class XmlNamedNodeIterator;

	protected:
		xml_node_struct* _root;

		typedef void (*unspecified_bool_type)(XmlNode***);

	public:
		/// @brief Default constructor. Constructs an empty node.
		XmlNode();

		/// @brief Constructs node from internal pointer
		/// @param p Internal pointer
		explicit XmlNode(xml_node_struct* p);

		/// @brief Safe bool conversion operator
		/// @return Unspecified bool type
		operator unspecified_bool_type() const;

		/// @brief Borland C++ workaround
		/// @return True if node is not empty, false otherwise
		bool operator!() const;

		/// @brief Comparison operators (compares wrapped node pointers)
		/// @param r Node to compare with
		/// @return True if nodes are equal, false otherwise
		bool operator==(const XmlNode& r) const;
		
		/// @brief Comparison operators (compares wrapped node pointers)
		/// @param r Node to compare with
		/// @return True if nodes are not equal, false otherwise
		bool operator!=(const XmlNode& r) const;
		
		/// @brief Comparison operators (compares wrapped node pointers)
		/// @param r Node to compare with
		/// @return True if this node is less than r, false otherwise
		bool operator<(const XmlNode& r) const;
		
		/// @brief Comparison operators (compares wrapped node pointers)
		/// @param r Node to compare with
		/// @return True if this node is greater than r, false otherwise
		bool operator>(const XmlNode& r) const;
		
		/// @brief Comparison operators (compares wrapped node pointers)
		/// @param r Node to compare with
		/// @return True if this node is less than or equal to r, false otherwise
		bool operator<=(const XmlNode& r) const;
		
		/// @brief Comparison operators (compares wrapped node pointers)
		/// @param r Node to compare with
		/// @return True if this node is greater than or equal to r, false otherwise
		bool operator>=(const XmlNode& r) const;

		/// @brief Check if node is empty.
		/// @return True if node is empty, false otherwise
		bool empty() const;

		/// @brief Get node type
		/// @return Node type
		xml_node_type type() const;

		/// @brief Get node name, or "" if node is empty or it has no name
		/// @return Node name
		const char_t* name() const;

		/// @brief Get node value, or "" if node is empty or it has no value
		/// @note For <node>text</node> node.value() does not return "text"! Use child_value() or text() methods to access text inside nodes.
		/// @return Node value
		const char_t* value() const;

		/// @brief Get attribute list
		/// @return First attribute
		XmlAttribute first_attribute() const;

		/// @brief Get attribute list
		/// @return Last attribute
		XmlAttribute last_attribute() const;

		/// @brief Get children list
		/// @return First child
		XmlNode first_child() const;
		/// @brief Get children list
		/// @return Last child
		XmlNode last_child() const;

		/// @brief Get next/previous sibling in the children list of the parent node
		/// @return Next sibling
		XmlNode next_sibling() const;
		/// @brief Get next/previous sibling in the children list of the parent node
		/// @return Previous sibling
		XmlNode previous_sibling() const;

		/// @brief Get parent node
		/// @return Parent node
		XmlNode parent() const;

		/// @brief Get root of DOM tree this node belongs to
		/// @return Root node
		XmlNode root() const;

		/// @brief Get text object for the current node
		/// @return Text object
		XmlText text() const;

		/// @brief Get child, attribute or next/previous sibling with the specified name
		/// @param name Name to search for
		/// @return Node, attribute, or sibling with the specified name
		XmlNode child(const char_t* name) const;

		/// @brief Get attribute with the specified name
		/// @param name Name to search for
		/// @return Attribute with the specified name, or empty attribute if not found
		XmlAttribute attribute(const char_t* name) const;

		/// @brief Get next/previous sibling with the specified name
		/// @param name Name to search for
		/// @return Next/previous sibling with the specified name, or empty node if not found
		XmlNode next_sibling(const char_t* name) const;

		/// @brief Get next/previous sibling with the specified name
		/// @param name Name to search for
		/// @return Next/previous sibling with the specified name, or empty node if not found
		XmlNode previous_sibling(const char_t* name) const;

		/// @brief Get attribute, starting the search from a hint (and updating hint so that searching for a sequence of attributes is fast)
		/// @param name Name to search for
		/// @param hint Hint for the search
		/// @return Attribute with the specified name, or empty attribute if not found
		XmlAttribute attribute(const char_t* name, XmlAttribute& hint) const;

		/// @brief Get child value of current node; that is, value of the first child node of type PCDATA/CDATA
		/// @return Child value
		const char_t* child_value() const;

		/// @brief Get child value of child with specified name. Equivalent to child(name).child_value().
		/// @param name Name to search for
		/// @return Child value
		const char_t* child_value(const char_t* name) const;

		/// @brief Set node name/value (returns false if node is empty, there is not enough memory, or node can not have name/value)
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_name(const char_t* rhs);
		/// @brief Set node name/value (returns false if node is empty, there is not enough memory, or node can not have name/value)
		/// @param rhs Value to set
		/// @return True if successful, false otherwise
		bool set_value(const char_t* rhs);

		/// @brief Add attribute with specified name. Returns added attribute, or empty attribute on errors.
		/// @param name Name to search for
		/// @return Added attribute
		XmlAttribute append_attribute(const char_t* name);
		/// @brief Add attribute with specified name. Returns added attribute, or empty attribute on errors.
		/// @param name Name to search for
		/// @return Added attribute
		XmlAttribute prepend_attribute(const char_t* name);
		/// @brief Add attribute with specified name. Returns added attribute, or empty attribute on errors.
		/// @param name Name to search for
		/// @param attr Attribute to insert
		/// @return Added attribute
		XmlAttribute insert_attribute_after(const char_t* name, const XmlAttribute& attr);
		/// @brief Add attribute with specified name. Returns added attribute, or empty attribute on errors.
		/// @param name Name to search for
		/// @param attr Attribute to insert
		/// @return Added attribute
		XmlAttribute insert_attribute_before(const char_t* name, const XmlAttribute& attr);

		/// @brief Add a copy of the specified attribute. Returns added attribute, or empty attribute on errors.
		/// @param proto Attribute to copy
		/// @return Added attribute
		XmlAttribute append_copy(const XmlAttribute& proto);
		/// @brief Add a copy of the specified attribute. Returns added attribute, or empty attribute on errors.
		/// @param proto Attribute to copy
		/// @return Added attribute
		XmlAttribute prepend_copy(const XmlAttribute& proto);
		/// @brief Add attribute with specified name. Returns added attribute, or empty attribute on errors.
		/// @param name Name to search for
		/// @param attr Attribute to insert
		/// @return Added attribute
		XmlAttribute insert_copy_after(const XmlAttribute& proto, const XmlAttribute& attr);
		/// @brief Add attribute with specified name. Returns added attribute, or empty attribute on errors.
		/// @param proto Attribute to copy
		/// @param attr Attribute to insert
		/// @return Added attribute
		XmlAttribute insert_copy_before(const XmlAttribute& proto, const XmlAttribute& attr);

		/// @brief Add child node with specified type. Returns added node, or empty node on errors.
		/// @param type Node type to add
		/// @return Added node
		XmlNode append_child(xml_node_type type = node_element);
		
		/// @brief Add child node with specified type. Returns added node, or empty node on errors.
		/// @param type Node type to add
		/// @return Added node
		XmlNode prepend_child(xml_node_type type = node_element);
		
		/// @brief Add child node with specified type. Returns added node, or empty node on errors.
		/// @param type Node type to add
		/// @param node Node to insert
		/// @return Added node
		XmlNode insert_child_after(xml_node_type type, const XmlNode& node);
		
		/// @brief Add child node with specified type. Returns added node, or empty node on errors.
		/// @param type Node type to add
		/// @param node Node to insert
		/// @return Added node
		XmlNode insert_child_before(xml_node_type type, const XmlNode& node);

		/// @brief Add child element with specified name. Returns added node, or empty node on errors.
		/// @param name Name to search for
		/// @return Added node
		XmlNode append_child(const char_t* name);
		
		/// @brief Add child element with specified name. Returns added node, or empty node on errors.
		/// @param name Name to search for
		/// @return Added node
		XmlNode prepend_child(const char_t* name);
		
		/// @brief Add child element with specified name. Returns added node, or empty node on errors.
		/// @param name Name to search for
		/// @param node Node to insert
		/// @return Added node
		XmlNode insert_child_after(const char_t* name, const XmlNode& node);
		
		/// @brief Add child element with specified name. Returns added node, or empty node on errors.
		/// @param name Name to search for
		/// @param node Node to insert
		/// @return Added node
		XmlNode insert_child_before(const char_t* name, const XmlNode& node);

		/// @brief Add a copy of the specified node as a child. Returns added node, or empty node on errors.
		XmlNode append_copy(const XmlNode& proto);
		XmlNode prepend_copy(const XmlNode& proto);
		XmlNode insert_copy_after(const XmlNode& proto, const XmlNode& node);
		XmlNode insert_copy_before(const XmlNode& proto, const XmlNode& node);

		// Move the specified node to become a child of this node. Returns moved node, or empty node on errors.
		XmlNode append_move(const XmlNode& moved);
		XmlNode prepend_move(const XmlNode& moved);
		XmlNode insert_move_after(const XmlNode& moved, const XmlNode& node);
		XmlNode insert_move_before(const XmlNode& moved, const XmlNode& node);

		/// @brief Remove specified attribute
		/// @param a Attribute to remove
		/// @return True if successful, false otherwise
		bool remove_attribute(const XmlAttribute& a);
		/// @brief Remove specified attribute
		/// @param name Name to search for
		/// @return True if successful, false otherwise
		bool remove_attribute(const char_t* name);

		/// @brief Remove all attributes
		/// @return True if successful, false otherwise
		bool remove_attributes();

		/// @brief Remove specified child
		/// @param n Child to remove
		/// @return True if successful, false otherwise
		bool remove_child(const XmlNode& n);
		/// @brief Remove specified child
		/// @param name Name to search for
		/// @return True if successful, false otherwise
		bool remove_child(const char_t* name);

		/// @brief Remove all children
		/// @return True if successful, false otherwise
		bool remove_children();

		/// @brief Parses buffer as an XML document fragment and appends all nodes as children of the current node.
		/// @param contents Buffer to parse
		/// @param size Size of the buffer
		/// @param options Parsing options
		/// @param encoding Encoding of the buffer
		/// @return Parsing result
		XmlParseResult append_buffer(const void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		/// @brief Find attribute using predicate. Returns first attribute for which predicate returned true.
		/// @param pred Predicate to use
		/// @return First attribute for which predicate returned true
		template <typename Predicate> XmlAttribute find_attribute(Predicate pred) const
		{
			if (!_root) return XmlAttribute();

			for (XmlAttribute attrib = first_attribute(); attrib; attrib = attrib.next_attribute())
				if (pred(attrib))
					return attrib;

			return XmlAttribute();
		}

		/// @brief Find child node using predicate. Returns first child for which predicate returned true.
		/// @param pred Predicate to use
		/// @return First child for which predicate returned true
		template <typename Predicate> XmlNode find_child(Predicate pred) const
		{
			if (!_root) return XmlNode();

			for (XmlNode node = first_child(); node; node = node.next_sibling())
				if (pred(node))
					return node;

			return XmlNode();
		}

		/// @brief Find node from subtree using predicate. Returns first node from subtree (depth-first), for which predicate returned true.
		/// @param pred Predicate to use
		/// @return First node from subtree (depth-first), for which predicate returned true
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

		/// @brief Find child node by attribute name/value
		/// @param name Name to search for
		/// @param attr_name Attribute name to search for
		/// @param attr_value Attribute value to search for
		/// @return Child node by attribute name/value
		XmlNode find_child_by_attribute(const char_t* name, const char_t* attr_name, const char_t* attr_value) const;
		
		/// @brief Find child node by attribute name/value
		/// @param attr_name Attribute name to search for
		/// @param attr_value Attribute value to search for
		/// @return Child node by attribute name/value
		XmlNode find_child_by_attribute(const char_t* attr_name, const char_t* attr_value) const;

		/// @brief Get the absolute node path from root as a text string.
		/// @param delimiter Delimiter to use
		/// @return Absolute node path from root as a text string
		string_t path(char_t delimiter = '/') const;

		/// @brief Search for a node by path consisting of node names and . or .. elements.
		/// @param path Path to search for
		/// @param delimiter Delimiter to use
		/// @return Node by path
		XmlNode first_element_by_path(const char_t* path, char_t delimiter = '/') const;

		/// @brief Recursively traverse subtree with xml_tree_walker
		/// @param walker Walker to use
		/// @return True if successful, false otherwise
		bool traverse(XmlTreeWalker& walker);

		/// @brief Print subtree using a writer object
		/// @param writer Writer to use
		/// @param indent Indent to use
		/// @param flags Formatting flags
		/// @param encoding Encoding to use
		/// @param depth Depth to use
		void print(XmlWriter& writer, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto, unsigned int depth = 0) const;

		/// @brief Print subtree to stream
		/// @param os Stream to use
		/// @param indent Indent to use
		/// @param flags Formatting flags
		/// @param encoding Encoding to use
		/// @param depth Depth to use
		void print(std::basic_ostream<char, std::char_traits<char> >& os, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto, unsigned int depth = 0) const;
		
		/// @brief Print subtree to stream
		/// @param os Stream to use
		/// @param indent Indent to use
		/// @param flags Formatting flags
		/// @param encoding Encoding to use
		/// @param depth Depth to use
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

		/// @brief Get node offset in parsed file/string (in char_t units) for debugging purposes
		/// @return Node offset in parsed file/string (in char_t units)
		ptrdiff_t offset_debug() const;

		/// @brief Get hash value (unique for handles to the same object)
		/// @return Hash value
		size_t hash_value() const;

		/// @brief Get internal pointer
		/// @return Internal pointer
		xml_node_struct* internal_object() const;
	};

	/// @brief A helper for working with text inside PCDATA nodes
	class XmlText
	{
		friend class XmlNode;

		xml_node_struct* _root;

		typedef void (*unspecified_bool_type)(XmlText***);

		/// @brief Constructor
		/// @param root Root node
		explicit XmlText(xml_node_struct* root);

		xml_node_struct* _data_new();
		xml_node_struct* _data() const;

	public:
		/// @brief Default constructor. Constructs an empty object.
		XmlText();

		/// @brief Safe bool conversion operator
		/// @return Safe bool conversion operator
		operator unspecified_bool_type() const;

		/// @brief Borland C++ workaround
		/// @return Borland C++ workaround
		bool operator!() const;

		/// @brief Check if text object is empty
		/// @return True if text object is empty, false otherwise
		bool empty() const;

		/// @brief Get text, or "" if object is empty
		/// @return Text, or "" if object is empty
		const char_t* get() const;

		/// @brief Get text, or the default value if object is empty
		/// @param def Default value to return if object is empty
		/// @return Text, or the default value if object is empty
		const char_t* as_string(const char_t* def = "") const;

		/// @brief Get text as a number, or the default value if conversion did not succeed or object is empty
		/// @param def Default value to return if conversion did not succeed or object is empty
		/// @return Text as a number, or the default value if conversion did not succeed or object is empty
		int as_int(int def = 0) const;
		
		/// @brief Get text as an unsigned integer, or the default value if conversion did not succeed or object is empty
		/// @param def Default value to return if conversion did not succeed or object is empty
		/// @return Text as an unsigned integer, or the default value if conversion did not succeed or object is empty
		unsigned int as_uint(unsigned int def = 0) const;
		
		/// @brief Get text as a double, or the default value if conversion did not succeed or object is empty
		/// @param def Default value to return if conversion did not succeed or object is empty
		/// @return Text as a double, or the default value if conversion did not succeed or object is empty
		double as_double(double def = 0) const;
		
		/// @brief Get text as a float, or the default value if conversion did not succeed or object is empty
		/// @param def Default value to return if conversion did not succeed or object is empty
		/// @return Text as a float, or the default value if conversion did not succeed or object is empty
		float as_float(float def = 0) const;
		
		/// @brief Get text as a long long, or the default value if conversion did not succeed or object is empty
		/// @param def Default value to return if conversion did not succeed or object is empty
		/// @return Text as a long long, or the default value if conversion did not succeed or object is empty
		long long as_llong(long long def = 0) const;
		
		/// @brief Get text as an unsigned long long, or the default value if conversion did not succeed or object is empty
		/// @param def Default value to return if conversion did not succeed or object is empty
		/// @return Text as an unsigned long long, or the default value if conversion did not succeed or object is empty
		unsigned long long as_ullong(unsigned long long def = 0) const;

		/// @brief Get text as a bool, or the default value if object is empty
		/// @param def Default value to return if object is empty
		/// @return Text as a bool, or the default value if object is empty
		bool as_bool(bool def = false) const;

		/// @brief Set text (returns false if object is empty or there is not enough memory)
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(const char_t* rhs);

		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(int rhs);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(unsigned int rhs);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(long rhs);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(unsigned long rhs);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(double rhs);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(double rhs, int precision);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(float rhs);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(float rhs, int precision);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(bool rhs);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(long long rhs);
		
		/// @brief Set text with type conversion (numbers are converted to strings, boolean is converted to "true"/"false")
		/// @param rhs Text to set
		/// @return True if successful, false otherwise
		bool set(unsigned long long rhs);

		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(const char_t* rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(int rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(unsigned int rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(long rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(unsigned long rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(double rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(float rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(bool rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(long long rhs);
		
		/// @brief Set text (equivalent to set without error checking)
		/// @param rhs Text to set
		/// @return Reference to this object
		XmlText& operator=(unsigned long long rhs);

		/// @brief Get the data node (node_pcdata or node_cdata) for this object
		/// @return Data node
		XmlNode data() const;
	};

	/// @brief Child node iterator (a bidirectional iterator over a collection of xml_node)
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

		/// @brief Default constructor
		XmlNodeIterator();

		/// @brief Construct an iterator which points to the specified node
		XmlNodeIterator(const XmlNode& node);

		// Iterator operators
		bool operator==(const XmlNodeIterator& rhs) const;
		bool operator!=(const XmlNodeIterator& rhs) const;

		/// @brief Get the current node
		/// @return Current node
		XmlNode& operator*() const;

		/// @brief Get the current node
		/// @return Current node
		XmlNode* operator->() const;

		/// @brief Move to the next node
		/// @return Reference to this object
		XmlNodeIterator& operator++();

		/// @brief Move to the next node
		/// @return Copy of this object
		XmlNodeIterator operator++(int);

		/// @brief Move to the previous node
		/// @return Reference to this object
		XmlNodeIterator& operator--();

		/// @brief Move to the previous node
		/// @return Copy of this object
		XmlNodeIterator operator--(int);
	};

	/// @brief Attribute iterator (a bidirectional iterator over a collection of xml_attribute)
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

		/// @brief Default constructor
		XmlAttributeIterator();

		/// @brief Construct an iterator which points to the specified attribute
		/// @param attr Attribute to point to
		/// @param parent Parent node
		XmlAttributeIterator(const XmlAttribute& attr, const XmlNode& parent);

		// Iterator operators
		bool operator==(const XmlAttributeIterator& rhs) const;
		bool operator!=(const XmlAttributeIterator& rhs) const;

		/// @brief Get the current attribute
		/// @return Current attribute
		XmlAttribute& operator*() const;

		/// @brief Get the current attribute
		/// @return Current attribute
		XmlAttribute* operator->() const;

		/// @brief Move to the next attribute
		/// @return Reference to this object
		XmlAttributeIterator& operator++();

		/// @brief Move to the next attribute
		/// @return Copy of this object
		XmlAttributeIterator operator++(int);

		/// @brief Move to the previous attribute
		/// @return Reference to this object
		XmlAttributeIterator& operator--();

		/// @brief Move to the previous attribute
		/// @return Copy of this object
		XmlAttributeIterator operator--(int);
	};

	/// @brief Named node range helper
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

		/// @brief Default constructor
		XmlNamedNodeIterator();

		/// @brief Construct an iterator which points to the specified node
		/// @param node Node to point to
		/// @param name Name of the node to point to
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

	/// @brief Abstract tree walker class (see xml_node::traverse)
	class XmlTreeWalker
	{
		friend class XmlNode;

	private:
		int _depth;

	protected:
		/// @brief Get current traversal depth
		/// @return Current traversal depth
		int depth() const;

	public:
		XmlTreeWalker();
		virtual ~XmlTreeWalker();

		/// @brief Callback that is called when traversal begins
		/// @param node Node being traversed
		/// @return True to continue traversal, false to stop
		virtual bool begin(XmlNode& node);

		/// @brief Callback that is called for each node traversed
		/// @param node Node being traversed
		/// @return True to continue traversal, false to stop
		virtual bool for_each(XmlNode& node) = 0;

		/// @brief Callback that is called when traversal ends
		/// @param node Node being traversed
		/// @return True to continue traversal, false to stop
		virtual bool end(XmlNode& node);
	};

	/// @brief Parsing status, returned as part of xml_parse_result object
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

	/// @brief Parsing result
	struct XmlParseResult
	{
		/// @brief Parsing status (see xml_parse_status)
		/// @details This member holds the status of the parsing operation. It can be one of the values defined in the XmlParseStatus enumeration.
		XmlParseStatus status;

		/// @brief Last parsed offset (in char_t units from start of input data)
		/// @details This member holds the offset of the last parsed character in the input data. It is measured in char_t units from the start of the input data.
		ptrdiff_t offset;
    
    /// @brief Line number of the last parsed character
    /// @details This member holds the line number of the last parsed character in the input data.
    arctic::Ui64 line = 0;

    /// @brief Column number of the last parsed character
    /// @details This member holds the column number of the last parsed character in the input data.
    arctic::Ui64 column = 0;

		/// @brief Source document encoding
		/// @details This member holds the encoding of the source document. It can be one of the values defined in the xml_encoding enumeration.
		xml_encoding encoding;

		/// @brief Default constructor, initializes object to failed state
		XmlParseResult();

		/// @brief Cast to bool operator
		/// @details This operator allows the object to be used in boolean contexts. It returns true if the parsing was successful, and false otherwise.
		operator bool() const;

		/// @brief Get error description
		/// @details This function returns a string description of the parsing error. If the parsing was successful, it returns an empty string.
    /// @return String description of the parsing error
		const char* description() const;
	};

	/// @brief Document class (DOM tree root)
	class  XmlDocument: public XmlNode
	{
	private:
		char_t* _buffer;

		char _memory[192];

		/// @brief Non-copyable semantics
		/// @details This class does not support copying.
		XmlDocument(const XmlDocument&);
		
		XmlDocument& operator=(const XmlDocument&);

		void _create();
		void _destroy();
		void _move(XmlDocument& rhs) noexcept;

	public:
		/// @brief Default constructor, makes empty document
		/// @details This constructor creates an empty XML document.
		XmlDocument();

		~XmlDocument();

		/// @brief Move semantics support
		/// @details This constructor and assignment operator support move semantics.
		XmlDocument(XmlDocument&& rhs) noexcept;

		/// @brief Assignment operator
		/// @details This assignment operator supports move semantics.
		XmlDocument& operator=(XmlDocument&& rhs) noexcept;

		/// @brief Removes all nodes, leaving the empty document
		/// @details This function removes all nodes from the document, leaving it empty.
		void reset();

		/// @brief Removes all nodes, then copies the entire contents of the specified document
		/// @details This function removes all nodes from the document and then copies the entire contents of the specified document.
		void reset(const XmlDocument& proto);

		/// @brief Load document from stream.
		/// @details This function loads a document from a stream. The stream is read until it is exhausted or a parse error occurs.
		/// @param stream The stream to load the document from.
		/// @param options Optional parsing options (see xml_parse_options).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		/// @return The result of the parsing operation.
		XmlParseResult load(std::basic_istream<char, std::char_traits<char> >& stream, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		/// @brief Load document from stream.
		/// @details This function loads a document from a stream. The stream is read until it is exhausted or a parse error occurs.
		/// @param stream The stream to load the document from.
		/// @param options Optional parsing options (see xml_parse_options).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		/// @return The result of the parsing operation.  
		XmlParseResult load(std::basic_istream<wchar_t, std::char_traits<wchar_t> >& stream, unsigned int options = parse_default);

		// (deprecated: use load_string instead) Load document from zero-terminated string. No encoding conversions are applied.
		PUGIXML_DEPRECATED XmlParseResult load(const char_t* contents, unsigned int options = parse_default);

		/// @brief Load document from zero-terminated string. No encoding conversions are applied.
		/// @details This function loads a document from a zero-terminated string. No encoding conversions are applied.
		/// @param contents The zero-terminated string to load the document from.
		/// @param options Optional parsing options (see xml_parse_options).
		/// @return The result of the parsing operation.
		XmlParseResult load_string(const char_t* contents, unsigned int options = parse_default);

		/// @brief Load document from file
		/// @details This function loads a document from a file.
		/// @param path The path to the file to load the document from.
		/// @param options Optional parsing options (see xml_parse_options).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		/// @return The result of the parsing operation.
		XmlParseResult load_file(const char* path, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);
		
    /// @brief Load document from file
    /// @param path The path to the file to load the document from.
    /// @param options Optional parsing options (see xml_parse_options).
    /// @param encoding Optional encoding to use for the document (see xml_encoding).
    /// @return The result of the parsing operation.
    XmlParseResult load_file(const wchar_t* path, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		/// @brief Load document from buffer. Copies/converts the buffer, so it may be deleted or changed after the function returns.
		/// @details This function loads a document from a buffer. The buffer is copied and converted if necessary.
		/// @param contents The buffer to load the document from.
		/// @param size The size of the buffer.
		/// @param options Optional parsing options (see xml_parse_options).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		/// @return The result of the parsing operation.
		XmlParseResult load_buffer(const void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		/// @brief Load document from buffer, using the buffer for in-place parsing (the buffer is modified and used for storage of document data).
		/// @details This function loads a document from a buffer. The buffer is modified and used for storage of document data.
		/// @param contents The buffer to load the document from.
		/// @param size The size of the buffer.
		/// @param options Optional parsing options (see xml_parse_options).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		/// @return The result of the parsing operation.
		/// @note You should ensure that buffer data will persist throughout the document's lifetime, and free the buffer memory manually once document is destroyed.
		XmlParseResult load_buffer_inplace(void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		/// @brief Load document from buffer, using the buffer for in-place parsing (the buffer is modified and used for storage of document data).
		/// @details This function loads a document from a buffer. The buffer is modified and used for storage of document data.
		/// @param contents The buffer to load the document from.
		/// @param size The size of the buffer.
		/// @param options Optional parsing options (see xml_parse_options).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		/// @return The result of the parsing operation.
		/// @note You should ensure that buffer data will persist throughout the document's lifetime, and free the buffer memory manually once document is destroyed.
		XmlParseResult load_buffer_inplace_own(void* contents, size_t size, unsigned int options = parse_default, xml_encoding encoding = encoding_auto);

		/// @brief Save XML document to writer (semantics is slightly different from xml_node::print, see documentation for details).
		/// @details This function saves the XML document to a writer. The semantics are slightly different from xml_node::print, see documentation for details.
		/// @param writer The writer to save the document to.
		/// @param indent The indentation string to use.
		/// @param flags Optional formatting flags (see xml_format_flags).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		void save(XmlWriter& writer, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto) const;

		/// @brief Save XML document to stream (semantics is slightly different from xml_node::print, see documentation for details).
		/// @details This function saves the XML document to a stream. The semantics are slightly different from xml_node::print, see documentation for details.
		/// @param stream The stream to save the document to.
		/// @param indent The indentation string to use.
		/// @param flags Optional formatting flags (see xml_format_flags).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		void save(std::basic_ostream<char, std::char_traits<char> >& stream, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto) const;

		/// @brief Save XML document to stream (semantics is slightly different from xml_node::print, see documentation for details).
		/// @details This function saves the XML document to a stream. The semantics are slightly different from xml_node::print, see documentation for details.
		/// @param stream The stream to save the document to.
		/// @param indent The indentation string to use.
		/// @param flags Optional formatting flags (see xml_format_flags).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		void save(std::basic_ostream<wchar_t, std::char_traits<wchar_t> >& stream, const char_t* indent = "\t", unsigned int flags = format_default) const;

		/// @brief Save XML to file
		/// @details This function saves the XML document to a file.
		/// @param path The path to the file to save the document to.
		/// @param indent The indentation string to use.
		/// @param flags Optional formatting flags (see xml_format_flags).
		/// @param encoding Optional encoding to use for the document (see xml_encoding).
		bool save_file(const char* path, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto) const;
		
    /// @brief Save XML to file
    /// @param path The path to the file to save the document to.
    /// @param indent The indentation string to use.
    /// @param flags Optional formatting flags (see xml_format_flags).
    /// @param encoding Optional encoding to use for the document (see xml_encoding).
    /// @return True if the document was saved successfully, false otherwise.
    bool save_file(const wchar_t* path, const char_t* indent = "\t", unsigned int flags = format_default, xml_encoding encoding = encoding_auto) const;

		/// @brief Get document element
		/// @details This function returns the document element.
		/// @return The document element.
		XmlNode document_element() const;
	};

	/// @brief Convert wide string to UTF8
	/// @details This function converts a wide string to a UTF8 string.
	/// @param str The wide string to convert.
	/// @return The UTF8 string.
	std::basic_string<char, std::char_traits<char>, std::allocator<char> >  as_utf8(const wchar_t* str);

	/// @brief Convert wide string to UTF8
	/// @details This function converts a wide string to a UTF8 string.
	/// @param str The wide string to convert.
	/// @return The UTF8 string.
	std::basic_string<char, std::char_traits<char>, std::allocator<char> >  as_utf8(const std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >& str);

	/// @brief Convert UTF8 to wide string
	/// @details This function converts a UTF8 string to a wide string.
	/// @param str The UTF8 string to convert.
	/// @return The wide string.
	std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >  as_wide(const char* str);

	/// @brief Convert UTF8 to wide string
	/// @details This function converts a UTF8 string to a wide string.
	/// @param str The UTF8 string to convert.
	/// @return The wide string.
	std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >  as_wide(const std::basic_string<char, std::char_traits<char>, std::allocator<char> >& str);

	/// @brief Memory allocation function interface; returns pointer to allocated memory or NULL on failure
	/// @details This function interface is used to allocate memory.
	/// @param size The size of the memory to allocate.
	/// @return Pointer to the allocated memory or NULL on failure.
	typedef void* (*allocation_function)(size_t size);

	/// @brief Memory deallocation function interface
	/// @details This function interface is used to deallocate memory.
	/// @param ptr The pointer to the memory to deallocate.
	typedef void (*deallocation_function)(void* ptr);

	/// @brief Override default memory management functions
	/// @details This function overrides the default memory management functions. All subsequent allocations/deallocations will be performed via supplied functions.
	/// @param allocate The allocation function.
	/// @param deallocate The deallocation function.
	void  set_memory_management_functions(allocation_function allocate, deallocation_function deallocate);

	/// @brief Get current memory management functions
	/// @details This function returns the current memory management functions.
	/// @return The allocation function.
	allocation_function  get_memory_allocation_function();
	
	/// @brief Get current memory management functions
	/// @details This function returns the current memory management functions.
	/// @return The deallocation function.
  deallocation_function  get_memory_deallocation_function();
}

#endif


