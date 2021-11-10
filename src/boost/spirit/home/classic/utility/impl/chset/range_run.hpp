/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_RANGE_RUN_HPP
#define BOOST_SPIRIT_RANGE_RUN_HPP

///////////////////////////////////////////////////////////////////////////////
#include <vector>

#include <boost/spirit/home/classic/namespace.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { 

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

namespace utility { namespace impl {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  range class
    //
    //      Implements a closed range of values. This class is used in
    //      the implementation of the range_run class.
    //
    //      { Low level implementation detail }
    //      { Not to be confused with BOOST_SPIRIT_CLASSIC_NS::range }
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT>
    struct range {

                        range(CharT first, CharT last);

        bool            is_valid() const;
        bool            includes(CharT v) const;
        bool            includes(range const& r) const;
        bool            overlaps(range const& r) const;
        void            merge(range const& r);

        CharT first;
        CharT last;
    };

    //////////////////////////////////
    template <typename CharT>
    struct range_char_compare {

        bool operator()(range<CharT> const& x, const CharT y) const
        { return x.first < y; }
        
        bool operator()(const CharT x, range<CharT> const& y) const
        { return x < y.first; }
        
        // This additional operator is required for the checked STL shipped
        // with VC8 testing the ordering of the iterators passed to the
        // std::lower_bound algo this range_char_compare<> predicate is passed
        // to.
        bool operator()(range<CharT> const& x, range<CharT> const& y) const
        { return x.first < y.first; }
    };

    //////////////////////////////////
    template <typename CharT>
    struct range_compare {

        bool operator()(range<CharT> const& x, range<CharT> const& y) const
        { return x.first < y.first; }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
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
    //      { Low level implementation detail }
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT>
    class range_run {

    public:

        typedef range<CharT> range_t;
        typedef std::vector<range_t> run_t;
        typedef typename run_t::iterator iterator;
        typedef typename run_t::const_iterator const_iterator;

        void            swap(range_run& rr);
        bool            test(CharT v) const;
        void            set(range_t const& r);
        void            clear(range_t const& r);
        void            clear();

        const_iterator  begin() const;
        const_iterator  end() const;

    private:

        void            merge(iterator iter, range_t const& r);

        run_t run;
    };

}}

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS::utility::impl

#endif

#include <boost/spirit/home/classic/utility/impl/chset/range_run.ipp>
