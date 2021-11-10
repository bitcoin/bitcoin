//
// detail/impl/epoll_reactor.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_EPOLL_REACTOR_IPP
#define BOOST_ASIO_DETAIL_IMPL_EPOLL_REACTOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_EPOLL)

#include <cstddef>
#include <sys/epoll.h>
#include <boost/asio/detail/epoll_reactor.hpp>
#include <boost/asio/detail/scheduler.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>

#if defined(BOOST_ASIO_HAS_TIMERFD)
# include <sys/timerfd.h>
#endif // defined(BOOST_ASIO_HAS_TIMERFD)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

epoll_reactor::epoll_reactor(boost::asio::execution_context& ctx)
  : execution_context_service_base<epoll_reactor>(ctx),
    scheduler_(use_service<scheduler>(ctx)),
    mutex_(BOOST_ASIO_CONCURRENCY_HINT_IS_LOCKING(
          REACTOR_REGISTRATION, scheduler_.concurrency_hint())),
    interrupter_(),
    epoll_fd_(do_epoll_create()),
    timer_fd_(do_timerfd_create()),
    shutdown_(false),
    registered_descriptors_mutex_(mutex_.enabled())
{
  // Add the interrupter's descriptor to epoll.
  epoll_event ev = { 0, { 0 } };
  ev.events = EPOLLIN | EPOLLERR | EPOLLET;
  ev.data.ptr = &interrupter_;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, interrupter_.read_descriptor(), &ev);
  interrupter_.interrupt();

  // Add the timer descriptor to epoll.
  if (timer_fd_ != -1)
  {
    ev.events = EPOLLIN | EPOLLERR;
    ev.data.ptr = &timer_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &ev);
  }
}

epoll_reactor::~epoll_reactor()
{
  if (epoll_fd_ != -1)
    close(epoll_fd_);
  if (timer_fd_ != -1)
    close(timer_fd_);
}

void epoll_reactor::shutdown()
{
  mutex::scoped_lock lock(mutex_);
  shutdown_ = true;
  lock.unlock();

  op_queue<operation> ops;

  while (descriptor_state* state = registered_descriptors_.first())
  {
    for (int i = 0; i < max_ops; ++i)
      ops.push(state->op_queue_[i]);
    state->shutdown_ = true;
    registered_descriptors_.free(state);
  }

  timer_queues_.get_all_timers(ops);

  scheduler_.abandon_operations(ops);
}

void epoll_reactor::notify_fork(
    boost::asio::execution_context::fork_event fork_ev)
{
  if (fork_ev == boost::asio::execution_context::fork_child)
  {
    if (epoll_fd_ != -1)
      ::close(epoll_fd_);
    epoll_fd_ = -1;
    epoll_fd_ = do_epoll_create();

    if (timer_fd_ != -1)
      ::close(timer_fd_);
    timer_fd_ = -1;
    timer_fd_ = do_timerfd_create();

    interrupter_.recreate();

    // Add the interrupter's descriptor to epoll.
    epoll_event ev = { 0, { 0 } };
    ev.events = EPOLLIN | EPOLLERR | EPOLLET;
    ev.data.ptr = &interrupter_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, interrupter_.read_descriptor(), &ev);
    interrupter_.interrupt();

    // Add the timer descriptor to epoll.
    if (timer_fd_ != -1)
    {
      ev.events = EPOLLIN | EPOLLERR;
      ev.data.ptr = &timer_fd_;
      epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &ev);
    }

    update_timeout();

    // Re-register all descriptors with epoll.
    mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);
    for (descriptor_state* state = registered_descriptors_.first();
        state != 0; state = state->next_)
    {
      ev.events = state->registered_events_;
      ev.data.ptr = state;
      int result = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, state->descriptor_, &ev);
      if (result != 0)
      {
        boost::system::error_code ec(errno,
            boost::asio::error::get_system_category());
        boost::asio::detail::throw_error(ec, "epoll re-registration");
      }
    }
  }
}

void epoll_reactor::init_task()
{
  scheduler_.init_task();
}

