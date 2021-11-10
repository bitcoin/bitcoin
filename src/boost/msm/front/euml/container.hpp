// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_CONTAINER_H
#define BOOST_MSM_FRONT_EUML_CONTAINER_H

#include <utility>
#include <boost/msm/front/euml/common.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/has_key.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/not.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/type_traits.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(iterator_category)

namespace boost { namespace msm { namespace front { namespace euml
{

template <class T>
struct Front_ : euml_action<Front_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_reference< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_reference< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).front();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).front();
    }
};

struct front_tag {};
struct Front_Helper: proto::extends< proto::terminal<front_tag>::type, Front_Helper, boost::msm::sm_domain>
{
    Front_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Front_<Arg1> type;
    };
};
Front_Helper const front_;

template <class T>
struct Back_ : euml_action<Back_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_reference< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_reference< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).back();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).back();
    }
};

struct back_tag {};
struct Back_Helper: proto::extends< proto::terminal<back_tag>::type, Back_Helper, boost::msm::sm_domain>
{
    Back_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Back_<Arg1> type;
    };
};
Back_Helper const back_;

template <class T>
struct Begin_ : euml_action<Begin_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).begin();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).begin();
    }
};

struct begin_tag {};
struct Begin_Helper: proto::extends< proto::terminal<begin_tag>::type, Begin_Helper, boost::msm::sm_domain>
{
    Begin_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Begin_<Arg1> type;
    };
};
Begin_Helper const begin_;

template <class T>
struct End_ : euml_action<End_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).end();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).end();
    }
};
struct end_tag {};
struct End_Helper: proto::extends< proto::terminal<end_tag>::type, End_Helper, boost::msm::sm_domain>
{
    End_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef End_<Arg1> type;
    };
};
End_Helper const end_;

template <class T>
struct RBegin_ : euml_action<RBegin_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_reverse_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_reverse_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).rbegin();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).rbegin();
    }
};

struct rbegin_tag {};
struct RBegin_Helper: proto::extends< proto::terminal<rbegin_tag>::type, RBegin_Helper, boost::msm::sm_domain>
{
    RBegin_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef RBegin_<Arg1> type;
    };
};
RBegin_Helper const rbegin_;

template <class T>
struct REnd_ : euml_action<REnd_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_reverse_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_reverse_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).rend();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).rend();
    }
};
struct rend_tag {};
struct REnd_Helper: proto::extends< proto::terminal<rend_tag>::type, REnd_Helper, boost::msm::sm_domain>
{
    REnd_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef REnd_<Arg1> type;
    };
};
REnd_Helper const rend_;

template <class Container,class Element>
struct Push_Back_ : euml_action<Push_Back_<Container,Element> >
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
        (Container()(evt,fsm,src,tgt)).push_back(Element()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).push_back(Element()(evt,fsm,state));        
    }
};
struct push_back_tag {};
struct Push_Back_Helper: proto::extends< proto::terminal<push_back_tag>::type, Push_Back_Helper, boost::msm::sm_domain>
{
    Push_Back_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Push_Back_<Arg1,Arg2> type;
    };
};
Push_Back_Helper const push_back_;

template <class Container>
struct Pop_Back_ : euml_action<Pop_Back_<Container> >
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
        (Container()(evt,fsm,src,tgt)).pop_back();
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).pop_back();        
    }
};
struct pop_back_tag {};
struct Pop_Back_Helper: proto::extends< proto::terminal<pop_back_tag>::type, Pop_Back_Helper, boost::msm::sm_domain>
{
    Pop_Back_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Pop_Back_<Arg1> type;
    };
};
Pop_Back_Helper const pop_back_;

template <class Container,class Element>
struct Push_Front_ : euml_action<Push_Front_<Container,Element> >
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
        (Container()(evt,fsm,src,tgt)).push_front(Element()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).push_front(Element()(evt,fsm,state));        
    }
};
struct push_front_tag {};
struct Push_Front_Helper: proto::extends< proto::terminal<push_front_tag>::type, Push_Front_Helper, boost::msm::sm_domain>
{
    Push_Front_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Push_Front_<Arg1,Arg2> type;
    };
};
Push_Front_Helper const push_front_;

template <class Container>
struct Pop_Front_ : euml_action<Pop_Front_<Container> >
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
        (Container()(evt,fsm,src,tgt)).pop_front();
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).pop_front();        
    }
};
struct pop_front_tag {};
struct Pop_Front_Helper: proto::extends< proto::terminal<pop_front_tag>::type, Pop_Front_Helper, boost::msm::sm_domain>
{
    Pop_Front_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Pop_Front_<Arg1> type;
    };
};
Pop_Front_Helper const pop_front_;

template <class Container>
struct Clear_ : euml_action<Clear_<Container> >
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
        (Container()(evt,fsm,src,tgt)).clear();
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).clear();        
    }
};
struct clear_tag {};
struct Clear_Helper: proto::extends< proto::terminal<clear_tag>::type, Clear_Helper, boost::msm::sm_domain>
{
    Clear_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Clear_<Arg1> type;
    };
};
Clear_Helper const clear_;

template <class Container>
struct ListReverse_ : euml_action<ListReverse_<Container> >
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
        (Container()(evt,fsm,src,tgt)).reverse();
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).reverse();        
    }
};
struct list_reverse_tag {};
struct ListReverse_Helper: proto::extends< proto::terminal<list_reverse_tag>::type, ListReverse_Helper, boost::msm::sm_domain>
{
    ListReverse_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef ListReverse_<Arg1> type;
    };
};
ListReverse_Helper const list_reverse_;

