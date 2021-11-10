//
// detail/impl/win_thread.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_WIN_THREAD_IPP
#define BOOST_ASIO_DETAIL_IMPL_WIN_THREAD_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_WINDOWS) \
  && !defined(BOOST_ASIO_WINDOWS_APP) \
  && !defined(UNDER_CE)

#include <process.h>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/detail/win_thread.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

win_thread::~win_thread()
{
  ::CloseHandle(thread_);

  // The exit_event_ handle is deliberately allowed to leak here since it
  // is an error for the owner of an internal thread not to join() it.
}

void win_thread::join()
{
  HANDLE handles[2] = { exit_event_, thread_ };
  ::WaitForMultipleObjects(2, handles, FALSE, INFINITE);
  ::CloseHandle(exit_event_);
  if (terminate_threads())
  {
    ::TerminateThread(thread_, 0);
  }
  else
  {
    ::QueueUserAPC(apc_function, thread_, 0);
    ::WaitForSingleObject(thread_, INFINITE);
  }
}

std::size_t win_thread::hardware_concurrency()
{
  SYSTEM_INFO system_info;
  ::GetSystemInfo(&system_info);
  return system_info.dwNumberOfProcessors;
}

void win_thread::start_thread(func_base* arg, unsigned int stack_size)
{
  ::HANDLE entry_event = 0;
  arg->entry_event_ = entry_event = ::CreateEventW(0, true, false, 0);
  if (!entry_event)
  {
    DWORD last_error = ::GetLastError();
    delete arg;
    boost::system::error_code ec(last_error,
        boost::asio::error::get_system_category());
    boost::asio::detail::throw_error(ec, "thread.entry_event");
  }

  arg->exit_event_ = exit_event_ = ::CreateEventW(0, true, false, 0);
  if (!exit_event_)
  {
    DWORD last_error = ::GetLastError();
    delete arg;
    boost::system::error_code ec(last_error,
        boost::asio::error::get_system_category());
    boost::asio::detail::throw_error(ec, "thread.exit_event");
  }

  unsigned int thread_id = 0;
  thread_ = reinterpret_cast<HANDLE>(::_beginthreadex(0,
        stack_size, win_thread_function, arg, 0, &thread_id));
  if (!thread_)
  {
    DWORD last_error = ::GetLastError();
    delete arg;
    if (entry_event)
      ::CloseHandle(entry_event);
    if (exit_event_)
      ::CloseHandle(exit_event_);
    boost::system::error_code ec(last_error,
        boost::asio::error::get_system_category());
    boost::asio::detail::throw_error(ec, "thread");
  }

  if (entry_event)
  {
    ::WaitForSingleObject(entry_event, INFINITE);
    ::CloseHandle(entry_event);
  }
}

unsigned int __stdcall win_thread_function(void* arg)
{
  win_thread::auto_func_base_ptr func = {
      static_cast<win_thread::func_base*>(arg) };

  ::SetEvent(func.ptr->entry_event_);

  func.ptr->run();

  // Signal that the thread has finished its work, but rather than returning go
  // to sleep to put the thread into a well known state. If the thread is being
  // joined during global object destruction then it may be killed using
  // TerminateThread (to avoid a deadlock in DllMain). Otherwise, the SleepEx
  // call will be interrupted using QueueUserAPC and the thread will shut down
  // cleanly.
  HANDLE exit_event = func.ptr->exit_event_;
  delete func.ptr;
  func.ptr = 0;
  ::SetEvent(exit_event);
  ::SleepEx(INFINITE, TRUE);

  return 0;
}

#if defined(WINVER) && (WINVER < 0x0500)
void __stdcall apc_function(ULONG) {}
#else
void __stdcall apc_function(ULONG_PTR) {}
#endif

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_WINDOWS)
       // && !defined(BOOST_ASIO_WINDOWS_APP)
       // && !defined(UNDER_CE)

#endif // BOOST_ASIO_DETAIL_IMPL_WIN_THREAD_IPP
