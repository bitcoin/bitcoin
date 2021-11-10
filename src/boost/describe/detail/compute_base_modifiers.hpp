#ifndef BOOST_DESCRIBE_DETAIL_COMPUTE_BASE_MODIFIERS_HPP_INCLUDED
#define BOOST_DESCRIBE_DETAIL_COMPUTE_BASE_MODIFIERS_HPP_INCLUDED

// Copyright 2020, 2021 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/describe/modifiers.hpp>
#include <type_traits>

namespace boost
{
namespace describe
{
namespace detail
{

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4594) // can never be instantiated - indirect virtual base class is inaccessible
# pragma warning(disable: 4624) // destructor was implicitly defined as deleted
#endif

// is_public_base_of

template<class T, class U> using is_public_base_of = std::is_convertible<U*, T*>;

// is_protected_base_of

struct ipb_final
{
    template<class T, class U> using fn = std::false_type;
};

struct ipb_non_final
{
    template<class T, class U> struct fn: U
    {
        static std::true_type f( T* );

        template<class X> static auto g( X x ) -> decltype( f(x) );
        static std::false_type g( ... );

        using type = decltype( g((U*)0) );
    };
};

template<class T, class U> using is_protected_base_of =
    typename std::conditional<std::is_final<U>::value || std::is_union<U>::value, ipb_final, ipb_non_final>::type::template fn<T, U>::type;

// is_virtual_base_of

template<class T, class U, class = void> struct can_cast: std::false_type {};
template<class T, class U> struct can_cast<T, U, decltype((void)(U*)(T*)0)>: std::true_type {};

template<class T, class U> using is_virtual_base_of =
    std::integral_constant<bool, can_cast<U, T>::value && !can_cast<T, U>::value>;

// compute_base_modifiers
template<class C, class B> constexpr unsigned compute_base_modifiers() noexcept
{
    return (is_public_base_of<B, C>::value? mod_public: (is_protected_base_of<B, C>::value? mod_protected: mod_private)) | (is_virtual_base_of<B, C>::value? mod_virtual: 0);
}

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

} // namespace detail
} // namespace describe
} // namespace boost

#endif // #ifndef BOOST_DESCRIBE_DETAIL_COMPUTE_BASE_MODIFIERS_HPP_INCLUDED
