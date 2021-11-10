// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_COMMON_H
#define BOOST_MSM_FRONT_EUML_COMMON_H

#include <boost/config.hpp>
#include <boost/msm/proto_config.hpp>

#include <iterator>
#include <utility>

#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/add_const.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/less_equal.hpp>

#if BOOST_VERSION >= 104000
#include <boost/mpl/string.hpp>
#endif

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/make_map.hpp>
#include <boost/fusion/include/pair.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/include/pair.hpp>
#include <boost/fusion/include/is_sequence.hpp>

#include <boost/type_traits/remove_reference.hpp>

#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/control/expr_if.hpp> 
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/less.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/cat.hpp> 

#include <boost/msm/msm_grammar.hpp>
#include <boost/msm/active_state_switching_policies.hpp>
#include <boost/msm/event_traits.hpp>
#include <boost/msm/front/functor_row.hpp>

namespace proto = boost::proto;

BOOST_MPL_HAS_XXX_TRAIT_DEF(tag_type)
BOOST_MPL_HAS_XXX_TRAIT_DEF(action_name)
BOOST_MPL_HAS_XXX_TRAIT_DEF(not_intern_euml_state)

namespace boost { namespace msm { namespace front { namespace euml
{
template <class T>
struct get_iterator 
{
    typedef typename T::iterator type;
};
template <class T>
struct get_reverse_iterator 
{
    typedef typename T::reverse_iterator type;
};
template <class T>
struct get_reference 
{
    typedef typename T::reference type;
};
template <class T>
struct get_size_type 
{
    typedef typename T::size_type type;
};
template <class T>
struct get_value_type 
{
    typedef typename T::value_type type;
};
template <class T>
struct get_first_type 
{
    typedef typename T::first_type type;
};
template <class T>
struct get_second_type 
{
    typedef typename T::second_type type;
};
template <class T>
struct get_action_tag_type 
{
    typedef typename ::boost::mpl::has_key<
        typename T::tag_type,action_tag>::type type;
};
template <class T>
struct get_state_action_tag_type 
{
    typedef typename ::boost::mpl::has_key<
        typename T::tag_type,state_action_tag>::type type;
};

template <class T,class EVT,class FSM,class SourceState,class TargetState>
struct get_result_type 
{
    typedef typename T::template transition_action_result<EVT,FSM,SourceState,TargetState>::type type;
};
template <class T,class Event,class FSM,class STATE>
struct get_result_type2 
{
    typedef typename T::template state_action_result<Event,FSM,STATE>::type type;
};
template<class SEQ>
struct get_sequence
{
    typedef typename SEQ::sequence type;
};

template <class T>
struct get_attributes_type
{
    typedef typename T::attributes_type type;
};

template <class T>
struct get_euml_tag_type
{
    typedef typename T::euml_tag_type type;
};

template <class T,class Arg1=void,class Arg2=void,class Arg3=void,class Arg4=void,class Arg5=void
#ifdef BOOST_MSVC
,class Arg6=void
#endif
>
struct get_fct 
{
    typedef typename T::template In<Arg1,Arg2,Arg3,Arg4,Arg5
#ifdef BOOST_MSVC
,Arg6
#endif
>::type type;
};

// used to differentiate between different types of euml_state's
template <class T,class Enable=void>
struct get_state_name 
{
    typedef T type;
};

template <class T>
struct get_state_name<T,typename ::boost::enable_if<has_not_intern_euml_state<T> >::type>
{
    typedef typename T::In::type type;
};

template <class T>
struct get_action_name 
{
    typedef typename T::action_name type;
};
template <class T>
struct get_event_name 
{
    typedef typename T::event_name type;
};

template <class EVT>
struct euml_event: proto::extends<typename proto::terminal<event_tag>::type, EVT, boost::msm::sm_domain>
{
    typedef event_tag euml_tag_type;
    typedef EVT event_name;
    using proto::extends<typename proto::terminal<event_tag>::type, EVT, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef EVT type;
    };
};

template <class STATE>
struct euml_state_intern: proto::extends<typename proto::terminal< boost::msm::state_tag>::type, STATE, boost::msm::state_domain>
{
    typedef state_tag euml_tag_type;
    using proto::extends<typename proto::terminal<state_tag>::type, STATE, boost::msm::state_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
        ,class Arg6 
#endif
    >
    struct In
    {
        typedef STATE type;
    };
};
template <class STATE>
struct euml_state: proto::extends<typename proto::terminal< boost::msm::state_tag>::type, STATE, boost::msm::state_domain>
{
    typedef state_tag euml_tag_type;
    typedef int not_intern_euml_state;
    using proto::extends<typename proto::terminal<state_tag>::type, STATE, boost::msm::state_domain>::operator=;
    struct In
    {
        typedef STATE type;
    };
};
template <class ACTION>
struct euml_action: proto::extends<typename proto::terminal<action_tag>::type, ACTION, boost::msm::sm_domain>
{
    typedef action_tag euml_tag_type;
    typedef ACTION action_name;
    using proto::extends<typename proto::terminal<action_tag>::type, ACTION, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef ACTION type;
    };
};
template <class FLAG>
struct euml_flag: proto::extends<typename proto::terminal<flag_tag>::type, FLAG, boost::msm::sm_domain>
{
    typedef flag_tag euml_tag_type;
    using proto::extends<typename proto::terminal<flag_tag>::type, FLAG, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef FLAG type;
    };
};

template <class CONFIG>
struct euml_config: proto::extends<typename proto::terminal<config_tag>::type, CONFIG, boost::msm::sm_domain>
{
    typedef config_tag euml_tag_type;
    using proto::extends<typename proto::terminal<config_tag>::type, CONFIG, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef CONFIG type;
    };
};

struct No_Exception : euml_config<No_Exception>
{
    typedef int no_exception_thrown;
    No_Exception(){}
};
struct No_Msg_Queue : euml_config<No_Msg_Queue>
{
    typedef int no_message_queue;
    No_Msg_Queue(){}
};
struct Deferred_Events : euml_config<Deferred_Events>
{
    typedef int activate_deferred_events;
    Deferred_Events(){}
};
No_Exception const no_exception=No_Exception();
No_Msg_Queue const no_msg_queue=No_Msg_Queue();
Deferred_Events const deferred_events=Deferred_Events();

struct ActiveStateSwitchBeforeTransition : euml_config<ActiveStateSwitchBeforeTransition>
{
    typedef boost::msm::active_state_switch_before_transition active_state_switch_policy;
    ActiveStateSwitchBeforeTransition(){}
};
ActiveStateSwitchBeforeTransition const switch_active_before_transition = ActiveStateSwitchBeforeTransition();

struct ActiveStateSwitchAfterExit : euml_config<ActiveStateSwitchAfterExit>
{
    typedef boost::msm::active_state_switch_after_exit active_state_switch_policy;
    ActiveStateSwitchAfterExit(){}
};
ActiveStateSwitchAfterExit const switch_active_after_exit = ActiveStateSwitchAfterExit();

struct ActiveStateSwitchAfterAction : euml_config<ActiveStateSwitchAfterAction>
{
    typedef boost::msm::active_state_switch_after_transition_action active_state_switch_policy;
    ActiveStateSwitchAfterAction(){}
};
ActiveStateSwitchAfterAction const switch_active_after_action = ActiveStateSwitchAfterAction();


struct invalid_type{};
struct make_invalid_type
{
    typedef invalid_type type;
};

template <class ROW>
struct make_vector_one_row 
{
    typedef boost::mpl::vector<ROW> type;
};
template <class T>
T make_T(T t) {return t;}

struct make_vector_no_row 
{
    typedef boost::mpl::vector0<> type;
};

