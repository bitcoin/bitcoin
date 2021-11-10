// Copyright 2008 Christophe Henry
// henry UNDERSCORE christophe AT hotmail DOT com
// This is an extended version of the state machine available in the boost::mpl library
// Distributed under the same license as the original.
// Copyright for the original version:
// Copyright 2005 David Abrahams and Aleksey Gurtovoy. Distributed
// under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MSM_BACK_FAVOR_COMPILE_TIME_H
#define BOOST_MSM_BACK_FAVOR_COMPILE_TIME_H

#include <utility>
#include <deque>

#include <boost/mpl/filter_view.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/any.hpp>

#include <boost/msm/common.hpp>
#include <boost/msm/back/metafunctions.hpp>
#include <boost/msm/back/common_types.hpp>
#include <boost/msm/back/dispatch_table.hpp>

namespace boost { namespace msm { namespace back
{

template <class Fsm>
struct process_any_event_helper
{
    process_any_event_helper(msm::back::HandledEnum& res_,Fsm* self_,::boost::any any_event_):
    res(res_),self(self_),any_event(any_event_),finished(false){}
    template <class Event>
    void operator()(boost::msm::wrap<Event> const&)
    {
        if ( ! finished && ::boost::any_cast<Event>(&any_event)!=0)
        {
            finished = true;
            res = self->process_event_internal(::boost::any_cast<Event>(any_event));
        }
    }
private:
    msm::back::HandledEnum&     res;
    Fsm*                        self;
    ::boost::any                any_event;
    bool                        finished;
};

#define BOOST_MSM_BACK_GENERATE_PROCESS_EVENT(fsmname)                                              \
    namespace boost { namespace msm { namespace back{                                               \
    template<>                                                                                      \
    ::boost::msm::back::HandledEnum fsmname::process_any_event( ::boost::any const& any_event)      \
    {                                                                                               \
        typedef ::boost::msm::back::recursive_get_transition_table<fsmname>::type stt;              \
        typedef ::boost::msm::back::generate_event_set<stt>::type stt_events;                       \
        typedef ::boost::msm::back::recursive_get_internal_transition_table<fsmname, ::boost::mpl::true_ >::type istt;    \
        typedef ::boost::msm::back::generate_event_set<create_real_stt<fsmname,istt>::type >::type istt_events;  \
        typedef ::boost::msm::back::set_insert_range<stt_events,istt_events>::type all_events;      \
        ::boost::msm::back::HandledEnum res= ::boost::msm::back::HANDLED_FALSE;                     \
        ::boost::mpl::for_each<all_events, ::boost::msm::wrap< ::boost::mpl::placeholders::_1> >    \
        (::boost::msm::back::process_any_event_helper<fsmname>(res,this,any_event));                \
        return res;                                                                                 \
    }                                                                                               \
    }}}

struct favor_compile_time
{
    typedef int compile_policy;
    typedef ::boost::mpl::false_ add_forwarding_rows;
};

// Generates a singleton runtime lookup table that maps current state
// to a function that makes the SM take its transition on the given
// Event type.
template <class Fsm,class Stt, class Event>
struct dispatch_table < Fsm, Stt, Event, ::boost::msm::back::favor_compile_time>
{
 private:
    // This is a table of these function pointers.
    typedef HandledEnum (*cell)(Fsm&, int,int,Event const&);
    typedef bool (*guard)(Fsm&, Event const&);

    // Compute the maximum state value in the sm so we know how big
    // to make the table
    typedef typename generate_state_set<Stt>::type state_list;
    BOOST_STATIC_CONSTANT(int, max_state = ( ::boost::mpl::size<state_list>::value));

