// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP


#include <cstddef>
#include <type_traits>
#include <vector>

#include <boost/range/begin.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>


namespace boost { namespace geometry
{

namespace detail { namespace partition
{

template <typename T, bool IsIntegral = std::is_integral<T>::value>
struct divide_interval
{
    static inline T apply(T const& mi, T const& ma)
    {
        static T const two = 2;
        return (mi + ma) / two;
    }
};

template <typename T>
struct divide_interval<T, true>
{
    static inline T apply(T const& mi, T const& ma)
    {
        // avoid overflow
        return mi / 2 + ma / 2 + (mi % 2 + ma % 2) / 2;
    }
};

template <int Dimension, typename Box>
inline void divide_box(Box const& box, Box& lower_box, Box& upper_box)
{
    typedef typename coordinate_type<Box>::type ctype;

    // Divide input box into two parts, e.g. left/right
    ctype mid = divide_interval<ctype>::apply(
                    geometry::get<min_corner, Dimension>(box),
                    geometry::get<max_corner, Dimension>(box));

    lower_box = box;
    upper_box = box;
    geometry::set<max_corner, Dimension>(lower_box, mid);
    geometry::set<min_corner, Dimension>(upper_box, mid);
}

// Divide forward_range into three subsets: lower, upper and oversized
// (not-fitting)
// (lower == left or bottom, upper == right or top)
template <typename Box, typename IteratorVector, typename OverlapsPolicy>
inline void divide_into_subsets(Box const& lower_box,
                                Box const& upper_box,
                                IteratorVector const& input,
                                IteratorVector& lower,
                                IteratorVector& upper,
                                IteratorVector& exceeding,
                                OverlapsPolicy const& overlaps_policy)
{
    typedef typename boost::range_iterator
        <
            IteratorVector const
        >::type it_type;

    for(it_type it = boost::begin(input); it != boost::end(input); ++it)
    {
        bool const lower_overlapping = overlaps_policy.apply(lower_box, **it);
        bool const upper_overlapping = overlaps_policy.apply(upper_box, **it);

        if (lower_overlapping && upper_overlapping)
        {
            exceeding.push_back(*it);
        }
        else if (lower_overlapping)
        {
            lower.push_back(*it);
        }
        else if (upper_overlapping)
        {
            upper.push_back(*it);
        }
        else
        {
            // Is nowhere. That is (since 1.58) possible, it might be
            // skipped by the OverlapsPolicy to enhance performance
        }
    }
}

template
<
    typename Box,
    typename IteratorVector,
    typename ExpandPolicy
>
inline void expand_with_elements(Box& total, IteratorVector const& input,
                                 ExpandPolicy const& expand_policy)
{
    typedef typename boost::range_iterator<IteratorVector const>::type it_type;
    for(it_type it = boost::begin(input); it != boost::end(input); ++it)
    {
        expand_policy.apply(total, **it);
    }
}


// Match forward_range with itself
template <typename IteratorVector, typename VisitPolicy>
inline bool handle_one(IteratorVector const& input, VisitPolicy& visitor)
{
    if (boost::empty(input))
    {
        return true;
    }

    typedef typename boost::range_iterator<IteratorVector const>::type it_type;

    // Quadratic behaviour at lowest level (lowest quad, or all exceeding)
    for (it_type it1 = boost::begin(input); it1 != boost::end(input); ++it1)
    {
        it_type it2 = it1;
        for (++it2; it2 != boost::end(input); ++it2)
        {
            if (! visitor.apply(**it1, **it2))
            {
                return false; // interrupt
            }
        }
    }

    return true;
}

// Match forward range 1 with forward range 2
template
<
    typename IteratorVector1,
    typename IteratorVector2,
    typename VisitPolicy
>
inline bool handle_two(IteratorVector1 const& input1,
                       IteratorVector2 const& input2,
                       VisitPolicy& visitor)
{
    typedef typename boost::range_iterator
        <
            IteratorVector1 const
        >::type iterator_type1;

    typedef typename boost::range_iterator
        <
            IteratorVector2 const
        >::type iterator_type2;

    if (boost::empty(input1) || boost::empty(input2))
    {
        return true;
    }

    for(iterator_type1 it1 = boost::begin(input1);
        it1 != boost::end(input1);
        ++it1)
    {
        for(iterator_type2 it2 = boost::begin(input2);
            it2 != boost::end(input2);
            ++it2)
        {
            if (! visitor.apply(**it1, **it2))
            {
                return false; // interrupt
            }
        }
    }

    return true;
}

template <typename IteratorVector>
inline bool recurse_ok(IteratorVector const& input,
                       std::size_t min_elements, std::size_t level)
{
    return boost::size(input) >= min_elements
        && level < 100;
}

template <typename IteratorVector1, typename IteratorVector2>
inline bool recurse_ok(IteratorVector1 const& input1,
                       IteratorVector2 const& input2,
                       std::size_t min_elements, std::size_t level)
{
    return boost::size(input1) >= min_elements
        && recurse_ok(input2, min_elements, level);
}

template
<
    typename IteratorVector1,
    typename IteratorVector2,
    typename IteratorVector3
>
inline bool recurse_ok(IteratorVector1 const& input1,
                       IteratorVector2 const& input2,
                       IteratorVector3 const& input3,
                       std::size_t min_elements, std::size_t level)
{
    return boost::size(input1) >= min_elements
        && recurse_ok(input2, input3, min_elements, level);
}


template <int Dimension, typename Box>
class partition_two_ranges;


template <int Dimension, typename Box>
class partition_one_range
{
    template <typename IteratorVector, typename ExpandPolicy>
    static inline Box get_new_box(IteratorVector const& input,
                                  ExpandPolicy const& expand_policy)
    {
        Box box;
        geometry::assign_inverse(box);
        expand_with_elements(box, input, expand_policy);
        return box;
    }

