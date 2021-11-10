// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/string.hpp>

// Include guard is after including v1/string.hpp to make it re-includable
#ifndef BOOST_METAPARSE_STRING_HPP
#define BOOST_METAPARSE_STRING_HPP

#include <boost/metaparse/string_tag.hpp>

#ifdef BOOST_METAPARSE_STRING
#  error BOOST_METAPARSE_STRING already defined
#endif
#define BOOST_METAPARSE_STRING BOOST_METAPARSE_V1_STRING

namespace boost
{
  namespace metaparse
  {
    using v1::string;
  }
}

#endif

