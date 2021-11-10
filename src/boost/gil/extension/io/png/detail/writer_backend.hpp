//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_PNG_DETAIL_WRITER_BACKEND_HPP
#define BOOST_GIL_EXTENSION_IO_PNG_DETAIL_WRITER_BACKEND_HPP

#include <boost/gil/extension/io/png/tags.hpp>
#include <boost/gil/extension/io/png/detail/base.hpp>
#include <boost/gil/extension/io/png/detail/supported_types.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/typedefs.hpp>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#pragma warning(disable:4611) //interaction between '_setjmp' and C++ object destruction is non-portable
#endif

///
/// PNG Writer Backend
///
template< typename Device >
struct writer_backend< Device
                     , png_tag
                     >
    : public detail::png_struct_info_wrapper
{

private:

    using this_t = writer_backend<Device, png_tag>;

public:

    using format_tag_t = png_tag;

    ///
    /// Constructor
    ///
    writer_backend( const Device&                      io_dev
                  , const image_write_info< png_tag >& info
                  )
    : png_struct_info_wrapper( false )
    , _io_dev( io_dev )
    , _info( info )
    {
        // Create and initialize the png_struct with the desired error handler
        // functions.  If you want to use the default stderr and longjump method,
        // you can supply NULL for the last three parameters.  We also check that
        // the library version is compatible with the one used at compile time,
        // in case we are using dynamically linked libraries.  REQUIRED.
        get()->_struct = png_create_write_struct( PNG_LIBPNG_VER_STRING
                                                , nullptr  // user_error_ptr
                                                , nullptr  // user_error_fn
                                                , nullptr  // user_warning_fn
                                                );

        io_error_if( get_struct() == nullptr
                   , "png_writer: fail to call png_create_write_struct()"
                   );

        // Allocate/initialize the image information data.  REQUIRED
        get()->_info = png_create_info_struct( get_struct() );

        if( get_info() == nullptr )
        {
            png_destroy_write_struct( &get()->_struct
                                    , nullptr
                                    );

            io_error( "png_writer: fail to call png_create_info_struct()" );
        }

        // Set error handling.  REQUIRED if you aren't supplying your own
        // error handling functions in the png_create_write_struct() call.
        if( setjmp( png_jmpbuf( get_struct() )))
        {
            //free all of the memory associated with the png_ptr and info_ptr
            png_destroy_write_struct( &get()->_struct
                                    , &get()->_info
                                    );

            io_error( "png_writer: fail to call setjmp()" );
        }

        init_io( get_struct() );
    }

protected:

    template< typename View >
    void write_header( const View& view )
    {
        using png_rw_info_t = detail::png_write_support
            <
                typename channel_type<typename get_pixel_type<View>::type>::type,
                typename color_space_type<View>::type
            >;

        // Set the image information here.  Width and height are up to 2^31,
        // bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
        // the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
        // PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
        // or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
        // PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
        // currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
        png_set_IHDR( get_struct()
                    , get_info()
                    , static_cast< png_image_width::type  >( view.width()  )
                    , static_cast< png_image_height::type >( view.height() )
                    , static_cast< png_bitdepth::type     >( png_rw_info_t::_bit_depth )
                    , static_cast< png_color_type::type   >( png_rw_info_t::_color_type )
                    , _info._interlace_method
                    , _info._compression_type
                    , _info._filter_method
                    );

#ifdef BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED
        if( _info._valid_cie_colors )
        {
            png_set_cHRM( get_struct()
                        , get_info()
                        , _info._white_x
                        , _info._white_y
                        , _info._red_x
                        , _info._red_y
                        , _info._green_x
                        , _info._green_y
                        , _info._blue_x
                        , _info._blue_y
                        );
        }

        if( _info._valid_file_gamma )
        {
            png_set_gAMA( get_struct()
                        , get_info()
                        , _info._file_gamma
                        );
        }
#else
        if( _info._valid_cie_colors )
        {
            png_set_cHRM_fixed( get_struct()
                              , get_info()
                              , _info._white_x
                              , _info._white_y
                              , _info._red_x
                              , _info._red_y
                              , _info._green_x
                              , _info._green_y
                              , _info._blue_x
                              , _info._blue_y
                              );
        }

        if( _info._valid_file_gamma )
        {
            png_set_gAMA_fixed( get_struct()
                              , get_info()
                              , _info._file_gamma
                              );
        }
#endif // BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED

        if( _info._valid_icc_profile )
        {
#if PNG_LIBPNG_VER_MINOR >= 5
            png_set_iCCP( get_struct()
                        , get_info()
                        , const_cast< png_charp >( _info._icc_name.c_str() )
                        , _info._iccp_compression_type
                        , reinterpret_cast< png_const_bytep >( & (_info._profile.front ()) )
                        , _info._profile_length
                        );
#else
            png_set_iCCP( get_struct()
                        , get_info()
                        , const_cast< png_charp >( _info._icc_name.c_str() )
                        , _info._iccp_compression_type
                        , const_cast< png_charp >( & (_info._profile.front()) )
                        , _info._profile_length
                        );
#endif
        }

        if( _info._valid_intent )
        {
            png_set_sRGB( get_struct()
                        , get_info()
                        , _info._intent
                        );
        }

        if( _info._valid_palette )
        {
            png_set_PLTE( get_struct()
                        , get_info()
                        , const_cast< png_colorp >( &_info._palette.front() )
                        , _info._num_palette
                        );
        }

        if( _info._valid_background )
        {
            png_set_bKGD( get_struct()
                        , get_info()
                        , const_cast< png_color_16p >( &_info._background )
                        );
        }

        if( _info._valid_histogram )
        {
            png_set_hIST( get_struct()
                        , get_info()
                        , const_cast< png_uint_16p >( &_info._histogram.front() )
                        );
        }

        if( _info._valid_offset )
        {
            png_set_oFFs( get_struct()
                        , get_info()
                        , _info._offset_x
                        , _info._offset_y
                        , _info._off_unit_type
                        );
        }

        if( _info._valid_pixel_calibration )
        {
            std::vector< const char* > params( _info._num_params );
            for( std::size_t i = 0; i < params.size(); ++i )
            {
                params[i] = _info._params[ i ].c_str();
            }

            png_set_pCAL( get_struct()
                        , get_info()
                        , const_cast< png_charp >( _info._purpose.c_str() )
                        , _info._X0
                        , _info._X1
                        , _info._cal_type
                        , _info._num_params
                        , const_cast< png_charp  >( _info._units.c_str() )
                        , const_cast< png_charpp >( &params.front()     )
                        );
        }

        if( _info._valid_resolution )
        {
            png_set_pHYs( get_struct()
                        , get_info()
                        , _info._res_x
                        , _info._res_y
                        , _info._phy_unit_type
                        );
        }

        if( _info._valid_significant_bits )
        {
            png_set_sBIT( get_struct()
                        , get_info()
                        , const_cast< png_color_8p >( &_info._sig_bits )
                        );
        }

#ifndef BOOST_GIL_IO_PNG_1_4_OR_LOWER

#ifdef BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED
        if( _info._valid_scale_factors )
        {
            png_set_sCAL( get_struct()
                        , get_info()
                        , this->_info._scale_unit
                        , this->_info._scale_width
                        , this->_info._scale_height
                        );
        }
#else
#ifdef BOOST_GIL_IO_PNG_FIXED_POINT_SUPPORTED
        if( _info._valid_scale_factors )
        {
            png_set_sCAL_fixed( get_struct()
                              , get_info()
                              , this->_info._scale_unit
                              , this->_info._scale_width
                              , this->_info._scale_height
                              );
        }
#else
        if( _info._valid_scale_factors )
        {
            png_set_sCAL_s( get_struct()
                          , get_info()
                          , this->_info._scale_unit
                          , const_cast< png_charp >( this->_info._scale_width.c_str()  )
                          , const_cast< png_charp >( this->_info._scale_height.c_str() )
                          );
        }

#endif // BOOST_GIL_IO_PNG_FIXED_POINT_SUPPORTED
#endif // BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED
#endif // BOOST_GIL_IO_PNG_1_4_OR_LOWER

        if( _info._valid_text )
        {
            std::vector< png_text > texts( _info._num_text );
            for( std::size_t i = 0; i < texts.size(); ++i )
            {
                png_text pt;
                pt.compression = _info._text[i]._compression;
                pt.key         = const_cast< png_charp >( this->_info._text[i]._key.c_str()  );
                pt.text        = const_cast< png_charp >( this->_info._text[i]._text.c_str() );
                pt.text_length = _info._text[i]._text.length();

                texts[i] = pt;
            }

            png_set_text( get_struct()
                        , get_info()
                        , &texts.front()
                        , _info._num_text
                        );
        }

        if( _info._valid_modification_time )
        {
            png_set_tIME( get_struct()
                        , get_info()
                        , const_cast< png_timep >( &_info._mod_time )
                        );
        }

        if( _info._valid_transparency_factors )
        {
            int sample_max = ( 1u << _info._bit_depth );

            /* libpng doesn't reject a tRNS chunk with out-of-range samples */
            if( !(  (  _info._color_type == PNG_COLOR_TYPE_GRAY
                    && (int) _info._trans_values[0].gray > sample_max
                    )
                 || (  _info._color_type == PNG_COLOR_TYPE_RGB
                    &&(  (int) _info._trans_values[0].red   > sample_max
                      || (int) _info._trans_values[0].green > sample_max
                      || (int) _info._trans_values[0].blue  > sample_max
                      )
                    )
                 )
              )
            {
                //@todo Fix that once reading transparency values works
/*
                png_set_tRNS( get_struct()
                            , get_info()
                            , trans
                            , num_trans
                            , trans_values
                            );
*/
            }
        }

        // Compression Levels - valid values are [0,9]
        png_set_compression_level( get_struct()
                                 , _info._compression_level
                                 );

        png_set_compression_mem_level( get_struct()
                                     , _info._compression_mem_level
                                     );

        png_set_compression_strategy( get_struct()
                                    , _info._compression_strategy
                                    );

        png_set_compression_window_bits( get_struct()
                                       , _info._compression_window_bits
                                       );

        png_set_compression_method( get_struct()
                                  , _info._compression_method
                                  );

        png_set_compression_buffer_size( get_struct()
                                       , _info._compression_buffer_size
                                       );

#ifdef BOOST_GIL_IO_PNG_DITHERING_SUPPORTED
        // Dithering
        if( _info._set_dithering )
        {
            png_set_dither( get_struct()
                          , &_info._dithering_palette.front()
                          , _info._dithering_num_palette
                          , _info._dithering_maximum_colors
                          , &_info._dithering_histogram.front()
                          , _info._full_dither
                          );
        }
#endif // BOOST_GIL_IO_PNG_DITHERING_SUPPORTED

        // Filter
        if( _info._set_filter )
        {
            png_set_filter( get_struct()
                          , 0
                          , _info._filter
                          );
        }

        // Invert Mono
        if( _info._invert_mono )
        {
            png_set_invert_mono( get_struct() );
        }

        // True Bits
        if( _info._set_true_bits )
        {
            png_set_sBIT( get_struct()
                        , get_info()
                        , &_info._true_bits.front()
                        );
        }

        // sRGB Intent
        if( _info._set_srgb_intent )
        {
            png_set_sRGB( get_struct()
                        , get_info()
                        , _info._srgb_intent
                        );
        }

        // Strip Alpha
        if( _info._strip_alpha )
        {
            png_set_strip_alpha( get_struct() );
        }

        // Swap Alpha
        if( _info._swap_alpha )
        {
            png_set_swap_alpha( get_struct() );
        }


        png_write_info( get_struct()
                      , get_info()
                      );
    }

protected:

    static void write_data( png_structp png_ptr
                          , png_bytep   data
                          , png_size_t  length
                          )
    {
        static_cast< Device* >( png_get_io_ptr( png_ptr ))->write( data
                                                                 , length );
    }

    static void flush( png_structp png_ptr )
    {
        static_cast< Device* >(png_get_io_ptr(png_ptr) )->flush();
    }

private:

    void init_io( png_structp png_ptr )
    {
        png_set_write_fn( png_ptr
                        , static_cast< void* >        ( &this->_io_dev      )
                        , static_cast< png_rw_ptr >   ( &this_t::write_data )
                        , static_cast< png_flush_ptr >( &this_t::flush      )
                        );
    }

public:

    Device _io_dev;

    image_write_info< png_tag > _info;
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
