// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_OPERATOR_H
#define BOOST_MSM_FRONT_EUML_OPERATOR_H

#include <iterator>
#include <boost/msm/front/euml/common.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/set.hpp>
#include <boost/type_traits.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(reference)
BOOST_MPL_HAS_XXX_TRAIT_DEF(key_type)

namespace boost { namespace msm { namespace front { namespace euml
{

template <class T1,class T2>
struct Or_ : euml_action<Or_<T1,T2> >
{
    template <class EVT,class FSM,class SourceState,class TargetState>
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)
    {
        return (T1()(evt,fsm,src,tgt) || T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)
    {
        return (T1()(evt,fsm,state) || T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct And_ : euml_action<And_<T1,T2> >
{
    template <class EVT,class FSM,class SourceState,class TargetState>
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)
    {
        return (T1()(evt,fsm,src,tgt) && T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)
    {
        return (T1()(evt,fsm,state) && T2()(evt,fsm,state));
    }
};
template <class T1>
struct Not_ : euml_action<Not_<T1> >
{
    template <class EVT,class FSM,class SourceState,class TargetState>
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)
    {
        return !(T1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)
    {
        return !(T1()(evt,fsm,state));
    }
};

template <class Condition,class Action1,class Action2, class Enable=void >                                             
struct If_Else_ : euml_action<If_Else_<Condition,Action1,Action2,Enable> > {};        

template <class Condition,class Action1,class Action2>
struct If_Else_<Condition,Action1,Action2
    , typename ::boost::enable_if<typename has_tag_type<Action1>::type >::type>
    : euml_action<If_Else_<Condition,Action1,Action2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Action1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Action1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Action1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        if (Condition()(evt,fsm,src,tgt))
        {
            return Action1()(evt,fsm,src,tgt);
        }
        return Action2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Action1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        if (Condition()(evt,fsm,state))
        {
            return Action1()(evt,fsm,state);
        }
        return Action2()(evt,fsm,state);
    }
};

template <class Condition,class Action1,class Action2>
struct If_Else_<Condition,Action1,Action2
    , typename ::boost::disable_if<typename has_tag_type<Action1>::type >::type>
    : euml_action<If_Else_<Condition,Action1,Action2> >
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
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        if (Condition()(evt,fsm,src,tgt))
        {
            return Action1()(evt,fsm,src,tgt);
        }
        return Action2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        if (Condition()(evt,fsm,state))
        {
            return Action1()(evt,fsm,state);
        }
        return Action2()(evt,fsm,state);
    }
};

struct if_tag 
{
};
struct If : proto::extends<proto::terminal<if_tag>::type, If, boost::msm::sm_domain>
{
    If(){}
    using proto::extends< proto::terminal<if_tag>::type, If, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef If_Else_<Arg1,Arg2,Arg3> type;
    };
};
If const if_then_else_;

template <class Condition,class Action1, class Enable=void >                                             
struct If_Then_ : euml_action<If_Then_<Condition,Action1,Enable> > {};        

template <class Condition,class Action1>
struct If_Then_<Condition,Action1
    , typename ::boost::enable_if<typename has_tag_type<Action1>::type >::type>
    : euml_action<If_Then_<Condition,Action1> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Action1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Action1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Action1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        if (Condition()(evt,fsm,src,tgt))
        {
            return Action1()(evt,fsm,src,tgt);
        }
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Action1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        if (Condition()(evt,fsm,state))
        {
            return Action1()(evt,fsm,state);
        }
    }
};

template <class Condition,class Action1>
struct If_Then_<Condition,Action1
    , typename ::boost::disable_if<typename has_tag_type<Action1>::type >::type>
    : euml_action<If_Then_<Condition,Action1> >
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
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        if (Condition()(evt,fsm,src,tgt))
        {
            return Action1()(evt,fsm,src,tgt);
        }
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        if (Condition()(evt,fsm,state))
        {
            return Action1()(evt,fsm,state);
        }
    }
};
struct if_then_tag 
{
};
struct If_Then : proto::extends< proto::terminal<if_then_tag>::type, If_Then, boost::msm::sm_domain>
{
    If_Then(){}
    using proto::extends< proto::terminal<if_then_tag>::type, If_Then, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef If_Then_<Arg1,Arg2> type;
    };
};
If_Then const if_then_;

