///////////////////////////////////////////////////////////////////////////////
/// \file default.hpp
/// Definintion of default_context, a default evaluation context for
/// proto::eval() that uses Boost.Typeof to deduce return types
/// of the built-in operators.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_CONTEXT_DEFAULT_HPP_EAN_01_08_2007
#define BOOST_PROTO_CONTEXT_DEFAULT_HPP_EAN_01_08_2007

#include <boost/config.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_shifted.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_member_pointer.hpp>
#include <boost/type_traits/is_member_object_pointer.hpp>
#include <boost/type_traits/is_member_function_pointer.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/tags.hpp>
#include <boost/proto/eval.hpp>
#include <boost/proto/traits.hpp> // for proto::child_c()
#include <boost/proto/detail/decltype.hpp>

namespace boost { namespace proto
{
/// INTERNAL ONLY
///
#define UNREF(x) typename boost::remove_reference<x>::type

    namespace context
    {
        template<
            typename Expr
          , typename Context
          , typename Tag        // = typename Expr::proto_tag
          , long Arity          // = Expr::proto_arity_c
        >
        struct default_eval
        {};

        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, tag::terminal, 0>
        {
            typedef
                typename proto::result_of::value<Expr &>::type
            result_type;

            result_type operator ()(Expr &expr, Context &) const
            {
                return proto::value(expr);
            }
        };

        /// INTERNAL ONLY
        ///
    #define BOOST_PROTO_UNARY_DEFAULT_EVAL(OP, TAG, MAKE)                                       \
        template<typename Expr, typename Context>                                               \
        struct default_eval<Expr, Context, TAG, 1>                                              \
        {                                                                                       \
        private:                                                                                \
            typedef typename proto::result_of::child_c<Expr, 0>::type e0;                       \
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;               \
        public:                                                                                 \
            BOOST_PROTO_DECLTYPE_(OP proto::detail::MAKE<r0>(), result_type)                    \
            result_type operator ()(Expr &expr, Context &ctx) const                             \
            {                                                                                   \
                return OP proto::eval(proto::child_c<0>(expr), ctx);                            \
            }                                                                                   \
        };                                                                                      \
        /**/

        /// INTERNAL ONLY
        ///
    #define BOOST_PROTO_BINARY_DEFAULT_EVAL(OP, TAG, LMAKE, RMAKE)                              \
        template<typename Expr, typename Context>                                               \
        struct default_eval<Expr, Context, TAG, 2>                                              \
        {                                                                                       \
        private:                                                                                \
            typedef typename proto::result_of::child_c<Expr, 0>::type e0;                       \
            typedef typename proto::result_of::child_c<Expr, 1>::type e1;                       \
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;               \
            typedef typename proto::result_of::eval<UNREF(e1), Context>::type r1;               \
        public:                                                                                 \
            BOOST_PROTO_DECLTYPE_(                                                              \
                proto::detail::LMAKE<r0>() OP proto::detail::RMAKE<r1>()                        \
              , result_type                                                                     \
            )                                                                                   \
            result_type operator ()(Expr &expr, Context &ctx) const                             \
            {                                                                                   \
                return proto::eval(                                                             \
                    proto::child_c<0>(expr), ctx) OP proto::eval(proto::child_c<1>(expr)        \
                  , ctx                                                                         \
                );                                                                              \
            }                                                                                   \
        };                                                                                      \
        /**/

