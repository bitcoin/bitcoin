//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_DYNAMIC_IMAGE_APPLY_OPERATION_HPP
#define BOOST_GIL_EXTENSION_DYNAMIC_IMAGE_APPLY_OPERATION_HPP

#include <boost/variant2/variant.hpp>

namespace boost { namespace gil {

/// \ingroup Variant
/// \brief Applies the visitor op to the variants
template <typename Variant1, typename Visitor>
BOOST_FORCEINLINE
auto apply_operation(Variant1&& arg1, Visitor&& op)
#if defined(BOOST_NO_CXX14_DECLTYPE_AUTO) || defined(BOOST_NO_CXX11_DECLTYPE_N3276)
    -> decltype(variant2::visit(std::forward<Visitor>(op), std::forward<Variant1>(arg1)))
#endif
{
    return variant2::visit(std::forward<Visitor>(op), std::forward<Variant1>(arg1));
}

/// \ingroup Variant
/// \brief Applies the visitor op to the variants
template <typename Variant1, typename Variant2, typename Visitor>
BOOST_FORCEINLINE
auto apply_operation(Variant1&& arg1, Variant2&& arg2, Visitor&& op)
#if defined(BOOST_NO_CXX14_DECLTYPE_AUTO) || defined(BOOST_NO_CXX11_DECLTYPE_N3276)
    -> decltype(variant2::visit(std::forward<Visitor>(op), std::forward<Variant1>(arg1), std::forward<Variant2>(arg2)))
#endif
{
    return variant2::visit(std::forward<Visitor>(op), std::forward<Variant1>(arg1), std::forward<Variant2>(arg2));
}

}}  // namespace boost::gil

#endif
