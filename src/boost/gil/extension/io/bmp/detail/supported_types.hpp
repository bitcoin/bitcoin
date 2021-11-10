//
// Copyright 2008 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_BMP_DETAIL_SUPPORTED_TYPES_HPP
#define BOOST_GIL_EXTENSION_IO_BMP_DETAIL_SUPPORTED_TYPES_HPP

#include <boost/gil/extension/io/bmp/tags.hpp>

#include <boost/gil/bit_aligned_pixel_reference.hpp>
#include <boost/gil/channel.hpp>
#include <boost/gil/color_base.hpp>
#include <boost/gil/packed_pixel.hpp>
#include <boost/gil/io/base.hpp>

#include <type_traits>

namespace boost { namespace gil { namespace detail {

// Read support

template< typename Channel
        , typename ColorSpace
        >
struct bmp_read_support : read_support_false
{
    static const bmp_bits_per_pixel::type bpp = 0;
};

template< typename BitField
        , bool     Mutable
        >
struct bmp_read_support< packed_dynamic_channel_reference< BitField
                                                         , 1
                                                         , Mutable
                                                         >
                       , gray_t
                       > : read_support_true
{
    static const bmp_bits_per_pixel::type bpp = 1;
};

template< typename BitField
        , bool     Mutable
        >
struct bmp_read_support< packed_dynamic_channel_reference< BitField
                                                         , 4
                                                         , Mutable
                                                         >
                       , gray_t
                       > : read_support_true
{
    static const bmp_bits_per_pixel::type bpp = 4;
};


template<>
struct bmp_read_support<uint8_t
                       , gray_t
                       > : read_support_true
{
    static const bmp_bits_per_pixel::type bpp = 8;
};



template<>
struct bmp_read_support<uint8_t
                       , rgb_t
                       > : read_support_true
{
    static const bmp_bits_per_pixel::type bpp = 24;
};


template<>
struct bmp_read_support<uint8_t
                       , rgba_t
                       > : read_support_true
{
    static const bmp_bits_per_pixel::type bpp = 32;
};


// Write support

template< typename Channel
        , typename ColorSpace
        >
struct bmp_write_support : write_support_false
{};

template<>
struct bmp_write_support<uint8_t
                        , rgb_t
                        > : write_support_true {};

template<>
struct bmp_write_support<uint8_t
                        , rgba_t
                        > : write_support_true {};

} // namespace detail

template<typename Pixel>
struct is_read_supported<Pixel, bmp_tag>
    : std::integral_constant
    <
        bool,
        detail::bmp_read_support
        <
            typename channel_type<Pixel>::type,
            typename color_space_type<Pixel>::type
        >::is_supported
    >
{
    using parent_t = detail::bmp_read_support
        <
            typename channel_type<Pixel>::type,
            typename color_space_type<Pixel>::type
        >;

    static const typename bmp_bits_per_pixel::type bpp = parent_t::bpp;
};

template<typename Pixel>
struct is_write_supported<Pixel, bmp_tag>
    : std::integral_constant
    <
        bool,
        detail::bmp_write_support
        <
            typename channel_type<Pixel>::type,
            typename color_space_type<Pixel>::type
        >::is_supported
    >
{};

} // namespace gil
} // namespace boost

#endif
