#ifndef BOOST_METAPARSE_V1_CPP11_IMPL_IS_NONE_C_HPP
#define BOOST_METAPARSE_V1_CPP11_IMPL_IS_NONE_C_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp11/impl/is_none_c_impl.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <char... Cs>
        struct is_none_c
        {
          typedef is_none_c type;

          template <class C>
          struct apply : is_none_c_impl<C::type::value, Cs...> {};
        };
      }
    }
  }
}

#endif

