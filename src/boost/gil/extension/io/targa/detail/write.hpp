//
// Copyright 2010-2012 Kenneth Riddile, Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TARGA_DETAIL_WRITE_HPP
#define BOOST_GIL_EXTENSION_IO_TARGA_DETAIL_WRITE_HPP

#include <boost/gil/extension/io/targa/tags.hpp>
#include <boost/gil/extension/io/targa/detail/writer_backend.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>

#include <vector>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

namespace detail {

template < int N > struct get_targa_view_type {};
template <> struct get_targa_view_type< 3 > { using type = bgr8_view_t; };
template <> struct get_targa_view_type< 4 > { using type = bgra8_view_t; };

struct targa_write_is_supported
{
    template< typename View >
    struct apply
        : public is_write_supported< typename get_pixel_type< View >::type
                                   , targa_tag
                                   >
    {};
};

} // detail

///
/// TARGA Writer
///
template< typename Device >
class writer< Device
            , targa_tag
            >
    : public writer_backend< Device
                           , targa_tag
                           >
{
private:
    using backend_t = writer_backend<Device, targa_tag>;

public:

    writer( const Device&                        io_dev
          , const image_write_info< targa_tag >& info
          )
    : backend_t( io_dev
               , info
               )
    {}

    template<typename View>
    void apply( const View& view )
    {
        write( view );
    }

private:

    template< typename View >
    void write( const View& view )
    {
        uint8_t bit_depth = static_cast<uint8_t>( num_channels<View>::value * 8 );

        // write the TGA header
        this->_io_dev.write_uint8( 0 ); // offset
        this->_io_dev.write_uint8( targa_color_map_type::_rgb );
        this->_io_dev.write_uint8( targa_image_type::_rgb );
        this->_io_dev.write_uint16( 0 ); // color map start
        this->_io_dev.write_uint16( 0 ); // color map length
        this->_io_dev.write_uint8( 0 ); // color map depth
        this->_io_dev.write_uint16( 0 ); // x origin
        this->_io_dev.write_uint16( 0 ); // y origin
        this->_io_dev.write_uint16( static_cast<uint16_t>( view.width() ) ); // width in pixels
        this->_io_dev.write_uint16( static_cast<uint16_t>( view.height() ) ); // height in pixels
        this->_io_dev.write_uint8( bit_depth );

        if( 32 == bit_depth )
        {
            this->_io_dev.write_uint8( 8 ); // 8-bit alpha channel descriptor
        }
        else
        {
            this->_io_dev.write_uint8( 0 );
        }

        write_image< View
                   , typename detail::get_targa_view_type< num_channels< View >::value >::type
                   >( view );
    }


    template< typename View
            , typename TGA_View
            >
    void write_image( const View& view )
    {
        size_t row_size = view.width() * num_channels<View>::value;
        byte_vector_t buffer( row_size );
        std::fill( buffer.begin(), buffer.end(), 0 );


        TGA_View row = interleaved_view( view.width()
                                       , 1
                                       , reinterpret_cast<typename TGA_View::value_type*>( &buffer.front() )
                                       , row_size
                                       );

        for( typename View::y_coord_t y = view.height() - 1; y > -1; --y )
        {
            copy_pixels( subimage_view( view
                                      , 0
                                      , static_cast<int>( y )
                                      , static_cast<int>( view.width() )
                                      , 1
                                      )
                       , row
                       );

            this->_io_dev.write( &buffer.front(), row_size );
        }

    }
};

///
/// TARGA Dynamic Image Writer
///
template< typename Device >
class dynamic_image_writer< Device
                          , targa_tag
                          >
    : public writer< Device
                   , targa_tag
                   >
{
    using parent_t = writer<Device, targa_tag>;

public:

    dynamic_image_writer( const Device&                        io_dev
                        , const image_write_info< targa_tag >& info
                        )
    : parent_t( io_dev
              , info
              )
    {}

    template< typename ...Views >
    void apply( const any_image_view< Views... >& views )
    {
        detail::dynamic_io_fnobj< detail::targa_write_is_supported
                                , parent_t
                                > op( this );

        apply_operation( views, op );
    }
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // gil
} // boost

#endif
