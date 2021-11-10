#ifndef BOOST_METAPARSE_V1_ENTIRE_INPUT_HPP
#define BOOST_METAPARSE_V1_ENTIRE_INPUT_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/first_of.hpp>
#include <boost/metaparse/v1/empty.hpp>
#include <boost/metaparse/v1/change_error_message.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P, class Msg = error::end_of_input_expected>
      struct entire_input :
        first_of<P, change_error_message<empty<void>, Msg> >
      {};

      template <class P>
      struct entire_input<P, error::end_of_input_expected> :
        first_of<P, empty<void> >
      {};
    }
  }
}

#endif

