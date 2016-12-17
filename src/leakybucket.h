// Copyright (c) 2015 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LEAKYBUCKET_H
#define BITCOIN_LEAKYBUCKET_H

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include <boost/chrono/chrono.hpp>

/** Default value for the maximum amount of data that can be received in a burst */
static const int64_t DEFAULT_MAX_RECV_BURST = LONG_LONG_MAX;
/** Default value for the maximum amount of data that can be sent in a burst */
static const int64_t DEFAULT_MAX_SEND_BURST = LONG_LONG_MAX;
/** Default value for the average amount of data received per second */
static const int64_t DEFAULT_AVE_RECV = LONG_LONG_MAX;
/** Default value for the average amount of data sent per second */
static const int64_t DEFAULT_AVE_SEND = LONG_LONG_MAX;
/** If we have to break the transmission up into chunks, this is the minimum send chunk size */
static const int64_t SEND_SHAPER_MIN_FRAG = 256;
/** If we have to break the transmission up into chunks, this is the minimum receive chunk size */
static const int64_t RECV_SHAPER_MIN_FRAG = 256;


class CLeakyBucket
{
protected:
    typedef boost::chrono::steady_clock CClock;

    int64_t level; // Current level of the bucket
    int64_t max;   // Maximum quantity allowed
    int64_t fill;  // Average rate per second
    static CClock clock;
    boost::chrono::time_point<CClock> lastFill;

    // This function is called internally to fill the leaky bucket based on the time difference between now and the last time the function was called.
    void fillIt()
    {
        boost::chrono::time_point<CClock> now = clock.now();
        CClock::duration elapsed(now - lastFill);
        int64_t msElapsed = boost::chrono::duration_cast<boost::chrono::milliseconds>(elapsed).count();
        if (msElapsed > 100) // note in practice msElapsed can be < 0, something to do with hyperthreading so reduce don't eliminate this conditional
        {
            lastFill = now;
            level += (fill * msElapsed) / 1000;
            if (level > max)
                level = max;
        }
    }

public:
    CLeakyBucket(int64_t maxp, int64_t fillp, int64_t startLevel = LONG_LONG_MAX) : max(maxp), fill(fillp)
    {
        lastFill = clock.now();
        // set the initial level to either what is specified by the user or to the maximum
        level = (startLevel < max) ? startLevel : max;
    }

    // Stop the operation of this leaky bucket (always return available tokens)
    // use "set" to restart
    void disable(void)
    {
        fill = LONG_LONG_MAX;
        max = LONG_LONG_MAX;
    }

    // Access the values in this bucket
    void get(int64_t* maxp, int64_t* fillp, int64_t* levelp = NULL)
    {
        if (maxp)
            *maxp = max;
        if (fillp)
            *fillp = fill;
        if (levelp)
            *levelp = level;
    }

    // Change the settings of the leaky bucket
    void set(int64_t maxp, int64_t fillp)
    {
        max = maxp;
        fill = fillp;
        if (level > max)
            level = max;        // if pinching, slow traffic quickly.
        lastFill = clock.now(); // need to reset the lastFill time in case we are turning on this leaky bucket.
    }

    // Return the # tokens available if that amount is larger than the cutoff, otherwise return 0
    int64_t available(int64_t cutoff = 0)
    {
        if (fill == LONG_LONG_MAX)
            return LONG_LONG_MAX; // shaping is off
        fillIt();
        return (level > cutoff) ? level : 0;
    }

    // Try to use amt tokens.  Returns TRUE if the tokens were consumed, false otherwise
    bool try_leak(int64_t amt)
    {
        if (fill == LONG_LONG_MAX)
            return true; // leaky bucket is turned off.
        assert(amt >= 0);
        fillIt();
        if (level >= amt) {
            level -= amt;
            return true;
        }
        return false;
    }

    // This function reduces the level in the bucket by amt, even if that makes the level negative, and returns true if the level is >= 0.  This function is useful in a situation like data receipt (with soft limits) where you are not certain how many bytes will be received until after you have received them.
    bool leak(int64_t amt)
    {
        if (fill == LONG_LONG_MAX)
            return true; // leaky bucket is turned off.
        fillIt();
        level -= amt;
        return (level >= 0);
    }
};

#endif
