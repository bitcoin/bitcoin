///////////////////////////////////////////////////////////////////////////////
/// \file default.hpp
/// Contains definition of the _default transform, which gives operators their
/// usual C++ meanings and uses Boost.Typeof to deduce return types.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TRANSFORM_DEFAULT_HPP_EAN_04_04_2008
#define BOOST_PROTO_TRANSFORM_DEFAULT_HPP_EAN_04_04_2008

#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_shifted.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/ref.hpp>
#include <boost/get_pointer.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_member_pointer.hpp>
#include <boost/type_traits/is_member_object_pointer.hpp>
#include <boost/type_traits/is_member_function_pointer.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/traits.hpp>
#include <boost/proto/transform/impl.hpp>
#include <boost/proto/transform/arg.hpp>
#include <boost/proto/detail/decltype.hpp>

namespace boost { namespace proto
{
    namespace detail
    {
        template<typename Grammar, typename Tag>
        struct default_case
          : not_<_>
        {};

        template<typename Grammar>
        struct default_case<Grammar, tag::terminal>
          : when<terminal<_>, _value>
        {};

        template<typename Grammar>
        struct default_cases
        {
            template<typename Tag>
            struct case_
              : default_case<Grammar, Tag>
            {};
        };

        #define BOOST_PROTO_UNARY_DEFAULT_EVAL(OP, TAG, MAKE)                                       \
        template<typename Grammar>                                                                  \
        struct BOOST_PP_CAT(default_, TAG)                                                          \
          : transform<BOOST_PP_CAT(default_, TAG)<Grammar> >                                        \
        {                                                                                           \
            template<typename Expr, typename State, typename Data>                                  \
            struct impl                                                                             \
              : transform_impl<Expr, State, Data>                                                   \
            {                                                                                       \
            private:                                                                                \
                typedef typename result_of::child_c<Expr, 0>::type e0;                              \
                typedef typename Grammar::template impl<e0, State, Data>::result_type r0;           \
            public:                                                                                 \
                BOOST_PROTO_DECLTYPE_(OP proto::detail::MAKE<r0>(), result_type)                    \
                result_type operator ()(                                                            \
                    typename impl::expr_param e                                                     \
                  , typename impl::state_param s                                                    \
                  , typename impl::data_param d                                                     \
                ) const                                                                             \
                {                                                                                   \
                    typename Grammar::template impl<e0, State, Data> t0;                            \
                    return OP t0(proto::child_c<0>(e), s, d);                                       \
                }                                                                                   \
            };                                                                                      \
        };                                                                                          \
                                                                                                    \
        template<typename Grammar>                                                                  \
        struct default_case<Grammar, tag::TAG>                                                      \
          : when<unary_expr<tag::TAG, Grammar>, BOOST_PP_CAT(default_, TAG)<Grammar> >              \
        {};                                                                                         \
        /**/

        #define BOOST_PROTO_BINARY_DEFAULT_EVAL(OP, TAG, LMAKE, RMAKE)                              \
        template<typename Grammar>                                                                  \
        struct BOOST_PP_CAT(default_, TAG)                                                          \
          : transform<BOOST_PP_CAT(default_, TAG)<Grammar> >                                        \
        {                                                                                           \
            template<typename Expr, typename State, typename Data>                                  \
            struct impl                                                                             \
              : transform_impl<Expr, State, Data>                                                   \
            {                                                                                       \
            private:                                                                                \
                typedef typename result_of::child_c<Expr, 0>::type e0;                              \
                typedef typename result_of::child_c<Expr, 1>::type e1;                              \
                typedef typename Grammar::template impl<e0, State, Data>::result_type r0;           \
                typedef typename Grammar::template impl<e1, State, Data>::result_type r1;           \
            public:                                                                                 \
                BOOST_PROTO_DECLTYPE_(                                                              \
                    proto::detail::LMAKE<r0>() OP proto::detail::RMAKE<r1>()                        \
                  , result_type                                                                     \
                )                                                                                   \
                result_type operator ()(                                                            \
                    typename impl::expr_param e                                                     \
                  , typename impl::state_param s                                                    \
                  , typename impl::data_param d                                                     \
                ) const                                                                             \
                {                                                                                   \
                    typename Grammar::template impl<e0, State, Data> t0;                            \
                    typename Grammar::template impl<e1, State, Data> t1;                            \
                    return t0(proto::child_c<0>(e), s, d)                                           \
                        OP t1(proto::child_c<1>(e), s, d);                                          \
                }                                                                                   \
            };                                                                                      \
        };                                                                                          \
                                                                                                    \
        template<typename Grammar>                                                                  \
        struct default_case<Grammar, tag::TAG>                                                      \
          : when<binary_expr<tag::TAG, Grammar, Grammar>, BOOST_PP_CAT(default_, TAG)<Grammar> >    \
        {};                                                                                         \
        /**/

