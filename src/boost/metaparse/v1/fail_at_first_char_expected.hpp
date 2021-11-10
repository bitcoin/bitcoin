#ifndef BOOST_METAPARSE_V1_FAIL_AT_FIRST_CHAR_EXPECTED_HPP
#define BOOST_METAPARSE_V1_FAIL_AT_FIRST_CHAR_EXPECTED_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/impl/void_.hpp>
#include <boost/metaparse/v1/accept.hpp>
#include <boost/metaparse/v1/reject.hpp>
#include <boost/metaparse/v1/is_error.hpp>
#include <boost/metaparse/v1/get_position.hpp>

#include <boost/metaparse/v1/error/expected_to_fail.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P>
      struct fail_at_first_char_expected
      {
      private:
        template <class S, class Pos>
        struct apply_err :
          boost::mpl::eval_if<
            typename boost::mpl::equal_to<
              Pos,
              typename get_position<typename P::template apply<S, Pos> >::type
            >::type,
            accept<impl::void_, S, Pos>,
            typename P::template apply<S, Pos>
          >
        {};
      public:
        typedef fail_at_first_char_expected type;

        template <class S, class Pos>
        struct apply :
          boost::mpl::eval_if<
            typename is_error<typename P::template apply<S, Pos> >::type,
            apply_err<S, Pos>,
            reject<error::expected_to_fail, Pos>
          >
        {};
      };
    }
  }
}

#endif