    struct chain_row
    {
        HandledEnum operator()(Fsm& fsm, int region,int state,Event const& evt) const
        {
            HandledEnum res = HANDLED_FALSE;
            typename std::deque<cell>::const_iterator it = one_state.begin();
            while (it != one_state.end() && (res != HANDLED_TRUE && res != HANDLED_DEFERRED ))
            {
                HandledEnum handled = (*it)(fsm,region,state,evt);
                // reject is considered as erasing an error (HANDLED_FALSE)
                if ((HANDLED_FALSE==handled) && (HANDLED_GUARD_REJECT==res) )
                    res = HANDLED_GUARD_REJECT;
                else
                    res = handled;
                ++it;
            }
            return res;
        }
        std::deque<cell> one_state;
    };
    template <class TransitionState>
    static HandledEnum call_submachine(Fsm& fsm, int , int , Event const& evt)
    {
        return (fsm.template get_state<TransitionState&>()).process_any_event( ::boost::any(evt));
    }
    // A function object for use with mpl::for_each that stuffs
    // transitions into cells.
    struct init_cell
    {
        init_cell(dispatch_table* self_)
          : self(self_)
        {}
        // version for transition event not base of our event
        template <class Transition>
        typename ::boost::disable_if<
            typename ::boost::is_same<typename Transition::current_state_type,Fsm>::type
        ,void>::type
        init_event_base_case(Transition const&, ::boost::mpl::true_ const &) const
        {
            typedef typename create_stt<Fsm>::type stt;
            BOOST_STATIC_CONSTANT(int, state_id =
                (get_state_id<stt,typename Transition::current_state_type>::value));
            self->entries[state_id+1].one_state.push_front(reinterpret_cast<cell>(&Transition::execute));
        }
        template <class Transition>
        typename ::boost::enable_if<
            typename ::boost::is_same<typename Transition::current_state_type,Fsm>::type
        ,void>::type
        init_event_base_case(Transition const&, ::boost::mpl::true_ const &) const
        {
            self->entries[0].one_state.push_front(reinterpret_cast<cell>(&Transition::execute));
        }

        // version for transition event base of our event
        template <class Transition>
        typename ::boost::disable_if<
            typename ::boost::is_same<typename Transition::current_state_type,Fsm>::type
        ,void>::type
        init_event_base_case(Transition const&, ::boost::mpl::false_ const &) const
        {
            typedef typename create_stt<Fsm>::type stt;
            BOOST_STATIC_CONSTANT(int, state_id =
                (get_state_id<stt,typename Transition::current_state_type>::value));
            self->entries[state_id+1].one_state.push_front(&Transition::execute);
        }
        template <class Transition>
        typename ::boost::enable_if<
            typename ::boost::is_same<typename Transition::current_state_type,Fsm>::type
        ,void>::type
        init_event_base_case(Transition const&, ::boost::mpl::false_ const &) const
        {
            self->entries[0].one_state.push_front(&Transition::execute);
        }
        // Cell initializer function object, used with mpl::for_each
        template <class Transition>
        typename ::boost::enable_if<typename has_not_real_row_tag<Transition>::type,void >::type
            operator()(Transition const&,boost::msm::back::dummy<0> = 0) const
        {
            // version for not real rows. No problem because irrelevant for process_event
        }
        template <class Transition>
        typename ::boost::disable_if<typename has_not_real_row_tag<Transition>::type,void >::type
        operator()(Transition const& tr,boost::msm::back::dummy<1> = 0) const
        {
            //only if the transition event is a base of our event is the reinterpret_case safe
            init_event_base_case(tr,
                ::boost::mpl::bool_<
                    ::boost::is_base_of<typename Transition::transition_event,Event>::type::value>() );
        }

        dispatch_table* self;
    };

