/*!
@file
Defines `boost::hana::demux`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FUNCTIONAL_DEMUX_HPP
#define BOOST_HANA_FUNCTIONAL_DEMUX_HPP

#include <boost/hana/basic_tuple.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-functional
    //! Invoke a function with the results of invoking other functions
    //! on its arguments.
    //!
    //! Specifically, `demux(f)(g...)` is a function such that
    //! @code
    //!     demux(f)(g...)(x...) == f(g(x...)...)
    //! @endcode
    //!
    //! Each `g` is called with all the arguments, and then `f` is called
    //! with the result of each `g`. Hence, the arity of `f` must match
    //! the number of `g`s.
    //!
    //! This is called `demux` because of a vague similarity between this
    //! device and a demultiplexer in signal processing. `demux` takes what
    //! can be seen as a continuation (`f`), a bunch of functions to split a
    //! signal (`g...`) and zero or more arguments representing the signal
    //! (`x...`). Then, it calls the continuation with the result of
    //! splitting the signal with whatever functions where given.
    //!
    //! @note
    //! When used with two functions only, `demux` is associative. In other
    //! words (and noting `demux(f, g) = demux(f)(g)` to ease the notation),
    //! it is true that `demux(demux(f, g), h) == demux(f, demux(g, h))`.
    //!
    //!
    //! Signature
    //! ---------
    //! The signature of `demux` is
    //! \f[
    //!     \mathtt{demux} :
    //!         (B_1 \times \dotsb \times B_n \to C)
    //!             \to ((A_1 \times \dotsb \times A_n \to B_1)
    //!                 \times \dotsb
    //!                 \times (A_1 \times \dotsb \times A_n \to B_n))
    //!             \to (A_1 \times \dotsb \times A_n \to C)
    //! \f]
    //!
    //! This can be rewritten more tersely as
    //! \f[
    //!     \mathtt{demux} :
    //!         \left(\prod_{i=1}^n B_i \to C \right)
    //!         \to \prod_{j=1}^n \left(\prod_{i=1}^n A_i \to B_j \right)
    //!         \to \left(\prod_{i=1}^n A_i \to C \right)
    //! \f]
    //!
    //!
    //! Link with normal composition
    //! ----------------------------
    //! The signature of `compose` is
    //! \f[
    //!     \mathtt{compose} : (B \to C) \times (A \to B) \to (A \to C)
    //! \f]
    //!
    //! A valid observation is that this coincides exactly with the type
    //! of `demux` when used with a single unary function. Actually, both
    //! functions are equivalent:
    //! @code
    //!     demux(f)(g)(x) == compose(f, g)(x)
    //! @endcode
    //!
    //! However, let's now consider the curried version of `compose`,
    //! `curry<2>(compose)`:
    //! \f[
    //!     \mathtt{curry_2(compose)} : (B \to C) \to ((A \to B) \to (A \to C))
    //! \f]
    //!
    //! For the rest of this explanation, we'll just consider the curried
    //! version of `compose` and so we'll use `compose` instead of
    //! `curry<2>(compose)` to lighten the notation. With currying, we can
    //! now consider `compose` applied to itself:
    //! \f[
    //!     \mathtt{compose(compose, compose)} :
    //!         (B \to C) \to (A_1 \to A_2 \to B) \to (A_1 \to A_2 \to C)
    //! \f]
    //!
    //! If we uncurry deeply the above expression, we obtain
    //! \f[
    //!     \mathtt{compose(compose, compose)} :
    //!         (B \to C) \times (A_1 \times A_2 \to B) \to (A_1 \times A_2 \to C)
    //! \f]
    //!
    //! This signature is exactly the same as that of `demux` when given a
    //! single binary function, and indeed they are equivalent definitions.
    //! We can also generalize this further by considering
    //! `compose(compose(compose, compose), compose)`:
    //! \f[
    //!     \mathtt{compose(compose(compose, compose), compose)} :
    //!         (B \to C) \to (A_1 \to A_2 \to A_3 \to B)
    //!             \to (A_1 \to A_2 \to A_3 \to C)
    //! \f]
    //!
    //! which uncurries to
    //! \f[
    //!     \mathtt{compose(compose(compose, compose), compose)} :
    //!         (B \to C) \times (A_1 \times A_2 \times A_3 \to B)
    //!             \to (A_1 \times A_2 \times A_3 \to C)
    //! \f]
    //!
    //! This signature is exactly the same as that of `demux` when given a
    //! single ternary function. Hence, for a single n-ary function `g`,
    //! `demux(f)(g)` is equivalent to the n-times composition of `compose`
    //! with itself, applied to `g` and `f`:
    //! @code
    //!     demux(f)(g) == fold_left([compose, ..., compose], id, compose)(g, f)
    //!                           //  ^^^^^^^^^^^^^^^^^^^^^ n times
    //! @endcode
    //!
    //! More information on this insight can be seen [here][1]. Also, I'm
    //! not sure how this insight could be generalized to more than one
    //! function `g`, or if that is even possible.
    //!
    //!
    //! Proof of associativity in the binary case
    //! -----------------------------------------
    //! As explained above, `demux` is associative when it is used with
    //! two functions only. Indeed, given functions `f`, `g` and `h` with
    //! suitable signatures, we have
    //! @code
    //!     demux(f)(demux(g)(h))(x...) == f(demux(g)(h)(x...))
    //!                                 == f(g(h(x...)))
    //! @endcode
    //!
    //! On the other hand, we have
    //! @code
    //!     demux(demux(f)(g))(h)(x...) == demux(f)(g)(h(x...))
    //!                                 == f(g(h(x...)))
    //! @endcode
    //!
    //! and hence `demux` is associative in the binary case.
    //!
    //!
    //! Example
    //! -------
    //! @include example/functional/demux.cpp
    //!
    //! [1]: http://stackoverflow.com/q/5821089/627587
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto demux = [](auto&& f) {
        return [perfect-capture](auto&& ...g) {
            return [perfect-capture](auto&& ...x) -> decltype(auto) {
                // x... can't be forwarded unless there is a single g
                // function, or that could cause double-moves.
                return forwarded(f)(forwarded(g)(x...)...);
            };
        };
    };
#else
    template <typename F>
    struct pre_demux_t;

    struct make_pre_demux_t {
        struct secret { };
        template <typename F>
        constexpr pre_demux_t<typename detail::decay<F>::type> operator()(F&& f) const {
            return {static_cast<F&&>(f)};
        }
    };

    template <typename Indices, typename F, typename ...G>
    struct demux_t;

    template <typename F>
    struct pre_demux_t {
        F f;

        template <typename ...G>
        constexpr demux_t<std::make_index_sequence<sizeof...(G)>, F,
                          typename detail::decay<G>::type...>
        operator()(G&& ...g) const& {
            return {make_pre_demux_t::secret{}, this->f, static_cast<G&&>(g)...};
        }

        template <typename ...G>
        constexpr demux_t<std::make_index_sequence<sizeof...(G)>, F,
                          typename detail::decay<G>::type...>
        operator()(G&& ...g) && {
            return {make_pre_demux_t::secret{}, static_cast<F&&>(this->f), static_cast<G&&>(g)...};
        }
    };

    template <std::size_t ...n, typename F, typename ...G>
    struct demux_t<std::index_sequence<n...>, F, G...> {
        template <typename ...T>
        constexpr demux_t(make_pre_demux_t::secret, T&& ...t)
            : storage_{static_cast<T&&>(t)...}
        { }

        basic_tuple<F, G...> storage_;

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) const& {
            return hana::at_c<0>(storage_)(
                hana::at_c<n+1>(storage_)(x...)...
            );
        }

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) & {
            return hana::at_c<0>(storage_)(
                hana::at_c<n+1>(storage_)(x...)...
            );
        }

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) && {
            return static_cast<F&&>(hana::at_c<0>(storage_))(
                static_cast<G&&>(hana::at_c<n+1>(storage_))(x...)...
            );
        }
    };

    template <typename F, typename G>
    struct demux_t<std::index_sequence<0>, F, G> {
        template <typename ...T>
        constexpr demux_t(make_pre_demux_t::secret, T&& ...t)
            : storage_{static_cast<T&&>(t)...}
        { }

        basic_tuple<F, G> storage_;

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) const& {
            return hana::at_c<0>(storage_)(
                hana::at_c<1>(storage_)(static_cast<X&&>(x)...)
            );
        }

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) & {
            return hana::at_c<0>(storage_)(
                hana::at_c<1>(storage_)(static_cast<X&&>(x)...)
            );
        }

        template <typename ...X>
        constexpr decltype(auto) operator()(X&& ...x) && {
            return static_cast<F&&>(hana::at_c<0>(storage_))(
                static_cast<G&&>(hana::at_c<1>(storage_))(static_cast<X&&>(x)...)
            );
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr make_pre_demux_t demux{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FUNCTIONAL_DEMUX_HPP
