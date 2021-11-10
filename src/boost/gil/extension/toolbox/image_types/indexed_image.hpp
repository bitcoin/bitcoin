//
// Copyright 2012 Christian Henning
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_EXTENSION_TOOLBOX_IMAGE_TYPES_INDEXED_IMAGE_HPP
#define BOOST_GIL_EXTENSION_TOOLBOX_IMAGE_TYPES_INDEXED_IMAGE_HPP

#include <boost/gil/extension/toolbox/metafunctions/is_bit_aligned.hpp>

#include <boost/gil/image.hpp>
#include <boost/gil/point.hpp>
#include <boost/gil/virtual_locator.hpp>
#include <boost/gil/detail/is_channel_integral.hpp>
#include <boost/gil/detail/mp11.hpp>

#include <cstddef>
#include <memory>

namespace boost { namespace gil {

template< typename Locator >
struct get_pixel_type_locator
{
    using type = mp11::mp_if
        <
            typename is_bit_aligned<typename Locator::value_type>::type,
            typename Locator::reference,
            typename Locator::value_type
        >;
};

// used for virtual locator
template< typename IndicesLoc
        , typename PaletteLoc
        >
struct indexed_image_deref_fn_base
{
    using indices_locator_t = IndicesLoc;
    using palette_locator_t = PaletteLoc;
    //using index_t = typename get_pixel_type_locator<indices_locator_t>::type;

    using const_t = indexed_image_deref_fn_base<IndicesLoc, PaletteLoc>;
    using value_type = typename PaletteLoc::value_type;
    using reference = value_type;
    using const_reference = value_type;
    using argument_type = point_t;
    using result_type = reference;

    static const bool is_mutable = false;

    indexed_image_deref_fn_base() {}

    indexed_image_deref_fn_base( const indices_locator_t& indices_loc
                               , const palette_locator_t& palette_loc
                               )
    : _indices_loc( indices_loc )
    , _palette_loc( palette_loc )
    {}

    void set_indices( const indices_locator_t& indices_loc ) { _indices_loc = indices_loc; }
    void set_palette( const palette_locator_t& palette_loc ) { _palette_loc = palette_loc; }

    const indices_locator_t& indices() const { return _indices_loc; }
    const palette_locator_t& palette() const { return _palette_loc; }

protected:

    indices_locator_t _indices_loc;
    palette_locator_t _palette_loc;
};


// used for virtual locator
template< typename IndicesLoc
        , typename PaletteLoc
        , typename Enable = void // there is specialization for integral indices
        >
struct indexed_image_deref_fn : indexed_image_deref_fn_base< IndicesLoc
                                                           , PaletteLoc
                                                           >
{
    using base_t = indexed_image_deref_fn_base
        <
            IndicesLoc,
            PaletteLoc
        >;

    indexed_image_deref_fn()
    : base_t()
    {}

    indexed_image_deref_fn( const typename base_t::indices_locator_t& indices_loc
                          , const typename base_t::palette_locator_t& palette_loc
                          )
    : base_t( indices_loc
            , palette_loc
            )
    {}

    typename base_t::result_type operator()( const point_t& p ) const
    {
        return * this->_palette_loc.xy_at( at_c<0>( *this->_indices_loc.xy_at( p )), 0 );
    }
};


template <typename IndicesLoc, typename PaletteLoc>
struct indexed_image_deref_fn
<
    IndicesLoc,
    PaletteLoc,
    typename std::enable_if
    <
        detail::is_channel_integral<typename IndicesLoc::value_type>::value
    >::type
> : indexed_image_deref_fn_base<IndicesLoc, PaletteLoc>
{
    using base_t = indexed_image_deref_fn_base<IndicesLoc, PaletteLoc>;

    indexed_image_deref_fn() : base_t() {}

    indexed_image_deref_fn(
        typename base_t::indices_locator_t const& indices_loc,
        typename base_t::palette_locator_t const& palette_loc)
        : base_t(indices_loc, palette_loc)
    {
    }

    typename base_t::result_type operator()(point_t const& p) const
    {
        return *this->_palette_loc.xy_at(*this->_indices_loc.xy_at(p), 0);
    }
};

template< typename IndicesLoc
        , typename PaletteLoc
        >
struct indexed_image_locator_type
{
    using type = virtual_2d_locator
        <
            indexed_image_deref_fn<IndicesLoc, PaletteLoc>,
            false
        >;
};

template< typename Locator > // indexed_image_locator_type< ... >::type
class indexed_image_view : public image_view< Locator >
{
public:

    using deref_fn_t = typename Locator::deref_fn_t;
    using indices_locator_t = typename deref_fn_t::indices_locator_t;
    using palette_locator_t = typename deref_fn_t::palette_locator_t;

    using const_t = indexed_image_view<Locator>;

    using indices_view_t = image_view<indices_locator_t>;
    using palette_view_t = image_view<palette_locator_t>;

    indexed_image_view()
    : image_view< Locator >()
    , _num_colors( 0 )
    {}

    indexed_image_view( const point_t& dimensions
                      , std::size_t    num_colors
                      , const Locator& locator
                      )
    : image_view< Locator >( dimensions, locator )
    , _num_colors( num_colors )
    {}

    template< typename IndexedView >
    indexed_image_view( const IndexedView& iv )
    : image_view< Locator >( iv )
    , _num_colors( iv._num_colors )
    {}

    std::size_t num_colors() const { return _num_colors; }


    const indices_locator_t& indices() const { return get_deref_fn().indices(); }
    const palette_locator_t& palette() const { return get_deref_fn().palette(); }

