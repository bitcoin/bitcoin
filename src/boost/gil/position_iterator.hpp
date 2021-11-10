//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_POSITION_ITERATOR_HPP
#define BOOST_GIL_POSITION_ITERATOR_HPP

#include <boost/gil/locator.hpp>

#include <boost/iterator/iterator_facade.hpp>

#include <type_traits>

namespace boost { namespace gil {

/// \defgroup PixelIteratorModelVirtual position_iterator
/// \ingroup PixelIteratorModel
/// \brief An iterator that remembers its current X,Y position and invokes a function object with it upon dereferencing.
/// Models PixelIteratorConcept, PixelBasedConcept, HasDynamicXStepTypeConcept. Used to create virtual image views.

/// \brief An iterator that remembers its current X,Y position and invokes a function object with it upon dereferencing.
/// Used to create virtual image views.
/// Models: StepIteratorConcept, PixelIteratorConcept, PixelBasedConcept, HasDynamicXStepTypeConcept
/// \ingroup PixelIteratorModelVirtual PixelBasedModel
/// \tparam Deref A function object that given a point returns a pixel reference. Models PixelDereferenceAdaptorConcept
/// \tparam Dim Dimension to advance along
template <typename Deref, int Dim>
struct position_iterator : public iterator_facade<position_iterator<Deref,Dim>,
                                                  typename Deref::value_type,
                                                  std::random_access_iterator_tag,
                                                  typename Deref::reference,
                                                  typename Deref::argument_type::template axis<Dim>::coord_t> {
    using parent_t = iterator_facade<position_iterator<Deref,Dim>,
                            typename Deref::value_type,
                            std::random_access_iterator_tag,
                            typename Deref::reference,
                            typename Deref::argument_type::template axis<Dim>::coord_t>;
    using difference_type = typename parent_t::difference_type;
    using reference = typename parent_t::reference;
    using point_t = typename Deref::argument_type;

    position_iterator() {}
    position_iterator(const point_t& p, const point_t& step, const Deref& d) : _p(p), _step(step), _d(d) {}

    position_iterator(const position_iterator& p) : _p(p._p), _step(p._step), _d(p._d) {}
    template <typename D> position_iterator(const position_iterator<D,Dim>& p) : _p(p._p), _step(p._step), _d(p._d) {}
    position_iterator& operator=(const position_iterator& p) { _p=p._p; _d=p._d; _step=p._step; return *this; }

    const point_t&   pos()      const { return _p; }
    const point_t&   step()     const { return _step; }
    const Deref&     deref_fn() const { return _d; }

    void set_step(difference_type s) { _step[Dim]=s; }
    /// For some reason operator[] provided by iterator_adaptor returns a custom class that is convertible to reference
    /// We require our own reference because it is registered in iterator_traits
    reference operator[](difference_type d) const { point_t p=_p; p[Dim]+=d*_step[Dim]; return _d(p); }

private:
    point_t _p, _step;
    Deref   _d;

    template <typename DE, int DI> friend struct position_iterator;
    friend class boost::iterator_core_access;
    reference dereference()     const { return _d(_p); }
    void increment()                  { _p[Dim]+=_step[Dim]; }
    void decrement()                  { _p[Dim]-=_step[Dim]; }
    void advance(difference_type d)   { _p[Dim]+=d*_step[Dim]; }

    difference_type distance_to(const position_iterator& it) const { return (it._p[Dim]-_p[Dim])/_step[Dim]; }
    bool equal(const position_iterator& it) const { return _p==it._p; }
};

template <typename Deref,int Dim>
struct const_iterator_type<position_iterator<Deref,Dim> > {
    using type = position_iterator<typename Deref::const_t,Dim>;
};

template <typename Deref, int Dim>
struct iterator_is_mutable<position_iterator<Deref, Dim>>
    : std::integral_constant<bool, Deref::is_mutable>
{
};

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename Deref,int Dim>
struct color_space_type<position_iterator<Deref,Dim> > : public color_space_type<typename Deref::value_type> {};

template <typename Deref,int Dim>
struct channel_mapping_type<position_iterator<Deref,Dim> > : public channel_mapping_type<typename Deref::value_type> {};

template <typename Deref,int Dim>
struct is_planar<position_iterator<Deref, Dim>> : std::false_type {};

template <typename Deref,int Dim>
struct channel_type<position_iterator<Deref,Dim> > : public channel_type<typename Deref::value_type> {};

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename Deref,int Dim>
struct dynamic_x_step_type<position_iterator<Deref,Dim> > {
    using type = position_iterator<Deref,Dim>;
};

} }  // namespace boost::gil

#endif
