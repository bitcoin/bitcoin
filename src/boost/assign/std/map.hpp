// Boost.Assign library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/assign/
//


#ifndef BOOST_ASSIGN_STD_MAP_HPP
#define BOOST_ASSIGN_STD_MAP_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/assign/list_inserter.hpp>
#include <boost/config.hpp>
#include <map>

namespace boost
{
namespace assign
{

    template< class K, class V, class C, class A, class P >
    inline list_inserter< assign_detail::call_insert< std::map<K,V,C,A> >, P >
    operator+=( std::map<K,V,C,A>& m, const P& p )
    {
        return insert( m )( p );
    }
    
    template< class K, class V, class C, class A, class P >
    inline list_inserter< assign_detail::call_insert< std::multimap<K,V,C,A> >, P >
    operator+=( std::multimap<K,V,C,A>& m, const P& p )
    {
        return insert( m )( p );
    }

}
}

#endif
