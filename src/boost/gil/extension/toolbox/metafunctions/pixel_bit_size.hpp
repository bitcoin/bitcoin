//
// Copyright 2012 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_TOOLBOX_METAFUNCTIONS_PIXEL_BIT_SIZE_HPP
#define BOOST_GIL_EXTENSION_TOOLBOX_METAFUNCTIONS_PIXEL_BIT_SIZE_HPP

#include <boost/gil/bit_aligned_pixel_reference.hpp>
#include <boost/gil/packed_pixel.hpp>

namespace boost{ namespace gil {

/// pixel_bit_size metafunctions
/// \brief Accumulates the all channel size.
///
/// \code
/// using image_t = bit_aligned_image5_type<16, 16, 16, 8, 8, devicen_layout_t<5>>::type;
/// const int size = pixel_bit_size<image_t::view_t::reference>::value;
/// \endcode
template< typename PixelRef>
struct pixel_bit_size : std::integral_constant<int, 0> {};

template <typename B, typename C, typename L, bool M>
struct pixel_bit_size<bit_aligned_pixel_reference<B, C, L, M>>
    : mp11::mp_fold
    <
        C,
        std::integral_constant<int, 0>,
        mp11::mp_plus
    >
{};

template <typename B, typename C, typename L, bool M>
struct pixel_bit_size<bit_aligned_pixel_reference<B, C, L, M> const>
    : mp11::mp_fold
    <
        C,
        std::integral_constant<int, 0>,
        mp11::mp_plus
    >
{};

template <typename B, typename C, typename L>
struct pixel_bit_size<packed_pixel<B, C, L>>
    : mp11::mp_fold
    <
        C,
        std::integral_constant<int, 0>,
        mp11::mp_plus
    >

{};

template <typename B, typename C, typename L>
struct pixel_bit_size<const packed_pixel<B,C,L> >
    : mp11::mp_fold
    <
        C,
        std::integral_constant<int, 0>,
        mp11::mp_plus
    >
{};

}} // namespace boost::gil

#endif
