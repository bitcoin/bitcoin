//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_VIRTUAL_LOCATOR_HPP
#define BOOST_GIL_VIRTUAL_LOCATOR_HPP

#include <boost/gil/dynamic_step.hpp>
#include <boost/gil/position_iterator.hpp>

#include <boost/assert.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace boost { namespace gil {

/// \ingroup PixelLocatorModel PixelBasedModel
/// \brief A 2D locator over a virtual image
/// Upon dereferencing, invokes a given function object passing it its coordinates.
/// Models:
///   PixelLocatorConcept,
///   HasDynamicXStepTypeConcept,
///   HasDynamicYStepTypeConcept,
///   HasTransposedTypeConcept
///
/// \tparam DerefFn Function object that given a point returns a reference.
///         Models PixelDereferenceAdaptorConcept.
/// \tparam IsTransposed Indicates if locator should navigate in transposed mode.
template <typename DerefFn, bool IsTransposed>
class virtual_2d_locator
    : public pixel_2d_locator_base
        <
            virtual_2d_locator<DerefFn, IsTransposed>,
            position_iterator<DerefFn, IsTransposed>,
            position_iterator<DerefFn, 1-IsTransposed>
        >
{
    using this_t = virtual_2d_locator<DerefFn, IsTransposed>;
public:
    using parent_t = pixel_2d_locator_base
        <
            virtual_2d_locator<DerefFn, IsTransposed>,
            position_iterator<DerefFn, IsTransposed>,
            position_iterator<DerefFn, 1-IsTransposed>
        >;
    using const_t = virtual_2d_locator<typename DerefFn::const_t, IsTransposed>;
    using deref_fn_t = DerefFn;
    using point_t = typename parent_t::point_t;
    using coord_t = typename parent_t::coord_t;
    using x_coord_t = typename parent_t::x_coord_t;
    using y_coord_t = typename parent_t::y_coord_t;
    using x_iterator = typename parent_t::x_iterator;
    using y_iterator = typename parent_t::y_iterator;

    template <typename NewDerefFn>
    struct add_deref
    {
        using type = virtual_2d_locator<deref_compose<NewDerefFn, DerefFn>, IsTransposed>;

        static type make(this_t const& loc, NewDerefFn const& new_deref_fn)
        {
            return type(loc.pos(), loc.step(),
                deref_compose<NewDerefFn, DerefFn>(new_deref_fn, loc.deref_fn()));
        }
    };

    virtual_2d_locator(
        point_t const& p = {0, 0},
        point_t const& step = {1, 1},
        deref_fn_t const& deref_fn = deref_fn_t())
        : y_pos_(p, step, deref_fn)
    {}

    template <typename D, bool TR>
    virtual_2d_locator(virtual_2d_locator<D, TR> const &loc, coord_t y_step)
        : y_pos_(loc.pos(), point_t(loc.step().x, loc.step().y * y_step), loc.deref_fn())
    {}

    template <typename D, bool TR>
    virtual_2d_locator(virtual_2d_locator<D, TR> const& loc, coord_t x_step, coord_t y_step, bool transpose = false)
        : y_pos_(loc.pos()
        , transpose ?
            point_t(loc.step().x * y_step, loc.step().y * x_step) :
            point_t(loc.step().x * x_step, loc.step().y * y_step)
        , loc.deref_fn())
    {
        BOOST_ASSERT(transpose == (IsTransposed != TR));
    }

    template <typename D, bool TR>
    virtual_2d_locator(virtual_2d_locator<D, TR> const& other) : y_pos_(other.y_pos_) {}

    virtual_2d_locator(virtual_2d_locator const& other) : y_pos_(other.y_pos_) {}
    virtual_2d_locator& operator=(virtual_2d_locator const& other) = default;

    bool operator==(const this_t& p) const { return y_pos_ == p.y_pos_; }

    auto x() -> x_iterator&
    {
        return *gil_reinterpret_cast<x_iterator*>(this);
    }

    auto x() const -> x_iterator const&
    {
        return *gil_reinterpret_cast_c<x_iterator const*>(this);
    }

    auto y() -> y_iterator& { return y_pos_; }
    auto y() const -> y_iterator const& { return y_pos_; }

    /// Returns the y distance between two x_iterators given the difference of their x positions
    auto y_distance_to(this_t const& it2, x_coord_t) const -> y_coord_t
    {
        return (it2.pos()[1 - IsTransposed] - pos()[1 - IsTransposed])
                / step()[1 - IsTransposed];
    }

    /// \todo TODO: is there no gap at the end of each row?
    ///       i.e. can we use x_iterator to visit every pixel instead of nested loops?
    bool is_1d_traversable(x_coord_t) const { return false; }

    // Methods specific for virtual 2D locator
    auto pos() const -> point_t const& { return y_pos_.pos(); }
    auto step() const -> point_t const& { return y_pos_.step(); }
    auto deref_fn() const -> deref_fn_t const& { return y_pos_.deref_fn(); }

private:
    template <typename D, bool TR>
    friend class virtual_2d_locator;

    y_iterator y_pos_; // current position, the step and the dereference object
};

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename D, bool TR>
struct channel_type<virtual_2d_locator<D, TR>>
    : channel_type<typename virtual_2d_locator<D, TR>::parent_t>
{
};

template <typename D, bool TR>
struct color_space_type<virtual_2d_locator<D, TR>>
    : color_space_type<typename virtual_2d_locator<D, TR>::parent_t>
{
};

template <typename D, bool TR>
struct channel_mapping_type<virtual_2d_locator<D, TR>>
    : channel_mapping_type<typename virtual_2d_locator<D, TR>::parent_t>
{
};

template <typename D, bool TR>
struct is_planar<virtual_2d_locator<D, TR>>
    : is_planar<typename virtual_2d_locator<D, TR>::parent_t>
{
};

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename D, bool TR>
struct dynamic_x_step_type<virtual_2d_locator<D,TR>>
{
    using type = virtual_2d_locator<D,TR>;
};

/////////////////////////////
//  HasDynamicYStepTypeConcept
/////////////////////////////

template <typename D, bool TR>
struct dynamic_y_step_type<virtual_2d_locator<D,TR>>
{
    using type = virtual_2d_locator<D,TR>;
};

/////////////////////////////
//  HasTransposedTypeConcept
/////////////////////////////

template <typename D, bool IsTransposed>
struct transposed_type<virtual_2d_locator<D,IsTransposed>>
{
    using type = virtual_2d_locator<D,1-IsTransposed>;
};

}}  // namespace boost::gil

#endif
