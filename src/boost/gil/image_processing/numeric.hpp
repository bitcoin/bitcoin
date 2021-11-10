//
// Copyright 2019 Olzhas Zhumabek <anonymous.from.applecity@gmail.com>
//
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_GIL_IMAGE_PROCESSING_NUMERIC_HPP
#define BOOST_GIL_IMAGE_PROCESSING_NUMERIC_HPP

#include <boost/gil/extension/numeric/kernel.hpp>
#include <boost/gil/extension/numeric/convolve.hpp>
#include <boost/gil/image_view.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/detail/math.hpp>
// fixes ambigious call to std::abs, https://stackoverflow.com/a/30084734/4593721
#include <cstdlib>
#include <cmath>

namespace boost { namespace gil {

/// \defgroup ImageProcessingMath
/// \brief Math operations for IP algorithms
///
/// This is mostly handful of mathemtical operations that are required by other
/// image processing algorithms
///
/// \brief Normalized cardinal sine
/// \ingroup ImageProcessingMath
///
/// normalized_sinc(x) = sin(pi * x) / (pi * x)
///
inline double normalized_sinc(double x)
{
    return std::sin(x * boost::gil::detail::pi) / (x * boost::gil::detail::pi);
}

/// \brief Lanczos response at point x
/// \ingroup ImageProcessingMath
///
/// Lanczos response is defined as:
/// x == 0: 1
/// -a < x && x < a: 0
/// otherwise: normalized_sinc(x) / normalized_sinc(x / a)
inline double lanczos(double x, std::ptrdiff_t a)
{
    // means == but <= avoids compiler warning
    if (0 <= x && x <= 0)
        return 1;

    if (-a < x && x < a)
        return normalized_sinc(x) / normalized_sinc(x / static_cast<double>(a));

    return 0;
}

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4244) // 'argument': conversion from 'const Channel' to 'BaseChannelValue', possible loss of data
#endif

inline void compute_tensor_entries(
    boost::gil::gray16s_view_t dx,
    boost::gil::gray16s_view_t dy,
    boost::gil::gray32f_view_t m11,
    boost::gil::gray32f_view_t m12_21,
    boost::gil::gray32f_view_t m22)
{
    for (std::ptrdiff_t y = 0; y < dx.height(); ++y) {
        for (std::ptrdiff_t x = 0; x < dx.width(); ++x) {
            auto dx_value = dx(x, y);
            auto dy_value = dy(x, y);
            m11(x, y) = dx_value * dx_value;
            m12_21(x, y) = dx_value * dy_value;
            m22(x, y) = dy_value * dy_value;
        }
    }
}

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

/// \brief Generate mean kernel
/// \ingroup ImageProcessingMath
///
/// Fills supplied view with normalized mean
/// in which all entries will be equal to
/// \code 1 / (dst.size()) \endcode
template <typename T = float, typename Allocator = std::allocator<T>>
inline detail::kernel_2d<T, Allocator> generate_normalized_mean(std::size_t side_length)
{
    if (side_length % 2 != 1)
        throw std::invalid_argument("kernel dimensions should be odd and equal");
    const float entry = 1.0f / static_cast<float>(side_length * side_length);

    detail::kernel_2d<T, Allocator> result(side_length, side_length / 2, side_length / 2);
    for (auto& cell: result) {
        cell = entry;
    }

    return result;
}

/// \brief Generate kernel with all 1s
/// \ingroup ImageProcessingMath
///
/// Fills supplied view with 1s (ones)
template <typename T = float, typename Allocator = std::allocator<T>>
inline detail::kernel_2d<T, Allocator> generate_unnormalized_mean(std::size_t side_length)
{
    if (side_length % 2 != 1)
        throw std::invalid_argument("kernel dimensions should be odd and equal");

    detail::kernel_2d<T, Allocator> result(side_length, side_length / 2, side_length / 2);
    for (auto& cell: result) {
        cell = 1.0f;
    }

    return result;
}

/// \brief Generate Gaussian kernel
/// \ingroup ImageProcessingMath
///
/// Fills supplied view with values taken from Gaussian distribution. See
/// https://en.wikipedia.org/wiki/Gaussian_blur
template <typename T = float, typename Allocator = std::allocator<T>>
inline detail::kernel_2d<T, Allocator> generate_gaussian_kernel(std::size_t side_length, double sigma)
{
    if (side_length % 2 != 1)
        throw std::invalid_argument("kernel dimensions should be odd and equal");


    const double denominator = 2 * boost::gil::detail::pi * sigma * sigma;
    auto middle = side_length / 2;
    std::vector<T, Allocator> values(side_length * side_length);
    for (std::size_t y = 0; y < side_length; ++y)
    {
        for (std::size_t x = 0; x < side_length; ++x)
        {
            const auto delta_x = middle > x ? middle - x : x - middle;
            const auto delta_y = middle > y ? middle - y : y - middle;
            const double power = (delta_x * delta_x +  delta_y * delta_y) / (2 * sigma * sigma);
            const double nominator = std::exp(-power);
            const float value = static_cast<float>(nominator / denominator);
            values[y * side_length + x] = value;
        }
    }

    return detail::kernel_2d<T, Allocator>(values.begin(), values.size(), middle, middle);
}

/// \brief Generates Sobel operator in horizontal direction
/// \ingroup ImageProcessingMath
///
/// Generates a kernel which will represent Sobel operator in
/// horizontal direction of specified degree (no need to convolve multiple times
/// to obtain the desired degree).
/// https://www.researchgate.net/publication/239398674_An_Isotropic_3_3_Image_Gradient_Operator
template <typename T = float, typename Allocator = std::allocator<T>>
inline detail::kernel_2d<T, Allocator> generate_dx_sobel(unsigned int degree = 1)
{
    switch (degree)
    {
        case 0:
        {
            return detail::get_identity_kernel<T, Allocator>();
        }
        case 1:
        {
            detail::kernel_2d<T, Allocator> result(3, 1, 1);
            std::copy(detail::dx_sobel.begin(), detail::dx_sobel.end(), result.begin());
            return result;
        }
        default:
            throw std::logic_error("not supported yet");
    }

    //to not upset compiler
    throw std::runtime_error("unreachable statement");
}

/// \brief Generate Scharr operator in horizontal direction
/// \ingroup ImageProcessingMath
///
/// Generates a kernel which will represent Scharr operator in
/// horizontal direction of specified degree (no need to convolve multiple times
/// to obtain the desired degree).
/// https://www.researchgate.net/profile/Hanno_Scharr/publication/220955743_Optimal_Filters_for_Extended_Optical_Flow/links/004635151972eda98f000000/Optimal-Filters-for-Extended-Optical-Flow.pdf
template <typename T = float, typename Allocator = std::allocator<T>>
inline detail::kernel_2d<T, Allocator> generate_dx_scharr(unsigned int degree = 1)
{
    switch (degree)
    {
        case 0:
        {
            return detail::get_identity_kernel<T, Allocator>();
        }
        case 1:
        {
            detail::kernel_2d<T, Allocator> result(3, 1, 1);
            std::copy(detail::dx_scharr.begin(), detail::dx_scharr.end(), result.begin());
            return result;
        }
        default:
            throw std::logic_error("not supported yet");
    }

    //to not upset compiler
    throw std::runtime_error("unreachable statement");
}

/// \brief Generates Sobel operator in vertical direction
/// \ingroup ImageProcessingMath
///
/// Generates a kernel which will represent Sobel operator in
/// vertical direction of specified degree (no need to convolve multiple times
/// to obtain the desired degree).
/// https://www.researchgate.net/publication/239398674_An_Isotropic_3_3_Image_Gradient_Operator
template <typename T = float, typename Allocator = std::allocator<T>>
inline detail::kernel_2d<T, Allocator> generate_dy_sobel(unsigned int degree = 1)
{
    switch (degree)
    {
        case 0:
        {
            return detail::get_identity_kernel<T, Allocator>();
        }
        case 1:
        {
            detail::kernel_2d<T, Allocator> result(3, 1, 1);
            std::copy(detail::dy_sobel.begin(), detail::dy_sobel.end(), result.begin());
            return result;
        }
        default:
            throw std::logic_error("not supported yet");
    }

    //to not upset compiler
    throw std::runtime_error("unreachable statement");
}

/// \brief Generate Scharr operator in vertical direction
/// \ingroup ImageProcessingMath
///
/// Generates a kernel which will represent Scharr operator in
/// vertical direction of specified degree (no need to convolve multiple times
/// to obtain the desired degree).
/// https://www.researchgate.net/profile/Hanno_Scharr/publication/220955743_Optimal_Filters_for_Extended_Optical_Flow/links/004635151972eda98f000000/Optimal-Filters-for-Extended-Optical-Flow.pdf
template <typename T = float, typename Allocator = std::allocator<T>>
inline detail::kernel_2d<T, Allocator> generate_dy_scharr(unsigned int degree = 1)
{
    switch (degree)
    {
        case 0:
        {
            return detail::get_identity_kernel<T, Allocator>();
        }
        case 1:
        {
            detail::kernel_2d<T, Allocator> result(3, 1, 1);
            std::copy(detail::dy_scharr.begin(), detail::dy_scharr.end(), result.begin());
            return result;
        }
        default:
            throw std::logic_error("not supported yet");
    }

    //to not upset compiler
    throw std::runtime_error("unreachable statement");
}

/// \brief Compute xy gradient, and second order x and y gradients
/// \ingroup ImageProcessingMath
///
/// Hessian matrix is defined as a matrix of partial derivates
/// for 2d case, it is [[ddxx, dxdy], [dxdy, ddyy].
/// d stands for derivative, and x or y stand for direction.
/// For example, dx stands for derivative (gradient) in horizontal
/// direction, and ddxx means second order derivative in horizon direction
/// https://en.wikipedia.org/wiki/Hessian_matrix
template <typename GradientView, typename OutputView>
inline void compute_hessian_entries(
    GradientView dx,
    GradientView dy,
    OutputView ddxx,
    OutputView dxdy,
    OutputView ddyy)
{
    auto sobel_x = generate_dx_sobel();
    auto sobel_y = generate_dy_sobel();
    detail::convolve_2d(dx, sobel_x, ddxx);
    detail::convolve_2d(dx, sobel_y, dxdy);
    detail::convolve_2d(dy, sobel_y, ddyy);
}

}} // namespace boost::gil

#endif