struct NoAction : euml_action<NoAction>
{
    NoAction(){}
    // return value if used inside a state action (entry/exit)
    template <class Event,class FSM,class STATE >
    struct state_action_result
    {
        typedef bool type;
    };
    // return value if used inside a transition (action/guard)
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result
    {
        typedef bool type;
    };
    // this functor can be used in both modes, state action and transition action
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    bool operator()(Event const&,FSM&,STATE& )
    {
        // does nothing
        return true;
    }
    template <class EVT,class FSM,class SourceState,class TargetState>
    bool operator()(EVT const& ,FSM& ,SourceState& ,TargetState&)const
    {
        // does nothing
        return true;
    }
};
NoAction const no_action = NoAction();

struct fsm_artefact_tag {};
template <class Index=void>
struct GetSource_ : euml_action<GetSource_<Index> >
{
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::fusion::result_of::at_key<typename SourceState::attributes_type,
                                                        Index >::type type;
    };
    typedef ::boost::mpl::set<action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& ,FSM& ,SourceState& src,TargetState&)const
    {
        return src.get_attribute(Index());
    }
};
template<>
struct GetSource_<void> : euml_action<GetSource_<void> >
{
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef SourceState& type;
    };
    typedef ::boost::mpl::set<action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
     operator()(EVT const& , FSM&,SourceState& src,TargetState& )const
    {
        return src;
    }
};
struct GetSource_Helper: proto::extends< proto::terminal<fsm_artefact_tag>::type, GetSource_Helper, boost::msm::sm_domain>
{
    GetSource_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef GetSource_<Arg1> type;
    };
};
GetSource_Helper const source_ = GetSource_Helper();

template <class Index=void>
struct GetTarget_ : euml_action<GetTarget_<Index> >
{
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::fusion::result_of::at_key<typename TargetState::attributes_type,
                                                        Index >::type type;
    };
    typedef ::boost::mpl::set<action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& ,FSM& ,SourceState& ,TargetState& tgt)const
    {
        return tgt.get_attribute(Index());
    }
};
template<>
struct GetTarget_<void> : euml_action<GetTarget_<void> >
{
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef TargetState& type;
    };
    typedef ::boost::mpl::set<action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
     operator()(EVT const& , FSM&,SourceState& ,TargetState& tgt)const
    {
        return tgt;
    }
};
struct GetTarget_Helper: proto::extends< proto::terminal<fsm_artefact_tag>::type, GetTarget_Helper, boost::msm::sm_domain>
{
    GetTarget_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef GetTarget_<Arg1> type;
    };
};
GetTarget_Helper const target_ = GetTarget_Helper();

template <class Index=void>
struct GetState_ : euml_action<GetState_<Index> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::fusion::result_of::at_key<typename STATE::attributes_type,
                                                        Index >::type type;
    };
    typedef ::boost::mpl::set<state_action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const&,FSM& ,STATE& state )
    {
        return state.get_attribute(Index());
    }
};
template<>
struct GetState_<void> : euml_action<GetState_<void> >
{
    using euml_action<GetState_ >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef STATE& type;
    };
    typedef ::boost::mpl::set<state_action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
     operator()(Event const&,FSM& ,STATE& state )
    {
        return state;
    }
};
struct GetState_Helper: proto::extends< proto::terminal<fsm_artefact_tag>::type, GetState_Helper, boost::msm::sm_domain>
{
    GetState_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef GetState_<Arg1> type;
    };
};
GetState_Helper const state_ = GetState_Helper();

template <class Index=void>
struct GetEvent_ : euml_action<GetEvent_<Index> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::add_const<
            typename ::boost::fusion::result_of::at_key<typename Event::attributes_type,
                                                        Index >::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::add_const<
            typename ::boost::fusion::result_of::at_key<typename EVT::attributes_type,
                                                        Index >::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& ,STATE& )
    {
        return evt.get_attribute(Index());
    }
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& evt ,FSM& ,SourceState& ,TargetState&)const
    {
        return evt.get_attribute(Index());
    }
};
template <>
struct GetEvent_<void> : euml_action<GetEvent_<void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef Event const& type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef EVT const& type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& ,STATE& )
    {
        return evt;
    }
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& evt ,FSM& ,SourceState& ,TargetState&)const
    {
        return evt;
    }
};
struct GetEvent_Helper: proto::extends< proto::terminal<fsm_artefact_tag>::type, GetEvent_Helper, boost::msm::sm_domain>
{
    GetEvent_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef GetEvent_<Arg1> type;
    };
};
GetEvent_Helper const event_ = GetEvent_Helper();

template <class Index=void>
struct GetFsm_ : euml_action<GetFsm_<Index> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::fusion::result_of::at_key<typename FSM::attributes_type,
                                                        Index >::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::fusion::result_of::at_key<typename FSM::attributes_type,
                                                        Index >::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const&,FSM& fsm,STATE& )
    {
        return fsm.get_attribute(Index());
    }
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState&)const
    {
        return fsm.get_attribute(Index());
    }
};
template<>
struct GetFsm_<void> : euml_action<GetFsm_<void> >
{
    using euml_action<GetFsm_>::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef FSM& type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef FSM& type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const&,FSM& fsm,STATE& )
    {
        return fsm;
    }
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState&)const
    {
        return fsm;
    }
};
struct GetFsm_Helper: proto::extends< proto::terminal<fsm_artefact_tag>::type, GetFsm_Helper, boost::msm::sm_domain>
{
    GetFsm_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef GetFsm_<Arg1> type;
    };
};
GetFsm_Helper const fsm_ = GetFsm_Helper();

template <class StateName,class Param1>
struct SubState_ : euml_action<SubState_<StateName, Param1> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef StateName& type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef StateName& type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    StateName& operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Param1()(evt,fsm,src,tgt)).template get_state<StateName&>();
    }
    template <class Event,class FSM,class STATE>
    StateName& operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        return (Param1()(evt,fsm,state)).template get_state<StateName&>();
    }
};
template <class StateName>
struct SubState_ <StateName,void>
    : euml_action<SubState_<StateName, void > >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef StateName& type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef StateName& type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    StateName& operator()(EVT const&,FSM& fsm,SourceState& ,TargetState& )const
    {
        return fsm.template get_state<StateName&>();
    }
    template <class Event,class FSM,class STATE>
    StateName& operator()(Event const& ,FSM& fsm,STATE&  )const
    {
        return fsm.template get_state<StateName&>();
    }
};

struct SubState_Helper: proto::extends< proto::terminal<fsm_artefact_tag>::type, SubState_Helper, boost::msm::sm_domain>
{
    SubState_Helper(){}
    using proto::extends< proto::terminal<fsm_artefact_tag>::type, SubState_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef SubState_<Arg1,Arg2> type;
    };
};
SubState_Helper const substate_ = SubState_Helper();

template <class Target,class Index>
struct GetAttribute_  : euml_action<GetAttribute_<Target, Index> >
{
    using euml_action<GetAttribute_<Target,Index> >::operator=;
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename 
            ::boost::fusion::result_of::at_key<
            typename get_attributes_type<
                    typename ::boost::remove_reference<
                                typename get_result_type2<Target,Event,FSM,STATE>::type>::type>::type,
                    Index >::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename 
            ::boost::fusion::result_of::at_key<
            typename get_attributes_type<
                    typename ::boost::remove_reference<
                        typename get_result_type<Target,EVT,FSM,SourceState,TargetState>::type>::type>::type,
                    Index >::type type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Target::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Target()(evt,fsm,src,tgt)).get_attribute(Index());
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Target::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Target()(evt,fsm,state)).get_attribute(Index());
    }
};

struct GetAttribute_Helper: proto::extends< proto::terminal<fsm_artefact_tag>::type, GetAttribute_Helper, boost::msm::sm_domain>
{
    GetAttribute_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef GetAttribute_<Arg1,Arg2> type;
    };
};
GetAttribute_Helper const attribute_ = GetAttribute_Helper();

