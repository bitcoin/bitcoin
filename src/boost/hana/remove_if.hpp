/*!
@file
Defines `boost::hana::remove_if`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_REMOVE_IF_HPP
#define BOOST_HANA_REMOVE_IF_HPP

#include <boost/hana/fwd/remove_if.hpp>

#include <boost/hana/concept/monad_plus.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/filter.hpp>
#include <boost/hana/functional/compose.hpp>
#include <boost/hana/not.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Pred>
    constexpr auto remove_if_t::operator()(Xs&& xs, Pred&& pred) const {
        using M = typename hana::tag_of<Xs>::type;
        using RemoveIf = BOOST_HANA_DISPATCH_IF(remove_if_impl<M>,
            hana::MonadPlus<M>::value
        );

        #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
            static_assert(hana::MonadPlus<M>::value,
            "hana::remove_if(xs, predicate) requires 'xs' to be a MonadPlus");
        #endif

        return RemoveIf::apply(static_cast<Xs&&>(xs),
                               static_cast<Pred&&>(pred));
    }
    //! @endcond

    template <typename M, bool condition>
    struct remove_if_impl<M, when<condition>> : default_ {
        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs&& xs, Pred&& pred) {
            return hana::filter(static_cast<Xs&&>(xs),
                        hana::compose(hana::not_, static_cast<Pred&&>(pred)));
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_REMOVE_IF_HPP
