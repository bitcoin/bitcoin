//
// experimental/coro.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2021 Klemens D. Morgenstern
//                    (klemens dot morgenstern at gmx dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_CORO_HPP
#define BOOST_ASIO_EXPERIMENTAL_CORO_HPP

#include <boost/asio/detail/config.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/experimental/detail/coro_promise_allocator.hpp>
#include <boost/asio/experimental/detail/partial_promise.hpp>
#include <boost/asio/experimental/detail/coro_traits.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/experimental/use_coro.hpp>
#include <boost/asio/post.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace experimental {

template <typename Yield, typename Return, typename Executor>
struct coro;

namespace detail {

template <typename Signature, typename Return, typename Executor>
struct coro_promise;

template <typename T>
struct is_noexcept : std::false_type
{
};

template <typename Return, typename... Args>
struct is_noexcept<Return(Args...)> : std::false_type
{
};

template <typename Return, typename... Args>
struct is_noexcept<Return(Args...) noexcept> : std::true_type
{
};

template <typename T>
constexpr bool is_noexcept_v = is_noexcept<T>::value;

template <typename T>
struct coro_error;

template <>
struct coro_error<boost::system::error_code>
{
  static boost::system::error_code invalid()
  {
    return error::fault;
  }

  static boost::system::error_code cancelled()
  {
    return error::operation_aborted;
  }

  static boost::system::error_code interrupted()
  {
    return error::interrupted;
  }

  static boost::system::error_code done()
  {
    return error::broken_pipe;
  }
};

template <>
struct coro_error<std::exception_ptr>
{
  static std::exception_ptr invalid()
  {
    return std::make_exception_ptr(
        boost::system::system_error(
          coro_error<boost::system::error_code>::invalid()));
  }

  static std::exception_ptr cancelled()
  {
    return std::make_exception_ptr(
        boost::system::system_error(
          coro_error<boost::system::error_code>::cancelled()));
  }

  static std::exception_ptr interrupted()
  {
    return std::make_exception_ptr(
        boost::system::system_error(
          coro_error<boost::system::error_code>::interrupted()));
  }

  static std::exception_ptr done()
  {
    return std::make_exception_ptr(
        boost::system::system_error(
          coro_error<boost::system::error_code>::done()));
  }
};

template <typename T, typename Coroutine, typename = void>
struct coro_with_arg
{
  using coro_t = Coroutine;
  T value;
  coro_t& coro;

  struct awaitable_t
  {
    T value;
    coro_t& coro;

    constexpr static bool await_ready() { return false; }

    template <typename Y, typename R, typename E>
    auto await_suspend(coroutine_handle<coro_promise<Y, R, E>> h)
      -> coroutine_handle<>
    {
      auto& hp = h.promise();
      if (hp.get_executor() == coro.get_executor())
      {
        coro.coro_->awaited_from = h;
        coro.coro_->reset_error();
        coro.coro_->input_ = std::move(value);
        struct cancel_handler
        {
          cancel_handler(coro_t& coro) : coro(coro.coro_) {}

          typename coro_t::promise_type* coro;

          void operator()(cancellation_type ct)
          {
            coro->cancel.signal.emit(ct);
          }
        };

        hp.cancel.state.slot().template emplace<cancel_handler>(coro);
        return coro.coro_->get_handle();
      } else
      {
        coro.coro_->awaited_from =
          dispatch_coroutine(hp.get_executor(), [h]() mutable
        { h.resume(); });
        coro.coro_->reset_error();
        coro.coro_->input_ = std::move(value);

        struct cancel_handler
        {
          cancel_handler(E e, coro_t& coro) : e(e), coro(coro.coro_) {}

          E e;
          typename coro_t::promise_type* coro;

          void operator()(cancellation_type ct)
          {
            boost::asio::dispatch(e,
                [ct, p = coro]() mutable
                {
                  p->cancel.signal.emit(ct);
                });
          }
        };

        hp.cancel.state.slot().template emplace<cancel_handler>(
            coro.get_executor(), coro);

        return dispatch_coroutine(
            hp.get_executor(), [h]() mutable { h.resume(); });
      }

    }

    auto await_resume() -> typename coro_t::result_type
    {
      coro.coro_->cancel.state.slot().clear();
      coro.coro_->rethrow_if();
      return std::move(coro.coro_->result_);
    }
  };

