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
//  Description : basic_cstring i/o implementation
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_BASIC_CSTRING_IO_HPP
#define BOOST_TEST_UTILS_BASIC_CSTRING_IO_HPP

// Boost.Test
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>

// STL
#include <iosfwd>
#include <string>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {

namespace unit_test {

#ifdef BOOST_CLASSIC_IOSTREAMS

template<typename CharT>
inline std::ostream&
operator<<( std::ostream& os, basic_cstring<CharT> const& str )
{
    typedef typename ut_detail::bcs_base_char<CharT>::type char_type;
    char_type const* const beg = reinterpret_cast<char_type const* const>( str.begin() );
    char_type const* const end = reinterpret_cast<char_type const* const>( str.end() );
    os << std::basic_string<char_type>( beg, end - beg );

    return os;
}

#else

template<typename CharT1, typename Tr,typename CharT2>
inline std::basic_ostream<CharT1,Tr>&
operator<<( std::basic_ostream<CharT1,Tr>& os, basic_cstring<CharT2> const& str )
{
    CharT1 const* const beg = reinterpret_cast<CharT1 const*>( str.begin() ); // !!
    CharT1 const* const end = reinterpret_cast<CharT1 const*>( str.end() );
    os << std::basic_string<CharT1,Tr>( beg, end - beg );

    return os;
}

#endif

//____________________________________________________________________________//


} // namespace unit_test

} // namespace boost

//____________________________________________________________________________//

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_BASIC_CSTRING_IO_HPP_071894GER
