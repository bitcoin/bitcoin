//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_BMP_DETAIL_READ_HPP
#define BOOST_GIL_EXTENSION_IO_BMP_DETAIL_READ_HPP

#include <boost/gil/extension/io/bmp/detail/is_allowed.hpp>
#include <boost/gil/extension/io/bmp/detail/reader_backend.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/bit_operations.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/row_buffer_helper.hpp>
#include <boost/gil/io/typedefs.hpp>

#include <boost/assert.hpp>

#include <type_traits>
#include <vector>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///
/// BMP Reader
///
template< typename Device
        , typename ConversionPolicy
        >
class reader< Device
            , bmp_tag
            , ConversionPolicy
            >
    : public reader_base< bmp_tag
                        , ConversionPolicy
                        >
    , public reader_backend< Device
                           , bmp_tag
                           >
{
private:

    using this_t = reader<Device, bmp_tag, ConversionPolicy>;
    using cc_t = typename ConversionPolicy::color_converter_type;

public:

    using backend_t = reader_backend< Device, bmp_tag>;

public:

    //
    // Constructor
    //
    reader( const Device&                         io_dev
          , const image_read_settings< bmp_tag >& settings
          )
    : backend_t( io_dev
               , settings
               )
    , _pitch( 0 )
    {}

    //
    // Constructor
    //
    reader( const Device&                         io_dev
          , const ConversionPolicy&               cc
          , const image_read_settings< bmp_tag >& settings
          )
    : reader_base< bmp_tag
                 , ConversionPolicy
                 >( cc )
    , backend_t( io_dev
               , settings
               )
    , _pitch( 0 )
    {}


    /// Read image.
    template< typename View >
    void apply( const View& dst_view )
    {
        if( this->_info._valid == false )
        {
            io_error( "Image header was not read." );
        }

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

        // the row pitch must be multiple 4 bytes
        if( this->_info._bits_per_pixel < 8 )
        {
            _pitch = static_cast<long>((( this->_info._width * this->_info._bits_per_pixel ) + 7 ) >> 3 );
        }
        else
        {
            _pitch = static_cast<long>( this->_info._width * (( this->_info._bits_per_pixel + 7 ) >> 3 ));
        }

        _pitch = (_pitch + 3) & ~3;

        switch( this->_info._bits_per_pixel )
        {
            case 1:
            {
                this->_scanline_length = ( this->_info._width * num_channels< rgba8_view_t >::value + 3 ) & ~3;

                read_palette_image
                    <
                        gray1_image_t::view_t,
                        detail::mirror_bits<byte_vector_t, std::true_type>
                    >(dst_view);
                break;
            }

            case 4:
            {
                switch ( this->_info._compression )
                {
                    case bmp_compression::_rle4:
                    {
                        ///@todo How can we determine that?
                        this->_scanline_length = 0;

                        read_palette_image_rle( dst_view );

                        break;
                    }

                    case bmp_compression::_rgb:
                    {
                        this->_scanline_length = ( this->_info._width * num_channels< rgba8_view_t >::value + 3 ) & ~3;

                        read_palette_image
                            <
                                gray4_image_t::view_t,
                                detail::swap_half_bytes<byte_vector_t, std::true_type>
                            >(dst_view);
                        break;
                    }

                    default:
                    {
                        io_error( "Unsupported compression mode in BMP file." );
                        break;
                    }
                }
                break;
            }

            case 8:
            {
                switch ( this->_info._compression )
                {
                    case bmp_compression::_rle8:
                    {
                        ///@todo How can we determine that?
                        this->_scanline_length = 0;

                        read_palette_image_rle( dst_view );
                        break;
                    }

                    case bmp_compression::_rgb:
                    {
                        this->_scanline_length = ( this->_info._width * num_channels< rgba8_view_t >::value + 3 ) & ~3;

                        read_palette_image< gray8_image_t::view_t
                                          , detail::do_nothing< std::vector< gray8_pixel_t > >
                                          > ( dst_view );
                        break;
                    }

                    default:
                    {
                        io_error( "Unsupported compression mode in BMP file." );
                        break;
                    }
                }

                break;
            }

            case 15: case 16:
            {
                this->_scanline_length = ( this->_info._width * num_channels< rgb8_view_t >::value + 3 ) & ~3;

                read_data_15( dst_view );

                break;
            }

            case 24:
            {
                this->_scanline_length = ( this->_info._width * num_channels< rgb8_view_t >::value + 3 ) & ~3;

                read_data< bgr8_view_t  >( dst_view );

                break;
            }

            case 32:
            {
                this->_scanline_length = ( this->_info._width * num_channels< rgba8_view_t >::value + 3 ) & ~3;

                read_data< bgra8_view_t >( dst_view );

                break;
            }
        }
    }

private:

    long get_offset( std::ptrdiff_t pos )
    {
        if( this->_info._height > 0 )
        {
            // the image is upside down
            return static_cast<long>( ( this->_info._offset
                                      + ( this->_info._height - 1 - pos ) * _pitch
                                    ));
        }
        else
        {
            return static_cast<long>( ( this->_info._offset
                                      + pos * _pitch
                                    ));
        }
    }

    template< typename View_Src
            , typename Byte_Manipulator
            , typename View_Dst
            >
    void read_palette_image( const View_Dst& view )
    {
        this->read_palette();

        using rh_t = detail::row_buffer_helper_view<View_Src>;
        using it_t = typename rh_t::iterator_t;

        rh_t rh( _pitch, true );

        // we have to swap bits
        Byte_Manipulator byte_manipulator;

        for( std::ptrdiff_t y = 0
           ; y < this->_settings._dim.y
           ; ++y
           )
        {
            this->_io_dev.seek( get_offset( y + this->_settings._top_left.y ));

            this->_io_dev.read( reinterpret_cast< byte_t* >( rh.data() )
                        , _pitch
                        );

            byte_manipulator( rh.buffer() );

            typename View_Dst::x_iterator dst_it = view.row_begin( y );

            it_t it  = rh.begin() + this->_settings._top_left.x;
            it_t end = it + this->_settings._dim.x;

            for( ; it != end; ++it, ++dst_it )
            {
                unsigned char c = get_color( *it, gray_color_t() );
                *dst_it = this->_palette[ c ];
            }
        }
    }

    template< typename View >
    void read_data_15( const View& view )
    {
        byte_vector_t row( _pitch );

        // read the color masks
        if( this->_info._compression == bmp_compression::_bitfield )
        {
            this->_mask.red.mask    = this->_io_dev.read_uint32();
            this->_mask.green.mask  = this->_io_dev.read_uint32();
            this->_mask.blue.mask   = this->_io_dev.read_uint32();

            this->_mask.red.width   = detail::count_ones( this->_mask.red.mask   );
            this->_mask.green.width = detail::count_ones( this->_mask.green.mask );
            this->_mask.blue.width  = detail::count_ones( this->_mask.blue.mask  );

            this->_mask.red.shift   = detail::trailing_zeros( this->_mask.red.mask   );
            this->_mask.green.shift = detail::trailing_zeros( this->_mask.green.mask );
            this->_mask.blue.shift  = detail::trailing_zeros( this->_mask.blue.mask  );
        }
        else if( this->_info._compression == bmp_compression::_rgb )
        {
            switch( this->_info._bits_per_pixel )
            {
                case 15:
                case 16:
                {
                    this->_mask.red.mask   = 0x007C00; this->_mask.red.width   = 5; this->_mask.red.shift   = 10;
                    this->_mask.green.mask = 0x0003E0; this->_mask.green.width = 5; this->_mask.green.shift =  5;
                    this->_mask.blue.mask  = 0x00001F; this->_mask.blue.width  = 5; this->_mask.blue.shift  =  0;

                    break;
                }

                case 24:
                case 32:
                {
                    this->_mask.red.mask   = 0xFF0000; this->_mask.red.width   = 8; this->_mask.red.shift   = 16;
                    this->_mask.green.mask = 0x00FF00; this->_mask.green.width = 8; this->_mask.green.shift =  8;
                    this->_mask.blue.mask  = 0x0000FF; this->_mask.blue.width  = 8; this->_mask.blue.shift  =  0;

                    break;
                }
            }
        }
        else
        {
            io_error( "bmp_reader::apply(): unsupported BMP compression" );
        }

        using image_t = rgb8_image_t;
        using it_t = typename image_t::view_t::x_iterator;

        for( std::ptrdiff_t y = 0
           ; y < this->_settings._dim.y
           ; ++y
           )
        {
            this->_io_dev.seek( get_offset( y + this->_settings._top_left.y ));

            this->_io_dev.read( &row.front()
                        , row.size()
                        );

            image_t img_row( this->_info._width, 1 );
            image_t::view_t v = gil::view( img_row );
            it_t it = v.row_begin( 0 );

            it_t beg = v.row_begin( 0 ) + this->_settings._top_left.x;
            it_t end = beg + this->_settings._dim.x;

            byte_t* src = &row.front();
            for( int32_t i = 0 ; i < this->_info._width; ++i, src += 2 )
            {
                int p = ( src[1] << 8 ) | src[0];

                int r = ((p & this->_mask.red.mask)   >> this->_mask.red.shift)   << (8 - this->_mask.red.width);
                int g = ((p & this->_mask.green.mask) >> this->_mask.green.shift) << (8 - this->_mask.green.width);
                int b = ((p & this->_mask.blue.mask)  >> this->_mask.blue.shift)  << (8 - this->_mask.blue.width);

                get_color( it[i], red_t()   ) = static_cast< byte_t >( r );
                get_color( it[i], green_t() ) = static_cast< byte_t >( g );
                get_color( it[i], blue_t()  ) = static_cast< byte_t >( b );
            }

            this->_cc_policy.read( beg
                                 , end
                                 , view.row_begin( y )
                                 );
        }
    }


    // 8-8-8 BGR
    // 8-8-8-8 BGRA
    template< typename View_Src
            , typename View_Dst
            >
    void read_data( const View_Dst& view )
    {
        byte_vector_t row( _pitch );

        View_Src v = interleaved_view( this->_info._width
                                     , 1
                                     , (typename View_Src::value_type*) &row.front()
                                     , this->_info._width * num_channels< View_Src >::value
                                     );

        typename View_Src::x_iterator beg = v.row_begin( 0 ) + this->_settings._top_left.x;
        typename View_Src::x_iterator end = beg + this->_settings._dim.x;

        for( std::ptrdiff_t y = 0
           ; y < this->_settings._dim.y
           ; ++y
           )
        {
            this->_io_dev.seek( get_offset( y + this->_settings._top_left.y ));

            this->_io_dev.read( &row.front()
                        , row.size()
                        );

            this->_cc_policy.read( beg
                                 , end
                                 , view.row_begin( y )
                                 );
        }
    }

    template< typename Buffer
            , typename View
            >
    void copy_row_if_needed( const Buffer&  buf
                           , const View&    view
                           , std::ptrdiff_t y
                           )
    {
        if(  y >= this->_settings._top_left.y
          && y <  this->_settings._dim.y
          )
        {
            typename Buffer::const_iterator beg = buf.begin() + this->_settings._top_left.x;
            typename Buffer::const_iterator end = beg + this->_settings._dim.x;

            std::copy( beg
                     , end
                     , view.row_begin( y )
                     );
        }
    }

    template< typename View_Dst >
    void read_palette_image_rle( const View_Dst& view )
    {
        BOOST_ASSERT(
            this->_info._compression == bmp_compression::_rle4 ||
            this->_info._compression == bmp_compression::_rle8);

        this->read_palette();

        // jump to start of rle4 data
        this->_io_dev.seek( this->_info._offset );

        // we need to know the stream position for padding purposes
        std::size_t stream_pos = this->_info._offset;

        using Buf_type = std::vector<rgba8_pixel_t>;
        Buf_type buf( this->_settings._dim.x );
        Buf_type::iterator dst_it  = buf.begin();
        Buf_type::iterator dst_end = buf.end();

        // If height is positive, the bitmap is a bottom-up DIB.
        // If height is negative, the bitmap is a top-down DIB.
        // The origin of a bottom-up DIB is the bottom left corner of the bitmap image,
        // which is the first pixel of the first row of bitmap data.
        // The origin of a top-down DIB is also the bottom left corner of the bitmap image,
        // but in this case the bottom left corner is the first pixel of the last row of bitmap data.
        // - "Programming Windows", 5th Ed. by Charles Petzold explains Windows docs ambiguities.
        std::ptrdiff_t ybeg = 0;
        std::ptrdiff_t yend = this->_settings._dim.y;
        std::ptrdiff_t yinc = 1;
        if( this->_info._height > 0 )
        {
            ybeg = this->_settings._dim.y - 1;
            yend = -1;
            yinc = -1;
        }

        std::ptrdiff_t y = ybeg;
        bool finished = false;

        while ( !finished )
        {
            std::ptrdiff_t count  = this->_io_dev.read_uint8();
            std::ptrdiff_t second = this->_io_dev.read_uint8();
            stream_pos += 2;

            if ( count )
            {
                // encoded mode

                // clamp to boundary
                if( count > dst_end - dst_it )
                {
                    count = dst_end - dst_it;
                }

                if( this->_info._compression == bmp_compression::_rle4 )
                {
                    std::ptrdiff_t cs[2] = { second >> 4, second & 0x0f };

                    for( int i = 0; i < count; ++i )
                    {
                        *dst_it++ = this->_palette[ cs[i & 1] ];
                    }
                }
                else
                {
                    for( int i = 0; i < count; ++i )
                    {
                        *dst_it++ = this->_palette[ second ];
                    }
                }
            }
            else
            {
                switch( second )
                {
                    case 0:  // end of row
                    {
                        copy_row_if_needed( buf, view, y );

                        y += yinc;
                        if( y == yend )
                        {
                            finished = true;
                        }
                        else
                        {
                            dst_it = buf.begin();
                            dst_end = buf.end();
                        }

                        break;
                    }

                    case 1:  // end of bitmap
                    {
                        copy_row_if_needed( buf, view, y );
                        finished = true;

                        break;
                    }

                    case 2:  // offset coordinates
                    {
                        std::ptrdiff_t dx = this->_io_dev.read_uint8();
                        std::ptrdiff_t dy = this->_io_dev.read_uint8() * yinc;
                        stream_pos += 2;

                        if( dy )
                        {
                            copy_row_if_needed( buf, view, y );
                        }

                        std::ptrdiff_t x = dst_it - buf.begin();
                        x += dx;

                        if( x > this->_info._width )
                        {
                            io_error( "Mangled BMP file." );
                        }

                        y += dy;
                        if( yinc > 0 ? y > yend : y < yend )
                        {
                            io_error( "Mangled BMP file." );
                        }

                        dst_it = buf.begin() + x;
                        dst_end = buf.end();

                        break;
                    }

                    default:  // absolute mode
                    {
                        count = second;

                        // clamp to boundary
                        if( count > dst_end - dst_it )
                        {
                            count = dst_end - dst_it;
                        }

                        if ( this->_info._compression == bmp_compression::_rle4 )
                        {
                            for( int i = 0; i < count; ++i )
                            {
                                uint8_t packed_indices = this->_io_dev.read_uint8();
                                ++stream_pos;

                                *dst_it++ = this->_palette[ packed_indices >> 4 ];
                                if( ++i == second )
                                    break;

                                *dst_it++ = this->_palette[ packed_indices & 0x0f ];
                            }
                        }
                        else
                        {
                            for( int i = 0; i < count; ++i )
                            {
                                uint8_t c = this->_io_dev.read_uint8();
                                ++stream_pos;
                                *dst_it++ = this->_palette[ c ];
                             }
                        }

                        // pad to word boundary
                        if( ( stream_pos - get_offset( 0 )) & 1 )
                        {
                            this->_io_dev.seek( 1, SEEK_CUR );
                            ++stream_pos;
                        }

                        break;
                    }
                }
            }
        }
    }

private:

    std::size_t _pitch;
};

