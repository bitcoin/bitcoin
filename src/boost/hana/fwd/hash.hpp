/*!
@file
Forward declares `boost::hana::hash`.

@copyright Louis Dionne 2016
@copyright Jason Rice 2016
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_HASH_HPP
#define BOOST_HANA_FWD_HASH_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Returns a `hana::type` representing the compile-time hash of an object.
    //! @ingroup group-Hashable
    //!
    //! Given an arbitrary object `x`, `hana::hash` returns a `hana::type`
    //! representing the hash of `x`. In normal programming, hashes are
    //! usually numerical values that can be used e.g. as indices in an
    //! array as part of the implementation of a hash table. In the context
    //! of metaprogramming, we are interested in type-level hashes instead.
    //! Thus, `hana::hash` must return a `hana::type` object instead of an
    //! integer. This `hana::type` must somehow summarize the object being
    //! hashed, but that summary may of course lose some information.
    //!
    //! In order for the `hash` function to be defined properly, it must be
    //! the case that whenever `x` is equal to `y`, then `hash(x)` is equal
    //! to `hash(y)`. This ensures that `hana::hash` is a function in the
    //! mathematical sense of the term.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `Hashable` `H`, the signature is
    //! \f$
    //!     \mathtt{hash} : H \to \mathtt{type\_tag}
    //! \f$
    //!
    //! @param x
    //! An object whose hash is to be computed.
    //!
    //!
    //! Example
    //! -------
    //! @include example/hash.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto hash = [](auto const& x) {
        return tag-dispatched;
    };
#else
    template <typename T, typename = void>
    struct hash_impl : hash_impl<T, when<true>> { };

    struct hash_t {
        template <typename X>
        constexpr auto operator()(X const& x) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr hash_t hash{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_HASH_HPP