  template <typename CompletionToken>
  auto async_resume(CompletionToken&& token) &&
  {
    return coro.async_resume(std::move(value),
        std::forward<CompletionToken>(token));
  }

  auto operator co_await() &&
  {
    return awaitable_t{std::move(value), coro};
  }
};

} // namespace detail

template <typename Yield = void, typename Return = void,
    typename Executor = any_io_executor>
struct coro
{
  using traits = detail::coro_traits<Yield, Return, Executor>;

  using input_type = typename traits::input_type;
  using yield_type = typename traits::yield_type;
  using return_type = typename traits::return_type;
  using error_type = typename traits::error_type;
  using result_type = typename traits::result_type;
  constexpr static bool is_noexcept = traits::is_noexcept;

  using promise_type = detail::coro_promise<Yield, Return, Executor>;

#if !defined(GENERATING_DOCUMENTATION)
  template <typename T, typename Coroutine, typename>
  friend struct detail::coro_with_arg;
#endif // !defined(GENERATING_DOCUMENTATION)

  using executor_type = Executor;

#if !defined(GENERATING_DOCUMENTATION)
  friend struct detail::coro_promise<Yield, Return, Executor>;
#endif // !defined(GENERATING_DOCUMENTATION)

  coro() = default;

  coro(coro&& lhs) noexcept
    : coro_(std::exchange(lhs.coro_, nullptr))
  {
  }

  coro(const coro &) = delete;

  coro& operator=(coro&& lhs) noexcept
  {
    std::swap(coro_, lhs.coro_);
    return *this;
  }

  coro& operator=(const coro&) = delete;

  ~coro()
  {
    if (coro_ != nullptr)
    {
      struct destroyer
      {
        detail::coroutine_handle<promise_type> handle;

        destroyer(const detail::coroutine_handle<promise_type>& handle)
          : handle(handle)
        {
        }

        destroyer(destroyer&& lhs)
          : handle(std::exchange(lhs.handle, nullptr))
        {
        }

        destroyer(const destroyer&) = delete;

        void operator()() {}

        ~destroyer()
        {
          if (handle)
            handle.destroy();
        }
      };

      auto handle =
        detail::coroutine_handle<promise_type>::from_promise(*coro_);
      if (handle)
        boost::asio::dispatch(coro_->get_executor(), destroyer{handle});
    }
  }

  executor_type get_executor() const
  {
    if (coro_)
      return coro_->get_executor();

    if constexpr (std::is_default_constructible_v<Executor>)
      return Executor{};
    else
      throw std::logic_error("Coroutine has no executor");
  }

  template <typename CompletionToken>
    requires std::is_void_v<input_type>
  auto async_resume(CompletionToken&& token)
  {
    return async_initiate<CompletionToken,
        typename traits::completion_handler>(
          initiate_async_resume(this), token);
  }


  template <typename CompletionToken, detail::convertible_to<input_type> T>
  auto async_resume(T&& ip, CompletionToken&& token)
  {
    return async_initiate<CompletionToken,
        typename traits::completion_handler>(
          initiate_async_resume(this), token, std::forward<T>(ip));
  }

  auto operator co_await() requires (std::is_void_v<input_type>)
  {
    return awaitable_t{*this};
  }

  template <detail::convertible_to<input_type> T>
  auto operator()(T&& ip)
  {
    return detail::coro_with_arg<std::decay_t<T>, coro>{
        std::forward<T>(ip), *this};
  }

  bool is_open() const
  {
    if (coro_)
    {
      auto handle =
        detail::coroutine_handle<promise_type>::from_promise(*coro_);
      return handle && !handle.done();
    }
    else
      return false;
  }

  explicit operator bool() const { return is_open(); }

  void cancel(cancellation_type ct = cancellation_type::all)
  {
    if (is_open() && !coro_->cancel.state.cancelled())
      boost::asio::dispatch(get_executor(),
          [ct, coro = coro_] { coro->cancel.signal.emit(ct); });
  }

private:
  struct awaitable_t
  {
    coro& coro_;

    constexpr static bool await_ready() { return false; }

