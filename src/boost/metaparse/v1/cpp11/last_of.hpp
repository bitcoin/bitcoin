#ifndef BOOST_METAPARSE_V1_CPP11_LAST_OF_HPP
#define BOOST_METAPARSE_V1_CPP11_LAST_OF_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp11/impl/nth_of_c.hpp>

#include <boost/metaparse/v1/fail.hpp>
#include <boost/metaparse/v1/error/index_out_of_range.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class... Ps>
      struct last_of
      {
        typedef last_of type;

        template <class S, class Pos>
        struct apply : impl::nth_of_c<sizeof...(Ps) - 1, S, Pos, Ps...> {};
      };

      template <>
      struct last_of<> : fail<error::index_out_of_range<0, -1, 0>> {};
    }
  }
}

#endif

