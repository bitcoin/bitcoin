/* Copyright 2016-2017 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/poly_collection for library home page.
 */

#ifndef BOOST_POLY_COLLECTION_DETAIL_IS_CONSTRUCIBLE_HPP
#define BOOST_POLY_COLLECTION_DETAIL_IS_CONSTRUCIBLE_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(BOOST_MSVC_FULL_VER,<190023918)
/* https://connect.microsoft.com/VisualStudio/Feedback/Details/2118677,
 * fixed in VS2015U2 according to
 * https://blogs.msdn.microsoft.com/vcblog/2016/03/31/
 * visual-c-2015-update-2-bug-fixes/, via github.com/dodheim
 */

#include <boost/type_traits/is_constructible.hpp>

namespace boost{

namespace poly_collection{

namespace detail{

template<typename T,typename... Args>
struct is_constructible:std::integral_constant<
  bool,
  boost::is_constructible<T,Args...>::value
>{};

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#else
#include <type_traits>

namespace boost{

namespace poly_collection{

namespace detail{

template<typename T,typename... Args>
using is_constructible=std::is_constructible<T,Args...>;

} /* namespace poly_collection::detail */

} /* namespace poly_collection */

} /* namespace boost */

#endif
#endif
