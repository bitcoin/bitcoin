#ifndef BOOST_METAPARSE_V1_ERROR_EXPECTED_TO_FAIL_HPP
#define BOOST_METAPARSE_V1_ERROR_EXPECTED_TO_FAIL_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/define_error.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace error
      {
        BOOST_METAPARSE_V1_DEFINE_ERROR(
          expected_to_fail,
          "Parser expected to fail"
        );
      }
    }
  }
}

#endif


