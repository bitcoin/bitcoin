// Copyright (C) 2016-2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_YAP_USER_MACROS_HPP_INCLUDED
#define BOOST_YAP_USER_MACROS_HPP_INCLUDED

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum.hpp>


#ifndef BOOST_YAP_DOXYGEN

// unary
#define BOOST_YAP_OPERATOR_unary_plus(...) +(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_negate(...) -(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_dereference(...) *(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_complement(...) ~(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_address_of(...) &(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_logical_not(...) !(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_pre_inc(...) ++(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_pre_dec(...) --(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_post_inc(...) ++(__VA_ARGS__, int)
#define BOOST_YAP_OPERATOR_post_dec(...) --(__VA_ARGS__, int)

// binary
#define BOOST_YAP_OPERATOR_shift_left(...) <<(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_shift_right(...) >>(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_multiplies(...) *(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_divides(...) /(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_modulus(...) %(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_plus(...) +(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_minus(...) -(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_less(...) <(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_greater(...) >(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_less_equal(...) <=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_greater_equal(...) >=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_equal_to(...) ==(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_not_equal_to(...) !=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_logical_or(...) ||(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_logical_and(...) &&(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_bitwise_and(...) &(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_bitwise_or(...) |(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_bitwise_xor(...) ^(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_comma(...) ,(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_mem_ptr(...) ->*(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_assign(...) =(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_shift_left_assign(...) <<=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_shift_right_assign(...) >>=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_multiplies_assign(...) *=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_divides_assign(...) /=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_modulus_assign(...) %=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_plus_assign(...) +=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_minus_assign(...) -=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_bitwise_and_assign(...) &=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_bitwise_or_assign(...) |=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_bitwise_xor_assign(...) ^=(__VA_ARGS__)
#define BOOST_YAP_OPERATOR_subscript(...) [](__VA_ARGS__)

#define BOOST_YAP_INDIRECT_CALL(macro) BOOST_PP_CAT(BOOST_YAP_OPERATOR_, macro)

#endif // BOOST_YAP_DOXYGEN


/** Defines operator overloads for unary operator \a op_name that each take an
    expression instantiated from \a expr_template and return an expression
    instantiated from the \a result_expr_template expression template.  One
    overload is defined for each of the qualifiers <code>const &</code>,
    <code>&</code>, and <code>&&</code>.  For the lvalue reference overloads,
    the argument is captured by reference into the resulting expression.  For
    the rvalue reference overload, the argument is moved into the resulting
    expression.

    Example:
    \snippet user_macros_snippets.cpp USER_UNARY_OPERATOR

    \param op_name The operator to be overloaded; this must be one of the \b
    unary enumerators in <code>expr_kind</code>, without the
    <code>expr_kind::</code> qualification.

    \param expr_template The expression template to which the overloads apply.
    \a expr_template must be an \ref ExpressionTemplate.

    \param result_expr_template The expression template to use to instantiate
    the result expression.  \a result_expr_template must be an \ref
    ExpressionTemplate.
*/
#define BOOST_YAP_USER_UNARY_OPERATOR(                                         \
    op_name, expr_template, result_expr_template)                              \
    template<::boost::yap::expr_kind Kind, typename Tuple>                     \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        expr_template<Kind, Tuple> const & x)                                  \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::operand_type_t<                 \
            result_expr_template,                                              \
            expr_template<Kind, Tuple> const &>;                               \
        using tuple_type = ::boost::hana::tuple<lhs_type>;                     \
        return result_expr_template<                                           \
            ::boost::yap::expr_kind::op_name,                                  \
            tuple_type>{                                                       \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(x)}};    \
    }                                                                          \
    template<::boost::yap::expr_kind Kind, typename Tuple>                     \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        expr_template<Kind, Tuple> & x)                                        \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::operand_type_t<                 \
            result_expr_template,                                              \
            expr_template<Kind, Tuple> &>;                                     \
        using tuple_type = ::boost::hana::tuple<lhs_type>;                     \
        return result_expr_template<                                           \
            ::boost::yap::expr_kind::op_name,                                  \
            tuple_type>{                                                       \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(x)}};    \
    }                                                                          \
    template<::boost::yap::expr_kind Kind, typename Tuple>                     \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        expr_template<Kind, Tuple> && x)                                       \
    {                                                                          \
        using tuple_type = ::boost::hana::tuple<expr_template<Kind, Tuple>>;   \
        return result_expr_template<                                           \
            ::boost::yap::expr_kind::op_name,                                  \
            tuple_type>{tuple_type{std::move(x)}};                             \
    }


