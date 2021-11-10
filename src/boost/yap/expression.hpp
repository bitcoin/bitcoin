// Copyright (C) 2016-2018 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_YAP_EXPRESSION_HPP_INCLUDED
#define BOOST_YAP_EXPRESSION_HPP_INCLUDED

#include <boost/yap/algorithm.hpp>


namespace boost { namespace yap {

    /** Reference expression template that provides all operator overloads.

        \note Due to a limitation of Doxygen, each of the
        <code>value()</code>, <code>left()</code>, <code>right()</code>, and
        operator overloads listed here is a stand-in for three member
        functions.  For each function <code>f</code>, the listing here is:
        \code return_type f (); \endcode  However, there are actually three
        functions:
        \code
        return_type f () const &;
        return_type f () &;
        return_type f () &&;
        \endcode
    */
    template<expr_kind Kind, typename Tuple>
    struct expression
    {
        using tuple_type = Tuple;

        static const expr_kind kind = Kind;

        /** Default constructor.  Does nothing. */
        constexpr expression() {}

        /** Moves \a rhs into the only data mamber, \c elements. */
        constexpr expression(tuple_type && rhs) :
            elements(static_cast<tuple_type &&>(rhs))
        {}

        tuple_type elements;

        /** A convenience member function that dispatches to the free function
            <code>value()</code>. */
        constexpr decltype(auto) value() &
        {
            return ::boost::yap::value(*this);
        }

#ifndef BOOST_YAP_DOXYGEN

        constexpr decltype(auto) value() const &
        {
            return ::boost::yap::value(*this);
        }

        constexpr decltype(auto) value() &&
        {
            return ::boost::yap::value(std::move(*this));
        }

#endif

        /** A convenience member function that dispatches to the free function
            <code>left()</code>. */
        constexpr decltype(auto) left() & { return ::boost::yap::left(*this); }

#ifndef BOOST_YAP_DOXYGEN

        constexpr decltype(auto) left() const &
        {
            return ::boost::yap::left(*this);
        }

        constexpr decltype(auto) left() &&
        {
            return ::boost::yap::left(std::move(*this));
        }

#endif

        /** A convenience member function that dispatches to the free function
            <code>right()</code>. */
        constexpr decltype(auto) right() &
        {
            return ::boost::yap::right(*this);
        }

#ifndef BOOST_YAP_DOXYGEN

        constexpr decltype(auto) right() const &
        {
            return ::boost::yap::right(*this);
        }

        constexpr decltype(auto) right() &&
        {
            return ::boost::yap::right(std::move(*this));
        }

#endif

        BOOST_YAP_USER_ASSIGN_OPERATOR(
            expression, ::boost::yap::expression)                   // =
        BOOST_YAP_USER_SUBSCRIPT_OPERATOR(::boost::yap::expression) // []
        BOOST_YAP_USER_CALL_OPERATOR(::boost::yap::expression)      // ()
    };

    /** Terminal expression specialization of the reference expression
        template.

        \note Due to a limitation of Doxygen, the <code>value()</code> member
        and each of the operator overloads listed here is a stand-in for three
        member functions.  For each function <code>f</code>, the listing here
        is: \code return_type f (); \endcode However, there are actually three
        functions:
        \code
        return_type f () const &;
        return_type f () &;
        return_type f () &&;
        \endcode
    */
    template<typename T>
    struct expression<expr_kind::terminal, hana::tuple<T>>
    {
        using tuple_type = hana::tuple<T>;

        static const expr_kind kind = expr_kind::terminal;

        /** Default constructor.  Does nothing. */
        constexpr expression() {}

        /** Forwards \a t into \c elements. */
        constexpr expression(T && t) : elements(static_cast<T &&>(t)) {}

        /** Copies \a rhs into the only data mamber, \c elements. */
        constexpr expression(hana::tuple<T> const & rhs) : elements(rhs) {}

        /** Moves \a rhs into the only data mamber, \c elements. */
        constexpr expression(hana::tuple<T> && rhs) : elements(std::move(rhs))
        {}

        tuple_type elements;

        /** A convenience member function that dispatches to the free function
            <code>value()</code>. */
        constexpr decltype(auto) value() &
        {
            return ::boost::yap::value(*this);
        }

#ifndef BOOST_YAP_DOXYGEN

        constexpr decltype(auto) value() const &
        {
            return ::boost::yap::value(*this);
        }

        constexpr decltype(auto) value() &&
        {
            return ::boost::yap::value(std::move(*this));
        }

#endif

        BOOST_YAP_USER_ASSIGN_OPERATOR(
            expression, ::boost::yap::expression)                   // =
        BOOST_YAP_USER_SUBSCRIPT_OPERATOR(::boost::yap::expression) // []
        BOOST_YAP_USER_CALL_OPERATOR(::boost::yap::expression)      // ()
    };

#ifndef BOOST_YAP_DOXYGEN

    BOOST_YAP_USER_UNARY_OPERATOR(unary_plus, expression, expression)  // +
    BOOST_YAP_USER_UNARY_OPERATOR(negate, expression, expression)      // -
    BOOST_YAP_USER_UNARY_OPERATOR(dereference, expression, expression) // *
    BOOST_YAP_USER_UNARY_OPERATOR(complement, expression, expression)  // ~
    BOOST_YAP_USER_UNARY_OPERATOR(address_of, expression, expression)  // &
    BOOST_YAP_USER_UNARY_OPERATOR(logical_not, expression, expression) // !
    BOOST_YAP_USER_UNARY_OPERATOR(pre_inc, expression, expression)     // ++
    BOOST_YAP_USER_UNARY_OPERATOR(pre_dec, expression, expression)     // --
    BOOST_YAP_USER_UNARY_OPERATOR(post_inc, expression, expression)    // ++(int)
    BOOST_YAP_USER_UNARY_OPERATOR(post_dec, expression, expression)    // --(int)

