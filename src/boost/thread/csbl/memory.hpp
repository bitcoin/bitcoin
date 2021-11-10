// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_MEMORY_HPP
#define BOOST_CSBL_MEMORY_HPP

// 20.7.2 Header <memory> synopsis

// 20.7.3, pointer traits
#include <boost/thread/csbl/memory/pointer_traits.hpp>

// 20.7.4, pointer safety
// 20.7.5, pointer alignment function

// 20.7.6, allocator argument tag
#include <boost/thread/csbl/memory/allocator_arg.hpp>

// 20.7.8, allocator traits
#include <boost/thread/csbl/memory/allocator_traits.hpp>

// 20.7.7, uses_allocator
#include <boost/thread/csbl/memory/scoped_allocator.hpp>

// 20.7.9, the default allocator:
namespace boost
{
  namespace csbl
  {
    using ::std::allocator;
  }
}
// 20.7.10, raw storage iterator:
// 20.7.11, temporary buffers:
// 20.7.12, specialized algorithms:

// 20.8.1 class template unique_ptr:
// default_delete
#include <boost/thread/csbl/memory/default_delete.hpp>
#include <boost/thread/csbl/memory/unique_ptr.hpp>

// 20.8.2.1, class bad_weak_ptr:
// 20.8.2.2, class template shared_ptr:
// 20.8.2.2.6, shared_ptr creation
// 20.8.2.2.7, shared_ptr comparisons:
// 20.8.2.2.8, shared_ptr specialized algorithms:
// 20.8.2.2.9, shared_ptr casts:
// 20.8.2.2.10, shared_ptr get_deleter:
// 20.8.2.2.11, shared_ptr I/O:
// 20.8.2.3, class template weak_ptr:
// 20.8.2.3.6, weak_ptr specialized algorithms:
// 20.8.2.3.7, class template owner_less:
// 20.8.2.4, class template enable_shared_from_this:
// 20.8.2.5, shared_ptr atomic access:
// 20.8.2.6 hash support

#endif // header
