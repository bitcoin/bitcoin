//
// detail/impl/dev_poll_reactor.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_DEV_POLL_REACTOR_IPP
#define BOOST_ASIO_DETAIL_IMPL_DEV_POLL_REACTOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_DEV_POLL)

#include <boost/asio/detail/dev_poll_reactor.hpp>
#include <boost/asio/detail/assert.hpp>
#include <boost/asio/detail/scheduler.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

dev_poll_reactor::dev_poll_reactor(boost::asio::execution_context& ctx)
  : boost::asio::detail::execution_context_service_base<dev_poll_reactor>(ctx),
    scheduler_(use_service<scheduler>(ctx)),
    mutex_(),
    dev_poll_fd_(do_dev_poll_create()),
    interrupter_(),
    shutdown_(false)
{
  // Add the interrupter's descriptor to /dev/poll.
  ::pollfd ev = { 0, 0, 0 };
  ev.fd = interrupter_.read_descriptor();
  ev.events = POLLIN | POLLERR;
  ev.revents = 0;
  ::write(dev_poll_fd_, &ev, sizeof(ev));
}

dev_poll_reactor::~dev_poll_reactor()
{
  shutdown();
  ::close(dev_poll_fd_);
}

void dev_poll_reactor::shutdown()
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  shutdown_ = true;
  lock.unlock();

  op_queue<operation> ops;

  for (int i = 0; i < max_ops; ++i)
    op_queue_[i].get_all_operations(ops);

  timer_queues_.get_all_timers(ops);

  scheduler_.abandon_operations(ops);
} 

void dev_poll_reactor::notify_fork(
    boost::asio::execution_context::fork_event fork_ev)
{
  if (fork_ev == boost::asio::execution_context::fork_child)
  {
    detail::mutex::scoped_lock lock(mutex_);

    if (dev_poll_fd_ != -1)
      ::close(dev_poll_fd_);
    dev_poll_fd_ = -1;
    dev_poll_fd_ = do_dev_poll_create();

    interrupter_.recreate();

    // Add the interrupter's descriptor to /dev/poll.
    ::pollfd ev = { 0, 0, 0 };
    ev.fd = interrupter_.read_descriptor();
    ev.events = POLLIN | POLLERR;
    ev.revents = 0;
    ::write(dev_poll_fd_, &ev, sizeof(ev));

    // Re-register all descriptors with /dev/poll. The changes will be written
    // to the /dev/poll descriptor the next time the reactor is run.
    for (int i = 0; i < max_ops; ++i)
    {
      reactor_op_queue<socket_type>::iterator iter = op_queue_[i].begin();
      reactor_op_queue<socket_type>::iterator end = op_queue_[i].end();
      for (; iter != end; ++iter)
      {
        ::pollfd& pending_ev = add_pending_event_change(iter->first);
        pending_ev.events |= POLLERR | POLLHUP;
        switch (i)
        {
        case read_op: pending_ev.events |= POLLIN; break;
        case write_op: pending_ev.events |= POLLOUT; break;
        case except_op: pending_ev.events |= POLLPRI; break;
        default: break;
        }
      }
    }
    interrupter_.interrupt();
  }
}

void dev_poll_reactor::init_task()
{
  scheduler_.init_task();
}

int dev_poll_reactor::register_descriptor(socket_type, per_descriptor_data&)
{
  return 0;
}

int dev_poll_reactor::register_internal_descriptor(int op_type,
    socket_type descriptor, per_descriptor_data&, reactor_op* op)
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  op_queue_[op_type].enqueue_operation(descriptor, op);
  ::pollfd& ev = add_pending_event_change(descriptor);
  ev.events = POLLERR | POLLHUP;
  switch (op_type)
  {
  case read_op: ev.events |= POLLIN; break;
  case write_op: ev.events |= POLLOUT; break;
  case except_op: ev.events |= POLLPRI; break;
  default: break;
  }
  interrupter_.interrupt();

  return 0;
}

void dev_poll_reactor::move_descriptor(socket_type,
    dev_poll_reactor::per_descriptor_data&,
    dev_poll_reactor::per_descriptor_data&)
{
}

