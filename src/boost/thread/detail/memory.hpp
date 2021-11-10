//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011-2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_DETAIL_MEMORY_HPP
#define BOOST_THREAD_DETAIL_MEMORY_HPP

#include <boost/config.hpp>

#include <boost/thread/csbl/memory/pointer_traits.hpp>
#include <boost/thread/csbl/memory/allocator_arg.hpp>
#include <boost/thread/csbl/memory/allocator_traits.hpp>
#include <boost/thread/csbl/memory/scoped_allocator.hpp>

namespace boost
{
  namespace thread_detail
  {
    template <class _Alloc>
    class allocator_destructor
    {
      typedef csbl::allocator_traits<_Alloc> alloc_traits;
    public:
      typedef typename alloc_traits::pointer pointer;
      typedef typename alloc_traits::size_type size_type;
    private:
      _Alloc alloc_;
      size_type s_;
    public:
      allocator_destructor(_Alloc& a, size_type s)BOOST_NOEXCEPT
      : alloc_(a), s_(s)
      {}
      void operator()(pointer p)BOOST_NOEXCEPT
      {
        alloc_traits::destroy(alloc_, p);
        alloc_traits::deallocate(alloc_, p, s_);
      }
    };
  } //namespace thread_detail
}
#endif //  BOOST_THREAD_DETAIL_MEMORY_HPP