    template
    <
        typename IteratorVector,
        typename VisitPolicy,
        typename ExpandPolicy,
        typename OverlapsPolicy,
        typename VisitBoxPolicy
    >
    static inline bool next_level(Box const& box,
                                  IteratorVector const& input,
                                  std::size_t level, std::size_t min_elements,
                                  VisitPolicy& visitor,
                                  ExpandPolicy const& expand_policy,
                                  OverlapsPolicy const& overlaps_policy,
                                  VisitBoxPolicy& box_policy)
    {
        if (recurse_ok(input, min_elements, level))
        {
            return partition_one_range
                <
                    1 - Dimension,
                    Box
                >::apply(box, input, level + 1, min_elements,
                         visitor, expand_policy, overlaps_policy, box_policy);
        }
        else
        {
            return handle_one(input, visitor);
        }
    }

    // Function to switch to two forward ranges if there are
    // geometries exceeding the separation line
    template
    <
        typename IteratorVector,
        typename VisitPolicy,
        typename ExpandPolicy,
        typename OverlapsPolicy,
        typename VisitBoxPolicy
    >
    static inline bool next_level2(Box const& box,
                                   IteratorVector const& input1,
                                   IteratorVector const& input2,
                                   std::size_t level, std::size_t min_elements,
                                   VisitPolicy& visitor,
                                   ExpandPolicy const& expand_policy,
                                   OverlapsPolicy const& overlaps_policy,
                                   VisitBoxPolicy& box_policy)
    {
        if (recurse_ok(input1, input2, min_elements, level))
        {
            return partition_two_ranges
                <
                    1 - Dimension, Box
                >::apply(box, input1, input2, level + 1, min_elements,
                         visitor, expand_policy, overlaps_policy,
                         expand_policy, overlaps_policy, box_policy);
        }
        else
        {
            return handle_two(input1, input2, visitor);
        }
    }

public :
    template
    <
        typename IteratorVector,
        typename VisitPolicy,
        typename ExpandPolicy,
        typename OverlapsPolicy,
        typename VisitBoxPolicy
    >
    static inline bool apply(Box const& box,
                             IteratorVector const& input,
                             std::size_t level,
                             std::size_t min_elements,
                             VisitPolicy& visitor,
                             ExpandPolicy const& expand_policy,
                             OverlapsPolicy const& overlaps_policy,
                             VisitBoxPolicy& box_policy)
    {
        box_policy.apply(box, level);

        Box lower_box, upper_box;
        divide_box<Dimension>(box, lower_box, upper_box);

        IteratorVector lower, upper, exceeding;
        divide_into_subsets(lower_box, upper_box,
                            input, lower, upper, exceeding,
                            overlaps_policy);

        if (! boost::empty(exceeding))
        {
            // Get the box of exceeding-only
            Box exceeding_box = get_new_box(exceeding, expand_policy);

                   // Recursively do exceeding elements only, in next dimension they
                   // will probably be less exceeding within the new box
            if (! (next_level(exceeding_box, exceeding, level, min_elements,
                              visitor, expand_policy, overlaps_policy, box_policy)
                   // Switch to two forward ranges, combine exceeding with
                   // lower resp upper, but not lower/lower, upper/upper
                && next_level2(exceeding_box, exceeding, lower, level, min_elements,
                               visitor, expand_policy, overlaps_policy, box_policy)
                && next_level2(exceeding_box, exceeding, upper, level, min_elements,
                               visitor, expand_policy, overlaps_policy, box_policy)) )
            {
                return false; // interrupt
            }
        }

        // Recursively call operation both parts
        return next_level(lower_box, lower, level, min_elements,
                          visitor, expand_policy, overlaps_policy, box_policy)
            && next_level(upper_box, upper, level, min_elements,
                          visitor, expand_policy, overlaps_policy, box_policy);
    }
};

template
<
    int Dimension,
    typename Box
>
class partition_two_ranges
{
    template
    <
        typename IteratorVector1,
        typename IteratorVector2,
        typename VisitPolicy,
        typename ExpandPolicy1,
        typename OverlapsPolicy1,
        typename ExpandPolicy2,
        typename OverlapsPolicy2,
        typename VisitBoxPolicy
    >
    static inline bool next_level(Box const& box,
                                  IteratorVector1 const& input1,
                                  IteratorVector2 const& input2,
                                  std::size_t level, std::size_t min_elements,
                                  VisitPolicy& visitor,
                                  ExpandPolicy1 const& expand_policy1,
                                  OverlapsPolicy1 const& overlaps_policy1,
                                  ExpandPolicy2 const& expand_policy2,
                                  OverlapsPolicy2 const& overlaps_policy2,
                                  VisitBoxPolicy& box_policy)
    {
        return partition_two_ranges
            <
                1 - Dimension, Box
            >::apply(box, input1, input2, level + 1, min_elements,
                     visitor, expand_policy1, overlaps_policy1,
                     expand_policy2, overlaps_policy2, box_policy);
    }