template <class Container, class Predicate, class Enable=void>
struct ListUnique_ : euml_action<ListUnique_<Container,Predicate,Enable> >
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
        (Container()(evt,fsm,src,tgt)).unique();
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).unique();        
    }
};
template <class Container, class Predicate >
struct ListUnique_<Container,Predicate,
               typename ::boost::disable_if<typename ::boost::is_same<Predicate,void>::type >::type> 
                    : euml_action<ListUnique_<Container,Predicate> >
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
        (Container()(evt,fsm,src,tgt)).unique(Predicate()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).unique(Predicate()(evt,fsm,state));        
    }
};
struct list_unique_tag {};
struct ListUnique_Helper: proto::extends< proto::terminal<list_unique_tag>::type, ListUnique_Helper, boost::msm::sm_domain>
{
    ListUnique_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef ListUnique_<Arg1,Arg2> type;
    };
};
ListUnique_Helper const list_unique_;

template <class Container, class Predicate, class Enable=void>
struct ListSort_ : euml_action<ListSort_<Container,Predicate,Enable> >
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
        (Container()(evt,fsm,src,tgt)).sort();
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).sort();        
    }
};
template <class Container, class Predicate >
struct ListSort_<Container,Predicate,
               typename ::boost::disable_if<typename ::boost::is_same<Predicate,void>::type >::type> 
                    : euml_action<ListSort_<Container,Predicate> >
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
        (Container()(evt,fsm,src,tgt)).sort(Predicate()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).sort(Predicate()(evt,fsm,state));        
    }
};
struct list_sort_tag {};
struct ListSort_Helper: proto::extends< proto::terminal<list_sort_tag>::type, ListSort_Helper, boost::msm::sm_domain>
{
    ListSort_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef ListSort_<Arg1,Arg2> type;
    };
};
ListSort_Helper const list_sort_;

template <class Container>
struct Capacity_ : euml_action<Capacity_<Container> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).capacity();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).capacity();        
    }
};
struct capacity_tag {};
struct Capacity_Helper: proto::extends< proto::terminal<capacity_tag>::type, Capacity_Helper, boost::msm::sm_domain>
{
    Capacity_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Capacity_<Arg1> type;
    };
};
Capacity_Helper const capacity_;

template <class Container>
struct Size_ : euml_action<Size_<Container> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).size();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).size();        
    }
};
struct size_tag {};
struct Size_Helper: proto::extends< proto::terminal<size_tag>::type, Size_Helper, boost::msm::sm_domain>
{
    Size_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Size_<Arg1> type;
    };
};
Size_Helper const size_;

template <class Container>
struct Max_Size_ : euml_action<Max_Size_<Container> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).max_size();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).max_size();        
    }
};
struct max_size_tag {};
struct Max_Size_Helper: proto::extends< proto::terminal<max_size_tag>::type, Max_Size_Helper, boost::msm::sm_domain>
{
    Max_Size_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Max_Size_<Arg1> type;
    };
};
Max_Size_Helper const max_size_;

template <class Container, class Value>
struct Reserve_ : euml_action<Reserve_<Container,Value> >
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
        (Container()(evt,fsm,src,tgt)).reserve(Value()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).reserve(Value()(evt,fsm,state));        
    }
};
struct reserve_tag {};
struct Reserve_Helper: proto::extends< proto::terminal<reserve_tag>::type, Reserve_Helper, boost::msm::sm_domain>
{
    Reserve_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Reserve_<Arg1,Arg2> type;
    };
};
Reserve_Helper const reserve_;

template <class Container, class Num, class Value ,class Enable=void >
struct Resize_ : euml_action<Resize_<Container,Num,Value> >
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
        (Container()(evt,fsm,src,tgt)).resize(Num()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).resize(Num()(evt,fsm,state));        
    }
};
template <class Container, class Num , class Value >
struct Resize_<Container,Num,Value,typename ::boost::disable_if<typename ::boost::is_same<Value,void>::type >::type> 
                    : euml_action<Resize_<Container,Num,Value> >
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
        (Container()(evt,fsm,src,tgt)).resize(Num()(evt,fsm,src,tgt),Value()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).resize(Num()(evt,fsm,state),Value()(evt,fsm,state));
    }
};
struct resize_tag {};
struct Resize_Helper: proto::extends< proto::terminal<resize_tag>::type, Resize_Helper, boost::msm::sm_domain>
{
    Resize_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Resize_<Arg1,Arg2,Arg3> type;
    };
};
Resize_Helper const resize_;