int epoll_reactor::register_descriptor(socket_type descriptor,
    epoll_reactor::per_descriptor_data& descriptor_data)
{
  descriptor_data = allocate_descriptor_state();

  BOOST_ASIO_HANDLER_REACTOR_REGISTRATION((
        context(), static_cast<uintmax_t>(descriptor),
        reinterpret_cast<uintmax_t>(descriptor_data)));

  {
    mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

    descriptor_data->reactor_ = this;
    descriptor_data->descriptor_ = descriptor;
    descriptor_data->shutdown_ = false;
    for (int i = 0; i < max_ops; ++i)
      descriptor_data->try_speculative_[i] = true;
  }

  epoll_event ev = { 0, { 0 } };
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLPRI | EPOLLET;
  descriptor_data->registered_events_ = ev.events;
  ev.data.ptr = descriptor_data;
  int result = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, descriptor, &ev);
  if (result != 0)
  {
    if (errno == EPERM)
    {
      // This file descriptor type is not supported by epoll. However, if it is
      // a regular file then operations on it will not block. We will allow
      // this descriptor to be used and fail later if an operation on it would
      // otherwise require a trip through the reactor.
      descriptor_data->registered_events_ = 0;
      return 0;
    }
    return errno;
  }

  return 0;
}

int epoll_reactor::register_internal_descriptor(
    int op_type, socket_type descriptor,
    epoll_reactor::per_descriptor_data& descriptor_data, reactor_op* op)
{
  descriptor_data = allocate_descriptor_state();

  BOOST_ASIO_HANDLER_REACTOR_REGISTRATION((
        context(), static_cast<uintmax_t>(descriptor),
        reinterpret_cast<uintmax_t>(descriptor_data)));

  {
    mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

    descriptor_data->reactor_ = this;
    descriptor_data->descriptor_ = descriptor;
    descriptor_data->shutdown_ = false;
    descriptor_data->op_queue_[op_type].push(op);
    for (int i = 0; i < max_ops; ++i)
      descriptor_data->try_speculative_[i] = true;
  }

  epoll_event ev = { 0, { 0 } };
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLPRI | EPOLLET;
  descriptor_data->registered_events_ = ev.events;
  ev.data.ptr = descriptor_data;
  int result = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, descriptor, &ev);
  if (result != 0)
    return errno;

  return 0;
}

void epoll_reactor::move_descriptor(socket_type,
    epoll_reactor::per_descriptor_data& target_descriptor_data,
    epoll_reactor::per_descriptor_data& source_descriptor_data)
{
  target_descriptor_data = source_descriptor_data;
  source_descriptor_data = 0;
}

void epoll_reactor::start_op(int op_type, socket_type descriptor,
    epoll_reactor::per_descriptor_data& descriptor_data, reactor_op* op,
    bool is_continuation, bool allow_speculative)
{
  if (!descriptor_data)
  {
    op->ec_ = boost::asio::error::bad_descriptor;
    post_immediate_completion(op, is_continuation);
    return;
  }

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  if (descriptor_data->shutdown_)
  {
    post_immediate_completion(op, is_continuation);
    return;
  }

  if (descriptor_data->op_queue_[op_type].empty())
  {
    if (allow_speculative
        && (op_type != read_op
          || descriptor_data->op_queue_[except_op].empty()))
    {
      if (descriptor_data->try_speculative_[op_type])
      {
        if (reactor_op::status status = op->perform())
        {
          if (status == reactor_op::done_and_exhausted)
            if (descriptor_data->registered_events_ != 0)
              descriptor_data->try_speculative_[op_type] = false;
          descriptor_lock.unlock();
          scheduler_.post_immediate_completion(op, is_continuation);
          return;
        }
      }

      if (descriptor_data->registered_events_ == 0)
      {
        op->ec_ = boost::asio::error::operation_not_supported;
        scheduler_.post_immediate_completion(op, is_continuation);
        return;
      }

      if (op_type == write_op)
      {
        if ((descriptor_data->registered_events_ & EPOLLOUT) == 0)
        {
          epoll_event ev = { 0, { 0 } };
          ev.events = descriptor_data->registered_events_ | EPOLLOUT;
          ev.data.ptr = descriptor_data;
          if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, descriptor, &ev) == 0)
          {
            descriptor_data->registered_events_ |= ev.events;
          }
          else
          {
            op->ec_ = boost::system::error_code(errno,
                boost::asio::error::get_system_category());
            scheduler_.post_immediate_completion(op, is_continuation);
            return;
          }
        }
      }
    }
    else if (descriptor_data->registered_events_ == 0)
    {
      op->ec_ = boost::asio::error::operation_not_supported;
      scheduler_.post_immediate_completion(op, is_continuation);
      return;
    }
    else
    {
      if (op_type == write_op)
      {
        descriptor_data->registered_events_ |= EPOLLOUT;
      }

      epoll_event ev = { 0, { 0 } };
      ev.events = descriptor_data->registered_events_;
      ev.data.ptr = descriptor_data;
      epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, descriptor, &ev);
    }
  }

  descriptor_data->op_queue_[op_type].push(op);
  scheduler_.work_started();
}

