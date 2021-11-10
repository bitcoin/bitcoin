#ifndef BOOST_LAMBDA2_LAMBDA2_HPP_INCLUDED
#define BOOST_LAMBDA2_LAMBDA2_HPP_INCLUDED

// Copyright 2020, 2021 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <functional>
#include <type_traits>
#include <utility>
#include <tuple>
#include <cstddef>

// Same format as BOOST_VERSION:
//   major * 100000 + minor * 100 + patch
#define BOOST_LAMBDA2_VERSION 107700

namespace boost
{
namespace lambda2
{

namespace lambda2_detail
{

struct subscript
{
    template<class T1, class T2> decltype(auto) operator()(T1&& t1, T2&& t2) const
    {
        return std::forward<T1>(t1)[ std::forward<T2>(t2) ];
    }
};

} // namespace lambda2_detail

// placeholders

template<int I> struct lambda2_arg
{
    template<class... A> decltype(auto) operator()( A&&... a ) const noexcept
    {
        return std::get<std::size_t{I-1}>( std::tuple<A&&...>( std::forward<A>(a)... ) );
    }

    template<class T> auto operator[]( T&& t ) const
    {
        return std::bind( lambda2_detail::subscript(), *this, std::forward<T>( t ) );
    }
};

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
# define BOOST_LAMBDA2_INLINE_VAR inline
#else
# define BOOST_LAMBDA2_INLINE_VAR
#endif

BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<1> _1{};
BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<2> _2{};
BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<3> _3{};
BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<4> _4{};
BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<5> _5{};
BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<6> _6{};
BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<7> _7{};
BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<8> _8{};
BOOST_LAMBDA2_INLINE_VAR constexpr lambda2_arg<9> _9{};

#undef BOOST_LAMBDA2_INLINE_VAR

} // namespace lambda2
} // namespace boost

namespace std
{

template<int I> struct is_placeholder< boost::lambda2::lambda2_arg<I> >: integral_constant<int, I>
{
};

} // namespace std

