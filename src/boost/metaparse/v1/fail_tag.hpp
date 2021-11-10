#ifndef BOOST_METAPARSE_V1_FAIL_TAG_HPP
#define BOOST_METAPARSE_V1_FAIL_TAG_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2009 - 2012.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#include <boost/metaparse/v1/fwd/get_message.hpp>
#include <boost/metaparse/v1/fwd/get_position.hpp>

#include <iostream>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      struct fail_tag { typedef fail_tag type; };

      template <>
      struct get_message_impl<fail_tag>
      {
        template <class A>
        struct apply { typedef typename A::message type; };
      };

      template <>
      struct get_position_impl<fail_tag>
      {
        template <class A>
        struct apply : A::source_position {};
      };
    }
  }
}

#endif