/** Defines operator overloads for binary operator \a op_name that each
    produce an expression instantiated from the \a expr_template expression
    template.  One overload is defined for each of the qualifiers <code>const
    &</code>, <code>&</code>, and <code>&&</code>.  For the lvalue reference
    overloads, <code>*this</code> is captured by reference into the resulting
    expression.  For the rvalue reference overload, <code>*this</code> is
    moved into the resulting expression.

    Note that this does not work for yap::expr_kinds assign, subscript, or
    call.  Use BOOST_YAP_USER_ASSIGN_OPERATOR,
    BOOST_YAP_USER_SUBSCRIPT_OPERATOR, or BOOST_YAP_USER_CALL_OPERATOR for
    those, respectively.

    Example:
    \snippet user_macros_snippets.cpp USER_BINARY_OPERATOR

    \param op_name The operator to be overloaded; this must be one of the \b
    binary enumerators in <code>expr_kind</code>, except assign, subscript, or
    call, without the <code>expr_kind::</code> qualification.

    \param expr_template The expression template to which the overloads apply.
    \a expr_template must be an \ref ExpressionTemplate.

    \param result_expr_template The expression template to use to instantiate
    the result expression.  \a result_expr_template must be an \ref
    ExpressionTemplate.
*/
#define BOOST_YAP_USER_BINARY_OPERATOR(                                        \
    op_name, expr_template, result_expr_template)                              \
    template<::boost::yap::expr_kind Kind, typename Tuple, typename Expr>      \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        expr_template<Kind, Tuple> const & lhs, Expr && rhs)                   \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::operand_type_t<                 \
            result_expr_template,                                              \
            expr_template<Kind, Tuple> const &>;                               \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<result_expr_template, Expr>;  \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return result_expr_template<                                           \
            ::boost::yap::expr_kind::op_name,                                  \
            tuple_type>{                                                       \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(lhs),    \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }                                                                          \
    template<::boost::yap::expr_kind Kind, typename Tuple, typename Expr>      \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        expr_template<Kind, Tuple> & lhs, Expr && rhs)                         \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::operand_type_t<                 \
            result_expr_template,                                              \
            expr_template<Kind, Tuple> &>;                                     \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<result_expr_template, Expr>;  \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return result_expr_template<                                           \
            ::boost::yap::expr_kind::op_name,                                  \
            tuple_type>{                                                       \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(lhs),    \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }                                                                          \
    template<::boost::yap::expr_kind Kind, typename Tuple, typename Expr>      \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        expr_template<Kind, Tuple> && lhs, Expr && rhs)                        \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::remove_cv_ref_t<                \
            expr_template<Kind, Tuple> &&>;                                    \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<result_expr_template, Expr>;  \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return result_expr_template<                                           \
            ::boost::yap::expr_kind::op_name,                                  \
            tuple_type>{                                                       \
            tuple_type{std::move(lhs),                                         \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }                                                                          \
    template<typename T, ::boost::yap::expr_kind Kind, typename Tuple>         \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        T && lhs, expr_template<Kind, Tuple> && rhs)                           \
        ->::boost::yap::detail::free_binary_op_result_t<                       \
            result_expr_template,                                              \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            expr_template<Kind, Tuple> &&>                                     \
    {                                                                          \
        using result_types = ::boost::yap::detail::free_binary_op_result<      \
            result_expr_template,                                              \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            expr_template<Kind, Tuple> &&>;                                    \
        using lhs_type = typename result_types::lhs_type;                      \
        using rhs_type = typename result_types::rhs_type;                      \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return {tuple_type{lhs_type{static_cast<T &&>(lhs)}, std::move(rhs)}}; \
    }                                                                          \
    template<typename T, ::boost::yap::expr_kind Kind, typename Tuple>         \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        T && lhs, expr_template<Kind, Tuple> const & rhs)                      \
        ->::boost::yap::detail::free_binary_op_result_t<                       \
            result_expr_template,                                              \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            expr_template<Kind, Tuple> const &>                                \
    {                                                                          \
        using result_types = ::boost::yap::detail::free_binary_op_result<      \
            result_expr_template,                                              \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            expr_template<Kind, Tuple> const &>;                               \
        using lhs_type = typename result_types::lhs_type;                      \
        using rhs_type = typename result_types::rhs_type;                      \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        using rhs_tuple_type = typename result_types::rhs_tuple_type;          \
        return {tuple_type{lhs_type{static_cast<T &&>(lhs)},                   \
                           rhs_type{rhs_tuple_type{std::addressof(rhs)}}}};    \
    }                                                                          \
    template<typename T, ::boost::yap::expr_kind Kind, typename Tuple>         \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(                  \
        T && lhs, expr_template<Kind, Tuple> & rhs)                            \
        ->::boost::yap::detail::free_binary_op_result_t<                       \
            result_expr_template,                                              \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            expr_template<Kind, Tuple> &>                                      \
    {                                                                          \
        using result_types = ::boost::yap::detail::free_binary_op_result<      \
            result_expr_template,                                              \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            expr_template<Kind, Tuple> &>;                                     \
        using lhs_type = typename result_types::lhs_type;                      \
        using rhs_type = typename result_types::rhs_type;                      \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        using rhs_tuple_type = typename result_types::rhs_tuple_type;          \
        return {tuple_type{lhs_type{static_cast<T &&>(lhs)},                   \
                           rhs_type{rhs_tuple_type{std::addressof(rhs)}}}};    \
    }


