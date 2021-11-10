// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_DETAIL_EMPTY_HPP
#define BOOST_RANGE_DETAIL_EMPTY_HPP

#include <boost/range/detail/common.hpp>

namespace boost 
{
    namespace range_detail
    {
        template< typename T >
        struct range_empty;

        //////////////////////////////////////////////////////////////////////
        // default
        //////////////////////////////////////////////////////////////////////
        
        template<>
        struct range_empty<std_container_>
        {
            template< typename C >
            static bool fun( C& c )
            {
                return c.empty();
            };
        };
                    
        //////////////////////////////////////////////////////////////////////
        // pair
        //////////////////////////////////////////////////////////////////////
        
        template<>
        struct range_empty<std_pair_>
        {
            template< typename P >
            static bool fun( const P& p )
            {
                return p.first == p.second;
            }
        };
 
        //////////////////////////////////////////////////////////////////////
        // array
        //////////////////////////////////////////////////////////////////////
        
        template<>
        struct range_empty<array_>
        {
            template< typename T, std::size_t sz >
            static bool fun( T BOOST_ARRAY_REF[sz] )
            {
                if( boost_range_array == 0 )
                    return true;
                return false;
            }
        };

        //////////////////////////////////////////////////////////////////////
        // string
        //////////////////////////////////////////////////////////////////////
        
        template<>
        struct range_empty<char_ptr_>
        {
            static bool fun( const char* s )
            {
                return s == 0 || s[0] == 0;
            }
        };

        template<>
        struct range_empty<const_char_ptr_>
        {
            static bool fun( const char* s )
            {
                return  s == 0 || s[0] == 0;
            }
        };

        template<>
        struct range_empty<wchar_t_ptr_>
        {
            static bool fun( const wchar_t* s )
            {
                return  s == 0 || s[0] == 0;
            }
        };
        
        template<>
        struct range_empty<const_wchar_t_ptr_>
        {
            static bool fun( const wchar_t* s )
            {
                return  s == 0 || s[0] == 0;
            }
        };

    } // namespace 'range_detail'
    
        
    template< typename C >
    inline bool 
    empty( const C& c )
    {
        return range_detail::range_empty<  BOOST_RANGE_DEDUCED_TYPENAME range_detail::range<C>::type >::fun( c );
    }

} // namespace 'boost'


#endif
