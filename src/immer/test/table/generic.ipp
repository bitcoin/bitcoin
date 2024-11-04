//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include "immer/memory_policy.hpp"
#ifndef SETUP_T
#error "define the map template to use in MAP_T"
#endif

#include <immer/algorithm.hpp>
#include <immer/table.hpp>

#include "test/dada.hpp"
#include "test/util.hpp"

#include <catch2/catch.hpp>

#include <random>
#include <unordered_map>
#include <unordered_set>

struct pair_key_fn
{
    template <typename F, typename S>
    F operator()(const std::pair<F, S>& p) const
    {
        return p.first;
    }

    template <typename F, typename S>
    auto operator()(std::pair<F, S> p, F k) const
    {
        p.first = std::move(k);
        return p;
    }

    template <typename F, typename S>
    F operator()(const dadaist<std::pair<F, S>>& p) const
    {
        return p.value.first;
    }

    template <typename F, typename S>
    auto operator()(dadaist<std::pair<F, S>> p, F k) const
    {
        p.value.first = std::move(k);
        return p;
    }
};

template <typename K,
          typename V,
          typename Hash = std::hash<K>,
          typename Eq   = std::equal_to<K>>
using table_map = immer::table<std::pair<K, V>,
                               pair_key_fn,
                               Hash,
                               Eq,
                               SETUP_T::memory_policy,
                               SETUP_T::bits>;

IMMER_RANGES_CHECK(std::ranges::forward_range<table_map<std::string, std::string>>);

template <typename T = uint32_t>
auto make_generator()
{
    auto engine = std::default_random_engine{42};
    auto dist   = std::uniform_int_distribution<T>{};
    return std::bind(dist, engine);
}

struct conflictor
{
    uint32_t v1;
    uint32_t v2;

    bool operator==(const conflictor& x) const
    {
        return v1 == x.v1 && v2 == x.v2;
    }
};

struct hash_conflictor
{
    std::size_t operator()(const conflictor& x) const { return x.v1; }
};

auto make_values_with_collisions(uint32_t n)
{
    auto gen   = make_generator();
    auto vals  = std::vector<std::pair<conflictor, uint32_t>>{};
    auto vals_ = std::unordered_set<conflictor, hash_conflictor>{};
    auto i     = 0u;
    generate_n(back_inserter(vals), n, [&] {
        auto newv = conflictor{};
        do {
            newv = {uint32_t(gen() % (n / 2)), gen()};
        } while (!vals_.insert(newv).second);
        return std::pair<conflictor, uint32_t>{newv, i++};
    });
    return vals;
}

auto make_test_map(uint32_t n)
{
    auto s = table_map<uint32_t, uint32_t>{};
    for (auto i = 0u; i < n; ++i)
        s = std::move(s).insert({i, i});
    return s;
}

auto make_test_map(const std::vector<std::pair<conflictor, uint32_t>>& vals)
{
    auto s = table_map<conflictor, uint32_t, hash_conflictor>{};
    for (auto&& v : vals)
        s = std::move(s).insert(v);
    return s;
}

TEST_CASE("instantiation")
{
    SECTION("default")
    {
        auto v = table_map<int, int>{};
        CHECK(v.size() == 0u);
    }
}

TEST_CASE("basic insertion")
{
    auto v1 = table_map<int, int>{};
    CHECK(v1.count(42) == 0);

    auto v2 = v1.insert({42, {}});
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);

    auto v3 = v2.insert({42, {}});
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);
    CHECK(v3.count(42) == 1);
}

TEST_CASE("initializer list and range constructors")
{
    auto v0 = std::unordered_map<std::string, int>{
        {{"foo", 42}, {"bar", 13}, {"baz", 18}, {"zab", 64}}};
    auto v1 = table_map<std::string, int>{
        {{"foo", 42}, {"bar", 13}, {"baz", 18}, {"zab", 64}}};
    auto v2 = table_map<std::string, int>{v0.begin(), v0.end()};
    CHECK(v1.size() == 4);
    CHECK(v1.count(std::string{"foo"}) == 1);
    CHECK(v1.at(std::string{"bar"}).second == 13);
    CHECK(v1 == v2);
}

