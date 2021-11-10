#ifndef BOOST_METAPARSE_V1_CPP98_IMPL_PUSH_BACK_C_HPP
#define BOOST_METAPARSE_V1_CPP98_IMPL_PUSH_BACK_C_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp98/fwd/string.hpp>
#include <boost/metaparse/v1/cpp98/impl/update_c.hpp>
#include <boost/metaparse/v1/cpp98/impl/size.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <class S, char C>
        struct push_back_c;

        template <class S, char C>
        struct push_back_c :
          update_c<typename S::type, size<typename S::type>::type::value, C>
        {};
      }
    }
  }
}

#endif

