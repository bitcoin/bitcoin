//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#ifndef SET_T
#error "define the set template to use in SET_T"
#include <immer/set.hpp>
#define SET_T ::immer::set
#endif

#include "test/dada.hpp"
#include "test/util.hpp"

#include <immer/algorithm.hpp>
#include <immer/box.hpp>

#include <catch2/catch.hpp>

#include <random>
#include <unordered_set>

using memory_policy_t = SET_T<unsigned>::memory_policy_type;

IMMER_RANGES_CHECK(std::input_iterator<SET_T<std::string>::iterator>);
IMMER_RANGES_CHECK(std::ranges::forward_range<SET_T<std::string>>);

template <typename T = unsigned>
auto make_generator()
{
    auto engine = std::default_random_engine{42};
    auto dist   = std::uniform_int_distribution<T>{};
    return std::bind(dist, engine);
}

template <typename T = unsigned>
auto make_generator_s()
{
    auto engine = std::default_random_engine{42};
    auto dist   = std::uniform_int_distribution<T>{};
    return
        [g = std::bind(dist, engine)]() mutable { return std::to_string(g()); };
}

struct conflictor
{
    unsigned v1;
    unsigned v2;

    bool operator==(const conflictor& x) const
    {
        return v1 == x.v1 && v2 == x.v2;
    }
};

struct hash_conflictor
{
    std::size_t operator()(const conflictor& x) const { return x.v1; }
};

auto make_values_with_collisions(unsigned n)
{
    auto gen   = make_generator();
    auto vals  = std::vector<conflictor>{};
    auto vals_ = std::unordered_set<conflictor, hash_conflictor>{};
    generate_n(back_inserter(vals), n, [&] {
        auto newv = conflictor{};
        do {
            newv = {unsigned(gen() % (n / 2)), gen()};
        } while (!vals_.insert(newv).second);
        return newv;
    });
    return vals;
}

auto make_test_set(unsigned n)
{
    auto s = SET_T<unsigned>{};
    for (auto i = 0u; i < n; ++i)
        s = std::move(s).insert(i);
    return s;
}

auto make_test_set(const std::vector<conflictor>& vals)
{
    auto s = SET_T<conflictor, hash_conflictor>{};
    for (auto&& v : vals)
        s = std::move(s).insert(v);
    return s;
}

template <std::size_t BufLen>
struct unaligned_str
{
    std::array<char, BufLen> m_data{};

    unaligned_str() = default;
    unaligned_str(const std::string& in)
    {
        for (std::size_t i = 0; i < std::min(m_data.size() - 1, in.size()); i++)
            m_data[i] = in[i];
    }
    unaligned_str(const char* in)
        : unaligned_str{std::string{in}}
    {}

    std::string str() const { return m_data.data(); }

    bool operator==(unaligned_str other) const
    {
        return m_data == other.m_data;
    }

    bool operator!=(unaligned_str other) const
    {
        return m_data != other.m_data;
    }
};

namespace std {
template <size_t BufLen>
struct hash<unaligned_str<BufLen>>
{
    size_t operator()(const unaligned_str<BufLen>& str) const
    {
        return std::hash<std::string>{}(str.str());
    }
};
} // namespace std

template <size_t BufLen>
void check_with_len()
{
    auto v = SET_T<unaligned_str<BufLen>>{};

    for (int i = 0; i < 1; i++)
        v = v.insert(std::to_string(i));

    CHECK(v.count("0") == 1);
}

TEST_CASE("insert type with no alignment requirement")
{
    check_with_len<1>();
    check_with_len<9>();
    check_with_len<15>();
    check_with_len<17>();
}

TEST_CASE("instantiation")
{
    SECTION("default")
    {
        auto v = SET_T<unsigned>{};
        CHECK(v.size() == 0u);
    }
}

TEST_CASE("initializer list and range constructors")
{
    auto v0 = std::unordered_set<std::string>{{"foo", "bar", "baz", "zab"}};
    auto v1 = SET_T<std::string>{{"foo", "bar", "baz", "zab"}};
    auto v2 = SET_T<std::string>{v0.begin(), v0.end()};
    CHECK(v1.size() == 4);
    CHECK(v1.count(std::string{"foo"}) == 1);
    CHECK(v1.count(std::string{"bar"}) == 1);
    CHECK(v1 == v2);
}

TEST_CASE("basic insertion")
{
    auto v1 = SET_T<unsigned>{};
    CHECK(v1.count(42) == 0);
    CHECK(v1.identity() == SET_T<unsigned>{}.identity());

    auto v2 = v1.insert(42);
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);
    CHECK(v1.identity() != v2.identity());

    auto v3 = v2.insert(42);
    // it would maybe be nice if this was not the case, but it is...
    CHECK(v2.identity() != v3.identity());
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);
    CHECK(v3.count(42) == 1);
}

