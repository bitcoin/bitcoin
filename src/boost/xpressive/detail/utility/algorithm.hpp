///////////////////////////////////////////////////////////////////////////////
// algorithm.hpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_UTILITY_ALGORITHM_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_UTILITY_ALGORITHM_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <string>
#include <climits>
#include <algorithm>
#include <boost/version.hpp>
#include <boost/range/end.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/xpressive/detail/utility/ignore_unused.hpp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// any
//
template<typename InIter, typename Pred>
inline bool any(InIter begin, InIter end, Pred pred)
{
    return end != std::find_if(begin, end, pred);
}

///////////////////////////////////////////////////////////////////////////////
// find_nth_if
//
template<typename FwdIter, typename Diff, typename Pred>
FwdIter find_nth_if(FwdIter begin, FwdIter end, Diff count, Pred pred)
{
    for(; begin != end; ++begin)
    {
        if(pred(*begin) && 0 == count--)
        {
            return begin;
        }
    }

    return end;
}

///////////////////////////////////////////////////////////////////////////////
// toi
//
template<typename InIter, typename Traits>
int toi(InIter &begin, InIter end, Traits const &tr, int radix = 10, int max = INT_MAX)
{
    detail::ignore_unused(tr);
    int i = 0, c = 0;
    for(; begin != end && -1 != (c = tr.value(*begin, radix)); ++begin)
    {
        if(max < ((i *= radix) += c))
            return i / radix;
    }
    return i;
}

///////////////////////////////////////////////////////////////////////////////
// advance_to
//
template<typename BidiIter, typename Diff>
inline bool advance_to_impl(BidiIter & iter, Diff diff, BidiIter end, std::bidirectional_iterator_tag)
{
    for(; 0 < diff && iter != end; --diff)
        ++iter;
    for(; 0 > diff && iter != end; ++diff)
        --iter;
    return 0 == diff;
}

template<typename RandIter, typename Diff>
inline bool advance_to_impl(RandIter & iter, Diff diff, RandIter end, std::random_access_iterator_tag)
{
    if(0 < diff)
    {
        if((end - iter) < diff)
            return false;
    }
    else if(0 > diff)
    {
        if((iter - end) < -diff)
            return false;
    }
    iter += diff;
    return true;
}

template<typename Iter, typename Diff>
inline bool advance_to(Iter & iter, Diff diff, Iter end)
{
    return detail::advance_to_impl(iter, diff, end, typename iterator_category<Iter>::type());
}

///////////////////////////////////////////////////////////////////////////////
// range_data
//
template<typename T>
struct range_data
  : range_value<T>
{};

template<typename T>
struct range_data<T *>
  : remove_const<T>
{};

template<typename T> std::ptrdiff_t is_null_terminated(T const &) { return 0; }
#if BOOST_VERSION >= 103500
inline std::ptrdiff_t is_null_terminated(char const *) { return 1; }
#ifndef BOOST_XPRESSIVE_NO_WREGEX
inline std::ptrdiff_t is_null_terminated(wchar_t const *) { return 1; }
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// data_begin/data_end
//
template<typename Cont>
typename range_data<Cont>::type const *data_begin(Cont const &cont)
{
    return &*boost::begin(cont);
}

template<typename Cont>
typename range_data<Cont>::type const *data_end(Cont const &cont)
{
    return &*boost::begin(cont) + boost::size(cont) - is_null_terminated(cont);
}

template<typename Char, typename Traits, typename Alloc>
Char const *data_begin(std::basic_string<Char, Traits, Alloc> const &str)
{
    return str.data();
}

template<typename Char, typename Traits, typename Alloc>
Char const *data_end(std::basic_string<Char, Traits, Alloc> const &str)
{
    return str.data() + str.size();
}

template<typename Char>
Char const *data_begin(Char const *const &sz)
{
    return sz;
}

template<typename Char>
Char const *data_end(Char const *const &sz)
{
    Char const *tmp = sz;
    for(; *tmp; ++tmp)
        ;
    return tmp;
}

}}}

#endif
