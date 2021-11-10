//==============================================================================
//         Copyright 2014          LRI    UMR 8623 CNRS/Univ Paris Sud XI
//         Copyright 2014          NumScale SAS
//
//          Distributed under the Boost Software License, Version 1.0.
//                 See accompanying file LICENSE.txt or copy at
//                     http://www.boost.org/LICENSE_1_0.txt
//==============================================================================
#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_NT2_NT2_RESIZE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_NT2_NT2_RESIZE_HPP_INCLUDED

#include <nt2/core/container/table/table.hpp>

#include <boost/numeric/odeint/util/same_size.hpp>

namespace boost { namespace numeric { namespace odeint {

template<typename T, typename S>
struct is_resizeable< nt2::container::table<T,S> >
{
  typedef boost::true_type type;
  static const bool value = type::value;
};

template<typename T, typename S>
struct same_size_impl< nt2::container::table<T,S>
                     , nt2::container::table<T,S>
                     >
{
  static bool same_size ( const nt2::container::table<T,S> &v1
                        , const nt2::container::table<T,S> &v2
                        )
  {
    return v1.extent() == v2.extent();
  }
};

template<typename T, typename S>
struct resize_impl< nt2::container::table<T,S>
                  , nt2::container::table<T,S>
                  >
{
  static void resize ( nt2::container::table<T,S> &v1
                     , const nt2::container::table<T,S> &v2
                     )
  {
    v1.resize( v2.extent() );
  }
};
} } }

#endif
