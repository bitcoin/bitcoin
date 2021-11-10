#ifndef BOOST_METAPARSE_V1_REPEATED1_HPP
#define BOOST_METAPARSE_V1_REPEATED1_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2009 - 2010.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/foldl1.hpp>
#include <boost/metaparse/v1/impl/back_inserter.hpp>

#include <boost/mpl/vector.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P>
      struct repeated1 :
        foldl1<P, boost::mpl::vector<>, impl::back_inserter>
      {};
    }
  }
}

#endif

