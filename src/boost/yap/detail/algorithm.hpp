// Copyright (C) 2016-2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_YAP_DETAIL_EXPRESSION_HPP_INCLUDED
#define BOOST_YAP_DETAIL_EXPRESSION_HPP_INCLUDED

#include <boost/yap/algorithm_fwd.hpp>

#include <boost/hana/size.hpp>
#include <boost/hana/tuple.hpp>

#include <memory>
#include <type_traits>


namespace boost { namespace yap { namespace detail {

    // static_const

    template<typename T>
    struct static_const
    {
        static constexpr T value{};
    };

    template<typename T>
    constexpr T static_const<T>::value;


    // partial_decay

    template<typename T>
    struct partial_decay
    {
        using type = T;
    };

    template<typename T>
    struct partial_decay<T[]>
    {
        using type = T *;
    };
    template<typename T, std::size_t N>
    struct partial_decay<T[N]>
    {
        using type = T *;
    };

    template<typename T>
    struct partial_decay<T (&)[]>
    {
        using type = T *;
    };
    template<typename T, std::size_t N>
    struct partial_decay<T (&)[N]>
    {
        using type = T *;
    };

    template<typename R, typename... A>
    struct partial_decay<R(A...)>
    {
        using type = R (*)(A...);
    };
    template<typename R, typename... A>
    struct partial_decay<R(A..., ...)>
    {
        using type = R (*)(A..., ...);
    };

    template<typename R, typename... A>
    struct partial_decay<R (&)(A...)>
    {
        using type = R (*)(A...);
    };
    template<typename R, typename... A>
    struct partial_decay<R (&)(A..., ...)>
    {
        using type = R (*)(A..., ...);
    };

    template<typename R, typename... A>
    struct partial_decay<R (*&)(A...)>
    {
        using type = R (*)(A...);
    };
    template<typename R, typename... A>
    struct partial_decay<R (*&)(A..., ...)>
    {
        using type = R (*)(A..., ...);
    };


    // operand_value_type_phase_1

    template<
        typename T,
        typename U = typename detail::partial_decay<T>::type,
        bool AddRValueRef = std::is_same<T, U>::value && !std::is_const<U>::value>
    struct operand_value_type_phase_1;

    template<typename T, typename U>
    struct operand_value_type_phase_1<T, U, true>
    {
        using type = U &&;
    };

    template<typename T, typename U>
    struct operand_value_type_phase_1<T, U, false>
    {
        using type = U;
    };


    // expr_ref

    template<template<expr_kind, class> class ExprTemplate, typename T>
    struct expr_ref
    {
        using type = expression_ref<ExprTemplate, T>;
    };

    template<template<expr_kind, class> class ExprTemplate, typename Tuple>
    struct expr_ref<ExprTemplate, ExprTemplate<expr_kind::expr_ref, Tuple> &>
    {
        using type = ExprTemplate<expr_kind::expr_ref, Tuple>;
    };

    template<template<expr_kind, class> class ExprTemplate, typename Tuple>
    struct expr_ref<
        ExprTemplate,
        ExprTemplate<expr_kind::expr_ref, Tuple> const &>
    {
        using type = ExprTemplate<expr_kind::expr_ref, Tuple>;
    };

    template<template<expr_kind, class> class ExprTemplate, typename T>
    using expr_ref_t = typename expr_ref<ExprTemplate, T>::type;

    template<template<expr_kind, class> class ExprTemplate, typename T>
    struct expr_ref_tuple;

    template<template<expr_kind, class> class ExprTemplate, typename Tuple>
    struct expr_ref_tuple<
        ExprTemplate,
        ExprTemplate<expr_kind::expr_ref, Tuple>>
    {
        using type = Tuple;
    };

    template<template<expr_kind, class> class ExprTemplate, typename T>
    using expr_ref_tuple_t = typename expr_ref_tuple<ExprTemplate, T>::type;


    // operand_type

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U = typename operand_value_type_phase_1<T>::type,
        bool RemoveRefs = std::is_rvalue_reference<U>::value,
        bool IsExpr = is_expr<T>::value,
        bool IsLRef = std::is_lvalue_reference<T>::value>
    struct operand_type;

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        bool RemoveRefs>
    struct operand_type<ExprTemplate, T, U, RemoveRefs, true, false>
    {
        using type = remove_cv_ref_t<T>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        bool RemoveRefs>
    struct operand_type<ExprTemplate, T, U, RemoveRefs, true, true>
    {
        using type = expr_ref_t<ExprTemplate, T>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        bool RemoveRefs,
        bool IsLRef>
    struct operand_type<ExprTemplate, T, U, RemoveRefs, true, IsLRef>
    {
        using type = remove_cv_ref_t<T>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        bool IsLRef>
    struct operand_type<ExprTemplate, T, U, true, false, IsLRef>
    {
        using type = terminal<ExprTemplate, std::remove_reference_t<U>>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        bool IsLRef>
    struct operand_type<ExprTemplate, T, U, false, false, IsLRef>
    {
        using type = terminal<ExprTemplate, U>;
    };

