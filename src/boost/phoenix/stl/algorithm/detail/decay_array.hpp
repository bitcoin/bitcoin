// Copyright 2005 Daniel Wallin. 
// Copyright 2005 Joel de Guzman.
//
// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Modeled after range_ex, Copyright 2004 Eric Niebler

#ifndef BOOST_PHOENIX_ALGORITHM_DETAIL_DECAY_ARRAY_HPP
#define BOOST_PHOENIX_ALGORITHM_DETAIL_DECAY_ARRAY_HPP

namespace boost { namespace phoenix {
namespace detail
{
  template<typename T>
  struct decay_array
  {
      typedef T type;
  };

  template<typename T, int N>
  struct decay_array<T[N]>
  {
      typedef T* type;
  };

  template<typename T, int N>
  struct decay_array<T (&)[N]>
  {
      typedef T* type;
  };
}
}}

#endif