void epoll_reactor::cancel_ops(socket_type,
    epoll_reactor::per_descriptor_data& descriptor_data)
{
  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  op_queue<operation> ops;
  for (int i = 0; i < max_ops; ++i)
  {
    while (reactor_op* op = descriptor_data->op_queue_[i].front())
    {
      op->ec_ = boost::asio::error::operation_aborted;
      descriptor_data->op_queue_[i].pop();
      ops.push(op);
    }
  }

  descriptor_lock.unlock();

  scheduler_.post_deferred_completions(ops);
}

void epoll_reactor::cancel_ops_by_key(socket_type,
    epoll_reactor::per_descriptor_data& descriptor_data,
    int op_type, void* cancellation_key)
{
  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  op_queue<operation> ops;
  op_queue<reactor_op> other_ops;
  while (reactor_op* op = descriptor_data->op_queue_[op_type].front())
  {
    descriptor_data->op_queue_[op_type].pop();
    if (op->cancellation_key_ == cancellation_key)
    {
      op->ec_ = boost::asio::error::operation_aborted;
      ops.push(op);
    }
    else
      other_ops.push(op);
  }
  descriptor_data->op_queue_[op_type].push(other_ops);

  descriptor_lock.unlock();

  scheduler_.post_deferred_completions(ops);
}

void epoll_reactor::deregister_descriptor(socket_type descriptor,
    epoll_reactor::per_descriptor_data& descriptor_data, bool closing)
{
  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  if (!descriptor_data->shutdown_)
  {
    if (closing)
    {
      // The descriptor will be automatically removed from the epoll set when
      // it is closed.
    }
    else if (descriptor_data->registered_events_ != 0)
    {
      epoll_event ev = { 0, { 0 } };
      epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, descriptor, &ev);
    }

    op_queue<operation> ops;
    for (int i = 0; i < max_ops; ++i)
    {
      while (reactor_op* op = descriptor_data->op_queue_[i].front())
      {
        op->ec_ = boost::asio::error::operation_aborted;
        descriptor_data->op_queue_[i].pop();
        ops.push(op);
      }
    }

    descriptor_data->descriptor_ = -1;
    descriptor_data->shutdown_ = true;

    descriptor_lock.unlock();

    BOOST_ASIO_HANDLER_REACTOR_DEREGISTRATION((
          context(), static_cast<uintmax_t>(descriptor),
          reinterpret_cast<uintmax_t>(descriptor_data)));

    scheduler_.post_deferred_completions(ops);

    // Leave descriptor_data set so that it will be freed by the subsequent
    // call to cleanup_descriptor_data.
  }
  else
  {
    // We are shutting down, so prevent cleanup_descriptor_data from freeing
    // the descriptor_data object and let the destructor free it instead.
    descriptor_data = 0;
  }
}

