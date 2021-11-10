//
// Copyright 2012 Kenneth Riddile, Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TARGA_DETAIL_READ_HPP
#define BOOST_GIL_EXTENSION_IO_TARGA_DETAIL_READ_HPP

#include <boost/gil/extension/io/targa/tags.hpp>
#include <boost/gil/extension/io/targa/detail/reader_backend.hpp>
#include <boost/gil/extension/io/targa/detail/is_allowed.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/bit_operations.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/row_buffer_helper.hpp>
#include <boost/gil/io/typedefs.hpp>

#include <type_traits>
#include <vector>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///
/// Targa Reader
///
template< typename Device
        , typename ConversionPolicy
        >
class reader< Device
            , targa_tag
            , ConversionPolicy
            >
    : public reader_base< targa_tag
                        , ConversionPolicy
                        >
    , public reader_backend< Device
                           , targa_tag
                           >
{
private:

    using this_t = reader<Device, targa_tag, ConversionPolicy>;
    using cc_t = typename ConversionPolicy::color_converter_type;

public:

    using backend_t = reader_backend<Device, targa_tag>;

    reader( const Device&                           io_dev
          , const image_read_settings< targa_tag >& settings
          )
    : reader_base< targa_tag
                 , ConversionPolicy
                 >()
    , backend_t( io_dev
               , settings
               )
    {}

    reader( const Device&                         io_dev
          , const cc_t&                           cc
          , const image_read_settings< targa_tag >& settings
          )
    : reader_base< targa_tag
                 , ConversionPolicy
                 >( cc )
    , backend_t( io_dev
               , settings
               )
    {}

    template< typename View >
    void apply( const View& dst_view )
    {
        using is_read_and_convert_t = typename std::is_same
            <
                ConversionPolicy,
                detail::read_and_no_convert
            >::type;

        io_error_if( !detail::is_allowed< View >( this->_info, is_read_and_convert_t() )
                   , "Image types aren't compatible."
                   );

        switch( this->_info._image_type )
        {
            case targa_image_type::_rgb:
            {
                if( this->_info._color_map_type != targa_color_map_type::_rgb )
                {
                    io_error( "Inconsistent color map type and image type in targa file." );
                }

                if( this->_info._color_map_length != 0 )
                {
                    io_error( "Non-indexed targa files containing a palette are not supported." );
                }

                switch( this->_info._bits_per_pixel )
                {
                    case 24:
                    {
                        this->_scanline_length = this->_info._width * ( this->_info._bits_per_pixel / 8 );

                        if( this->_info._screen_origin_bit )
                        {
                            read_data< bgr8_view_t >( flipped_up_down_view( dst_view ) );
                        }
                        else
                        {
                            read_data< bgr8_view_t >( dst_view );
                        }

                        break;
                    }
                    case 32:
                    {
                        this->_scanline_length = this->_info._width * ( this->_info._bits_per_pixel / 8 );

                        if( this->_info._screen_origin_bit )
                        {
                            read_data< bgra8_view_t >( flipped_up_down_view( dst_view ) );
                        }
                        else
                        {
                            read_data< bgra8_view_t >( dst_view );
                        }

                        break;
                    }
                    default:
                    {
                        io_error( "Unsupported bit depth in targa file." );
                        break;
                    }
                }

                break;
            }
            case targa_image_type::_rle_rgb:
            {
                if( this->_info._color_map_type != targa_color_map_type::_rgb )
                {
                    io_error( "Inconsistent color map type and image type in targa file." );
                }

                if( this->_info._color_map_length != 0 )
                {
                    io_error( "Non-indexed targa files containing a palette are not supported." );
                }

                switch( this->_info._bits_per_pixel )
                {
                    case 24:
                    {
                        if( this->_info._screen_origin_bit )
                        {
                            read_rle_data< bgr8_view_t >( flipped_up_down_view( dst_view ) );
                        }
                        else
                        {
                            read_rle_data< bgr8_view_t >( dst_view );
                        }
                        break;
                    }
                    case 32:
                    {
                        if( this->_info._screen_origin_bit )
                        {
                            read_rle_data< bgra8_view_t >( flipped_up_down_view( dst_view ) );
                        }
                        else
                        {
                            read_rle_data< bgra8_view_t >( dst_view );
                        }
                        break;
                    }
                    default:
                    {
                        io_error( "Unsupported bit depth in targa file." );
                        break;
                    }
                }

                break;
            }
            default:
            {
                io_error( "Unsupported image type in targa file." );
                break;
            }
        }
    }

private:

    // 8-8-8 BGR
    // 8-8-8-8 BGRA
    template< typename View_Src, typename View_Dst >
    void read_data( const View_Dst& view )
    {
        byte_vector_t row( this->_info._width * (this->_info._bits_per_pixel / 8) );

        // jump to first scanline
        this->_io_dev.seek( static_cast< long >( this->_info._offset ));

        View_Src v = interleaved_view( this->_info._width,
                                       1,
                                       reinterpret_cast<typename View_Src::value_type*>( &row.front() ),
                                       this->_info._width * num_channels< View_Src >::value
                                     );

        typename View_Src::x_iterator beg = v.row_begin( 0 ) + this->_settings._top_left.x;
        typename View_Src::x_iterator end = beg + this->_settings._dim.x;

        // read bottom up since targa origin is bottom left
        for( std::ptrdiff_t y = this->_settings._dim.y - 1; y > -1; --y )
        {
            // @todo: For now we're reading the whole scanline which is
            // slightly inefficient. Later versions should try to read
            // only the bytes which are necessary.
            this->_io_dev.read( &row.front(), row.size() );
            this->_cc_policy.read( beg, end, view.row_begin(y) );
        }
    }

    // 8-8-8 BGR
    // 8-8-8-8 BGRA
    template< typename View_Src, typename View_Dst >
    void read_rle_data( const View_Dst& view )
    {
        targa_depth::type bytes_per_pixel = this->_info._bits_per_pixel / 8;
        size_t image_size = this->_info._width * this->_info._height * bytes_per_pixel;
        byte_vector_t image_data( image_size );

        this->_io_dev.seek( static_cast< long >( this->_info._offset ));

        for( size_t pixel = 0; pixel < image_size; )
        {
            targa_offset::type current_byte = this->_io_dev.read_uint8();

            if( current_byte & 0x80 ) // run length chunk (high bit = 1)
            {
                uint8_t chunk_length = current_byte - 127;
                uint8_t pixel_data[4];
                for( size_t channel = 0; channel < bytes_per_pixel; ++channel )
                {
                    pixel_data[channel] = this->_io_dev.read_uint8();
                }

                // Repeat the next pixel chunk_length times
                for( uint8_t i = 0; i < chunk_length; ++i, pixel += bytes_per_pixel )
                {
                    memcpy( &image_data[pixel], pixel_data, bytes_per_pixel );
                }
            }
            else // raw chunk
            {
                uint8_t chunk_length = current_byte + 1;

                // Write the next chunk_length pixels directly
                size_t pixels_written = chunk_length * bytes_per_pixel;
                this->_io_dev.read( &image_data[pixel], pixels_written );
                pixel += pixels_written;
            }
        }

        View_Src v = flipped_up_down_view( interleaved_view( this->_info._width,
                                                             this->_info._height,
                                                             reinterpret_cast<typename View_Src::value_type*>( &image_data.front() ),
                                                             this->_info._width * num_channels< View_Src >::value ) );

        for( std::ptrdiff_t y = 0; y != this->_settings._dim.y; ++y )
        {
            typename View_Src::x_iterator beg = v.row_begin( y ) + this->_settings._top_left.x;
            typename View_Src::x_iterator end = beg + this->_settings._dim.x;
            this->_cc_policy.read( beg, end, view.row_begin(y) );
        }
    }
};