template <class Index>
struct Source_ : euml_action<Source_<Index> >
{
    using euml_action<Source_<Index> >::operator=;
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename 
            ::boost::fusion::result_of::at_key<typename SourceState::attributes_type,
                                               Index >::type type;
    };
    typedef ::boost::mpl::set<action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& , FSM&,SourceState& src,TargetState& )const
    {
        return src.get_attribute(Index());
    }
};
template <class Index>
struct Target_ : euml_action<Target_<Index> >
{
    using euml_action<Target_<Index> >::operator=;
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename 
            ::boost::fusion::result_of::at_key<typename TargetState::attributes_type,
                                               Index >::type type;
    };
    typedef ::boost::mpl::set<action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& ,FSM& ,SourceState& ,TargetState& tgt)const
    {
        return tgt.get_attribute(Index());
    }
};
template <class Index>
struct State_ : euml_action<State_<Index> >
{
    using euml_action<State_<Index> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename 
            ::boost::fusion::result_of::at_key<typename STATE::attributes_type,
                                               Index >::type type;
    };
    typedef ::boost::mpl::set<state_action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const&,FSM& ,STATE& state )
    {
        return state.get_attribute(Index());
    }
};
template <class Index>
struct Event_ : euml_action<Event_<Index> >
{
    using euml_action<Event_<Index> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::add_const<
            typename ::boost::fusion::result_of::at_key<typename Event::attributes_type,
                                                        Index >::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::add_const<
            typename ::boost::fusion::result_of::at_key<typename EVT::attributes_type,
                                                        Index >::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& ,STATE& )
    {
        return evt.get_attribute(Index());
    }
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& evt ,FSM& ,SourceState& ,TargetState&)const
    {
        return evt.get_attribute(Index());
    }
};
template <class StateType,class Index>
struct State_Attribute_ : euml_action<State_Attribute_<StateType,Index> >
{
    using euml_action<State_Attribute_<StateType,Index> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename 
            ::boost::fusion::result_of::at_key<typename StateType::attributes_type,
                                               Index >::type type;
    };
    typedef ::boost::mpl::set<state_action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const&,FSM& fsm,STATE& )
    {
        return fsm.template get_state<StateType&>().get_attribute(Index());
    }
};

template <class Index>
struct Fsm_ : euml_action<Fsm_<Index> >
{
    using euml_action<Fsm_<Index> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename 
            ::boost::fusion::result_of::at_key<typename FSM::attributes_type,
                                               Index >::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename 
            ::boost::fusion::result_of::at_key<typename FSM::attributes_type,
                                               Index >::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const&,FSM& fsm,STATE& )
    {
        return fsm.get_attribute(Index());
    }
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        operator()(EVT const& ,FSM& fsm,SourceState& ,TargetState&)const
    {
        return fsm.get_attribute(Index());
    }
};

struct True_ : euml::euml_action<True_>
{
    using euml_action<True_>::operator=;
    True_(){}

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef bool type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef bool type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    bool operator()(EVT const&,FSM&,SourceState& ,TargetState& )
    {
        return true;
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const&,FSM&,STATE& )
    {
        return true;
    }
};
True_ const true_ = True_();

struct False_ : euml::euml_action<False_>
{
    using euml_action<False_>::operator=;
    False_(){}

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef bool type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef bool type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    bool operator()(EVT const&,FSM&,SourceState& ,TargetState& )
    {
        return false;
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const&,FSM&,STATE& )
    {
        return false;
    }
};
False_ const false_ = False_();

template <int Val>
struct Int_ : euml_action<Int_<Val> >
{
    using euml_action<Int_<Val> >::operator=;
    typedef ::boost::mpl::int_<Val> value_type;
    enum {value = Val};

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef int type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef int type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    int operator()(EVT const& , FSM& ,SourceState& ,TargetState& )
    {
        return Val;
    }
    template <class Event,class FSM,class STATE>
    int operator()(Event const& ,FSM& ,STATE& )
    {
        return Val;
    }
};

template <char Val>
struct Char_ : euml_action<Char_<Val> >
{
    using euml_action<Char_<Val> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef char type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef char type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    char operator()(EVT const& , FSM& ,SourceState& ,TargetState& )
    {
        return Val;
    }
    template <class Event,class FSM,class STATE>
    char operator()(Event const& ,FSM& ,STATE& )
    {
        return Val;
    }
};

template <size_t Val>
struct Size_t_ : euml_action<Size_t_<Val> >
{
    using euml_action<Size_t_<Val> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef size_t type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef size_t type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    size_t operator()(EVT const& , FSM& ,SourceState& ,TargetState& )
    {
        return Val;
    }
    template <class Event,class FSM,class STATE>
    size_t operator()(Event const& ,FSM& ,STATE& )
    {
        return Val;
    }
};

#if BOOST_VERSION >= 104000

template <class T>
struct String_ : euml_action<String_<T> >
{
    using euml_action<String_<T> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef char const* type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef char const* type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    char const* operator()(EVT const& , FSM& ,SourceState& ,TargetState& )
    {
        return ::boost::mpl::c_str<T>::value;
    }
    template <class Event,class FSM,class STATE>
    char const* operator()(Event const& ,FSM& ,STATE& )
    {
        return ::boost::mpl::c_str<T>::value;
    }
};
#endif


template <class T>
struct Predicate_ : euml_action<Predicate_<T> >
{
    using euml_action<Predicate_<T> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef T type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef T type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    T operator()(EVT const& , FSM& ,SourceState& ,TargetState& )
    {
        return T();
    }
    template <class Event,class FSM,class STATE>
    T operator()(Event const& ,FSM& ,STATE& )
    {
        return T();
    }
};

template <class ToProcessEvt,class Param1, class Param2, class Param3, class Param4>
struct Process_ : euml_action<Process_<ToProcessEvt, Param1, Param2, Param3, Param4> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
        (Param2()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
        (Param3()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
        (Param4()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(ToProcessEvt());
        (Param2()(evt,fsm,state)).process_event(ToProcessEvt());
        (Param3()(evt,fsm,state)).process_event(ToProcessEvt());
        (Param4()(evt,fsm,state)).process_event(ToProcessEvt());
    }
};
template <class ToProcessEvt>
struct Process_ <ToProcessEvt,void,void,void,void>
    : euml_action<Process_<ToProcessEvt, void, void, void, void > >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const&,FSM& fsm,SourceState& ,TargetState& )const
    {
        fsm.process_event(ToProcessEvt());
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& ,FSM& fsm,STATE&  )const
    {
        fsm.process_event(ToProcessEvt());
    }
};

template <class ToProcessEvt,class Param1>
struct Process_ <ToProcessEvt,Param1,void,void,void>
    : euml_action<Process_<ToProcessEvt, Param1, void, void, void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(ToProcessEvt());
    }
};

template <class ToProcessEvt,class Param1, class Param2>
struct Process_ <ToProcessEvt,Param1,Param2,void,void>
    : euml_action<Process_<ToProcessEvt, Param1, Param2, void, void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
        (Param2()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(ToProcessEvt());
        (Param2()(evt,fsm,state)).process_event(ToProcessEvt());
    }
};

template <class ToProcessEvt,class Param1, class Param2, class Param3>
struct Process_ <ToProcessEvt,Param1,Param2,Param3,void>
    : euml_action<Process_<ToProcessEvt, Param1, Param2, Param3, void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
        (Param2()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
        (Param3()(evt,fsm,src,tgt)).process_event(ToProcessEvt());
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(ToProcessEvt());
        (Param2()(evt,fsm,state)).process_event(ToProcessEvt());
        (Param3()(evt,fsm,state)).process_event(ToProcessEvt());
    }
};

