/*

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_IS_INVOCABLE_HPP
#define BOOST_CLBL_TRTS_IS_INVOCABLE_HPP

#include <boost/callable_traits/detail/core.hpp>
#include <boost/callable_traits/detail/is_invocable_impl.hpp>

namespace boost { namespace callable_traits {

//[ is_invocable_hpp
/*`[section:ref_is_invocable is_invocable]
[heading Header]
``#include <boost/callable_traits/is_invocable.hpp>``
[heading Definition]
*/

// inherits from either std::true_type or std::false_type
template<typename T, typename... Args>
struct is_invocable;

// inherits from either std::true_type or std::false_type
template<typename Ret, typename T, typename... Args>
struct is_invocable_r;

//<-
template<typename T, typename... Args>
struct is_invocable : detail::is_invocable_impl<T, Args...>::type {
    using type = typename detail::is_invocable_impl<T, Args...>::type;
};

template<typename Ret, typename T, typename... Args>
struct is_invocable_r
  : detail::is_invocable_r_impl<
        typename detail::is_invocable_impl<T, Args...>::type, Ret, T, Args...>::type
{
    using type = typename detail::is_invocable_r_impl<
        typename detail::is_invocable_impl<T, Args...>::type, Ret, T, Args...>::type;
};

#ifdef BOOST_CLBL_TRTS_DISABLE_VARIABLE_TEMPLATES

template<typename T, typename... Args>
struct is_invocable_v {
    static_assert(std::is_same<T, detail::dummy>::value,
        "Variable templates not supported on this compiler.");
};

template<typename Ret, typename T, typename... Args>
struct is_invocable_r_v {
    static_assert(std::is_same<T, detail::dummy>::value,
        "Variable templates not supported on this compiler.");
};

#else
//->
// only available when variable templates are supported
template<typename T, typename... Args>
//<-
BOOST_CLBL_TRAITS_INLINE_VAR
//->
constexpr bool is_invocable_v = //see below
//<-
    detail::is_invocable_impl<T, Args...>::type::value;
//->

// only available when variable templates are supported
template<typename Ret, typename T, typename... Args>
//<-
BOOST_CLBL_TRAITS_INLINE_VAR
//->
constexpr bool is_invocable_r_v = //see below
//<-
    detail::is_invocable_r_impl<
        typename detail::is_invocable_impl<T, Args...>::type,
        Ret, T, Args...>::type::value;
#endif

}} // namespace boost::callable_traits
//->

/*`
[heading Constraints]
* none

[heading Behavior]
* standalone c++11 implementation of c++17 `std::is_invocable`, `std::is_invocable_r`
[note ref-qualified overloads of `operator()` with different signatures are not handled correctly yet.]

[heading Example Program]
[import ../example/is_invocable.cpp]
[is_invocable]
[endsect]
*/
//]

#endif // #ifndef BOOST_CLBL_TRTS_IS_INVOCABLE_HPP
