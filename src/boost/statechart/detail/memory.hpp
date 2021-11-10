#ifndef BOOST_STATECHART_DETAIL_MEMORY_HPP_INCLUDED
#define BOOST_STATECHART_DETAIL_MEMORY_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2005-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/detail/avoid_unused_warning.hpp>

#include <boost/assert.hpp>
#include <boost/detail/allocator_utilities.hpp>

#include <cstddef> // std::size_t
#include <memory>  // std::allocator_traits


namespace boost
{
namespace statechart
{

#ifdef BOOST_NO_CXX11_ALLOCATOR
typedef void none;
#else
// The specialization std::allocator<void> doesn't satisfy C++17's
// allocator completeness requirements. Therefore it is deprecated
// and should no longer be used. Supply a replacement type for all
// the allocator default template arguments in the library.
struct none {};
#endif

namespace detail
{



// defect: 'allocate' and 'deallocate' cannot handle stateful allocators!

template< class MostDerived, class Allocator >
void * allocate( std::size_t size )
{
  avoid_unused_warning( size );
  // The assert below fails when memory is allocated for an event<>,
  // simple_state<> or state<> subtype object, *and* the first template
  // parameter passed to one of these templates is not equal to the most-
  // derived object being constructed.
  // The following examples apply to all these subtypes:
  // // Example 1
  // struct A {};
  // struct B : sc::simple_state< A, /* ... */ >
  // // Above, the first template parameter must be equal to the most-
  // // derived type
  // 
  // // Example 2
  // struct A : sc::event< A >
  // struct B : A { /* ... */ };
  // void f() { delete new B(); }
  // // Above the most-derived type being constructed is B, but A was passed
  // // as the most-derived type to event<>.
  BOOST_ASSERT( size == sizeof( MostDerived ) );
  typedef typename boost::detail::allocator::rebind_to<
    Allocator, MostDerived
  >::type md_allocator;
  md_allocator alloc;
#ifdef BOOST_NO_CXX11_ALLOCATOR
  return alloc.allocate( 1, static_cast< MostDerived * >( 0 ) );
#else
  typedef std::allocator_traits<md_allocator> md_traits;
  return md_traits::allocate( alloc, 1, static_cast< MostDerived * >( 0 ) );
#endif
}

template< class MostDerived, class Allocator >
void deallocate( void * pObject )
{
  typedef typename boost::detail::allocator::rebind_to<
    Allocator, MostDerived
  >::type md_allocator;
  md_allocator alloc;
#ifdef BOOST_NO_CXX11_ALLOCATOR
  alloc.deallocate( static_cast< MostDerived * >( pObject ), 1 );
#else
  typedef std::allocator_traits<md_allocator> md_traits;
  md_traits::deallocate( alloc, static_cast< MostDerived * >( pObject ), 1 );
#endif
}



} // namespace detail
} // namespace statechart
} // namespace boost



#endif
