/*!
@file
Defines `boost::hana::map`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_MAP_HPP
#define BOOST_HANA_MAP_HPP

#include <boost/hana/fwd/map.hpp>

#include <boost/hana/all_of.hpp>
#include <boost/hana/basic_tuple.hpp>
#include <boost/hana/bool.hpp>
#include <boost/hana/concept/comparable.hpp>
#include <boost/hana/concept/constant.hpp>
#include <boost/hana/concept/product.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/contains.hpp>
#include <boost/hana/core/is_a.hpp>
#include <boost/hana/core/make.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/detail/fast_and.hpp>
#include <boost/hana/detail/has_duplicates.hpp>
#include <boost/hana/detail/hash_table.hpp>
#include <boost/hana/detail/intrinsics.hpp>
#include <boost/hana/detail/operators/adl.hpp>
#include <boost/hana/detail/operators/comparable.hpp>
#include <boost/hana/detail/operators/searchable.hpp>
#include <boost/hana/equal.hpp>
#include <boost/hana/find.hpp>
#include <boost/hana/first.hpp>
#include <boost/hana/fold_left.hpp>
#include <boost/hana/functional/demux.hpp>
#include <boost/hana/functional/on.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/fwd/any_of.hpp>
#include <boost/hana/fwd/at_key.hpp>
#include <boost/hana/fwd/difference.hpp>
#include <boost/hana/fwd/erase_key.hpp>
#include <boost/hana/fwd/intersection.hpp>
#include <boost/hana/fwd/is_subset.hpp>
#include <boost/hana/fwd/keys.hpp>
#include <boost/hana/fwd/union.hpp>
#include <boost/hana/insert.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/keys.hpp>
#include <boost/hana/length.hpp>
#include <boost/hana/optional.hpp>
#include <boost/hana/remove_if.hpp>
#include <boost/hana/second.hpp>
#include <boost/hana/unpack.hpp>
#include <boost/hana/value.hpp>


#include <cstddef>
#include <type_traits>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //////////////////////////////////////////////////////////////////////////
    // operators
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <>
        struct comparable_operators<map_tag> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // map
    //////////////////////////////////////////////////////////////////////////
    //! @cond
    namespace detail {
        template <typename ...>
        struct storage_is_default_constructible;
        template <typename ...T>
        struct storage_is_default_constructible<hana::basic_tuple<T...>> {
            static constexpr bool value = detail::fast_and<
                BOOST_HANA_TT_IS_CONSTRUCTIBLE(T)...
            >::value;
        };

        template <typename ...>
        struct storage_is_copy_constructible;
        template <typename ...T>
        struct storage_is_copy_constructible<hana::basic_tuple<T...>> {
            static constexpr bool value = detail::fast_and<
                BOOST_HANA_TT_IS_CONSTRUCTIBLE(T, T const&)...
            >::value;
        };

        template <typename ...>
        struct storage_is_move_constructible;
        template <typename ...T>
        struct storage_is_move_constructible<hana::basic_tuple<T...>> {
            static constexpr bool value = detail::fast_and<
                BOOST_HANA_TT_IS_CONSTRUCTIBLE(T, T&&)...
            >::value;
        };

        template <typename ...>
        struct storage_is_copy_assignable;
        template <typename ...T>
        struct storage_is_copy_assignable<hana::basic_tuple<T...>> {
            static constexpr bool value = detail::fast_and<
                BOOST_HANA_TT_IS_ASSIGNABLE(T, T const&)...
            >::value;
        };

        template <typename ...>
        struct storage_is_move_assignable;
        template <typename ...T>
        struct storage_is_move_assignable<hana::basic_tuple<T...>> {
            static constexpr bool value = detail::fast_and<
                BOOST_HANA_TT_IS_ASSIGNABLE(T, T&&)...
            >::value;
        };

        template <typename HashTable, typename Storage>
        struct map_impl final
            : detail::searchable_operators<map_impl<HashTable, Storage>>
            , detail::operators::adl<map_impl<HashTable, Storage>>
        {
            using hash_table_type = HashTable;
            using storage_type = Storage;

            Storage storage;

            using hana_tag = map_tag;

            template <typename ...P, typename = typename std::enable_if<
                std::is_same<
                    Storage,
                    hana::basic_tuple<typename detail::decay<P>::type...>
                >::value
            >::type>
            explicit constexpr map_impl(P&& ...pairs)
                : storage{static_cast<P&&>(pairs)...}
            { }

            explicit constexpr map_impl(Storage&& xs)
                : storage(static_cast<Storage&&>(xs))
            { }

            template <typename ...Dummy, typename = typename std::enable_if<
                detail::storage_is_default_constructible<Storage, Dummy...>::value
            >::type>
            constexpr map_impl()
                : storage()
            { }

            template <typename ...Dummy, typename = typename std::enable_if<
                detail::storage_is_copy_constructible<Storage, Dummy...>::value
            >::type>
            constexpr map_impl(map_impl const& other)
                : storage(other.storage)
            { }

            template <typename ...Dummy, typename = typename std::enable_if<
                detail::storage_is_move_constructible<Storage, Dummy...>::value
            >::type>
            constexpr map_impl(map_impl&& other)
                : storage(static_cast<Storage&&>(other.storage))
            { }

            template <typename ...Dummy, typename = typename std::enable_if<
                detail::storage_is_move_assignable<Storage, Dummy...>::value
            >::type>
            constexpr map_impl& operator=(map_impl&& other) {
                storage = static_cast<Storage&&>(other.storage);
                return *this;
            }

            template <typename ...Dummy, typename = typename std::enable_if<
                detail::storage_is_copy_assignable<Storage, Dummy...>::value
            >::type>
            constexpr map_impl& operator=(map_impl const& other) {
                storage = other.storage;
                return *this;
            }

            // Prevent the compiler from defining the default copy and move
            // constructors, which interfere with the SFINAE above.
            ~map_impl() = default;
        };
        //! @endcond

        template <typename Storage>
        struct KeyAtIndex {
            template <std::size_t i>
            using apply = decltype(hana::first(hana::at_c<i>(std::declval<Storage>())));
        };

        template <typename ...Pairs>
        struct make_map_type {
            using Storage = hana::basic_tuple<Pairs...>;
            using HashTable = typename detail::make_hash_table<
                detail::KeyAtIndex<Storage>::template apply, sizeof...(Pairs)
            >::type;
            using type = detail::map_impl<HashTable, Storage>;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // make<map_tag>
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct make_impl<map_tag> {
        template <typename ...Pairs>
        static constexpr auto apply(Pairs&& ...pairs) {
#if defined(BOOST_HANA_CONFIG_ENABLE_DEBUG_MODE)
            static_assert(detail::fast_and<hana::Product<Pairs>::value...>::value,
            "hana::make_map(pairs...) requires all the 'pairs' to be Products");

            static_assert(detail::fast_and<
                hana::Comparable<decltype(hana::first(pairs))>::value...
            >::value,
            "hana::make_map(pairs...) requires all the keys to be Comparable");

            static_assert(detail::fast_and<
                hana::Constant<
                    decltype(hana::equal(hana::first(pairs), hana::first(pairs)))
                >::value...
            >::value,
            "hana::make_map(pairs...) requires all the keys to be "
            "Comparable at compile-time");

            //! @todo
            //! This can be implemented more efficiently by doing the check
            //! inside each bucket instead.
            static_assert(!detail::has_duplicates<decltype(hana::first(pairs))...>::value,
            "hana::make_map({keys, values}...) requires all the keys to be unique");

            static_assert(!detail::has_duplicates<decltype(hana::hash(hana::first(pairs)))...>::value,
            "hana::make_map({keys, values}...) requires all the keys to have different hashes");
#endif

            using Map = typename detail::make_map_type<typename detail::decay<Pairs>::type...>::type;
            return Map{hana::make_basic_tuple(static_cast<Pairs&&>(pairs)...)};
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // keys
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct keys_impl<map_tag> {
        template <typename Map>
        static constexpr decltype(auto) apply(Map&& map) {
            return hana::transform(static_cast<Map&&>(map).storage, hana::first);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // values
    //////////////////////////////////////////////////////////////////////////
    //! @cond
    template <typename Map>
    constexpr decltype(auto) values_t::operator()(Map&& map) const {
        return hana::transform(static_cast<Map&&>(map).storage, hana::second);
    }
    //! @endcond

    //////////////////////////////////////////////////////////////////////////
    // insert
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct insert_impl<map_tag> {
        template <typename Map, typename Pair>
        static constexpr auto helper(Map&& map, Pair&& pair, ...) {
            using RawMap = typename std::remove_reference<Map>::type;
            using HashTable = typename RawMap::hash_table_type;
            using NewHashTable = typename detail::bucket_insert<
                HashTable,
                decltype(hana::first(pair)),
                decltype(hana::length(map.storage))::value
            >::type;

            using NewStorage = decltype(
                hana::append(static_cast<Map&&>(map).storage, static_cast<Pair&&>(pair))
            );
            return detail::map_impl<NewHashTable, NewStorage>(
                hana::append(static_cast<Map&&>(map).storage, static_cast<Pair&&>(pair))
            );
        }

        template <typename Map, typename Pair, std::size_t i>
        static constexpr auto
        helper(Map&& map, Pair&&,
               hana::optional<std::integral_constant<std::size_t, i>>)
        {
            return static_cast<Map&&>(map);
        }

        //! @todo
        //! Here, we insert only if the key is not already in the map.
        //! This should be handled by `bucket_insert`, and that would also
        //! be more efficient.
        template <typename Map, typename Pair>
        static constexpr auto apply(Map&& map, Pair&& pair) {
            using RawMap = typename std::remove_reference<Map>::type;
            using Storage = typename RawMap::storage_type;
            using HashTable = typename RawMap::hash_table_type;
            using Key = decltype(hana::first(pair));
            using MaybeIndex = typename detail::find_index<
              HashTable, Key, detail::KeyAtIndex<Storage>::template apply
            >::type;
            return helper(static_cast<Map&&>(map), static_cast<Pair&&>(pair), MaybeIndex{});
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // erase_key
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct erase_key_impl<map_tag> {
        //! @todo
        //! We could implement some kind of `bucket_erase` metafunction
        //! that would be much more efficient than this.
        template <typename Map, typename Key>
        static constexpr auto
        erase_key_helper(Map&& map, Key const&, hana::false_) {
            return static_cast<Map&&>(map);
        }

        template <typename Map, typename Key>
        static constexpr auto
        erase_key_helper(Map&& map, Key const& key, hana::true_) {
            return hana::unpack(
                hana::remove_if(static_cast<Map&&>(map).storage,
                                hana::on(hana::equal.to(key), hana::first)),
                hana::make_map
            );
        }

        template <typename Map, typename Key>
        static constexpr auto apply_impl(Map&& map, Key const& key, hana::false_) {
            return erase_key_helper(static_cast<Map&&>(map), key,
                                    hana::contains(map, key));
        }

        template <typename Map, typename Key>
        static constexpr auto apply_impl(Map&& map, Key const&, hana::true_) {
            return static_cast<Map&&>(map);
        }

        template <typename Map, typename Key>
        static constexpr auto apply(Map&& map, Key const& key) {
            constexpr bool is_empty = decltype(hana::length(map))::value == 0;
            return apply_impl(static_cast<Map&&>(map), key, hana::bool_<is_empty>{});
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct equal_impl<map_tag, map_tag> {
        template <typename M1, typename M2>
        static constexpr auto equal_helper(M1 const&, M2 const&, hana::false_) {
            return hana::false_c;
        }

        template <typename M1, typename M2>
        static constexpr auto equal_helper(M1 const& m1, M2 const& m2, hana::true_) {
            return hana::all_of(hana::keys(m1), hana::demux(equal)(
                hana::partial(hana::find, m1),
                hana::partial(hana::find, m2)
            ));
        }

        template <typename M1, typename M2>
        static constexpr auto apply(M1 const& m1, M2 const& m2) {
            return equal_impl::equal_helper(m1, m2, hana::bool_c<
                decltype(hana::length(m1.storage))::value ==
                decltype(hana::length(m2.storage))::value
            >);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Searchable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct find_impl<map_tag> {
        template <typename Map>
        static constexpr auto find_helper(Map&&, ...) {
            return hana::nothing;
        }

        template <typename Map, std::size_t i>
        static constexpr auto
        find_helper(Map&& map, hana::optional<std::integral_constant<std::size_t, i>>) {
            return hana::just(hana::second(hana::at_c<i>(static_cast<Map&&>(map).storage)));
        }

        template <typename Map, typename Key>
        static constexpr auto apply(Map&& map, Key const&) {
            using RawMap = typename std::remove_reference<Map>::type;
            using Storage = typename RawMap::storage_type;
            using HashTable = typename RawMap::hash_table_type;
            using MaybeIndex = typename detail::find_index<
              HashTable, Key, detail::KeyAtIndex<Storage>::template apply
            >::type;
            return find_helper(static_cast<Map&&>(map), MaybeIndex{});
        }
    };

    template <>
    struct find_if_impl<map_tag> {
        template <typename M, typename Pred>
        static constexpr auto apply(M&& map, Pred&& pred) {
            return hana::transform(
                hana::find_if(static_cast<M&&>(map).storage,
                    hana::compose(static_cast<Pred&&>(pred), hana::first)),
                hana::second
            );
        }
    };

    template <>
    struct contains_impl<map_tag> {
        template <typename Map, typename Key>
        static constexpr auto apply(Map const&, Key const&) {
            using RawMap = typename std::remove_reference<Map>::type;
            using HashTable = typename RawMap::hash_table_type;
            using Storage = typename RawMap::storage_type;
            using MaybeIndex = typename detail::find_index<
                HashTable, Key, detail::KeyAtIndex<Storage>::template apply
            >::type;
            return hana::bool_<!decltype(hana::is_nothing(MaybeIndex{}))::value>{};
        }
    };

    template <>
    struct any_of_impl<map_tag> {
        template <typename M, typename Pred>
        static constexpr auto apply(M const& map, Pred const& pred)
        { return hana::any_of(hana::keys(map), pred); }
    };

    template <>
    struct is_subset_impl<map_tag, map_tag> {
        template <typename Ys>
        struct all_contained {
            Ys const& ys;
            template <typename ...X>
            constexpr auto operator()(X const& ...x) const {
                return hana::bool_c<detail::fast_and<
                    hana::value<decltype(hana::contains(ys, x))>()...
                >::value>;
            }
        };

        template <typename Xs, typename Ys>
        static constexpr auto apply(Xs const& xs, Ys const& ys) {
            auto ys_keys = hana::keys(ys);
            return hana::unpack(hana::keys(xs), all_contained<decltype(ys_keys)>{ys_keys});
        }
    };

    template <>
    struct at_key_impl<map_tag> {
        template <typename Map, typename Key>
        static constexpr decltype(auto) apply(Map&& map, Key const&) {
            using RawMap = typename std::remove_reference<Map>::type;
            using HashTable = typename RawMap::hash_table_type;
            using Storage = typename RawMap::storage_type;
            using MaybeIndex = typename detail::find_index<
                HashTable, Key, detail::KeyAtIndex<Storage>::template apply
            >::type;
            static_assert(!decltype(hana::is_nothing(MaybeIndex{}))::value,
                "hana::at_key(map, key) requires the 'key' to be present in the 'map'");
            constexpr std::size_t index = decltype(*MaybeIndex{}){}();
            return hana::second(hana::at_c<index>(static_cast<Map&&>(map).storage));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // union_
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct union_impl<map_tag> {
        template <typename Xs, typename Ys>
        static constexpr auto apply(Xs&& xs, Ys&& ys) {
            return hana::fold_left(static_cast<Xs&&>(xs), static_cast<Ys&&>(ys),
                                   hana::insert);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // intersection_
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <typename Ys>
        struct map_insert_if_contains {
            Ys const& ys;

            // Second template param will be pair
            // Get its key and check if it exists, if it does, insert key, value pair.
            template <typename Result, typename Pair>
            static constexpr auto helper(Result&& result, Pair&& pair, hana::true_) {
                return hana::insert(static_cast<Result&&>(result), static_cast<Pair&&>(pair));
            }

            template <typename Result, typename Pair>
            static constexpr auto helper(Result&& result, Pair&&, hana::false_) {
                return static_cast<Result&&>(result);
            }

            template <typename Result, typename Pair>
            constexpr auto operator()(Result&& result, Pair&& pair) const {
                constexpr bool keep = hana::value<decltype(hana::contains(ys, hana::first(pair)))>();
                return map_insert_if_contains::helper(static_cast<Result&&>(result),
                                                      static_cast<Pair&&>(pair),
                                                      hana::bool_c<keep>);
            }
        };
    }

    template <>
    struct intersection_impl<map_tag> {
        template <typename Xs, typename Ys>
        static constexpr auto apply(Xs&& xs, Ys const& ys) {
            return hana::fold_left(static_cast<Xs&&>(xs), hana::make_map(),
                                   detail::map_insert_if_contains<Ys>{ys});
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // difference
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct difference_impl<map_tag> {
        template <typename Xs, typename Ys>
        static constexpr auto apply(Xs&& xs, Ys&& ys) {
            return hana::fold_left(
                    hana::keys(static_cast<Ys&&>(ys)),
                    static_cast<Xs&&>(xs),
                    hana::erase_key);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct unpack_impl<map_tag> {
        template <typename M, typename F>
        static constexpr decltype(auto) apply(M&& map, F&& f) {
            return hana::unpack(static_cast<M&&>(map).storage,
                                static_cast<F&&>(f));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Construction from a Foldable
    //////////////////////////////////////////////////////////////////////////
    template <typename F>
    struct to_impl<map_tag, F, when<hana::Foldable<F>::value>> {
        template <typename Xs>
        static constexpr decltype(auto) apply(Xs&& xs) {
            return hana::fold_left(
                static_cast<Xs&&>(xs), hana::make_map(), hana::insert
            );
        }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_MAP_HPP
