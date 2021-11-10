//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_COLOR_CONVERT_HPP
#define BOOST_GIL_COLOR_CONVERT_HPP

#include <boost/gil/channel_algorithm.hpp>
#include <boost/gil/cmyk.hpp>
#include <boost/gil/color_base_algorithm.hpp>
#include <boost/gil/gray.hpp>
#include <boost/gil/metafunctions.hpp>
#include <boost/gil/pixel.hpp>
#include <boost/gil/rgb.hpp>
#include <boost/gil/rgba.hpp>
#include <boost/gil/utilities.hpp>

#include <algorithm>
#include <functional>
#include <type_traits>

namespace boost { namespace gil {

/// Support for fast and simple color conversion.
/// Accurate color conversion using color profiles can be supplied separately in a dedicated module.

// Forward-declare
template <typename P> struct channel_type;

////////////////////////////////////////////////////////////////////////////////////////
///
///                 COLOR SPACE CONVERSION
///
////////////////////////////////////////////////////////////////////////////////////////

/// \ingroup ColorConvert
/// \brief Color Convertion function object. To be specialized for every src/dst color space
template <typename C1, typename C2>
struct default_color_converter_impl
{
    static_assert(
        std::is_same<C1, C2>::value,
        "default_color_converter_impl not specialized for given color spaces");
};

/// \ingroup ColorConvert
/// \brief When the color space is the same, color convertion performs channel depth conversion
template <typename C>
struct default_color_converter_impl<C,C> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        static_for_each(src,dst,default_channel_converter());
    }
};

namespace detail {

/// red * .3 + green * .59 + blue * .11 + .5

// The default implementation of to_luminance uses float0..1 as the intermediate channel type
template <typename RedChannel, typename GreenChannel, typename BlueChannel, typename GrayChannelValue>
struct rgb_to_luminance_fn {
    GrayChannelValue operator()(const RedChannel& red, const GreenChannel& green, const BlueChannel& blue) const {
        return channel_convert<GrayChannelValue>(float32_t(
            channel_convert<float32_t>(red  )*0.30f +
            channel_convert<float32_t>(green)*0.59f +
            channel_convert<float32_t>(blue )*0.11f) );
    }
};

// performance specialization for unsigned char
template <typename GrayChannelValue>
struct rgb_to_luminance_fn<uint8_t,uint8_t,uint8_t, GrayChannelValue> {
    GrayChannelValue operator()(uint8_t red, uint8_t green, uint8_t blue) const {
        return channel_convert<GrayChannelValue>(uint8_t(
            ((uint32_t(red  )*4915 + uint32_t(green)*9667 + uint32_t(blue )*1802) + 8192) >> 14));
    }
};

template <typename GrayChannel, typename RedChannel, typename GreenChannel, typename BlueChannel>
typename channel_traits<GrayChannel>::value_type rgb_to_luminance(const RedChannel& red, const GreenChannel& green, const BlueChannel& blue) {
    return rgb_to_luminance_fn<RedChannel,GreenChannel,BlueChannel,
                               typename channel_traits<GrayChannel>::value_type>()(red,green,blue);
}

}   // namespace detail

/// \ingroup ColorConvert
/// \brief Gray to RGB
template <>
struct default_color_converter_impl<gray_t,rgb_t> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        get_color(dst,red_t())  =
            channel_convert<typename color_element_type<P2, red_t  >::type>(get_color(src,gray_color_t()));
        get_color(dst,green_t())=
            channel_convert<typename color_element_type<P2, green_t>::type>(get_color(src,gray_color_t()));
        get_color(dst,blue_t()) =
            channel_convert<typename color_element_type<P2, blue_t >::type>(get_color(src,gray_color_t()));
    }
};

/// \ingroup ColorConvert
/// \brief Gray to CMYK
/// \todo FIXME: Where does this calculation come from? Shouldn't gray be inverted?
///              Currently, white becomes black and black becomes white.
template <>
struct default_color_converter_impl<gray_t,cmyk_t> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        get_color(dst,cyan_t())=
            channel_traits<typename color_element_type<P2, cyan_t   >::type>::min_value();
        get_color(dst,magenta_t())=
            channel_traits<typename color_element_type<P2, magenta_t>::type>::min_value();
        get_color(dst,yellow_t())=
            channel_traits<typename color_element_type<P2, yellow_t >::type>::min_value();
        get_color(dst,black_t())=
            channel_convert<typename color_element_type<P2, black_t >::type>(get_color(src,gray_color_t()));
    }
};

