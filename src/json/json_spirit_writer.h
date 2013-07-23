#ifndef JSON_SPIRIT_WRITER
#define JSON_SPIRIT_WRITER

//          Copyright John W. Wilkinson 2007 - 2013
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.06

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "json_spirit_value.h"
#include "json_spirit_writer_options.h"
#include <iostream>

namespace json_spirit
{
    // these functions to convert JSON Values to text

#ifdef JSON_SPIRIT_VALUE_ENABLED
    void         write( const Value&  value, std::ostream&  os, unsigned int options = 0 );
    std::string  write( const Value&  value, unsigned int options = 0 );
#endif

#ifdef JSON_SPIRIT_MVALUE_ENABLED
    void         write( const mValue& value, std::ostream&  os, unsigned int options = 0 );
    std::string  write( const mValue& value, unsigned int options = 0 );
#endif

#if defined( JSON_SPIRIT_WVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
    void         write( const wValue&  value, std::wostream& os, unsigned int options = 0 );
    std::wstring write( const wValue&  value, unsigned int options = 0 );
#endif

#if defined( JSON_SPIRIT_WMVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
    void         write( const wmValue& value, std::wostream& os, unsigned int options = 0 );
    std::wstring write( const wmValue& value, unsigned int options = 0 );
#endif

    // these "formatted" versions of the "write" functions are the equivalent of the above functions
    // with option "pretty_print"
    
#ifdef JSON_SPIRIT_VALUE_ENABLED
    void         write_formatted( const Value& value, std::ostream&  os );
    std::string  write_formatted( const Value& value );
#endif
#ifdef JSON_SPIRIT_MVALUE_ENABLED
    void         write_formatted( const mValue& value, std::ostream&  os );
    std::string  write_formatted( const mValue& value );
#endif

#if defined( JSON_SPIRIT_WVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
    void         write_formatted( const wValue& value, std::wostream& os );
    std::wstring write_formatted( const wValue& value );
#endif
#if defined( JSON_SPIRIT_WMVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
    void         write_formatted( const wmValue& value, std::wostream& os );
    std::wstring write_formatted( const wmValue& value );
#endif
}

#endif