/** Defines operator overloads for \a operator=() that each produce an
    expression instantiated from the \a expr_template expression template.
    One overload is defined for each of the qualifiers <code>const &</code>,
    <code>&</code>, and <code>&&</code>.  For the lvalue reference overloads,
    <code>*this</code> is captured by reference into the resulting expression.
    For the rvalue reference overload, <code>*this</code> is moved into the
    resulting expression.

    The \a rhs parameter to each of the defined overloads may be any type,
    including an expression, except that the overloads are constrained by
    std::enable_if<> not to conflict with the assignment and move assignement
    operators.  If \a rhs is a non-expression, it is wrapped in a terminal
    expression.

    Example:
    \snippet user_macros_snippets.cpp USER_ASSIGN_OPERATOR

    \param this_type The type of the class the operator is a member of; this
    is required to avoid clashing with the assignment and move assignement
    operators.

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.
*/
#define BOOST_YAP_USER_ASSIGN_OPERATOR(this_type, expr_template)               \
    template<                                                                  \
        typename Expr,                                                         \
        typename = std::enable_if_t<                                           \
            !::boost::yap::detail::copy_or_move<this_type, Expr &&>::value>>   \
    constexpr auto operator=(Expr && rhs) const &                              \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::                                \
            operand_type_t<expr_template, this_type const &>;                  \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<expr_template, Expr>;         \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return expr_template<::boost::yap::expr_kind::assign, tuple_type>{     \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(*this),  \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }                                                                          \
    template<                                                                  \
        typename Expr,                                                         \
        typename = std::enable_if_t<                                           \
            !::boost::yap::detail::copy_or_move<this_type, Expr &&>::value>>   \
    constexpr auto operator=(Expr && rhs) &                                    \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::                                \
            operand_type_t<expr_template, decltype(*this)>;                    \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<expr_template, Expr>;         \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return expr_template<::boost::yap::expr_kind::assign, tuple_type>{     \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(*this),  \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }                                                                          \
    template<                                                                  \
        typename Expr,                                                         \
        typename = std::enable_if_t<                                           \
            !::boost::yap::detail::copy_or_move<this_type, Expr &&>::value>>   \
    constexpr auto operator=(Expr && rhs) &&                                   \
    {                                                                          \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<expr_template, Expr>;         \
        using tuple_type = ::boost::hana::tuple<this_type, rhs_type>;          \
        return expr_template<::boost::yap::expr_kind::assign, tuple_type>{     \
            tuple_type{std::move(*this),                                       \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }


/** Defines operator overloads for \a operator[]() that each produce an
    expression instantiated from the \a expr_template expression template.
    One overload is defined for each of the qualifiers <code>const &</code>,
    <code>&</code>, and <code>&&</code>.  For the lvalue reference overloads,
    <code>*this</code> is captured by reference into the resulting expression.
    For the rvalue reference overload, <code>*this</code> is moved into the
    resulting expression.

    The \a rhs parameter to each of the defined overloads may be any type,
    including an expression, except that the overloads are constrained by
    std::enable_if<> not to conflict with the assignment and move assignement
    operators.  If \a rhs is a non-expression, it is wrapped in a terminal
    expression.

    Example:
    \snippet user_macros_snippets.cpp USER_SUBSCRIPT_OPERATOR

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.
*/
#define BOOST_YAP_USER_SUBSCRIPT_OPERATOR(expr_template)                       \
    template<typename Expr>                                                    \
    constexpr auto operator[](Expr && rhs) const &                             \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::                                \
            operand_type_t<expr_template, decltype(*this)>;                    \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<expr_template, Expr>;         \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return expr_template<::boost::yap::expr_kind::subscript, tuple_type>{  \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(*this),  \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }                                                                          \
    template<typename Expr>                                                    \
    constexpr auto operator[](Expr && rhs) &                                   \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::                                \
            operand_type_t<expr_template, decltype(*this)>;                    \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<expr_template, Expr>;         \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return expr_template<::boost::yap::expr_kind::subscript, tuple_type>{  \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(*this),  \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }                                                                          \
    template<typename Expr>                                                    \
    constexpr auto operator[](Expr && rhs) &&                                  \
    {                                                                          \
        using lhs_type =                                                       \
            ::boost::yap::detail::remove_cv_ref_t<decltype(*this)>;            \
        using rhs_type =                                                       \
            ::boost::yap::detail::operand_type_t<expr_template, Expr>;         \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return expr_template<::boost::yap::expr_kind::subscript, tuple_type>{  \
            tuple_type{std::move(*this),                                       \
                       ::boost::yap::detail::make_operand<rhs_type>{}(         \
                           static_cast<Expr &&>(rhs))}};                       \
    }


/** Defines operator overloads for the call operator taking any number of
    parameters ("operator()") that each produce an expression instantiated
    from the \a expr_template expression template.  One overload is defined
    for each of the qualifiers <code>const &</code>, <code>&</code>, and
    <code>&&</code>.  For the lvalue reference overloads, <code>*this</code>
    is captured by reference into the resulting expression.  For the rvalue
    reference overload, <code>*this</code> is moved into the resulting
    expression.

    The \a u parameters to each of the defined overloads may be any type,
    including an expression.  Each non-expression is wrapped in a terminal
    expression.

    Example:
    \snippet user_macros_snippets.cpp USER_CALL_OPERATOR

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.
*/
#define BOOST_YAP_USER_CALL_OPERATOR(expr_template)                            \
    template<typename... U>                                                    \
    constexpr auto operator()(U &&... u) const &                               \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::                                \
            operand_type_t<expr_template, decltype(*this)>;                    \
        using tuple_type = ::boost::hana::tuple<                               \
            lhs_type,                                                          \
            ::boost::yap::detail::operand_type_t<expr_template, U>...>;        \
        return expr_template<::boost::yap::expr_kind::call, tuple_type>{       \
            tuple_type{                                                        \
                ::boost::yap::detail::make_operand<lhs_type>{}(*this),         \
                ::boost::yap::detail::make_operand<                            \
                    ::boost::yap::detail::operand_type_t<expr_template, U>>{}( \
                    static_cast<U &&>(u))...}};                                \
    }                                                                          \
    template<typename... U>                                                    \
    constexpr auto operator()(U &&... u) &                                     \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::                                \
            operand_type_t<expr_template, decltype(*this)>;                    \
        using tuple_type = ::boost::hana::tuple<                               \
            lhs_type,                                                          \
            ::boost::yap::detail::operand_type_t<expr_template, U>...>;        \
        return expr_template<::boost::yap::expr_kind::call, tuple_type>{       \
            tuple_type{                                                        \
                ::boost::yap::detail::make_operand<lhs_type>{}(*this),         \
                ::boost::yap::detail::make_operand<                            \
                    ::boost::yap::detail::operand_type_t<expr_template, U>>{}( \
                    static_cast<U &&>(u))...}};                                \
    }                                                                          \
    template<typename... U>                                                    \
    constexpr auto operator()(U &&... u) &&                                    \
    {                                                                          \
        using this_type =                                                      \
            ::boost::yap::detail::remove_cv_ref_t<decltype(*this)>;            \
        using tuple_type = ::boost::hana::tuple<                               \
            this_type,                                                         \
            ::boost::yap::detail::operand_type_t<expr_template, U>...>;        \
        return expr_template<::boost::yap::expr_kind::call, tuple_type>{       \
            tuple_type{                                                        \
                std::move(*this),                                              \
                ::boost::yap::detail::make_operand<                            \
                    ::boost::yap::detail::operand_type_t<expr_template, U>>{}( \
                    static_cast<U &&>(u))...}};                                \
    }


