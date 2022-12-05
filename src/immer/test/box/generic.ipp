//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#ifndef BOX_T
#error "define the box template to use in BOX_T"
#endif

#include <catch.hpp>

TEST_CASE("construction and copy")
{
    auto x = BOX_T<int>{};
    CHECK(x == 0);

    auto y = x;
    CHECK(&x.get() == &y.get());

    auto z = std::move(x);
    CHECK(&z.get() == &y.get());
}

TEST_CASE("equality")
{
    auto x = BOX_T<int>{};
    auto y = x;
    CHECK(x == 0.0f);
    CHECK(x == y);
    CHECK(x == BOX_T<int>{});
    CHECK(x != BOX_T<int>{42});
}

struct counters
{
    unsigned construct = 0;
    unsigned copy      = 0;
    unsigned compare   = 0;

    static counters* p_;
    struct scope
    {
        scope(counters* p)
        {
            assert(p == p_ || !p_);
            p_ = p;
        }
        ~scope() { p_ = nullptr; }
    };
    scope use() { return {this}; }
};
counters* counters::p_ = nullptr;

template <typename T>
struct spionage
{
    T value = {};

    spionage()
        : value{}
    {
        ++counters::p_->construct;
    }
    spionage(T v)
        : value{v}
    {
        ++counters::p_->construct;
    }
    spionage(const spionage& v)
        : value{v.value}
    {
        ++counters::p_->copy;
    }
    spionage(spionage&& v)
        : value{std::move(v.value)}
    {
        ++counters::p_->copy;
    }
    bool operator==(const spionage& b) const
    {
        ++counters::p_->compare;
        return value == b.value;
    }
    bool operator!=(const spionage& b) const
    {
        ++counters::p_->compare;
        return value != b.value;
    }
    bool operator==(const T& b) const
    {
        ++counters::p_->compare;
        return value == b;
    }
    bool operator!=(const T& b) const
    {
        ++counters::p_->compare;
        return value != b;
    }
};

TEST_CASE("equality not too much construction")
{
    auto c = counters{};
    auto s = c.use();

    auto x = BOX_T<spionage<int>>{};
    auto y = x;
    CHECK(x == 0.0f);
    CHECK(x == y);
    CHECK(c.copy == 0);
    CHECK(c.construct == 1);

    CHECK(x == BOX_T<spionage<int>>{});
    CHECK(x != BOX_T<spionage<int>>{42});
    CHECK(c.copy == 0);
    CHECK(c.construct == 3);
}

TEST_CASE("update")
{
    auto x = BOX_T<int>{};
    auto y = x.update([](auto v) { return v + 1; });
    CHECK(x == 0);
    CHECK(y == 1);
}

TEST_CASE("update move")
{
    auto x    = BOX_T<int>{};
    auto addr = &x.get();
    auto y    = std::move(x).update(
        [](auto&& v) { return std::forward<decltype(v)>(v) + 1; });

    CHECK(y == 1);
    if (std::is_empty<typename BOX_T<int>::memory_policy::refcount>::value) {
        CHECK(&y.get() != addr);
    } else {
        CHECK(&y.get() == addr);
    }
}
