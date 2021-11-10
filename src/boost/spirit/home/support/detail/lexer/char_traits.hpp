// char_traits.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_CHAR_TRAITS_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_CHAR_TRAITS_HPP

// Make sure wchar_t is defined
#include <string>

namespace boost
{
namespace lexer
{
template<typename CharT>
struct char_traits
{
    typedef CharT char_type;
    typedef CharT index_type;

    static index_type call (CharT ch)
    {
       return ch;
    }
};

template<>
struct char_traits<char>
{
    typedef char char_type;
    typedef unsigned char index_type;
        
    static index_type call (char ch)
    {
        return static_cast<index_type>(ch);
    }
};

template<>
struct char_traits<wchar_t>
{
    typedef wchar_t char_type;
    typedef wchar_t index_type;

    static index_type call (wchar_t ch)
    {
        return ch;
    }
};
}
}

#endif
