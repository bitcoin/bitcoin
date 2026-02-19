// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_TOKENBUCKET_H
#define BITCOIN_UTIL_TOKENBUCKET_H

#include <util/time.h>

namespace util {

/** A token bucket rate limiter.
 *
 * Tokens are added at a steady rate (m_rate per second) up to a capacity
 * cap (m_cap). Tokens are removed by calling decrement(), which returns
 * false if the bucket is emptied.
 *
 * Typical usage:
 *   bucket.increment(now);   // refill based on elapsed time
 *   if (bucket.value() >= 1) bucket.decrement(1);  // consume a token
 */
template <typename Clock>
class TokenBucket
{
public:
    using clock = Clock;
    using time_point = typename Clock::time_point;
    using duration = typename Clock::duration;

    const double m_rate{1};     //!< Tokens added per second
    const double m_cap{0};      //!< Maximum token balance

    /** @param rate   Tokens added per second.
     *  @param value  Initial token balance (clamped to cap).
     *  @param cap    Maximum token balance. */
    TokenBucket(double rate, double value, double cap) : m_rate{rate}, m_cap{cap}, m_value{std::min(value, cap)} {}

    /** Refill tokens based on elapsed time since last call. No refill
     *  occurs on the first call (establishes the time baseline). */
    void increment(const time_point& now)
    {
        if (now > m_last_updated) {
            if (m_value < m_cap && m_last_updated.time_since_epoch().count() > 0) {
                double inc = m_rate * std::chrono::duration_cast<SecondsDouble>(now - m_last_updated).count();
                m_value = std::min(m_cap, m_value + inc);
            }
        }
        m_last_updated = now;
    }

    /** Consume n tokens. Returns false if the balance dropped to the given floor. */
    bool decrement(double n = 1.0, double floor = 0.0)
    {
        m_value -= n;
        return (m_value > floor);
    }

    /** Current token balance. */
    double value() const { return m_value; }

private:
    time_point m_last_updated{duration{0}};
    double m_value{0};
};

} // namespace util

#endif // BITCOIN_UTIL_TOKENBUCKET_H
