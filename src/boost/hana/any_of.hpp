/*!
@file
Defines `boost::hana::any_of`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_ANY_OF_HPP
#define BOOST_HANA_ANY_OF_HPP

#include <boost/hana/fwd/any_of.hpp>

#include <boost/hana/accessors.hpp>
#include <boost/hana/at.hpp>
#include <boost/hana/bool.hpp>
#include <boost/hana/concept/searchable.hpp>
#include <boost/hana/concept/sequence.hpp>
#include <boost/hana/concept/struct.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/drop_front.hpp>
#include <boost/hana/first.hpp>
#include <boost/hana/front.hpp>
#include <boost/hana/functional/compose.hpp>
#include <boost/hana/if.hpp>
#include <boost/hana/is_empty.hpp>
#include <boost/hana/length.hpp>

#include <cstddef>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename Pred>
    constexpr auto any_of_t::operator()(Xs&& xs, Pred&& pred) const {
        using S = typename hana::tag_of<Xs>::type;
        using AnyOf = BOOST_HANA_DISPATCH_IF(any_of_impl<S>,
            hana::Searchable<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Searchable<S>::value,
        "hana::any_of(xs, pred) requires 'xs' to be a Searchable");
    #endif

        return AnyOf::apply(static_cast<Xs&&>(xs), static_cast<Pred&&>(pred));
    }
    //! @endcond

    template <typename S, bool condition>
    struct any_of_impl<S, when<condition>> : default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...) = delete;
    };

    template <typename S>
    struct any_of_impl<S, when<Sequence<S>::value>> {
        //! @cond
        template <std::size_t k, std::size_t Len>
        struct any_of_helper {
            template <typename Xs, typename Pred>
            static constexpr auto apply(bool prev_cond, Xs&& xs, Pred&& pred) {
                return prev_cond ? hana::true_c
                    : any_of_impl::any_of_helper<k + 1, Len>::apply(
                        hana::if_(pred(hana::at_c<k>(xs)), hana::true_c, hana::false_c),
                        static_cast<Xs&&>(xs),
                        static_cast<Pred&&>(pred)
                    );
            }

            template <typename Xs, typename Pred>
            static constexpr auto apply(hana::true_, Xs&&, Pred&&)
            { return hana::true_c; }

            template <typename Xs, typename Pred>
            static constexpr auto apply(hana::false_, Xs&& xs, Pred&& pred) {
                auto cond = hana::if_(pred(hana::at_c<k>(xs)), hana::true_c,
                                                               hana::false_c);
                return any_of_impl::any_of_helper<k + 1, Len>::apply(cond,
                                        static_cast<Xs&&>(xs),
                                        static_cast<Pred&&>(pred));
            }
        };

        template <std::size_t Len>
        struct any_of_helper<Len, Len> {
            template <typename Cond, typename Xs, typename Pred>
            static constexpr auto apply(Cond cond, Xs&&, Pred&&)
            { return cond; }
        };

        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs&& xs, Pred&& pred) {
            constexpr std::size_t len = decltype(hana::length(xs))::value;
            return any_of_impl::any_of_helper<0, len>::apply(hana::false_c,
                                            static_cast<Xs&&>(xs),
                                            static_cast<Pred&&>(pred));
        }
        //! @endcond
    };

    template <typename It>
    struct any_of_impl<It, when<
        hana::Iterable<It>::value &&
        !Sequence<It>::value
    >> {
        template <typename Xs, typename Pred>
        static constexpr auto lazy_any_of_helper(hana::false_, bool prev_cond, Xs&& xs, Pred&& pred) {
            decltype(auto) tail = hana::drop_front(static_cast<Xs&&>(xs));
            constexpr bool done = decltype(hana::is_empty(tail))::value;
            return prev_cond ? hana::true_c
                : lazy_any_of_helper(hana::bool_<done>{},
                    hana::if_(pred(hana::front(xs)), hana::true_{}, hana::false_{}),
                    static_cast<decltype(tail)&&>(tail),
                    static_cast<Pred&&>(pred)
                );
        }

        template <typename Xs, typename Pred>
        static constexpr auto lazy_any_of_helper(hana::false_, hana::true_, Xs&&, Pred&&)
        { return hana::true_c; }

        template <typename Xs, typename Pred>
        static constexpr auto lazy_any_of_helper(hana::false_, hana::false_, Xs&& xs, Pred&& pred) {
            constexpr bool done = decltype(hana::is_empty(hana::drop_front(xs)))::value;
            return lazy_any_of_helper(hana::bool_c<done>,
                hana::if_(pred(hana::front(xs)), hana::true_c, hana::false_c),
                hana::drop_front(static_cast<Xs&&>(xs)),
                static_cast<Pred&&>(pred)
            );
        }

        template <typename Cond, typename Xs, typename Pred>
        static constexpr auto lazy_any_of_helper(hana::true_, Cond cond, Xs&&, Pred&&)
        { return cond; }

        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs&& xs, Pred&& pred) {
            constexpr bool done = decltype(hana::is_empty(xs))::value;
            return lazy_any_of_helper(hana::bool_c<done>, hana::false_c,
                                      static_cast<Xs&&>(xs),
                                      static_cast<Pred&&>(pred));
        }
    };

    template <typename T, std::size_t N>
    struct any_of_impl<T[N]> {
        template <typename Xs, typename Pred>
        static constexpr bool any_of_helper(bool cond, Xs&& xs, Pred&& pred) {
            if (cond) return true;
            for (std::size_t i = 1; i < N; ++i)
                if (pred(static_cast<Xs&&>(xs)[i]))
                    return true;
            return false;
        }

        // Since an array contains homogeneous data, if the predicate returns
        // a compile-time logical at any index, it must do so at every index
        // (because the type of the elements won't change)! In this case, we
        // then only need to evaluate the predicate on the first element.
        template <typename Xs, typename Pred>
        static constexpr auto
        any_of_helper(hana::true_, Xs&& /*xs*/, Pred&&)
        { return hana::true_c; }

        template <typename Xs, typename Pred>
        static constexpr auto
        any_of_helper(hana::false_, Xs&&, Pred&&)
        { return hana::false_c; }

        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs&& xs, Pred&& pred) {
            auto cond = hana::if_(pred(static_cast<Xs&&>(xs)[0]), hana::true_c,
                                                                  hana::false_c);
            return any_of_helper(cond, static_cast<Xs&&>(xs),
                                       static_cast<Pred&&>(pred));
        }
    };

    template <typename S>
    struct any_of_impl<S, when<hana::Struct<S>::value>> {
        template <typename X, typename Pred>
        static constexpr decltype(auto) apply(X const&, Pred&& pred) {
            return hana::any_of(hana::accessors<S>(),
                    hana::compose(static_cast<Pred&&>(pred), hana::first));
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_ANY_OF_HPP
