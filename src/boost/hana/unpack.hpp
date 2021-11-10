/*!
@file
Defines `boost::hana::unpack`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_UNPACK_HPP
#define BOOST_HANA_UNPACK_HPP

#include <boost/hana/fwd/unpack.hpp>

#include <boost/hana/accessors.hpp>
#include <boost/hana/at.hpp>
#include <boost/hana/concept/foldable.hpp>
#include <boost/hana/concept/iterable.hpp>
#include <boost/hana/concept/struct.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/first.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/fwd/fold_left.hpp>
#include <boost/hana/length.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/second.hpp>

#include <cstddef>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //! @cond
    template <typename Xs, typename F>
    constexpr decltype(auto) unpack_t::operator()(Xs&& xs, F&& f) const {
        using S = typename hana::tag_of<Xs>::type;
        using Unpack = BOOST_HANA_DISPATCH_IF(unpack_impl<S>,
            hana::Foldable<S>::value
        );

    #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
        static_assert(hana::Foldable<S>::value,
        "hana::unpack(xs, f) requires 'xs' to be Foldable");
    #endif

        return Unpack::apply(static_cast<Xs&&>(xs), static_cast<F&&>(f));
    }
    //! @endcond

    template <typename T, bool condition>
    struct unpack_impl<T, when<condition>> : default_ {
        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return hana::fold_left(static_cast<Xs&&>(xs),
                                   static_cast<F&&>(f),
                                   hana::partial)();
        }
    };

    template <typename It>
    struct unpack_impl<It, when<
        hana::Iterable<It>::value && !is_default<length_impl<It>>::value
    >> {
        template <typename Xs, typename F, std::size_t ...i>
        static constexpr decltype(auto)
        unpack_helper(Xs&& xs, F&& f, std::index_sequence<i...>) {
            return static_cast<F&&>(f)(hana::at_c<i>(static_cast<Xs&&>(xs))...);
        }

        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            constexpr std::size_t N = decltype(hana::length(xs))::value;
            return unpack_helper(static_cast<Xs&&>(xs), static_cast<F&&>(f),
                                 std::make_index_sequence<N>{});
        }
    };

    template <typename T, std::size_t N>
    struct unpack_impl<T[N]> {
        template <typename Xs, typename F, std::size_t ...i>
        static constexpr decltype(auto)
        unpack_helper(Xs&& xs, F&& f, std::index_sequence<i...>) {
            return static_cast<F&&>(f)(static_cast<Xs&&>(xs)[i]...);
        }

        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return unpack_impl::unpack_helper(static_cast<Xs&&>(xs),
                                              static_cast<F&&>(f),
                                              std::make_index_sequence<N>{});
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Model for Products
    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct unpack_impl<T, when<hana::Product<T>::value>> {
        template <typename P, typename F>
        static constexpr decltype(auto) apply(P&& p, F&& f) {
            return static_cast<F&&>(f)(
                hana::first(static_cast<P&&>(p)),
                hana::second(static_cast<P&&>(p))
            );
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Model for Structs
    //////////////////////////////////////////////////////////////////////////
    namespace struct_detail {
        // This is equivalent to `demux`, except that `demux` can't forward
        // the `udt` because it does not know the `g`s are accessors. Hence,
        // this can result in faster code.
        struct almost_demux {
            template <typename F, typename Udt, typename ...Members>
            constexpr decltype(auto)
            operator()(F&& f, Udt&& udt, Members&& ...g) const {
                return static_cast<F&&>(f)(hana::make_pair(
                    hana::first(static_cast<Members&&>(g)),
                    hana::second(static_cast<Members&&>(g))
                                                (static_cast<Udt&&>(udt))
                )...);
            }
        };
    }

    template <typename S>
    struct unpack_impl<S, when<hana::Struct<S>::value>> {
        template <typename Udt, typename F>
        static constexpr decltype(auto) apply(Udt&& udt, F&& f) {
            return hana::unpack(hana::accessors<S>(),
                hana::partial(struct_detail::almost_demux{},
                              static_cast<F&&>(f),
                              static_cast<Udt&&>(udt)));
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_UNPACK_HPP
