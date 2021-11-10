//
// Copyright 2007-2008 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IO_READER_BASE_HPP
#define BOOST_GIL_IO_READER_BASE_HPP

#include <boost/gil/io/base.hpp>

#include <boost/assert.hpp>

namespace boost { namespace gil {

/// Reader Base Class
///
/// It provides some basic functionality which is shared for all readers.
/// For instance, it recreates images when necessary. It checks whether
/// user supplied coordinates are valid.
///
/// @tparam FormatTag        A format tag, like jpeg_tag.
/// @tparam ConversionPolicy Conversion policy, see coversion_policies.hpp.
template< typename FormatTag
        , typename ConversionPolicy
        >
struct reader_base
{
public:

    ///
    /// Default Constructor
    ///
    reader_base()
    :_cc_policy()
    {}

    ///
    /// Constructor
    ///
    reader_base( const ConversionPolicy& cc )
    :_cc_policy( cc )
    {}

    /// Initializes an image. But also does some check ups.
    ///
    /// @tparam Image Image which implements boost::gil's ImageConcept.
    ///
    /// @param img  The image.
    /// @param info The image read info.
    template< typename Image >
    void init_image( Image&                                  img
                   , const image_read_settings< FormatTag >& settings
                   )
    {
        //setup( backend._settings._dim );

        BOOST_ASSERT(settings._dim.x && settings._dim.y);

        img.recreate( settings._dim.x
                    , settings._dim.y
                    );
    }

    template< typename View >
    void init_view( const View&                             view
                  , const image_read_settings< FormatTag >&
                  )
    {
        setup( view.dimensions() );
    }

private:

    void setup( const point_t& /* dim */ )
    {
        //check_coordinates( dim );

        //if( dim == point_t( 0, 0 ))
        //{
        //    _settings._dim.x = _info._width;
        //    _settings._dim.y = _info._height;
        //}
        //else
        //{
        //    _settings._dim = dim;
        //}
    }

    void check_coordinates( const point_t& /* dim */ )
    {
       //using int_t = point_t::value_type;

       //int_t width  = static_cast< int_t >( _info._width  );
       //int_t height = static_cast< int_t >( _info._height );

       //io_error_if( (  _settings._top_left.x < 0
       //             || _settings._top_left.y < 0
       //             || dim.x < 0
       //             || dim.y < 0
       //             )
       //          , "User provided view has incorrect size." );


       //io_error_if( (  ( width  ) <  _settings._top_left.x
       //             && ( width  ) <= dim.x
       //             && ( height ) <  _settings._top_left.y
       //             && ( height ) <= dim.y  )
       //          , "User provided view has incorrect size."       );

       //io_error_if( (  ( _settings._top_left.x + dim.x ) > width
       //             || ( _settings._top_left.y + dim.y ) > height
       //            )
       //          , "User provided view has incorrect size." );
    }

protected:

    ConversionPolicy _cc_policy;
};

} // namespace gil
} // namespace boost

#endif
