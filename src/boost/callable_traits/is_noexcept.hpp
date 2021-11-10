/*
@file is_noexcept

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_IS_NOEXCEPT_HPP
#define BOOST_CLBL_TRTS_IS_NOEXCEPT_HPP

#include <boost/callable_traits/detail/core.hpp>

namespace boost { namespace callable_traits {

//[ is_noexcept_hpp
/*`[section:ref_is_noexcept is_noexcept]
[heading Header]
``#include <boost/callable_traits/is_noexcept.hpp>``
[heading Definition]
*/

// inherits from either std::true_type or std::false_type
template<typename T>
struct is_noexcept;

//<-
template<typename T>
struct is_noexcept : detail::traits<detail::shallow_decay<T>>::is_noexcept {
    using type = typename detail::traits<
        detail::shallow_decay<T>>::is_noexcept;
};

#ifdef BOOST_CLBL_TRTS_DISABLE_VARIABLE_TEMPLATES

template<typename T>
struct is_noexcept_v {
    static_assert(std::is_same<T, detail::dummy>::value,
        "Variable templates not supported on this compiler.");
};

#else
//->
// only available when variable templates are supported
template<typename T>
//<-
BOOST_CLBL_TRAITS_INLINE_VAR
//->
constexpr bool is_noexcept_v = //see below
//<-
    detail::traits<detail::shallow_decay<T>>::is_noexcept::value;

#endif

}} // namespace boost::callable_traits
//->

/*`
[heading Constraints]
* none
*
[heading Behavior]
* `is_noexcept<T>::value` is `true` when either: 
  * `T` is a function type, function pointer type, function reference type, or member function pointer type where the function has a `noexcept` specifier
  * `T` is a function object with a non-overloaded `operator()`, where the `operator()` has a `noexcept` specifier
* On compilers that support variable templates, `is_noexcept_v<T>` is equivalent to `is_noexcept<T>::value`.

[heading Input/Output Examples]
[table
    [[`T`]                              [`is_noexcept_v<T>`]]
    [[`int() const noexcept`]           [`true`]]
    [[`int(* const &)() noexcept`]      [`true`]]
    [[`int(&)() noexcept`]              [`true`]]
    [[`int(foo::*)() noexcept`]         [`true`]]
    [[`int() const`]                    [`false`]]
    [[`int() volatile`]                 [`false`]]
    [[`int(foo::*)() const`]            [`false`]]
    [[`int() const`]                    [`false`]]
    [[`int() volatile`]                 [`false`]]
    [[`int() &`]                        [`false`]]
    [[`int(*)()`]                       [`false`]]
    [[`int`]                            [`false`]]
    [[`int foo::*`]                     [`false`]]
    [[`const int foo::*`]               [`false`]]
]

[heading Example Program]
[import ../example/is_noexcept.cpp]
[is_noexcept]
[endsect]
*/
//]

#endif // #ifndef BOOST_CLBL_TRTS_IS_NOEXCEPT_HPP
