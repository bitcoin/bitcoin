#ifndef BOOST_METAPARSE_V1_CPP98_IMPL_POP_BACK_HPP
#define BOOST_METAPARSE_V1_CPP98_IMPL_POP_BACK_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/cpp98/fwd/string.hpp>
#include <boost/metaparse/v1/cpp98/impl/push_front_c.hpp>
#include <boost/metaparse/v1/cpp98/impl/size.hpp>
#include <boost/metaparse/v1/cpp98/impl/update_c.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <class S>
        struct pop_back;

        template <class S>
        struct pop_back :
          update_c<
            typename S::type,
            size<typename S::type>::type::value - 1,
            BOOST_NO_CHAR
          >
        {};
      }
    }
  }
}

#endif