void dev_poll_reactor::start_op(int op_type, socket_type descriptor,
    dev_poll_reactor::per_descriptor_data&, reactor_op* op,
    bool is_continuation, bool allow_speculative)
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  if (shutdown_)
  {
    post_immediate_completion(op, is_continuation);
    return;
  }

  if (allow_speculative)
  {
    if (op_type != read_op || !op_queue_[except_op].has_operation(descriptor))
    {
      if (!op_queue_[op_type].has_operation(descriptor))
      {
        if (op->perform())
        {
          lock.unlock();
          scheduler_.post_immediate_completion(op, is_continuation);
          return;
        }
      }
    }
  }

  bool first = op_queue_[op_type].enqueue_operation(descriptor, op);
  scheduler_.work_started();
  if (first)
  {
    ::pollfd& ev = add_pending_event_change(descriptor);
    ev.events = POLLERR | POLLHUP;
    if (op_type == read_op
        || op_queue_[read_op].has_operation(descriptor))
      ev.events |= POLLIN;
    if (op_type == write_op
        || op_queue_[write_op].has_operation(descriptor))
      ev.events |= POLLOUT;
    if (op_type == except_op
        || op_queue_[except_op].has_operation(descriptor))
      ev.events |= POLLPRI;
    interrupter_.interrupt();
  }
}

void dev_poll_reactor::cancel_ops(socket_type descriptor,
    dev_poll_reactor::per_descriptor_data&)
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  cancel_ops_unlocked(descriptor, boost::asio::error::operation_aborted);
}

void dev_poll_reactor::cancel_ops_by_key(socket_type descriptor,
    dev_poll_reactor::per_descriptor_data&,
    int op_type, void* cancellation_key)
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  op_queue<operation> ops;
  bool need_interrupt = op_queue_[op_type].cancel_operations_by_key(
      descriptor, ops, cancellation_key, boost::asio::error::operation_aborted);
  scheduler_.post_deferred_completions(ops);
  if (need_interrupt)
    interrupter_.interrupt();
}

void dev_poll_reactor::deregister_descriptor(socket_type descriptor,
    dev_poll_reactor::per_descriptor_data&, bool)
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  // Remove the descriptor from /dev/poll.
  ::pollfd& ev = add_pending_event_change(descriptor);
  ev.events = POLLREMOVE;
  interrupter_.interrupt();

  // Cancel any outstanding operations associated with the descriptor.
  cancel_ops_unlocked(descriptor, boost::asio::error::operation_aborted);
}

void dev_poll_reactor::deregister_internal_descriptor(
    socket_type descriptor, dev_poll_reactor::per_descriptor_data&)
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  // Remove the descriptor from /dev/poll. Since this function is only called
  // during a fork, we can apply the change immediately.
  ::pollfd ev = { 0, 0, 0 };
  ev.fd = descriptor;
  ev.events = POLLREMOVE;
  ev.revents = 0;
  ::write(dev_poll_fd_, &ev, sizeof(ev));

  // Destroy all operations associated with the descriptor.
  op_queue<operation> ops;
  boost::system::error_code ec;
  for (int i = 0; i < max_ops; ++i)
    op_queue_[i].cancel_operations(descriptor, ops, ec);
}

void dev_poll_reactor::cleanup_descriptor_data(
    dev_poll_reactor::per_descriptor_data&)
{
}

