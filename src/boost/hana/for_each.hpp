/*!
@file
Defines `boost::hana::for_each`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FOR_EACH_HPP
#define BOOST_HANA_FOR_EACH_HPP

#include <boost/hana/fwd/for_each.hpp>

#include <boost/hana/concept/foldable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/unpack.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename F>
    constexpr void for_each_t::operator()(Xs&& xs, F&& f) const {
        using S = typename hana::tag_of<Xs>::type;
        using ForEach = BOOST_HANA_DISPATCH_IF(for_each_impl<S>,
            hana::Foldable<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Foldable<S>::value,
        "hana::for_each(xs, f) requires 'xs' to be Foldable");
    #endif

        return ForEach::apply(static_cast<Xs&&>(xs), static_cast<F&&>(f));
    }
    //! @endcond

    namespace detail {
        template <typename F>
        struct on_each {
            F f;
            template <typename ...Xs>
            constexpr void operator()(Xs&& ...xs) const {
                using Swallow = int[];
                (void)Swallow{0, ((void)(*f)(static_cast<Xs&&>(xs)), 0)...};
            }
        };
    }

    template <typename T, bool condition>
    struct for_each_impl<T, when<condition>> : default_ {
        template <typename Xs, typename F>
        static constexpr void apply(Xs&& xs, F&& f) {
            // We use a pointer instead of a reference to avoid a Clang ICE.
            hana::unpack(static_cast<Xs&&>(xs),
                         detail::on_each<decltype(&f)>{&f});
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FOR_EACH_HPP