namespace boost
{
namespace lambda2
{

namespace lambda2_detail
{

// additional function objects

#define BOOST_LAMBDA2_UNARY_FN(op, fn) \
    struct fn \
    { \
        template<class T> decltype(auto) operator()(T&& t) const \
        { \
            return op std::forward<T>(t); \
        } \
    };

#define BOOST_LAMBDA2_POSTFIX_FN(op, fn) \
    struct fn \
    { \
        template<class T> decltype(auto) operator()(T&& t) const \
        { \
            return std::forward<T>(t) op; \
        } \
    };

#define BOOST_LAMBDA2_BINARY_FN(op, fn) \
    struct fn \
    { \
        template<class T1, class T2> decltype(auto) operator()(T1&& t1, T2&& t2) const \
        { \
            return std::forward<T1>(t1) op std::forward<T2>(t2); \
        } \
    };

BOOST_LAMBDA2_BINARY_FN(<<, left_shift)
BOOST_LAMBDA2_BINARY_FN(>>, right_shift)

BOOST_LAMBDA2_UNARY_FN(+, unary_plus)
BOOST_LAMBDA2_UNARY_FN(*, dereference)

BOOST_LAMBDA2_UNARY_FN(++, increment)
BOOST_LAMBDA2_UNARY_FN(--, decrement)

BOOST_LAMBDA2_POSTFIX_FN(++, postfix_increment)
BOOST_LAMBDA2_POSTFIX_FN(--, postfix_decrement)

BOOST_LAMBDA2_BINARY_FN(+=, plus_equal)
BOOST_LAMBDA2_BINARY_FN(-=, minus_equal)
BOOST_LAMBDA2_BINARY_FN(*=, multiplies_equal)
BOOST_LAMBDA2_BINARY_FN(/=, divides_equal)
BOOST_LAMBDA2_BINARY_FN(%=, modulus_equal)
BOOST_LAMBDA2_BINARY_FN(&=, bit_and_equal)
BOOST_LAMBDA2_BINARY_FN(|=, bit_or_equal)
BOOST_LAMBDA2_BINARY_FN(^=, bit_xor_equal)
BOOST_LAMBDA2_BINARY_FN(<<=, left_shift_equal)
BOOST_LAMBDA2_BINARY_FN(>>=, right_shift_equal)

// operators

template<class T> using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<class T, class T2 = remove_cvref_t<T>> using is_lambda_expression =
    std::integral_constant<bool, std::is_placeholder<T2>::value || std::is_bind_expression<T2>::value>;

template<class A> using enable_unary_lambda =
    std::enable_if_t<is_lambda_expression<A>::value>;

template<class A, class B> using enable_binary_lambda =
    std::enable_if_t<is_lambda_expression<A>::value || is_lambda_expression<B>::value>;

} // namespace lambda2_detail

#define BOOST_LAMBDA2_UNARY_LAMBDA(op, fn) \
    template<class A, class = lambda2_detail::enable_unary_lambda<A>> \
    auto operator op( A&& a ) \
    { \
        return std::bind( fn(), std::forward<A>(a) ); \
    }

#define BOOST_LAMBDA2_POSTFIX_LAMBDA(op, fn) \
    template<class A, class = lambda2_detail::enable_unary_lambda<A>> \
    auto operator op( A&& a, int ) \
    { \
        return std::bind( fn(), std::forward<A>(a) ); \
    }

#define BOOST_LAMBDA2_BINARY_LAMBDA(op, fn) \
    template<class A, class B, class = lambda2_detail::enable_binary_lambda<A, B>> \
    auto operator op( A&& a, B&& b ) \
    { \
        return std::bind( fn(), std::forward<A>(a), std::forward<B>(b) ); \
    }

// standard

BOOST_LAMBDA2_BINARY_LAMBDA(+, std::plus<>)
BOOST_LAMBDA2_BINARY_LAMBDA(-, std::minus<>)
BOOST_LAMBDA2_BINARY_LAMBDA(*, std::multiplies<>)
BOOST_LAMBDA2_BINARY_LAMBDA(/, std::divides<>)
BOOST_LAMBDA2_BINARY_LAMBDA(%, std::modulus<>)
BOOST_LAMBDA2_UNARY_LAMBDA(-, std::negate<>)

BOOST_LAMBDA2_BINARY_LAMBDA(==, std::equal_to<>)
BOOST_LAMBDA2_BINARY_LAMBDA(!=, std::not_equal_to<>)
BOOST_LAMBDA2_BINARY_LAMBDA(>, std::greater<>)
BOOST_LAMBDA2_BINARY_LAMBDA(<, std::less<>)
BOOST_LAMBDA2_BINARY_LAMBDA(>=, std::greater_equal<>)
BOOST_LAMBDA2_BINARY_LAMBDA(<=, std::less_equal<>)

BOOST_LAMBDA2_BINARY_LAMBDA(&&, std::logical_and<>)
BOOST_LAMBDA2_BINARY_LAMBDA(||, std::logical_or<>)
BOOST_LAMBDA2_UNARY_LAMBDA(!, std::logical_not<>)

BOOST_LAMBDA2_BINARY_LAMBDA(&, std::bit_and<>)
BOOST_LAMBDA2_BINARY_LAMBDA(|, std::bit_or<>)
BOOST_LAMBDA2_BINARY_LAMBDA(^, std::bit_xor<>)
BOOST_LAMBDA2_UNARY_LAMBDA(~, std::bit_not<>)

// additional

BOOST_LAMBDA2_BINARY_LAMBDA(<<, lambda2_detail::left_shift)
BOOST_LAMBDA2_BINARY_LAMBDA(>>, lambda2_detail::right_shift)

BOOST_LAMBDA2_UNARY_LAMBDA(+, lambda2_detail::unary_plus)
BOOST_LAMBDA2_UNARY_LAMBDA(*, lambda2_detail::dereference)

BOOST_LAMBDA2_UNARY_LAMBDA(++, lambda2_detail::increment)
BOOST_LAMBDA2_UNARY_LAMBDA(--, lambda2_detail::decrement)

BOOST_LAMBDA2_POSTFIX_LAMBDA(++, lambda2_detail::postfix_increment)
BOOST_LAMBDA2_POSTFIX_LAMBDA(--, lambda2_detail::postfix_decrement)

// compound assignment

BOOST_LAMBDA2_BINARY_LAMBDA(+=, lambda2_detail::plus_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(-=, lambda2_detail::minus_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(*=, lambda2_detail::multiplies_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(/=, lambda2_detail::divides_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(%=, lambda2_detail::modulus_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(&=, lambda2_detail::bit_and_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(|=, lambda2_detail::bit_or_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(^=, lambda2_detail::bit_xor_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(<<=, lambda2_detail::left_shift_equal)
BOOST_LAMBDA2_BINARY_LAMBDA(>>=, lambda2_detail::right_shift_equal)

} // namespace lambda2
} // namespace boost

#endif // #ifndef BOOST_LAMBDA2_LAMBDA2_HPP_INCLUDED
