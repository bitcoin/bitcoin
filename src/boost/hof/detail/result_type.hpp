/*=============================================================================
    Copyright (c) 2016 Paul Fultz II
    result_type.hpp
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_RESULT_TYPE_HPP
#define BOOST_HOF_GUARD_RESULT_TYPE_HPP

#include <boost/hof/detail/holder.hpp>
#include <utility>

namespace boost { namespace hof { namespace detail {

template<class F, class=void>
struct function_result_type
{};

template<class F>
struct function_result_type<F, typename holder<
    typename F::result_type
>::type>
{
    typedef typename F::result_type result_type;
};

template<class F, class G, class=void>
struct compose_function_result_type
: function_result_type<F>
{};

template<class F, class G>
struct compose_function_result_type<F, G, typename holder<
    decltype(std::declval<F>()(std::declval<typename G::result_type>()))
>::type>
{
    typedef decltype(std::declval<F>()(std::declval<typename G::result_type>())) result_type;
};

}}} // namespace boost::hof

#endif
