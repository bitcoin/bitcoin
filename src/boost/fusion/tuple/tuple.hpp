/*=============================================================================
    Copyright (c) 2014-2015 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef FUSION_TUPLE_14122014_0102
#define FUSION_TUPLE_14122014_0102

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/tuple/tuple_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
// With no variadics, we will use the C++03 version
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_FUSION_HAS_VARIADIC_TUPLE)
# include <boost/fusion/tuple/detail/tuple.hpp>
#else

///////////////////////////////////////////////////////////////////////////////
// C++11 interface
///////////////////////////////////////////////////////////////////////////////
#include <boost/core/enable_if.hpp>
#include <boost/fusion/container/vector/vector.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/comparison.hpp>
#include <boost/fusion/sequence/io.hpp>
#include <boost/fusion/support/detail/and.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <utility>

namespace boost { namespace fusion
{
    template <typename ...T>
    struct tuple
        : vector_detail::vector_data<
              typename detail::make_index_sequence<sizeof...(T)>::type
            , T...
          >
    {
        typedef vector_detail::vector_data<
            typename detail::make_index_sequence<sizeof...(T)>::type
          , T...
        > base;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        BOOST_DEFAULTED_FUNCTION(tuple(), {})

        template <
            typename ...U
          , typename = typename boost::enable_if_c<
                sizeof...(U) >= sizeof...(T)
            >::type
        >
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        tuple(tuple<U...> const& other)
            : base(vector_detail::each_elem(), other) {}

        template <
            typename ...U
          , typename = typename boost::enable_if_c<
                sizeof...(U) >= sizeof...(T)
            >::type
        >
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        tuple(tuple<U...>&& other)
            : base(vector_detail::each_elem(), std::move(other)) {}

        template <
            typename ...U
          , typename = typename boost::enable_if_c<(
                fusion::detail::and_<is_convertible<U, T>...>::value &&
                sizeof...(U) >= 1
            )>::type
        >
        /*BOOST_CONSTEXPR*/ BOOST_FUSION_GPU_ENABLED
        explicit
        tuple(U&&... args)
            : base(vector_detail::each_elem(), std::forward<U>(args)...) {}

        template<typename U1, typename U2>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        tuple(std::pair<U1, U2> const& other)
            : base(vector_detail::each_elem(), other.first, other.second) {}

        template<typename U1, typename U2>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        tuple(std::pair<U1, U2>&& other)
            : base(vector_detail::each_elem(), std::move(other.first), std::move(other.second)) {}

        template<typename U>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        tuple& operator=(U&& rhs)
        {
            base::assign_sequence(std::forward<U>(rhs));
            return *this;
        }
    };

    template <typename Tuple>
    struct tuple_size : result_of::size<Tuple> {};

    template <int N, typename Tuple>
    struct tuple_element : result_of::value_at_c<Tuple, N> {};

    template <int N, typename Tuple>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::at_c<Tuple, N>::type
    get(Tuple& tup)
    {
        return at_c<N>(tup);
    }

    template <int N, typename Tuple>
    BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
    inline typename result_of::at_c<Tuple const, N>::type
    get(Tuple const& tup)
    {
        return at_c<N>(tup);
    }
}}

#endif
#endif

