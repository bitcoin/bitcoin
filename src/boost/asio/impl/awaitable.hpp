//
// impl/awaitable.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_AWAITABLE_HPP
#define BOOST_ASIO_IMPL_AWAITABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <exception>
#include <new>
#include <tuple>
#include <utility>
#include <boost/asio/cancellation_signal.hpp>
#include <boost/asio/cancellation_state.hpp>
#include <boost/asio/detail/thread_context.hpp>
#include <boost/asio/detail/thread_info_base.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/post.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/this_coro.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

struct awaitable_thread_has_context_switched {};

// An awaitable_thread represents a thread-of-execution that is composed of one
// or more "stack frames", with each frame represented by an awaitable_frame.
// All execution occurs in the context of the awaitable_thread's executor. An
// awaitable_thread continues to "pump" the stack frames by repeatedly resuming
// the top stack frame until the stack is empty, or until ownership of the
// stack is transferred to another awaitable_thread object.
//
//                +------------------------------------+
//                | top_of_stack_                      |
//                |                                    V
// +--------------+---+                            +-----------------+
// |                  |                            |                 |
// | awaitable_thread |<---------------------------+ awaitable_frame |
// |                  |           attached_thread_ |                 |
// +--------------+---+           (Set only when   +---+-------------+
//                |               frames are being     |
//                |               actively pumped      | caller_
//                |               by a thread, and     |
//                |               then only for        V
//                |               the top frame.)  +-----------------+
//                |                                |                 |
//                |                                | awaitable_frame |
//                |                                |                 |
//                |                                +---+-------------+
//                |                                    |
//                |                                    | caller_
//                |                                    :
//                |                                    :
//                |                                    |
//                |                                    V
//                |                                +-----------------+
//                | bottom_of_stack_               |                 |
//                +------------------------------->| awaitable_frame |
//                                                 |                 |
//                                                 +-----------------+

template <typename Executor>
class awaitable_frame_base
{
public:
#if !defined(BOOST_ASIO_DISABLE_AWAITABLE_FRAME_RECYCLING)
  void* operator new(std::size_t size)
  {
    return boost::asio::detail::thread_info_base::allocate(
        boost::asio::detail::thread_info_base::awaitable_frame_tag(),
        boost::asio::detail::thread_context::top_of_thread_call_stack(),
        size);
  }

  void operator delete(void* pointer, std::size_t size)
  {
    boost::asio::detail::thread_info_base::deallocate(
        boost::asio::detail::thread_info_base::awaitable_frame_tag(),
        boost::asio::detail::thread_context::top_of_thread_call_stack(),
        pointer, size);
  }
#endif // !defined(BOOST_ASIO_DISABLE_AWAITABLE_FRAME_RECYCLING)

  // The frame starts in a suspended state until the awaitable_thread object
  // pumps the stack.
  auto initial_suspend() noexcept
  {
    return suspend_always();
  }

  // On final suspension the frame is popped from the top of the stack.
  auto final_suspend() noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;

      bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
        this->this_->pop_frame();
      }

      void await_resume() const noexcept
      {
      }
    };

    return result{this};
  }

  void set_except(std::exception_ptr e) noexcept
  {
    pending_exception_ = e;
  }

  void set_error(const boost::system::error_code& ec)
  {
    this->set_except(std::make_exception_ptr(boost::system::system_error(ec)));
  }

  void unhandled_exception()
  {
    set_except(std::current_exception());
  }

  void rethrow_exception()
  {
    if (pending_exception_)
    {
      std::exception_ptr ex = std::exchange(pending_exception_, nullptr);
      std::rethrow_exception(ex);
    }
  }

  void clear_cancellation_slot()
  {
    this->attached_thread_->entry_point()->cancellation_state_.slot().clear();
  }

  template <typename T>
  auto await_transform(awaitable<T, Executor> a) const
  {
    if (attached_thread_->entry_point()->throw_if_cancelled_)
      if (!!attached_thread_->get_cancellation_state().cancelled())
        do_throw_error(boost::asio::error::operation_aborted, "co_await");
    return a;
  }

  // This await transformation obtains the associated executor of the thread of
  // execution.
  auto await_transform(this_coro::executor_t) noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;

      bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
      }

      auto await_resume() const noexcept
      {
        return this_->attached_thread_->get_executor();
      }
    };

    return result{this};
  }

  // This await transformation obtains the associated cancellation state of the
  // thread of execution.
  auto await_transform(this_coro::cancellation_state_t) noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;

      bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
      }

      auto await_resume() const noexcept
      {
        return this_->attached_thread_->get_cancellation_state();
      }
    };

    return result{this};
  }

  // This await transformation resets the associated cancellation state.
  auto await_transform(this_coro::reset_cancellation_state_0_t) noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;

      bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
      }

      auto await_resume() const
      {
        return this_->attached_thread_->reset_cancellation_state();
      }
    };

    return result{this};
  }

  // This await transformation resets the associated cancellation state.
  template <typename Filter>
  auto await_transform(
      this_coro::reset_cancellation_state_1_t<Filter> reset) noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;
      Filter filter_;

      bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
      }

      auto await_resume()
      {
        return this_->attached_thread_->reset_cancellation_state(
            BOOST_ASIO_MOVE_CAST(Filter)(filter_));
      }
    };

    return result{this, BOOST_ASIO_MOVE_CAST(Filter)(reset.filter)};
  }

  // This await transformation resets the associated cancellation state.
  template <typename InFilter, typename OutFilter>
  auto await_transform(
      this_coro::reset_cancellation_state_2_t<InFilter, OutFilter> reset)
    noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;
      InFilter in_filter_;
      OutFilter out_filter_;

      bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
      }

      auto await_resume()
      {
        return this_->attached_thread_->reset_cancellation_state(
            BOOST_ASIO_MOVE_CAST(InFilter)(in_filter_),
            BOOST_ASIO_MOVE_CAST(OutFilter)(out_filter_));
      }
    };

    return result{this,
        BOOST_ASIO_MOVE_CAST(InFilter)(reset.in_filter),
        BOOST_ASIO_MOVE_CAST(OutFilter)(reset.out_filter)};
  }

  // This await transformation determines whether cancellation is propagated as
  // an exception.
  auto await_transform(this_coro::throw_if_cancelled_0_t)
    noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;

      bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
      }

      auto await_resume()
      {
        return this_->attached_thread_->throw_if_cancelled();
      }
    };

    return result{this};
  }

  // This await transformation sets whether cancellation is propagated as an
  // exception.
  auto await_transform(this_coro::throw_if_cancelled_1_t throw_if_cancelled)
    noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;
      bool value_;

      bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
      }

      auto await_resume()
      {
        this_->attached_thread_->throw_if_cancelled(value_);
      }
    };

    return result{this, throw_if_cancelled.value};
  }

  // This await transformation is used to run an async operation's initiation
  // function object after the coroutine has been suspended. This ensures that
  // immediate resumption of the coroutine in another thread does not cause a
  // race condition.
  template <typename Function>
  auto await_transform(Function f,
      typename enable_if<
        is_convertible<
          typename result_of<Function(awaitable_frame_base*)>::type,
          awaitable_thread<Executor>*
        >::value
      >::type* = 0)
  {
    struct result
    {
      Function function_;
      awaitable_frame_base* this_;

      bool await_ready() const noexcept
      {
        return false;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
        function_(this_);
      }

      void await_resume() const noexcept
      {
      }
    };

    return result{std::move(f), this};
  }

  // Access the awaitable thread's has_context_switched_ flag.
  auto await_transform(detail::awaitable_thread_has_context_switched) noexcept
  {
    struct result
    {
      awaitable_frame_base* this_;

      bool await_ready() const noexcept
      {
        return true;
      }

      void await_suspend(coroutine_handle<void>) noexcept
      {
      }

      bool& await_resume() const noexcept
      {
        return this_->attached_thread_->entry_point()->has_context_switched_;
      }
    };

    return result{this};
  }

  void attach_thread(awaitable_thread<Executor>* handler) noexcept
  {
    attached_thread_ = handler;
  }

  awaitable_thread<Executor>* detach_thread() noexcept
  {
    attached_thread_->entry_point()->has_context_switched_ = true;
    return std::exchange(attached_thread_, nullptr);
  }

  void push_frame(awaitable_frame_base<Executor>* caller) noexcept
  {
    caller_ = caller;
    attached_thread_ = caller_->attached_thread_;
    attached_thread_->entry_point()->top_of_stack_ = this;
    caller_->attached_thread_ = nullptr;
  }

  void pop_frame() noexcept
  {
    if (caller_)
      caller_->attached_thread_ = attached_thread_;
    attached_thread_->entry_point()->top_of_stack_ = caller_;
    attached_thread_ = nullptr;
    caller_ = nullptr;
  }

  void resume()
  {
    coro_.resume();
  }

  void destroy()
  {
    coro_.destroy();
  }

