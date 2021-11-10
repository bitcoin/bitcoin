//
// Copyright 2010 Kenneth Riddile
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TARGA_DETAIL_IS_ALLOWED_HPP
#define BOOST_GIL_EXTENSION_IO_TARGA_DETAIL_IS_ALLOWED_HPP

#include <boost/gil/extension/io/targa/tags.hpp>

#include <type_traits>

namespace boost { namespace gil { namespace detail {

template< typename View >
bool is_allowed( const image_read_info< targa_tag >& info
               , std::true_type   // is read_and_no_convert
               )
{
    targa_depth::type src_bits_per_pixel = 0;

    switch( info._bits_per_pixel )
    {
        case 24:
        case 32:
        {
            src_bits_per_pixel = info._bits_per_pixel;
            break;
        }
        default:
        {
            io_error( "Pixel size not supported." );
            break;
        }
    }

    using channel_t = typename channel_traits<typename element_type<typename View::value_type>::type>::value_type;
    targa_depth::type dst_bits_per_pixel = detail::unsigned_integral_num_bits< channel_t >::value * num_channels< View >::value;

    return ( dst_bits_per_pixel == src_bits_per_pixel );
}

template< typename View >
bool is_allowed( const image_read_info< targa_tag >& /* info */
               , std::false_type  // is read_and_convert
               )
{
    return true;
}

} // namespace detail
} // namespace gil
} // namespace boost

#endif
