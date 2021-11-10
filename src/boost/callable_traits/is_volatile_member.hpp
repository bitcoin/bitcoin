/*
 *
@Copyright Barrett Adair 2015-2017

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_IS_VOLATILE_MEMBER_HPP
#define BOOST_CLBL_TRTS_IS_VOLATILE_MEMBER_HPP

#include <boost/callable_traits/detail/core.hpp>

namespace boost { namespace callable_traits {

//[ is_volatile_member_hpp
/*`[section:ref_is_volatile_member is_volatile_member]
[heading Header]
``#include <boost/callable_traits/is_volatile_member.hpp>``
[heading Definition]
*/


// inherits from either std::true_type or std::false_type
template<typename T>
struct is_volatile_member;

//<-
template<typename T>
struct is_volatile_member : detail::traits<
    detail::shallow_decay<T>>::is_volatile_member {

    using type = typename detail::traits<
        detail::shallow_decay<T>>::is_volatile_member;
};

#ifdef BOOST_CLBL_TRTS_DISABLE_VARIABLE_TEMPLATES

template<typename T>
struct is_volatile_member_v {
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
constexpr bool is_volatile_member_v = //see below
//<-
    detail::traits<detail::shallow_decay<T>>::is_volatile_member::value;

#endif

}} // namespace boost::callable_traits
//->


/*`
[heading Constraints]
* none

[heading Behavior]
* `is_volatile_member<T>::value` is `true` when either: 
  * `T` is a function type with a `volatile` member qualifier
  * `T` is a pointer to a member function with a `volatile` member qualifier
  * `T` is a function object with a non-overloaded `operator()`, where the `operator()` has a `volatile` member qualifier
* On compilers that support variable templates, `is_volatile_member_v<T>` is equivalent to `is_volatile_member<T>::value`.

[heading Input/Output Examples]
[table
    [[`T`]                              [`is_volatile_member_v<T>`]]
    [[`int() volatile`]                 [`true`]]
    [[`int() const volatile`]           [`true`]]
    [[`int() volatile &&`]              [`true`]]
    [[`int(foo::*)() volatile`]         [`true`]]
    [[`int(foo::* const)() volatile`]   [`true`]]
    [[`int(foo::*)() const volatile`]   [`true`]]
    [[`int(foo::*)() const volatile &&`][`true`]]
    [[`int()`]                          [`false`]]
    [[`int() const`]                    [`false`]]
    [[`int() &&`]                       [`false`]]
    [[`int(*)()`]                       [`false`]]
    [[`int`]                            [`false`]]
    [[`int foo::*`]                     [`false`]]
    [[`volatile int foo::*`]            [`false`]]
]

[heading Example Program]
[import ../example/is_volatile_member.cpp]
[is_volatile_member]
[endsect]
*/
//]

#endif // #ifndef BOOST_CLBL_TRTS_IS_VOLATILE_MEMBER_HPP
