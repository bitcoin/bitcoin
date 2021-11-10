//
// experimental/detail/coro_promise_allocator.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2021 Klemens D. Morgenstern
//                    (klemens dot morgenstern at gmx dot net)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_DETAIL_CORO_PROMISE_ALLOCATOR_HPP
#define BOOST_ASIO_EXPERIMENTAL_DETAIL_CORO_PROMISE_ALLOCATOR_HPP

#include <boost/asio/detail/config.hpp>
#include <boost/asio/experimental/detail/coro_traits.hpp>

namespace boost {
namespace asio {
namespace experimental {
namespace detail {

template <typename Coroutine,
     typename Executor = typename Coroutine::executor_type,
     typename Allocator = typename std::allocator_traits<
       associated_allocator_t<Executor>>:: template rebind_alloc<std::byte>,
     bool Noexcept = noexcept(std::declval<Allocator>().allocate(0u))>
struct coro_promise_allocator
{
  using allocator_type = Allocator;

  template <typename... Args>
  void* operator new(const std::size_t size, Executor executor, Args&&...)
  {
    static_assert(std::is_nothrow_move_constructible_v<allocator_type>);
    allocator_type alloc{get_associated_allocator(executor)};

    const auto csize = size + sizeof(alloc);
    const auto raw =
      std::allocator_traits<allocator_type>::allocate(alloc, csize);
    new (raw) allocator_type(std::move(alloc));
    return static_cast<std::byte*>(raw) + sizeof(alloc);
  }

  template <execution_context Context, typename... Args>
  void* operator new(const std::size_t size, Context&& ctx, Args&&... args)
  {
    return coro_promise_allocator::operator new(size,
        ctx.get_executor(), std::forward<Args>(args)...);
  }

  void operator delete(void* raw, std::size_t size )
  {
    auto * alloc_p = static_cast<allocator_type*>(raw);

    auto alloc = std::move(*alloc_p);
    alloc_p->~allocator_type();
    std::allocator_traits<allocator_type>::deallocate(alloc,
        static_cast<std::byte*>(raw), size + sizeof(allocator_type));
  }
};

template <typename Coroutine, typename Executor>
struct coro_promise_allocator<Coroutine,
    Executor, std::allocator<std::byte>, false>
{

};

template <typename Coroutine, typename Executor, typename Allocator>
struct coro_promise_allocator<Coroutine, Executor, Allocator, true>
{
  using allocator_type = Allocator;

  template <typename... Args>
  void* operator new(const std::size_t size,
      Executor executor, Args&&...) noexcept
  {
    static_assert(std::is_nothrow_move_constructible_v<allocator_type>);
    allocator_type alloc{get_associated_allocator(executor)};

    const auto csize = size + sizeof(alloc);
    const auto raw =
      std::allocator_traits<allocator_type>::allocate(alloc, csize);
    if (raw == nullptr)
      return nullptr;
    new (raw) allocator_type(std::move(alloc));
    return static_cast<std::byte*>(raw) + sizeof(alloc);
  }

  template <execution_context Context, typename... Args>
  void* operator new(const std::size_t size,
      Context&& ctx, Args&&... args) noexcept
  {
    return coro_promise_allocator::operator new(size,
        ctx.get_executor(), std::forward<Args>(args)...);
  }

  void operator delete(void* raw, std::size_t size) noexcept
  {
    auto * alloc_p = static_cast<allocator_type*>(raw);
    auto alloc = std::move(*alloc_p);
    alloc_p->~allocator_type();
    const auto csize = size + sizeof(allocator_type);
    std::allocator_traits<allocator_type>::deallocate(alloc,
        static_cast<std::byte*>(raw) - sizeof(allocator_type), csize);
  }

  static auto get_return_object_on_allocation_failure() noexcept -> Coroutine
  {
    return Coroutine{};
  }
};

} // namespace detail
} // namespace experimental
} // namespace asio
} // namespace boost

#endif // BOOST_ASIO_EXPERIMENTAL_DETAIL_CORO_PROMISE_ALLOCATOR_HPP
