#ifndef BOOST_STATECHART_ASYNCHRONOUS_STATE_MACHINE_HPP_INCLUDED
#define BOOST_STATECHART_ASYNCHRONOUS_STATE_MACHINE_HPP_INCLUDED
//////////////////////////////////////////////////////////////////////////////
// Copyright 2002-2006 Andreas Huber Doenni
// Distributed under the Boost Software License, Version 1.0. (See accompany-
// ing file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//////////////////////////////////////////////////////////////////////////////



#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/fifo_scheduler.hpp>
#include <boost/statechart/null_exception_translator.hpp>
#include <boost/statechart/event_processor.hpp>

#include <memory>   // std::allocator


namespace boost
{
namespace statechart
{



class event_base;



//////////////////////////////////////////////////////////////////////////////
template< class MostDerived,
          class InitialState,
          class Scheduler = fifo_scheduler<>,
          class Allocator = std::allocator< none >,
          class ExceptionTranslator = null_exception_translator >
class asynchronous_state_machine : public state_machine<
  MostDerived, InitialState, Allocator, ExceptionTranslator >,
  public event_processor< Scheduler >
{
  typedef state_machine< MostDerived,
    InitialState, Allocator, ExceptionTranslator > machine_base;
  typedef event_processor< Scheduler > processor_base;
  protected:
    //////////////////////////////////////////////////////////////////////////
    typedef asynchronous_state_machine my_base;

    asynchronous_state_machine( typename processor_base::my_context ctx ) :
      processor_base( ctx )
    {
    }

    virtual ~asynchronous_state_machine() {}

  public:
    //////////////////////////////////////////////////////////////////////////
    // The following declarations should be private.
    // They are only public because many compilers lack template friends.
    //////////////////////////////////////////////////////////////////////////
    void terminate()
    {
      processor_base::terminate();
    }

  private:
    //////////////////////////////////////////////////////////////////////////
    virtual void initiate_impl()
    {
      machine_base::initiate();
    }

    virtual void process_event_impl( const event_base & evt )
    {
      machine_base::process_event( evt );
    }

    virtual void terminate_impl()
    {
      machine_base::terminate();
    }
};



} // namespace statechart
} // namespace boost



#endif