TEST_CASE("accessor")
{
    const auto n = 666u;
    auto v       = make_test_map(n);
    CHECK(v[0].second == 0);
    CHECK(v[42].second == 42);
    CHECK(v[665].second == 665);
    CHECK(v[666].second == 0);
    CHECK(v[1234].second == 0);
}

TEST_CASE("at")
{
    const auto n = 666u;
    auto v       = make_test_map(n);
    CHECK(v.at(0).second == 0);
    CHECK(v.at(42).second == 42);
    CHECK(v.at(665).second == 665);
#ifndef IMMER_NO_EXCEPTIONS
    CHECK_THROWS_AS(v.at(666), std::out_of_range);
    CHECK_THROWS_AS(v.at(1234), std::out_of_range);
#endif
}

TEST_CASE("find")
{
    const auto n = 666u;
    auto v       = make_test_map(n);
    CHECK(v.find(0)->second == 0);
    CHECK(v.find(42)->second == 42);
    CHECK(v.find(665)->second == 665);
    CHECK(v.find(666) == nullptr);
    CHECK(v.find(1234) == nullptr);
}

TEST_CASE("equals and insert")
{
    const auto n = 666u;
    auto v       = make_test_map(n);

    CHECK(v == v);
    CHECK(v != v.insert({1234, 42}));
    CHECK(v != v.erase(32));
    CHECK(v == v.insert({1234, 42}).erase(1234));
    CHECK(v == v.erase(32).insert({32, 32}));
}

TEST_CASE("iterator")
{
    const auto N = 666u;
    auto v       = make_test_map(N);

    SECTION("empty set")
    {
        auto s = table_map<uint32_t, uint32_t>{};
        CHECK(s.begin() == s.end());
    }

    SECTION("works with range loop")
    {
        auto seen = std::unordered_set<uint32_t>{};
        for (const auto& x : v)
            CHECK(seen.insert(x.first).second);
        CHECK(seen.size() == v.size());
    }

    SECTION("iterator and collisions")
    {
        auto vals = make_values_with_collisions(N);
        auto s    = make_test_map(vals);
        auto seen = std::unordered_set<conflictor, hash_conflictor>{};
        for (const auto& x : s)
            CHECK(seen.insert(x.first).second);
        CHECK(seen.size() == s.size());
    }
}

TEST_CASE("accumulate")
{
    const auto n = 666u;
    auto v       = make_test_map(n);

    auto expected_n = [](auto n) { return n * (n - 1) / 2; };

    SECTION("sum collection")
    {
        auto acc = [](uint32_t acc, const std::pair<uint32_t, uint32_t>& x) {
            return acc + x.first + x.second;
        };
        auto sum = immer::accumulate(v, 0u, acc);
        CHECK(sum == 2 * expected_n(v.size()));
    }

    SECTION("sum collisions")
    {
        auto vals = make_values_with_collisions(n);
        auto s    = make_test_map(vals);
        auto acc  = [](uint32_t r, std::pair<conflictor, uint32_t> x) {
            return r + x.first.v1 + x.first.v2 + x.second;
        };
        auto sum1 = std::accumulate(vals.begin(), vals.end(), 0u, acc);
        auto sum2 = immer::accumulate(s, 0u, acc);
        CHECK(sum1 == sum2);
    }
}

