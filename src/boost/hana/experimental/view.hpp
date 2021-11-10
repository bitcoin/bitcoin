/*
@file
Defines experimental views.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_EXPERIMENTAL_VIEW_HPP
#define BOOST_HANA_EXPERIMENTAL_VIEW_HPP

#include <boost/hana/and.hpp>
#include <boost/hana/at.hpp>
#include <boost/hana/bool.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/fold_left.hpp>
#include <boost/hana/functional/compose.hpp>
#include <boost/hana/functional/on.hpp>
#include <boost/hana/fwd/ap.hpp>
#include <boost/hana/fwd/concat.hpp>
#include <boost/hana/fwd/drop_front.hpp>
#include <boost/hana/fwd/empty.hpp>
#include <boost/hana/fwd/equal.hpp>
#include <boost/hana/fwd/flatten.hpp>
#include <boost/hana/fwd/is_empty.hpp>
#include <boost/hana/fwd/less.hpp>
#include <boost/hana/fwd/lift.hpp>
#include <boost/hana/fwd/transform.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/length.hpp>
#include <boost/hana/lexicographical_compare.hpp>
#include <boost/hana/range.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/unpack.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>


// Pros of views
//     - No temporary container created between algorithms
//     - Lazy, so only the minimum is required
//
// Cons of views
//     - Reference semantics mean possibility for dangling references
//     - Lose the ability to move from temporary containers
//     - When fetching the members of a view multiple times, no caching is done.
//       So for example, `t = transform(xs, f); at_c<0>(t); at_c<0>(t)` will
//       compute `f(at_c<0>(xs))` twice.
//     - push_back creates a joint_view and a single_view. The single_view holds
//       the value as a member. When doing multiple push_backs, we end up with a
//         joint_view<xxx, joint_view<single_view<T>, joint_view<single_view<T>, ....>>>
//       which contains a reference to `xxx` and all the `T`s by value. Such a
//       "view" is not cheap to copy, which is inconsistent with the usual
//       expectations about views.

BOOST_HANA_NAMESPACE_BEGIN

namespace experimental {
    struct view_tag;

    namespace detail {
        template <typename Sequence>
        struct is_view {
            static constexpr bool value = false;
        };

        template <typename Sequence>
        using view_storage = typename std::conditional<
            detail::is_view<Sequence>::value, Sequence, Sequence&
        >::type;
    }

    //////////////////////////////////////////////////////////////////////////
    // sliced_view
    //////////////////////////////////////////////////////////////////////////
    template <typename Sequence, std::size_t ...indices>
    struct sliced_view_t {
        detail::view_storage<Sequence> sequence_;
        using hana_tag = view_tag;
    };

    template <typename Sequence, typename Indices>
    constexpr auto sliced(Sequence& sequence, Indices const& indices) {
        return hana::unpack(indices, [&](auto ...i) {
            return sliced_view_t<Sequence, decltype(i)::value...>{sequence};
        });
    }

    namespace detail {
        template <typename Sequence, std::size_t ...i>
        struct is_view<sliced_view_t<Sequence, i...>> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // transformed_view
    //////////////////////////////////////////////////////////////////////////
    template <typename Sequence, typename F>
    struct transformed_view_t {
        detail::view_storage<Sequence> sequence_;
        F f_;
        using hana_tag = view_tag;
    };

    template <typename Sequence, typename F>
    constexpr transformed_view_t<Sequence, typename hana::detail::decay<F>::type>
    transformed(Sequence& sequence, F&& f) {
        return {sequence, static_cast<F&&>(f)};
    }

    namespace detail {
        template <typename Sequence, typename F>
        struct is_view<transformed_view_t<Sequence, F>> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // filtered_view
    //////////////////////////////////////////////////////////////////////////
#if 0
    template <typename Sequence, typename Pred>
    using filtered_view_t = sliced_view_t<Sequence, detail::filtered_indices<...>>;

    template <typename Sequence, typename Pred>
    constexpr filtered_view_t<Sequence, Pred> filtered(Sequence& sequence, Pred&& pred) {
        return {sequence};
    }
#endif

    //////////////////////////////////////////////////////////////////////////
    // joined_view
    //////////////////////////////////////////////////////////////////////////
    template <typename Sequence1, typename Sequence2>
    struct joined_view_t {
        detail::view_storage<Sequence1> sequence1_;
        detail::view_storage<Sequence2> sequence2_;
        using hana_tag = view_tag;
    };

    struct make_joined_view_t {
        template <typename Sequence1, typename Sequence2>
        constexpr joined_view_t<Sequence1, Sequence2> operator()(Sequence1& s1, Sequence2& s2) const {
            return {s1, s2};
        }
    };
    BOOST_HANA_INLINE_VARIABLE constexpr make_joined_view_t joined{};

    namespace detail {
        template <typename Sequence1, typename Sequence2>
        struct is_view<joined_view_t<Sequence1, Sequence2>> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // single_view
    //////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct single_view_t {
        T value_;
        using hana_tag = view_tag;
    };

    template <typename T>
    constexpr single_view_t<typename hana::detail::decay<T>::type> single_view(T&& t) {
        return {static_cast<T&&>(t)};
    }

    namespace detail {
        template <typename T>
        struct is_view<single_view_t<T>> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // empty_view
    //////////////////////////////////////////////////////////////////////////
    struct empty_view_t {
        using hana_tag = view_tag;
    };

    constexpr empty_view_t empty_view() {
        return {};
    }

    namespace detail {
        template <>
        struct is_view<empty_view_t> {
            static constexpr bool value = true;
        };
    }
} // end namespace experimental

//////////////////////////////////////////////////////////////////////////
// Foldable
//////////////////////////////////////////////////////////////////////////
template <>
struct unpack_impl<experimental::view_tag> {
    // sliced_view
    template <typename Sequence, std::size_t ...i, typename F>
    static constexpr decltype(auto)
    apply(experimental::sliced_view_t<Sequence, i...> view, F&& f) {
        (void)view; // Remove spurious unused variable warning with GCC
        return static_cast<F&&>(f)(hana::at_c<i>(view.sequence_)...);
    }

    // transformed_view
    template <typename Sequence, typename F, typename G>
    static constexpr decltype(auto)
    apply(experimental::transformed_view_t<Sequence, F> view, G&& g) {
        return hana::unpack(view.sequence_, hana::on(static_cast<G&&>(g), view.f_));
    }

    // joined_view
    template <typename View, typename F, std::size_t ...i1, std::size_t ...i2>
    static constexpr decltype(auto)
    unpack_joined(View view, F&& f, std::index_sequence<i1...>,
                                    std::index_sequence<i2...>)
    {
        (void)view; // Remove spurious unused variable warning with GCC
        return static_cast<F&&>(f)(hana::at_c<i1>(view.sequence1_)...,
                                   hana::at_c<i2>(view.sequence2_)...);
    }

    template <typename S1, typename S2, typename F>
    static constexpr decltype(auto)
    apply(experimental::joined_view_t<S1, S2> view, F&& f) {
        constexpr auto N1 = decltype(hana::length(view.sequence1_))::value;
        constexpr auto N2 = decltype(hana::length(view.sequence2_))::value;
        return unpack_joined(view, static_cast<F&&>(f),
                             std::make_index_sequence<N1>{},
                             std::make_index_sequence<N2>{});
    }

    // single_view
    template <typename T, typename F>
    static constexpr decltype(auto) apply(experimental::single_view_t<T> view, F&& f) {
        return static_cast<F&&>(f)(view.value_);
    }

    // empty_view
    template <typename F>
    static constexpr decltype(auto) apply(experimental::empty_view_t, F&& f) {
        return static_cast<F&&>(f)();
    }
};

//////////////////////////////////////////////////////////////////////////
// Iterable
//////////////////////////////////////////////////////////////////////////
template <>
struct at_impl<experimental::view_tag> {
    // sliced_view
    template <typename Sequence, std::size_t ...i, typename N>
    static constexpr decltype(auto)
    apply(experimental::sliced_view_t<Sequence, i...> view, N const&) {
        constexpr std::size_t indices[] = {i...};
        constexpr std::size_t n = indices[N::value];
        return hana::at_c<n>(view.sequence_);
    }

    // transformed_view
    template <typename Sequence, typename F, typename N>
    static constexpr decltype(auto)
    apply(experimental::transformed_view_t<Sequence, F> view, N const& n) {
        return view.f_(hana::at(view.sequence_, n));
    }

    // joined_view
    template <std::size_t Left, typename View, typename N>
    static constexpr decltype(auto) at_joined_view(View view, N const&, hana::true_) {
        return hana::at_c<N::value>(view.sequence1_);
    }

    template <std::size_t Left, typename View, typename N>
    static constexpr decltype(auto) at_joined_view(View view, N const&, hana::false_) {
        return hana::at_c<N::value - Left>(view.sequence2_);
    }

    template <typename S1, typename S2, typename N>
    static constexpr decltype(auto)
    apply(experimental::joined_view_t<S1, S2> view, N const& n) {
        constexpr auto Left = decltype(hana::length(view.sequence1_))::value;
        return at_joined_view<Left>(view, n, hana::bool_c<(N::value < Left)>);
    }

    // single_view
    template <typename T, typename N>
    static constexpr decltype(auto) apply(experimental::single_view_t<T> view, N const&) {
        static_assert(N::value == 0,
        "trying to fetch an out-of-bounds element in a hana::single_view");
        return view.value_;
    }

    // empty_view
    template <typename N>
    static constexpr decltype(auto) apply(experimental::empty_view_t, N const&) = delete;
};

template <>
struct length_impl<experimental::view_tag> {
    // sliced_view
    template <typename Sequence, std::size_t ...i>
    static constexpr auto
    apply(experimental::sliced_view_t<Sequence, i...>) {
        return hana::size_c<sizeof...(i)>;
    }

    // transformed_view
    template <typename Sequence, typename F>
    static constexpr auto apply(experimental::transformed_view_t<Sequence, F> view) {
        return hana::length(view.sequence_);
    }

    // joined_view
    template <typename S1, typename S2>
    static constexpr auto apply(experimental::joined_view_t<S1, S2> view) {
        return hana::size_c<
            decltype(hana::length(view.sequence1_))::value +
            decltype(hana::length(view.sequence2_))::value
        >;
    }

    // single_view
    template <typename T>
    static constexpr auto apply(experimental::single_view_t<T>) {
        return hana::size_c<1>;
    }

    // empty_view
    static constexpr auto apply(experimental::empty_view_t) {
        return hana::size_c<0>;
    }
};

template <>
struct is_empty_impl<experimental::view_tag> {
    // sliced_view
    template <typename Sequence, std::size_t ...i>
    static constexpr auto
    apply(experimental::sliced_view_t<Sequence, i...>) {
        return hana::bool_c<sizeof...(i) == 0>;
    }

    // transformed_view
    template <typename Sequence, typename F>
    static constexpr auto apply(experimental::transformed_view_t<Sequence, F> view) {
        return hana::is_empty(view.sequence_);
    }

    // joined_view
    template <typename S1, typename S2>
    static constexpr auto apply(experimental::joined_view_t<S1, S2> view) {
        return hana::and_(hana::is_empty(view.sequence1_),
                          hana::is_empty(view.sequence2_));
    }

    // single_view
    template <typename T>
    static constexpr auto apply(experimental::single_view_t<T>) {
        return hana::false_c;
    }

    // empty_view
    static constexpr auto apply(experimental::empty_view_t) {
        return hana::true_c;
    }
};

template <>
struct drop_front_impl<experimental::view_tag> {
    template <typename View, typename N>
    static constexpr auto apply(View view, N const&) {
        constexpr auto n = N::value;
        constexpr auto Length = decltype(hana::length(view))::value;
        return experimental::sliced(view, hana::range_c<std::size_t, n, Length>);
    }
};

//////////////////////////////////////////////////////////////////////////
// Functor
//////////////////////////////////////////////////////////////////////////
template <>
struct transform_impl<experimental::view_tag> {
    template <typename Sequence, typename F, typename G>
    static constexpr auto
    apply(experimental::transformed_view_t<Sequence, F> view, G&& g) {
        return experimental::transformed(view.sequence_,
                                         hana::compose(static_cast<G&&>(g), view.f_));
    }

    template <typename View, typename F>
    static constexpr auto apply(View view, F&& f) {
        return experimental::transformed(view, static_cast<F&&>(f));
    }
};

//////////////////////////////////////////////////////////////////////////
// Applicative
//////////////////////////////////////////////////////////////////////////
template <>
struct lift_impl<experimental::view_tag> {
    template <typename T>
    static constexpr auto apply(T&& t) {
        return experimental::single_view(static_cast<T&&>(t));
    }
};

template <>
struct ap_impl<experimental::view_tag> {
    template <typename F, typename X>
    static constexpr auto apply(F&& f, X&& x) {
        // TODO: Implement cleverly; we most likely need a cartesian_product
        //       view or something like that.
        return hana::ap(hana::to_tuple(f), hana::to_tuple(x));
    }
};

//////////////////////////////////////////////////////////////////////////
// Monad
//////////////////////////////////////////////////////////////////////////
template <>
struct flatten_impl<experimental::view_tag> {
    template <typename View>
    static constexpr auto apply(View view) {
        // TODO: Implement a flattened_view instead
        return hana::fold_left(view, experimental::empty_view(),
                                     experimental::joined);
    }
};

//////////////////////////////////////////////////////////////////////////
// MonadPlus
//////////////////////////////////////////////////////////////////////////
template <>
struct concat_impl<experimental::view_tag> {
    template <typename View1, typename View2>
    static constexpr auto apply(View1 view1, View2 view2) {
        return experimental::joined(view1, view2);
    }
};

template <>
struct empty_impl<experimental::view_tag> {
    static constexpr auto apply() {
        return experimental::empty_view();
    }
};

//////////////////////////////////////////////////////////////////////////
// Comparable
//////////////////////////////////////////////////////////////////////////
template <>
struct equal_impl<experimental::view_tag, experimental::view_tag> {
    template <typename View1, typename View2>
    static constexpr auto apply(View1 v1, View2 v2) {
        // TODO: Use a lexicographical comparison algorithm.
        return hana::equal(hana::to_tuple(v1), hana::to_tuple(v2));
    }
};

template <typename S>
struct equal_impl<experimental::view_tag, S, hana::when<hana::Sequence<S>::value>> {
    template <typename View1, typename Seq>
    static constexpr auto apply(View1 v1, Seq const& s) {
        // TODO: Use a lexicographical comparison algorithm.
        return hana::equal(hana::to_tuple(v1), hana::to_tuple(s));
    }
};

template <typename S>
struct equal_impl<S, experimental::view_tag, hana::when<hana::Sequence<S>::value>> {
    template <typename Seq, typename View2>
    static constexpr auto apply(Seq const& s, View2 v2) {
        // TODO: Use a lexicographical comparison algorithm.
        return hana::equal(hana::to_tuple(s), hana::to_tuple(v2));
    }
};

//////////////////////////////////////////////////////////////////////////
// Orderable
//////////////////////////////////////////////////////////////////////////
template <>
struct less_impl<experimental::view_tag, experimental::view_tag> {
    template <typename View1, typename View2>
    static constexpr auto apply(View1 v1, View2 v2) {
        return hana::lexicographical_compare(v1, v2);
    }
};

template <typename S>
struct less_impl<experimental::view_tag, S, hana::when<hana::Sequence<S>::value>> {
    template <typename View1, typename Seq>
    static constexpr auto apply(View1 v1, Seq const& s) {
        return hana::lexicographical_compare(v1, s);
    }
};

template <typename S>
struct less_impl<S, experimental::view_tag, hana::when<hana::Sequence<S>::value>> {
    template <typename Seq, typename View2>
    static constexpr auto apply(Seq const& s, View2 v2) {
        return hana::lexicographical_compare(s, v2);
    }
};

BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_EXPERIMENTAL_VIEW_HPP
