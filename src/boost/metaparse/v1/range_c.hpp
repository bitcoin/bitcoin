#ifndef BOOST_METAPARSE_V1_RANGE_C_HPP
#define BOOST_METAPARSE_V1_RANGE_C_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/accept_when.hpp>
#include <boost/metaparse/v1/one_char.hpp>
#include <boost/metaparse/v1/util/in_range_c.hpp>
#include <boost/metaparse/v1/error/unexpected_character.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <char From, char To>
      struct range_c :
        accept_when<
          one_char,
          util::in_range_c<char, From, To>,
          error::unexpected_character
        >
      {};
    }
  }
}

#endif

