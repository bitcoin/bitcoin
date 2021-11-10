//
// Copyright 2012 Olivier Tournaire, Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_RAW_DETAIL_READ_HPP
#define BOOST_GIL_EXTENSION_IO_RAW_DETAIL_READ_HPP

#include <boost/gil/extension/io/raw/tags.hpp>
#include <boost/gil/extension/io/raw/detail/device.hpp>
#include <boost/gil/extension/io/raw/detail/is_allowed.hpp>
#include <boost/gil/extension/io/raw/detail/reader_backend.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/bit_operations.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/row_buffer_helper.hpp>
#include <boost/gil/io/typedefs.hpp>

#include <cstdio>
#include <sstream>
#include <type_traits>
#include <vector>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

#define BUILD_INTERLEAVED_VIEW(color_layout, bits_per_pixel) \
{ \
    color_layout##bits_per_pixel##_view_t build = boost::gil::interleaved_view(processed_image->width, \
                                                                             processed_image->height, \
                                                                             (color_layout##bits_per_pixel##_pixel_t*)processed_image->data, \
                                                                             processed_image->colors*processed_image->width*processed_image->bits/8); \
    this->_cc_policy.read( build.begin(), build.end(), dst_view.begin() ); \
} \


template< typename Device
        , typename ConversionPolicy
        >
class reader< Device
            , raw_tag
            , ConversionPolicy
            >
    : public reader_base< raw_tag
                        , ConversionPolicy
                        >
    , public reader_backend< Device
                           , raw_tag
                           >
{
private:

    using this_t = reader<Device, raw_tag, ConversionPolicy>;
    using cc_t = typename ConversionPolicy::color_converter_type;

public:

    using backend_t = reader_backend<Device, raw_tag>;

    //
    // Constructor
    //
    reader( const Device&                         io_dev
          , const image_read_settings< raw_tag >& settings
          )
    : backend_t( io_dev
               , settings
               )
    {}

    //
    // Constructor
    //
    reader( const Device&                         io_dev
          , const cc_t&                           cc
          , const image_read_settings< raw_tag >& settings
          )
    : reader_base< raw_tag
                 , ConversionPolicy
                 >( cc )
    , backend_t( io_dev
               , settings
               )
    {}

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

        // TODO: better error handling based on return code
        int return_code = this->_io_dev.unpack();
        io_error_if( return_code != LIBRAW_SUCCESS, "Unable to unpack image" );
        this->_info._unpack_function_name = this->_io_dev.get_unpack_function_name();

        return_code = this->_io_dev.dcraw_process();
        io_error_if( return_code != LIBRAW_SUCCESS, "Unable to emulate dcraw behavior to process image" );

        libraw_processed_image_t* processed_image = this->_io_dev.dcraw_make_mem_image(&return_code);
        io_error_if( return_code != LIBRAW_SUCCESS, "Unable to dcraw_make_mem_image" );

        if(processed_image->colors!=1 && processed_image->colors!=3)
            io_error( "Image is neither gray nor RGB" );

        if(processed_image->bits!=8 && processed_image->bits!=16)
            io_error( "Image is neither 8bit nor 16bit" );

        // TODO Olivier Tournaire
        // Here, we should use a metafunction to reduce code size and avoid a (compile time) macro
        if(processed_image->bits==8)
        {
            if(processed_image->colors==1){ BUILD_INTERLEAVED_VIEW(gray, 8); }
            else                          { BUILD_INTERLEAVED_VIEW(rgb,  8); }
        }
        else if(processed_image->bits==16)
        {
            if(processed_image->colors==1){ BUILD_INTERLEAVED_VIEW(gray, 16); }
            else                          { BUILD_INTERLEAVED_VIEW(rgb,  16); }
        }
    }
};

namespace detail {

struct raw_read_is_supported
{
    template< typename View >
    struct apply : public is_read_supported< typename get_pixel_type< View >::type
                                           , raw_tag
                                           >
    {};
};

struct raw_type_format_checker
{
    raw_type_format_checker( const image_read_info< raw_tag >& info )
    : _info( info )
    {}

    template< typename Image >
    bool apply()
    {
        using view_t = typename Image::view_t;
        return is_allowed<view_t>(_info, std::true_type{});
    }

private:
    ///todo: do we need this here. Should be part of reader_backend
    const image_read_info< raw_tag >& _info;
};

} // namespace detail

///
/// RAW Dynamic Reader
///
template< typename Device >
class dynamic_image_reader< Device
                          , raw_tag
                          >
    : public reader< Device
                   , raw_tag
                   , detail::read_and_no_convert
                   >
{
    using parent_t = reader<Device, raw_tag, detail::read_and_no_convert>;

public:

    dynamic_image_reader( const Device&                         io_dev
                        , const image_read_settings< raw_tag >& settings
                        )
    : parent_t( io_dev
              , settings
              )
    {}

    template< typename ...Images >
    void apply( any_image< Images... >& images )
    {
        detail::raw_type_format_checker format_checker( this->_info );

        if( !construct_matched( images
                               , format_checker
                               ))
        {
            std::ostringstream error_message;
            error_message << "No matching image type between those of the given any_image and that of the file.\n";
            error_message << "Image type must be {gray||rgb}{8||16} unsigned for RAW image files.";
            io_error( error_message.str().c_str() );
        }
        else
        {
            if( !this->_info._valid )
	      this->read_header();
            this->init_image(images, this->_settings);

            detail::dynamic_io_fnobj< detail::raw_read_is_supported
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
