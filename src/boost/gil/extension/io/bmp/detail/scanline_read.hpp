//
// Copyright 2008 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_BMP_DETAIL_SCANLINE_READ_HPP
#define BOOST_GIL_EXTENSION_IO_BMP_DETAIL_SCANLINE_READ_HPP

#include <boost/gil/extension/io/bmp/detail/is_allowed.hpp>
#include <boost/gil/extension/io/bmp/detail/reader_backend.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/bit_operations.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/row_buffer_helper.hpp>
#include <boost/gil/io/scanline_read_iterator.hpp>
#include <boost/gil/io/typedefs.hpp>

#include <functional>
#include <type_traits>
#include <vector>

namespace boost { namespace gil {

///
/// BMP Scanline Reader
///
template< typename Device >
class scanline_reader< Device
                     , bmp_tag
                     >
    : public reader_backend< Device
                           , bmp_tag
                           >
{
public:

    using tag_t = bmp_tag;
    using backend_t = reader_backend<Device, tag_t>;
    using this_t = scanline_reader<Device, tag_t>;
    using iterator_t = scanline_read_iterator<this_t>;

public:

    //
    // Constructor
    //
    scanline_reader( Device&                               device
                   , const image_read_settings< bmp_tag >& settings
                   )
    : backend_t( device
                    , settings
                    )

    , _pitch( 0 )
    {
        initialize();
    }

    /// Read part of image defined by View and return the data.
    void read( byte_t* dst, int pos )
    {
        // jump to scanline
        long offset = 0;

        if( this->_info._height > 0 )
        {
            // the image is upside down
            offset = this->_info._offset
                   + ( this->_info._height - 1 - pos ) * this->_pitch;
        }
        else
        {
            offset = this->_info._offset
                   + pos * _pitch;
        }

        this->_io_dev.seek( offset );


        // read data
        _read_function(this, dst);
    }

    /// Skip over a scanline.
    void skip( byte_t*, int )
    {
        // nothing to do.
    }

    iterator_t begin() { return iterator_t( *this ); }
    iterator_t end()   { return iterator_t( *this, this->_info._height ); }

private:

    void initialize()
    {
        if( this->_info._bits_per_pixel < 8 )
        {
            _pitch = (( this->_info._width * this->_info._bits_per_pixel ) + 7 ) >> 3;
        }
        else
        {
            _pitch = this->_info._width * (( this->_info._bits_per_pixel + 7 ) >> 3);
        }

        _pitch = (_pitch + 3) & ~3;

        //

        switch( this->_info._bits_per_pixel )
        {
            case 1:
            {
                this->_scanline_length = ( this->_info._width * num_channels< rgba8_view_t >::value + 3 ) & ~3;

                read_palette();
                _buffer.resize( _pitch );

                _read_function = std::mem_fn(&this_t::read_1_bit_row);

                break;
            }

            case 4:
            {
                switch( this->_info._compression )
                {
                    case bmp_compression::_rle4:
                    {
                        io_error( "Cannot read run-length encoded images in iterator mode. Try to read as whole image." );

                        break;
                    }

                    case bmp_compression::_rgb :
                    {
                        this->_scanline_length = ( this->_info._width * num_channels< rgba8_view_t >::value + 3 ) & ~3;

                        read_palette();
                        _buffer.resize( _pitch );

                        _read_function = std::mem_fn(&this_t::read_4_bits_row);

                        break;
                    }

                    default:
                    {
                        io_error( "Unsupported compression mode in BMP file." );
                    }
                }

                break;
            }

            case 8:
            {
                switch( this->_info._compression )
                {
                    case bmp_compression::_rle8:
                    {
                        io_error( "Cannot read run-length encoded images in iterator mode. Try to read as whole image." );

                        break;
                    }
                    case bmp_compression::_rgb:
                    {
                        this->_scanline_length = ( this->_info._width * num_channels< rgba8_view_t >::value + 3 ) & ~3;

                        read_palette();
                        _buffer.resize( _pitch );

                        _read_function = std::mem_fn(&this_t::read_8_bits_row);

                        break;
                    }

                    default: { io_error( "Unsupported compression mode in BMP file." ); break; }
                }

                break;
            }

            case 15:
            case 16:
            {
                this->_scanline_length = ( this->_info._width * num_channels< rgb8_view_t >::value + 3 ) & ~3;

                _buffer.resize( _pitch );

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
                    io_error( "Unsupported BMP compression." );
                }


                _read_function = std::mem_fn(&this_t::read_15_bits_row);

                break;
            }

            case 24:
            {
                this->_scanline_length = ( this->_info._width * num_channels< rgb8_view_t >::value + 3 ) & ~3;
                _read_function = std::mem_fn(&this_t::read_row);

                break;
            }

            case 32:
            {
                this->_scanline_length = ( this->_info._width * num_channels< rgba8_view_t >::value + 3 ) & ~3;
                _read_function = std::mem_fn(&this_t::read_row);

                break;
            }

            default:
            {
                io_error( "Unsupported bits per pixel." );
            }
        }
    }