    template<template<expr_kind, class> class ExprTemplate, typename T>
    using operand_type_t = typename operand_type<ExprTemplate, T>::type;


    // make_operand

    template<typename T>
    struct make_operand
    {
        template<typename U>
        constexpr auto operator()(U && u)
        {
            return T{static_cast<U &&>(u)};
        }
    };

    template<template<expr_kind, class> class ExprTemplate, typename Tuple>
    struct make_operand<ExprTemplate<expr_kind::expr_ref, Tuple>>
    {
        constexpr auto operator()(ExprTemplate<expr_kind::expr_ref, Tuple> expr)
        {
            return expr;
        }

        template<typename U>
        constexpr auto operator()(U && u)
        {
            return ExprTemplate<expr_kind::expr_ref, Tuple>{
                Tuple{std::addressof(u)}};
        }
    };


    // free_binary_op_result

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U,
        bool TNonExprUExpr = !is_expr<T>::value && is_expr<U>::value,
        bool ULvalueRef = std::is_lvalue_reference<U>::value>
    struct free_binary_op_result;

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U>
    struct free_binary_op_result<ExprTemplate, OpKind, T, U, true, true>
    {
        using lhs_type = operand_type_t<ExprTemplate, T>;
        using rhs_type = expr_ref_t<ExprTemplate, U>;
        using rhs_tuple_type = expr_ref_tuple_t<ExprTemplate, rhs_type>;
        using type = ExprTemplate<OpKind, hana::tuple<lhs_type, rhs_type>>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U>
    struct free_binary_op_result<ExprTemplate, OpKind, T, U, true, false>
    {
        using lhs_type = operand_type_t<ExprTemplate, T>;
        using rhs_type = remove_cv_ref_t<U>;
        using type = ExprTemplate<OpKind, hana::tuple<lhs_type, rhs_type>>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U>
    using free_binary_op_result_t =
        typename free_binary_op_result<ExprTemplate, OpKind, T, U>::type;


    // ternary_op_result

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        typename V,
        bool Valid =
            is_expr<T>::value || is_expr<U>::value || is_expr<V>::value>
    struct ternary_op_result;

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        typename V>
    struct ternary_op_result<ExprTemplate, T, U, V, true>
    {
        using cond_type = operand_type_t<ExprTemplate, T>;
        using then_type = operand_type_t<ExprTemplate, U>;
        using else_type = operand_type_t<ExprTemplate, V>;
        using type = ExprTemplate<
            expr_kind::if_else,
            hana::tuple<cond_type, then_type, else_type>>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        typename V>
    using ternary_op_result_t =
        typename ternary_op_result<ExprTemplate, T, U, V>::type;


    // udt_any_ternary_op_result

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        typename V,
        template<class> class UdtTrait,
        bool Valid = !is_expr<T>::value && !is_expr<U>::value &&
                     !is_expr<V>::value &&
                     (UdtTrait<remove_cv_ref_t<T>>::value ||
                      UdtTrait<remove_cv_ref_t<U>>::value ||
                      UdtTrait<remove_cv_ref_t<V>>::value)>
    struct udt_any_ternary_op_result;

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        typename V,
        template<class> class UdtTrait>
    struct udt_any_ternary_op_result<ExprTemplate, T, U, V, UdtTrait, true>
    {
        using cond_type = operand_type_t<ExprTemplate, T>;
        using then_type = operand_type_t<ExprTemplate, U>;
        using else_type = operand_type_t<ExprTemplate, V>;
        using type = ExprTemplate<
            expr_kind::if_else,
            hana::tuple<cond_type, then_type, else_type>>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        typename T,
        typename U,
        typename V,
        template<class> class UdtTrait>
    using udt_any_ternary_op_result_t =
        typename udt_any_ternary_op_result<ExprTemplate, T, U, V, UdtTrait>::
            type;


    // udt_unary_op_result

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        template<class> class UdtTrait,
        bool Valid = !is_expr<T>::value && UdtTrait<remove_cv_ref_t<T>>::value>
    struct udt_unary_op_result;

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        template<class> class UdtTrait>
    struct udt_unary_op_result<ExprTemplate, OpKind, T, UdtTrait, true>
    {
        using x_type = operand_type_t<ExprTemplate, T>;
        using type = ExprTemplate<OpKind, hana::tuple<x_type>>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        template<class> class UdtTrait>
    using udt_unary_op_result_t =
        typename udt_unary_op_result<ExprTemplate, OpKind, T, UdtTrait>::type;


    // udt_udt_binary_op_result

