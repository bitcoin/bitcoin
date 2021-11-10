//
// Copyright 2009 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_JPEG_DETAIL_IS_ALLOWED_HPP
#define BOOST_GIL_EXTENSION_IO_JPEG_DETAIL_IS_ALLOWED_HPP

#include <boost/gil/extension/io/jpeg/tags.hpp>

#include <type_traits>

namespace boost { namespace gil { namespace detail {

template< typename View >
bool is_allowed( const image_read_info< jpeg_tag >& info
               , std::true_type   // is read_and_no_convert
               )
{
    if( info._color_space == JCS_YCbCr )
    {
        // We read JCS_YCbCr files as rgb.
        return ( is_read_supported< typename View::value_type
                                  , jpeg_tag
                                  >::_color_space == JCS_RGB );
    }

    return ( is_read_supported< typename View::value_type
                              , jpeg_tag
                              >::_color_space == info._color_space );
}

template< typename View >
bool is_allowed( const image_read_info< jpeg_tag >& /* info */
               , std::false_type  // is read_and_convert
               )
{
    return true;
}

} // namespace detail
} // namespace gil
} // namespace boost

#endif
