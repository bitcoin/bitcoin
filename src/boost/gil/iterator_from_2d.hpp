//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_ITERATOR_FROM_2D_HPP
#define BOOST_GIL_ITERATOR_FROM_2D_HPP

#include <boost/gil/concepts.hpp>
#include <boost/gil/locator.hpp>
#include <boost/gil/pixel_iterator.hpp>
#include <boost/gil/point.hpp>

#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace boost { namespace gil {

/// pixel step iterator, pixel image iterator and pixel dereference iterator

////////////////////////////////////////////////////////////////////////////////////////
///
///                 ITERATOR FROM 2D ADAPTOR
///
////////////////////////////////////////////////////////////////////////////////////////


/// \defgroup PixelIteratorModelFromLocator iterator_from_2d
/// \ingroup PixelIteratorModel
/// \brief An iterator over two-dimensional locator. Useful for iterating over the pixels of an image view. Models PixelIteratorConcept, PixelBasedConcept, HasDynamicXStepTypeConcept


/// \ingroup PixelIteratorModelFromLocator PixelBasedModel
/// \brief Provides 1D random-access navigation to the pixels of the image. Models: PixelIteratorConcept, PixelBasedConcept, HasDynamicXStepTypeConcept
///
/// Pixels are traversed from the top to the bottom row and from the left to the right
/// within each row

template <typename Loc2>    // Models PixelLocatorConcept
class iterator_from_2d : public iterator_facade<iterator_from_2d<Loc2>,
                                                typename Loc2::value_type,
                                                std::random_access_iterator_tag,
                                                typename Loc2::reference,
                                                typename Loc2::coord_t> {
    BOOST_GIL_CLASS_REQUIRE(Loc2, boost::gil, PixelLocatorConcept)
public:
    using parent_t = iterator_facade<iterator_from_2d<Loc2>,
                            typename Loc2::value_type,
                            std::random_access_iterator_tag,
                            typename Loc2::reference,
                            typename Loc2::coord_t>;
    using reference = typename parent_t::reference;
    using difference_type = typename parent_t::difference_type;
    using x_iterator = typename Loc2::x_iterator;
    using point_t = typename Loc2::point_t;

    std::ptrdiff_t width()         const { return _width; }            // number of pixels per image row
    std::ptrdiff_t x_pos()         const { return _coords.x; }         // current x position
    std::ptrdiff_t y_pos()         const { return _coords.y; }         // current y position

    /// For some reason operator[] provided by iterator_adaptor returns a custom class that is convertible to reference
    /// We require our own reference because it is registered in iterator_traits
    reference operator[](difference_type d) const { return *(*this+d); }

    bool            is_1d_traversable() const { return _p.is_1d_traversable(width()); }   // is there no gap at the end of each row?
    x_iterator&     x()                   { return _p.x(); }

    iterator_from_2d() = default;
    iterator_from_2d(const Loc2& p, std::ptrdiff_t width, std::ptrdiff_t x=0, std::ptrdiff_t y=0) : _coords(x,y), _width(width), _p(p) {}
    iterator_from_2d(const iterator_from_2d& pit) : _coords(pit._coords), _width(pit._width), _p(pit._p) {}
    template <typename Loc> iterator_from_2d(const iterator_from_2d<Loc>& pit) : _coords(pit._coords), _width(pit._width), _p(pit._p) {}
    iterator_from_2d& operator=(iterator_from_2d const& other) = default;

private:
    template <typename Loc> friend class iterator_from_2d;
    friend class boost::iterator_core_access;
    reference dereference() const { return *_p; }
    void increment() {
        ++_coords.x;
        ++_p.x();
        if (_coords.x>=_width) {
            _coords.x=0;
            ++_coords.y;
            _p+=point_t(-_width,1);
        }
    }
    void decrement() {
        --_coords.x;
        --_p.x();
        if (_coords.x<0) {
            _coords.x=_width-1;
            --_coords.y;
            _p+=point_t(_width,-1);
        }
    }

    BOOST_FORCEINLINE void advance(difference_type d) {
        if (_width==0) return;  // unfortunately we need to check for that. Default-constructed images have width of 0 and the code below will throw if executed.
        point_t delta;
        if (_coords.x+d>=0) {  // not going back to a previous row?
            delta.x=(_coords.x+(std::ptrdiff_t)d)%_width - _coords.x;
            delta.y=(_coords.x+(std::ptrdiff_t)d)/_width;
        } else {
            delta.x=(_coords.x+(std::ptrdiff_t)d*(1-_width))%_width -_coords.x;
            delta.y=-(_width-_coords.x-(std::ptrdiff_t)d-1)/_width;
        }
        _p+=delta;
        _coords.x+=delta.x;
        _coords.y+=delta.y;
    }

    difference_type distance_to(const iterator_from_2d& it) const {
        if (_width==0) return 0;
        return (it.y_pos()-_coords.y)*_width + (it.x_pos()-_coords.x);
    }

    bool equal(iterator_from_2d const& it) const
    {
        BOOST_ASSERT(_width == it.width()); // they must belong to the same image
        return _coords == it._coords && _p == it._p;
    }

    point_t _coords;
    std::ptrdiff_t _width;
    Loc2 _p;
};

template <typename Loc> // Models PixelLocatorConcept
struct const_iterator_type<iterator_from_2d<Loc> > {
    using type = iterator_from_2d<typename Loc::const_t>;
};

template <typename Loc> // Models PixelLocatorConcept
struct iterator_is_mutable<iterator_from_2d<Loc> > : public iterator_is_mutable<typename Loc::x_iterator> {};


/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename Loc>
struct dynamic_x_step_type<iterator_from_2d<Loc> > {
    using type = iterator_from_2d<typename dynamic_x_step_type<Loc>::type>;
};


/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename Loc> // Models PixelLocatorConcept
struct color_space_type<iterator_from_2d<Loc> > : public color_space_type<Loc> {};

template <typename Loc> // Models PixelLocatorConcept
struct channel_mapping_type<iterator_from_2d<Loc> > : public channel_mapping_type<Loc> {};

template <typename Loc> // Models PixelLocatorConcept
struct is_planar<iterator_from_2d<Loc> > : public is_planar<Loc> {};

template <typename Loc> // Models PixelLocatorConcept
struct channel_type<iterator_from_2d<Loc> > : public channel_type<Loc> {};

} }  // namespace boost::gil

#endif
