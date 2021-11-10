#ifndef BOOST_STATECHART_DETAIL_CONSTRUCTOR_HPP_INCLUDED
#define BOOST_STATECHART_DETAIL_CONSTRUCTOR_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/advance.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/erase.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/long.hpp>



namespace boost
{
namespace statechart
{
namespace detail
{



template< class ContextList, class OutermostContextBase >
struct constructor;

//////////////////////////////////////////////////////////////////////////////
template< class ContextList, class OutermostContextBase >
struct outer_constructor
{
  typedef typename mpl::front< ContextList >::type to_construct;
  typedef typename to_construct::context_ptr_type context_ptr_type;
  typedef typename to_construct::inner_context_ptr_type
    inner_context_ptr_type;

  typedef typename to_construct::inner_initial_list inner_initial_list;
  typedef typename mpl::pop_front< ContextList >::type inner_context_list;
  typedef typename mpl::front< inner_context_list >::type::orthogonal_position
    inner_orthogonal_position;
  typedef typename mpl::advance<
    typename mpl::begin< inner_initial_list >::type,
    inner_orthogonal_position >::type to_construct_iter;

  typedef typename mpl::erase<
    inner_initial_list,
    to_construct_iter,
    typename mpl::end< inner_initial_list >::type
  >::type first_inner_initial_list;

  typedef typename mpl::erase<
    inner_initial_list,
    typename mpl::begin< inner_initial_list >::type,
    typename mpl::next< to_construct_iter >::type
  >::type last_inner_initial_list;

  static void construct(
    const context_ptr_type & pContext,
    OutermostContextBase & outermostContextBase )
  {
    const inner_context_ptr_type pInnerContext =
      to_construct::shallow_construct( pContext, outermostContextBase );
    to_construct::template deep_construct_inner<
      first_inner_initial_list >( pInnerContext, outermostContextBase );
    constructor< inner_context_list, OutermostContextBase >::construct(
      pInnerContext, outermostContextBase );
    to_construct::template deep_construct_inner<
      last_inner_initial_list >( pInnerContext, outermostContextBase );
  }
};

//////////////////////////////////////////////////////////////////////////////
template< class ContextList, class OutermostContextBase >
struct inner_constructor
{
  typedef typename mpl::front< ContextList >::type to_construct;
  typedef typename to_construct::context_ptr_type context_ptr_type;

  static void construct(
    const context_ptr_type & pContext,
    OutermostContextBase & outermostContextBase )
  {
    to_construct::deep_construct( pContext, outermostContextBase );
  }
};

//////////////////////////////////////////////////////////////////////////////
template< class ContextList, class OutermostContextBase >
struct constructor_impl : public mpl::eval_if<
  mpl::equal_to< mpl::size< ContextList >, mpl::long_< 1 > >,
  mpl::identity< inner_constructor< ContextList, OutermostContextBase > >,
  mpl::identity< outer_constructor< ContextList, OutermostContextBase > > >
{
};


//////////////////////////////////////////////////////////////////////////////
template< class ContextList, class OutermostContextBase >
struct constructor :
  constructor_impl< ContextList, OutermostContextBase >::type {};

//////////////////////////////////////////////////////////////////////////////
template< class CommonContext, class DestinationState >
struct make_context_list
{
  typedef typename mpl::reverse< typename mpl::push_front<
    typename mpl::erase<
      typename DestinationState::context_type_list,
      typename mpl::find<
        typename DestinationState::context_type_list,
        CommonContext
      >::type,
      typename mpl::end<
        typename DestinationState::context_type_list
      >::type
    >::type,
    DestinationState
  >::type >::type type;
};



} // namespace detail
} // namespace statechart
} // namespace boost



#endif
