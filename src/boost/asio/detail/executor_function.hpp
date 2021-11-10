//
// detail/executor_function.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_EXECUTOR_FUNCTION_HPP
#define BOOST_ASIO_DETAIL_EXECUTOR_FUNCTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/memory.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

#if defined(BOOST_ASIO_HAS_MOVE)

// Lightweight, move-only function object wrapper.
class executor_function
{
public:
  template <typename F, typename Alloc>
  explicit executor_function(F f, const Alloc& a)
  {
    // Allocate and construct an object to wrap the function.
    typedef impl<F, Alloc> impl_type;
    typename impl_type::ptr p = {
      detail::addressof(a), impl_type::ptr::allocate(a), 0 };
    impl_ = new (p.v) impl_type(BOOST_ASIO_MOVE_CAST(F)(f), a);
    p.v = 0;
  }

  executor_function(executor_function&& other) BOOST_ASIO_NOEXCEPT
    : impl_(other.impl_)
  {
    other.impl_ = 0;
  }

  ~executor_function()
  {
    if (impl_)
      impl_->complete_(impl_, false);
  }

  void operator()()
  {
    if (impl_)
    {
      impl_base* i = impl_;
      impl_ = 0;
      i->complete_(i, true);
    }
  }

private:
  // Base class for polymorphic function implementations.
  struct impl_base
  {
    void (*complete_)(impl_base*, bool);
  };

  // Polymorphic function implementation.
  template <typename Function, typename Alloc>
  struct impl : impl_base
  {
    BOOST_ASIO_DEFINE_TAGGED_HANDLER_ALLOCATOR_PTR(
        thread_info_base::executor_function_tag, impl);

    template <typename F>
    impl(BOOST_ASIO_MOVE_ARG(F) f, const Alloc& a)
      : function_(BOOST_ASIO_MOVE_CAST(F)(f)),
        allocator_(a)
    {
      complete_ = &executor_function::complete<Function, Alloc>;
    }

    Function function_;
    Alloc allocator_;
  };

  // Helper to complete function invocation.
  template <typename Function, typename Alloc>
  static void complete(impl_base* base, bool call)
  {
    // Take ownership of the function object.
    impl<Function, Alloc>* i(static_cast<impl<Function, Alloc>*>(base));
    Alloc allocator(i->allocator_);
    typename impl<Function, Alloc>::ptr p = {
      detail::addressof(allocator), i, i };

    // Make a copy of the function so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the function may be the true owner of the memory
    // associated with the function. Consequently, a local copy of the function
    // is required to ensure that any owning sub-object remains valid until
    // after we have deallocated the memory here.
    Function function(BOOST_ASIO_MOVE_CAST(Function)(i->function_));
    p.reset();

    // Make the upcall if required.
    if (call)
    {
      boost_asio_handler_invoke_helpers::invoke(function, function);
    }
  }

  impl_base* impl_;
};

#else // defined(BOOST_ASIO_HAS_MOVE)

// Not so lightweight, copyable function object wrapper.
class executor_function
{
public:
  template <typename F, typename Alloc>
  explicit executor_function(const F& f, const Alloc&)
    : impl_(new impl<typename decay<F>::type>(f))
  {
  }

  void operator()()
  {
    impl_->complete_(impl_.get());
  }

private:
  // Base class for polymorphic function implementations.
  struct impl_base
  {
    void (*complete_)(impl_base*);
  };

  // Polymorphic function implementation.
  template <typename F>
  struct impl : impl_base
  {
    impl(const F& f)
      : function_(f)
    {
      complete_ = &executor_function::complete<F>;
    }

    F function_;
  };

  // Helper to complete function invocation.
  template <typename F>
  static void complete(impl_base* i)
  {
    static_cast<impl<F>*>(i)->function_();
  }

  shared_ptr<impl_base> impl_;
};

#endif // defined(BOOST_ASIO_HAS_MOVE)

// Lightweight, non-owning, copyable function object wrapper.
class executor_function_view
{
public:
  template <typename F>
  explicit executor_function_view(F& f) BOOST_ASIO_NOEXCEPT
    : complete_(&executor_function_view::complete<F>),
      function_(&f)
  {
  }

  void operator()()
  {
    complete_(function_);
  }

private:
  // Helper to complete function invocation.
  template <typename F>
  static void complete(void* f)
  {
    (*static_cast<F*>(f))();
  }

  void (*complete_)(void*);
  void* function_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_EXECUTOR_FUNCTION_HPP
