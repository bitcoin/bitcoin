/*!
@file
Adapts `boost::mpl::list` for use with Hana.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_EXT_BOOST_MPL_LIST_HPP
#define BOOST_HANA_EXT_BOOST_MPL_LIST_HPP

#include <boost/hana/concept/foldable.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>
#include <boost/hana/ext/boost/mpl/integral_c.hpp>
#include <boost/hana/fwd/at.hpp>
#include <boost/hana/fwd/core/to.hpp>
#include <boost/hana/fwd/core/tag_of.hpp>
#include <boost/hana/fwd/drop_front.hpp>
#include <boost/hana/fwd/equal.hpp>
#include <boost/hana/fwd/is_empty.hpp>
#include <boost/hana/fwd/less.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/length.hpp>
#include <boost/hana/type.hpp>
#include <boost/hana/unpack.hpp>

#include <boost/mpl/at.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/list.hpp>
#include <boost/mpl/sequence_tag.hpp>
#include <boost/mpl/size.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>


#ifdef BOOST_HANA_DOXYGEN_INVOKED
namespace boost { namespace mpl {
    //! @ingroup group-ext-mpl
    //! Adapter for Boost.MPL lists.
    //!
    //!
    //! Modeled concepts
    //! ----------------
    //! It is possible for MPL lists to model a couple of concepts.
    //! However, because they are only able to hold types, they lack
    //! the generality required to model concepts like `Functor`,
    //! `Sequence` and other related concepts.
    //!
    //! 1. `Comparable`\n
    //! Two MPL lists are equal if and only if they contain the same
    //! number of types, and if all those types are equal.
    //! @include example/ext/boost/mpl/list/comparable.cpp
    //!
    //! 2. `Foldable`\n
    //! Folding a MPL list is equivalent to folding it as a `Sequence`.
    //! @include example/ext/boost/mpl/list/foldable.cpp
    //!
    //! 3. `Iterable`\n
    //! Iterating over a MPL list is just iterating over each of the
    //! types it contains, as if it were a `Sequence`.
    //! @include example/ext/boost/mpl/list/iterable.cpp
    //!
    //! 4. `Searchable`\n
    //! A MPL list can be searched as if it were a tuple containing
    //! `hana::type`s.
    //! @include example/ext/boost/mpl/list/searchable.cpp
    //!
    //!
    //! Conversion from any `Foldable`
    //! ------------------------------
    //! A MPL list can be created from any `Foldable`. More precisely,
    //! for a `Foldable` `xs` whose linearization is `[x1, ..., xn]`,
    //! @code
    //!     to<ext::boost::mpl::list_tag>(xs) == mpl::list<t1, ..., tn>{}
    //! @endcode
    //! where `tk` is the type of `xk`, or the type contained in `xk` if
    //! `xk` is a `hana::type`.
    //! @warning
    //! The limitations on the size of `mpl::list`s are inherited by
    //! this conversion utility, and hence trying to convert a `Foldable`
    //! containing more than [BOOST_MPL_LIMIT_LIST_SIZE][1] elements is
    //! an error.
    //! @include example/ext/boost/mpl/list/conversion.cpp
    //!
    //! [1]: http://www.boost.org/doc/libs/release/libs/mpl/doc/refmanual/limit-list-size.html
    template <typename ...T>
    struct list { };
}}
#endif


BOOST_HANA_NAMESPACE_BEGIN
    namespace ext { namespace boost { namespace mpl {
        using list_tag = ::boost::mpl::sequence_tag< ::boost::mpl::list<>>::type;
    }}}

    template <typename T>
    struct tag_of<T, when<
        std::is_same<
            typename ::boost::mpl::sequence_tag<T>::type,
            ::boost::mpl::sequence_tag< ::boost::mpl::list<>>::type
        >::value
    >> {
        using type = ext::boost::mpl::list_tag;
    };

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct equal_impl<ext::boost::mpl::list_tag, ext::boost::mpl::list_tag> {
        template <typename Xs, typename Ys>
        static constexpr auto apply(Xs const&, Ys const&) {
            return typename ::boost::mpl::equal<Xs, Ys>::type{};
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct length_impl<ext::boost::mpl::list_tag> {
        template <typename Xs>
        static constexpr auto apply(Xs const&) {
            return hana::size_c< ::boost::mpl::size<Xs>::type::value>;
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Iterable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct at_impl<ext::boost::mpl::list_tag> {
        template <typename Ts, typename N>
        static constexpr auto apply(Ts const&, N const&) {
            constexpr std::size_t n = N::value;
            using T = typename ::boost::mpl::at_c<Ts, n>::type;
            return hana::type_c<T>;
        }
    };

    template <>
    struct drop_front_impl<ext::boost::mpl::list_tag> {
        template <std::size_t n, typename Xs, std::size_t ...i>
        static constexpr auto drop_front_helper(Xs const&, std::index_sequence<i...>) {
            return boost::mpl::list<
                typename boost::mpl::at_c<Xs, n + i>::type...
            >{};
        }

        template <typename Xs, typename N>
        static constexpr auto apply(Xs const& xs, N const&) {
            constexpr std::size_t n = N::value;
            constexpr std::size_t len = decltype(hana::length(xs))::value;
            return drop_front_helper<n>(xs,
                    std::make_index_sequence<(n < len ? len - n : 0)>{});
        }
    };

    template <>
    struct is_empty_impl<ext::boost::mpl::list_tag> {
        template <typename Xs>
        static constexpr auto apply(Xs const&)
        { return typename ::boost::mpl::empty<Xs>::type{}; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Conversion from a Foldable
    //////////////////////////////////////////////////////////////////////////
    template <typename F>
    struct to_impl<ext::boost::mpl::list_tag, F, when<hana::Foldable<F>::value>> {
        template <typename Xs>
        static constexpr decltype(auto) apply(Xs&& xs) {
            auto list_type = hana::unpack(static_cast<Xs&&>(xs),
                                          hana::template_<::boost::mpl::list>);
            return typename decltype(list_type)::type{};
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_EXT_BOOST_MPL_LIST_HPP
