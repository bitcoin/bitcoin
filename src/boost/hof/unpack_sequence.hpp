/*=============================================================================
    Copyright (c) 2016 Paul Fultz II
    unpack_sequence.hpp
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_HOF_GUARD_UNPACK_SEQUENCE_HPP
#define BOOST_HOF_GUARD_UNPACK_SEQUENCE_HPP

/// unpack_sequence
/// ===============
/// 
/// How to unpack a sequence can be defined by specializing `unpack_sequence`.
/// By default, `std::tuple` is already specialized. To implement this, one
/// needs to provide a static `apply` function which will unpack the sequence
/// to the parameters of the function.
/// 
/// Synopsis
/// --------
/// 
///     template<class Sequence, class=void>
///     struct unpack_sequence;
/// 
/// Example
/// -------
/// 
///     #include <boost/hof.hpp>
///     #include <cassert>
/// 
///     struct my_sequence
///     {
///         int x;
///         int y;
///     };
///     
///     namespace boost { namespace hof {
///         template<>
///         struct unpack_sequence<my_sequence>
///         {
///             template<class F, class Sequence>
///             constexpr static auto apply(F&& f, Sequence&& s) BOOST_HOF_RETURNS
///             (
///                 f(s.x, s.y)
///             );
///         };
///     }} // namespace boost::hof
/// 
///     int main() {
///     }
/// 
/// See Also
/// --------
/// 
/// * [unpack](unpack)
/// * [is_unpackable](is_unpackable)
/// 

#include <boost/hof/config.hpp>

namespace boost { namespace hof {

template<class Sequence, class=void>
struct unpack_sequence
{
    typedef void not_unpackable;
};

}} // namespace boost::hof

#endif
