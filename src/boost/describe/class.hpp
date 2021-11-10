#ifndef BOOST_DESCRIBE_CLASS_HPP_INCLUDED
#define BOOST_DESCRIBE_CLASS_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/describe/detail/config.hpp>

#if !defined(BOOST_DESCRIBE_CXX14)

#define BOOST_DESCRIBE_CLASS(C, Bases, Public, Protected, Private)
#define BOOST_DESCRIBE_STRUCT(C, Bases, Members)

#else

#include <boost/describe/detail/bases.hpp>
#include <boost/describe/detail/members.hpp>
#include <type_traits>

namespace boost
{
namespace describe
{

#if defined(_MSC_VER) && !defined(__clang__)

#define BOOST_DESCRIBE_PP_UNPACK(...) __VA_ARGS__

#define BOOST_DESCRIBE_CLASS(C, Bases, Public, Protected, Private) \
    friend BOOST_DESCRIBE_BASES(C, BOOST_DESCRIBE_PP_UNPACK Bases) \
    friend BOOST_DESCRIBE_PUBLIC_MEMBERS(C, BOOST_DESCRIBE_PP_UNPACK Public) \
    friend BOOST_DESCRIBE_PROTECTED_MEMBERS(C, BOOST_DESCRIBE_PP_UNPACK Protected) \
    friend BOOST_DESCRIBE_PRIVATE_MEMBERS(C, BOOST_DESCRIBE_PP_UNPACK Private)

#define BOOST_DESCRIBE_STRUCT(C, Bases, Members) \
    static_assert(std::is_class<C>::value, "BOOST_DESCRIBE_STRUCT should only be used with class types"); \
    BOOST_DESCRIBE_BASES(C, BOOST_DESCRIBE_PP_UNPACK Bases) \
    BOOST_DESCRIBE_PUBLIC_MEMBERS(C, BOOST_DESCRIBE_PP_UNPACK Members) \
    BOOST_DESCRIBE_PROTECTED_MEMBERS(C) \
    BOOST_DESCRIBE_PRIVATE_MEMBERS(C)

#else

#if defined(__GNUC__) && __GNUC__ >= 8
# define BOOST_DESCRIBE_PP_UNPACK(...) __VA_OPT__(,) __VA_ARGS__
#else
# define BOOST_DESCRIBE_PP_UNPACK(...) , ##__VA_ARGS__
#endif

#define BOOST_DESCRIBE_BASES_(...) BOOST_DESCRIBE_BASES(__VA_ARGS__)
#define BOOST_DESCRIBE_PUBLIC_MEMBERS_(...) BOOST_DESCRIBE_PUBLIC_MEMBERS(__VA_ARGS__)
#define BOOST_DESCRIBE_PROTECTED_MEMBERS_(...) BOOST_DESCRIBE_PROTECTED_MEMBERS(__VA_ARGS__)
#define BOOST_DESCRIBE_PRIVATE_MEMBERS_(...) BOOST_DESCRIBE_PRIVATE_MEMBERS(__VA_ARGS__)

#define BOOST_DESCRIBE_CLASS(C, Bases, Public, Protected, Private) \
    friend BOOST_DESCRIBE_BASES_(C BOOST_DESCRIBE_PP_UNPACK Bases) \
    friend BOOST_DESCRIBE_PUBLIC_MEMBERS_(C BOOST_DESCRIBE_PP_UNPACK Public) \
    friend BOOST_DESCRIBE_PROTECTED_MEMBERS_(C BOOST_DESCRIBE_PP_UNPACK Protected) \
    friend BOOST_DESCRIBE_PRIVATE_MEMBERS_(C BOOST_DESCRIBE_PP_UNPACK Private)

#define BOOST_DESCRIBE_STRUCT(C, Bases, Members) \
    static_assert(std::is_class<C>::value, "BOOST_DESCRIBE_STRUCT should only be used with class types"); \
    BOOST_DESCRIBE_BASES_(C BOOST_DESCRIBE_PP_UNPACK Bases) \
    BOOST_DESCRIBE_PUBLIC_MEMBERS_(C BOOST_DESCRIBE_PP_UNPACK Members) \
    BOOST_DESCRIBE_PROTECTED_MEMBERS_(C) \
    BOOST_DESCRIBE_PRIVATE_MEMBERS_(C)

#endif

} // namespace describe
} // namespace boost

#endif // !defined(BOOST_DESCRIBE_CXX14)

#endif // #ifndef BOOST_DESCRIBE_CLASS_HPP_INCLUDED
