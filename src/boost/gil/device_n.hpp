//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_DEVICE_N_HPP
#define BOOST_GIL_DEVICE_N_HPP

#include <boost/gil/metafunctions.hpp>
#include <boost/gil/utilities.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <boost/config.hpp>

#include <cstddef>
#include <type_traits>

namespace boost { namespace gil {


// TODO: Document the DeviceN Color Space and Color Model
// with reference to the Adobe documentation
// https://www.adobe.com/content/dam/acom/en/devnet/postscript/pdfs/TN5604.DeviceN_Color.pdf

/// \brief unnamed color
/// \ingroup ColorNameModel
template <int N>
struct devicen_color_t {};

template <int N>
struct devicen_t;

/// \brief Unnamed color space of 1, 3, 4, or 5 channels
/// \tparam N Number of color components (1, 3, 4 or 5).
/// \ingroup ColorSpaceModel
template <int N>
struct devicen_t
{
private:
    template <typename T>
    using color_t = devicen_color_t<T::value>;

    static_assert(
        N == 1 || (3 <= N && N <= 5),
        "invalid number of DeviceN color components");

public:
    using type = mp11::mp_transform<color_t, mp11::mp_iota_c<N>>;
};

/// \brief unnamed color layout of up to five channels
/// \ingroup LayoutModel
template <int N>
struct devicen_layout_t : layout<typename devicen_t<N>::type> {};

/// \ingroup ImageViewConstructors
/// \brief from 2-channel planar data
template <typename IC>
inline typename type_from_x_iterator<planar_pixel_iterator<IC,devicen_t<2>>>::view_t
planar_devicen_view(std::size_t width, std::size_t height, IC c0, IC c1, std::ptrdiff_t rowsize_in_bytes)
{
    using view_t = typename type_from_x_iterator<planar_pixel_iterator<IC,devicen_t<2>>>::view_t;
    return view_t(width, height, typename view_t::locator(typename view_t::x_iterator(c0,c1), rowsize_in_bytes));
}

/// \ingroup ImageViewConstructors
/// \brief from 3-channel planar data
template <typename IC>
inline
auto planar_devicen_view(std::size_t width, std::size_t height, IC c0, IC c1, IC c2, std::ptrdiff_t rowsize_in_bytes)
    -> typename type_from_x_iterator<planar_pixel_iterator<IC,devicen_t<3>>>::view_t
{
    using view_t = typename type_from_x_iterator<planar_pixel_iterator<IC,devicen_t<3>>>::view_t;
    return view_t(width, height, typename view_t::locator(typename view_t::x_iterator(c0,c1,c2), rowsize_in_bytes));
}

/// \ingroup ImageViewConstructors
/// \brief from 4-channel planar data
template <typename IC>
inline
auto planar_devicen_view(std::size_t width, std::size_t height, IC c0, IC c1, IC c2, IC c3, std::ptrdiff_t rowsize_in_bytes)
    -> typename type_from_x_iterator<planar_pixel_iterator<IC,devicen_t<4>>>::view_t
{
    using view_t = typename type_from_x_iterator<planar_pixel_iterator<IC,devicen_t<4>>>::view_t;
    return view_t(width, height, typename view_t::locator(typename view_t::x_iterator(c0,c1,c2,c3), rowsize_in_bytes));
}

/// \ingroup ImageViewConstructors
/// \brief from 5-channel planar data
template <typename IC>
inline
auto planar_devicen_view(std::size_t width, std::size_t height, IC c0, IC c1, IC c2, IC c3, IC c4, std::ptrdiff_t rowsize_in_bytes)
    -> typename type_from_x_iterator<planar_pixel_iterator<IC,devicen_t<5>>>::view_t
{
    using view_t = typename type_from_x_iterator<planar_pixel_iterator<IC,devicen_t<5>>>::view_t;
    return view_t(width, height, typename view_t::locator(typename view_t::x_iterator(c0,c1,c2,c3,c4), rowsize_in_bytes));
}

}}  // namespace boost::gil

#endif
