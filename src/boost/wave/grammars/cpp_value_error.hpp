/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WAVE_CPP_VALUE_ERROR_INCLUDED)
#define BOOST_WAVE_CPP_VALUE_ERROR_INCLUDED

#include <boost/wave/wave_config.hpp>

// this must occur after all of the includes and before any code appears
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost {
namespace wave {
namespace grammars {

///////////////////////////////////////////////////////////////////////////////
//
//  value_error enum type
//
//    This is used to encode any error occurred during the evaluation of a
//    conditional preprocessor expression
//
///////////////////////////////////////////////////////////////////////////////
enum value_error {
    error_noerror = 0x0,
    error_division_by_zero = 0x1,
    error_integer_overflow = 0x2,
    error_character_overflow = 0x4
};

///////////////////////////////////////////////////////////////////////////////
}   //  namespace grammars
}   //  namespace wave
}   //  namespace boost

// the suffix header occurs after all of the code
#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // !defined(BOOST_WAVE_CPP_VALUE_ERROR_INCLUDED)
