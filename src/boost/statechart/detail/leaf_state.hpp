#ifndef BOOST_STATECHART_DETAIL_LEAF_STATE_HPP_INCLUDED
#define BOOST_STATECHART_DETAIL_LEAF_STATE_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/detail/state_base.hpp>



namespace boost
{
namespace statechart
{
namespace detail
{



//////////////////////////////////////////////////////////////////////////////
template< class Allocator, class RttiPolicy >
class leaf_state : public state_base< Allocator, RttiPolicy >
{
  typedef state_base< Allocator, RttiPolicy > base_type;
  protected:
    //////////////////////////////////////////////////////////////////////////
    leaf_state( typename RttiPolicy::id_provider_type idProvider ) :
      base_type( idProvider )
    {
    }

    ~leaf_state() {}

  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    void set_list_position(
      typename base_type::state_list_type::iterator listPosition )
    {
      listPosition_ = listPosition;
    }

    typedef typename base_type::leaf_state_ptr_type
      direct_state_base_ptr_type;

    virtual void remove_from_state_list(
      typename base_type::state_list_type::iterator & statesEnd,
      typename base_type::node_state_base_ptr_type & pOutermostUnstableState,
      bool performFullExit )
    {
      --statesEnd;
      swap( *listPosition_, *statesEnd );
      ( *listPosition_ )->set_list_position( listPosition_ );
      direct_state_base_ptr_type & pState = *statesEnd;
      // Because the list owns the leaf_state, this leads to the immediate
      // termination of this state.
      pState->exit_impl( pState, pOutermostUnstableState, performFullExit );
    }

    virtual void exit_impl(
      direct_state_base_ptr_type & pSelf,
      typename base_type::node_state_base_ptr_type & pOutermostUnstableState,
      bool performFullExit ) = 0;

  private:
    //////////////////////////////////////////////////////////////////////////
    typename base_type::state_list_type::iterator listPosition_;
};



} // namespace detail
} // namespace statechart
} // namespace boost



#endif
