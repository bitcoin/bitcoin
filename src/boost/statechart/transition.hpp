#ifndef BOOST_STATECHART_TRANSITION_HPP_INCLUDED
#define BOOST_STATECHART_TRANSITION_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/result.hpp>

#include <boost/statechart/detail/reaction_dispatcher.hpp>



namespace boost
{
namespace statechart
{



//////////////////////////////////////////////////////////////////////////////
template< class Event, class Destination,
          class TransitionContext = detail::no_context< Event >,
          void ( TransitionContext::*pTransitionAction )( const Event & ) =
            &detail::no_context< Event >::no_function >
class transition
{
  private:
    //////////////////////////////////////////////////////////////////////////
    template< class State >
    struct reactions
    {
      static result react_without_action( State & stt )
      {
        return stt.template transit< Destination >();
      }

      static result react_with_action( State & stt, const Event & evt )
      {
        return stt.template transit< Destination >( pTransitionAction, evt );
      }
    };

  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    template< class State, class EventBase, class IdType >
    static detail::reaction_result react(
      State & stt, const EventBase & evt, const IdType & eventType )
    {
      typedef detail::reaction_dispatcher<
        reactions< State >, State, EventBase, Event, TransitionContext, IdType
      > dispatcher;
      return dispatcher::react( stt, evt, eventType );
    }
};



} // namespace statechart
} // namespace boost



#endif
