#ifndef JSON_SPIRIT_UTILS
#define JSON_SPIRIT_UTILS

//          Copyright John W. Wilkinson 2007 - 2009.
// Distributed under the MIT License, see accompanying file LICENSE.txt

// json spirit version 4.03

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "json_spirit_value.h"
#include <map>

namespace json_spirit
{ 
    template< class Obj_t, class Map_t >
    void obj_to_map( const Obj_t& obj, Map_t& mp_obj )
    {
        mp_obj.clear();

        for( typename Obj_t::const_iterator i = obj.begin(); i != obj.end(); ++i )
        {
            mp_obj[ i->name_ ] = i->value_;
        }
    }

    template< class Obj_t, class Map_t >
    void map_to_obj( const Map_t& mp_obj, Obj_t& obj )
    {
        obj.clear();

        for( typename Map_t::const_iterator i = mp_obj.begin(); i != mp_obj.end(); ++i )
        {
            obj.push_back( typename Obj_t::value_type( i->first, i->second ) );
        }
    }

    typedef std::map< std::string, Value > Mapped_obj;

#ifndef BOOST_NO_STD_WSTRING
    typedef std::map< std::wstring, wValue > wMapped_obj;
#endif

    template< class Object_type, class String_type >
    const typename Object_type::value_type::Value_type& find_value( const Object_type& obj, const String_type& name )
    {
        for( typename Object_type::const_iterator i = obj.begin(); i != obj.end(); ++i )
        {
            if( i->name_ == name )
            {
                return i->value_;
            }
        }

        return Object_type::value_type::Value_type::null;
    }
}

#endif
