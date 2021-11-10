/*!
@file
Defines `boost::hana::curry`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FUNCTIONAL_CURRY_HPP
#define BOOST_HANA_FUNCTIONAL_CURRY_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/functional/apply.hpp>
#include <boost/hana/functional/partial.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-functional
    //! Curry a function up to the given number of arguments.
    //!
    //! [Currying][Wikipedia.currying] is a technique in which we consider a
    //! function taking multiple arguments (or, equivalently, a tuple of
    //! arguments), and turn it into a function which takes a single argument
    //! and returns a function to handle the remaining arguments. To help
    //! visualize, let's denote the type of a function `f` which takes
    //! arguments of types `X1, ..., Xn` and returns a `R` as
    //! @code
    //!     (X1, ..., Xn) -> R
    //! @endcode
    //!
    //! Then, currying is the process of taking `f` and turning it into an
    //! equivalent function (call it `g`) of type
    //! @code
    //!     X1 -> (X2 -> (... -> (Xn -> R)))
    //! @endcode
    //!
    //! This gives us the following equivalence, where `x1`, ..., `xn` are
    //! objects of type `X1`, ..., `Xn` respectively:
    //! @code
    //!     f(x1, ..., xn) == g(x1)...(xn)
    //! @endcode
    //!
    //! Currying can be useful in several situations, especially when working
    //! with higher-order functions.
    //!
    //! This `curry` utility is an implementation of currying in C++.
    //! Specifically, `curry<n>(f)` is a function such that
    //! @code
    //!     curry<n>(f)(x1)...(xn) == f(x1, ..., xn)
    //! @endcode
    //!
    //! Note that the `n` has to be specified explicitly because the existence
    //! of functions with variadic arguments in C++ make it impossible to know
    //! when currying should stop.
    //!
    //! Unlike usual currying, this implementation also allows a curried
    //! function to be called with several arguments at a time. Hence, the
    //! following always holds
    //! @code
    //!     curry<n>(f)(x1, ..., xk) == curry<n - k>(f)(x1)...(xk)
    //! @endcode
    //!
    //! Of course, this requires `k` to be less than or equal to `n`; failure
    //! to satisfy this will trigger a static assertion. This syntax is
    //! supported because it makes curried functions usable where normal
    //! functions are expected.
    //!
    //! Another "extension" is that `curry<0>(f)` is supported: `curry<0>(f)`
    //! is a nullary function; whereas the classical definition for currying
    //! seems to leave this case undefined, as nullary functions don't make
    //! much sense in purely functional languages.
    //!
    //!
    //! Example
    //! -------
    //! @include example/functional/curry.cpp
    //!
    //!
    //! [Wikipedia.currying]: http://en.wikipedia.org/wiki/Currying
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <std::size_t n>
    constexpr auto curry = [](auto&& f) {
        return [perfect-capture](auto&& x1) {
            return [perfect-capture](auto&& x2) {
                ...
                    return [perfect-capture](auto&& xn) -> decltype(auto) {
                        return forwarded(f)(
                            forwarded(x1), forwarded(x2), ..., forwarded(xn)
                        );
                    };
            };
        };
    };
#else
    template <std::size_t n, typename F>
    struct curry_t;

    template <std::size_t n>
    struct make_curry_t {
        template <typename F>
        constexpr curry_t<n, typename detail::decay<F>::type>
        operator()(F&& f) const { return {static_cast<F&&>(f)}; }
    };

    template <std::size_t n>
    BOOST_HANA_INLINE_VARIABLE constexpr make_curry_t<n> curry{};

    namespace curry_detail { namespace {
        template <std::size_t n>
        constexpr make_curry_t<n> curry_or_call{};

        template <>
        constexpr auto curry_or_call<0> = apply;
    }}

    template <std::size_t n, typename F>
    struct curry_t {
        F f;

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) const& {
            static_assert(sizeof...(x) <= n,
            "too many arguments provided to boost::hana::curry");
            return curry_detail::curry_or_call<n - sizeof...(x)>(
                partial(f, static_cast<X&&>(x)...)
            );
        }

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) & {
            static_assert(sizeof...(x) <= n,
            "too many arguments provided to boost::hana::curry");
            return curry_detail::curry_or_call<n - sizeof...(x)>(
                partial(f, static_cast<X&&>(x)...)
            );
        }

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) && {
            static_assert(sizeof...(x) <= n,
            "too many arguments provided to boost::hana::curry");
            return curry_detail::curry_or_call<n - sizeof...(x)>(
                partial(std::move(f), static_cast<X&&>(x)...)
            );
        }
    };

    template <typename F>
    struct curry_t<0, F> {
        F f;

        constexpr decltype(auto) operator()() const&
        { return f(); }

        constexpr decltype(auto) operator()() &
        { return f(); }

        constexpr decltype(auto) operator()() &&
        { return std::move(f)(); }
    };
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FUNCTIONAL_CURRY_HPP
