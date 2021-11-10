//
// Copyright 2012 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_TOOLBOX_COLOR_CONVERTERS_GRAY_TO_RGBA_HPP
#define BOOST_GIL_EXTENSION_TOOLBOX_COLOR_CONVERTERS_GRAY_TO_RGBA_HPP

#include <boost/gil/color_convert.hpp>

namespace boost{ namespace gil {

/// This one is missing in gil ( color_convert.hpp ).
template <>
struct default_color_converter_impl<gray_t,rgba_t>
{
    template <typename P1, typename P2>
    void operator()(const P1& src, P2& dst) const
    {
        get_color(dst,red_t())  =
            channel_convert<typename color_element_type<P2, red_t  >::type>(get_color(src,gray_color_t()));
        get_color(dst,green_t())=
            channel_convert<typename color_element_type<P2, green_t>::type>(get_color(src,gray_color_t()));
        get_color(dst,blue_t()) =
            channel_convert<typename color_element_type<P2, blue_t >::type>(get_color(src,gray_color_t()));

        using channel_t = typename channel_type<P2>::type;
        get_color(dst,alpha_t()) = channel_traits< channel_t >::max_value();
    }
};

}} // namespace boost::gil

#endif