TEST_CASE("insert a lot")
{
    constexpr auto N = 666u;

    auto gen  = make_generator();
    auto vals = std::vector<unsigned>{};
    generate_n(back_inserter(vals), N, gen);
    auto s = SET_T<unsigned>{};

    SECTION("immutable")
    {
        for (auto i = 0u; i < N; ++i) {
            s = s.insert(vals[i]);
            CHECK(s.size() == i + 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 1);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 0);
        }
    }
    SECTION("move")
    {
        for (auto i = 0u; i < N; ++i) {
            s = std::move(s).insert(vals[i]);
            CHECK(s.size() == i + 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 1);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 0);
        }
    }
}

TEST_CASE("insert conflicts")
{
    constexpr auto N = 666u;
    auto vals        = make_values_with_collisions(N);
    auto s           = SET_T<conflictor, hash_conflictor>{};
    SECTION("immutable")
    {
        for (auto i = 0u; i < N; ++i) {
            s = s.insert(vals[i]);
            CHECK(s.size() == i + 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 1);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 0);
        }
    }
    SECTION("move")
    {
        for (auto i = 0u; i < N; ++i) {
            s = std::move(s).insert(vals[i]);
            CHECK(s.size() == i + 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 1);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 0);
        }
    }
}

#if !IMMER_IS_LIBGC_TEST
TEST_CASE("insert boxed move string")
{
    constexpr auto N = 666u;
    constexpr auto S = 7;
    auto s           = SET_T<immer::box<std::string, memory_policy_t>>{};
    SECTION("preserve immutability")
    {
        auto s0 = s;
        auto i0 = 0u;
        for (auto i = 0u; i < N; ++i) {
            if (i % S == 0) {
                s0 = s;
                i0 = i;
            }
            s = std::move(s).insert(std::to_string(i));
            {
                CHECK(s.size() == i + 1);
                for (auto j : test_irange(0u, i + 1))
                    CHECK(s.count(std::to_string(j)) == 1);
                for (auto j : test_irange(i + 1u, N))
                    CHECK(s.count(std::to_string(j)) == 0);
            }
            {
                CHECK(s0.size() == i0);
                for (auto j : test_irange(0u, i0))
                    CHECK(s0.count(std::to_string(j)) == 1);
                for (auto j : test_irange(i0, N))
                    CHECK(s0.count(std::to_string(j)) == 0);
            }
        }
    }
}
#endif

TEST_CASE("erase a lot")
{
    constexpr auto N = 666u;
    auto gen         = make_generator();
    auto vals        = std::vector<unsigned>{};
    generate_n(back_inserter(vals), N, gen);

    auto s = SET_T<unsigned>{};
    for (auto i = 0u; i < N; ++i)
        s = std::move(s).insert(vals[i]);

    SECTION("immutable")
    {
        for (auto i = 0u; i < N; ++i) {
            s = s.erase(vals[i]);
            CHECK(s.size() == N - i - 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 0);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 1);
        }
    }
    SECTION("move")
    {
        for (auto i = 0u; i < N; ++i) {
            s = std::move(s).erase(vals[i]);
            CHECK(s.size() == N - i - 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 0);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 1);
        }
    }
}

#if !IMMER_IS_LIBGC_TEST
TEST_CASE("erase a lot boxed string")
{
    constexpr auto N = 666u;
    auto gen         = make_generator_s();
    auto vals        = std::vector<immer::box<std::string, memory_policy_t>>{};
    generate_n(back_inserter(vals), N, gen);

    auto s = SET_T<immer::box<std::string, memory_policy_t>>{};
    for (auto i = 0u; i < N; ++i)
        s = std::move(s).insert(vals[i]);

    SECTION("immutable")
    {
        for (auto i = 0u; i < N; ++i) {
            s = s.erase(vals[i]);
            CHECK(s.size() == N - i - 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 0);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 1);
        }
    }
    SECTION("move")
    {
        for (auto i = 0u; i < N; ++i) {
            s = std::move(s).erase(vals[i]);
            CHECK(s.size() == N - i - 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 0);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 1);
        }
    }
    SECTION("move preserve immutability")
    {
        constexpr auto S = 7;
        auto s0          = s;
        auto i0          = 0u;
        for (auto i = 0u; i < N; ++i) {
            if (i % S == 0) {
                s0 = s;
                i0 = i;
            }
            s = std::move(s).erase(vals[i]);
            {
                CHECK(s.size() == N - i - 1);
                for (auto j : test_irange(0u, i + 1))
                    CHECK(s.count(vals[j]) == 0);
                for (auto j : test_irange(i + 1u, N))
                    CHECK(s.count(vals[j]) == 1);
            }
            {
                CHECK(s0.size() == N - i0);
                for (auto j : test_irange(0u, i0))
                    CHECK(s0.count(vals[j]) == 0);
                for (auto j : test_irange(i0, N))
                    CHECK(s0.count(vals[j]) == 1);
            }
        }
    }
}
#endif

