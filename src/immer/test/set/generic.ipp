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

#include <catch.hpp>

#include <random>
#include <unordered_set>

template <typename T = unsigned>
auto make_generator()
{
    auto engine = std::default_random_engine{42};
    auto dist   = std::uniform_int_distribution<T>{};
    return std::bind(dist, engine);
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
        s = s.insert(i);
    return s;
}

auto make_test_set(const std::vector<conflictor>& vals)
{
    auto s = SET_T<conflictor, hash_conflictor>{};
    for (auto&& v : vals)
        s = s.insert(v);
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

TEST_CASE("basic insertion")
{
    auto v1 = SET_T<unsigned>{};
    CHECK(v1.count(42) == 0);

    auto v2 = v1.insert(42);
    CHECK(v1.count(42) == 0);
    CHECK(v2.count(42) == 1);

    auto v3 = v2.insert(42);
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

    for (auto i = 0u; i < N; ++i) {
        s = s.insert(vals[i]);
        CHECK(s.size() == i + 1);
        for (auto j : test_irange(0u, i + 1))
            CHECK(s.count(vals[j]) == 1);
        for (auto j : test_irange(i + 1u, N))
            CHECK(s.count(vals[j]) == 0);
    }
}

TEST_CASE("insert conflicts")
{
    constexpr auto N = 666u;
    auto vals        = make_values_with_collisions(N);
    auto s           = SET_T<conflictor, hash_conflictor>{};
    for (auto i = 0u; i < N; ++i) {
        s = s.insert(vals[i]);
        CHECK(s.size() == i + 1);
        for (auto j : test_irange(0u, i + 1))
            CHECK(s.count(vals[j]) == 1);
        for (auto j : test_irange(i + 1u, N))
            CHECK(s.count(vals[j]) == 0);
    }
}

TEST_CASE("erase a lot")
{
    constexpr auto N = 666u;
    auto gen         = make_generator();
    auto vals        = std::vector<unsigned>{};
    generate_n(back_inserter(vals), N, gen);

    auto s = SET_T<unsigned>{};
    for (auto i = 0u; i < N; ++i)
        s = s.insert(vals[i]);

    for (auto i = 0u; i < N; ++i) {
        s = s.erase(vals[i]);
        CHECK(s.size() == N - i - 1);
        for (auto j : test_irange(0u, i + 1))
            CHECK(s.count(vals[j]) == 0);
        for (auto j : test_irange(i + 1u, N))
            CHECK(s.count(vals[j]) == 1);
    }
}

TEST_CASE("erase conflicts")
{
    constexpr auto N = 666u;
    auto vals        = make_values_with_collisions(N);
    auto s           = SET_T<conflictor, hash_conflictor>{};
    for (auto i = 0u; i < N; ++i)
        s = s.insert(vals[i]);

    for (auto i = 0u; i < N; ++i) {
        s = s.erase(vals[i]);
        CHECK(s.size() == N - i - 1);
        for (auto j : test_irange(0u, i + 1))
            CHECK(s.count(vals[j]) == 0);
        for (auto j : test_irange(i + 1u, N))
            CHECK(s.count(vals[j]) == 1);
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
