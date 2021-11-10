#ifndef BOOST_STATECHART_EVENT_BASE_HPP_INCLUDED
#define BOOST_STATECHART_EVENT_BASE_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/detail/rtti_policy.hpp>
#include <boost/statechart/detail/counted_base.hpp>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/config.hpp>



namespace boost
{
namespace statechart
{
namespace detail
{



// This helper is necessary because there doesn't seem to be consensus among
// compilers on how a friend declaration for a function in another namespace
// has to look like.
class delete_helper
{
  public:
    template< class T >
    static void delete_object( const T * pObject )
    {
      delete pObject;
    }
};



} // namespace detail



//////////////////////////////////////////////////////////////////////////////
class event_base : public detail::rtti_policy::rtti_base_type<
  detail::counted_base<> >
{
  typedef detail::rtti_policy::rtti_base_type<
    detail::counted_base<> > base_type;
  public:
    //////////////////////////////////////////////////////////////////////////
    intrusive_ptr< const event_base > intrusive_from_this() const;

  protected:
    //////////////////////////////////////////////////////////////////////////
    event_base( detail::rtti_policy::id_provider_type idProvider ) :
      base_type( idProvider )
    {
    }

    virtual ~event_base() {}

  private:
    //////////////////////////////////////////////////////////////////////////
    virtual intrusive_ptr< const event_base > clone() const = 0;

    friend class detail::delete_helper;
};



#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
} // namespace statechart
#endif



inline void intrusive_ptr_add_ref( const ::boost::statechart::event_base * pBase )
{
  pBase->add_ref();
}

inline void intrusive_ptr_release( const ::boost::statechart::event_base * pBase )
{
  if ( pBase->release() )
  {
    ::boost::statechart::detail::delete_helper::delete_object( pBase );
  }
}



#ifndef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
} // namespace statechart
#endif
namespace statechart
{



// We're implementing this here so that GCC3.4.2 can find
// intrusive_ptr_add_ref, which is indirectly called from the intrusive_ptr
// ctor.
inline intrusive_ptr< const event_base > event_base::intrusive_from_this() const
{
  if ( base_type::ref_counted() )
  {
    return intrusive_ptr< const event_base >( this );
  }
  else
  {
    return clone();
  }
}



} // namespace statechart
} // namespace boost



#endif
