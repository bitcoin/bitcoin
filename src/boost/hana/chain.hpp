/*!
@file
Defines `boost::hana::chain`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_CHAIN_HPP
#define BOOST_HANA_CHAIN_HPP

#include <boost/hana/fwd/chain.hpp>

#include <boost/hana/concept/monad.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/flatten.hpp>
#include <boost/hana/transform.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename F>
    constexpr decltype(auto) chain_t::operator()(Xs&& xs, F&& f) const {
        using M = typename hana::tag_of<Xs>::type;
        using Chain = BOOST_HANA_DISPATCH_IF(chain_impl<M>,
            hana::Monad<M>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Monad<M>::value,
        "hana::chain(xs, f) requires 'xs' to be a Monad");
    #endif

        return Chain::apply(static_cast<Xs&&>(xs), static_cast<F&&>(f));
    }
    //! @endcond

    template <typename M, bool condition>
    struct chain_impl<M, when<condition>> : default_ {
        template <typename Xs, typename F>
        static constexpr auto apply(Xs&& xs, F&& f) {
            return hana::flatten(hana::transform(static_cast<Xs&&>(xs),
                                                 static_cast<F&&>(f)));
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_CHAIN_HPP
