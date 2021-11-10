#ifndef BOOST_STATECHART_EXCEPTION_TRANSLATOR_HPP_INCLUDED
#define BOOST_STATECHART_EXCEPTION_TRANSLATOR_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/event.hpp>
#include <boost/statechart/result.hpp>



namespace boost
{
namespace statechart
{



//////////////////////////////////////////////////////////////////////////////
class exception_thrown : public event< exception_thrown > {};



//////////////////////////////////////////////////////////////////////////////
template< class ExceptionEvent = exception_thrown >
class exception_translator
{
  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    template< class Action, class ExceptionEventHandler >
    result operator()( Action action, ExceptionEventHandler eventHandler )
    {
      try
      {
        return action();
      }
      catch ( ... )
      {
        return eventHandler( ExceptionEvent() );
      }
    }
};



} // namespace statechart
} // namespace boost



#endif
