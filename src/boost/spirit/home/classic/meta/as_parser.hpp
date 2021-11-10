/*=============================================================================
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_AS_PARSER_HPP)
#define BOOST_SPIRIT_AS_PARSER_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/core/primitives/primitives.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Helper templates to derive the parser type from an auxiliary type
    //  and to generate an object of the required parser type given an
    //  auxiliary object. Supported types to convert are parsers,
    //  single characters and character strings.
    //
    ///////////////////////////////////////////////////////////////////////////
    namespace impl
    {
        template<typename T>
        struct default_as_parser
        {
            typedef T type;
            static type const& convert(type const& p)
            {
                return p;
            }
        };

        struct char_as_parser
        {
            typedef chlit<char> type;
            static type convert(char ch)
            {
                return type(ch);
            }
        };

        struct wchar_as_parser
        {
            typedef chlit<wchar_t> type;
            static type convert(wchar_t ch)
            {
                return type(ch);
            }
        };

        struct string_as_parser
        {
            typedef strlit<char const*> type;
            static type convert(char const* str)
            {
                return type(str);
            }
        };

        struct wstring_as_parser
        {
            typedef strlit<wchar_t const*> type;
            static type convert(wchar_t const* str)
            {
                return type(str);
            }
        };
    }

    template<typename T>
    struct as_parser : impl::default_as_parser<T> {};

    template<>
    struct as_parser<char> : impl::char_as_parser {};

    template<>
    struct as_parser<wchar_t> : impl::wchar_as_parser {};

    template<>
    struct as_parser<char*> : impl::string_as_parser {};

    template<>
    struct as_parser<char const*> : impl::string_as_parser {};

    template<>
    struct as_parser<wchar_t*> : impl::wstring_as_parser {};

    template<>
    struct as_parser<wchar_t const*> : impl::wstring_as_parser {};

    template<int N>
    struct as_parser<char[N]> : impl::string_as_parser {};

    template<int N>
    struct as_parser<wchar_t[N]> : impl::wstring_as_parser {};

    template<int N>
    struct as_parser<char const[N]> : impl::string_as_parser {};

    template<int N>
    struct as_parser<wchar_t const[N]> : impl::wstring_as_parser {};

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif
