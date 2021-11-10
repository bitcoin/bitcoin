//
// detail/gcc_x86_fenced_block.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_GCC_X86_FENCED_BLOCK_HPP
#define BOOST_ASIO_DETAIL_GCC_X86_FENCED_BLOCK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

#include <boost/asio/detail/noncopyable.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class gcc_x86_fenced_block
  : private noncopyable
{
public:
  enum half_t { half };
  enum full_t { full };

  // Constructor for a half fenced block.
  explicit gcc_x86_fenced_block(half_t)
  {
  }

  // Constructor for a full fenced block.
  explicit gcc_x86_fenced_block(full_t)
  {
    lbarrier();
  }

  // Destructor.
  ~gcc_x86_fenced_block()
  {
    sbarrier();
  }

private:
  static int barrier()
  {
    int r = 0, m = 1;
    __asm__ __volatile__ (
        "xchgl %0, %1" :
        "=r"(r), "=m"(m) :
        "0"(1), "m"(m) :
        "memory", "cc");
    return r;
  }

  static void lbarrier()
  {
#if defined(__SSE2__)
# if (__GNUC__ >= 4) && !defined(__INTEL_COMPILER) && !defined(__ICL)
    __builtin_ia32_lfence();
# else // (__GNUC__ >= 4) && !defined(__INTEL_COMPILER) && !defined(__ICL)
    __asm__ __volatile__ ("lfence" ::: "memory");
# endif // (__GNUC__ >= 4) && !defined(__INTEL_COMPILER) && !defined(__ICL)
#else // defined(__SSE2__)
    barrier();
#endif // defined(__SSE2__)
  }

  static void sbarrier()
  {
#if defined(__SSE2__)
# if (__GNUC__ >= 4) && !defined(__INTEL_COMPILER) && !defined(__ICL)
    __builtin_ia32_sfence();
# else // (__GNUC__ >= 4) && !defined(__INTEL_COMPILER) && !defined(__ICL)
    __asm__ __volatile__ ("sfence" ::: "memory");
# endif // (__GNUC__ >= 4) && !defined(__INTEL_COMPILER) && !defined(__ICL)
#else // defined(__SSE2__)
    barrier();
#endif // defined(__SSE2__)
  }
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

#endif // BOOST_ASIO_DETAIL_GCC_X86_FENCED_BLOCK_HPP