#ifndef BOOST_YAP_DOXYGEN

#define BOOST_YAP_USER_CALL_OPERATOR_OPERAND_T(z, n, expr_template)            \
    ::boost::yap::detail::operand_type_t<expr_template, BOOST_PP_CAT(U, n)>
#define BOOST_YAP_USER_CALL_OPERATOR_MAKE_OPERAND(z, n, expr_template)         \
    ::boost::yap::detail::make_operand<::boost::yap::detail::operand_type_t<   \
        expr_template,                                                         \
        BOOST_PP_CAT(U, n)>>{}(                                                \
        static_cast<BOOST_PP_CAT(U, n) &&>(BOOST_PP_CAT(u, n)))

#endif

/** Defines operator overloads for the call operator taking N parameters
    ("operator()(t0, t1, ... tn-1)") that each produce an expression
    instantiated from the \a expr_template expression template.  One overload
    is defined for each of the qualifiers <code>const &</code>,
    <code>&</code>, and <code>&&</code>.  For the lvalue reference overloads,
    <code>*this</code> is captured by reference into the resulting expression.
    For the rvalue reference overload, <code>*this</code> is moved into the
    resulting expression.

    The \a u parameters to each of the defined overloads may be any type,
    including an expression.  Each non-expression is wrapped in a terminal
    expression.

    Example:
    \snippet user_macros_snippets.cpp USER_CALL_OPERATOR

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.

    \param n The number of parameters accepted by the operator() overloads.  n
    must be <= BOOST_PP_LIMIT_REPEAT.
*/
#define BOOST_YAP_USER_CALL_OPERATOR_N(expr_template, n)                       \
    template<BOOST_PP_ENUM_PARAMS(n, typename U)>                              \
    constexpr auto operator()(BOOST_PP_ENUM_BINARY_PARAMS(n, U, &&u)) const &  \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::                                \
            operand_type_t<expr_template, decltype(*this)>;                    \
        using tuple_type = ::boost::hana::tuple<                               \
            lhs_type,                                                          \
            BOOST_PP_ENUM(                                                     \
                n, BOOST_YAP_USER_CALL_OPERATOR_OPERAND_T, expr_template)>;    \
        return expr_template<::boost::yap::expr_kind::call, tuple_type>{       \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(*this),  \
                       BOOST_PP_ENUM(                                          \
                           n,                                                  \
                           BOOST_YAP_USER_CALL_OPERATOR_MAKE_OPERAND,          \
                           expr_template)}};                                   \
    }                                                                          \
    template<BOOST_PP_ENUM_PARAMS(n, typename U)>                              \
    constexpr auto operator()(BOOST_PP_ENUM_BINARY_PARAMS(n, U, &&u)) &        \
    {                                                                          \
        using lhs_type = ::boost::yap::detail::                                \
            operand_type_t<expr_template, decltype(*this)>;                    \
        using tuple_type = ::boost::hana::tuple<                               \
            lhs_type,                                                          \
            BOOST_PP_ENUM(                                                     \
                n, BOOST_YAP_USER_CALL_OPERATOR_OPERAND_T, expr_template)>;    \
        return expr_template<::boost::yap::expr_kind::call, tuple_type>{       \
            tuple_type{::boost::yap::detail::make_operand<lhs_type>{}(*this),  \
                       BOOST_PP_ENUM(                                          \
                           n,                                                  \
                           BOOST_YAP_USER_CALL_OPERATOR_MAKE_OPERAND,          \
                           expr_template)}};                                   \
    }                                                                          \
    template<BOOST_PP_ENUM_PARAMS(n, typename U)>                              \
    constexpr auto operator()(BOOST_PP_ENUM_BINARY_PARAMS(n, U, &&u)) &&       \
    {                                                                          \
        using this_type =                                                      \
            ::boost::yap::detail::remove_cv_ref_t<decltype(*this)>;            \
        using tuple_type = ::boost::hana::tuple<                               \
            this_type,                                                         \
            BOOST_PP_ENUM(                                                     \
                n, BOOST_YAP_USER_CALL_OPERATOR_OPERAND_T, expr_template)>;    \
        return expr_template<::boost::yap::expr_kind::call, tuple_type>{       \
            tuple_type{std::move(*this),                                       \
                       BOOST_PP_ENUM(                                          \
                           n,                                                  \
                           BOOST_YAP_USER_CALL_OPERATOR_MAKE_OPERAND,          \
                           expr_template)}};                                   \
    }


