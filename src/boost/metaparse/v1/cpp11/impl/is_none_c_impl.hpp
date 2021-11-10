#ifndef BOOST_METAPARSE_V1_CPP11_IMPL_IS_NONE_C_IMPL_HPP
#define BOOST_METAPARSE_V1_CPP11_IMPL_IS_NONE_C_IMPL_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mpl/bool.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <char D, char... Cs>
        struct is_none_c_impl;

        template <char D>
        struct is_none_c_impl<D> : boost::mpl::bool_<true> {};

        template <char D, char... Cs>
        struct is_none_c_impl<D, D, Cs...> : boost::mpl::bool_<false> {};

        template <char D, char C, char... Cs>
        struct is_none_c_impl<D, C, Cs...> : is_none_c_impl<D, Cs...> {};
      }
    }
  }
}

#endif

