/*!
@file
Defines `boost::hana::then`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_THEN_HPP
#define BOOST_HANA_THEN_HPP

#include <boost/hana/fwd/then.hpp>

#include <boost/hana/chain.hpp>
#include <boost/hana/concept/monad.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/functional/always.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Before, typename Xs>
    constexpr decltype(auto) then_t::operator()(Before&& before, Xs&& xs) const {
        using M = typename hana::tag_of<Before>::type;
        using Then = BOOST_HANA_DISPATCH_IF(then_impl<M>,
            hana::Monad<M>::value &&
            hana::Monad<Xs>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Monad<M>::value,
        "hana::then(before, xs) requires 'before' to be a Monad");

        static_assert(hana::Monad<Xs>::value,
        "hana::then(before, xs) requires 'xs' to be a Monad");
    #endif

        return Then::apply(static_cast<Before&&>(before),
                           static_cast<Xs&&>(xs));
    }
    //! @endcond

    template <typename M, bool condition>
    struct then_impl<M, when<condition>> : default_ {
        template <typename Xs, typename Ys>
        static constexpr decltype(auto) apply(Xs&& xs, Ys&& ys) {
            return hana::chain(static_cast<Xs&&>(xs),
                               hana::always(static_cast<Ys&&>(ys)));
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_THEN_HPP
