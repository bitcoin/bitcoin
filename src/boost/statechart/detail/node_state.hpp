#ifndef BOOST_STATECHART_DETAIL_NODE_STATE_HPP_INCLUDED
#define BOOST_STATECHART_DETAIL_NODE_STATE_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/detail/state_base.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/assert.hpp>  // BOOST_ASSERT

#include <algorithm> // std::find_if



namespace boost
{
namespace statechart
{
namespace detail
{



template< class Allocator, class RttiPolicy >
class node_state_base : public state_base< Allocator, RttiPolicy >
{
  typedef state_base< Allocator, RttiPolicy > base_type;
  protected:
    //////////////////////////////////////////////////////////////////////////
    node_state_base( typename RttiPolicy::id_provider_type idProvider ) :
      base_type( idProvider )
    {
    }

    ~node_state_base() {}

  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    typedef base_type state_base_type;
    typedef intrusive_ptr< node_state_base > direct_state_base_ptr_type;
    virtual void exit_impl(
      direct_state_base_ptr_type & pSelf,
      typename base_type::node_state_base_ptr_type & pOutermostUnstableState,
      bool performFullExit ) = 0;
};

//////////////////////////////////////////////////////////////////////////////
template< class OrthogonalRegionCount, class Allocator, class RttiPolicy >
class node_state : public node_state_base< Allocator, RttiPolicy >
{
  typedef node_state_base< Allocator, RttiPolicy > base_type;
  protected:
    //////////////////////////////////////////////////////////////////////////
    node_state( typename RttiPolicy::id_provider_type idProvider ) :
      base_type( idProvider )
    {
      for ( orthogonal_position_type pos = 0; 
            pos < OrthogonalRegionCount::value; ++pos )
      {
        pInnerStates[ pos ] = 0;
      }
    }

    ~node_state() {}

  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    typedef typename base_type::state_base_type state_base_type;

    void add_inner_state( orthogonal_position_type position,
                          state_base_type * pInnerState )
    {
      BOOST_ASSERT( ( position < OrthogonalRegionCount::value ) &&
                    ( pInnerStates[ position ] == 0 ) );
      pInnerStates[ position ] = pInnerState;
    }

    void remove_inner_state( orthogonal_position_type position )
    {
      BOOST_ASSERT( position < OrthogonalRegionCount::value );
      pInnerStates[ position ] = 0;
    }

    virtual void remove_from_state_list(
      typename state_base_type::state_list_type::iterator & statesEnd,
      typename state_base_type::node_state_base_ptr_type &
        pOutermostUnstableState,
      bool performFullExit )
    {
      state_base_type ** const pPastEnd =
        &pInnerStates[ OrthogonalRegionCount::value ];
      // We must not iterate past the last inner state because *this* state
      // will no longer exist when the last inner state has been removed
      state_base_type ** const pFirstNonNull = std::find_if(
        &pInnerStates[ 0 ], pPastEnd, &node_state::is_not_null );

      if ( pFirstNonNull == pPastEnd )
      {
        // The state does not have inner states but is still alive, this must
        // be the outermost unstable state then.
        BOOST_ASSERT( get_pointer( pOutermostUnstableState ) == this );
        typename state_base_type::node_state_base_ptr_type pSelf =
          pOutermostUnstableState;
        pSelf->exit_impl( pSelf, pOutermostUnstableState, performFullExit );
      }
      else
      {
        // Destroy inner states in the reverse order of construction
        for ( state_base_type ** pState = pPastEnd; pState != pFirstNonNull; )
        {
          --pState;

          // An inner orthogonal state might have been terminated long before,
          // that's why we have to check for 0 pointers
          if ( *pState != 0 )
          {
            ( *pState )->remove_from_state_list(
              statesEnd, pOutermostUnstableState, performFullExit );
          }
        }
      }
    }

    typedef typename base_type::direct_state_base_ptr_type
      direct_state_base_ptr_type;

  private:
    //////////////////////////////////////////////////////////////////////////
    static bool is_not_null( const state_base_type * pInner )
    {
      return pInner != 0;
    }

    state_base_type * pInnerStates[ OrthogonalRegionCount::value ];
};



} // namespace detail
} // namespace statechart
} // namespace boost



#endif
