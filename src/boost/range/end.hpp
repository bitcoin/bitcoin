// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_END_HPP
#define BOOST_RANGE_END_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/range/config.hpp>

#include <boost/range/detail/implementation_help.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/const_iterator.hpp>
#include <boost/config.hpp>
#include <boost/config/workaround.hpp>

namespace boost
{

#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
namespace range_detail
{
#endif

        //////////////////////////////////////////////////////////////////////
        // primary template
        //////////////////////////////////////////////////////////////////////
        template< typename C >
        BOOST_CONSTEXPR inline BOOST_DEDUCED_TYPENAME range_iterator<C>::type
        range_end( C& c )
        {
            //
            // If you get a compile-error here, it is most likely because
            // you have not implemented range_begin() properly in
            // the namespace of C
            //
            return c.end();
        }

        //////////////////////////////////////////////////////////////////////
        // pair
        //////////////////////////////////////////////////////////////////////

        template< typename Iterator >
        BOOST_CONSTEXPR inline Iterator range_end( const std::pair<Iterator,Iterator>& p )
        {
            return p.second;
        }

        template< typename Iterator >
        BOOST_CONSTEXPR inline Iterator range_end( std::pair<Iterator,Iterator>& p )
        {
            return p.second;
        }

        //////////////////////////////////////////////////////////////////////
        // array
        //////////////////////////////////////////////////////////////////////

        template< typename T, std::size_t sz >
        BOOST_CONSTEXPR inline const T* range_end( const T (&a)[sz] ) BOOST_NOEXCEPT
        {
            return range_detail::array_end<T,sz>( a );
        }

        template< typename T, std::size_t sz >
        BOOST_CONSTEXPR inline T* range_end( T (&a)[sz] ) BOOST_NOEXCEPT
        {
            return range_detail::array_end<T,sz>( a );
        }

#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
} // namespace 'range_detail'
#endif

namespace range_adl_barrier
{

template< class T >
#if !BOOST_WORKAROUND(BOOST_GCC, < 40700)
BOOST_CONSTEXPR
#endif
inline BOOST_DEDUCED_TYPENAME range_iterator<T>::type end( T& r )
{
#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
    using namespace range_detail;
#endif
    return range_end( r );
}

template< class T >
#if !BOOST_WORKAROUND(BOOST_GCC, < 40700)
BOOST_CONSTEXPR
#endif
inline BOOST_DEDUCED_TYPENAME range_iterator<const T>::type end( const T& r )
{
#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
    using namespace range_detail;
#endif
    return range_end( r );
}

    } // namespace range_adl_barrier
} // namespace 'boost'

namespace boost
{
    namespace range_adl_barrier
    {
        template< class T >
        BOOST_CONSTEXPR inline BOOST_DEDUCED_TYPENAME range_iterator<const T>::type
        const_end( const T& r )
        {
            return boost::range_adl_barrier::end( r );
        }
    } // namespace range_adl_barrier
    using namespace range_adl_barrier;
} // namespace boost

#endif

