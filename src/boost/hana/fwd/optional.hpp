/*!
@file
Forward declares `boost::hana::optional`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_OPTIONAL_HPP
#define BOOST_HANA_FWD_OPTIONAL_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/operators/adl.hpp>
#include <boost/hana/fwd/core/make.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-datatypes
    //! Optional value whose optional-ness is known at compile-time.
    //!
    //! An `optional` either contains a value (represented as `just(x)`), or
    //! it is empty (represented as `nothing`). In essence, `hana::optional`
    //! is pretty much like a `boost::optional` or the upcoming `std::optional`,
    //! except for the fact that whether a `hana::optional` is empty or not is
    //! known at compile-time. This can be particularly useful for returning
    //! from a function that might fail, but whose reason for failing is not
    //! important. Of course, whether the function will fail has to be known
    //! at compile-time.
    //!
    //! This is really an important difference between `hana::optional` and
    //! `std::optional`. Unlike `std::optional<T>{}` and `std::optional<T>{x}`
    //! who share the same type (`std::optional<T>`), `hana::just(x)` and
    //! `hana::nothing` do not share the same type, since the state of the
    //! optional has to be known at compile-time. Hence, whether a `hana::just`
    //! or a `hana::nothing` will be returned from a function has to be known
    //! at compile-time for the return type of that function to be computable
    //! by the compiler. This makes `hana::optional` well suited for static
    //! metaprogramming tasks, but very poor for anything dynamic.
    //!
    //! @note
    //! When you use a container, remember not to make assumptions about its
    //! representation, unless the documentation gives you those guarantees.
    //! More details [in the tutorial](@ref tutorial-containers-types).
    //!
    //!
    //! Interoperation with `type`s
    //! ---------------------------
    //! When a `just` contains an object of type `T` which is a `type`,
    //! it has a nested `::%type` alias equivalent to `T::%type`. `nothing`,
    //! however, never has a nested `::%type` alias. If `t` is a `type`,
    //! this allows `decltype(just(t))` to be seen as a nullary metafunction
    //! equivalent to `decltype(t)`. Along with the `sfinae` function,
    //! this allows `hana::optional` to interact seamlessly with
    //! SFINAE-friendly metafunctions.
    //! Example:
    //! @include example/optional/sfinae_friendly_metafunctions.cpp
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! 1. `Comparable`\n
    //! Two `optional`s are equal if and only if they are both empty or they
    //! both contain a value and those values are equal.
    //! @include example/optional/comparable.cpp
    //!
    //! 2. `Orderable`\n
    //! Optional values can be ordered by considering the value they are
    //! holding, if any. To handle the case of an empty optional value, we
    //! arbitrarily set `nothing` as being less than any other `just`. Hence,
    //! @code
    //!     just(x) < just(y) if and only if x < y
    //!     nothing < just(anything)
    //! @endcode
    //! Example:
    //! @include example/optional/orderable.cpp
    //!
    //! 3. `Functor`\n
    //! An optional value can be seen as a list containing either one element
    //! (`just(x)`) or no elements at all (`nothing`). As such, mapping
    //! a function over an optional value is equivalent to applying it to
    //! its value if there is one, and to `nothing` otherwise:
    //! @code
    //!     transform(just(x), f) == just(f(x))
    //!     transform(nothing, f) == nothing
    //! @endcode
    //! Example:
    //! @include example/optional/functor.cpp
    //!
    //! 4. `Applicative`\n
    //! First, a value can be made optional with `lift<optional_tag>`, which
    //! is equivalent to `just`. Second, one can feed an optional value to an
    //! optional function with `ap`, which will return `just(f(x))` if there
    //! is both a function _and_ a value, and `nothing` otherwise:
    //! @code
    //!     ap(just(f), just(x)) == just(f(x))
    //!     ap(nothing, just(x)) == nothing
    //!     ap(just(f), nothing) == nothing
    //!     ap(nothing, nothing) == nothing
    //! @endcode
    //! A simple example:
    //! @include example/optional/applicative.cpp
    //! A more complex example:
    //! @include example/optional/applicative.complex.cpp
    //!
    //! 5. `Monad`\n
    //! The `Monad` model makes it easy to compose actions that might fail.
    //! One can feed an optional value if there is one into a function with
    //! `chain`, which will return `nothing` if there is no value. Finally,
    //! optional-optional values can have their redundant level of optionality
    //! removed with `flatten`. Also note that the `|` operator can be used in
    //! place of the `chain` function.
    //! Example:
    //! @include example/optional/monad.cpp
    //!
    //! 6. `MonadPlus`\n
    //! The `MonadPlus` model allows choosing the first valid value out of
    //! two optional values with `concat`. If both optional values are
    //! `nothing`s, `concat` will return `nothing`.
    //! Example:
    //! @include example/optional/monad_plus.cpp
    //!
    //! 7. `Foldable`\n
    //! Folding an optional value is equivalent to folding a list containing
    //! either no elements (for `nothing`) or `x` (for `just(x)`).
    //! Example:
    //! @include example/optional/foldable.cpp
    //!
    //! 8. `Searchable`\n
    //! Searching an optional value is equivalent to searching a list
    //! containing `x` for `just(x)` and an empty list for `nothing`.
    //! Example:
    //! @include example/optional/searchable.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename ...T>
    struct optional {
        // 5.3.1, Constructors

        //! Default-construct an `optional`. Only exists if the optional
        //! contains a value, and if that value is DefaultConstructible.
        constexpr optional() = default;

        //! Copy-construct an `optional`.
        //! An empty optional may only be copy-constructed from another
        //! empty `optional`, and an `optional` with a value may only be
        //! copy-constructed from another `optional` with a value.
        //! Furthermore, this constructor only exists if the value
        //! held in the `optional` is CopyConstructible.
        optional(optional const&) = default;

        //! Move-construct an `optional`.
        //! An empty optional may only be move-constructed from another
        //! empty `optional`, and an `optional` with a value may only be
        //! move-constructed from another `optional` with a value.
        //! Furthermore, this constructor only exists if the value
        //! held in the `optional` is MoveConstructible.
        optional(optional&&) = default;

        //! Construct an `optional` holding a value of type `T` from another
        //! object of type `T`. The value is copy-constructed.
        constexpr optional(T const& t)
            : value_(t)
        { }

        //! Construct an `optional` holding a value of type `T` from another
        //! object of type `T`. The value is move-constructed.
        constexpr optional(T&& t)
            : value_(static_cast<T&&>(t))
        { }

        // 5.3.3, Assignment

        //! Copy-assign an `optional`.
        //! An empty optional may only be copy-assigned from another empty
        //! `optional`, and an `optional` with a value may only be copy-assigned
        //! from another `optional` with a value. Furthermore, this assignment
        //! operator only exists if the value held in the `optional` is
        //! CopyAssignable.
        constexpr optional& operator=(optional const&) = default;

        //! Move-assign an `optional`.
        //! An empty optional may only be move-assigned from another empty
        //! `optional`, and an `optional` with a value may only be move-assigned
        //! from another `optional` with a value. Furthermore, this assignment
        //! operator only exists if the value held in the `optional` is
        //! MoveAssignable.
        constexpr optional& operator=(optional&&) = default;

        // 5.3.5, Observers

        //! Returns a pointer to the contained value, or a `nullptr` if the
        //! `optional` is empty.
        //!
        //!
        //! @note Overloads of this method are provided for both the `const`
        //! and the non-`const` cases.
        //!
        //!
        //! Example
        //! -------
        //! @include example/optional/value.cpp
        constexpr T* operator->();

        //! Extract the content of an `optional`, or fail at compile-time.
        //!
        //! If `*this` contains a value, that value is returned. Otherwise,
        //! a static assertion is triggered.
        //!
        //! @note
        //! Overloads of this method are provided for the cases where `*this`
        //! is a reference, a rvalue-reference and their `const` counterparts.
        //!
        //!
        //! Example
        //! -------
        //! @include example/optional/value.cpp
        constexpr T& value();

        //! Equivalent to `value()`, provided for convenience.
        //!
        //! @note
        //! Overloads of this method are provided for the cases where `*this`
        //! is a reference, a rvalue-reference and their `const` counterparts.
        //!
        //!
        //! Example
        //! -------
        //! @include example/optional/value.cpp
        constexpr T& operator*();

        //! Return the contents of an `optional`, with a fallback result.
        //!
        //! If `*this` contains a value, that value is returned. Otherwise,
        //! the default value provided is returned.
        //!
        //! @note
        //! Overloads of this method are provided for the cases where `*this`
        //! is a reference, a rvalue-reference and their `const` counterparts.
        //!
        //!
        //! @param default_
        //! The default value to return if `*this` does not contain a value.
        //!
        //!
        //! Example
        //! -------
        //! @include example/optional/value_or.cpp
        template <typename U>
        constexpr decltype(auto) value_or(U&& default_);

        //! Equivalent to `hana::chain`.
        template <typename ...T, typename F>
        friend constexpr auto operator|(optional<T...>, F);

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
    };
#else
    template <typename ...T>
    struct optional;
#endif

    //! Tag representing a `hana::optional`.
    //! @relates hana::optional
    struct optional_tag { };

    //! Create an optional value.
    //! @relates hana::optional
    //!
    //! Specifically, `make<optional_tag>()` is equivalent to `nothing`, and
    //! `make<optional_tag>(x)` is equivalent to `just(x)`. This is provided
    //! for consistency with the other `make<...>` functions.
    //!
    //!
    //! Example
    //! -------
    //! @include example/optional/make.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <>
    constexpr auto make<optional_tag> = []([auto&& x]) {
        return optional<std::decay<decltype(x)>::type>{forwarded(x)};
    };
#endif

    //! Alias to `make<optional_tag>`; provided for convenience.
    //! @relates hana::optional
    //!
    //!
    //! Example
    //! -------
    //! @include example/optional/make.cpp
    BOOST_HANA_INLINE_VARIABLE constexpr auto make_optional = make<optional_tag>;

    //! Create an optional value containing `x`.
    //! @relates hana::optional
    //!
    //!
    //! Example
    //! -------
    //! @include example/optional/just.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto just = [](auto&& x) {
        return optional<std::decay<decltype(x)>::type>{forwarded(x)};
    };
#else
    struct make_just_t {
        template <typename T>
        constexpr auto operator()(T&&) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr make_just_t just{};
#endif

    //! An empty optional value.
    //! @relates hana::optional
    //!
    //!
    //! Example
    //! -------
    //! @include example/optional/nothing.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr optional<> nothing{};
#else
    template <>
    struct optional<> : detail::operators::adl<optional<>> {
        // 5.3.1, Constructors
        constexpr optional() = default;
        constexpr optional(optional const&) = default;
        constexpr optional(optional&&) = default;

        // 5.3.3, Assignment
        constexpr optional& operator=(optional const&) = default;
        constexpr optional& operator=(optional&&) = default;

        // 5.3.5, Observers
        constexpr decltype(nullptr) operator->() const { return nullptr; }

        template <typename ...dummy>
        constexpr auto value() const;

        template <typename ...dummy>
        constexpr auto operator*() const;

        template <typename U>
        constexpr U&& value_or(U&& u) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr optional<> nothing{};
#endif

    //! Apply a function to the contents of an optional, with a fallback
    //! result.
    //! @relates hana::optional
    //!
    //! Specifically, `maybe` takes a default value, a function and an
    //! optional value. If the optional value is `nothing`, the default
    //! value is returned. Otherwise, the function is applied to the
    //! content of the `just`.
    //!
    //!
    //! @param default_
    //! A default value returned if `m` is `nothing`.
    //!
    //! @param f
    //! A function called as `f(x)` if and only if `m` is an optional value
    //! of the form `just(x)`. In that case, the result returend by `maybe`
    //! is the result of `f`.
    //!
    //! @param m
    //! An optional value.
    //!
    //!
    //! Example
    //! -------
    //! @include example/optional/maybe.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto maybe = [](auto&& default_, auto&& f, auto&& m) -> decltype(auto) {
        if (m is a just(x)) {
            return forwarded(f)(forwarded(x));
        else
            return forwarded(default_);
        }
    };
#else
    struct maybe_t {
        template <typename Def, typename F, typename T>
        constexpr decltype(auto) operator()(Def&&, F&& f, optional<T> const& m) const
        { return static_cast<F&&>(f)(m.value_); }

        template <typename Def, typename F, typename T>
        constexpr decltype(auto) operator()(Def&&, F&& f, optional<T>& m) const
        { return static_cast<F&&>(f)(m.value_); }

        template <typename Def, typename F, typename T>
        constexpr decltype(auto) operator()(Def&&, F&& f, optional<T>&& m) const
        { return static_cast<F&&>(f)(static_cast<optional<T>&&>(m).value_); }

        template <typename Def, typename F>
        constexpr Def operator()(Def&& def, F&&, optional<> const&) const
        { return static_cast<Def&&>(def); }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr maybe_t maybe{};
#endif

    //! Calls a function if the call expression is well-formed.
    //! @relates hana::optional
    //!
    //! Given a function `f`, `sfinae` returns a new function applying `f`
    //! to its arguments and returning `just` the result if the call is
    //! well-formed, and `nothing` otherwise. In other words, `sfinae(f)(x...)`
    //! is `just(f(x...))` if that expression is well-formed, and `nothing`
    //! otherwise. Note, however, that it is possible for an expression
    //! `f(x...)` to be well-formed as far as SFINAE is concerned, but
    //! trying to actually compile `f(x...)` still fails. In this case,
    //! `sfinae` won't be able to detect it and a hard failure is likely
    //! to happen.
    //!
    //!
    //! @note
    //! The function given to `sfinae` must not return `void`, since
    //! `just(void)` does not make sense. A compilation error is
    //! triggered if the function returns void.
    //!
    //!
    //! Example
    //! -------
    //! @include example/optional/sfinae.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    auto sfinae = [](auto&& f) {
        return [perfect-capture](auto&& ...x) {
            if (decltype(forwarded(f)(forwarded(x)...)) is well-formed)
                return just(forwarded(f)(forwarded(x)...));
            else
                return nothing;
        };
    };
#else
    struct sfinae_t {
        template <typename F>
        constexpr decltype(auto) operator()(F&& f) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr sfinae_t sfinae{};
#endif

    //! Return whether an `optional` contains a value.
    //! @relates hana::optional
    //!
    //! Specifically, returns a compile-time true-valued `Logical` if `m` is
    //! of the form `just(x)` for some `x`, and a false-valued one otherwise.
    //!
    //!
    //! Example
    //! -------
    //! @include example/optional/is_just.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto is_just = [](auto const& m) {
        return m is a just(x);
    };
#else
    struct is_just_t {
        template <typename ...T>
        constexpr auto operator()(optional<T...> const&) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr is_just_t is_just{};
#endif

    //! Return whether an `optional` is empty.
    //! @relates hana::optional
    //!
    //! Specifically, returns a compile-time true-valued `Logical` if `m` is
    //! a `nothing`, and a false-valued one otherwise.
    //!
    //!
    //! Example
    //! -------
    //! @include example/optional/is_nothing.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto is_nothing = [](auto const& m) {
        return m is a nothing;
    };
#else
    struct is_nothing_t {
        template <typename ...T>
        constexpr auto operator()(optional<T...> const&) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr is_nothing_t is_nothing{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_OPTIONAL_HPP
