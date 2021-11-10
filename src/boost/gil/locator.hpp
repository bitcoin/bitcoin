//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_LOCATOR_HPP
#define BOOST_GIL_LOCATOR_HPP

#include <boost/gil/dynamic_step.hpp>
#include <boost/gil/pixel_iterator.hpp>
#include <boost/gil/point.hpp>

#include <boost/assert.hpp>

#include <cstddef>

namespace boost { namespace gil {

/// Pixel 2D locator

//forward declarations
template <typename P> std::ptrdiff_t memunit_step(const P*);
template <typename P> P* memunit_advanced(const P* p, std::ptrdiff_t diff);
template <typename P> P& memunit_advanced_ref(P* p, std::ptrdiff_t diff);
template <typename Iterator, typename D> struct iterator_add_deref;
template <typename T> class point;
namespace detail {
    // helper class specialized for each axis of pixel_2d_locator
    template <std::size_t D, typename Loc>  class locator_axis;
}

template <typename T> struct channel_type;
template <typename T> struct color_space_type;
template <typename T> struct channel_mapping_type;
template <typename T> struct is_planar;
template <typename T> struct num_channels;

/// Base template for types that model HasTransposedTypeConcept.
/// The type of a locator or a view that has X and Y swapped.
/// By default it is the same.
template <typename LocatorOrView>
struct transposed_type
{
    using type = LocatorOrView;
};

/// \class pixel_2d_locator_base
/// \brief base class for models of PixelLocatorConcept
/// \ingroup PixelLocatorModel PixelBasedModel
///
/// Pixel locator is similar to a pixel iterator, but allows for 2D navigation of pixels within an image view.
/// It has a 2D difference_type and supports random access operations like:
/// \code
///     difference_type offset2(2,3);
///     locator+=offset2;
///     locator[offset2]=my_pixel;
/// \endcode
///
/// In addition, each coordinate acts as a random-access iterator that can be modified separately:
/// "++locator.x()" or "locator.y()+=10" thereby moving the locator horizontally or vertically.
///
/// It is called a locator because it doesn't implement the complete interface of a random access iterator.
/// For example, increment and decrement operations don't make sense (no way to specify dimension).
/// Also 2D difference between two locators cannot be computed without knowledge of the X position within the image.
///
/// This base class provides most of the methods and type aliases needed to create a model of a locator. GIL provides two
/// locator models as subclasses of \p pixel_2d_locator_base. A memory-based locator, \p memory_based_2d_locator and a virtual
/// locator, \p virtual_2d_locator.
/// The minimum functionality a subclass must provide is this:
/// \code
/// class my_locator : public pixel_2d_locator_base<my_locator, ..., ...> {  // supply the types for x-iterator and y-iterator
///        using const_t = ...;                      // read-only locator
///
///        template <typename Deref> struct add_deref {
///            using type = ...;                     // locator that invokes the Deref dereference object upon pixel access
///            static type make(const my_locator& loc, const Deref& d);
///        };
///
///        my_locator();
///        my_locator(const my_locator& pl);
///
///        // constructors with dynamic step in y (and x). Only valid for locators with dynamic steps
///        my_locator(const my_locator& loc, coord_t y_step);
///        my_locator(const my_locator& loc, coord_t x_step, coord_t y_step, bool transpose);
///
///        bool              operator==(const my_locator& p) const;
///
///        // return _references_ to horizontal/vertical iterators. Advancing them moves this locator
///        x_iterator&       x();
///        y_iterator&       y();
///        x_iterator const& x() const;
///        y_iterator const& y() const;
///
///        // return the vertical distance to another locator. Some models need the horizontal distance to compute it
///        y_coord_t         y_distance_to(const my_locator& loc2, x_coord_t xDiff) const;
///
///        // return true iff incrementing an x-iterator located at the last column will position it at the first
///        // column of the next row. Some models need the image width to determine that.
///        bool              is_1d_traversable(x_coord_t width) const;
/// };
/// \endcode
///
/// Models may choose to override some of the functions in the base class with more efficient versions.
///

template <typename Loc, typename XIterator, typename YIterator>    // The concrete subclass, the X-iterator and the Y-iterator
class pixel_2d_locator_base
{
public:
    using x_iterator = XIterator;
    using y_iterator = YIterator;