template <class Condition,class Body>
struct While_Do_ : euml_action<While_Do_<Condition,Body> >
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
    void operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        Body body_;
        Condition cond_;
        while (cond_(evt,fsm,src,tgt))
        {
            body_(evt,fsm,src,tgt);
        }
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        Body body_;
        Condition cond_;
        while (cond_(evt,fsm,state))
        {
            body_(evt,fsm,state);
        }
    }
};
struct while_do_tag 
{
};
struct While_Do_Helper : proto::extends< proto::terminal<while_do_tag>::type, While_Do_Helper, boost::msm::sm_domain>
{
    While_Do_Helper(){}
    using proto::extends< proto::terminal<while_do_tag>::type, While_Do_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef While_Do_<Arg1,Arg2> type;
    };
};
While_Do_Helper const while_;

template <class Condition,class Body>
struct Do_While_ : euml_action<Do_While_<Condition,Body> >
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
    void operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        Condition cond_;
        Body body_;
        do
        {
            body_(evt,fsm,src,tgt);
        } while (cond_(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        Condition cond_;
        Body body_;
        do
        {
            body_(evt,fsm,state);
        } while (cond_(evt,fsm,state));
    }
};
struct do_while_tag 
{
};
struct Do_While_Helper : proto::extends< proto::terminal<do_while_tag>::type, Do_While_Helper, boost::msm::sm_domain>
{
    Do_While_Helper(){}
    using proto::extends< proto::terminal<do_while_tag>::type, Do_While_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Do_While_<Arg1,Arg2> type;
    };
};
Do_While_Helper const do_while_;

template <class Begin,class End,class EndLoop,class Body>
struct For_Loop_ : euml_action<For_Loop_<Begin,End,EndLoop,Body> >
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
    void operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        End end_;
        EndLoop end_loop_;
        Body body_;
        for(Begin()(evt,fsm,src,tgt);end_(evt,fsm,src,tgt);end_loop_(evt,fsm,src,tgt))
        {
            body_(evt,fsm,src,tgt);
        }
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        End end_;
        EndLoop end_loop_;
        Body body_;
        for(Begin()(evt,fsm,state);end_(evt,fsm,state);end_loop_(evt,fsm,state))
        {
            body_(evt,fsm,state);
        }
    }
};
struct for_loop_tag 
{
};
struct For_Loop_Helper : proto::extends< proto::terminal<for_loop_tag>::type, For_Loop_Helper, boost::msm::sm_domain>
{
    For_Loop_Helper(){}
    using proto::extends< proto::terminal<for_loop_tag>::type, For_Loop_Helper, boost::msm::sm_domain>::operator=;
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef For_Loop_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
For_Loop_Helper const for_;




template <class T>
struct Deref_ : euml_action<Deref_<T> >
{
    Deref_(){}
    using euml_action<Deref_<T> >::operator=;
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::add_reference<
                    typename std::iterator_traits <
                        typename ::boost::remove_reference<
                            typename get_result_type2<T,Event,FSM,STATE>::type>::type>::value_type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::add_reference<
                    typename std::iterator_traits< 
                        typename ::boost::remove_reference<
                            typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type
                    >::value_type
        >::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return *(T()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return *(T()(evt,fsm,state));
    }
};

template <class T>
struct Pre_inc_ : euml_action<Pre_inc_<T> >
{
    using euml_action<Pre_inc_<T> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return ++T()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return ++T()(evt,fsm,state);
    }
};
template <class T>
struct Pre_dec_ : euml_action<Pre_dec_<T> >
{
    using euml_action<Pre_dec_<T> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return --T()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return --T()(evt,fsm,state);
    }
};
template <class T>
struct Post_inc_ : euml_action<Post_inc_<T> >
{
    using euml_action<Post_inc_<T> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T()(evt,fsm,src,tgt)++;
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T()(evt,fsm,state)++;
    }
};
template <class T>
struct Post_dec_ : euml_action<Post_dec_<T> >
{
    using euml_action<Post_dec_<T> >::operator=;

    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T()(evt,fsm,src,tgt)--;
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T()(evt,fsm,state)--;
    }
};