/** Defines a 3-parameter function <code>if_else()</code> that acts as an
    analogue to the ternary operator (<code>?:</code>), since the ternary
    operator is not user-overloadable.  The return type of
    <code>if_else()</code> is an expression instantiated from the \a
    expr_template expression template.

    At least one parameter to <code>if_else()</code> must be an expression.

    For each parameter E passed to <code>if_else()</code>, if E is an rvalue,
    E is moved into the result, and otherwise E is captured by reference into
    the result.

    Example:
    \snippet user_macros_snippets.cpp USER_EXPR_IF_ELSE

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.
*/
#define BOOST_YAP_USER_EXPR_IF_ELSE(expr_template)                             \
    template<typename Expr1, typename Expr2, typename Expr3>                   \
    constexpr auto if_else(Expr1 && expr1, Expr2 && expr2, Expr3 && expr3)     \
        ->::boost::yap::detail::                                               \
            ternary_op_result_t<expr_template, Expr1, Expr2, Expr3>            \
    {                                                                          \
        using result_types = ::boost::yap::detail::                            \
            ternary_op_result<expr_template, Expr1, Expr2, Expr3>;             \
        using cond_type = typename result_types::cond_type;                    \
        using then_type = typename result_types::then_type;                    \
        using else_type = typename result_types::else_type;                    \
        using tuple_type =                                                     \
            ::boost::hana::tuple<cond_type, then_type, else_type>;             \
        return {tuple_type{::boost::yap::detail::make_operand<cond_type>{}(    \
                               static_cast<Expr1 &&>(expr1)),                  \
                           ::boost::yap::detail::make_operand<then_type>{}(    \
                               static_cast<Expr2 &&>(expr2)),                  \
                           ::boost::yap::detail::make_operand<else_type>{}(    \
                               static_cast<Expr3 &&>(expr3))}};                \
    }


