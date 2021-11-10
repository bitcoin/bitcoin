/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_RANGE_RUN_IPP
#define BOOST_SPIRIT_RANGE_RUN_IPP

///////////////////////////////////////////////////////////////////////////////
#include <algorithm> // for std::lower_bound
#include <boost/spirit/home/classic/core/assert.hpp> // for BOOST_SPIRIT_ASSERT
#include <boost/spirit/home/classic/utility/impl/chset/range_run.hpp>
#include <boost/spirit/home/classic/debug.hpp>
#include <boost/limits.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    namespace utility { namespace impl {

        ///////////////////////////////////////////////////////////////////////
        //
        //  range class implementation
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename CharT>
        inline range<CharT>::range(CharT first_, CharT last_)
        : first(first_), last(last_) {}

        //////////////////////////////////
        template <typename CharT>
        inline bool
        range<CharT>::is_valid() const
        { return first <= last; }

        //////////////////////////////////
        template <typename CharT>
        inline bool
        range<CharT>::includes(range const& r) const
        { return (first <= r.first) && (last >= r.last); }

        //////////////////////////////////
        template <typename CharT>
        inline bool
        range<CharT>::includes(CharT v) const
        { return (first <= v) && (last >= v); }

        //////////////////////////////////
        template <typename CharT>
        inline bool
        range<CharT>::overlaps(range const& r) const
        {
            CharT decr_first =
                first == (std::numeric_limits<CharT>::min)() ? first : first-1;
            CharT incr_last =
                last == (std::numeric_limits<CharT>::max)() ? last : last+1;

            return (decr_first <= r.last) && (incr_last >= r.first);
        }

        //////////////////////////////////
        template <typename CharT>
        inline void
        range<CharT>::merge(range const& r)
        {
            first = (std::min)(first, r.first);
            last = (std::max)(last, r.last);
        }

        ///////////////////////////////////////////////////////////////////////
        //
        //  range_run class implementation
        //
        ///////////////////////////////////////////////////////////////////////
        template <typename CharT>
        inline bool
        range_run<CharT>::test(CharT v) const
        {
            if (!run.empty())
            {
                const_iterator iter =
                    std::lower_bound(
                        run.begin(), run.end(), v,
                        range_char_compare<CharT>()
                    );

                if (iter != run.end() && iter->includes(v))
                    return true;
                if (iter != run.begin())
                    return (--iter)->includes(v);
            }
            return false;
        }

        //////////////////////////////////
        template <typename CharT>
        inline void
        range_run<CharT>::swap(range_run& rr)
        { run.swap(rr.run); }

        //////////////////////////////////
        template <typename CharT>
        void
        range_run<CharT>::merge(iterator iter, range<CharT> const& r)
        {
            iter->merge(r);
            iterator i = iter + 1;

            while (i != run.end() && iter->overlaps(*i))
                iter->merge(*i++);

            run.erase(iter+1, i);
        }

        //////////////////////////////////
        template <typename CharT>
        void
        range_run<CharT>::set(range<CharT> const& r)
        {
            BOOST_SPIRIT_ASSERT(r.is_valid());
            if (!run.empty())
            {
                iterator iter =
                    std::lower_bound(
                        run.begin(), run.end(), r,
                        range_compare<CharT>()
                    );

                if ((iter != run.end() && iter->includes(r)) ||
                    ((iter != run.begin()) && (iter - 1)->includes(r)))
                    return;

                if (iter != run.begin() && (iter - 1)->overlaps(r))
                    merge(--iter, r);

                else if (iter != run.end() && iter->overlaps(r))
                    merge(iter, r);

                else
                    run.insert(iter, r);
            }
            else
            {
                run.push_back(r);
            }
        }

        //////////////////////////////////
        template <typename CharT>
        void
        range_run<CharT>::clear(range<CharT> const& r)
        {
            BOOST_SPIRIT_ASSERT(r.is_valid());
            if (!run.empty())
            {
                iterator iter =
                    std::lower_bound(
                        run.begin(), run.end(), r,
                        range_compare<CharT>()
                    );

                iterator left_iter;

                if ((iter != run.begin()) &&
                        (left_iter = (iter - 1))->includes(r.first))
                {
                    if (left_iter->last > r.last)
                    {
                        CharT save_last = left_iter->last;
                        left_iter->last = r.first-1;
                        run.insert(iter, range<CharT>(r.last+1, save_last));
                        return;
                    }
                    else
                    {
                        left_iter->last = r.first-1;
                    }
                }
                
                iterator i = iter;
                while (i != run.end() && r.includes(*i))
                    i++;
                if (i != run.end() && i->includes(r.last))
                    i->first = r.last+1;
                run.erase(iter, i);
            }
        }

        //////////////////////////////////
        template <typename CharT>
        inline void
        range_run<CharT>::clear()
        { run.clear(); }

        //////////////////////////////////
        template <typename CharT>
        inline typename range_run<CharT>::const_iterator
        range_run<CharT>::begin() const
        { return run.begin(); }

        //////////////////////////////////
        template <typename CharT>
        inline typename range_run<CharT>::const_iterator
        range_run<CharT>::end() const
        { return run.end(); }

    }} // namespace utility::impl

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace boost::spirit

#endif
