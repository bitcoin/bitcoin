/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_X3_SIMPLE_TRACE_DECEMBER_06_2008_1102AM)
#define BOOST_SPIRIT_X3_SIMPLE_TRACE_DECEMBER_06_2008_1102AM

#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/spirit/home/x3/support/traits/print_token.hpp>
#include <boost/spirit/home/x3/support/traits/print_attribute.hpp>
#include <boost/spirit/home/x3/nonterminal/debug_handler_state.hpp>
#include <boost/type_traits/is_same.hpp>
#include <iostream>

//  The stream to use for debug output
#if !defined(BOOST_SPIRIT_X3_DEBUG_OUT)
#define BOOST_SPIRIT_X3_DEBUG_OUT std::cerr
#endif

//  number of tokens to print while debugging
#if !defined(BOOST_SPIRIT_X3_DEBUG_PRINT_SOME)
#define BOOST_SPIRIT_X3_DEBUG_PRINT_SOME 20
#endif

//  number of spaces to indent
#if !defined(BOOST_SPIRIT_X3_DEBUG_INDENT)
#define BOOST_SPIRIT_X3_DEBUG_INDENT 2
#endif

namespace boost { namespace spirit { namespace x3
{
    namespace detail
    {
        template <typename Char>
        inline void token_printer(std::ostream& o, Char c)
        {
            // allow customization of the token printer routine
            x3::traits::print_token(o, c);
        }
    }

    template <int IndentSpaces = 2, int CharsToPrint = 20>
    struct simple_trace
    {
        simple_trace(std::ostream& out)
          : out(out), indent(0) {}

        void print_indent(int n) const
        {
            n *= IndentSpaces;
            for (int i = 0; i != n; ++i)
                out << ' ';
        }

        template <typename Iterator>
        void print_some(
            char const* tag
          , Iterator first, Iterator const& last) const
        {
            print_indent(indent);
            out << '<' << tag << '>';
            int const n = CharsToPrint;
            for (int i = 0; first != last && i != n && *first; ++i, ++first)
                detail::token_printer(out, *first);
            out << "</" << tag << '>' << std::endl;

            // $$$ FIXME convert invalid xml characters (e.g. '<') to valid
            // character entities. $$$
        }

        template <typename Iterator, typename Attribute, typename State>
        void operator()(
            Iterator const& first
          , Iterator const& last
          , Attribute const& attr
          , State state
          , std::string const& rule_name) const
        {
            switch (state)
            {
                case pre_parse:
                    print_indent(indent++);
                    out
                        << '<' << rule_name << '>'
                        << std::endl;
                    print_some("try", first, last);
                    break;

                case successful_parse:
                    print_some("success", first, last);
                    if (!is_same<Attribute, unused_type>::value)
                    {
                        print_indent(indent);
                        out
                            << "<attributes>";
                        traits::print_attribute(out, attr);
                        out
                            << "</attributes>";
                        out << std::endl;
                    }
                    print_indent(--indent);
                    out
                        << "</" << rule_name << '>'
                        << std::endl;
                    break;

                case failed_parse:
                    print_indent(indent);
                    out << "<fail/>" << std::endl;
                    print_indent(--indent);
                    out
                        << "</" << rule_name << '>'
                        << std::endl;
                    break;
            }
        }

        std::ostream& out;
        mutable int indent;
    };

    namespace detail
    {
        typedef simple_trace<
            BOOST_SPIRIT_X3_DEBUG_INDENT, BOOST_SPIRIT_X3_DEBUG_PRINT_SOME>
        simple_trace_type;

        inline simple_trace_type&
        get_simple_trace()
        {
            static simple_trace_type tracer(BOOST_SPIRIT_X3_DEBUG_OUT);
            return tracer;
        }
    }
}}}

#endif