    template<typename T, template<class> class UdtTrait>
    struct is_udt_arg
    {
        static bool const value =
            !is_expr<T>::value && UdtTrait<remove_cv_ref_t<T>>::value;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U,
        template<class> class TUdtTrait,
        template<class> class UUdtTrait,
        bool Valid =
            is_udt_arg<T, TUdtTrait>::value && is_udt_arg<U, UUdtTrait>::value>
    struct udt_udt_binary_op_result;

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U,
        template<class> class TUdtTrait,
        template<class> class UUdtTrait>
    struct udt_udt_binary_op_result<
        ExprTemplate,
        OpKind,
        T,
        U,
        TUdtTrait,
        UUdtTrait,
        true>
    {
        using lhs_type = operand_type_t<ExprTemplate, T>;
        using rhs_type = operand_type_t<ExprTemplate, U>;
        using type = ExprTemplate<OpKind, hana::tuple<lhs_type, rhs_type>>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U,
        template<class> class TUdtTrait,
        template<class> class UUdtTrait>
    using udt_udt_binary_op_result_t = typename udt_udt_binary_op_result<
        ExprTemplate,
        OpKind,
        T,
        U,
        TUdtTrait,
        UUdtTrait>::type;


    // udt_any_binary_op_result

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U,
        template<class> class UdtTrait,
        bool Valid = !is_expr<T>::value && !is_expr<U>::value &&
                     (UdtTrait<remove_cv_ref_t<T>>::value ||
                      UdtTrait<remove_cv_ref_t<U>>::value)>
    struct udt_any_binary_op_result;

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U,
        template<class> class UdtTrait>
    struct udt_any_binary_op_result<ExprTemplate, OpKind, T, U, UdtTrait, true>
    {
        using lhs_type = operand_type_t<ExprTemplate, T>;
        using rhs_type = operand_type_t<ExprTemplate, U>;
        using type = ExprTemplate<OpKind, hana::tuple<lhs_type, rhs_type>>;
    };

    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind OpKind,
        typename T,
        typename U,
        template<class> class UdtTrait>
    using udt_any_binary_op_result_t = typename udt_any_binary_op_result<
        ExprTemplate,
        OpKind,
        T,
        U,
        UdtTrait>::type;


    // not_copy_or_move

    template<typename LeftT, typename RightT>
    struct copy_or_move : std::false_type
    {
    };

    template<typename T>
    struct copy_or_move<T, T const &> : std::true_type
    {
    };

    template<typename T>
    struct copy_or_move<T, T &> : std::true_type
    {
    };

    template<typename T>
    struct copy_or_move<T, T &&> : std::true_type
    {
    };


    // expr_arity

    enum class expr_arity { invalid, one, two, three, n };

    template<expr_kind Kind>
    constexpr expr_arity arity_of()
    {
        switch (Kind) {
        case expr_kind::expr_ref:

        case expr_kind::terminal:

        // unary
        case expr_kind::unary_plus:  // +
        case expr_kind::negate:      // -
        case expr_kind::dereference: // *
        case expr_kind::complement:  // ~
        case expr_kind::address_of:  // &
        case expr_kind::logical_not: // !
        case expr_kind::pre_inc:     // ++
        case expr_kind::pre_dec:     // --
        case expr_kind::post_inc:    // ++(int)
        case expr_kind::post_dec:    // --(int)
            return expr_arity::one;

        // binary
        case expr_kind::shift_left:         // <<
        case expr_kind::shift_right:        // >>
        case expr_kind::multiplies:         // *
        case expr_kind::divides:            // /
        case expr_kind::modulus:            // %
        case expr_kind::plus:               // +
        case expr_kind::minus:              // -
        case expr_kind::less:               // <
        case expr_kind::greater:            // >
        case expr_kind::less_equal:         // <=
        case expr_kind::greater_equal:      // >=
        case expr_kind::equal_to:           // ==
        case expr_kind::not_equal_to:       // !=
        case expr_kind::logical_or:         // ||
        case expr_kind::logical_and:        // &&
        case expr_kind::bitwise_and:        // &
        case expr_kind::bitwise_or:         // |
        case expr_kind::bitwise_xor:        // ^
        case expr_kind::comma:              // :
        case expr_kind::mem_ptr:            // ->*
        case expr_kind::assign:             // =
        case expr_kind::shift_left_assign:  // <<=
        case expr_kind::shift_right_assign: // >>=
        case expr_kind::multiplies_assign:  // *=
        case expr_kind::divides_assign:     // /=
        case expr_kind::modulus_assign:     // %=
        case expr_kind::plus_assign:        // +=
        case expr_kind::minus_assign:       // -=
        case expr_kind::bitwise_and_assign: // &=
        case expr_kind::bitwise_or_assign:  // |=
        case expr_kind::bitwise_xor_assign: // ^=
        case expr_kind::subscript:          // []
            return expr_arity::two;

        // ternary
        case expr_kind::if_else: // (analogous to) ?:
            return expr_arity::three;

        // n-ary
        case expr_kind::call: // ()
            return expr_arity::n;

        default: return expr_arity::invalid;
        }
    }

}}}

#endif
