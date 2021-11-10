#ifndef BOOST_STATECHART_RESULT_HPP_INCLUDED
#define BOOST_STATECHART_RESULT_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2010 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/assert.hpp>



namespace boost
{
namespace statechart
{
namespace detail
{



//////////////////////////////////////////////////////////////////////////////
enum reaction_result
{
  no_reaction,
  do_forward_event,
  do_discard_event,
  do_defer_event,
  consumed
};

struct result_utility;

//////////////////////////////////////////////////////////////////////////////
class safe_reaction_result
{
  public:
    //////////////////////////////////////////////////////////////////////////
    safe_reaction_result( const safe_reaction_result & other ) :
      reactionResult_( other.reactionResult_ )
    {
      // This assert fails when an attempt is made to make multiple copies of
      // a result value. This makes little sense, given the requirement that
      // an obtained result value must be returned out of the react function.
      BOOST_ASSERT( reactionResult_ != consumed );
      other.reactionResult_ = consumed;
    }

    ~safe_reaction_result()
    {
      // This assert fails when an obtained result value is not returned out
      // of the react() function. This can happen if the user accidentally
      // makes more than one call to reaction functions inside react() or
      // accidentally makes one or more calls to reaction functions outside
      // react()
      BOOST_ASSERT( reactionResult_ == consumed );
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    safe_reaction_result( reaction_result reactionResult ) :
      reactionResult_( reactionResult )
    {
    }

    operator reaction_result() const
    {
      const reaction_result val = reactionResult_;
      reactionResult_ = consumed;
      return val;
    }

    safe_reaction_result & operator=( const safe_reaction_result & );

    mutable reaction_result reactionResult_;

    friend struct result_utility;
};



} // namespace detail



#ifdef NDEBUG
  typedef detail::reaction_result result;
#else
  typedef detail::safe_reaction_result result;
#endif


namespace detail
{



//////////////////////////////////////////////////////////////////////////////
struct result_utility
{
  static ::boost::statechart::result make_result( reaction_result value )
  {
    return value;
  }

  static reaction_result get_result( ::boost::statechart::result value )
  {
    return value;
  }
};



} // namespace detail
} // namespace statechart
} // namespace boost



#endif
