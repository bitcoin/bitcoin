//
// Copyright 2012 Chung-Lin Wen, Davide Anastasia
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_TOOLBOX_COLOR_SPACES_XYZ_HPP
#define BOOST_GIL_EXTENSION_TOOLBOX_COLOR_SPACES_XYZ_HPP

#include <boost/gil/color_convert.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/detail/mp11.hpp>

namespace boost{ namespace gil {

/// \addtogroup ColorNameModel
/// \{
namespace xyz_color_space
{
/// \brief x Color Component
struct x_t {};
/// \brief y Color Component
struct y_t {};
/// \brief z Color Component
struct z_t {};
}
/// \}

/// \ingroup ColorSpaceModel
using xyz_t = mp11::mp_list
<
    xyz_color_space::x_t,
    xyz_color_space::y_t,
    xyz_color_space::z_t
>;

/// \ingroup LayoutModel
using xyz_layout_t = layout<xyz_t>;

BOOST_GIL_DEFINE_ALL_TYPEDEFS(32f, float32_t, xyz)

/// \ingroup ColorConvert
/// \brief RGB to XYZ
/// <a href="http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html">Link</a>
/// \note rgb_t is assumed to be sRGB D65
template <>
struct default_color_converter_impl< rgb_t, xyz_t >
{
private:
    BOOST_FORCEINLINE
    float32_t inverse_companding(float32_t sample) const
    {
        if ( sample > 0.04045f )
        {
            return powf((( sample + 0.055f ) / 1.055f ), 2.4f);
        }
        else
        {
            return ( sample / 12.92f );
        }
    }

public:
    template <typename P1, typename P2>
    void operator()( const P1& src, P2& dst ) const
    {
        using namespace xyz_color_space;

        float32_t red(
            inverse_companding(
                channel_convert<float32_t>(get_color(src, red_t()))));
        float32_t green(
            inverse_companding(
                channel_convert<float32_t>(get_color(src, green_t()))));
        float32_t blue(
            inverse_companding(
                channel_convert<float32_t>(get_color(src, blue_t()))));

        get_color( dst, x_t() ) =
                red * 0.4124564f +
                green * 0.3575761f +
                blue * 0.1804375f;
        get_color( dst, y_t() ) =
                red * 0.2126729f +
                green * 0.7151522f +
                blue * 0.0721750f;
        get_color( dst, z_t() ) =
                red * 0.0193339f +
                green * 0.1191920f +
                blue * 0.9503041f;
    }
};

/// \ingroup ColorConvert
/// \brief XYZ to RGB
template <>
struct default_color_converter_impl<xyz_t,rgb_t>
{
private:
    BOOST_FORCEINLINE
    float32_t companding(float32_t sample) const
    {
        if ( sample > 0.0031308f )
        {
            return ( 1.055f * powf( sample, 1.f/2.4f ) - 0.055f );
        }
        else
        {
            return ( 12.92f * sample );
        }
    }

public:
    template <typename P1, typename P2>
    void operator()( const P1& src, P2& dst) const
    {
        using namespace xyz_color_space;

        // Note: ideally channel_convert should be compiled out, because xyz_t
        // is float32_t natively only
        float32_t x( channel_convert<float32_t>( get_color( src, x_t() ) ) );
        float32_t y( channel_convert<float32_t>( get_color( src, y_t() ) ) );
        float32_t z( channel_convert<float32_t>( get_color( src, z_t() ) ) );

        get_color(dst,red_t())  =
                channel_convert<typename color_element_type<P2, red_t>::type>(
                    companding( x *  3.2404542f +
                                y * -1.5371385f +
                                z * -0.4985314f )
                    );
        get_color(dst,green_t()) =
                channel_convert<typename color_element_type<P2, green_t>::type>(
                    companding( x * -0.9692660f +
                                y *  1.8760108f +
                                z *  0.0415560f )
                    );
        get_color(dst,blue_t()) =
                channel_convert<typename color_element_type<P2, blue_t>::type>(
                    companding( x *  0.0556434f +
                                y * -0.2040259f +
                                z *  1.0572252f )
                    );
    }
};

} // namespace gil
} // namespace boost

#endif // BOOST_GIL_EXTENSION_TOOLBOX_COLOR_SPACES_XYZ_HPP
