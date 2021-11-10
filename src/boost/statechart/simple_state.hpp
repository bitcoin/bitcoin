#ifndef BOOST_STATECHART_SIMPLE_STATE_HPP_INCLUDED
#define BOOST_STATECHART_SIMPLE_STATE_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2010 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event.hpp>

#include <boost/statechart/detail/leaf_state.hpp>
#include <boost/statechart/detail/node_state.hpp>
#include <boost/statechart/detail/constructor.hpp>
#include <boost/statechart/detail/memory.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/clear.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>

#include <boost/mpl/plus.hpp>
#include <boost/mpl/max_element.hpp>
#include <boost/mpl/greater.hpp>

#include <boost/get_pointer.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/assert.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/polymorphic_cast.hpp> // boost::polymorphic_downcast

#include <cstddef> // std::size_t



namespace boost
{
namespace statechart
{
namespace detail
{



//////////////////////////////////////////////////////////////////////////////
template< class T >
struct make_list : public mpl::eval_if<
  mpl::is_sequence< T >,
  mpl::identity< T >,
  mpl::identity< mpl::list< T > > > {};

//////////////////////////////////////////////////////////////////////////////
template< class MostDerived, class Context, class InnerInitial >
struct simple_state_base_type
{
  private:
    typedef typename Context::outermost_context_base_type::allocator_type
      allocator_type;
    typedef typename Context::outermost_context_base_type::rtti_policy_type
      rtti_policy_type;
    typedef typename detail::make_list< InnerInitial >::type
      inner_initial_list;
    typedef typename mpl::size< inner_initial_list >::type
      inner_initial_list_size;

  public:
    typedef typename mpl::eval_if<
      mpl::empty< inner_initial_list >,
      mpl::identity< typename rtti_policy_type::
        template rtti_derived_type< MostDerived, leaf_state<
          allocator_type,
          rtti_policy_type > > >,
      mpl::identity< typename rtti_policy_type::
        template rtti_derived_type< MostDerived, node_state<
          inner_initial_list_size,
          allocator_type,
          rtti_policy_type > > > >::type type;
};


//////////////////////////////////////////////////////////////////////////////
struct no_transition_function
{
  template< class CommonContext >
  void operator()( CommonContext & ) const {}
};

template< class TransitionContext, class Event >
class transition_function
{
  public:
    transition_function(
      void ( TransitionContext::*pTransitionAction )( const Event & ),
      const Event & evt
    ) :
      pTransitionAction_( pTransitionAction ),
      evt_( evt )
    {
    }

    template< class CommonContext >
    void operator()( CommonContext & commonContext ) const
    {
      ( commonContext.template context< TransitionContext >()
        .*pTransitionAction_ )( evt_ );
    }

  private:
    // avoids C4512 (assignment operator could not be generated)
    transition_function & operator=( const transition_function & );