void epoll_reactor::deregister_internal_descriptor(socket_type descriptor,
    epoll_reactor::per_descriptor_data& descriptor_data)
{
  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  if (!descriptor_data->shutdown_)
  {
    epoll_event ev = { 0, { 0 } };
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, descriptor, &ev);

    op_queue<operation> ops;
    for (int i = 0; i < max_ops; ++i)
      ops.push(descriptor_data->op_queue_[i]);

    descriptor_data->descriptor_ = -1;
    descriptor_data->shutdown_ = true;

    descriptor_lock.unlock();

    BOOST_ASIO_HANDLER_REACTOR_DEREGISTRATION((
          context(), static_cast<uintmax_t>(descriptor),
          reinterpret_cast<uintmax_t>(descriptor_data)));

    // Leave descriptor_data set so that it will be freed by the subsequent
    // call to cleanup_descriptor_data.
  }
  else
  {
    // We are shutting down, so prevent cleanup_descriptor_data from freeing
    // the descriptor_data object and let the destructor free it instead.
    descriptor_data = 0;
  }
}

void epoll_reactor::cleanup_descriptor_data(
    per_descriptor_data& descriptor_data)
{
  if (descriptor_data)
  {
    free_descriptor_state(descriptor_data);
    descriptor_data = 0;
  }
}

void epoll_reactor::run(long usec, op_queue<operation>& ops)
{
  // This code relies on the fact that the scheduler queues the reactor task
  // behind all descriptor operations generated by this function. This means,
  // that by the time we reach this point, any previously returned descriptor
  // operations have already been dequeued. Therefore it is now safe for us to
  // reuse and return them for the scheduler to queue again.

  // Calculate timeout. Check the timer queues only if timerfd is not in use.
  int timeout;
  if (usec == 0)
    timeout = 0;
  else
  {
    timeout = (usec < 0) ? -1 : ((usec - 1) / 1000 + 1);
    if (timer_fd_ == -1)
    {
      mutex::scoped_lock lock(mutex_);
      timeout = get_timeout(timeout);
    }
  }

  // Block on the epoll descriptor.
  epoll_event events[128];
  int num_events = epoll_wait(epoll_fd_, events, 128, timeout);

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
  // Trace the waiting events.
  for (int i = 0; i < num_events; ++i)
  {
    void* ptr = events[i].data.ptr;
    if (ptr == &interrupter_)
    {
      // Ignore.
    }
# if defined(BOOST_ASIO_HAS_TIMERFD)
    else if (ptr == &timer_fd_)
    {
      // Ignore.
    }
# endif // defined(BOOST_ASIO_HAS_TIMERFD)
    else
    {
      unsigned event_mask = 0;
      if ((events[i].events & EPOLLIN) != 0)
        event_mask |= BOOST_ASIO_HANDLER_REACTOR_READ_EVENT;
      if ((events[i].events & EPOLLOUT))
        event_mask |= BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT;
      if ((events[i].events & (EPOLLERR | EPOLLHUP)) != 0)
        event_mask |= BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT;
      BOOST_ASIO_HANDLER_REACTOR_EVENTS((context(),
            reinterpret_cast<uintmax_t>(ptr), event_mask));
    }
  }
#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

#if defined(BOOST_ASIO_HAS_TIMERFD)
  bool check_timers = (timer_fd_ == -1);
#else // defined(BOOST_ASIO_HAS_TIMERFD)
  bool check_timers = true;
#endif // defined(BOOST_ASIO_HAS_TIMERFD)

  // Dispatch the waiting events.
  for (int i = 0; i < num_events; ++i)
  {
    void* ptr = events[i].data.ptr;
    if (ptr == &interrupter_)
    {
      // No need to reset the interrupter since we're leaving the descriptor
      // in a ready-to-read state and relying on edge-triggered notifications
      // to make it so that we only get woken up when the descriptor's epoll
      // registration is updated.

#if defined(BOOST_ASIO_HAS_TIMERFD)
      if (timer_fd_ == -1)
        check_timers = true;
#else // defined(BOOST_ASIO_HAS_TIMERFD)
      check_timers = true;
#endif // defined(BOOST_ASIO_HAS_TIMERFD)
    }
#if defined(BOOST_ASIO_HAS_TIMERFD)
    else if (ptr == &timer_fd_)
    {
      check_timers = true;
    }
#endif // defined(BOOST_ASIO_HAS_TIMERFD)
    else
    {
      // The descriptor operation doesn't count as work in and of itself, so we
      // don't call work_started() here. This still allows the scheduler to
      // stop if the only remaining operations are descriptor operations.
      descriptor_state* descriptor_data = static_cast<descriptor_state*>(ptr);
      if (!ops.is_enqueued(descriptor_data))
      {
        descriptor_data->set_ready_events(events[i].events);
        ops.push(descriptor_data);
      }
      else
      {
        descriptor_data->add_ready_events(events[i].events);
      }
    }
  }

  if (check_timers)
  {
    mutex::scoped_lock common_lock(mutex_);
    timer_queues_.get_ready_timers(ops);

#if defined(BOOST_ASIO_HAS_TIMERFD)
    if (timer_fd_ != -1)
    {
      itimerspec new_timeout;
      itimerspec old_timeout;
      int flags = get_timeout(new_timeout);
      timerfd_settime(timer_fd_, flags, &new_timeout, &old_timeout);
    }
#endif // defined(BOOST_ASIO_HAS_TIMERFD)
  }
}

