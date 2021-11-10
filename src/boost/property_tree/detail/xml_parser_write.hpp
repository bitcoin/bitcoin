// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
// Copyright (C) 2013 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_WRITE_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_WRITE_HPP_INCLUDED

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/detail/xml_parser_utils.hpp>
#include <string>
#include <ostream>
#include <iomanip>

namespace boost { namespace property_tree { namespace xml_parser
{
    template<class Str>
    void write_xml_indent(std::basic_ostream<typename Str::value_type> &stream,
          int indent,
          const xml_writer_settings<Str> & settings
          )
    {
        stream << std::basic_string<typename Str::value_type>(indent * settings.indent_count, settings.indent_char);
    }

    template<class Str>
    void write_xml_comment(std::basic_ostream<typename Str::value_type> &stream,
                           const Str &s,
                           int indent,
                           bool separate_line,
                           const xml_writer_settings<Str> & settings
                           )
    {
        typedef typename Str::value_type Ch;
        if (separate_line)
            write_xml_indent(stream,indent,settings);
        stream << Ch('<') << Ch('!') << Ch('-') << Ch('-');
        stream << s;
        stream << Ch('-') << Ch('-') << Ch('>');
        if (separate_line)
            stream << Ch('\n');
    }

    template<class Str>
    void write_xml_text(std::basic_ostream<typename Str::value_type> &stream,
                        const Str &s,
                        int indent, 
                        bool separate_line,
                        const xml_writer_settings<Str> & settings
                        )
    {
        typedef typename Str::value_type Ch;
        if (separate_line)
            write_xml_indent(stream,indent,settings);
        stream << encode_char_entities(s);
        if (separate_line)
            stream << Ch('\n');
    }

    template<class Ptree>
    void write_xml_element(std::basic_ostream<typename Ptree::key_type::value_type> &stream,
                           const typename Ptree::key_type &key,
                           const Ptree &pt,
                           int indent,
                           const xml_writer_settings<typename Ptree::key_type> & settings)
    {
        typedef typename Ptree::key_type::value_type Ch;
        typedef typename Ptree::key_type Str;
        typedef typename Ptree::const_iterator It;

        bool want_pretty = settings.indent_count > 0;
        // Find if elements present
        bool has_elements = false;
        bool has_attrs_only = pt.data().empty();
        for (It it = pt.begin(), end = pt.end(); it != end; ++it)
        {
            if (it->first != xmlattr<Str>() )
            {
                has_attrs_only = false;
                if (it->first != xmltext<Str>())
                {
                    has_elements = true;
                    break;
                }
            }
        }

        // Write element
        if (pt.data().empty() && pt.empty())    // Empty key
        {
            if (indent >= 0)
            {
                write_xml_indent(stream,indent,settings);
                stream << Ch('<') << key << 
                          Ch('/') << Ch('>');
                if (want_pretty)
                    stream << Ch('\n');
            }
        }
        else    // Nonempty key
        {
            // Write opening tag, attributes and data
            if (indent >= 0)
            {
                // Write opening brace and key
                write_xml_indent(stream,indent,settings);
                stream << Ch('<') << key;

                // Write attributes
                if (optional<const Ptree &> attribs = pt.get_child_optional(xmlattr<Str>()))
                    for (It it = attribs.get().begin(); it != attribs.get().end(); ++it)
                        stream << Ch(' ') << it->first << Ch('=')
                               << Ch('"')
                               << encode_char_entities(
                                    it->second.template get_value<Str>())
                               << Ch('"');

                if ( has_attrs_only )
                {
                    // Write closing brace
                    stream << Ch('/') << Ch('>');
                    if (want_pretty)
                        stream << Ch('\n');
                }
                else
                {
                    // Write closing brace
                    stream << Ch('>');

                    // Break line if needed and if we want pretty-printing
                    if (has_elements && want_pretty)
                        stream << Ch('\n');
                }
            }

            // Write data text, if present
            if (!pt.data().empty())
                write_xml_text(stream,
                    pt.template get_value<Str>(),
                    indent + 1, has_elements && want_pretty, settings);

            // Write elements, comments and texts
            for (It it = pt.begin(); it != pt.end(); ++it)
            {
                if (it->first == xmlattr<Str>())
                    continue;
                else if (it->first == xmlcomment<Str>())
                    write_xml_comment(stream,
                        it->second.template get_value<Str>(),
                        indent + 1, want_pretty, settings);
                else if (it->first == xmltext<Str>())
                    write_xml_text(stream,
                        it->second.template get_value<Str>(),
                        indent + 1, has_elements && want_pretty, settings);
                else
                    write_xml_element(stream, it->first, it->second,
                        indent + 1, settings);
            }

            // Write closing tag
            if (indent >= 0 && !has_attrs_only)
            {
                if (has_elements)
                    write_xml_indent(stream,indent,settings);
                stream << Ch('<') << Ch('/') << key << Ch('>');
                if (want_pretty)
                    stream << Ch('\n');
            }

        }
    }

    template<class Ptree>
    void write_xml_internal(std::basic_ostream<typename Ptree::key_type::value_type> &stream, 
                            const Ptree &pt,
                            const std::string &filename,
                            const xml_writer_settings<typename Ptree::key_type> & settings)
    {
        typedef typename Ptree::key_type Str;
        stream  << detail::widen<Str>("<?xml version=\"1.0\" encoding=\"")
                << settings.encoding
                << detail::widen<Str>("\"?>\n");
        write_xml_element(stream, Str(), pt, -1, settings);
        if (!stream)
            BOOST_PROPERTY_TREE_THROW(xml_parser_error("write error", filename, 0));
    }

} } }

#endif
