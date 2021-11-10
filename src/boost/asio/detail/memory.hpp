//
// detail/memory.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_MEMORY_HPP
#define BOOST_ASIO_DETAIL_MEMORY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <boost/asio/detail/throw_exception.hpp>

#if !defined(BOOST_ASIO_HAS_STD_SHARED_PTR)
# include <boost/make_shared.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/weak_ptr.hpp>
#endif // !defined(BOOST_ASIO_HAS_STD_SHARED_PTR)

#if !defined(BOOST_ASIO_HAS_STD_ADDRESSOF)
# include <boost/utility/addressof.hpp>
#endif // !defined(BOOST_ASIO_HAS_STD_ADDRESSOF)

#if !defined(BOOST_ASIO_HAS_STD_ALIGNED_ALLOC) \
  && defined(BOOST_ASIO_HAS_BOOST_ALIGN) \
  && defined(BOOST_ASIO_HAS_ALIGNOF)
# include <boost/align/aligned_alloc.hpp>
#endif // !defined(BOOST_ASIO_HAS_STD_ALIGNED_ALLOC)
       //   && defined(BOOST_ASIO_HAS_BOOST_ALIGN)
       //   && defined(BOOST_ASIO_HAS_ALIGNOF)

namespace boost {
namespace asio {
namespace detail {

#if defined(BOOST_ASIO_HAS_STD_SHARED_PTR)
using std::make_shared;
using std::shared_ptr;
using std::weak_ptr;
#else // defined(BOOST_ASIO_HAS_STD_SHARED_PTR)
using boost::make_shared;
using boost::shared_ptr;
using boost::weak_ptr;
#endif // defined(BOOST_ASIO_HAS_STD_SHARED_PTR)

#if defined(BOOST_ASIO_HAS_STD_ADDRESSOF)
using std::addressof;
#else // defined(BOOST_ASIO_HAS_STD_ADDRESSOF)
using boost::addressof;
#endif // defined(BOOST_ASIO_HAS_STD_ADDRESSOF)

} // namespace detail

#if defined(BOOST_ASIO_HAS_CXX11_ALLOCATORS)
using std::allocator_arg_t;
# define BOOST_ASIO_USES_ALLOCATOR(t) \
  namespace std { \
    template <typename Allocator> \
    struct uses_allocator<t, Allocator> : true_type {}; \
  } \
  /**/
# define BOOST_ASIO_REBIND_ALLOC(alloc, t) \
  typename std::allocator_traits<alloc>::template rebind_alloc<t>
  /**/
#else // defined(BOOST_ASIO_HAS_CXX11_ALLOCATORS)
struct allocator_arg_t {};
# define BOOST_ASIO_USES_ALLOCATOR(t)
# define BOOST_ASIO_REBIND_ALLOC(alloc, t) \
  typename alloc::template rebind<t>::other
  /**/
#endif // defined(BOOST_ASIO_HAS_CXX11_ALLOCATORS)

inline void* aligned_new(std::size_t align, std::size_t size)
{
#if defined(BOOST_ASIO_HAS_STD_ALIGNED_ALLOC) && defined(BOOST_ASIO_HAS_ALIGNOF)
  size = (size % align == 0) ? size : size + (align - size % align);
  void* ptr = std::aligned_alloc(align, size);
  if (!ptr)
  {
    std::bad_alloc ex;
    boost::asio::detail::throw_exception(ex);
  }
  return ptr;
#elif defined(BOOST_ASIO_HAS_BOOST_ALIGN) && defined(BOOST_ASIO_HAS_ALIGNOF)
  size = (size % align == 0) ? size : size + (align - size % align);
  void* ptr = boost::alignment::aligned_alloc(align, size);
  if (!ptr)
  {
    std::bad_alloc ex;
    boost::asio::detail::throw_exception(ex);
  }
  return ptr;
#elif defined(BOOST_ASIO_MSVC) && defined(BOOST_ASIO_HAS_ALIGNOF)
  size = (size % align == 0) ? size : size + (align - size % align);
  void* ptr = _aligned_malloc(size, align);
  if (!ptr)
  {
    std::bad_alloc ex;
    boost::asio::detail::throw_exception(ex);
  }
  return ptr;
#else // defined(BOOST_ASIO_MSVC) && defined(BOOST_ASIO_HAS_ALIGNOF)
  (void)align;
  return ::operator new(size);
#endif // defined(BOOST_ASIO_MSVC) && defined(BOOST_ASIO_HAS_ALIGNOF)
}

inline void aligned_delete(void* ptr)
{
#if defined(BOOST_ASIO_HAS_STD_ALIGNED_ALLOC) && defined(BOOST_ASIO_HAS_ALIGNOF)
  std::free(ptr);
#elif defined(BOOST_ASIO_HAS_BOOST_ALIGN) && defined(BOOST_ASIO_HAS_ALIGNOF)
  boost::alignment::aligned_free(ptr);
#elif defined(BOOST_ASIO_MSVC) && defined(BOOST_ASIO_HAS_ALIGNOF)
  _aligned_free(ptr);
#else // defined(BOOST_ASIO_MSVC) && defined(BOOST_ASIO_HAS_ALIGNOF)
  ::operator delete(ptr);
#endif // defined(BOOST_ASIO_MSVC) && defined(BOOST_ASIO_HAS_ALIGNOF)
}

} // namespace asio
} // namespace boost

#endif // BOOST_ASIO_DETAIL_MEMORY_HPP