/** Defines a function <code>if_else()</code> that acts as an analogue to the
    ternary operator (<code>?:</code>), since the ternary operator is not
    user-overloadable.  The return type of <code>if_else()</code> is an
    expression instantiated from the \a expr_template expression template.

    Each parameter to <code>if_else()</code> may be any type that is \b not an
    expression.  At least on parameter must be a type <code>T</code> for which
    \code udt_trait<std::remove_cv_t<std::remove_reference_t<T>>>::value
    \endcode is true.  Each parameter is wrapped in a terminal expression.

    Example:
    \snippet user_macros_snippets.cpp USER_UDT_ANY_IF_ELSE

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.

    \param udt_trait A trait template to use to constrain which types are
    accepted as template parameters to <code>if_else()</code>.
*/
#define BOOST_YAP_USER_UDT_ANY_IF_ELSE(expr_template, udt_trait)               \
    template<typename Expr1, typename Expr2, typename Expr3>                   \
    constexpr auto if_else(Expr1 && expr1, Expr2 && expr2, Expr3 && expr3)     \
        ->::boost::yap::detail::udt_any_ternary_op_result_t<                   \
            expr_template,                                                     \
            Expr1,                                                             \
            Expr2,                                                             \
            Expr3,                                                             \
            udt_trait>                                                         \
    {                                                                          \
        using result_types = ::boost::yap::detail::udt_any_ternary_op_result<  \
            expr_template,                                                     \
            Expr1,                                                             \
            Expr2,                                                             \
            Expr3,                                                             \
            udt_trait>;                                                        \
        using cond_type = typename result_types::cond_type;                    \
        using then_type = typename result_types::then_type;                    \
        using else_type = typename result_types::else_type;                    \
        using tuple_type =                                                     \
            ::boost::hana::tuple<cond_type, then_type, else_type>;             \
        return {tuple_type{::boost::yap::detail::make_operand<cond_type>{}(    \
                               static_cast<Expr1 &&>(expr1)),                  \
                           ::boost::yap::detail::make_operand<then_type>{}(    \
                               static_cast<Expr2 &&>(expr2)),                  \
                           ::boost::yap::detail::make_operand<else_type>{}(    \
                               static_cast<Expr3 &&>(expr3))}};                \
    }