    template <typename IteratorVector, typename ExpandPolicy>
    static inline Box get_new_box(IteratorVector const& input,
                                  ExpandPolicy const& expand_policy)
    {
        Box box;
        geometry::assign_inverse(box);
        expand_with_elements(box, input, expand_policy);
        return box;
    }

    template
    <
        typename IteratorVector1, typename IteratorVector2,
        typename ExpandPolicy1, typename ExpandPolicy2
    >
    static inline Box get_new_box(IteratorVector1 const& input1,
                                  IteratorVector2 const& input2,
                                  ExpandPolicy1 const& expand_policy1,
                                  ExpandPolicy2 const& expand_policy2)
    {
        Box box = get_new_box(input1, expand_policy1);
        expand_with_elements(box, input2, expand_policy2);
        return box;
    }

public :
    template
    <
        typename IteratorVector1,
        typename IteratorVector2,
        typename VisitPolicy,
        typename ExpandPolicy1,
        typename OverlapsPolicy1,
        typename ExpandPolicy2,
        typename OverlapsPolicy2,
        typename VisitBoxPolicy
    >
    static inline bool apply(Box const& box,
                             IteratorVector1 const& input1,
                             IteratorVector2 const& input2,
                             std::size_t level,
                             std::size_t min_elements,
                             VisitPolicy& visitor,
                             ExpandPolicy1 const& expand_policy1,
                             OverlapsPolicy1 const& overlaps_policy1,
                             ExpandPolicy2 const& expand_policy2,
                             OverlapsPolicy2 const& overlaps_policy2,
                             VisitBoxPolicy& box_policy)
    {
        box_policy.apply(box, level);

        Box lower_box, upper_box;
        divide_box<Dimension>(box, lower_box, upper_box);

        IteratorVector1 lower1, upper1, exceeding1;
        IteratorVector2 lower2, upper2, exceeding2;
        divide_into_subsets(lower_box, upper_box,
                            input1, lower1, upper1, exceeding1,
                            overlaps_policy1);
        divide_into_subsets(lower_box, upper_box,
                            input2, lower2, upper2, exceeding2,
                            overlaps_policy2);

        if (! boost::empty(exceeding1))
        {
            // All exceeding from 1 with 2:

            if (recurse_ok(exceeding1, exceeding2, min_elements, level))
            {
                Box exceeding_box = get_new_box(exceeding1, exceeding2,
                                                expand_policy1, expand_policy2);
                if (! next_level(exceeding_box, exceeding1, exceeding2, level,
                                 min_elements, visitor, expand_policy1, overlaps_policy1,
                                 expand_policy2, overlaps_policy2, box_policy))
                {
                    return false; // interrupt
                }
            }
            else
            {
                if (! handle_two(exceeding1, exceeding2, visitor))
                {
                    return false; // interrupt
                }
            }

            // All exceeding from 1 with lower and upper of 2:

            // (Check sizes of all three forward ranges to avoid recurse into
            // the same combinations again and again)
            if (recurse_ok(lower2, upper2, exceeding1, min_elements, level))
            {
                Box exceeding_box = get_new_box(exceeding1, expand_policy1);
                if (! (next_level(exceeding_box, exceeding1, lower2, level,
                                  min_elements, visitor, expand_policy1, overlaps_policy1,
                                  expand_policy2, overlaps_policy2, box_policy)
                    && next_level(exceeding_box, exceeding1, upper2, level,
                                  min_elements, visitor, expand_policy1, overlaps_policy1,
                                  expand_policy2, overlaps_policy2, box_policy)) )
                {
                    return false; // interrupt
                }
            }
            else
            {
                if (! (handle_two(exceeding1, lower2, visitor)
                    && handle_two(exceeding1, upper2, visitor)) )
                {
                    return false; // interrupt
                }
            }
        }

        if (! boost::empty(exceeding2))
        {
            // All exceeding from 2 with lower and upper of 1:
            if (recurse_ok(lower1, upper1, exceeding2, min_elements, level))
            {
                Box exceeding_box = get_new_box(exceeding2, expand_policy2);
                if (! (next_level(exceeding_box, lower1, exceeding2, level,
                                  min_elements, visitor, expand_policy1, overlaps_policy1,
                                  expand_policy2, overlaps_policy2, box_policy)
                    && next_level(exceeding_box, upper1, exceeding2, level,
                                  min_elements, visitor, expand_policy1, overlaps_policy1,
                                  expand_policy2, overlaps_policy2, box_policy)) )
                {
                    return false; // interrupt
                }
            }
            else
            {
                if (! (handle_two(lower1, exceeding2, visitor)
                    && handle_two(upper1, exceeding2, visitor)) )
                {
                    return false; // interrupt
                }
            }
        }

        if (recurse_ok(lower1, lower2, min_elements, level))
        {
            if (! next_level(lower_box, lower1, lower2, level,
                             min_elements, visitor, expand_policy1, overlaps_policy1,
                             expand_policy2, overlaps_policy2, box_policy) )
            {
                return false; // interrupt
            }
        }
        else
        {
            if (! handle_two(lower1, lower2, visitor))
            {
                return false; // interrupt
            }
        }

        if (recurse_ok(upper1, upper2, min_elements, level))
        {
            if (! next_level(upper_box, upper1, upper2, level,
                             min_elements, visitor, expand_policy1, overlaps_policy1,
                             expand_policy2, overlaps_policy2, box_policy) )
            {
                return false; // interrupt
            }
        }
        else
        {
            if (! handle_two(upper1, upper2, visitor))
            {
                return false; // interrupt
            }
        }

        return true;
    }
};

struct visit_no_policy
{
    template <typename Box>
    static inline void apply(Box const&, std::size_t )
    {}
};

struct include_all_policy
{
    template <typename Item>
    static inline bool apply(Item const&)
    {
        return true;
    }
};


}} // namespace detail::partition

template
<
    typename Box,
    typename IncludePolicy1 = detail::partition::include_all_policy,
    typename IncludePolicy2 = detail::partition::include_all_policy
>
class partition
{
    static const std::size_t default_min_elements = 16;

