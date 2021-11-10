//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_NUMERIC_PIXEL_NUMERIC_OPERATIONS_HPP
#define BOOST_GIL_EXTENSION_NUMERIC_PIXEL_NUMERIC_OPERATIONS_HPP

#include <boost/gil/extension/numeric/channel_numeric_operations.hpp>

#include <boost/gil/color_base_algorithm.hpp>
#include <boost/gil/pixel.hpp>

namespace boost { namespace gil {

// Function objects and utilities for pixel-wise numeric operations.
//
// List of currently defined functors:
//   pixel_plus_t (+)
//   pixel_minus_t (-)
//   pixel_multiplies_scalar_t (*)
//   pixel_divides_scalar_t (/)
//   pixel_halves_t (/=2),
//   pixel_zeros_t (=0)
//   pixel_assigns_t (=)

/// \ingroup PixelNumericOperations
/// \brief Performs channel-wise addition of two pixels.
/// \tparam PixelRef1 - models PixelConcept
/// \tparam PixelRef2 - models PixelConcept
/// \tparam PixelResult - models PixelValueConcept
template <typename PixelRef1, typename PixelRef2, typename PixelResult>
struct pixel_plus_t
{
    auto operator()(PixelRef1 const& p1, PixelRef2 const& p2) const -> PixelResult
    {
        PixelResult result;
        static_transform(p1, p2, result,
            channel_plus_t
            <
                typename channel_type<PixelRef1>::type,
                typename channel_type<PixelRef2>::type,
                typename channel_type<PixelResult>::type
            >());
        return result;
    }
};

/// \ingroup PixelNumericOperations
/// \brief Performs channel-wise subtraction of two pixels.
/// \tparam PixelRef1 - models PixelConcept
/// \tparam PixelRef2 - models PixelConcept
/// \tparam PixelResult - models PixelValueConcept
template <typename PixelRef1, typename PixelRef2, typename PixelResult>
struct pixel_minus_t
{
    auto operator()(PixelRef1 const& p1, PixelRef2 const& p2) const -> PixelResult
    {
        PixelResult result;
        static_transform(p1, p2, result,
            channel_minus_t
            <
                typename channel_type<PixelRef1>::type,
                typename channel_type<PixelRef2>::type,
                typename channel_type<PixelResult>::type
            >());
        return result;
    }
};

/// \ingroup PixelNumericOperations
/// \brief Performs channel-wise multiplication of pixel elements by scalar.
/// \tparam PixelRef - models PixelConcept
/// \tparam Scalar - models a scalar type
/// \tparam PixelResult - models PixelValueConcept
template <typename PixelRef, typename Scalar, typename PixelResult>
struct pixel_multiplies_scalar_t
{
    auto operator()(PixelRef const& p, Scalar const& s) const -> PixelResult
    {
        PixelResult result;
        static_transform(p, result,
            std::bind(
                channel_multiplies_scalar_t<typename channel_type<PixelRef>::type,
                Scalar,
                typename channel_type<PixelResult>::type>(),
                std::placeholders::_1, s));
        return result;
    }
};

/// \ingroup PixelNumericOperations
/// \brief Performs channel-wise multiplication of two pixels.
/// \tparam PixelRef1 - models PixelConcept
/// \tparam PixelRef1 - models PixelConcept
/// \tparam PixelResult - models PixelValueConcept
template <typename PixelRef1, typename PixelRef2, typename PixelResult>
struct pixel_multiply_t
{
    auto operator()(PixelRef1 const& p1, PixelRef2 const& p2) const -> PixelResult
    {
        PixelResult result;
        static_transform(p1, p2, result,
            channel_multiplies_t
            <
                typename channel_type<PixelRef1>::type,
                typename channel_type<PixelRef2>::type,
                typename channel_type<PixelResult>::type
            >());
        return result;
    }
};

/// \ingroup PixelNumericOperations
/// \brief Performs channel-wise division of pixel elements by scalar.
/// \tparam PixelRef - models PixelConcept
/// \tparam Scalar - models a scalar type
/// \tparam PixelResult - models PixelValueConcept
template <typename PixelRef, typename Scalar, typename PixelResult>
struct pixel_divides_scalar_t
{
    auto operator()(PixelRef const& p, Scalar const& s) const -> PixelResult
    {
        PixelResult result;
        static_transform(p, result,
            std::bind(channel_divides_scalar_t<typename channel_type<PixelRef>::type,
                Scalar,
                typename channel_type<PixelResult>::type>(),
                std::placeholders::_1, s));
        return result;
    }
};

/// \ingroup PixelNumericOperations
/// \brief Performs channel-wise division of two pixels.
/// \tparam PixelRef1 - models PixelConcept
/// \tparam PixelRef1 - models PixelConcept
/// \tparam PixelResult - models PixelValueConcept
template <typename PixelRef1, typename PixelRef2, typename PixelResult>
struct pixel_divide_t
{
    auto operator()(PixelRef1 const& p1, PixelRef2 const& p2) const -> PixelResult
    {
        PixelResult result;
        static_transform(p1, p2, result,
            channel_divides_t
            <
                typename channel_type<PixelRef1>::type,
                typename channel_type<PixelRef2>::type,
                typename channel_type<PixelResult>::type
            >());
        return result;
    }
};

/// \ingroup PixelNumericOperations
/// \brief Performs channel-wise division by 2
/// \tparam PixelRef - models PixelConcept
template <typename PixelRef>
struct pixel_halves_t
{
    auto operator()(PixelRef& p) const -> PixelRef&
    {
        static_for_each(p, channel_halves_t<typename channel_type<PixelRef>::type>());
        return p;
    }
};

/// \ingroup PixelNumericOperations
/// \brief Sets pixel elements to zero (for whatever zero means)
/// \tparam PixelRef - models PixelConcept
template <typename PixelRef>
struct pixel_zeros_t
{
    auto operator()(PixelRef& p) const -> PixelRef&
    {
        static_for_each(p, channel_zeros_t<typename channel_type<PixelRef>::type>());
        return p;
    }
};

/// \brief Sets pixel elements to zero (for whatever zero means)
/// \tparam Pixel - models PixelConcept
template <typename Pixel>
void zero_channels(Pixel& p)
{
    static_for_each(p, channel_zeros_t<typename channel_type<Pixel>::type>());
}

/// \ingroup PixelNumericOperations
/// \brief Casts and assigns a pixel to another
///
/// A generic implementation for casting and assigning a pixel to another.
/// User should specialize it for better performance.
///
/// \tparam PixelRef - models PixelConcept
/// \tparam PixelResult - models PixelValueConcept
template <typename PixelRef, typename PixelResult>
struct pixel_assigns_t
{
    auto operator()(PixelRef const& src, PixelResult& dst) const -> PixelResult
    {
        static_for_each(src, dst,
            channel_assigns_t
            <
                typename channel_type<PixelRef>::type,
                typename channel_type<PixelResult>::type
            >());
        return dst;
    }
};

}} // namespace boost::gil

#endif