protected:
  coroutine_handle<void> coro_ = nullptr;
  awaitable_thread<Executor>* attached_thread_ = nullptr;
  awaitable_frame_base<Executor>* caller_ = nullptr;
  std::exception_ptr pending_exception_ = nullptr;
};

template <typename T, typename Executor>
class awaitable_frame
  : public awaitable_frame_base<Executor>
{
public:
  awaitable_frame() noexcept
  {
  }

  awaitable_frame(awaitable_frame&& other) noexcept
    : awaitable_frame_base<Executor>(std::move(other))
  {
  }

  ~awaitable_frame()
  {
    if (has_result_)
      static_cast<T*>(static_cast<void*>(result_))->~T();
  }

  awaitable<T, Executor> get_return_object() noexcept
  {
    this->coro_ = coroutine_handle<awaitable_frame>::from_promise(*this);
    return awaitable<T, Executor>(this);
  };

  template <typename U>
  void return_value(U&& u)
  {
    new (&result_) T(std::forward<U>(u));
    has_result_ = true;
  }

  template <typename... Us>
  void return_values(Us&&... us)
  {
    this->return_value(std::forward_as_tuple(std::forward<Us>(us)...));
  }

  T get()
  {
    this->caller_ = nullptr;
    this->rethrow_exception();
    return std::move(*static_cast<T*>(static_cast<void*>(result_)));
  }

private:
  alignas(T) unsigned char result_[sizeof(T)];
  bool has_result_ = false;
};

template <typename Executor>
class awaitable_frame<void, Executor>
  : public awaitable_frame_base<Executor>
{
public:
  awaitable<void, Executor> get_return_object()
  {
    this->coro_ = coroutine_handle<awaitable_frame>::from_promise(*this);
    return awaitable<void, Executor>(this);
  };

  void return_void()
  {
  }

  void get()
  {
    this->caller_ = nullptr;
    this->rethrow_exception();
  }
};

struct awaitable_thread_entry_point {};

template <typename Executor>
class awaitable_frame<awaitable_thread_entry_point, Executor>
  : public awaitable_frame_base<Executor>
{
public:
  awaitable_frame()
    : top_of_stack_(0),
      has_executor_(false),
      has_context_switched_(false),
      throw_if_cancelled_(true)
  {
  }

  ~awaitable_frame()
  {
    if (has_executor_)
      u_.executor_.~Executor();
  }

  awaitable<awaitable_thread_entry_point, Executor> get_return_object()
  {
    this->coro_ = coroutine_handle<awaitable_frame>::from_promise(*this);
    return awaitable<awaitable_thread_entry_point, Executor>(this);
  };

  void return_void()
  {
  }

  void get()
  {
    this->caller_ = nullptr;
    this->rethrow_exception();
  }

private:
  template <typename> friend class awaitable_frame_base;
  template <typename, typename> friend class awaitable_handler_base;
  template <typename> friend class awaitable_thread;

  union u
  {
    u() {}
    ~u() {}
    char c_;
    Executor executor_;
  } u_;

  awaitable_frame_base<Executor>* top_of_stack_;
  boost::asio::cancellation_slot parent_cancellation_slot_;
  boost::asio::cancellation_state cancellation_state_;
  bool has_executor_;
  bool has_context_switched_;
  bool throw_if_cancelled_;
};

template <typename Executor>
class awaitable_thread
{
public:
  typedef Executor executor_type;
  typedef cancellation_slot cancellation_slot_type;

  // Construct from the entry point of a new thread of execution.
  awaitable_thread(awaitable<awaitable_thread_entry_point, Executor> p,
      const Executor& ex, cancellation_slot parent_cancel_slot,
      cancellation_state cancel_state)
    : bottom_of_stack_(std::move(p))
  {
    bottom_of_stack_.frame_->top_of_stack_ = bottom_of_stack_.frame_;
    new (&bottom_of_stack_.frame_->u_.executor_) Executor(ex);
    bottom_of_stack_.frame_->has_executor_ = true;
    bottom_of_stack_.frame_->parent_cancellation_slot_ = parent_cancel_slot;
    bottom_of_stack_.frame_->cancellation_state_ = cancel_state;
  }

