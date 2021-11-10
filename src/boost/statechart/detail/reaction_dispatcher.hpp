#ifndef BOOST_STATECHART_REACTION_DISPATCHER_HPP_INCLUDED
#define BOOST_STATECHART_REACTION_DISPATCHER_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2008 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/result.hpp>

#include <boost/mpl/if.hpp>

#include <boost/polymorphic_cast.hpp> // boost::polymorphic_downcast
#include <boost/type_traits/is_same.hpp>



namespace boost
{
namespace statechart
{
namespace detail
{



//////////////////////////////////////////////////////////////////////////////
template< class Event >
struct no_context
{
  void no_function( const Event & );
};

//////////////////////////////////////////////////////////////////////////////
template<
  class Reactions, class State, class EventBase, class Event,
  class ActionContext, class IdType >
class reaction_dispatcher
{
  private:
    struct without_action
    {
      static result react( State & stt, const EventBase & )
      {
        return Reactions::react_without_action( stt );
      }
    };

    struct base_with_action
    {
      static result react( State & stt, const EventBase & evt )
      {
        return Reactions::react_with_action( stt, evt );
      }
    };

    struct base
    {
      static result react(
        State & stt, const EventBase & evt, const IdType & )
      {
        typedef typename mpl::if_<
          is_same< ActionContext, detail::no_context< Event > >,
          without_action, base_with_action
        >::type reaction;
        return reaction::react( stt, evt );
      }
    };

    struct derived_with_action
    {
      static result react( State & stt, const EventBase & evt )
      {
        return Reactions::react_with_action(
          stt, *polymorphic_downcast< const Event * >( &evt ) );
      }
    };

    struct derived
    {
      static result react(
        State & stt, const EventBase & evt, const IdType & eventType )
      {
        if ( eventType == Event::static_type() )
        {
          typedef typename mpl::if_<
            is_same< ActionContext, detail::no_context< Event > >,
            without_action, derived_with_action
          >::type reaction;
          return reaction::react( stt, evt );
        }
        else
        {
          return detail::result_utility::make_result( detail::no_reaction );
        }
      }
    };

  public:
    static reaction_result react(
      State & stt, const EventBase & evt, const IdType & eventType )
    {
      typedef typename mpl::if_<
        is_same< Event, EventBase >, base, derived
      >::type reaction;
      return result_utility::get_result(
        reaction::react( stt, evt, eventType ) );
    }
};



} // namespace detail
} // namespace statechart
} // namespace boost



#endif