    template
    <
        typename IncludePolicy,
        typename ForwardRange,
        typename IteratorVector,
        typename ExpandPolicy
    >
    static inline void expand_to_range(ForwardRange const& forward_range,
                                       Box& total,
                                       IteratorVector& iterator_vector,
                                       ExpandPolicy const& expand_policy)
    {
        for(typename boost::range_iterator<ForwardRange const>::type
                it = boost::begin(forward_range);
            it != boost::end(forward_range);
            ++it)
        {
            if (IncludePolicy::apply(*it))
            {
                expand_policy.apply(total, *it);
                iterator_vector.push_back(it);
            }
        }
    }

public:
    template
    <
        typename ForwardRange,
        typename VisitPolicy,
        typename ExpandPolicy,
        typename OverlapsPolicy
    >
    static inline bool apply(ForwardRange const& forward_range,
                             VisitPolicy& visitor,
                             ExpandPolicy const& expand_policy,
                             OverlapsPolicy const& overlaps_policy)
    {
        return apply(forward_range, visitor, expand_policy, overlaps_policy,
                     default_min_elements, detail::partition::visit_no_policy());
    }

    template
    <
        typename ForwardRange,
        typename VisitPolicy,
        typename ExpandPolicy,
        typename OverlapsPolicy
    >
    static inline bool apply(ForwardRange const& forward_range,
                             VisitPolicy& visitor,
                             ExpandPolicy const& expand_policy,
                             OverlapsPolicy const& overlaps_policy,
                             std::size_t min_elements)
    {
        return apply(forward_range, visitor, expand_policy, overlaps_policy,
                     min_elements, detail::partition::visit_no_policy());
    }