//version for process_(event_)
template <>
struct Process_ <GetEvent_<void>,void,void,void,void>
    : euml_action<Process_<GetEvent_<void>, void, void, void, void > >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& )const
    {
        fsm.process_event(evt);
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  )const
    {
        fsm.process_event(evt);
    }
};

struct process_tag {};
struct Process_Helper: proto::extends< proto::terminal<process_tag>::type, Process_Helper, boost::msm::sm_domain>
{
    Process_Helper(){}
    using proto::extends< proto::terminal<process_tag>::type, Process_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Process_<Arg1,Arg2,Arg3,Arg4,Arg5> type;
    };
};
Process_Helper const process_ = Process_Helper();

template <class Param1, class Param2, class Param3, class Param4>
struct Reprocess_ : euml_action<Reprocess_<Param1, Param2, Param3, Param4> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(evt);
        (Param2()(evt,fsm,src,tgt)).process_event(evt);
        (Param3()(evt,fsm,src,tgt)).process_event(evt);
        (Param4()(evt,fsm,src,tgt)).process_event(evt);
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(evt);
        (Param2()(evt,fsm,state)).process_event(evt);
        (Param3()(evt,fsm,state)).process_event(evt);
        (Param4()(evt,fsm,state)).process_event(evt);
    }
};
template <>
struct Reprocess_ <void,void,void,void>
    : euml_action<Reprocess_<void, void, void, void > >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& )const
    {
        fsm.process_event(evt);
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  )const
    {
        fsm.process_event(evt);
    }
};

template <class Param1>
struct Reprocess_ <Param1,void,void,void>
    : euml_action<Reprocess_<Param1, void, void, void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(evt);
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(evt);
    }
};

template <class Param1, class Param2>
struct Reprocess_ <Param1,Param2,void,void>
    : euml_action<Reprocess_<Param1, Param2, void, void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(evt);
        (Param2()(evt,fsm,src,tgt)).process_event(evt);
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(evt);
        (Param2()(evt,fsm,state)).process_event(evt);
    }
};

template <class Param1, class Param2, class Param3>
struct Reprocess_ <Param1,Param2,Param3,void>
    : euml_action<Reprocess_<Param1, Param2, Param3, void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(evt);
        (Param2()(evt,fsm,src,tgt)).process_event(evt);
        (Param3()(evt,fsm,src,tgt)).process_event(evt);
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(evt);
        (Param2()(evt,fsm,state)).process_event(evt);
        (Param3()(evt,fsm,state)).process_event(evt);
    }
};
struct reprocess_tag {};
struct Reprocess_Helper: proto::extends< proto::terminal<reprocess_tag>::type, Reprocess_Helper, boost::msm::sm_domain>
{
    Reprocess_Helper(){}
    using proto::extends< proto::terminal<reprocess_tag>::type, Reprocess_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Reprocess_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
Reprocess_Helper const reprocess_ = Reprocess_Helper();

template <class ToProcessEvt,class Value,class Param1, class Param2, class Param3>
struct Process2_ : euml_action<Process2_<ToProcessEvt,Value, Param1, Param2, Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(ToProcessEvt(Value()(evt,fsm,src,tgt)));
        (Param2()(evt,fsm,src,tgt)).process_event(ToProcessEvt(Value()(evt,fsm,src,tgt)));
        (Param3()(evt,fsm,src,tgt)).process_event(ToProcessEvt(Value()(evt,fsm,src,tgt)));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(ToProcessEvt(Value()(evt,fsm,state)));
        (Param2()(evt,fsm,state)).process_event(ToProcessEvt(Value()(evt,fsm,state)));
        (Param3()(evt,fsm,state)).process_event(ToProcessEvt(Value()(evt,fsm,state)));
    }
};

template <class ToProcessEvt,class Value>
struct Process2_ <ToProcessEvt,Value,void,void,void>
    : euml_action<Process2_<ToProcessEvt,Value, void, void, void > >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        fsm.process_event(ToProcessEvt(Value()(evt,fsm,src,tgt)));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        fsm.process_event(ToProcessEvt(Value()(evt,fsm,state)));
    }
};

template <class ToProcessEvt,class Value,class Param1>
struct Process2_ <ToProcessEvt,Value,Param1,void,void>
    : euml_action<Process2_<ToProcessEvt,Value, Param1, void, void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(ToProcessEvt(Value()(evt,fsm,src,tgt)));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(ToProcessEvt(Value()(evt,fsm,state)));
    }
};

template <class ToProcessEvt,class Value,class Param1, class Param2>
struct Process2_ <ToProcessEvt,Value,Param1,Param2,void>
    : euml_action<Process2_<ToProcessEvt,Value, Param1, Param2, void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef void type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef void type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Param1()(evt,fsm,src,tgt)).process_event(ToProcessEvt(Value()(evt,fsm,src,tgt)));
        (Param2()(evt,fsm,src,tgt)).process_event(ToProcessEvt(Value()(evt,fsm,src,tgt)));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        (Param1()(evt,fsm,state)).process_event(ToProcessEvt(Value()(evt,fsm,state)));
        (Param2()(evt,fsm,state)).process_event(ToProcessEvt(Value()(evt,fsm,state)));
    }
};

struct process2_tag {};
struct Process2_Helper : proto::extends< proto::terminal<process2_tag>::type, Process2_Helper, boost::msm::sm_domain>
{
    Process2_Helper(){}
    using proto::extends< proto::terminal<process2_tag>::type, Process2_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Process2_<Arg1,Arg2,Arg3,Arg4,Arg5> type;
    };
};
Process2_Helper const process2_ = Process2_Helper();

template <class Flag,class Param1=void, class Enable=void >                                             
struct Get_Flag_ : euml_action<Get_Flag_<Flag,Param1,Enable> > {};        

template <class Flag,class Param1>
struct Get_Flag_ <Flag,Param1
    , typename ::boost::enable_if<typename ::boost::is_same<Param1,void>::type >::type>
    : euml_action<Get_Flag_<Flag, Param1> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef bool type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef bool type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    bool operator()(EVT const&,FSM& fsm,SourceState& ,TargetState& )const
    {
        return fsm.template is_flag_active<Flag>();
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& ,FSM& fsm,STATE&  )const
    {
        return fsm.template is_flag_active<Flag>();
    }
};

template <class Flag,class Param1>
struct Get_Flag_ <Flag,Param1
    , typename ::boost::disable_if<
            typename ::boost::is_same<Param1,void>::type
            >::type>
    : euml_action<Get_Flag_<Flag, Param1> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef bool type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef bool type;
    };

    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    bool operator()(EVT const& evt,FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Param1()(evt,fsm,src,tgt)).template is_flag_active<Flag>();
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE&  state)const
    {
        return (Param1()(evt,fsm,state)).template is_flag_active<Flag>();
    }
};

struct Get_Flag_Helper: proto::extends< proto::terminal<fsm_artefact_tag>::type, Get_Flag_Helper, boost::msm::sm_domain>
{
    Get_Flag_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Get_Flag_<Arg1,Arg2> type;
    };
};
Get_Flag_Helper const is_flag_ = Get_Flag_Helper();

// deferring an event
struct DeferEvent_ : euml_action< DeferEvent_ >
{
    typedef ::boost::mpl::set<action_tag> tag_type;
    // mark as deferring to avoid stack overflows in certain conditions
    typedef int deferring_action;
    template <class EVT,class FSM,class SourceState,class TargetState>
    void operator()(EVT const& evt,FSM& fsm,SourceState& ,TargetState& ) const
    {
        fsm.defer_event(evt);
    }
};
struct Defer_Helper : proto::extends< proto::terminal<fsm_artefact_tag>::type, Defer_Helper, boost::msm::sm_domain>
{
    Defer_Helper(){}
    using proto::extends< proto::terminal<fsm_artefact_tag>::type, Defer_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef DeferEvent_ type;
    };
};
Defer_Helper const defer_ = Defer_Helper();

