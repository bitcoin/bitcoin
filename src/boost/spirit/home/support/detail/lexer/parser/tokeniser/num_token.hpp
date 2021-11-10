// num_token.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TOKENISER_NUM_TOKEN_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARSER_TOKENISER_NUM_TOKEN_HPP

#include <boost/config.hpp>
#include "../../consts.hpp" // null_token
#include "../../size_t.hpp"
#include <boost/detail/workaround.hpp>

namespace boost
{
namespace lexer
{
namespace detail
{
template<typename CharT>
struct basic_num_token
{
    enum type {BEGIN, REGEX, OREXP, SEQUENCE, SUB, EXPRESSION, REPEAT,
        DUP, OR, CHARSET, MACRO, OPENPAREN, CLOSEPAREN, OPT, AOPT,
        ZEROORMORE, AZEROORMORE, ONEORMORE, AONEORMORE, REPEATN, AREPEATN,
        END};

    type _type;
    std::size_t _id;
    std::size_t _min;
    bool _comma;
    std::size_t _max;
    CharT _macro[max_macro_len + 1];
    static const char _precedence_table[END + 1][END + 1];
    static const char *_precedence_strings[END + 1];

    basic_num_token (const type type_ = BEGIN,
        const std::size_t id_ = null_token) :
        _type (type_),
        _id (id_),
        _min (0),
        _comma (false),
        _max (0)
    {
        *_macro = 0;
    }

    void set (const type type_)
    {
        _type = type_;
        _id = null_token;
    }

    void set (const type type_, const std::size_t id_)
    {
        _type = type_;
        _id = id_;
    }

    void min_max (const std::size_t min_, const bool comma_,
        const std::size_t max_)
    {
        _min = min_;
        _comma = comma_;
        _max = max_;
    }

    char precedence (const type type_) const
    {
        return _precedence_table[_type][type_];
    }

    const char *precedence_string () const
    {
        return _precedence_strings[_type];
    }
};

template<typename CharT>
const char basic_num_token<CharT>::_precedence_table[END + 1][END + 1] = {
//        BEG, REG, ORE, SEQ, SUB, EXP, RPT, DUP,  | , CHR, MCR,  ( ,  ) ,  ? , ?? ,  * , *? ,  + , +?, {n}?, {n}, END
/*BEGIN*/{' ', '<', '<', '<', '<', '<', '<', ' ', ' ', '<', '<', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/*REGEX*/{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '=', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/*OREXP*/{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '=', '>', '>', ' ', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/* SEQ */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', ' ', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/* SUB */{' ', ' ', ' ', ' ', ' ', '=', '<', ' ', '>', '<', '<', '<', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/*EXPRE*/{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/* RPT */{' ', ' ', ' ', ' ', ' ', ' ', ' ', '=', '>', '>', '>', '>', '>', '<', '<', '<', '<', '<', '<', '<', '<', '>'},
/*DUPLI*/{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/*  |  */{' ', ' ', ' ', '=', '<', '<', '<', ' ', ' ', '<', '<', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
/*CHARA*/{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>'},
/*MACRO*/{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>'},
/*  (  */{' ', '=', '<', '<', '<', '<', '<', ' ', ' ', '<', '<', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
/*  )  */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>'},
/*  ?  */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/* ??  */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/*  *  */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/* *?  */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/*  +  */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/* +?  */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/*{n,m}*/{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', '<', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/*{nm}?*/{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>', '>', '>', '>', '>', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '>'},
/* END */{' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}
};

template<typename CharT>
const char *basic_num_token<CharT>::_precedence_strings[END + 1] =
#if BOOST_WORKAROUND(BOOST_INTEL_CXX_VERSION, BOOST_TESTED_AT(910))
{{"BEGIN"}, {"REGEX"}, {"OREXP"}, {"SEQUENCE"}, {"SUB"}, {"EXPRESSION"},
    {"REPEAT"}, {"DUPLICATE"}, {"|"}, {"CHARSET"}, {"MACRO"},
    {"("}, {")"}, {"?"}, {"??"}, {"*"}, {"*?"}, {"+"}, {"+?"}, {"{n[,[m]]}"},
    {"{n[,[m]]}?"}, {"END"}};
#else
{"BEGIN", "REGEX", "OREXP", "SEQUENCE", "SUB", "EXPRESSION", "REPEAT",
    "DUPLICATE", "|", "CHARSET", "MACRO", "(", ")", "?", "??", "*", "*?",
    "+", "+?", "{n[,[m]]}", "{n[,[m]]}?", "END"};
#endif
}
}
}

#endif