// version for 3 parameters (sequence containers)
template <class Container, class Param1, class Param2, class Param3 >
struct Insert_ : euml_action<Insert_<Container,Param1,Param2,Param3> >
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
        (Container()(evt,fsm,src,tgt)).insert(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                              Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).insert(Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                            Param3()(evt,fsm,state));
    }
};
// version for 2 parameters
template <class Container, class Param1, class Param2>
struct Insert_ < Container,Param1,Param2,void>
    : euml_action<Insert_<Container,Param1,Param2,void> >
{
    // return value will actually not be correct for set::insert(it1,it2), should be void
    // but it's ok as nobody should call an inexistent return type
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    // version for transition + second param not an iterator (meaning that, Container is not an associative container)
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::and_<
            typename ::boost::mpl::has_key<
                typename Container::tag_type,action_tag>::type,
                typename ::boost::mpl::not_<
                    typename has_iterator_category<
                        typename Param2::template transition_action_result<EVT,FSM,SourceState,TargetState>::type
                    >::type
                   >::type
                >::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).insert(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }

    // version for transition + second param is an iterator (meaning that, Container is an associative container)
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::and_<
            typename ::boost::mpl::has_key<
                typename Container::tag_type,action_tag>::type,
                    typename has_iterator_category<
                        typename Param2::template transition_action_result<EVT,FSM,SourceState,TargetState>::type
                    >::type
                >::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Container()(evt,fsm,src,tgt)).insert(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }

    // version for state action + second param not an iterator (meaning that, Container is not an associative container)
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::and_<
            typename ::boost::mpl::has_key<
                typename Container::tag_type,state_action_tag>::type,
                typename ::boost::mpl::not_<
                    typename has_iterator_category<
                        typename Param2::template state_action_result<Event,FSM,STATE>::type
                    >::type
                   >::type
                >::type,
            typename state_action_result<Event,FSM,STATE>::type 
        >::type  
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).insert(Param1()(evt,fsm,state),Param2()(evt,fsm,state));        
    }

    // version for state action + second param is an iterator (meaning that, Container is an associative container)
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::and_<
            typename ::boost::mpl::has_key<
                typename Container::tag_type,state_action_tag>::type,
                    typename has_iterator_category<
                        typename Param2::template state_action_result<Event,FSM,STATE>::type
                    >::type
                >::type,
            typename state_action_result<Event,FSM,STATE>::type 
        >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).insert(Param1()(evt,fsm,state),Param2()(evt,fsm,state));        
    }
};

// version for 1 parameter (associative containers)
template <class Container, class Param1>
struct Insert_ < Container,Param1,void,void>
    : euml_action<Insert_<Container,Param1,void,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename std::pair<
            typename get_iterator< 
                typename ::boost::remove_reference<
                    typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type,bool> type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename std::pair<
            typename get_iterator< 
                typename ::boost::remove_reference<
                    typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type,bool> type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).insert(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).insert(Param1()(evt,fsm,state));        
    }
};
struct insert_tag {};
struct Insert_Helper: proto::extends< proto::terminal<insert_tag>::type, Insert_Helper, boost::msm::sm_domain>
{
    Insert_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Insert_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
Insert_Helper const insert_;

template <class Container1,class Container2>
struct Swap_ : euml_action<Swap_<Container1,Container2> >
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
        (Container1()(evt,fsm,src,tgt)).swap(Container2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container1()(evt,fsm,state)).swap(Container2()(evt,fsm,state));        
    }
};
struct swap_tag {};
struct Swap_Helper: proto::extends< proto::terminal<swap_tag>::type, Swap_Helper, boost::msm::sm_domain>
{
    Swap_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Swap_<Arg1,Arg2> type;
    };
};
Swap_Helper const swap_;

template <class Container, class Iterator1, class Iterator2 ,class Enable=void >
struct Erase_ : euml_action<Erase_<Container,Iterator1,Iterator2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Iterator1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Iterator1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Iterator1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
    operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).erase(Iterator1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Iterator1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
    operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).erase(Iterator1()(evt,fsm,state));        
    }
};
template <class Container, class Iterator1 , class Iterator2 >
struct Erase_<Container,Iterator1,Iterator2,
              typename ::boost::disable_if<typename ::boost::is_same<Iterator2,void>::type >::type> 
                    : euml_action<Erase_<Container,Iterator1,Iterator2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Iterator1,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Iterator1,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Iterator1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
    operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).erase(Iterator1()(evt,fsm,src,tgt),Iterator2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Iterator1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
    operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).erase(Iterator1()(evt,fsm,state),Iterator2()(evt,fsm,state));        
    }
};
struct erase_tag {};
struct Erase_Helper: proto::extends< proto::terminal<erase_tag>::type, Erase_Helper, boost::msm::sm_domain>
{
    Erase_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Erase_<Arg1,Arg2,Arg3> type;
    };
};
Erase_Helper const erase_;

template <class Container>
struct Empty_ : euml_action<Empty_<Container> >
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
        return (Container()(evt,fsm,src,tgt)).empty();
    }
    template <class Event,class FSM,class STATE>
    bool operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).empty();        
    }
};
struct empty_tag {};
struct Empty_Helper: proto::extends< proto::terminal<empty_tag>::type, Empty_Helper, boost::msm::sm_domain>
{
    Empty_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Empty_<Arg1> type;
    };
};
Empty_Helper const empty_;

template <class Container,class Element>
struct ListRemove_ : euml_action<ListRemove_<Container,Element> >
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
        (Container()(evt,fsm,src,tgt)).remove(Element()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).remove(Element()(evt,fsm,state));        
    }
};
struct list_remove_tag {};
struct ListRemove_Helper: proto::extends< proto::terminal<list_remove_tag>::type, ListRemove_Helper, boost::msm::sm_domain>
{
    ListRemove_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef ListRemove_<Arg1,Arg2> type;
    };
};
ListRemove_Helper const list_remove_;

template <class Container,class Element>
struct ListRemove_If_ : euml_action<ListRemove_If_<Container,Element> >
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
        (Container()(evt,fsm,src,tgt)).remove_if(Element()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).remove_if(Element()(evt,fsm,state));        
    }
};
struct list_remove_if_tag {};
struct ListRemove_If_Helper: proto::extends< proto::terminal<list_remove_if_tag>::type, ListRemove_If_Helper, boost::msm::sm_domain>
{
    ListRemove_If_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef ListRemove_If_<Arg1,Arg2> type;
    };
};
ListRemove_If_Helper const list_remove_if_;

