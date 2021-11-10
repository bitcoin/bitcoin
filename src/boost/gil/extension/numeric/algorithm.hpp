//
// Copyright 2005-2007 Adobe Systems Incorporated
// Copyright 2019 Pranam Lashkari <plashkari628@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_NUMERIC_ALGORITHM_HPP
#define BOOST_GIL_EXTENSION_NUMERIC_ALGORITHM_HPP

#include <boost/gil/extension/numeric/pixel_numeric_operations.hpp>

#include <boost/gil/metafunctions.hpp>
#include <boost/gil/pixel_iterator.hpp>
#include <boost/gil/image.hpp>

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace boost { namespace gil {

/// \brief Reference proxy associated with a type that has a \p "reference" member type alias.
///
/// The reference proxy is the reference type, but with stripped-out C++ reference.
/// Models PixelConcept.
template <typename T>
struct pixel_proxy : std::remove_reference<typename T::reference> {};

/// \brief std::for_each for a pair of iterators
template <typename Iterator1, typename Iterator2, typename BinaryFunction>
BinaryFunction for_each(Iterator1 first1, Iterator1 last1, Iterator2 first2, BinaryFunction f)
{
    while (first1 != last1)
        f(*first1++, *first2++);
    return f;
}

template <typename SrcIterator, typename DstIterator>
inline
auto assign_pixels(SrcIterator src, SrcIterator src_end, DstIterator dst) -> DstIterator
{
    for_each(src, src_end, dst,
        pixel_assigns_t
        <
            typename pixel_proxy<typename std::iterator_traits<SrcIterator>::value_type>::type,
            typename pixel_proxy<typename std::iterator_traits<DstIterator>::value_type>::type
        >());
    return dst + (src_end - src);
}

namespace detail {

template <std::size_t Size>
struct inner_product_k_t
{
    template
    <
        class InputIterator1,
        class InputIterator2,
        class T,
        class BinaryOperation1,
        class BinaryOperation2
    >
    static T apply(
        InputIterator1 first1,
        InputIterator2 first2, T init,
        BinaryOperation1 binary_op1,
        BinaryOperation2 binary_op2)
    {
        init = binary_op1(init, binary_op2(*first1, *first2));
        return inner_product_k_t<Size - 1>::template apply(
            first1 + 1, first2 + 1, init, binary_op1, binary_op2);
    }
};

template <>
struct inner_product_k_t<0>
{
    template
    <
        class InputIterator1,
        class InputIterator2,
        class T,
        class BinaryOperation1,
        class BinaryOperation2
    >
    static T apply(
        InputIterator1 first1,
        InputIterator2 first2,
        T init,
        BinaryOperation1 binary_op1,
        BinaryOperation2 binary_op2)
    {
        return init;
    }
};

} // namespace detail

/// static version of std::inner_product
template
<
    std::size_t Size,
    class InputIterator1,
    class InputIterator2,
    class T,
    class BinaryOperation1,
    class BinaryOperation2
>
BOOST_FORCEINLINE
T inner_product_k(
    InputIterator1 first1,
    InputIterator2 first2,
    T init,
    BinaryOperation1 binary_op1,
    BinaryOperation2 binary_op2)
{
    return detail::inner_product_k_t<Size>::template apply(
        first1, first2, init, binary_op1, binary_op2);
}

/// \brief 1D un-guarded cross-correlation with a variable-size kernel
template
<
    typename PixelAccum,
    typename SrcIterator,
    typename KernelIterator,
    typename Size,
    typename DstIterator
>
inline
auto correlate_pixels_n(
    SrcIterator src_begin,
    SrcIterator src_end,
    KernelIterator kernel_begin,
    Size kernel_size,
    DstIterator dst_begin)
    -> DstIterator
{
    using src_pixel_ref_t = typename pixel_proxy
        <
            typename std::iterator_traits<SrcIterator>::value_type
        >::type;
    using dst_pixel_ref_t = typename pixel_proxy
        <
            typename std::iterator_traits<DstIterator>::value_type
        >::type;
    using kernel_value_t = typename std::iterator_traits<KernelIterator>::value_type;

    PixelAccum accum_zero;
    pixel_zeros_t<PixelAccum>()(accum_zero);
    while (src_begin != src_end)
    {
        pixel_assigns_t<PixelAccum, dst_pixel_ref_t>()(
            std::inner_product(
                src_begin,
                src_begin + kernel_size,
                kernel_begin,
                accum_zero,
                pixel_plus_t<PixelAccum, PixelAccum, PixelAccum>(),
                pixel_multiplies_scalar_t<src_pixel_ref_t, kernel_value_t, PixelAccum>()),
            *dst_begin);

        ++src_begin;
        ++dst_begin;
    }
    return dst_begin;
}

/// \brief 1D un-guarded cross-correlation with a fixed-size kernel
template
<
    std::size_t Size,
    typename PixelAccum,
    typename SrcIterator,
    typename KernelIterator,
    typename DstIterator
>
inline
auto correlate_pixels_k(
    SrcIterator src_begin,
    SrcIterator src_end,
    KernelIterator kernel_begin,
    DstIterator dst_begin)
    -> DstIterator
{
    using src_pixel_ref_t = typename pixel_proxy
        <
            typename std::iterator_traits<SrcIterator>::value_type
        >::type;
    using dst_pixel_ref_t = typename pixel_proxy
        <
            typename std::iterator_traits<DstIterator>::value_type
        >::type;
    using kernel_type = typename std::iterator_traits<KernelIterator>::value_type;

    PixelAccum accum_zero;
    pixel_zeros_t<PixelAccum>()(accum_zero);
    while (src_begin != src_end)
    {
        pixel_assigns_t<PixelAccum, dst_pixel_ref_t>()(
            inner_product_k<Size>(
                src_begin,
                kernel_begin,
                accum_zero,
                pixel_plus_t<PixelAccum, PixelAccum, PixelAccum>(),
                pixel_multiplies_scalar_t<src_pixel_ref_t, kernel_type, PixelAccum>()),
            *dst_begin);

        ++src_begin;
        ++dst_begin;
    }
    return dst_begin;
}

/// \brief destination is set to be product of the source and a scalar
/// \tparam PixelAccum - TODO
/// \tparam SrcView Models ImageViewConcept
/// \tparam DstView Models MutableImageViewConcept
template <typename PixelAccum, typename SrcView, typename Scalar, typename DstView>
inline
void view_multiplies_scalar(SrcView const& src_view, Scalar const& scalar, DstView const& dst_view)
{
    static_assert(std::is_scalar<Scalar>::value, "Scalar is not scalar");
    BOOST_ASSERT(src_view.dimensions() == dst_view.dimensions());
    using src_pixel_ref_t = typename pixel_proxy<typename SrcView::value_type>::type;
    using dst_pixel_ref_t = typename pixel_proxy<typename DstView::value_type>::type;
    using y_coord_t = typename SrcView::y_coord_t;

    y_coord_t const height = src_view.height();
    for (y_coord_t y = 0; y < height; ++y)
    {
        typename SrcView::x_iterator it_src = src_view.row_begin(y);
        typename DstView::x_iterator it_dst = dst_view.row_begin(y);
        typename SrcView::x_iterator it_src_end = src_view.row_end(y);
        while (it_src != it_src_end)
        {
            pixel_assigns_t<PixelAccum, dst_pixel_ref_t>()(
                pixel_multiplies_scalar_t<src_pixel_ref_t, Scalar, PixelAccum>()(*it_src, scalar),
                *it_dst);

            ++it_src;
            ++it_dst;
        }
    }
}


/// \ingroup ImageAlgorithms
/// \brief Boundary options for image boundary extension
enum class boundary_option
{
    output_ignore,  /// do nothing to the output
    output_zero,    /// set the output to zero
    extend_padded,  /// assume the source boundaries to be padded already
    extend_zero,    /// assume the source boundaries to be zero
    extend_constant /// assume the source boundaries to be the boundary value
};

namespace detail
{

template <typename SrcView, typename RltView>
void extend_row_impl(
    SrcView const& src_view,
    RltView result_view,
    std::size_t extend_count,
    boundary_option option)
{
    std::ptrdiff_t extend_count_ = static_cast<std::ptrdiff_t>(extend_count);

    if (option == boundary_option::extend_constant)
    {
        for (std::ptrdiff_t i = 0; i < result_view.height(); i++)
        {
            if(i >= extend_count_ && i < extend_count_ + src_view.height())
            {
                assign_pixels(
                    src_view.row_begin(i - extend_count_),
                    src_view.row_end(i - extend_count_),
                    result_view.row_begin(i)
                );
            }
            else if(i < extend_count_)
            {
                assign_pixels(src_view.row_begin(0), src_view.row_end(0), result_view.row_begin(i));
            }
            else
            {
                assign_pixels(
                    src_view.row_begin(src_view.height() - 1),
                    src_view.row_end(src_view.height() - 1),
                    result_view.row_begin(i)
                );
            }

        }
    }
    else if (option == boundary_option::extend_zero)
    {
        typename SrcView::value_type acc_zero;
        pixel_zeros_t<typename SrcView::value_type>()(acc_zero);

        for (std::ptrdiff_t i = 0; i < result_view.height(); i++)
        {
            if (i >= extend_count_ && i < extend_count_ + src_view.height())
            {
                assign_pixels(
                    src_view.row_begin(i - extend_count_),
                    src_view.row_end(i - extend_count_),
                    result_view.row_begin(i)
                );
            }
            else
            {
                std::fill_n(result_view.row_begin(i), result_view.width(), acc_zero);
            }
        }
    }
    else if (option == boundary_option::extend_padded)
    {
        auto original_view = subimage_view(
            src_view,
            0,
            -extend_count,
            src_view.width(),
            src_view.height() + (2 * extend_count)
        );
        for (std::ptrdiff_t i = 0; i < result_view.height(); i++)
        {
            assign_pixels(
                original_view.row_begin(i),
                original_view.row_end(i),
                result_view.row_begin(i)
            );
        }
    }
    else
    {
        BOOST_ASSERT_MSG(false, "Invalid boundary option");
    }
}

} //namespace detail


/// \brief adds new row at top and bottom.
/// Image padding introduces new pixels around the edges of an image.
/// The border provides space for annotations or acts as a boundary when using advanced filtering techniques.
/// \tparam SrcView Models ImageViewConcept
/// \tparam extend_count number of rows to be added each side
/// \tparam option - TODO
template <typename SrcView>
auto extend_row(
    SrcView const& src_view,
    std::size_t extend_count,
    boundary_option option
) -> typename gil::image<typename SrcView::value_type>
{
    typename gil::image<typename SrcView::value_type>
        result_img(src_view.width(), src_view.height() + (2 * extend_count));

    auto result_view = view(result_img);
    detail::extend_row_impl(src_view, result_view, extend_count, option);
    return result_img;
}


/// \brief adds new column at left and right.
/// Image padding introduces new pixels around the edges of an image.
/// The border provides space for annotations or acts as a boundary when using advanced filtering techniques.
/// \tparam SrcView Models ImageViewConcept
/// \tparam extend_count number of columns to be added each side
/// \tparam option - TODO
template <typename SrcView>
auto extend_col(
    SrcView const& src_view,
    std::size_t extend_count,
    boundary_option option
) -> typename gil::image<typename SrcView::value_type>
{
    auto src_view_rotate = rotated90cw_view(src_view);

    typename gil::image<typename SrcView::value_type>
        result_img(src_view.width() + (2 * extend_count), src_view.height());

    auto result_view = rotated90cw_view(view(result_img));
    detail::extend_row_impl(src_view_rotate, result_view, extend_count, option);
    return result_img;
}

/// \brief adds new row and column at all sides.
/// Image padding introduces new pixels around the edges of an image.
/// The border provides space for annotations or acts as a boundary when using advanced filtering techniques.
/// \tparam SrcView Models ImageViewConcept
/// \tparam extend_count number of rows/column to be added each side
/// \tparam option - TODO
template <typename SrcView>
auto extend_boundary(
    SrcView const& src_view,
    std::size_t extend_count,
    boundary_option option
) -> typename gil::image<typename SrcView::value_type>
{
    if (option == boundary_option::extend_padded)
    {
        typename gil::image<typename SrcView::value_type>
            result_img(src_view.width()+(2 * extend_count), src_view.height()+(2 * extend_count));
        typename gil::image<typename SrcView::value_type>::view_t result_view = view(result_img);

        auto original_view = subimage_view(
            src_view,
            -extend_count,
            -extend_count,
            src_view.width() + (2 * extend_count),
            src_view.height() + (2 * extend_count)
        );

        for (std::ptrdiff_t i = 0; i < result_view.height(); i++)
        {
            assign_pixels(
                original_view.row_begin(i),
                original_view.row_end(i),
                result_view.row_begin(i)
            );
        }

        return result_img;
    }

    auto auxilary_img = extend_col(src_view, extend_count, option);
    return extend_row(view(auxilary_img), extend_count, option);
}

}} // namespace boost::gil

#endif
