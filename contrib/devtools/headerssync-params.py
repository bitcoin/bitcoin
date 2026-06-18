#!/usr/bin/env python3
# Copyright (c) 2022 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Script to find the optimal parameters for the headerssync module through simulation."""

from datetime import datetime, timedelta
from math import log, exp, sqrt, expm1
import random
import sys

# Parameters:

# Aim for still working fine at some point in the future. [datetime]
TIME = datetime(2028, 10, 10)

# Expected block interval. [timedelta]
BLOCK_INTERVAL = timedelta(seconds=600)

# The number of headers corresponding to the minchainwork parameter. [headers]
MINCHAINWORK_HEADERS = 938343

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
#      height is a multiple of {period} plus an offset value. The offset, like the salt, is chosen
#      randomly when the synchronization starts and kept fixed afterwards.
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

# Timestamp of the genesis block
GENESIS_TIME = datetime(2009, 1, 3)

# Derived values:

# What rate of headers worth of RAM attackers are allowed to cause in the victim. [headers/s]
LIMIT_HEADERRATE = ATTACK_FRACTION / BLOCK_INTERVAL.total_seconds()

# How many headers can attackers (jointly) send a victim per second. [headers/s]
NET_HEADERRATE = ATTACK_BANDWIDTH / NET_HEADER_SIZE

# What fraction of headers sent by attackers can at most be accepted by a victim [unitless]
LIMIT_FRACTION = LIMIT_HEADERRATE / NET_HEADERRATE


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

    # Each batch's probability is a factor 2**(HEADER_BATCH_COUNT/period) smaller than the previous
    # one, so the contributions of all future batches form a geometric series. future_limit is an
    # upper bound on the ratio between that future sum and a single batch's HEADER_BATCH_COUNT*prob.
    future_limit = HEADER_BATCH_COUNT / expm1(log(2) * HEADER_BATCH_COUNT / period)
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

        # Iterate over all possible alignments of commitments w.r.t. the first batch. Since the
        # offset is randomized, the attacker cannot know/choose it, so we try all {period} values,
        # computing the average attack rate over them by dividing each contribution by period.
        for align in range(period):
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
                    # Update attack rate, divided by period to average over the alignments.
                    rate += accept_forged_headers * prob / period
                    # If this means we exceed limit, bail out early (performance optimization).
                    if limit is not None and rate >= limit:
                        return rate, None
                    # If the maximal term being added is negligible compared to rate, stop
                    # iterating.
                    if future_limit * prob < 1.0e-16 * rate:
                        break
                # Update state from a new incoming batch (which is all forged)
                after_good_commit += HEADER_BATCH_COUNT
                forged_in_buf += HEADER_BATCH_COUNT

        if max_rate is None or rate > max_rate:
            max_rate = rate
            max_honest = honest

    return max_rate, max_honest


