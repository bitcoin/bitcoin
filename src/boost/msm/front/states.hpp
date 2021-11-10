// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_STATES_H
#define BOOST_MSM_FRONT_STATES_H

#include <boost/mpl/bool.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/msm/front/common_states.hpp>
#include <boost/msm/row_tags.hpp>
#include <boost/msm/back/metafunctions.hpp>

namespace boost { namespace msm { namespace front
{

struct no_sm_ptr 
{
    // tags
    typedef ::boost::mpl::bool_<false>   needs_sm;
};
struct sm_ptr 
{
    // tags
    typedef ::boost::mpl::bool_<true>    needs_sm;
};
// kept for backward compatibility
struct NoSMPtr 
{
    // tags
    typedef ::boost::mpl::bool_<false>   needs_sm;
};
struct SMPtr 
{
    // tags
    typedef ::boost::mpl::bool_<true>    needs_sm;
};

// provides the typedefs and interface. Concrete states derive from it.
// template argument: pointer-to-fsm policy
template<class BASE = default_base_state,class SMPtrPolicy = no_sm_ptr>
struct state :  public boost::msm::front::detail::state_base<BASE>, SMPtrPolicy
{
    // tags
    // default: no flag
    typedef ::boost::mpl::vector0<>       flag_list;
    typedef ::boost::mpl::vector0<>       internal_flag_list;
    //default: no deferred events
    typedef ::boost::mpl::vector0<>       deferred_events;
};

// terminate state simply defines the TerminateFlag flag
// template argument: pointer-to-fsm policy
template<class BASE = default_base_state,class SMPtrPolicy = no_sm_ptr>
struct terminate_state : public boost::msm::front::detail::state_base<BASE>, SMPtrPolicy
{
    // tags
    typedef ::boost::mpl::vector0<>                               flag_list;
    typedef ::boost::mpl::vector< boost::msm::TerminateFlag>      internal_flag_list;
    //default: no deferred events
    typedef ::boost::mpl::vector0<>                               deferred_events;
};

// terminate state simply defines the InterruptedFlag and EndInterruptFlag<EndInterruptEvent> flags
// template argument: event which ends the interrupt
// template argument: pointer-to-fsm policy
template <class EndInterruptEvent,class BASE = default_base_state,class SMPtrPolicy = no_sm_ptr>
struct interrupt_state : public boost::msm::front::detail::state_base<BASE>, SMPtrPolicy
{
    // tags
    typedef ::boost::mpl::vector0<>                           flag_list;
    typedef typename boost::msm::back::build_interrupt_state_flag_list<
        typename boost::msm::back::get_interrupt_events<EndInterruptEvent>::type
    >::type internal_flag_list; 

    //default: no deferred events
    typedef ::boost::mpl::vector0<>                           deferred_events;
};

// not a state but a bunch of extra typedefs to handle direct entry into a composite state. To be derived from
// template argument: zone index of this state
template <int ZoneIndex=-1>
struct explicit_entry 
{
    typedef int explicit_entry_state;
    enum {zone_index=ZoneIndex};
};

// to be derived from. Makes a type an entry (pseudo) state. Actually an almost full-fledged state
// template argument: containing composite
// template argument: zone index of this state
// template argument: pointer-to-fsm policy
template<int ZoneIndex=-1,class BASE = default_base_state,class SMPtrPolicy = no_sm_ptr>
struct entry_pseudo_state
    :  public boost::msm::front::detail::state_base<BASE>,SMPtrPolicy
{
    // tags
    typedef int                          pseudo_entry;
    enum {zone_index=ZoneIndex};
    typedef int explicit_entry_state;
    // default: no flag
    typedef ::boost::mpl::vector0<>       flag_list;
    typedef ::boost::mpl::vector0<>       internal_flag_list;
    //default: no deferred events
    typedef ::boost::mpl::vector0<>       deferred_events;
};

// to be derived from. Makes a state an exit (pseudo) state. Actually an almost full-fledged state
// template argument: event to forward
// template argument: pointer-to-fsm policy
template<class Event,class BASE = default_base_state,class SMPtrPolicy = no_sm_ptr>
struct exit_pseudo_state : public boost::msm::front::detail::state_base<BASE> , SMPtrPolicy
{
    typedef Event       event;
    typedef BASE        Base;
    typedef SMPtrPolicy PtrPolicy;
    typedef int         pseudo_exit;

    // default: no flag
    typedef ::boost::mpl::vector0<>  flag_list;
    typedef ::boost::mpl::vector0<>  internal_flag_list;
    //default: no deferred events
    typedef ::boost::mpl::vector0<>  deferred_events;
};

}}}

#endif //BOOST_MSM_FRONT_STATES_H

