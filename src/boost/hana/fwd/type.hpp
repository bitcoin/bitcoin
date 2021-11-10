/*!
@file
Forward declares `boost::hana::type` and related utilities.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_TYPE_HPP
#define BOOST_HANA_FWD_TYPE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/core/make.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Base class of `hana::type`; used for pattern-matching.
    //! @relates hana::type
    //!
    //! Example
    //! -------
    //! @include example/type/basic_type.cpp
    template <typename T>
    struct basic_type;

    //! @ingroup group-datatypes
    //! C++ type in value-level representation.
    //!
    //! A `type` is a special kind of object representing a C++ type like
    //! `int`, `void`, `std::vector<float>` or anything else you can imagine.
    //!
    //! This page explains how `type`s work at a low level. To gain
    //! intuition about type-level metaprogramming in Hana, you should
    //! read the [tutorial section](@ref tutorial-type) on type-level
    //! computations.
    //!
    //!
    //! @note
    //! For subtle reasons, the actual representation of `hana::type` is
    //! implementation-defined. In particular, `hana::type` may be a dependent
    //! type, so one should not attempt to do pattern matching on it. However,
    //! one can assume that `hana::type` _inherits_ from `hana::basic_type`,
    //! which can be useful when declaring overloaded functions:
    //! @code
    //!     template <typename T>
    //!     void f(hana::basic_type<T>) {
    //!         // do something with T
    //!     }
    //! @endcode
    //! The full story is that [ADL][] causes template arguments to be
    //! instantiated. Hence, if `hana::type` were defined naively, expressions
    //! like `hana::type<T>{} == hana::type<U>{}` would cause both `T` and `U`
    //! to be instantiated. This is usually not a problem, except when `T` or
    //! `U` should not be instantiated. To avoid these instantiations,
    //! `hana::type` is implemented using some cleverness, and that is
    //! why the representation is implementation-defined. When that
    //! behavior is not required, `hana::basic_type` can be used instead.
    //!
    //!
    //! @anchor type_lvalues_and_rvalues
    //! Lvalues and rvalues
    //! -------------------
    //! When storing `type`s in heterogeneous containers, some algorithms
    //! will return references to those objects. Since we are primarily
    //! interested in accessing their nested `::%type`, receiving a reference
    //! is undesirable; we would end up trying to fetch the nested `::%type`
    //! inside a reference type, which is a compilation error:
    //! @code
    //!   auto ts = make_tuple(type_c<int>, type_c<char>);
    //!   using T = decltype(ts[0_c])::type; // error: 'ts[0_c]' is a reference!
    //! @endcode
    //!
    //! For this reason, `type`s provide an overload of the unary `+`
    //! operator that can be used to turn a lvalue into a rvalue. So when
    //! using a result which might be a reference to a `type` object, one
    //! can use `+` to make sure a rvalue is obtained before fetching its
    //! nested `::%type`:
    //! @code
    //!   auto ts = make_tuple(type_c<int>, type_c<char>);
    //!   using T = decltype(+ts[0_c])::type; // ok: '+ts[0_c]' is an rvalue
    //! @endcode
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! 1. `Comparable`\n
    //! Two types are equal if and only if they represent the same C++ type.
    //! Hence, equality is equivalent to the `std::is_same` type trait.
    //! @include example/type/comparable.cpp
    //!
    //! 2. `Hashable`\n
    //! The hash of a type is just that type itself. In other words, `hash`
    //! is the identity function on `hana::type`s.
    //! @include example/type/hashable.cpp
    //!
    //! [ADL]: http://en.cppreference.com/w/cpp/language/adl
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename T>
    struct type {
        //! Returns rvalue of self.
        //! See @ref type_lvalues_and_rvalues "description".
        constexpr auto operator+() const;

        //! Equivalent to `hana::equal`
        template <typename X, typename Y>
        friend constexpr auto operator==(X&& x, Y&& y);

        //! Equivalent to `hana::not_equal`
        template <typename X, typename Y>
        friend constexpr auto operator!=(X&& x, Y&& y);
    };
#else
    template <typename T>
    struct type_impl;

    template <typename T>
    using type = typename type_impl<T>::_;
#endif

    //! Tag representing `hana::type`.
    //! @relates hana::type
    struct type_tag { };

    //! Creates an object representing the C++ type `T`.
    //! @relates hana::type
    template <typename T>
    BOOST_HANA_INLINE_VARIABLE constexpr type<T> type_c{};

    //! `decltype` keyword, lifted to Hana.
    //! @relates hana::type
    //!
    //! @deprecated
    //! The semantics of `decltype_` can be confusing, and `hana::typeid_`
    //! should be preferred instead. `decltype_` may be removed in the next
    //! major version of the library.
    //!
    //! `decltype_` is somewhat equivalent to `decltype` in that it returns
    //! the type of an object, except it returns it as a `hana::type` which
    //! is a first-class citizen of Hana instead of a raw C++ type.
    //! Specifically, given an object `x`, `decltype_` satisfies
    //! @code
    //!   decltype_(x) == type_c<decltype(x) with references stripped>
    //! @endcode
    //!
    //! As you can see, `decltype_` will strip any reference from the
    //! object's actual type. The reason for doing so is explained below.
    //! However, any `cv`-qualifiers will be retained. Also, when given a
    //! `hana::type`, `decltype_` is just the identity function. Hence,
    //! for any C++ type `T`,
    //! @code
    //!   decltype_(type_c<T>) == type_c<T>
    //! @endcode
    //!
    //! In conjunction with the way `metafunction` & al. are specified, this
    //! behavior makes it easier to interact with both types and values at
    //! the same time. However, it does make it impossible to create a `type`
    //! containing another `type` with `decltype_`. In other words, it is
    //! not possible to create a `type_c<decltype(type_c<T>)>` with this
    //! utility, because `decltype_(type_c<T>)` would be just `type_c<T>`
    //! instead of `type_c<decltype(type_c<T>)>`. This use case is assumed
    //! to be rare and a hand-coded function can be used if this is needed.
    //!
    //!
    //! ### Rationale for stripping the references
    //! The rules for template argument deduction are such that a perfect
    //! solution that always matches `decltype` is impossible. Hence, we
    //! have to settle on a solution that's good and and consistent enough
    //! for our needs. One case where matching `decltype`'s behavior is
    //! impossible is when the argument is a plain, unparenthesized variable
    //! or function parameter. In that case, `decltype_`'s argument will be
    //! deduced as a reference to that variable, but `decltype` would have
    //! given us the actual type of that variable, without references. Also,
    //! given the current definition of `metafunction` & al., it would be
    //! mostly useless if `decltype_` could return a reference, because it
    //! is unlikely that `F` expects a reference in its simplest use case:
    //! @code
    //!   int i = 0;
    //!   auto result = metafunction<F>(i);
    //! @endcode
    //!
    //! Hence, always discarding references seems to be the least painful
    //! solution.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/decltype.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto decltype_ = see documentation;
#else
    struct decltype_t {
        template <typename T>
        constexpr auto operator()(T&&) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr decltype_t decltype_{};
#endif

    //! Returns a `hana::type` representing the type of a given object.
    //! @relates hana::type
    //!
    //! `hana::typeid_` is somewhat similar to `typeid` in that it returns
    //! something that represents the type of an object. However, what
    //! `typeid` returns represent the _runtime_ type of the object, while
    //! `hana::typeid_` returns the _static_ type of the object. Specifically,
    //! given an object `x`, `typeid_` satisfies
    //! @code
    //!   typeid_(x) == type_c<decltype(x) with ref and cv-qualifiers stripped>
    //! @endcode
    //!
    //! As you can see, `typeid_` strips any reference and cv-qualifier from
    //! the object's actual type. The reason for doing so is that it faithfully
    //! models how the language's `typeid` behaves with respect to reference
    //! and cv-qualifiers, and it also turns out to be the desirable behavior
    //! most of the time. Also, when given a `hana::type`, `typeid_` is just
    //! the identity function. Hence, for any C++ type `T`,
    //! @code
    //!   typeid_(type_c<T>) == type_c<T>
    //! @endcode
    //!
    //! In conjunction with the way `metafunction` & al. are specified, this
    //! behavior makes it easier to interact with both types and values at
    //! the same time. However, it does make it impossible to create a `type`
    //! containing another `type` using `typeid_`. This use case is assumed
    //! to be rare and a hand-coded function can be used if this is needed.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/typeid.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto typeid_ = see documentation;
#else
    struct typeid_t {
        template <typename T>
        constexpr auto operator()(T&&) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr typeid_t typeid_{};
#endif

#ifdef BOOST_HANA_DOXYGEN_INVOKED
    //! Equivalent to `decltype_`, provided for convenience.
    //! @relates hana::type
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/make.cpp
    template <>
    constexpr auto make<type_tag> = hana::decltype_;
#endif

    //! Equivalent to `make<type_tag>`, provided for convenience.
    //! @relates hana::type
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/make.cpp
    BOOST_HANA_INLINE_VARIABLE constexpr auto make_type = hana::make<type_tag>;

    //! `sizeof` keyword, lifted to Hana.
    //! @relates hana::type
    //!
    //! `sizeof_` is somewhat equivalent to `sizeof` in that it returns the
    //! size of an expression or type, but it takes an arbitrary expression
    //! or a `hana::type` and returns its size as an `integral_constant`.
    //! Specifically, given an expression `expr`, `sizeof_` satisfies
    //! @code
    //!   sizeof_(expr) == size_t<sizeof(decltype(expr) with references stripped)>
    //! @endcode
    //!
    //! However, given a `type`, `sizeof_` will simply fetch the size
    //! of the C++ type represented by that object. In other words,
    //! @code
    //!   sizeof_(type_c<T>) == size_t<sizeof(T)>
    //! @endcode
    //!
    //! The behavior of `sizeof_` is consistent with that of `decltype_`.
    //! In particular, see `decltype_`'s documentation to understand why
    //! references are always stripped by `sizeof_`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/sizeof.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto sizeof_ = [](auto&& x) {
        using T = typename decltype(hana::decltype_(x))::type;
        return hana::size_c<sizeof(T)>;
    };
#else
    struct sizeof_t {
        template <typename T>
        constexpr auto operator()(T&&) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr sizeof_t sizeof_{};
#endif

    //! `alignof` keyword, lifted to Hana.
    //! @relates hana::type
    //!
    //! `alignof_` is somewhat equivalent to `alignof` in that it returns the
    //! alignment required by any instance of a type, but it takes a `type`
    //! and returns its alignment as an `integral_constant`. Like `sizeof`
    //! which works for expressions and type-ids, `alignof_` can also be
    //! called on an arbitrary expression. Specifically, given an expression
    //! `expr` and a C++ type `T`, `alignof_` satisfies
    //! @code
    //!   alignof_(expr) == size_t<alignof(decltype(expr) with references stripped)>
    //!   alignof_(type_c<T>) == size_t<alignof(T)>
    //! @endcode
    //!
    //! The behavior of `alignof_` is consistent with that of `decltype_`.
    //! In particular, see `decltype_`'s documentation to understand why
    //! references are always stripped by `alignof_`.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/alignof.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto alignof_ = [](auto&& x) {
        using T = typename decltype(hana::decltype_(x))::type;
        return hana::size_c<alignof(T)>;
    };
#else
    struct alignof_t {
        template <typename T>
        constexpr auto operator()(T&&) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr alignof_t alignof_{};
#endif

    //! Checks whether a SFINAE-friendly expression is valid.
    //! @relates hana::type
    //!
    //! Given a SFINAE-friendly function, `is_valid` returns whether the
    //! function call is valid with the given arguments. Specifically, given
    //! a function `f` and arguments `args...`,
    //! @code
    //!   is_valid(f, args...) == whether f(args...) is valid
    //! @endcode
    //!
    //! The result is returned as a compile-time `Logical`. Furthermore,
    //! `is_valid` can be used in curried form as follows:
    //! @code
    //!   is_valid(f)(args...)
    //! @endcode
    //!
    //! This syntax makes it easy to create functions that check the validity
    //! of a generic expression on any given argument(s).
    //!
    //! @warning
    //! To check whether calling a nullary function `f` is valid, one should
    //! use the `is_valid(f)()` syntax. Indeed, `is_valid(f /* no args */)`
    //! will be interpreted as the currying of `is_valid` to `f` rather than
    //! the application of `is_valid` to `f` and no arguments.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/is_valid.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto is_valid = [](auto&& f) {
        return [](auto&& ...args) {
            return whether f(args...) is a valid expression;
        };
    };
#else
    struct is_valid_t {
        template <typename F>
        constexpr auto operator()(F&&) const;

        template <typename F, typename ...Args>
        constexpr auto operator()(F&&, Args&&...) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr is_valid_t is_valid{};
#endif

    //! Lift a template to a Metafunction.
    //! @ingroup group-Metafunction
    //!
    //! Given a template class or template alias `f`, `template_<f>` is a
    //! `Metafunction` satisfying
    //! @code
    //!     template_<f>(type_c<x>...) == type_c<f<x...>>
    //!     decltype(template_<f>)::apply<x...>::type == f<x...>
    //! @endcode
    //!
    //! @note
    //! In a SFINAE context, the expression `template_<f>(type_c<x>...)` is
    //! valid whenever the expression `f<x...>` is valid.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/template.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <template <typename ...> class F>
    constexpr auto template_ = [](basic_type<T>...) {
        return hana::type_c<F<T...>>;
    };
#else
    template <template <typename ...> class F>
    struct template_t;

    template <template <typename ...> class F>
    BOOST_HANA_INLINE_VARIABLE constexpr template_t<F> template_{};
#endif

    //! Lift a MPL-style metafunction to a Metafunction.
    //! @ingroup group-Metafunction
    //!
    //! Given a MPL-style metafunction, `metafunction<f>` is a `Metafunction`
    //! satisfying
    //! @code
    //!     metafunction<f>(type_c<x>...) == type_c<f<x...>::type>
    //!     decltype(metafunction<f>)::apply<x...>::type == f<x...>::type
    //! @endcode
    //!
    //! @note
    //! In a SFINAE context, the expression `metafunction<f>(type_c<x>...)` is
    //! valid whenever the expression `f<x...>::%type` is valid.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/metafunction.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <template <typename ...> class F>
    constexpr auto metafunction = [](basic_type<T>...) {
        return hana::type_c<typename F<T...>::type>;
    };
#else
    template <template <typename ...> class f>
    struct metafunction_t;

    template <template <typename ...> class f>
    BOOST_HANA_INLINE_VARIABLE constexpr metafunction_t<f> metafunction{};
#endif

    //! Lift a MPL-style metafunction class to a Metafunction.
    //! @ingroup group-Metafunction
    //!
    //! Given a MPL-style metafunction class, `metafunction_class<f>` is a
    //! `Metafunction` satisfying
    //! @code
    //!     metafunction_class<f>(type_c<x>...) == type_c<f::apply<x...>::type>
    //!     decltype(metafunction_class<f>)::apply<x...>::type == f::apply<x...>::type
    //! @endcode
    //!
    //! @note
    //! In a SFINAE context, the expression `metafunction_class<f>(type_c<x>...)`
    //! is valid whenever the expression `f::apply<x...>::%type` is valid.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/metafunction_class.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename F>
    constexpr auto metafunction_class = [](basic_type<T>...) {
        return hana::type_c<typename F::template apply<T...>::type>;
    };
#else
    template <typename F>
    struct metafunction_class_t;

    template <typename F>
    BOOST_HANA_INLINE_VARIABLE constexpr metafunction_class_t<F> metafunction_class{};
#endif

    //! Turn a `Metafunction` into a function taking `type`s and returning a
    //! default-constructed object.
    //! @ingroup group-Metafunction
    //!
    //! Given a `Metafunction` `f`, `integral` returns a new `Metafunction`
    //! that default-constructs an object of the type returned by `f`. More
    //! specifically, the following holds:
    //! @code
    //!     integral(f)(t...) == decltype(f(t...))::type{}
    //! @endcode
    //!
    //! The principal use case for `integral` is to transform `Metafunction`s
    //! returning a type that inherits from a meaningful base like
    //! `std::integral_constant` into functions returning e.g. a
    //! `hana::integral_constant`.
    //!
    //! @note
    //! - This is not a `Metafunction` because it does not return a `type`.
    //!   As such, it would not make sense to make `decltype(integral(f))`
    //!   a MPL metafunction class like the usual `Metafunction`s are.
    //!
    //! - When using `integral` with metafunctions returning
    //!   `std::integral_constant`s, don't forget to include the
    //!   boost/hana/ext/std/integral_constant.hpp header to ensure
    //!   Hana can interoperate with the result.
    //!
    //! - In a SFINAE context, the expression `integral(f)(t...)` is valid
    //!   whenever the expression `decltype(f(t...))::%type` is valid.
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/integral.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto integral = [](auto f) {
        return [](basic_type<T>...) {
            return decltype(f)::apply<T...>::type{};
        };
    };
#else
    template <typename F>
    struct integral_t;

    struct make_integral_t {
        template <typename F>
        constexpr integral_t<F> operator()(F const&) const
        { return {}; }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr make_integral_t integral{};
#endif

    //! Alias to `integral(metafunction<F>)`, provided for convenience.
    //! @ingroup group-Metafunction
    //!
    //!
    //! Example
    //! -------
    //! @include example/type/trait.cpp
    template <template <typename ...> class F>
    BOOST_HANA_INLINE_VARIABLE constexpr auto trait = hana::integral(hana::metafunction<F>);
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_TYPE_HPP
