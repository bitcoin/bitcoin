/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
================================================_==============================*/
#if !defined(BOOST_SPIRIT_X3_PRINT_TOKEN_JANUARY_20_2013_0814AM)
#define BOOST_SPIRIT_X3_PRINT_TOKEN_JANUARY_20_2013_0814AM

#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <cctype>
#include <ios>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    // generate debug output for lookahead token (character) stream
    namespace detail
    {
        struct token_printer_debug_for_chars
        {
            template<typename Out, typename Char>
            static void print(Out& o, Char c)
            {
                using namespace std;    // allow for ADL to find the proper iscntrl

                switch (c)
                {
                    case '\a': o << "\\a"; break;
                    case '\b': o << "\\b"; break;
                    case '\f': o << "\\f"; break;
                    case '\n': o << "\\n"; break;
                    case '\r': o << "\\r"; break;
                    case '\t': o << "\\t"; break;
                    case '\v': o << "\\v"; break;
                    default:
                        if (c >= 0 && c < 127)
                        {
                          if (iscntrl(c))
                            o << "\\" << std::oct << int(c);
                          else if (isprint(c))
                            o << char(c);
                          else
                            o << "\\x" << std::hex << int(c);
                        }
                        else
                          o << "\\x" << std::hex << int(c);
                }
            }
        };

        // for token types where the comparison with char constants wouldn't work
        struct token_printer_debug
        {
            template<typename Out, typename T>
            static void print(Out& o, T const& val)
            {
                o << val;
            }
        };
    }

    template <typename T, typename Enable = void>
    struct token_printer_debug
      : mpl::if_<
            mpl::and_<
                is_convertible<T, char>, is_convertible<char, T> >
          , detail::token_printer_debug_for_chars
          , detail::token_printer_debug>::type
    {};

    template <typename Out, typename T>
    inline void print_token(Out& out, T const& val)
    {
        // allow to customize the token printer routine
        token_printer_debug<T>::print(out, val);
    }
}}}}

#endif
