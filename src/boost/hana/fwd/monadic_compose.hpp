/*!
@file
Forward declares `boost::hana::monadic_compose`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_MONADIC_COMPOSE_HPP
#define BOOST_HANA_FWD_MONADIC_COMPOSE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Composition of monadic functions.
    //! @ingroup group-Monad
    //!
    //! Given two monadic functions `f` and `g`, `monadic_compose` returns
    //! a new function equivalent to the composition of `f` with `g`, except
    //! the result of `g` is `chain`ed into `f` instead of simply passed to
    //! it, as with normal composition. `monadic_compose` satisfies
    //! @code
    //!     monadic_compose(f, g)(x) == chain(g(x), f)
    //! @endcode
    //!
    //!
    //! @note
    //! Unlike `compose`, `monadic_compose` does not generalize nicely to
    //! arities higher than one. Hence, only unary functions may be used
    //! with `monadic_compose`.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `Monad` `M` and two functions @f$ f : B \to M(C) @f$ and
    //! @f$ g : A \to M(B) @f$, the signature is
    //! @f$
    //!     \mathtt{monadic\_compose}
    //!         : (B \to M(C)) \times (A \to M(B)) \to (A \to M(C))
    //! @f$.
    //!
    //! @param f
    //! A monadic function with signature @f$ B \to M(C) @f$.
    //!
    //! @param g
    //! A monadic function with signature @f$ A \to M(B) @f$.
    //!
    //!
    //! @note
    //! This method is not tag-dispatched, so it can't be customized directly.
    //!
    //!
    //! Example
    //! -------
    //! @include example/monadic_compose.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto monadic_compose = [](auto&& f, auto&& g) {
        return [perfect-capture](auto&& x) -> decltype(auto) {
            return hana::chain(forwarded(g)(forwarded(x)), forwarded(f));
        };
    };
#else
    struct monadic_compose_t {
        template <typename F, typename G>
        constexpr auto operator()(F&& f, G&& g) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr monadic_compose_t monadic_compose{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_MONADIC_COMPOSE_HPP
