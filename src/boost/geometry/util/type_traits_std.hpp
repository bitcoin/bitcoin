// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_UTIL_TYPE_TRAITS_STD_HPP
#define BOOST_GEOMETRY_UTIL_TYPE_TRAITS_STD_HPP


#include <cstddef>
#include <type_traits>


namespace boost { namespace geometry
{


namespace util
{


// C++17
template <bool B>
using bool_constant = std::integral_constant<bool, B>;

// non-standard
template <int I>
using int_constant = std::integral_constant<int, I>;

// non-standard
template <std::size_t I>
using index_constant = std::integral_constant<std::size_t, I>;

// non-standard
template <std::size_t S>
using size_constant = std::integral_constant<std::size_t, S>;


// C++17
template <typename ...>
struct conjunction
    : std::true_type
{};
template<typename Trait>
struct conjunction<Trait>
    : Trait
{};
template <typename Trait, typename ...Traits>
struct conjunction<Trait, Traits...>
    : std::conditional_t<Trait::value, conjunction<Traits...>, Trait>
{};

// C++17
template <typename ...>
struct disjunction
    : std::false_type
{};
template <typename Trait>
struct disjunction<Trait>
    : Trait
{};
template <typename Trait, typename ...Traits>
struct disjunction<Trait, Traits...>
    : std::conditional_t<Trait::value, Trait, disjunction<Traits...>>
{};

// C++17
template <typename Trait>
struct negation
    : bool_constant<!Trait::value>
{};


// non-standard
/*
template <typename ...Traits>
using and_ = conjunction<Traits...>;

template <typename ...Traits>
using or_ = disjunction<Traits...>;

template <typename Trait>
using not_ = negation<Trait>;
*/


// C++20
template <typename T>
struct remove_cvref
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

// non-standard
template <typename T>
struct remove_cref
{
    using type = std::remove_const_t<std::remove_reference_t<T>>;
};

template <typename T>
using remove_cref_t = typename remove_cref<T>::type;

// non-standard
template <typename T>
struct remove_cptrref
{
    using type = std::remove_const_t
        <
            std::remove_pointer_t<std::remove_reference_t<T>>
        >;
};

template <typename T>
using remove_cptrref_t = typename remove_cptrref<T>::type;


// non-standard
template <typename From, typename To>
struct transcribe_const
{
    using type = std::conditional_t
                    <
                        std::is_const<std::remove_reference_t<From>>::value,
                        std::add_const_t<To>,
                        To
                    >;
};

template <typename From, typename To>
using transcribe_const_t = typename transcribe_const<From, To>::type;


// non-standard
template <typename From, typename To>
struct transcribe_reference
{
    using type = std::remove_reference_t<To>;
};

template <typename From, typename To>
struct transcribe_reference<From &, To>
{
    using type = std::remove_reference_t<To> &;
};

template <typename From, typename To>
struct transcribe_reference<From &&, To>
{
    using type = std::remove_reference_t<To> &&;
};

template <typename From, typename To>
using transcribe_reference_t = typename transcribe_reference<From, To>::type;


// non-standard
template <typename From, typename To>
struct transcribe_cref
{
    using type = transcribe_reference_t<From, transcribe_const_t<From, To>>;
};

template <typename From, typename To>
using transcribe_cref_t = typename transcribe_cref<From, To>::type;


} // namespace util


// Deprecated utilities, defined for backward compatibility but might be
// removed in the future.


/*!
    \brief Meta-function to define a const or non const type
    \ingroup utility
    \details If the boolean template parameter is true, the type parameter
        will be defined as const, otherwise it will be defined as it was.
        This meta-function is used to have one implementation for both
        const and non const references
    \note This traits class is completely independant from Boost.Geometry
        and might be a separate addition to Boost
    \note Used in a.o. for_each, interior_rings, exterior_ring
    \par Example
    \code
        void foo(typename add_const_if_c<IsConst, Point>::type& point)
    \endcode
*/
template <bool IsConst, typename Type>
struct add_const_if_c
{
    typedef std::conditional_t
        <
            IsConst,
            Type const,
            Type
        > type;
};


namespace util
{

template <typename T>
using bare_type = remove_cptrref<T>;

} // namespace util


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_TYPE_TRAITS_STD_HPP
