//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_RGBA_HPP
#define BOOST_GIL_RGBA_HPP

#include <boost/gil/planar_pixel_iterator.hpp>
#include <boost/gil/rgb.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <cstddef>
#include <type_traits>

namespace boost { namespace gil {

/// \ingroup ColorNameModel
/// \brief Alpha
struct alpha_t {};

/// \ingroup ColorSpaceModel
using rgba_t =mp11::mp_list<red_t, green_t, blue_t, alpha_t>;

/// \ingroup LayoutModel
using rgba_layout_t = layout<rgba_t>;

/// \ingroup LayoutModel
using bgra_layout_t = layout<rgba_t, mp11::mp_list_c<int, 2, 1, 0, 3>>;

/// \ingroup LayoutModel
using argb_layout_t = layout<rgba_t, mp11::mp_list_c<int, 1, 2, 3, 0>>;

/// \ingroup LayoutModel
using abgr_layout_t = layout<rgba_t, mp11::mp_list_c<int, 3, 2, 1, 0>>;

/// \ingroup ImageViewConstructors
/// \brief from raw RGBA planar data
template <typename ChannelPtr>
inline
auto planar_rgba_view(std::size_t width, std::size_t height,
    ChannelPtr r, ChannelPtr g, ChannelPtr b, ChannelPtr a,
    std::ptrdiff_t rowsize_in_bytes)
    -> typename type_from_x_iterator<planar_pixel_iterator<ChannelPtr, rgba_t> >::view_t
{
    using pixel_iterator_t = planar_pixel_iterator<ChannelPtr, rgba_t>;
    using view_t = typename type_from_x_iterator<pixel_iterator_t>::view_t;
    using locator_t = typename view_t::locator;

    locator_t loc(pixel_iterator_t(r, g, b, a), rowsize_in_bytes);
    return view_t(width, height, loc);
}

}} // namespace boost::gil

#endif
