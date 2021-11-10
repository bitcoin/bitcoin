// charset.hpp
// Copyright (c) 2007-2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARTITION_CHARSET_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_PARTITION_CHARSET_HPP

#include <set>
#include "../size_t.hpp"
#include "../string_token.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
template<typename CharT>
struct basic_charset
{
    typedef basic_string_token<CharT> token;
    typedef std::set<std::size_t> index_set;

    token _token;
    index_set _index_set;

    basic_charset ()
    {
    }

    basic_charset (const token &token_, const std::size_t index_) :
        _token (token_)
    {
        _index_set.insert (index_);
    }

    bool empty () const
    {
        return _token.empty () && _index_set.empty ();
    }

    void intersect (basic_charset &rhs_, basic_charset &overlap_)
    {
        _token.intersect (rhs_._token, overlap_._token);

        if (!overlap_._token.empty ())
        {
            typename index_set::const_iterator iter_ = _index_set.begin ();
            typename index_set::const_iterator end_ = _index_set.end ();

            for (; iter_ != end_; ++iter_)
            {
                overlap_._index_set.insert (*iter_);
            }

            iter_ = rhs_._index_set.begin ();
            end_ = rhs_._index_set.end ();

            for (; iter_ != end_; ++iter_)
            {
                overlap_._index_set.insert (*iter_);
            }

            if (_token.empty ())
            {
                _index_set.clear ();
            }

            if (rhs_._token.empty ())
            {
                rhs_._index_set.clear ();
            }
        }
    }
};
}
}
}

#endif
