/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_RANGE_FUNCTIONS_MAY_16_2006_0720_PM)
#define BOOST_SPIRIT_RANGE_FUNCTIONS_MAY_16_2006_0720_PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <limits>

namespace boost { namespace spirit { namespace support { namespace detail
{
    template <typename Range>
    inline bool
    is_valid(Range const& range)
    {
        // test for valid ranges
        return range.first <= range.last;
    }

    template <typename Range>
    inline bool
    includes(Range const& range, Range const& other)
    {
        // see if two ranges intersect
        return (range.first <= other.first) && (range.last >= other.last);
    }

    template <typename Range>
    inline bool
    includes(Range const& range, typename Range::value_type val)
    {
        // see if val is in range
        return (range.first <= val) && (range.last >= val);
    }

    template <typename Range>
    inline bool
    can_merge(Range const& range, Range const& other)
    {
        // see if a 'range' overlaps, or is adjacent to
        // another range 'other', so we can merge them

        typedef typename Range::value_type value_type;
        typedef std::numeric_limits<value_type> limits;

        value_type decr_first =
            range.first == limits::min()
            ? range.first : range.first-1;

        value_type incr_last =
            range.last == limits::max()
            ? range.last : range.last+1;

        return (decr_first <= other.last) && (incr_last >= other.first);
    }

    template <typename Range>
    inline void
    merge(Range& result, Range const& other)
    {
        // merge two ranges
        if (result.first > other.first)
            result.first = other.first;
        if (result.last < other.last)
            result.last = other.last;
    }

    template <typename Range>
    struct range_compare
    {
        // compare functor with a value or another range

        typedef typename Range::value_type value_type;

        bool operator()(Range const& x, const value_type y) const
        {
            return x.first < y;
        }

        bool operator()(value_type const x, Range const& y) const
        {
            return x < y.first;
        }

        bool operator()(Range const& x, Range const& y) const
        {
            return x.first < y.first;
        }
    };
}}}}

#endif