template <class Container, class ToMerge, class Predicate, class Enable=void>
struct ListMerge_ : euml_action<ListMerge_<Container,ToMerge,Predicate,Enable> >
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
        (Container()(evt,fsm,src,tgt)).merge(ToMerge()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).merge(ToMerge()(evt,fsm,state));        
    }
};
template <class Container, class ToMerge, class Predicate >
struct ListMerge_<Container,ToMerge,Predicate,
               typename ::boost::disable_if<typename ::boost::is_same<Predicate,void>::type >::type> 
                    : euml_action<ListMerge_<Container,ToMerge,Predicate> >
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
        (Container()(evt,fsm,src,tgt)).merge(ToMerge()(evt,fsm,src,tgt),Predicate()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).merge(ToMerge()(evt,fsm,state),Predicate()(evt,fsm,state));
    }
};
struct list_merge_tag {};
struct ListMerge_Helper: proto::extends< proto::terminal<list_merge_tag>::type, ListMerge_Helper, boost::msm::sm_domain>
{
    ListMerge_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef ListMerge_<Arg1,Arg2,Arg3> type;
    };
};
ListMerge_Helper const list_merge_;

template <class Container, class Param1, class Param2, class Param3, class Param4 ,class Enable=void >
struct Splice_ : euml_action<Splice_<Container,Param1,Param2,Param3,Param4,Enable> >
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
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).splice(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).splice(Param1()(evt,fsm,state),Param2()(evt,fsm,state));        
    }
};
template <class Container, class Param1, class Param2, class Param3, class Param4 >
struct Splice_<Container,Param1,Param2,Param3,Param4,
               typename ::boost::disable_if<  
                    typename ::boost::mpl::or_<typename ::boost::is_same<Param3,void>::type,
                                               typename ::boost::mpl::not_<
                                                    typename ::boost::is_same<Param4,void>::type>::type>::type >::type> 
                    : euml_action<Splice_<Container,Param1,Param2,Param3,Param4> >
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
        (Container()(evt,fsm,src,tgt)).splice(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                              Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).splice(Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                            Param3()(evt,fsm,state));
    }
};
template <class Container, class Param1, class Param2, class Param3, class Param4 >
struct Splice_<Container,Param1,Param2,Param3,Param4,
               typename ::boost::disable_if<typename ::boost::is_same<Param4,void>::type >::type> 
                    : euml_action<Splice_<Container,Param1,Param2,Param3,Param4> >
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
        (Container()(evt,fsm,src,tgt)).splice(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                              Param3()(evt,fsm,src,tgt),Param4()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    void operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).splice(Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                            Param3()(evt,fsm,state),Param4()(evt,fsm,state));
    }
};
struct splice_tag {};
struct Splice_Helper: proto::extends< proto::terminal<splice_tag>::type, Splice_Helper, boost::msm::sm_domain>
{
    Splice_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Splice_<Arg1,Arg2,Arg3,Arg4,Arg5> type;
    };
};
Splice_Helper const splice_;

//template <class Container, class Param1, class Param2, class Param3, class Enable=void >
//struct StringFind_ : euml_action<StringFind_<Container,Param1,Param2,Param3,Enable> >
//{
//};
template <class Container,class Param1, class Param2, class Param3>
struct StringFind_ : euml_action<StringFind_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            find(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            find(Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));
    }
};

template <class Container,class Param1>
struct StringFind_ < Container,Param1,void,void>
                : euml_action<StringFind_<Container,Param1,void,void> >

