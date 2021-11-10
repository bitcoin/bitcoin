/*=============================================================================
    Copyright (c) 2001-2008 Hartmut Kaiser
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#ifndef BOOST_SPIRIT_CLASSIC_TREE_IMPL_TREE_TO_XML_IPP
#define BOOST_SPIRIT_CLASSIC_TREE_IMPL_TREE_TO_XML_IPP

#include <cstdio>
#include <cstdarg>
#include <locale>
#include <string>
#include <cstring>

#include <map>
#include <iostream>
#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/scoped_array.hpp>

#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#define BOOST_SPIRIT_OSSTREAM std::ostrstream
inline
std::string BOOST_SPIRIT_GETSTRING(std::ostrstream& ss)
{
    ss << std::ends;
    std::string rval = ss.str();
    ss.freeze(false);
    return rval;
}
#else
#include <sstream>
#define BOOST_SPIRIT_GETSTRING(ss) ss.str()
#define BOOST_SPIRIT_OSSTREAM std::basic_ostringstream<CharT>
#endif

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

namespace impl {

    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT>
    struct string_lit;

    template <>
    struct string_lit<char>
    {
        static char get(char c) { return c; }
        static std::string get(char const* str = "") { return str; }
    };

    template <>
    struct string_lit<wchar_t>
    {
        static wchar_t get(char c)
        {
            typedef std::ctype<wchar_t> ctype_t;
            return std::use_facet<ctype_t>(std::locale()).widen(c);
        }
        static std::basic_string<wchar_t> get(char const* source = "")
        {
            using namespace std;        // some systems have size_t in ns std
            size_t len = strlen(source);
            boost::scoped_array<wchar_t> result (new wchar_t[len+1]);
            result.get()[len] = '\0';

            // working with wide character streams is supported only if the
            // platform provides the std::ctype<wchar_t> facet
            BOOST_ASSERT(std::has_facet<std::ctype<wchar_t> >(std::locale()));

            std::use_facet<std::ctype<wchar_t> >(std::locale())
                .widen(source, source + len, result.get());
            return result.get();
        }
    };
}

// xml formatting helper classes
namespace xml {

    template <typename CharT>
    inline void
    encode (std::basic_string<CharT> &str, char s, char const *r, int len)
    {
        typedef typename std::basic_string<CharT>::size_type size_type;

        size_type pos = 0;
        while ((pos = str.find_first_of (impl::string_lit<CharT>::get(s), pos)) !=
                size_type(std::basic_string<CharT>::npos))
        {
            str.replace (pos, 1, impl::string_lit<CharT>::get(r));
            pos += len;
        }
    }

    template <typename CharT>
    inline std::basic_string<CharT>
    encode (std::basic_string<CharT> str)
    {
        encode(str, '&', "&amp;", 3);
        encode(str, '<', "&lt;", 2);
        encode(str, '>', "&gt;", 2);
        encode(str, '\r', "\\r", 1);
        encode(str, '\n', "\\n", 1);
        return str;
    }

    template <typename CharT>
    inline std::basic_string<CharT>
    encode (CharT const *text)
    {
        return encode (std::basic_string<CharT>(text));
    }

    // format a xml attribute
    template <typename CharT>
    struct attribute
    {
        attribute()
        {
        }

        attribute (std::basic_string<CharT> const& key_,
                   std::basic_string<CharT> const& value_)
          : key (key_), value(value_)
        {
        }

        bool has_value()
        {
            return value.size() > 0;
        }

        std::basic_string<CharT> key;
        std::basic_string<CharT> value;
    };

    template <typename CharT>
    inline std::basic_ostream<CharT>&
    operator<< (std::basic_ostream<CharT> &ostrm, attribute<CharT> const &attr)
    {
        if (0 == attr.key.size())
            return ostrm;
        ostrm << impl::string_lit<CharT>::get(" ") << encode(attr.key)
              << impl::string_lit<CharT>::get("=\"") << encode(attr.value)
              << impl::string_lit<CharT>::get("\"");
        return ostrm;
    }

    // output a xml element (base class, not used directly)
    template <typename CharT>
    class element
    {
    protected:
        element(std::basic_ostream<CharT> &ostrm_, bool incr_indent_ = true)
        :   ostrm(ostrm_), incr_indent(incr_indent_)
        {
            if (incr_indent) ++get_indent();
        }
        ~element()
        {
            if (incr_indent) --get_indent();
        }

    public:
        void output_space ()
        {
            for (int i = 0; i < get_indent(); i++)
                ostrm << impl::string_lit<CharT>::get("    ");
        }

    protected:
        int &get_indent()
        {
            static int indent;

            return indent;
        }

        std::basic_ostream<CharT> &ostrm;
        bool incr_indent;
    };

    // a xml node
    template <typename CharT>
    class node : public element<CharT>
    {
    public:
        node (std::basic_ostream<CharT> &ostrm_,
              std::basic_string<CharT> const& tag_, attribute<CharT> &attr)
        :   element<CharT>(ostrm_), tag(tag_)
        {
            this->output_space();
            this->ostrm
                  << impl::string_lit<CharT>::get("<") << tag_ << attr
                  << impl::string_lit<CharT>::get(">\n");
        }
        node (std::basic_ostream<CharT> &ostrm_,
              std::basic_string<CharT> const& tag_)
        :   element<CharT>(ostrm_), tag(tag_)
        {
            this->output_space();
            this->ostrm
                  << impl::string_lit<CharT>::get("<") << tag_
                  << impl::string_lit<CharT>::get(">\n");
        }
        ~node()
        {
            this->output_space();
            this->ostrm
                  << impl::string_lit<CharT>::get("</") << tag
                  << impl::string_lit<CharT>::get(">\n");
        }

    private:
        std::basic_string<CharT> tag;
    };

    template <typename CharT>
    class text : public element<CharT>
    {
    public:
        text (std::basic_ostream<CharT> &ostrm_,
              std::basic_string<CharT> const& tag,
              std::basic_string<CharT> const& textlit)
        :   element<CharT>(ostrm_)
        {
            this->output_space();
            this->ostrm
                  << impl::string_lit<CharT>::get("<") << tag
                  << impl::string_lit<CharT>::get(">") << encode(textlit)
                  << impl::string_lit<CharT>::get("</") << tag
                  << impl::string_lit<CharT>::get(">\n");
        }

        text (std::basic_ostream<CharT> &ostrm_,
              std::basic_string<CharT> const& tag,
              std::basic_string<CharT> const& textlit,
              attribute<CharT> &attr)
        :   element<CharT>(ostrm_)
        {
            this->output_space();
            this->ostrm
                  << impl::string_lit<CharT>::get("<") << tag << attr
                  << impl::string_lit<CharT>::get(">") << encode(textlit)
                  << impl::string_lit<CharT>::get("</") << tag
                  << impl::string_lit<CharT>::get(">\n");
        }

        text (std::basic_ostream<CharT> &ostrm_,
              std::basic_string<CharT> const& tag,
              std::basic_string<CharT> const& textlit,
              attribute<CharT> &attr1, attribute<CharT> &attr2)
        :   element<CharT>(ostrm_)
        {
            this->output_space();
            this->ostrm
                  << impl::string_lit<CharT>::get("<") << tag << attr1 << attr2
                  << impl::string_lit<CharT>::get(">") << encode(textlit)
                  << impl::string_lit<CharT>::get("</") << tag
                  << impl::string_lit<CharT>::get(">\n");
        }
    };

    // a xml comment
    template <typename CharT>
    class comment : public element<CharT>
    {
    public:
        comment (std::basic_ostream<CharT> &ostrm_,
                 std::basic_string<CharT> const& commentlit)
        :   element<CharT>(ostrm_, false)
        {
            if ('\0' != commentlit[0])
            {
                this->output_space();
                this->ostrm << impl::string_lit<CharT>::get("<!-- ")
                      << encode(commentlit)
                      << impl::string_lit<CharT>::get(" -->\n");
            }
        }
    };

    // a xml document
    template <typename CharT>
    class document : public element<CharT>
    {
    public:
        document (std::basic_ostream<CharT> &ostrm_)
        :   element<CharT>(ostrm_)
        {
            this->get_indent() = -1;
            this->ostrm << impl::string_lit<CharT>::get(
                "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
        }

        document (std::basic_ostream<CharT> &ostrm_,
                  std::basic_string<CharT> const& mainnode,
                  std::basic_string<CharT> const& dtd)
        :   element<CharT>(ostrm_)
        {
            this->get_indent() = -1;
            this->ostrm << impl::string_lit<CharT>::get(
                "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");

            this->output_space();
            this->ostrm << impl::string_lit<CharT>::get("<!DOCTYPE ") << mainnode
                  << impl::string_lit<CharT>::get(" SYSTEM \"") << dtd
                  << impl::string_lit<CharT>::get("\">\n");
        }
        ~document()
        {
            BOOST_SPIRIT_ASSERT(-1 == this->get_indent());
        }
    };

} // end of namespace xml

namespace impl {

    ///////////////////////////////////////////////////////////////////////////
    // look up the rule name from the given parser_id
    template <typename AssocContainerT>
    inline typename AssocContainerT::value_type::second_type
    get_rulename (AssocContainerT const &id_to_name_map,
        BOOST_SPIRIT_CLASSIC_NS::parser_id const &id)
    {
        typename AssocContainerT::const_iterator it = id_to_name_map.find(id);
        if (it != id_to_name_map.end())
            return (*it).second;
        typedef typename AssocContainerT::value_type::second_type second_t;
        return second_t();
    }

    // dump a parse tree as xml
    template <
        typename CharT, typename IteratorT, typename GetIdT, typename GetValueT
    >
    inline void
    token_to_xml (std::basic_ostream<CharT> &ostrm, IteratorT const &it,
        bool is_root, GetIdT const &get_token_id, GetValueT const &get_token_value)
    {
        BOOST_SPIRIT_OSSTREAM stream;

        stream << get_token_id(*it) << std::ends;
        xml::attribute<CharT> token_id (
                impl::string_lit<CharT>::get("id"),
                BOOST_SPIRIT_GETSTRING(stream).c_str());
        xml::attribute<CharT> is_root_attr (
                impl::string_lit<CharT>::get("is_root"),
                impl::string_lit<CharT>::get(is_root ? "1" : ""));
        xml::attribute<CharT> nil;
        xml::text<CharT>(ostrm,
                impl::string_lit<CharT>::get("token"),
                get_token_value(*it).c_str(),
                token_id,
                is_root_attr.has_value() ? is_root_attr : nil);
    }

    template <
        typename CharT, typename TreeNodeT, typename AssocContainerT,
        typename GetIdT, typename GetValueT
    >
    inline void
    tree_node_to_xml (std::basic_ostream<CharT> &ostrm, TreeNodeT const &node,
        AssocContainerT const& id_to_name_map, GetIdT const &get_token_id,
        GetValueT const &get_token_value)
    {
        typedef typename TreeNodeT::const_iterator node_iter_t;
        typedef
            typename TreeNodeT::value_type::parse_node_t::const_iterator_t
            value_iter_t;

        xml::attribute<CharT> nil;
        node_iter_t end = node.end();
        for (node_iter_t it = node.begin(); it != end; ++it)
        {
            // output a node
            xml::attribute<CharT> id (
                impl::string_lit<CharT>::get("rule"),
                get_rulename(id_to_name_map, (*it).value.id()).c_str());
            xml::node<CharT> currnode (ostrm,
                impl::string_lit<CharT>::get("parsenode"),
                (*it).value.id() != 0 && id.has_value() ? id : nil);

            // first dump the value
            std::size_t cnt = std::distance((*it).value.begin(), (*it).value.end());

            if (1 == cnt)
            {
                token_to_xml (ostrm, (*it).value.begin(),
                    (*it).value.is_root(), get_token_id, get_token_value);
            }
            else if (cnt > 1)
            {
                xml::node<CharT> value (ostrm,
                        impl::string_lit<CharT>::get("value"));
                bool is_root = (*it).value.is_root();

                value_iter_t val_end = (*it).value.end();
                for (value_iter_t val_it = (*it).value.begin();
                val_it != val_end; ++val_it)
                {
                    token_to_xml (ostrm, val_it, is_root, get_token_id,
                        get_token_value);
                }
            }
            tree_node_to_xml(ostrm, (*it).children, id_to_name_map,
                get_token_id, get_token_value);      // dump all subnodes
        }
    }

    template <typename CharT, typename TreeNodeT, typename AssocContainerT>
    inline void
    tree_node_to_xml (std::basic_ostream<CharT> &ostrm, TreeNodeT const &node,
            AssocContainerT const& id_to_name_map)
    {
        typedef typename TreeNodeT::const_iterator node_iter_t;

        xml::attribute<CharT> nil;
        node_iter_t end = node.end();
        for (node_iter_t it = node.begin(); it != end; ++it)
        {
            // output a node
            xml::attribute<CharT> id (
                impl::string_lit<CharT>::get("rule"),
                get_rulename(id_to_name_map, (*it).value.id()).c_str());
            xml::node<CharT> currnode (ostrm,
                impl::string_lit<CharT>::get("parsenode"),
                (*it).value.id() != parser_id() && id.has_value() ? id : nil);

            // first dump the value
            if ((*it).value.begin() != (*it).value.end())
            {
                std::basic_string<CharT> tokens ((*it).value.begin(), (*it).value.end());

                if (tokens.size() > 0)
                {
                    // output all subtokens as one string (for better readability)
                    xml::attribute<CharT> is_root (
                        impl::string_lit<CharT>::get("is_root"),
                        impl::string_lit<CharT>::get((*it).value.is_root() ? "1" : ""));
                    xml::text<CharT>(ostrm,
                        impl::string_lit<CharT>::get("value"), tokens.c_str(),
                        is_root.has_value() ? is_root : nil);
                }

            }
            // dump all subnodes
            tree_node_to_xml(ostrm, (*it).children, id_to_name_map);
        }
    }

} // namespace impl

///////////////////////////////////////////////////////////////////////////////
// dump a parse tree as a xml stream (generic variant)
template <
    typename CharT, typename TreeNodeT, typename AssocContainerT,
    typename GetIdT, typename GetValueT
>
inline void
basic_tree_to_xml (std::basic_ostream<CharT> &ostrm, TreeNodeT const &tree,
std::basic_string<CharT> const &input_line, AssocContainerT const& id_to_name,
        GetIdT const &get_token_id, GetValueT const &get_token_value)
{
    // generate xml dump
    xml::document<CharT> doc (ostrm,
            impl::string_lit<CharT>::get("parsetree"),
            impl::string_lit<CharT>::get("parsetree.dtd"));
    xml::comment<CharT> input (ostrm, input_line.c_str());
    xml::attribute<CharT> ver (
            impl::string_lit<CharT>::get("version"),
            impl::string_lit<CharT>::get("1.0"));
    xml::node<CharT> mainnode (ostrm,
            impl::string_lit<CharT>::get("parsetree"), ver);

    impl::tree_node_to_xml (ostrm, tree, id_to_name, get_token_id,
        get_token_value);
}

// dump a parse tree as a xml steam (for character based parsers)
template <typename CharT, typename TreeNodeT, typename AssocContainerT>
inline void
basic_tree_to_xml (std::basic_ostream<CharT> &ostrm, TreeNodeT const &tree,
        std::basic_string<CharT> const &input_line,
        AssocContainerT const& id_to_name)
{
    // generate xml dump
    xml::document<CharT> doc (ostrm,
            impl::string_lit<CharT>::get("parsetree"),
            impl::string_lit<CharT>::get("parsetree.dtd"));
    xml::comment<CharT> input (ostrm, input_line.c_str());
    xml::attribute<CharT> ver (
            impl::string_lit<CharT>::get("version"),
            impl::string_lit<CharT>::get("1.0"));
    xml::node<CharT> mainnode (ostrm,
            impl::string_lit<CharT>::get("parsetree"), ver);

    impl::tree_node_to_xml(ostrm, tree, id_to_name);
}

template <typename CharT, typename TreeNodeT>
inline void
basic_tree_to_xml (std::basic_ostream<CharT> &ostrm, TreeNodeT const &tree,
        std::basic_string<CharT> const &input_line)
{
    return basic_tree_to_xml<CharT>(ostrm, tree, input_line,
        std::map<BOOST_SPIRIT_CLASSIC_NS::parser_id, std::basic_string<CharT> >());
}

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#undef BOOST_SPIRIT_OSSTREAM
#undef BOOST_SPIRIT_GETSTRING

#endif
