//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_DETAIL_READER_BACKEND_HPP
#define BOOST_GIL_EXTENSION_IO_DETAIL_READER_BACKEND_HPP

#include <boost/gil/extension/io/png/tags.hpp>
#include <boost/gil/extension/io/png/detail/base.hpp>
#include <boost/gil/extension/io/png/detail/supported_types.hpp>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#pragma warning(disable:4611) //interaction between '_setjmp' and C++ object destruction is non-portable
#endif

///
/// PNG Backend
///
template<typename Device >
struct reader_backend< Device
                     , png_tag
                     >
    : public detail::png_struct_info_wrapper
{
public:

    using format_tag_t = png_tag;
    using this_t = reader_backend<Device, png_tag>;

public:

    reader_backend( const Device&                         io_dev
                  , const image_read_settings< png_tag >& settings
                  )
    : _io_dev( io_dev )

    , _settings( settings )
    , _info()
    , _scanline_length( 0 )

    , _number_passes( 0 )
    {
        read_header();

        if( _settings._dim.x == 0 )
        {
            _settings._dim.x = _info._width;
        }

        if( _settings._dim.y == 0 )
        {
            _settings._dim.y = _info._height;
        }
    }

    void read_header()
    {
        using boost::gil::detail::PNG_BYTES_TO_CHECK;

        // check the file's first few bytes
        byte_t buf[PNG_BYTES_TO_CHECK];

        io_error_if( _io_dev.read( buf
                                , PNG_BYTES_TO_CHECK
                                ) != PNG_BYTES_TO_CHECK
                   , "png_check_validity: failed to read image"
                   );

        io_error_if( png_sig_cmp( png_bytep(buf)
                                , png_size_t(0)
                                , PNG_BYTES_TO_CHECK
                                ) != 0
                   , "png_check_validity: invalid png image"
                   );

        // Create and initialize the png_struct with the desired error handler
        // functions.  If you want to use the default stderr and longjump method,
        // you can supply NULL for the last three parameters.  We also supply the
        // the compiler header file version, so that we know if the application
        // was compiled with a compatible version of the library.  REQUIRED
        get()->_struct = png_create_read_struct( PNG_LIBPNG_VER_STRING
                                             , nullptr  // user_error_ptr
                                             , nullptr  // user_error_fn
                                             , nullptr  // user_warning_fn
                                             );

        io_error_if( get()->_struct == nullptr
                   , "png_reader: fail to call png_create_write_struct()"
                   );

        png_uint_32 user_chunk_data[4];
        user_chunk_data[0] = 0;
        user_chunk_data[1] = 0;
        user_chunk_data[2] = 0;
        user_chunk_data[3] = 0;
        png_set_read_user_chunk_fn( get_struct()
                                  , user_chunk_data
                                  , this_t::read_user_chunk_callback
                                  );

        // Allocate/initialize the memory for image information.  REQUIRED.
        get()->_info = png_create_info_struct( get_struct() );

        if( get_info() == nullptr )
        {
            png_destroy_read_struct( &get()->_struct
                                   , nullptr
                                   , nullptr
                                   );

            io_error( "png_reader: fail to call png_create_info_struct()" );
        }

        // Set error handling if you are using the setjmp/longjmp method (this is
        // the normal method of doing things with libpng).  REQUIRED unless you
        // set up your own error handlers in the png_create_read_struct() earlier.
        if( setjmp( png_jmpbuf( get_struct() )))
        {
            //free all of the memory associated with the png_ptr and info_ptr
            png_destroy_read_struct( &get()->_struct
                                   , &get()->_info
                                   , nullptr
                                   );

            io_error( "png is invalid" );
        }

        png_set_read_fn( get_struct()
                       , static_cast< png_voidp >( &this->_io_dev )
                       , this_t::read_data
                       );

        // Set up a callback function that will be
        // called after each row has been read, which you can use to control
        // a progress meter or the like.
        png_set_read_status_fn( get_struct()
                              , this_t::read_row_callback
                              );

        // Set up a callback which implements user defined transformation.
        // @todo
        png_set_read_user_transform_fn( get_struct()
                                      , png_user_transform_ptr( nullptr )
                                      );

        png_set_keep_unknown_chunks( get_struct()
                                   , PNG_HANDLE_CHUNK_ALWAYS
                                   , nullptr
                                   , 0
                                   );


        // Make sure we read the signature.
        // @todo make it an option
        png_set_sig_bytes( get_struct()
                         , PNG_BYTES_TO_CHECK
                         );

        // The call to png_read_info() gives us all of the information from the
        // PNG file before the first IDAT (image data chunk).  REQUIRED
        png_read_info( get_struct()
                     , get_info()
                     );

        ///
        /// Start reading the image information
        ///

        // get PNG_IHDR chunk information from png_info structure
        png_get_IHDR( get_struct()
                    , get_info()
                    , &this->_info._width
                    , &this->_info._height
                    , &this->_info._bit_depth
                    , &this->_info._color_type
                    , &this->_info._interlace_method
                    , &this->_info._compression_method
                    , &this->_info._filter_method
                    );

        // get number of color channels in image
        this->_info._num_channels = png_get_channels( get_struct()
                                              , get_info()
                                              );

#ifdef BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED

        // Get CIE chromacities and referenced white point
        if( this->_settings._read_cie_chromacities )
        {
            this->_info._valid_cie_colors = png_get_cHRM( get_struct()
                                                        , get_info()
                                                        , &this->_info._white_x, &this->_info._white_y
                                                        ,   &this->_info._red_x,   &this->_info._red_y
                                                        , &this->_info._green_x, &this->_info._green_y
                                                        ,  &this->_info._blue_x,  &this->_info._blue_y
                                                        );
        }

        // get the gamma value
        if( this->_settings._read_file_gamma )
        {
            this->_info._valid_file_gamma = png_get_gAMA( get_struct()
                                                        , get_info()
                                                        , &this->_info._file_gamma
                                                        );

            if( this->_info._valid_file_gamma == false )
            {
                this->_info._file_gamma = 1.0;
            }
        }
#else

        // Get CIE chromacities and referenced white point
        if( this->_settings._read_cie_chromacities )
        {
            this->_info._valid_cie_colors = png_get_cHRM_fixed( get_struct()
                                                              , get_info()
                                                              , &this->_info._white_x, &this->_info._white_y
                                                              ,   &this->_info._red_x,   &this->_info._red_y
                                                              , &this->_info._green_x, &this->_info._green_y
                                                              ,  &this->_info._blue_x,  &this->_info._blue_y
                                                              );
        }

        // get the gamma value
        if( this->_settings._read_file_gamma )
        {
            this->_info._valid_file_gamma = png_get_gAMA_fixed( get_struct()
                                                              , get_info()
                                                              , &this->_info._file_gamma
                                                              );

            if( this->_info._valid_file_gamma == false )
            {
                this->_info._file_gamma = 1;
            }
        }
#endif // BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED

        // get the embedded ICC profile data
        if( this->_settings._read_icc_profile )
        {
#if PNG_LIBPNG_VER_MINOR >= 5
            png_charp icc_name = png_charp( nullptr );
            png_bytep profile  = png_bytep( nullptr );

            this->_info._valid_icc_profile = png_get_iCCP( get_struct()
                                                         , get_info()
                                                         , &icc_name
                                                         , &this->_info._iccp_compression_type
                                                         , &profile
                                                         , &this->_info._profile_length
                                                         );
#else
            png_charp icc_name = png_charp( NULL );
            png_charp profile  = png_charp( NULL );

            this->_info._valid_icc_profile = png_get_iCCP( get_struct()
                                                         , get_info()
                                                         , &icc_name
                                                         , &this->_info._iccp_compression_type
                                                         , &profile
                                                         , &this->_info._profile_length
                                                         );
#endif
            if( icc_name )
            {
                this->_info._icc_name.append( icc_name
                                            , std::strlen( icc_name )
                                            );
            }

            if( this->_info._profile_length != 0 )
            {
                std:: copy_n (profile, this->_info._profile_length, std:: back_inserter (this->_info._profile));
            }
        }

        // get the rendering intent
        if( this->_settings._read_intent )
        {
            this->_info._valid_intent = png_get_sRGB( get_struct()
                                                    , get_info()
                                                    , &this->_info._intent
                                                    );
        }

        // get image palette information from png_info structure
        if( this->_settings._read_palette )
        {
            png_colorp palette = png_colorp( nullptr );

            this->_info._valid_palette = png_get_PLTE( get_struct()
                                                     , get_info()
                                                     , &palette
                                                     , &this->_info._num_palette
                                                     );

            if( this->_info._num_palette > 0 )
            {
                this->_info._palette.resize( this->_info._num_palette );
                std::copy( palette
                         , palette + this->_info._num_palette
                         , &this->_info._palette.front()
                         );
            }
        }

        // get background color
        if( this->_settings._read_background )
        {
            png_color_16p background = png_color_16p( nullptr );

            this->_info._valid_background = png_get_bKGD( get_struct()
                                                        , get_info()
                                                        , &background
                                                        );
            if( background )
            {
                this->_info._background = *background;
            }
        }

        // get the histogram
        if( this->_settings._read_histogram )
        {
            png_uint_16p histogram = png_uint_16p( nullptr );

            this->_info._valid_histogram = png_get_hIST( get_struct()
                                                       , get_info()
                                                       , &histogram
                                                       );

            if( histogram )
            {
                // the number of values is set by the number of colors inside
                // the palette.
                if( this->_settings._read_palette == false )
                {
                    png_colorp palette = png_colorp( nullptr );
                    png_get_PLTE( get_struct()
                                , get_info()
                                , &palette
                                , &this->_info._num_palette
                                );
                }

                std::copy( histogram
                         , histogram + this->_info._num_palette
                         , &this->_info._histogram.front()
                         );
            }
        }

        // get screen offsets for the given image
        if( this->_settings._read_screen_offsets )
        {
            this->_info._valid_offset = png_get_oFFs( get_struct()
                                                    , get_info()
                                                    , &this->_info._offset_x
                                                    , &this->_info._offset_y
                                                    , &this->_info._off_unit_type
                                                    );
        }


        // get pixel calibration settings
        if( this->_settings._read_pixel_calibration )
        {
            png_charp purpose = png_charp ( nullptr );
            png_charp units   = png_charp ( nullptr );
            png_charpp params = png_charpp( nullptr );

            this->_info._valid_pixel_calibration = png_get_pCAL( get_struct()
                                                               , get_info()
                                                               , &purpose
                                                               , &this->_info._X0
                                                               , &this->_info._X1
                                                               , &this->_info._cal_type
                                                               , &this->_info._num_params
                                                               , &units
                                                               , &params
                                                               );
            if( purpose )
            {
                this->_info._purpose.append( purpose
                                           , std::strlen( purpose )
                                           );
            }

            if( units )
            {
                this->_info._units.append( units
                                         , std::strlen( units )
                                         );
            }

            if( this->_info._num_params > 0 )
            {
                this->_info._params.resize( this->_info._num_params );

                for( png_CAL_nparam::type i = 0
                   ; i < this->_info._num_params
                   ; ++i
                   )
                {
                    this->_info._params[i].append( params[i]
                                                 , std::strlen( params[i] )
                                                 );
                }
            }
        }

        // get the physical resolution
        if( this->_settings._read_physical_resolution )
        {
            this->_info._valid_resolution = png_get_pHYs( get_struct()
                                                        , get_info()
                                                        , &this->_info._res_x
                                                        , &this->_info._res_y
                                                        , &this->_info._phy_unit_type
                                                        );
        }

        // get the image resolution in pixels per meter.
        if( this->_settings._read_pixels_per_meter )
        {
            this->_info._pixels_per_meter = png_get_pixels_per_meter( get_struct()
                                                                    , get_info()
                                                                    );
        }


        // get number of significant bits for each color channel
        if( this->_settings._read_number_of_significant_bits )
        {
            png_color_8p sig_bits = png_color_8p( nullptr );

            this->_info._valid_significant_bits = png_get_sBIT( get_struct()
                                                              , get_info()
                                                              , &sig_bits
                                                              );

            // @todo Is there one or more colors?
            if( sig_bits )
            {
                this->_info._sig_bits = *sig_bits;
            }
        }

#ifndef BOOST_GIL_IO_PNG_1_4_OR_LOWER

#ifdef BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED

        // get physical scale settings
        if( this->_settings._read_scale_factors )
        {
            this->_info._valid_scale_factors = png_get_sCAL( get_struct()
                                                           , get_info()
                                                           , &this->_info._scale_unit
                                                           , &this->_info._scale_width
                                                           , &this->_info._scale_height
                                                           );
        }
#else
#ifdef BOOST_GIL_IO_PNG_FIXED_POINT_SUPPORTED
        if( this->_settings._read_scale_factors )
        {
            this->_info._valid_scale_factors = png_get_sCAL_fixed( get_struct()
                                                                 , get_info()
                                                                 , &this->_info._scale_unit
                                                                 , &this->_info._scale_width
                                                                 , &this->_info._scale_height
                                                                 );
        }
#else
        if( this->_settings._read_scale_factors )
        {
            png_charp scale_width  = nullptr;
            png_charp scale_height = nullptr;

            this->_info._valid_scale_factors = png_get_sCAL_s(
                get_struct(), get_info(), &this->_info._scale_unit, &scale_width, &scale_height);

            if (this->_info._valid_scale_factors)
            {
                if( scale_width )
                {
                    this->_info._scale_width.append( scale_width
                                                   , std::strlen( scale_width )
                                                   );
                }

                if( scale_height )
                {
                    this->_info._scale_height.append( scale_height
                                                    , std::strlen( scale_height )
                                                    );
                }
            }
        }
#endif // BOOST_GIL_IO_PNG_FIXED_POINT_SUPPORTED
#endif // BOOST_GIL_IO_PNG_FLOATING_POINT_SUPPORTED
#endif // BOOST_GIL_IO_PNG_1_4_OR_LOWER

        // get comments information from png_info structure
        if( this->_settings._read_comments )
        {
            png_textp text = png_textp( nullptr );

            this->_info._valid_text = png_get_text( get_struct()
                                                  , get_info()
                                                  , &text
                                                  , &this->_info._num_text
                                                  );

            if( this->_info._num_text > 0 )
            {
                this->_info._text.resize( this->_info._num_text );

                for( png_num_text::type i = 0
                   ; i < this->_info._num_text
                   ; ++i
                   )
                {
                    this->_info._text[i]._compression = text[i].compression;
                    this->_info._text[i]._key.append( text[i].key
                                                    , std::strlen( text[i].key )
                                                    );

                    this->_info._text[i]._text.append( text[i].text
                                                     , std::strlen( text[i].text )
                                                     );
                }
            }
        }

        // get last modification time
        if( this->_settings._read_last_modification_time )
        {
            png_timep mod_time = png_timep( nullptr );
            this->_info._valid_modification_time = png_get_tIME( get_struct()
                                                               , get_info()
                                                               , &mod_time
                                                               );
            if( mod_time )
            {
                this->_info._mod_time = *mod_time;
            }
        }

        // get transparency data
        if( this->_settings._read_transparency_data )
        {
            png_bytep     trans        = png_bytep    ( nullptr );
            png_color_16p trans_values = png_color_16p( nullptr );

            this->_info._valid_transparency_factors = png_get_tRNS( get_struct()
                                                                  , get_info()
                                                                  , &trans
                                                                  , &this->_info._num_trans
                                                                  , &trans_values
                                                                  );

            if( trans )
            {
                //@todo What to do, here? How do I know the length of the "trans" array?
            }

            if( this->_info._num_trans )
            {
                this->_info._trans_values.resize( this->_info._num_trans );
                std::copy( trans_values
                         , trans_values + this->_info._num_trans
                         , &this->_info._trans_values.front()
                         );
            }
        }

        // @todo One day!
/*
        if( false )
        {
            png_unknown_chunkp unknowns = png_unknown_chunkp( NULL );
            int num_unknowns = static_cast< int >( png_get_unknown_chunks( get_struct()
                                                                         , get_info()
                                                                         , &unknowns
                                                                         )
                                                 );
        }
*/
    }

    /// Check if image is large enough.
    void check_image_size( const point_t& img_dim )
    {
        if( _settings._dim.x > 0 )
        {
            if( img_dim.x < _settings._dim.x ) { io_error( "Supplied image is too small" ); }
        }
        else
        {
            if( img_dim.x < _info._width ) { io_error( "Supplied image is too small" ); }
        }


        if( _settings._dim.y > 0 )
        {
            if( img_dim.y < _settings._dim.y ) { io_error( "Supplied image is too small" ); }
        }
        else
        {
            if( img_dim.y < _info._height ) { io_error( "Supplied image is too small" ); }
        }
    }

protected:

    static void read_data( png_structp png_ptr
                         , png_bytep   data
                         , png_size_t length
                         )
    {
        static_cast<Device*>(png_get_io_ptr(png_ptr) )->read( data
                                                            , length );
    }

    static void flush( png_structp png_ptr )
    {
        static_cast<Device*>(png_get_io_ptr(png_ptr) )->flush();
    }


    static int read_user_chunk_callback( png_struct*        /* png_ptr */
                                       , png_unknown_chunkp /* chunk */
                                       )
    {
        // @todo
        return 0;
    }

    static void read_row_callback( png_structp /* png_ptr    */
                                 , png_uint_32 /* row_number */
                                 , int         /* pass       */
                                 )
    {
        // @todo
    }

public:

    Device _io_dev;

    image_read_settings< png_tag > _settings;
    image_read_info    < png_tag > _info;

    std::size_t _scanline_length;

    std::size_t _number_passes;
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
