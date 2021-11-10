/*=============================================================================
    Copyright (c) 2012 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_CLEAR_10172012_0940)
#define BOOST_FUSION_CLEAR_10172012_0940

#include <boost/mpl/identity.hpp>
#include <boost/fusion/adapted/std_tuple/tag_of.hpp>

namespace boost { namespace fusion { namespace detail
{
  template <typename Tag>
  struct clear;

  template <>
  struct clear<std_tuple_tag> : mpl::identity<std::tuple<> > {};

}}}

#endif