    template
    <
        typename ForwardRange,
        typename VisitPolicy,
        typename ExpandPolicy,
        typename OverlapsPolicy,
        typename VisitBoxPolicy
    >
    static inline bool apply(ForwardRange const& forward_range,
                             VisitPolicy& visitor,
                             ExpandPolicy const& expand_policy,
                             OverlapsPolicy const& overlaps_policy,
                             std::size_t min_elements,
                             VisitBoxPolicy box_visitor)
    {
        typedef typename boost::range_iterator
            <
                ForwardRange const
            >::type iterator_type;

        if (std::size_t(boost::size(forward_range)) > min_elements)
        {
            std::vector<iterator_type> iterator_vector;
            Box total;
            assign_inverse(total);
            expand_to_range<IncludePolicy1>(forward_range, total,
                                            iterator_vector, expand_policy);

            return detail::partition::partition_one_range
                <
                    0, Box
                >::apply(total, iterator_vector, 0, min_elements,
                         visitor, expand_policy, overlaps_policy, box_visitor);
        }
        else
        {
            for(iterator_type it1 = boost::begin(forward_range);
                it1 != boost::end(forward_range);
                ++it1)
            {
                iterator_type it2 = it1;
                for(++it2; it2 != boost::end(forward_range); ++it2)
                {
                    if (! visitor.apply(*it1, *it2))
                    {
                        return false; // interrupt
                    }
                }
            }
        }

        return true;
    }

    template
    <
        typename ForwardRange1,
        typename ForwardRange2,
        typename VisitPolicy,
        typename ExpandPolicy1,
        typename OverlapsPolicy1
    >
    static inline bool apply(ForwardRange1 const& forward_range1,
                             ForwardRange2 const& forward_range2,
                             VisitPolicy& visitor,
                             ExpandPolicy1 const& expand_policy1,
                             OverlapsPolicy1 const& overlaps_policy1)
    {
        return apply(forward_range1, forward_range2, visitor,
                     expand_policy1, overlaps_policy1, expand_policy1, overlaps_policy1,
                     default_min_elements, detail::partition::visit_no_policy());
    }

