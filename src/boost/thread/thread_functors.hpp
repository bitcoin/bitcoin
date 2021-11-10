// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2009-2012 Anthony Williams
// (C) Copyright 2012 Vicente J. Botet Escriba

// Based on the Anthony's idea of scoped_thread in CCiA

#ifndef BOOST_THREAD_THREAD_FUNCTORS_HPP
#define BOOST_THREAD_THREAD_FUNCTORS_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/thread_only.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  struct detach
  {
    template <class Thread>
    void operator()(Thread& t)
    {
      t.detach();
    }
  };

  struct detach_if_joinable
  {
    template <class Thread>
    void operator()(Thread& t)
    {
      if (t.joinable())
      {
        t.detach();
      }
    }
  };

  struct join_if_joinable
  {
    template <class Thread>
    void operator()(Thread& t)
    {
      if (t.joinable())
      {
        t.join();
      }
    }
  };

#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
  struct interrupt_and_join_if_joinable
  {
    template <class Thread>
    void operator()(Thread& t)
    {
      if (t.joinable())
      {
        t.interrupt();
        t.join();
      }
    }
  };
#endif
}
#include <boost/config/abi_suffix.hpp>

#endif
