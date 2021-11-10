/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2018 Andrey Semashev
 */
/*!
 * \file   atomic/detail/type_traits/is_iec559.hpp
 *
 * This header defines \c is_iec559 type trait
 */

#ifndef BOOST_ATOMIC_DETAIL_TYPE_TRAITS_IS_IEC559_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_TYPE_TRAITS_IS_IEC559_HPP_INCLUDED_

#include <limits>
#include <boost/atomic/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< typename T >
struct is_iec559
{
    static BOOST_CONSTEXPR_OR_CONST bool value = !!std::numeric_limits< T >::is_iec559;
};

#if defined(BOOST_HAS_FLOAT128)
// libstdc++ does not specialize numeric_limits for __float128
template< >
struct is_iec559< boost::float128_type >
{
    static BOOST_CONSTEXPR_OR_CONST bool value = true;
};
#endif // defined(BOOST_HAS_FLOAT128)

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_TYPE_TRAITS_IS_IEC559_HPP_INCLUDED_