TEST_CASE("erase conflicts")
{
    constexpr auto N = 666u;
    auto vals        = make_values_with_collisions(N);
    auto s           = SET_T<conflictor, hash_conflictor>{};
    for (auto i = 0u; i < N; ++i)
        s = std::move(s).insert(vals[i]);

    SECTION("immutable")
    {
        for (auto i = 0u; i < N; ++i) {
            s = s.erase(vals[i]);
            CHECK(s.size() == N - i - 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 0);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 1);
        }
    }
    SECTION("move")
    {
        for (auto i = 0u; i < N; ++i) {
            s = std::move(s).erase(vals[i]);
            CHECK(s.size() == N - i - 1);
            for (auto j : test_irange(0u, i + 1))
                CHECK(s.count(vals[j]) == 0);
            for (auto j : test_irange(i + 1u, N))
                CHECK(s.count(vals[j]) == 1);
        }
    }
}

TEST_CASE("accumulate")
{
    const auto n = 666u;
    auto v       = make_test_set(n);

    auto expected_n = [](auto n) { return n * (n - 1) / 2; };

    SECTION("sum collection")
    {
        auto sum = immer::accumulate(v, 0u);
        CHECK(sum == expected_n(v.size()));
    }

    SECTION("sum collisions")
    {
        auto vals = make_values_with_collisions(n);
        auto s    = make_test_set(vals);
        auto acc  = [](unsigned r, conflictor x) { return r + x.v1 + x.v2; };
        auto sum1 = std::accumulate(vals.begin(), vals.end(), 0u, acc);
        auto sum2 = immer::accumulate(s, 0u, acc);
        CHECK(sum1 == sum2);
    }
}

TEST_CASE("find")
{
    const auto n = 666u;
    auto v       = make_test_set(n);
    CHECK(*v.find(0) == 0);
    CHECK(*v.find(42) == 42);
    CHECK(*v.find(665) == 665);
    CHECK(v.find(666) == nullptr);
    CHECK(v.find(1234) == nullptr);
}

TEST_CASE("iterator")
{
    const auto N = 666u;
    auto v       = make_test_set(N);

    SECTION("empty set")
    {
        auto s = SET_T<unsigned>{};
        CHECK(s.begin() == s.end());
    }

    SECTION("works with range loop")
    {
        auto seen = std::unordered_set<unsigned>{};
        for (const auto& x : v)
            CHECK(seen.insert(x).second);
        CHECK(seen.size() == v.size());
    }

    SECTION("works with standard algorithms")
    {
        auto s = std::vector<unsigned>(N);
        std::iota(s.begin(), s.end(), 0u);
        std::equal(v.begin(), v.end(), s.begin(), s.end());
    }

    SECTION("iterator and collisions")
    {
        auto vals = make_values_with_collisions(N);
        auto s    = make_test_set(vals);
        auto seen = std::unordered_set<conflictor, hash_conflictor>{};
        for (const auto& x : s)
            CHECK(seen.insert(x).second);
        CHECK(seen.size() == s.size());
    }
}

struct non_default
{
    unsigned value;
    non_default(unsigned v)
        : value{v}
    {}
    non_default() = delete;
    operator unsigned() const { return value; }

#if IMMER_DEBUG_PRINT
    friend std::ostream& operator<<(std::ostream& os, non_default x)
    {
        os << "ND{" << x.value << "}";
        return os;
    }
#endif
};

namespace std {

template <>
struct hash<non_default>
{
    std::size_t operator()(const non_default& x)
    {
        return std::hash<decltype(x.value)>{}(x.value);
    }
};

} // namespace std

TEST_CASE("non default")
{
    const auto n = 666u;

    auto v = SET_T<non_default>{};
    for (auto i = 0u; i < n; ++i)
        v = v.insert({i});

    CHECK(v.size() == n);
}

TEST_CASE("equals")
{
    const auto n = 666u;
    auto v       = make_test_set(n);

    CHECK(v == v);
    CHECK(v != v.insert(1234));
    CHECK(v == v.erase(1234));
    CHECK(v == v.insert(1234).erase(1234));
    CHECK(v == v.erase(64).insert(64));
    CHECK(v != v.erase(1234).insert(1234));
}

