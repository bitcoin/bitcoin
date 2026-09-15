// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <headerssync.h>

#include <pow.h>
#include <util/check.h>
#include <util/log.h>
#include <util/time.h>
#include <util/vector.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <utility>

HeadersSyncState::HeadersSyncState(NodeId id,
                                   const Consensus::Params& consensus_params,
                                   const HeadersSyncParams& params,
                                   const CBlockIndex& chain_start,
                                   const arith_uint256& minimum_required_work)
    : m_commit_offset((assert(params.commitment_period > 0), // HeadersSyncParams field must be initialized to non-zero.
                       FastRandomContext().randrange(params.commitment_period))),
      m_id(id),
      m_consensus_params(consensus_params),
      m_params(params),
      m_chain_start(chain_start),
      m_minimum_required_work(minimum_required_work),
      m_current_chain_work(chain_start.nChainWork),
      m_last_header_received(m_chain_start.GetBlockHeader()),
      m_current_height(chain_start.nHeight)
{
    // Estimate the number of blocks that could possibly exist on the peer's
    // chain *right now* using 6 blocks/second (fastest blockrate given the MTP
    // rule) times the number of seconds from the last allowed block until
    // today. This serves as a memory bound on how many commitments we might
    // store from this peer, and we can safely give up syncing if the peer
    // exceeds this bound, because it's not possible for a consensus-valid
    // chain to be longer than this (at the current time -- in the future we
    // could try again, if necessary, to sync a longer chain).
    const auto now{NodeClock::now()};
    const int64_t max_seconds_since_start{Ticks<std::chrono::seconds>(now - NodeSeconds{std::chrono::seconds{chain_start.GetMedianTimePast()}})
                                          + MAX_FUTURE_BLOCK_TIME};
    if (max_seconds_since_start < 0) {
        throw SystemClockError{strprintf(
            "System clock is more than %d minutes behind chain start MTP (%s vs %s).",
            MAX_FUTURE_BLOCK_TIME / 60,
            FormatISO8601DateTime(TicksSinceEpoch<std::chrono::seconds>(now)),
            FormatISO8601DateTime(chain_start.GetMedianTimePast()))};
    }
    m_max_commitments = 6 * max_seconds_since_start / m_params.commitment_period;

    LogDebug(BCLog::NET, "Initial headers sync started with peer=%d: height=%i, max_commitments=%i, min_work=%s\n", m_id, m_current_height, m_max_commitments, m_minimum_required_work.ToString());
}

/** Free any memory in use, and mark this object as no longer usable. This is
 * required to guarantee that we won't reuse this object with the same
 * SaltedUint256Hasher for another sync. */
void HeadersSyncState::Finalize()
{
    Assume(m_download_state != State::FINAL);
    ClearShrink(m_header_commitments);
    m_last_header_received.SetNull();
    ClearShrink(m_redownloaded_headers);
    m_redownload_buffer_last_hash.SetNull();
    m_redownload_buffer_first_prev_hash.SetNull();
    m_process_all_remaining_headers = false;
    m_current_height = 0;

    m_download_state = State::FINAL;
}

/** Process the next batch of headers received from our peer.
 *  Validate and store commitments, and compare total chainwork to our target to
 *  see if we can switch to REDOWNLOAD mode.  */
