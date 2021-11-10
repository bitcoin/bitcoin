/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_RANGE_RUN_MAY_16_2006_0807_PM)
#define BOOST_SPIRIT_RANGE_RUN_MAY_16_2006_0807_PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/char_set/range_functions.hpp>
#include <boost/assert.hpp>
#include <algorithm>

namespace boost { namespace spirit { namespace support { namespace detail
{
    template <typename Run, typename Iterator, typename Range>
    inline bool
    try_merge(Run& run, Iterator iter, Range const& range)
    {
        // if *iter intersects with, or is adjacent to, 'range'...
        if (can_merge(*iter, range))
        {
            // merge range and *iter
            merge(*iter, range);

            // collapse all subsequent ranges that can merge with *iter:
            Iterator i = iter+1;
            // 1. skip subsequent ranges completely included in *iter
            while (i != run.end() && i->last <= iter->last)
                ++i;
            // 2. collapse next range if adjacent or overlapping with *iter
            if (i != run.end() && i->first-1 <= iter->last)
            {
                iter->last = i->last;
                ++i;
            }

            // erase all ranges that were collapsed
            run.erase(iter+1, i);
            return true;
        }
        return false;
    }

    template <typename Char>
    inline bool
    range_run<Char>::test(Char val) const
    {
        if (run.empty())
            return false;

        // search the ranges for one that potentially includes val
        typename storage_type::const_iterator iter =
            std::upper_bound(
                run.begin(), run.end(), val,
                range_compare<range_type>()
            );

        // return true if *(iter-1) includes val
        return iter != run.begin() && includes(*(--iter), val);
    }

    template <typename Char>
    inline void
    range_run<Char>::swap(range_run& other)
    {
        run.swap(other.run);
    }

    template <typename Char>
    void
    range_run<Char>::set(range_type const& range)
    {
        BOOST_ASSERT(is_valid(range));
        if (run.empty())
        {
            // the vector is empty, insert 'range'
            run.push_back(range);
            return;
        }

        // search the ranges for one that potentially includes 'range'
        typename storage_type::iterator iter =
            std::upper_bound(
                run.begin(), run.end(), range,
                range_compare<range_type>()
            );

        if (iter != run.begin())
        {
            // if *(iter-1) includes 'range', return early
            if (includes(*(iter-1), range))
            {
                return;
            }

            // if *(iter-1) can merge with 'range', merge them and return
            if (try_merge(run, iter-1, range))
            {
                return;
            }
        }

        // if *iter can merge with with 'range', merge them
        if (iter == run.end() || !try_merge(run, iter, range))
        {
            // no overlap, insert 'range'
            run.insert(iter, range);
        }
    }

    template <typename Char>
    void
    range_run<Char>::clear(range_type const& range)
    {
        BOOST_ASSERT(is_valid(range));
        if (!run.empty())
        {
            // search the ranges for one that potentially includes 'range'
            typename storage_type::iterator iter =
                std::upper_bound(
                    run.begin(), run.end(), range,
                    range_compare<range_type>()
                );

            // 'range' starts with or after another range:
            if (iter != run.begin())
            {
                typename storage_type::iterator const left_iter = iter-1;

                // 'range' starts after '*left_iter':
                if (left_iter->first < range.first)
                {
                    // if 'range' is completely included inside '*left_iter':
                    // need to break it apart into two ranges (punch a hole),
                    if (left_iter->last > range.last)
                    {
                        Char save_last = left_iter->last;
                        left_iter->last = range.first-1;
                        run.insert(iter, range_type(range.last+1, save_last));
                        return;
                    }
                    // if 'range' contains 'left_iter->last':
                    // truncate '*left_iter' (clip its right)
                    else if (left_iter->last >= range.first)
                    {
                        left_iter->last = range.first-1;
                    }
                }

                // 'range' has the same left bound as '*left_iter': it
                // must be removed or truncated by the code below
                else
                {
                    iter = left_iter;
                }
            }

            // remove or truncate subsequent ranges that overlap with 'range':
            typename storage_type::iterator i = iter;
            // 1. skip subsequent ranges completely included in 'range'
            while (i != run.end() && i->last <= range.last)
                ++i;
            // 2. clip left of next range if overlapping with 'range'
            if (i != run.end() && i->first <= range.last)
                i->first = range.last+1;

            // erase all ranges that 'range' contained
            run.erase(iter, i);
        }
    }

    template <typename Char>
    inline void
    range_run<Char>::clear()
    {
        run.clear();
    }
}}}}

#endif
