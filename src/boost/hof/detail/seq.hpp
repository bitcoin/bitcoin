/*=============================================================================
    Copyright (c) 2012 Paul Fultz II
    seq.h
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_FUNCTION_DETAIL_SEQ_H
#define BOOST_HOF_GUARD_FUNCTION_DETAIL_SEQ_H

#include <cstdlib>

namespace boost { namespace hof { 

namespace detail {

template<std::size_t ...>
struct seq 
{
    typedef seq type;
};

template <class, class>
struct merge_seq;

template <size_t... Xs, size_t... Ys>
struct merge_seq<seq<Xs...>, seq<Ys...>>
: seq<Xs..., (sizeof...(Xs)+Ys)...>
{};

template<std::size_t N>
struct gens 
: merge_seq<
    typename gens<N/2>::type,
    typename gens<N - N/2>::type
> 
{};

template<> struct gens<0> : seq<> {}; 
template<> struct gens<1> : seq<0> {}; 


}
}} // namespace boost::hof

#endif