    template <typename Y, typename R, typename E>
    auto await_suspend(
        detail::coroutine_handle<detail::coro_promise<Y, R, E>> h)
      -> detail::coroutine_handle<>
    {
      auto& hp = h.promise();
      if (hp.get_executor() == coro_.get_executor())
      {
        coro_.coro_->awaited_from = h;
        coro_.coro_->reset_error();

        struct cancel_handler
        {
          cancel_handler(coro& coro_) : coro_(coro_.coro_) {}

          typename coro::promise_type* coro_;

          void operator()(cancellation_type ct)
          {
            coro_->cancel.signal.emit(ct);
          }
        };

        hp.cancel.state.slot().template emplace<cancel_handler>(coro_);
        return coro_.coro_->get_handle();
      }
      else
      {
        coro_.coro_->awaited_from = detail::dispatch_coroutine(
            hp.get_executor(), [h]() mutable { h.resume(); });
        coro_.coro_->reset_error();

        struct cancel_handler
        {
          cancel_handler(E e, coro& coro) : e(e), coro_(coro.coro_) {}

          E e;
          typename coro::promise_type* coro_;

          void operator()(cancellation_type ct)
          {
            boost::asio::dispatch(e,
                [ct, p = coro_]() mutable
                {
                  p->cancel.signal.emit(ct);
                });
          }
        };

        hp.cancel.state.slot().template emplace<cancel_handler>(
            coro_.get_executor(), coro_);

        return detail::dispatch_coroutine(
            hp.get_executor(), [h]() mutable { h.resume(); });
      }
    }

    auto await_resume() -> result_type
    {
      coro_.coro_->cancel.state.slot().clear();
      coro_.coro_->rethrow_if();
      if constexpr (!std::is_void_v<result_type>)
        return std::move(coro_.coro_->result_);
    }
  };

  class initiate_async_resume
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_resume(coro* self)
      : self_(self)
    {
    }

    executor_type get_executor() const noexcept
    {
      return self_->get_executor();
    }

    template <typename E, typename WaitHandler>
    auto handle(E exec, WaitHandler&& handler,
        std::true_type /* error is noexcept */,
        std::true_type  /* result is void */) const //noexcept
    {
      return [self = self_,
          h = std::forward<WaitHandler>(handler),
          exec = std::move(exec)]() mutable
      {
        assert(self);
        auto ch =
          detail::coroutine_handle<promise_type>::from_promise(*self->coro_);
        assert(ch && !ch.done());
        assert(self->coro_->awaited_from == detail::noop_coroutine());

        self->coro_->awaited_from =
          post_coroutine(std::move(exec), std::move(h));
        self->coro_->reset_error();
        ch.resume();
      };
    }

    template <typename E, typename WaitHandler>
    requires (!std::is_void_v<result_type>)
    auto handle(E exec, WaitHandler&& handler,
        std::true_type /* error is noexcept */,
        std::false_type  /* result is void */) const //noexcept
    {
      return [self = self_,
          h = std::forward<WaitHandler>(handler),
          exec = std::move(exec)]() mutable
      {
        assert(self);

        auto ch =
          detail::coroutine_handle<promise_type>::from_promise(*self->coro_);
        assert(ch && !ch.done());
        assert(self->coro_->awaited_from == detail::noop_coroutine());

        self->coro_->awaited_from =
          detail::post_coroutine(exec,
              [self, h = std::move(h)]() mutable
              {
                std::move(h)(std::move(self->coro_->result_));
              });
        self->coro_->reset_error();
        ch.resume();
      };
    }

    template <typename E, typename WaitHandler>
    auto handle(E exec, WaitHandler&& handler,
        std::false_type /* error is noexcept */,
        std::true_type  /* result is void */) const
    {
      return [self = self_,
          h = std::forward<WaitHandler>(handler),
          exec = std::move(exec)]() mutable
      {
        if (!self)
        {
          boost::asio::post(exec,
              [h = std::move(h)]() mutable
              {
                h(detail::coro_error<error_type>::invalid());
              });
          return;
        }

        auto ch =
          detail::coroutine_handle<promise_type>::from_promise(*self->coro_);
        if (!ch)
        {
          boost::asio::post(exec,
              [h = std::move(h)]() mutable
              {
                h(detail::coro_error<error_type>::invalid());
              });
        }
        else if (ch.done())
        {
          boost::asio::post(exec,
              [h = std::move(h)]() mutable
              {
                h(detail::coro_error<error_type>::done());
              });
        }
        else
        {
          assert(self->coro_->awaited_from == detail::noop_coroutine());
          self->coro_->awaited_from =
            detail::post_coroutine(exec,
                [self, h = std::move(h)]() mutable
                {
                  std::move(h)(std::move(self->coro_->error_));
                });
          self->coro_->reset_error();
          ch.resume();
        }
      };
    }