    // aliasesrequired by ConstRandomAccessNDLocatorConcept
    static const std::size_t num_dimensions=2;
    using value_type = typename std::iterator_traits<x_iterator>::value_type;
    using reference = typename std::iterator_traits<x_iterator>::reference;    // result of dereferencing
    using coord_t = typename std::iterator_traits<x_iterator>::difference_type;      // 1D difference type (same for all dimensions)
    using difference_type = point<coord_t>; // result of operator-(locator,locator)
    using point_t = difference_type;
    template <std::size_t D> struct axis
    {
        using coord_t = typename detail::locator_axis<D,Loc>::coord_t;
        using iterator = typename detail::locator_axis<D,Loc>::iterator;
    };

// aliases required by ConstRandomAccess2DLocatorConcept
    using x_coord_t = typename point_t::template axis<0>::coord_t;
    using y_coord_t = typename point_t::template axis<1>::coord_t;

    bool              operator!=(const Loc& p)          const { return !(concrete()==p); }

    x_iterator        x_at(x_coord_t dx, y_coord_t dy)  const { Loc tmp=concrete(); tmp+=point_t(dx,dy); return tmp.x(); }
    x_iterator        x_at(const difference_type& d)    const { Loc tmp=concrete(); tmp+=d;              return tmp.x(); }
    y_iterator        y_at(x_coord_t dx, y_coord_t dy)  const { Loc tmp=concrete(); tmp+=point_t(dx,dy); return tmp.y(); }
    y_iterator        y_at(const difference_type& d)    const { Loc tmp=concrete(); tmp+=d;              return tmp.y(); }
    Loc               xy_at(x_coord_t dx, y_coord_t dy) const { Loc tmp=concrete(); tmp+=point_t(dx,dy); return tmp; }
    Loc               xy_at(const difference_type& d)   const { Loc tmp=concrete(); tmp+=d;              return tmp; }

    template <std::size_t D> typename axis<D>::iterator&       axis_iterator()                       { return detail::locator_axis<D,Loc>()(concrete()); }
    template <std::size_t D> typename axis<D>::iterator const& axis_iterator()                 const { return detail::locator_axis<D,Loc>()(concrete()); }
    template <std::size_t D> typename axis<D>::iterator        axis_iterator(const point_t& p) const { return detail::locator_axis<D,Loc>()(concrete(),p); }

    reference         operator()(x_coord_t dx, y_coord_t dy) const { return *x_at(dx,dy); }
    reference         operator[](const difference_type& d)   const { return *x_at(d.x,d.y); }

    reference         operator*()                            const { return *concrete().x(); }

    Loc&              operator+=(const difference_type& d)         { concrete().x()+=d.x; concrete().y()+=d.y; return concrete(); }
    Loc&              operator-=(const difference_type& d)         { concrete().x()-=d.x; concrete().y()-=d.y; return concrete(); }

    Loc               operator+(const difference_type& d)    const { return xy_at(d); }
    Loc               operator-(const difference_type& d)    const { return xy_at(-d); }

    // Some locators can cache 2D coordinates for faster subsequent access. By default there is no caching
    using cached_location_t = difference_type;
    cached_location_t cache_location(const difference_type& d)  const { return d; }
    cached_location_t cache_location(x_coord_t dx, y_coord_t dy)const { return difference_type(dx,dy); }

private:
    Loc&              concrete()       { return (Loc&)*this; }
    const Loc&        concrete() const { return (const Loc&)*this; }

    template <typename X> friend class pixel_2d_locator;
};

// helper classes for each axis of pixel_2d_locator_base
namespace detail {
    template <typename Loc>
    class locator_axis<0,Loc> {
        using point_t = typename Loc::point_t;
    public:
        using coord_t = typename point_t::template axis<0>::coord_t;
        using iterator = typename Loc::x_iterator;

        inline iterator&        operator()(      Loc& loc)                   const { return loc.x(); }
        inline iterator  const& operator()(const Loc& loc)                   const { return loc.x(); }
        inline iterator         operator()(      Loc& loc, const point_t& d) const { return loc.x_at(d); }
        inline iterator         operator()(const Loc& loc, const point_t& d) const { return loc.x_at(d); }
    };

    template <typename Loc>
    class locator_axis<1,Loc> {
        using point_t = typename Loc::point_t;
    public:
        using coord_t = typename point_t::template axis<1>::coord_t;
        using iterator = typename Loc::y_iterator;

