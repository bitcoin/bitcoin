/*!
@file
Forward declares `boost::hana::tuple`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_TUPLE_HPP
#define BOOST_HANA_FWD_TUPLE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/fwd/core/make.hpp>
#include <boost/hana/fwd/core/to.hpp>
#include <boost/hana/fwd/integral_constant.hpp>
#include <boost/hana/fwd/type.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-datatypes
    //! General purpose index-based heterogeneous sequence with a fixed length.
    //!
    //! The tuple is the bread and butter for static metaprogramming.
    //! Conceptually, it is like a `std::tuple`; it is a container able
    //! of holding objects of different types and whose size is fixed at
    //! compile-time. However, Hana's tuple provides much more functionality
    //! than its `std` counterpart, and it is also much more efficient than
    //! all standard library implementations tested so far.
    //!
    //! Tuples are index-based sequences. If you need an associative
    //! sequence with a key-based access, then you should consider
    //! `hana::map` or `hana::set` instead.
    //!
    //! @note
    //! When you use a container, remember not to make assumptions about its
    //! representation, unless the documentation gives you those guarantees.
    //! More details [in the tutorial](@ref tutorial-containers-types).
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! `Sequence`, and all the concepts it refines
    //!
    //!
    //! Provided operators
    //! ------------------
    //! For convenience, the following operators are provided:
    //! @code
    //!     xs == ys        ->          equal(xs, ys)
    //!     xs != ys        ->          not_equal(xs, ys)
    //!
    //!     xs < ys         ->          less(xs, ys)
    //!     xs <= ys        ->          less_equal(xs, ys)
    //!     xs > ys         ->          greater(xs, ys)
    //!     xs >= ys        ->          greater_equal(xs, ys)
    //!
    //!     xs | f          ->          chain(xs, f)
    //!
    //!     xs[n]           ->          at(xs, n)
    //! @endcode
    //!
    //!
    //! Example
    //! -------
    //! @include example/tuple/tuple.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename ...Xn>
    struct tuple {
        //! Default constructs the `tuple`. Only exists when all the elements
        //! of the tuple are default constructible.
        constexpr tuple();

        //! Initialize each element of the tuple with the corresponding element
        //! from `xn...`. Only exists when all the elements of the tuple are
        //! copy-constructible.
        //!
        //! @note
        //! Unlike the corresponding constructor for `std::tuple`, this
        //! constructor is not explicit. This allows returning a tuple
        //! from a function with the brace-initialization syntax.
        constexpr tuple(Xn const& ...xn);

        //! Initialize each element of the tuple by perfect-forwarding the
        //! corresponding element in `yn...`. Only exists when all the
        //! elements of the created tuple are constructible from the
        //! corresponding perfect-forwarded value.
        //!
        //! @note
        //! Unlike the corresponding constructor for `std::tuple`, this
        //! constructor is not explicit. This allows returning a tuple
        //! from a function with the brace-initialization syntax.
        template <typename ...Yn>
        constexpr tuple(Yn&& ...yn);

        //! Copy-initialize a tuple from another tuple. Only exists when all
        //! the elements of the constructed tuple are copy-constructible from
        //! the corresponding element in the source tuple.
        template <typename ...Yn>
        constexpr tuple(tuple<Yn...> const& other);

        //! Move-initialize a tuple from another tuple. Only exists when all
        //! the elements of the constructed tuple are move-constructible from
        //! the corresponding element in the source tuple.
        template <typename ...Yn>
        constexpr tuple(tuple<Yn...>&& other);

        //! Assign a tuple to another tuple. Only exists when all the elements
        //! of the destination tuple are assignable from the corresponding
        //! element in the source tuple.
        template <typename ...Yn>
        constexpr tuple& operator=(tuple<Yn...> const& other);

        //! Move-assign a tuple to another tuple. Only exists when all the
        //! elements of the destination tuple are move-assignable from the
        //! corresponding element in the source tuple.
        template <typename ...Yn>
        constexpr tuple& operator=(tuple<Yn...>&& other);

        //! Equivalent to `hana::chain`.
        template <typename ...T, typename F>
        friend constexpr auto operator|(tuple<T...>, F);

        //! Equivalent to `hana::equal`
        template <typename X, typename Y>
        friend constexpr auto operator==(X&& x, Y&& y);

        //! Equivalent to `hana::not_equal`
        template <typename X, typename Y>
        friend constexpr auto operator!=(X&& x, Y&& y);

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

        //! Equivalent to `hana::at`
        template <typename N>
        constexpr decltype(auto) operator[](N&& n);
    };
#else
    template <typename ...Xn>
    struct tuple;
#endif

    //! Tag representing `hana::tuple`s.
    //! @related tuple
    struct tuple_tag { };

#ifdef BOOST_HANA_DOXYGEN_INVOKED
    //! Function object for creating a `tuple`.
    //! @relates hana::tuple
    //!
    //! Given zero or more objects `xs...`, `make<tuple_tag>` returns a new tuple
    //! containing those objects. The elements are held by value inside the
    //! resulting tuple, and they are hence copied or moved in. This is
    //! analogous to `std::make_tuple` for creating Hana tuples.
    //!
    //!
    //! Example
    //! -------
    //! @include example/tuple/make.cpp
    template <>
    constexpr auto make<tuple_tag> = [](auto&& ...xs) {
        return tuple<std::decay_t<decltype(xs)>...>{forwarded(xs)...};
    };
#endif

    //! Alias to `make<tuple_tag>`; provided for convenience.
    //! @relates hana::tuple
    BOOST_HANA_INLINE_VARIABLE constexpr auto make_tuple = make<tuple_tag>;

    //! Equivalent to `to<tuple_tag>`; provided for convenience.
    //! @relates hana::tuple
    BOOST_HANA_INLINE_VARIABLE constexpr auto to_tuple = to<tuple_tag>;

    //! Create a tuple specialized for holding `hana::type`s.
    //! @relates hana::tuple
    //!
    //! This is functionally equivalent to `make<tuple_tag>(type_c<T>...)`, except
    //! that using `tuple_t` allows the library to perform some compile-time
    //! optimizations. Also note that the type of the objects returned by
    //! `tuple_t` and an equivalent call to `make<tuple_tag>` may differ.
    //!
    //!
    //! Example
    //! -------
    //! @include example/tuple/tuple_t.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename ...T>
    constexpr implementation_defined tuple_t{};
#else
    template <typename ...T>
    BOOST_HANA_INLINE_VARIABLE constexpr hana::tuple<hana::type<T>...> tuple_t{};
#endif

    //! Create a tuple specialized for holding `hana::integral_constant`s.
    //! @relates hana::tuple
    //!
    //! This is functionally equivalent to `make<tuple_tag>(integral_c<T, v>...)`,
    //! except that using `tuple_c` allows the library to perform some
    //! compile-time optimizations. Also note that the type of the objects
    //! returned by `tuple_c` and an equivalent call to `make<tuple_tag>` may differ.
    //!
    //!
    //! Example
    //! -------
    //! @include example/tuple/tuple_c.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename T, T ...v>
    constexpr implementation_defined tuple_c{};
#else
    template <typename T, T ...v>
    BOOST_HANA_INLINE_VARIABLE constexpr hana::tuple<hana::integral_constant<T, v>...> tuple_c{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_TUPLE_HPP
