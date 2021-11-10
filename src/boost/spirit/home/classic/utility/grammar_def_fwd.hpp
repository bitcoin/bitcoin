/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_GRAMMAR_DEF_FWD_HPP)
#define BOOST_SPIRIT_GRAMMAR_DEF_FWD_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/phoenix/tuples.hpp>

#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>

#if !defined(BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT)
#define BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT PHOENIX_LIMIT
#endif

//  Calculate an integer rounded up to the nearest integer dividable by 3
#if BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT > 12
#define BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A     15
#elif BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT > 9
#define BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A     12
#elif BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT > 6
#define BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A     9
#elif BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT > 3
#define BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A     6
#else
#define BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A     3
#endif

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    template <
        typename T,
        BOOST_PP_ENUM_BINARY_PARAMS(
            BOOST_PP_DEC(BOOST_SPIRIT_GRAMMAR_STARTRULE_TYPE_LIMIT_A),
            typename T, = ::phoenix::nil_t BOOST_PP_INTERCEPT
        )
    >
    class grammar_def;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS

#endif