struct explicit_tag {};
struct Explicit_Helper : proto::extends< proto::terminal<explicit_tag>::type, Explicit_Helper, boost::msm::sm_domain>
{
    Explicit_Helper(){}
    using proto::extends< proto::terminal<explicit_tag>::type, Explicit_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef typename Arg1::template direct<Arg2> type;
    };
};
Explicit_Helper const explicit_ = Explicit_Helper();

struct entry_pt_tag {};
struct Entry_Pt_Helper : proto::extends< proto::terminal<entry_pt_tag>::type, Entry_Pt_Helper, boost::msm::sm_domain>
{
    Entry_Pt_Helper(){}
    using proto::extends< proto::terminal<entry_pt_tag>::type, Entry_Pt_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef typename Arg1::template entry_pt<Arg2> type;
    };
};
Entry_Pt_Helper const entry_pt_ = Entry_Pt_Helper();

struct exit_pt_tag {};
struct Exit_Pt_Helper : proto::extends< proto::terminal<exit_pt_tag>::type, Exit_Pt_Helper, boost::msm::sm_domain>
{
    Exit_Pt_Helper(){}
    using proto::extends< proto::terminal<exit_pt_tag>::type, Exit_Pt_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef typename Arg1::template exit_pt<Arg2> type;
    };
};
Exit_Pt_Helper const exit_pt_ = Exit_Pt_Helper();

#ifdef BOOST_MSVC
#define BOOST_MSM_EUML_FUNCTION(functor,function,function_name,result_trans,result_state)               \
    template <class Param1=void , class Param2=void , class Param3=void , class Param4=void,            \
    class Param5=void,class Param6=void,class Enable=void >                                             \
    struct functor : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6,Enable> > {};        \
    template <class Param1, class Param2, class Param3, class Param4, class Param5, class Param6>       \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::enable_if<typename ::boost::is_same<Param1,void>::type>::type>                \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type                        \
        operator()(EVT const& , FSM& ,SourceState& ,TargetState& )const {                               \
        return function ();}                                                                            \
        template <class Event,class FSM,class STATE>                                                    \
        typename state_action_result<Event,FSM,STATE>::type                                             \
        operator()(Event const& ,FSM& ,STATE&  )const {                                                 \
        return function ();} };                                                                         \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param1,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param2,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt));}                                                   \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state));} };                                                  \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param2,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param3,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));}                         \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state));} };                          \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param3,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param4,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));} \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));} };  \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param4,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param5,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt)  \
        ,Param4()(evt,fsm,src,tgt));}                                                                   \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state)        \
        ,Param4()(evt,fsm,state));} };                                                                  \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param5,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param6,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt)  \
        ,Param4()(evt,fsm,src,tgt),Param5()(evt,fsm,src,tgt));}                                         \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state)        \
        ,Param4()(evt,fsm,state),Param5()(evt,fsm,state));} };                                          \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::is_same<Param6,void>::type>::type>               \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt)  \
        ,Param4()(evt,fsm,src,tgt),Param5()(evt,fsm,src,tgt),Param6()(evt,fsm,src,tgt));}               \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state)        \
        ,Param4()(evt,fsm,state),Param5()(evt,fsm,state),Param6()(evt,fsm,state));} };                  \
        struct function_name ## tag{};                                                                  \
        struct functor ## Helper : proto::extends< proto::terminal< function_name ## tag >::type,       \
        functor ## Helper , boost::msm::sm_domain> { functor ## Helper(){}                              \
        template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5,class Arg6>                    \
        struct In {typedef functor <Arg1,Arg2,Arg3,Arg4,Arg5,Arg6> type;}; };                           \
        functor ## Helper const function_name = functor ## Helper ();

#define BOOST_MSM_EUML_METHOD(functor,function,function_name,result_trans,result_state)                 \
    template <class Param1=void , class Param2=void , class Param3=void , class Param4=void,            \
    class Param5=void,class Param6=void,class Enable=void >                                             \
    struct functor : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6,Enable> > {};        \
    template <class Param1, class Param2, class Param3, class Param4, class Param5, class Param6>       \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::enable_if<typename ::boost::is_same<Param2,void>::type>::type>                \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function();}                                                 \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function();} };                                                \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param2,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param3,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt));}                        \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state));} };                         \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param3,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param4,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));} \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state),Param3()(evt,fsm,state));} }; \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param4,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param5,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt) \
        ,Param4()(evt,fsm,src,tgt));}                                                                   \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state),Param3()(evt,fsm,state)       \
        ,Param4()(evt,fsm,state));} };                                                                  \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param5,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param6,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt) \
        ,Param4()(evt,fsm,src,tgt),Param5()(evt,fsm,src,tgt));}                                         \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state),Param3()(evt,fsm,state)       \
        ,Param4()(evt,fsm,state),Param5()(evt,fsm,state));} };                                          \
    template <class Param1, class Param2, class Param3, class Param4, class Param5,class Param6>        \
    struct functor<Param1,Param2,Param3,Param4,Param5,Param6,                                           \
        typename ::boost::disable_if<typename ::boost::is_same<Param6,void>::type>::type>               \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Param6> > {                            \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt) \
        ,Param4()(evt,fsm,src,tgt),Param5()(evt,fsm,src,tgt),Param6()(evt,fsm,src,tgt));}               \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state),Param3()(evt,fsm,state)       \
        ,Param4()(evt,fsm,state),Param5()(evt,fsm,state),Param6()(evt,fsm,state));} };                  \
        struct function_name ## tag{};                                                                  \
        struct functor ## Helper : proto::extends< proto::terminal< function_name ## tag >::type,       \
        functor ## Helper , boost::msm::sm_domain> { functor ## Helper(){}                              \
        template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5,class Arg6>                    \
        struct In {typedef functor <Arg1,Arg2,Arg3,Arg4,Arg5,Arg6> type;}; };                           \
        functor ## Helper const function_name = functor ## Helper ();

#else

#define BOOST_MSM_EUML_FUNCTION(functor,function,function_name,result_trans,result_state)               \
    template <class Param1=void , class Param2=void , class Param3=void , class Param4=void,            \
    class Param5=void,class Enable=void >                                                               \
    struct functor : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Enable> > {};               \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param1,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param2,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt));}                                                   \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state));} };                                                  \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param2,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param3,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));}                         \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state));} };                          \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param3,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param4,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));} \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));} };  \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param4,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param5,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt)  \
        ,Param4()(evt,fsm,src,tgt));}                                                                   \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state)        \
        ,Param4()(evt,fsm,state));} };                                                                  \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::is_same<Param5,void>::type>::type>               \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return function (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt)  \
        ,Param4()(evt,fsm,src,tgt),Param5()(evt,fsm,src,tgt));}                                         \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return function (Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state)        \
        ,Param4()(evt,fsm,state),Param5()(evt,fsm,state));} };                                          \
        struct function_name ## tag{};                                                                  \
        struct functor ## Helper : proto::extends< proto::terminal< function_name ## tag >::type,       \
        functor ## Helper , boost::msm::sm_domain> { functor ## Helper(){}                              \
        template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5>                               \
        struct In {typedef functor <Arg1,Arg2,Arg3,Arg4,Arg5> type;}; };                                \
        functor ## Helper const function_name = functor ## Helper ();

