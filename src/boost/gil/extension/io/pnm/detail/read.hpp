//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_PNM_DETAIL_READ_HPP
#define BOOST_GIL_EXTENSION_IO_PNM_DETAIL_READ_HPP

#include <boost/gil/extension/io/pnm/tags.hpp>
#include <boost/gil/extension/io/pnm/detail/reader_backend.hpp>
#include <boost/gil/extension/io/pnm/detail/is_allowed.hpp>

#include <boost/gil.hpp> // FIXME: Include what you use!
#include <boost/gil/io/base.hpp>
#include <boost/gil/io/bit_operations.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/row_buffer_helper.hpp>
#include <boost/gil/io/typedefs.hpp>

#include <type_traits>
#include <vector>

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

///
/// PNM Reader
///
template< typename Device
        , typename ConversionPolicy
        >
class reader< Device
            , pnm_tag
            , ConversionPolicy
            >
    : public reader_base< pnm_tag
                        , ConversionPolicy
                        >
    , public reader_backend< Device
                           , pnm_tag
                           >
{

private:

    using this_t = reader<Device, pnm_tag, ConversionPolicy>;
    using cc_t = typename ConversionPolicy::color_converter_type;

public:

    using backend_t = reader_backend<Device, pnm_tag>;

    reader( const Device&                         io_dev
          , const image_read_settings< pnm_tag >& settings
          )
    : reader_base< pnm_tag
                 , ConversionPolicy
                 >()
    , backend_t( io_dev
               , settings
               )
    {}

    reader( const Device&                         io_dev
          , const cc_t&                           cc
          , const image_read_settings< pnm_tag >& settings
          )
    : reader_base< pnm_tag
                 , ConversionPolicy
                 >( cc )
    , backend_t( io_dev
               , settings
               )
    {}

    template<typename View>
    void apply( const View& view )
    {
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

        switch( this->_info._type )
        {
            // reading mono text is reading grayscale but with only two values
            case pnm_image_type::mono_asc_t::value:
            case pnm_image_type::gray_asc_t::value:
            {
                this->_scanline_length = this->_info._width;

                read_text_data< gray8_view_t >( view );

                break;
            }

            case pnm_image_type::color_asc_t::value:
            {
                this->_scanline_length = this->_info._width * num_channels< rgb8_view_t >::value;

                read_text_data< rgb8_view_t  >( view );

                break;
            }

            case pnm_image_type::mono_bin_t::value:
            {
                //gray1_image_t
                this->_scanline_length = ( this->_info._width + 7 ) >> 3;

                read_bin_data< gray1_image_t::view_t >( view );

                break;
            }

            case pnm_image_type::gray_bin_t::value:
            {
                // gray8_image_t
                this->_scanline_length = this->_info._width;

                read_bin_data< gray8_view_t >( view );

                break;
            }

            case pnm_image_type::color_bin_t::value:
            {
                // rgb8_image_t
                this->_scanline_length = this->_info._width * num_channels< rgb8_view_t >::value;

                read_bin_data< rgb8_view_t >( view );
                break;
            }
        }
    }

private:

    template< typename View_Src
            , typename View_Dst
            >
    void read_text_data( const View_Dst& dst )
    {
        using y_t = typename View_Dst::y_coord_t;

        byte_vector_t row( this->_scanline_length );

        //Skip scanlines if necessary.
        for( int y = 0; y <  this->_settings._top_left.y; ++y )
        {
            read_text_row< View_Src >( dst, row, y, false );
        }

        for( y_t y = 0; y < dst.height(); ++y )
        {
            read_text_row< View_Src >( dst, row, y, true );
        }
    }

    template< typename View_Src
            , typename View_Dst
            >
    void read_text_row( const View_Dst&              dst
                      , byte_vector_t&               row
                      , typename View_Dst::y_coord_t y
                      , bool                         process
                      )
    {
        View_Src src = interleaved_view( this->_info._width
                                       , 1
                                       , (typename View_Src::value_type*) &row.front()
                                       , this->_scanline_length
                                       );

        for( uint32_t x = 0; x < this->_scanline_length; ++x )
        {
            for( uint32_t k = 0; ; )
            {
                int ch = this->_io_dev.getc_unchecked();

                if( isdigit( ch ))
                {
                    buf[ k++ ] = static_cast< char >( ch );
                }
                else if( k )
                {
                    buf[ k ] = 0;
                    break;
                }
                else if( ch == EOF || !isspace( ch ))
                {
                    return;
                }
            }

            if( process )
            {
                int value = atoi( buf );

                if( this->_info._max_value == 1 )
                {
                    using channel_t = typename channel_type<typename get_pixel_type<View_Dst>::type>::type;

                    // for pnm format 0 is white
                    row[x] = ( value != 0 )
                             ? typename channel_traits< channel_t >::value_type( 0 )
                             : channel_traits< channel_t >::max_value();
                }
                else
                {
                    row[x] = static_cast< byte_t >( value );
                }
            }
        }

        if( process )
        {
            // We are reading a gray1_image like a gray8_image but the two pixel_t
            // aren't compatible. Though, read_and_no_convert::read(...) wont work.
            copy_data< View_Dst
                     , View_Src >( dst
                                 , src
                                 , y
                                 , typename std::is_same< View_Dst
                                                   , gray1_image_t::view_t
                                                   >::type()
                                 );
        }
    }

    template< typename View_Dst
            , typename View_Src
            >
    void copy_data( const View_Dst&              dst
                  , const View_Src&              src
                  , typename View_Dst::y_coord_t y
                  , std::true_type // is gray1_view
                  )
    {
        if(  this->_info._max_value == 1 )
        {
            typename View_Dst::x_iterator it = dst.row_begin( y );

            for( typename View_Dst::x_coord_t x = 0
               ; x < dst.width()
               ; ++x
               )
            {
                it[x] = src[x];
            }
        }
        else
        {
            copy_data(dst, src, y, std::false_type{});
        }
    }

    template< typename View_Dst
            , typename View_Src
            >
    void copy_data( const View_Dst&              view
                  , const View_Src&              src
                  , typename View_Dst::y_coord_t y
                  , std::false_type // is gray1_view
                  )
    {
        typename View_Src::x_iterator beg = src.row_begin( 0 ) + this->_settings._top_left.x;
        typename View_Src::x_iterator end = beg + this->_settings._dim.x;

        this->_cc_policy.read( beg
                             , end
                             , view.row_begin( y )
                             );
    }


    template< typename View_Src
            , typename View_Dst
            >
    void read_bin_data( const View_Dst& view )
    {
        using y_t = typename View_Dst::y_coord_t;
        using is_bit_aligned_t = typename is_bit_aligned<typename View_Src::value_type>::type;

        using rh_t = detail::row_buffer_helper_view<View_Src>;
        rh_t rh( this->_scanline_length, true );

        typename rh_t::iterator_t beg = rh.begin() + this->_settings._top_left.x;
        typename rh_t::iterator_t end = beg + this->_settings._dim.x;

        // For bit_aligned images we need to negate all bytes in the row_buffer
        // to make sure that 0 is black and 255 is white.
        detail::negate_bits
            <
                typename rh_t::buffer_t,
                std::integral_constant<bool, is_bit_aligned_t::value> // TODO: Simplify after MPL removal
            > neg;

        detail::swap_half_bytes
            <
                typename rh_t::buffer_t,
                std::integral_constant<bool, is_bit_aligned_t::value> // TODO: Simplify after MPL removal
            > swhb;

        //Skip scanlines if necessary.
        for( y_t y = 0; y < this->_settings._top_left.y; ++y )
        {
            this->_io_dev.read( reinterpret_cast< byte_t* >( rh.data() )
                        , this->_scanline_length
                        );
        }

        for( y_t y = 0; y < view.height(); ++y )
        {
            this->_io_dev.read( reinterpret_cast< byte_t* >( rh.data() )
                        , this->_scanline_length
                        );

            neg( rh.buffer() );
            swhb( rh.buffer() );

            this->_cc_policy.read( beg
                                 , end
                                 , view.row_begin( y )
                                 );
        }
    }

private:

    char buf[16];

};


