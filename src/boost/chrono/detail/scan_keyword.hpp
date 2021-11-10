//  scan_keyword.hpp  --------------------------------------------------------------//
//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//  Adaptation to Boost of the libcxx

//  Copyright 2010 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_CHRONO_DETAIL_SCAN_KEYWORD_HPP
#define BOOST_CHRONO_DETAIL_SCAN_KEYWORD_HPP

#include <boost/chrono/config.hpp>

#include <boost/move/unique_ptr.hpp>
#include <ios>
#include <exception>
#include <stdlib.h>
#include <boost/throw_exception.hpp>

namespace boost {
    using movelib::unique_ptr;

namespace chrono {
namespace chrono_detail {

inline void free_aux(void* ptr) { free(ptr); }

// scan_keyword
// Scans [b, e) until a match is found in the basic_strings range
//  [kb, ke) or until it can be shown that there is no match in [kb, ke).
//  b will be incremented (visibly), consuming CharT until a match is found
//  or proved to not exist.  A keyword may be "", in which will match anything.
//  If one keyword is a prefix of another, and the next CharT in the input
//  might match another keyword, the algorithm will attempt to find the longest
//  matching keyword.  If the longer matching keyword ends up not matching, then
//  no keyword match is found.  If no keyword match is found, ke is returned
//  and failbit is set in err.
//  Else an iterator pointing to the matching keyword is found.  If more than
//  one keyword matches, an iterator to the first matching keyword is returned.
//  If on exit b == e, eofbit is set in err.
//  Examples:
//  Keywords:  "a", "abb"
//  If the input is "a", the first keyword matches and eofbit is set.
//  If the input is "abc", no match is found and "ab" are consumed.

template <class InputIterator, class ForwardIterator>
ForwardIterator
scan_keyword(InputIterator& b, InputIterator e,
               ForwardIterator kb, ForwardIterator ke,
               std::ios_base::iostate& err
               )
{
    typedef typename std::iterator_traits<InputIterator>::value_type CharT;
    size_t nkw = std::distance(kb, ke);
    const unsigned char doesnt_match = '\0';
    const unsigned char might_match = '\1';
    const unsigned char does_match = '\2';
    unsigned char statbuf[100];
    unsigned char* status = statbuf;
    //  Change free by free_aux to avoid
    // Error: Could not find a match for boost::interprocess::unique_ptr<unsigned char, void(*)(void*)>::unique_ptr(int, extern "C" void(void*))
    unique_ptr<unsigned char, void(*)(void*)> stat_hold(0, free_aux);
    if (nkw > sizeof(statbuf))
    {
        status = (unsigned char*)malloc(nkw);
        if (status == 0)
          throw_exception(std::bad_alloc());
        stat_hold.reset(status);
    }
    size_t n_might_match = nkw;  // At this point, any keyword might match
    size_t n_does_match = 0;       // but none of them definitely do
    // Initialize all statuses to might_match, except for "" keywords are does_match
    unsigned char* st = status;
    for (ForwardIterator ky = kb; ky != ke; ++ky, ++st)
    {
        if (!ky->empty())
            *st = might_match;
        else
        {
            *st = does_match;
            --n_might_match;
            ++n_does_match;
        }
    }
    // While there might be a match, test keywords against the next CharT
    for (size_t indx = 0; b != e && n_might_match > 0; ++indx)
    {
        // Peek at the next CharT but don't consume it
        CharT c = *b;
        bool consume = false;
        // For each keyword which might match, see if the indx character is c
        // If a match if found, consume c
        // If a match is found, and that is the last character in the keyword,
        //    then that keyword matches.
        // If the keyword doesn't match this character, then change the keyword
        //    to doesn't match
        st = status;
        for (ForwardIterator ky = kb; ky != ke; ++ky, ++st)
        {
            if (*st == might_match)
            {
                CharT kc = (*ky)[indx];
                if (c == kc)
                {
                    consume = true;
                    if (ky->size() == indx+1)
                    {
                        *st = does_match;
                        --n_might_match;
                        ++n_does_match;
                    }
                }
                else
                {
                    *st = doesnt_match;
                    --n_might_match;
                }
            }
        }
        // consume if we matched a character
        if (consume)
        {
            ++b;
            // If we consumed a character and there might be a matched keyword that
            //   was marked matched on a previous iteration, then such keywords
            //   which are now marked as not matching.
            if (n_might_match + n_does_match > 1)
            {
                st = status;
                for (ForwardIterator ky = kb; ky != ke; ++ky, ++st)
                {
                    if (*st == does_match && ky->size() != indx+1)
                    {
                        *st = doesnt_match;
                        --n_does_match;
                    }
                }
            }
        }
    }
    // We've exited the loop because we hit eof and/or we have no more "might matches".
    if (b == e)
        err |= std::ios_base::eofbit;
    // Return the first matching result
    for (st = status; kb != ke; ++kb, ++st)
        if (*st == does_match)
            break;
    if (kb == ke)
        err |= std::ios_base::failbit;
    return kb;
}
}
}
}
#endif // BOOST_CHRONO_DETAIL_SCAN_KEYWORD_HPP