#define BOOST_MSM_EUML_METHOD(functor,function,function_name,result_trans,result_state)                 \
    template <class Param1=void , class Param2=void , class Param3=void , class Param4=void,            \
    class Param5=void,class Enable=void >                                                               \
    struct functor : euml_action<functor<Param1,Param2,Param3,Param4,Param5,Enable> > {};               \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::enable_if<typename ::boost::is_same<Param2,void>::type>::type>                \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function();}                                                 \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function();} };                                                \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param2,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param3,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt));}                        \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state));} };                         \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param3,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param4,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));} \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state),Param3()(evt,fsm,state));} }; \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::mpl::or_<                                        \
        typename ::boost::is_same<Param4,void>::type,typename ::boost::mpl::not_<                       \
        typename ::boost::is_same<Param5,void>::type>::type>::type >::type>                             \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt) \
        ,Param4()(evt,fsm,src,tgt));}                                                                   \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state),Param3()(evt,fsm,state)       \
        ,Param4()(evt,fsm,state));} };                                                                  \
    template <class Param1, class Param2, class Param3, class Param4, class Param5>                     \
    struct functor<Param1,Param2,Param3,Param4,Param5,                                                  \
        typename ::boost::disable_if<typename ::boost::is_same<Param5,void>::type>::type>               \
        : euml_action<functor<Param1,Param2,Param3,Param4,Param5> > {                                   \
        template <class Event,class FSM,class STATE > struct state_action_result {                      \
        typedef result_state type;} ;                                                                   \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        struct transition_action_result { typedef result_trans type;};                                  \
        typedef ::boost::mpl::set<boost::msm::front::state_action_tag,boost::msm::front::action_tag> tag_type; \
        template <class EVT,class FSM,class SourceState,class TargetState>                              \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::action_tag>::type,                                 \
        typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type                \
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const {                   \
        return (Param1()(evt,fsm,src,tgt)).function(Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt) \
        ,Param4()(evt,fsm,src,tgt),Param5()(evt,fsm,src,tgt));}                                         \
        template <class Event,class FSM,class STATE>                                                    \
        typename ::boost::enable_if<typename ::boost::mpl::has_key<                                     \
        typename Param1::tag_type,boost::msm::front::state_action_tag>::type,                           \
        typename state_action_result<Event,FSM,STATE>::type >::type                                     \
        operator()(Event const& evt,FSM& fsm,STATE& state )const {                                      \
        return (Param1()(evt,fsm,state)).function(Param2()(evt,fsm,state),Param3()(evt,fsm,state)       \
        ,Param4()(evt,fsm,state),Param5()(evt,fsm,state));} };                                          \
        struct function_name ## tag{};                                                                  \
        struct functor ## Helper : proto::extends< proto::terminal< function_name ## tag >::type,       \
        functor ## Helper , boost::msm::sm_domain> { functor ## Helper(){}                              \
        template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5>                               \
        struct In {typedef functor <Arg1,Arg2,Arg3,Arg4,Arg5> type;}; };                                \
        functor ## Helper const function_name = functor ## Helper ();

#endif

#define RESULT_TYPE2_PARAM1 typename get_result_type2<Param1,Event,FSM,STATE>::type
#define RESULT_TYPE_PARAM1 typename get_result_type<Param1,EVT,FSM,SourceState,TargetState>::type 
#define RESULT_TYPE2_PARAM2 typename get_result_type2<Param2,Event,FSM,STATE>::type
#define RESULT_TYPE_PARAM2 typename get_result_type<Param2,EVT,FSM,SourceState,TargetState>::type 
#define RESULT_TYPE2_PARAM3 typename get_result_type2<Param3,Event,FSM,STATE>::type
#define RESULT_TYPE_PARAM3 typename get_result_type<Param3,EVT,FSM,SourceState,TargetState>::type 
#define RESULT_TYPE2_PARAM4 typename get_result_type2<Param4,Event,FSM,STATE>::type
#define RESULT_TYPE_PARAM4 typename get_result_type<Param4,EVT,FSM,SourceState,TargetState>::type 
#define RESULT_TYPE2_PARAM5 typename get_result_type2<Param5,Event,FSM,STATE>::type
#define RESULT_TYPE_PARAM5 typename get_result_type<Param5,EVT,FSM,SourceState,TargetState>::type 
#define RESULT_TYPE2_PARAM6 typename get_result_type2<Param6,Event,FSM,STATE>::type
#define RESULT_TYPE_PARAM6 typename get_result_type<Param6,EVT,FSM,SourceState,TargetState>::type 


#define RESULT_TYPE2_DIFF_TYPE_ITER_TRAITS_PARAM1 typename std::iterator_traits<typename get_result_type2<Param1,Event,FSM,STATE>::type>::difference_type
#define RESULT_TYPE_DIFF_TYPE_ITER_TRAITS_PARAM1 typename std::iterator_traits<typename get_result_type<Param1,EVT,FSM,SourceState,TargetState>::type>::difference_type 

#define RESULT_TYPE2_REMOVE_REF_PARAM1 typename ::boost::remove_reference<typename get_result_type2<Param1,Event,FSM,STATE>::type>::type
#define RESULT_TYPE_REMOVE_REF_PARAM1 typename ::boost::remove_reference<typename get_result_type<Param1,EVT,FSM,SourceState,TargetState>::type>::type 

#define RESULT_TYPE2_PAIR_REMOVE_REF_PARAM1 std::pair<RESULT_TYPE2_REMOVE_REF_PARAM1,RESULT_TYPE2_REMOVE_REF_PARAM1>
#define RESULT_TYPE_PAIR_REMOVE_REF_PARAM1 std::pair<RESULT_TYPE_REMOVE_REF_PARAM1,RESULT_TYPE_REMOVE_REF_PARAM1>

#define RESULT_TYPE2_GET_REF_REMOVE_REF_PARAM1 typename get_reference<typename ::boost::remove_reference<typename get_result_type2<Param1,Event,FSM,STATE>::type>::type>::type
#define RESULT_TYPE_GET_REF_REMOVE_REF_PARAM1 typename get_reference<typename ::boost::remove_reference<typename get_result_type<Param1,EVT,FSM,SourceState,TargetState>::type>::type>::type 

#define RESULT_TYPE2_GET_ITERATOR_REMOVE_REF_PARAM1 typename get_iterator<typename ::boost::remove_reference<typename get_result_type2<Param1,Event,FSM,STATE>::type>::type>::type
#define RESULT_TYPE_GET_ITERATOR_REMOVE_REF_PARAM1 typename get_iterator<typename ::boost::remove_reference<typename get_result_type<Param1,EVT,FSM,SourceState,TargetState>::type>::type>::type 

#define RESULT_TYPE2_GET_REV_ITERATOR_REMOVE_REF_PARAM1 typename get_reverse_iterator<typename ::boost::remove_reference<typename get_result_type2<Param1,Event,FSM,STATE>::type>::type>::type
#define RESULT_TYPE_GET_REV_ITERATOR_REMOVE_REF_PARAM1 typename get_reverse_iterator<typename ::boost::remove_reference<typename get_result_type<Param1,EVT,FSM,SourceState,TargetState>::type>::type>::type 

#define RESULT_TYPE2_GET_SIZE_TYPE_REMOVE_REF_PARAM1 typename get_size_type<typename ::boost::remove_reference<typename get_result_type2<Param1,Event,FSM,STATE>::type>::type>::type
#define RESULT_TYPE_GET_SIZE_TYPE_REMOVE_REF_PARAM1 typename get_size_type<typename ::boost::remove_reference<typename get_result_type<Param1,EVT,FSM,SourceState,TargetState>::type>::type>::type 

#define BOOST_MSM_EUML_ACTION(instance_name)                                                        \
    struct instance_name ## _impl;                                                              \
    struct instance_name ## _helper :  boost::msm::front::euml::euml_action<instance_name ## _impl>    \
    {                                                                                           \
        instance_name ## _helper(){}                                                            \
        typedef instance_name ## _impl action_name;                                             \
    };                                                                                          \
    static instance_name ## _helper instance_name;                                              \
    struct instance_name ## _impl : instance_name ## _helper

#define BOOST_MSM_EUML_DECLARE_ACTION(instance_name)                                                        \
    struct instance_name ;                                                                      \
    struct instance_name ## _helper :  boost::msm::front::euml::euml_action<instance_name >     \
    {                                                                                           \
        instance_name ## _helper(){}                                                            \
        typedef instance_name action_name;                                                      \
    };                                                                                          \
    struct instance_name : instance_name ## _helper