HeadersSyncState::ProcessingResult HeadersSyncState::ProcessNextHeaders(
        std::span<const CBlockHeader> received_headers, const bool full_headers_message)
{
    ProcessingResult ret;

    Assume(!received_headers.empty());
    if (received_headers.empty()) return ret;

    Assume(m_download_state != State::FINAL);
    if (m_download_state == State::FINAL) return ret;

    if (m_download_state == State::PRESYNC) {
        // During PRESYNC, we minimally validate block headers and
        // occasionally add commitments to them, until we reach our work
        // threshold (at which point m_download_state is updated to REDOWNLOAD).
        ret.success = ValidateAndStoreHeadersCommitments(received_headers);
        if (ret.success) {
            if (full_headers_message || m_download_state == State::REDOWNLOAD) {
                // A full headers message means the peer may have more to give us;
                // also if we just switched to REDOWNLOAD then we need to re-request
                // headers from the beginning.
                ret.request_more = true;
            } else {
                Assume(m_download_state == State::PRESYNC);
                // If we're in PRESYNC and we get a non-full headers
                // message, then the peer's chain has ended and definitely doesn't
                // have enough work, so we can stop our sync.
                LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: incomplete headers message at height=%i (presync phase)\n", m_id, m_current_height);
            }
        }
    } else if (m_download_state == State::REDOWNLOAD) {
        // During REDOWNLOAD, we compare our stored commitments to what we
        // receive, and add headers to our redownload buffer. When the buffer
        // gets big enough (meaning that we've checked enough commitments),
        // we'll return a batch of headers to the caller for processing.
        ret.success = true;
        for (const auto& hdr : received_headers) {
            if (!ValidateAndStoreRedownloadedHeader(hdr)) {
                // Something went wrong -- the peer gave us an unexpected chain.
                // We could consider looking at the reason for failure and
                // punishing the peer, but for now just give up on sync.
                ret.success = false;
                break;
            }
        }

        if (ret.success) {
            // Return any headers that are ready for acceptance.
            ret.pow_validated_headers = PopHeadersReadyForAcceptance();

            // If we hit our target blockhash, then all remaining headers will be
            // returned and we can clear any leftover internal state.
            if (m_redownloaded_headers.empty() && m_process_all_remaining_headers) {
                LogDebug(BCLog::NET, "Initial headers sync complete with peer=%d: releasing all at height=%i (redownload phase)\n", m_id, m_redownload_buffer_last_height);
            } else if (full_headers_message) {
                // If the headers message is full, we need to request more.
                ret.request_more = true;
            } else {
                // For some reason our peer gave us a high-work chain, but is now
                // declining to serve us that full chain again. Give up.
                // Note that there's no more processing to be done with these
                // headers, so we can still return success.
                LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: incomplete headers message at height=%i (redownload phase)\n", m_id, m_redownload_buffer_last_height);
            }
        }
    }

    if (!(ret.success && ret.request_more)) Finalize();
    return ret;
}

bool HeadersSyncState::ValidateAndStoreHeadersCommitments(std::span<const CBlockHeader> headers)
{
    // The caller should not give us an empty set of headers.
    Assume(headers.size() > 0);
    if (headers.size() == 0) return true;

    Assume(m_download_state == State::PRESYNC);
    if (m_download_state != State::PRESYNC) return false;

    if (headers[0].hashPrevBlock != m_last_header_received.GetHash()) {
        // Somehow our peer gave us a header that doesn't connect.
        // This might be benign -- perhaps our peer reorged away from the chain
        // they were on. Give up on this sync for now (likely we will start a
        // new sync with a new starting point).
        LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: non-continuous headers at height=%i (presync phase)\n", m_id, m_current_height);
        return false;
    }

    // If it does connect, (minimally) validate and occasionally store
    // commitments.
    for (const auto& hdr : headers) {
        if (!ValidateAndProcessSingleHeader(hdr)) {
            return false;
        }
    }

    if (m_current_chain_work >= m_minimum_required_work) {
        m_redownloaded_headers.clear();
        m_redownload_buffer_last_height = m_chain_start.nHeight;
        m_redownload_buffer_first_prev_hash = m_chain_start.GetBlockHash();
        m_redownload_buffer_last_hash = m_chain_start.GetBlockHash();
        m_redownload_chain_work = m_chain_start.nChainWork;
        m_download_state = State::REDOWNLOAD;
        LogDebug(BCLog::NET, "Initial headers sync transition with peer=%d: reached sufficient work at height=%i, redownloading from height=%i\n", m_id, m_current_height, m_redownload_buffer_last_height);
    }
    return true;
}

