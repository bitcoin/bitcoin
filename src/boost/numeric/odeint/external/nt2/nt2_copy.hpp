//==============================================================================
//         Copyright 2014          LASMEA UMR 6602 CNRS/Univ. Clermont II
//         Copyright 2014          LRI    UMR 8623 CNRS/Univ Paris Sud XI
//         Copyright 2014          MetaScale SAS
//
//          Distributed under the Boost Software License, Version 1.0.
//                 See accompanying file LICENSE.txt or copy at
//                     http://www.boost.org/LICENSE_1_0.txt
//==============================================================================
#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_NT2_NT2_COPY_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_NT2_NT2_COPY_HPP_INCLUDED

#include <nt2/core/container/table/table.hpp>

#include <boost/numeric/odeint/util/copy.hpp>

namespace boost { namespace numeric { namespace odeint {

template<typename T, typename S>
struct copy_impl< nt2::container::table<T,S>
                , nt2::container::table<T,S>
                >
{
  static void copy ( const nt2::container::table<T,S> &v1
                   , nt2::container::table<T,S> &v2
                   )
  {
    v2 = v1;
  }
};
} } }

#endif
