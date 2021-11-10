// Copyright (C) 2016-2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_YAP_DETAIL_TRANSFORM_HPP_INCLUDED
#define BOOST_YAP_DETAIL_TRANSFORM_HPP_INCLUDED

#include <boost/yap/algorithm_fwd.hpp>

#include <boost/hana/transform.hpp>

#include <cassert>


namespace boost { namespace yap { namespace detail {

    template<int I, typename T, typename... Ts>
    struct nth_element_impl
    {
        using type = typename nth_element_impl<I - 1, Ts...>::type;
    };

    template<typename T, typename... Ts>
    struct nth_element_impl<0, T, Ts...>
    {
        using type = T;
    };

    template<int I, typename... Ts>
    using nth_element = typename nth_element_impl<I, Ts...>::type;

    template<typename T, bool RemoveRefs = std::is_rvalue_reference<T>::value>
    struct rvalue_ref_to_value;

    template<typename T>
    struct rvalue_ref_to_value<T, true>
    {
        using type = typename std::remove_reference<T>::type;
    };

    template<typename T>
    struct rvalue_ref_to_value<T, false>
    {
        using type = T;
    };

    template<typename T>
    using rvalue_ref_to_value_t = typename rvalue_ref_to_value<T>::type;

    template<bool IsRvalueRef>
    struct rvalue_mover
    {
        template<typename T>
        constexpr decltype(auto) operator()(T && t) const
        {
            return static_cast<T &&>(t);
        }
    };

    template<>
    struct rvalue_mover<true>
    {
        template<typename T>
        constexpr std::remove_reference_t<T> operator()(T && t) const
        {
            return std::move(t);
        }
    };

    template<typename... PlaceholderArgs>
    struct placeholder_transform_t
    {
        using tuple_t = hana::tuple<rvalue_ref_to_value_t<PlaceholderArgs>...>;

        constexpr placeholder_transform_t(PlaceholderArgs &&... args) :
            placeholder_args_(static_cast<PlaceholderArgs &&>(args)...)
        {}

        template<long long I>
        constexpr decltype(auto)
        operator()(expr_tag<expr_kind::terminal>, boost::yap::placeholder<I>) const
        {
            static_assert(
                I <= decltype(hana::size(std::declval<tuple_t>()))::value,
                "Out of range placeholder index,");
            using nth_type = nth_element<I - 1, PlaceholderArgs...>;
            return as_expr<minimal_expr>(
                rvalue_mover<!std::is_lvalue_reference<nth_type>::value>{}(
                    placeholder_args_[hana::llong<I - 1>{}]));
        }

        tuple_t placeholder_args_;
    };

    template<typename... PlaceholderArgs>
    struct evaluation_transform_t
    {
        using tuple_t = hana::tuple<rvalue_ref_to_value_t<PlaceholderArgs>...>;

        constexpr evaluation_transform_t(PlaceholderArgs &&... args) :
            placeholder_args_(static_cast<PlaceholderArgs &&>(args)...)
        {}

        template<long long I>
        constexpr decltype(auto)
        operator()(expr_tag<expr_kind::terminal>, boost::yap::placeholder<I>) const
        {
            static_assert(
                I <= decltype(hana::size(std::declval<tuple_t>()))::value,
                "Out of range placeholder index,");
            using nth_type = nth_element<I - 1, PlaceholderArgs...>;
            return rvalue_mover<!std::is_lvalue_reference<nth_type>::value>{}(
                placeholder_args_[hana::llong<I - 1>{}]);
        }

        template<typename T>
        constexpr decltype(auto) operator()(expr_tag<expr_kind::terminal>, T && t) const
        {
            return static_cast<T &&>(t);
        }

#define BOOST_YAP_UNARY_OPERATOR_CASE(op, op_name)                             \
    template<typename T>                                                       \
    constexpr decltype(auto) operator()(expr_tag<expr_kind::op_name>, T && t) const \
    {                                                                          \
        return op transform(                                                   \
            as_expr<minimal_expr>(static_cast<T &&>(t)), *this);               \
    }

