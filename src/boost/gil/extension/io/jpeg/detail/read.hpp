//
// Copyright 2007-2012 Christian Henning, Andreas Pokorny, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_JPEG_DETAIL_READ_HPP
#define BOOST_GIL_EXTENSION_IO_JPEG_DETAIL_READ_HPP

#include <boost/gil/extension/io/jpeg/tags.hpp>
#include <boost/gil/extension/io/jpeg/detail/base.hpp>
#include <boost/gil/extension/io/jpeg/detail/is_allowed.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/typedefs.hpp>

#include <csetjmp>
#include <type_traits>
#include <vector>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#pragma warning(disable:4611) //interaction between '_setjmp' and C++ object destruction is non-portable
#endif

///
/// JPEG Reader
///
template< typename Device
        , typename ConversionPolicy
        >
class reader< Device
            , jpeg_tag
            , ConversionPolicy
            >
    : public reader_base< jpeg_tag
                        , ConversionPolicy
                        >
    , public reader_backend< Device
                           , jpeg_tag
                           >
{
private:

    using this_t = reader<Device, jpeg_tag, ConversionPolicy>;
    using cc_t = typename ConversionPolicy::color_converter_type;

public:

    using backend_t = reader_backend<Device, jpeg_tag>;

public:

    //
    // Constructor
    //
    reader( const Device&                          io_dev
          , const image_read_settings< jpeg_tag >& settings
          )
    : reader_base< jpeg_tag
                 , ConversionPolicy
                 >()

    , backend_t( io_dev
               , settings
               )
    {}

    //
    // Constructor
    //
    reader( const Device&                                          io_dev
          , const typename ConversionPolicy::color_converter_type& cc
          , const image_read_settings< jpeg_tag >&                 settings
          )
    : reader_base< jpeg_tag
                 , ConversionPolicy
                 >( cc )
    , backend_t( io_dev
               , settings
               )
    {}

    template<typename View>
    void apply( const View& view )
    {
        // Fire exception in case of error.
        if( setjmp( this->_mark ))
        {
            this->raise_error();
        }

        this->get()->dct_method = this->_settings._dct_method;

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

        if( jpeg_start_decompress( this->get() ) == false )
        {
            io_error( "Cannot start decompression." );
        }

        switch( this->_info._color_space )
        {
            case JCS_GRAYSCALE:
            {
                this->_scanline_length = this->_info._width;
                read_rows< gray8_pixel_t >( view );

                break;
            }

            case JCS_RGB:
            //!\todo add Y'CbCr? We loose image quality when reading JCS_YCbCr as JCS_RGB
            case JCS_YCbCr:
            {
                this->_scanline_length = this->_info._width * num_channels< rgb8_view_t >::value;

                read_rows< rgb8_pixel_t  >( view );
                break;
            }

            case JCS_CMYK:
            //!\todo add Y'CbCrK? We loose image quality when reading JCS_YCCK as JCS_CMYK
            case JCS_YCCK:
            {
                this->get()->out_color_space = JCS_CMYK;
                this->_scanline_length = this->_info._width * num_channels< cmyk8_view_t >::value;

                read_rows< cmyk8_pixel_t >( view );

                break;
            }
            default: { io_error( "Unsupported jpeg color space." ); }
        }

        jpeg_finish_decompress ( this->get() );
    }

private:

    template< typename ImagePixel
            , typename View
            >
    void read_rows( const View& view )
    {
        using buffer_t = std::vector<ImagePixel>;
        buffer_t buffer( this->_info._width );

        // In case of an error we'll jump back to here and fire an exception.
        // @todo Is the buffer above cleaned up when the exception is thrown?
        //       The strategy right now is to allocate necessary memory before
        //       the setjmp.
        if( setjmp( this->_mark ))
        {
            this->raise_error();
        }


        JSAMPLE *row_adr = reinterpret_cast< JSAMPLE* >( &buffer[0] );

        //Skip scanlines if necessary.
        for( int y = 0; y <  this->_settings._top_left.y; ++y )
        {
            io_error_if( jpeg_read_scanlines( this->get()
                                            , &row_adr
                                            , 1
                                            ) !=1
                       , "jpeg_read_scanlines: fail to read JPEG file"
                       );
        }

        // Read data.
        for( int y = 0; y < view.height(); ++y )
        {
            io_error_if( jpeg_read_scanlines( this->get()
                                            , &row_adr
                                            , 1
                                            ) != 1
                       , "jpeg_read_scanlines: fail to read JPEG file"
                       );

            typename buffer_t::iterator beg = buffer.begin() + this->_settings._top_left.x;
            typename buffer_t::iterator end = beg + this->_settings._dim.x;

            this->_cc_policy.read( beg
                                 , end
                                 , view.row_begin( y )
                                 );
        }

        //@todo: There might be a better way to do that.
        while( this->get()->output_scanline < this->get()->image_height )
        {
            io_error_if( jpeg_read_scanlines( this->get()
                                            , &row_adr
                                            , 1
                                            ) !=1
                       , "jpeg_read_scanlines: fail to read JPEG file"
                       );
        }

    }
};

namespace detail {

struct jpeg_type_format_checker
{
    jpeg_type_format_checker( jpeg_color_space::type color_space )
    : _color_space( color_space )
    {}

    template< typename Image >
    bool apply()
    {
        return is_read_supported< typename get_pixel_type< typename Image::view_t >::type
                                , jpeg_tag
                                >::_color_space == _color_space;
    }

private:

    jpeg_color_space::type _color_space;
};

struct jpeg_read_is_supported
{
    template< typename View >
    struct apply : public is_read_supported< typename get_pixel_type< View >::type
                                           , jpeg_tag
                                           >
    {};
};

} // namespace detail

///
/// JPEG Dynamic Reader
///
template< typename Device >
class dynamic_image_reader< Device
                          , jpeg_tag
                          >
    : public reader< Device
                   , jpeg_tag
                   , detail::read_and_no_convert
                   >
{
    using parent_t = reader<Device, jpeg_tag, detail::read_and_no_convert>;

public:

    dynamic_image_reader( const Device&                          io_dev
                        , const image_read_settings< jpeg_tag >& settings
                        )
    : parent_t( io_dev
              , settings
              )
    {}

    template< typename ...Images >
    void apply( any_image< Images... >& images )
    {
        detail::jpeg_type_format_checker format_checker( this->_info._color_space != JCS_YCbCr
                                                       ? this->_info._color_space
                                                       : JCS_RGB
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

            detail::dynamic_io_fnobj< detail::jpeg_read_is_supported
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