    template
    <
        typename ForwardRange1,
        typename ForwardRange2,
        typename VisitPolicy,
        typename ExpandPolicy1,
        typename OverlapsPolicy1,
        typename ExpandPolicy2,
        typename OverlapsPolicy2
    >
    static inline bool apply(ForwardRange1 const& forward_range1,
                             ForwardRange2 const& forward_range2,
                             VisitPolicy& visitor,
                             ExpandPolicy1 const& expand_policy1,
                             OverlapsPolicy1 const& overlaps_policy1,
                             ExpandPolicy2 const& expand_policy2,
                             OverlapsPolicy2 const& overlaps_policy2)
    {
        return apply(forward_range1, forward_range2, visitor,
                     expand_policy1, overlaps_policy1, expand_policy2, overlaps_policy2,
                     default_min_elements, detail::partition::visit_no_policy());
    }

    template
    <
        typename ForwardRange1,
        typename ForwardRange2,
        typename VisitPolicy,
        typename ExpandPolicy1,
        typename OverlapsPolicy1,
        typename ExpandPolicy2,
        typename OverlapsPolicy2
    >
    static inline bool apply(ForwardRange1 const& forward_range1,
                             ForwardRange2 const& forward_range2,
                             VisitPolicy& visitor,
                             ExpandPolicy1 const& expand_policy1,
                             OverlapsPolicy1 const& overlaps_policy1,
                             ExpandPolicy2 const& expand_policy2,
                             OverlapsPolicy2 const& overlaps_policy2,
                             std::size_t min_elements)
    {
        return apply(forward_range1, forward_range2, visitor,
                     expand_policy1, overlaps_policy1, expand_policy2, overlaps_policy2,
                     min_elements, detail::partition::visit_no_policy());
    }

    template
    <
        typename ForwardRange1,
        typename ForwardRange2,
        typename VisitPolicy,
        typename ExpandPolicy1,
        typename OverlapsPolicy1,
        typename ExpandPolicy2,
        typename OverlapsPolicy2,
        typename VisitBoxPolicy
    >
    static inline bool apply(ForwardRange1 const& forward_range1,
                             ForwardRange2 const& forward_range2,
                             VisitPolicy& visitor,
                             ExpandPolicy1 const& expand_policy1,
                             OverlapsPolicy1 const& overlaps_policy1,
                             ExpandPolicy2 const& expand_policy2,
                             OverlapsPolicy2 const& overlaps_policy2,
                             std::size_t min_elements,
                             VisitBoxPolicy box_visitor)
    {
        typedef typename boost::range_iterator
            <
                ForwardRange1 const
            >::type iterator_type1;

        typedef typename boost::range_iterator
            <
                ForwardRange2 const
            >::type iterator_type2;

        if (std::size_t(boost::size(forward_range1)) > min_elements
            && std::size_t(boost::size(forward_range2)) > min_elements)
        {
            std::vector<iterator_type1> iterator_vector1;
            std::vector<iterator_type2> iterator_vector2;
            Box total;
            assign_inverse(total);
            expand_to_range<IncludePolicy1>(forward_range1, total,
                                            iterator_vector1, expand_policy1);
            expand_to_range<IncludePolicy2>(forward_range2, total,
                                            iterator_vector2, expand_policy2);

            return detail::partition::partition_two_ranges
                <
                    0, Box
                >::apply(total, iterator_vector1, iterator_vector2,
                         0, min_elements, visitor, expand_policy1,
                         overlaps_policy1, expand_policy2, overlaps_policy2,
                         box_visitor);
        }
        else
        {
            for(iterator_type1 it1 = boost::begin(forward_range1);
                it1 != boost::end(forward_range1);
                ++it1)
            {
                for(iterator_type2 it2 = boost::begin(forward_range2);
                    it2 != boost::end(forward_range2);
                    ++it2)
                {
                    if (! visitor.apply(*it1, *it2))
                    {
                        return false; // interrupt
                    }
                }
            }
        }

        return true;
    }
};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_PARTITION_HPP