/// \ingroup ColorConvert
/// \brief RGB to Gray
template <>
struct default_color_converter_impl<rgb_t,gray_t> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        get_color(dst,gray_color_t()) =
            detail::rgb_to_luminance<typename color_element_type<P2,gray_color_t>::type>(
                get_color(src,red_t()), get_color(src,green_t()), get_color(src,blue_t())
            );
    }
};


/// \ingroup ColorConvert
/// \brief RGB to CMYK (not the fastest code in the world)
///
/// k = min(1 - r, 1 - g, 1 - b)
/// c = (1 - r - k) / (1 - k)
/// m = (1 - g - k) / (1 - k)
/// y = (1 - b - k) / (1 - k)
/// where `1` denotes max value of channel type of destination pixel.
///
/// The conversion from RGB to CMYK is based on CMY->CMYK (Version 2)
/// from the Principles of Digital Image Processing - Fundamental Techniques
/// by Burger, Wilhelm, Burge, Mark J.
/// and it is a gross approximation not precise enough for professional work.
///
/// \todo FIXME: The original implementation did not handle properly signed CMYK pixels as destination
///
template <>
struct default_color_converter_impl<rgb_t, cmyk_t>
{
    template <typename SrcPixel, typename DstPixel>
    void operator()(SrcPixel const& src, DstPixel& dst) const
    {
        using src_t = typename channel_type<SrcPixel>::type;
        src_t const r = get_color(src, red_t());  
        src_t const g = get_color(src, green_t());
        src_t const b = get_color(src, blue_t());

        using dst_t   = typename channel_type<DstPixel>::type;
        dst_t const c = channel_invert(channel_convert<dst_t>(r)); // c = 1 - r
        dst_t const m = channel_invert(channel_convert<dst_t>(g)); // m = 1 - g
        dst_t const y = channel_invert(channel_convert<dst_t>(b)); // y = 1 - b
        dst_t const k = (std::min)(c, (std::min)(m, y));           // k = minimum(c, m, y)

        // Apply color correction, strengthening, reducing non-zero components by
        // s = 1 / (1 - k) for k < 1, where 1 denotes dst_t max, otherwise s = 1 (literal).
        dst_t const dst_max = channel_traits<dst_t>::max_value();
        dst_t const s_div   = dst_max - k;
        if (s_div != 0)
        {
            double const s              = dst_max / static_cast<double>(s_div);
            get_color(dst, cyan_t())    = static_cast<dst_t>((c - k) * s);
            get_color(dst, magenta_t()) = static_cast<dst_t>((m - k) * s);
            get_color(dst, yellow_t())  = static_cast<dst_t>((y - k) * s);
        }
        else
        {
            // Black only for k = 1 (max of dst_t)
            get_color(dst, cyan_t())    = channel_traits<dst_t>::min_value();
            get_color(dst, magenta_t()) = channel_traits<dst_t>::min_value();
            get_color(dst, yellow_t())  = channel_traits<dst_t>::min_value();
        }
        get_color(dst, black_t()) = k; 
    }
};

/// \ingroup ColorConvert
/// \brief CMYK to RGB (not the fastest code in the world)
///
/// r = 1 - min(1, c*(1-k)+k)
/// g = 1 - min(1, m*(1-k)+k)
/// b = 1 - min(1, y*(1-k)+k)
template <>
struct default_color_converter_impl<cmyk_t,rgb_t> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        using T1 = typename channel_type<P1>::type;
        get_color(dst,red_t())  =
            channel_convert<typename color_element_type<P2,red_t>::type>(
                channel_invert<T1>(
                    (std::min)(channel_traits<T1>::max_value(),
                             T1(channel_multiply(get_color(src,cyan_t()),channel_invert(get_color(src,black_t())))+get_color(src,black_t())))));
        get_color(dst,green_t())=
            channel_convert<typename color_element_type<P2,green_t>::type>(
                channel_invert<T1>(
                    (std::min)(channel_traits<T1>::max_value(),
                             T1(channel_multiply(get_color(src,magenta_t()),channel_invert(get_color(src,black_t())))+get_color(src,black_t())))));
        get_color(dst,blue_t()) =
            channel_convert<typename color_element_type<P2,blue_t>::type>(
                channel_invert<T1>(
                    (std::min)(channel_traits<T1>::max_value(),
                             T1(channel_multiply(get_color(src,yellow_t()),channel_invert(get_color(src,black_t())))+get_color(src,black_t())))));
    }
};


