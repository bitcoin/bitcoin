/*!
@file
Forward declares `boost::hana::prepend`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_PREPEND_HPP
#define BOOST_HANA_FWD_PREPEND_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Prepend an element to a monadic structure.
    //! @ingroup group-MonadPlus
    //!
    //! Given a monadic structure `xs` and an element `x`, `prepend` returns
    //! a new monadic structure which is the result of lifting `x` into the
    //! monadic structure and then combining that (to the left) with `xs`.
    //! In other words,
    //! @code
    //!     prepend(xs, x) == concat(lift<Xs>(x), xs)
    //! @endcode
    //!
    //! For sequences, this has the intuitive behavior of simply prepending
    //! an element to the beginning of the sequence, hence the name.
    //!
    //! > #### Rationale for not calling this `push_front`
    //! > While `push_front` is the de-facto name used in the standard library,
    //! > it also strongly suggests mutation of the underlying sequence, which
    //! > is not the case here. The author also finds that `push_front`
    //! > suggests too strongly the sole interpretation of putting an
    //! > element to the front of a sequence, whereas `prepend` is slightly
    //! > more nuanced and bears its name better for e.g. `hana::optional`.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a MonadPlus `M`, the signature is
    //! @f$ \mathtt{prepend} : M(T) \times T \to M(T) @f$.
    //!
    //! @param xs
    //! A monadic structure that will be combined to the right of the element.
    //!
    //! @param x
    //! An element to combine to the left of the monadic structure.
    //!
    //!
    //! Example
    //! -------
    //! @include example/prepend.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto prepend = [](auto&& xs, auto&& x) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct prepend_impl : prepend_impl<M, when<true>> { };

    struct prepend_t {
        template <typename Xs, typename X>
        constexpr auto operator()(Xs&& xs, X&& x) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr prepend_t prepend{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_PREPEND_HPP
