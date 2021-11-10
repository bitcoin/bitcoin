/*

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_HAS_VOID_RETURN_HPP
#define BOOST_CLBL_TRTS_HAS_VOID_RETURN_HPP

#include <boost/callable_traits/detail/core.hpp>

namespace boost { namespace callable_traits {

//[ has_void_return_hpp
/*`[section:ref_has_void_return has_void_return]
[heading Header]
``#include <boost/callable_traits/has_void_return.hpp>``
[heading Definition]
*/

// inherits from either std::true_type or std::false_type
template<typename T>
struct has_void_return;

//<-
template<typename T>
struct has_void_return
    : std::is_same<typename detail::traits<
        detail::shallow_decay<T>>::return_type, void> {};

#ifdef BOOST_CLBL_TRTS_DISABLE_VARIABLE_TEMPLATES

template<typename T>
struct has_void_return_v {
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
constexpr bool has_void_return_v = //see below
//<-
    std::is_same<typename detail::traits<
        detail::shallow_decay<T>>::return_type, void>::value;

#endif

}} // namespace boost::callable_traits
//->


/*`
[heading Constraints]
* none

[heading Behavior]
* `std::false_type` is inherited by `has_void_return<T>` and is aliased by `typename has_void_return<T>::type`, except when one of the following criteria is met, in which case `std::true_type` would be similarly inherited and aliased:
  * `T` is a function, function pointer, or function reference where the function's return type is `void`.
  * `T` is a pointer to a member function whose return type is `void`.
  * `T` is a function object with a non-overloaded `operator()`, where the `operator()` function returns `void`.
* On compilers that support variable templates, `has_void_return_v<T>` is equivalent to `has_void_return<T>::value`.

[heading Input/Output Examples]
[table
    [[`T`]                              [`has_void_return_v<T>`]]
    [[`void()`]                         [`true`]]
    [[`void(int) const`]                [`true`]]
    [[`void(* const &)()`]              [`true`]]
    [[`void(&)()`]                      [`true`]]
    [[`void(foo::*)() const`]           [`true`]]
    [[`int(*)()`]                       [`false`]]
    [[`int(*&)()`]                      [`false`]]
    [[`int`]                            [`false`]]
    [[`int foo::*`]                     [`false`]]
    [[`void* foo::*`]                   [`false`]]
]

[heading Example Program]
[import ../example/has_void_return.cpp]
[has_void_return]
[endsect]
*/
//]

#endif
