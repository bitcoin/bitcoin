/* Copyright 2016-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_EXCEPTION_HPP
#define BOOST_POLY_COLLECTION_EXCEPTION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <stdexcept>
#include <typeinfo>

namespace boost{

namespace poly_collection{

struct unregistered_type:std::logic_error
{
  unregistered_type(const std::type_info& info):
    std::logic_error{"type not registered"},
    pinfo{&info}
  {}

  const std::type_info* pinfo;
};

struct not_copy_constructible:std::logic_error
{
  not_copy_constructible(const std::type_info& info):
    std::logic_error{"type is not copy constructible"},
    pinfo{&info}
  {}

  const std::type_info* pinfo;
};

struct not_equality_comparable:std::logic_error
{
  not_equality_comparable(const std::type_info& info):
    std::logic_error{"type does not support equality comparison"},
    pinfo{&info}
  {}

  const std::type_info* pinfo;
};

} /* namespace poly_collection */

} /* namespace boost */

#endif