TEST_CASE("update a lot")
{
    auto v       = make_test_map(666u);
    auto incr_id = [](auto&& p) {
        return std::make_pair(p.first, p.second + 1);
    };
    SECTION("immutable")
    {
        for (decltype(v.size()) i = 0; i < v.size(); ++i) {
            v = v.update(i, incr_id);
            CHECK(v[i].second == i + 1);
        }
    }
    SECTION("move")
    {
        for (decltype(v.size()) i = 0; i < v.size(); ++i) {
            v = std::move(v).update(i, incr_id);
            CHECK(v[i].second == i + 1);
        }
    }
    SECTION("if_exists immutable")
    {
        for (decltype(v.size()) i = 0; i < v.size(); ++i) {
            v = v.update_if_exists(i, incr_id);
            CHECK(v[i].second == i + 1);
        }
    }
    SECTION("if_exists move")
    {
        for (decltype(v.size()) i = 0; i < v.size(); ++i) {
            v = std::move(v).update_if_exists(i, incr_id);
            CHECK(v[i].second == i + 1);
        }
    }
}

TEST_CASE("exception safety")
{
    constexpr auto n = 2666u;

    using dadaist_map_t =
        immer::table<dadaist<std::pair<uint32_t, uint32_t>>,        //
                     pair_key_fn,                                   //
                     std::hash<uint32_t>,                           //
                     std::equal_to<uint32_t>,                       //
                     dadaist_memory_policy<SETUP_T::memory_policy>, //
                     SETUP_T::bits                                  //
                     >;

    auto make_pair = [](auto f, auto s) {
        return dadaist<std::pair<decltype(f), decltype(s)>>(
            std::make_pair(f, s));
    };
    SECTION("update collisions")
    {
        auto v = dadaist_map_t{};
        auto d = dadaism{};
        for (auto i = 0u; i < n; ++i)
            v = std::move(v).insert(make_pair(i, i));
        for (auto i = 0u; i < v.size();) {
            try {
                auto s = d.next();
                v      = v.update(i, [&make_pair](auto x) {
                    return make_pair(x.value.first, x.value.second + 1);
                });
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.at(i).value.second == i + 1);
            for (auto i : test_irange(i, n))
                CHECK(v.at(i).value.second == i);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }
    using dadaist_conflictor_map_t =
        immer::table<dadaist<std::pair<conflictor, uint32_t>>,      //
                     pair_key_fn,                                   //
                     hash_conflictor,                               //
                     std::equal_to<conflictor>,                     //
                     dadaist_memory_policy<SETUP_T::memory_policy>, //
                     SETUP_T::bits                                  //
                     >;

    SECTION("update collisisions")
    {
        auto vals = make_values_with_collisions(n);
        auto v    = dadaist_conflictor_map_t{};
        auto d    = dadaism{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert(vals[i]);
        for (auto i = 0u; i < v.size();) {
            try {
                auto s = d.next();
                v      = v.update(vals[i].first, [&make_pair](auto x) {
                    return make_pair(x.value.first, x.value.second + 1);
                });
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.at(vals[i].first).value.second == vals[i].second + 1);
            for (auto i : test_irange(i, n))
                CHECK(v.at(vals[i].first).value.second == vals[i].second);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }

    SECTION("update collisisions move")
    {
        auto vals = make_values_with_collisions(n);
        auto v    = dadaist_conflictor_map_t{};
        auto d    = dadaism{};
        for (auto i = 0u; i < n; ++i)
            v = std::move(v).insert(vals[i]);
        for (auto i = 0u; i < v.size();) {
            try {
                auto s = d.next();
                v      = v.update(vals[i].first, [&make_pair](auto x) {
                    return make_pair(x.value.first, x.value.second + 1);
                });
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.at(vals[i].first).value.second == vals[i].second + 1);
            for (auto i : test_irange(i, n))
                CHECK(v.at(vals[i].first).value.second == vals[i].second);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }
}

namespace {

struct KeyType
{
    explicit KeyType(uint32_t v)
        : value(v)
    {}
    uint32_t value;
};

struct LookupType
{
    explicit LookupType(uint32_t v)
        : value(v)
    {}
    uint32_t value;
};

struct TransparentHash
{
    using hash_type      = std::hash<uint32_t>;
    using is_transparent = void;

    size_t operator()(KeyType const& k) const { return hash_type{}(k.value); }
    size_t operator()(LookupType const& k) const
    {
        return hash_type{}(k.value);
    }
};

bool operator==(KeyType const& k, KeyType const& l)
{
    return k.value == l.value;
}

bool operator==(KeyType const& k, LookupType const& l)
{
    return k.value == l.value;
}

} // namespace

TEST_CASE("lookup with transparent hash")
{
    SECTION("default")
    {
        auto m = table_map<KeyType, int, TransparentHash, std::equal_to<>>{};
        m      = m.insert({KeyType{1}, 12});

        auto const& v = m.at(LookupType{1});
        CHECK(v.second == 12);
    }
}

namespace {

class KElem
{
public:
    KElem(int* elem) { this->elem = elem; }

    bool operator==(const KElem& other) const
    {
        return this->elem == other.elem;
    }

    bool operator!=(const KElem& other) const { return !(*this == other); }

    int* elem;
};

struct HashBlock
{
    size_t operator()(const KElem& block) const noexcept
    {
        return (uintptr_t) block.elem & 0xffffffff00000000;
    }
};

} // namespace

TEST_CASE("issue 134")
{
    int a[100];
    table_map<KElem, KElem, HashBlock> m;
    for (int i = 0; i < 100; i++) {
        m = m.insert({KElem(a + i), KElem(a + i)});
    }
}

void test_diff(uint32_t old_num,
               uint32_t add_num,
               uint32_t remove_num,
               uint32_t change_num)
{
    auto values = make_values_with_collisions(old_num + add_num);
    std::vector<std::pair<conflictor, uint32_t>> initial_values(
        values.begin(), values.begin() + old_num);
    std::vector<std::pair<conflictor, uint32_t>> new_values(
        values.begin() + old_num, values.end());
    auto map = make_test_map(initial_values);

    std::vector<conflictor> old_keys;
    for (auto const& val : map)
        old_keys.push_back(val.first);

    auto first_snapshot = map;
    CHECK(old_num == first_snapshot.size());

    // remove
    auto shuffle = old_keys;
    std::random_shuffle(shuffle.begin(), shuffle.end());
    std::vector<conflictor> remove_keys(shuffle.begin(),
                                        shuffle.begin() + remove_num);
    std::vector<conflictor> rest_keys(shuffle.begin() + remove_num,
                                      shuffle.end());

    using key_set = std::unordered_set<conflictor, hash_conflictor>;
    key_set removed_keys(remove_keys.begin(), remove_keys.end());
    for (auto const& key : remove_keys)
        map = map.erase(key);
    CHECK(old_num - remove_num == map.size());

    // add
    key_set added_keys;
    for (auto const& data : new_values) {
        map = map.insert({data.first, data.second});
        added_keys.insert(data.first);
    }

    // change
    key_set changed_keys;
    for (auto i = 0u; i < change_num; i++) {
        auto key = rest_keys[i];
        map      = map.update(key, [](auto val) {
            return std::make_pair(val.first, val.second + 1);
        });
        changed_keys.insert(key);
    }

    diff(
        first_snapshot,
        map,
        [&](auto const& data) { REQUIRE(added_keys.erase(data.first) > 0); },
        [&](auto const& data) { REQUIRE(removed_keys.erase(data.first) > 0); },
        [&](auto const& old_data, auto const& new_data) {
            (void) old_data;
            REQUIRE(changed_keys.erase(new_data.first) > 0);
        });

    CHECK(added_keys.empty());
    CHECK(changed_keys.empty());
    CHECK(removed_keys.empty());
}

TEST_CASE("diff")
{
    test_diff(16, 10, 10, 3);
    test_diff(100, 10, 10, 10);
    test_diff(1500, 10, 1000, 100);
    test_diff(16, 1500, 10, 3);
    test_diff(100, 0, 0, 50);
}

TEST_CASE("const map")
{
    const auto x = table_map<std::string, int>{}.insert({"A", 1});
    auto it      = x.begin();
    CHECK(it->first == "A");
    CHECK(it->second == 1);
}
