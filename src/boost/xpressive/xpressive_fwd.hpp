///////////////////////////////////////////////////////////////////////////////
/// \file xpressive_fwd.hpp
/// Forward declarations for all of xpressive's public data types.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_XPRESSIVE_FWD_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_XPRESSIVE_FWD_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <string>
#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/iterator/iterator_traits.hpp>

#if BOOST_VERSION >= 103500
# define BOOST_PROTO_FUSION_V2
#endif


#if defined(BOOST_NO_STD_LOCALE) & !defined(BOOST_XPRESSIVE_USE_C_TRAITS)
# define BOOST_XPRESSIVE_USE_C_TRAITS
#endif

#if defined(BOOST_NO_CWCHAR) || defined(BOOST_NO_CWCTYPE) || defined(BOOST_NO_STD_WSTRING)
# ifndef BOOST_XPRESSIVE_NO_WREGEX
#  define BOOST_XPRESSIVE_NO_WREGEX
# endif
#endif

// Stack protection under MS Windows
// Config logic taken from boost/regex/config.hpp
#ifndef BOOST_XPRESSIVE_HAS_MS_STACK_GUARD
# if (defined(_WIN32) || defined(_WIN64) || defined(_WINCE))                    \
     && !(defined(__GNUC__) || defined(__BORLANDC__) && defined(__clang__))     \
     && !(defined(BOOST_BORLANDC) && (BOOST_BORLANDC >= 0x600))                 \
     && !(defined(__MWERKS__) && (__MWERKS__ <= 0x3003))
#  define BOOST_XPRESSIVE_HAS_MS_STACK_GUARD 1
# else
#  define BOOST_XPRESSIVE_HAS_MS_STACK_GUARD 0
# endif
#endif

#include <boost/proto/proto_fwd.hpp>

namespace boost { namespace xpressive
{

    template<typename Char>
    struct cpp_regex_traits;

    template<typename Char>
    struct c_regex_traits;

    template<typename Elem>
    struct null_regex_traits;

    namespace detail
    {
        template<typename Char>
        struct default_regex_traits
        {
            #ifdef BOOST_XPRESSIVE_USE_C_TRAITS
            typedef c_regex_traits<Char> type;
            #else
            typedef cpp_regex_traits<Char> type;
            #endif
        };

        struct mark_placeholder;
        typedef proto::expr<proto::tag::terminal, proto::term<mark_placeholder>, 0> basic_mark_tag;

        struct regex_domain;

    } // namespace detail

    struct mark_tag;

    typedef void const *regex_id_type;

    struct regex_error;

    struct regex_traits_version_1_tag;

    struct regex_traits_version_2_tag;

    // DEPRECATED
    /// INTERNAL ONLY
    ///
    struct regex_traits_version_1_case_fold_tag;

    template<typename Trait>
    struct has_fold_case;

    template<typename BidiIter>
    struct basic_regex;

    template<typename BidiIter>
    struct match_results;

    template<typename BidiIter>
    struct regex_iterator;

    template<typename BidiIter>
    struct regex_token_iterator;

    template<typename BidiIter>
    struct regex_id_filter_predicate;

    template<typename BidiIter>
    struct sub_match;

    template<typename RegexTraits>
    struct compiler_traits;

    template<typename Char, typename Impl = typename detail::default_regex_traits<Char>::type>
    struct regex_traits;

    template
    <
        typename BidiIter
      , typename RegexTraits = regex_traits<typename iterator_value<BidiIter>::type>
      , typename CompilerTraits = compiler_traits<RegexTraits>
    >
    struct regex_compiler;

    template<typename T>
    struct value;

    template<typename T>
    struct reference;

    template<typename T>
    struct local;

    template<typename T, int I = 0, typename Dummy = proto::is_proto_expr>
    struct placeholder;

    namespace op
    {
        struct at;
        struct push;
        struct push_back;
        struct push_front;
        struct pop;
        struct pop_back;
        struct pop_front;
        struct front;
        struct back;
        struct top;
        struct first;
        struct second;
        struct matched;
        struct length;
        struct str;
        struct insert;
        struct make_pair;
        template<typename T>
        struct as;
        template<typename T>
        struct static_cast_;
        template<typename T>
        struct dynamic_cast_;
        template<typename T>
        struct const_cast_;
        template<typename T>
        struct construct;
        template<typename Except>
        struct throw_;
        struct unwrap_reference;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Common typedefs
    //
    typedef basic_regex<std::string::const_iterator>                sregex;
    typedef basic_regex<char const *>                               cregex;

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    typedef basic_regex<std::wstring::const_iterator>               wsregex;
    typedef basic_regex<wchar_t const *>                            wcregex;
    #endif

    typedef sub_match<std::string::const_iterator>                  ssub_match;
    typedef sub_match<char const *>                                 csub_match;

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    typedef sub_match<std::wstring::const_iterator>                 wssub_match;
    typedef sub_match<wchar_t const *>                              wcsub_match;
    #endif

    typedef regex_compiler<std::string::const_iterator>             sregex_compiler;
    typedef regex_compiler<char const *>                            cregex_compiler;

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    typedef regex_compiler<std::wstring::const_iterator>            wsregex_compiler;
    typedef regex_compiler<wchar_t const *>                         wcregex_compiler;
    #endif

    typedef regex_iterator<std::string::const_iterator>             sregex_iterator;
    typedef regex_iterator<char const *>                            cregex_iterator;

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    typedef regex_iterator<std::wstring::const_iterator>            wsregex_iterator;
    typedef regex_iterator<wchar_t const *>                         wcregex_iterator;
    #endif

    typedef regex_token_iterator<std::string::const_iterator>       sregex_token_iterator;
    typedef regex_token_iterator<char const *>                      cregex_token_iterator;

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    typedef regex_token_iterator<std::wstring::const_iterator>      wsregex_token_iterator;
    typedef regex_token_iterator<wchar_t const *>                   wcregex_token_iterator;
    #endif

    typedef match_results<std::string::const_iterator>              smatch;
    typedef match_results<char const *>                             cmatch;

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    typedef match_results<std::wstring::const_iterator>             wsmatch;
    typedef match_results<wchar_t const *>                          wcmatch;
    #endif

    typedef regex_id_filter_predicate<std::string::const_iterator>  sregex_id_filter_predicate;
    typedef regex_id_filter_predicate<char const *>                 cregex_id_filter_predicate;

    #ifndef BOOST_XPRESSIVE_NO_WREGEX
    typedef regex_id_filter_predicate<std::wstring::const_iterator> wsregex_id_filter_predicate;
    typedef regex_id_filter_predicate<wchar_t const *>              wcregex_id_filter_predicate;
    #endif

}} // namespace boost::xpressive

#endif
