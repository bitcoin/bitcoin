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
//  Description : common code used by any agent serving as OF_XML printer
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_XML_PRINTER_HPP
#define BOOST_TEST_UTILS_XML_PRINTER_HPP

// Boost.Test
#include <boost/test/detail/global_typedef.hpp>
#include <boost/test/utils/basic_cstring/basic_cstring.hpp>
#include <boost/test/utils/custom_manip.hpp>
#include <boost/test/utils/foreach.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

// Boost
#include <boost/config.hpp>

// STL
#include <iostream>
#include <map>

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace utils {

// ************************************************************************** //
// **************               xml print helpers              ************** //
// ************************************************************************** //

inline void
print_escaped( std::ostream& where_to, const_string value )
{
#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST) && !defined(BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX)
    static std::map<char,char const*> const char_type{{
        {'<' , "lt"},
        {'>' , "gt"},
        {'&' , "amp"},
        {'\'', "apos"},
        {'"' , "quot"}
    }};
#else
    static std::map<char,char const*> char_type;

    if( char_type.empty() ) {
        char_type['<'] = "lt";
        char_type['>'] = "gt";
        char_type['&'] = "amp";
        char_type['\'']= "apos";
        char_type['"'] = "quot";
    }
#endif

    BOOST_TEST_FOREACH( char, c, value ) {
        std::map<char,char const*>::const_iterator found_ref = char_type.find( c );

        if( found_ref != char_type.end() )
            where_to << '&' << found_ref->second << ';';
        else
            where_to << c;
    }
}

//____________________________________________________________________________//

inline void
print_escaped( std::ostream& where_to, std::string const& value )
{
    print_escaped( where_to, const_string( value ) );
}

//____________________________________________________________________________//

template<typename T>
inline void
print_escaped( std::ostream& where_to, T const& value )
{
    where_to << value;
}

//____________________________________________________________________________//

inline void
print_escaped_cdata( std::ostream& where_to, const_string value )
{
    static const_string cdata_end( "]]>" );

    const_string::size_type pos = value.find( cdata_end );
    if( pos == const_string::npos )
        where_to << value;
    else {
        where_to << value.substr( 0, pos+2 ) << cdata_end
                 << BOOST_TEST_L( "<![CDATA[" ) << value.substr( pos+2 );
    }
}

//____________________________________________________________________________//

typedef custom_manip<struct attr_value_t> attr_value;

template<typename T>
inline std::ostream&
operator<<( custom_printer<attr_value> const& p, T const& value )
{
    *p << "=\"";
    print_escaped( *p, value );
    *p << '"';

    return *p;
}

//____________________________________________________________________________//

typedef custom_manip<struct cdata_t> cdata;

inline std::ostream&
operator<<( custom_printer<cdata> const& p, const_string value )
{
    *p << BOOST_TEST_L( "<![CDATA[" );
    print_escaped_cdata( *p, value );
    return  *p << BOOST_TEST_L( "]]>" );
}

//____________________________________________________________________________//

} // namespace utils
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_XML_PRINTER_HPP
