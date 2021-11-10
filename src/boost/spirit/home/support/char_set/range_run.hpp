/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_RANGE_RUN_MAY_16_2006_0801_PM)
#define BOOST_SPIRIT_RANGE_RUN_MAY_16_2006_0801_PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/char_set/range.hpp>
#include <vector>

namespace boost { namespace spirit { namespace support { namespace detail
{
    ///////////////////////////////////////////////////////////////////////////
    //  range_run
    //
    //      An implementation of a sparse bit (boolean) set. The set uses
    //      a sorted vector of disjoint ranges. This class implements the
    //      bare minimum essentials from which the full range of set
    //      operators can be implemented. The set is constructed from
    //      ranges. Internally, adjacent or overlapping ranges are
    //      coalesced.
    //
    //      range_runs are very space-economical in situations where there
    //      are lots of ranges and a few individual disjoint values.
    //      Searching is O(log n) where n is the number of ranges.
    //
    //      { Low level interface }
    ///////////////////////////////////////////////////////////////////////////
    template <typename Char>
    class range_run
    {
    public:

        typedef range<Char> range_type;
        typedef std::vector<range_type> storage_type;

        void swap(range_run& other);
        bool test(Char v) const;
        void set(range_type const& range);
        void clear(range_type const& range);
        void clear();

    private:

        storage_type run;
    };
}}}}

#include <boost/spirit/home/support/char_set/range_run_impl.hpp>
#endif
