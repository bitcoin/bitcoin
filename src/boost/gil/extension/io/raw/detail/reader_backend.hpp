//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_RAW_DETAIL_READER_BACKEND_HPP
#define BOOST_GIL_EXTENSION_IO_RAW_DETAIL_READER_BACKEND_HPP

#include <boost/gil/extension/io/raw/tags.hpp>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///
/// RAW Backend
///
template< typename Device >
struct reader_backend< Device
                     , raw_tag
                     >
{
public:

    using format_tag_t = raw_tag;

public:

    reader_backend( const Device&                         io_dev
                  , const image_read_settings< raw_tag >& settings
                  )
    : _io_dev  ( io_dev   )
    , _settings( settings )
    , _info()
    , _scanline_length( 0 )
    {
        read_header();

        if( _settings._dim.x == 0 )
        {
            _settings._dim.x = _info._width;
        }

        if( _settings._dim.y == 0 )
        {
            _settings._dim.y = _info._height;
        }
    }

    void read_header()
    {
        _io_dev.get_mem_image_format( &_info._width
                                    , &_info._height
                                    , &_info._samples_per_pixel
                                    , &_info._bits_per_pixel
                                    );

        // iparams
        _info._camera_manufacturer = _io_dev.get_camera_manufacturer();
        _info._camera_model        = _io_dev.get_camera_model();
        _info._raw_images_count    = _io_dev.get_raw_count();
        _info._dng_version         = _io_dev.get_dng_version();
        _info._number_colors       = _io_dev.get_colors();
        //_io_dev.get_filters();
        _info._colors_description  = _io_dev.get_cdesc();

        // image_sizes
        _info._raw_width      = _io_dev.get_raw_width();
        _info._raw_height     = _io_dev.get_raw_height();
        _info._visible_width  = _io_dev.get_image_width();
        _info._visible_height = _io_dev.get_image_height();
        _info._top_margin     = _io_dev.get_top_margin();
        _info._left_margin    = _io_dev.get_left_margin();
        _info._output_width   = _io_dev.get_iwidth();
        _info._output_height  = _io_dev.get_iheight();
        _info._pixel_aspect   = _io_dev.get_pixel_aspect();
        _info._flip           = _io_dev.get_flip();

        // imgother
        _info._iso_speed         = _io_dev.get_iso_speed();
        _info._shutter           = _io_dev.get_shutter();
        _info._aperture          = _io_dev.get_aperture();
        _info._focal_length      = _io_dev.get_focal_len();
        _info._timestamp         = _io_dev.get_timestamp();
        _info._shot_order        = static_cast< uint16_t >( _io_dev.get_shot_order() );
        //_io_dev.get_gpsdata();
        _info._image_description = _io_dev.get_desc();
        _info._artist            = _io_dev.get_artist();

        _info._libraw_version = _io_dev.get_version();

        _info._valid = true;
    }

    /// Check if image is large enough.
    void check_image_size( const point_t& img_dim )
    {
        if( _settings._dim.x > 0 )
        {
            if( img_dim.x < _settings._dim.x ) { io_error( "Supplied image is too small" ); }
        }
        else
        {
            if( img_dim.x < _info._width ) { io_error( "Supplied image is too small" ); }
        }


        if( _settings._dim.y > 0 )
        {
            if( img_dim.y < _settings._dim.y ) { io_error( "Supplied image is too small" ); }
        }
        else
        {
            if( img_dim.y < _info._height ) { io_error( "Supplied image is too small" ); }
        }
    }

public:

    Device _io_dev;

    image_read_settings< raw_tag > _settings;
    image_read_info< raw_tag >     _info;

    std::size_t _scanline_length;
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
