#ifndef JSON_SPIRIT_WRITER
#define JSON_SPIRIT_WRITER

//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.03

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "json_spirit_value.h"
#include <iostream>

namespace json_spirit
{
    // functions to convert JSON Values to text, 
    // the "formatted" versions add whitespace to format the output nicely

    void         write          ( const Value& value, std::ostream&  os );
    void         write_formatted( const Value& value, std::ostream&  os );
    std::string  write          ( const Value& value );
    std::string  write_formatted( const Value& value );

#ifndef BOOST_NO_STD_WSTRING

    void         write          ( const wValue& value, std::wostream& os );
    void         write_formatted( const wValue& value, std::wostream& os );
    std::wstring write          ( const wValue& value );
    std::wstring write_formatted( const wValue& value );

#endif

    void         write          ( const mValue& value, std::ostream&  os );
    void         write_formatted( const mValue& value, std::ostream&  os );
    std::string  write          ( const mValue& value );
    std::string  write_formatted( const mValue& value );

#ifndef BOOST_NO_STD_WSTRING

    void         write          ( const wmValue& value, std::wostream& os );
    void         write_formatted( const wmValue& value, std::wostream& os );
    std::wstring write          ( const wmValue& value );
    std::wstring write_formatted( const wmValue& value );

#endif
}

#endif
