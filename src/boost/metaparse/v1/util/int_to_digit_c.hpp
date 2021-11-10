#ifndef BOOST_METAPARSE_V1_UTIL_INT_TO_DIGIT_C_HPP
#define BOOST_METAPARSE_V1_UTIL_INT_TO_DIGIT_C_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/mpl/char.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace util
      {
        template <int N>
        struct int_to_digit_c;

        template <> struct int_to_digit_c<0> : boost::mpl::char_<'0'> {};
        template <> struct int_to_digit_c<1> : boost::mpl::char_<'1'> {};
        template <> struct int_to_digit_c<2> : boost::mpl::char_<'2'> {};
        template <> struct int_to_digit_c<3> : boost::mpl::char_<'3'> {};
        template <> struct int_to_digit_c<4> : boost::mpl::char_<'4'> {};
        template <> struct int_to_digit_c<5> : boost::mpl::char_<'5'> {};
        template <> struct int_to_digit_c<6> : boost::mpl::char_<'6'> {};
        template <> struct int_to_digit_c<7> : boost::mpl::char_<'7'> {};
        template <> struct int_to_digit_c<8> : boost::mpl::char_<'8'> {};
        template <> struct int_to_digit_c<9> : boost::mpl::char_<'9'> {};
      }
    }
  }
}

#endif

