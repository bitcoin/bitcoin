//  Copyright (c) 2001-2011 Hartmut Kaiser
//  Copyright (c) 2001-2011 Joel de Guzman
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_SIMPLE_TRACE_APR_21_2010_0155PM)
#define BOOST_SPIRIT_KARMA_SIMPLE_TRACE_APR_21_2010_0155PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/karma/nonterminal/debug_handler_state.hpp>
#include <boost/fusion/include/out.hpp>
#include <iostream>

//  The stream to use for debug output
#if !defined(BOOST_SPIRIT_DEBUG_OUT)
#define BOOST_SPIRIT_DEBUG_OUT std::cerr
#endif

//  number of tokens to print while debugging
#if !defined(BOOST_SPIRIT_DEBUG_PRINT_SOME)
#define BOOST_SPIRIT_DEBUG_PRINT_SOME 20
#endif

//  number of spaces to indent
#if !defined(BOOST_SPIRIT_DEBUG_INDENT)
#define BOOST_SPIRIT_DEBUG_INDENT 2
#endif

namespace boost { namespace spirit { namespace karma
{
    struct simple_trace
    {
        int& get_indent() const
        {
            static int indent = 0;
            return indent;
        }

        void print_indent() const
        {
            int n = get_indent();
            n *= BOOST_SPIRIT_DEBUG_INDENT;
            for (int i = 0; i != n; ++i)
                BOOST_SPIRIT_DEBUG_OUT << ' ';
        }

        template <typename Buffer>
        void print_some(char const* tag, Buffer const& buffer) const
        {
            print_indent();
            BOOST_SPIRIT_DEBUG_OUT << '<' << tag << '>' << std::flush;
            {
                std::ostreambuf_iterator<char> out(BOOST_SPIRIT_DEBUG_OUT);
                buffer.buffer_copy_to(out, BOOST_SPIRIT_DEBUG_PRINT_SOME);
            }
            BOOST_SPIRIT_DEBUG_OUT << "</" << tag << '>' << std::endl;
        }

        template <typename OutputIterator, typename Context, typename State
          , typename Buffer>
        void operator()(
            OutputIterator&, Context const& context
          , State state, std::string const& rule_name
          , Buffer const& buffer) const
        {
            switch (state)
            {
                case pre_generate:
                    print_indent();
                    ++get_indent();
                    BOOST_SPIRIT_DEBUG_OUT
                        << '<' << rule_name << '>' << std::endl;
                    print_indent();
                    ++get_indent();
                    BOOST_SPIRIT_DEBUG_OUT << "<try>" << std::endl;;
                    print_indent();
                    BOOST_SPIRIT_DEBUG_OUT << "<attributes>";
                    traits::print_attribute(
                        BOOST_SPIRIT_DEBUG_OUT,
                        context.attributes
                    );
                    BOOST_SPIRIT_DEBUG_OUT << "</attributes>" << std::endl;
                    if (!fusion::empty(context.locals))
                    {
                        print_indent();
                        BOOST_SPIRIT_DEBUG_OUT
                            << "<locals>" << context.locals << "</locals>"
                            << std::endl;
                    }
                    --get_indent();
                    print_indent();
                    BOOST_SPIRIT_DEBUG_OUT << "</try>" << std::endl;;
                    break;

                case successful_generate:
                    print_indent();
                    ++get_indent();
                    BOOST_SPIRIT_DEBUG_OUT << "<success>" << std::endl;
                    print_some("result", buffer);
                    if (!fusion::empty(context.locals))
                    {
                        print_indent();
                        BOOST_SPIRIT_DEBUG_OUT
                            << "<locals>" << context.locals << "</locals>"
                            << std::endl;
                    }
                    --get_indent();
                    print_indent();
                    BOOST_SPIRIT_DEBUG_OUT << "</success>" << std::endl;
                    --get_indent();
                    print_indent();
                    BOOST_SPIRIT_DEBUG_OUT 
                        << "</" << rule_name << '>' << std::endl;
                    break;

                case failed_generate:
                    print_indent();
                    BOOST_SPIRIT_DEBUG_OUT << "<fail/>" << std::endl;
                    --get_indent();
                    print_indent();
                    BOOST_SPIRIT_DEBUG_OUT 
                        << "</" << rule_name << '>' << std::endl;
                    break;
            }
        }
    };
}}}

#endif
