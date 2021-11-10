//-----------------------------------------------------------------------------
// boost variant/detail/element_index.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2014-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_DETAIL_ELEMENT_INDEX_HPP
#define BOOST_VARIANT_DETAIL_ELEMENT_INDEX_HPP

#include <boost/config.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/mpl/find_if.hpp>

namespace boost { namespace detail { namespace variant {

template <class VariantElement, class T>
struct variant_element_functor :
    boost::mpl::or_<
        boost::is_same<VariantElement, T>,
        boost::is_same<VariantElement, boost::recursive_wrapper<T> >,
        boost::is_same<VariantElement, T& >
    >
{};

template <class Types, class T>
struct element_iterator_impl :
    boost::mpl::find_if<
        Types,
        boost::mpl::or_<
            variant_element_functor<boost::mpl::_1, T>,
            variant_element_functor<boost::mpl::_1, typename boost::remove_cv<T>::type >
        >
    >
{};

template <class Variant, class T>
struct element_iterator :
    element_iterator_impl< typename Variant::types, typename boost::remove_reference<T>::type >
{};

template <class Variant, class T>
struct holds_element :
    boost::mpl::not_<
        boost::is_same<
            typename boost::mpl::end<typename Variant::types>::type,
            typename element_iterator<Variant, T>::type
        >
    >
{};


}}} // namespace boost::detail::variant

#endif // BOOST_VARIANT_DETAIL_ELEMENT_INDEX_HPP
