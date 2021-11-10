//
// Copyright 2013 Christian Henning and Juan V. Puertos
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_TOOLBOX_IMAGE_TYPES_SUBCHROMA_IMAGE_HPP
#define BOOST_GIL_EXTENSION_TOOLBOX_IMAGE_TYPES_SUBCHROMA_IMAGE_HPP

#include <boost/gil/dynamic_step.hpp>
#include <boost/gil/image.hpp>
#include <boost/gil/image_view.hpp>
#include <boost/gil/point.hpp>
#include <boost/gil/virtual_locator.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>

namespace boost { namespace gil {

namespace detail {

template< int J, int A, int B>
struct scaling_factors
{
    static_assert(std::integral_constant<int, J>::value == 4, "");

    static_assert(
        std::integral_constant<int, A>::value == 4 ||
        std::integral_constant<int, A>::value == 2 ||
        std::integral_constant<int, A>::value == 1,
        "");

    static_assert(
        std::integral_constant<int, B>::value == 4 ||
        std::integral_constant<int, B>::value == 2 ||
        std::integral_constant<int, B>::value == 1 ||
        std::integral_constant<int, B>::value == 0,
        "");

    static constexpr int ss_X =
        std::integral_constant<int, J>::value / std::integral_constant<int, A>::value;

    static constexpr int ss_Y =
        mp11::mp_if_c
        <
            std::integral_constant<int, B>::value == 0,
            std::integral_constant<int, 2>,
            mp11::mp_if_c
            <
                std::integral_constant<int, A>::value == std::integral_constant<int, B>::value,
                std::integral_constant<int, 1>,
                std::integral_constant<int, 4>
            >
        >::value;
};

} // namespace detail

////////////////////////////////////////////////////////////////////////////////////////
/// \class subchroma_image_deref_fn
/// \ingroup PixelLocatorModel PixelBasedModel
/// \brief Used for virtual_2D_locator
///
////////////////////////////////////////////////////////////////////////////////////////
template< typename Locator
        , typename Factors
        >
struct subchroma_image_deref_fn
{
    using plane_locator_t = gray8_view_t::locator;

    using const_t = subchroma_image_deref_fn<Locator, Factors>;
    using value_type = typename Locator::value_type;
    using reference = value_type;
    using const_reference = value_type;
    using argument_type = point_t;
    using result_type = reference;

    static const bool is_mutable = false;

    /// default constructor
    subchroma_image_deref_fn() {}

    /// constructor
    subchroma_image_deref_fn( const plane_locator_t& y_locator
                            , const plane_locator_t& v_locator
                            , const plane_locator_t& u_locator
                             )
    : _y_locator( y_locator )
    , _v_locator( v_locator )
    , _u_locator( u_locator )
    {}

    /// operator()
    result_type operator()( const point_t& p ) const
    {
        using scaling_factors_t = detail::scaling_factors
            <
                mp11::mp_at_c<Factors, 0>::value,
                mp11::mp_at_c<Factors, 1>::value,
                mp11::mp_at_c<Factors, 2>::value
            >;

        plane_locator_t y = _y_locator.xy_at( p );
        plane_locator_t v = _v_locator.xy_at( p.x / scaling_factors_t::ss_X, p.y / scaling_factors_t::ss_X );
        plane_locator_t u = _u_locator.xy_at( p.x / scaling_factors_t::ss_X, p.y / scaling_factors_t::ss_X );

        return value_type( at_c< 0 >( *y )
                         , at_c< 0 >( *v )
                         , at_c< 0 >( *u )
                         );
    }

    ///
    const plane_locator_t& y_locator() const { return _y_locator; }
    const plane_locator_t& v_locator() const { return _v_locator; }
    const plane_locator_t& u_locator() const { return _u_locator; }

private:

