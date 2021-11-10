/*!
@file
Defines `boost::hana::find_if`.

@copyright Louis Dionne 2013-2017
@copyright Jason Rice 2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FIND_IF_HPP
#define BOOST_HANA_FIND_IF_HPP

#include <boost/hana/fwd/find_if.hpp>

#include <boost/hana/accessors.hpp>
#include <boost/hana/at.hpp>
#include <boost/hana/bool.hpp>
#include <boost/hana/concept/iterable.hpp>
#include <boost/hana/concept/searchable.hpp>
#include <boost/hana/concept/struct.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/first.hpp>
#include <boost/hana/functional/compose.hpp>
#include <boost/hana/index_if.hpp>
#include <boost/hana/second.hpp>
#include <boost/hana/transform.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Pred>
    constexpr auto find_if_t::operator()(Xs&& xs, Pred&& pred) const {
        using S = typename hana::tag_of<Xs>::type;
        using FindIf = BOOST_HANA_DISPATCH_IF(find_if_impl<S>,
            hana::Searchable<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Searchable<S>::value,
        "hana::find_if(xs, pred) requires 'xs' to be a Searchable");
    #endif

        return FindIf::apply(static_cast<Xs&&>(xs), static_cast<Pred&&>(pred));
    }
    //! @endcond

    template <typename S, bool condition>
    struct find_if_impl<S, when<condition>> : default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...) = delete;
    };

    namespace detail {
        template <typename Xs>
        struct partial_at {
            Xs const& xs;

            template <typename I>
            constexpr decltype(auto) operator()(I i) const {
                return hana::at(xs, i);
            }
        };
    }

    template <typename Tag>
    struct find_if_impl<Tag, when<Iterable<Tag>::value>> {
        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs&& xs, Pred&& pred) {
            using Result = decltype(hana::index_if(
                static_cast<Xs&&>(xs), static_cast<Pred&&>(pred)));

            return hana::transform(Result{},
                detail::partial_at<std::decay_t<Xs>>{static_cast<Xs&&>(xs)});
        }
    };

    template <typename T, std::size_t N>
    struct find_if_impl<T[N]> {
        template <typename Xs>
        static constexpr auto find_if_helper(Xs&&, hana::false_)
        { return hana::nothing; }

        template <typename Xs>
        static constexpr auto find_if_helper(Xs&& xs, hana::true_)
        { return hana::just(static_cast<Xs&&>(xs)[0]); }

        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs&& xs, Pred&& pred) {
            return find_if_helper(static_cast<Xs&&>(xs),
                hana::bool_c<decltype(
                    static_cast<Pred&&>(pred)(static_cast<Xs&&>(xs)[0])
                )::value>
            );
        }
    };

    namespace struct_detail {
        template <typename X>
        struct get_member {
            X x;
            template <typename Member>
            constexpr decltype(auto) operator()(Member&& member) && {
                return hana::second(static_cast<Member&&>(member))(
                    static_cast<X&&>(x)
                );
            }
        };
    }

    template <typename S>
    struct find_if_impl<S, when<hana::Struct<S>::value>> {
        template <typename X, typename Pred>
        static constexpr decltype(auto) apply(X&& x, Pred&& pred) {
            return hana::transform(
                hana::find_if(hana::accessors<S>(),
                    hana::compose(static_cast<Pred&&>(pred), hana::first)
                ),
                struct_detail::get_member<X>{static_cast<X&&>(x)}
            );
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FIND_IF_HPP
