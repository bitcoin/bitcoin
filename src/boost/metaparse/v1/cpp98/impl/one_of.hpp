#ifndef BOOST_METAPARSE_V1_IMPL_ONE_OF_HPP
#define BOOST_METAPARSE_V1_IMPL_ONE_OF_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/error/none_of_the_expected_cases_found.hpp>
#include <boost/metaparse/v1/fail.hpp>

#include <boost/metaparse/v1/cpp98/impl/one_of_fwd_op.hpp>

#include <boost/mpl/fold.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <class Parsers>
        struct one_of
        {
          typedef one_of type;

          template <class S, class Pos>
          struct apply :
            boost::mpl::fold<
              Parsers,
              fail<error::none_of_the_expected_cases_found>::apply<S, Pos>,
              one_of_fwd_op<S, Pos>
            >::type
          {};
        };
      }
    }
  }
}

#endif

