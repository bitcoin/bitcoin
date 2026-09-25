// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Implementations of now() for NodeClock and MockableSteadyClock that delegate
// to _now_nondet(). This file is linked into bitcoin_util but NOT into
// bitcoinkernel, so calls from libbitcoinkernel source files produce link errors
// by design, preventing accidental nondeterminism.

#include <util/time.h>

NodeClock::time_point NodeClock::now() noexcept { return _now_nondet(); }

MockableSteadyClock::time_point MockableSteadyClock::now() noexcept { return _now_nondet(); }
