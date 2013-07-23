//          Copyright John W. Wilkinson 2007 - 2013
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.06

#include "json_spirit_writer.h"
#include "json_spirit_writer_template.h"

using namespace json_spirit;

#ifdef JSON_SPIRIT_VALUE_ENABLED
    void json_spirit::write( const Value& value, std::ostream& os, unsigned int options )
    {
        write_stream( value, os, options );
    }
    std::string json_spirit::write( const Value& value, unsigned int options )
    {
        return write_string( value, options );
    }

    void json_spirit::write_formatted( const Value& value, std::ostream& os )
    {
        write_stream( value, os, pretty_print );
    }

    std::string json_spirit::write_formatted( const Value& value )
    {
        return write_string( value, pretty_print );
    }
#endif

#ifdef JSON_SPIRIT_MVALUE_ENABLED
    void json_spirit::write( const mValue& value, std::ostream& os, unsigned int options )
    {
        write_stream( value, os, options );
    }

    std::string json_spirit::write( const mValue& value, unsigned int options )
    {
        return write_string( value, options );
    }

    void json_spirit::write_formatted( const mValue& value, std::ostream& os )
    {
        write_stream( value, os, pretty_print );
    }

    std::string json_spirit::write_formatted( const mValue& value )
    {
        return write_string( value, pretty_print );
    }
#endif

#if defined( JSON_SPIRIT_WVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
    void json_spirit::write( const wValue& value, std::wostream& os, unsigned int options )
    {
        write_stream( value, os, options );
    }

    std::wstring json_spirit::write( const wValue& value, unsigned int options )
    {
        return write_string( value, options );
    }

    void json_spirit::write_formatted( const wValue& value, std::wostream& os )
    {
        write_stream( value, os, pretty_print );
    }

    std::wstring json_spirit::write_formatted( const wValue& value )
    {
        return write_string( value, pretty_print );
    }
#endif

#if defined( JSON_SPIRIT_WMVALUE_ENABLED ) && !defined( BOOST_NO_STD_WSTRING )
    void json_spirit::write_formatted( const wmValue& value, std::wostream& os )
    {
        write_stream( value, os, pretty_print );
    }

    std::wstring json_spirit::write_formatted( const wmValue& value )
    {
        return write_string( value, pretty_print );
    }

    void json_spirit::write( const wmValue& value, std::wostream& os, unsigned int options )
    {
        write_stream( value, os, options );
    }

    std::wstring json_spirit::write( const wmValue& value, unsigned int options )
    {
        return write_string( value, options );
    }
#endif
