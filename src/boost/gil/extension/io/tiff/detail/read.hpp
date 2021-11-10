//
// Copyright 2007-2012 Christian Henning, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_READER_HPP
#define BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_READER_HPP

#include <boost/gil/extension/io/tiff/detail/device.hpp>
#include <boost/gil/extension/io/tiff/detail/is_allowed.hpp>
#include <boost/gil/extension/io/tiff/detail/reader_backend.hpp>

#include <boost/gil/io/base.hpp>
#include <boost/gil/io/bit_operations.hpp>
#include <boost/gil/io/conversion_policies.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>
#include <boost/gil/io/reader_base.hpp>
#include <boost/gil/io/row_buffer_helper.hpp>

#include <boost/assert.hpp>

#include <algorithm>
#include <string>
#include <type_traits>
#include <vector>

// taken from jpegxx - https://bitbucket.org/edd/jpegxx/src/ea2492a1a4a6/src/ijg_headers.hpp
#ifndef BOOST_GIL_EXTENSION_IO_TIFF_C_LIB_COMPILED_AS_CPLUSPLUS
    extern "C" {
#endif

#include <tiff.h>
#include <tiffio.h>

#ifndef BOOST_GIL_EXTENSION_IO_TIFF_C_LIB_COMPILED_AS_CPLUSPLUS
    }
#endif

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

template < int K >
struct plane_recursion
{
   template< typename View
           , typename Device
           , typename ConversionPolicy
           >
   static
   void read_plane( const View& dst_view
                  , reader< Device
                          , tiff_tag
                          , ConversionPolicy >* p
                  )
   {
        using plane_t = typename kth_channel_view_type<K, View>::type;
        plane_t plane = kth_channel_view<K>( dst_view );

        p->template read_data< detail::row_buffer_helper_view< plane_t > >( plane, K );

        plane_recursion< K - 1 >::read_plane( dst_view, p );
   }
};

template <>
struct plane_recursion< -1 >
{
   template< typename View
           , typename Device
           , typename ConversionPolicy
           >
   static
   void read_plane( const View&               /* dst_view */
                  , reader< Device
                          , tiff_tag
                          , ConversionPolicy
                          >*                  /* p         */
                  )
    {}
};

///
/// Tiff Reader
///
template< typename Device
        , typename ConversionPolicy
        >
