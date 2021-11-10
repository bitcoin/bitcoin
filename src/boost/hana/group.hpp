/*!
@file
Defines `boost::hana::group`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_GROUP_HPP
#define BOOST_HANA_GROUP_HPP

#include <boost/hana/fwd/group.hpp>

#include <boost/hana/at.hpp>
#include <boost/hana/concept/sequence.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/detail/algorithm.hpp>
#include <boost/hana/detail/array.hpp>
#include <boost/hana/detail/nested_by.hpp> // required by fwd decl
#include <boost/hana/equal.hpp>
#include <boost/hana/length.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs>
    constexpr auto group_t::operator()(Xs&& xs) const {
        using S = typename hana::tag_of<Xs>::type;
        using Group = BOOST_HANA_DISPATCH_IF(group_impl<S>,
            hana::Sequence<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Sequence<S>::value,
        "hana::group(xs) requires 'xs' to be a Sequence");
    #endif

        return Group::apply(static_cast<Xs&&>(xs));
    }

    template <typename Xs, typename Predicate>
    constexpr auto group_t::operator()(Xs&& xs, Predicate&& pred) const {
        using S = typename hana::tag_of<Xs>::type;
        using Group = BOOST_HANA_DISPATCH_IF(group_impl<S>,
            hana::Sequence<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Sequence<S>::value,
        "hana::group(xs, predicate) requires 'xs' to be a Sequence");
    #endif

        return Group::apply(static_cast<Xs&&>(xs),
                            static_cast<Predicate&&>(pred));
    }
    //! @endcond

    namespace detail {
        template <typename Xs, std::size_t ...i>
        constexpr auto get_subsequence_(Xs&& xs, std::index_sequence<i...>) {
            using S = typename hana::tag_of<Xs>::type;
            return hana::make<S>(hana::at_c<i>(static_cast<Xs&&>(xs))...);
        }

        template <std::size_t offset, typename Indices>
        struct offset_by;

        template <std::size_t offset, std::size_t ...i>
        struct offset_by<offset, std::index_sequence<i...>> {
            using type = std::index_sequence<(offset + i)...>;
        };

        template <bool ...b>
        struct group_indices {
            static constexpr bool bs[sizeof...(b)] = {b...};
            static constexpr std::size_t n_groups =
                    detail::count(bs, bs + sizeof(bs), false) + 1;

            static constexpr auto compute_info() {
                detail::array<std::size_t, n_groups> sizes{}, offsets{};
                for (std::size_t g = 0, i = 0, offset = 0; g < n_groups; ++g) {
                    offsets[g] = offset;

                    sizes[g] = 1;
                    while (i < sizeof...(b) && bs[i++])
                        ++sizes[g];

                    offset += sizes[g];
                }
                return std::make_pair(offsets, sizes);
            }

            static constexpr auto info = compute_info();
            static constexpr auto group_offsets = info.first;
            static constexpr auto group_sizes = info.second;

            template <typename S, typename Xs, std::size_t ...i>
            static constexpr auto finish(Xs&& xs, std::index_sequence<i...>) {
                return hana::make<S>(
                    detail::get_subsequence_(
                        static_cast<Xs&&>(xs),
                        typename offset_by<
                            group_offsets[i],
                            std::make_index_sequence<group_sizes[i]>
                        >::type{}
                    )...
                );
            }
        };
    } // end namespace detail

    template <typename S, bool condition>
    struct group_impl<S, when<condition>> : default_ {
        template <typename Xs, typename Pred, std::size_t ...i>
        static constexpr auto
        group_helper(Xs&& xs, Pred&& pred, std::index_sequence<0, i...>) {
            using info = detail::group_indices<static_cast<bool>(decltype(
                pred(hana::at_c<i - 1>(static_cast<Xs&&>(xs)),
                     hana::at_c<i>(static_cast<Xs&&>(xs)))
            )::value)...>;
            return info::template finish<S>(static_cast<Xs&&>(xs),
                std::make_index_sequence<info::n_groups>{}
            );
        }

        template <typename Xs, typename Pred>
        static constexpr auto
        group_helper(Xs&& xs, Pred&&, std::index_sequence<0>) {
            return hana::make<S>(static_cast<Xs&&>(xs));
        }

        template <typename Xs, typename Pred>
        static constexpr auto
        group_helper(Xs&&, Pred&&, std::index_sequence<>) {
            return hana::make<S>();
        }

        template <typename Xs, typename Pred>
        static constexpr auto apply(Xs&& xs, Pred&& pred) {
            constexpr std::size_t len = decltype(hana::length(xs))::value;
            return group_helper(static_cast<Xs&&>(xs),
                                static_cast<Pred&&>(pred),
                                std::make_index_sequence<len>{});
        }

        template <typename Xs>
        static constexpr auto apply(Xs&& xs)
        { return group_impl::apply(static_cast<Xs&&>(xs), hana::equal); }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_GROUP_HPP
