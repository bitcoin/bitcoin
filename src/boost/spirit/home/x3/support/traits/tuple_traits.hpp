/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
================================================_==============================*/
#if !defined(BOOST_SPIRIT_X3_TUPLE_TRAITS_JANUARY_2012_1132PM)
#define BOOST_SPIRIT_X3_TUPLE_TRAITS_JANUARY_2012_1132PM

#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/is_view.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    template <typename A, typename B>
    struct has_same_size
      : mpl::bool_<(
            fusion::result_of::size<A>::value ==
            fusion::result_of::size<B>::value
        )>
    {};

    template <typename T, std::size_t N>
    struct has_size
      : mpl::bool_<(fusion::result_of::size<T>::value == N)>
    {};

    template <typename A, typename B>
    struct is_same_size_sequence
      : mpl::and_<
            fusion::traits::is_sequence<A>
          , fusion::traits::is_sequence<B>
          , has_same_size<A, B>
        >
    {};

    template <typename Seq>
    struct is_size_one_sequence
      : mpl::and_<
            fusion::traits::is_sequence<Seq>
          , has_size<Seq, 1>
        >
    {};

    template <typename View>
    struct is_size_one_view
      : mpl::and_<
            fusion::traits::is_view<View>
          , has_size<View, 1>
        >
    {};
}}}}

#endif
