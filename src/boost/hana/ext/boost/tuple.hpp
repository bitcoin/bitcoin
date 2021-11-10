/*!
@file
Adapts `boost::tuple` for use with Hana.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_EXT_BOOST_TUPLE_HPP
#define BOOST_HANA_EXT_BOOST_TUPLE_HPP

#include <boost/hana/bool.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/fwd/at.hpp>
#include <boost/hana/fwd/core/make.hpp>
#include <boost/hana/fwd/core/tag_of.hpp>
#include <boost/hana/fwd/drop_front.hpp>
#include <boost/hana/fwd/is_empty.hpp>
#include <boost/hana/fwd/length.hpp>
#include <boost/hana/integral_constant.hpp>

#include <boost/tuple/tuple.hpp>

#include <cstddef>
#include <utility>


#ifdef BOOST_HANA_DOXYGEN_INVOKED
namespace boost {
    //! @ingroup group-ext-boost
    //! Adapter for `boost::tuple`s.
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! A `boost::tuple` is a model of the `Sequence` concept, and all the
    //! concepts it refines. That makes it essentially the same as a Hana
    //! tuple, although the complexity of some operations might differ from
    //! that of Hana's tuple.
    //!
    //! @include example/ext/boost/tuple.cpp
    template <typename ...T>
    struct tuple { };
}
#endif


BOOST_HANA_NAMESPACE_BEGIN
    namespace ext { namespace boost { struct tuple_tag; }}

    template <typename ...Xs>
    struct tag_of<boost::tuple<Xs...>> {
        using type = ext::boost::tuple_tag;
    };

    template <typename H, typename T>
    struct tag_of<boost::tuples::cons<H, T>> {
        using type = ext::boost::tuple_tag;
    };

    template <>
    struct tag_of<boost::tuples::null_type> {
        using type = ext::boost::tuple_tag;
    };

    //////////////////////////////////////////////////////////////////////////
    // Iterable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct at_impl<ext::boost::tuple_tag> {
        template <typename Xs, typename N>
        static constexpr decltype(auto) apply(Xs&& xs, N const&) {
            constexpr std::size_t n = N::value;
            return static_cast<Xs&&>(xs).template get<n>();
        }
    };

    template <>
    struct drop_front_impl<ext::boost::tuple_tag> {
        template <std::size_t n, typename Xs, std::size_t ...i>
        static constexpr auto drop_front_helper(Xs&& xs, std::index_sequence<i...>) {
            return hana::make<ext::boost::tuple_tag>(
                hana::at_c<n + i>(static_cast<Xs&&>(xs))...
            );
        }

        template <typename Xs, typename N>
        static constexpr auto apply(Xs&& xs, N const&) {
            constexpr std::size_t n = N::value;
            constexpr std::size_t len = decltype(hana::length(xs))::value;
            return drop_front_helper<n>(static_cast<Xs&&>(xs),
                    std::make_index_sequence<(n < len ? len - n : 0)>{});
        }
    };

    template <>
    struct is_empty_impl<ext::boost::tuple_tag> {
        static constexpr auto apply(boost::tuples::null_type const&)
        { return hana::true_c; }

        template <typename H, typename T>
        static constexpr auto apply(boost::tuples::cons<H, T> const&)
        { return hana::false_c; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct length_impl<ext::boost::tuple_tag> {
        template <typename Xs>
        static constexpr auto apply(Xs const&) {
            return hana::size_c<boost::tuples::length<Xs>::value>;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Sequence
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct Sequence<ext::boost::tuple_tag> {
        static constexpr bool value = true;
    };

    template <>
    struct make_impl<ext::boost::tuple_tag> {
        template <typename ...Xs>
        static constexpr auto apply(Xs&& ...xs) {
            return boost::tuples::tuple<
                typename detail::decay<Xs>::type...
            >{static_cast<Xs&&>(xs)...};
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_EXT_BOOST_TUPLE_HPP