template <class T1,class T2>
struct Plus_ : euml_action<Plus_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)+T2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)+T2()(evt,fsm,state);
    }
};
template <class T1,class T2>
struct Minus_ : euml_action<Minus_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)-T2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)-T2()(evt,fsm,state);
    }
};
template <class T1,class T2>
struct Multiplies_ : euml_action<Multiplies_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)*T2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)*T2()(evt,fsm,state);
    }
};
template <class T1,class T2>
struct Divides_ : euml_action<Divides_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)/T2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)/T2()(evt,fsm,state);
    }
};
template <class T1,class T2>
struct Modulus_ : euml_action<Modulus_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)%T2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)%T2()(evt,fsm,state);
    }
};
template <class T1,class T2>
struct Bitwise_And_ : euml_action<Bitwise_And_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)&T2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)&T2()(evt,fsm,state);
    }
};
template <class T1,class T2>
struct Bitwise_Or_ : euml_action<Bitwise_Or_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)|T2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)|T2()(evt,fsm,state);
    }
};
template <class T1,class T2>
struct Bitwise_Xor_ : euml_action<Bitwise_Xor_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)^T2()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)^T2()(evt,fsm,state);
    }
};
template <class T1,class T2>
struct Subscript_ : euml_action<Subscript_<T1,T2> >
{
    template <class T>
    struct get_reference 
    {
        typedef typename T::reference type;
    };
    template <class T>
    struct get_mapped_type 
    {
        typedef typename T::value_type::second_type& type;
    };
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type container_type;
        typedef typename ::boost::mpl::eval_if<
            typename has_key_type<container_type>::type,
            get_mapped_type<container_type>,
            ::boost::mpl::eval_if<
                typename ::boost::is_pointer<container_type>::type,
                ::boost::add_reference<typename ::boost::remove_pointer<container_type>::type >,
                get_reference<container_type> 
             >
        >::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type container_type;
        typedef typename ::boost::mpl::eval_if<
            typename has_key_type<container_type>::type,
            get_mapped_type<container_type>,
            ::boost::mpl::eval_if<
                typename ::boost::is_pointer<container_type>::type,
                ::boost::add_reference<typename ::boost::remove_pointer<container_type>::type >,
                get_reference<container_type> 
             >
        >::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return T1()(evt,fsm,src,tgt)[T2()(evt,fsm,src,tgt)];
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return T1()(evt,fsm,state)[T2()(evt,fsm,state)];
    }
};
template <class T1,class T2>
struct Plus_Assign_ : euml_action<Plus_Assign_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)+=T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)+=T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct Minus_Assign_ : euml_action<Minus_Assign_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)-=T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)-=T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct Multiplies_Assign_ : euml_action<Multiplies_Assign_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)*=T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)*=T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct Divides_Assign_ : euml_action<Divides_Assign_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)/=T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)/=T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct Modulus_Assign_ : euml_action<Modulus_Assign_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)%=T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)%=T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct ShiftLeft_Assign_ : euml_action<ShiftLeft_Assign_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)<<=T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)<<=T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct ShiftRight_Assign_ : euml_action<ShiftRight_Assign_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)>>=T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)>>=T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct ShiftLeft_ : euml_action<ShiftLeft_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)<<T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)<<T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct ShiftRight_ : euml_action<ShiftRight_<T1,T2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)>>T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)>>T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct Assign_ : euml_action<Assign_<T1,T2> >
{
    using euml_action< Assign_<T1,T2> >::operator=;
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<T1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt)=T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T1()(evt,fsm,state)=T2()(evt,fsm,state));
    }
};
template <class T1>
struct Unary_Plus_ : euml_action<Unary_Plus_<T1> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return +T1()(evt,fsm,src,tgt);
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return +T1()(evt,fsm,state);
    }
};
template <class T1>
struct Unary_Minus_ : euml_action<Unary_Minus_<T1> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type2<T1,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::remove_reference<
            typename get_result_type<T1,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return -(T1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return -(T1()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct Less_ : euml_action<Less_<T1,T2> >
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
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt) < T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)const
    {
        return (T1()(evt,fsm,state) < T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct LessEqual_ : euml_action<LessEqual_<T1,T2> >
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
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt) <= T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)const
    {
        return (T1()(evt,fsm,state) <= T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct Greater_ : euml_action<Greater_<T1,T2> >
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
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt) > T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)const
    {
        return (T1()(evt,fsm,state) > T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct GreaterEqual_ : euml_action<GreaterEqual_<T1,T2> >
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
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt) >= T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)const
    {
        return (T1()(evt,fsm,state) >= T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct EqualTo_ : euml_action<EqualTo_<T1,T2> >
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
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt) == T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)const
    {
        return (T1()(evt,fsm,state) == T2()(evt,fsm,state));
    }
};
template <class T1,class T2>
struct NotEqualTo_ : euml_action<NotEqualTo_<T1,T2> >
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
    bool operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T1()(evt,fsm,src,tgt) != T2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state)const
    {
        return (T1()(evt,fsm,state) != T2()(evt,fsm,state));
    }
};

}}}}

#endif // BOOST_MSM_FRONT_EUML_OPERATOR_H
