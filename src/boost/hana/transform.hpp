/*!
@file
Defines `boost::hana::transform`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_TRANSFORM_HPP
#define BOOST_HANA_TRANSFORM_HPP

#include <boost/hana/fwd/transform.hpp>

#include <boost/hana/bool.hpp>
#include <boost/hana/concept/functor.hpp>
#include <boost/hana/concept/sequence.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/functional/always.hpp>
#include <boost/hana/fwd/adjust_if.hpp>
#include <boost/hana/unpack.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename F>
    constexpr auto transform_t::operator()(Xs&& xs, F&& f) const {
        using S = typename hana::tag_of<Xs>::type;
        using Transform = BOOST_HANA_DISPATCH_IF(transform_impl<S>,
            hana::Functor<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Functor<S>::value,
        "hana::transform(xs, f) requires 'xs' to be a Functor");
    #endif

        return Transform::apply(static_cast<Xs&&>(xs), static_cast<F&&>(f));
    }
    //! @endcond

    template <typename Fun, bool condition>
    struct transform_impl<Fun, when<condition>> : default_ {
        template <typename Xs, typename F>
        static constexpr auto apply(Xs&& xs, F&& f) {
            return hana::adjust_if(static_cast<Xs&&>(xs),
                                   hana::always(hana::true_c),
                                   static_cast<F&&>(f));
        }
    };

    template <typename S>
    struct transform_impl<S, when<Sequence<S>::value>> {
        //! @cond
        template <typename F>
        struct transformer {
            F f;
            template <typename ...Xs>
            constexpr auto operator()(Xs&& ...xs) const {
                return hana::make<S>((*f)(static_cast<Xs&&>(xs))...);
            }
        };
        //! @endcond

        template <typename Xs, typename F>
        static constexpr auto apply(Xs&& xs, F&& f) {
            // We use a pointer to workaround a Clang 3.5 ICE
            return hana::unpack(static_cast<Xs&&>(xs),
                                transformer<decltype(&f)>{&f});
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_TRANSFORM_HPP