#define BOOST_MSM_EUML_EVENT(instance_name)                                                     \
    struct instance_name ## _helper : boost::msm::front::euml::euml_event<instance_name ## _helper>{   \
    instance_name ## _helper(){}                                                                \
    instance_name ## _helper const& operator()() const {return *this;} };                       \
    static instance_name ## _helper instance_name;

// an event matching any event
struct kleene_ : boost::msm::front::euml::euml_event<kleene_>, public boost::any
{
    kleene_() : boost::any(){}
    template<typename ValueType>
    kleene_(const ValueType & v) : boost::any(v){}
};
static kleene_ kleene;

#define BOOST_MSM_EUML_DECLARE_EVENT(instance_name)                                             \
    struct instance_name : boost::msm::front::euml::euml_event<instance_name >{                 \
    instance_name(){}                                                                           \
    instance_name const& operator()() const {return *this;} };

#define MSM_EUML_CONCAT(param1,param2) param1
#define MSM_EUML_EVENT_INSTANCE_HELPER_EXECUTE1(z, n, unused) ARG ## n arg ## n
#define MSM_EUML_EVENT_INSTANCE_HELPER_EXECUTE2(z, n, unused) arg ## n
#define MSM_EUML_EVENT_INSTANCE_HELPER_ATTRIBUTE_MAP_ENTRY(z, n, unused)                        \
    typename boost::fusion::result_of::first<                                                   \
            typename ::boost::remove_reference<                                                 \
            typename boost::fusion::result_of::at_c<T, BOOST_PP_CAT( , n)>::type>::type>::type  \

#define MSM_EUML_EVENT_HELPER_GET_ATTRIBUTE(z, n, unused)                                       \
    get_attribute(                                                                              \
        typename boost::fusion::result_of::first<                                               \
        typename ::boost::remove_reference<                                                     \
        typename boost::fusion::result_of::at_c<T, n>::type>::type>::type())=arg ## n;

#define MSM_EUML_EVENT_HELPER_CONSTRUCTORS(z, n, mytuple)                                       \
    template <BOOST_PP_ENUM_PARAMS(n, class ARG)>                                               \
    BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 0, mytuple) , _helper)(BOOST_PP_ENUM(n, MSM_EUML_EVENT_INSTANCE_HELPER_EXECUTE1, ~ )):    \
    BOOST_PP_TUPLE_ELEM(2, 1, mytuple)(){                                                       \
    init(BOOST_PP_ENUM(n, MSM_EUML_EVENT_INSTANCE_HELPER_EXECUTE2, ~ ),attribute_vec());}                                                                             

#define MSM_EUML_EVENT_INSTANCE_HELPER_ATTRIBUTE_MAP(z, n, unused)                              \
    template <BOOST_PP_ENUM_PARAMS(n, class ARG),class T>                                       \
    void init(BOOST_PP_ENUM(n, MSM_EUML_EVENT_INSTANCE_HELPER_EXECUTE1, ~ ), \
    T,typename ::boost::enable_if< typename boost::mpl::eval_if< typename ::boost::fusion::traits::is_sequence<T>::type,size_helper<T,n>,::boost::mpl::false_>::type,void >::type* =0)      \
    {                                                                                           \
        BOOST_PP_REPEAT_FROM_TO(0,n ,                                                           \
        MSM_EUML_EVENT_HELPER_GET_ATTRIBUTE, ~)                                                 \
    }                                                                             

#define MSM_EUML_EVENT_INSTANCE_HELPER_OPERATOR_IMPL(z, n, instance)                            \
    template <BOOST_PP_ENUM_PARAMS(n, class ARG)>                                               \
        BOOST_PP_CAT(instance,_helper) operator()                                               \
        (BOOST_PP_ENUM(n, MSM_EUML_EVENT_INSTANCE_HELPER_EXECUTE1, ~ ))const{                   \
        return BOOST_PP_CAT(instance,_helper) (BOOST_PP_ENUM(n, MSM_EUML_EVENT_INSTANCE_HELPER_EXECUTE2, ~ ));}
        
#if defined(FUSION_MAX_MAP_SIZE)

#define BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(instance_name, attributes_name)                    \
    struct instance_name ## _helper :                                                           \
        boost::msm::front::euml::euml_event<instance_name ## _helper> , public attributes_name  \
    {                                                                                           \
        template <class T,int checked_size> struct size_helper                                  \
        {                                                                                       \
            typedef typename ::boost::mpl::less_equal<                                          \
                typename ::boost::fusion::result_of::size<T>::type,                             \
                ::boost::mpl::int_<checked_size> >::type type;                                  \
        };                                                                                      \
        BOOST_PP_CAT(instance_name,_helper()) : attributes_name(){}                             \
        typedef attributes_name::attributes_type attribute_map;                                 \
        typedef ::boost::fusion::result_of::as_vector<attribute_map>::type attribute_vec;       \
        BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(FUSION_MAX_MAP_SIZE ,1),                         \
        MSM_EUML_EVENT_HELPER_CONSTRUCTORS, (instance_name,attributes_name))                    \
        BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(FUSION_MAX_MAP_SIZE ,1),                         \
        MSM_EUML_EVENT_INSTANCE_HELPER_ATTRIBUTE_MAP, ~)                                        \
        BOOST_PP_CAT(instance_name,_helper) operator()(){                                       \
        return BOOST_PP_CAT(instance_name,_helper)();}                                          \
        BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(FUSION_MAX_MAP_SIZE ,1),                         \
        MSM_EUML_EVENT_INSTANCE_HELPER_OPERATOR_IMPL, instance_name)                            \
    };                                                                                          \
    static instance_name ## _helper instance_name;

#else

#define BOOST_MSM_EUML_EVENT_WITH_ATTRIBUTES(instance_name, attributes_name)                    \
    struct instance_name ## _helper :                                                           \
        boost::msm::front::euml::euml_event<instance_name ## _helper> , public attributes_name  \
    {                                                                                           \
        template <class T,int checked_size> struct size_helper                                  \
        {                                                                                       \
            typedef typename ::boost::mpl::less_equal<                                          \
                typename ::boost::fusion::result_of::size<T>::type,                             \
                ::boost::mpl::int_<checked_size> >::type type;                                  \
        };                                                                                      \
        BOOST_PP_CAT(instance_name,_helper()) : attributes_name(){}                             \
        typedef attributes_name::attributes_type attribute_map;                                 \
        typedef ::boost::fusion::result_of::as_vector<attribute_map>::type attribute_vec;       \
        BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(10 ,1),                                          \
        MSM_EUML_EVENT_HELPER_CONSTRUCTORS, (instance_name,attributes_name))                    \
        BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(10 ,1),                                          \
        MSM_EUML_EVENT_INSTANCE_HELPER_ATTRIBUTE_MAP, ~)                                        \
        BOOST_PP_CAT(instance_name,_helper) operator()(){                                       \
        return BOOST_PP_CAT(instance_name,_helper)();}                                          \
        BOOST_PP_REPEAT_FROM_TO(1,BOOST_PP_ADD(10 ,1),                                          \
        MSM_EUML_EVENT_INSTANCE_HELPER_OPERATOR_IMPL, instance_name)                            \
    };                                                                                          \
    static instance_name ## _helper instance_name;

#endif

#define BOOST_MSM_EUML_EVENT_NAME(instance_name) instance_name ## _helper

#define BOOST_MSM_EUML_FLAG_NAME(instance_name) instance_name ## _helper

#define BOOST_MSM_EUML_FLAG(instance_name)                                                      \
    struct instance_name ## _helper : boost::msm::front::euml::euml_flag<instance_name ## _helper>{};  \
    static instance_name ## _helper instance_name;