        BOOST_YAP_UNARY_OPERATOR_CASE(+, unary_plus)
        BOOST_YAP_UNARY_OPERATOR_CASE(-, negate)
        BOOST_YAP_UNARY_OPERATOR_CASE(*, dereference)
        BOOST_YAP_UNARY_OPERATOR_CASE(~, complement)
        BOOST_YAP_UNARY_OPERATOR_CASE(&, address_of)
        BOOST_YAP_UNARY_OPERATOR_CASE(!, logical_not)
        BOOST_YAP_UNARY_OPERATOR_CASE(++, pre_inc)
        BOOST_YAP_UNARY_OPERATOR_CASE(--, pre_dec)

        template<typename T>
        constexpr decltype(auto) operator()(expr_tag<expr_kind::post_inc>, T && t) const
        {
            return transform(
                as_expr<minimal_expr>(static_cast<T &&>(t)), *this)++;
        }
        template<typename T>
        constexpr decltype(auto) operator()(expr_tag<expr_kind::post_dec>, T && t) const
        {
            return transform(
                as_expr<minimal_expr>(static_cast<T &&>(t)), *this)--;
        }

#undef BOOST_YAP_UNARY_OPERATOR_CASE

#define BOOST_YAP_BINARY_OPERATOR_CASE(op, op_name)                            \
    template<typename T, typename U>                                           \
    constexpr decltype(auto) operator()(expr_tag<expr_kind::op_name>, T && t, U && u) const \
    {                                                                          \
        return transform(as_expr<minimal_expr>(static_cast<T &&>(t)), *this)   \
            op transform(as_expr<minimal_expr>(static_cast<U &&>(u)), *this);  \
    }

        BOOST_YAP_BINARY_OPERATOR_CASE(<<, shift_left)
        BOOST_YAP_BINARY_OPERATOR_CASE(>>, shift_right)
        BOOST_YAP_BINARY_OPERATOR_CASE(*, multiplies)
        BOOST_YAP_BINARY_OPERATOR_CASE(/, divides)
        BOOST_YAP_BINARY_OPERATOR_CASE(%, modulus)
        BOOST_YAP_BINARY_OPERATOR_CASE(+, plus)
        BOOST_YAP_BINARY_OPERATOR_CASE(-, minus)
        BOOST_YAP_BINARY_OPERATOR_CASE(<, less)
        BOOST_YAP_BINARY_OPERATOR_CASE(>, greater)
        BOOST_YAP_BINARY_OPERATOR_CASE(<=, less_equal)
        BOOST_YAP_BINARY_OPERATOR_CASE(>=, greater_equal)
        BOOST_YAP_BINARY_OPERATOR_CASE(==, equal_to)
        BOOST_YAP_BINARY_OPERATOR_CASE(!=, not_equal_to)
        BOOST_YAP_BINARY_OPERATOR_CASE(||, logical_or)
        BOOST_YAP_BINARY_OPERATOR_CASE(&&, logical_and)
        BOOST_YAP_BINARY_OPERATOR_CASE(&, bitwise_and)
        BOOST_YAP_BINARY_OPERATOR_CASE(|, bitwise_or)
        BOOST_YAP_BINARY_OPERATOR_CASE (^, bitwise_xor)

        // clang-format off
//[ evaluation_transform_comma
        template<typename T, typename U>
        constexpr decltype(auto) operator()(expr_tag<expr_kind::comma>, T && t, U && u) const
        {
            return transform(
                       as_expr<minimal_expr>(static_cast<T &&>(t)), *this),
                   transform(
                       as_expr<minimal_expr>(static_cast<U &&>(u)), *this);
        }
//]
        // clang-format on

