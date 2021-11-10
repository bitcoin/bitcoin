// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_UTIL_PROMOTE_INTEGRAL_HPP
#define BOOST_GEOMETRY_UTIL_PROMOTE_INTEGRAL_HPP

// For now deactivate the use of multiprecision integers
// TODO: activate it later
#define BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER


#include <climits>
#include <cstddef>
#include <type_traits>

#if !defined(BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER)
#include <boost/multiprecision/cpp_int.hpp>
#endif


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace promote_integral
{

// meta-function that returns the bit size of a type
template
<
    typename T,
    bool IsFundamental = std::is_fundamental<T>::value
>
struct bit_size
{};


// for fundamental types, just return CHAR_BIT * sizeof(T)
template <typename T>
struct bit_size<T, true>
    : std::integral_constant<std::size_t, (CHAR_BIT * sizeof(T))>
{};


#if !defined(BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER)
// partial specialization for cpp_int
template
<
    unsigned MinSize,
    unsigned MaxSize,
    boost::multiprecision::cpp_integer_type SignType,
    boost::multiprecision::cpp_int_check_type Checked,
    typename Allocator,
    boost::multiprecision::expression_template_option ExpressionTemplates
>
struct bit_size
    <
        boost::multiprecision::number
            <
                boost::multiprecision::cpp_int_backend
                    <
                        MinSize, MaxSize, SignType, Checked, Allocator
                    >,
                ExpressionTemplates
            >,
        false
    >
    : std::integral_constant<std::size_t, MaxSize>
{};
#endif // BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER


template <typename T, std::size_t MinSize, typename ...Ts>
struct promote_to_larger
{
    // if promotion fails, keep the number T
    // (and cross fingers that overflow will not occur)
    typedef T type;
};

template <typename T, std::size_t MinSize, typename CurrentT, typename ...Ts>
struct promote_to_larger<T, MinSize, CurrentT, Ts...>
{
    typedef std::conditional_t
        <
            (bit_size<CurrentT>::value >= MinSize),
            CurrentT,
            typename promote_to_larger<T, MinSize, Ts...>::type
        > type;
};

template <typename ...Ts>
struct integral_types {};

template <typename T, std::size_t MinSize, typename ...Ts>
struct promote_to_larger<T, MinSize, integral_types<Ts...>>
    : promote_to_larger<T, MinSize, Ts...>
{};


}} // namespace detail::promote_integral
#endif // DOXYGEN_NO_DETAIL



/*!
    \brief Meta-function to define an integral type with size
    than is (roughly) twice the bit size of T
    \ingroup utility
    \details
    This meta-function tries to promote the fundamental integral type T
    to a another integral type with size (roughly) twice the bit size of T.

    To do this, two times the bit size of T is tested against the bit sizes of:
         short, int, long, long long, boost::int128_t
    and the one that first matches is chosen.

    For unsigned types the bit size of T is tested against the bit
    sizes of the types above, if T is promoted to a signed type, or
    the bit sizes of
         unsigned short, unsigned int, unsigned long, std::size_t,
         unsigned long long, boost::uint128_t
    if T is promoted to an unsigned type.

    By default an unsigned type is promoted to a signed type.
    This behavior is controlled by the PromoteUnsignedToUnsigned
    boolean template parameter, whose default value is "false".
    To promote an unsigned type to an unsigned type set the value of
    this template parameter to "true".

    If the macro BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER is not
    defined, boost's multiprecision integer cpp_int<> is used as a
    last resort.

    If BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER is defined and an
    appropriate type cannot be detected, the input type is returned as is.

    Finally, if the passed type is either a floating-point type or a
    user-defined type it is returned as is.

    \note boost::int128_type and boost::uint128_type are considered
    only if the macros BOOST_HAS_INT128 and BOOST_GEOMETRY_ENABLE_INT128
    are defined
*/
template
<
    typename T,
    bool PromoteUnsignedToUnsigned = false,
    bool UseCheckedInteger = false,
    bool IsIntegral = std::is_integral<T>::value
