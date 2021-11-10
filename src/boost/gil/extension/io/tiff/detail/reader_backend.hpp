//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_READER_BACKEND_HPP
#define BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_READER_BACKEND_HPP

#include <boost/gil/extension/io/tiff/tags.hpp>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///
/// TIFF Backend
///
template< typename Device >
struct reader_backend< Device
                     , tiff_tag
                     >
{
public:

    using format_tag_t = tiff_tag;

public:

    reader_backend( const Device&                          io_dev
                  , const image_read_settings< tiff_tag >& settings
                  )
    : _io_dev  ( io_dev   )
    , _settings( settings )
    , _info()

    , _scanline_length( 0 )

    , _red  ( nullptr )
    , _green( nullptr )
    , _blue ( nullptr )
    {
        init_multipage_read( settings );

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
        io_error_if( _io_dev.template get_property<tiff_image_width>               ( _info._width ) == false
                    , "cannot read tiff tag." );
        io_error_if( _io_dev.template get_property<tiff_image_height>              ( _info._height ) == false
                    , "cannot read tiff tag." );
        io_error_if( _io_dev.template get_property<tiff_compression>               ( _info._compression ) == false
                    , "cannot read tiff tag." );
        io_error_if( _io_dev.template get_property<tiff_samples_per_pixel>         ( _info._samples_per_pixel ) == false
                    , "cannot read tiff tag." );
        io_error_if( _io_dev.template get_property<tiff_bits_per_sample>           ( _info._bits_per_sample ) == false
                    , "cannot read tiff tag." );
        io_error_if( _io_dev.template get_property<tiff_sample_format>             ( _info._sample_format ) == false
                    , "cannot read tiff tag." );
        io_error_if( _io_dev.template get_property<tiff_planar_configuration>      ( _info._planar_configuration ) == false
                    , "cannot read tiff tag." );
        io_error_if( _io_dev.template get_property<tiff_photometric_interpretation>( _info._photometric_interpretation  ) == false
                    , "cannot read tiff tag." );

        _info._is_tiled = false;

        // Tile tags
        if( _io_dev.is_tiled() )
        {
            _info._is_tiled = true;

            io_error_if( !_io_dev.template get_property< tiff_tile_width  >( _info._tile_width )
                        , "cannot read tiff_tile_width tag." );
            io_error_if( !_io_dev.template get_property< tiff_tile_length >( _info._tile_length )
                        , "cannot read tiff_tile_length tag." );
        }

        io_error_if( _io_dev.template get_property<tiff_resolution_unit>( _info._resolution_unit) == false
          , "cannot read tiff tag");
        io_error_if( _io_dev. template get_property<tiff_x_resolution>( _info._x_resolution ) == false
          , "cannot read tiff tag" );
        io_error_if( _io_dev. template get_property<tiff_y_resolution>( _info._y_resolution ) == false
          , "cannot read tiff tag" );

        /// optional and non-baseline properties below here
        _io_dev. template get_property <tiff_icc_profile> ( _info._icc_profile );
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
            if( (tiff_image_width::type) img_dim.x < _info._width ) { io_error( "Supplied image is too small" ); }
        }


        if( _settings._dim.y > 0 )
        {
            if( img_dim.y < _settings._dim.y ) { io_error( "Supplied image is too small" ); }
        }
        else
        {
            if( (tiff_image_height::type) img_dim.y < _info._height ) { io_error( "Supplied image is too small" ); }
        }
    }

private:

    void init_multipage_read( const image_read_settings< tiff_tag >& settings )
    {
        if( settings._directory > 0 )
        {
            _io_dev.set_directory( settings._directory );
        }
    }

public:

    Device _io_dev;

    image_read_settings< tiff_tag > _settings;
    image_read_info< tiff_tag >     _info;

    std::size_t _scanline_length;

    // palette
    tiff_color_map::red_t   _red;
    tiff_color_map::green_t _green;
    tiff_color_map::blue_t  _blue;

    rgb16_planar_view_t _palette;
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
