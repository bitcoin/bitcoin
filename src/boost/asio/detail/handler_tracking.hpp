//
// detail/handler_tracking.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_HANDLER_TRACKING_HPP
#define BOOST_ASIO_DETAIL_HANDLER_TRACKING_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

namespace boost {
namespace asio {

class execution_context;

} // namespace asio
} // namespace boost

#if defined(BOOST_ASIO_CUSTOM_HANDLER_TRACKING)
# include BOOST_ASIO_CUSTOM_HANDLER_TRACKING
#elif defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# include <boost/system/error_code.hpp>
# include <boost/asio/detail/cstdint.hpp>
# include <boost/asio/detail/static_mutex.hpp>
# include <boost/asio/detail/tss_ptr.hpp>
#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

#if defined(BOOST_ASIO_CUSTOM_HANDLER_TRACKING)

// The user-specified header must define the following macros:
// - BOOST_ASIO_INHERIT_TRACKED_HANDLER
// - BOOST_ASIO_ALSO_INHERIT_TRACKED_HANDLER
// - BOOST_ASIO_HANDLER_TRACKING_INIT
// - BOOST_ASIO_HANDLER_CREATION(args)
// - BOOST_ASIO_HANDLER_COMPLETION(args)
// - BOOST_ASIO_HANDLER_INVOCATION_BEGIN(args)
// - BOOST_ASIO_HANDLER_INVOCATION_END
// - BOOST_ASIO_HANDLER_OPERATION(args)
// - BOOST_ASIO_HANDLER_REACTOR_REGISTRATION(args)
// - BOOST_ASIO_HANDLER_REACTOR_DEREGISTRATION(args)
// - BOOST_ASIO_HANDLER_REACTOR_READ_EVENT
// - BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT
// - BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT
// - BOOST_ASIO_HANDLER_REACTOR_EVENTS(args)
// - BOOST_ASIO_HANDLER_REACTOR_OPERATION(args)

# if !defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
#  define BOOST_ASIO_ENABLE_HANDLER_TRACKING 1
# endif /// !defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

#elif defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

class handler_tracking
{
public:
  class completion;

  // Base class for objects containing tracked handlers.
  class tracked_handler
  {
  private:
    // Only the handler_tracking class will have access to the id.
    friend class handler_tracking;
    friend class completion;
    uint64_t id_;

  protected:
    // Constructor initialises with no id.
    tracked_handler() : id_(0) {}

    // Prevent deletion through this type.
    ~tracked_handler() {}
  };

  // Initialise the tracking system.
  BOOST_ASIO_DECL static void init();

  class location
  {
  public:
    // Constructor adds a location to the stack.
    BOOST_ASIO_DECL explicit location(const char* file,
        int line, const char* func);

    // Destructor removes a location from the stack.
    BOOST_ASIO_DECL ~location();

  private:
    // Disallow copying and assignment.
    location(const location&) BOOST_ASIO_DELETED;
    location& operator=(const location&) BOOST_ASIO_DELETED;

    friend class handler_tracking;
    const char* file_;
    int line_;
    const char* func_;
    location* next_;
  };

  // Record the creation of a tracked handler.
  BOOST_ASIO_DECL static void creation(
      execution_context& context, tracked_handler& h,
      const char* object_type, void* object,
      uintmax_t native_handle, const char* op_name);

  class completion
  {
  public:
    // Constructor records that handler is to be invoked with no arguments.
    BOOST_ASIO_DECL explicit completion(const tracked_handler& h);

    // Destructor records only when an exception is thrown from the handler, or
    // if the memory is being freed without the handler having been invoked.
    BOOST_ASIO_DECL ~completion();

    // Records that handler is to be invoked with no arguments.
    BOOST_ASIO_DECL void invocation_begin();

    // Records that handler is to be invoked with one arguments.
    BOOST_ASIO_DECL void invocation_begin(const boost::system::error_code& ec);

    // Constructor records that handler is to be invoked with two arguments.
    BOOST_ASIO_DECL void invocation_begin(
        const boost::system::error_code& ec, std::size_t bytes_transferred);

    // Constructor records that handler is to be invoked with two arguments.
    BOOST_ASIO_DECL void invocation_begin(
        const boost::system::error_code& ec, int signal_number);

    // Constructor records that handler is to be invoked with two arguments.
    BOOST_ASIO_DECL void invocation_begin(
        const boost::system::error_code& ec, const char* arg);

    // Record that handler invocation has ended.
    BOOST_ASIO_DECL void invocation_end();

  private:
    friend class handler_tracking;
    uint64_t id_;
    bool invoked_;
    completion* next_;
  };

