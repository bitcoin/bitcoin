// Copyright (C) 2016-2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_YAP_ALGORITHM_HPP_INCLUDED
#define BOOST_YAP_ALGORITHM_HPP_INCLUDED

#include <boost/yap/algorithm_fwd.hpp>
#include <boost/yap/user_macros.hpp>
#include <boost/yap/detail/algorithm.hpp>

#include <boost/hana/size.hpp>
#include <boost/hana/comparing.hpp>


namespace boost { namespace yap {

#ifdef BOOST_NO_CONSTEXPR_IF

    namespace detail {

        template<typename Expr, bool MutableRvalueRef>
        struct deref_impl
        {
            constexpr decltype(auto) operator()(Expr && expr)
            {
                return std::move(*expr.elements[hana::llong_c<0>]);
            }
        };

        template<typename Expr>
        struct deref_impl<Expr, false>
        {
            constexpr decltype(auto) operator()(Expr && expr)
            {
                return *expr.elements[hana::llong_c<0>];
            }
        };
    }

#endif

    /** "Dereferences" a reference-expression, forwarding its referent to
       the caller. */
    template<typename Expr>
    constexpr decltype(auto) deref(Expr && expr)
    {
        static_assert(
            is_expr<Expr>::value, "deref() is only defined for expressions.");

        static_assert(
            detail::remove_cv_ref_t<Expr>::kind == expr_kind::expr_ref,
            "deref() is only defined for expr_ref-kind expressions.");

#ifdef BOOST_NO_CONSTEXPR_IF
        return detail::deref_impl < Expr,
               std::is_rvalue_reference<Expr>::value &&
                   !std::is_const<std::remove_reference_t<Expr>>::value >
                       {}(static_cast<Expr &&>(expr));
#else
        using namespace hana::literals;
        if constexpr (
            std::is_rvalue_reference<Expr>::value &&
            !std::is_const<std::remove_reference_t<Expr>>::value) {
            return std::move(*expr.elements[0_c]);
        } else {
            return *expr.elements[0_c];
        }
#endif
    }

    namespace detail {

        template<typename Tuple, long long I>
        struct lvalue_ref_ith_element
            : std::is_lvalue_reference<decltype(
                  std::declval<Tuple>()[hana::llong<I>{}])>
        {
        };

#ifdef BOOST_NO_CONSTEXPR_IF

        template<bool ValueOfTerminalsOnly, typename T>
        constexpr decltype(auto) value_impl(T && x);

        template<
            typename T,
            bool IsExprRef,
            bool ValueOfTerminalsOnly,
            bool TakeValue,
            bool IsLvalueRef>
        struct value_expr_impl;

        template<
            typename T,
            bool ValueOfTerminalsOnly,
            bool TakeValue,
            bool IsLvalueRef>
        struct value_expr_impl<
            T,
            true,
            ValueOfTerminalsOnly,
            TakeValue,
            IsLvalueRef>
        {
            constexpr decltype(auto) operator()(T && x)
            {
                return ::boost::yap::detail::value_impl<ValueOfTerminalsOnly>(
                    ::boost::yap::deref(static_cast<T &&>(x)));
            }
        };

        template<typename T, bool ValueOfTerminalsOnly>
        struct value_expr_impl<T, false, ValueOfTerminalsOnly, true, true>
        {
            constexpr decltype(auto) operator()(T && x)
            {
                return x.elements[hana::llong_c<0>];
            }
        };

        template<typename T, bool ValueOfTerminalsOnly>
        struct value_expr_impl<T, false, ValueOfTerminalsOnly, true, false>
        {
            constexpr decltype(auto) operator()(T && x)
            {
                return std::move(x.elements[hana::llong_c<0>]);
            }
        };

        template<typename T, bool ValueOfTerminalsOnly, bool IsLvalueRef>
        struct value_expr_impl<
            T,
            false,
            ValueOfTerminalsOnly,
            false,
            IsLvalueRef>
        {
            constexpr decltype(auto) operator()(T && x)
            {
                return static_cast<T &&>(x);
            }
        };

        template<typename T, bool IsExpr, bool ValueOfTerminalsOnly>
        struct value_impl_t
        {
            constexpr decltype(auto) operator()(T && x)
            {
                constexpr expr_kind kind = detail::remove_cv_ref_t<T>::kind;
                constexpr detail::expr_arity arity = detail::arity_of<kind>();
                return value_expr_impl < T, kind == expr_kind::expr_ref,
                       ValueOfTerminalsOnly,
                       (ValueOfTerminalsOnly && kind == expr_kind::terminal) ||
                           (!ValueOfTerminalsOnly &&
                            arity == detail::expr_arity::one),
                       std::is_lvalue_reference<T>::value ||
                           detail::lvalue_ref_ith_element<
                               decltype(x.elements),
                               0>::value > {}(static_cast<T &&>(x));
            }
        };

