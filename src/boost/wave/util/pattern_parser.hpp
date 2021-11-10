/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Global application configuration

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_SPIRIT_PATTERN_PARSER_HPP)
#define BOOST_SPIRIT_PATTERN_PARSER_HPP

#include <boost/spirit/include/classic_primitives.hpp>
#include <boost/wave/wave_config.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace util {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  pattern_and class
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT = char>
    struct pattern_and
      : public boost::spirit::classic::char_parser<pattern_and<CharT> >
    {
        pattern_and(CharT pattern_, unsigned long pattern_mask_ = 0UL)
        :   pattern(pattern_),
            pattern_mask((0UL != pattern_mask_) ?
                pattern_mask_ : (unsigned long)pattern_)
        {}

        template <typename T>
        bool test(T pattern_) const
        { return ((unsigned long)pattern_ & pattern_mask) == (unsigned long)pattern; }

        CharT         pattern;
        unsigned long pattern_mask;
    };

    template <typename CharT>
    inline pattern_and<CharT>
    pattern_p(CharT pattern, unsigned long pattern_mask = 0UL)
    { return pattern_and<CharT>(pattern, pattern_mask); }

///////////////////////////////////////////////////////////////////////////////
}   // namespace util
}   // namespace wave
}   // namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // defined(BOOST_SPIRIT_PATTERN_PARSER_HPP)