        BOOST_YAP_BINARY_OPERATOR_CASE(->*, mem_ptr)
        BOOST_YAP_BINARY_OPERATOR_CASE(=, assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(<<=, shift_left_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(>>=, shift_right_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(*=, multiplies_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(/=, divides_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(%=, modulus_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(+=, plus_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(-=, minus_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(&=, bitwise_and_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(|=, bitwise_or_assign)
        BOOST_YAP_BINARY_OPERATOR_CASE(^=, bitwise_xor_assign)

        template<typename T, typename U>
        constexpr decltype(auto)
        operator()(expr_tag<expr_kind::subscript>, T && t, U && u) const
        {
            return transform(
                as_expr<minimal_expr>(static_cast<T &&>(t)), *this)[transform(
                as_expr<minimal_expr>(static_cast<U &&>(u)), *this)];
        }

#undef BOOST_YAP_BINARY_OPERATOR_CASE

        template<typename T, typename U, typename V>
        constexpr decltype(auto)
        operator()(expr_tag<expr_kind::if_else>, T && t, U && u, V && v) const
        {
            return transform(as_expr<minimal_expr>(static_cast<T &&>(t)), *this)
                       ? transform(
                             as_expr<minimal_expr>(static_cast<U &&>(u)), *this)
                       : transform(
                             as_expr<minimal_expr>(static_cast<V &&>(v)),
                             *this);
        }

        // clang-format off
//[ evaluation_transform_call
        template<typename Callable, typename... Args>
        constexpr decltype(auto) operator()(
            expr_tag<expr_kind::call>, Callable && callable, Args &&... args) const
        {
            return transform(as_expr<minimal_expr>(static_cast<Callable &&>(callable)), *this)(
                transform(as_expr<minimal_expr>(static_cast<Args &&>(args)), *this)...
            );
        }
//]
        // clang-format on

        tuple_t placeholder_args_;
    };


    template<bool Strict, int I, bool IsExprRef>
    struct transform_impl;

    template<
        bool Strict,
        typename Expr,
        typename TransformTuple,
        int I,
        expr_arity Arity,
        typename = void_t<>>
    struct transform_expression_tag;


    // Forward terminals/recurively transform noterminasl; attempted last.

    template<bool IsLvalueRef, bool IsTerminal, bool Strict>
    struct default_transform
    {
        template<typename Expr, typename TransformTuple>
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            return static_cast<Expr &&>(expr);
        }
    };

    template<bool IsLvalueRef, bool IsTerminal>
    struct default_transform<IsLvalueRef, IsTerminal, true>
    {
        struct incomplete;

        // If you're getting an error because this function is uncallable,
        // that's by design.  You called yap::transform_strict(expr, xfrom)
        // and one or more subexpression of 'expr' are not callable with any
        // overload in 'xform'.
        template<typename Expr, typename TransformTuple>
        constexpr incomplete operator()(Expr && expr, TransformTuple transforms) const;
    };

    template<
        expr_kind Kind,
        template<expr_kind, class> class ExprTemplate,
        typename OldTuple,
        typename NewTuple>
    constexpr auto make_expr_from_tuple(
        ExprTemplate<Kind, OldTuple> const & expr, NewTuple && tuple)
    {
        return ExprTemplate<Kind, NewTuple>{std::move(tuple)};
    }

    template<expr_kind Kind, typename Expr, typename NewTuple>
    constexpr auto make_expr_from_tuple(Expr const & expr, NewTuple && tuple)
    {
        return minimal_expr<Kind, NewTuple>{std::move(tuple)};
    }

    template<typename Expr, typename Tuple, typename TransformTuple>
    constexpr decltype(auto) transform_nonterminal(
        Expr const & expr, Tuple && tuple, TransformTuple transforms)
    {
        auto transformed_tuple =
            hana::transform(static_cast<Tuple &&>(tuple), [&](auto && element) {
                using element_t = decltype(element);
                auto const kind = remove_cv_ref_t<element_t>::kind;
                ::boost::yap::detail::
                    transform_impl<false, 0, kind == expr_kind::expr_ref>
                        xform;
                return xform(static_cast<element_t &&>(element), transforms);
            });
        auto const kind = remove_cv_ref_t<Expr>::kind;
        return make_expr_from_tuple<kind>(expr, std::move(transformed_tuple));
    }

    template<>
    struct default_transform<true, false, false>
    {
        template<typename Expr, typename TransformTuple>
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            return transform_nonterminal(expr, expr.elements, transforms);
        }
    };

    template<>
    struct default_transform<false, false, false>
    {
        template<typename Expr, typename TransformTuple>
        constexpr decltype(auto)
        operator()(Expr && expr, TransformTuple transforms) const
        {
            return transform_nonterminal(
                expr, std::move(expr.elements), transforms);
        }
    };

    // Dispatch to the next transform, or to the default transform if there is
    // no next transform.

    template<
        bool Strict,
        typename Expr,
        typename TransformTuple,
        int I,
        bool NextTransformExists>
    struct next_or_default_transform
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            // Use the next transform.
            constexpr expr_kind kind = remove_cv_ref_t<Expr>::kind;
            return detail::
                transform_impl<Strict, I + 1, kind == expr_kind::expr_ref>{}(
                    static_cast<Expr &&>(expr), transforms);
        }
    };

    template<bool Strict, typename Expr, typename TransformTuple, int I>
    struct next_or_default_transform<Strict, Expr, TransformTuple, I, false>
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            // No next transform exists; use the default transform.
            constexpr expr_kind kind = remove_cv_ref_t<Expr>::kind;
            return default_transform<
                std::is_lvalue_reference<Expr>::value,
                kind == expr_kind::terminal,
                Strict>{}(static_cast<Expr &&>(expr), transforms);
        }
    };

    // Expression-matching; attempted second.

    template<
        bool Strict,
        typename Expr,
        typename TransformTuple,
        int I,
        typename = detail::void_t<>>
    struct transform_expression_expr
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            // No expr-matching succeeded; use the next or default transform.
            return next_or_default_transform<
                Strict,
                Expr,
                TransformTuple,
                I,
                I + 1 < decltype(hana::size(
                            std::declval<TransformTuple>()))::value>{}(
                static_cast<Expr &&>(expr), transforms);
        }
    };

