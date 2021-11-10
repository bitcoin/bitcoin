#ifndef BOOST_METAPARSE_V1_CPP98_IMPL_LATER_RESULT_HPP
#define BOOST_METAPARSE_V1_CPP98_IMPL_LATER_RESULT_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/get_position.hpp>

#include <boost/mpl/if.hpp>
#include <boost/mpl/less.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      namespace impl
      {
        template <class R1, class R2>
        struct later_result :
          boost::mpl::if_<
            typename boost::mpl::less<
              typename get_position<R2>::type,
              typename get_position<R1>::type
            >::type,
            R1,
            R2
          >
        {};
      }
    }
  }
}

#endif