        template<typename T, bool ValueOfTerminalsOnly>
        struct value_impl_t<T, false, ValueOfTerminalsOnly>
        {
            constexpr decltype(auto) operator()(T && x)
            {
                return static_cast<T &&>(x);
            }
        };

        template<bool ValueOfTerminalsOnly, typename T>
        constexpr decltype(auto) value_impl(T && x)
        {
            return detail::
                value_impl_t<T, is_expr<T>::value, ValueOfTerminalsOnly>{}(
                    static_cast<T &&>(x));
        }

#else

        template<bool ValueOfTerminalsOnly, typename T>
        constexpr decltype(auto) value_impl(T && x)
        {
            if constexpr (is_expr<T>::value) {
                using namespace hana::literals;
                constexpr expr_kind kind = remove_cv_ref_t<T>::kind;
                constexpr expr_arity arity = arity_of<kind>();
                if constexpr (kind == expr_kind::expr_ref) {
                    return value_impl<ValueOfTerminalsOnly>(
                        ::boost::yap::deref(static_cast<T &&>(x)));
                } else if constexpr (
                    kind == expr_kind::terminal ||
                    (!ValueOfTerminalsOnly && arity == expr_arity::one)) {
                    if constexpr (
                        std::is_lvalue_reference<T>::value ||
                        detail::
                            lvalue_ref_ith_element<decltype(x.elements), 0>{}) {
                        return x.elements[0_c];
                    } else {
                        return std::move(x.elements[0_c]);
                    }
                } else {
                    return static_cast<T &&>(x);
                }
            } else {
                return static_cast<T &&>(x);
            }
        }

#endif
    }

    /** Forwards the sole element of \a x to the caller, possibly calling
        <code>deref()</code> first if \a x is a reference expression, or
        forwards \a x to the caller unchanged.

        More formally:

        - If \a x is not an expression, \a x is forwarded to the caller.

        - Otherwise, if \a x is a reference expression, the result is
        <code>value(deref(x))</code>.

        - Otherwise, if \a x is an expression with only one value (a unary
        expression or a terminal expression), the result is the forwarded
        first element of \a x.

        - Otherwise, \a x is forwarded to the caller. */
    template<typename T>
    constexpr decltype(auto) value(T && x)
    {
        return detail::value_impl<false>(static_cast<T &&>(x));
    }

#ifdef BOOST_NO_CONSTEXPR_IF

    template<typename Expr, typename I>
    constexpr decltype(auto) get(Expr && expr, I const & i);

    namespace detail {

        template<long long I, typename Expr, bool IsExpr, bool IsLvalueRef>
        struct get_impl;

        template<long long I, typename Expr, bool IsLvalueRef>
        struct get_impl<I, Expr, true, IsLvalueRef>
        {
            constexpr decltype(auto) operator()(Expr && expr, hana::llong<I> i)
            {
                return ::boost::yap::get(
                    ::boost::yap::deref(static_cast<Expr &&>(expr)), i);
            }
        };

        template<long long I, typename Expr>
        struct get_impl<I, Expr, false, true>
        {
            constexpr decltype(auto) operator()(Expr && expr, hana::llong<I> i)
            {
                return expr.elements[i];
            }
        };

        template<long long I, typename Expr>
        struct get_impl<I, Expr, false, false>
        {
            constexpr decltype(auto) operator()(Expr && expr, hana::llong<I> i)
            {
                return std::move(expr.elements[i]);
            }
        };
    }

#endif

    /** Forwards the <i>i</i>-th element of \a expr to the caller.  If \a
       expr is a reference expression, the result is <code>get(deref(expr),
        i)</code>.

        \note <code>get()</code> is only valid if \a Expr is an expression.
    */
    template<typename Expr, typename I>
    constexpr decltype(auto) get(Expr && expr, I const & i)
    {
        static_assert(
            is_expr<Expr>::value, "get() is only defined for expressions.");
        static_assert(
            hana::IntegralConstant<I>::value,
            "'i' must be an IntegralConstant");

        constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;

        static_assert(
            kind == expr_kind::expr_ref ||
                (0 <= I::value &&
                 I::value < decltype(hana::size(expr.elements))::value),
            "In get(expr, I), I must be a valid index into expr's tuple "
            "elements.");

#ifdef BOOST_NO_CONSTEXPR_IF
        return detail::get_impl<
            I::value,
            Expr,
            kind == expr_kind::expr_ref,
            std::is_lvalue_reference<Expr>::value>{}(static_cast<Expr &&>(expr), i);
#else
        using namespace hana::literals;
        if constexpr (kind == expr_kind::expr_ref) {
            return ::boost::yap::get(
                ::boost::yap::deref(static_cast<Expr &&>(expr)), i);
        } else {
            if constexpr (std::is_lvalue_reference<Expr>::value) {
                return expr.elements[i];
            } else {
                return std::move(expr.elements[i]);
            }
        }
#endif
    }