void dev_poll_reactor::run(long usec, op_queue<operation>& ops)
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);

  // We can return immediately if there's no work to do and the reactor is
  // not supposed to block.
  if (usec == 0 && op_queue_[read_op].empty() && op_queue_[write_op].empty()
      && op_queue_[except_op].empty() && timer_queues_.all_empty())
    return;

  // Write the pending event registration changes to the /dev/poll descriptor.
  std::size_t events_size = sizeof(::pollfd) * pending_event_changes_.size();
  if (events_size > 0)
  {
    errno = 0;
    int result = ::write(dev_poll_fd_,
        &pending_event_changes_[0], events_size);
    if (result != static_cast<int>(events_size))
    {
      boost::system::error_code ec = boost::system::error_code(
          errno, boost::asio::error::get_system_category());
      for (std::size_t i = 0; i < pending_event_changes_.size(); ++i)
      {
        int descriptor = pending_event_changes_[i].fd;
        for (int j = 0; j < max_ops; ++j)
          op_queue_[j].cancel_operations(descriptor, ops, ec);
      }
    }
    pending_event_changes_.clear();
    pending_event_change_index_.clear();
  }

  // Calculate timeout.
  int timeout;
  if (usec == 0)
    timeout = 0;
  else
  {
    timeout = (usec < 0) ? -1 : ((usec - 1) / 1000 + 1);
    timeout = get_timeout(timeout);
  }
  lock.unlock();

  // Block on the /dev/poll descriptor.
  ::pollfd events[128] = { { 0, 0, 0 } };
  ::dvpoll dp = { 0, 0, 0 };
  dp.dp_fds = events;
  dp.dp_nfds = 128;
  dp.dp_timeout = timeout;
  int num_events = ::ioctl(dev_poll_fd_, DP_POLL, &dp);

  lock.lock();

  // Dispatch the waiting events.
  for (int i = 0; i < num_events; ++i)
  {
    int descriptor = events[i].fd;
    if (descriptor == interrupter_.read_descriptor())
    {
      interrupter_.reset();
    }
    else
    {
      bool more_reads = false;
      bool more_writes = false;
      bool more_except = false;

      // Exception operations must be processed first to ensure that any
      // out-of-band data is read before normal data.
      if (events[i].events & (POLLPRI | POLLERR | POLLHUP))
        more_except =
          op_queue_[except_op].perform_operations(descriptor, ops);
      else
        more_except = op_queue_[except_op].has_operation(descriptor);

      if (events[i].events & (POLLIN | POLLERR | POLLHUP))
        more_reads = op_queue_[read_op].perform_operations(descriptor, ops);
      else
        more_reads = op_queue_[read_op].has_operation(descriptor);

      if (events[i].events & (POLLOUT | POLLERR | POLLHUP))
        more_writes = op_queue_[write_op].perform_operations(descriptor, ops);
      else
        more_writes = op_queue_[write_op].has_operation(descriptor);

      if ((events[i].events & (POLLERR | POLLHUP)) != 0
            && !more_except && !more_reads && !more_writes)
      {
        // If we have an event and no operations associated with the
        // descriptor then we need to delete the descriptor from /dev/poll.
        // The poll operation can produce POLLHUP or POLLERR events when there
        // is no operation pending, so if we do not remove the descriptor we
        // can end up in a tight polling loop.
        ::pollfd ev = { 0, 0, 0 };
        ev.fd = descriptor;
        ev.events = POLLREMOVE;
        ev.revents = 0;
        ::write(dev_poll_fd_, &ev, sizeof(ev));
      }
      else
      {
        ::pollfd ev = { 0, 0, 0 };
        ev.fd = descriptor;
        ev.events = POLLERR | POLLHUP;
        if (more_reads)
          ev.events |= POLLIN;
        if (more_writes)
          ev.events |= POLLOUT;
        if (more_except)
          ev.events |= POLLPRI;
        ev.revents = 0;
        int result = ::write(dev_poll_fd_, &ev, sizeof(ev));
        if (result != sizeof(ev))
        {
          boost::system::error_code ec(errno,
              boost::asio::error::get_system_category());
          for (int j = 0; j < max_ops; ++j)
            op_queue_[j].cancel_operations(descriptor, ops, ec);
        }
      }
    }
  }
  timer_queues_.get_ready_timers(ops);
}

void dev_poll_reactor::interrupt()
{
  interrupter_.interrupt();
}

int dev_poll_reactor::do_dev_poll_create()
{
  int fd = ::open("/dev/poll", O_RDWR);
  if (fd == -1)
  {
    boost::system::error_code ec(errno,
        boost::asio::error::get_system_category());
    boost::asio::detail::throw_error(ec, "/dev/poll");
  }
  return fd;
}

void dev_poll_reactor::do_add_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.insert(&queue);
}

void dev_poll_reactor::do_remove_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.erase(&queue);
}

int dev_poll_reactor::get_timeout(int msec)
{
  // By default we will wait no longer than 5 minutes. This will ensure that
  // any changes to the system clock are detected after no longer than this.
  const int max_msec = 5 * 60 * 1000;
  return timer_queues_.wait_duration_msec(
      (msec < 0 || max_msec < msec) ? max_msec : msec);
}

void dev_poll_reactor::cancel_ops_unlocked(socket_type descriptor,
    const boost::system::error_code& ec)
{
  bool need_interrupt = false;
  op_queue<operation> ops;
  for (int i = 0; i < max_ops; ++i)
    need_interrupt = op_queue_[i].cancel_operations(
        descriptor, ops, ec) || need_interrupt;
  scheduler_.post_deferred_completions(ops);
  if (need_interrupt)
    interrupter_.interrupt();
}

::pollfd& dev_poll_reactor::add_pending_event_change(int descriptor)
{
  hash_map<int, std::size_t>::iterator iter
    = pending_event_change_index_.find(descriptor);
  if (iter == pending_event_change_index_.end())
  {
    std::size_t index = pending_event_changes_.size();
    pending_event_changes_.reserve(pending_event_changes_.size() + 1);
    pending_event_change_index_.insert(std::make_pair(descriptor, index));
    pending_event_changes_.push_back(::pollfd());
    pending_event_changes_[index].fd = descriptor;
    pending_event_changes_[index].revents = 0;
    return pending_event_changes_[index];
  }
  else
  {
    return pending_event_changes_[iter->second];
  }
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_DEV_POLL)

#endif // BOOST_ASIO_DETAIL_IMPL_DEV_POLL_REACTOR_IPP
