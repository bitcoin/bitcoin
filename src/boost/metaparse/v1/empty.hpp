#ifndef BOOST_METAPARSE_V1_EMPTY_HPP
#define BOOST_METAPARSE_V1_EMPTY_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2010 - 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/error/end_of_input_expected.hpp>
#include <boost/metaparse/v1/reject.hpp>
#include <boost/metaparse/v1/accept.hpp>
#include <boost/metaparse/v1/define_error.hpp>

#include <boost/mpl/empty.hpp>
#include <boost/mpl/if.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class Result>
      struct empty
      {
        typedef empty type;
        
        template <class S, class Pos>
        struct apply :
          boost::mpl::if_<
            boost::mpl::empty<S>,
            accept<Result, S, Pos>,
            reject<error::end_of_input_expected, Pos>
          >
        {};
      };
    }
  }
}

#endif