{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find(Param1()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2>
struct StringFind_ <Container,Param1,Param2,void>
                : euml_action<StringFind_<Container,Param1,Param2,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

struct string_find_tag {};
struct StringFind_Helper: proto::extends< proto::terminal<string_find_tag>::type, StringFind_Helper, boost::msm::sm_domain>
{
    StringFind_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringFind_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
StringFind_Helper const string_find_;

template <class Container, class Param1, class Param2, class Param3, class Enable=void >
struct StringRFind_ : euml_action<StringRFind_<Container,Param1,Param2,Param3,Enable> >
{
};

template <class Container,class Param1, class Param2, class Param3>
struct StringRFind_ < 
        Container,Param1,Param2,Param3,
        typename ::boost::enable_if< 
                    typename ::boost::is_same<Param2,void>::type
                    >::type
                >
                : euml_action<StringRFind_<Container,Param1,Param2,Param3> >

{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).rfind(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).rfind(Param1()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringRFind_ < 
                Container,Param1,Param2,Param3,
                    typename ::boost::enable_if<
                        typename ::boost::mpl::and_<
                            typename ::boost::is_same<Param3,void>::type,
                            typename ::boost::mpl::not_<
                                typename ::boost::is_same<Param2,void>::type
                                                >::type
                                             >::type
                                       >::type
                    >
                : euml_action<StringRFind_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).rfind(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).rfind(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringRFind_< 
    Container,Param1,Param2,Param3,
            typename ::boost::disable_if< 
                                typename ::boost::is_same<Param3,void>::type
                                >::type
                >
                : euml_action<StringRFind_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            rfind(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            rfind(Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));
    }
};

struct string_rfind_tag {};
struct StringRFind_Helper: proto::extends< proto::terminal<string_rfind_tag>::type, StringRFind_Helper, boost::msm::sm_domain>
{
    StringRFind_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringRFind_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
StringRFind_Helper const string_rfind_;

template <class Container,class Param1, class Param2, class Param3>
struct StringFindFirstOf_ : euml_action<StringFindFirstOf_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            find_first_of(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            find_first_of(Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));
    }
};
template <class Container,class Param1>
struct StringFindFirstOf_ <Container,Param1,void,void>
                : euml_action<StringFindFirstOf_<Container,Param1,void,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find_first_of(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find_first_of(Param1()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2>
struct StringFindFirstOf_ <Container,Param1,Param2,void>
                : euml_action<StringFindFirstOf_<Container,Param1,Param2,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find_first_of(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find_first_of(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

struct string_find_first_of_tag {};
struct StringFindFirstOf_Helper: 
    proto::extends< proto::terminal<string_find_first_of_tag>::type, StringFindFirstOf_Helper, boost::msm::sm_domain>
{
    StringFindFirstOf_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringFindFirstOf_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
StringFindFirstOf_Helper const string_find_first_of_;

template <class Container, class Param1, class Param2, class Param3, class Enable=void >
struct StringFindFirstNotOf_ : euml_action<StringFindFirstNotOf_<Container,Param1,Param2,Param3,Enable> >
{
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindFirstNotOf_ < 
        Container,Param1,Param2,Param3,
        typename ::boost::enable_if< 
                    typename ::boost::is_same<Param2,void>::type
                    >::type
                >
                : euml_action<StringFindFirstNotOf_<Container,Param1,Param2,Param3> >

{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find_first_not_of(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find_first_not_of(Param1()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindFirstNotOf_ < 
                Container,Param1,Param2,Param3,
                    typename ::boost::enable_if<
                        typename ::boost::mpl::and_<
                            typename ::boost::is_same<Param3,void>::type,
                            typename ::boost::mpl::not_<
                                typename ::boost::is_same<Param2,void>::type
                                                >::type
                                             >::type
                                       >::type
                    >
                : euml_action<StringFindFirstNotOf_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find_first_not_of(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find_first_not_of(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindFirstNotOf_< 
    Container,Param1,Param2,Param3,
            typename ::boost::disable_if< 
                                typename ::boost::is_same<Param3,void>::type
                                >::type
                >
                : euml_action<StringFindFirstNotOf_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            find_first_not_of(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            find_first_not_of(Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));
    }
};

struct string_find_first_not_of_tag {};
struct StringFindFirstNotOf_Helper: 
    proto::extends< proto::terminal<string_find_first_not_of_tag>::type, StringFindFirstNotOf_Helper, boost::msm::sm_domain>
{
    StringFindFirstNotOf_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringFindFirstNotOf_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
StringFindFirstNotOf_Helper const string_find_first_not_of_;

template <class Container, class Param1, class Param2, class Param3, class Enable=void >
struct StringFindLastOf_ : euml_action<StringFindLastOf_<Container,Param1,Param2,Param3,Enable> >
{
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindLastOf_ < 
        Container,Param1,Param2,Param3,
        typename ::boost::enable_if< 
                    typename ::boost::is_same<Param2,void>::type
                    >::type
                >
                : euml_action<StringFindLastOf_<Container,Param1,Param2,Param3> >

{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find_last_of(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find_last_of(Param1()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindLastOf_ < 
                Container,Param1,Param2,Param3,
                    typename ::boost::enable_if<
                        typename ::boost::mpl::and_<
                            typename ::boost::is_same<Param3,void>::type,
                            typename ::boost::mpl::not_<
                                typename ::boost::is_same<Param2,void>::type
                                                >::type
                                             >::type
                                       >::type
                    >
                : euml_action<StringFindLastOf_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find_last_of(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find_last_of(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindLastOf_< 
    Container,Param1,Param2,Param3,
            typename ::boost::disable_if< 
                                typename ::boost::is_same<Param3,void>::type
                                >::type
                >
                : euml_action<StringFindLastOf_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            find_last_of(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            find_last_of(Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));
    }
};

struct string_find_last_of_tag {};
struct StringFindLastOf_Helper: 
    proto::extends< proto::terminal<string_find_last_of_tag>::type, StringFindLastOf_Helper, boost::msm::sm_domain>
{
    StringFindLastOf_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringFindLastOf_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
StringFindLastOf_Helper const string_find_last_of_;

template <class Container, class Param1, class Param2, class Param3, class Enable=void >
struct StringFindLastNotOf_ : euml_action<StringFindLastNotOf_<Container,Param1,Param2,Param3,Enable> >
{
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindLastNotOf_ < 
        Container,Param1,Param2,Param3,
        typename ::boost::enable_if< 
                    typename ::boost::is_same<Param2,void>::type
                    >::type
                >
                : euml_action<StringFindLastNotOf_<Container,Param1,Param2,Param3> >

{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find_last_not_of(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find_last_not_of(Param1()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindLastNotOf_ < 
                Container,Param1,Param2,Param3,
                    typename ::boost::enable_if<
                        typename ::boost::mpl::and_<
                            typename ::boost::is_same<Param3,void>::type,
                            typename ::boost::mpl::not_<
                                typename ::boost::is_same<Param2,void>::type
                                                >::type
                                             >::type
                                       >::type
                    >
                : euml_action<StringFindLastNotOf_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).find_last_not_of(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).find_last_not_of(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringFindLastNotOf_< 
    Container,Param1,Param2,Param3,
            typename ::boost::disable_if< 
                                typename ::boost::is_same<Param3,void>::type
                                >::type
                >
                : euml_action<StringFindLastNotOf_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            find_last_not_of(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            find_last_not_of(Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));
    }
};

struct string_find_last_not_of_tag {};
struct StringFindLastNotOf_Helper: 
    proto::extends< proto::terminal<string_find_last_of_tag>::type, StringFindLastNotOf_Helper, boost::msm::sm_domain>
{
    StringFindLastNotOf_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringFindLastNotOf_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
StringFindLastNotOf_Helper const string_find_last_not_of_;

template <class Container>
struct Npos_ : euml_action<Npos_<Container> >
{
    Npos_(){}
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename Container::size_type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename Container::size_type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return Container::npos;
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return Container::npos;
    }
};

// version for 2 parameters
template <class Container, class Param1, class Param2>
struct Associative_Erase_ : euml_action<Associative_Erase_<Container,Param1,Param2> >
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
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Container()(evt,fsm,src,tgt)).erase(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).erase(Param1()(evt,fsm,state),Param2()(evt,fsm,state));        
    }
};
// version for 1 parameter
template <class Container, class Param1>
struct Associative_Erase_ < Container,Param1,void>
    : euml_action<Associative_Erase_<Container,Param1,void> >
{
    // return value will actually not be correct for set::erase(it), should be void
    // but it's ok as nobody should call an inexistent return type
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    // version for transition + param is an iterator
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::and_<
            typename ::boost::mpl::has_key<
                typename Container::tag_type,action_tag>::type,
                    typename has_iterator_category<
                        typename Param1::template transition_action_result<EVT,FSM,SourceState,TargetState>::type
                    >::type
                >::type,
            void 
        >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        (Container()(evt,fsm,src,tgt)).erase(Param1()(evt,fsm,src,tgt));
    }

    // version for state action + param is an iterator
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::and_<
            typename ::boost::mpl::has_key<
                typename Container::tag_type,state_action_tag>::type,
                    typename has_iterator_category<
                        typename Param1::template state_action_result<Event,FSM,STATE>::type
                    >::type
                >::type,
            void 
        >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        (Container()(evt,fsm,state)).erase(Param1()(evt,fsm,state));        
    }

    // version for transition + param not an iterator
    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::and_<
            typename ::boost::mpl::has_key<
                typename Container::tag_type,action_tag>::type,
                typename ::boost::mpl::not_<
                    typename has_iterator_category<
                        typename Param1::template transition_action_result<EVT,FSM,SourceState,TargetState>::type
                    >::type
                   >::type
                >::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type 
        >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).erase(Param1()(evt,fsm,src,tgt));
    }

    // version for state action + param not an iterator
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::and_<
            typename ::boost::mpl::has_key<
                typename Container::tag_type,state_action_tag>::type,
                typename ::boost::mpl::not_<
                    typename has_iterator_category<
                        typename Param1::template state_action_result<Event,FSM,STATE>::type
                    >::type
                   >::type
                >::type,
            typename state_action_result<Event,FSM,STATE>::type 
        >::type  
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).erase(Param1()(evt,fsm,state));        
    }
};

struct associative_erase_tag {};
struct Associative_Erase_Helper: proto::extends< proto::terminal<associative_erase_tag>::type, Associative_Erase_Helper, boost::msm::sm_domain>
{
    Associative_Erase_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Associative_Erase_<Arg1,Arg2,Arg3> type;
    };
};
Associative_Erase_Helper const associative_erase_;


template <class T, class Param>
struct Associative_Find_ : euml_action<Associative_Find_<T,Param> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).find(Param()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).find(Param()(evt,fsm,state));
    }
};

struct associative_find_tag {};
struct Associative_Find_Helper: proto::extends< proto::terminal<associative_find_tag>::type, Associative_Find_Helper, boost::msm::sm_domain>
{
    Associative_Find_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Associative_Find_<Arg1,Arg2> type;
    };
};
Associative_Find_Helper const associative_find_;

template <class Container,class Param>
struct AssociativeCount_ : euml_action<AssociativeCount_<Container,Param> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).count(Param()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).count(Param()(evt,fsm,state));        
    }
};
struct associative_count_tag {};
struct AssociativeCount_Helper: proto::extends< proto::terminal<associative_count_tag>::type, AssociativeCount_Helper, boost::msm::sm_domain>
{
    AssociativeCount_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef AssociativeCount_<Arg1,Arg2> type;
    };
};
AssociativeCount_Helper const associative_count_;

