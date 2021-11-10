//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : basic_cstring class wrap C string and provide std_string like
//                interface
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_BASIC_CSTRING_FWD_HPP
#define BOOST_TEST_UTILS_BASIC_CSTRING_FWD_HPP

#include <boost/test/detail/config.hpp>


namespace boost {

namespace unit_test {

template<typename CharT> class BOOST_SYMBOL_VISIBLE basic_cstring;
typedef basic_cstring<char const>   const_string;
#if BOOST_WORKAROUND(__DECCXX_VER, BOOST_TESTED_AT(60590041))
typedef const_string                literal_string;
#else
typedef const_string const          literal_string;
#endif

typedef char const* const           c_literal_string;

#if defined(BOOST_TEST_STRING_VIEW)
template <class CharT, class string_view_t>
class BOOST_SYMBOL_VISIBLE stringview_cstring_helper;
#endif


} // namespace unit_test

} // namespace boost

#endif // BOOST_TEST_UTILS_BASIC_CSTRING_FWD_HPP
