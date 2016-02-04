#ifndef JSON_SPIRIT_ERROR_POSITION
#define JSON_SPIRIT_ERROR_POSITION

//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.03

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <string>

namespace json_spirit
{
    // An Error_position exception is thrown by the "read_or_throw" functions below on finding an error.
    // Note the "read_or_throw" functions are around 3 times slower than the standard functions "read" 
    // functions that return a bool.
    //
    struct Error_position
    {
        Error_position();
        Error_position( unsigned int line, unsigned int column, const std::string& reason );
        bool operator==( const Error_position& lhs ) const;
        unsigned int line_;
        unsigned int column_;
        std::string reason_;
    };

    inline Error_position::Error_position()
    :   line_( 0 )
    ,   column_( 0 )
    {
    }

    inline Error_position::Error_position( unsigned int line, unsigned int column, const std::string& reason )
    :   line_( line )
    ,   column_( column )
    ,   reason_( reason )
    {
    }

    inline bool Error_position::operator==( const Error_position& lhs ) const
    {
        if( this == &lhs ) return true;

        return ( reason_ == lhs.reason_ ) &&
               ( line_   == lhs.line_ ) &&
               ( column_ == lhs.column_ ); 
}
}

#endif
