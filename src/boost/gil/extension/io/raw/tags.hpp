//
// Copyright 2013 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_RAW_TAGS_HPP
#define BOOST_GIL_EXTENSION_IO_RAW_TAGS_HPP

#include <boost/gil/io/base.hpp>

// Historically, LibRaw expects WIN32, not _WIN32 (see https://github.com/LibRaw/LibRaw/pull/206)
#ifdef _MSC_VER
#ifndef WIN32
#define WIN32
#endif
#pragma warning(push)
#pragma warning(disable:4251) // 'type' needs to have dll-interface to be used by clients of class
#endif

#include <libraw/libraw.h>

namespace boost { namespace gil {

/// Defines tiff tag.
struct raw_tag : format_tag {};

/// Defines type for image width property.
struct raw_image_width : property_base< int32_t > {};

/// Defines type for image height property.
struct raw_image_height : property_base< int32_t > {};

/// Defines type for samples per pixel property.
struct raw_samples_per_pixel : property_base< int32_t > {};

/// Defines type for bits per pixel property.
struct raw_bits_per_pixel : property_base< int32_t > {};

/// Defines type for camera manufacturer.
struct raw_camera_manufacturer : property_base< std::string > {};

/// Defines type for camera model.
struct raw_camera_model : property_base< std::string > {};

/// Defines type for raw images count.
struct raw_raw_images_count : property_base< unsigned > {};

/// Defines type for dng version.
struct raw_dng_version : property_base< unsigned > {};

/// Defines type for number of colors.
struct raw_number_colors : property_base< int32_t > {};

/// Defines type for colors description.
struct raw_colors_description : property_base< std::string > {};

/// Defines type for raw height.
struct raw_raw_height : property_base< uint16_t > {};

/// Defines type for raw width.
struct raw_raw_width : property_base< uint16_t > {};

/// Defines type for visible height.
struct raw_visible_height : property_base< uint16_t > {};

/// Defines type for visible width.
struct raw_visible_width : property_base< uint16_t > {};

/// Defines type for top margin.
struct raw_top_margin : property_base< uint16_t > {};

/// Defines type for left margin.
struct raw_left_margin : property_base< uint16_t > {};

/// Defines type for output height.
struct raw_output_height : property_base< uint16_t > {};

/// Defines type for output width.
struct raw_output_width : property_base< uint16_t > {};

/// Defines type for pixel aspect.
struct raw_pixel_aspect : property_base< double > {};

/// Defines type for image orientation.
struct raw_flip : property_base< uint32_t > {};

/// Defines type for iso speed.
struct raw_iso_speed : property_base< float > {};

/// Defines type for shutter.
struct raw_shutter : property_base< float > {};

/// Defines type for aperture.
struct raw_aperture : property_base< float > {};

/// Defines type for focal length.
struct raw_focal_length : property_base< float > {};

/// Defines type for timestamp.
struct raw_timestamp : property_base< time_t > {};

/// Defines type for shot order.
struct raw_shot_order : property_base< uint16_t > {};

/// Defines type for image description.
struct raw_image_description : property_base< std::string > {};

/// Defines type for artist.
struct raw_artist : property_base< std::string > {};

/// Defines type for libraw version.
struct raw_libraw_version : property_base< std::string > {};

/// Defines type for unpack function name.
struct raw_unpack_function_name : property_base< std::string > {};

/// Read information for raw images.
///
/// The structure is returned when using read_image_info.
template<>
struct image_read_info< raw_tag >
{
    /// Default contructor.
    image_read_info< raw_tag >()
    : _valid( false )
    {}

    // Here, width and height of the image are the ones of the output height (ie after having been processed by dcraw emulator)
    raw_image_width::type       _width;
    raw_image_height::type      _height;
    raw_samples_per_pixel::type _samples_per_pixel;
    raw_bits_per_pixel::type    _bits_per_pixel;

    raw_camera_manufacturer::type _camera_manufacturer;
    raw_camera_model::type        _camera_model;

    raw_raw_images_count::type   _raw_images_count;
    raw_dng_version::type        _dng_version;
    raw_number_colors::type      _number_colors;
    raw_colors_description::type _colors_description;

    raw_raw_width::type      _raw_width;
    raw_raw_height::type     _raw_height;
    raw_visible_width::type  _visible_width;
    raw_visible_height::type _visible_height;
    raw_top_margin::type     _top_margin;
    raw_left_margin::type    _left_margin;
    raw_output_width::type   _output_width;
    raw_output_height::type  _output_height;
    raw_pixel_aspect::type   _pixel_aspect;
    raw_flip::type           _flip;

    raw_iso_speed::type         _iso_speed;
    raw_shutter::type           _shutter;
    raw_aperture::type          _aperture;
    raw_focal_length::type      _focal_length;
    raw_timestamp::type         _timestamp;
    raw_shot_order::type        _shot_order;
    raw_image_description::type _image_description;
    raw_artist::type            _artist;

    raw_libraw_version::type       _libraw_version;
    raw_unpack_function_name::type _unpack_function_name;

    /// Used internaly to identify if the header has been read.
    bool _valid;
};

/// Read settings for raw images.
///
/// The structure can be used for all read_xxx functions, except read_image_info.
template<>
struct image_read_settings< raw_tag > : public image_read_settings_base
{
    /// Default constructor
    image_read_settings()
    : image_read_settings_base()
    {}

    /// Constructor
    /// \param top_left Top left coordinate for reading partial image.
    /// \param dim      Dimensions for reading partial image.
    image_read_settings( const point_t& top_left
                       , const point_t& dim
                       )
    : image_read_settings_base( top_left
                              , dim
                              )
    {}
};

/// Write information for raw images.
///
/// The structure can be used for write_view() function.
template<>
struct image_write_info< raw_tag >
{
};

}} // namespace boost::gil

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
