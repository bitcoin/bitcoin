//
// Copyright 2012 Chung-Lin Wen
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_TOOLBOX_COLOR_SPACES_LAB_HPP
#define BOOST_GIL_EXTENSION_TOOLBOX_COLOR_SPACES_LAB_HPP

#include <boost/gil/extension/toolbox/color_spaces/xyz.hpp>

#include <boost/gil/color_convert.hpp>
#include <boost/gil.hpp> // FIXME: Include what you use, not everything, even in extensions!
#include <boost/gil/detail/mp11.hpp>

namespace boost{ namespace gil {

/// \addtogroup ColorNameModel
/// \{
namespace lab_color_space
{
/// \brief Luminance
struct luminance_t {};
/// \brief a Color Component
struct a_color_opponent_t {};
/// \brief b Color Component
struct b_color_opponent_t {};
}
/// \}

/// \ingroup ColorSpaceModel
using lab_t = mp11::mp_list
<
    lab_color_space::luminance_t,
    lab_color_space::a_color_opponent_t,
    lab_color_space::b_color_opponent_t
>;

/// \ingroup LayoutModel
using lab_layout_t = layout<lab_t>;

BOOST_GIL_DEFINE_ALL_TYPEDEFS(32f, float32_t, lab)

/// \ingroup ColorConvert
/// \brief LAB to XYZ
template <>
struct default_color_converter_impl< lab_t, xyz_t >
{
    template <typename P1, typename P2>
    void operator()( const P1& src, P2& dst ) const
    {
        using namespace lab_color_space;
        using namespace xyz_color_space;

        float32_t p = ((get_color(src, luminance_t()) + 16.f)/116.f);

        get_color(dst, y_t()) =
                1.f * powf(p, 3.f);

        get_color(dst, x_t()) =
                0.95047f * powf((p +
                                 (get_color(src, a_color_opponent_t())/500.f)
                                 ), 3.f);
        get_color(dst, z_t()) =
                1.08883f * powf((p -
                                 (get_color(src, b_color_opponent_t())/200.f)
                                 ), 3.f);
    }
};

/// \ingroup ColorConvert
/// \brief XYZ to LAB
/// \note I assume \c xyz_t
template <>
struct default_color_converter_impl< xyz_t, lab_t >
{
private:
    /// \ref http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_Lab.html
    BOOST_FORCEINLINE
    float32_t forward_companding(float32_t value) const
    {
        if (value > 216.f/24389.f)
        {
            return powf(value, 1.f/3.f);
        }
        else
        {
            return ((24389.f/27.f * value + 16.f)/116.f);
        }
    }

public:
    template <typename P1, typename P2>
    void operator()( const P1& src, P2& dst ) const
    {
        using namespace lab_color_space;

        float32_t f_y =
                forward_companding(
                    channel_convert<float32_t>(
                        get_color(src, xyz_color_space::y_t())
                        )
                    // / 1.f
                    );

        float32_t f_x =
                forward_companding(
                    channel_convert<float32_t>(
                        get_color(src, xyz_color_space::x_t())
                        )
                    * (1.f / 0.95047f)  // if the compiler is smart, it should
                                        // precalculate this, no?
                    );

        float32_t f_z =
                forward_companding(
                    channel_convert<float32_t>(
                        get_color(src, xyz_color_space::z_t())
                        )
                    * (1.f / 1.08883f)  // if the compiler is smart, it should
                                        // precalculate this, no?
                    );

        get_color(dst, luminance_t()) =
                116.f * f_y - 16.f;

        get_color(dst, a_color_opponent_t()) =
                500.f * (f_x - f_y);

        get_color(dst, b_color_opponent_t()) =
                200.f * (f_y - f_z);
    }
};


/// \ingroup ColorConvert
/// \brief RGB to LAB
template <>
struct default_color_converter_impl< rgb_t, lab_t >
{
    template <typename P1, typename P2>
    void operator()( const P1& src, P2& dst ) const
    {
        using namespace lab_color_space;

        xyz32f_pixel_t xyz32f_temp_pixel;
        default_color_converter_impl<rgb_t, xyz_t>()(src, xyz32f_temp_pixel);
        default_color_converter_impl<xyz_t, lab_t>()(xyz32f_temp_pixel, dst);
    }
};

/// \ingroup ColorConvert
/// \brief LAB to RGB
template <>
struct default_color_converter_impl<lab_t,rgb_t>
{
    template <typename P1, typename P2>
    void operator()( const P1& src, P2& dst) const
    {
        using namespace lab_color_space;

        xyz32f_pixel_t xyz32f_temp_pixel;
        default_color_converter_impl<lab_t, xyz_t>()(src, xyz32f_temp_pixel);
        default_color_converter_impl<xyz_t, rgb_t>()(xyz32f_temp_pixel, dst);
    }
};

} // namespace gil
} // namespace boost

#endif
