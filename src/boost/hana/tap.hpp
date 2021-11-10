/*!
@file
Defines `boost::hana::tap`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_TAP_HPP
#define BOOST_HANA_TAP_HPP

#include <boost/hana/fwd/tap.hpp>

#include <boost/hana/concept/monad.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/lift.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename M>
    template <typename F>
    constexpr auto tap_t<M>::operator()(F&& f) const {
    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Monad<M>::value,
        "hana::tap<M> requires 'M' to be a Monad");
    #endif

        using Tap = BOOST_HANA_DISPATCH_IF(tap_impl<M>,
            hana::Monad<M>::value
        );

        return Tap::apply(static_cast<F&&>(f));
    }
    //! @endcond

    namespace detail {
        template <typename M>
        struct tap_helper {
            template <typename F, typename X>
            constexpr auto operator()(F&& f, X&& x) const {
                (void)static_cast<F&&>(f)(x);
                return hana::lift<M>(static_cast<X&&>(x));
            }
        };
    }

    template <typename M, bool condition>
    struct tap_impl<M, when<condition>> : default_ {
        template <typename F>
        static constexpr auto apply(F&& f)
        { return hana::partial(detail::tap_helper<M>{}, static_cast<F&&>(f)); }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_TAP_HPP