#define BOOST_MSM_EUML_DECLARE_FLAG(instance_name)                                                      \
    struct instance_name : boost::msm::front::euml::euml_flag<instance_name >{};

#define BOOST_MSM_EUML_STATE_NAME(instance_name) instance_name ## _helper

#define BOOST_MSM_EUML_BUILD_STT_HELPER build_stt(
#define BOOST_MSM_EUML_BUILD_INTERNAL_STT_HELPER build_internal_stt(
#define BOOST_MSM_EUML_BUILD_STT_HELPER2(expr) expr)
#define BOOST_MSM_EUML_ENTRY_STATE_HELPER(expr) ,expr


#define BOOST_MSM_EUML_ATTRIBUTES(expr,instance_name)                                                   \
    typedef BOOST_TYPEOF(build_attributes expr) instance_name;

// following macros declare a state type but do not create an instance
#define BOOST_MSM_EUML_DECLARE_STATE(expr,instance_name)                                                    \
    struct instance_name ## tag{};                                                                          \
    typedef BOOST_TYPEOF(build_state<instance_name ## tag> expr) instance_name;

#define BOOST_MSM_EUML_DECLARE_INTERRUPT_STATE(expr,instance_name)                                                  \
    struct instance_name ## tag{};                                                                                  \
    typedef BOOST_TYPEOF(build_interrupt_state<instance_name ## tag> expr) instance_name;

#define BOOST_MSM_EUML_DECLARE_TERMINATE_STATE(expr,instance_name)                                          \
    struct instance_name ## tag{};                                                                          \
    typedef BOOST_TYPEOF(build_terminate_state<instance_name ## tag> expr) instance_name;

#define BOOST_MSM_EUML_DECLARE_EXPLICIT_ENTRY_STATE(region,expr,instance_name)                          \
    struct instance_name ## tag{};                                                                  \
    typedef BOOST_TYPEOF(build_explicit_entry_state<instance_name ## tag BOOST_MSM_EUML_ENTRY_STATE_HELPER(region) > expr) instance_name;

#define BOOST_MSM_EUML_DECLARE_ENTRY_STATE(region,expr,instance_name)                                       \
    struct instance_name ## tag{};                                                                  \
    typedef BOOST_TYPEOF(build_entry_state<instance_name ## tag BOOST_MSM_EUML_ENTRY_STATE_HELPER(region) > expr) instance_name;

#define BOOST_MSM_EUML_DECLARE_EXIT_STATE(expr,instance_name)                                               \
    struct instance_name ## tag{};                                                                          \
    typedef BOOST_TYPEOF(build_exit_state<instance_name ## tag> expr) instance_name;

#define BOOST_MSM_EUML_DECLARE_STATE_MACHINE(expr,instance_name)                                       \
    struct instance_name ## tag{};                                                                  \
    typedef BOOST_TYPEOF(build_sm<instance_name ## tag> expr) instance_name;

#define BOOST_MSM_EUML_DECLARE_TRANSITION_TABLE(expr,instance_name)                                     \
    typedef int using_declared_table;                                                                   \
    typedef BOOST_TYPEOF(BOOST_MSM_EUML_BUILD_STT_HELPER  BOOST_MSM_EUML_BUILD_STT_HELPER2(expr)) instance_name;

#define BOOST_MSM_EUML_DECLARE_INTERNAL_TRANSITION_TABLE(expr)                                                                     \
    typedef BOOST_TYPEOF(                                                                                                       \
    BOOST_MSM_EUML_BUILD_INTERNAL_STT_HELPER BOOST_MSM_EUML_BUILD_STT_HELPER2(expr)) internal_transition_table;

// following macros declare a state type and create an instance
#define BOOST_MSM_EUML_STATE(expr,instance_name)                                                        \
    struct instance_name ## tag{};                                                                      \
    typedef BOOST_TYPEOF(build_state<instance_name ## tag> expr) instance_name ## _helper;              \
    static instance_name ## _helper instance_name;

#define BOOST_MSM_EUML_INTERRUPT_STATE(expr,instance_name)                                              \
    struct instance_name ## tag{};                                                                      \
    typedef BOOST_TYPEOF(build_interrupt_state<instance_name ## tag> expr) instance_name ## _helper;    \
    static instance_name ## _helper instance_name;

#define BOOST_MSM_EUML_TERMINATE_STATE(expr,instance_name)                                              \
    struct instance_name ## tag{};                                                                      \
    typedef BOOST_TYPEOF(build_terminate_state<instance_name ## tag> expr) instance_name ## _helper;    \
    static instance_name ## _helper instance_name;

#define BOOST_MSM_EUML_EXPLICIT_ENTRY_STATE(region,expr,instance_name)                                  \
    struct instance_name ## tag{};                                                                      \
    typedef BOOST_TYPEOF(build_explicit_entry_state<instance_name ## tag BOOST_MSM_EUML_ENTRY_STATE_HELPER(region) > expr) instance_name ## _helper;  \
    static instance_name ## _helper instance_name;

#define BOOST_MSM_EUML_ENTRY_STATE(region,expr,instance_name)                                       \
    struct instance_name ## tag{};                                                                  \
    typedef BOOST_TYPEOF(build_entry_state<instance_name ## tag BOOST_MSM_EUML_ENTRY_STATE_HELPER(region) > expr) instance_name ## _helper;  \
    static instance_name ## _helper instance_name;

#define BOOST_MSM_EUML_EXIT_STATE(expr,instance_name)                                                       \
    struct instance_name ## tag{};                                                                          \
    typedef BOOST_TYPEOF(build_exit_state<instance_name ## tag> expr) instance_name ## _helper;             \
    static instance_name ## _helper instance_name;


#ifndef BOOST_MSVC

#define BOOST_MSM_EUML_TRANSITION_TABLE(expr,instance_name)                                     \
    typedef BOOST_TYPEOF(BOOST_MSM_EUML_BUILD_STT_HELPER BOOST_MSM_EUML_BUILD_STT_HELPER2(expr)) instance_name ## _def;                  \
    struct instance_name ## _helper :  public instance_name ## _def{instance_name ## _helper(){}};                          \
    static instance_name ## _helper instance_name;

#define BOOST_MSM_EUML_INTERNAL_TRANSITION_TABLE(expr,instance_name)                            \
    typedef BOOST_TYPEOF(BOOST_MSM_EUML_BUILD_INTERNAL_STT_HELPER BOOST_MSM_EUML_BUILD_STT_HELPER2(expr)) instance_name ## _def;         \
    struct instance_name ## _helper :  public instance_name ## _def{instance_name ## _helper(){}};   \
    static instance_name ## _helper instance_name;

#else

#define BOOST_MSM_EUML_TRANSITION_TABLE(expr,instance_name)                                     \
    struct instance_name ## _helper :                                                           \
    public BOOST_TYPEOF(BOOST_MSM_EUML_BUILD_STT_HELPER BOOST_MSM_EUML_BUILD_STT_HELPER2(expr)) \
    {instance_name ## _helper(){}} ;                                                            \
    static instance_name ## _helper instance_name;

#define BOOST_MSM_EUML_INTERNAL_TRANSITION_TABLE(expr,instance_name)                                        \
    struct instance_name ## _helper :                                                                       \
    public BOOST_TYPEOF(BOOST_MSM_EUML_BUILD_INTERNAL_STT_HELPER BOOST_MSM_EUML_BUILD_STT_HELPER2(expr))    \
    {instance_name ## _helper(){}} ;                                                                        \
    static instance_name ## _helper instance_name;

#endif

}}}} // boost::msm::front::euml

namespace boost { namespace msm{
    template<> 
    struct is_kleene_event< boost::msm::front::euml::kleene_ >
    { 
      typedef ::boost::mpl::true_ type;
    };
}}

#endif // BOOST_MSM_FRONT_EUML_COMMON_H