template <class T, class Param>
struct Associative_Lower_Bound_ : euml_action<Associative_Lower_Bound_<T,Param> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).lower_bound(Param()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).lower_bound(Param()(evt,fsm,state));
    }
};

struct associative_lower_bound_tag {};
struct Associative_Lower_Bound_Helper: proto::extends< proto::terminal<associative_lower_bound_tag>::type, 
                                                       Associative_Lower_Bound_Helper, boost::msm::sm_domain>
{
    Associative_Lower_Bound_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Associative_Lower_Bound_<Arg1,Arg2> type;
    };
};
Associative_Lower_Bound_Helper const associative_lower_bound_;

template <class T, class Param>
struct Associative_Upper_Bound_ : euml_action<Associative_Upper_Bound_<T,Param> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).upper_bound(Param()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).upper_bound(Param()(evt,fsm,state));
    }
};

struct associative_upper_bound_tag {};
struct Associative_Upper_Bound_Helper: proto::extends< proto::terminal<associative_upper_bound_tag>::type, 
                                                       Associative_Upper_Bound_Helper, boost::msm::sm_domain>
{
    Associative_Upper_Bound_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Associative_Upper_Bound_<Arg1,Arg2> type;
    };
};
Associative_Upper_Bound_Helper const associative_upper_bound_;

template <class T>
struct First_ : euml_action<First_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_first_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_first_type< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).first;
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).first;
    }
};

struct first_tag {};
struct First_Helper: proto::extends< proto::terminal<first_tag>::type, First_Helper, boost::msm::sm_domain>
{
    First_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef First_<Arg1> type;
    };
};
First_Helper const first_;

template <class T>
struct Second_ : euml_action<Second_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_second_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_second_type< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).second;
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).second;
    }
};

struct second_tag {};
struct Second_Helper: proto::extends< proto::terminal<second_tag>::type, Second_Helper, boost::msm::sm_domain>
{
    Second_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Second_<Arg1> type;
    };
};
Second_Helper const second_;

