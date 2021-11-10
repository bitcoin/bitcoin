//
// detail/impl/handler_tracking.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_HANDLER_TRACKING_IPP
#define BOOST_ASIO_DETAIL_IMPL_HANDLER_TRACKING_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_CUSTOM_HANDLER_TRACKING)

// The handler tracking implementation is provided by the user-specified header.

#elif defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

#include <cstdarg>
#include <cstdio>
#include <boost/asio/detail/handler_tracking.hpp>

#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
# include <boost/asio/time_traits.hpp>
#elif defined(BOOST_ASIO_HAS_CHRONO)
# include <boost/asio/detail/chrono.hpp>
# include <boost/asio/detail/chrono_time_traits.hpp>
# include <boost/asio/wait_traits.hpp>
#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)

#if defined(BOOST_ASIO_WINDOWS_RUNTIME)
# include <boost/asio/detail/socket_types.hpp>
#elif !defined(BOOST_ASIO_WINDOWS)
# include <unistd.h>
#endif // !defined(BOOST_ASIO_WINDOWS)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

struct handler_tracking_timestamp
{
  uint64_t seconds;
  uint64_t microseconds;

  handler_tracking_timestamp()
  {
#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    boost::posix_time::time_duration now =
      boost::posix_time::microsec_clock::universal_time() - epoch;
#elif defined(BOOST_ASIO_HAS_CHRONO)
    typedef chrono_time_traits<chrono::system_clock,
        boost::asio::wait_traits<chrono::system_clock> > traits_helper;
    traits_helper::posix_time_duration now(
        chrono::system_clock::now().time_since_epoch());
#endif
    seconds = static_cast<uint64_t>(now.total_seconds());
    microseconds = static_cast<uint64_t>(now.total_microseconds() % 1000000);
  }
};

struct handler_tracking::tracking_state
{
  static_mutex mutex_;
  uint64_t next_id_;
  tss_ptr<completion>* current_completion_;
  tss_ptr<location>* current_location_;
};

handler_tracking::tracking_state* handler_tracking::get_state()
{
  static tracking_state state = { BOOST_ASIO_STATIC_MUTEX_INIT, 1, 0, 0 };
  return &state;
}

void handler_tracking::init()
{
  static tracking_state* state = get_state();

  state->mutex_.init();

  static_mutex::scoped_lock lock(state->mutex_);
  if (state->current_completion_ == 0)
    state->current_completion_ = new tss_ptr<completion>;
  if (state->current_location_ == 0)
    state->current_location_ = new tss_ptr<location>;
}

handler_tracking::location::location(
    const char* file, int line, const char* func)
  : file_(file),
    line_(line),
    func_(func),
    next_(*get_state()->current_location_)
{
  if (file_)
    *get_state()->current_location_ = this;
}

handler_tracking::location::~location()
{
  if (file_)
    *get_state()->current_location_ = next_;
}

void handler_tracking::creation(execution_context&,
    handler_tracking::tracked_handler& h,
    const char* object_type, void* object,
    uintmax_t /*native_handle*/, const char* op_name)
{
  static tracking_state* state = get_state();

  static_mutex::scoped_lock lock(state->mutex_);
  h.id_ = state->next_id_++;
  lock.unlock();

  handler_tracking_timestamp timestamp;

  uint64_t current_id = 0;
  if (completion* current_completion = *state->current_completion_)
    current_id = current_completion->id_;

  for (location* current_location = *state->current_location_;
      current_location; current_location = current_location->next_)
  {
    write_line(
#if defined(BOOST_ASIO_WINDOWS)
        "@asio|%I64u.%06I64u|%I64u^%I64u|%s%s%.80s%s(%.80s:%d)\n",
#else // defined(BOOST_ASIO_WINDOWS)
        "@asio|%llu.%06llu|%llu^%llu|%s%s%.80s%s(%.80s:%d)\n",
#endif // defined(BOOST_ASIO_WINDOWS)
        timestamp.seconds, timestamp.microseconds,
        current_id, h.id_,
        current_location == *state->current_location_ ? "in " : "called from ",
        current_location->func_ ? "'" : "",
        current_location->func_ ? current_location->func_ : "",
        current_location->func_ ? "' " : "",
        current_location->file_, current_location->line_);
  }

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|%I64u*%I64u|%.20s@%p.%.50s\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|%llu*%llu|%.20s@%p.%.50s\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds,
      current_id, h.id_, object_type, object, op_name);
}

handler_tracking::completion::completion(
    const handler_tracking::tracked_handler& h)
  : id_(h.id_),
    invoked_(false),
    next_(*get_state()->current_completion_)
{
  *get_state()->current_completion_ = this;
}

handler_tracking::completion::~completion()
{
  if (id_)
  {
    handler_tracking_timestamp timestamp;

    write_line(
#if defined(BOOST_ASIO_WINDOWS)
        "@asio|%I64u.%06I64u|%c%I64u|\n",
#else // defined(BOOST_ASIO_WINDOWS)
        "@asio|%llu.%06llu|%c%llu|\n",
#endif // defined(BOOST_ASIO_WINDOWS)
        timestamp.seconds, timestamp.microseconds,
        invoked_ ? '!' : '~', id_);
  }

  *get_state()->current_completion_ = next_;
}

void handler_tracking::completion::invocation_begin()
{
  handler_tracking_timestamp timestamp;

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|>%I64u|\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|>%llu|\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds, id_);

  invoked_ = true;
}

