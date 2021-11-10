//
// Copyright 2007-2008 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_PNG_OLD_HPP
#define BOOST_GIL_EXTENSION_IO_PNG_OLD_HPP

#include <boost/gil/extension/io/png.hpp>

namespace boost { namespace gil {

/// \ingroup PNG_IO
/// \brief Returns the width and height of the PNG file at the specified location.
/// Throws std::ios_base::failure if the location does not correspond to a valid PNG file
template<typename String>
inline point_t png_read_dimensions(String const& filename)
{
    using backend_t = typename get_reader_backend<String, png_tag>::type;
    backend_t backend = read_image_info(filename, png_tag());
    return { static_cast<std::ptrdiff_t>(backend._info._width), static_cast<std::ptrdiff_t>(backend._info._height) };
}

/// \ingroup PNG_IO
/// \brief Loads the image specified by the given png image file name into the given view.
/// Triggers a compile assert if the view color space and channel depth are not supported by the PNG library or by the I/O extension.
/// Throws std::ios_base::failure if the file is not a valid PNG file, or if its color space or channel depth are not
/// compatible with the ones specified by View, or if its dimensions don't match the ones of the view.
template< typename String
        , typename View
        >
inline
void png_read_view( const String& filename
                   , const View&   view
                   )
{
    read_view( filename
             , view
             , png_tag()
             );
}

/// \ingroup PNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, and loads the pixels into it.
/// Triggers a compile assert if the image color space or channel depth are not supported by the PNG library or by the I/O extension.
/// Throws std::ios_base::failure if the file is not a valid PNG file, or if its color space or channel depth are not
/// compatible with the ones specified by Image
template< typename String
        , typename Image
        >
inline
void png_read_image( const String& filename
                   , Image&        img
                   )
{
    read_image( filename
              , img
              , png_tag()
              );
}

/// \ingroup PNG_IO
/// \brief Loads the image specified by the given png image file name and color-converts it into the given view.
/// Throws std::ios_base::failure if the file is not a valid PNG file, or if its dimensions don't match the ones of the view.
template< typename String
        , typename View
        , typename CC
        >
inline
void png_read_and_convert_view( const String& filename
                               , const View&   view
                               , CC            cc
                               )
{
    read_and_convert_view( filename
                         , view
                         , cc
                         , png_tag()
                         );
}

/// \ingroup PNG_IO
/// \brief Loads the image specified by the given png image file name and color-converts it into the given view.
/// Throws std::ios_base::failure if the file is not a valid PNG file, or if its dimensions don't match the ones of the view.
template< typename String
        , typename View
        >
inline
void png_read_and_convert_view( const String& filename
                               , const View&   view
                               )
{
    read_and_convert_view( filename
                         , view
                         , png_tag()
                         );
}

/// \ingroup PNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, loads and color-converts the pixels into it.
/// Throws std::ios_base::failure if the file is not a valid PNG file
template< typename String
        , typename Image
        , typename CC
        >
inline
void png_read_and_convert_image( const String& filename
                                , Image& img
                                , CC     cc
                                )
{
    read_and_convert_image( filename
                          , img
                          , cc
                          , png_tag()
                          );
}

/// \ingroup PNG_IO
/// \brief Allocates a new image whose dimensions are determined by the given png image file, loads and color-converts the pixels into it.
/// Throws std::ios_base::failure if the file is not a valid PNG file
template< typename String
        , typename Image
        >
inline
void png_read_and_convert_image( const String filename
                                , Image&       img
                                )
{
    read_and_convert_image( filename
                          , img
                          , png_tag()
                          );
}

/// \ingroup PNG_IO
/// \brief Saves the view to a png file specified by the given png image file name.
/// Triggers a compile assert if the view color space and channel depth are not supported by the PNG library or by the I/O extension.
/// Throws std::ios_base::failure if it fails to create the file.
template< typename String
        , typename View
        >
inline
void png_write_view( const String& filename
                    , const View&   view
                    )
{
    write_view( filename
              , view
              , png_tag()
              );
}

}  // namespace gil
}  // namespace boost

#endif