template <class T, class Param>
struct Associative_Equal_Range_ : euml_action<Associative_Equal_Range_<T,Param> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef std::pair<
            typename get_iterator< 
                typename ::boost::remove_reference<
                    typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type,
            typename get_iterator< 
                typename ::boost::remove_reference<
                    typename get_result_type2<T,Event,FSM,STATE>::type>::type>::type > type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef std::pair<
            typename get_iterator< 
                typename ::boost::remove_reference<
                    typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type,
            typename get_iterator< 
                typename ::boost::remove_reference<
                    typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type>::type > type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (T()(evt,fsm,src,tgt)).equal_range(Param()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (T()(evt,fsm,state)).equal_range(Param()(evt,fsm,state));
    }
};

struct associative_equal_range_tag {};
struct Associative_Equal_Range_Helper: proto::extends< proto::terminal<associative_equal_range_tag>::type, 
                                                       Associative_Equal_Range_Helper, boost::msm::sm_domain>
{
    Associative_Equal_Range_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Associative_Equal_Range_<Arg1,Arg2> type;
    };
};
Associative_Equal_Range_Helper const associative_equal_range_;

template <class Container,class Param1, class Param2>
struct Substr_ : euml_action<Substr_<Container,Param1,Param2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            substr(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            substr(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};
template <class Container>
struct Substr_ <Container,void,void>
                : euml_action<Substr_<Container,void,void> >

{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).substr();
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).substr();
    }
};

template <class Container,class Param1>
struct Substr_ < Container,Param1,void>
                : euml_action<Substr_<Container,Param1,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type2<Container,Event,FSM,STATE>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename remove_reference<
            typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).substr(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).substr(Param1()(evt,fsm,state));
    }
};
struct substr_tag {};
struct Substr_Helper: proto::extends< proto::terminal<substr_tag>::type, Substr_Helper, boost::msm::sm_domain>
{
    Substr_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Substr_<Arg1,Arg2,Arg3> type;
    };
};
Substr_Helper const substr_;

template <class Container, class Param1, class Param2, class Param3, class Param4 >
struct StringCompare_ : euml_action<StringCompare_<Container,Param1,Param2,Param3,Param4> >
{
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
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).compare(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                                      Param3()(evt,fsm,src,tgt),Param4()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).compare(Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                                    Param3()(evt,fsm,state),Param4()(evt,fsm,state));
    }
};
template <class Container, class Param1 >
struct StringCompare_<Container,Param1,void,void,void>  
    : euml_action<StringCompare_<Container,Param1,void,void,void> >
{
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
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).compare(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).compare(Param1()(evt,fsm,state));        
    }
};

template <class Container, class Param1, class Param2>
struct StringCompare_<Container,Param1,Param2,void,void> 
                    : euml_action<StringCompare_<Container,Param1,Param2,void,void> >
{
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
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).compare(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).compare(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

template <class Container, class Param1, class Param2, class Param3 >
struct StringCompare_<Container,Param1,Param2,Param3,void> 
                    : euml_action<StringCompare_<Container,Param1,Param2,Param3,void> >
{
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
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).compare(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                              Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).compare(Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                            Param3()(evt,fsm,state));
    }
};

struct string_compare_tag {};
struct StringCompare_Helper: proto::extends< proto::terminal<string_compare_tag>::type, StringCompare_Helper, boost::msm::sm_domain>
{
    StringCompare_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringCompare_<Arg1,Arg2,Arg3,Arg4,Arg5> type;
    };
};
StringCompare_Helper const string_compare_;

template <class Container, class Param1, class Param2, class Param3 >
struct Append_ : euml_action<Append_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).append (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                                      Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).append (Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                                    Param3()(evt,fsm,state));
    }
};
template <class Container, class Param1>
struct Append_<Container,Param1,void,void> 
                    : euml_action<Append_<Container,Param1,void,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).append(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).append(Param1()(evt,fsm,state));        
    }
};

template <class Container, class Param1, class Param2 >
struct Append_<Container,Param1,Param2,void> 
                    : euml_action<Append_<Container,Param1,Param2,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).append(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).append(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

struct append_tag {};
struct Append_Helper: proto::extends< proto::terminal<append_tag>::type, Append_Helper, boost::msm::sm_domain>
{
    Append_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Append_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
Append_Helper const append_;

template <class Container, class Param1, class Param2, class Param3, class Param4 >
struct StringInsert_ : euml_action<StringInsert_<Container,Param1,Param2,Param3,Param4> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).insert(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                                     Param3()(evt,fsm,src,tgt),Param4()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).insert(Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                                   Param3()(evt,fsm,state),Param4()(evt,fsm,state));
    }
};
template <class Container, class Param1, class Param2>
struct StringInsert_ <Container,Param1,Param2,void,void>
                : euml_action<StringInsert_<Container,Param1,Param2,void,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).insert(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).insert(Param1()(evt,fsm,state),Param2()(evt,fsm,state));        
    }
};
template <class Container, class Param1, class Param2, class Param3>
struct StringInsert_<Container,Param1,Param2,Param3,void> 
                    : euml_action<StringInsert_<Container,Param1,Param2,Param3,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).insert(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                                     Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).insert(Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                                   Param3()(evt,fsm,state));
    }
};

struct string_insert_tag {};
struct StringInsert_Helper: proto::extends< proto::terminal<string_insert_tag>::type, StringInsert_Helper, boost::msm::sm_domain>
{
    StringInsert_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringInsert_<Arg1,Arg2,Arg3,Arg4,Arg5> type;
    };
};
StringInsert_Helper const string_insert_;

