// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <headerssync.h>

#include <pow.h>
#include <random.h>
#include <util/check.h>
#include <util/log.h>
#include <util/time.h>
#include <util/vector.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

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
    const auto max_seconds_since_start{(Ticks<std::chrono::seconds>(NodeClock::now() - NodeSeconds{std::chrono::seconds{chain_start.GetMedianTimePast()}}))
                                       + MAX_FUTURE_BLOCK_TIME};
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

    int next_height = m_current_height + 1;

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
//  below computes the optimal choice, taking into account:
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
    /** How many headers are sent at once. [headers] */
    constexpr int64_t HEADER_BATCH_COUNT{2000};
    // The unit tests would need to be regenerated if this constant were to change, so
    // static_assert to give a more targeted error message.
    static_assert(COMPACT_HEADER_SIZE == 384);

    /** Maximal accepted headers per attack in a (period, bufsize) configuration.
     *
     * If limit is provided, the computation is stopped early when the result is known to exceed the
     * value in limit. */
    auto attack_rate = [&](int64_t period, int64_t bufsize, std::optional<double> limit = std::nullopt) noexcept -> double {
        /** Each batch's probability is a factor 2^(HEADER_BATCH_COUNT/period) smaller than the
         *  previous one, so the contributions of all future batches form a geometric series.
         *  future_limit bounds the ratio of that sum to a single batch's HEADER_BATCH_COUNT*prob.
         */
        const double future_limit = HEADER_BATCH_COUNT / std::expm1(std::numbers::ln2 * HEADER_BATCH_COUNT / period);

        // Let the current batch 0 being received be the first one in which the attacker starts
        // lying. Iterate over all possible values for the number of honest headers in batch 0, to
        // find the one which grants the attacker the highest success rate.
        std::optional<double> max_rate;
        for (int64_t honest = 0; honest < HEADER_BATCH_COUNT; ++honest) {
            /** The number of headers the attack under consideration will on average get accepted. This
             *  is the number being computed. */
            double rate{0.0};

            // Iterate over all {period} possible alignments of commitments w.r.t. the first batch.
            // Since the offset is randomized, the attacker cannot know/choose it, so we try all
            // values, computing the average attack rate over them by dividing each contribution by
            // period.
            for (int64_t align = 0; align < period; ++align) {
                // These state variables capture the situation after receiving the first batch.
                /** The number of headers received after the last commitment for an honest block
                 *  (adding {period} before the modulo keeps the dividend non-negative). */
                int64_t after_good_commit = HEADER_BATCH_COUNT - honest + (honest + period - align - 1) % period;
                /** The number of forged headers in the redownload buffer. */
                int64_t forged_in_buf = HEADER_BATCH_COUNT - honest;

                // Now iterate over the next batches of headers received, adding contributions to rate.
                while (true) {
                    // Process the first HEADER_BATCH_COUNT headers in the buffer:
                    const int64_t accept_forged_headers = std::max<int64_t>(forged_in_buf - bufsize, 0);
                    forged_in_buf -= accept_forged_headers;
                    if (accept_forged_headers) {
                        /** The probability the attack has not been detected yet at this point. */
                        const double prob = std::ldexp(1.0, -int(after_good_commit / period));
                        // Update attack rate, divided by period to average over the alignments.
                        rate += accept_forged_headers * prob / period;
                        // If this means we exceed limit, bail out early (performance optimization).
                        if (limit && rate >= *limit) return rate;
                        // Stop once an upper bound on all remaining batches' contribution is
                        // negligible compared to rate.
                        if (future_limit * prob < 1.0e-16 * rate) break;
                    }
                    // Update state from a new incoming batch (which is all forged).
                    after_good_commit += HEADER_BATCH_COUNT;
                    forged_in_buf += HEADER_BATCH_COUNT;
                }
            }

            // Remember the highest success rate.
            if (rate > max_rate) max_rate = rate;
        }

        return *max_rate;
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

    /** Determine how big bufsize needs to be given a specific period length.
     *
     * This is the smallest bufsize such that the attack rate against (period, bufsize) is below
     * attack_headers. If max_mem is provided and no such bufsize exists that needs less than
     * max_mem bits of memory, std::nullopt is returned.
     *
     * min_bufsize is the minimal result to be considered.
     */
    auto find_bufsize = [&](int64_t period, std::optional<int64_t> max_mem = std::nullopt, int64_t min_bufsize = 1) noexcept -> std::optional<int64_t> {
        int64_t succ_buf, fail_buf;
        if (!max_mem) {
            succ_buf = min_bufsize - 1;
            fail_buf = min_bufsize;
            // First double iteratively until an upper bound for failure is found.
            while (attack_rate(period, fail_buf, attack_headers) >= attack_headers) {
                const int64_t next_fail = 3 * fail_buf - 2 * succ_buf;
                succ_buf = fail_buf;
                fail_buf = next_fail;
            }
        } else {
            // If a long low-work header chain exists that exceeds max_mem already, give up.
            if (max_headers / period > *max_mem) return std::nullopt;
            // Otherwise, verify that the maximal buffer size that permits a mainchain sync with less
            // than max_mem memory is sufficient to get the attack rate below attack_headers. If not,
            // also give up.
            const int64_t max_buf = (*max_mem - (minchainwork_headers / period)) / COMPACT_HEADER_SIZE;
            if (max_buf < min_bufsize) return std::nullopt;
            if (attack_rate(period, max_buf, attack_headers) >= attack_headers) return std::nullopt;
            // If it is sufficient, that's an upper bound to start our search.
            succ_buf = min_bufsize - 1;
            fail_buf = max_buf;
        }
        // Then perform a bisection search to narrow it down.
        while (fail_buf > succ_buf + 1) {
            const int64_t try_buf = (succ_buf + fail_buf) / 2;
            if (attack_rate(period, try_buf, attack_headers) >= attack_headers) {
                succ_buf = try_buf;
            } else {
                fail_buf = try_buf;
            }
        }
        return fail_buf;
    };

    /** Solve the equation x*exp(x)=value (x > 0, value > 0). */
    constexpr auto lambert_w = [](double value) noexcept -> double {
        // Initial approximation.
        double approx = std::max(std::log(value), 0.0);
        for (int i = 0; i < 10; ++i) {
            // Newton-Raphson iteration steps.
            approx += (value * std::exp(-approx) - approx) / (approx + 1.0);
        }
        return approx;
    };

    // When period*bufsize = memory_scale, the per-peer memory for a mainchain sync and a maximally
    // long low-difficulty header sync are equal.
    const double memory_scale = double(max_headers - minchainwork_headers) / COMPACT_HEADER_SIZE;
    // Compute approximation for {bufsize/period}, using a formula for a simplified problem.
    const double approx_ratio = lambert_w(std::log(4.0) * memory_scale / (attack_headers * attack_headers)) / std::log(4.0);
    // Use those for a first attempt.
    int64_t period = int64_t(std::sqrt(memory_scale / approx_ratio) + 0.5);
    int64_t bufsize = find_bufsize(period).value();
    int64_t best_period = period;
    int64_t best_bufsize = bufsize;
    int64_t best_mem = memory_usage(period, bufsize);
    // (period, bufsize) configurations found so far, used to lower-bound find_bufsize.
    std::vector<std::pair<int64_t, int64_t>> maps{{period, bufsize}};

    // Consider all period values between 1 and minchainwork_headers, except the one just tried.
    std::vector<int64_t> periods;
    periods.reserve(minchainwork_headers);
    for (int64_t iv = 1; iv <= minchainwork_headers; ++iv) {
        if (iv != period) periods.push_back(iv);
    }

    // The search order is randomized; the result is the true optimum regardless of seed.
    InsecureRandomContext rng(/*seedval=*/0);
    // Iterate, picking a random element from periods, computing its corresponding bufsize, and
    // then using the result to shrink the period.
    while (true) {
        // Remove all periods whose memory usage for low-work long chain sync exceed the best
        // memory usage we've found so far.
        std::erase_if(periods, [&](int64_t p) { return max_headers / p >= best_mem; });
        // Stop if there is nothing left to try.
        if (periods.empty()) break;
        // Pick a random remaining option for period size, and compute corresponding bufsize.
        const std::size_t idx = rng.randrange(periods.size());
        period = periods[idx];
        periods[idx] = periods.back();
        periods.pop_back();
        // The buffer size (at a given attack level) cannot shrink as the period grows. Find the
        // largest period smaller than the selected one we know the buffer size for, and use that
        // as a lower bound to find_bufsize.
        std::pair<int64_t, int64_t> lower{0, 0};
        for (const auto& entry : maps) {
            if (entry.first < period && entry > lower) lower = entry;
        }
        const int64_t min_bufsize = lower.second;
        const std::optional<int64_t> found = find_bufsize(period, best_mem, min_bufsize);
        if (found) {
            // We found a (period, bufsize) configuration with better memory usage than our best
            // so far. Remember it for future lower bounds.
            bufsize = *found;
            maps.emplace_back(period, bufsize);
            const int64_t mem = memory_usage(period, bufsize);
            Assume(mem <= best_mem);
            // Remove all periods that are on the other side of the former best as the new best.
            std::erase_if(periods, [&](int64_t p) { return (p < best_period) != (period < best_period); });
            best_period = period;
            best_bufsize = bufsize;
            best_mem = mem;
        } else {
            // The (period, bufsize) configuration we found is worse than what we already had.
            // Remove all periods that are on the other side of the tried configuration as the
            // best one.
            std::erase_if(periods, [&](int64_t p) { return (p < period) != (best_period < period); });
        }
    }

    // Break ties deterministically toward the smallest period (the convex memory curve can be flat
    // over several adjacent periods), so the result does not depend on the random search order.
    while (best_period > 1) {
        const int64_t cand_bufsize = find_bufsize(best_period - 1).value();
        const int64_t cand_mem = memory_usage(best_period - 1, cand_bufsize);
        if (cand_mem != best_mem) break;
        best_period -= 1;
        best_bufsize = cand_bufsize;
        best_mem = cand_mem;
    }

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

    /** Maximum number of headers a valid Bitcoin chain can have over the given timespan (from genesis
     *  to now). When exploiting the timewarp attack, this can be up to 6 per second since genesis. */
    const int64_t max_headers = 6 * timespan.count();

    return ComputeHeadersSyncParamsInner(max_headers, minchainwork_headers,
                                         LIMIT_FRACTION * minchainwork_headers);
}