void epoll_reactor::interrupt()
{
  epoll_event ev = { 0, { 0 } };
  ev.events = EPOLLIN | EPOLLERR | EPOLLET;
  ev.data.ptr = &interrupter_;
  epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, interrupter_.read_descriptor(), &ev);
}

int epoll_reactor::do_epoll_create()
{
#if defined(EPOLL_CLOEXEC)
  int fd = epoll_create1(EPOLL_CLOEXEC);
#else // defined(EPOLL_CLOEXEC)
  int fd = -1;
  errno = EINVAL;
#endif // defined(EPOLL_CLOEXEC)

  if (fd == -1 && (errno == EINVAL || errno == ENOSYS))
  {
    fd = epoll_create(epoll_size);
    if (fd != -1)
      ::fcntl(fd, F_SETFD, FD_CLOEXEC);
  }

  if (fd == -1)
  {
    boost::system::error_code ec(errno,
        boost::asio::error::get_system_category());
    boost::asio::detail::throw_error(ec, "epoll");
  }

  return fd;
}

int epoll_reactor::do_timerfd_create()
{
#if defined(BOOST_ASIO_HAS_TIMERFD)
# if defined(TFD_CLOEXEC)
  int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
# else // defined(TFD_CLOEXEC)
  int fd = -1;
  errno = EINVAL;
# endif // defined(TFD_CLOEXEC)

  if (fd == -1 && errno == EINVAL)
  {
    fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (fd != -1)
      ::fcntl(fd, F_SETFD, FD_CLOEXEC);
  }

  return fd;
#else // defined(BOOST_ASIO_HAS_TIMERFD)
  return -1;
#endif // defined(BOOST_ASIO_HAS_TIMERFD)
}

epoll_reactor::descriptor_state* epoll_reactor::allocate_descriptor_state()
{
  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);
  return registered_descriptors_.alloc(BOOST_ASIO_CONCURRENCY_HINT_IS_LOCKING(
        REACTOR_IO, scheduler_.concurrency_hint()));
}

void epoll_reactor::free_descriptor_state(epoll_reactor::descriptor_state* s)
{
  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);
  registered_descriptors_.free(s);
}

void epoll_reactor::do_add_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.insert(&queue);
}

void epoll_reactor::do_remove_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.erase(&queue);
}

void epoll_reactor::update_timeout()
{
#if defined(BOOST_ASIO_HAS_TIMERFD)
  if (timer_fd_ != -1)
  {
    itimerspec new_timeout;
    itimerspec old_timeout;
    int flags = get_timeout(new_timeout);
    timerfd_settime(timer_fd_, flags, &new_timeout, &old_timeout);
    return;
  }
#endif // defined(BOOST_ASIO_HAS_TIMERFD)
  interrupt();
}