template <class Container,class Param1, class Param2>
struct StringErase_ : euml_action<StringErase_<Container,Param1,Param2> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            erase(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            erase(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};
template <class Container>
struct StringErase_ <Container,void,void>
                : euml_action<StringErase_<Container,void,void> >

{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).erase();
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).erase();
    }
};

template <class Container,class Param1>
struct StringErase_ <Container,Param1,void>
                : euml_action<StringErase_<Container,Param1,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).erase(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).erase(Param1()(evt,fsm,state));
    }
};

struct string_erase_tag {};
struct StringErase_Helper: proto::extends< proto::terminal<string_erase_tag>::type, StringErase_Helper, boost::msm::sm_domain>
{
    StringErase_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringErase_<Arg1,Arg2,Arg3> type;
    };
};
StringErase_Helper const string_erase_;

template <class Container, class Param1, class Param2, class Param3 >
struct StringAssign_ : euml_action<StringAssign_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).assign (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                                      Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).assign (Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                                    Param3()(evt,fsm,state));
    }
};
template <class Container,class Param1>
struct StringAssign_ < 
        Container,Param1,void,void>
                : euml_action<StringAssign_<Container,Param1,void,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).assign(Param1()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).assign(Param1()(evt,fsm,state));        
    }
};

template <class Container, class Param1, class Param2 >
struct StringAssign_<Container,Param1,Param2,void> 
                    : euml_action<StringAssign_<Container,Param1,Param2,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).assign(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).assign(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};
struct assign_tag {};
struct StringAssign_Helper: proto::extends< proto::terminal<assign_tag>::type, StringAssign_Helper, boost::msm::sm_domain>
{
    StringAssign_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringAssign_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
StringAssign_Helper const string_assign_;

template <class Container,class Param1, class Param2, class Param3, class Param4>
struct StringReplace_ : euml_action<StringReplace_<Container,Param1,Param2,Param3,Param4> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).replace (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                                       Param3()(evt,fsm,src,tgt),Param4()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).replace (Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                                     Param3()(evt,fsm,state),Param4()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringReplace_<Container,Param1,Param2,Param3,void> 
                    : euml_action<StringReplace_<Container,Param1,Param2,Param3,void> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Container,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).replace(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),
                                                      Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).replace(Param1()(evt,fsm,state),Param2()(evt,fsm,state),
                                                    Param3()(evt,fsm,state));        
    }
};

struct string_replace_tag {};
struct StringReplace_Helper: proto::extends< proto::terminal<string_replace_tag>::type, StringReplace_Helper, boost::msm::sm_domain>
{
    StringReplace_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringReplace_<Arg1,Arg2,Arg3,Arg4,Arg5> type;
    };
};
StringReplace_Helper const string_replace_;

template <class Container>
struct CStr_ : euml_action<CStr_<Container> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::add_const<
            typename get_value_type< 
                typename ::boost::remove_reference<
                    typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type>::type* type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::add_const<
            typename get_value_type< 
                typename ::boost::remove_reference<
                    typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type>::type* type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).c_str();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).c_str();        
    }
};
struct c_str_tag {};
struct CStr_Helper: proto::extends< proto::terminal<c_str_tag>::type, CStr_Helper, boost::msm::sm_domain>
{
    CStr_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef CStr_<Arg1> type;
    };
};
CStr_Helper const c_str_;

template <class Container>
struct StringData_ : euml_action<StringData_<Container> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename ::boost::add_const<
            typename get_value_type< 
                typename ::boost::remove_reference<
                    typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type>::type* type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename ::boost::add_const<
            typename get_value_type< 
                typename ::boost::remove_reference<
                    typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type>::type* type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).data();
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Container::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).data();        
    }
};
struct string_data_tag {};
struct StringData_Helper: proto::extends< proto::terminal<string_data_tag>::type, StringData_Helper, boost::msm::sm_domain>
{
    StringData_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringData_<Arg1> type;
    };
};
StringData_Helper const string_data_;

template <class Container, class Param1, class Param2, class Param3, class Enable=void >
struct StringCopy_ : euml_action<StringCopy_<Container,Param1,Param2,Param3,Enable> >
{
};

template <class Container,class Param1, class Param2, class Param3>
struct StringCopy_< 
    Container,Param1,Param2,Param3,
            typename ::boost::enable_if< 
                                typename ::boost::is_same<Param3,void>::type
                                >::type
                >
                : euml_action<StringCopy_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).copy(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).copy(Param1()(evt,fsm,state),Param2()(evt,fsm,state));
    }
};

template <class Container,class Param1, class Param2, class Param3>
struct StringCopy_< 
    Container,Param1,Param2,Param3,
            typename ::boost::disable_if< 
                                typename ::boost::is_same<Param3,void>::type
                                >::type
                >
                : euml_action<StringCopy_<Container,Param1,Param2,Param3> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type2<Container,Event,FSM,STATE>::type>::type>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_size_type< 
            typename ::boost::remove_reference<
                typename get_result_type<Container,EVT,FSM,SourceState,TargetState>::type>::type>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState> 
    typename transition_action_result<EVT,FSM,SourceState,TargetState>::type
       operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return (Container()(evt,fsm,src,tgt)).
            copy(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename state_action_result<Event,FSM,STATE>::type
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return (Container()(evt,fsm,state)).
            copy(Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state));
    }
};

struct string_copy_tag {};
struct StringCopy_Helper: proto::extends< proto::terminal<string_copy_tag>::type, StringCopy_Helper, boost::msm::sm_domain>
{
    StringCopy_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef StringCopy_<Arg1,Arg2,Arg3,Arg4> type;
    };
};
StringCopy_Helper const string_copy_;

}}}}

#endif //BOOST_MSM_FRONT_EUML_CONTAINER_H