/** Defines a free/non-member operator overload for unary operator \a op_name
    that produces an expression instantiated from the \a expr_template
    expression template.

    The parameter to the defined operator overload may be any type that is \b
    not an expression and for which \code
    udt_trait<std::remove_cv_t<std::remove_reference_t<T>>>::value \endcode is
    true.  The parameter is wrapped in a terminal expression.

    Example:
    \snippet user_macros_snippets.cpp USER_UDT_UNARY_OPERATOR

    \param op_name The operator to be overloaded; this must be one of the \b
    unary enumerators in <code>expr_kind</code>, without the
    <code>expr_kind::</code> qualification.

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.

    \param udt_trait A trait template to use to constrain which types are
    accepted as template parameters to the defined operator overload.
*/
#define BOOST_YAP_USER_UDT_UNARY_OPERATOR(op_name, expr_template, udt_trait)   \
    template<typename T>                                                       \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(T && x)           \
        ->::boost::yap::detail::udt_unary_op_result_t<                         \
            expr_template,                                                     \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            udt_trait>                                                         \
    {                                                                          \
        using result_types = ::boost::yap::detail::udt_unary_op_result<        \
            expr_template,                                                     \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            udt_trait>;                                                        \
        using x_type = typename result_types::x_type;                          \
        using tuple_type = ::boost::hana::tuple<x_type>;                       \
        return {tuple_type{x_type{static_cast<T &&>(x)}}};                     \
    }


