//
// Copyright 2007-2008 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_JPEG_DETAIL_WRITE_HPP
#define BOOST_GIL_EXTENSION_IO_JPEG_DETAIL_WRITE_HPP

#include <boost/gil/extension/io/jpeg/tags.hpp>
#include <boost/gil/extension/io/jpeg/detail/supported_types.hpp>
#include <boost/gil/extension/io/jpeg/detail/writer_backend.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>

#include <vector>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#pragma warning(disable:4611) //interaction between '_setjmp' and C++ object destruction is non-portable
#endif

namespace detail {

struct jpeg_write_is_supported
{
    template< typename View >
    struct apply
        : public is_write_supported< typename get_pixel_type< View >::type
                                   , jpeg_tag
                                   >
    {};
};

} // detail

///
/// JPEG Writer
///
template< typename Device >
class writer< Device
            , jpeg_tag
            >
    : public writer_backend< Device
                           , jpeg_tag
                           >
{
public:

    using backend_t = writer_backend<Device, jpeg_tag>;

public:

    writer( const Device&                       io_dev
          , const image_write_info< jpeg_tag >& info
          )
    : backend_t( io_dev
               , info
               )
    {}

    template<typename View>
    void apply( const View& view )
    {
        write_rows( view );
    }

private:

    template<typename View>
    void write_rows( const View& view )
    {
        std::vector< pixel< typename channel_type< View >::type
                          , layout<typename color_space_type< View >::type >
                          >
                   > row_buffer( view.width() );

        // In case of an error we'll jump back to here and fire an exception.
        // @todo Is the buffer above cleaned up when the exception is thrown?
        //       The strategy right now is to allocate necessary memory before
        //       the setjmp.
        if( setjmp( this->_mark )) { this->raise_error(); }

        using channel_t = typename channel_type<typename View::value_type>::type;

        this->get()->image_width      = JDIMENSION( view.width()  );
        this->get()->image_height     = JDIMENSION( view.height() );
        this->get()->input_components = num_channels<View>::value;
        this->get()->in_color_space   = detail::jpeg_write_support< channel_t
                                                                  , typename color_space_type< View >::type
                                                                  >::_color_space;

        jpeg_set_defaults( this->get() );

        jpeg_set_quality( this->get()
                        , this->_info._quality
                        , TRUE
                        );

        // Needs to be done after jpeg_set_defaults() since it's overridding this value back to slow.
        this->get()->dct_method = this->_info._dct_method;


        // set the pixel dimensions
        this->get()->density_unit = this->_info._density_unit;
        this->get()->X_density    = this->_info._x_density;
        this->get()->Y_density    = this->_info._y_density;

        // done reading header information

        jpeg_start_compress( this->get()
                           , TRUE
                           );

        JSAMPLE* row_addr = reinterpret_cast< JSAMPLE* >( &row_buffer[0] );

        for( int y =0; y != view.height(); ++ y )
        {
            std::copy( view.row_begin( y )
                     , view.row_end  ( y )
                     , row_buffer.begin()
                     );

            jpeg_write_scanlines( this->get()
                                , &row_addr
                                , 1
                                );
        }

        jpeg_finish_compress ( this->get() );
    }
};

///
/// JPEG Dyamic Image Writer
///
template< typename Device >
class dynamic_image_writer< Device
                          , jpeg_tag
                          >
    : public writer< Device
                   , jpeg_tag
                   >
{
    using parent_t = writer<Device, jpeg_tag>;

public:

    dynamic_image_writer( const Device&                       io_dev
                        , const image_write_info< jpeg_tag >& info
                        )
    : parent_t( io_dev
              , info
              )
    {}

    template< typename ...Views >
    void apply( const any_image_view< Views... >& views )
    {
        detail::dynamic_io_fnobj< detail::jpeg_write_is_supported
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
