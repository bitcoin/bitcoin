/*!
@file
Forward declares `boost::hana::append`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_APPEND_HPP
#define BOOST_HANA_FWD_APPEND_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Append an element to a monadic structure.
    //! @ingroup group-MonadPlus
    //!
    //! Given an element `x` and a monadic structure `xs`, `append` returns a
    //! new monadic structure which is the result of lifting `x` into the
    //! monadic structure and then combining that (to the right) with `xs`.
    //! In other words,
    //! @code
    //!     append(xs, x) == concat(xs, lift<Xs>(x))
    //! @endcode
    //! where `Xs` is the tag of `xs`. For sequences, this has the intuitive
    //! behavior of simply appending an element to the end of the sequence,
    //! hence the name.
    //!
    //! > #### Rationale for not calling this `push_back`
    //! > See the rationale for using `prepend` instead of `push_front`.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a MonadPlus `M`, the signature is
    //! @f$ \mathtt{append} : M(T) \times T \to M(T) @f$.
    //!
    //! @param xs
    //! A monadic structure that will be combined to the left of the element.
    //!
    //! @param x
    //! An element to combine to the right of the monadic structure.
    //!
    //!
    //! Example
    //! -------
    //! @include example/append.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto append = [](auto&& xs, auto&& x) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct append_impl : append_impl<M, when<true>> { };

    struct append_t {
        template <typename Xs, typename X>
        constexpr auto operator()(Xs&& xs, X&& x) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr append_t append{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_APPEND_HPP
