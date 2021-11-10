/*

@Copyright Barrett Adair 2015-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

*/

#ifndef BOOST_CLBL_TRTS_ARGS_HPP
#define BOOST_CLBL_TRTS_ARGS_HPP

#include <boost/callable_traits/detail/core.hpp>

namespace boost { namespace callable_traits {

//[ args_hpp
/*`[section:ref_args args]
[heading Header]
``#include <boost/callable_traits/args.hpp>``
[heading Definition]
*/

template<typename T, template<class...> class Container = std::tuple>
using args_t = //see below
//<-
    detail::try_but_fail_if_invalid<
        typename detail::traits<
            detail::shallow_decay<T>>::template expand_args<Container>,
        cannot_expand_the_parameter_list_of_first_template_argument>;

namespace detail {

    template<typename T, template<class...> class Container,
        typename = std::false_type>
    struct args_impl {};

    template<typename T, template<class...> class Container>
    struct args_impl <T, Container, typename std::is_same<
        args_t<T, Container>, detail::dummy>::type>
    {
        using type = args_t<T, Container>;
    };
}

//->

template<typename T,
    template<class...> class Container = std::tuple>
struct args : detail::args_impl<T, Container> {};

//<-
}} // namespace boost::callable_traits
//->

/*`
[heading Constraints]
* `T` must be one of the following:
  * function
  * function pointer
  * function reference
  * member function pointer
  * member data pointer
  * user-defined type with a non-overloaded `operator()`
  * type of a non-generic lambda

[heading Behavior]
* When the constraints are violated, a substitution failure occurs.
* When `T` is a function, function pointer, or function reference, the aliased type is `Container` instantiated with the function's parameter types.
* When `T` is a function object, the aliased type is `Container` instantiated with the `T::operator()` parameter types.
* When `T` is a member function pointer, the aliased type is a `Container` instantiation, where the first type argument is a reference to the parent class of `T`, qualified according to the member qualifiers on `T`, such that the first type is equivalent to `boost::callable_traits::qualified_class_of_t<T>`. The subsequent type arguments, if any, are the parameter types of the member function.
* When `T` is a member data pointer, the aliased type is `Container` with a single element, which is a `const` reference to the parent class of `T`.

[heading Input/Output Examples]
[table
    [[`T`]                              [`args_t<T>`]]
    [[`void(float, char, int)`]         [`std::tuple<float, char, int>`]]
    [[`void(*)(float, char, int)`]      [`std::tuple<float, char, int`]]
    [[`void(&)(float, char, int)`]      [`std::tuple<float, char, int`]]
    [[`void(float, char, int) const &&`][`std::tuple<float, char, int>`]]
    [[`void(*)()`]                      [`std::tuple<>`]]
    [[`void(foo::* const &)(float, char, int)`] [`std::tuple<foo&, float, char, int>`]]
    [[`int(foo::*)(int) const`]         [`std::tuple<const foo&, int>`]]
    [[`void(foo::*)() volatile &&`]     [`std::tuple<volatile foo &&>`]]
    [[`int foo::*`]                     [`std::tuple<const foo&>`]]
    [[`const int foo::*`]               [`std::tuple<const foo&>`]]
    [[`int`]                            [(substitution failure)]]
    [[`int (*const)()`]                 [(substitution failure)]]
]

[heading Example Program]
[import ../example/args.cpp]
[args]
[endsect]
*/
//]

#endif // #ifndef BOOST_CLBL_TRTS_ARGS_HPP
