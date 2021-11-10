//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_CONCEPTS_IMAGE_HPP
#define BOOST_GIL_CONCEPTS_IMAGE_HPP

#include <boost/gil/concepts/basic.hpp>
#include <boost/gil/concepts/concept_check.hpp>
#include <boost/gil/concepts/fwd.hpp>
#include <boost/gil/concepts/image_view.hpp>
#include <boost/gil/concepts/point.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <type_traits>

#if defined(BOOST_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedefs"
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

namespace boost { namespace gil {

/// \ingroup ImageConcept
/// \brief N-dimensional container of values
///
/// \code
/// concept RandomAccessNDImageConcept<typename Image> : Regular<Image>
/// {
///     typename view_t; where MutableRandomAccessNDImageViewConcept<view_t>;
///     typename const_view_t = view_t::const_t;
///     typename point_t      = view_t::point_t;
///     typename value_type   = view_t::value_type;
///     typename allocator_type;
///
///     Image::Image(point_t dims, std::size_t alignment=1);
///     Image::Image(point_t dims, value_type fill_value, std::size_t alignment);
///
///     void Image::recreate(point_t new_dims, std::size_t alignment=1);
///     void Image::recreate(point_t new_dims, value_type fill_value, std::size_t alignment);
///
///     const point_t&        Image::dimensions() const;
///     const const_view_t&   const_view(const Image&);
///     const view_t&         view(Image&);
/// };
/// \endcode
template <typename Image>
struct RandomAccessNDImageConcept
{
    void constraints()
    {
        gil_function_requires<Regular<Image>>();

        using view_t = typename Image::view_t;
        gil_function_requires<MutableRandomAccessNDImageViewConcept<view_t>>();

        using const_view_t = typename Image::const_view_t;
        using pixel_t = typename Image::value_type;
        using point_t = typename Image::point_t;
        gil_function_requires<PointNDConcept<point_t>>();

        const_view_t cv = const_view(image);
        ignore_unused_variable_warning(cv);
        view_t v = view(image);
        ignore_unused_variable_warning(v);

        pixel_t fill_value;
        point_t pt = image.dimensions();
        Image image1(pt);
        Image image2(pt, 1);
        Image image3(pt, fill_value, 1);
        image.recreate(pt);
        image.recreate(pt, 1);
        image.recreate(pt, fill_value, 1);
    }
    Image image;
};


/// \ingroup ImageConcept
/// \brief 2-dimensional container of values
///
/// \code
/// concept RandomAccess2DImageConcept<RandomAccessNDImageConcept Image>
/// {
///     typename x_coord_t = const_view_t::x_coord_t;
///     typename y_coord_t = const_view_t::y_coord_t;
///
///     Image::Image(x_coord_t width, y_coord_t height, std::size_t alignment=1);
///     Image::Image(x_coord_t width, y_coord_t height, value_type fill_value, std::size_t alignment);
///
///     x_coord_t Image::width() const;
///     y_coord_t Image::height() const;
///
///     void Image::recreate(x_coord_t width, y_coord_t height, std::size_t alignment=1);
///     void Image::recreate(x_coord_t width, y_coord_t height, value_type fill_value, std::size_t alignment);
/// };
/// \endcode
template <typename Image>
struct RandomAccess2DImageConcept
{
    void constraints()
    {
        gil_function_requires<RandomAccessNDImageConcept<Image>>();
        using x_coord_t = typename Image::x_coord_t;
        using y_coord_t = typename Image::y_coord_t;
        using value_t = typename Image::value_type;

        gil_function_requires<MutableRandomAccess2DImageViewConcept<typename Image::view_t>>();

        x_coord_t w=image.width();
        y_coord_t h=image.height();
        value_t fill_value;
        Image im1(w,h);
        Image im2(w,h,1);
        Image im3(w,h,fill_value,1);
        image.recreate(w,h);
        image.recreate(w,h,1);
        image.recreate(w,h,fill_value,1);
    }
    Image image;
};

/// \ingroup ImageConcept
/// \brief 2-dimensional image whose value type models PixelValueConcept
///
/// \code
/// concept ImageConcept<RandomAccess2DImageConcept Image>
/// {
///     where MutableImageViewConcept<view_t>;
///     typename coord_t  = view_t::coord_t;
/// };
/// \endcode
template <typename Image>
struct ImageConcept
{
    void constraints()
    {
        gil_function_requires<RandomAccess2DImageConcept<Image>>();
        gil_function_requires<MutableImageViewConcept<typename Image::view_t>>();
        using coord_t = typename Image::coord_t;
        static_assert(num_channels<Image>::value == mp11::mp_size<typename color_space_type<Image>::type>::value, "");

        static_assert(std::is_same<coord_t, typename Image::x_coord_t>::value, "");
        static_assert(std::is_same<coord_t, typename Image::y_coord_t>::value, "");
    }
    Image image;
};

}} // namespace boost::gil

#if defined(BOOST_CLANG)
#pragma clang diagnostic pop
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic pop
#endif

#endif
