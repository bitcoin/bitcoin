// boost lockfree
//
// Copyright (C) 2011, 2016 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LOCKFREE_DETAIL_PARAMETER_HPP
#define BOOST_LOCKFREE_DETAIL_PARAMETER_HPP

#include <boost/align/aligned_allocator.hpp>
#include <boost/core/allocator_access.hpp>
#include <boost/lockfree/detail/prefix.hpp>
#include <boost/lockfree/policies.hpp>
#include <boost/parameter/binding.hpp>
#include <boost/parameter/parameters.hpp>

#include <boost/mpl/void.hpp>



namespace boost {
namespace lockfree {
namespace detail {

namespace mpl = boost::mpl;

template <typename bound_args, typename tag_type>
struct has_arg
{
    typedef typename parameter::binding<bound_args, tag_type, mpl::void_>::type type;
    static const bool value = mpl::is_not_void_<type>::type::value;
};


template <typename bound_args>
struct extract_capacity
{
    static const bool has_capacity = has_arg<bound_args, tag::capacity>::value;

    typedef typename mpl::if_c<has_capacity,
                               typename has_arg<bound_args, tag::capacity>::type,
                               mpl::size_t< 0 >
                              >::type capacity_t;

    static const std::size_t capacity = capacity_t::value;
};


template <typename bound_args, typename T>
struct extract_allocator
{
    static const bool has_allocator = has_arg<bound_args, tag::allocator>::value;

    typedef typename mpl::if_c<has_allocator,
                               typename has_arg<bound_args, tag::allocator>::type,
                               boost::alignment::aligned_allocator<T, BOOST_LOCKFREE_CACHELINE_BYTES>
                              >::type allocator_arg;

    typedef typename boost::allocator_rebind<allocator_arg, T>::type type;
};

template <typename bound_args, bool default_ = false>
struct extract_fixed_sized
{
    static const bool has_fixed_sized = has_arg<bound_args, tag::fixed_sized>::value;

    typedef typename mpl::if_c<has_fixed_sized,
                               typename has_arg<bound_args, tag::fixed_sized>::type,
                               mpl::bool_<default_>
                              >::type type;

    static const bool value = type::value;
};


} /* namespace detail */
} /* namespace lockfree */
} /* namespace boost */

#endif /* BOOST_LOCKFREE_DETAIL_PARAMETER_HPP */