        BOOST_PROTO_UNARY_DEFAULT_EVAL(+, proto::tag::unary_plus, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(-, proto::tag::negate, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(*, proto::tag::dereference, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(~, proto::tag::complement, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(&, proto::tag::address_of, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(!, proto::tag::logical_not, make)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(++, proto::tag::pre_inc, make_mutable)
        BOOST_PROTO_UNARY_DEFAULT_EVAL(--, proto::tag::pre_dec, make_mutable)

        BOOST_PROTO_BINARY_DEFAULT_EVAL(<<, proto::tag::shift_left, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(>>, proto::tag::shift_right, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(*, proto::tag::multiplies, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(/, proto::tag::divides, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(%, proto::tag::modulus, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(+, proto::tag::plus, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(-, proto::tag::minus, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(<, proto::tag::less, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(>, proto::tag::greater, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(<=, proto::tag::less_equal, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(>=, proto::tag::greater_equal, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(==, proto::tag::equal_to, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(!=, proto::tag::not_equal_to, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(||, proto::tag::logical_or, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(&&, proto::tag::logical_and, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(&, proto::tag::bitwise_and, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(|, proto::tag::bitwise_or, make, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(^, proto::tag::bitwise_xor, make, make)

        BOOST_PROTO_BINARY_DEFAULT_EVAL(=, proto::tag::assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(<<=, proto::tag::shift_left_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(>>=, proto::tag::shift_right_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(*=, proto::tag::multiplies_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(/=, proto::tag::divides_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(%=, proto::tag::modulus_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(+=, proto::tag::plus_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(-=, proto::tag::minus_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(&=, proto::tag::bitwise_and_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(|=, proto::tag::bitwise_or_assign, make_mutable, make)
        BOOST_PROTO_BINARY_DEFAULT_EVAL(^=, proto::tag::bitwise_xor_assign, make_mutable, make)

    #undef BOOST_PROTO_UNARY_DEFAULT_EVAL
    #undef BOOST_PROTO_BINARY_DEFAULT_EVAL

        /// INTERNAL ONLY
        template<typename Expr, typename Context>
        struct is_member_function_eval
          : is_member_function_pointer<
                typename detail::uncvref<
                    typename proto::result_of::eval<
                        typename remove_reference<
                            typename proto::result_of::child_c<Expr, 1>::type
                        >::type
                      , Context
                    >::type
                >::type
            >
        {};

        /// INTERNAL ONLY
        template<typename Expr, typename Context, bool IsMemFunCall>
        struct memfun_eval
        {
        private:
            typedef typename result_of::child_c<Expr, 0>::type e0;
            typedef typename result_of::child_c<Expr, 1>::type e1;
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;
            typedef typename proto::result_of::eval<UNREF(e1), Context>::type r1;
        public:
            typedef typename detail::mem_ptr_fun<r0, r1>::result_type result_type;
            result_type operator ()(Expr &expr, Context &ctx) const
            {
                return detail::mem_ptr_fun<r0, r1>()(
                    proto::eval(proto::child_c<0>(expr), ctx)
                  , proto::eval(proto::child_c<1>(expr), ctx)
                );
            }
        };

        /// INTERNAL ONLY
        template<typename Expr, typename Context>
        struct memfun_eval<Expr, Context, true>
        {
        private:
            typedef typename result_of::child_c<Expr, 0>::type e0;
            typedef typename result_of::child_c<Expr, 1>::type e1;
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;
            typedef typename proto::result_of::eval<UNREF(e1), Context>::type r1;
        public:
            typedef detail::memfun<r0, r1> result_type;
            result_type const operator ()(Expr &expr, Context &ctx) const
            {
                return detail::memfun<r0, r1>(
                    proto::eval(proto::child_c<0>(expr), ctx)
                  , proto::eval(proto::child_c<1>(expr), ctx)
                );
            }
        };

        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, tag::mem_ptr, 2>
          : memfun_eval<Expr, Context, is_member_function_eval<Expr, Context>::value>
        {};

        // Handle post-increment specially.
        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, proto::tag::post_inc, 1>
        {
        private:
            typedef typename proto::result_of::child_c<Expr, 0>::type e0;
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;
        public:
            BOOST_PROTO_DECLTYPE_(proto::detail::make_mutable<r0>() ++, result_type)
            result_type operator ()(Expr &expr, Context &ctx) const
            {
                return proto::eval(proto::child_c<0>(expr), ctx) ++;
            }
        };

        // Handle post-decrement specially.
        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, proto::tag::post_dec, 1>
        {
        private:
            typedef typename proto::result_of::child_c<Expr, 0>::type e0;
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;
        public:
            BOOST_PROTO_DECLTYPE_(proto::detail::make_mutable<r0>() --, result_type)
            result_type operator ()(Expr &expr, Context &ctx) const
            {
                return proto::eval(proto::child_c<0>(expr), ctx) --;
            }
        };

        // Handle subscript specially.
        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, proto::tag::subscript, 2>
        {
        private:
            typedef typename proto::result_of::child_c<Expr, 0>::type e0;
            typedef typename proto::result_of::child_c<Expr, 1>::type e1;
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;
            typedef typename proto::result_of::eval<UNREF(e1), Context>::type r1;
        public:
            BOOST_PROTO_DECLTYPE_(proto::detail::make_subscriptable<r0>()[proto::detail::make<r1>()], result_type)
            result_type operator ()(Expr &expr, Context &ctx) const
            {
                return proto::eval(proto::child_c<0>(expr), ctx)[proto::eval(proto::child_c<1>(expr), ctx)];
            }
        };

        // Handle if_else_ specially.
        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, proto::tag::if_else_, 3>
        {
        private:
            typedef typename proto::result_of::child_c<Expr, 0>::type e0;
            typedef typename proto::result_of::child_c<Expr, 1>::type e1;
            typedef typename proto::result_of::child_c<Expr, 2>::type e2;
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;
            typedef typename proto::result_of::eval<UNREF(e1), Context>::type r1;
            typedef typename proto::result_of::eval<UNREF(e2), Context>::type r2;
        public:
            BOOST_PROTO_DECLTYPE_(
                proto::detail::make<r0>()
              ? proto::detail::make<r1>()
              : proto::detail::make<r2>()
              , result_type
            )
            result_type operator ()(Expr &expr, Context &ctx) const
            {
                return proto::eval(proto::child_c<0>(expr), ctx)
                      ? proto::eval(proto::child_c<1>(expr), ctx)
                      : proto::eval(proto::child_c<2>(expr), ctx);
            }
        };

        // Handle comma specially.
        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, proto::tag::comma, 2>
        {
        private:
            typedef typename proto::result_of::child_c<Expr, 0>::type e0;
            typedef typename proto::result_of::child_c<Expr, 1>::type e1;
            typedef typename proto::result_of::eval<UNREF(e0), Context>::type r0;
            typedef typename proto::result_of::eval<UNREF(e1), Context>::type r1;
        public:
            typedef typename proto::detail::comma_result<r0, r1>::type result_type;
            result_type operator ()(Expr &expr, Context &ctx) const
            {
                return proto::eval(proto::child_c<0>(expr), ctx), proto::eval(proto::child_c<1>(expr), ctx);
            }
        };

        // Handle function specially
        #define BOOST_PROTO_DEFAULT_EVAL_TYPE(Z, N, DATA)                                       \
            typename proto::result_of::eval<                                                    \
                typename remove_reference<                                                      \
                    typename proto::result_of::child_c<DATA, N>::type                           \
                >::type                                                                         \
              , Context                                                                         \
            >::type                                                                             \
            /**/

        #define BOOST_PROTO_DEFAULT_EVAL(Z, N, DATA)                                            \
            proto::eval(proto::child_c<N>(DATA), context)                                       \
            /**/

        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, proto::tag::function, 1>
        {
            typedef
                typename proto::detail::result_of_fixup<
                    BOOST_PROTO_DEFAULT_EVAL_TYPE(~, 0, Expr)
                >::type
            function_type;

            typedef
                typename BOOST_PROTO_RESULT_OF<function_type()>::type
            result_type;

            result_type operator ()(Expr &expr, Context &context) const
            {
                return BOOST_PROTO_DEFAULT_EVAL(~, 0, expr)();
            }
        };

        template<typename Expr, typename Context>
        struct default_eval<Expr, Context, proto::tag::function, 2>
        {
            typedef
                typename proto::detail::result_of_fixup<
                    BOOST_PROTO_DEFAULT_EVAL_TYPE(~, 0, Expr)
                >::type
            function_type;

            typedef
                typename detail::result_of_<
                    function_type(BOOST_PROTO_DEFAULT_EVAL_TYPE(~, 1, Expr))
                >::type
            result_type;

            result_type operator ()(Expr &expr, Context &context) const
            {
                return this->invoke(
                    expr
                  , context
                  , is_member_function_pointer<function_type>()
                  , is_member_object_pointer<function_type>()
                );
            }

        private:
            result_type invoke(Expr &expr, Context &context, mpl::false_, mpl::false_) const
            {
                return BOOST_PROTO_DEFAULT_EVAL(~, 0, expr)(BOOST_PROTO_DEFAULT_EVAL(~, 1, expr));
            }

            result_type invoke(Expr &expr, Context &context, mpl::true_, mpl::false_) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename detail::class_member_traits<function_type>::class_type class_type;
                return (
                    BOOST_PROTO_GET_POINTER(class_type, (BOOST_PROTO_DEFAULT_EVAL(~, 1, expr))) ->* 
                    BOOST_PROTO_DEFAULT_EVAL(~, 0, expr)
                )();
            }

            result_type invoke(Expr &expr, Context &context, mpl::false_, mpl::true_) const
            {
                BOOST_PROTO_USE_GET_POINTER();
                typedef typename detail::class_member_traits<function_type>::class_type class_type;
                return (
                    BOOST_PROTO_GET_POINTER(class_type, (BOOST_PROTO_DEFAULT_EVAL(~, 1, expr))) ->*
                    BOOST_PROTO_DEFAULT_EVAL(~, 0, expr)
                );
            }
        };

        // Additional specialization are generated by the preprocessor
        #include <boost/proto/context/detail/default_eval.hpp>

        #undef BOOST_PROTO_DEFAULT_EVAL_TYPE
        #undef BOOST_PROTO_DEFAULT_EVAL

        /// default_context
        ///
        struct default_context
        {
            /// default_context::eval
            ///
            template<typename Expr, typename ThisContext = default_context const>
            struct eval
              : default_eval<Expr, ThisContext>
            {};
        };

    } // namespace context

}} // namespace boost::proto

#undef UNREF

#endif