    // Cell default-initializer function object, used with mpl::for_each
    // initializes with call_no_transition, defer_transition or default_eventless_transition
    // variant for non-anonymous transitions
    template <class EventType,class Enable=void>
    struct default_init_cell
    {
        default_init_cell(dispatch_table* self_,chain_row* tofill_entries_)
            : self(self_),tofill_entries(tofill_entries_)
        {}
        template <bool deferred,bool composite, int some_dummy=0>
        struct helper
        {};
        template <int some_dummy> struct helper<true,false,some_dummy>
        {
            template <class State>
            static void execute(boost::msm::wrap<State> const&,chain_row* tofill)
            {
                typedef typename create_stt<Fsm>::type stt;
                BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
                cell call_no_transition = &Fsm::defer_transition;
                tofill[state_id+1].one_state.push_back(call_no_transition);
            }
        };
        template <int some_dummy> struct helper<true,true,some_dummy>
        {
            template <class State>
            static void execute(boost::msm::wrap<State> const&,chain_row* tofill)
            {
                typedef typename create_stt<Fsm>::type stt;
                BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
                cell call_no_transition = &Fsm::defer_transition;
                tofill[state_id+1].one_state.push_back(call_no_transition);
            }
        };
        template <int some_dummy> struct helper<false,true,some_dummy>
        {
            template <class State>
            static
            typename ::boost::enable_if<
                typename ::boost::is_same<State,Fsm>::type
            ,void>::type
            execute(boost::msm::wrap<State> const&,chain_row* tofill,boost::msm::back::dummy<0> = 0)
            {
                // for internal tables
                cell call_no_transition_internal = &Fsm::call_no_transition;
                tofill[0].one_state.push_front(call_no_transition_internal);
            }
            template <class State>
            static
            typename ::boost::disable_if<
                typename ::boost::is_same<State,Fsm>::type
            ,void>::type
            execute(boost::msm::wrap<State> const&,chain_row* tofill,boost::msm::back::dummy<1> = 0)
            {
                typedef typename create_stt<Fsm>::type stt;
                BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
                cell call_no_transition = &call_submachine< State >;
                tofill[state_id+1].one_state.push_front(call_no_transition);
            }
        };
        template <int some_dummy> struct helper<false,false,some_dummy>
        {
            template <class State>
            static void execute(boost::msm::wrap<State> const&,chain_row* tofill)
            {
                typedef typename create_stt<Fsm>::type stt;
                BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
                cell call_no_transition = &Fsm::call_no_transition;
                tofill[state_id+1].one_state.push_back(call_no_transition);
            }
        };
        template <class State>
        void operator()(boost::msm::wrap<State> const& s)
        {
            helper<has_state_delayed_event<State,Event>::type::value,
                   is_composite_state<State>::type::value>::execute(s,tofill_entries);
        }
        dispatch_table* self;
        chain_row* tofill_entries;
    };

    // variant for anonymous transitions
    template <class EventType>
    struct default_init_cell<EventType,
                             typename ::boost::enable_if<
                                typename is_completion_event<EventType>::type>::type>
    {
        default_init_cell(dispatch_table* self_,chain_row* tofill_entries_)
            : self(self_),tofill_entries(tofill_entries_)
        {}

        // this event is a compound one (not a real one, just one for use in event-less transitions)
        // Note this event cannot be used as deferred!
        template <class State>
        void operator()(boost::msm::wrap<State> const&)
        {
            typedef typename create_stt<Fsm>::type stt;
            BOOST_STATIC_CONSTANT(int, state_id = (get_state_id<stt,State>::value));
            cell call_no_transition = &Fsm::default_eventless_transition;
            tofill_entries[state_id+1].one_state.push_back(call_no_transition);
        }

        dispatch_table* self;
        chain_row* tofill_entries;
    };

 public:
    // initialize the dispatch table for a given Event and Fsm
    dispatch_table()
    {
        // Initialize cells for no transition
        ::boost::mpl::for_each<
            ::boost::mpl::filter_view<
                    Stt, ::boost::is_base_of<transition_event< ::boost::mpl::placeholders::_>, Event> > >
        (init_cell(this));

        ::boost::mpl::for_each<
            typename generate_state_set<Stt>::type,
            boost::msm::wrap< ::boost::mpl::placeholders::_1> >
         (default_init_cell<Event>(this,entries));

    }

    // The singleton instance.
    static const dispatch_table instance;

 public: // data members
     chain_row entries[max_state+1];
};

template <class Fsm,class Stt, class Event>
const boost::msm::back::dispatch_table<Fsm,Stt, Event,favor_compile_time>
dispatch_table<Fsm,Stt, Event,favor_compile_time>::instance;

}}} // boost::msm::back

#endif //BOOST_MSM_BACK_FAVOR_COMPILE_TIME_H