    template<bool Strict, typename Expr, typename TransformTuple, int I>
    struct transform_expression_expr<
        Strict,
        Expr,
        TransformTuple,
        I,
        void_t<decltype((*std::declval<TransformTuple>()[hana::llong<I>{}])(
            std::declval<Expr>()))>>
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            return (*transforms[hana::llong<I>{}])(static_cast<Expr &&>(expr));
        }
    };


    // Tag-matching; attempted first.

    template<
        bool Strict,
        typename Expr,
        typename TransformTuple,
        int I,
        expr_arity Arity,
        typename>
    struct transform_expression_tag
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            // No tag-matching succeeded; try expr-matching.
            return transform_expression_expr<Strict, Expr, TransformTuple, I>{}(
                static_cast<Expr &&>(expr), transforms);
        }
    };

    template<typename T>
    constexpr decltype(auto) terminal_value(T && x)
    {
        return value_impl<true>(static_cast<T &&>(x));
    }


    template<bool Strict, typename Expr, typename TransformTuple, int I>
    struct transform_expression_tag<
        Strict,
        Expr,
        TransformTuple,
        I,
        expr_arity::one,
        void_t<decltype((*std::declval<TransformTuple>()[hana::llong<I>{}])(
            expr_tag<remove_cv_ref_t<Expr>::kind>{},
            terminal_value(::boost::yap::value(std::declval<Expr>()))))>>
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            return (*transforms[hana::llong<I>{}])(
                expr_tag<remove_cv_ref_t<Expr>::kind>{},
                terminal_value(
                    ::boost::yap::value(static_cast<Expr &&>(expr))));
        }
    };

    template<bool Strict, typename Expr, typename TransformTuple, int I>
    struct transform_expression_tag<
        Strict,
        Expr,
        TransformTuple,
        I,
        expr_arity::two,
        void_t<decltype((*std::declval<TransformTuple>()[hana::llong<I>{}])(
            expr_tag<remove_cv_ref_t<Expr>::kind>{},
            terminal_value(::boost::yap::left(std::declval<Expr>())),
            terminal_value(::boost::yap::right(std::declval<Expr>()))))>>
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            return (*transforms[hana::llong<I>{}])(
                expr_tag<remove_cv_ref_t<Expr>::kind>{},
                terminal_value(::boost::yap::left(static_cast<Expr &&>(expr))),
                terminal_value(
                    ::boost::yap::right(static_cast<Expr &&>(expr))));
        }
    };

    template<bool Strict, typename Expr, typename TransformTuple, int I>
    struct transform_expression_tag<
        Strict,
        Expr,
        TransformTuple,
        I,
        expr_arity::three,
        void_t<decltype((*std::declval<TransformTuple>()[hana::llong<I>{}])(
            expr_tag<remove_cv_ref_t<Expr>::kind>{},
            terminal_value(::boost::yap::cond(std::declval<Expr>())),
            terminal_value(::boost::yap::then(std::declval<Expr>())),
            terminal_value(::boost::yap::else_(std::declval<Expr>()))))>>
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            return (*transforms[hana::llong<I>{}])(
                expr_tag<remove_cv_ref_t<Expr>::kind>{},
                terminal_value(::boost::yap::cond(static_cast<Expr &&>(expr))),
                terminal_value(::boost::yap::then(static_cast<Expr &&>(expr))),
                terminal_value(
                    ::boost::yap::else_(static_cast<Expr &&>(expr))));
        }
    };

    template<typename Expr, typename Transform>
    struct transform_call_unpacker
    {
        template<long long... I>
        constexpr auto operator()(
            Expr && expr,
            Transform & transform,
            std::integer_sequence<long long, I...>) const
            -> decltype(transform(
                expr_tag<expr_kind::call>{},
                terminal_value(::boost::yap::get(
                    static_cast<Expr &&>(expr), hana::llong_c<I>))...))
        {
            return transform(
                expr_tag<expr_kind::call>{},
                terminal_value(::boost::yap::get(
                    static_cast<Expr &&>(expr), hana::llong_c<I>))...);
        }
    };

    template<typename Expr>
    constexpr auto indices_for(Expr const & expr)
    {
        constexpr long long size = decltype(hana::size(expr.elements))::value;
        return std::make_integer_sequence<long long, size>();
    }

    template<bool Strict, typename Expr, typename TransformTuple, int I>
    struct transform_expression_tag<
        Strict,
        Expr,
        TransformTuple,
        I,
        expr_arity::n,
        void_t<decltype(
            transform_call_unpacker<
                Expr,
                decltype(*std::declval<TransformTuple>()[hana::llong<I>{}])>{}(
                std::declval<Expr>(),
                *std::declval<TransformTuple>()[hana::llong<I>{}],
                indices_for(std::declval<Expr>())))>>
    {
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            using transform_t = decltype(*transforms[hana::llong<I>{}]);
            return transform_call_unpacker<Expr, transform_t>{}(
                static_cast<Expr &&>(expr),
                *transforms[hana::llong<I>{}],
                indices_for(expr));
        }
    };

    template<bool Strict, int I, bool IsExprRef>
    struct transform_impl
    {
        template<typename Expr, typename TransformTuple>
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
            return detail::transform_expression_tag<
                Strict,
                Expr,
                TransformTuple,
                I,
                detail::arity_of<kind>()>{}(
                static_cast<Expr &&>(expr), transforms);
        }
    };

    template<bool Strict, int I>
    struct transform_impl<Strict, I, true>
    {
        template<typename Expr, typename TransformTuple>
        constexpr decltype(auto) operator()(Expr && expr, TransformTuple transforms) const
        {
            return detail::transform_impl<Strict, I, false>{}(
                ::boost::yap::deref(static_cast<Expr &&>(expr)), transforms);
        }
    };

}}}

#endif
