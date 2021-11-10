//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_BMP_DETAIL_WRITE_HPP
#define BOOST_GIL_EXTENSION_IO_BMP_DETAIL_WRITE_HPP

#include <boost/gil/extension/io/bmp/tags.hpp>
#include <boost/gil/extension/io/bmp/detail/writer_backend.hpp>

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

struct bmp_write_is_supported
{
    template< typename View >
    struct apply
        : public is_write_supported< typename get_pixel_type< View >::type
                                   , bmp_tag
                                   >
    {};
};

template < int N > struct get_bgr_cs {};
template <> struct get_bgr_cs< 1 > { using type = gray8_view_t; };
template <> struct get_bgr_cs< 3 > { using type = bgr8_view_t;  };
template <> struct get_bgr_cs< 4 > { using type = bgra8_view_t; };

} // namespace detail

///
/// BMP Writer
///
template< typename Device >
class writer< Device
            , bmp_tag
            >
    : public writer_backend< Device
                           , bmp_tag
                           >
{
public:

    writer( const Device&                      io_dev
          , const image_write_info< bmp_tag >& info
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

    using backend_t = writer_backend<Device, bmp_tag>;

    template< typename View >
    void write( const View& view )
    {
        // using channel_t = typename channel_type<
        //             typename get_pixel_type<View>::type>::type;

        // using color_space_t = typename color_space_type<View>::type;

        // check if supported
/*
        /// todo
        if( bmp_read_write_support_private<channel_t, color_space_t>::channel != 8)
        {
            io_error("Input view type is incompatible with the image type");
        }
*/

        // compute the file size
        int bpp = num_channels< View >::value * 8;
        int entries = 0;

/*
        /// @todo: Not supported for now. bit_aligned_images refer to indexed images
        ///        in this context.
        if( bpp <= 8 )
        {
            entries = 1u << bpp;
        }
*/

        std::size_t spn = ( view.width() * num_channels< View >::value + 3 ) & ~3;
        std::size_t ofs = bmp_header_size::_size
                        + bmp_header_size::_win32_info_size
                        + entries * 4;

        std::size_t siz = ofs + spn * view.height();

        // write the BMP file header
        this->_io_dev.write_uint16( bmp_signature );
        this->_io_dev.write_uint32( (uint32_t) siz );
        this->_io_dev.write_uint16( 0 );
        this->_io_dev.write_uint16( 0 );
        this->_io_dev.write_uint32( (uint32_t) ofs );

        // writes Windows information header
        this->_io_dev.write_uint32( bmp_header_size::_win32_info_size );
        this->_io_dev.write_uint32( static_cast< uint32_t >( view.width()  ));
        this->_io_dev.write_uint32( static_cast< uint32_t >( view.height() ));
        this->_io_dev.write_uint16( 1 );
        this->_io_dev.write_uint16( static_cast< uint16_t >( bpp ));
        this->_io_dev.write_uint32( bmp_compression::_rgb );
        this->_io_dev.write_uint32( 0 );
        this->_io_dev.write_uint32( 0 );
        this->_io_dev.write_uint32( 0 );
        this->_io_dev.write_uint32( entries );
        this->_io_dev.write_uint32( 0 );

        write_image< View
                   , typename detail::get_bgr_cs< num_channels< View >::value >::type
                   >( view, spn );
    }


    template< typename View
            , typename BMP_View
            >
    void write_image( const View&       view
                    , const std::size_t spn
                    )
    {
        byte_vector_t buffer( spn );
        std::fill( buffer.begin(), buffer.end(), 0 );


        BMP_View row = interleaved_view( view.width()
                                       , 1
                                       , (typename BMP_View::value_type*) &buffer.front()
                                       , spn
                                       );

        for( typename View::y_coord_t y = view.height() - 1; y > -1; --y  )
        {
            copy_pixels( subimage_view( view
                                      , 0
                                      , (int) y
                                      , (int) view.width()
                                      , 1
                                      )
                       , row
                       );

            this->_io_dev.write( &buffer.front(), spn );
        }

    }
};

///
/// BMP Dynamic Image Writer
///
template< typename Device >
class dynamic_image_writer< Device
                          , bmp_tag
                          >
    : public writer< Device
                   , bmp_tag
                   >
{
    using parent_t = writer<Device, bmp_tag>;

public:

    dynamic_image_writer( const Device&                      io_dev
                        , const image_write_info< bmp_tag >& info
                        )
    : parent_t( io_dev
              , info
              )
    {}

    template< typename ...Views >
    void apply( const any_image_view< Views... >& views )
    {
        detail::dynamic_io_fnobj< detail::bmp_write_is_supported
                                , parent_t
                                > op( this );

        apply_operation( views
                       , op
                       );
    }
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // gil
} // boost

#endif