TEST_CASE("equals collisions")
{
    const auto n = 666u;
    auto vals    = make_values_with_collisions(n);
    auto v       = make_test_set(vals);

    CHECK(v == v);
    CHECK(v != v.erase(vals[42]));
    CHECK(v == v.erase(vals[42]).insert(vals[42]));
    CHECK(v ==
          v.erase(vals[42]).erase(vals[13]).insert(vals[42]).insert(vals[13]));
    CHECK(v ==
          v.erase(vals[42]).erase(vals[13]).insert(vals[13]).insert(vals[42]));
}

TEST_CASE("exception safety")
{
    constexpr auto n = 2666u;

    using dadaist_set_t = typename dadaist_wrapper<SET_T<unsigned>>::type;
    using dadaist_conflictor_set_t =
        typename dadaist_wrapper<SET_T<conflictor, hash_conflictor>>::type;

    SECTION("insert")
    {
        auto v = dadaist_set_t{};
        auto d = dadaism{};
        for (auto i = 0u; v.size() < n;) {
            try {
                auto s = d.next();
                v      = v.insert({i});
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.count({i}) == 1);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }

    SECTION("insert collisions")
    {
        auto vals = make_values_with_collisions(n);
        auto v    = dadaist_conflictor_set_t{};
        auto d    = dadaism{};
        for (auto i = 0u; v.size() < n;) {
            try {
                auto s = d.next();
                v      = v.insert({vals[i]});
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.count({vals[i]}) == 1);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }

    SECTION("erase")
    {
        auto v = dadaist_set_t{};
        auto d = dadaism{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert({i});
        for (auto i = 0u; v.size() > 0;) {
            try {
                auto s = d.next();
                v      = v.erase({i});
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.count({i}) == 0);
            for (auto i : test_irange(i, n))
                CHECK(v.count({i}) == 1);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }

    SECTION("erase collisisions")
    {
        auto vals = make_values_with_collisions(n);
        auto v    = dadaist_conflictor_set_t{};
        auto d    = dadaism{};
        for (auto i = 0u; i < n; ++i)
            v = v.insert({vals[i]});
        for (auto i = 0u; v.size() > 0;) {
            try {
                auto s = d.next();
                v      = v.erase({vals[i]});
                ++i;
            } catch (dada_error) {}
            for (auto i : test_irange(0u, i))
                CHECK(v.count({vals[i]}) == 0);
            for (auto i : test_irange(i, n))
                CHECK(v.count({vals[i]}) == 1);
        }
        CHECK(d.happenings > 0);
        IMMER_TRACE_E(d.happenings);
    }
}

namespace {
struct KeyType
{
    explicit KeyType(unsigned v)
        : value(v)
    {}
    unsigned value;
};

struct LookupType
{
    explicit LookupType(unsigned v)
        : value(v)
    {}
    unsigned value;
};

struct TransparentHash
{
    using hash_type      = std::hash<unsigned>;
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
        auto m = SET_T<KeyType, TransparentHash, std::equal_to<>>{};
        m      = m.insert(KeyType{1});

        CHECK(m.count(LookupType{1}) == 1);
        CHECK(m.count(LookupType{2}) == 0);
    }
}

void test_diff(unsigned old_num, unsigned add_num, unsigned remove_num)
{
    auto values = make_values_with_collisions(old_num + add_num);
    std::vector<conflictor> initial_values(values.begin(),
                                           values.begin() + old_num);
    std::vector<conflictor> new_values(values.begin() + old_num, values.end());
    auto set = make_test_set(initial_values);

    auto first_snapshot = set;
    CHECK(old_num == first_snapshot.size());

    // remove
    auto shuffle = initial_values;
    std::random_shuffle(shuffle.begin(), shuffle.end());
    std::vector<conflictor> remove_keys(shuffle.begin(),
                                        shuffle.begin() + remove_num);

    using key_set = std::unordered_set<conflictor, hash_conflictor>;
    key_set removed_keys(remove_keys.begin(), remove_keys.end());
    for (auto const& key : remove_keys)
        set = set.erase(key);
    CHECK(old_num - remove_num == set.size());

    // add
    key_set added_keys;
    for (auto const& data : new_values) {
        set = set.insert(data);
        added_keys.insert(data);
    }

    diff(
        first_snapshot,
        set,
        [&](auto const& data) { REQUIRE(added_keys.erase(data) > 0); },
        [&](auto const& data) { REQUIRE(removed_keys.erase(data) > 0); });

    CHECK(added_keys.empty());
    CHECK(removed_keys.empty());
}

TEST_CASE("diff")
{
    test_diff(16, 10, 10);
    test_diff(100, 10, 10);
    test_diff(1500, 10, 1000);
    test_diff(16, 1500, 10);
}