int epoll_reactor::get_timeout(int msec)
{
  // By default we will wait no longer than 5 minutes. This will ensure that
  // any changes to the system clock are detected after no longer than this.
  const int max_msec = 5 * 60 * 1000;
  return timer_queues_.wait_duration_msec(
      (msec < 0 || max_msec < msec) ? max_msec : msec);
}

#if defined(BOOST_ASIO_HAS_TIMERFD)
int epoll_reactor::get_timeout(itimerspec& ts)
{
  ts.it_interval.tv_sec = 0;
  ts.it_interval.tv_nsec = 0;

  long usec = timer_queues_.wait_duration_usec(5 * 60 * 1000 * 1000);
  ts.it_value.tv_sec = usec / 1000000;
  ts.it_value.tv_nsec = usec ? (usec % 1000000) * 1000 : 1;

  return usec ? 0 : TFD_TIMER_ABSTIME;
}
#endif // defined(BOOST_ASIO_HAS_TIMERFD)

struct epoll_reactor::perform_io_cleanup_on_block_exit
{
  explicit perform_io_cleanup_on_block_exit(epoll_reactor* r)
    : reactor_(r), first_op_(0)
  {
  }

  ~perform_io_cleanup_on_block_exit()
  {
    if (first_op_)
    {
      // Post the remaining completed operations for invocation.
      if (!ops_.empty())
        reactor_->scheduler_.post_deferred_completions(ops_);

      // A user-initiated operation has completed, but there's no need to
      // explicitly call work_finished() here. Instead, we'll take advantage of
      // the fact that the scheduler will call work_finished() once we return.
    }
    else
    {
      // No user-initiated operations have completed, so we need to compensate
      // for the work_finished() call that the scheduler will make once this
      // operation returns.
      reactor_->scheduler_.compensating_work_started();
    }
  }

  epoll_reactor* reactor_;
  op_queue<operation> ops_;
  operation* first_op_;
};

epoll_reactor::descriptor_state::descriptor_state(bool locking)
  : operation(&epoll_reactor::descriptor_state::do_complete),
    mutex_(locking)
{
}

operation* epoll_reactor::descriptor_state::perform_io(uint32_t events)
{
  mutex_.lock();
  perform_io_cleanup_on_block_exit io_cleanup(reactor_);
  mutex::scoped_lock descriptor_lock(mutex_, mutex::scoped_lock::adopt_lock);

  // Exception operations must be processed first to ensure that any
  // out-of-band data is read before normal data.
  static const int flag[max_ops] = { EPOLLIN, EPOLLOUT, EPOLLPRI };
  for (int j = max_ops - 1; j >= 0; --j)
  {
    if (events & (flag[j] | EPOLLERR | EPOLLHUP))
    {
      try_speculative_[j] = true;
      while (reactor_op* op = op_queue_[j].front())
      {
        if (reactor_op::status status = op->perform())
        {
          op_queue_[j].pop();
          io_cleanup.ops_.push(op);
          if (status == reactor_op::done_and_exhausted)
          {
            try_speculative_[j] = false;
            break;
          }
        }
        else
          break;
      }
    }
  }

  // The first operation will be returned for completion now. The others will
  // be posted for later by the io_cleanup object's destructor.
  io_cleanup.first_op_ = io_cleanup.ops_.front();
  io_cleanup.ops_.pop();
  return io_cleanup.first_op_;
}

void epoll_reactor::descriptor_state::do_complete(
    void* owner, operation* base,
    const boost::system::error_code& ec, std::size_t bytes_transferred)
{
  if (owner)
  {
    descriptor_state* descriptor_data = static_cast<descriptor_state*>(base);
    uint32_t events = static_cast<uint32_t>(bytes_transferred);
    if (operation* op = descriptor_data->perform_io(events))
    {
      op->complete(owner, ec, 0);
    }
  }
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_EPOLL)

#endif // BOOST_ASIO_DETAIL_IMPL_EPOLL_REACTOR_IPP
