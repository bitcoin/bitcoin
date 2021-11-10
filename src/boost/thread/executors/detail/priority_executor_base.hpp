// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_DETAIL_PRIORITY_EXECUTOR_BASE_HPP
#define BOOST_THREAD_EXECUTORS_DETAIL_PRIORITY_EXECUTOR_BASE_HPP

#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/concurrent_queues/sync_timed_queue.hpp>
#include <boost/thread/executors/work.hpp>

namespace boost
{
namespace executors
{
namespace detail
{
  template <class Queue>
  class priority_executor_base
  {
  public:
    //typedef boost::function<void()> work;
    typedef executors::work_pq work;
  protected:
    typedef Queue queue_type;
    queue_type _workq;

    priority_executor_base() {}
  public:

    ~priority_executor_base()
    {
      if(!closed())
      {
        this->close();
      }
    }

    void close()
    {
      _workq.close();
    }

    bool closed()
    {
      return _workq.closed();
    }

    void loop()
    {
      try
      {
        for(;;)
        {
          try {
            work task;
            queue_op_status st = _workq.wait_pull(task);
            if (st == queue_op_status::closed) return;
            task();
          }
          catch (boost::thread_interrupted&)
          {
            return;
          }
        }
      }
      catch (...)
      {
        std::terminate();
        return;
      }
    }
  }; //end class

} //end detail namespace
} //end executors namespace
} //end boost namespace
#endif
