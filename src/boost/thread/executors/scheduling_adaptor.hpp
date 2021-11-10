// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_SCHEDULING_ADAPTOR_HPP
#define BOOST_THREAD_EXECUTORS_SCHEDULING_ADAPTOR_HPP

#include <boost/thread/detail/config.hpp>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION && defined BOOST_THREAD_PROVIDES_EXECUTORS && defined BOOST_THREAD_USES_MOVE
#include <boost/thread/executors/detail/scheduled_executor_base.hpp>

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4355) // 'this' : used in base member initializer list
#endif

namespace boost
{
namespace executors
{

  template <typename Executor>
  class scheduling_adaptor : public detail::scheduled_executor_base<>
  {
  private:
    Executor& _exec;
    thread _scheduler;
  public:

    scheduling_adaptor(Executor& ex)
      : super(),
        _exec(ex),
        _scheduler(&super::loop, this) {}

    ~scheduling_adaptor()
    {
      this->close();
      _scheduler.interrupt();
      _scheduler.join();
    }

    Executor& underlying_executor()
    {
        return _exec;
    }

  private:
    typedef detail::scheduled_executor_base<> super;
  }; //end class

} //end executors

  using executors::scheduling_adaptor;

} //end boost

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#endif
#endif
