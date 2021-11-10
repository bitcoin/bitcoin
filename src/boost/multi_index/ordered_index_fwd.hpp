/* Copyright 2003-2015 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_ORDERED_INDEX_FWD_HPP
#define BOOST_MULTI_INDEX_ORDERED_INDEX_FWD_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/multi_index/detail/ord_index_args.hpp>
#include <boost/multi_index/detail/ord_index_impl_fwd.hpp>

namespace boost{

namespace multi_index{

/* ordered_index specifiers */

template<typename Arg1,typename Arg2=mpl::na,typename Arg3=mpl::na>
struct ordered_unique;

template<typename Arg1,typename Arg2=mpl::na,typename Arg3=mpl::na>
struct ordered_non_unique;

} /* namespace multi_index */

} /* namespace boost */

#endif
