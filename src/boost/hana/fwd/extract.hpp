/*!
@file
Forward declares `boost::hana::extract`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_EXTRACT_HPP
#define BOOST_HANA_FWD_EXTRACT_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! Extract a value in a given comonadic context.
    //! @ingroup group-Comonad
    //!
    //! Given a value inside a comonadic context, extract it from that
    //! context, performing whatever effects are mandated by that context.
    //! This can be seen as the dual operation to the `lift` method of the
    //! Applicative concept.
    //!
    //!
    //! Signature
    //! ---------
    //! Given a Comonad `W`, the signature is
    //! \f$
    //!     \mathtt{extract} : W(T) \to T
    //! \f$
    //!
    //! @param w
    //! The value to be extracted inside a comonadic context.
    //!
    //!
    //! Example
    //! -------
    //! @include example/extract.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto extract = [](auto&& w) -> decltype(auto) {
        return tag-dispatched;
    };
#else
    template <typename W, typename = void>
    struct extract_impl : extract_impl<W, when<true>> { };

    struct extract_t {
        template <typename W_>
        constexpr decltype(auto) operator()(W_&& w) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr extract_t extract{};
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_EXTRACT_HPP
