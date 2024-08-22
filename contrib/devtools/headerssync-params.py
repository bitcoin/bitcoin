#!/usr/bin/env python3
# Copyright (c) 2022 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Script to find the optimal parameters for the headerssync module through simulation."""

from math import log, exp, sqrt
from datetime import datetime, timedelta
import random

# Parameters:

# Aim for still working fine at some point in the future. [datetime]
TIME = datetime(2027, 4, 1)

# Expected block interval. [timedelta]
BLOCK_INTERVAL = timedelta(seconds=600)

# The number of headers corresponding to the minchainwork parameter. [headers]
MINCHAINWORK_HEADERS = 856760

# Combined processing bandwidth from all attackers to one victim. [bit/s]
# 6 Gbit/s is approximately the speed at which a single thread of a Ryzen 5950X CPU thread can hash
# headers. In practice, the victim's network bandwidth and network processing overheads probably
# impose a far lower number, but it's a useful upper bound.
ATTACK_BANDWIDTH = 6000000000

# How much additional permanent memory usage are attackers (jointly) allowed to cause in the victim,
# expressed as fraction of the normal memory usage due to mainchain growth, for the duration the
# attack is sustained. [unitless]
# 0.2 means that attackers, while they keep up the attack, can cause permanent memory usage due to
# headers storage to grow at 1.2 header per BLOCK_INTERVAL.
ATTACK_FRACTION = 0.2

# When this is set, the mapping from period size to memory usage (at optimal buffer size for that
# period) is assumed to be convex. This greatly speeds up the computation, and does not appear
# to influence the outcome. Set to False for a stronger guarantee to get the optimal result.
ASSUME_CONVEX = True

# Explanation:
#
#  The headerssync module implements a DoS protection against low-difficulty header spam which does
#  not rely on checkpoints. In short it works as follows:
#
#  - (initial) header synchronization is split into two phases:
#    - A commitment phase, in which headers are downloaded from the peer, and a very compact
#      commitment to them is remembered in per-peer memory. The commitment phase ends when the
#      received chain's combined work reaches a predetermined threshold.
#    - A redownload phase, during which the headers are downloaded a second time from the same peer,
#      and compared against the commitment constructed in the first phase. If there is a match, the
#      redownloaded headers are fed to validation and accepted into permanent storage.
#
#    This separation guarantees that no headers are accepted into permanent storage without
#    requiring the peer to first prove the chain actually has sufficient work.
#
#  - To actually implement this commitment mechanism, the following approach is used:
#    - Keep a *1 bit* commitment (constructed using a salted hash function), for every block whose
#      height is a multiple of {period} plus an offset value. If RANDOMIZE_OFFSET, the offset,
#      like the salt, is chosen randomly when the synchronization starts and kept fixed afterwards.
#    - When redownloading, headers are fed through a per-peer queue that holds {bufsize} headers,
#      before passing them to validation. All the headers in this queue are verified against the
#      commitment bits created in the first phase before any header is released from it. This means
#      {bufsize/period} bits are checked "on top of" each header before actually processing it,
#      which results in a commitment structure with roughly {bufsize/period} bits of security, as
#      once a header is modified, due to the prevhash inclusion, all future headers necessarily
#      change as well.
#
#  The question is what these {period} and {bufsize} parameters need to be set to. This program
#  exhaustively tests a range of values to find the optimal choice, taking into account:
#
#  - Minimizing the (maximum of) two scenarios that trigger per-peer memory usage:
#
#    - When downloading a (likely honest) chain that reaches the chainwork threshold after {n}
#      blocks, and then redownloads them, we will consume per-peer memory that is sufficient to
#      store {n/period} commitment bits and {bufsize} headers. We only consider attackers without
#      sufficient hashpower (as otherwise they are from a PoW perspective not attackers), which
#      means {n} is restricted to the honest chain's length before reaching minchainwork.
#
#    - When downloading a (likely false) chain of {n} headers that never reaches the chainwork
#      threshold, we will consume per-peer memory that is sufficient to store {n/period}
#      commitment bits. Such a chain may be very long, by exploiting the timewarp bug to avoid
#      ramping up difficulty. There is however an absolute limit on how long such a chain can be: 6
#      blocks per second since genesis, due to the increasing MTP consensus rule.
#
#  - Not gratuitously preventing synchronizing any valid chain, however difficult such a chain may
#    be to construct. In particular, the above scenario with an enormous timewarp-expoiting chain
#    cannot simply be ignored, as it is legal that the honest main chain is like that. We however
#    do not bother minimizing the memory usage in that case (because a billion-header long honest
#    chain will inevitably use far larger amounts of memory than designed for).
#
#  - Keep the rate at which attackers can get low-difficulty headers accepted to the block index
#    negligible. Specifically, the possibility exists for an attacker to send the honest main
#    chain's headers during the commitment phase, but then start deviating at an attacker-chosen
#    point by sending novel low-difficulty headers instead. Depending on how high we set the
#    {bufsize/period} ratio, we can make the probability that such a header makes it in
#    arbitrarily small, but at the cost of higher memory during the redownload phase. It turns out,
#    some rate of memory usage growth is expected anyway due to chain growth, so permitting the
#    attacker to increase that rate by a small factor isn't concerning. The attacker may start
#    somewhat later than genesis, as long as the difficulty doesn't get too high. This reduces
#    the attacker bandwidth required at the cost of higher PoW needed for constructing the
#    alternate chain. This trade-off is ignored here, as it results in at most a small constant
#    factor in attack rate.


