#ifndef BOOST_METAPARSE_V1_REPEATED_REJECT_INCOMPLETE_HPP
#define BOOST_METAPARSE_V1_REPEATED_REJECT_INCOMPLETE_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2015.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/foldl_reject_incomplete.hpp>
#include <boost/metaparse/v1/impl/back_inserter.hpp>

#include <boost/mpl/vector.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P>
      struct repeated_reject_incomplete :
        foldl_reject_incomplete<P, boost::mpl::vector<>, impl::back_inserter>
      {};
    }
  }
}

#endif