namespace detail {

class targa_type_format_checker
{
public:

    targa_type_format_checker( const targa_depth::type& bpp )
    : _bpp( bpp )
    {}

    template< typename Image >
    bool apply()
    {
        if( _bpp < 32 )
        {
            return pixels_are_compatible< typename Image::value_type, rgb8_pixel_t >::value
                   ? true
                   : false;
        }
        else
        {
            return pixels_are_compatible< typename Image::value_type, rgba8_pixel_t >::value
                   ? true
                   : false;
        }
    }

private:

    // to avoid C4512
    targa_type_format_checker& operator=( const targa_type_format_checker& ) { return *this; }

private:

    const targa_depth::type _bpp;
};

struct targa_read_is_supported
{
    template< typename View >
    struct apply : public is_read_supported< typename get_pixel_type< View >::type
                                           , targa_tag
                                           >
    {};
};

} // namespace detail

///
/// Targa Dynamic Image Reader
///
template< typename Device >
class dynamic_image_reader< Device
                          , targa_tag
                          >
    : public reader< Device
                   , targa_tag
                   , detail::read_and_no_convert
                   >
{
    using parent_t = reader<Device, targa_tag, detail::read_and_no_convert>;

public:

    dynamic_image_reader( const Device&                           io_dev
                        , const image_read_settings< targa_tag >& settings
                        )
    : parent_t( io_dev
              , settings
              )
    {}

    template< typename ...Images >
    void apply( any_image< Images... >& images )
    {
        detail::targa_type_format_checker format_checker( this->_info._bits_per_pixel );

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

            detail::dynamic_io_fnobj< detail::targa_read_is_supported
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