    template <typename E, typename WaitHandler>
    auto handle(E exec, WaitHandler&& handler,
        std::false_type /* error is noexcept */,
        std::false_type  /* result is void */) const
    {
      return [self = self_, h = std::forward<WaitHandler>(handler),
          exec = std::move(exec)]() mutable
      {
        if (!self)
        {
          boost::asio::post(exec,
              [h = std::move(h)]() mutable
              {
                h(detail::coro_error<error_type>::invalid(), result_type{});
              });
          return;
        }

        auto ch =
          detail::coroutine_handle<promise_type>::from_promise(*self->coro_);
        if (!ch)
        {
          boost::asio::post(exec,
              [h = std::move(h)]() mutable
              {
                h(detail::coro_error<error_type>::invalid(), result_type{});
              });
        }
        else if (ch.done())
        {
          boost::asio::post(exec,
              [h = std::move(h)]() mutable
              {
                h(detail::coro_error<error_type>::done(), result_type{});
              });
        }
        else
        {
          assert(self->coro_->awaited_from == detail::noop_coroutine());
          self->coro_->awaited_from =
              detail::post_coroutine(exec,
                       [h = std::move(h), self]() mutable
                       {
                         std::move(h)(std::move(self->coro_->error_),
                              std::move(self->coro_->result_));
                       });
          self->coro_->reset_error();
          ch.resume();
        }
      };
    }

    struct cancel_handler
    {
      promise_type* coro;

      cancel_handler(promise_type* coro) : coro(coro) {}

      void operator()(cancellation_type ct)
      {
        boost::asio::dispatch(coro->get_executor(),
            [ct, k = coro] { k->cancel.signal.emit(ct); });
      }
    };

    template <typename WaitHandler>
    void operator()(WaitHandler&& handler) const
    {
      const auto exec =
        boost::asio::prefer(
            get_associated_executor(handler, get_executor()),
            execution::outstanding_work.tracked);

      using cancel_t = decltype(self_->coro_->cancel);
      self_->coro_->cancel.~cancel_t();
      new(&self_->coro_->cancel) cancel_t();

      auto cancel = get_associated_cancellation_slot(handler);
      if (cancel.is_connected())
        cancel.template emplace<cancel_handler>(self_->coro_);

      boost::asio::dispatch(get_executor(),
           handle(exec, std::forward<WaitHandler>(handler),
             std::integral_constant<bool, is_noexcept>{},
            std::is_void<result_type>{}));
    }

    template <typename WaitHandler, typename Input>
    void operator()(WaitHandler&& handler, Input&& input) const
    {
      const auto exec =
        boost::asio::prefer(
            get_associated_executor(handler, get_executor()),
            execution::outstanding_work.tracked);
      auto cancel = get_associated_cancellation_slot(handler);

      using cancel_t = decltype(self_->coro_->cancel);
      self_->coro_->cancel.~cancel_t();
      new(&self_->coro_->cancel) cancel_t();

      if (cancel.is_connected())
        cancel.template emplace<cancel_handler>(self_->coro_);

      boost::asio::dispatch(get_executor(),
          [h = handle(exec, std::forward<WaitHandler>(handler),
            std::integral_constant<bool, is_noexcept>{},
            std::is_void<result_type>{}),
             in = std::forward<Input>(input), self = self_]() mutable
          {
            self->coro_->input_ = std::move(in);
            std::move(h)();
          });
    }

  private:
    coro* self_;
  };

  explicit coro(promise_type* const cr) : coro_(cr) {}

  promise_type* coro_{nullptr};
};

namespace detail {

template <bool IsNoexcept>
struct coro_promise_error;

template <>
struct coro_promise_error<false>
{
  std::exception_ptr error_;

  void reset_error() { error_ = std::exception_ptr{}; }

  void unhandled_exception() { error_ = std::current_exception(); }

  void rethrow_if()
  {
    if (error_)
      std::rethrow_exception(error_);
  }
};

template <>
struct coro_promise_error<true>
{
  void reset_error() {}

  void unhandled_exception() noexcept { throw; }

  void rethrow_if() {}
};

template <typename T = void>
struct yield_input
{
  T& value;
  coroutine_handle<> awaited_from{noop_coroutine()};

  bool await_ready() const noexcept
  {
    return false;
  }

  auto await_suspend(coroutine_handle<>) noexcept
  {
    return std::exchange(awaited_from, noop_coroutine());
  }

