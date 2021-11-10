//
// Copyright 2007-2012 Christian Henning, Lubomir Bourdev
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_WRITE_HPP
#define BOOST_GIL_EXTENSION_IO_TIFF_DETAIL_WRITE_HPP

#include <boost/gil/extension/io/tiff/tags.hpp>
#include <boost/gil/extension/io/tiff/detail/writer_backend.hpp>
#include <boost/gil/extension/io/tiff/detail/device.hpp>

#include <boost/gil/premultiply.hpp>
#include <boost/gil/io/base.hpp>
#include <boost/gil/io/device.hpp>
#include <boost/gil/io/dynamic_io_new.hpp>

#include <algorithm>
#include <string>
#include <type_traits>
#include <vector>

extern "C" {
#include "tiff.h"
#include "tiffio.h"
}

namespace boost { namespace gil {

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(push)
#pragma warning(disable:4512) //assignment operator could not be generated
#endif

namespace detail {

template <typename PixelReference>
struct my_interleaved_pixel_iterator_type_from_pixel_reference
{
private:
    using pixel_t = typename std::remove_reference<PixelReference>::type::value_type;

public:
    using type = typename iterator_type_from_pixel
        <
            pixel_t,
            false,
            false,
            true
        >::type;
};


template< typename Channel
        , typename Layout
        , bool Mutable
        >
struct my_interleaved_pixel_iterator_type_from_pixel_reference< const bit_aligned_pixel_reference< byte_t
                                                                                                 , Channel
                                                                                                 , Layout
                                                                                                 , Mutable
                                                                                                 >
                                                              >
    : public iterator_type_from_pixel< const bit_aligned_pixel_reference< uint8_t
                                                                        , Channel
                                                                        , Layout
                                                                        , Mutable
                                                                        >
                                     ,false
                                     ,false
                                     ,true
                                     > {};

struct tiff_write_is_supported
{
    template< typename View >
    struct apply
        : public is_write_supported< typename get_pixel_type< View >::type
                                   , tiff_tag
                                   >
    {};
};

} // namespace detail

///
/// TIFF Writer
///
template < typename Device, typename Log >
class writer< Device
            , tiff_tag
            , Log
            >
    : public writer_backend< Device
                           , tiff_tag
                           >
{
private:
    using backend_t = writer_backend<Device, tiff_tag>;

public:

    writer( const Device&                       io_dev
          , const image_write_info< tiff_tag >& info
          )
    : backend_t( io_dev
               , info
               )
    {}

    template<typename View>
    void apply( const View& view )
    {
        write_view( view );
    }

private:

    template< typename View >
    void write_view( const View& view )
    {
        using pixel_t = typename View::value_type;
        // get the type of the first channel (heterogeneous pixels might be broken for now!)
        using channel_t = typename channel_traits<typename element_type<pixel_t>::type>::value_type;
        tiff_bits_per_sample::type bits_per_sample = detail::unsigned_integral_num_bits< channel_t >::value;

        tiff_samples_per_pixel::type samples_per_pixel = num_channels< pixel_t >::value;

        this->write_header( view );

        if( this->_info._is_tiled == false )
        {
            write_data( view
                      , (view.width() * samples_per_pixel * bits_per_sample + 7) / 8
                      , typename is_bit_aligned< pixel_t >::type()
                      );
        }
        else
        {
            tiff_tile_width::type  tw = this->_info._tile_width;
            tiff_tile_length::type th = this->_info._tile_length;

            if(!this->_io_dev.check_tile_size( tw, th ))
            {
                io_error( "Tile sizes need to be multiples of 16." );
            }

            // tile related tags
            this->_io_dev.template set_property<tiff_tile_width> ( tw );
            this->_io_dev.template set_property<tiff_tile_length>( th );

            write_tiled_data( view
                            , tw
                            , th
                            , typename is_bit_aligned< pixel_t >::type()
                            );
        }
    }

	//////////////////////////////

	template<typename View>
	void write_bit_aligned_view_to_dev( const View&       view
                                      , const std::size_t row_size_in_bytes
                                      , const std::true_type&    // has_alpha
                                      )
    {
        byte_vector_t row( row_size_in_bytes );

        using x_it_t = typename View::x_iterator;
        x_it_t row_it = x_it_t( &(*row.begin()));

		auto pm_view = premultiply_view <typename View:: value_type> (view);

        for( typename View::y_coord_t y = 0; y < pm_view.height(); ++y )
        {
					std::copy( pm_view.row_begin( y )
										 , pm_view.row_end( y )
										 , row_it
						);


            this->_io_dev.write_scaline( row
                                       , (uint32) y
                                       , 0
                                       );

            // @todo: do optional bit swapping here if you need to...
        }
    }

	template<typename View>
	void write_bit_aligned_view_to_dev( const View&       view
                                      , const std::size_t row_size_in_bytes
                                      , const std::false_type&    // has_alpha
                                      )
    {
        byte_vector_t row( row_size_in_bytes );

        using x_it_t = typename View::x_iterator;
        x_it_t row_it = x_it_t( &(*row.begin()));

        for( typename View::y_coord_t y = 0; y < view.height(); ++y )
        {
			std::copy( view.row_begin( y )
                     , view.row_end( y )
                     , row_it
				     );


            this->_io_dev.write_scaline( row
                                       , (uint32) y
                                       , 0
                                       );

            // @todo: do optional bit swapping here if you need to...
        }
    }

    /////////////////////////////

    template< typename View >
    void write_data( const View&   view
                   , std::size_t   row_size_in_bytes
                   , const std::true_type&    // bit_aligned
                   )
    {
        using colour_space_t = typename color_space_type<typename View::value_type>::type;
        using has_alpha_t = mp11::mp_contains<colour_space_t, alpha_t>;

        write_bit_aligned_view_to_dev(view, row_size_in_bytes, has_alpha_t());
    }

    template< typename View>
    void write_tiled_data( const View&            view
                         , tiff_tile_width::type  tw
                         , tiff_tile_length::type th
                         , const std::true_type&    // bit_aligned
                         )
    {
        byte_vector_t row( this->_io_dev.get_tile_size() );

        using x_it_t = typename View::x_iterator;
        x_it_t row_it = x_it_t( &(*row.begin()));

        internal_write_tiled_data(view, tw, th, row, row_it);
    }

    template< typename View >
    void write_data( const View&   view
                   , std::size_t
                   , const std::false_type&    // bit_aligned
                   )
    {
        std::vector< pixel< typename channel_type< View >::type
                          , layout<typename color_space_type< View >::type >
                          >
                   > row( view.size() );

        byte_t* row_addr = reinterpret_cast< byte_t* >( &row.front() );

				// @todo: is there an overhead to doing this when there's no
				// alpha to premultiply by? I'd hope it's optimised out.
				auto pm_view = premultiply_view <typename View:: value_type> (view);

        for( typename View::y_coord_t y = 0; y < pm_view.height(); ++y )
        {
					std::copy( pm_view.row_begin( y )
										 , pm_view.row_end( y )
										 , row.begin()
						);

            this->_io_dev.write_scaline( row_addr
                                       , (uint32) y
                                       , 0
                                       );

            // @todo: do optional bit swapping here if you need to...
        }
    }

    template< typename View >
    void write_tiled_data( const View&            view
                         , tiff_tile_width::type  tw
                         , tiff_tile_length::type th
                         , const std::false_type&    // bit_aligned
                         )
    {
        byte_vector_t row( this->_io_dev.get_tile_size() );

        using x_iterator = typename detail::my_interleaved_pixel_iterator_type_from_pixel_reference<typename View::reference>::type;
        x_iterator row_it = x_iterator( &(*row.begin()));

        internal_write_tiled_data(view, tw, th, row, row_it);
    }


	//////////////////////////////

	template< typename View
            , typename IteratorType
            >
	void write_tiled_view_to_dev( const View&  view
                                , IteratorType it
                                , const std::true_type& // has_alpha
                                )
    {
        auto pm_view = premultiply_view <typename View:: value_type>( view );

        std::copy( pm_view.begin()
                 , pm_view.end()
                 , it
                 );
    }


	template< typename View
            , typename IteratorType
            >
	void write_tiled_view_to_dev( const View&  view
                                , IteratorType it
                                , const std::false_type& // has_alpha
                                )
    {
        std::copy( view.begin()
                 , view.end()
                 , it
                 );
    }

    /////////////////////////////



    template< typename View,
              typename IteratorType
            >
    void internal_write_tiled_data( const View&            view
                                  , tiff_tile_width::type  tw
                                  , tiff_tile_length::type th
                                  , byte_vector_t&         row
                                  , IteratorType           it
                                  )
    {
        std::ptrdiff_t i = 0, j = 0;
        View tile_subimage_view;
        while( i < view.height() )
        {
            while( j < view.width() )
            {
                if( j + tw < view.width() && i + th < view.height() )
                {
                    // a tile is fully included in the image: just copy values
                    tile_subimage_view = subimage_view( view
                                                      , static_cast< int >( j  )
                                                      , static_cast< int >( i  )
                                                      , static_cast< int >( tw )
                                                      , static_cast< int >( th )
                                                      );

                    using colour_space_t = typename color_space_type<typename View::value_type>::type;
                    using has_alpha_t = mp11::mp_contains<colour_space_t, alpha_t>;

                    write_tiled_view_to_dev(tile_subimage_view, it, has_alpha_t());
                }
                else
                {
                    std::ptrdiff_t width  = view.width();
                    std::ptrdiff_t height = view.height();

                    std::ptrdiff_t current_tile_width  = ( j + tw < width ) ? tw : width  - j;
                    std::ptrdiff_t current_tile_length = ( i + th < height) ? th : height - i;

                    tile_subimage_view = subimage_view( view
                                                      , static_cast< int >( j )
                                                      , static_cast< int >( i )
                                                      , static_cast< int >( current_tile_width )
                                                      , static_cast< int >( current_tile_length )
                                                      );

                    for( typename View::y_coord_t y = 0; y < tile_subimage_view.height(); ++y )
                    {
                        std::copy( tile_subimage_view.row_begin( y )
                                 , tile_subimage_view.row_end( y )
                                 , it
                                 );
                        std::advance(it, tw);
                    }

                    it = IteratorType( &(*row.begin()));
                }

                this->_io_dev.write_tile( row
                                        , static_cast< uint32 >( j )
                                        , static_cast< uint32 >( i )
                                        , 0
                                        , 0
                                        );
                j += tw;
            }
            j = 0;
            i += th;
        }
        // @todo: do optional bit swapping here if you need to...
    }
};

///
/// TIFF Dynamic Image Writer
///
template< typename Device >
class dynamic_image_writer< Device
                          , tiff_tag
                          >
    : public writer< Device
                   , tiff_tag
                   >
{
    using parent_t = writer<Device, tiff_tag>;

public:

    dynamic_image_writer( const Device&                       io_dev
                        , const image_write_info< tiff_tag >& info
                        )
    : parent_t( io_dev
              , info
              )
    {}

    template< typename ...Views >
    void apply( const any_image_view< Views... >& views )
    {
        detail::dynamic_io_fnobj< detail::tiff_write_is_supported
                                , parent_t
                                > op( this );

        apply_operation( views, op );
    }
};

#if BOOST_WORKAROUND(BOOST_MSVC, >= 1400)
#pragma warning(pop)
#endif

} // namespace gil
} // namespace boost

#endif
