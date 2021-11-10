//
// Copyright 2012 Kenneth Riddile, Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TARGA_DETAIL_SCANLINE_READ_HPP
#define BOOST_GIL_EXTENSION_IO_TARGA_DETAIL_SCANLINE_READ_HPP

#include <boost/gil/extension/io/targa/detail/is_allowed.hpp>
#include <boost/gil/extension/io/targa/detail/reader_backend.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/bit_operations.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/row_buffer_helper.hpp>
#include <boost/gil/io/scanline_read_iterator.hpp>
#include <boost/gil/io/typedefs.hpp>

#include <vector>

namespace boost { namespace gil {

///
/// Targa Scanline Reader
///
template< typename Device >
class scanline_reader< Device
                     , targa_tag
                     >
    : public reader_backend< Device
                           , targa_tag
                           >
{
public:

    using tag_t = targa_tag;
    using backend_t = reader_backend<Device, tag_t>;
    using this_t = scanline_reader<Device, tag_t>;
    using iterator_t = scanline_read_iterator<this_t>;

    //
    // Constructor
    //
    scanline_reader( Device&                                 device
                   , const image_read_settings< targa_tag >& settings
                   )
    : backend_t( device
                    , settings
                    )
    {
        initialize();
    }

    /// Read part of image defined by View and return the data.
    void read( byte_t* dst, int pos )
    {
        // jump to scanline
        long offset = this->_info._offset
                    + ( this->_info._height - 1 - pos ) * static_cast< long >( this->_scanline_length );

        this->_io_dev.seek( offset );


        read_row( dst );
    }

    /// Skip over a scanline.
    void skip( byte_t*, int )
    {
        this->_io_dev.seek( static_cast<long>( this->_scanline_length )
                          , SEEK_CUR
                          );
    }

    iterator_t begin() { return iterator_t( *this ); }
    iterator_t end()   { return iterator_t( *this, this->_info._height ); }

private:

    void initialize()
    {
        if( this->_info._color_map_type != targa_color_map_type::_rgb )
        {
            io_error( "scanline reader cannot read indexed targa files." );
        }

        if( this->_info._image_type != targa_image_type::_rgb )
        {
            io_error( "scanline reader cannot read this targa image type." );
        }

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

                if( this->_info._screen_origin_bit )
                {
                    io_error( "scanline reader cannot read targa files which have screen origin bit set." );
                }

                switch( this->_info._bits_per_pixel )
                {
                    case 24:
                    case 32:
                    {
                        this->_scanline_length = this->_info._width * ( this->_info._bits_per_pixel / 8 );

                        // jump to first scanline
                        this->_io_dev.seek( static_cast< long >( this->_info._offset ));

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

    void read_row( byte_t* dst )
    {
        this->_io_dev.read( dst, this->_scanline_length );
    }
};

} // namespace gil
} // namespace boost

#endif
