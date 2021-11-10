///////////////////////////////////////////////////////////////////////////////
// regex_impl.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_REGEX_IMPL_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_REGEX_IMPL_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include <boost/xpressive/regex_traits.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>
#include <boost/xpressive/detail/dynamic/matchable.hpp>
#include <boost/xpressive/detail/utility/tracking_ptr.hpp>
#include <boost/xpressive/detail/utility/counted_base.hpp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// finder
template<typename BidiIter>
struct finder
  : counted_base<finder<BidiIter> >
{
    virtual ~finder() {}
    virtual bool ok_for_partial_matches() const { return true; }
    virtual bool operator ()(match_state<BidiIter> &state) const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// traits
template<typename Char>
struct traits
  : counted_base<traits<Char> >
{
    virtual ~traits() {}
    virtual Char tolower(Char ch) const = 0;
    virtual Char toupper(Char ch) const = 0;
    virtual bool in_range(Char from, Char to, Char ch) const = 0;
    virtual int value(Char ch, int radix) const = 0;
};

///////////////////////////////////////////////////////////////////////////////
// named_mark
template<typename Char>
struct named_mark
{
    typedef typename detail::string_type<Char>::type string_type;

    named_mark(string_type name, std::size_t mark_nbr)
      : name_(name)
      , mark_nbr_(mark_nbr)
    {}

    string_type name_;
    std::size_t mark_nbr_;
};

///////////////////////////////////////////////////////////////////////////////
// traits_holder
template<typename Traits>
struct traits_holder
  : traits<typename Traits::char_type>
{
    typedef typename Traits::char_type char_type;

    explicit traits_holder(Traits const &tr)
      : traits_(tr)
    {
    }

    Traits const &traits() const
    {
        return this->traits_;
    }

    char_type tolower(char_type ch) const
    {
        return this->tolower_(ch, typename Traits::version_tag());
    }

    char_type toupper(char_type ch) const
    {
        return this->toupper_(ch, typename Traits::version_tag());
    }

    int value(char_type ch, int radix) const
    {
        return this->traits_.value(ch, radix);
    }

    bool in_range(char_type from, char_type to, char_type ch) const
    {
        return this->traits_.in_range(from, to, ch);
    }

private:
    char_type tolower_(char_type ch, regex_traits_version_1_tag) const
    {
        return ch;
    }

    char_type toupper_(char_type ch, regex_traits_version_1_tag) const
    {
        return ch;
    }

    char_type tolower_(char_type ch, regex_traits_version_2_tag) const
    {
        return this->traits_.tolower(ch);
    }

    char_type toupper_(char_type ch, regex_traits_version_2_tag) const
    {
        return this->traits_.toupper(ch);
    }

    Traits traits_;
};

///////////////////////////////////////////////////////////////////////////////
// regex_impl
//
template<typename BidiIter>
struct regex_impl
  : enable_reference_tracking<regex_impl<BidiIter> >
{
    typedef typename iterator_value<BidiIter>::type char_type;

    regex_impl()
      : enable_reference_tracking<regex_impl<BidiIter> >()
      , xpr_()
      , traits_()
      , finder_()
      , named_marks_()
      , mark_count_(0)
      , hidden_mark_count_(0)
    {
        #ifdef BOOST_XPRESSIVE_DEBUG_CYCLE_TEST
        ++instances;
        #endif
    }

    regex_impl(regex_impl<BidiIter> const &that)
      : enable_reference_tracking<regex_impl<BidiIter> >(that)
      , xpr_(that.xpr_)
      , traits_(that.traits_)
      , finder_(that.finder_)
      , named_marks_(that.named_marks_)
      , mark_count_(that.mark_count_)
      , hidden_mark_count_(that.hidden_mark_count_)
    {
        #ifdef BOOST_XPRESSIVE_DEBUG_CYCLE_TEST
        ++instances;
        #endif
    }

    ~regex_impl()
    {
        #ifdef BOOST_XPRESSIVE_DEBUG_CYCLE_TEST
        --instances;
        #endif
    }

    void swap(regex_impl<BidiIter> &that)
    {
        enable_reference_tracking<regex_impl<BidiIter> >::swap(that);
        this->xpr_.swap(that.xpr_);
        this->traits_.swap(that.traits_);
        this->finder_.swap(that.finder_);
        this->named_marks_.swap(that.named_marks_);
        std::swap(this->mark_count_, that.mark_count_);
        std::swap(this->hidden_mark_count_, that.hidden_mark_count_);
    }

    intrusive_ptr<matchable_ex<BidiIter> const> xpr_;
    intrusive_ptr<traits<char_type> const> traits_;
    intrusive_ptr<finder<BidiIter> > finder_;
    std::vector<named_mark<char_type> > named_marks_;
    std::size_t mark_count_;
    std::size_t hidden_mark_count_;

    #ifdef BOOST_XPRESSIVE_DEBUG_CYCLE_TEST
    static int instances;
    #endif

private:
    regex_impl &operator =(regex_impl const &);
};

template<typename BidiIter>
void swap(regex_impl<BidiIter> &left, regex_impl<BidiIter> &right)
{
    left.swap(right);
}

#ifdef BOOST_XPRESSIVE_DEBUG_CYCLE_TEST
template<typename BidiIter>
int regex_impl<BidiIter>::instances = 0;
#endif

}}} // namespace boost::xpressive::detail

#endif
