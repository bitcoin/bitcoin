#ifndef BOOST_METAPARSE_V1_CPP11_IMPL_PUSH_FRONT_RESULT_HPP
#define BOOST_METAPARSE_V1_CPP11_IMPL_PUSH_FRONT_RESULT_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/get_result.hpp>

#include <boost/mpl/push_front.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <class Value>
        struct push_front_result
        {
          typedef push_front_result type;

          template <class Seq>
          struct apply :
            boost::mpl::push_front<Seq, typename get_result<Value>::type>
          {};
        };
      }
    }
  }
}

#endif
