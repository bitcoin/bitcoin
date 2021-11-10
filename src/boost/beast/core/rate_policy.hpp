//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_RATE_POLICY_HPP
#define BOOST_BEAST_CORE_RATE_POLICY_HPP

#include <boost/beast/core/detail/config.hpp>
#include <cstdint>
#include <limits>

namespace boost {
namespace beast {

/** Helper class to assist implementing a <em>RatePolicy</em>.

    This class is used by the implementation to gain access to the
    private members of a user-defined object meeting the requirements
    of <em>RatePolicy</em>. To use it, simply declare it as a friend
    in your class:
    
    @par Example
    @code
    class custom_rate_policy
    {
        friend class beast::rate_policy_access;
        ...
    @endcode

    @par Concepts

    @li <em>RatePolicy</em>

    @see beast::basic_stream
*/
class rate_policy_access
{
private:
    template<class, class, class>
    friend class basic_stream;

    template<class Policy>
    static
    std::size_t
    available_read_bytes(Policy& policy)
    {
        return policy.available_read_bytes();
    }

    template<class Policy>
    static
    std::size_t
    available_write_bytes(Policy& policy)
    {
        return policy.available_write_bytes();
    }

    template<class Policy>
    static
    void
    transfer_read_bytes(
        Policy& policy, std::size_t n)
    {
        return policy.transfer_read_bytes(n);
    }

    template<class Policy>
    static
    void
    transfer_write_bytes(
        Policy& policy, std::size_t n)
    {
        return policy.transfer_write_bytes(n);
    }

    template<class Policy>
    static
    void
    on_timer(Policy& policy)
    {
        return policy.on_timer();
    }
};

//------------------------------------------------------------------------------

/** A rate policy with unlimited throughput.

    This rate policy object does not apply any rate limit.

    @par Concepts

    @li <em>RatePolicy</em>

    @see beast::basic_stream, beast::tcp_stream
*/
class unlimited_rate_policy
{
    friend class rate_policy_access;

    static std::size_t constexpr all =
        (std::numeric_limits<std::size_t>::max)();

    std::size_t
    available_read_bytes() const noexcept
    {
        return all;
    }

    std::size_t
    available_write_bytes() const noexcept
    {
        return all;
    }

    void
    transfer_read_bytes(std::size_t) const noexcept
    {
    }

    void
    transfer_write_bytes(std::size_t) const noexcept
    {
    }

    void
    on_timer() const noexcept
    {
    }
};

//------------------------------------------------------------------------------

/** A rate policy with simple, configurable limits on reads and writes.

    This rate policy allows for simple individual limits on the amount
    of bytes per second allowed for reads and writes.

    @par Concepts

    @li <em>RatePolicy</em>

    @see beast::basic_stream
*/
class simple_rate_policy
{
    friend class rate_policy_access;

    static std::size_t constexpr all =
        (std::numeric_limits<std::size_t>::max)();

    std::size_t rd_remain_ = all;
    std::size_t wr_remain_ = all;
    std::size_t rd_limit_ = all;
    std::size_t wr_limit_ = all;

    std::size_t
    available_read_bytes() const noexcept
    {
        return rd_remain_;
    }

    std::size_t
    available_write_bytes() const noexcept
    {
        return wr_remain_;
    }

    void
    transfer_read_bytes(std::size_t n) noexcept
    {
        if( rd_remain_ != all)
            rd_remain_ =
                (n < rd_remain_) ? rd_remain_ - n : 0;
    }

    void
    transfer_write_bytes(std::size_t n) noexcept
    {
        if( wr_remain_ != all)
            wr_remain_ =
                (n < wr_remain_) ? wr_remain_ - n : 0;
    }

    void
    on_timer() noexcept
    {
        rd_remain_ = rd_limit_;
        wr_remain_ = wr_limit_;
    }

public:
    /// Set the limit of bytes per second to read
    void
    read_limit(std::size_t bytes_per_second) noexcept
    {
        rd_limit_ = bytes_per_second;
        if( rd_remain_ > bytes_per_second)
            rd_remain_ = bytes_per_second;
    }

    /// Set the limit of bytes per second to write
    void
    write_limit(std::size_t bytes_per_second) noexcept
    {
        wr_limit_ = bytes_per_second;
        if( wr_remain_ > bytes_per_second)
            wr_remain_ = bytes_per_second;
    }
};

} // beast
} // boost

#endif
