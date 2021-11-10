// consts.h
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_CONSTS_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_CONSTS_HPP

#include <boost/config.hpp>
#include <boost/integer_traits.hpp>
#include "size_t.hpp"

namespace boost
{
namespace lexer
{
    enum regex_flags {none = 0, icase = 1, dot_not_newline = 2};
    // 0 = end state, 1 = id, 2 = unique_id, 3 = lex state, 4 = bol, 5 = eol,
    // 6 = dead_state_index
    enum {end_state_index, id_index, unique_id_index, state_index, bol_index,
        eol_index, dead_state_index, dfa_offset};

    const std::size_t max_macro_len = 30;
    const std::size_t num_chars = 256;
    // If sizeof(wchar_t) == sizeof(size_t) then don't overflow to 0
    // by adding one to comparison.
    const std::size_t num_wchar_ts =
        (boost::integer_traits<wchar_t>::const_max < 0x110000) ?
        boost::integer_traits<wchar_t>::const_max +
        static_cast<std::size_t> (1) : 0x110000;
    const std::size_t null_token = static_cast<std::size_t> (~0);
    const std::size_t bol_token = static_cast<std::size_t> (~1);
    const std::size_t eol_token = static_cast<std::size_t> (~2);
    const std::size_t end_state = 1;
    const std::size_t npos = static_cast<std::size_t> (~0);
}
}

#endif
