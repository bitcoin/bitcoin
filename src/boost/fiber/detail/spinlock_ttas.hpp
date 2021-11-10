
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_SPINLOCK_TTAS_H
#define BOOST_FIBERS_SPINLOCK_TTAS_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <random>
#include <thread>

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/cpu_relax.hpp>
#include <boost/fiber/detail/spinlock_status.hpp>

// based on informations from:
// https://software.intel.com/en-us/articles/benefitting-power-and-performance-sleep-loops
// https://software.intel.com/en-us/articles/long-duration-spin-wait-loops-on-hyper-threading-technology-enabled-intel-processors

namespace boost {
namespace fibers {
namespace detail {

class spinlock_ttas {
private:
    template< typename FBSplk >
    friend class spinlock_rtm;

    std::atomic< spinlock_status >              state_{ spinlock_status::unlocked };

public:
    spinlock_ttas() = default;

    spinlock_ttas( spinlock_ttas const&) = delete;
    spinlock_ttas & operator=( spinlock_ttas const&) = delete;

    void lock() noexcept {
        static thread_local std::minstd_rand generator{ std::random_device{}() };
        std::size_t collisions = 0 ;
        for (;;) {
            // avoid using multiple pause instructions for a delay of a specific cycle count
            // the delay of cpu_relax() (pause on Intel) depends on the processor family
            // the cycle count can not guaranteed from one system to the next
            // -> check the shared variable 'state_' in between each cpu_relax() to prevent
            //    unnecessarily long delays on some systems
            std::size_t retries = 0;
            // test shared variable 'status_'
            // first access to 'state_' -> chache miss
            // sucessive acccess to 'state_' -> cache hit
            // if 'state_' was released by other fiber
            // cached 'state_' is invalidated -> cache miss
            while ( spinlock_status::locked == state_.load( std::memory_order_relaxed) ) {
#if !defined(BOOST_FIBERS_SPIN_SINGLE_CORE)
                if ( BOOST_FIBERS_SPIN_BEFORE_SLEEP0 > retries) {
                    ++retries;
                    // give CPU a hint that this thread is in a "spin-wait" loop
                    // delays the next instruction's execution for a finite period of time (depends on processor family)
                    // the CPU is not under demand, parts of the pipeline are no longer being used
                    // -> reduces the power consumed by the CPU
                    // -> prevent pipeline stalls
                    cpu_relax();
                } else if ( BOOST_FIBERS_SPIN_BEFORE_YIELD > retries) {
                    ++retries;
                    // std::this_thread::sleep_for( 0us) has a fairly long instruction path length,
                    // combined with an expensive ring3 to ring 0 transition costing about 1000 cycles
                    // std::this_thread::sleep_for( 0us) lets give up this_thread the remaining part of its time slice
                    // if and only if a thread of equal or greater priority is ready to run
                    static constexpr std::chrono::microseconds us0{ 0 };
                    std::this_thread::sleep_for( us0);
                } else {
                    // std::this_thread::yield() allows this_thread to give up the remaining part of its time slice,
                    // but only to another thread on the same processor
                    // instead of constant checking, a thread only checks if no other useful work is pending
                    std::this_thread::yield();
                }
#else
                std::this_thread::yield();
#endif
            }
            // test-and-set shared variable 'status_'
            // everytime 'status_' is signaled over the bus, even if the test failes
            if ( spinlock_status::locked == state_.exchange( spinlock_status::locked, std::memory_order_acquire) ) {
                // spinlock now contended
                // utilize 'Binary Exponential Backoff' algorithm
                // linear_congruential_engine is a random number engine based on Linear congruential generator (LCG)
                std::uniform_int_distribution< std::size_t > distribution{
                    0, static_cast< std::size_t >( 1) << (std::min)(collisions, static_cast< std::size_t >( BOOST_FIBERS_CONTENTION_WINDOW_THRESHOLD)) };
                const std::size_t z = distribution( generator);
                ++collisions;
                for ( std::size_t i = 0; i < z; ++i) {
                    // -> reduces the power consumed by the CPU
                    // -> prevent pipeline stalls
                    cpu_relax();
                }
            } else {
                // success, thread has acquired the lock
                break;
            }
        }
    }

    bool try_lock() noexcept {
        return spinlock_status::unlocked == state_.exchange( spinlock_status::locked, std::memory_order_acquire);
    }

    void unlock() noexcept {
        state_.store( spinlock_status::unlocked, std::memory_order_release);
    }
};

}}}

#endif // BOOST_FIBERS_SPINLOCK_TTAS_H
