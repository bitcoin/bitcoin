/*!
@file
Defines `boost::hana::iterate`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FUNCTIONAL_ITERATE_HPP
#define BOOST_HANA_FUNCTIONAL_ITERATE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/functional/partial.hpp>

#include <cstddef>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-functional
    //! Applies another function `n` times to its argument.
    //!
    //! Given a function `f` and an argument `x`, `iterate<n>(f, x)` returns
    //! the result of applying `f` `n` times to its argument. In other words,
    //! @code
    //!     iterate<n>(f, x) == f(f( ... f(x)))
    //!                         ^^^^^^^^^^ n times total
    //! @endcode
    //!
    //! If `n == 0`, `iterate<n>(f, x)` returns the `x` argument unchanged
    //! and `f` is never applied. It is important to note that the function
    //! passed to `iterate<n>` must be a unary function. Indeed, since `f`
    //! will be called with the result of the previous `f` application, it
    //! may only take a single argument.
    //!
    //! In addition to what's documented above, `iterate` can also be
    //! partially applied to the function argument out-of-the-box. In
    //! other words, `iterate<n>(f)` is a function object applying `f`
    //! `n` times to the argument it is called with, which means that
    //! @code
    //!     iterate<n>(f)(x) == iterate<n>(f, x)
    //! @endcode
    //!
    //! This is provided for convenience, and it turns out to be especially
    //! useful in conjunction with higher-order algorithms.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a function \f$ f : T \to T \f$ and `x` and argument of data
    //! type `T`, the signature is
    //! \f$
    //!     \mathtt{iterate_n} : (T \to T) \times T \to T
    //! \f$
    //!
    //! @tparam n
    //! An unsigned integer representing the number of times that `f`
    //! should be applied to its argument.
    //!
    //! @param f
    //! A function to apply `n` times to its argument.
    //!
    //! @param x
    //! The initial value to call `f` with.
    //!
    //!
    //! Example
    //! -------
    //! @include example/functional/iterate.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <std::size_t n>
    constexpr auto iterate = [](auto&& f) {
        return [perfect-capture](auto&& x) -> decltype(auto) {
            return f(f( ... f(forwarded(x))));
        };
    };
#else
    template <std::size_t n, typename = when<true>>
    struct iterate_t;

    template <>
    struct iterate_t<0> {
        template <typename F, typename X>
        constexpr X operator()(F&&, X&& x) const
        { return static_cast<X&&>(x); }
    };

    template <>
    struct iterate_t<1> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return f(static_cast<X&&>(x));
        }
    };

    template <>
    struct iterate_t<2> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return f(f(static_cast<X&&>(x)));
        }
    };

    template <>
    struct iterate_t<3> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return f(f(f(static_cast<X&&>(x))));
        }
    };

    template <>
    struct iterate_t<4> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return f(f(f(f(static_cast<X&&>(x)))));
        }
    };

    template <>
    struct iterate_t<5> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return f(f(f(f(f(static_cast<X&&>(x))))));
        }
    };

    template <std::size_t n>
    struct iterate_t<n, when<(n >= 6) && (n < 12)>> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return iterate_t<n - 6>{}(f,
                f(f(f(f(f(f(static_cast<X&&>(x)))))))
            );
        }
    };

    template <std::size_t n>
    struct iterate_t<n, when<(n >= 12) && (n < 24)>> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return iterate_t<n - 12>{}(f,
                f(f(f(f(f(f(f(f(f(f(f(f(
                    static_cast<X&&>(x)
                ))))))))))))
            );
        }
    };

    template <std::size_t n>
    struct iterate_t<n, when<(n >= 24) && (n < 48)>> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return iterate_t<n - 24>{}(f,
                f(f(f(f(f(f(f(f(f(f(f(f(
                f(f(f(f(f(f(f(f(f(f(f(f(
                    static_cast<X&&>(x)
                ))))))))))))
                ))))))))))))
            );
        }
    };

    template <std::size_t n>
    struct iterate_t<n, when<(n >= 48)>> {
        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return iterate_t<n - 48>{}(f,
                f(f(f(f(f(f(f(f(f(f(f(f(
                f(f(f(f(f(f(f(f(f(f(f(f(
                f(f(f(f(f(f(f(f(f(f(f(f(
                f(f(f(f(f(f(f(f(f(f(f(f(
                    static_cast<X&&>(x)
                ))))))))))))
                ))))))))))))
                ))))))))))))
                ))))))))))))
            );
        }
    };

    template <std::size_t n>
    struct make_iterate_t {
        template <typename F>
        constexpr decltype(auto) operator()(F&& f) const
        { return hana::partial(iterate_t<n>{}, static_cast<F&&>(f)); }

        template <typename F, typename X>
        constexpr decltype(auto) operator()(F&& f, X&& x) const {
            return iterate_t<n>{}(static_cast<F&&>(f),
                                  static_cast<X&&>(x));
        }
    };

    template <std::size_t n>
    BOOST_HANA_INLINE_VARIABLE constexpr make_iterate_t<n> iterate{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FUNCTIONAL_ITERATE_HPP
