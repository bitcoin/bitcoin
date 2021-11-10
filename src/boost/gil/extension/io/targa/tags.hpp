//
// Copyright 2010 Kenneth Riddile
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TARGA_TAGS_HPP
#define BOOST_GIL_EXTENSION_IO_TARGA_TAGS_HPP

#include <boost/gil/io/base.hpp>

namespace boost { namespace gil {

/// Defines targa tag.
struct targa_tag : format_tag {};

/// See http://en.wikipedia.org/wiki/Truevision_TGA#Header for reference.
/// http://local.wasp.uwa.edu.au/~pbourke/dataformats/tga/


/// Defines type for header sizes.
struct targa_header_size : property_base< uint8_t >
{
    static const type _size = 18; /// Constant size for targa file header size.
};

/// Defines type for offset value.
struct targa_offset : property_base< uint8_t > {};

/// Defines type for color map type property.
struct targa_color_map_type : property_base< uint8_t >
{
    static const type _rgb = 0;
    static const type _indexed = 1;
};

/// Defines type for image type property.
struct targa_image_type : property_base< uint8_t >
{
    static const type _none          = 0;  /// no image data
    static const type _indexed       = 1;  /// indexed
    static const type _rgb           = 2;  /// RGB
    static const type _greyscale     = 3;  /// greyscale
    static const type _rle_indexed   = 9;  /// indexed with RLE compression
    static const type _rle_rgb       = 10; /// RGB with RLE compression
    static const type _rle_greyscale = 11; /// greyscale with RLE compression
};

/// Defines type for color map start property.
struct targa_color_map_start : property_base< uint16_t > {};

/// Defines type for color map length property.
struct targa_color_map_length : property_base< uint16_t > {};

/// Defines type for color map bit depth property.
struct targa_color_map_depth : property_base< uint8_t > {};

/// Defines type for origin x and y value properties.
struct targa_origin_element : property_base< uint16_t > {};

/// Defines type for image dimension properties.
struct targa_dimension : property_base< uint16_t > {};

/// Defines type for image bit depth property.
struct targa_depth : property_base< uint8_t > {};

/// Defines type for image descriptor property.
struct targa_descriptor : property_base< uint8_t > {};

struct targa_screen_origin_bit : property_base< bool > {};

/// Read information for targa images.
///
/// The structure is returned when using read_image_info.
template<>
struct image_read_info< targa_tag >
{
    /// Default contructor.
    image_read_info< targa_tag >()
    : _screen_origin_bit(false)
    , _valid( false )
    {}

    /// The size of this header:
    targa_header_size::type _header_size;

    /// The offset, i.e. starting address, of the byte where the targa data can be found.
    targa_offset::type _offset;

    /// The type of color map used by the image, i.e. RGB or indexed.
    targa_color_map_type::type _color_map_type;

    /// The type of image data, i.e compressed, indexed, uncompressed RGB, etc.
    targa_image_type::type _image_type;

    /// Index of first entry in the color map table.
    targa_color_map_start::type _color_map_start;

    /// Number of entries in the color map table.
    targa_color_map_length::type _color_map_length;

    /// Bit depth for each color map entry.
    targa_color_map_depth::type _color_map_depth;

    /// X coordinate of the image origin.
    targa_origin_element::type _x_origin;

    /// Y coordinate of the image origin.
    targa_origin_element::type _y_origin;

    /// Width of the image in pixels.
    targa_dimension::type _width;

    /// Height of the image in pixels.
    targa_dimension::type _height;

    /// Bit depth of the image.
    targa_depth::type _bits_per_pixel;

    /// The targa image descriptor.
    targa_descriptor::type _descriptor;

    // false: Origin in lower left-hand corner.
    // true: Origin in upper left-hand corner.
    targa_screen_origin_bit::type _screen_origin_bit;

    /// Used internally to identify if the header has been read.
    bool _valid;
};

/// Read settings for targa images.
///
/// The structure can be used for all read_xxx functions, except read_image_info.
template<>
struct image_read_settings< targa_tag > : public image_read_settings_base
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

/// Write information for targa images.
///
/// The structure can be used for write_view() function.
template<>
struct image_write_info< targa_tag >
{
};

} // namespace gil
} // namespace boost

#endif