  // Transfer ownership from another awaitable_thread.
  awaitable_thread(awaitable_thread&& other) noexcept
    : bottom_of_stack_(std::move(other.bottom_of_stack_))
  {
  }

  // Clean up with a last ditch effort to ensure the thread is unwound within
  // the context of the executor.
  ~awaitable_thread()
  {
    if (bottom_of_stack_.valid())
    {
      // Coroutine "stack unwinding" must be performed through the executor.
      auto* bottom_frame = bottom_of_stack_.frame_;
      (post)(bottom_frame->u_.executor_,
          [a = std::move(bottom_of_stack_)]() mutable
          {
            (void)awaitable<awaitable_thread_entry_point, Executor>(
                std::move(a));
          });
    }
  }

  awaitable_frame<awaitable_thread_entry_point, Executor>* entry_point()
  {
    return bottom_of_stack_.frame_;
  }

  executor_type get_executor() const noexcept
  {
    return bottom_of_stack_.frame_->u_.executor_;
  }

  cancellation_state get_cancellation_state() const noexcept
  {
    return bottom_of_stack_.frame_->cancellation_state_;
  }

  void reset_cancellation_state()
  {
    bottom_of_stack_.frame_->cancellation_state_ =
      cancellation_state(bottom_of_stack_.frame_->parent_cancellation_slot_);
  }

  template <typename Filter>
  void reset_cancellation_state(BOOST_ASIO_MOVE_ARG(Filter) filter)
  {
    bottom_of_stack_.frame_->cancellation_state_ =
      cancellation_state(bottom_of_stack_.frame_->parent_cancellation_slot_,
        BOOST_ASIO_MOVE_CAST(Filter)(filter));
  }

  template <typename InFilter, typename OutFilter>
  void reset_cancellation_state(BOOST_ASIO_MOVE_ARG(InFilter) in_filter,
      BOOST_ASIO_MOVE_ARG(OutFilter) out_filter)
  {
    bottom_of_stack_.frame_->cancellation_state_ =
      cancellation_state(bottom_of_stack_.frame_->parent_cancellation_slot_,
        BOOST_ASIO_MOVE_CAST(InFilter)(in_filter),
        BOOST_ASIO_MOVE_CAST(OutFilter)(out_filter));
  }

  bool throw_if_cancelled() const
  {
    return bottom_of_stack_.frame_->throw_if_cancelled_;
  }

  void throw_if_cancelled(bool value)
  {
    bottom_of_stack_.frame_->throw_if_cancelled_ = value;
  }

  cancellation_slot_type get_cancellation_slot() const noexcept
  {
    return bottom_of_stack_.frame_->cancellation_state_.slot();
  }

  // Launch a new thread of execution.
  void launch()
  {
    bottom_of_stack_.frame_->top_of_stack_->attach_thread(this);
    pump();
  }

protected:
  template <typename> friend class awaitable_frame_base;

  // Repeatedly resume the top stack frame until the stack is empty or until it
  // has been transferred to another resumable_thread object.
  void pump()
  {
    do
      bottom_of_stack_.frame_->top_of_stack_->resume();
    while (bottom_of_stack_.frame_ && bottom_of_stack_.frame_->top_of_stack_);

    if (bottom_of_stack_.frame_)
    {
      awaitable<awaitable_thread_entry_point, Executor> a(
          std::move(bottom_of_stack_));
      a.frame_->rethrow_exception();
    }
  }

  awaitable<awaitable_thread_entry_point, Executor> bottom_of_stack_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#if !defined(GENERATING_DOCUMENTATION)
# if defined(BOOST_ASIO_HAS_STD_COROUTINE)

namespace std {

template <typename T, typename Executor, typename... Args>
struct coroutine_traits<boost::asio::awaitable<T, Executor>, Args...>
{
  typedef boost::asio::detail::awaitable_frame<T, Executor> promise_type;
};

} // namespace std

# else // defined(BOOST_ASIO_HAS_STD_COROUTINE)

namespace std { namespace experimental {

template <typename T, typename Executor, typename... Args>
struct coroutine_traits<boost::asio::awaitable<T, Executor>, Args...>
{
  typedef boost::asio::detail::awaitable_frame<T, Executor> promise_type;
};

}} // namespace std::experimental

# endif // defined(BOOST_ASIO_HAS_STD_COROUTINE)
#endif // !defined(GENERATING_DOCUMENTATION)

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_AWAITABLE_HPP