    void ( TransitionContext::*pTransitionAction_ )( const Event & );
    const Event & evt_;
};


template< bool contextHasInheritedDeepHistory, bool contextHasDeepHistory >
struct deep_history_storer
{
  template< class HistorizedState, class LeafState, class Context >
  static void store_deep_history( Context & ) {}
};

template<>
struct deep_history_storer< true, false >
{
  template< class HistorizedState, class LeafState, class Context >
  static void store_deep_history( Context & ctx )
  {
    ctx.template store_deep_history_impl< LeafState >();
  }
};

template<>
struct deep_history_storer< true, true >
{
  template< class HistorizedState, class LeafState, class Context >
  static void store_deep_history( Context & ctx )
  {
    ctx.outermost_context_base().template store_deep_history<
      HistorizedState, LeafState >();
    ctx.template store_deep_history_impl< LeafState >();
  }
};



} // namespace detail



//////////////////////////////////////////////////////////////////////////////
enum history_mode
{
  has_no_history,
  has_shallow_history,
  has_deep_history,
  has_full_history // shallow & deep
};



//////////////////////////////////////////////////////////////////////////////
template< class MostDerived,
          class Context,
          class InnerInitial = mpl::list<>,
          history_mode historyMode = has_no_history >
class simple_state : public detail::simple_state_base_type< MostDerived,
  typename Context::inner_context_type, InnerInitial >::type
{
  typedef typename detail::simple_state_base_type<
    MostDerived, typename Context::inner_context_type,
    InnerInitial >::type base_type;

  public:
    //////////////////////////////////////////////////////////////////////////
    typedef mpl::list<> reactions;

    typedef typename Context::inner_context_type context_type;

    template< detail::orthogonal_position_type innerOrthogonalPosition >
    struct orthogonal
    {
      typedef mpl::integral_c<
        detail::orthogonal_position_type,
        innerOrthogonalPosition > inner_orthogonal_position;
      typedef MostDerived inner_context_type;
    };

    typedef typename context_type::outermost_context_type
      outermost_context_type;

    outermost_context_type & outermost_context()
    {
      // This assert fails when an attempt is made to access the state machine
      // from a constructor of a state that is *not* a subtype of state<>.
      // To correct this, derive from state<> instead of simple_state<>.
      BOOST_ASSERT( get_pointer( pContext_ ) != 0 );
      return pContext_->outermost_context();
    }

    const outermost_context_type & outermost_context() const
    {
      // This assert fails when an attempt is made to access the state machine
      // from a constructor of a state that is *not* a subtype of state<>.
      // To correct this, derive from state<> instead of simple_state<>.
      BOOST_ASSERT( get_pointer( pContext_ ) != 0 );
      return pContext_->outermost_context();
    }

    template< class OtherContext >
    OtherContext & context()
    {
      typedef typename mpl::if_<
        is_base_of< OtherContext, MostDerived >,
        context_impl_this_context,
        context_impl_other_context
      >::type impl;
      return impl::template context_impl< OtherContext >( *this );
    }

    template< class OtherContext >
    const OtherContext & context() const
    {
      typedef typename mpl::if_<
        is_base_of< OtherContext, MostDerived >,
        context_impl_this_context,
        context_impl_other_context
      >::type impl;
      return impl::template context_impl< OtherContext >( *this );
    }

    template< class Target >
    Target state_cast() const
    {
      return outermost_context_base().template state_cast< Target >();
    }

    template< class Target >
    Target state_downcast() const
    {
      return outermost_context_base().template state_downcast< Target >();
    }

    typedef typename context_type::state_base_type state_base_type;
    typedef typename context_type::state_iterator state_iterator;

    state_iterator state_begin() const
    {
      return outermost_context_base().state_begin();
    }

    state_iterator state_end() const
    {
      return outermost_context_base().state_end();
    }


    typedef typename context_type::event_base_ptr_type event_base_ptr_type;

    void post_event( const event_base_ptr_type & pEvent )
    {
      outermost_context_base().post_event_impl( pEvent );
    }

    void post_event( const event_base & evt )
    {
      outermost_context_base().post_event_impl( evt );
    }

    result discard_event()
    {
      return detail::result_utility::make_result( detail::do_discard_event );
    }

    result forward_event()
    {
      return detail::result_utility::make_result( detail::do_forward_event );
    }

    result defer_event()
    {
      this->state_base_type::defer_event();
      return detail::result_utility::make_result( detail::do_defer_event );
    }

    template< class DestinationState >
    result transit()
    {
      return transit_impl< DestinationState, outermost_context_type >(
        detail::no_transition_function() );
    }

    template< class DestinationState, class TransitionContext, class Event >
    result transit(
      void ( TransitionContext::*pTransitionAction )( const Event & ),
      const Event & evt )
    {
      return transit_impl< DestinationState, TransitionContext >(
        detail::transition_function< TransitionContext, Event >(
          pTransitionAction, evt ) );
    }

    result terminate()
    {
      outermost_context_base().terminate_as_reaction( *this );
      return detail::result_utility::make_result( detail::do_discard_event );
    }

    template<
      class HistoryContext,
      detail::orthogonal_position_type orthogonalPosition >
    void clear_shallow_history()
    {
      outermost_context_base().template clear_shallow_history<
        HistoryContext, orthogonalPosition >();
    }

    template<
      class HistoryContext,
      detail::orthogonal_position_type orthogonalPosition >
    void clear_deep_history()
    {
      outermost_context_base().template clear_deep_history<
        HistoryContext, orthogonalPosition >();
    }

    const event_base * triggering_event() const
    {
      return outermost_context_base().triggering_event();
    }

  protected:
    //////////////////////////////////////////////////////////////////////////
    simple_state() : pContext_( 0 ) {}

    ~simple_state()
    {
      // As a result of a throwing derived class constructor, this destructor
      // can be called before the context is set.
      if ( get_pointer( pContext_ ) != 0 )
      {
        if ( this->deferred_events() )
        {
          outermost_context_base().release_events();
        }

        pContext_->remove_inner_state( orthogonal_position::value );
      }
    }

  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    typedef typename Context::inner_orthogonal_position orthogonal_position;

    // If you receive a
    // "use of undefined type 'boost::STATIC_ASSERTION_FAILURE<x>'" or similar
    // compiler error here then either this state resides in a non-existent
    // orthogonal region of the outer state or the outer state does not have
    // inner states.
    BOOST_STATIC_ASSERT( ( mpl::less<
      orthogonal_position,
      typename context_type::no_of_orthogonal_regions >::value ) );

    typedef MostDerived inner_context_type;
    typedef mpl::integral_c< detail::orthogonal_position_type, 0 >
      inner_orthogonal_position;

    typedef typename context_type::event_base_type event_base_type;
    typedef typename context_type::rtti_policy_type rtti_policy_type;

    typedef typename context_type::outermost_context_base_type
      outermost_context_base_type;
    typedef typename context_type::inner_context_ptr_type context_ptr_type;
    typedef typename context_type::state_list_type state_list_type;
    typedef intrusive_ptr< inner_context_type > inner_context_ptr_type;
    typedef typename detail::make_list< InnerInitial >::type
      inner_initial_list;
    typedef typename mpl::size< inner_initial_list >::type
      inner_initial_list_size;
    typedef mpl::integral_c<
      detail::orthogonal_position_type,
      inner_initial_list_size::value > no_of_orthogonal_regions;
    typedef typename mpl::push_front<
      typename context_type::context_type_list,
      context_type >::type context_type_list;

    // If you receive a
    // "use of undefined type 'boost::STATIC_ASSERTION_FAILURE<x>'" or similar
    // compiler error here then the direct or indirect context of this state
    // has deep history _and_ this state has two or more orthogonal regions.
    // Boost.Statechart does not currently support deep history in a state whose
    // direct or indirect inner states have two or more orthogonal regions.
    // Please consult the documentation on how to work around this limitation.
    BOOST_STATIC_ASSERT( ( mpl::or_<
      mpl::less<
        no_of_orthogonal_regions,
        mpl::integral_c< detail::orthogonal_position_type, 2 > >,
      mpl::not_<
        typename context_type::inherited_deep_history > >::value ) );

    typedef mpl::bool_< ( historyMode & has_shallow_history ) != 0 >
      shallow_history;
    typedef typename context_type::shallow_history stores_shallow_history;

    typedef mpl::bool_< ( historyMode & has_deep_history ) != 0 >
      deep_history;
    typedef typename mpl::or_<
      deep_history, 
      typename context_type::inherited_deep_history
    >::type inherited_deep_history;
    typedef typename mpl::and_<
      inherited_deep_history,
      mpl::empty< inner_initial_list > >::type stores_deep_history;

    void * operator new( std::size_t size )
    {
      return detail::allocate< MostDerived,
        typename outermost_context_type::allocator_type >( size );
    }

    void operator delete( void * pState )
    {
      detail::deallocate< MostDerived,
        typename outermost_context_type::allocator_type >( pState );
    }

    outermost_context_base_type & outermost_context_base()
    {
      // This assert fails when an attempt is made to access the state machine
      // from a constructor of a state that is *not* a subtype of state<>.
      // To correct this, derive from state<> instead of simple_state<>.
      BOOST_ASSERT( get_pointer( pContext_ ) != 0 );
      return pContext_->outermost_context_base();
    }

    const outermost_context_base_type & outermost_context_base() const
    {
      // This assert fails when an attempt is made to access the state machine
      // from a constructor of a state that is *not* a subtype of state<>.
      // To correct this, derive from state<> instead of simple_state<>.
      BOOST_ASSERT( get_pointer( pContext_ ) != 0 );
      return pContext_->outermost_context_base();
    }

    virtual const state_base_type * outer_state_ptr() const
    {
      typedef typename mpl::if_<
        is_same< outermost_context_type, context_type >,
        outer_state_ptr_impl_outermost,
        outer_state_ptr_impl_non_outermost
      >::type impl;
      return impl::outer_state_ptr_impl( *this );
    }

    virtual detail::reaction_result react_impl(
      const event_base_type & evt,
      typename rtti_policy_type::id_type eventType )
    {
      typedef typename detail::make_list<
        typename MostDerived::reactions >::type reaction_list;
      detail::reaction_result reactionResult =
        local_react< reaction_list >( evt, eventType );

      // At this point we can only safely access pContext_ if the handler did
      // not return do_discard_event!
      if ( reactionResult == detail::do_forward_event )
      {
        // TODO: The following call to react_impl of our outer state should
        // be made with a context_type:: prefix to call directly instead of
        // virtually. For some reason the compiler complains...
        reactionResult = pContext_->react_impl( evt, eventType );
      }

      return reactionResult;
    }

    virtual void exit_impl(
      typename base_type::direct_state_base_ptr_type & pSelf,
      typename state_base_type::node_state_base_ptr_type &
        pOutermostUnstableState,
      bool performFullExit )
    {
      inner_context_ptr_type pMostDerivedSelf =
        polymorphic_downcast< MostDerived * >( this );
      pSelf = 0;
      exit_impl( pMostDerivedSelf, pOutermostUnstableState, performFullExit );
    }

    void exit_impl(
      inner_context_ptr_type & pSelf,
      typename state_base_type::node_state_base_ptr_type &
        pOutermostUnstableState,
      bool performFullExit )
    {
      switch ( this->ref_count() )
      {
        case 2:
          if ( get_pointer( pOutermostUnstableState ) ==
            static_cast< state_base_type * >( this ) )
          {
            pContext_->set_outermost_unstable_state(
              pOutermostUnstableState );
            BOOST_FALLTHROUGH;
          }
          else
          {
            break;
          }
        case 1:
        {
          if ( get_pointer( pOutermostUnstableState ) == 0 )
          {
            pContext_->set_outermost_unstable_state(
              pOutermostUnstableState );
          }

          if ( performFullExit )
          {
            pSelf->exit();
            check_store_shallow_history< stores_shallow_history >();
            check_store_deep_history< stores_deep_history >();
          }

          context_ptr_type pContext = pContext_;
          pSelf = 0;
          pContext->exit_impl(
            pContext, pOutermostUnstableState, performFullExit );
          break;
        }
        default:
          break;
      }
    }

    void set_outermost_unstable_state(
      typename state_base_type::node_state_base_ptr_type &
        pOutermostUnstableState )
    {
      pOutermostUnstableState = this;
    }

    template< class OtherContext >
    const typename OtherContext::inner_context_ptr_type & context_ptr() const
    {
      typedef typename mpl::if_<
        is_same< OtherContext, context_type >,
        context_ptr_impl_my_context,
        context_ptr_impl_other_context
      >::type impl;

      return impl::template context_ptr_impl< OtherContext >( *this );
    }

    static void initial_deep_construct(
      outermost_context_base_type & outermostContextBase )
    {
      deep_construct( &outermostContextBase, outermostContextBase );
    }

    static void deep_construct(
      const context_ptr_type & pContext,
      outermost_context_base_type & outermostContextBase )
    {
      const inner_context_ptr_type pInnerContext(
        shallow_construct( pContext, outermostContextBase ) );
      deep_construct_inner< inner_initial_list >(
        pInnerContext, outermostContextBase );
    }

    static inner_context_ptr_type shallow_construct(
      const context_ptr_type & pContext,
      outermost_context_base_type & outermostContextBase )
    {
      const inner_context_ptr_type pInnerContext( new MostDerived );
      pInnerContext->set_context( pContext );
      outermostContextBase.add( pInnerContext );
      return pInnerContext;
    }

    void set_context( const context_ptr_type & pContext )
    {
      BOOST_ASSERT( get_pointer( pContext ) != 0 );
      pContext_ = pContext;
      base_type::set_context(
        orthogonal_position::value, get_pointer( pContext ) );
    }

    template< class InnerList >
    static void deep_construct_inner(
      const inner_context_ptr_type & pInnerContext,
      outermost_context_base_type & outermostContextBase )
    {
      typedef typename mpl::if_<
        mpl::empty< InnerList >,
        deep_construct_inner_impl_empty,
        deep_construct_inner_impl_non_empty
      >::type impl;
      impl::template deep_construct_inner_impl< InnerList >(
        pInnerContext, outermostContextBase );
    }

    template< class LeafState >
    void store_deep_history_impl()
    {
      detail::deep_history_storer<
        context_type::inherited_deep_history::value,
        context_type::deep_history::value
      >::template store_deep_history< MostDerived, LeafState >(
        *pContext_ );
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    struct context_ptr_impl_other_context
    {
      template< class OtherContext, class State >
      static const typename OtherContext::inner_context_ptr_type &
      context_ptr_impl( const State & stt )
      {
        // This assert fails when an attempt is made to access an outer 
        // context from a constructor of a state that is *not* a subtype of
        // state<>. To correct this, derive from state<> instead of
        // simple_state<>.
        BOOST_ASSERT( get_pointer( stt.pContext_ ) != 0 );
        return stt.pContext_->template context_ptr< OtherContext >();
      }
    };
    friend struct context_ptr_impl_other_context;

    struct context_ptr_impl_my_context
    {
      template< class OtherContext, class State >
      static const typename OtherContext::inner_context_ptr_type &
      context_ptr_impl( const State & stt )
      {
        // This assert fails when an attempt is made to access an outer 
        // context from a constructor of a state that is *not* a subtype of
        // state<>. To correct this, derive from state<> instead of
        // simple_state<>.
        BOOST_ASSERT( get_pointer( stt.pContext_ ) != 0 );
        return stt.pContext_;
      }
    };
    friend struct context_ptr_impl_my_context;

    struct context_impl_other_context
    {
      template< class OtherContext, class State >
      static OtherContext & context_impl( State & stt )
      {
        // This assert fails when an attempt is made to access an outer 
        // context from a constructor of a state that is *not* a subtype of
        // state<>. To correct this, derive from state<> instead of
        // simple_state<>.
        BOOST_ASSERT( get_pointer( stt.pContext_ ) != 0 );
        return stt.pContext_->template context< OtherContext >();
      }
    };
    friend struct context_impl_other_context;

    struct context_impl_this_context
    {
      template< class OtherContext, class State >
      static OtherContext & context_impl( State & stt )
      {
        return *polymorphic_downcast< MostDerived * >( &stt );
      }
    };
    friend struct context_impl_this_context;

    template< class DestinationState,
              class TransitionContext,
              class TransitionAction >
    result transit_impl( const TransitionAction & transitionAction )
    {
      typedef typename mpl::find_if<
        context_type_list,
        mpl::contains<
          typename DestinationState::context_type_list,
          mpl::placeholders::_ > >::type common_context_iter;
      typedef typename mpl::deref< common_context_iter >::type
        common_context_type;
      typedef typename mpl::distance<
        typename mpl::begin< context_type_list >::type,
        common_context_iter >::type termination_state_position;
      typedef typename mpl::push_front< context_type_list, MostDerived >::type
        possible_transition_contexts;
      typedef typename mpl::at<
        possible_transition_contexts,
        termination_state_position >::type termination_state_type;

      termination_state_type & terminationState(
        context< termination_state_type >() );
      const typename
        common_context_type::inner_context_ptr_type pCommonContext(
          terminationState.template context_ptr< common_context_type >() );
      outermost_context_base_type & outermostContextBase(
        pCommonContext->outermost_context_base() );

      #ifdef BOOST_STATECHART_RELAX_TRANSITION_CONTEXT
      typedef typename mpl::distance<
        typename mpl::begin< possible_transition_contexts >::type,
        typename mpl::find<
          possible_transition_contexts, TransitionContext >::type
      >::type proposed_transition_context_position;

      typedef typename mpl::plus<
        termination_state_position,
        mpl::long_< 1 >
      >::type uml_transition_context_position;

      typedef typename mpl::deref< typename mpl::max_element<
        mpl::list<
          proposed_transition_context_position,
          uml_transition_context_position >,
        mpl::greater< mpl::placeholders::_, mpl::placeholders::_ >
      >::type >::type real_transition_context_position;

      typedef typename mpl::at<
        possible_transition_contexts,
        real_transition_context_position >::type real_transition_context_type;

      #ifdef BOOST_MSVC
      #  pragma warning( push )
      #  pragma warning( disable: 4127 ) // conditional expression is constant
      #endif
      if ( ( proposed_transition_context_position::value == 0 ) &&
           ( inner_initial_list_size::value == 0 ) )
      {
        transitionAction( *polymorphic_downcast< MostDerived * >( this ) );
        outermostContextBase.terminate_as_part_of_transit( terminationState );
      }
      else if ( proposed_transition_context_position::value >=
                uml_transition_context_position::value )
      {
        real_transition_context_type & transitionContext =
          context< real_transition_context_type >();
        outermostContextBase.terminate_as_part_of_transit( terminationState );
        transitionAction( transitionContext );
      }
      else
      {
        typename real_transition_context_type::inner_context_ptr_type
          pTransitionContext = context_ptr< real_transition_context_type >();
        outermostContextBase.terminate_as_part_of_transit(
          *pTransitionContext );
        transitionAction( *pTransitionContext );
        pTransitionContext = 0;
        outermostContextBase.terminate_as_part_of_transit( terminationState );
      }
      #ifdef BOOST_MSVC
      #  pragma warning( pop )
      #endif
      #else
      outermostContextBase.terminate_as_part_of_transit( terminationState );
      transitionAction( *pCommonContext );
      #endif

      typedef typename detail::make_context_list<
        common_context_type, DestinationState >::type context_list_type;

      // If you receive a
      // "use of undefined type 'boost::STATIC_ASSERTION_FAILURE<x>'" or
      // similar compiler error here then you tried to make an invalid
      // transition between different orthogonal regions.
      BOOST_STATIC_ASSERT( ( mpl::equal_to<
        typename termination_state_type::orthogonal_position,
        typename mpl::front< context_list_type >::type::orthogonal_position
      >::value ) );

      detail::constructor<
        context_list_type, outermost_context_base_type >::construct(
          pCommonContext, outermostContextBase );

      return detail::result_utility::make_result( detail::do_discard_event );
    }

    struct local_react_impl_non_empty
    {
      template< class ReactionList, class State >
      static detail::reaction_result local_react_impl(
        State & stt,
        const event_base_type & evt,
        typename rtti_policy_type::id_type eventType )
      {
        detail::reaction_result reactionResult =
          mpl::front< ReactionList >::type::react(
            *polymorphic_downcast< MostDerived * >( &stt ),
            evt, eventType );

        if ( reactionResult == detail::no_reaction )
        {
          reactionResult = stt.template local_react<
            typename mpl::pop_front< ReactionList >::type >(
              evt, eventType );
        }

        return reactionResult;
      }
    };
    friend struct local_react_impl_non_empty;

    struct local_react_impl_empty
    {
      template< class ReactionList, class State >
      static detail::reaction_result local_react_impl(
        State &, const event_base_type &, typename rtti_policy_type::id_type )
      {
        return detail::do_forward_event;
      }
    };

    template< class ReactionList >
    detail::reaction_result local_react(
      const event_base_type & evt,
      typename rtti_policy_type::id_type eventType )
    {
      typedef typename mpl::if_<
        mpl::empty< ReactionList >,
        local_react_impl_empty,
        local_react_impl_non_empty
      >::type impl;
      return impl::template local_react_impl< ReactionList >(
        *this, evt, eventType );
    }

    struct outer_state_ptr_impl_non_outermost
    {
      template< class State >
      static const state_base_type * outer_state_ptr_impl( const State & stt )
      {
        return get_pointer( stt.pContext_ );
      }
    };
    friend struct outer_state_ptr_impl_non_outermost;

    struct outer_state_ptr_impl_outermost
    {
      template< class State >
      static const state_base_type * outer_state_ptr_impl( const State & )
      {
        return 0;
      }
    };

    struct deep_construct_inner_impl_non_empty
    {
      template< class InnerList >
      static void deep_construct_inner_impl(
        const inner_context_ptr_type & pInnerContext,
        outermost_context_base_type & outermostContextBase )
      {
        typedef typename mpl::front< InnerList >::type current_inner;

        // If you receive a
        // "use of undefined type 'boost::STATIC_ASSERTION_FAILURE<x>'" or
        // similar compiler error here then there is a mismatch between the
        // orthogonal position of a state and its position in the inner
        // initial list of its outer state.
        BOOST_STATIC_ASSERT( ( is_same<
          current_inner,
          typename mpl::at<
            typename current_inner::context_type::inner_initial_list,
            typename current_inner::orthogonal_position >::type >::value ) );

        current_inner::deep_construct( pInnerContext, outermostContextBase );
        deep_construct_inner< typename mpl::pop_front< InnerList >::type >(
          pInnerContext, outermostContextBase );
      }
    };

    struct deep_construct_inner_impl_empty
    {
      template< class InnerList >
      static void deep_construct_inner_impl(
        const inner_context_ptr_type &, outermost_context_base_type & ) {}
    };

    struct check_store_shallow_history_impl_no
    {
      template< class State >
      static void check_store_shallow_history_impl( State & ) {}
    };

    struct check_store_shallow_history_impl_yes
    {
      template< class State >
      static void check_store_shallow_history_impl( State & stt )
      {
        stt.outermost_context_base().template store_shallow_history<
          MostDerived >();
      }
    };
    friend struct check_store_shallow_history_impl_yes;

    template< class StoreShallowHistory >
    void check_store_shallow_history()
    {
      typedef typename mpl::if_<
        StoreShallowHistory,
        check_store_shallow_history_impl_yes,
        check_store_shallow_history_impl_no
      >::type impl;
      impl::check_store_shallow_history_impl( *this );
    }

    struct check_store_deep_history_impl_no
    {
      template< class State >
      static void check_store_deep_history_impl( State & ) {}
    };

    struct check_store_deep_history_impl_yes
    {
      template< class State >
      static void check_store_deep_history_impl( State & stt )
      {
        stt.template store_deep_history_impl< MostDerived >();
      }
    };
    friend struct check_store_deep_history_impl_yes;

    template< class StoreDeepHistory >
    void check_store_deep_history()
    {
      typedef typename mpl::if_<
        StoreDeepHistory,
        check_store_deep_history_impl_yes,
        check_store_deep_history_impl_no
      >::type impl;
      impl::check_store_deep_history_impl( *this );
    }


    context_ptr_type pContext_;
};



#ifdef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
} // namespace statechart
#endif



template< class MostDerived, class Context,
          class InnerInitial, history_mode historyMode >
inline void intrusive_ptr_release( const ::boost::statechart::simple_state<
  MostDerived, Context, InnerInitial, historyMode > * pBase )
{
  if ( pBase->release() )
  {
    // The cast is necessary because the simple_state destructor is non-
    // virtual (and inaccessible from this context)
    delete polymorphic_downcast< const MostDerived * >( pBase );
  }
}



#ifndef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
} // namespace statechart
#endif



} // namespace boost



#endif
