/*

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_class_of_HPP
#define BOOST_CLBL_TRTS_class_of_HPP

#include <boost/callable_traits/detail/core.hpp>

namespace boost { namespace callable_traits {

//[ class_of_hpp
/*`
[section:ref_class_of class_of]
[heading Header]
``#include <boost/callable_traits/class_of.hpp>``
[heading Definition]
*/

template<typename T>
using class_of_t = //see below
//<-
    detail::try_but_fail_if_invalid<
        typename detail::traits<detail::shallow_decay<T>>::class_type,
        type_is_not_a_member_pointer>;

namespace detail {

    template<typename T, typename = std::false_type>
    struct class_of_impl {};

    template<typename T>
    struct class_of_impl <T, typename std::is_same<
        class_of_t<T>, detail::dummy>::type>
    {
        using type = class_of_t<T>;
    };
}

//->

template<typename T>
struct class_of : detail::class_of_impl<T> {};

//<-
}} // namespace boost::callable_traits
//->

/*`
[heading Constraints]
* `T` must be a member pointer

[heading Behavior]
* A substitution failure occurs if the constraints are violated.
* The aliased type is the parent class of the member. In other words, if `T` is expanded to `U C::*`, the aliased type is `C`.

[heading Input/Output Examples]
[table
    [[`T`]                              [`class_of_t<T>`]]
    [[`int foo::*`]                     [`foo`]]
    [[`void(foo::* const &)() const`]           [`foo`]]
]

[heading Example Program]
[import ../example/class_of.cpp]
[class_of]
[endsect]
*/
//]

#endif // #ifndef BOOST_CLBL_TRTS_class_of_HPP