  T await_resume() const noexcept { return std::move(value); }
};

template <>
struct yield_input<void>
{
  coroutine_handle<> awaited_from{noop_coroutine()};

  bool await_ready() const noexcept
  {
    return false;
  }

  auto await_suspend(coroutine_handle<>) noexcept
  {
    return std::exchange(awaited_from, noop_coroutine());
  }

  constexpr void await_resume() const noexcept {}
};

struct coro_awaited_from
{
  coroutine_handle<> awaited_from{noop_coroutine()};

  auto final_suspend() noexcept
  {
    struct suspendor
    {
      coroutine_handle<> awaited_from;

      constexpr static bool await_ready() noexcept { return false; }

      auto await_suspend(coroutine_handle<>) noexcept
      {
        return std::exchange(awaited_from, noop_coroutine());
      }

      constexpr static void await_resume() noexcept {}
    };

    return suspendor{std::exchange(awaited_from, noop_coroutine())};
  }

  ~coro_awaited_from()
  {
    awaited_from.resume();
  }//must be on the right executor
};

template <typename Yield, typename Input, typename Return>
struct coro_promise_exchange : coro_awaited_from
{
  using result_type = coro_result_t<Yield, Return>;

  result_type result_;
  Input input_;

  auto yield_value(Yield&& y)
  {
    result_ = std::move(y);
    return yield_input<Input>{std::move(input_),
      std::exchange(awaited_from, noop_coroutine())};
  }

  auto yield_value(const Yield& y)
  {
    result_ = y;
    return yield_input<Input>{std::move(input_),
      std::exchange(awaited_from, noop_coroutine())};
  }

  void return_value(const Return& r) { result_ = r; }

  void return_value(Return&& r) { result_ = std::move(r); }
};

template <typename YieldReturn>
struct coro_promise_exchange<YieldReturn, void, YieldReturn> : coro_awaited_from
{
  using result_type = coro_result_t<YieldReturn, YieldReturn>;

  result_type result_;

  auto yield_value(const YieldReturn& y)
  {
    result_ = y;
    return yield_input<void>{std::exchange(awaited_from, noop_coroutine())};
  }

  auto yield_value(YieldReturn&& y)
  {
    result_ = std::move(y);
    return yield_input<void>{std::exchange(awaited_from, noop_coroutine())};
  }

  void return_value(const YieldReturn& r) { result_ = r; }

  void return_value(YieldReturn&& r) { result_ = std::move(r); }
};

template <typename Yield, typename Return>
struct coro_promise_exchange<Yield, void, Return> : coro_awaited_from
{
  using result_type = coro_result_t<Yield, Return>;

  result_type result_;

  auto yield_value(const Yield& y)
  {
    result_.template emplace<0>(y);
    return yield_input<void>{std::exchange(awaited_from, noop_coroutine())};
  }

  auto yield_value(Yield&& y)
  {
    result_.template emplace<0>(std::move(y));
    return yield_input<void>{std::exchange(awaited_from, noop_coroutine())};
  }

  void return_value(const Return& r)
  { result_.template emplace<1>(r); }

  void return_value(Return&& r)
  { result_.template emplace<1>(std::move(r)); }
};

template <typename Yield, typename Input>
struct coro_promise_exchange<Yield, Input, void> : coro_awaited_from
{
  using result_type = coro_result_t<Yield, void>;

  result_type result_;
  Input input_;

  auto yield_value(Yield&& y)
  {
    result_ = std::move(y);
    return yield_input<Input>{input_,
      std::exchange(awaited_from, noop_coroutine())};
  }

  auto yield_value(const Yield& y)
  {
    result_ = y;
    return yield_input<Input>{input_,
      std::exchange(awaited_from, noop_coroutine())};
  }

  void return_void() { result_.reset(); }
};

template <typename Return>
struct coro_promise_exchange<void, void, Return> : coro_awaited_from
{
  using result_type = coro_result_t<void, Return>;

  result_type result_;

  void yield_value();

  void return_value(const Return& r) { result_ = r; }

  void return_value(Return&& r) { result_ = std::move(r); }
};


template <>
struct coro_promise_exchange<void, void, void> : coro_awaited_from
{
  void return_void() {}

  void yield_value();
};

template <typename Yield>
struct coro_promise_exchange<Yield, void, void> : coro_awaited_from
{
  using result_type = coro_result_t<Yield, void>;

  result_type result_;