class reader< Device
            , tiff_tag
            , ConversionPolicy
            >
    : public reader_base< tiff_tag
                        , ConversionPolicy >

    , public reader_backend< Device
                           , tiff_tag
                           >
{
private:

    using this_t = reader<Device, tiff_tag, ConversionPolicy>;
    using cc_t = typename ConversionPolicy::color_converter_type;

public:

    using backend_t = reader_backend<Device, tiff_tag>;

    reader( const Device&                          io_dev
          , const image_read_settings< tiff_tag >& settings
          )
    : reader_base< tiff_tag
                 , ConversionPolicy
                 >()
    , backend_t( io_dev
               , settings
               )
    {}

    reader( const Device&                                          io_dev
          , const typename ConversionPolicy::color_converter_type& cc
          , const image_read_settings< tiff_tag >&                 settings
          )
    : reader_base< tiff_tag
                 , ConversionPolicy
                 >( cc )
    , backend_t( io_dev
               , settings
               )
    {}

    // only works for homogeneous image types
    template< typename View >
    void apply( View& dst_view )
    {
        if( this->_info._photometric_interpretation == PHOTOMETRIC_PALETTE )
        {
            this->_scanline_length = this->_info._width
                                   * num_channels< rgb16_view_t >::value
                                   * sizeof( channel_type<rgb16_view_t>::type );

            // Steps:
            // 1. Read indices. It's an array of grayX_pixel_t.
            // 2. Read palette. It's an array of rgb16_pixel_t.
            // 3. ??? Create virtual image or transform the two arrays
            //    into a rgb16_image_t object. The latter might
            //    be a good first solution.

            switch( this->_info._bits_per_sample )
            {
                case 1:  { read_palette_image< gray1_image_t  >( dst_view ); break; }
                case 2:  { read_palette_image< gray2_image_t  >( dst_view ); break; }
                case 4:  { read_palette_image< gray4_image_t  >( dst_view ); break; }
                case 8:  { read_palette_image< gray8_image_t  >( dst_view ); break; }
                case 16: { read_palette_image< gray16_image_t >( dst_view ); break; }

                default: { io_error( "Not supported palette " ); }
            }

            return;

        }
        else
        {
            this->_scanline_length = this->_io_dev.get_scanline_size();

            // In case we only read the image the user's type and
            // the tiff type need to compatible. Which means:
            // color_spaces_are_compatible && channels_are_pairwise_compatible

            using is_read_only = typename std::is_same
                <
                    ConversionPolicy,
                    detail::read_and_no_convert
                >::type;

            io_error_if( !detail::is_allowed< View >( this->_info
                                                    , is_read_only()
                                                    )
                       , "Image types aren't compatible."
                       );

            if( this->_info._planar_configuration == PLANARCONFIG_SEPARATE )
            {
                plane_recursion< num_channels< View >::value - 1 >::read_plane( dst_view
                                                                              , this
                                                                              );
            }
            else if( this->_info._planar_configuration == PLANARCONFIG_CONTIG )
            {
                read( dst_view
                    , typename is_read_only::type()
                    );
            }
            else
            {
                io_error( "Wrong planar configuration setting." );
            }
        }
    }

private:

    template< typename View >
    void read( View v
             , std::true_type // is_read_only
             )
    {
        read_data< detail::row_buffer_helper_view< View > >( v, 0 );
    }

    template< typename View >
    void read( View v
             , std::false_type  // is_read_only
             )
    {
        // the read_data function needs to know what gil type the source image is
        // to have the default color converter function correctly

        switch( this->_info._photometric_interpretation )
        {
            case PHOTOMETRIC_MINISWHITE:
            case PHOTOMETRIC_MINISBLACK:
            {
                switch( this->_info._bits_per_sample )
                {
                    case  1: { read_data< detail::row_buffer_helper_view< gray1_image_t::view_t > >( v, 0 );  break; }
                    case  2: { read_data< detail::row_buffer_helper_view< gray2_image_t::view_t > >( v, 0 );  break; }
                    case  4: { read_data< detail::row_buffer_helper_view< gray4_image_t::view_t > >( v, 0 );  break; }
                    case  8: { read_data< detail::row_buffer_helper_view< gray8_view_t  > >( v, 0 );  break; }
                    case 16: { read_data< detail::row_buffer_helper_view< gray16_view_t > >( v, 0 );  break; }
                    case 32: { read_data< detail::row_buffer_helper_view< gray32_view_t > >( v, 0 );  break; }
                    default: { io_error( "Image type is not supported." ); }
                }

                break;
            }

            case PHOTOMETRIC_RGB:
            {
                switch( this->_info._samples_per_pixel )
                {
                    case 3:
                    {
                        switch( this->_info._bits_per_sample )
                        {
                            case  8: { read_data< detail::row_buffer_helper_view< rgb8_view_t  > >( v, 0 );  break; }
                            case 16: { read_data< detail::row_buffer_helper_view< rgb16_view_t > >( v, 0 );  break; }
                            case 32: { read_data< detail::row_buffer_helper_view< rgb32_view_t > >( v, 0 );  break; }
                            default: { io_error( "Image type is not supported." ); }
                        }

                        break;
                    }

                    case 4:
                    {
                        switch( this->_info._bits_per_sample )
                        {
                            case  8: { read_data< detail::row_buffer_helper_view< rgba8_view_t  > >( v, 0 );  break; }
                            case 16: { read_data< detail::row_buffer_helper_view< rgba16_view_t > >( v, 0 );  break; }
                            case 32: { read_data< detail::row_buffer_helper_view< rgba32_view_t > >( v, 0 );  break; }
                            default: { io_error( "Image type is not supported." ); }
                        }

                        break;
                    }

                    default: { io_error( "Image type is not supported." ); }
                }

                break;
            }
            case PHOTOMETRIC_SEPARATED: // CYMK
            {
                switch( this->_info._bits_per_sample )
                {
                    case  8: { read_data< detail::row_buffer_helper_view< cmyk8_view_t  > >( v, 0 );  break; }
                    case 16: { read_data< detail::row_buffer_helper_view< cmyk16_view_t > >( v, 0 );  break; }
                    case 32: { read_data< detail::row_buffer_helper_view< cmyk32_view_t > >( v, 0 );  break; }
                    default: { io_error( "Image type is not supported." ); }
                }

                break;
            }

            default: { io_error( "Image type is not supported." ); }
        }
    }

   template< typename PaletteImage
           , typename View
           >
   void read_palette_image( const View& dst_view )
   {
      PaletteImage indices( this->_info._width  - this->_settings._top_left.x
                          , this->_info._height - this->_settings._top_left.y );

      // read the palette first
      read_data< detail::row_buffer_helper_view
          <
            typename PaletteImage::view_t>
        >(view(indices), 0);

      read_palette_image(dst_view, view(indices),
          typename std::is_same<View, rgb16_view_t>::type());
   }

   template< typename View
           , typename Indices_View
           >
   void read_palette_image( const View&         dst_view
                          , const Indices_View& indices_view
                          , std::true_type   // is View rgb16_view_t
                          )
   {
      tiff_color_map::red_t   red   = nullptr;
      tiff_color_map::green_t green = nullptr;
      tiff_color_map::blue_t  blue  = nullptr;

      this->_io_dev.get_field_defaulted( red, green, blue );

      using channel_t = typename channel_traits<typename element_type<typename Indices_View::value_type>::type>::value_type;

      int num_colors = channel_traits< channel_t >::max_value();

      rgb16_planar_view_t palette = planar_rgb_view( num_colors
                                                   , 1
                                                   , red
                                                   , green
                                                   , blue
                                                   , sizeof(uint16_t) * num_colors );

      for( typename rgb16_view_t::y_coord_t y = 0; y < dst_view.height(); ++y )
      {
         typename rgb16_view_t::x_iterator it  = dst_view.row_begin( y );
         typename rgb16_view_t::x_iterator end = dst_view.row_end( y );

         typename Indices_View::x_iterator indices_it = indices_view.row_begin( y );

         for( ; it != end; ++it, ++indices_it )
         {
            uint16_t i = gil::at_c<0>( *indices_it );

            *it = palette[i];
         }
      }
   }

   template< typename View
           , typename Indices_View
           >
   inline
   void read_palette_image( const View&         /* dst_view     */
                          , const Indices_View& /* indices_view */
                          , std::false_type  // is View rgb16_view_t
                          )
   {
      io_error( "User supplied image type must be rgb16_image_t." );
   }

   template< typename Buffer >
   void skip_over_rows( Buffer& buffer
                      , int     plane
                      )
   {
      if( this->_info._compression != COMPRESSION_NONE )
      {
         // Skipping over rows is not possible for compressed images(  no random access ). See man
         // page ( diagnostics section ) for more information.
         for( std::ptrdiff_t row = 0; row < this->_settings._top_left.y; ++row )
         {
            this->_io_dev.read_scanline( buffer
                                 , row
                                 , static_cast< tsample_t >( plane ));
         }
      }
   }

   template< typename Buffer
           , typename View
           >
   void read_data( const View& dst_view
                 , int         /* plane */ )
    {
        if( this->_io_dev.is_tiled() )
        {
            read_tiled_data< Buffer >( dst_view, 0 );
        }
        else
        {
            read_stripped_data< Buffer >( dst_view, 0 );
        }
    }


   template< typename Buffer
           , typename View
           >
   void read_tiled_data( const View& dst_view
                       , int         plane
                       )
   {
      if(  dst_view.width()  != this->_info._width
        || dst_view.height() != this->_info._height
        )
      {
          // read a subimage
          read_tiled_data_subimage< Buffer >( dst_view, plane );
      }
      else
      {
          // read full image
          read_tiled_data_full< Buffer >( dst_view, plane );
      }
   }

   template< typename Buffer
           , typename View
           >
   void read_tiled_data_subimage( const View& dst_view
                                , int         plane
                                )
   {
       ///@todo: why is
       /// using row_buffer_helper_t = Buffer;
       /// not working? I get compiler error with MSVC10.
       /// read_stripped_data IS working.
       using row_buffer_helper_t = detail::row_buffer_helper_view<View>;

       using it_t = typename row_buffer_helper_t::iterator_t;

       tiff_image_width::type  image_width  = this->_info._width;
       tiff_image_height::type image_height = this->_info._height;

       tiff_tile_width::type  tile_width  = this->_info._tile_width;
       tiff_tile_length::type tile_height = this->_info._tile_length;

       std::ptrdiff_t subimage_x = this->_settings._top_left.x;
       std::ptrdiff_t subimage_y = this->_settings._top_left.y;

       std::ptrdiff_t subimage_width  = this->_settings._dim.x;
       std::ptrdiff_t subimage_height = this->_settings._dim.y;

       row_buffer_helper_t row_buffer_helper(this->_io_dev.get_tile_size(), true );

       for( unsigned int y = 0; y < image_height; y += tile_height )
       {
           for( unsigned int x = 0; x < image_width; x += tile_width )
           {
               uint32_t current_tile_width  = ( x + tile_width  <  image_width ) ? tile_width  : image_width  - x;
               uint32_t current_tile_length = ( y + tile_height < image_height ) ? tile_height : image_height - y;

               this->_io_dev.read_tile( row_buffer_helper.buffer()
                                , x
                                , y
                                , 0
                                , static_cast< tsample_t >( plane )
                                );

               // these are all whole image coordinates
               point_t tile_top_left   ( x, y );
               point_t tile_lower_right( x + current_tile_width - 1, y + current_tile_length - 1 );

               point_t view_top_left   ( subimage_x, subimage_y );
               point_t view_lower_right( subimage_x + subimage_width  - 1
                                       , subimage_y + subimage_height - 1 );

               if(  tile_top_left.x    > view_lower_right.x
                 || tile_top_left.y    > view_lower_right.y
                 || tile_lower_right.x < view_top_left.x
                 || tile_lower_right.y < view_top_left.y
                 )
               {
                   // current tile and dst_view do not overlap
                   continue;
               }
               else
               {
                   // dst_view is overlapping the current tile

                   // next is to define the portion in the tile that needs to be copied

                   // get the whole image coordinates
                   std::ptrdiff_t img_x0 = ( tile_top_left.x >= view_top_left.x ) ? tile_top_left.x : view_top_left.x;
                   std::ptrdiff_t img_y0 = ( tile_top_left.y >= view_top_left.y ) ? tile_top_left.y : view_top_left.y;

                   std::ptrdiff_t img_x1 = ( tile_lower_right.x <= view_lower_right.x ) ? tile_lower_right.x : view_lower_right.x;
                   std::ptrdiff_t img_y1 = ( tile_lower_right.y <= view_lower_right.y ) ? tile_lower_right.y : view_lower_right.y;

                   // convert to tile coordinates
                   std::ptrdiff_t tile_x0 = img_x0 - x;
                   std::ptrdiff_t tile_y0 = img_y0 - y;
                   std::ptrdiff_t tile_x1 = img_x1 - x;
                   std::ptrdiff_t tile_y1 = img_y1 - y;

                   BOOST_ASSERT(tile_x0 >= 0 && tile_y0 >= 0 && tile_x1 >= 0 && tile_y1 >= 0);
                   BOOST_ASSERT(tile_x0 <= img_x1 && tile_y0 <= img_y1);
                   BOOST_ASSERT(tile_x0 < tile_width && tile_y0 < tile_height && tile_x1 < tile_width && tile_y1 < tile_height);

                   std::ptrdiff_t tile_subimage_view_width  = tile_x1 - tile_x0 + 1;
                   std::ptrdiff_t tile_subimage_view_height = tile_y1 - tile_y0 + 1;

                   // convert to dst_view coordinates
                   std::ptrdiff_t dst_x0 = img_x0 - subimage_x;
                   std::ptrdiff_t dst_y0 = img_y0 - subimage_y;
                   BOOST_ASSERT(dst_x0 >= 0 && dst_y0 >= 0);

                   View dst_subimage_view = subimage_view( dst_view
                                                         , (int) dst_x0
                                                         , (int) dst_y0
                                                         , (int) tile_subimage_view_width
                                                         , (int) tile_subimage_view_height
                                                         );

                   // the row_buffer is a 1D array which represents a 2D image. We cannot
                   // use interleaved_view here, since row_buffer could be bit_aligned.
                   // Interleaved_view's fourth parameter "rowsize_in_bytes" doesn't work
                   // for bit_aligned pixels.

                   for( std::ptrdiff_t dst_row = 0; dst_row < dst_subimage_view.height(); ++dst_row )
                   {
                       std::ptrdiff_t tile_row = dst_row + tile_y0;

                       // jump to the beginning of the current tile row
                       it_t begin = row_buffer_helper.begin() + tile_row * tile_width;

                       begin    += tile_x0;
                       it_t end  = begin + dst_subimage_view.width();

                       this->_cc_policy.read( begin
                                            , end
                                            , dst_subimage_view.row_begin( dst_row )
                                            );
                    } //for
               }
           } // for
       } // for
   }

   template< typename Buffer
           , typename View
           >
   void read_tiled_data_full( const View& dst_view
                            , int         plane
                            )
   {
       ///@todo: why is
       /// using row_buffer_helper_t = Buffer;
       /// not working? I get compiler error with MSVC10.
       /// read_stripped_data IS working.
       using row_buffer_helper_t = detail::row_buffer_helper_view<View>;

       using it_t = typename row_buffer_helper_t::iterator_t;

       tiff_image_width::type  image_width  = this->_info._width;
       tiff_image_height::type image_height = this->_info._height;

       tiff_tile_width::type  tile_width  = this->_info._tile_width;
       tiff_tile_length::type tile_height = this->_info._tile_length;

       row_buffer_helper_t row_buffer_helper(this->_io_dev.get_tile_size(), true );

       for( unsigned int y = 0; y < image_height; y += tile_height )
       {
           for( unsigned int x = 0; x < image_width; x += tile_width )
           {
               uint32_t current_tile_width  = ( x + tile_width  <  image_width ) ? tile_width  : image_width  - x;
               uint32_t current_tile_length = ( y + tile_height < image_height ) ? tile_height : image_height - y;

               this->_io_dev.read_tile( row_buffer_helper.buffer()
                                , x
                                , y
                                , 0
                                , static_cast< tsample_t >( plane )
                                );

               View dst_subimage_view = subimage_view( dst_view
                                                     , x
                                                     , y
                                                     , current_tile_width
                                                     , current_tile_length
                                                     );

               // the row_buffer is a 1D array which represents a 2D image. We cannot
               // use interleaved_view here, since row_buffer could be bit_aligned.
               // Interleaved_view's fourth parameter "rowsize_in_bytes" doesn't work
               // for bit_aligned pixels.

               for( int row = 0; row < dst_subimage_view.height(); ++row )
               {
                   it_t begin = row_buffer_helper.begin() + row * tile_width;
                   it_t end   = begin + dst_subimage_view.width();

                   this->_cc_policy.read( begin
                                        , end
                                        , dst_subimage_view.row_begin( row )
                                        );
                } //for
           } // for
       } // for
   }

   template< typename Buffer
           , typename View
           >
   void read_stripped_data( const View& dst_view
                          , int         plane     )
   {
      using is_view_bit_aligned_t = typename is_bit_aligned<typename View::value_type>::type;

      //using row_buffer_helper_t =detail::row_buffer_helper_view<View>;
      using row_buffer_helper_t = Buffer;
      using it_t = typename row_buffer_helper_t::iterator_t;

      std::size_t size_to_allocate = buffer_size< typename View::value_type >( dst_view.width()
                                                                             , is_view_bit_aligned_t() );
      row_buffer_helper_t row_buffer_helper( size_to_allocate, true );

      it_t begin = row_buffer_helper.begin();

      it_t first = begin + this->_settings._top_left.x;
      it_t last  = first + this->_settings._dim.x; // one after last element

      // I don't think tiff allows for random access of row, that's why we need
      // to read and discard rows when reading subimages.
      skip_over_rows( row_buffer_helper.buffer()
                    , plane
                    );

      std::ptrdiff_t row     = this->_settings._top_left.y;
      std::ptrdiff_t row_end = row + this->_settings._dim.y;
      std::ptrdiff_t dst_row = 0;

      for(
         ; row < row_end
         ; ++row, ++dst_row
         )
      {
         this->_io_dev.read_scanline( row_buffer_helper.buffer()
                              , row
                              , static_cast< tsample_t >( plane )
                              );

         this->_cc_policy.read( first
                              , last
                              , dst_view.row_begin( dst_row ));
      }
   }

    template< typename Pixel >
    std::size_t buffer_size( std::size_t width
                           , std::false_type // is_bit_aligned
                           )
    {
        std::size_t scanline_size_in_bytes = this->_io_dev.get_scanline_size();

        std::size_t element_size = sizeof( Pixel );

        std::size_t ret = std::max( width
                                  , (( scanline_size_in_bytes + element_size - 1 ) / element_size )
                                  );

        return ret;
    }

    template< typename Pixel >
    std::size_t buffer_size( std::size_t /* width */
                            , std::true_type  // is_bit_aligned
                            )
    {
        return this->_io_dev.get_scanline_size();
    }

private:

   template < int K > friend struct plane_recursion;
};

