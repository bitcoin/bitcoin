/*!
@file
Defines `boost::hana::detail::hash_table`.

@copyright Louis Dionne 2016
@copyright Jason Rice 2016
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_HASH_TABLE_HPP
#define BOOST_HANA_DETAIL_HASH_TABLE_HPP

#include <boost/hana/equal.hpp>
#include <boost/hana/ext/std/integer_sequence.hpp>
#include <boost/hana/ext/std/integral_constant.hpp>
#include <boost/hana/find_if.hpp>
#include <boost/hana/fold_left.hpp>
#include <boost/hana/hash.hpp>
#include <boost/hana/optional.hpp>
#include <boost/hana/range.hpp>
#include <boost/hana/type.hpp>
#include <boost/hana/value.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN namespace detail {
    template <typename Hash, std::size_t ...i>
    struct bucket { };

    template <typename ...Buckets>
    struct hash_table
        : Buckets...
    { };

    // find_indices:
    //  Returns an `index_sequence` containing possible indices for the given
    //  `Key` in the `Map`.
    template <typename Hash, std::size_t ...i>
    std::index_sequence<i...> find_indices_impl(bucket<Hash, i...> const&);

    template <typename Hash>
    std::index_sequence<> find_indices_impl(...);

    template <typename Map, typename Key>
    struct find_indices {
        using Hash = typename decltype(hana::hash(std::declval<Key>()))::type;
        using type = decltype(detail::find_indices_impl<Hash>(std::declval<Map>()));
    };
    // end find_indices

    // find_index:
    //  Returns the actual index of a `Key` in the `Map`. The type of the key
    //  associated to any given index must be retrievable with the `KeyAtIndex`
    //  alias.
    template <template <std::size_t> class KeyAtIndex, typename Key>
    struct find_pred {
        template <typename Index>
        auto operator()(Index const&) const -> decltype(
            hana::equal(std::declval<KeyAtIndex<Index::value>>(),
                        std::declval<Key>())
        );
    };

    template <typename Indices, typename Key, template <std::size_t> class KeyAtIndex>
    struct find_index_impl {
        using type = decltype(hana::find_if(Indices{}, find_pred<KeyAtIndex, Key>{}));
    };

    // This is a peephole optimization for buckets that have a single entry.
    // It provides a nice speedup in the at_key.number_of_lookups benchmark.
    // It is perhaps possible to make this part of `find_if` itself, but we
    // should make sure that we retain that speedup.
    template <std::size_t i, typename Key, template <std::size_t> class KeyAtIndex>
    struct find_index_impl<std::index_sequence<i>, Key, KeyAtIndex> {
        using Equal = decltype(
            hana::equal(std::declval<KeyAtIndex<i>>(),
                        std::declval<Key>())
        );
        using type = typename std::conditional<Equal::value,
            hana::optional<std::integral_constant<std::size_t, i>>,
            hana::optional<>
        >::type;
    };

    template <typename Map, typename Key, template <std::size_t> class KeyAtIndex>
    struct find_index {
        using Indices = typename find_indices<Map, Key>::type;
        using type = typename find_index_impl<Indices, Key, KeyAtIndex>::type;
    };
    // end find_index

    // bucket_insert:
    //  Inserts the given `Index` into the bucket of the `Map` in which `Key` falls.
    template <typename Bucket, typename Hash, std::size_t Index>
    struct update_bucket {
        using type = Bucket;
    };

    template <std::size_t ...i, typename Hash, std::size_t Index>
    struct update_bucket<bucket<Hash, i...>, Hash, Index> {
        using type = bucket<Hash, i..., Index>;
    };

    template <typename Map, typename Key, std::size_t Index, bool =
        (find_indices<Map, Key>::type::size() > 0)
    >
    struct bucket_insert;

    template <typename ...Buckets, typename Key, std::size_t Index>
    struct bucket_insert<hash_table<Buckets...>, Key, Index, true> {
        // There is a bucket for that Hash; append the new index to it.
        using Hash = typename decltype(hana::hash(std::declval<Key>()))::type;
        using type = hash_table<typename update_bucket<Buckets, Hash, Index>::type...>;
    };

    template <typename ...Buckets, typename Key, std::size_t Index>
    struct bucket_insert<hash_table<Buckets...>, Key, Index, false> {
        // There is no bucket for that Hash; insert a new bucket.
        using Hash = typename decltype(hana::hash(std::declval<Key>()))::type;
        using type = hash_table<Buckets..., bucket<Hash, Index>>;
    };
    // end bucket_insert

    // make_hash_table:
    //  Creates a `hash_table` type able of holding the given number of
    //  elements. The type of the key associated to any given index must
    //  be retrievable using the `KeyAtIndex` alias. All the keys must
    //  be distinct and have different hashes too.
    template <template <std::size_t> class KeyAtIndex, std::size_t N,
              typename Indices = std::make_index_sequence<N>>
    struct make_hash_table;

    template <template <std::size_t> class KeyAtIndex, std::size_t N, std::size_t ...i>
    struct make_hash_table<KeyAtIndex, N, std::index_sequence<i...>> {
        using type = hash_table<
            bucket<typename decltype(hana::hash(std::declval<KeyAtIndex<i>>()))::type, i>...
        >;
    };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_HASH_TABLE_HPP