/** Defines a free/non-member operator overload for binary operator \a op_name
    that produces an expression instantiated from the \a expr_template
    expression template.

    The \a lhs parameter to the defined operator overload may be any type that
    is \b not an expression and for which \code
    t_udt_trait<std::remove_cv_t<std::remove_reference_t<T>>>::value \endcode is
    true.  The parameter is wrapped in a terminal expression.

    The \a rhs parameter to the defined operator overload may be any type that
    is \b not an expression and for which \code
    u_udt_trait<std::remove_cv_t<std::remove_reference_t<U>>>::value \endcode is
    true.  The parameter is wrapped in a terminal expression.

    Example:
    \snippet user_macros_snippets.cpp USER_UDT_UDT_BINARY_OPERATOR

    \param op_name The operator to be overloaded; this must be one of the \b
    binary enumerators in <code>expr_kind</code>, without the
    <code>expr_kind::</code> qualification.

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.

    \param t_udt_trait A trait template to use to constrain which types are
    accepted as \a T template parameters to the defined operator overload.

    \param u_udt_trait A trait template to use to constrain which types are
    accepted as \a U template parameters to the defined operator overload.
*/
#define BOOST_YAP_USER_UDT_UDT_BINARY_OPERATOR(                                \
    op_name, expr_template, t_udt_trait, u_udt_trait)                          \
    template<typename T, typename U>                                           \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(T && lhs, U && rhs) \
        ->::boost::yap::detail::udt_udt_binary_op_result_t<                    \
            expr_template,                                                     \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            U,                                                                 \
            t_udt_trait,                                                       \
            u_udt_trait>                                                       \
    {                                                                          \
        using result_types = ::boost::yap::detail::udt_udt_binary_op_result<   \
            expr_template,                                                     \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            U,                                                                 \
            t_udt_trait,                                                       \
            u_udt_trait>;                                                      \
        using lhs_type = typename result_types::lhs_type;                      \
        using rhs_type = typename result_types::rhs_type;                      \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return {tuple_type{                                                    \
            lhs_type{static_cast<T &&>(lhs)},                                  \
            rhs_type{static_cast<U &&>(rhs)},                                  \
        }};                                                                    \
    }


/** Defines a free/non-member operator overload for binary operator \a op_name
    that produces an expression instantiated from the \a expr_template
    expression template.

    The \a lhs and \a rhs parameters to the defined operator overload may be any
   types that are \b not expressions.  Each parameter is wrapped in a terminal
   expression.

    At least one of the parameters to the defined operator overload must be a
    type \c T for which \code
    udt_trait<std::remove_cv_t<std::remove_reference_t<T>>>::value \endcode is
    true.

    Example:
    \snippet user_macros_snippets.cpp USER_UDT_ANY_BINARY_OPERATOR

    \param op_name The operator to be overloaded; this must be one of the \b
    binary enumerators in <code>expr_kind</code>, without the
    <code>expr_kind::</code> qualification.

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.

    \param udt_trait A trait template to use to constrain which types are
    accepted as template parameters to the defined operator overload.
*/
#define BOOST_YAP_USER_UDT_ANY_BINARY_OPERATOR(                                \
    op_name, expr_template, udt_trait)                                         \
    template<typename T, typename U>                                           \
    constexpr auto operator BOOST_YAP_INDIRECT_CALL(op_name)(T && lhs, U && rhs) \
        ->::boost::yap::detail::udt_any_binary_op_result_t<                    \
            expr_template,                                                     \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            U,                                                                 \
            udt_trait>                                                         \
    {                                                                          \
        using result_types = ::boost::yap::detail::udt_any_binary_op_result<   \
            expr_template,                                                     \
            ::boost::yap::expr_kind::op_name,                                  \
            T,                                                                 \
            U,                                                                 \
            udt_trait>;                                                        \
        using lhs_type = typename result_types::lhs_type;                      \
        using rhs_type = typename result_types::rhs_type;                      \
        using tuple_type = ::boost::hana::tuple<lhs_type, rhs_type>;           \
        return {tuple_type{lhs_type{static_cast<T &&>(lhs)},                   \
                           rhs_type{static_cast<U &&>(rhs)}}};                 \
    }


/** Defines user defined literal template that creates literal placeholders
    instantiated from the \a expr_template expression template.  It is
    recommended that you put this in its own namespace.

    \param expr_template The expression template to use to instantiate the
    result expression.  \a expr_template must be an \ref
    ExpressionTemplate.
*/
#define BOOST_YAP_USER_LITERAL_PLACEHOLDER_OPERATOR(expr_template)             \
    template<char... c>                                                        \
    constexpr auto operator"" _p()                                             \
    {                                                                          \
        using i = ::boost::hana::llong<                                        \
            ::boost::hana::ic_detail::parse<sizeof...(c)>({c...})>;            \
        static_assert(1 <= i::value, "Placeholders must be >= 1.");            \
        return expr_template<                                                  \
            ::boost::yap::expr_kind::terminal,                                 \
            ::boost::hana::tuple<::boost::yap::placeholder<i::value>>>{};      \
    }

#endif
