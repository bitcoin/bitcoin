/*!
@file
Defines the barebones `boost::hana::integral_constant` template, but no
operations on it.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_INTEGRAL_CONSTANT_HPP
#define BOOST_HANA_DETAIL_INTEGRAL_CONSTANT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/operators/adl.hpp>

#include <type_traits>


BOOST_HANA_NAMESPACE_BEGIN
    //! Tag representing `hana::integral_constant`.
    //! @relates hana::integral_constant
    template <typename T>
    struct integral_constant_tag {
        using value_type = T;
    };

    namespace ic_detail {
        template <typename T, T v>
        struct with_index_t {
            template <typename F>
            constexpr void operator()(F&& f) const;
        };

        template <typename T, T v>
        struct times_t {
            static constexpr with_index_t<T, v> with_index{};

            template <typename F>
            constexpr void operator()(F&& f) const;
        };
    }

    //! @ingroup group-datatypes
    //! Compile-time value of an integral type.
    //!
    //! An `integral_constant` is an object that represents a compile-time
    //! integral value. As the name suggests, `hana::integral_constant` is
    //! basically equivalent to `std::integral_constant`, except that
    //! `hana::integral_constant` also provide other goodies to make them
    //! easier to use, like arithmetic operators and similar features. In
    //! particular, `hana::integral_constant` is guaranteed to inherit from
    //! the corresponding `std::integral_constant`, and hence have the same
    //! members and capabilities. The sections below explain the extensions
    //! to `std::integral_constant` provided by `hana::integral_constant`.
    //!
    //!
    //! Arithmetic operators
    //! --------------------
    //! `hana::integral_constant` provides arithmetic operators that return
    //! `hana::integral_constant`s to ease writing compile-time arithmetic:
    //! @snippet example/integral_constant.cpp operators
    //!
    //! It is pretty important to realize that these operators return other
    //! `integral_constant`s, not normal values of an integral type.
    //! Actually, all those operators work pretty much in the same way.
    //! Simply put, for an operator `@`,
    //! @code
    //!     integral_constant<T, x>{} @ integral_constant<T, y>{} == integral_constant<T, x @ y>{}
    //! @endcode
    //!
    //! The fact that the operators return `Constant`s is very important
    //! because it allows all the information that's known at compile-time
    //! to be conserved as long as it's only used with other values known at
    //! compile-time. It is also interesting to observe that whenever an
    //! `integral_constant` is combined with a normal runtime value, the
    //! result will be a runtime value (because of the implicit conversion).
    //! In general, this gives us the following table
    //!
    //! left operand        | right operand       | result
    //! :-----------------: | :-----------------: | :-----------------:
    //! `integral_constant` | `integral_constant` | `integral_constant`
    //! `integral_constant` | runtime             | runtime
    //! runtime             | `integral_constant` | runtime
    //! runtime             | runtime             | runtime
    //!
    //! The full range of provided operators is
    //! - Arithmetic: binary `+`, binary `-`, `/`, `*`, `%`, unary `+`, unary `-`
    //! - Bitwise: `~`, `&`, `|`, `^`, `<<`, `>>`
    //! - Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
    //! - %Logical: `||`, `&&`, `!`
    //!
    //!
    //! Construction with user-defined literals
    //! ---------------------------------------
    //! `integral_constant`s of type `long long` can be created with the
    //! `_c` user-defined literal, which is contained in the `literals`
    //! namespace:
    //! @snippet example/integral_constant.cpp literals
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! 1. `Constant` and `IntegralConstant`\n
    //! An `integral_constant` is a model of the `IntegralConstant` concept in
    //! the most obvious way possible. Specifically,
    //! @code
    //!     integral_constant<T, v>::value == v // of type T
    //! @endcode
    //! The model of `Constant` follows naturally from the model of `IntegralConstant`, i.e.
    //! @code
    //!     value<integral_constant<T, v>>() == v // of type T
    //! @endcode
    //!
    //! 2. `Comparable`, `Orderable`, `Logical`, `Monoid`, `Group`, `Ring`, and `EuclideanRing`, `Hashable`\n
    //! Those models are exactly those provided for `Constant`s, which are
    //! documented in their respective concepts.
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename T, T v>
    struct integral_constant {
        //! Call a function n times.
        //!
        //! `times` allows a nullary function to be invoked `n` times:
        //! @code
        //!     int_<3>::times(f)
        //! @endcode
        //! should be expanded by any decent compiler to
        //! @code
        //!     f(); f(); f();
        //! @endcode
        //!
        //! This can be useful in several contexts, e.g. for loop unrolling:
        //! @snippet example/integral_constant.cpp times_loop_unrolling
        //!
        //! Note that `times` is really a static function object, not just a
        //! static function. This allows `int_<n>::%times` to be passed to
        //! higher-order algorithms:
        //! @snippet example/integral_constant.cpp times_higher_order
        //!
        //! Also, since static members can be accessed using both the `.` and
        //! the `::` syntax, one can take advantage of this (loophole?) to
        //! call `times` on objects just as well as on types:
        //! @snippet example/integral_constant.cpp from_object
        //!
        //! @note
        //! `times` is equivalent to the `hana::repeat` function, which works
        //! on an arbitrary `IntegralConstant`.
        //!
        //! Sometimes, it is also useful to know the index we're at inside the
        //! function. This can be achieved by using `times.with_index`:
        //! @snippet example/integral_constant.cpp times_with_index_runtime
        //!
        //! Remember that `times` is a _function object_, and hence it can
        //! have subobjects. `with_index` is just a function object nested
        //! inside `times`, which allows for this nice little interface. Also
        //! note that the indices passed to the function are `integral_constant`s;
        //! they are known at compile-time. Hence, we can do compile-time stuff
        //! with them, like indexing inside a tuple:
        //! @snippet example/integral_constant.cpp times_with_index_compile_time
        //!
        //! @note
        //! `times.with_index(f)` guarantees that the calls to `f` will be
        //! done in order of ascending index. In other words, `f` will be
        //! called as `f(0)`, `f(1)`, `f(2)`, etc., but with `integral_constant`s
        //! instead of normal integers. Side effects can also be done in the
        //! function passed to `times` and `times.with_index`.
        template <typename F>
        static constexpr void times(F&& f) {
            f(); f(); ... f(); // n times total
        }

        //! Equivalent to `hana::plus`
        template <typename X, typename Y>
        friend constexpr auto operator+(X&& x, Y&& y);

        //! Equivalent to `hana::minus`
        template <typename X, typename Y>
        friend constexpr auto operator-(X&& x, Y&& y);

        //! Equivalent to `hana::negate`
        template <typename X>
        friend constexpr auto operator-(X&& x);

        //! Equivalent to `hana::mult`
        template <typename X, typename Y>
        friend constexpr auto operator*(X&& x, Y&& y);

        //! Equivalent to `hana::div`
        template <typename X, typename Y>
        friend constexpr auto operator/(X&& x, Y&& y);

        //! Equivalent to `hana::mod`
        template <typename X, typename Y>
        friend constexpr auto operator%(X&& x, Y&& y);

        //! Equivalent to `hana::equal`
        template <typename X, typename Y>
        friend constexpr auto operator==(X&& x, Y&& y);

        //! Equivalent to `hana::not_equal`
        template <typename X, typename Y>
        friend constexpr auto operator!=(X&& x, Y&& y);

        //! Equivalent to `hana::or_`
        template <typename X, typename Y>
        friend constexpr auto operator||(X&& x, Y&& y);

        //! Equivalent to `hana::and_`
        template <typename X, typename Y>
        friend constexpr auto operator&&(X&& x, Y&& y);

        //! Equivalent to `hana::not_`
        template <typename X>
        friend constexpr auto operator!(X&& x);

        //! Equivalent to `hana::less`
        template <typename X, typename Y>
        friend constexpr auto operator<(X&& x, Y&& y);

        //! Equivalent to `hana::greater`
        template <typename X, typename Y>
        friend constexpr auto operator>(X&& x, Y&& y);

        //! Equivalent to `hana::less_equal`
        template <typename X, typename Y>
        friend constexpr auto operator<=(X&& x, Y&& y);

        //! Equivalent to `hana::greater_equal`
        template <typename X, typename Y>
        friend constexpr auto operator>=(X&& x, Y&& y);
    };
#else
    template <typename T, T v>
#ifdef BOOST_HANA_WORKAROUND_MSVC_EMPTYBASE
    struct __declspec(empty_bases) integral_constant
#else
    struct integral_constant
#endif
        : std::integral_constant<T, v>
        , detail::operators::adl<integral_constant<T, v>>
    {
        using type = integral_constant; // override std::integral_constant::type
        static constexpr ic_detail::times_t<T, v> times{};
        using hana_tag = integral_constant_tag<T>;
    };
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_INTEGRAL_CONSTANT_HPP