>
class promote_integral
{
private:
    static bool const is_unsigned = std::is_unsigned<T>::value;

    typedef detail::promote_integral::bit_size<T> bit_size_type;

#if !defined(BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER)
    // Define the proper check policy for the multiprecision integer
    typedef std::conditional_t
        <
            UseCheckedInteger,
            std::integral_constant
                <
                    boost::multiprecision::cpp_int_check_type,
                    boost::multiprecision::checked
                >,
            std::integral_constant
                <
                    boost::multiprecision::cpp_int_check_type,
                    boost::multiprecision::unchecked
                >
        > check_policy_type;

    // Meta-function to get the multiprecision integer type for the
    // given size and sign type (signed/unsigned)
    template
    <
        unsigned int Size,
        boost::multiprecision::cpp_integer_type SignType
    >
    struct multiprecision_integer_type
    {
        typedef boost::multiprecision::number
            <
                boost::multiprecision::cpp_int_backend
                    <
                        Size,
                        Size,
                        SignType,
                        check_policy_type::value,
                        void
                    >
            > type;
    };
#endif

    // Define the minimum size (in bits) needed for the promoted type
    // If T is the input type and P the promoted type, then the
    // minimum number of bits for P are (below b stands for the number
    // of bits of T):
    // * if T is unsigned and P is unsigned: 2 * b
    // * if T is signed and P is signed: 2 * b - 1
    // * if T is unsigned and P is signed: 2 * b + 1
    typedef std::conditional_t
        <
            (PromoteUnsignedToUnsigned && is_unsigned),
            std::integral_constant<std::size_t, (2 * bit_size_type::value)>,
            std::conditional_t
                <
                    is_unsigned,
                    std::integral_constant<std::size_t, (2 * bit_size_type::value + 1)>,
                    std::integral_constant<std::size_t, (2 * bit_size_type::value - 1)>
                >
        > min_bit_size_type;

    // Define the list of signed integral types we are going to use
    // for promotion
    typedef detail::promote_integral::integral_types
        <
            short,
            int,
            long,
            long long
#if defined(BOOST_HAS_INT128) && defined(BOOST_GEOMETRY_ENABLE_INT128)
            , boost::int128_type
#endif
#if !defined(BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER)
            , typename multiprecision_integer_type
                <
                    min_bit_size_type::value,
                    boost::multiprecision::signed_magnitude
                >::type
#endif
        > signed_integral_types;

    // Define the list of unsigned integral types we are going to use
    // for promotion
    typedef detail::promote_integral::integral_types
        <
            unsigned short,
            unsigned int,
            unsigned long,
            std::size_t,
            unsigned long long
#if defined(BOOST_HAS_INT128) && defined(BOOST_GEOMETRY_ENABLE_INT128)
            , boost::uint128_type
#endif
#if !defined(BOOST_GEOMETRY_NO_MULTIPRECISION_INTEGER)
            , typename multiprecision_integer_type
                <
                    min_bit_size_type::value,
                    boost::multiprecision::unsigned_magnitude
                >::type
#endif
        > unsigned_integral_types;

    // Define the list of integral types that will be used for
    // promotion (depending in whether we was to promote unsigned to
    // unsigned or not)
    typedef std::conditional_t
        <
            (is_unsigned && PromoteUnsignedToUnsigned),
            unsigned_integral_types,
            signed_integral_types
        > integral_types;

public:
    typedef typename detail::promote_integral::promote_to_larger
        <
            T,
            min_bit_size_type::value,
            integral_types            
        >::type type;
};


template <typename T, bool PromoteUnsignedToUnsigned, bool UseCheckedInteger>
class promote_integral
    <
        T, PromoteUnsignedToUnsigned, UseCheckedInteger, false
    >
{
public:
    typedef T type;
};


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_PROMOTE_INTEGRAL_HPP
