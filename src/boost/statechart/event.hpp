#ifndef BOOST_STATECHART_EVENT_HPP_INCLUDED
#define BOOST_STATECHART_EVENT_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2007 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event_base.hpp>
#include <boost/statechart/detail/rtti_policy.hpp>
#include <boost/statechart/detail/memory.hpp>

#include <boost/polymorphic_cast.hpp> // boost::polymorphic_downcast

#include <memory> // std::allocator



namespace boost
{
namespace statechart
{



//////////////////////////////////////////////////////////////////////////////
template< class MostDerived, class Allocator = std::allocator< none > >
class event : public detail::rtti_policy::rtti_derived_type<
  MostDerived, event_base >
{
  public:
    //////////////////////////////////////////////////////////////////////////
    // Compiler-generated copy constructor and copy assignment operator are
    // fine

    void * operator new( std::size_t size )
    {
      return detail::allocate< MostDerived, Allocator >( size );
    }

    void * operator new( std::size_t, void * p )
    {
      return p;
    }

    void operator delete( void * pEvent )
    {
      detail::deallocate< MostDerived, Allocator >( pEvent );
    }

    void operator delete( void * pEvent, void * p )
    {
    }

  protected:
    //////////////////////////////////////////////////////////////////////////
    event() {}
    virtual ~event() {}

  private:
    //////////////////////////////////////////////////////////////////////////
    virtual intrusive_ptr< const event_base > clone() const
    {
      return intrusive_ptr< const event_base >( new MostDerived(
        *polymorphic_downcast< const MostDerived * >( this ) ) );
    }
};



} // namespace statechart
} // namespace boost



#endif