        inline iterator&        operator()(      Loc& loc)               const { return loc.y(); }
        inline iterator const&  operator()(const Loc& loc)               const { return loc.y(); }
        inline iterator     operator()(      Loc& loc, const point_t& d) const { return loc.y_at(d); }
        inline iterator     operator()(const Loc& loc, const point_t& d) const { return loc.y_at(d); }
    };
}

template <typename Loc, typename XIt, typename YIt>
struct channel_type<pixel_2d_locator_base<Loc,XIt,YIt> > : public channel_type<XIt> {};

template <typename Loc, typename XIt, typename YIt>
struct color_space_type<pixel_2d_locator_base<Loc,XIt,YIt> > : public color_space_type<XIt> {};

template <typename Loc, typename XIt, typename YIt>
struct channel_mapping_type<pixel_2d_locator_base<Loc,XIt,YIt> > : public channel_mapping_type<XIt> {};

template <typename Loc, typename XIt, typename YIt>
struct is_planar<pixel_2d_locator_base<Loc,XIt,YIt> > : public is_planar<XIt> {};

/// \class memory_based_2d_locator
/// \brief Memory-based pixel locator. Models: PixelLocatorConcept,HasDynamicXStepTypeConcept,HasDynamicYStepTypeConcept,HasTransposedTypeConcept
/// \ingroup PixelLocatorModel PixelBasedModel
///
/// The class takes a step iterator as a parameter. The step iterator provides navigation along the vertical axis
/// while its base iterator provides horizontal navigation.
///
/// Each instantiation is optimal in terms of size and efficiency.
/// For example, xy locator over interleaved rgb image results in a step iterator consisting of
/// one std::ptrdiff_t for the row size and one native pointer (8 bytes total). ++locator.x() resolves to pointer
/// increment. At the other extreme, a 2D navigation of the even pixels of a planar CMYK image results in a step
/// iterator consisting of one std::ptrdiff_t for the doubled row size, and one step iterator consisting of
/// one std::ptrdiff_t for the horizontal step of two and a CMYK planar_pixel_iterator consisting of 4 pointers (24 bytes).
/// In this case ++locator.x() results in four native pointer additions.
///
/// Note also that \p memory_based_2d_locator does not require that its element type be a pixel. It could be
/// instantiated with an iterator whose \p value_type models only \p Regular. In this case the locator
/// models the weaker RandomAccess2DLocatorConcept, and does not model PixelBasedConcept.
/// Many generic algorithms don't require the elements to be pixels.
////////////////////////////////////////////////////////////////////////////////////////

template <typename StepIterator>
class memory_based_2d_locator : public pixel_2d_locator_base<memory_based_2d_locator<StepIterator>, typename iterator_adaptor_get_base<StepIterator>::type, StepIterator> {
    using this_t = memory_based_2d_locator<StepIterator>;
    BOOST_GIL_CLASS_REQUIRE(StepIterator, boost::gil, StepIteratorConcept)
public:
    using parent_t = pixel_2d_locator_base<memory_based_2d_locator<StepIterator>, typename iterator_adaptor_get_base<StepIterator>::type, StepIterator>;
    using const_t = memory_based_2d_locator<typename const_iterator_type<StepIterator>::type>; // same as this type, but over const values

    using coord_t = typename parent_t::coord_t;
    using x_coord_t = typename parent_t::x_coord_t;
    using y_coord_t = typename parent_t::y_coord_t;
    using x_iterator = typename parent_t::x_iterator;
    using y_iterator = typename parent_t::y_iterator;
    using difference_type = typename parent_t::difference_type;
    using reference = typename parent_t::reference;

    template <typename Deref> struct add_deref
    {
        using type = memory_based_2d_locator<typename iterator_add_deref<StepIterator,Deref>::type>;
        static type make(const memory_based_2d_locator<StepIterator>& loc, const Deref& nderef) {
            return type(iterator_add_deref<StepIterator,Deref>::make(loc.y(),nderef));
        }
    };

    memory_based_2d_locator() {}
    memory_based_2d_locator(const StepIterator& yit) : _p(yit) {}
    template <typename SI> memory_based_2d_locator(const memory_based_2d_locator<SI>& loc, coord_t y_step) : _p(loc.x(), loc.row_size()*y_step) {}
    template <typename SI> memory_based_2d_locator(const memory_based_2d_locator<SI>& loc, coord_t x_step, coord_t y_step, bool transpose=false)
        : _p(make_step_iterator(loc.x(),(transpose ? loc.row_size() : loc.pixel_size())*x_step),
                                        (transpose ? loc.pixel_size() : loc.row_size())*y_step ) {}

    memory_based_2d_locator(x_iterator xit, std::ptrdiff_t row_bytes) : _p(xit,row_bytes) {}
    template <typename X> memory_based_2d_locator(const memory_based_2d_locator<X>& pl) : _p(pl._p) {}
    memory_based_2d_locator(const memory_based_2d_locator& pl) : _p(pl._p) {}
    memory_based_2d_locator& operator=(memory_based_2d_locator const& other) = default;