    plane_locator_t _y_locator;
    plane_locator_t _v_locator;
    plane_locator_t _u_locator;
};


////////////////////////////////////////////////////////////////////////////////////////
/// \class subchroma_image_locator_type
/// \ingroup PixelLocatorModel PixelBasedModel
/// \brief
///
////////////////////////////////////////////////////////////////////////////////////////
template< typename Locator
        , typename Factors
        >
struct subchroma_image_locator
{
    using type = virtual_2d_locator
        <
            subchroma_image_deref_fn<Locator, Factors>, // Deref
            false // IsTransposed
        >;
};

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template < typename Locator, typename Factors >
struct channel_type< subchroma_image_locator< Locator, Factors > >
    : public channel_type< typename subchroma_image_locator< Locator, Factors >::type > {};

template < typename Locator, typename Factors >
struct color_space_type< subchroma_image_locator< Locator, Factors > >
    : public color_space_type< typename subchroma_image_locator< Locator, Factors >::type > {};

template < typename Locator, typename Factors >
struct channel_mapping_type< subchroma_image_locator< Locator, Factors > >
    : public channel_mapping_type< typename subchroma_image_locator< Locator, Factors >::type > {};

template < typename Locator, typename Factors >
struct is_planar< subchroma_image_locator< Locator, Factors > >
    : public is_planar< typename subchroma_image_locator< Locator, Factors >::type > {};

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template < typename Locator, typename Factors >
struct dynamic_x_step_type< subchroma_image_locator< Locator, Factors > >
{
    using type = typename subchroma_image_locator<Locator, Factors>::type;
};

/////////////////////////////
//  HasDynamicYStepTypeConcept
/////////////////////////////

template < typename Locator, typename Factors >
struct dynamic_y_step_type< subchroma_image_locator< Locator, Factors > >
{
    using type = typename subchroma_image_locator<Locator, Factors>::type;
};

/////////////////////////////
//  HasTransposedTypeConcept
/////////////////////////////

template < typename Locator, typename Factors >
struct transposed_type< subchroma_image_locator< Locator, Factors > >
{
    using type = typename subchroma_image_locator<Locator, Factors>::type;
};

//////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
/// \class subchroma_image_view
/// \ingroup ImageViewModel PixelBasedModel
/// \brief A lightweight object that interprets a subchroma image.
///
////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename Locator,
    typename Factors = mp11::mp_list_c<int, 4, 4, 4>
>
class subchroma_image_view : public image_view<Locator>
{
public:

    using locator = Locator;
    using deref_fn_t = typename locator::deref_fn_t;
    using plane_locator_t = typename deref_fn_t::plane_locator_t;


    using const_t = subchroma_image_view<Locator, Factors>;

    using plane_view_t = image_view<plane_locator_t>;

    /// default constructor
    subchroma_image_view()
    : image_view< Locator >()
    {}

    /// constructor
    subchroma_image_view( const point_t& y_dimensions
                        , const point_t& v_dimensions
                        , const point_t& u_dimensions
                        , const Locator& locator
                        )
    : image_view< Locator >( y_dimensions, locator )
    , _y_dimensions( y_dimensions )
    , _v_dimensions( v_dimensions )
    , _u_dimensions( u_dimensions )
    {}

    /// copy constructor
    template< typename Subchroma_View >
    subchroma_image_view( const Subchroma_View& v )
    : image_view< locator >( v )
    {}

    const point_t& v_ssfactors() const { return point_t( get_deref_fn().vx_ssfactor(), get_deref_fn().vx_ssfactor() ); }
    const point_t& u_ssfactors() const { return point_t( get_deref_fn().ux_ssfactor(), get_deref_fn().ux_ssfactor() ); }

    const point_t& y_dimension() const { return _y_dimensions; }
    const point_t& v_dimension() const { return _v_dimensions; }
    const point_t& u_dimension() const { return _u_dimensions; }

    const plane_locator_t& y_plane() const { return get_deref_fn().y_locator(); }
    const plane_locator_t& v_plane() const { return get_deref_fn().v_locator(); }
    const plane_locator_t& u_plane() const { return get_deref_fn().u_locator(); }

    const plane_view_t y_plane_view() const { return plane_view_t( _y_dimensions, y_plane() ); }
    const plane_view_t v_plane_view() const { return plane_view_t( _v_dimensions, v_plane() ); }
    const plane_view_t u_plane_view() const { return plane_view_t( _u_dimensions, u_plane() ); }


private:

    const deref_fn_t& get_deref_fn() const { return this->pixels().deref_fn(); }

private:

