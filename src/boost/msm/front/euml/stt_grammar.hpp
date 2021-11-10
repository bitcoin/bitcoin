// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_FRONT_EUML_STT_GRAMMAR_H
#define BOOST_MSM_FRONT_EUML_STT_GRAMMAR_H

#include <boost/msm/front/euml/common.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/eval_if.hpp>

#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/front/euml/guard_grammar.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>

namespace proto = boost::proto;

namespace boost { namespace msm { namespace front { namespace euml
{

template <class SOURCE,class EVENT,class TARGET,class ACTION=none,class GUARD=none>
struct TempRow
{
    typedef SOURCE  Source;
    typedef EVENT   Evt;
    typedef TARGET  Target;
    typedef ACTION  Action;
    typedef GUARD   Guard;
};

template <class TEMP_ROW>
struct convert_to_row
{
    typedef Row<typename TEMP_ROW::Source,typename TEMP_ROW::Evt,typename TEMP_ROW::Target,
                typename TEMP_ROW::Action,typename TEMP_ROW::Guard> type;
};
template <class TEMP_ROW>
struct convert_to_internal_row
{
    typedef Internal<typename TEMP_ROW::Evt,
                     typename TEMP_ROW::Action,typename TEMP_ROW::Guard> type;
};
// explicit + fork + entry point + exit point grammar
struct BuildEntry
    : proto::or_<
        proto::when<
                    proto::function<proto::terminal<proto::_>,proto::terminal<state_tag>,proto::terminal<state_tag> >,
                    get_fct<proto::_child_c<0>,get_state_name<proto::_child_c<1>() >(),get_state_name<proto::_child_c<2>() >() >()
        >
    >
{};

// row grammar
struct BuildNextStates
   : proto::or_<
        proto::when<
                    proto::terminal<state_tag>,
                    get_state_name<proto::_>()
        >,
        proto::when<
                      BuildEntry,
                      BuildEntry
        >,
        proto::when<
                    proto::comma<BuildEntry,BuildEntry >,
                    ::boost::mpl::push_back<
                        make_vector_one_row<BuildEntry(proto::_left)>(),
                        BuildEntry(proto::_right)>()
        >,
        proto::when <
                    proto::comma<BuildNextStates,BuildEntry >,
                    ::boost::mpl::push_back<
                                BuildNextStates(proto::_left),
                                BuildEntry(proto::_right) >()                
        >
   >
{};

template <class EventGuard,class ActionClass>
struct fusion_event_action_guard 
{
    typedef TempRow<none,typename EventGuard::Evt,none,typename ActionClass::Action,typename EventGuard::Guard> type;
};

template <class SourceGuard,class ActionClass>
struct fusion_source_action_guard 
{
    typedef TempRow<typename SourceGuard::Source,none,none,typename ActionClass::Action,typename SourceGuard::Guard> type;
};

template <class SourceClass,class EventClass>
struct fusion_source_event_action_guard 
{
    typedef TempRow<typename SourceClass::Source,typename EventClass::Evt,
                    none,typename EventClass::Action,typename EventClass::Guard> type;
};
template <class Left,class Right>
struct fusion_left_right 
{
    typedef TempRow<typename Right::Source,typename Right::Evt,typename Left::Target
                   ,typename Right::Action,typename Right::Guard> type;
};

struct BuildEventPlusGuard
    : proto::or_<
        proto::when<
            proto::subscript<proto::terminal<event_tag>, GuardGrammar >,
            TempRow<none,proto::_left,none,none, GuardGrammar(proto::_right)>(proto::_right)
        >
    >
 {};

struct BuildSourceState
   : proto::or_<
        proto::when<
                    proto::terminal<state_tag>,
                    get_state_name<proto::_>()
        >,
        proto::when<
                    BuildEntry,
                    BuildEntry
        >
   >
{};

struct BuildSourcePlusGuard
    : proto::when<
            proto::subscript<BuildSourceState,GuardGrammar >,
            TempRow<BuildSourceState(proto::_left),none,none,none,GuardGrammar(proto::_right)>(proto::_right)
        >
{};

struct BuildEvent
    : proto::or_<
        // just event without guard/action
         proto::when<
                proto::terminal<event_tag>,
                TempRow<none,proto::_,none>() >
        // event / action
       , proto::when<
                proto::divides<proto::terminal<event_tag>,ActionGrammar >,
                TempRow<none,proto::_left,none,ActionGrammar(proto::_right) >(proto::_right) >
        // event [ guard ]
       , proto::when<
                proto::subscript<proto::terminal<event_tag>,GuardGrammar >,
                TempRow<none,proto::_left,none,none,GuardGrammar(proto::_right)>(proto::_right) >
        // event [ guard ] / action 
       , proto::when<
                proto::divides<BuildEventPlusGuard, ActionGrammar>,
                fusion_event_action_guard<BuildEventPlusGuard(proto::_left),
                                          TempRow<none,none,none,ActionGrammar(proto::_right)>(proto::_right)
                                           >() 
                >
        >
{};
struct BuildSource
    : proto::or_<
        // after == if just state without event or guard/action
         proto::when<
                BuildSourceState,
                TempRow<BuildSourceState(proto::_),none,none>() >
        // == source / action
       , proto::when<
                proto::divides<BuildSourceState,ActionGrammar >,
                TempRow<BuildSourceState(proto::_left),none,none,ActionGrammar(proto::_right) >(proto::_right) >
        // == source [ guard ]
       , proto::when<
                proto::subscript<BuildSourceState,GuardGrammar >,
                TempRow<BuildSourceState(proto::_left),none,none,none,GuardGrammar(proto::_right)>(proto::_right) >
        // == source [ guard ] / action 
       , proto::when<
                proto::divides<BuildSourcePlusGuard,
                               ActionGrammar >,
                fusion_source_action_guard<BuildSourcePlusGuard(proto::_left),
                                           TempRow<none,none,none,ActionGrammar(proto::_right)>(proto::_right)
                                           >() 
                >
        >
{};

struct BuildRight
    : proto::or_<
         proto::when<
                proto::plus<BuildSource,BuildEvent >,
                fusion_source_event_action_guard<BuildSource(proto::_left),BuildEvent(proto::_right)>()
         >,
         proto::when<
                BuildSource,
                BuildSource
         >
    >
{};

struct BuildRow
    : proto::or_<
        // grammar 1
        proto::when<
            proto::equal_to<BuildNextStates,BuildRight >,
            convert_to_row<
                fusion_left_right<TempRow<none,none,BuildNextStates(proto::_left)>,BuildRight(proto::_right)> >()
        >,
        // internal events
        proto::when<
            BuildRight,
            convert_to_row<
                fusion_left_right<TempRow<none,none,none>,BuildRight(proto::_)> >()
        >,
        // grammar 2
        proto::when<
            proto::equal_to<BuildRight,BuildNextStates>,
            convert_to_row<
                fusion_left_right<TempRow<none,none,BuildNextStates(proto::_right)>,BuildRight(proto::_left)> >()
        >
    >
{};

// stt grammar
struct BuildStt
   : proto::or_<
        proto::when<
                    proto::comma<BuildStt,BuildStt>,
                    boost::mpl::push_back<BuildStt(proto::_left),BuildRow(proto::_right)>()
                >,
        proto::when <
                    BuildRow,
                    make_vector_one_row<BuildRow(proto::_)>()
        >
   >
{};

template <class Expr>
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr,BuildStt>::type,
    boost::result_of<BuildStt(Expr)>,
    make_invalid_type>::type
build_stt(Expr const&)
{
    return typename boost::result_of<BuildStt(Expr)>::type();
}

// internal stt grammar
struct BuildInternalRow
    :   proto::when<
            BuildEvent,
            convert_to_internal_row<
                fusion_left_right<TempRow<none,none,none>,BuildEvent(proto::_)> >()
        >
{};
struct BuildInternalStt
   : proto::or_<
        proto::when<
                    proto::comma<BuildInternalStt,BuildInternalStt>,
                    boost::mpl::push_back<BuildInternalStt(proto::_left),BuildInternalRow(proto::_right)>()
                >,
        proto::when <
                    BuildInternalRow,
                    make_vector_one_row<BuildInternalRow(proto::_)>()
        >
   >
{};

template <class Expr>
typename ::boost::mpl::eval_if<
    typename proto::matches<Expr,BuildInternalStt>::type,
    boost::result_of<BuildInternalStt(Expr)>,
    make_invalid_type>::type
build_internal_stt(Expr const&)
{
    return typename boost::result_of<BuildInternalStt(Expr)>::type();
}


}}}}
#endif //BOOST_MSM_FRONT_EUML_STT_GRAMMAR_H
