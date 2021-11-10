/*=============================================================================
    Copyright (c) 2014-2015 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef FUSION_VECTOR_11052014_1625
#define FUSION_VECTOR_11052014_1625

#include <boost/config.hpp>
#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/vector/detail/config.hpp>
#include <boost/fusion/container/vector/vector_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
// Without variadics, we will use the PP version
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_FUSION_HAS_VARIADIC_VECTOR)
# include <boost/fusion/container/vector/detail/cpp03/vector.hpp>
#else

///////////////////////////////////////////////////////////////////////////////
// C++11 interface
///////////////////////////////////////////////////////////////////////////////
#include <boost/fusion/support/sequence_base.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/support/detail/and.hpp>
#include <boost/fusion/support/detail/index_sequence.hpp>
#include <boost/fusion/container/vector/detail/at_impl.hpp>
#include <boost/fusion/container/vector/detail/value_at_impl.hpp>
#include <boost/fusion/container/vector/detail/begin_impl.hpp>
#include <boost/fusion/container/vector/detail/end_impl.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/mpl/int.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <cstddef>
#include <utility>

namespace boost { namespace fusion
{
    struct vector_tag;
    struct random_access_traversal_tag;

    namespace vector_detail
    {
        struct each_elem {};

        template <
            typename This, typename T, typename T_, std::size_t Size, bool IsSeq
        >
        struct can_convert_impl : false_type {};

        template <typename This, typename T, typename Sequence, std::size_t Size>
        struct can_convert_impl<This, T, Sequence, Size, true> : true_type {};

        template <typename This, typename Sequence, typename T>
        struct can_convert_impl<This, Sequence, T, 1, true>
            : integral_constant<
                  bool
                , !is_convertible<
                      Sequence
                    , typename fusion::extension::value_at_impl<vector_tag>::
                          template apply< This, mpl::int_<0> >::type
                  >::value
              >
        {};

        template <typename This, typename T, typename T_, std::size_t Size>
        struct can_convert
            : can_convert_impl<
                  This, T, T_, Size, traits::is_sequence<T_>::value
              >
        {};

        template <typename T, bool IsSeq, std::size_t Size>
        struct is_longer_sequence_impl : false_type {};

        template <typename Sequence, std::size_t Size>
        struct is_longer_sequence_impl<Sequence, true, Size>
            : integral_constant<
                  bool, (fusion::result_of::size<Sequence>::value >= Size)
              >
        {};

        template<typename T, std::size_t Size>
        struct is_longer_sequence
            : is_longer_sequence_impl<T, traits::is_sequence<T>::value, Size>
        {};

        // forward_at_c allows to access Nth element even if ForwardSequence
        // since fusion::at_c requires RandomAccessSequence.
        namespace result_of
        {
            template <typename Sequence, int N>
            struct forward_at_c
                : fusion::result_of::deref<
                      typename fusion::result_of::advance_c<
                          typename fusion::result_of::begin<
                              typename remove_reference<Sequence>::type
                          >::type
                        , N
                      >::type
                  >
            {};
        }

        template <int N, typename Sequence>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        inline typename result_of::forward_at_c<Sequence, N>::type
        forward_at_c(Sequence&& seq)
        {
            typedef typename
                result_of::forward_at_c<Sequence, N>::type
            result;
            return std::forward<result>(*advance_c<N>(begin(seq)));
        }

        // Object proxy since preserve object order
        template <std::size_t, typename T>
        struct store
        {
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            store()
                : elem() // value-initialized explicitly
            {}

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            store(store const& rhs)
                : elem(rhs.elem)
            {}

            BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            store&
            operator=(store const& rhs)
            {
                elem = rhs.elem;
                return *this;
            }

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            store(store&& rhs)
                : elem(static_cast<T&&>(rhs.elem))
            {}

            BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            store&
            operator=(store&& rhs)
            {
                elem = static_cast<T&&>(rhs.elem);
                return *this;
            }

            template <
                typename U
              , typename = typename boost::disable_if<
                    is_base_of<store, typename remove_reference<U>::type>
                >::type
            >
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            store(U&& rhs)
                : elem(std::forward<U>(rhs))
            {}

            using elem_type = T;
            T elem;
        };

        // placed outside of vector_data due to GCC < 6 bug
        template <std::size_t J, typename U>
        static inline BOOST_FUSION_GPU_ENABLED
        store<J, U> store_at_impl(store<J, U>*);

        template <typename I, typename ...T>
        struct vector_data;

        template <std::size_t ...I, typename ...T>
        struct vector_data<detail::index_sequence<I...>, T...>
            : store<I, T>...
            , sequence_base<vector_data<detail::index_sequence<I...>, T...> >
        {
            typedef vector_tag                  fusion_tag;
            typedef fusion_sequence_tag         tag; // this gets picked up by MPL
            typedef mpl::false_                 is_view;
            typedef random_access_traversal_tag category;
            typedef mpl::int_<sizeof...(T)>     size;
            typedef vector<T...>                type_sequence;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            BOOST_DEFAULTED_FUNCTION(vector_data(), {})

            template <
                typename Sequence
              , typename Sequence_ = typename remove_reference<Sequence>::type
              , typename = typename boost::enable_if<
                    can_convert<vector_data, Sequence, Sequence_, sizeof...(I)>
                >::type
            >
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            explicit
            vector_data(each_elem, Sequence&& rhs)
                : store<I, T>(forward_at_c<I>(std::forward<Sequence>(rhs)))...
            {}

            template <typename ...U>
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            explicit
            vector_data(each_elem, U&&... var)
                : store<I, T>(std::forward<U>(var))...
            {}

            template <typename Sequence>
            BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            void
            assign_sequence(Sequence&& seq)
            {
                assign(std::forward<Sequence>(seq), detail::index_sequence<I...>());
            }

            template <typename Sequence>
            BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            void
            assign(Sequence&&, detail::index_sequence<>) {}

            template <typename Sequence, std::size_t N, std::size_t ...M>
            BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            void
            assign(Sequence&& seq, detail::index_sequence<N, M...>)
            {
                at_impl(mpl::int_<N>()) = vector_detail::forward_at_c<N>(seq);
                assign(std::forward<Sequence>(seq), detail::index_sequence<M...>());
            }

        private:
            template <std::size_t J>
            using store_at = decltype(store_at_impl<J>(static_cast<vector_data*>(nullptr)));

        public:
            template <typename J>
            BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            typename store_at<J::value>::elem_type& at_impl(J)
            {
                return store_at<J::value>::elem;
            }

            template <typename J>
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            typename store_at<J::value>::elem_type const& at_impl(J) const
            {
                return store_at<J::value>::elem;
            }
        };
    } // namespace boost::fusion::vector_detail

    template <typename... T>
    struct vector
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
        BOOST_DEFAULTED_FUNCTION(vector(), {})

        template <
            typename... U
          , typename = typename boost::enable_if_c<(
                sizeof...(U) >= 1 &&
                fusion::detail::and_<is_convertible<U, T>...>::value &&
                !fusion::detail::and_<
                    is_base_of<vector, typename remove_reference<U>::type>...
                >::value
            )>::type
        >
        // XXX: constexpr become error due to pull-request #79, booooo!!
        //      In the (near) future release, should be fixed.
        /* BOOST_CONSTEXPR */ BOOST_FUSION_GPU_ENABLED
        explicit vector(U&&... u)
            : base(vector_detail::each_elem(), std::forward<U>(u)...)
        {}

        template <
            typename Sequence
          , typename = typename boost::enable_if_c<
                vector_detail::is_longer_sequence<
                    typename remove_reference<Sequence>::type, sizeof...(T)
                >::value
            >::type
        >
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector(Sequence&& seq)
            : base(vector_detail::each_elem(), std::forward<Sequence>(seq))
        {}

        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        vector&
        operator=(Sequence&& rhs)
        {
            base::assign_sequence(std::forward<Sequence>(rhs));
            return *this;
        }
    };
}}

#endif
#endif

