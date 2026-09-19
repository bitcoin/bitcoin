// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_UTIL_REACHABILITY_H
#define BITCOIN_TEST_FUZZ_UTIL_REACHABILITY_H

#include <source_location>
#include <string_view>

/// A reachability condition explicitly registered by a fuzz target.
///
/// Goals are identified by their declaration location and aggregated across the
/// lifetime of the fuzz process. Set FUZZ_ENFORCE_REACHABILITY=1 to fail at
/// shutdown if a registered goal was never satisfied.
struct ReachabilityGoal {
    std::string_view message;
    std::source_location location;

    constexpr explicit ReachabilityGoal(
        std::string_view message,
        std::source_location location = std::source_location::current())
        : message{message}, location{location}
    {
    }
};

/// Register a goal before executing any fuzz inputs.
void RegisterReachabilityGoal(const ReachabilityGoal& goal);

/// Record whether a registered goal is satisfied by at least one fuzz input.
void ObserveReachabilityGoal(bool reached, const ReachabilityGoal& goal);

#endif // BITCOIN_TEST_FUZZ_UTIL_REACHABILITY_H