    bool                  operator==(const this_t& p)  const { return _p==p._p; }

    x_iterator const&     x()                          const { return _p.base(); }
    y_iterator const&     y()                          const { return _p; }
    x_iterator&           x()                                { return _p.base(); }
    y_iterator&           y()                                { return _p; }

    // These are faster versions of functions already provided in the superclass
    x_iterator x_at      (x_coord_t dx, y_coord_t dy)  const { return memunit_advanced(x(), offset(dx,dy)); }
    x_iterator x_at      (const difference_type& d)    const { return memunit_advanced(x(), offset(d.x,d.y)); }
    this_t     xy_at     (x_coord_t dx, y_coord_t dy)  const { return this_t(x_at( dx , dy ), row_size()); }
    this_t     xy_at     (const difference_type& d)    const { return this_t(x_at( d.x, d.y), row_size()); }
    reference  operator()(x_coord_t dx, y_coord_t dy)  const { return memunit_advanced_ref(x(),offset(dx,dy)); }
    reference  operator[](const difference_type& d)    const { return memunit_advanced_ref(x(),offset(d.x,d.y)); }
    this_t&    operator+=(const difference_type& d)          { memunit_advance(x(),offset(d.x,d.y)); return *this; }
    this_t&    operator-=(const difference_type& d)          { memunit_advance(x(),offset(-d.x,-d.y)); return *this; }

    // Memory-based locators can have 1D caching of 2D relative coordinates
    using cached_location_t = std::ptrdiff_t; // type used to store relative location (to allow for more efficient repeated access)
    cached_location_t cache_location(const difference_type& d)  const { return offset(d.x,d.y); }
    cached_location_t cache_location(x_coord_t dx, y_coord_t dy)const { return offset(dx,dy); }
    reference         operator[](const cached_location_t& loc)  const { return memunit_advanced_ref(x(),loc); }

    // Only make sense for memory-based locators
    std::ptrdiff_t         row_size()                           const { return memunit_step(y()); }    // distance in mem units (bytes or bits) between adjacent rows
    std::ptrdiff_t         pixel_size()                         const { return memunit_step(x()); }    // distance in mem units (bytes or bits) between adjacent pixels on the same row

    bool                   is_1d_traversable(x_coord_t width)   const { return row_size()-pixel_size()*width==0; }   // is there no gap at the end of each row?

    // Returns the vertical distance (it2.y-it1.y) between two x_iterators given the difference of their x positions
    std::ptrdiff_t y_distance_to(this_t const& p2, x_coord_t xDiff) const
    {
        std::ptrdiff_t rowDiff = memunit_distance(x(), p2.x()) - pixel_size() * xDiff;
        BOOST_ASSERT((rowDiff % row_size()) == 0);
        return rowDiff / row_size();
    }

private:
    template <typename X> friend class memory_based_2d_locator;
    std::ptrdiff_t offset(x_coord_t x, y_coord_t y)        const { return y*row_size() + x*pixel_size(); }
    StepIterator _p;
};

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename SI>
struct color_space_type<memory_based_2d_locator<SI> > : public color_space_type<typename memory_based_2d_locator<SI>::parent_t> {
};

template <typename SI>
struct channel_mapping_type<memory_based_2d_locator<SI> > : public channel_mapping_type<typename memory_based_2d_locator<SI>::parent_t> {
};

template <typename SI>
struct is_planar<memory_based_2d_locator<SI> > : public is_planar<typename memory_based_2d_locator<SI>::parent_t> {
};

template <typename SI>
struct channel_type<memory_based_2d_locator<SI> > : public channel_type<typename memory_based_2d_locator<SI>::parent_t> {
};

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

// Take the base iterator of SI (which is typically a step iterator) and change it to have a step in x
template <typename SI>
struct dynamic_x_step_type<memory_based_2d_locator<SI> > {
private:
    using base_iterator_t = typename iterator_adaptor_get_base<SI>::type;
    using base_iterator_step_t = typename dynamic_x_step_type<base_iterator_t>::type;
    using dynamic_step_base_t = typename iterator_adaptor_rebind<SI, base_iterator_step_t>::type;
public:
    using type = memory_based_2d_locator<dynamic_step_base_t>;
};

/////////////////////////////
//  HasDynamicYStepTypeConcept
/////////////////////////////

template <typename SI>
struct dynamic_y_step_type<memory_based_2d_locator<SI> > {
    using type = memory_based_2d_locator<SI>;
};
} }  // namespace boost::gil

#endif