/// \ingroup ColorConvert
/// \brief CMYK to Gray
///
/// gray = (1 - 0.212c - 0.715m - 0.0722y) * (1 - k)
template <>
struct default_color_converter_impl<cmyk_t,gray_t> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const  {
        get_color(dst,gray_color_t())=
            channel_convert<typename color_element_type<P2,gray_color_t>::type>(
                channel_multiply(
                    channel_invert(
                       detail::rgb_to_luminance<typename color_element_type<P1,black_t>::type>(
                            get_color(src,cyan_t()),
                            get_color(src,magenta_t()),
                            get_color(src,yellow_t())
                       )
                    ),
                    channel_invert(get_color(src,black_t()))));
    }
};

namespace detail {

template <typename Pixel>
auto alpha_or_max_impl(Pixel const& p, std::true_type) -> typename channel_type<Pixel>::type
{
    return get_color(p,alpha_t());
}
template <typename Pixel>
auto alpha_or_max_impl(Pixel const&, std::false_type) -> typename channel_type<Pixel>::type
{
    return channel_traits<typename channel_type<Pixel>::type>::max_value();
}

} // namespace detail

// Returns max_value if the pixel has no alpha channel. Otherwise returns the alpha.
template <typename Pixel>
auto alpha_or_max(Pixel const& p) -> typename channel_type<Pixel>::type
{
    return detail::alpha_or_max_impl(
        p,
        mp11::mp_contains<typename color_space_type<Pixel>::type, alpha_t>());
}


/// \ingroup ColorConvert
/// \brief Converting any pixel type to RGBA. Note: Supports homogeneous pixels only.
template <typename C1>
struct default_color_converter_impl<C1,rgba_t> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        using T2 = typename channel_type<P2>::type;
        pixel<T2,rgb_layout_t> tmp;
        default_color_converter_impl<C1,rgb_t>()(src,tmp);
        get_color(dst,red_t())  =get_color(tmp,red_t());
        get_color(dst,green_t())=get_color(tmp,green_t());
        get_color(dst,blue_t()) =get_color(tmp,blue_t());
        get_color(dst,alpha_t())=channel_convert<T2>(alpha_or_max(src));
    }
};

/// \ingroup ColorConvert
///  \brief Converting RGBA to any pixel type. Note: Supports homogeneous pixels only.
///
/// Done by multiplying the alpha to get to RGB, then converting the RGB to the target pixel type
/// Note: This may be slower if the compiler doesn't optimize out constructing/destructing a temporary RGB pixel.
///       Consider rewriting if performance is an issue
template <typename C2>
struct default_color_converter_impl<rgba_t,C2> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        using T1 = typename channel_type<P1>::type;
        default_color_converter_impl<rgb_t,C2>()(
            pixel<T1,rgb_layout_t>(channel_multiply(get_color(src,red_t()),  get_color(src,alpha_t())),
                                   channel_multiply(get_color(src,green_t()),get_color(src,alpha_t())),
                                   channel_multiply(get_color(src,blue_t()), get_color(src,alpha_t())))
            ,dst);
    }
};

/// \ingroup ColorConvert
/// \brief Unfortunately RGBA to RGBA must be explicitly provided - otherwise we get ambiguous specialization error.
template <>
struct default_color_converter_impl<rgba_t,rgba_t> {
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const {
        static_for_each(src,dst,default_channel_converter());
    }
};

/// @defgroup ColorConvert Color Space Converion
/// \ingroup ColorSpaces
/// \brief Support for conversion between pixels of different color spaces and channel depths

/// \ingroup PixelAlgorithm ColorConvert
/// \brief class for color-converting one pixel to another
struct default_color_converter {
    template <typename SrcP, typename DstP>
    void operator()(const SrcP& src,DstP& dst) const {
        using SrcColorSpace = typename color_space_type<SrcP>::type;
        using DstColorSpace = typename color_space_type<DstP>::type;
        default_color_converter_impl<SrcColorSpace,DstColorSpace>()(src,dst);
    }
};

/// \ingroup PixelAlgorithm
/// \brief helper function for converting one pixel to another using GIL default color-converters
///     where ScrP models HomogeneousPixelConcept
///           DstP models HomogeneousPixelValueConcept
template <typename SrcP, typename DstP>
inline void color_convert(const SrcP& src, DstP& dst) {
    default_color_converter()(src,dst);
}

} }  // namespace boost::gil

#endif