    /** Returns <code>get(expr, boost::hana::llong_c<I>)</code>. */
    template<long long I, typename Expr>
    constexpr decltype(auto) get_c(Expr && expr)
    {
        return ::boost::yap::get(static_cast<Expr &&>(expr), hana::llong_c<I>);
    }

    /** Returns the left operand in a binary operator expression.

        Equivalent to <code>get(expr, 0_c)</code>.

        \note <code>left()</code> is only valid if \a Expr is a binary
        operator expression.
    */
    template<typename Expr>
    constexpr decltype(auto) left(Expr && expr)
    {
        using namespace hana::literals;
        return ::boost::yap::get(static_cast<Expr &&>(expr), 0_c);
        constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
        static_assert(
            kind == expr_kind::expr_ref ||
                detail::arity_of<kind>() == detail::expr_arity::two,
            "left() is only defined for binary expressions.");
    }

    /** Returns the right operand in a binary operator expression.

        Equivalent to <code>get(expr, 1_c)</code>.

        \note <code>right()</code> is only valid if \a Expr is a binary
        operator expression.
    */
    template<typename Expr>
    constexpr decltype(auto) right(Expr && expr)
    {
        using namespace hana::literals;
        return ::boost::yap::get(static_cast<Expr &&>(expr), 1_c);
        constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
        static_assert(
            kind == expr_kind::expr_ref ||
                detail::arity_of<kind>() == detail::expr_arity::two,
            "right() is only defined for binary expressions.");
    }

    /** Returns the condition expression in an if_else expression.

        Equivalent to <code>get(expr, 0_c)</code>.

        \note <code>cond()</code> is only valid if \a Expr is an
        <code>expr_kind::if_else</code> expression.
    */
    template<typename Expr>
    constexpr decltype(auto) cond(Expr && expr)
    {
        using namespace hana::literals;
        return ::boost::yap::get(static_cast<Expr &&>(expr), 0_c);
        constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
        static_assert(
            kind == expr_kind::expr_ref || kind == expr_kind::if_else,
            "cond() is only defined for if_else expressions.");
    }

    /** Returns the then-expression in an if_else expression.

        Equivalent to <code>get(expr, 1_c)</code>.

        \note <code>then()</code> is only valid if \a Expr is an
        <code>expr_kind::if_else</code> expression.
    */
    template<typename Expr>
    constexpr decltype(auto) then(Expr && expr)
    {
        using namespace hana::literals;
        return ::boost::yap::get(static_cast<Expr &&>(expr), 1_c);
        constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
        static_assert(
            kind == expr_kind::expr_ref || kind == expr_kind::if_else,
            "then() is only defined for if_else expressions.");
    }

    /** Returns the else-expression in an if_else expression.

        Equivalent to <code>get(expr, 2_c)</code>.

        \note <code>else_()</code> is only valid if \a Expr is an
        <code>expr_kind::if_else</code> expression.
    */
    template<typename Expr>
    constexpr decltype(auto) else_(Expr && expr)
    {
        using namespace hana::literals;
        return ::boost::yap::get(static_cast<Expr &&>(expr), 2_c);
        constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
        static_assert(
            kind == expr_kind::expr_ref || kind == expr_kind::if_else,
            "else_() is only defined for if_else expressions.");
    }

    /** Returns the callable in a call expression.

        Equivalent to <code>get(expr, 0)</code>.

        \note <code>callable()</code> is only valid if \a Expr is an
        <code>expr_kind::call</code> expression.
    */
    template<typename Expr>
    constexpr decltype(auto) callable(Expr && expr)
    {
        return ::boost::yap::get(static_cast<Expr &&>(expr), hana::llong_c<0>);
        constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
        static_assert(
            kind == expr_kind::expr_ref ||
                detail::arity_of<kind>() == detail::expr_arity::n,
            "callable() is only defined for call expressions.");
    }

