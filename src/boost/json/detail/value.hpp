//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_VALUE_HPP
#define BOOST_JSON_DETAIL_VALUE_HPP

#include <boost/json/kind.hpp>
#include <boost/json/storage_ptr.hpp>
#include <cstdint>
#include <limits>
#include <new>
#include <utility>

BOOST_JSON_NS_BEGIN
namespace detail {

struct key_t
{
};

#if 0
template<class T>
struct to_number_limit
    : std::numeric_limits<T>
{
};

template<class T>
struct to_number_limit<T const>
    : to_number_limit<T>
{
};

template<>
struct to_number_limit<long long>
{
    static constexpr long long (min)() noexcept
    {
        return -9223372036854774784;
    }

    static constexpr long long (max)() noexcept
    {
        return 9223372036854774784;
    }
};

template<>
struct to_number_limit<unsigned long long>
{
    static constexpr
    unsigned long long (min)() noexcept
    {
        return 0;
    }

    static constexpr
    unsigned long long (max)() noexcept
    {
        return 18446744073709549568ULL;
    }
};
#else

template<class T>
class to_number_limit
{
    // unsigned

    static constexpr
    double min1(std::false_type)
    {
        return 0.0;
    }

    static constexpr
    double max1(std::false_type)
    {
        return max2u(std::integral_constant<
            bool, (std::numeric_limits<T>::max)() ==
            UINT64_MAX>{});
    }

    static constexpr
    double max2u(std::false_type)
    {
        return static_cast<double>(
            (std::numeric_limits<T>::max)());
    }

    static constexpr
    double max2u(std::true_type)
    {
        return 18446744073709549568.0;
    }

    // signed

    static constexpr
    double min1(std::true_type)
    {
        return min2s(std::integral_constant<
            bool, (std::numeric_limits<T>::max)() ==
            INT64_MAX>{});
    }

    static constexpr
    double min2s(std::false_type)
    {
        return static_cast<double>(
            (std::numeric_limits<T>::min)());
    }

    static constexpr
    double min2s(std::true_type)
    {
        return -9223372036854774784.0;
    }

    static constexpr
    double max1(std::true_type)
    {
        return max2s(std::integral_constant<
            bool, (std::numeric_limits<T>::max)() ==
            INT64_MAX>{});
    }

    static constexpr
    double max2s(std::false_type)
    {
        return static_cast<double>(
            (std::numeric_limits<T>::max)());
    }

    static constexpr
    double max2s(std::true_type)
    {
        return 9223372036854774784.0;
    }

public:
    static constexpr
    double (min)() noexcept
    {
        return min1(std::is_signed<T>{});
    }

    static constexpr
    double (max)() noexcept
    {
        return max1(std::is_signed<T>{});
    }
};

#endif

struct scalar
{
    storage_ptr sp; // must come first
    kind k;         // must come second
    union
    {
        bool b;
        std::int64_t i;
        std::uint64_t u;
        double d;
    };

    explicit
    scalar(storage_ptr sp_ = {}) noexcept
        : sp(std::move(sp_))
        , k(json::kind::null)
    {
    }

    explicit
    scalar(bool b_,
        storage_ptr sp_ = {}) noexcept
        : sp(std::move(sp_))
        , k(json::kind::bool_)
        , b(b_)
    {
    }

    explicit
    scalar(std::int64_t i_,
        storage_ptr sp_ = {}) noexcept
        : sp(std::move(sp_))
        , k(json::kind::int64)
        , i(i_)
    {
    }

    explicit
    scalar(std::uint64_t u_,
        storage_ptr sp_ = {}) noexcept
        : sp(std::move(sp_))
        , k(json::kind::uint64)
        , u(u_)
    {
    }

    explicit
    scalar(double d_,
        storage_ptr sp_ = {}) noexcept
        : sp(std::move(sp_))
        , k(json::kind::double_)
        , d(d_)
    {
    }
};

struct access
{
    template<class Value, class... Args>
    static
    Value&
    construct_value(Value* p, Args&&... args)
    {
        return *reinterpret_cast<
            Value*>(::new(p) Value(
            std::forward<Args>(args)...));
    }

    template<class KeyValuePair, class... Args>
    static
    KeyValuePair&
    construct_key_value_pair(
        KeyValuePair* p, Args&&... args)
    {
        return *reinterpret_cast<
            KeyValuePair*>(::new(p)
                KeyValuePair(
                    std::forward<Args>(args)...));
    }

    template<class Value>
    static
    char const*
    release_key(
        Value& jv,
        std::size_t& len) noexcept
    {
        BOOST_ASSERT(jv.is_string());
        jv.str_.sp_.~storage_ptr();
        return jv.str_.impl_.release_key(len);
    }

    using index_t = std::uint32_t;

    template<class KeyValuePair>
    static
    index_t&
    next(KeyValuePair& e) noexcept
    {
        return e.next_;
    }

    template<class KeyValuePair>
    static
    index_t const&
    next(KeyValuePair const& e) noexcept
    {
        return e.next_;
    }
};

} // detail
BOOST_JSON_NS_END

#endif