void handler_tracking::completion::invocation_begin(
    const boost::system::error_code& ec)
{
  handler_tracking_timestamp timestamp;

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|>%I64u|ec=%.20s:%d\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|>%llu|ec=%.20s:%d\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds,
      id_, ec.category().name(), ec.value());

  invoked_ = true;
}

void handler_tracking::completion::invocation_begin(
    const boost::system::error_code& ec, std::size_t bytes_transferred)
{
  handler_tracking_timestamp timestamp;

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|>%I64u|ec=%.20s:%d,bytes_transferred=%I64u\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|>%llu|ec=%.20s:%d,bytes_transferred=%llu\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds,
      id_, ec.category().name(), ec.value(),
      static_cast<uint64_t>(bytes_transferred));

  invoked_ = true;
}

void handler_tracking::completion::invocation_begin(
    const boost::system::error_code& ec, int signal_number)
{
  handler_tracking_timestamp timestamp;

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|>%I64u|ec=%.20s:%d,signal_number=%d\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|>%llu|ec=%.20s:%d,signal_number=%d\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds,
      id_, ec.category().name(), ec.value(), signal_number);

  invoked_ = true;
}

void handler_tracking::completion::invocation_begin(
    const boost::system::error_code& ec, const char* arg)
{
  handler_tracking_timestamp timestamp;

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|>%I64u|ec=%.20s:%d,%.50s\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|>%llu|ec=%.20s:%d,%.50s\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds,
      id_, ec.category().name(), ec.value(), arg);

  invoked_ = true;
}

void handler_tracking::completion::invocation_end()
{
  if (id_)
  {
    handler_tracking_timestamp timestamp;

    write_line(
#if defined(BOOST_ASIO_WINDOWS)
        "@asio|%I64u.%06I64u|<%I64u|\n",
#else // defined(BOOST_ASIO_WINDOWS)
        "@asio|%llu.%06llu|<%llu|\n",
#endif // defined(BOOST_ASIO_WINDOWS)
        timestamp.seconds, timestamp.microseconds, id_);

    id_ = 0;
  }
}

void handler_tracking::operation(execution_context&,
    const char* object_type, void* object,
    uintmax_t /*native_handle*/, const char* op_name)
{
  static tracking_state* state = get_state();

  handler_tracking_timestamp timestamp;

  unsigned long long current_id = 0;
  if (completion* current_completion = *state->current_completion_)
    current_id = current_completion->id_;

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|%I64u|%.20s@%p.%.50s\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|%llu|%.20s@%p.%.50s\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds,
      current_id, object_type, object, op_name);
}

void handler_tracking::reactor_registration(execution_context& /*context*/,
    uintmax_t /*native_handle*/, uintmax_t /*registration*/)
{
}

void handler_tracking::reactor_deregistration(execution_context& /*context*/,
    uintmax_t /*native_handle*/, uintmax_t /*registration*/)
{
}

void handler_tracking::reactor_events(execution_context& /*context*/,
    uintmax_t /*native_handle*/, unsigned /*events*/)
{
}

void handler_tracking::reactor_operation(
    const tracked_handler& h, const char* op_name,
    const boost::system::error_code& ec)
{
  handler_tracking_timestamp timestamp;

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|.%I64u|%s,ec=%.20s:%d\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|.%llu|%s,ec=%.20s:%d\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds,
      h.id_, op_name, ec.category().name(), ec.value());
}

void handler_tracking::reactor_operation(
    const tracked_handler& h, const char* op_name,
    const boost::system::error_code& ec, std::size_t bytes_transferred)
{
  handler_tracking_timestamp timestamp;

  write_line(
#if defined(BOOST_ASIO_WINDOWS)
      "@asio|%I64u.%06I64u|.%I64u|%s,ec=%.20s:%d,bytes_transferred=%I64u\n",
#else // defined(BOOST_ASIO_WINDOWS)
      "@asio|%llu.%06llu|.%llu|%s,ec=%.20s:%d,bytes_transferred=%llu\n",
#endif // defined(BOOST_ASIO_WINDOWS)
      timestamp.seconds, timestamp.microseconds,
      h.id_, op_name, ec.category().name(), ec.value(),
      static_cast<uint64_t>(bytes_transferred));
}

void handler_tracking::write_line(const char* format, ...)
{
  using namespace std; // For sprintf (or equivalent).

  va_list args;
  va_start(args, format);

  char line[256] = "";
#if defined(BOOST_ASIO_HAS_SECURE_RTL)
  int length = vsprintf_s(line, sizeof(line), format, args);
#else // defined(BOOST_ASIO_HAS_SECURE_RTL)
  int length = vsprintf(line, format, args);
#endif // defined(BOOST_ASIO_HAS_SECURE_RTL)

  va_end(args);

#if defined(BOOST_ASIO_WINDOWS_RUNTIME)
  wchar_t wline[256] = L"";
  mbstowcs_s(0, wline, sizeof(wline) / sizeof(wchar_t), line, length);
  ::OutputDebugStringW(wline);
#elif defined(BOOST_ASIO_WINDOWS)
  HANDLE stderr_handle = ::GetStdHandle(STD_ERROR_HANDLE);
  DWORD bytes_written = 0;
  ::WriteFile(stderr_handle, line, length, &bytes_written, 0);
#else // defined(BOOST_ASIO_WINDOWS)
  ::write(STDERR_FILENO, line, length);
#endif // defined(BOOST_ASIO_WINDOWS)
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

#endif // BOOST_ASIO_DETAIL_IMPL_HANDLER_TRACKING_IPP