    /** Returns the <i>i-th</i> argument expression in a call expression.

        Equivalent to <code>get(expr, i + 1)</code>.

        \note <code>argument()</code> is only valid if \a Expr is an
        <code>expr_kind::call</code> expression.
    */
    template<long long I, typename Expr>
    constexpr decltype(auto) argument(Expr && expr, hana::llong<I> i)
    {
        return ::boost::yap::get(
            static_cast<Expr &&>(expr), hana::llong_c<I + 1>);
        constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
        static_assert(
            kind == expr_kind::expr_ref ||
                detail::arity_of<kind>() == detail::expr_arity::n,
            "argument() is only defined for call expressions.");
        static_assert(
            kind == expr_kind::expr_ref ||
                (0 <= I && I < decltype(hana::size(expr.elements))::value - 1),
            "I must be a valid call-expression argument index.");
    }

    /** Makes a new expression instantiated from the expression template \a
        ExprTemplate, of kind \a Kind, with the given values as its
       elements.

        For each parameter P:

        - If P is an expression, P is moved into the result if P is an
       rvalue and captured by reference into the result otherwise.

        - Otherwise, P is wrapped in a terminal expression.

        \note <code>make_expression()</code> is only valid if the number of
        parameters passed is appropriate for \a Kind.
    */
    template<
        template<expr_kind, class> class ExprTemplate,
        expr_kind Kind,
        typename... T>
    constexpr auto make_expression(T &&... t)
    {
        constexpr detail::expr_arity arity = detail::arity_of<Kind>();
        static_assert(
            (arity == detail::expr_arity::one && sizeof...(T) == 1) ||
                (arity == detail::expr_arity::two && sizeof...(T) == 2) ||
                (arity == detail::expr_arity::three && sizeof...(T) == 3) ||
                arity == detail::expr_arity::n,
            "The number of parameters passed to make_expression() must "
            "match the arity "
            "implied by the expr_kind template parameter.");
        using tuple_type =
            hana::tuple<detail::operand_type_t<ExprTemplate, T>...>;
        return ExprTemplate<Kind, tuple_type>{tuple_type{
            detail::make_operand<detail::operand_type_t<ExprTemplate, T>>{}(
                static_cast<T &&>(t))...}};
    }

    /** Makes a new terminal expression instantiated from the expression
        template \a ExprTemplate, with the given value as its sole element.

        \note <code>make_terminal()</code> is only valid if \a T is \b not
       an expression.
    */
    template<template<expr_kind, class> class ExprTemplate, typename T>
    constexpr auto make_terminal(T && t)
    {
        static_assert(
            !is_expr<T>::value,
            "make_terminal() is only defined for non expressions.");
        using result_type = detail::operand_type_t<ExprTemplate, T>;
        using tuple_type = decltype(std::declval<result_type>().elements);
        return result_type{tuple_type{static_cast<T &&>(t)}};
    }

#ifdef BOOST_NO_CONSTEXPR_IF

    namespace detail {

        template<
            template<expr_kind, class> class ExprTemplate,
            typename T,
            bool IsExpr>
        struct as_expr_impl
        {
            constexpr decltype(auto) operator()(T && t)
            {
                return static_cast<T &&>(t);
            }
        };

        template<template<expr_kind, class> class ExprTemplate, typename T>
        struct as_expr_impl<ExprTemplate, T, false>
        {
            constexpr decltype(auto) operator()(T && t)
            {
                return make_terminal<ExprTemplate>(static_cast<T &&>(t));
            }
        };
    }

#endif

    /** Returns an expression formed from \a t as follows:

        - If \a t is an expression, \a t is forwarded to the caller.

        - Otherwise, \a t is wrapped in a terminal expression.
    */
    template<template<expr_kind, class> class ExprTemplate, typename T>
    constexpr decltype(auto) as_expr(T && t)
    {
#ifdef BOOST_NO_CONSTEXPR_IF
        return detail::as_expr_impl<ExprTemplate, T, is_expr<T>::value>{}(
            static_cast<T &&>(t));
#else
        if constexpr (is_expr<T>::value) {
            return static_cast<T &&>(t);
        } else {
            return make_terminal<ExprTemplate>(static_cast<T &&>(t));
        }
#endif
    }

    /** A callable type that evaluates its contained expression when called.

        \see <code>make_expression_function()</code>
    */
    template<typename Expr>
    struct expression_function
    {
        template<typename... U>
        constexpr decltype(auto) operator()(U &&... u)
        {
            return ::boost::yap::evaluate(expr, static_cast<U &&>(u)...);
        }

        Expr expr;
    };

    namespace detail {

        template<expr_kind Kind, typename Tuple>
        struct expression_function_expr
        {
            static const expr_kind kind = Kind;
            Tuple elements;
        };
    }

