//
// system_context.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_SYSTEM_CONTEXT_HPP
#define BOOST_ASIO_SYSTEM_CONTEXT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/scheduler.hpp>
#include <boost/asio/detail/thread_group.hpp>
#include <boost/asio/execution.hpp>
#include <boost/asio/execution_context.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

template <typename Blocking, typename Relationship, typename Allocator>
class basic_system_executor;

/// The executor context for the system executor.
class system_context : public execution_context
{
public:
  /// The executor type associated with the context.
  typedef basic_system_executor<
      execution::blocking_t::possibly_t,
      execution::relationship_t::fork_t,
      std::allocator<void>
    > executor_type;

  /// Destructor shuts down all threads in the system thread pool.
  BOOST_ASIO_DECL ~system_context();

  /// Obtain an executor for the context.
  executor_type get_executor() BOOST_ASIO_NOEXCEPT;

  /// Signal all threads in the system thread pool to stop.
  BOOST_ASIO_DECL void stop();

  /// Determine whether the system thread pool has been stopped.
  BOOST_ASIO_DECL bool stopped() const BOOST_ASIO_NOEXCEPT;

  /// Join all threads in the system thread pool.
  BOOST_ASIO_DECL void join();

#if defined(GENERATING_DOCUMENTATION)
private:
#endif // defined(GENERATING_DOCUMENTATION)
  // Constructor creates all threads in the system thread pool.
  BOOST_ASIO_DECL system_context();

private:
  template <typename, typename, typename> friend class basic_system_executor;

  struct thread_function;

  // Helper function to create the underlying scheduler.
  BOOST_ASIO_DECL detail::scheduler& add_scheduler(detail::scheduler* s);

  // The underlying scheduler.
  detail::scheduler& scheduler_;

  // The threads in the system thread pool.
  detail::thread_group threads_;

  // The number of threads in the pool.
  std::size_t num_threads_;
};

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/system_context.hpp>
#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/impl/system_context.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_SYSTEM_CONTEXT_HPP