bool HeadersSyncState::ValidateAndProcessSingleHeader(const CBlockHeader& current)
{
    Assume(m_download_state == State::PRESYNC);
    if (m_download_state != State::PRESYNC) return false;

    int64_t next_height = m_current_height + 1;

    // Verify that the difficulty isn't growing too fast; an adversary with
    // limited hashing capability has a greater chance of producing a high
    // work chain if they compress the work into as few blocks as possible,
    // so don't let anyone give a chain that would violate the difficulty
    // adjustment maximum.
    if (!PermittedDifficultyTransition(m_consensus_params, next_height,
                m_last_header_received.nBits, current.nBits)) {
        LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: invalid difficulty transition at height=%i (presync phase)\n", m_id, next_height);
        return false;
    }

    if (next_height % m_params.commitment_period == m_commit_offset) {
        // Add a commitment.
        m_header_commitments.push_back(m_hasher(current.GetHash()) & 1);
        if (m_header_commitments.size() > m_max_commitments) {
            // The peer's chain is too long; give up.
            // It's possible the chain grew since we started the sync; so
            // potentially we could succeed in syncing the peer's chain if we
            // try again later.
            LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: exceeded max commitments at height=%i (presync phase)\n", m_id, next_height);
            return false;
        }
    }

    m_current_chain_work += GetBlockProof(current);
    m_last_header_received = current;
    m_current_height = next_height;

    return true;
}

bool HeadersSyncState::ValidateAndStoreRedownloadedHeader(const CBlockHeader& header)
{
    Assume(m_download_state == State::REDOWNLOAD);
    if (m_download_state != State::REDOWNLOAD) return false;

    int64_t next_height = m_redownload_buffer_last_height + 1;

    // Ensure that we're working on a header that connects to the chain we're
    // downloading.
    if (header.hashPrevBlock != m_redownload_buffer_last_hash) {
        LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: non-continuous headers at height=%i (redownload phase)\n", m_id, next_height);
        return false;
    }

    // Check that the difficulty adjustments are within our tolerance:
    uint32_t previous_nBits{0};
    if (!m_redownloaded_headers.empty()) {
        previous_nBits = m_redownloaded_headers.back().nBits;
    } else {
        previous_nBits = m_chain_start.nBits;
    }

    if (!PermittedDifficultyTransition(m_consensus_params, next_height,
                previous_nBits, header.nBits)) {
        LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: invalid difficulty transition at height=%i (redownload phase)\n", m_id, next_height);
        return false;
    }

    // Track work on the redownloaded chain
    m_redownload_chain_work += GetBlockProof(header);

    if (m_redownload_chain_work >= m_minimum_required_work) {
        m_process_all_remaining_headers = true;
    }

    // If we're at a header for which we previously stored a commitment, verify
    // it is correct. Failure will result in aborting download.
    // Also, don't check commitments once we've gotten to our target blockhash;
    // it's possible our peer has extended its chain between our first sync and
    // our second, and we don't want to return failure after we've seen our
    // target blockhash just because we ran out of commitments.
    if (!m_process_all_remaining_headers && next_height % m_params.commitment_period == m_commit_offset) {
        if (m_header_commitments.size() == 0) {
            LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: commitment overrun at height=%i (redownload phase)\n", m_id, next_height);
            // Somehow our peer managed to feed us a different chain and
            // we've run out of commitments.
            return false;
        }
        bool commitment = m_hasher(header.GetHash()) & 1;
        bool expected_commitment = m_header_commitments.front();
        m_header_commitments.pop_front();
        if (commitment != expected_commitment) {
            LogDebug(BCLog::NET, "Initial headers sync aborted with peer=%d: commitment mismatch at height=%i (redownload phase)\n", m_id, next_height);
            return false;
        }
    }

    // Store this header for later processing.
    m_redownloaded_headers.emplace_back(header);
    m_redownload_buffer_last_height = next_height;
    m_redownload_buffer_last_hash = header.GetHash();

    return true;
}

std::vector<CBlockHeader> HeadersSyncState::PopHeadersReadyForAcceptance()
{
    std::vector<CBlockHeader> ret;

    Assume(m_download_state == State::REDOWNLOAD);
    if (m_download_state != State::REDOWNLOAD) return ret;

    while (m_redownloaded_headers.size() > m_params.redownload_buffer_size ||
            (m_redownloaded_headers.size() > 0 && m_process_all_remaining_headers)) {
        ret.emplace_back(m_redownloaded_headers.front().GetFullHeader(m_redownload_buffer_first_prev_hash));
        m_redownloaded_headers.pop_front();
        m_redownload_buffer_first_prev_hash = ret.back().GetHash();
    }
    return ret;
}

CBlockLocator HeadersSyncState::NextHeadersRequestLocator() const
{
    Assume(m_download_state != State::FINAL);
    if (m_download_state == State::FINAL) return {};

    auto chain_start_locator = LocatorEntries(&m_chain_start);
    std::vector<uint256> locator;

    if (m_download_state == State::PRESYNC) {
        // During pre-synchronization, we continue from the last header received.
        locator.push_back(m_last_header_received.GetHash());
    }

    if (m_download_state == State::REDOWNLOAD) {
        // During redownload, we will download from the last received header that we stored.
        locator.push_back(m_redownload_buffer_last_hash);
    }

    locator.insert(locator.end(), chain_start_locator.begin(), chain_start_locator.end());

    return CBlockLocator{std::move(locator)};
}

//  PARAMETER OPTIMIZATION
//
//  The headerssync module implements a DoS protection against low-difficulty header spam which does
//  not rely on checkpoints. In short it works as follows:
//
//  - (initial) header synchronization is split into two phases:
//    - A commitment phase, in which headers are downloaded from the peer, and a very compact
//      commitment to them is remembered in per-peer memory. The commitment phase ends when the
//      received chain's combined work reaches a predetermined threshold.
//    - A redownload phase, during which the headers are downloaded a second time from the same
//      peer, and compared against the commitment constructed in the first phase. If there is a
//      match, the redownloaded headers are fed to validation and accepted into permanent storage.
//
//    This separation guarantees that no headers are accepted into permanent storage without
//    requiring the peer to first prove the chain actually has sufficient work.
//
//  - To actually implement this commitment mechanism, the following approach is used:
//    - Keep a *1 bit* commitment (constructed using a salted hash function), for every block whose
//      height is a multiple of {period} plus an offset value. The offset, like the salt, is chosen
//      randomly when the synchronization starts and kept fixed afterwards.
//    - When redownloading, headers are fed through a per-peer queue that holds {bufsize} headers,
//      before passing them to validation. All the headers in this queue are verified against the
//      commitment bits created in the first phase before any header is released from it. This means
//      {bufsize/period} bits are checked "on top of" each header before actually processing it,
//      which results in a commitment structure with roughly {bufsize/period} bits of security, as
//      once a header is modified, due to the prevhash inclusion, all future headers necessarily
//      change as well.
//
//  The question is what these {period} and {bufsize} parameters need to be set to. The code
//  below computes a near-optimal choice, taking into account:
//
//  - Minimizing the (maximum of) two scenarios that trigger per-peer memory usage:
//
//    - When downloading a (likely honest) chain that reaches the chainwork threshold after {n}
//      blocks, and then redownloads them, we will consume per-peer memory that is sufficient to
//      store {n/period} commitment bits and {bufsize} headers. We only consider attackers without
//      sufficient hashpower (as otherwise they are from a PoW perspective not attackers), which
//      means {n} is restricted to the honest chain's length before reaching minchainwork.
//
//    - When downloading a (likely false) chain of {n} headers that never reaches the chainwork
//      threshold, we will consume per-peer memory that is sufficient to store {n/period}
//      commitment bits. Such a chain may be very long, by exploiting the timewarp bug to avoid
//      ramping up difficulty. There is however an absolute limit on how long such a chain can be: 6
//      blocks per second since genesis, due to the increasing MTP consensus rule.
//
//  - Not gratuitously preventing synchronizing any valid chain, however difficult such a chain may
//    be to construct. In particular, the above scenario with an enormous timewarp-exploiting chain
//    cannot simply be ignored, as it is legal that the honest main chain is like that. We however
//    do not bother minimizing the memory usage in that case (because a billion-header long honest
//    chain will inevitably use far larger amounts of memory than designed for).
//
//  - Keep the rate at which attackers can get low-difficulty headers accepted to the block index
//    negligible. Specifically, the possibility exists for an attacker to send the honest main
//    chain's headers during the commitment phase, but then start deviating at an attacker-chosen
//    point by sending novel low-difficulty headers instead. Depending on how high we set the
//    {bufsize/period} ratio, we can make the probability that such a header makes it in
//    arbitrarily small, but at the cost of higher memory during the redownload phase. It turns out,
//    some rate of memory usage growth is expected anyway due to chain growth, so permitting the
//    attacker to increase that rate by a small factor isn't concerning. The attacker may start
//    somewhat later than genesis, as long as the difficulty doesn't get too high. This reduces
//    the attacker bandwidth required at the cost of higher PoW needed for constructing the
//    alternate chain. This trade-off is ignored here, as it results in at most a small constant
//    factor in attack rate.