namespace detail {

struct pnm_type_format_checker
{
    pnm_type_format_checker( pnm_image_type::type type )
    : _type( type )
    {}

    template< typename Image >
    bool apply()
    {
        using is_supported_t = is_read_supported
            <
                typename get_pixel_type<typename Image::view_t>::type,
                pnm_tag
            >;

        return is_supported_t::_asc_type == _type
            || is_supported_t::_bin_type == _type;
    }

private:

    pnm_image_type::type _type;
};

struct pnm_read_is_supported
{
    template< typename View >
    struct apply : public is_read_supported< typename get_pixel_type< View >::type
                                           , pnm_tag
                                           >
    {};
};

} // namespace detail

///
/// PNM Dynamic Image Reader
///
template< typename Device
        >
class dynamic_image_reader< Device
                          , pnm_tag
                          >
    : public reader< Device
                   , pnm_tag
                   , detail::read_and_no_convert
                   >
{
    using parent_t = reader
        <
            Device,
            pnm_tag,
            detail::read_and_no_convert
        >;

public:

    dynamic_image_reader( const Device&                         io_dev
                        , const image_read_settings< pnm_tag >& settings
                        )
    : parent_t( io_dev
              , settings
              )
    {}

    template< typename ...Images >
    void apply( any_image< Images... >& images )
    {
        detail::pnm_type_format_checker format_checker( this->_info._type );

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

            detail::dynamic_io_fnobj< detail::pnm_read_is_supported
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
