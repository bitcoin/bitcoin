///////////////////////////////////////////////////////////////////////////////
/// \file boyer_moore.hpp
///   Contains the boyer-moore implementation. Note: this is *not* a general-
///   purpose boyer-moore implementation. It truncates the search string at
///   256 characters, but it is sufficient for the needs of xpressive.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_BOYER_MOORE_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_BOYER_MOORE_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
# pragma warning(push)
# pragma warning(disable : 4100) // unreferenced formal parameter
#endif

#include <climits>  // for UCHAR_MAX
#include <cstddef>  // for std::ptrdiff_t
#include <utility>  // for std::max
#include <vector>
#include <boost/mpl/bool.hpp>
#include <boost/noncopyable.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// boyer_moore
//
template<typename BidiIter, typename Traits>
struct boyer_moore
  : noncopyable
{
    typedef typename iterator_value<BidiIter>::type char_type;
    typedef Traits traits_type;
    typedef has_fold_case<Traits> case_fold;
    typedef typename Traits::string_type string_type;

    // initialize the Boyer-Moore search data structure, using the
    // search sub-sequence to prime the pump.
    boyer_moore(char_type const *begin, char_type const *end, Traits const &tr, bool icase)
      : begin_(begin)
      , last_(begin)
      , fold_()
      , find_fun_(
              icase
            ? (case_fold() ? &boyer_moore::find_nocase_fold_ : &boyer_moore::find_nocase_)
            : &boyer_moore::find_
        )
    {
        std::ptrdiff_t const uchar_max = UCHAR_MAX;
        std::ptrdiff_t diff = std::distance(begin, end);
        this->length_  = static_cast<unsigned char>((std::min)(diff, uchar_max));
        std::fill_n(static_cast<unsigned char *>(this->offsets_), uchar_max + 1, this->length_);
        --this->length_;

        icase ? this->init_(tr, case_fold()) : this->init_(tr, mpl::false_());
    }

    BidiIter find(BidiIter begin, BidiIter end, Traits const &tr) const
    {
        return (this->*this->find_fun_)(begin, end, tr);
    }

private:

    void init_(Traits const &tr, mpl::false_)
    {
        for(unsigned char offset = this->length_; offset; --offset, ++this->last_)
        {
            this->offsets_[tr.hash(*this->last_)] = offset;
        }
    }

    void init_(Traits const &tr, mpl::true_)
    {
        this->fold_.reserve(this->length_ + 1);
        for(unsigned char offset = this->length_; offset; --offset, ++this->last_)
        {
            this->fold_.push_back(tr.fold_case(*this->last_));
            for(typename string_type::const_iterator beg = this->fold_.back().begin(), end = this->fold_.back().end();
                beg != end; ++beg)
            {
                this->offsets_[tr.hash(*beg)] = offset;
            }
        }
        this->fold_.push_back(tr.fold_case(*this->last_));
    }

    // case-sensitive Boyer-Moore search
    BidiIter find_(BidiIter begin, BidiIter end, Traits const &tr) const
    {
        typedef typename boost::iterator_difference<BidiIter>::type diff_type;
        diff_type const endpos = std::distance(begin, end);
        diff_type offset = static_cast<diff_type>(this->length_);

        for(diff_type curpos = offset; curpos < endpos; curpos += offset)
        {
            std::advance(begin, offset);

            char_type const *pat_tmp = this->last_;
            BidiIter str_tmp = begin;

            for(; tr.translate(*str_tmp) == *pat_tmp; --pat_tmp, --str_tmp)
            {
                if(pat_tmp == this->begin_)
                {
                    return str_tmp;
                }
            }

            offset = this->offsets_[tr.hash(tr.translate(*begin))];
        }

        return end;
    }

    // case-insensitive Boyer-Moore search
    BidiIter find_nocase_(BidiIter begin, BidiIter end, Traits const &tr) const
    {
        typedef typename boost::iterator_difference<BidiIter>::type diff_type;
        diff_type const endpos = std::distance(begin, end);
        diff_type offset = static_cast<diff_type>(this->length_);

        for(diff_type curpos = offset; curpos < endpos; curpos += offset)
        {
            std::advance(begin, offset);

            char_type const *pat_tmp = this->last_;
            BidiIter str_tmp = begin;

            for(; tr.translate_nocase(*str_tmp) == *pat_tmp; --pat_tmp, --str_tmp)
            {
                if(pat_tmp == this->begin_)
                {
                    return str_tmp;
                }
            }

            offset = this->offsets_[tr.hash(tr.translate_nocase(*begin))];
        }

        return end;
    }

    // case-insensitive Boyer-Moore search with case-folding
    BidiIter find_nocase_fold_(BidiIter begin, BidiIter end, Traits const &tr) const
    {
        typedef typename boost::iterator_difference<BidiIter>::type diff_type;
        diff_type const endpos = std::distance(begin, end);
        diff_type offset = static_cast<diff_type>(this->length_);

        for(diff_type curpos = offset; curpos < endpos; curpos += offset)
        {
            std::advance(begin, offset);

            string_type const *pat_tmp = &this->fold_.back();
            BidiIter str_tmp = begin;

            for(; pat_tmp->end() != std::find(pat_tmp->begin(), pat_tmp->end(), *str_tmp);
                --pat_tmp, --str_tmp)
            {
                if(pat_tmp == &this->fold_.front())
                {
                    return str_tmp;
                }
            }

            offset = this->offsets_[tr.hash(*begin)];
        }

        return end;
    }

private:

    char_type const *begin_;
    char_type const *last_;
    std::vector<string_type> fold_;
    BidiIter (boyer_moore::*const find_fun_)(BidiIter, BidiIter, Traits const &) const;
    unsigned char length_;
    unsigned char offsets_[UCHAR_MAX + 1];
};

}}} // namespace boost::xpressive::detail

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif
