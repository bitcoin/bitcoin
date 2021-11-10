//
// experimental/detail/completion_handler_erasure.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2021 Klemens D. Morgenstern
//                    (klemens dot morgenstern at gmx dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_DETAIL_COMPLETION_HANDLER_ERASURE_HPP
#define BOOST_ASIO_EXPERIMENTAL_DETAIL_COMPLETION_HANDLER_ERASURE_HPP

#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/dispatch.hpp>

namespace boost {
namespace asio {

class any_io_executor;

namespace experimental {
namespace detail {

template<typename Signature, typename Executor>
struct completion_handler_erasure_base;

template<typename Func, typename Signature, typename Executor>
struct completion_handler_erasure_impl;

template<typename Return,  typename ... Args, typename Executor>
struct completion_handler_erasure_base<Return(Args...), Executor>
{
  Executor executor;

  completion_handler_erasure_base(Executor&& executor)
    : executor(std::move(executor))
  {
  }

  virtual Return call(Args ...args) = 0;
  virtual ~completion_handler_erasure_base() = default;
};

template<typename Func, typename Return,  typename ... Args, typename Executor>
struct completion_handler_erasure_impl<Func, Return(Args...), Executor> final
    : completion_handler_erasure_base<Return(Args...), Executor>
{
  completion_handler_erasure_impl(Executor&& exec, Func&& func)
    : completion_handler_erasure_base<Return(Args...), Executor>(
        std::move(exec)), func(std::move(func))
  {
  }

  virtual Return call(Args ...args) override
  {
    std::move(func)(std::move(args)...);
  }

  Func func;
};

template<typename Signature, typename Executor = any_io_executor>
struct completion_handler_erasure;

template<typename Return,  typename ... Args, typename Executor>
struct completion_handler_erasure<Return(Args...), Executor>
{
  struct deleter_t
  {
    using allocator_base = typename associated_allocator<Executor>::type;
    using allocator_type =
      typename std::allocator_traits<allocator_base>::template rebind_alloc<
        completion_handler_erasure_base<Return(Args...), Executor>>;

    allocator_type allocator;
    std::size_t size;

    template<typename Func>
    static std::unique_ptr<
        completion_handler_erasure_base<Return(Args...), Executor>, deleter_t>
    make(Executor exec, Func&& func)
    {
      using type = completion_handler_erasure_impl<
          std::remove_reference_t<Func>, Return(Args...), Executor>;
      using alloc_type =  typename std::allocator_traits<
          allocator_base>::template rebind_alloc<type>;
      auto alloc = alloc_type(get_associated_allocator(exec));
      auto size = sizeof(type);
      auto p = std::allocator_traits<alloc_type>::allocate(alloc, size);
      auto res = std::unique_ptr<type, deleter_t>(
          p, deleter_t{allocator_type(alloc), size});
      std::allocator_traits<alloc_type>::construct(alloc,
          p, std::move(exec), std::forward<Func>(func));
      return res;
    }

    void operator()(
        completion_handler_erasure_base<Return(Args...), Executor> * p)
    {
      std::allocator_traits<allocator_type>::destroy(allocator, p);
      std::allocator_traits<allocator_type>::deallocate(allocator, p, size);
    }
  };

  completion_handler_erasure(const completion_handler_erasure&) = delete;
  completion_handler_erasure(completion_handler_erasure&&) = default;
  completion_handler_erasure& operator=(
      const completion_handler_erasure&) = delete;
  completion_handler_erasure& operator=(
      completion_handler_erasure&&) = default;

  constexpr completion_handler_erasure() = default;

  constexpr completion_handler_erasure(nullptr_t)
    : completion_handler_erasure()
  {
  }

  template<typename Func>
  completion_handler_erasure(Executor exec, Func&& func)
    : impl_(deleter_t::make(std::move(exec), std::forward<Func>(func)))
  {
  }

  ~completion_handler_erasure()
  {
    if (auto f = std::exchange(impl_, nullptr); f != nullptr)
    {
      boost::asio::dispatch(f->executor,
          [f = std::move(f)]() mutable
          {
            std::move(f)->call(Args{}...);
          });
    }
  }

  Return operator()(Args ... args)
  {
    if (auto f = std::exchange(impl_, nullptr); f != nullptr)
      f->call(std::move(args)...);
  }

  constexpr bool operator==(nullptr_t) const noexcept {return impl_ == nullptr;}
  constexpr bool operator!=(nullptr_t) const noexcept {return impl_ != nullptr;}
  constexpr bool operator!() const noexcept {return impl_ == nullptr;}

private:
  std::unique_ptr<
    completion_handler_erasure_base<Return(Args...), Executor>, deleter_t>
      impl_;
};

} // namespace detail
} // namespace experimental
} // namespace asio
} // namespace boost

#endif // BOOST_ASIO_EXPERIMENTAL_DETAIL_COMPLETION_HANDLER_ERASURE_HPP
