//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_PNM_DETAIL_SCANLINE_READ_HPP
#define BOOST_GIL_EXTENSION_IO_PNM_DETAIL_SCANLINE_READ_HPP

#include <boost/gil/extension/io/pnm/detail/is_allowed.hpp>
#include <boost/gil/extension/io/pnm/detail/reader_backend.hpp>

#include <boost/gil.hpp> // FIXME: Include what you use!
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
/// PNM Reader
///
template< typename Device >
class scanline_reader< Device
                     , pnm_tag
                     >
    : public reader_backend< Device
                           , pnm_tag
                           >
{
public:

    using tag_t = pnm_tag;
    using backend_t = reader_backend<Device, tag_t>;
    using this_t = scanline_reader<Device, tag_t>;
    using iterator_t = scanline_read_iterator<this_t>;

public:
    scanline_reader( Device&                                device
                   , const image_read_settings< pnm_tag >& settings
                   )
    : backend_t( device
               , settings
               )
    {
        initialize();
    }

    /// Read part of image defined by View and return the data.
    void read( byte_t* dst
             , int
             )
    {
        _read_function( this, dst );
    }

    /// Skip over a scanline.
    void skip( byte_t*, int )
    {
        _skip_function( this );
    }

    iterator_t begin() { return iterator_t( *this ); }
    iterator_t end()   { return iterator_t( *this, this->_info._height ); }

private:

    void initialize()
    {
        switch( this->_info._type )
        {
            // reading mono text is reading grayscale but with only two values
            case pnm_image_type::mono_asc_t::value:
            case pnm_image_type::gray_asc_t::value:
            {
                this->_scanline_length = this->_info._width;

                _read_function = std::mem_fn(&this_t::read_text_row);
                _skip_function = std::mem_fn(&this_t::skip_text_row);

                break;
            }

            case pnm_image_type::color_asc_t::value:
            {
                this->_scanline_length = this->_info._width * num_channels< rgb8_view_t >::value;

                _read_function = std::mem_fn(&this_t::read_text_row);
                _skip_function = std::mem_fn(&this_t::skip_text_row);

                break;
            }


            case pnm_image_type::mono_bin_t::value:
            {
                //gray1_image_t
                this->_scanline_length = ( this->_info._width + 7 ) >> 3;

                _read_function = std::mem_fn(&this_t::read_binary_bit_row);
                _skip_function = std::mem_fn(&this_t::skip_binary_row);

                break;
            }

            case pnm_image_type::gray_bin_t::value:
            {
                // gray8_image_t
                this->_scanline_length = this->_info._width;

                _read_function = std::mem_fn(&this_t::read_binary_byte_row);
                _skip_function = std::mem_fn(&this_t::skip_binary_row);

                break;
            }

            case pnm_image_type::color_bin_t::value:
            {
                // rgb8_image_t
                this->_scanline_length = this->_info._width * num_channels< rgb8_view_t >::value;

                _read_function = std::mem_fn(&this_t::read_binary_byte_row);
                _skip_function = std::mem_fn(&this_t::skip_binary_row);

                break;
            }

            default: { io_error( "Unsupported pnm file." ); break; }
        }
    }

    void read_text_row( byte_t* dst )
    {
        for( std::size_t x = 0; x < this->_scanline_length; ++x )
        {
            for( uint32_t k = 0; ; )
            {
                int ch = this->_io_dev.getc_unchecked();

                if( isdigit( ch ))
                {
                    _text_buffer[ k++ ] = static_cast< char >( ch );
                }
                else if( k )
                {
                    _text_buffer[ k ] = 0;
                    break;
                }
                else if( ch == EOF || !isspace( ch ))
                {
                    return;
                }
            }

            int value = atoi( _text_buffer );

            if( this->_info._max_value == 1 )
            {
                // for pnm format 0 is white
                dst[x] = ( value != 0 )
                            ? 0
                            : 255;
            }
            else
            {
                dst[x] = static_cast< byte_t >( value );
            }
        }
    }

    void skip_text_row()
    {
        for( std::size_t x = 0; x < this->_scanline_length; ++x )
        {
            for( uint32_t k = 0; ; )
            {
                int ch = this->_io_dev.getc_unchecked();

                if( isdigit( ch ))
                {
                    k++;
                }
                else if( k )
                {
                    break;
                }
                else if( ch == EOF || !isspace( ch ))
                {
                    return;
                }
            }
        }
    }


    void read_binary_bit_row( byte_t* dst )
    {
        this->_io_dev.read( dst
                    , this->_scanline_length
                    );

        _negate_bits    ( dst, this->_scanline_length );
        _swap_half_bytes( dst, this->_scanline_length );

    }

    void read_binary_byte_row( byte_t* dst )
    {
        this->_io_dev.read( dst
                    , this->_scanline_length
                    );
    }

    void skip_binary_row()
    {
        this->_io_dev.seek( static_cast<long>( this->_scanline_length ), SEEK_CUR );
    }

private:

    char _text_buffer[16];

    // For bit_aligned images we need to negate all bytes in the row_buffer
    // to make sure that 0 is black and 255 is white.
    detail::negate_bits<std::vector<byte_t>, std::true_type> _negate_bits;
    detail::swap_half_bytes<std::vector<byte_t>, std::true_type> _swap_half_bytes;

    std::function<void(this_t*, byte_t*)> _read_function;
    std::function<void(this_t*)> _skip_function;
};


} // namespace gil
} // namespace boost

#endif
