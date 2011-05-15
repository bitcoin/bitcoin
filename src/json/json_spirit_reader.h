#ifndef JSON_SPIRIT_READER
#define JSON_SPIRIT_READER

//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.03

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "json_spirit_value.h"
#include "json_spirit_error_position.h"
#include <iostream>

namespace json_spirit
{
    // functions to reads a JSON values

    bool read( const std::string& s, Value& value );
    bool read( std::istream& is,     Value& value );
    bool read( std::string::const_iterator& begin, std::string::const_iterator end, Value& value );

    void read_or_throw( const std::string& s, Value& value );
    void read_or_throw( std::istream& is,     Value& value );
    void read_or_throw( std::string::const_iterator& begin, std::string::const_iterator end, Value& value );

#ifndef BOOST_NO_STD_WSTRING

    bool read( const std::wstring& s, wValue& value );
    bool read( std::wistream&  is,    wValue& value );
    bool read( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wValue& value );

    void read_or_throw( const std::wstring& s, wValue& value );
    void read_or_throw( std::wistream& is,     wValue& value );
    void read_or_throw( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wValue& value );

#endif

    bool read( const std::string& s, mValue& value );
    bool read( std::istream& is,     mValue& value );
    bool read( std::string::const_iterator& begin, std::string::const_iterator end, mValue& value );

    void read_or_throw( const std::string& s, mValue& value );
    void read_or_throw( std::istream& is,     mValue& value );
    void read_or_throw( std::string::const_iterator& begin, std::string::const_iterator end, mValue& value );

#ifndef BOOST_NO_STD_WSTRING

    bool read( const std::wstring& s, wmValue& value );
    bool read( std::wistream& is,     wmValue& value );
    bool read( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wmValue& value );

    void read_or_throw( const std::wstring& s, wmValue& value );
    void read_or_throw( std::wistream& is,     wmValue& value );
    void read_or_throw( std::wstring::const_iterator& begin, std::wstring::const_iterator end, wmValue& value );

#endif
}

#endif
