#ifndef BOOST_METAPARSE_V1_GET_RESULT_HPP
#define BOOST_METAPARSE_V1_GET_RESULT_HPP

//    Copyright Abel Sinkovics (abel@sinkovics.hu) 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/fwd/get_result.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class>
      struct get_result_impl;

      template <class T>
      struct get_result :
        get_result_impl<typename T::type::tag>::template apply<typename T::type>
      {};
    }
  }
}

#endif