namespace detail {

class bmp_type_format_checker
{
public:

    bmp_type_format_checker( const bmp_bits_per_pixel::type& bpp )
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
    bmp_type_format_checker& operator=( const bmp_type_format_checker& ) { return *this; }

private:

    const bmp_bits_per_pixel::type _bpp;
};

struct bmp_read_is_supported
{
    template< typename View >
    struct apply : public is_read_supported< typename get_pixel_type< View >::type
                                           , bmp_tag
                                           >
    {};
};

} // namespace detail

///
/// BMP Dynamic Reader
///
template< typename Device >
class dynamic_image_reader< Device
                          , bmp_tag
                          >
    : public reader< Device
                   , bmp_tag
                   , detail::read_and_no_convert
                   >
{
    using parent_t = reader<Device, bmp_tag, detail::read_and_no_convert>;

public:

    dynamic_image_reader( const Device&                         io_dev
                        , const image_read_settings< bmp_tag >& settings
                        )
    : parent_t( io_dev
              , settings
              )
    {}

    template< typename ...Images >
    void apply( any_image< Images... >& images )
    {
        detail::bmp_type_format_checker format_checker( this->_info._bits_per_pixel );

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

            detail::dynamic_io_fnobj< detail::bmp_read_is_supported
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

} // gil
} // boost

#endif
