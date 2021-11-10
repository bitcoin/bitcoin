// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
// Copyright (C) 2009 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_XML_PARSER_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_XML_PARSER_HPP_INCLUDED

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/detail/xml_parser_write.hpp>
#include <boost/property_tree/detail/xml_parser_error.hpp>
#include <boost/property_tree/detail/xml_parser_writer_settings.hpp>
#include <boost/property_tree/detail/xml_parser_flags.hpp>
#include <boost/property_tree/detail/xml_parser_read_rapidxml.hpp>

#include <fstream>
#include <string>
#include <locale>

namespace boost { namespace property_tree { namespace xml_parser
{

    /**
     * Reads XML from an input stream and translates it to property tree.
     * @note Clears existing contents of property tree.  In case of error the
     *       property tree unmodified.
     * @note XML attributes are placed under keys named @c \<xmlattr\>.
     * @throw xml_parser_error In case of error deserializing the property tree.
     * @param stream Stream from which to read in the property tree.
     * @param[out] pt The property tree to populate.
     * @param flags Flags controlling the behaviour of the parser.
     *              The following flags are supported:
     * @li @c no_concat_text -- Prevents concatenation of text nodes into
     *                          datastring of property tree.  Puts them in
     *                          separate @c \<xmltext\> strings instead.
     * @li @c no_comments -- Skip XML comments.
     * @li @c trim_whitespace -- Trim leading and trailing whitespace from text,
     *                           and collapse sequences of whitespace.
     */
    template<class Ptree>
    void read_xml(std::basic_istream<
                      typename Ptree::key_type::value_type
                  > &stream,
                  Ptree &pt,
                  int flags = 0)
    {
        read_xml_internal(stream, pt, flags, std::string());
    }

    /**
     * Reads XML from a file using the given locale and translates it to
     * property tree.
     * @note Clears existing contents of property tree.  In case of error the
     *       property tree unmodified.
     * @note XML attributes are placed under keys named @c \<xmlattr\>.
     * @throw xml_parser_error In case of error deserializing the property tree.
     * @param filename The file from which to read in the property tree.
     * @param[out] pt The property tree to populate.
     * @param flags Flags controlling the bahviour of the parser.
     *              The following flags are supported:
     * @li @c no_concat_text -- Prevents concatenation of text nodes into
     *                          datastring of property tree.  Puts them in
     *                          separate @c \<xmltext\> strings instead.
     * @li @c no_comments -- Skip XML comments.
     * @param loc The locale to use when reading in the file contents.
     */
    template<class Ptree>
    void read_xml(const std::string &filename,
                  Ptree &pt,
                  int flags = 0,
                  const std::locale &loc = std::locale())
    {
        BOOST_ASSERT(validate_flags(flags));
        std::basic_ifstream<typename Ptree::key_type::value_type>
            stream(filename.c_str());
        if (!stream)
            BOOST_PROPERTY_TREE_THROW(xml_parser_error(
                "cannot open file", filename, 0));
        stream.imbue(loc);
        read_xml_internal(stream, pt, flags, filename);
    }

    /**
     * Translates the property tree to XML and writes it the given output
     * stream.
     * @throw xml_parser_error In case of error translating the property tree to
     *                         XML or writing to the output stream.
     * @param stream The stream to which to write the XML representation of the 
     *               property tree.
     * @param pt The property tree to tranlsate to XML and output.
     * @param settings The settings to use when writing out the property tree as
     *                 XML.
     */
    template<class Ptree>
    void write_xml(std::basic_ostream<
                       typename Ptree::key_type::value_type
                   > &stream,
                   const Ptree &pt,
                   const xml_writer_settings<
                       typename Ptree::key_type
                   > & settings = xml_writer_settings<
                                    typename Ptree::key_type>() )
    {
        write_xml_internal(stream, pt, std::string(), settings);
    }

    /**
     * Translates the property tree to XML and writes it the given file.
     * @throw xml_parser_error In case of error translating the property tree to
     *                         XML or writing to the output stream.
     * @param filename The file to which to write the XML representation of the 
     *                 property tree.
     * @param pt The property tree to tranlsate to XML and output.
     * @param loc The locale to use when writing the output to file.
     * @param settings The settings to use when writing out the property tree as
     *                 XML.
     */
    template<class Ptree>
    void write_xml(const std::string &filename,
                   const Ptree &pt,
                   const std::locale &loc = std::locale(),
                   const xml_writer_settings<
                       typename Ptree::key_type
                   > & settings = xml_writer_settings<typename Ptree::key_type>())
    {
        std::basic_ofstream<typename Ptree::key_type::value_type>
            stream(filename.c_str());
        if (!stream)
            BOOST_PROPERTY_TREE_THROW(xml_parser_error(
                "cannot open file", filename, 0));
        stream.imbue(loc);
        write_xml_internal(stream, pt, filename, settings);
    }

} } }

namespace boost { namespace property_tree
{
    using xml_parser::read_xml;
    using xml_parser::write_xml;
    using xml_parser::xml_parser_error;

    using xml_parser::xml_writer_settings;
    using xml_parser::xml_writer_make_settings;
} }

#endif