  auto yield_value(const Yield& y)
  {
    result_ = y;
    return yield_input<void>{std::exchange(awaited_from, noop_coroutine())};
  }

  auto yield_value(Yield&& y)
  {
    result_ = std::move(y);
    return yield_input<void>{std::exchange(awaited_from, noop_coroutine())};
  }

  void return_void() { result_.reset(); }
};

template <typename Yield, typename Return, typename Executor>
struct coro_promise final :
  coro_promise_allocator<coro<Yield, Return, Executor>>,
  coro_promise_error<coro_traits<Yield, Return, Executor>::is_noexcept>,
  coro_promise_exchange<
      typename coro_traits<Yield, Return, Executor>::yield_type,
      typename coro_traits<Yield, Return, Executor>::input_type,
      typename coro_traits<Yield, Return, Executor>::return_type>
{
  using coro_type = coro<Yield, Return, Executor>;

  auto handle()
  { return coroutine_handle<coro_promise>::from_promise(this); }

  using executor_type = Executor;

  executor_type executor_;
  struct cancel_pair
  {
    cancellation_signal signal;
    boost::asio::cancellation_state state{signal.slot()};

  };
  cancel_pair cancel;

  using allocator_type =
    typename std::allocator_traits<associated_allocator_t < Executor>>::
  template rebind_alloc<std::byte>;
  using traits = coro_traits<Yield, Return, Executor>;

  using input_type = typename traits::input_type;
  using yield_type = typename traits::yield_type;
  using return_type = typename traits::return_type;
  using error_type = typename traits::error_type;
  using result_type = typename traits::result_type;
  constexpr static bool is_noexcept = traits::is_noexcept;

  auto get_executor() const -> Executor { return executor_; }

  auto get_handle()
  {
    return coroutine_handle<coro_promise>::from_promise(*this);
  }

  template <typename... Args>
  coro_promise(Executor executor, Args &&...) noexcept
    : executor_(std::move(executor))
  {
  }

  template <execution_context Context, typename... Args>
  coro_promise(Context&& ctx, Args&&...) noexcept
    : executor_(ctx.get_executor())
  {
  }

  auto get_return_object()
  {
    return coro<Yield, Return, Executor>{this};
  }

  auto initial_suspend() noexcept
  {
    return suspend_always{};
  }

  using coro_promise_exchange<
      typename coro_traits<Yield, Return, Executor>::yield_type,
      typename coro_traits<Yield, Return, Executor>::input_type,
      typename coro_traits<Yield, Return, Executor>::return_type>::yield_value;

  auto await_transform(this_coro::executor_t) const
  {
    struct exec_helper
    {
      const executor_type& value;

      constexpr static bool await_ready() noexcept { return true; }

      constexpr static void await_suspend(coroutine_handle<>) noexcept {}

      executor_type await_resume() const noexcept { return value; }
    };

    return exec_helper{executor_};
  }

  auto await_transform(this_coro::cancellation_state_t) const
  {
    struct exec_helper
    {
      const boost::asio::cancellation_state& value;

      constexpr static bool await_ready() noexcept { return true; }

      constexpr static void await_suspend(coroutine_handle<>) noexcept {}

      const boost::asio::cancellation_state& await_resume() const noexcept
      {
        return value;
      }
    };

    return exec_helper{cancel.state};
  }

  template <typename Yield_, typename Return_, typename Executor_>
  auto await_transform(coro<Yield_, Return_, Executor_>& kr) -> decltype(auto)
  {
    return kr;
  }

  template <typename Yield_, typename Return_, typename Executor_>
  auto await_transform(coro<Yield_, Return_, Executor_>&& kr)
  {
    return std::move(kr);
  }

  template <typename T_, typename Coroutine>
  auto await_transform(coro_with_arg<T_, Coroutine>&& kr) -> decltype(auto)
  {
    return std::move(kr);
  }

  template <typename T_>
    requires requires(T_ t) {{ t.async_wait(use_coro) }; }
  auto await_transform(T_ &t) -> decltype(auto)
  {
    return await_transform(t.async_wait(use_coro));
  }

  template <typename... Ts>
  auto await_transform(coro_init_handler<Executor, Ts...>&& kr) const
  {
    if constexpr (is_noexcept)
      return std::move(kr).as_noexcept(cancel.state.slot());
    else
      return std::move(kr).as_throwing(cancel.state.slot());
  }
};

} // namespace detail
} // namespace experimental
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_CORO_HPP