    /** Returns a callable object that \a expr has been forwarded into. This
        is useful for using expressions as function objects.

        Lvalue expressions are stored in the result by reference; rvalue
        expressions are moved into the result.

        \note <code>make_expression_function()</code> is only valid if \a
        Expr is an expression.
    */
    template<typename Expr>
    constexpr auto make_expression_function(Expr && expr)
    {
        static_assert(
            is_expr<Expr>::value,
            "make_expression_function() is only defined for expressions.");
        using stored_type =
            detail::operand_type_t<detail::expression_function_expr, Expr &&>;
        return expression_function<stored_type>{
            detail::make_operand<stored_type>{}(static_cast<Expr &&>(expr))};
    }
}}

#include <boost/yap/detail/transform.hpp>

namespace boost { namespace yap {

    /** Returns a transform object that replaces placeholders within an
        expression with the given values.
    */
    template<typename... T>
    constexpr auto replacements(T &&... t)
    {
        return detail::placeholder_transform_t<T...>(static_cast<T &&>(t)...);
    }

    /** Returns \a expr with the placeholders replaced by YAP terminals
        containing the given values.

        \note <code>replace_placeholders(expr, t...)</code> is only valid if
        \a expr is an expression, and <code>max_p <= sizeof...(t)</code>,
        where <code>max_p</code> is the maximum placeholder index in \a expr.
    */
    template<typename Expr, typename... T>
    constexpr decltype(auto) replace_placeholders(Expr && expr, T &&... t)
    {
        static_assert(
            is_expr<Expr>::value,
            "evaluate() is only defined for expressions.");
        return transform(
            static_cast<Expr &&>(expr), replacements(static_cast<T &&>(t)...));
    }

    /** Returns a transform object that evaluates an expression using the
        built-in semantics.  The transform replaces any placeholders with the
        given values.
    */
    template<typename... T>
    constexpr auto evaluation(T &&... t)
    {
        return detail::evaluation_transform_t<T...>(static_cast<T &&>(t)...);
    }

    /** Evaluates \a expr using the built-in semantics, replacing any
        placeholders with the given values.

        \note <code>evaluate(expr)</code> is only valid if \a expr is an
        expression.
    */
    template<typename Expr, typename... T>
    constexpr decltype(auto) evaluate(Expr && expr, T &&... t)
    {
        static_assert(
            is_expr<Expr>::value,
            "evaluate() is only defined for expressions.");
        return transform(
            static_cast<Expr &&>(expr), evaluation(static_cast<T &&>(t)...));
    }

    namespace detail {

        template<typename... Transforms>
        constexpr auto make_transform_tuple(Transforms &... transforms)
        {
            return hana::tuple<Transforms *...>{&transforms...};
        }

        template<bool Strict>
        struct transform_
        {
            template<typename Expr, typename Transform, typename... Transforms>
            constexpr decltype(auto) operator()(
                Expr && expr, Transform & transform, Transforms &... transforms) const
            {
                auto transform_tuple =
                    detail::make_transform_tuple(transform, transforms...);
                constexpr expr_kind kind = detail::remove_cv_ref_t<Expr>::kind;
                return detail::
                    transform_impl<Strict, 0, kind == expr_kind::expr_ref>{}(
                        static_cast<Expr &&>(expr), transform_tuple);
            }
        };
    }

    /** Returns the result of transforming (all or part of) \a expr using
        whatever overloads of <code>Transform::operator()</code> match \a
        expr.

        \note Transformations can do anything: they may have side effects;
        they may mutate values; they may mutate types; and they may do any
        combination of these.
    */
    template<typename Expr, typename Transform, typename... Transforms>
    constexpr decltype(auto)
    transform(Expr && expr, Transform && transform, Transforms &&... transforms)
    {
        static_assert(
            is_expr<Expr>::value,
            "transform() is only defined for expressions.");
        return detail::transform_<false>{}(
            static_cast<Expr &&>(expr), transform, transforms...);
    }

    /** Returns the result of transforming \a expr using whichever overload of
        <code>Transform::operator()</code> best matches \a expr.  If no
        overload of <code>Transform::operator()</code> matches, a compile-time
        error results.

        \note Transformations can do anything: they may have side effects;
        they may mutate values; they may mutate types; and they may do any
        combination of these.
    */
    template<typename Expr, typename Transform, typename... Transforms>
    constexpr decltype(auto) transform_strict(
        Expr && expr, Transform && transform, Transforms &&... transforms)
    {
        static_assert(
            is_expr<Expr>::value,
            "transform() is only defined for expressions.");
        return detail::transform_<true>{}(
            static_cast<Expr &&>(expr), transform, transforms...);
    }

}}

#endif