    void read_palette()
    {
        if( this->_palette.size() > 0 )
        {
            // palette has been read already.
            return;
        }

        int entries = this->_info._num_colors;

        if( entries == 0 )
        {
            entries = 1u << this->_info._bits_per_pixel;
        }

		this->_palette.resize( entries, rgba8_pixel_t(0,0,0,0) );

        for( int i = 0; i < entries; ++i )
        {
            get_color( this->_palette[i], blue_t()  ) = this->_io_dev.read_uint8();
            get_color( this->_palette[i], green_t() ) = this->_io_dev.read_uint8();
            get_color( this->_palette[i], red_t()   ) = this->_io_dev.read_uint8();

            // there are 4 entries when windows header
            // but 3 for os2 header
            if( this->_info._header_size == bmp_header_size::_win32_info_size )
            {
                this->_io_dev.read_uint8();
            }

        } // for
    }

    template< typename View >
    void read_bit_row( byte_t* dst )
    {
        using src_view_t = View;
        using dst_view_t = rgba8_image_t::view_t;

        src_view_t src_view = interleaved_view( this->_info._width
                                              , 1
                                              , (typename src_view_t::x_iterator) &_buffer.front()
                                              , this->_pitch
                                              );

        dst_view_t dst_view = interleaved_view( this->_info._width
                                              , 1
                                              , (typename dst_view_t::value_type*) dst
                                              , num_channels< dst_view_t >::value * this->_info._width
                                              );


        typename src_view_t::x_iterator src_it = src_view.row_begin( 0 );
        typename dst_view_t::x_iterator dst_it = dst_view.row_begin( 0 );

        for( dst_view_t::x_coord_t i = 0
           ; i < this->_info._width
           ; ++i, src_it++, dst_it++
           )
        {
            unsigned char c = get_color( *src_it, gray_color_t() );
            *dst_it = this->_palette[c];
        }
    }

    // Read 1 bit image. The colors are encoded by an index.
    void read_1_bit_row( byte_t* dst )
    {
        this->_io_dev.read( &_buffer.front(), _pitch );
        _mirror_bits( _buffer );

        read_bit_row< gray1_image_t::view_t >( dst );
    }

    // Read 4 bits image. The colors are encoded by an index.
    void read_4_bits_row( byte_t* dst )
    {
        this->_io_dev.read( &_buffer.front(), _pitch );
        _swap_half_bytes( _buffer );

        read_bit_row< gray4_image_t::view_t >( dst );
    }

    /// Read 8 bits image. The colors are encoded by an index.
    void read_8_bits_row( byte_t* dst )
    {
        this->_io_dev.read( &_buffer.front(), _pitch );

        read_bit_row< gray8_image_t::view_t >( dst );
    }

    /// Read 15 or 16 bits image.
    void read_15_bits_row( byte_t* dst )
    {
        using dst_view_t = rgb8_view_t;

        dst_view_t dst_view = interleaved_view( this->_info._width
                                              , 1
                                              , (typename dst_view_t::value_type*) dst
                                              , this->_pitch
                                              );

        typename dst_view_t::x_iterator dst_it = dst_view.row_begin( 0 );

        //
        byte_t* src = &_buffer.front();
        this->_io_dev.read( src, _pitch );

        for( dst_view_t::x_coord_t i = 0
           ; i < this->_info._width
           ; ++i, src += 2
           )
        {
            int p = ( src[1] << 8 ) | src[0];

            int r = ((p & this->_mask.red.mask)   >> this->_mask.red.shift)   << (8 - this->_mask.red.width);
            int g = ((p & this->_mask.green.mask) >> this->_mask.green.shift) << (8 - this->_mask.green.width);
            int b = ((p & this->_mask.blue.mask)  >> this->_mask.blue.shift)  << (8 - this->_mask.blue.width);

            get_color( dst_it[i], red_t()   ) = static_cast< byte_t >( r );
            get_color( dst_it[i], green_t() ) = static_cast< byte_t >( g );
            get_color( dst_it[i], blue_t()  ) = static_cast< byte_t >( b );
        }
    }

    void read_row( byte_t* dst )
    {
        this->_io_dev.read( dst, _pitch );
    }

private:

    // the row pitch must be multiple of 4 bytes
    int _pitch;

    std::vector<byte_t> _buffer;
    detail::mirror_bits <std::vector<byte_t>, std::true_type> _mirror_bits;
    detail::swap_half_bytes<std::vector<byte_t>, std::true_type> _swap_half_bytes;

    std::function<void(this_t*, byte_t*)> _read_function;
};

} // namespace gil
} // namespace boost

#endif
