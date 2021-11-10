// char_state_machine.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_CONVERSION_CHAR_STATE_MACHINE_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_CONVERSION_CHAR_STATE_MACHINE_HPP

#include "../consts.hpp"
#include <map>
#include "../size_t.hpp"
#include "../string_token.hpp"
#include <vector>

namespace boost
{
namespace lexer
{
namespace detail
{
template<typename CharT>
struct basic_char_state_machine
{
    struct state
    {
        typedef basic_string_token<CharT> string_token;
        typedef std::map<std::size_t, string_token> size_t_string_token_map;
        typedef std::pair<std::size_t, string_token> size_t_string_token_pair;

        bool _end_state;
        std::size_t _id;
        std::size_t _unique_id;
        std::size_t _state;
        std::size_t _bol_index;
        std::size_t _eol_index;
        size_t_string_token_map _transitions;

        state () :
            _end_state (false),
            _id (0),
            _unique_id (npos),
            _state (0),
            _bol_index (npos),
            _eol_index (npos)
        {
        }
    };

    typedef std::vector<state> state_vector;
    typedef std::vector<state_vector> state_vector_vector;

    state_vector_vector _sm_vector;

    bool empty () const
    {
        return _sm_vector.empty ();
    }

    void clear ()
    {
        _sm_vector.clear ();
    }

    void swap (basic_char_state_machine &csm_)
    {
        _sm_vector.swap (csm_._sm_vector);
    }
};

typedef basic_char_state_machine<char> char_state_machine;
typedef basic_char_state_machine<wchar_t> wchar_state_machine;

}
}
}

#endif