        BOOST_PROTO_UNARY_DEFAULT_EVAL(+, unary_plus, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(-, negate, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(*, dereference, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(~, complement, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(&, address_of, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(!, logical_not, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(++, pre_inc, make_mutable)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(--, pre_dec, make_mutable)

        BOOST_PROTO_BINARY_DEFAULT_EVAL(<<, shift_left, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(>>, shift_right, make_mutable, make_mutable)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(*, multiplies, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(/, divides, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(%, modulus, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(+, plus, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(-, minus, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(<, less, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(>, greater, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(<=, less_equal, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(>=, greater_equal, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(==, equal_to, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(!=, not_equal_to, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(||, logical_or, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(&&, logical_and, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(&, bitwise_and, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(|, bitwise_or, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(^, bitwise_xor, make, make)

        BOOST_PROTO_BINARY_DEFAULT_EVAL(=, assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(<<=, shift_left_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(>>=, shift_right_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(*=, multiplies_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(/=, divides_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(%=, modulus_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(+=, plus_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(-=, minus_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(&=, bitwise_and_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(|=, bitwise_or_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(^=, bitwise_xor_assign, make_mutable, make)

        #undef BOOST_PROTO_UNARY_DEFAULT_EVAL
        #undef BOOST_PROTO_BINARY_DEFAULT_EVAL

        /// INTERNAL ONLY
        template<typename Grammar, typename Expr, typename State, typename Data>
        struct is_member_function_invocation
          : is_member_function_pointer<
                typename uncvref<
                    typename Grammar::template impl<
                        typename result_of::child_c<Expr, 1>::type
                      , State
                      , Data
                    >::result_type
                >::type
            >
        {};

        /// INTERNAL ONLY
        template<typename Grammar, typename Expr, typename State, typename Data, bool IsMemFunCall>
        struct default_mem_ptr_impl
          : transform_impl<Expr, State, Data>
        {
        private:
            typedef typename result_of::child_c<Expr, 0>::type e0;
            typedef typename result_of::child_c<Expr, 1>::type e1;
            typedef typename Grammar::template impl<e0, State, Data>::result_type r0;
            typedef typename Grammar::template impl<e1, State, Data>::result_type r1;
        public:
            typedef typename detail::mem_ptr_fun<r0, r1>::result_type result_type;
            result_type operator ()(
                typename default_mem_ptr_impl::expr_param e
              , typename default_mem_ptr_impl::state_param s
              , typename default_mem_ptr_impl::data_param d
            ) const
            {
                typename Grammar::template impl<e0, State, Data> t0;
                typename Grammar::template impl<e1, State, Data> t1;
                return detail::mem_ptr_fun<r0, r1>()(
                    t0(proto::child_c<0>(e), s, d)
                  , t1(proto::child_c<1>(e), s, d)
                );
            }
        };

        /// INTERNAL ONLY
        template<typename Grammar, typename Expr, typename State, typename Data>
        struct default_mem_ptr_impl<Grammar, Expr, State, Data, true>
          : transform_impl<Expr, State, Data>
        {
        private:
            typedef typename result_of::child_c<Expr, 0>::type e0;
            typedef typename result_of::child_c<Expr, 1>::type e1;
            typedef typename Grammar::template impl<e0, State, Data>::result_type r0;
            typedef typename Grammar::template impl<e1, State, Data>::result_type r1;
        public:
            typedef detail::memfun<r0, r1> result_type;
            result_type const operator ()(
                typename default_mem_ptr_impl::expr_param e
              , typename default_mem_ptr_impl::state_param s
              , typename default_mem_ptr_impl::data_param d
            ) const
            {
                typename Grammar::template impl<e0, State, Data> t0;
                typename Grammar::template impl<e1, State, Data> t1;
                return detail::memfun<r0, r1>(
                    t0(proto::child_c<0>(e), s, d)
                  , t1(proto::child_c<1>(e), s, d)
                );
            }
        };

        template<typename Grammar>
        struct default_mem_ptr
          : transform<default_mem_ptr<Grammar> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : default_mem_ptr_impl<
                    Grammar
                  , Expr
                  , State
                  , Data
                  , is_member_function_invocation<Grammar, Expr, State, Data>::value
                >
            {};
        };

        template<typename Grammar>
        struct default_case<Grammar, tag::mem_ptr>
          : when<mem_ptr<Grammar, Grammar>, default_mem_ptr<Grammar> >
        {};

        template<typename Grammar>
        struct default_post_inc
          : transform<default_post_inc<Grammar> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : transform_impl<Expr, State, Data>
            {
            private:
                typedef typename result_of::child_c<Expr, 0>::type e0;
                typedef typename Grammar::template impl<e0, State, Data>::result_type r0;
            public:
                BOOST_PROTO_DECLTYPE_(proto::detail::make_mutable<r0>() ++, result_type)
                result_type operator ()(
                    typename impl::expr_param e
                  , typename impl::state_param s
                  , typename impl::data_param d
                ) const
                {
                    typename Grammar::template impl<e0, State, Data> t0;
                    return t0(proto::child_c<0>(e), s, d) ++;
                }
            };
        };

        template<typename Grammar>
        struct default_case<Grammar, tag::post_inc>
          : when<post_inc<Grammar>, default_post_inc<Grammar> >
        {};

        template<typename Grammar>
        struct default_post_dec
          : transform<default_post_dec<Grammar> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : transform_impl<Expr, State, Data>
            {
            private:
                typedef typename result_of::child_c<Expr, 0>::type e0;
                typedef typename Grammar::template impl<e0, State, Data>::result_type r0;
            public:
                BOOST_PROTO_DECLTYPE_(proto::detail::make_mutable<r0>() --, result_type)
                result_type operator ()(
                    typename impl::expr_param e
                  , typename impl::state_param s
                  , typename impl::data_param d
                ) const
                {
                    typename Grammar::template impl<e0, State, Data> t0;
                    return t0(proto::child_c<0>(e), s, d) --;
                }
            };
        };

        template<typename Grammar>
        struct default_case<Grammar, tag::post_dec>
          : when<post_dec<Grammar>, default_post_dec<Grammar> >
        {};

        template<typename Grammar>
        struct default_subscript
          : transform<default_subscript<Grammar> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : transform_impl<Expr, State, Data>
            {
            private:
                typedef typename result_of::child_c<Expr, 0>::type e0;
                typedef typename result_of::child_c<Expr, 1>::type e1;
                typedef typename Grammar::template impl<e0, State, Data>::result_type r0;
                typedef typename Grammar::template impl<e1, State, Data>::result_type r1;
            public:
                BOOST_PROTO_DECLTYPE_(
                    proto::detail::make_subscriptable<r0>() [ proto::detail::make<r1>() ]
                  , result_type
                )
                result_type operator ()(
                    typename impl::expr_param e
                  , typename impl::state_param s
                  , typename impl::data_param d
                ) const
                {
                    typename Grammar::template impl<e0, State, Data> t0;
                    typename Grammar::template impl<e1, State, Data> t1;
                    return t0(proto::child_c<0>(e), s, d) [
                            t1(proto::child_c<1>(e), s, d) ];
                }
            };
        };

        template<typename Grammar>
        struct default_case<Grammar, tag::subscript>
          : when<subscript<Grammar, Grammar>, default_subscript<Grammar> >
        {};

        template<typename Grammar>
        struct default_if_else_
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : transform_impl<Expr, State, Data>
            {
            private:
                typedef typename result_of::child_c<Expr, 0>::type e0;
                typedef typename result_of::child_c<Expr, 1>::type e1;
                typedef typename result_of::child_c<Expr, 2>::type e2;
                typedef typename Grammar::template impl<e0, State, Data>::result_type r0;
                typedef typename Grammar::template impl<e1, State, Data>::result_type r1;
                typedef typename Grammar::template impl<e2, State, Data>::result_type r2;
            public:
                BOOST_PROTO_DECLTYPE_(
                    proto::detail::make<r0>()
                  ? proto::detail::make<r1>()
                  : proto::detail::make<r2>()
                  , result_type
                )
                result_type operator ()(
                    typename impl::expr_param e
                  , typename impl::state_param s
                  , typename impl::data_param d
                ) const
                {
                    typename Grammar::template impl<e0, State, Data> t0;
                    typename Grammar::template impl<e1, State, Data> t1;
                    typename Grammar::template impl<e2, State, Data> t2;
                    return t0(proto::child_c<0>(e), s, d)
                          ? t1(proto::child_c<1>(e), s, d)
                          : t2(proto::child_c<2>(e), s, d);
                }
            };
        };

        template<typename Grammar>
        struct default_case<Grammar, tag::if_else_>
          : when<if_else_<Grammar, Grammar, Grammar>, default_if_else_<Grammar> >
        {};

        template<typename Grammar>
        struct default_comma
          : transform<default_comma<Grammar> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : transform_impl<Expr, State, Data>
            {
            private:
                typedef typename result_of::child_c<Expr, 0>::type e0;
                typedef typename result_of::child_c<Expr, 1>::type e1;
                typedef typename Grammar::template impl<e0, State, Data>::result_type r0;
                typedef typename Grammar::template impl<e1, State, Data>::result_type r1;
            public:
                typedef typename proto::detail::comma_result<r0, r1>::type result_type;
                result_type operator ()(
                    typename impl::expr_param e
                  , typename impl::state_param s
                  , typename impl::data_param d
                ) const
                {
                    typename Grammar::template impl<e0, State, Data> t0;
                    typename Grammar::template impl<e1, State, Data> t1;
                    return t0(proto::child_c<0>(e), s, d)
                          , t1(proto::child_c<1>(e), s, d);
                }
            };
        };

        template<typename Grammar>
        struct default_case<Grammar, tag::comma>
          : when<comma<Grammar, Grammar>, default_comma<Grammar> >
        {};

        template<typename Grammar, typename Expr, typename State, typename Data, long Arity>
        struct default_function_impl;

        template<typename Grammar>
        struct default_function
          : transform<default_function<Grammar> >
        {
            template<typename Expr, typename State, typename Data>
            struct impl
              : default_function_impl<
                    Grammar
                  , Expr
                  , State
                  , Data
                  , transform_impl<Expr, State, Data>::expr::proto_arity_c
                >
            {};
        };

        template<typename Grammar>
        struct default_case<Grammar, tag::function>
          : when<function<Grammar, vararg<Grammar> >, default_function<Grammar> >
        {};

        #define BOOST_PROTO_DEFAULT_EVAL_TYPE(Z, N, DATA)                                       \
            typedef                                                                             \
                typename result_of::child_c<DATA, N>::type                                      \
            BOOST_PP_CAT(e, N);                                                                 \
                                                                                                \
            typedef                                                                             \
                typename Grammar::template impl<BOOST_PP_CAT(e, N), State, Data>::result_type   \
            BOOST_PP_CAT(r, N);                                                                 \
            /**/

        #define BOOST_PROTO_DEFAULT_EVAL(Z, N, DATA)                                            \
            typename Grammar::template impl<BOOST_PP_CAT(e, N), State, Data>()(                 \
                proto::child_c<N>(DATA), s, d                                                   \
            )                                                                                   \
            /**/

        template<typename Grammar, typename Expr, typename State, typename Data>
        struct default_function_impl<Grammar, Expr, State, Data, 1>
          : transform_impl<Expr, State, Data>
        {
            BOOST_PROTO_DEFAULT_EVAL_TYPE(~, 0, Expr)

            typedef
                typename proto::detail::result_of_fixup<r0>::type
            function_type;

            typedef
                typename BOOST_PROTO_RESULT_OF<function_type()>::type
            result_type;

            result_type operator ()(
                typename default_function_impl::expr_param e
              , typename default_function_impl::state_param s
              , typename default_function_impl::data_param d
            ) const
            {
                return BOOST_PROTO_DEFAULT_EVAL(~, 0, e)();
            }
        };

        template<typename Grammar, typename Expr, typename State, typename Data>
        struct default_function_impl<Grammar, Expr, State, Data, 2>
          : transform_impl<Expr, State, Data>
        {
            BOOST_PROTO_DEFAULT_EVAL_TYPE(~, 0, Expr)
            BOOST_PROTO_DEFAULT_EVAL_TYPE(~, 1, Expr)

            typedef
                typename proto::detail::result_of_fixup<r0>::type
            function_type;

            typedef
                typename detail::result_of_<function_type(r1)>::type
            result_type;

            result_type operator ()(
                typename default_function_impl::expr_param e
              , typename default_function_impl::state_param s
              , typename default_function_impl::data_param d
            ) const
            {
                return this->invoke(
                    e
                  , s
                  , d
                  , is_member_function_pointer<function_type>()
                  , is_member_object_pointer<function_type>()
                );
            }

        private:
            result_type invoke(
                typename default_function_impl::expr_param e
              , typename default_function_impl::state_param s
              , typename default_function_impl::data_param d
              , mpl::false_
              , mpl::false_
            ) const
            {
                return BOOST_PROTO_DEFAULT_EVAL(~, 0, e)(BOOST_PROTO_DEFAULT_EVAL(~, 1, e));
            }

            result_type invoke(
                typename default_function_impl::expr_param e
              , typename default_function_impl::state_param s
              , typename default_function_impl::data_param d
              , mpl::true_
              , mpl::false_
            ) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename detail::class_member_traits<function_type>::class_type class_type;
                return (
                    BOOST_PROTO_GET_POINTER(class_type, (BOOST_PROTO_DEFAULT_EVAL(~, 1, e))) ->* 
                    BOOST_PROTO_DEFAULT_EVAL(~, 0, e)
                )();
            }

            result_type invoke(
                typename default_function_impl::expr_param e
              , typename default_function_impl::state_param s
              , typename default_function_impl::data_param d
              , mpl::false_
              , mpl::true_
            ) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename detail::class_member_traits<function_type>::class_type class_type;
                return (
                    BOOST_PROTO_GET_POINTER(class_type, (BOOST_PROTO_DEFAULT_EVAL(~, 1, e))) ->*
                    BOOST_PROTO_DEFAULT_EVAL(~, 0, e)
                );
            }
        };

        #include <boost/proto/transform/detail/default_function_impl.hpp>

        #undef BOOST_PROTO_DEFAULT_EVAL_TYPE
        #undef BOOST_PROTO_DEFAULT_EVAL
    }

    template<typename Grammar /*= detail::_default*/>
    struct _default
      : switch_<detail::default_cases<Grammar> >
    {};

    template<typename Grammar>
    struct is_callable<_default<Grammar> >
      : mpl::true_
    {};

    namespace detail
    {
        // Loopy indirection that allows proto::_default<> to be
        // used without specifying a Grammar argument.
        struct _default
          : proto::_default<>
        {};
    }

}}

#endif