    indices_view_t get_indices_view() const { return indices_view_t(this->dimensions(), indices() );}
    palette_view_t get_palette_view() const { return palette_view_t(point_t(num_colors(), 1), palette());}

private:

    const deref_fn_t& get_deref_fn() const { return this->pixels().deref_fn(); }

private:

    template< typename Locator2 > friend class indexed_image_view;

    std::size_t _num_colors;
};

// build an indexed_image_view from two views
template<typename Index_View, typename Palette_View>
indexed_image_view
<
    typename indexed_image_locator_type
    <
        typename Index_View::locator
        , typename Palette_View::locator
    >::type
>
    view(Index_View iv, Palette_View pv)
{
    using view_t = indexed_image_view
        <
            typename indexed_image_locator_type
                <
                    typename Index_View::locator,
                    typename Palette_View::locator
                >::type
        >;

    using defer_fn_t = indexed_image_deref_fn
        <
            typename Index_View::locator,
            typename Palette_View::locator
        >;

    return view_t(
        iv.dimensions()
        , pv.dimensions().x
        , typename view_t::locator(point_t(0, 0), point_t(1, 1), defer_fn_t(iv.xy_at(0, 0), pv.xy_at(0, 0)))
    );
}

template< typename Index
        , typename Pixel
        , typename IndicesAllocator = std::allocator< unsigned char >
        , typename PalleteAllocator = std::allocator< unsigned char >
        >
class indexed_image
{
public:

    using indices_t = image<Index, false, IndicesAllocator>;
    using palette_t = image<Pixel, false, PalleteAllocator>;

    using indices_view_t = typename indices_t::view_t;
    using palette_view_t = typename palette_t::view_t;

    using indices_const_view_t = typename indices_t::const_view_t;
    using palette_const_view_t = typename palette_t::const_view_t;

    using indices_locator_t = typename indices_view_t::locator;
    using palette_locator_t = typename palette_view_t::locator;

    using locator_t = typename indexed_image_locator_type
        <
            indices_locator_t,
            palette_locator_t
        >::type;

    using x_coord_t = typename indices_t::coord_t;
    using y_coord_t = typename indices_t::coord_t;


    using view_t = indexed_image_view<locator_t>;
    using const_view_t = typename view_t::const_t;

    indexed_image( const x_coord_t   width = 0
                 , const y_coord_t   height = 0
                 , const std::size_t num_colors = 1
                 , const std::size_t indices_alignment = 0
                 , const std::size_t palette_alignment = 0
                 )
    : _indices( width     , height, indices_alignment, IndicesAllocator() )
    , _palette( num_colors,      1, palette_alignment, PalleteAllocator() )
    {
        init( point_t( width, height ), num_colors );
    }

    indexed_image( const point_t&    dimensions
                 , const std::size_t num_colors = 1
                 , const std::size_t indices_alignment = 0
                 , const std::size_t palette_alignment = 0
                 )
    : _indices( dimensions,    indices_alignment, IndicesAllocator() )
    , _palette( num_colors, 1, palette_alignment, PalleteAllocator() )
    {
        init( dimensions, num_colors );
    }

    indexed_image( const indexed_image& img )
    : _indices( img._indices )
    , _palette( img._palette )
    {}

    template <typename Pixel2, typename Index2>
    indexed_image( const indexed_image< Pixel2, Index2 >& img )
    {
        _indices = img._indices;
        _palette = img._palette;
    }

    indexed_image& operator= ( const indexed_image& img )
    {
        _indices = img._indices;
        _palette = img._palette;

        return *this;
    }

    indices_const_view_t get_indices_const_view() const { return static_cast< indices_const_view_t >( _view.get_indices_view()); }
    palette_const_view_t get_palette_const_view() const { return static_cast< palette_const_view_t >( _view.get_palette_view()); }

    indices_view_t get_indices_view() { return _view.get_indices_view(); }
    palette_view_t get_palette_view() { return _view.get_palette_view(); }

public:

    view_t _view;

private:

    void init( const point_t&    dimensions
             , const std::size_t num_colors
             )
    {
        using defer_fn_t = indexed_image_deref_fn
            <
                indices_locator_t,
                palette_locator_t
            >;

        defer_fn_t deref_fn( view( _indices ).xy_at( 0, 0 )
                           , view( _palette ).xy_at( 0, 0 )
                           );

        locator_t locator( point_t( 0, 0 ) // p
                         , point_t( 1, 1 ) // step
                         , deref_fn
                         );

        _view = view_t( dimensions
                      , num_colors
                      , locator
                      );
    }

private:

    indices_t _indices;
    palette_t _palette;
};

template< typename Index
        , typename Pixel
        >
inline
const typename indexed_image< Index, Pixel >::view_t& view( indexed_image< Index, Pixel >& img )
{
    return img._view;
}

template< typename Index
        , typename Pixel
        >
inline
const typename indexed_image< Index, Pixel >::const_view_t const_view( indexed_image< Index, Pixel >& img )
{
    return static_cast< const typename indexed_image< Index, Pixel >::const_view_t>( img._view );
}

// Whole image has one color and all indices are set to 0.
template< typename Locator
        , typename Value
        >
void fill_pixels( const indexed_image_view< Locator >& view
                , const Value&                         value
                )
{
    using view_t = indexed_image_view<Locator>;

    fill_pixels( view.get_indices_view(), typename view_t::indices_view_t::value_type( 0 ));
    *view.get_palette_view().begin() = value;
}

} // namespace gil
} // namespace boost

#endif
