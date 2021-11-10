//
// Copyright 2007-2012 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_PNG_DETAIL_READ_HPP
#define BOOST_GIL_EXTENSION_IO_PNG_DETAIL_READ_HPP

#include <boost/gil/extension/io/png/tags.hpp>
#include <boost/gil/extension/io/png/detail/reader_backend.hpp>
#include <boost/gil/extension/io/png/detail/is_allowed.hpp>

#include <boost/gil.hpp> // FIXME: Include what you use!
#include <boost/gil/io/base.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>
#include <boost/gil/io/error.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/row_buffer_helper.hpp>
#include <boost/gil/io/typedefs.hpp>

#include <type_traits>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///
/// PNG Reader
///
template< typename Device
        , typename ConversionPolicy
        >
class reader< Device
            , png_tag
            , ConversionPolicy
            >
    : public reader_base< png_tag
                        , ConversionPolicy >
    , public reader_backend< Device
                           , png_tag
                           >
{
private:

    using this_t = reader<Device, png_tag, ConversionPolicy>;
    using cc_t = typename ConversionPolicy::color_converter_type;

public:

    using backend_t = reader_backend<Device, png_tag>;

public:

    reader( const Device&                         io_dev
          , const image_read_settings< png_tag >& settings
          )
    : reader_base< png_tag
                 , ConversionPolicy
                 >()
    , backend_t( io_dev
               , settings
               )
    {}

    reader( const Device&                                          io_dev
          , const typename ConversionPolicy::color_converter_type& cc
          , const image_read_settings< png_tag >&                  settings
          )
    : reader_base< png_tag
                 , ConversionPolicy
                 >( cc )
    , backend_t( io_dev
               , settings
               )
    {}

    template< typename View >
    void apply( const View& view )
    {
        // guard from errors in the following functions
        if (setjmp( png_jmpbuf( this->get_struct() )))
        {
            io_error("png is invalid");
        }

        // The info structures are filled at this point.

        // Now it's time for some transformations.

        if( little_endian() )
        {
            if( this->_info._bit_depth == 16 )
            {
                // Swap bytes of 16 bit files to least significant byte first.
                png_set_swap( this->get_struct() );
            }

            if( this->_info._bit_depth < 8 )
            {
                // swap bits of 1, 2, 4 bit packed pixel formats
                png_set_packswap( this->get_struct() );
            }
        }

        if( this->_info._color_type == PNG_COLOR_TYPE_PALETTE )
        {
            png_set_palette_to_rgb( this->get_struct() );
        }

        if( png_get_valid( this->get_struct(), this->get_info(), PNG_INFO_tRNS ) )
        {
            png_set_tRNS_to_alpha( this->get_struct() );
        }

        // Tell libpng to handle the gamma conversion for you.  The final call
        // is a good guess for PC generated images, but it should be configurable
        // by the user at run time by the user.  It is strongly suggested that
        // your application support gamma correction.
        if( this->_settings._apply_screen_gamma )
        {
            // png_set_gamma will change the image data!

#ifdef BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED
        png_set_gamma( this->get_struct()
                     , this->_settings._screen_gamma
                     , this->_info._file_gamma
                     );
#else
        png_set_gamma( this->get_struct()
                     , this->_settings._screen_gamma
                     , this->_info._file_gamma
                     );
#endif // BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED
        }

        // Turn on interlace handling.  REQUIRED if you are not using
        // png_read_image().  To see how to handle interlacing passes,
        // see the png_read_row() method below:
        this->_number_passes = png_set_interlace_handling( this->get_struct() );


        // The above transformation might have changed the bit_depth and color type.
        png_read_update_info( this->get_struct()
                            , this->get_info()
                            );

        this->_info._bit_depth = png_get_bit_depth( this->get_struct()
                                                  , this->get_info()
                                                  );

        this->_info._num_channels = png_get_channels( this->get_struct()
                                                    , this->get_info()
                                                    );

        this->_info._color_type = png_get_color_type( this->get_struct()
                                                    , this->get_info()
                                                    );

        this->_scanline_length = png_get_rowbytes( this->get_struct()
                                                 , this->get_info()
                                                 );

        switch( this->_info._color_type )
        {
            case PNG_COLOR_TYPE_GRAY:
            {
                switch( this->_info._bit_depth )
                {
                    case  1: read_rows< gray1_image_t::view_t::reference >( view ); break;
                    case  2: read_rows< gray2_image_t::view_t::reference >( view ); break;
                    case  4: read_rows< gray4_image_t::view_t::reference >( view ); break;
                    case  8: read_rows< gray8_pixel_t  >( view ); break;
                    case 16: read_rows< gray16_pixel_t >( view ); break;
                    default: io_error( "png_reader::read_data(): unknown combination of color type and bit depth" );
                }

                break;
            }
            case PNG_COLOR_TYPE_GA:
            {
                #ifdef BOOST_GIL_IO_ENABLE_GRAY_ALPHA
                switch( this->_info._bit_depth )
                {
                    case  8: read_rows< gray_alpha8_pixel_t > ( view ); break;
                    case 16: read_rows< gray_alpha16_pixel_t >( view ); break;
                    default: io_error( "png_reader::read_data(): unknown combination of color type and bit depth" );
                }
                #else
                    io_error( "gray_alpha isn't enabled. Define BOOST_GIL_IO_ENABLE_GRAY_ALPHA when building application." );
                #endif // BOOST_GIL_IO_ENABLE_GRAY_ALPHA


                break;
            }
            case PNG_COLOR_TYPE_RGB:
            {
                switch( this->_info._bit_depth )
                {
                    case 8:  read_rows< rgb8_pixel_t > ( view ); break;
                    case 16: read_rows< rgb16_pixel_t >( view ); break;
                    default: io_error( "png_reader::read_data(): unknown combination of color type and bit depth" );
                }

                break;
            }
            case PNG_COLOR_TYPE_RGBA:
            {
                switch( this->_info._bit_depth )
                {
                    case  8: read_rows< rgba8_pixel_t > ( view ); break;
                    case 16: read_rows< rgba16_pixel_t >( view ); break;
                    default: io_error( "png_reader_color_convert::read_data(): unknown combination of color type and bit depth" );
                }

                break;
            }
            default: io_error( "png_reader_color_convert::read_data(): unknown color type" );
        }

        // read rest of file, and get additional chunks in info_ptr
        png_read_end( this->get_struct()
                    , nullptr
                    );
    }

private:

    template< typename ImagePixel
            , typename View
            >
    void read_rows( const View& view )
    {
        // guard from errors in the following functions
        if (setjmp( png_jmpbuf( this->get_struct() )))
        {
            io_error("png is invalid");
        }

        using row_buffer_helper_t = detail::row_buffer_helper_view<ImagePixel>;

        using it_t = typename row_buffer_helper_t::iterator_t;

        using is_read_and_convert_t = typename std::is_same
            <
                ConversionPolicy,
                detail::read_and_no_convert
            >::type;

        io_error_if( !detail::is_allowed< View >( this->_info
                                                , is_read_and_convert_t()
                                                )
                   , "Image types aren't compatible."
                   );

        std::size_t rowbytes = png_get_rowbytes( this->get_struct()
                                               , this->get_info()
                                               );

        row_buffer_helper_t buffer( rowbytes
                                  , true
                                  );

        png_bytep row_ptr = (png_bytep)( &( buffer.data()[0]));

        for( std::size_t pass = 0; pass < this->_number_passes; pass++ )
        {
            if( pass == this->_number_passes - 1 )
            {
                // skip lines if necessary
                for( std::ptrdiff_t y = 0; y < this->_settings._top_left.y; ++y )
                {
                    // Read the image using the "sparkle" effect.
                    png_read_rows( this->get_struct()
                                 , &row_ptr
                                 , nullptr
                                 , 1
                                 );
                }

                for( std::ptrdiff_t y = 0
                   ; y < this->_settings._dim.y
                   ; ++y
                   )
                {
                    // Read the image using the "sparkle" effect.
                    png_read_rows( this->get_struct()
                                 , &row_ptr
                                 , nullptr
                                 , 1
                                 );

                    it_t first = buffer.begin() + this->_settings._top_left.x;
                    it_t last  = first + this->_settings._dim.x; // one after last element

                    this->_cc_policy.read( first
                                         , last
                                         , view.row_begin( y ));
                }

                // Read the rest of the image. libpng needs that.
                std::ptrdiff_t remaining_rows = static_cast< std::ptrdiff_t >( this->_info._height )
                                              - this->_settings._top_left.y
                                              - this->_settings._dim.y;
                for( std::ptrdiff_t y = 0
                   ; y < remaining_rows
                   ; ++y
                   )
                {
                    // Read the image using the "sparkle" effect.
                    png_read_rows( this->get_struct()
                                 , &row_ptr
                                 , nullptr
                                 , 1
                                 );
                }
            }
            else
            {
                for( int y = 0; y < view.height(); ++y )
                {
                    // Read the image using the "sparkle" effect.
                    png_read_rows( this->get_struct()
                                 , &row_ptr
                                 , nullptr
                                 , 1
                                 );
                }
            }
        }
    }
};

