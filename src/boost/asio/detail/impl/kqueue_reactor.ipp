//
// detail/impl/kqueue_reactor.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2005 Stefan Arentz (stefan at soze dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_KQUEUE_REACTOR_IPP
#define BOOST_ASIO_DETAIL_IMPL_KQUEUE_REACTOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_KQUEUE)

#include <boost/asio/detail/kqueue_reactor.hpp>
#include <boost/asio/detail/scheduler.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>

#if defined(__NetBSD__)
# include <sys/param.h>
#endif

#include <boost/asio/detail/push_options.hpp>

#if defined(__NetBSD__) && __NetBSD_Version__ < 999001500
# define BOOST_ASIO_KQUEUE_EV_SET(ev, ident, filt, flags, fflags, data, udata) \
    EV_SET(ev, ident, filt, flags, fflags, data, \
      reinterpret_cast<intptr_t>(static_cast<void*>(udata)))
#else
# define BOOST_ASIO_KQUEUE_EV_SET(ev, ident, filt, flags, fflags, data, udata) \
    EV_SET(ev, ident, filt, flags, fflags, data, udata)
#endif

namespace boost {
namespace asio {
namespace detail {

kqueue_reactor::kqueue_reactor(boost::asio::execution_context& ctx)
  : execution_context_service_base<kqueue_reactor>(ctx),
    scheduler_(use_service<scheduler>(ctx)),
    mutex_(BOOST_ASIO_CONCURRENCY_HINT_IS_LOCKING(
          REACTOR_REGISTRATION, scheduler_.concurrency_hint())),
    kqueue_fd_(do_kqueue_create()),
    interrupter_(),
    shutdown_(false),
    registered_descriptors_mutex_(mutex_.enabled())
{
  struct kevent events[1];
  BOOST_ASIO_KQUEUE_EV_SET(&events[0], interrupter_.read_descriptor(),
      EVFILT_READ, EV_ADD, 0, 0, &interrupter_);
  if (::kevent(kqueue_fd_, events, 1, 0, 0, 0) == -1)
  {
    boost::system::error_code error(errno,
        boost::asio::error::get_system_category());
    boost::asio::detail::throw_error(error);
  }
}

kqueue_reactor::~kqueue_reactor()
{
  close(kqueue_fd_);
}

void kqueue_reactor::shutdown()
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

void kqueue_reactor::notify_fork(
    boost::asio::execution_context::fork_event fork_ev)
{
  if (fork_ev == boost::asio::execution_context::fork_child)
  {
    // The kqueue descriptor is automatically closed in the child.
    kqueue_fd_ = -1;
    kqueue_fd_ = do_kqueue_create();

    interrupter_.recreate();

    struct kevent events[2];
    BOOST_ASIO_KQUEUE_EV_SET(&events[0], interrupter_.read_descriptor(),
        EVFILT_READ, EV_ADD, 0, 0, &interrupter_);
    if (::kevent(kqueue_fd_, events, 1, 0, 0, 0) == -1)
    {
      boost::system::error_code ec(errno,
          boost::asio::error::get_system_category());
      boost::asio::detail::throw_error(ec, "kqueue interrupter registration");
    }

    // Re-register all descriptors with kqueue.
    mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);
    for (descriptor_state* state = registered_descriptors_.first();
        state != 0; state = state->next_)
    {
      if (state->num_kevents_ > 0)
      {
        BOOST_ASIO_KQUEUE_EV_SET(&events[0], state->descriptor_,
            EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, state);
        BOOST_ASIO_KQUEUE_EV_SET(&events[1], state->descriptor_,
            EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, state);
        if (::kevent(kqueue_fd_, events, state->num_kevents_, 0, 0, 0) == -1)
        {
          boost::system::error_code ec(errno,
              boost::asio::error::get_system_category());
          boost::asio::detail::throw_error(ec, "kqueue re-registration");
        }
      }
    }
  }
}

void kqueue_reactor::init_task()
{
  scheduler_.init_task();
}

int kqueue_reactor::register_descriptor(socket_type descriptor,
    kqueue_reactor::per_descriptor_data& descriptor_data)
{
  descriptor_data = allocate_descriptor_state();

  BOOST_ASIO_HANDLER_REACTOR_REGISTRATION((
        context(), static_cast<uintmax_t>(descriptor),
        reinterpret_cast<uintmax_t>(descriptor_data)));

  mutex::scoped_lock lock(descriptor_data->mutex_);

  descriptor_data->descriptor_ = descriptor;
  descriptor_data->num_kevents_ = 0;
  descriptor_data->shutdown_ = false;

  return 0;
}

int kqueue_reactor::register_internal_descriptor(
    int op_type, socket_type descriptor,
    kqueue_reactor::per_descriptor_data& descriptor_data, reactor_op* op)
{
  descriptor_data = allocate_descriptor_state();

  BOOST_ASIO_HANDLER_REACTOR_REGISTRATION((
        context(), static_cast<uintmax_t>(descriptor),
        reinterpret_cast<uintmax_t>(descriptor_data)));

  mutex::scoped_lock lock(descriptor_data->mutex_);

  descriptor_data->descriptor_ = descriptor;
  descriptor_data->num_kevents_ = 1;
  descriptor_data->shutdown_ = false;
  descriptor_data->op_queue_[op_type].push(op);

  struct kevent events[1];
  BOOST_ASIO_KQUEUE_EV_SET(&events[0], descriptor, EVFILT_READ,
      EV_ADD | EV_CLEAR, 0, 0, descriptor_data);
  if (::kevent(kqueue_fd_, events, 1, 0, 0, 0) == -1)
    return errno;

  return 0;
}

void kqueue_reactor::move_descriptor(socket_type,
    kqueue_reactor::per_descriptor_data& target_descriptor_data,
    kqueue_reactor::per_descriptor_data& source_descriptor_data)
{
  target_descriptor_data = source_descriptor_data;
  source_descriptor_data = 0;
}

void kqueue_reactor::start_op(int op_type, socket_type descriptor,
    kqueue_reactor::per_descriptor_data& descriptor_data, reactor_op* op,
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
    static const int num_kevents[max_ops] = { 1, 2, 1 };

    if (allow_speculative
        && (op_type != read_op
          || descriptor_data->op_queue_[except_op].empty()))
    {
      if (op->perform())
      {
        descriptor_lock.unlock();
        scheduler_.post_immediate_completion(op, is_continuation);
        return;
      }

      if (descriptor_data->num_kevents_ < num_kevents[op_type])
      {
        struct kevent events[2];
        BOOST_ASIO_KQUEUE_EV_SET(&events[0], descriptor, EVFILT_READ,
            EV_ADD | EV_CLEAR, 0, 0, descriptor_data);
        BOOST_ASIO_KQUEUE_EV_SET(&events[1], descriptor, EVFILT_WRITE,
            EV_ADD | EV_CLEAR, 0, 0, descriptor_data);
        if (::kevent(kqueue_fd_, events, num_kevents[op_type], 0, 0, 0) != -1)
        {
          descriptor_data->num_kevents_ = num_kevents[op_type];
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
    else
    {
      if (descriptor_data->num_kevents_ < num_kevents[op_type])
        descriptor_data->num_kevents_ = num_kevents[op_type];

      struct kevent events[2];
      BOOST_ASIO_KQUEUE_EV_SET(&events[0], descriptor, EVFILT_READ,
          EV_ADD | EV_CLEAR, 0, 0, descriptor_data);
      BOOST_ASIO_KQUEUE_EV_SET(&events[1], descriptor, EVFILT_WRITE,
          EV_ADD | EV_CLEAR, 0, 0, descriptor_data);
      ::kevent(kqueue_fd_, events, descriptor_data->num_kevents_, 0, 0, 0);
    }
  }

  descriptor_data->op_queue_[op_type].push(op);
  scheduler_.work_started();
}

void kqueue_reactor::cancel_ops(socket_type,
    kqueue_reactor::per_descriptor_data& descriptor_data)
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

void kqueue_reactor::cancel_ops_by_key(socket_type,
    kqueue_reactor::per_descriptor_data& descriptor_data,
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

void kqueue_reactor::deregister_descriptor(socket_type descriptor,
    kqueue_reactor::per_descriptor_data& descriptor_data, bool closing)
{
  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  if (!descriptor_data->shutdown_)
  {
    if (closing)
    {
      // The descriptor will be automatically removed from the kqueue when it
      // is closed.
    }
    else
    {
      struct kevent events[2];
      BOOST_ASIO_KQUEUE_EV_SET(&events[0], descriptor,
          EVFILT_READ, EV_DELETE, 0, 0, 0);
      BOOST_ASIO_KQUEUE_EV_SET(&events[1], descriptor,
          EVFILT_WRITE, EV_DELETE, 0, 0, 0);
      ::kevent(kqueue_fd_, events, descriptor_data->num_kevents_, 0, 0, 0);
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

void kqueue_reactor::deregister_internal_descriptor(socket_type descriptor,
    kqueue_reactor::per_descriptor_data& descriptor_data)
{
  if (!descriptor_data)
    return;

  mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

  if (!descriptor_data->shutdown_)
  {
    struct kevent events[2];
    BOOST_ASIO_KQUEUE_EV_SET(&events[0], descriptor,
        EVFILT_READ, EV_DELETE, 0, 0, 0);
    BOOST_ASIO_KQUEUE_EV_SET(&events[1], descriptor,
        EVFILT_WRITE, EV_DELETE, 0, 0, 0);
    ::kevent(kqueue_fd_, events, descriptor_data->num_kevents_, 0, 0, 0);

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

void kqueue_reactor::cleanup_descriptor_data(
    per_descriptor_data& descriptor_data)
{
  if (descriptor_data)
  {
    free_descriptor_state(descriptor_data);
    descriptor_data = 0;
  }
}

void kqueue_reactor::run(long usec, op_queue<operation>& ops)
{
  mutex::scoped_lock lock(mutex_);

  // Determine how long to block while waiting for events.
  timespec timeout_buf = { 0, 0 };
  timespec* timeout = usec ? get_timeout(usec, timeout_buf) : &timeout_buf;

  lock.unlock();

  // Block on the kqueue descriptor.
  struct kevent events[128];
  int num_events = kevent(kqueue_fd_, 0, 0, events, 128, timeout);

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
  // Trace the waiting events.
  for (int i = 0; i < num_events; ++i)
  {
    void* ptr = reinterpret_cast<void*>(events[i].udata);
    if (ptr != &interrupter_)
    {
      unsigned event_mask = 0;
      switch (events[i].filter)
      {
      case EVFILT_READ:
        event_mask |= BOOST_ASIO_HANDLER_REACTOR_READ_EVENT;
        break;
      case EVFILT_WRITE:
        event_mask |= BOOST_ASIO_HANDLER_REACTOR_WRITE_EVENT;
        break;
      }
      if ((events[i].flags & (EV_ERROR | EV_OOBAND)) != 0)
        event_mask |= BOOST_ASIO_HANDLER_REACTOR_ERROR_EVENT;
      BOOST_ASIO_HANDLER_REACTOR_EVENTS((context(),
            reinterpret_cast<uintmax_t>(ptr), event_mask));
    }
  }
#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

  // Dispatch the waiting events.
  for (int i = 0; i < num_events; ++i)
  {
    void* ptr = reinterpret_cast<void*>(events[i].udata);
    if (ptr == &interrupter_)
    {
      interrupter_.reset();
    }
    else
    {
      descriptor_state* descriptor_data = static_cast<descriptor_state*>(ptr);
      mutex::scoped_lock descriptor_lock(descriptor_data->mutex_);

      if (events[i].filter == EVFILT_WRITE
          && descriptor_data->num_kevents_ == 2
          && descriptor_data->op_queue_[write_op].empty())
      {
        // Some descriptor types, like serial ports, don't seem to support
        // EV_CLEAR with EVFILT_WRITE. Since we have no pending write
        // operations we'll remove the EVFILT_WRITE registration here so that
        // we don't end up in a tight spin.
        struct kevent delete_events[1];
        BOOST_ASIO_KQUEUE_EV_SET(&delete_events[0],
            descriptor_data->descriptor_, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
        ::kevent(kqueue_fd_, delete_events, 1, 0, 0, 0);
        descriptor_data->num_kevents_ = 1;
      }

      // Exception operations must be processed first to ensure that any
      // out-of-band data is read before normal data.
#if defined(__NetBSD__)
      static const unsigned int filter[max_ops] =
#else
      static const int filter[max_ops] =
#endif
        { EVFILT_READ, EVFILT_WRITE, EVFILT_READ };
      for (int j = max_ops - 1; j >= 0; --j)
      {
        if (events[i].filter == filter[j])
        {
          if (j != except_op || events[i].flags & EV_OOBAND)
          {
            while (reactor_op* op = descriptor_data->op_queue_[j].front())
            {
              if (events[i].flags & EV_ERROR)
              {
                op->ec_ = boost::system::error_code(
                    static_cast<int>(events[i].data),
                    boost::asio::error::get_system_category());
                descriptor_data->op_queue_[j].pop();
                ops.push(op);
              }
              if (op->perform())
              {
                descriptor_data->op_queue_[j].pop();
                ops.push(op);
              }
              else
                break;
            }
          }
        }
      }
    }
  }

  lock.lock();
  timer_queues_.get_ready_timers(ops);
}

void kqueue_reactor::interrupt()
{
  interrupter_.interrupt();
}

int kqueue_reactor::do_kqueue_create()
{
  int fd = ::kqueue();
  if (fd == -1)
  {
    boost::system::error_code ec(errno,
        boost::asio::error::get_system_category());
    boost::asio::detail::throw_error(ec, "kqueue");
  }
  return fd;
}

kqueue_reactor::descriptor_state* kqueue_reactor::allocate_descriptor_state()
{
  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);
  return registered_descriptors_.alloc(BOOST_ASIO_CONCURRENCY_HINT_IS_LOCKING(
        REACTOR_IO, scheduler_.concurrency_hint()));
}

void kqueue_reactor::free_descriptor_state(kqueue_reactor::descriptor_state* s)
{
  mutex::scoped_lock descriptors_lock(registered_descriptors_mutex_);
  registered_descriptors_.free(s);
}

void kqueue_reactor::do_add_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.insert(&queue);
}

void kqueue_reactor::do_remove_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.erase(&queue);
}

timespec* kqueue_reactor::get_timeout(long usec, timespec& ts)
{
  // By default we will wait no longer than 5 minutes. This will ensure that
  // any changes to the system clock are detected after no longer than this.
  const long max_usec = 5 * 60 * 1000 * 1000;
  usec = timer_queues_.wait_duration_usec(
      (usec < 0 || max_usec < usec) ? max_usec : usec);
  ts.tv_sec = usec / 1000000;
  ts.tv_nsec = (usec % 1000000) * 1000;
  return &ts;
}

} // namespace detail
} // namespace asio
} // namespace boost

#undef BOOST_ASIO_KQUEUE_EV_SET

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_KQUEUE)

#endif // BOOST_ASIO_DETAIL_IMPL_KQUEUE_REACTOR_IPP
