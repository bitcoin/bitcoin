// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_TRANSFORMATION_H
#define BOOST_MSM_FRONT_EUML_TRANSFORMATION_H

#include <algorithm>
#include <boost/msm/front/euml/common.hpp>

namespace boost { namespace msm { namespace front { namespace euml
{
#ifdef __STL_CONFIG_H
BOOST_MSM_EUML_FUNCTION(FillN_ , std::fill_n , fill_n_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(Rotate_ , std::rotate , rotate_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(GenerateN_ , std::generate_n , generate_n_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )

#else
BOOST_MSM_EUML_FUNCTION(FillN_ , std::fill_n , fill_n_ , void , void )
BOOST_MSM_EUML_FUNCTION(Rotate_ , std::rotate , rotate_ , void , void )
BOOST_MSM_EUML_FUNCTION(GenerateN_ , std::generate_n , generate_n_ , void , void )
#endif

BOOST_MSM_EUML_FUNCTION(Copy_ , std::copy , copy_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(CopyBackward_ , std::copy_backward , copy_backward_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(Reverse_ , std::reverse , reverse_ , void , void )
BOOST_MSM_EUML_FUNCTION(ReverseCopy_ , std::reverse_copy , reverse_copy_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(Remove_ , std::remove , remove_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(RemoveIf_ , std::remove_if , remove_if_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(RemoveCopy_ , std::remove_copy , remove_copy_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(RemoveCopyIf_ , std::remove_copy_if , remove_copy_if_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(Fill_ , std::fill , fill_ , void , void )
BOOST_MSM_EUML_FUNCTION(Generate_ , std::generate , generate_ , void , void )
BOOST_MSM_EUML_FUNCTION(Unique_ , std::unique , unique_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(UniqueCopy_ , std::unique_copy , unique_copy_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
#if __cplusplus < 201703L
BOOST_MSM_EUML_FUNCTION(RandomShuffle_ , std::random_shuffle , random_shuffle_ , void , void )
#endif
#if __cplusplus >= 201103L
BOOST_MSM_EUML_FUNCTION(Shuffle_ , std::shuffle , shuffle_ , void , void )
#endif
BOOST_MSM_EUML_FUNCTION(RotateCopy_ , std::rotate_copy , rotate_copy_ , RESULT_TYPE_PARAM4 , RESULT_TYPE2_PARAM4 )
BOOST_MSM_EUML_FUNCTION(Partition_ , std::partition , partition_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(StablePartition_ , std::stable_partition , stable_partition_ , RESULT_TYPE_PARAM1 , RESULT_TYPE2_PARAM1 )
BOOST_MSM_EUML_FUNCTION(Sort_ , std::sort , sort_ , void , void )
BOOST_MSM_EUML_FUNCTION(StableSort_ , std::stable_sort , stable_sort_ , void , void )
BOOST_MSM_EUML_FUNCTION(PartialSort_ , std::partial_sort , partial_sort_ , void , void )
BOOST_MSM_EUML_FUNCTION(PartialSortCopy_ , std::partial_sort_copy , partial_sort_copy_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(NthElement_ , std::nth_element , nth_element_ , void , void )
BOOST_MSM_EUML_FUNCTION(Merge_ , std::merge , merge_ , RESULT_TYPE_PARAM5 , RESULT_TYPE2_PARAM5 )
BOOST_MSM_EUML_FUNCTION(InplaceMerge_ , std::inplace_merge , inplace_merge_ , void , void )
BOOST_MSM_EUML_FUNCTION(SetUnion_ , std::set_union , set_union_ , RESULT_TYPE_PARAM5 , RESULT_TYPE2_PARAM5 )
BOOST_MSM_EUML_FUNCTION(PushHeap_ , std::push_heap , push_heap_ , void , void )
BOOST_MSM_EUML_FUNCTION(PopHeap_ , std::pop_heap , pop_heap_ , void , void )
BOOST_MSM_EUML_FUNCTION(MakeHeap_ , std::make_heap , make_heap_ , void , void )
BOOST_MSM_EUML_FUNCTION(SortHeap_ , std::sort_heap , sort_heap_ , void , void )
BOOST_MSM_EUML_FUNCTION(NextPermutation_ , std::next_permutation , next_permutation_ , bool , bool )
BOOST_MSM_EUML_FUNCTION(PrevPermutation_ , std::prev_permutation , prev_permutation_ , bool , bool )
BOOST_MSM_EUML_FUNCTION(InnerProduct_ , std::inner_product , inner_product_ , RESULT_TYPE_PARAM4 , RESULT_TYPE2_PARAM4 )
BOOST_MSM_EUML_FUNCTION(PartialSum_ , std::partial_sum , partial_sum_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(AdjacentDifference_ , std::adjacent_difference , adjacent_difference_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(Replace_ , std::replace , replace_ , void , void )
BOOST_MSM_EUML_FUNCTION(ReplaceIf_ , std::replace_if , replace_if_ , void , void )
BOOST_MSM_EUML_FUNCTION(ReplaceCopy_ , std::replace_copy , replace_copy_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )
BOOST_MSM_EUML_FUNCTION(ReplaceCopyIf_ , std::replace_copy_if , replace_copy_if_ , RESULT_TYPE_PARAM3 , RESULT_TYPE2_PARAM3 )



template <class T>
struct BackInserter_ : euml_action<BackInserter_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef std::back_insert_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type> type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef std::back_insert_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type> type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return std::back_inserter(T()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return std::back_inserter(T()(evt,fsm,state));
    }
};

struct back_inserter_tag {};
struct BackInserter_Helper: proto::extends< proto::terminal<back_inserter_tag>::type, BackInserter_Helper, boost::msm::sm_domain>
{
    BackInserter_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef BackInserter_<Arg1> type;
    };
};
BackInserter_Helper const back_inserter_;

template <class T>
struct FrontInserter_ : euml_action<FrontInserter_<T> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef std::front_insert_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type> type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef std::front_insert_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type> type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return std::front_inserter(T()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return std::front_inserter(T()(evt,fsm,state));
    }
};

struct front_inserter_tag {};
struct FrontInserter_Helper: proto::extends< proto::terminal<front_inserter_tag>::type, FrontInserter_Helper, boost::msm::sm_domain>
{
    FrontInserter_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef FrontInserter_<Arg1> type;
    };
};
FrontInserter_Helper const front_inserter_;

template <class T,class Pos>
struct Inserter_ : euml_action<Inserter_<T,Pos> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef std::insert_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type2<T,Event,FSM,STATE>::type>::type> type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef std::insert_iterator< 
            typename ::boost::remove_reference<
                typename get_result_type<T,EVT,FSM,SourceState,TargetState>::type>::type> type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
        operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return std::inserter(T()(evt,fsm,src,tgt),Pos()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename T::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
        operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return std::inserter(T()(evt,fsm,state),Pos()(evt,fsm,state));
    }
};

struct inserter_tag {};
struct Inserter_Helper: proto::extends< proto::terminal<inserter_tag>::type, Inserter_Helper, boost::msm::sm_domain>
{
    Inserter_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Inserter_<Arg1,Arg2> type;
    };
};
Inserter_Helper const inserter_;

template <class Param1, class Param2, class Param3, class Param4, class Param5, class Enable=void >
struct Transform_ : euml_action<Transform_<Param1,Param2,Param3,Param4,Param5,Enable> >
{
};

template <class Param1, class Param2, class Param3, class Param4, class Param5>
struct Transform_<Param1,Param2,Param3,Param4,Param5,
                  typename ::boost::enable_if<typename ::boost::is_same<Param5,void>::type >::type> 
                    : euml_action<Transform_<Param1,Param2,Param3,Param4,Param5> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Param3,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Param3,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Param1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return std::transform(Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt),
                              Param4()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Param1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return std::transform(Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state),
                              Param4()(evt,fsm,state));        
    }
};

template <class Param1, class Param2, class Param3, class Param4, class Param5>
struct Transform_<Param1,Param2,Param3,Param4,Param5,
               typename ::boost::disable_if<typename ::boost::is_same<Param5,void>::type >::type> 
                    : euml_action<Transform_<Param1,Param2,Param3,Param4,Param5> >
{
    template <class Event,class FSM,class STATE >
    struct state_action_result 
    {
        typedef typename get_result_type2<Param4,Event,FSM,STATE>::type type;
    };
    template <class EVT,class FSM,class SourceState,class TargetState>
    struct transition_action_result 
    {
        typedef typename get_result_type<Param4,EVT,FSM,SourceState,TargetState>::type type;
    };
    typedef ::boost::mpl::set<state_action_tag,action_tag> tag_type;

    template <class EVT,class FSM,class SourceState,class TargetState>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Param1::tag_type,action_tag>::type,
            typename transition_action_result<EVT,FSM,SourceState,TargetState>::type >::type 
     operator()(EVT const& evt, FSM& fsm,SourceState& src,TargetState& tgt)const
    {
        return std::transform (Param1()(evt,fsm,src,tgt),Param2()(evt,fsm,src,tgt),Param3()(evt,fsm,src,tgt),
                               Param4()(evt,fsm,src,tgt),Param5()(evt,fsm,src,tgt));
    }
    template <class Event,class FSM,class STATE>
    typename ::boost::enable_if<
        typename ::boost::mpl::has_key<
            typename Param1::tag_type,state_action_tag>::type,
            typename state_action_result<Event,FSM,STATE>::type >::type 
     operator()(Event const& evt,FSM& fsm,STATE& state )const
    {
        return std::transform (Param1()(evt,fsm,state),Param2()(evt,fsm,state),Param3()(evt,fsm,state),
                               Param4()(evt,fsm,state),Param5()(evt,fsm,state));
    }
};
struct transform_tag {};
struct Transform_Helper: proto::extends< proto::terminal<transform_tag>::type, Transform_Helper, boost::msm::sm_domain>
{
    Transform_Helper(){}
    template <class Arg1,class Arg2,class Arg3,class Arg4,class Arg5 
#ifdef BOOST_MSVC 
 ,class Arg6 
#endif
>
    struct In
    {
        typedef Transform_<Arg1,Arg2,Arg3,Arg4,Arg5> type;
    };
};
Transform_Helper const transform_;

}}}}

#endif //BOOST_MSM_FRONT_EUML_TRANSFORMATION_H