    BOOST_YAP_USER_BINARY_OPERATOR(shift_left, expression, expression)         // <<
    BOOST_YAP_USER_BINARY_OPERATOR(shift_right, expression, expression)        // >>
    BOOST_YAP_USER_BINARY_OPERATOR(multiplies, expression, expression)         // *
    BOOST_YAP_USER_BINARY_OPERATOR(divides, expression, expression)            // /
    BOOST_YAP_USER_BINARY_OPERATOR(modulus, expression, expression)            // %
    BOOST_YAP_USER_BINARY_OPERATOR(plus, expression, expression)               // +
    BOOST_YAP_USER_BINARY_OPERATOR(minus, expression, expression)              // -
    BOOST_YAP_USER_BINARY_OPERATOR(less, expression, expression)               // <
    BOOST_YAP_USER_BINARY_OPERATOR(greater, expression, expression)            // >
    BOOST_YAP_USER_BINARY_OPERATOR(less_equal, expression, expression)         // <=
    BOOST_YAP_USER_BINARY_OPERATOR(greater_equal, expression, expression)      // >=
    BOOST_YAP_USER_BINARY_OPERATOR(equal_to, expression, expression)           // ==
    BOOST_YAP_USER_BINARY_OPERATOR(not_equal_to, expression, expression)       // !=
    BOOST_YAP_USER_BINARY_OPERATOR(logical_or, expression, expression)         // ||
    BOOST_YAP_USER_BINARY_OPERATOR(logical_and, expression, expression)        // &&
    BOOST_YAP_USER_BINARY_OPERATOR(bitwise_and, expression, expression)        // &
    BOOST_YAP_USER_BINARY_OPERATOR(bitwise_or, expression, expression)         // |
    BOOST_YAP_USER_BINARY_OPERATOR(bitwise_xor, expression, expression)        // ^
    BOOST_YAP_USER_BINARY_OPERATOR(comma, expression, expression)              // ,
    BOOST_YAP_USER_BINARY_OPERATOR(mem_ptr, expression, expression)            // ->*
    BOOST_YAP_USER_BINARY_OPERATOR(shift_left_assign, expression, expression)  // <<=
    BOOST_YAP_USER_BINARY_OPERATOR(shift_right_assign, expression, expression) // >>=
    BOOST_YAP_USER_BINARY_OPERATOR(multiplies_assign, expression, expression)  // *=
    BOOST_YAP_USER_BINARY_OPERATOR(divides_assign, expression, expression)     // /=
    BOOST_YAP_USER_BINARY_OPERATOR(modulus_assign, expression, expression)     // %=
    BOOST_YAP_USER_BINARY_OPERATOR(plus_assign, expression, expression)        // +=
    BOOST_YAP_USER_BINARY_OPERATOR(minus_assign, expression, expression)       // -=
    BOOST_YAP_USER_BINARY_OPERATOR(bitwise_and_assign, expression, expression) // &=
    BOOST_YAP_USER_BINARY_OPERATOR(bitwise_or_assign, expression, expression)  // |=
    BOOST_YAP_USER_BINARY_OPERATOR(bitwise_xor_assign, expression, expression) // ^=

    BOOST_YAP_USER_EXPR_IF_ELSE(expression)

#else

    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator+(Expr &&);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator-(Expr &&);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator*(Expr &&);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator~(Expr &&);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator&(Expr &&);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator!(Expr &&);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator++(Expr &&);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator--(Expr &&);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator++(Expr &&, int);
    /** \see BOOST_YAP_USER_UNARY_OPERATOR for full semantics. */
    template<typename Expr>
    constexpr auto operator--(Expr &&, int);

    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator<<(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator>>(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator*(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator/(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator%(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator+(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator-(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator<(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator>(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator<=(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator>=(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator==(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator!=(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator||(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator&&(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator&(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator|(LExpr && lhs, RExpr && rhs);
    /** \see BOOST_YAP_USER_BINARY_OPERATOR for full semantics. */
    template<typename LExpr, typename RExpr>
    constexpr auto operator^(LExpr && lhs, RExpr && rhs);

    /** \see BOOST_YAP_USER_EXPR_IF_ELSE for full semantics. */
    template<typename Expr1, typename Expr2, typename Expr3>
    constexpr auto if_else(Expr1 && expr1, Expr2 && expr2, Expr3 && expr3);

#endif

    /** Returns <code>make_expression<boost::yap::expression, Kind>(...)</code>.
     */
    template<expr_kind Kind, typename... T>
    constexpr auto make_expression(T &&... t)
    {
        return make_expression<expression, Kind>(static_cast<T &&>(t)...);
    }

    /** Returns <code>make_terminal<boost::yap::expression>(t)</code>. */
    template<typename T>
    constexpr auto make_terminal(T && t)
    {
        return make_terminal<expression>(static_cast<T &&>(t));
    }

    /** Returns <code>as_expr<boost::yap::expression>(t)</code>. */
    template<typename T>
    constexpr decltype(auto) as_expr(T && t)
    {
        return as_expr<expression>(static_cast<T &&>(t));
    }

}}

#endif
