/*!
@file
Forward declares `boost::hana::concat`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCAT_HPP
#define BOOST_HANA_FWD_CONCAT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Combine two monadic structures together.
    //! @ingroup group-MonadPlus
    //!
    //! Given two monadic structures, `concat` combines them together and
    //! returns a new monadic structure. The exact definition of `concat`
    //! will depend on the exact model of MonadPlus at hand, but for
    //! sequences it corresponds intuitively to simple concatenation.
    //!
    //! Also note that combination is not required to be commutative.
    //! In other words, there is no requirement that
    //! @code
    //!     concat(xs, ys) == concat(ys, xs)
    //! @endcode
    //! and indeed it does not hold in general.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a `MonadPlus` `M`, the signature of `concat` is
    //! @f$ \mathtt{concat} : M(T) \times M(T) \to M(T) @f$.
    //!
    //! @param xs, ys
    //! Two monadic structures to combine together.
    //!
    //!
    //! Example
    //! -------
    //! @include example/concat.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto concat = [](auto&& xs, auto&& ys) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct concat_impl : concat_impl<M, when<true>> { };

    struct concat_t {
        template <typename Xs, typename Ys>
        constexpr auto operator()(Xs&& xs, Ys&& ys) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr concat_t concat{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCAT_HPP
