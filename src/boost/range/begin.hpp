// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_BEGIN_HPP
#define BOOST_RANGE_BEGIN_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/range/config.hpp>

#include <boost/range/iterator.hpp>
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
    range_begin( C& c )
    {
        //
        // If you get a compile-error here, it is most likely because
        // you have not implemented range_begin() properly in
        // the namespace of C
        //
        return c.begin();
    }

    //////////////////////////////////////////////////////////////////////
    // pair
    //////////////////////////////////////////////////////////////////////

    template< typename Iterator >
    BOOST_CONSTEXPR inline Iterator range_begin( const std::pair<Iterator,Iterator>& p )
    {
        return p.first;
    }

    template< typename Iterator >
    BOOST_CONSTEXPR inline Iterator range_begin( std::pair<Iterator,Iterator>& p )
    {
        return p.first;
    }

    //////////////////////////////////////////////////////////////////////
    // array
    //////////////////////////////////////////////////////////////////////

    //
    // May this be discarded? Or is it needed for bad compilers?
    //
    template< typename T, std::size_t sz >
    BOOST_CONSTEXPR inline const T* range_begin( const T (&a)[sz] ) BOOST_NOEXCEPT
    {
        return a;
    }

    template< typename T, std::size_t sz >
    BOOST_CONSTEXPR inline T* range_begin( T (&a)[sz] ) BOOST_NOEXCEPT
    {
        return a;
    }


#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
} // namespace 'range_detail'
#endif

// Use a ADL namespace barrier to avoid ambiguity with other unqualified
// calls. This is particularly important with C++0x encouraging
// unqualified calls to begin/end.
namespace range_adl_barrier
{

template< class T >
#if !BOOST_WORKAROUND(BOOST_GCC, < 40700)
BOOST_CONSTEXPR
#endif
inline BOOST_DEDUCED_TYPENAME range_iterator<T>::type begin( T& r )
{
#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
    using namespace range_detail;
#endif
    return range_begin( r );
}

template< class T >
#if !BOOST_WORKAROUND(BOOST_GCC, < 40700)
BOOST_CONSTEXPR
#endif
inline BOOST_DEDUCED_TYPENAME range_iterator<const T>::type begin( const T& r )
{
#if !BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
    using namespace range_detail;
#endif
    return range_begin( r );
}

    } // namespace range_adl_barrier
} // namespace boost

namespace boost
{
    namespace range_adl_barrier
    {
        template< class T >
        inline BOOST_DEDUCED_TYPENAME range_iterator<const T>::type
        const_begin( const T& r )
        {
            return boost::range_adl_barrier::begin( r );
        }
    } // namespace range_adl_barrier

    using namespace range_adl_barrier;
} // namespace boost

#endif

