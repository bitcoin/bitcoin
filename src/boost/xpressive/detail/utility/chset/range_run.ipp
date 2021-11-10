/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_XPRESSIVE_SPIRIT_RANGE_RUN_IPP
#define BOOST_XPRESSIVE_SPIRIT_RANGE_RUN_IPP

///////////////////////////////////////////////////////////////////////////////
#include <algorithm> // for std::lower_bound
#include <boost/limits.hpp>
#include <boost/assert.hpp>
#include <boost/xpressive/detail/utility/chset/range_run.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////
//
//  range class implementation
//
///////////////////////////////////////////////////////////////////////
template<typename Char>
inline range<Char>::range(Char first, Char last)
  : first_(first)
  , last_(last)
{
}

//////////////////////////////////
template<typename Char>
inline bool range<Char>::is_valid() const
{
    return this->first_ <= this->last_;
}

//////////////////////////////////
template<typename Char>
inline bool range<Char>::includes(range<Char> const &r) const
{
    return (this->first_ <= r.first_) && (this->last_ >= r.last_);
}

//////////////////////////////////
template<typename Char>
inline bool range<Char>::includes(Char v) const
{
    return (this->first_ <= v) && (this->last_ >= v);
}

//////////////////////////////////
template<typename Char>
inline bool range<Char>::overlaps(range<Char> const &r) const
{
    Char decr_first = (std::min)(this->first_, Char(this->first_-1));
    Char incr_last = (std::max)(this->last_, Char(this->last_+1));

    return (decr_first <= r.last_) && (incr_last >= r.first_);
}

//////////////////////////////////
template<typename Char>
inline void range<Char>::merge(range<Char> const &r)
{
    this->first_ = (std::min)(this->first_, r.first_);
    this->last_ = (std::max)(this->last_, r.last_);
}

///////////////////////////////////////////////////////////////////////
//
//  range_run class implementation
//
///////////////////////////////////////////////////////////////////////
template<typename Char>
inline bool range_run<Char>::empty() const
{
    return this->run_.empty();
}

template<typename Char>
inline bool range_run<Char>::test(Char v) const
{
    if(this->run_.empty())
    {
        return false;
    }

    const_iterator iter = std::lower_bound(
        this->run_.begin()
      , this->run_.end()
      , range<Char>(v, v)
      , range_compare<Char>()
    );

    return (iter != this->run_.end() && iter->includes(v))
        || (iter != this->run_.begin() && (--iter)->includes(v));
}

template<typename Char>
template<typename Traits>
inline bool range_run<Char>::test(Char v, Traits const &tr) const
{
    const_iterator begin = this->run_.begin();
    const_iterator end = this->run_.end();

    for(; begin != end; ++begin)
    {
        if(tr.in_range_nocase(begin->first_, begin->last_, v))
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////
template<typename Char>
inline void range_run<Char>::swap(range_run<Char> &rr)
{
    this->run_.swap(rr.run_);
}

//////////////////////////////////
template<typename Char>
void range_run<Char>::merge(iterator iter, range<Char> const &r)
{
    BOOST_ASSERT(iter != this->run_.end());
    iter->merge(r);

    iterator i = iter;
    while(++i != this->run_.end() && iter->overlaps(*i))
    {
        iter->merge(*i);
    }

    this->run_.erase(++iter, i);
}

//////////////////////////////////
template<typename Char>
void range_run<Char>::set(range<Char> const &r)
{
    BOOST_ASSERT(r.is_valid());
    if(!this->run_.empty())
    {
        iterator iter = std::lower_bound(this->run_.begin(), this->run_.end(), r, range_compare<Char>());

        if((iter != this->run_.end() && iter->includes(r)) ||
           (iter != this->run_.begin() && (iter - 1)->includes(r)))
        {
            return;
        }
        else if(iter != this->run_.begin() && (iter - 1)->overlaps(r))
        {
            this->merge(--iter, r);
        }
        else if(iter != this->run_.end() && iter->overlaps(r))
        {
            this->merge(iter, r);
        }
        else
        {
            this->run_.insert(iter, r);
        }
    }
    else
    {
        this->run_.push_back(r);
    }
}

//////////////////////////////////
template<typename Char>
void range_run<Char>::clear(range<Char> const &r)
{
    BOOST_ASSERT(r.is_valid());
    if(!this->run_.empty())
    {
        iterator iter = std::lower_bound(this->run_.begin(), this->run_.end(), r, range_compare<Char>());
        iterator left_iter;

        if((iter != this->run_.begin()) &&
           (left_iter = (iter - 1))->includes(r.first_))
        {
            if(left_iter->last_ > r.last_)
            {
                Char save_last = left_iter->last_;
                left_iter->last_ = r.first_-1;
                this->run_.insert(iter, range<Char>(r.last_+1, save_last));
                return;
            }
            else
            {
                left_iter->last_ = r.first_-1;
            }
        }

        iterator i = iter;
        for(; i != this->run_.end() && r.includes(*i); ++i) {}
        if(i != this->run_.end() && i->includes(r.last_))
        {
            i->first_ = r.last_+1;
        }
        this->run_.erase(iter, i);
    }
}

//////////////////////////////////
template<typename Char>
inline void range_run<Char>::clear()
{
    this->run_.clear();
}

//////////////////////////////////
template<typename Char>
inline typename range_run<Char>::const_iterator range_run<Char>::begin() const
{
    return this->run_.begin();
}

//////////////////////////////////
template<typename Char>
inline typename range_run<Char>::const_iterator range_run<Char>::end() const
{
    return this->run_.end();
}

}}} // namespace boost::xpressive::detail

#endif