def memory_usage(period, bufsize, max_headers, minchainwork_headers):
    """Peak per-peer memory the (period,bufsize) configuration needs (the larger of the timewarp
    and mainchain scenarios)."""

    # Per-peer memory usage for a timewarp chain that never meets minchainwork
    mem_timewarp = max_headers // period
    # Per-peer memory usage for being fed the main chain
    mem_mainchain = (minchainwork_headers // period) + bufsize * COMPACT_HEADER_SIZE
    # The peak per-peer memory usage is the larger of the two.
    return max(mem_timewarp, mem_mainchain)

def find_bufsize(period, attack_headers, max_headers, minchainwork_headers, max_mem=None, min_bufsize=1):
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
        if max_headers // period > max_mem:
            return None
        # Otherwise, verify that the maximal buffer size that permits a mainchain sync with less
        # than max_mem memory is sufficient to get the attack rate below attack_headers. If not,
        # also give up.
        max_buf = (max_mem - (minchainwork_headers // period)) // COMPACT_HEADER_SIZE
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


def compute(max_headers, minchainwork_headers, attack_headers):
    """Find the best (period, bufsize) configuration for:

    - No more than max_headers headers are possible (6 per second since genesis).
    - There are minchainwork_headers in the minchainwork chain.
    - Up to attack_headers low-difficulty headers are allowed to be accepted by
      the victim, per attack."""

    # When period*bufsize = memory_scale, the per-peer memory for a mainchain sync and a maximally
    # long low-difficulty header sync are equal.
    memory_scale = (max_headers - minchainwork_headers) / COMPACT_HEADER_SIZE
    # Compute approximation for {bufsize/period}, using a formula for a simplified problem.
    approx_ratio = lambert_w(log(4) * memory_scale / attack_headers**2) / log(4)
    # Use those for a first attempt.
    period = int(sqrt(memory_scale / approx_ratio) + 0.5)
    bufsize = find_bufsize(period, attack_headers, max_headers, minchainwork_headers)
    mem = memory_usage(period, bufsize, max_headers, minchainwork_headers)
    best = (period, bufsize, mem)
    maps = [(period, bufsize), (minchainwork_headers + 1, None)]

    # Consider all period values between 1 and minchainwork_headers, except the one just tried.
    periods = [iv for iv in range(1, minchainwork_headers + 1) if iv != period]
    # Iterate, picking a random element from periods, computing its corresponding bufsize, and
    # then using the result to shrink the period.
    while True:
        # Remove all periods whose memory usage for low-work long chain sync exceed the best
        # memory usage we've found so far.
        periods = [p for p in periods if max_headers // p < best[2]]
        # Stop if there is nothing left to try.
        if len(periods) == 0:
            break
        # Pick a random remaining option for period size, and compute corresponding bufsize.
        period = periods.pop(random.randrange(len(periods)))
        # The buffer size (at a given attack level) cannot shrink as the period grows. Find the
        # largest period smaller than the selected one we know the buffer size for, and use that
        # as a lower bound to find_bufsize.
        min_bufsize = max([(p, b) for p, b in maps if p < period] + [(0,0)])[1]
        bufsize = find_bufsize(period, attack_headers, max_headers, minchainwork_headers, best[2], min_bufsize)
        if bufsize is not None:
            # We found a (period, bufsize) configuration with better memory usage than our best
            # so far. Remember it for future lower bounds.
            maps.append((period, bufsize))
            mem = memory_usage(period, bufsize, max_headers, minchainwork_headers)
            assert mem <= best[2]
            # Remove all periods that are on the other side of the former best as the new
            # best.
            periods = [p for p in periods if (p < best[0]) == (period < best[0])]
            best = (period, bufsize, mem)
        else:
            # The (period, bufsize) configuration we found is worse than what we already had.
            # Remove all periods that are on the other side of the tried configuration as the
            # best one.
            periods = [p for p in periods if (p < period) == (best[0] < period)]

    # Break ties deterministically toward the smallest period (the memory usage as a function of
    # period can be flat over several adjacent periods), so the result does not depend on the random
    # search order.
    period, bufsize, mem = best
    while period > 1:
        cand_bufsize = find_bufsize(period - 1, attack_headers, max_headers, minchainwork_headers)
        cand_mem = memory_usage(period - 1, cand_bufsize, max_headers, minchainwork_headers)
        if cand_mem != mem:
            break
        period, bufsize, mem = period - 1, cand_bufsize, cand_mem
    return period, bufsize


def analyze(when):
    """Find the best configuration and print it out."""

    # Maximum number of headers a valid Bitcoin chain can have over the given timespan (from genesis
    # to the target date). When exploiting the timewarp attack, this can be up to 6 per second since
    # genesis.
    max_headers = 6 * ((when - GENESIS_TIME) // timedelta(seconds=1))
    attack_headers = LIMIT_FRACTION * MINCHAINWORK_HEADERS
    period, bufsize = compute(max_headers, MINCHAINWORK_HEADERS, attack_headers)
    # Report it, in a form that can be pasted into chainparams.
    print()
    print(f"Given current min chainwork headers of {MINCHAINWORK_HEADERS}, the optimal parameters for low")
    print(f"memory usage on mainchain for release until {TIME:%Y-%m-%d} is:")
    print()
    print(f"        // Generated by headerssync-params.py on {datetime.today():%Y-%m-%d}.")
    print( "        m_headers_sync_params = HeadersSyncParams{")
    print(f"            .commitment_period = {period},")
    print(f"            .redownload_buffer_size = {bufsize},"
          f" // {bufsize}/{period} = ~{bufsize/period:.1f} commitments")
    print( "        };")


# Test vectors for compute(), as a list of tuples containing:
# - inputs: max_headers, minchainwork_headers, attack_headers
# - expected outputs: period, bufsize
#
# These were created by starting from a random starting point:
# - timespan: log-normal distributed with mode 12 years and mean 18 years
# - minchainwork_headers: exponentially distributed with mean 1000000
# - attack_headers: log-normal distributed with median = LIMIT_FRACTION * minchainwork_headers and
#                   sigma = 1.5
# And then walking in a random direction in 3D log-space, collecting 100 transition points where
# the optimal (period, bufsize) changes. Among those, the transition whose max_headers and
# minchainwork_headers are closest to integers is picked. Filtering is applied to remove cases
# that are too close to machine precision, as those are genuinely too ambiguous.

TEST_VECTORS = [
    (1719079112, 2486165, 2.4371139965488073e-05, 437, 10253),
    (1849920928, 1375438, 0.00010047802426348386, 473, 10199),
    (1811416960, 7221184, 0.0003834358670986813, 489, 9628),
    (7677261228, 752770, 7.657623409916339e-06, 869, 23031),
    (1971752357, 1225799, 1.7845208713901033e-05, 462, 11091),
    (3159905880, 2665516, 6.735316426018734e-06, 565, 14578),
    (2683034630, 193996, 1.719177483252805e-05, 536, 13059),
    (1697590446, 848276, 1.139595371908207e-05, 425, 10414),
    (2228709291, 1595505, 0.00046448453932950096, 544, 10681),
    (2088183178, 1397130, 0.0009937322357444738, 542, 10045),
    (2848303623, 2429248, 0.0009830357473239024, 628, 11820),
    (2755700321, 1725832, 3.079889435377424e-05, 552, 13016),
    (1573880545, 1264386, 6.55879555496996e-05, 432, 9502),
    (2448458334, 306585, 6.341304654686745e-06, 499, 12802),
    (4006740955, 536513, 9.862654648507075e-05, 684, 15275),
    (3222917775, 11128, 4.1587518500733085e-07, 532, 15806),
    (2595931233, 109205, 2.345792746509269e-06, 500, 13547),
    (1547460218, 342682, 5.786285545229493e-06, 399, 10123),
    (3412396570, 857000, 9.055283299520372e-06, 591, 15058),
    (1608565500, 1047750, 2.9746757899990667e-05, 426, 9850),
    (6751831959, 3420, 1.9796744871262838e-07, 748, 23538),
    (2112492775, 949804, 3.878638517429712e-05, 489, 11245),
    (5323388613, 226195, 1.259247706141301e-05, 738, 18782),
    (2516536860, 1823375, 0.0005282972243682878, 579, 11330),
    (4153605296, 3153542, 7.208345736641362e-05, 689, 15710),
    (1392991392, 2455336, 1.4467550657002429e-05, 389, 9333),
    (3045192620, 387654, 0.00028714231249025725, 621, 12789),
    (2470936704, 374758, 2.304932998416373e-06, 488, 13211),
    (5432386284, 1767951, 0.0004270020432999718, 829, 17080),
    (2575424700, 428736, 4.513652725339203e-05, 541, 12418),
    (2081820804, 1384033, 3.4110799029667034e-05, 484, 11217),
    (2140791604, 25777, 1.6633229346223927e-08, 406, 13727),
    (1676484364, 1560700, 9.011220006487238e-06, 420, 10410),
    (1542576120, 965810, 0.00011890792700529461, 436, 9229),
    (2038026484, 1779007, 6.22198825015376e-05, 487, 10873),
    (2600282368, 1746143, 2.558733122710413e-05, 534, 12693),
    (6469970160, 57485, 1.540887312697366e-07, 728, 23138),
    (3393786462, 656909, 1.6802005293828858e-05, 599, 14741),
    (1923909893, 8086, 1.5806088698106541e-07, 405, 12385),
    (4359755744, 1081995, 0.0011488135311153401, 773, 14703),
    (3329013904, 723851, 0.0016408648354929998, 689, 12598),
    (2785727394, 1049862, 3.914501343290894e-05, 559, 12996),
    (3794021233, 643680, 2.7740369347053645e-06, 602, 16387),
    (5346173856, 345137, 7.285354474681436e-06, 729, 19094),
    (3350561084, 1051927, 9.397334309598376e-05, 627, 13934),
    (9319364840, 1244089, 0.00011545830655088754, 1029, 23605),
    (4216249682, 971771, 0.00025973759887733955, 723, 15204),
    (1105318683, 222719, 0.00010566643251054803, 370, 7783),
    (6168635190, 1335470, 2.876811767724857e-05, 811, 19828),
    (1709740236, 57056, 3.177749891774726e-07, 389, 11475),
]


def selftest():
    """Check that compute() reproduces every result in TEST_VECTORS."""

    for max_headers, minchainwork_headers, attack_headers, exp_period, exp_bufsize in TEST_VECTORS:
        period, bufsize = compute(max_headers, minchainwork_headers, attack_headers)
        assert (period, bufsize) == (exp_period, exp_bufsize), \
            (max_headers, minchainwork_headers, attack_headers, period, bufsize, exp_period, exp_bufsize)
    print(f"All {len(TEST_VECTORS)} self-tests passed")


if __name__ == "__main__":
    if "--selftest" in sys.argv[1:]:
        selftest()
    else:
        analyze(TIME)
