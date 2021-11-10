// boost heap: heap node helper classes
//
// Copyright (C) 2010 Tim Blechmann
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HEAP_DETAIL_HEAP_COMPARISON_HPP
#define BOOST_HEAP_DETAIL_HEAP_COMPARISON_HPP

#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/concept/assert.hpp>
#include <boost/heap/heap_concepts.hpp>
#include <boost/type_traits/conditional.hpp>

#ifdef BOOST_HEAP_SANITYCHECKS
#define BOOST_HEAP_ASSERT BOOST_ASSERT
#else
#define BOOST_HEAP_ASSERT(expression)
#endif

namespace boost  {
namespace heap   {
namespace detail {

template <typename Heap1, typename Heap2>
bool value_equality(Heap1 const & lhs, Heap2 const & rhs,
                    typename Heap1::value_type lval, typename Heap2::value_type rval)
{
    typename Heap1::value_compare const & cmp = lhs.value_comp();
    bool ret = !(cmp(lval, rval)) && !(cmp(rval, lval));

    // if this assertion is triggered, the value_compare objects of lhs and rhs return different values
    BOOST_ASSERT((ret == (!(rhs.value_comp()(lval, rval)) && !(rhs.value_comp()(rval, lval)))));

    return ret;
}

template <typename Heap1, typename Heap2>
bool value_compare(Heap1 const & lhs, Heap2 const & rhs,
                    typename Heap1::value_type lval, typename Heap2::value_type rval)
{
    typename Heap1::value_compare const & cmp = lhs.value_comp();
    bool ret = cmp(lval, rval);

    // if this assertion is triggered, the value_compare objects of lhs and rhs return different values
    BOOST_ASSERT((ret == rhs.value_comp()(lval, rval)));
    return ret;
}

struct heap_equivalence_copy
{
    template <typename Heap1, typename Heap2>
    bool operator()(Heap1 const & lhs, Heap2 const & rhs)
    {
        BOOST_CONCEPT_ASSERT((boost::heap::PriorityQueue<Heap1>));
        BOOST_CONCEPT_ASSERT((boost::heap::PriorityQueue<Heap2>));

        // if this assertion is triggered, the value_compare types are incompatible
        BOOST_STATIC_ASSERT((boost::is_same<typename Heap1::value_compare, typename Heap2::value_compare>::value));

        if (Heap1::constant_time_size && Heap2::constant_time_size)
            if (lhs.size() != rhs.size())
                return false;

        if (lhs.empty() && rhs.empty())
            return true;

        Heap1 lhs_copy(lhs);
        Heap2 rhs_copy(rhs);

        while (true) {
            if (!value_equality(lhs_copy, rhs_copy, lhs_copy.top(), rhs_copy.top()))
                return false;

            lhs_copy.pop();
            rhs_copy.pop();

            if (lhs_copy.empty() && rhs_copy.empty())
                return true;

            if (lhs_copy.empty())
                return false;

            if (rhs_copy.empty())
                return false;
        }
    }
};


struct heap_equivalence_iteration
{
    template <typename Heap1, typename Heap2>
    bool operator()(Heap1 const & lhs, Heap2 const & rhs)
    {
        BOOST_CONCEPT_ASSERT((boost::heap::PriorityQueue<Heap1>));
        BOOST_CONCEPT_ASSERT((boost::heap::PriorityQueue<Heap2>));

        // if this assertion is triggered, the value_compare types are incompatible
        BOOST_STATIC_ASSERT((boost::is_same<typename Heap1::value_compare, typename Heap2::value_compare>::value));

        if (Heap1::constant_time_size && Heap2::constant_time_size)
            if (lhs.size() != rhs.size())
                return false;

        if (lhs.empty() && rhs.empty())
            return true;

        typename Heap1::ordered_iterator it1 = lhs.ordered_begin();
        typename Heap1::ordered_iterator it1_end = lhs.ordered_end();
        typename Heap1::ordered_iterator it2 = rhs.ordered_begin();
        typename Heap1::ordered_iterator it2_end = rhs.ordered_end();
        while (true) {
            if (!value_equality(lhs, rhs, *it1, *it2))
                return false;

            ++it1;
            ++it2;

            if (it1 == it1_end && it2 == it2_end)
                return true;

            if (it1 == it1_end || it2 == it2_end)
                return false;
        }
    }
};


template <typename Heap1,
          typename Heap2
         >
bool heap_equality(Heap1 const & lhs, Heap2 const & rhs)
{
    const bool use_ordered_iterators = Heap1::has_ordered_iterators && Heap2::has_ordered_iterators;

    typedef typename boost::conditional<use_ordered_iterators,
                                      heap_equivalence_iteration,
                                      heap_equivalence_copy
                                     >::type equivalence_check;

    equivalence_check eq_check;
    return eq_check(lhs, rhs);
}


struct heap_compare_iteration
{
    template <typename Heap1,
              typename Heap2
             >
    bool operator()(Heap1 const & lhs, Heap2 const & rhs)
    {
        typename Heap1::size_type left_size = lhs.size();
        typename Heap2::size_type right_size = rhs.size();
        if (left_size < right_size)
            return true;

        if (left_size > right_size)
            return false;

        typename Heap1::ordered_iterator it1 = lhs.ordered_begin();
        typename Heap1::ordered_iterator it1_end = lhs.ordered_end();
        typename Heap1::ordered_iterator it2 = rhs.ordered_begin();
        typename Heap1::ordered_iterator it2_end = rhs.ordered_end();
        while (true) {
            if (value_compare(lhs, rhs, *it1, *it2))
                return true;

            if (value_compare(lhs, rhs, *it2, *it1))
                return false;

            ++it1;
            ++it2;

            if (it1 == it1_end && it2 == it2_end)
                return true;

            if (it1 == it1_end || it2 == it2_end)
                return false;
        }
    }
};

struct heap_compare_copy
{
    template <typename Heap1,
              typename Heap2
             >
    bool operator()(Heap1 const & lhs, Heap2 const & rhs)
    {
        typename Heap1::size_type left_size = lhs.size();
        typename Heap2::size_type right_size = rhs.size();
        if (left_size < right_size)
            return true;

        if (left_size > right_size)
            return false;

        Heap1 lhs_copy(lhs);
        Heap2 rhs_copy(rhs);

        while (true) {
            if (value_compare(lhs_copy, rhs_copy, lhs_copy.top(), rhs_copy.top()))
                return true;

            if (value_compare(lhs_copy, rhs_copy, rhs_copy.top(), lhs_copy.top()))
                return false;

            lhs_copy.pop();
            rhs_copy.pop();

            if (lhs_copy.empty() && rhs_copy.empty())
                return false;
        }
    }
};

template <typename Heap1,
          typename Heap2
         >
bool heap_compare(Heap1 const & lhs, Heap2 const & rhs)
{
    const bool use_ordered_iterators = Heap1::has_ordered_iterators && Heap2::has_ordered_iterators;

    typedef typename boost::conditional<use_ordered_iterators,
                                      heap_compare_iteration,
                                      heap_compare_copy
                                     >::type compare_check;

    compare_check check_object;
    return check_object(lhs, rhs);
}


} /* namespace detail */
} /* namespace heap */
} /* namespace boost */


#undef BOOST_HEAP_ASSERT

#endif // BOOST_HEAP_DETAIL_HEAP_COMPARISON_HPP
