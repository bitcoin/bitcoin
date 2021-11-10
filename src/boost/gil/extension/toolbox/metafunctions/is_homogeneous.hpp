//
// Copyright 2012 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_TOOLBOX_METAFUNCTIONS_IS_HOMOGENEOUS_HPP
#define BOOST_GIL_EXTENSION_TOOLBOX_METAFUNCTIONS_IS_HOMOGENEOUS_HPP

#include <boost/gil/pixel.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <type_traits>

namespace boost{ namespace gil {

/// is_homogeneous metafunctions
/// \brief Determines if a pixel types are homogeneous.

template<typename C,typename CMP, int Next, int Last>
struct is_homogeneous_impl;

template<typename C, typename CMP, int Last>
struct is_homogeneous_impl<C, CMP, Last, Last> : std::true_type {};

template<typename C, typename CMP, int Next, int Last>
struct is_homogeneous_impl
    : mp11::mp_and
        <
            is_homogeneous_impl<C, CMP, Next + 1, Last>,
            std::is_same<CMP, mp11::mp_at_c<C, Next>>
        >
{};

template <typename P>
struct is_homogeneous : std::false_type {};

// pixel
template <typename C, typename L>
struct is_homogeneous<pixel<C, L>> : std::true_type {};

template <typename C, typename L >
struct is_homogeneous<pixel<C, L> const> : std::true_type {};

template <typename C, typename L>
struct is_homogeneous<pixel<C, L>&> : std::true_type {};

template <typename C, typename L>
struct is_homogeneous<pixel<C, L> const&> : std::true_type {};

// planar pixel reference
template <typename Channel, typename ColorSpace>
struct is_homogeneous<planar_pixel_reference<Channel, ColorSpace>> : std::true_type {};

template <typename Channel, typename ColorSpace>
struct is_homogeneous<planar_pixel_reference<Channel, ColorSpace> const> : std::true_type {};

template<typename C, typename CMP, int I, int Last>
struct is_homogeneous_impl_p {};

// for packed_pixel
template <typename B, typename C, typename L>
struct is_homogeneous<packed_pixel<B, C, L>>
    : is_homogeneous_impl_p
    <
        C,
        mp11::mp_at_c<C, 0>,
        1,
        mp11::mp_size<C>::value
    > {};

template< typename B
        , typename C
        , typename L
        >
struct is_homogeneous<packed_pixel<B, C, L> const>
    : is_homogeneous_impl_p
    <
        C,
        mp11::mp_at_c<C, 0>,
        1,
        mp11::mp_size<C>::value
    > {};

// for bit_aligned_pixel_reference
template <typename B, typename C, typename L, bool M>
struct is_homogeneous<bit_aligned_pixel_reference<B, C, L, M>>
    : is_homogeneous_impl<C, mp11::mp_at_c<C, 0>, 1, mp11::mp_size<C>::value>
{};

template <typename B, typename C, typename L, bool M>
struct is_homogeneous<const bit_aligned_pixel_reference<B, C, L, M> >
    : is_homogeneous_impl<C, mp11::mp_at_c<C, 0>, 1, mp11::mp_size<C>::value>
{};

}} // namespace boost::gil

#endif
