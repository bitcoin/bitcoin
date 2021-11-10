// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_INTERNAL_ROW_HPP
#define BOOST_MSM_INTERNAL_ROW_HPP

#include <boost/type_traits/is_base_of.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/msm/back/common_types.hpp>
#include <boost/msm/row_tags.hpp>
#include <boost/msm/front/detail/row2_helper.hpp>

namespace boost { namespace msm { namespace front
{
    template<
        class Event
        , typename CalledForAction
        , void (CalledForAction::*action)(Event const&)
    >
    struct a_internal
    {
        typedef sm_a_i_row_tag row_type_tag;
        typedef Event Evt;
        template <class FSM,class SourceState,class TargetState,class AllStates>
        static ::boost::msm::back::HandledEnum action_call(FSM& fsm,Event const& evt,SourceState& src,TargetState& tgt, 
                                AllStates& all_states)
        {
            // in this front-end, we don't need to know source and target states
            ::boost::msm::front::detail::row2_action_helper<CalledForAction,Event,action>::call_helper
                (fsm,evt,src,tgt,all_states,
                ::boost::mpl::bool_< ::boost::is_base_of<CalledForAction,FSM>::type::value>());
            return ::boost::msm::back::HANDLED_TRUE;
        }
    };

    template<
        class Event
        , typename CalledForAction
        , void (CalledForAction::*action)(Event const&)
        , typename CalledForGuard
        , bool (CalledForGuard::*guard)(Event const&)
    >
    struct internal
    {
        typedef sm_i_row_tag row_type_tag;
        typedef Event Evt;
        template <class FSM,class SourceState,class TargetState,class AllStates>
        static ::boost::msm::back::HandledEnum action_call(FSM& fsm,Event const& evt,SourceState& src,TargetState& tgt, 
                                AllStates& all_states)
        {
            // in this front-end, we don't need to know source and target states
            ::boost::msm::front::detail::row2_action_helper<CalledForAction,Event,action>::call_helper
                (fsm,evt,src,tgt,all_states,
                ::boost::mpl::bool_< ::boost::is_base_of<CalledForAction,FSM>::type::value>());
            return ::boost::msm::back::HANDLED_TRUE;
        }
        template <class FSM,class SourceState,class TargetState,class AllStates>
        static bool guard_call(FSM& fsm,Event const& evt,SourceState& src,TargetState& tgt,
                               AllStates& all_states)
        {
            // in this front-end, we don't need to know source and target states
            return ::boost::msm::front::detail::row2_guard_helper<CalledForGuard,Event,guard>::call_helper
                (fsm,evt,src,tgt,all_states,
                ::boost::mpl::bool_< ::boost::is_base_of<CalledForGuard,FSM>::type::value>());
        }
    };
    template<
        class Event
        , typename CalledForGuard
        , bool (CalledForGuard::*guard)(Event const&)
    >
    struct g_internal
    {
        typedef sm_g_i_row_tag row_type_tag;
        typedef Event Evt;
        template <class FSM,class SourceState,class TargetState,class AllStates>
        static bool guard_call(FSM& fsm,Event const& evt,SourceState& src,TargetState& tgt,
                               AllStates& all_states)
        {
            // in this front-end, we don't need to know source and target states
            return ::boost::msm::front::detail::row2_guard_helper<CalledForGuard,Event,guard>::call_helper
                (fsm,evt,src,tgt,all_states,
                ::boost::mpl::bool_< ::boost::is_base_of<CalledForGuard,FSM>::type::value>());
        }
    };
    template<
        class Event
    >
    struct _internal
    {
        typedef sm__i_row_tag row_type_tag;
        typedef Event Evt;
    };
}}}

#endif //BOOST_MSM_INTERNAL_ROW_HPP

