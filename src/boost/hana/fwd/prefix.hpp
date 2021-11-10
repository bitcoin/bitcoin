/*!
@file
Forward declares `boost::hana::prefix`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_PREFIX_HPP
#define BOOST_HANA_FWD_PREFIX_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Inserts a value before each element of a monadic structure.
    //! @ingroup group-MonadPlus
    //!
    //! Given a monadic structure `xs` and a value `z` called the prefix,
    //! `prefix` returns a new monadic structure. `prefix` satisfies
    //! @code
    //!     prefix(xs, z) == flatten(transform(xs, [](auto x) {
    //!         return concat(lift<M>(z), lift<M>(x));
    //!     }))
    //! @endcode
    //!
    //! For sequences, this simply corresponds to inserting the prefix before
    //! each element of the sequence. For example, given a sequence
    //! `[x1, ..., xn]`, `prefix` will return
    //! @code
    //!     [z, x1, z, x2, ..., z, xn]
    //! @endcode
    //! As explained above, this can be generalized to other MonadPlus models,
    //! with various levels of interest.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a MonadPlus `M`, the signature is
    //! @f$ \mathrm{prefix} : M(T) \times T \to M(T) @f$.
    //!
    //! @param xs
    //! A monadic structure.
    //!
    //! @param pref
    //! A value (the prefix) to insert before each element of a monadic
    //! structure.
    //!
    //!
    //! Example
    //! -------
    //! @include example/prefix.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto prefix = [](auto&& xs, auto&& pref) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct prefix_impl : prefix_impl<M, when<true>> { };

    struct prefix_t {
        template <typename Xs, typename Pref>
        constexpr auto operator()(Xs&& xs, Pref&& pref) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr prefix_t prefix{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_PREFIX_HPP
