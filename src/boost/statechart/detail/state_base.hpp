#ifndef BOOST_STATECHART_DETAIL_STATE_BASE_HPP_INCLUDED
#define BOOST_STATECHART_DETAIL_STATE_BASE_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/result.hpp>
#include <boost/statechart/event.hpp>

#include <boost/statechart/detail/counted_base.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp> // BOOST_MSVC

#include <boost/detail/workaround.hpp>
#include <boost/detail/allocator_utilities.hpp>

#ifdef BOOST_MSVC
#  pragma warning( push )
#  pragma warning( disable: 4702 ) // unreachable code (in release mode only)
#endif

#include <list>

#ifdef BOOST_MSVC
#  pragma warning( pop )
#endif



namespace boost
{
namespace statechart
{
namespace detail
{



template< class Allocator, class RttiPolicy >
class leaf_state;
template< class Allocator, class RttiPolicy >
class node_state_base;

typedef unsigned char orthogonal_position_type;



//////////////////////////////////////////////////////////////////////////////
template< class Allocator, class RttiPolicy >
class state_base :
  #ifndef NDEBUG
  noncopyable,
  #endif
  public RttiPolicy::template rtti_base_type<
    // Derived class objects will be created, handled and destroyed by exactly
    // one thread --> locking is not necessary
    counted_base< false > >
{
  typedef typename RttiPolicy::template rtti_base_type<
    counted_base< false > > base_type;

  public:
    //////////////////////////////////////////////////////////////////////////
    void exit() {}

    virtual const state_base * outer_state_ptr() const = 0;

  protected:
    //////////////////////////////////////////////////////////////////////////
    state_base( typename RttiPolicy::id_provider_type idProvider ) :
      base_type( idProvider ),
      deferredEvents_( false )
    {
    }

    #if BOOST_WORKAROUND( __GNUC__, BOOST_TESTED_AT( 4 ) )
    // We make the destructor virtual for GCC because with this compiler there
    // is currently no way to disable the "has virtual functions but
    // non-virtual destructor" warning on a class by class basis. Although it
    // can be done on the compiler command line with -Wno-non-virtual-dtor,
    // this is undesirable as this would also suppress legitimate warnings for
    // types that are not states.
    virtual ~state_base() {}
    #else
    // This destructor is not virtual for performance reasons. The library
    // ensures that a state object is never deleted through a state_base
    // pointer but only through a pointer to the most-derived type.
    ~state_base() {}
    #endif

  protected:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only protected because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    void defer_event()
    {
      deferredEvents_ = true;
    }

    bool deferred_events() const
    {
      return deferredEvents_;
    }

    template< class Context >
    void set_context( orthogonal_position_type position, Context * pContext )
    {
      pContext->add_inner_state( position, this );
    }

  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    virtual detail::reaction_result react_impl(
      const event_base & evt,
      typename RttiPolicy::id_type eventType ) = 0;

    typedef intrusive_ptr< node_state_base< Allocator, RttiPolicy > >
      node_state_base_ptr_type;
    typedef intrusive_ptr< leaf_state< Allocator, RttiPolicy > >
      leaf_state_ptr_type;
    typedef std::list<
      leaf_state_ptr_type,
      typename boost::detail::allocator::rebind_to<
        Allocator, leaf_state_ptr_type >::type
    > state_list_type;

    virtual void remove_from_state_list(
      typename state_list_type::iterator & statesEnd,
      node_state_base_ptr_type & pOutermostUnstableState,
      bool performFullExit ) = 0;

  private:
    //////////////////////////////////////////////////////////////////////////
    bool deferredEvents_;
};



#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
} // namespace detail
} // namespace statechart
#endif



template< class Allocator, class RttiPolicy >
inline void intrusive_ptr_add_ref(
  const ::boost::statechart::detail::state_base< Allocator, RttiPolicy > * pBase )
{
  pBase->add_ref();
}

template< class Allocator, class RttiPolicy >
inline void intrusive_ptr_release( 
  const ::boost::statechart::detail::state_base< Allocator, RttiPolicy > * pBase )
{
  if ( pBase->release() )
  {
    // The state_base destructor is *not* virtual for performance reasons
    // but intrusive_ptr< state_base > objects are nevertheless used to point
    // to states. This assert ensures that such a pointer is never the last
    // one referencing a state object.
    BOOST_ASSERT( false );
  }
}



#ifndef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
} // namespace detail
} // namespace statechart
#endif



} // namespace boost



#endif
