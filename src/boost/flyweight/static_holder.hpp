/* Copyright 2006-2009 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_STATIC_HOLDER_HPP
#define BOOST_FLYWEIGHT_STATIC_HOLDER_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/flyweight/static_holder_fwd.hpp>
#include <boost/flyweight/holder_tag.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>

/* Simplest holder storing the T object as a local static variable.
 */

namespace boost{

namespace flyweights{

template<typename C>
struct static_holder_class:holder_marker
{
  static C& get()
  {
    static C c;
    return c;
  }

  typedef static_holder_class type;
  BOOST_MPL_AUX_LAMBDA_SUPPORT(1,static_holder_class,(C))
};

/* static_holder_class specifier */

struct static_holder:holder_marker
{
  template<typename C>
  struct apply
  {
    typedef static_holder_class<C> type;
  };
};

} /* namespace flyweights */

} /* namespace boost */

#endif