std::pair<size_t, size_t> ComputeHeadersSyncParamsInner(int64_t max_headers, int64_t minchainwork_headers, double attack_headers)
{
    /** Headers in the redownload buffer are stored without prevhash, as a CompressedHeader. [bits] */
    constexpr int64_t COMPACT_HEADER_SIZE{sizeof(CompressedHeader) * 8};
    // The unit tests would need to be regenerated if this constant were to change, so
    // static_assert to give a more targeted error message.
    static_assert(COMPACT_HEADER_SIZE == 384);

    // The chain has at least one header.
    Assume(minchainwork_headers > 0);
    // Some small attacker success rate is inevitable.
    Assume(attack_headers > 0);
    // Step 1 below relies on this to guarantee the two memory-usage curves intersect.
    Assume(max_headers > 2 * minchainwork_headers);

    /** Expected number of forged headers that get accepted, per attack, in a (period, bufsize)
     *  configuration.
     *
     *  We ignore the effects of headers arriving in batches of size 2000, and instead treat them
     *  as arriving one by one. This overestimates the attacker's ability slightly, because the
     *  batches are only accepted in their entirety, if all checks inside succeed. Thus, our result
     *  here will be a conservative overestimate.
     *
     *  The attack we model then consists of:
     *  - During the commitment phase, the attacker sends just the honest chain.
     *  - During the redownload phase, the attacker starts forging headers at some point. Let F be
     *    the number of forged headers before the first commitment check, P the period, and B the
     *    bufsize. So F is in [0, P-1]. Now the following happens:
     *    - The first B forged headers fill up the victim's buffer, and are not accepted. There are
     *      C = ceil((B-F)/P) commitment checks within those first B forgeries, which gives a
     *      probability of 2^-C that the attack does not fail outright before any forgeries are
     *      accepted.
     *    - After that, there are (F + P*C - B) headers before the next check. Each of these causes
     *      one forged header to be pushed out of the buffer and be accepted.
     *    - After that, every group of P headers contains 1 more check, and if it passes, P more
     *      headers are pushed out of the buffer and get accepted.
     *    - Assume the attacker keeps doing this forever as long as they succeed; stopping early
     *      never helps them. This is thanks to ignoring the batching; with batching, we need to
     *      model the attacker's ability to strategically stop the attack. They are in principle
     *      bounded by the length of the honest chain, but for simplicity we overestimate this as
     *      infinitely long.
     *
     *  Together, this means the number of accepted headers is
     *
     *  n(F) = 2^-C * (F + P*C - B + 1/2*P + 1/4*P + 1/8*P + ...)
     *       = 2^-C * (F + P*C - B + P)
     *
     *  Since the position of the first commitment is randomized, we need to average this expression
     *  over all F in [0, P-1]. Let K=floor(B/P), and M=(B mod P). Then we have:
     *  - for F in [0, M-1]: C = K+1, so avg n(0..M-1) = 2^-(K+1) * ((M-1)/2   + P*K - B + 2*P)
     *  - for F in [M, P-1]: C = K,   so avg n(M..P-1) = 2^-K     * ((M+P-1)/2 + P*K - B + P)
     *  Taking the weighted average over both (weight M for first, weight P-M for second) gives:
     *
     *      2^-K     * (6*P^2 - 4*P*M + M^2 - 2*P + M) / (4*P)
     *    = 2^-(K+2) * (6*P^2 - 4*P*M + M^2 - 2*P + M) / P
     */
    auto attack_rate = [](int64_t period, int64_t bufsize) noexcept -> double {
        auto k = bufsize / period;
        auto m = bufsize - k * period;
        return std::ldexp((period * (6.0 * period - 4.0 * m - 2.0) + m * (m + 1.0)) / period, -int(k + 2));
    };

    /** The peak per-peer memory a (period,bufsize) configuration needs. */
    auto memory_usage = [max_headers, minchainwork_headers](int64_t period, int64_t bufsize) noexcept -> int64_t {
        /** Per-peer memory usage for a timewarp chain that never meets minchainwork (one bit per
         *  period). */
        const int64_t mem_timewarp = max_headers / period;
        /** Per-peer memory usage for being fed the main chain (one bit per period +
         *  redownload buffer size). */
        const int64_t mem_mainchain = (minchainwork_headers / period) + bufsize * COMPACT_HEADER_SIZE;
        /** The peak per-peer memory usage is the larger of the two. */
        return std::max(mem_timewarp, mem_mainchain);
    };

    /** Smallest bufsize whose attack rate against (period, bufsize) is below attack_headers. */
    auto find_bufsize = [&](int64_t period) noexcept -> int64_t {
        int64_t succ_buf{0};
        int64_t fail_buf{1};
        // First double iteratively until an upper bound for failure is found.
        while (attack_rate(period, fail_buf) >= attack_headers) {
            const int64_t next_fail = 3 * fail_buf - 2 * succ_buf;
            succ_buf = fail_buf;
            fail_buf = next_fail;
        }
        // Then perform a bisection search to narrow it down.
        while (fail_buf > succ_buf + 1) {
            const int64_t try_buf = (succ_buf + fail_buf) / 2;
            if (attack_rate(period, try_buf) >= attack_headers) {
                succ_buf = try_buf;
            } else {
                fail_buf = try_buf;
            }
        }
        return fail_buf;
    };

    // The overall strategy to solve our problem (minimizing peak memory usage while staying under
    // the acceptable attack rate), is as follows:
    //
    // Step 1: assume that period and bufsize can be arbitrary real numbers, and accurately find
    //         the optimal solution to that continuous relaxation of the problem.
    // Step 2: try the two integer periods surrounding the continuous-optimal one, and return the
    //         one whose memory usage is lowest (breaking ties toward the smallest period).
    //
    // As a function of the period, the memory usage is the maximum of a decreasing (timewarp) and
    // an increasing (mainchain) branch, which cross at the continuous optimum. The best integer
    // period is therefore one of the two surrounding it, unless rounding bufsize up to an integer
    // outweighs a whole step further along the timewarp branch. Experimentally, this only happens
    // for absurdly high attack rates, and even then, it only results in memory usage slightly
    // above the optimal.

    // Step 1: find the optimum solution to the continuous relaxation of the problem.
    const double cont_period = [&]() -> double {
        // Abstractly, our goal is to find (period, bufsize):
        // (1) such that attack_rate(period, bufsize) <= attack_headers
        // (2) such that memory_usage(period, bufsize) is minimal
        //
        // Both of these turn into exact equalities in the continuous relaxation:
        // (1) becomes attack_rate(period, bufsize) = attack_headers, because there is no need to
        //     overshoot.
        // (2) becomes mem_timewarp(period, bufsize) = mem_mainchain(period, bufsize), because
        //     timewarp memory goes down with increasing period, and mainchain memory goes up with
        //     increasing period (with bufsize set to keep the attack rate constant). The maximum
        //     of the two is therefore minimized exactly where they intersect. This can be shown to
        //     be the case whenever max_headers is larger than 2 * minchainwork_headers, which is
        //     practically always true.
        //
        // For the continuous relaxation, the expression from attack_rate is used with real-valued
        // period and bufsize (and thus M in [0, P)). As a function of bufsize it is continuous and
        // strictly decreasing, and for the bufsize values sharing a given K it takes values in
        // (2^-(K+2) * (3*P - 1), 2^-(K+1) * (3*P - 1)]. Thus (1) can be solved for bufsize
        // directly: K is the unique integer for which attack_headers falls in that range, after
        // which M follows from the quadratic equation
        //
        //   M^2 - (4*P - 1)*M + 6*P^2 - 2*P - attack_headers * P * 2^(K+2) = 0.
        //
        auto cont_bufsize = [&](double period) noexcept -> double {
            const double k = std::floor(std::log2(1.5 * period - 0.5) - std::log2(attack_headers));
            const double m = 0.5 * (4.0 * period - 1.0 - std::sqrt(std::ldexp(attack_headers * period, int(k + 4)) - 8.0 * period * period + 1.0));
            return k * period + m;
        };

        // Substituting this into (2), written out and multiplied by period, gives:
        //
        //   COMPACT_HEADER_SIZE * period * cont_bufsize(period) = max_headers - minchainwork_headers
        //
        // whose left-hand side increases with period. Solve it by bisection, over the range that
        // the integer period is restricted to (which yields an endpoint when the real solution
        // lies outside of it).
        const double target = double(max_headers - minchainwork_headers) / COMPACT_HEADER_SIZE;
        double lo = 1.0;
        double hi = minchainwork_headers;
        while (true) {
            const double mid = 0.5 * (lo + hi);
            if (!(lo < mid && mid < hi)) break;
            if (mid * cont_bufsize(mid) < target) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        return hi;
    }();

    // Step 2: try the integer periods just below and above cont_period (both of which are legal
    //         periods, as cont_period lies within the legal range), and pick the one which,
    //         together with its corresponding bufsize, results in the lowest memory usage.
    const auto make_config = [&](double real_period) noexcept {
        const int64_t period = real_period;
        const int64_t bufsize = find_bufsize(period);
        return std::tuple{memory_usage(period, bufsize), period, bufsize};
    };
    // Tuples compare lexicographically, so this picks the lowest memory usage, and among equally
    // low ones the smallest period.
    const auto [_memory, best_period, best_bufsize] = std::min(
        make_config(std::floor(cont_period)),
        make_config(std::ceil(cont_period)));

    return {size_t(best_period), size_t(best_bufsize)};
}

// Derive max_headers (the longest a valid chain could be by now) and the attack_headers budget from
// the chain's age and minimum-chain-work header count, then optimize.
std::pair<size_t, size_t> ComputeHeadersSyncParams(std::chrono::seconds timespan, int64_t minchainwork_headers)
{
    /** Expected block interval. [seconds] */
    constexpr double BLOCK_INTERVAL{600.0};
    /** Combined processing bandwidth from all attackers to one victim. [bit/s]
     *
     * 6 Gbit/s is approximately the speed at which a single thread of a Ryzen 5950X CPU thread can
     * hash headers. In practice, the victim's network bandwidth and network processing overheads
     * probably impose a far lower number, but it's a useful upper bound. */
    constexpr double ATTACK_BANDWIDTH{6000000000.0};
    /** How much additional permanent memory usage are attackers (jointly) allowed to cause in the
     *  victim, expressed as fraction of the normal memory usage due to mainchain growth, for the
     *  duration the attack is sustained. [unitless]. 0.2 means that attackers, while they keep up the
     * attack, can cause permanent memory usage due to headers storage to grow at 1.2 header per
     * BLOCK_INTERVAL. */
    constexpr double ATTACK_FRACTION{0.2};
    /** How many bits a header uses in P2P protocol. [bits] */
    constexpr int64_t NET_HEADER_SIZE{81 * 8};
    /** What rate of headers worth of RAM attackers are allowed to cause in the victim. [headers/s] */
    constexpr double LIMIT_HEADERRATE{ATTACK_FRACTION / BLOCK_INTERVAL};
    /** How many headers can attackers (jointly) send a victim per second. [headers/s] */
    constexpr double NET_HEADERRATE{ATTACK_BANDWIDTH / NET_HEADER_SIZE};
    /** What fraction of headers sent by attackers can at most be accepted by a victim. [unitless] */
    constexpr double LIMIT_FRACTION{LIMIT_HEADERRATE / NET_HEADERRATE};

    // The chain has at least one header.
    Assume(minchainwork_headers > 0);

    /** Maximum number of headers a valid Bitcoin chain can have over the given timespan (from genesis
     *  to now). When exploiting the timewarp attack, this can be up to 6 per second since genesis.
     *  Clamp below such that max_headers > 2 * minchainwork_headers, which the optimizer relies on.
     *  Any consistent clock satisfies this by a wide margin (the chain demonstrably reached
     *  minchainwork_headers), but mocked or badly wrong clocks may not. */
    const int64_t max_headers = std::max<int64_t>(6 * timespan.count(), 2 * minchainwork_headers + 1);

    return ComputeHeadersSyncParamsInner(max_headers, minchainwork_headers,
                                         LIMIT_FRACTION * minchainwork_headers);
}
