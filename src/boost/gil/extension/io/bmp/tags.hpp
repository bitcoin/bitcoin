//
// Copyright 2008 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_BMP_TAGS_HPP
#define BOOST_GIL_EXTENSION_IO_BMP_TAGS_HPP

#include <boost/gil/io/base.hpp>

namespace boost { namespace gil {

/// Defines bmp tag.
struct bmp_tag : format_tag {};

    /// See http://en.wikipedia.org/wiki/BMP_file_format#BMP_File_Header for reference.

/// Defines type for offset value.
struct bmp_offset : property_base< uint32_t > {};

/// Defines type for header sizes.
struct bmp_header_size : property_base< uint32_t >
{
    static const type _size            = 14; /// Constant size for bmp file header size.
    static const type _win32_info_size = 40; /// Constant size for win32 bmp info header size.
    static const type _os2_info_size   = 12; /// Constant size for os2 bmp info header size.
};

/// Defines type for image width property.
struct bmp_image_width : property_base< int32_t > {};

/// Defines type for image height property.
struct bmp_image_height : property_base< int32_t > {};

/// Defines type for bits per pixels property.
struct bmp_bits_per_pixel : property_base< uint16_t > {};

/// Defines type for compression property.
struct bmp_compression : property_base< uint32_t >
{
    static const type _rgb      = 0; /// RGB without compression
    static const type _rle8     = 1; /// 8 bit index with RLE compression
    static const type _rle4     = 2; /// 4 bit index with RLE compression
    static const type _bitfield = 3; /// 16 or 32 bit fields without compression
};

/// Defines type for image size property.
struct bmp_image_size : property_base< uint32_t > {};

/// Defines type for horizontal resolution property.
struct bmp_horizontal_resolution : property_base< int32_t > {};

/// Defines type for vertical resolution property.
struct bmp_vertical_resolution : property_base< int32_t > {};

/// Defines type for number of colors property.
struct bmp_num_colors : property_base< uint32_t > {};

/// Defines type for important number of colors property.
struct bmp_num_important_colors : property_base< uint32_t > {};

/// if height is negative then image is stored top-down instead of bottom-up.
struct bmp_top_down : property_base< bool > {};

static const uint32_t bmp_signature = 0x4D42; /// Constant signature for bmp file format.

/// Read information for bmp images.
///
/// The structure is returned when using read_image_info.
template<>
struct image_read_info< bmp_tag >
{
    /// Default contructor.
    image_read_info< bmp_tag >()
    : _top_down(false)
    , _valid( false )
    {}

    /// The offset, i.e. starting address, of the byte where the bitmap data can be found.
    bmp_offset::type _offset;

    /// The size of this header:
    /// - 40 bytes for Windows V3 header
    /// - 12 bytes for OS/2 V1 header
    bmp_header_size::type _header_size;

    /// The bitmap width in pixels ( signed integer ).
    bmp_image_width::type  _width;

    /// The bitmap height in pixels ( signed integer ).
    bmp_image_height::type _height;

    /// The number of bits per pixel, which is the color depth of the image.
    /// Typical values are 1, 4, 8, 16, 24 and 32.
    bmp_bits_per_pixel::type _bits_per_pixel;

    /// The compression method being used. See above for a list of possible values.
    bmp_compression::type _compression;

    /// The image size. This is the size of the raw bitmap data (see below),
    /// and should not be confused with the file size.
    bmp_image_size::type _image_size;

    /// The horizontal resolution of the image. (pixel per meter, signed integer)
    bmp_horizontal_resolution::type _horizontal_resolution;

    /// The vertical resolution of the image. (pixel per meter, signed integer)
    bmp_vertical_resolution::type   _vertical_resolution;

    /// The number of colors in the color palette, or 0 to default to 2^n - 1.
    bmp_num_colors::type            _num_colors;

    /// The number of important colors used, or 0 when every color is important;
    /// generally ignored.
    bmp_num_important_colors::type  _num_important_colors;

    bmp_top_down::type _top_down;

    /// Used internaly to identify is the header has been read.
    bool _valid;
};

/// Read settings for bmp images.
///
/// The structure can be used for all read_xxx functions, except read_image_info.
template<>
struct image_read_settings< bmp_tag > : public image_read_settings_base
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

/// Write information for bmp images.
///
/// The structure can be used for write_view() function.
template<>
struct image_write_info< bmp_tag >
{
};

} // namespace gil
} // namespace boost

#endif