# System properties:

# Headers in the redownload buffer are stored without prevhash. [bits]
COMPACT_HEADER_SIZE = 48 * 8

# How many bits a header uses in P2P protocol. [bits]
NET_HEADER_SIZE = 81 * 8

# How many headers are sent at once. [headers]
HEADER_BATCH_COUNT = 2000

# Whether or not the offset of which blocks heights get checksummed is randomized.
RANDOMIZE_OFFSET = True

# Timestamp of the genesis block
GENESIS_TIME = datetime(2009, 1, 3)

# Derived values:

# What rate of headers worth of RAM attackers are allowed to cause in the victim. [headers/s]
LIMIT_HEADERRATE = ATTACK_FRACTION / BLOCK_INTERVAL.total_seconds()

# How many headers can attackers (jointly) send a victim per second. [headers/s]
NET_HEADERRATE = ATTACK_BANDWIDTH / NET_HEADER_SIZE

# What fraction of headers sent by attackers can at most be accepted by a victim [unitless]
LIMIT_FRACTION = LIMIT_HEADERRATE / NET_HEADERRATE

# How many headers we permit attackers to cause being accepted per attack. [headers/attack]
ATTACK_HEADERS = LIMIT_FRACTION * MINCHAINWORK_HEADERS


def find_max_headers(when):
    """Compute the maximum number of headers a valid Bitcoin chain can have at given time."""
    # When exploiting the timewarp attack, this can be up to 6 per second since genesis.
    return 6 * ((when - GENESIS_TIME) // timedelta(seconds=1))


def lambert_w(value):
    """Solve the equation x*exp(x)=value (x > 0, value > 0)."""
    # Initial approximation.
    approx = max(log(value), 0.0)
    for _ in range(10):
        # Newton-Rhapson iteration steps.
        approx += (value * exp(-approx) - approx) / (approx + 1.0)
    return approx


def attack_rate(period, bufsize, limit=None):
    """Compute maximal accepted headers per attack in (period, bufsize) configuration.

    If limit is provided, the computation is stopped early when the result is known to exceed the
    value in limit.
    """

    max_rate = None
    max_honest = None
    # Let the current batch 0 being received be the first one in which the attacker starts lying.
    # They will only ever start doing so right after a commitment block, but where that is can be
    # in a number of places. Let honest be the number of honest headers in this current batch,
    # preceding the forged ones.
    for honest in range(HEADER_BATCH_COUNT):
        # The number of headers the attack under consideration will on average get accepted.
        # This is the number being computed.
        rate = 0

        # Iterate over the possible alignments of commitments w.r.t. the first batch. In case
        # the alignments are randomized, try all values. If not, the attacker can know/choose
        # the alignment, and will always start forging right after a commitment.
        if RANDOMIZE_OFFSET:
            align_choices = list(range(period))
        else:
            align_choices = [(honest - 1) % period]
        # Now loop over those possible alignment values, computing the average attack rate
        # over them by dividing each contribution by len(align_choices).
        for align in align_choices:
            # These state variables capture the situation after receiving the first batch.
            # - The number of headers received after the last commitment for an honest block:
            after_good_commit = HEADER_BATCH_COUNT - honest + ((honest - align - 1) % period)
            # - The number of forged headers in the redownload buffer:
            forged_in_buf = HEADER_BATCH_COUNT - honest

            # Now iterate over the next batches of headers received, adding contributions to the
            # rate variable.
            while True:
                # Process the first HEADER_BATCH_COUNT headers in the buffer:
                accept_forged_headers = max(forged_in_buf - bufsize, 0)
                forged_in_buf -= accept_forged_headers
                if accept_forged_headers:
                    # The probability the attack has not been detected yet at this point:
                    prob = 0.5 ** (after_good_commit // period)
                    # Update attack rate, divided by align_choices to average over the alignments.
                    rate += accept_forged_headers * prob / len(align_choices)
                    # If this means we exceed limit, bail out early (performance optimization).
                    if limit is not None and rate >= limit:
                        return rate, None
                    # If the maximal term being added is negligible compared to rate, stop
                    # iterating.
                    if HEADER_BATCH_COUNT * prob < 1.0e-16 * rate * len(align_choices):
                        break
                # Update state from a new incoming batch (which is all forged)
                after_good_commit += HEADER_BATCH_COUNT
                forged_in_buf += HEADER_BATCH_COUNT

        if max_rate is None or rate > max_rate:
            max_rate = rate
            max_honest = honest

    return max_rate, max_honest


def memory_usage(period, bufsize, when):
    """How much memory (max,mainchain,timewarp) does the (period,bufsize) configuration need?"""

    # Per-peer memory usage for a timewarp chain that never meets minchainwork
    mem_timewarp = find_max_headers(when) // period
    # Per-peer memory usage for being fed the main chain
    mem_mainchain = (MINCHAINWORK_HEADERS // period) + bufsize * COMPACT_HEADER_SIZE
    # Maximum per-peer memory usage
    max_mem = max(mem_timewarp, mem_mainchain)

    return max_mem, mem_mainchain, mem_timewarp

def find_bufsize(period, attack_headers, when, max_mem=None, min_bufsize=1):
    """Determine how big bufsize needs to be given a specific period length.

    Given a period, find the smallest value of bufsize such that the attack rate against the
    (period, bufsize) configuration is below attack_headers. If max_mem is provided, and no
    such bufsize exists that needs less than max_mem bits of memory, None is returned.
    min_bufsize is the minimal result to be considered."""

    if max_mem is None:
        succ_buf = min_bufsize - 1
        fail_buf = min_bufsize
        # First double iteratively until an upper bound for failure is found.
        while True:
            if attack_rate(period, fail_buf, attack_headers)[0] < attack_headers:
                break
            succ_buf, fail_buf = fail_buf, 3 * fail_buf - 2 * succ_buf
    else:
        # If a long low-work header chain exists that exceeds max_mem already, give up.
        if find_max_headers(when) // period > max_mem:
            return None
        # Otherwise, verify that the maximal buffer size that permits a mainchain sync with less
        # than max_mem memory is sufficient to get the attack rate below attack_headers. If not,
        # also give up.
        max_buf = (max_mem - (MINCHAINWORK_HEADERS // period)) // COMPACT_HEADER_SIZE
        if max_buf < min_bufsize:
            return None
        if attack_rate(period, max_buf, attack_headers)[0] >= attack_headers:
            return None
        # If it is sufficient, that's an upper bound to start our search.
        succ_buf = min_bufsize - 1
        fail_buf = max_buf

    # Then perform a bisection search to narrow it down.
    while fail_buf > succ_buf + 1:
        try_buf = (succ_buf + fail_buf) // 2
        if attack_rate(period, try_buf, attack_headers)[0] >= attack_headers:
            succ_buf = try_buf
        else:
            fail_buf = try_buf
    return fail_buf


def optimize(when):
    """Find the best (period, bufsize) configuration."""

    # When period*bufsize = memory_scale, the per-peer memory for a mainchain sync and a maximally
    # long low-difficulty header sync are equal.
    memory_scale = (find_max_headers(when) - MINCHAINWORK_HEADERS) / COMPACT_HEADER_SIZE
    # Compute approximation for {bufsize/period}, using a formula for a simplified problem.
    approx_ratio = lambert_w(log(4) * memory_scale / ATTACK_HEADERS**2) / log(4)
    # Use those for a first attempt.
    print("Searching configurations:")
    period = int(sqrt(memory_scale / approx_ratio) + 0.5)
    bufsize = find_bufsize(period, ATTACK_HEADERS, when)
    mem = memory_usage(period, bufsize, when)
    best = (period, bufsize, mem)
    maps = [(period, bufsize), (MINCHAINWORK_HEADERS + 1, None)]
    print(f"- Initial: period={period}, buffer={bufsize}, mem={mem[0] / 8192:.3f} KiB")

    # Consider all period values between 1 and MINCHAINWORK_HEADERS, except the one just tried.
    periods = [iv for iv in range(1, MINCHAINWORK_HEADERS + 1) if iv != period]
    # Iterate, picking a random element from periods, computing its corresponding bufsize, and
    # then using the result to shrink the period.
    while True:
        # Remove all periods whose memory usage for low-work long chain sync exceed the best
        # memory usage we've found so far.
        periods = [p for p in periods if find_max_headers(when) // p < best[2][0]]
        # Stop if there is nothing left to try.
        if len(periods) == 0:
            break
        # Pick a random remaining option for period size, and compute corresponding bufsize.
        period = periods.pop(random.randrange(len(periods)))
        # The buffer size (at a given attack level) cannot shrink as the period grows. Find the
        # largest period smaller than the selected one we know the buffer size for, and use that
        # as a lower bound to find_bufsize.
        min_bufsize = max([(p, b) for p, b in maps if p < period] + [(0,0)])[1]
        bufsize = find_bufsize(period, ATTACK_HEADERS, when, best[2][0], min_bufsize)
        if bufsize is not None:
            # We found a (period, bufsize) configuration with better memory usage than our best
            # so far. Remember it for future lower bounds.
            maps.append((period, bufsize))
            mem = memory_usage(period, bufsize, when)
            assert mem[0] <= best[2][0]
            if ASSUME_CONVEX:
                # Remove all periods that are on the other side of the former best as the new
                # best.
                periods = [p for p in periods if (p < best[0]) == (period < best[0])]
            best = (period, bufsize, mem)
            print(f"- New best: period={period}, buffer={bufsize}, mem={mem[0] / 8192:.3f} KiB")
        else:
            # The (period, bufsize) configuration we found is worse than what we already had.
            if ASSUME_CONVEX:
                # Remove all periods that are on the other side of the tried configuration as the
                # best one.
                periods = [p for p in periods if (p < period) == (best[0] < period)]

    # Return the result.
    period, bufsize, _ = best
    return period, bufsize


def analyze(when):
    """Find the best configuration and print it out."""

    period, bufsize = optimize(when)
    # Compute accurate statistics for the best found configuration.
    _, mem_mainchain, mem_timewarp = memory_usage(period, bufsize, when)
    headers_per_attack, _ = attack_rate(period, bufsize)
    attack_volume = NET_HEADER_SIZE * MINCHAINWORK_HEADERS
    # And report them.
    print()
    print("Optimal configuration:")
    print()
    print("//! Store one header commitment per HEADER_COMMITMENT_PERIOD blocks.")
    print(f"constexpr size_t HEADER_COMMITMENT_PERIOD{{{period}}};")
    print()
    print("//! Only feed headers to validation once this many headers on top have been")
    print("//! received and validated against commitments.")
    print(f"constexpr size_t REDOWNLOAD_BUFFER_SIZE{{{bufsize}}};"
          f" // {bufsize}/{period} = ~{bufsize/period:.1f} commitments")
    print()
    print("Properties:")
    print(f"- Per-peer memory for mainchain sync: {mem_mainchain / 8192:.3f} KiB")
    print(f"- Per-peer memory for timewarp attack: {mem_timewarp / 8192:.3f} KiB")
    print(f"- Attack rate: {1/headers_per_attack:.1f} attacks for 1 header of memory growth")
    print(f"  (where each attack costs {attack_volume / 8388608:.3f} MiB bandwidth)")


analyze(TIME)
