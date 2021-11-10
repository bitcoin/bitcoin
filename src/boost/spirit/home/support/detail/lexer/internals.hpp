// internals.hpp
// Copyright (c) 2009 Ben Hanson (http://www.benhanson.net/)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_INTERNALS_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_LEXER_INTERNALS_HPP

#include "containers/ptr_vector.hpp"

namespace boost
{
namespace lexer
{
namespace detail
{
struct internals
{
    typedef std::vector<std::size_t> size_t_vector;
    typedef ptr_vector<size_t_vector> size_t_vector_vector;

    size_t_vector_vector _lookup;
    size_t_vector _dfa_alphabet;
    size_t_vector_vector _dfa;
    bool _seen_BOL_assertion;
    bool _seen_EOL_assertion;

    internals () :
        _seen_BOL_assertion (false),
        _seen_EOL_assertion (false)
    {
    }

    void clear ()
    {
        _lookup.clear ();
        _dfa_alphabet.clear ();
        _dfa.clear ();
        _seen_BOL_assertion = false;
        _seen_EOL_assertion = false;
    }

    void swap (internals &internals_)
    {
        _lookup->swap (*internals_._lookup);
        _dfa_alphabet.swap (internals_._dfa_alphabet);
        _dfa->swap (*internals_._dfa);
        std::swap (_seen_BOL_assertion, internals_._seen_BOL_assertion);
        std::swap (_seen_EOL_assertion, internals_._seen_EOL_assertion);
    }

private:
    internals (const internals &); // No copy construction.
    internals &operator = (const internals &); // No assignment.
};
}
}
}

#endif
