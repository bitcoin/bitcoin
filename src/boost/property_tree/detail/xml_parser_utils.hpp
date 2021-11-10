// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_UTILS_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_XML_PARSER_UTILS_HPP_INCLUDED

#include <boost/property_tree/detail/ptree_utils.hpp>
#include <boost/property_tree/detail/xml_parser_error.hpp>
#include <boost/property_tree/detail/xml_parser_writer_settings.hpp>
#include <string>
#include <algorithm>
#include <locale>

namespace boost { namespace property_tree { namespace xml_parser
{

    template<class Str>
    Str condense(const Str &s)
    {
        typedef typename Str::value_type Ch;
        Str r;
        std::locale loc;
        bool space = false;
        typename Str::const_iterator end = s.end();
        for (typename Str::const_iterator it = s.begin();
             it != end; ++it)
        {
            if (isspace(*it, loc) || *it == Ch('\n'))
            {
                if (!space)
                    r += Ch(' '), space = true;
            }
            else
                r += *it, space = false;
        }
        return r;
    }


    template<class Str>
    Str encode_char_entities(const Str &s)
    {
        // Don't do anything for empty strings.
        if(s.empty()) return s;

        typedef typename Str::value_type Ch;

        Str r;
        // To properly round-trip spaces and not uglify the XML beyond
        // recognition, we have to encode them IF the text contains only spaces.
        Str sp(1, Ch(' '));
        if(s.find_first_not_of(sp) == Str::npos) {
            // The first will suffice.
            r = detail::widen<Str>("&#32;");
            r += Str(s.size() - 1, Ch(' '));
        } else {
            typename Str::const_iterator end = s.end();
            for (typename Str::const_iterator it = s.begin(); it != end; ++it)
            {
                switch (*it)
                {
                    case Ch('<'): r += detail::widen<Str>("&lt;"); break;
                    case Ch('>'): r += detail::widen<Str>("&gt;"); break;
                    case Ch('&'): r += detail::widen<Str>("&amp;"); break;
                    case Ch('"'): r += detail::widen<Str>("&quot;"); break;
                    case Ch('\''): r += detail::widen<Str>("&apos;"); break;
                    default: r += *it; break;
                }
            }
        }
        return r;
    }

    template<class Str>
    Str decode_char_entities(const Str &s)
    {
        typedef typename Str::value_type Ch;
        Str r;
        typename Str::const_iterator end = s.end();
        for (typename Str::const_iterator it = s.begin(); it != end; ++it)
        {
            if (*it == Ch('&'))
            {
                typename Str::const_iterator semicolon = std::find(it + 1, end, Ch(';'));
                if (semicolon == end)
                    BOOST_PROPERTY_TREE_THROW(xml_parser_error("invalid character entity", "", 0));
                Str ent(it + 1, semicolon);
                if (ent == detail::widen<Str>("lt")) r += Ch('<');
                else if (ent == detail::widen<Str>("gt")) r += Ch('>');
                else if (ent == detail::widen<Str>("amp")) r += Ch('&');
                else if (ent == detail::widen<Str>("quot")) r += Ch('"');
                else if (ent == detail::widen<Str>("apos")) r += Ch('\'');
                else
                    BOOST_PROPERTY_TREE_THROW(xml_parser_error("invalid character entity", "", 0));
                it = semicolon;
            }
            else
                r += *it;
        }
        return r;
    }

    template<class Str>
    const Str &xmldecl()
    {
        static Str s = detail::widen<Str>("<?xml>");
        return s;
    }

    template<class Str>
    const Str &xmlattr()
    {
        static Str s = detail::widen<Str>("<xmlattr>");
        return s;
    }

    template<class Str>
    const Str &xmlcomment()
    {
        static Str s = detail::widen<Str>("<xmlcomment>");
        return s;
    }

    template<class Str>
    const Str &xmltext()
    {
        static Str s = detail::widen<Str>("<xmltext>");
        return s;
    }

} } }

#endif