namespace detail {

struct png_type_format_checker
{
    png_type_format_checker( png_bitdepth::type   bit_depth
                           , png_color_type::type color_type
                           )
    : _bit_depth ( bit_depth  )
    , _color_type( color_type )
    {}

    template< typename Image >
    bool apply()
    {
        using is_supported_t = is_read_supported
            <
                typename get_pixel_type<typename Image::view_t>::type,
                png_tag
            >;

        return is_supported_t::_bit_depth  == _bit_depth
            && is_supported_t::_color_type == _color_type;
    }

private:

    png_bitdepth::type   _bit_depth;
    png_color_type::type _color_type;
};

struct png_read_is_supported
{
    template< typename View >
    struct apply : public is_read_supported< typename get_pixel_type< View >::type
                                           , png_tag
                                           >
    {};
};

} // namespace detail


///
/// PNG Dynamic Image Reader
///
template< typename Device
        >
class dynamic_image_reader< Device
                          , png_tag
                          >
    : public reader< Device
                   , png_tag
                   , detail::read_and_no_convert
                   >
{
    using parent_t = reader
        <
            Device,
            png_tag,
            detail::read_and_no_convert
        >;

public:

    dynamic_image_reader( const Device&                         io_dev
                        , const image_read_settings< png_tag >& settings
                        )
    : parent_t( io_dev
              , settings
              )
    {}

    template< typename ...Images >
    void apply( any_image< Images... >& images )
    {
        detail::png_type_format_checker format_checker( this->_info._bit_depth
                                                      , this->_info._color_type
                                                      );

        if( !construct_matched( images
                              , format_checker
                              ))
        {
            io_error( "No matching image type between those of the given any_image and that of the file" );
        }
        else
        {
            this->init_image( images
                            , this->_settings
                            );

            detail::dynamic_io_fnobj< detail::png_read_is_supported
                                    , parent_t
                                    > op( this );

            apply_operation( view( images )
                           , op
                           );
        }
    }
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