  // Record an operation that is not directly associated with a handler.
  BOOST_ASIO_DECL static void operation(execution_context& context,
      const char* object_type, void* object,
      uintmax_t native_handle, const char* op_name);

  // Record that a descriptor has been registered with the reactor.
  BOOST_ASIO_DECL static void reactor_registration(execution_context& context,
      uintmax_t native_handle, uintmax_t registration);

  // Record that a descriptor has been deregistered from the reactor.
  BOOST_ASIO_DECL static void reactor_deregistration(execution_context& context,
      uintmax_t native_handle, uintmax_t registration);

  // Record a reactor-based operation that is associated with a handler.
  BOOST_ASIO_DECL static void reactor_events(execution_context& context,
      uintmax_t registration, unsigned events);

  // Record a reactor-based operation that is associated with a handler.
  BOOST_ASIO_DECL static void reactor_operation(
      const tracked_handler& h, const char* op_name,
      const boost::system::error_code& ec);

  // Record a reactor-based operation that is associated with a handler.
  BOOST_ASIO_DECL static void reactor_operation(
      const tracked_handler& h, const char* op_name,
      const boost::system::error_code& ec, std::size_t bytes_transferred);

  // Write a line of output.
  BOOST_ASIO_DECL static void write_line(const char* format, ...);

private:
  struct tracking_state;
  BOOST_ASIO_DECL static tracking_state* get_state();
};

# define BOOST_ASIO_INHERIT_TRACKED_HANDLER \
  : public boost::asio::detail::handler_tracking::tracked_handler

# define BOOST_ASIO_ALSO_INHERIT_TRACKED_HANDLER \
  , public boost::asio::detail::handler_tracking::tracked_handler

# define BOOST_ASIO_HANDLER_TRACKING_INIT \
  boost::asio::detail::handler_tracking::init()

# define BOOST_ASIO_HANDLER_LOCATION(args) \
  boost::asio::detail::handler_tracking::location tracked_location args

# define BOOST_ASIO_HANDLER_CREATION(args) \
  boost::asio::detail::handler_tracking::creation args

# define BOOST_ASIO_HANDLER_COMPLETION(args) \
  boost::asio::detail::handler_tracking::completion tracked_completion args

# define BOOST_ASIO_HANDLER_INVOCATION_BEGIN(args) \
  tracked_completion.invocation_begin args

# define BOOST_ASIO_HANDLER_INVOCATION_END \
  tracked_completion.invocation_end()

# define BOOST_ASIO_HANDLER_OPERATION(args) \
  boost::asio::detail::handler_tracking::operation args

# define BOOST_ASIO_HANDLER_REACTOR_REGISTRATION(args) \
  boost::asio::detail::handler_tracking::reactor_registration args

# define BOOST_ASIO_HANDLER_REACTOR_DEREGISTRATION(args) \
  boost::asio::detail::handler_tracking::reactor_deregistration args

# define BOOST_ASIO_HANDLER_REACTOR_READ_EVENT 1
# define BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT 2
# define BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT 4

# define BOOST_ASIO_HANDLER_REACTOR_EVENTS(args) \
  boost::asio::detail::handler_tracking::reactor_events args

# define BOOST_ASIO_HANDLER_REACTOR_OPERATION(args) \
  boost::asio::detail::handler_tracking::reactor_operation args

#else // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

# define BOOST_ASIO_INHERIT_TRACKED_HANDLER
# define BOOST_ASIO_ALSO_INHERIT_TRACKED_HANDLER
# define BOOST_ASIO_HANDLER_TRACKING_INIT (void)0
# define BOOST_ASIO_HANDLER_LOCATION(loc) (void)0
# define BOOST_ASIO_HANDLER_CREATION(args) (void)0
# define BOOST_ASIO_HANDLER_COMPLETION(args) (void)0
# define BOOST_ASIO_HANDLER_INVOCATION_BEGIN(args) (void)0
# define BOOST_ASIO_HANDLER_INVOCATION_END (void)0
# define BOOST_ASIO_HANDLER_OPERATION(args) (void)0
# define BOOST_ASIO_HANDLER_REACTOR_REGISTRATION(args) (void)0
# define BOOST_ASIO_HANDLER_REACTOR_DEREGISTRATION(args) (void)0
# define BOOST_ASIO_HANDLER_REACTOR_READ_EVENT 0
# define BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT 0
# define BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT 0
# define BOOST_ASIO_HANDLER_REACTOR_EVENTS(args) (void)0
# define BOOST_ASIO_HANDLER_REACTOR_OPERATION(args) (void)0

#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/handler_tracking.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_DETAIL_HANDLER_TRACKING_HPP
