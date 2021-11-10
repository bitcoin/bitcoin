//
// Copyright 2009 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_PNM_DETAIL_IS_ALLOWED_HPP
#define BOOST_GIL_EXTENSION_IO_PNM_DETAIL_IS_ALLOWED_HPP

#include <boost/gil/extension/io/pnm/tags.hpp>

#include <type_traits>

namespace boost { namespace gil { namespace detail {

template< typename View >
bool is_allowed( const image_read_info< pnm_tag >& info
               , std::true_type   // is read_and_no_convert
               )
{
    pnm_image_type::type asc_type = is_read_supported< typename get_pixel_type< View >::type
                                                     , pnm_tag
                                                     >::_asc_type;

    pnm_image_type::type bin_type = is_read_supported< typename get_pixel_type< View >::type
                                                     , pnm_tag
                                                     >::_bin_type;
    if( info._type == pnm_image_type::mono_asc_t::value )
    {
        // ascii mono images are read gray8_image_t
        return (  asc_type == pnm_image_type::gray_asc_t::value );
    }


    return (  asc_type == info._type
           || bin_type == info._type
           );
}

template< typename View >
bool is_allowed( const image_read_info< pnm_tag >& /* info */
               , std::false_type  // is read_and_convert
               )
{
    return true;
}

} // namespace detail
} // namespace gil
} // namespace boost

#endif
