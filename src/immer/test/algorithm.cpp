#include <immer/array.hpp>
#include <immer/flex_vector.hpp>
#include <immer/map.hpp>
#include <immer/set.hpp>
#include <immer/table.hpp>
#include <immer/vector.hpp>

#include <immer/algorithm.hpp>

#include <catch2/catch.hpp>

struct thing
{
    int id = 0;
};
bool operator==(const thing& a, const thing& b) { return a.id == b.id; }

TEST_CASE("iteration exposes const")
{
    auto do_check = [](auto v) {
        using value_t = typename decltype(v)::value_type;
        immer::for_each(v, [](auto&& x) {
            static_assert(std::is_same<const value_t&, decltype(x)>::value, "");
        });
    };

    do_check(immer::vector<int>{});
    do_check(immer::flex_vector<int>{});
    do_check(immer::array<int>{});
    do_check(immer::map<int, int>{});
    do_check(immer::set<int>{});
    do_check(immer::table<thing>{});
}

TEST_CASE("chunked iteration exposes const")
{
    auto do_check = [](auto v) {
        using value_t = typename decltype(v)::value_type;
        immer::for_each_chunk(v, [](auto a, auto b) {
            static_assert(std::is_same<const value_t*, decltype(a)>::value, "");
            static_assert(std::is_same<const value_t*, decltype(b)>::value, "");
        });
    };

    do_check(immer::vector<int>{});
    do_check(immer::flex_vector<int>{});
    do_check(immer::array<int>{});
    do_check(immer::map<int, int>{});
    do_check(immer::set<int>{});
    do_check(immer::table<thing>{});
}

TEST_CASE("accumulate")
{
    auto do_check = [](auto v) {
        using value_t = typename decltype(v)::value_type;
        immer::accumulate(v, value_t{}, [](auto&& a, auto&& b) {
            static_assert(std::is_same<value_t&&, decltype(a)>::value, "");
            static_assert(std::is_same<const value_t&, decltype(b)>::value, "");
            return std::move(a);
        });
    };

    do_check(immer::vector<int>{});
    do_check(immer::flex_vector<int>{});
    do_check(immer::array<int>{});
    do_check(immer::map<int, int>{});
    do_check(immer::set<int>{});
    do_check(immer::table<thing>{});
}

TEST_CASE("diffing exposes const")
{
    auto do_check = [](auto v) {
        using value_t = typename decltype(v)::value_type;
        immer::diff(
            v,
            v,
            [](auto&& x) {
                static_assert(std::is_same<const value_t&, decltype(x)>::value,
                              "");
            },
            [](auto&& x) {
                static_assert(std::is_same<const value_t&, decltype(x)>::value,
                              "");
            },
            [](auto&& x, auto&& y) {
                static_assert(std::is_same<const value_t&, decltype(x)>::value,
                              "");
                static_assert(std::is_same<const value_t&, decltype(y)>::value,
                              "");
            });
    };

    do_check(immer::map<int, int>{});
    do_check(immer::set<int>{});
    do_check(immer::table<thing>{});
}

TEST_CASE("all_of")
{
    auto do_check = [](auto v) {
        using value_t = typename decltype(v)::value_type;
        immer::all_of(v, [](auto&& x) {
            static_assert(std::is_same<const value_t&, decltype(x)>::value, "");
            return true;
        });
    };

    do_check(immer::vector<int>{});
    do_check(immer::flex_vector<int>{});
    do_check(immer::array<int>{});
    // not supported
    // do_check(immer::map<int, int>{});
    // do_check(immer::set<int>{});
    // do_check(immer::table<thing>{});
}

TEST_CASE("update vectors")
{
    auto do_check = [](auto v) {
        if (false)
            (void) v.update(0, [](auto&& x) {
                using type_t = std::decay_t<decltype(x)>;
                // vectors do copy first the whole array, and then move the
                // copied value into the function
                static_assert(std::is_same<type_t&&, decltype(x)>::value, "");
                return x;
            });
    };

    do_check(immer::vector<int>{});
    do_check(immer::flex_vector<int>{});
    do_check(immer::array<int>{});
}

TEST_CASE("update maps")
{
    auto do_check = [](auto v) {
        (void) v.update(0, [](auto&& x) {
            using type_t = std::decay_t<decltype(x)>;
            // for maps, we actually do not make a copy at all but pase the
            // original instance directly, as const..
            static_assert(std::is_same<const type_t&, decltype(x)>::value, "");
            return x;
        });
    };

    do_check(immer::map<int, int>{});
    do_check(immer::table<thing>{});
}

TEST_CASE("update_if_exists maps")
{
    auto do_check = [](auto v) {
        (void) v.update_if_exists(0, [](auto&& x) {
            using type_t = std::decay_t<decltype(x)>;
            // for maps, we actually do not make a copy at all but pase the
            // original instance directly, as const..
            static_assert(std::is_same<const type_t&, decltype(x)>::value, "");
            return x;
        });
    };

    do_check(immer::map<int, int>{});
    do_check(immer::table<thing>{});
}

TEST_CASE("update vectors move")
{
    auto do_check = [](auto v) {
        if (false)
            (void) std::move(v).update(0, [](auto&& x) {
                using type_t = std::decay_t<decltype(x)>;
                // vectors do copy first the whole array, and then move the
                // copied value into the function
                static_assert(std::is_same<type_t&&, decltype(x)>::value, "");
                return x;
            });
    };

    do_check(immer::vector<int>{});
    do_check(immer::flex_vector<int>{});
    do_check(immer::array<int>{});
}

TEST_CASE("update maps move")
{
    auto do_check = [](auto v) {
        (void) std::move(v).update(0, [](auto&& x) {
            using type_t = std::decay_t<decltype(x)>;
            // for maps, we actually do not make a copy at all but pase the
            // original instance directly, as const..
            static_assert(std::is_same<const type_t&, decltype(x)>::value ||
                              std::is_same<type_t&&, decltype(x)>::value,
                          "");
            return x;
        });
    };

    do_check(immer::map<int, int>{});
    do_check(immer::table<thing>{});
}

TEST_CASE("update_if_exists maps move")
{
    auto do_check = [](auto v) {
        (void) std::move(v).update_if_exists(0, [](auto&& x) {
            using type_t = std::decay_t<decltype(x)>;
            // for maps, we actually do not make a copy at all but pase the
            // original instance directly, as const..
            static_assert(std::is_same<const type_t&, decltype(x)>::value ||
                              std::is_same<type_t&&, decltype(x)>::value,
                          "");
            return x;
        });
    };

    do_check(immer::map<int, int>{});
    do_check(immer::table<thing>{});
}
