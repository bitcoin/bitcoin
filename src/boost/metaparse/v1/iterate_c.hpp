#ifndef BOOST_METAPARSE_V1_ITERATE_C_HPP
#define BOOST_METAPARSE_V1_ITERATE_C_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/metaparse/v1/impl/iterate_impl.hpp>

#include <boost/mpl/deque.hpp>

namespace boost
{
  namespace metaparse
  {
    namespace v1
    {
      template <class P, int N>
      struct iterate_c : impl::iterate_impl<N, P, boost::mpl::deque<> > {};
    }
  }
}

#endif

