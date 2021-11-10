//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_CONCEPTS_IMAGE_VIEW_HPP
#define BOOST_GIL_CONCEPTS_IMAGE_VIEW_HPP

#include <boost/gil/concepts/basic.hpp>
#include <boost/gil/concepts/concept_check.hpp>
#include <boost/gil/concepts/fwd.hpp>
#include <boost/gil/concepts/pixel.hpp>
#include <boost/gil/concepts/pixel_dereference.hpp>
#include <boost/gil/concepts/pixel_iterator.hpp>
#include <boost/gil/concepts/pixel_locator.hpp>
#include <boost/gil/concepts/point.hpp>
#include <boost/gil/concepts/detail/utility.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

#if defined(BOOST_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedefs"
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

namespace boost { namespace gil {

/// \defgroup ImageViewNDConcept ImageViewNDLocatorConcept
/// \ingroup ImageViewConcept
/// \brief N-dimensional range

/// \defgroup ImageView2DConcept ImageView2DLocatorConcept
/// \ingroup ImageViewConcept
/// \brief 2-dimensional range

/// \defgroup PixelImageViewConcept ImageViewConcept
/// \ingroup ImageViewConcept
/// \brief 2-dimensional range over pixel data

/// \ingroup ImageViewNDConcept
/// \brief N-dimensional view over immutable values
///
/// \code
/// concept RandomAccessNDImageViewConcept<Regular View>
/// {
///     typename value_type;
///     typename reference;       // result of dereferencing
///     typename difference_type; // result of operator-(iterator,iterator) (1-dimensional!)
///     typename const_t;  where RandomAccessNDImageViewConcept<View>; // same as View, but over immutable values
///     typename point_t;  where PointNDConcept<point_t>; // N-dimensional point
///     typename locator;  where RandomAccessNDLocatorConcept<locator>; // N-dimensional locator.
///     typename iterator; where RandomAccessTraversalConcept<iterator>; // 1-dimensional iterator over all values
///     typename reverse_iterator; where RandomAccessTraversalConcept<reverse_iterator>;
///     typename size_type;       // the return value of size()
///
///     // Equivalent to RandomAccessNDLocatorConcept::axis
///     template <size_t D> struct axis {
///         typename coord_t = point_t::axis<D>::coord_t;
///         typename iterator; where RandomAccessTraversalConcept<iterator>;   // iterator along D-th axis.
///         where SameType<coord_t, iterator::difference_type>;
///         where SameType<iterator::value_type,value_type>;
///     };
///
///     // Defines the type of a view similar to this type, except it invokes Deref upon dereferencing
///     template <PixelDereferenceAdaptorConcept Deref> struct add_deref {
///         typename type;        where RandomAccessNDImageViewConcept<type>;
///         static type make(const View& v, const Deref& deref);
///     };
///
///     static const size_t num_dimensions = point_t::num_dimensions;
///
///     // Create from a locator at the top-left corner and dimensions
///     View::View(const locator&, const point_type&);
///
///     size_type        View::size()       const; // total number of elements
///     reference        operator[](View, const difference_type&) const; // 1-dimensional reference
///     iterator         View::begin()      const;
///     iterator         View::end()        const;
///     reverse_iterator View::rbegin()     const;
///     reverse_iterator View::rend()       const;
///     iterator         View::at(const point_t&);
///     point_t          View::dimensions() const; // number of elements along each dimension
///     bool             View::is_1d_traversable() const;   // can an iterator over the first dimension visit each value? I.e. are there gaps between values?
///
///     // iterator along a given dimension starting at a given point
///     template <size_t D> View::axis<D>::iterator View::axis_iterator(const point_t&) const;
///
///     reference operator()(View,const point_t&) const;
/// };
/// \endcode
template <typename View>
struct RandomAccessNDImageViewConcept
{
    void constraints()
    {
        gil_function_requires<Regular<View>>();

        using value_type = typename View::value_type;
        using reference = typename View::reference; // result of dereferencing
        using pointer = typename View::pointer;
        using difference_type = typename View::difference_type; // result of operator-(1d_iterator,1d_iterator)
        using const_t = typename View::const_t; // same as this type, but over const values
        using point_t = typename View::point_t; // N-dimensional point
        using locator = typename View::locator; // N-dimensional locator
        using iterator = typename View::iterator;
        using const_iterator = typename View::const_iterator;
        using reverse_iterator = typename View::reverse_iterator;
        using size_type = typename View::size_type;
        static const std::size_t N=View::num_dimensions;

        gil_function_requires<RandomAccessNDLocatorConcept<locator>>();
        gil_function_requires<boost_concepts::RandomAccessTraversalConcept<iterator>>();
        gil_function_requires<boost_concepts::RandomAccessTraversalConcept<reverse_iterator>>();

        using first_it_type = typename View::template axis<0>::iterator;
        using last_it_type = typename View::template axis<N-1>::iterator;
        gil_function_requires<boost_concepts::RandomAccessTraversalConcept<first_it_type>>();
        gil_function_requires<boost_concepts::RandomAccessTraversalConcept<last_it_type>>();

//        static_assert(typename std::iterator_traits<first_it_type>::difference_type, typename point_t::template axis<0>::coord_t>::value, "");
//        static_assert(typename std::iterator_traits<last_it_type>::difference_type, typename point_t::template axis<N-1>::coord_t>::value, "");

        // point_t must be an N-dimensional point, each dimension of which must have the same type as difference_type of the corresponding iterator
        gil_function_requires<PointNDConcept<point_t>>();
        static_assert(point_t::num_dimensions == N, "");
        static_assert(std::is_same
            <
                typename std::iterator_traits<first_it_type>::difference_type,
                typename point_t::template axis<0>::coord_t
            >::value, "");
        static_assert(std::is_same
            <
                typename std::iterator_traits<last_it_type>::difference_type,
                typename point_t::template axis<N-1>::coord_t
            >::value, "");

        point_t p;
        locator lc;
        iterator it;
        reverse_iterator rit;
        difference_type d; detail::initialize_it(d); ignore_unused_variable_warning(d);

        View(p,lc); // view must be constructible from a locator and a point

        p = view.dimensions();
        lc = view.pixels();
        size_type sz = view.size(); ignore_unused_variable_warning(sz);
        bool is_contiguous = view.is_1d_traversable();
        ignore_unused_variable_warning(is_contiguous);

        it = view.begin();
        it = view.end();
        rit = view.rbegin();
        rit = view.rend();

        reference r1 = view[d]; ignore_unused_variable_warning(r1); // 1D access
        reference r2 = view(p); ignore_unused_variable_warning(r2); // 2D access

        // get 1-D iterator of any dimension at a given pixel location
        first_it_type fi = view.template axis_iterator<0>(p);
        ignore_unused_variable_warning(fi);
        last_it_type li = view.template axis_iterator<N-1>(p);
        ignore_unused_variable_warning(li);

        using deref_t = PixelDereferenceAdaptorArchetype<typename View::value_type>;
        using dtype = typename View::template add_deref<deref_t>::type;
    }
    View view;
};

/// \ingroup ImageView2DConcept
/// \brief 2-dimensional view over immutable values
///
/// \code
/// concept RandomAccess2DImageViewConcept<RandomAccessNDImageViewConcept View> {
///     where num_dimensions==2;
///
///     typename x_iterator = axis<0>::iterator;
///     typename y_iterator = axis<1>::iterator;
///     typename x_coord_t  = axis<0>::coord_t;
///     typename y_coord_t  = axis<1>::coord_t;
///     typename xy_locator = locator;
///
///     x_coord_t View::width()  const;
///     y_coord_t View::height() const;
///
///     // X-navigation
///     x_iterator View::x_at(const point_t&) const;
///     x_iterator View::row_begin(y_coord_t) const;
///     x_iterator View::row_end  (y_coord_t) const;
///
///     // Y-navigation
///     y_iterator View::y_at(const point_t&) const;
///     y_iterator View::col_begin(x_coord_t) const;
///     y_iterator View::col_end  (x_coord_t) const;
///
///     // navigating in 2D
///     xy_locator View::xy_at(const point_t&) const;
///
///     // (x,y) versions of all methods taking point_t
///     View::View(x_coord_t,y_coord_t,const locator&);
///     iterator View::at(x_coord_t,y_coord_t) const;
///     reference operator()(View,x_coord_t,y_coord_t) const;
///     xy_locator View::xy_at(x_coord_t,y_coord_t) const;
///     x_iterator View::x_at(x_coord_t,y_coord_t) const;
///     y_iterator View::y_at(x_coord_t,y_coord_t) const;
/// };
/// \endcode
template <typename View>
struct RandomAccess2DImageViewConcept
{
    void constraints()
    {
        gil_function_requires<RandomAccessNDImageViewConcept<View>>();
        static_assert(View::num_dimensions == 2, "");

        // TODO: This executes the requirements for RandomAccessNDLocatorConcept again. Fix it to improve compile time
        gil_function_requires<RandomAccess2DLocatorConcept<typename View::locator>>();

        using dynamic_x_step_t = typename dynamic_x_step_type<View>::type;
        using dynamic_y_step_t = typename dynamic_y_step_type<View>::type;
        using transposed_t = typename transposed_type<View>::type;
        using x_iterator = typename View::x_iterator;
        using y_iterator = typename View::y_iterator;
        using x_coord_t = typename View::x_coord_t;
        using y_coord_t = typename View::y_coord_t;
        using xy_locator = typename View::xy_locator;

        x_coord_t xd = 0; ignore_unused_variable_warning(xd);
        y_coord_t yd = 0; ignore_unused_variable_warning(yd);
        x_iterator xit;
        y_iterator yit;
        typename View::point_t d;

        View(xd, yd, xy_locator()); // constructible with width, height, 2d_locator

        xy_locator lc = view.xy_at(xd, yd);
        lc = view.xy_at(d);

        typename View::reference r = view(xd, yd);
        ignore_unused_variable_warning(r);
        xd = view.width();
        yd = view.height();

        xit = view.x_at(d);
        xit = view.x_at(xd,yd);
        xit = view.row_begin(xd);
        xit = view.row_end(xd);

        yit = view.y_at(d);
        yit = view.y_at(xd,yd);
        yit = view.col_begin(xd);
        yit = view.col_end(xd);
    }
    View view;
};

/// \brief GIL view as Collection.
///
/// \see https://www.boost.org/libs/utility/Collection.html
///
template <typename View>
struct CollectionImageViewConcept
{
    void constraints()
    {
        using value_type = typename View::value_type;
        using iterator = typename View::iterator;
        using const_iterator =  typename View::const_iterator;
        using reference = typename View::reference;
        using const_reference = typename View::const_reference;
        using pointer = typename View::pointer;
        using difference_type = typename View::difference_type;
        using size_type=  typename View::size_type;

        iterator i;
        i = view1.begin();
        i = view2.end();

        const_iterator ci;
        ci = view1.begin();
        ci = view2.end();

        size_type s;
        s = view1.size();
        s = view2.size();
        ignore_unused_variable_warning(s);

        view1.empty();

        view1.swap(view2);
    }
    View view1;
    View view2;
};

/// \brief GIL view as ForwardCollection.
///
/// \see https://www.boost.org/libs/utility/Collection.html
///
template <typename View>
struct ForwardCollectionImageViewConcept
{
    void constraints()
    {
        gil_function_requires<CollectionImageViewConcept<View>>();

        using reference = typename View::reference;
        using const_reference = typename View::const_reference;

        reference r = view.front();
        ignore_unused_variable_warning(r);

        const_reference cr = view.front();
        ignore_unused_variable_warning(cr);
    }
    View view;
};

/// \brief GIL view as ReversibleCollection.
///
/// \see https://www.boost.org/libs/utility/Collection.html
///
template <typename View>
struct ReversibleCollectionImageViewConcept
{
    void constraints()
    {
        gil_function_requires<CollectionImageViewConcept<View>>();

        using reverse_iterator = typename View::reverse_iterator;
        using reference = typename View::reference;
        using const_reference = typename View::const_reference;

        reverse_iterator i;
        i = view.rbegin();
        i = view.rend();

        reference r = view.back();
        ignore_unused_variable_warning(r);

        const_reference cr = view.back();
        ignore_unused_variable_warning(cr);
    }
    View view;
};

/// \ingroup PixelImageViewConcept
/// \brief GIL's 2-dimensional view over immutable GIL pixels
/// \code
/// concept ImageViewConcept<RandomAccess2DImageViewConcept View>
/// {
///     where PixelValueConcept<value_type>;
///     where PixelIteratorConcept<x_iterator>;
///     where PixelIteratorConcept<y_iterator>;
///     where x_coord_t == y_coord_t;
///
///     typename coord_t = x_coord_t;
///
///     std::size_t View::num_channels() const;
/// };
/// \endcode
template <typename View>
struct ImageViewConcept
{
    void constraints()
    {
        gil_function_requires<RandomAccess2DImageViewConcept<View>>();

        // TODO: This executes the requirements for RandomAccess2DLocatorConcept again. Fix it to improve compile time
        gil_function_requires<PixelLocatorConcept<typename View::xy_locator>>();

        static_assert(std::is_same<typename View::x_coord_t, typename View::y_coord_t>::value, "");

        using coord_t = typename View::coord_t; // 1D difference type (same for all dimensions)
        std::size_t num_chan = view.num_channels(); ignore_unused_variable_warning(num_chan);
    }
    View view;
};

namespace detail {

/// \tparam View Models RandomAccessNDImageViewConcept
template <typename View>
struct RandomAccessNDImageViewIsMutableConcept
{
    void constraints()
    {
        gil_function_requires<detail::RandomAccessNDLocatorIsMutableConcept<typename View::locator>>();

        gil_function_requires<detail::RandomAccessIteratorIsMutableConcept<typename View::iterator>>();

        gil_function_requires<detail::RandomAccessIteratorIsMutableConcept
            <
                typename View::reverse_iterator
            >>();

        gil_function_requires<detail::RandomAccessIteratorIsMutableConcept
            <
                typename View::template axis<0>::iterator
            >>();

        gil_function_requires<detail::RandomAccessIteratorIsMutableConcept
            <
                typename View::template axis<View::num_dimensions - 1>::iterator
            >>();

        typename View::difference_type diff;
        initialize_it(diff);
        ignore_unused_variable_warning(diff);

        typename View::point_t pt;
        typename View::value_type v;
        initialize_it(v);

        view[diff] = v;
        view(pt) = v;
    }
    View view;
};

/// \tparam View Models RandomAccessNDImageViewConcept
template <typename View>
struct RandomAccess2DImageViewIsMutableConcept
{
    void constraints()
    {
        gil_function_requires<detail::RandomAccessNDImageViewIsMutableConcept<View>>();
        typename View::x_coord_t xd = 0; ignore_unused_variable_warning(xd);
        typename View::y_coord_t yd = 0; ignore_unused_variable_warning(yd);
        typename View::value_type v; initialize_it(v);
        view(xd, yd) = v;
    }
    View view;
};

/// \tparam View Models ImageViewConcept
template <typename View>
struct PixelImageViewIsMutableConcept
{
    void constraints()
    {
        gil_function_requires<detail::RandomAccess2DImageViewIsMutableConcept<View>>();
    }
};

} // namespace detail

/// \ingroup ImageViewNDConcept
/// \brief N-dimensional view over mutable values
///
/// \code
/// concept MutableRandomAccessNDImageViewConcept<RandomAccessNDImageViewConcept View>
/// {
///     where Mutable<reference>;
/// };
/// \endcode
template <typename View>
struct MutableRandomAccessNDImageViewConcept
{
    void constraints()
    {
        gil_function_requires<RandomAccessNDImageViewConcept<View>>();
        gil_function_requires<detail::RandomAccessNDImageViewIsMutableConcept<View>>();
    }
};

/// \ingroup ImageView2DConcept
/// \brief 2-dimensional view over mutable values
///
/// \code
/// concept MutableRandomAccess2DImageViewConcept<RandomAccess2DImageViewConcept View>
///     : MutableRandomAccessNDImageViewConcept<View> {};
/// \endcode
template <typename View>
struct MutableRandomAccess2DImageViewConcept
{
    void constraints()
    {
        gil_function_requires<RandomAccess2DImageViewConcept<View>>();
        gil_function_requires<detail::RandomAccess2DImageViewIsMutableConcept<View>>();
    }
};

/// \ingroup PixelImageViewConcept
/// \brief GIL's 2-dimensional view over mutable GIL pixels
///
/// \code
/// concept MutableImageViewConcept<ImageViewConcept View>
///     : MutableRandomAccess2DImageViewConcept<View> {};
/// \endcode
template <typename View>
struct MutableImageViewConcept
{
    void constraints()
    {
        gil_function_requires<ImageViewConcept<View>>();
        gil_function_requires<detail::PixelImageViewIsMutableConcept<View>>();
    }
};

/// \brief Returns whether two views are compatible
///
/// Views are compatible if their pixels are compatible.
/// Compatible views can be assigned and copy constructed from one another.
///
/// \tparam V1 Models ImageViewConcept
/// \tparam V2 Models ImageViewConcept
///
template <typename V1, typename V2>
struct views_are_compatible
    : pixels_are_compatible<typename V1::value_type, typename V2::value_type>
{
};

/// \ingroup ImageViewConcept
/// \brief Views are compatible if they have the same color spaces and compatible channel values.
///
/// Constness and layout are not important for compatibility.
///
/// \code
/// concept ViewsCompatibleConcept<ImageViewConcept V1, ImageViewConcept V2>
/// {
///     where PixelsCompatibleConcept<V1::value_type, P2::value_type>;
/// };
/// \endcode
template <typename V1, typename V2>
struct ViewsCompatibleConcept
{
    void constraints()
    {
        static_assert(views_are_compatible<V1, V2>::value, "");
    }
};

}} // namespace boost::gil

#if defined(BOOST_CLANG)
#pragma clang diagnostic pop
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic pop
#endif

#endif