namespace detail {

struct tiff_type_format_checker
{
    tiff_type_format_checker( const image_read_info< tiff_tag >& info )
    : _info( info )
    {}

    template< typename Image >
    bool apply()
    {
        using view_t = typename Image::view_t;

        return is_allowed< view_t >( _info
                                   , std::true_type()
                                   );
    }

private:
    tiff_type_format_checker& operator=( const tiff_type_format_checker& ) { return *this; }

private:

    const image_read_info< tiff_tag > _info;
};

struct tiff_read_is_supported
{
    template< typename View >
    struct apply : public is_read_supported< typename get_pixel_type< View >::type
                                           , tiff_tag
                                           >
    {};
};

} // namespace detail


///
/// Tiff Dynamic Image Reader
///
template< typename Device >
class dynamic_image_reader< Device
                          , tiff_tag
                          >
    : public reader< Device
                   , tiff_tag
                   , detail::read_and_no_convert
                   >
{
    using parent_t = reader<Device, tiff_tag, detail::read_and_no_convert>;

public:

    dynamic_image_reader( const Device&                          io_dev
                        , const image_read_settings< tiff_tag >& settings
                        )
    : parent_t( io_dev
              , settings
              )
    {}

    template< typename ...Images >
    void apply( any_image< Images... >& images )
    {
        detail::tiff_type_format_checker format_checker( this->_info );

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

            detail::dynamic_io_fnobj< detail::tiff_read_is_supported
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

} // namespace gil
} // namespace boost

#endif
