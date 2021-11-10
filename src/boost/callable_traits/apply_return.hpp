/*

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_APPLY_RETURN_HPP
#define BOOST_CLBL_TRTS_APPLY_RETURN_HPP

#include <boost/callable_traits/detail/core.hpp>

namespace boost { namespace callable_traits {

BOOST_CLBL_TRTS_DEFINE_SFINAE_ERROR_ORIGIN(apply_return)
BOOST_CLBL_TRTS_SFINAE_MSG(apply_return, invalid_types_for_apply_return)

namespace detail {

    template<typename T, typename R>
    struct apply_return_helper {
        using type = typename detail::traits<T>::template apply_return<R>;
    };

    //special case
    template<typename... Args, typename R>
    struct apply_return_helper<std::tuple<Args...>, R> {
        using type = R(Args...);
    };
}

//[ apply_return_hpp
/*`
[section:ref_apply_return apply_return]
[heading Header]
``#include <boost/callable_traits/apply_return.hpp>``
[heading Definition]
*/

template<typename T, typename R>
using apply_return_t = //see below
//<-
    detail::try_but_fail_if_invalid<
        typename detail::apply_return_helper<T, R>::type,
        invalid_types_for_apply_return>;

namespace detail {

    template<typename T, typename R, typename = std::false_type>
    struct apply_return_impl {};

    template<typename T, typename R>
    struct apply_return_impl <T, R, typename std::is_same<
        apply_return_t<T, R>, detail::dummy>::type>
    {
        using type = apply_return_t<T, R>;
    };
}
    //->

template<typename T, typename R>
struct apply_return : detail::apply_return_impl<T, R> {};

//<-
}} // namespace boost::callable_traits
//->

/*`
[heading Constraints]
* `T` must one of the following:
  * `std::tuple` template instantiation
  * function
  * function pointer
  * function reference
  * member function pointer
  * member data pointer
* If `T` is a pointer, it may not be cv/ref qualified

[heading Behavior]
* When `T` is `std::tuple<Args...>`, the aliased type is `R(Args...)`.
* When `T` is a function, function pointer, function reference, or member function pointer, the aliased type's return type is `R`, but is otherwise identical to `T`.
* When `T` is a member data pointer of class `foo` to a `U` type (such that `T` is `U foo::*`), the aliased type is `R foo::*`.

[heading Input/Output Examples]
[table
    [[`T`]                              [`apply_return_t<T, float>`]]
    [[`std::tuple<int, int>`]           [`float(int, int)`]]
    [[`int()`]                          [`float()`]]
    [[`int (&)()`]                      [`float(&)()`]]
    [[`int (*)()`]                      [`float(*)()`]]
    [[`int (*)(...)`]                   [`float(*)()`]]
    [[`int(foo::*)()`]                  [`float(foo::*)()`]]
    [[`int(foo::*)() &`]                [`float(foo::*)() &`]]
    [[`int(foo::*)() &&`]               [`float(foo::*)() &&`]]
    [[`int(foo::*)() const`]            [`float(foo::*)() const`]]
    [[`int(foo::*)() transaction_safe`] [`float(foo::*)() transaction_safe`]]
    [[`int foo::*`]                     [`float foo::*`]]
    [[`int`]                            [(substitution failure)]]
    [[`int (*const)()`]                 [(substitution failure)]]
]

[heading Example Program]
[/import ../example/apply_return.cpp]
[apply_return]
[endsect]
*/
//]
#endif
