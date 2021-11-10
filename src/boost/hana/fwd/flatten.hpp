/*!
@file
Forward declares `boost::hana::flatten`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_FLATTEN_HPP
#define BOOST_HANA_FWD_FLATTEN_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Collapse two levels of monadic structure into a single level.
    //! @ingroup group-Monad
    //!
    //! Given a monadic value wrapped into two levels of monad, `flatten`
    //! removes one such level. An implementation of `flatten` must satisfy
    //! @code
    //!     flatten(xs) == chain(xs, id)
    //! @endcode
    //!
    //! For `Sequence`s, this simply takes a `Sequence` of `Sequence`s, and
    //! returns a (non-recursively) flattened `Sequence`.
    //!
    //!
    //! Signature
    //! ---------
    //! For a `Monad` `M`, the signature of `flatten` is
    //! @f$
    //!     \mathtt{flatten} : M(M(T)) \to M(T)
    //! @f$
    //!
    //! @param xs
    //! A value with two levels of monadic structure, which should be
    //! collapsed into a single level of structure.
    //!
    //!
    //! Example
    //! -------
    //! @include example/flatten.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto flatten = [](auto&& xs) {
        return tag-dispatched;
    };
#else
    template <typename M, typename = void>
    struct flatten_impl : flatten_impl<M, when<true>> { };

    struct flatten_t {
        template <typename Xs>
        constexpr auto operator()(Xs&& xs) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr flatten_t flatten{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_FLATTEN_HPP
