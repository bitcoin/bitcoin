// Copyright (C) 2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.
#if 0
#ifndef BOOST_CSBL_QUEUE_HPP
#define BOOST_CSBL_QUEUE_HPP

#include <boost/config.hpp>
// MSVC has some trouble instantiating a non_copyable type
//C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\INCLUDE\xmemory0(606) : error C2248: 'non_copyable::non_copyable' : cannot access private member declared in class 'non_copyable'
//        ..\libs\thread\test\sync\mutual_exclusion\queue_views\single_thread_pass.cpp(24) : see declaration of 'non_copyable::non_copyable'
//        ..\libs\thread\test\sync\mutual_exclusion\queue_views\single_thread_pass.cpp(23) : see declaration of 'non_copyable'
//        C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\INCLUDE\xmemory0(605) : while compiling class template member function 'void std::allocator<_Ty>::construct(_Ty *,const _Ty &)'
//        with
//        [
//            _Ty=non_copyable
//        ]
#if defined BOOST_THREAD_USES_BOOST_QUEUE || defined BOOST_NO_CXX11_RVALUE_REFERENCES || (defined _MSC_VER && _MSC_FULL_VER < 180020827)
#ifndef BOOST_THREAD_USES_BOOST_QUEUE
#define BOOST_THREAD_USES_BOOST_QUEUE
#endif
#include <boost/container/queue.hpp>
#else
#include <queue>
#endif

namespace boost
{
  namespace csbl
  {
#if defined BOOST_THREAD_USES_BOOST_QUEUE
    using ::boost::container::queue;

#else
    using ::std::queue;

#endif

  }
}
#endif // header
#endif