    point_t _y_dimensions;
    point_t _v_dimensions;
    point_t _u_dimensions;
};


/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template < typename Locator, typename Factors >
struct channel_type< subchroma_image_view< Locator, Factors > >
    : public channel_type< Locator > {};

template < typename Locator, typename Factors >
struct color_space_type< subchroma_image_view< Locator, Factors > >
    : public color_space_type< Locator > {};

template < typename Locator, typename Factors >
struct channel_mapping_type< subchroma_image_view< Locator, Factors > >
     : public channel_mapping_type< Locator > {};

template < typename Locator, typename Factors >
struct is_planar< subchroma_image_view< Locator, Factors > >
    : public is_planar< Locator > {};

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template < typename Locator, typename Factors >
struct dynamic_x_step_type< subchroma_image_view< Locator, Factors > >
{
    using type = image_view<typename dynamic_x_step_type<Locator>::type>;
};

/////////////////////////////
//  HasDynamicYStepTypeConcept
/////////////////////////////

template < typename Locator, typename Factors >
struct dynamic_y_step_type< subchroma_image_view< Locator, Factors > >
{
    using type = image_view<typename dynamic_y_step_type<Locator>::type>;
};

/////////////////////////////
//  HasTransposedTypeConcept
/////////////////////////////

template < typename Locator, typename Factors >
struct transposed_type< subchroma_image_view< Locator, Factors > >
{
    using type = image_view<typename transposed_type<Locator>::type>;
};

////////////////////////////////////////////////////////////////////////////////////////
/// \ingroup ImageModel PixelBasedModel
/// \brief container interface over image view. Models ImageConcept, PixelBasedConcept
///
/// A subchroma image holds a bunch of planes which don't need to have the same resolution.
///
////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename Pixel,
    typename Factors = mp11::mp_list_c<int, 4, 4, 4>,
    typename Allocator = std::allocator<unsigned char>
>
class subchroma_image : public detail::scaling_factors
    <
        mp11::mp_at_c<Factors, 0>::value,
        mp11::mp_at_c<Factors, 1>::value,
        mp11::mp_at_c<Factors, 2>::value
    >
{

private:
    using parent_t = detail::scaling_factors
        <
            mp11::mp_at_c<Factors, 0>::value,
            mp11::mp_at_c<Factors, 1>::value,
            mp11::mp_at_c<Factors, 2>::value
        >;

public:

    using channel_t = typename channel_type<Pixel>::type;
    using pixel_t = pixel<channel_t, gray_layout_t>;

    using plane_image_t = image<pixel_t, false, Allocator>;

    using plane_view_t = typename plane_image_t::view_t;
    using plane_const_view_t = typename plane_image_t::const_view_t;
    using plane_locator_t = typename plane_view_t::locator;

    using pixel_view_t = typename view_type_from_pixel<Pixel>::type;
    using pixel_locator_t = typename pixel_view_t::locator;

    using locator_t = typename subchroma_image_locator
        <
            pixel_locator_t,
            Factors
        >::type;

    using x_coord_t = typename plane_image_t::coord_t;
    using y_coord_t = typename plane_image_t::coord_t;

    using view_t = subchroma_image_view<locator_t, Factors>;
    using const_view_t = typename view_t::const_t;


    /// constructor
    subchroma_image( const x_coord_t y_width
                   , const y_coord_t y_height
                   )
    : _y_plane(        y_width,        y_height, 0, Allocator() )
    , _v_plane( y_width / parent_t::ss_X, y_height / parent_t::ss_Y, 0, Allocator() )
    , _u_plane( y_width / parent_t::ss_X, y_height / parent_t::ss_Y, 0, Allocator() )
    {
        init();
    }

public:

    view_t _view;

private:

    void init()
    {
        using defer_fn_t = subchroma_image_deref_fn<pixel_locator_t, Factors>;

        defer_fn_t deref_fn( view( _y_plane ).xy_at( 0, 0 )
                           , view( _v_plane ).xy_at( 0, 0 )
                           , view( _u_plane ).xy_at( 0, 0 )
                           );

        // init a virtual_2d_locator
        locator_t locator( point_t( 0, 0 ) // p
                         , point_t( 1, 1 ) // step
                         , deref_fn
                         );

        _view = view_t( _y_plane.dimensions()
                      , _v_plane.dimensions()
                      , _u_plane.dimensions()
                      , locator
                      );
    }


private:

    plane_image_t _y_plane;
    plane_image_t _v_plane;
    plane_image_t _u_plane;
};


/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template < typename Pixel, typename Factors, typename Alloc >
struct channel_type< subchroma_image< Pixel, Factors, Alloc > >
    : channel_type< Pixel > {};

template < typename Pixel, typename Factors, typename Alloc >
struct color_space_type< subchroma_image< Pixel, Factors, Alloc > >
    : color_space_type< Pixel > {};

template < typename Pixel, typename Factors, typename Alloc >
struct channel_mapping_type<  subchroma_image< Pixel, Factors, Alloc > >
    : channel_mapping_type< Pixel > {};

template < typename Pixel, typename Factors, typename Alloc >
struct is_planar< subchroma_image< Pixel, Factors, Alloc > >
    : std::integral_constant<bool, false>
{};


/////////////////////////////////////////////////////////////////////////////////////////
/// \name view, const_view
/// \brief Get an image view from an subchroma_image
/// \ingroup ImageModel
/// \brief Returns the non-constant-pixel view of an image
/////////////////////////////////////////////////////////////////////////////////////////
template< typename Pixel
        , typename Factors
        >
inline
const typename subchroma_image< Pixel, Factors >::view_t& view( subchroma_image< Pixel, Factors >& img )
{
    return img._view;
}

template< typename Pixel
        , typename Factors
        >
inline
const typename subchroma_image< Pixel, Factors >::const_view_t const_view( subchroma_image< Pixel, Factors >& img )
{
    return static_cast< const typename subchroma_image< Pixel, Factors >::const_view_t>( img._view );
}

/////////////////////////////////////////////////////////////////////////////////////////
/// \ingroup ImageViewSTLAlgorithmsFillPixels
/// \brief std::fill for subchroma_image views
/////////////////////////////////////////////////////////////////////////////////////////
template< typename Locator
        , typename Factors
        , typename Pixel
        >
void fill_pixels( const subchroma_image_view< Locator, Factors >& view
                , const Pixel&                                    value
                )
{
    using channel_t = typename subchroma_image
        <
            Pixel,
            Factors
        >::plane_view_t::value_type;

    fill_pixels( view.y_plane_view(), channel_t( at_c< 0 >( value )));
    fill_pixels( view.v_plane_view(), channel_t( at_c< 1 >( value )));
    fill_pixels( view.u_plane_view(), channel_t( at_c< 2 >( value )));
}

/////////////////////////////////////////////////////////////////////////////////////////
/// \ingroup ImageViewConstructors
/// \brief Creates a subchroma view from a raw memory
/////////////////////////////////////////////////////////////////////////////////////////
template< typename Pixel
        , typename Factors
        >
typename subchroma_image< Pixel
                        , Factors
                        >::view_t subchroma_view( std::size_t    y_width
                                                , std::size_t    y_height
                                                , unsigned char* y_base
                                                )
{
    using scaling_factors_t = detail::scaling_factors
        <
            mp11::mp_at_c<Factors, 0>::type::value,
            mp11::mp_at_c<Factors, 1>::type::value,
            mp11::mp_at_c<Factors, 2>::type::value
        >;

    std::size_t y_channel_size = 1;
    std::size_t u_channel_size = 1;

    unsigned char* u_base = y_base + ( y_width  * y_height * y_channel_size );
    unsigned char* v_base = u_base + ( y_width  / scaling_factors_t::ss_X )
                                   * u_channel_size;

    using plane_view_t = typename subchroma_image<Pixel, Factors>::plane_view_t;
    using plane_value_t = typename plane_view_t::value_type;

    plane_view_t y_plane = interleaved_view( y_width
                                           , y_height
                                           , (plane_value_t*) y_base // pixels
                                           , y_width                 // rowsize_in_bytes
                                           );

    plane_view_t v_plane = interleaved_view( y_width  / scaling_factors_t::ss_X
                                           , y_height / scaling_factors_t::ss_Y
                                           , (plane_value_t*) v_base // pixels
                                           , y_width                 // rowsize_in_bytes
                                           );

    plane_view_t u_plane = interleaved_view( y_width  / scaling_factors_t::ss_X
                                           , y_height / scaling_factors_t::ss_Y
                                           , (plane_value_t*) u_base // pixels
                                           , y_width                 // rowsize_in_bytes
                                           );

    using defer_fn_t = subchroma_image_deref_fn
        <
            typename subchroma_image<Pixel, Factors>::pixel_locator_t,
            Factors
        >;

    defer_fn_t deref_fn( y_plane.xy_at( 0, 0 )
                       , v_plane.xy_at( 0, 0 )
                       , u_plane.xy_at( 0, 0 )
                       );


    using locator_t = typename subchroma_image<Pixel, Factors>::locator_t;

    locator_t locator( point_t( 0, 0 ) // p
                     , point_t( 1, 1 ) // step
                     , deref_fn
                     );

    using view_t = typename subchroma_image<Pixel, Factors>::view_t;

    return view_t( point_t(                           y_width,                           y_height )
                 , point_t( y_width / scaling_factors_t::ss_X, y_height / scaling_factors_t::ss_Y )
                 , point_t( y_width / scaling_factors_t::ss_X, y_height / scaling_factors_t::ss_Y )
                 , locator
                 );
}

} // namespace gil
} // namespace boost

#endif
