// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <versionbits.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace {
class TestConditionChecker : public AbstractThresholdConditionChecker
{
private:
    mutable ThresholdConditionCache m_cache;
    const Consensus::Params dummy_params{};

public:
    const int64_t m_begin;
    const int64_t m_end;
    const int m_period;
    const int m_threshold;
    const int m_min_activation_height;
    const int m_bit;

    TestConditionChecker(int64_t begin, int64_t end, int period, int threshold, int min_activation_height, int bit)
        : m_begin{begin}, m_end{end}, m_period{period}, m_threshold{threshold}, m_min_activation_height{min_activation_height}, m_bit{bit}
    {
        assert(m_period > 0);
        assert(0 <= m_threshold && m_threshold <= m_period);
        assert(0 <= m_bit && m_bit < 32 && m_bit < VERSIONBITS_NUM_BITS);
        assert(0 <= m_min_activation_height);
    }

    bool Condition(const CBlockIndex* pindex, const Consensus::Params& params) const override { return Condition(pindex->nVersion); }
    int64_t BeginTime(const Consensus::Params& params) const override { return m_begin; }
    int64_t EndTime(const Consensus::Params& params) const override { return m_end; }
    int Period(const Consensus::Params& params) const override { return m_period; }
    int Threshold(const Consensus::Params& params) const override { return m_threshold; }
    int MinActivationHeight(const Consensus::Params& params) const override { return m_min_activation_height; }

    ThresholdState GetStateFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateFor(pindexPrev, dummy_params, m_cache); }
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateSinceHeightFor(pindexPrev, dummy_params, m_cache); }
    BIP9Stats GetStateStatisticsFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateStatisticsFor(pindexPrev, dummy_params); }

    bool Condition(int64_t version) const
    {
        return ((version >> m_bit) & 1) != 0 && (version & VERSIONBITS_TOP_MASK) == VERSIONBITS_TOP_BITS;
    }

    bool Condition(const CBlockIndex* pindex) const { return Condition(pindex->nVersion); }
};

/** Track blocks mined for test */
class Blocks
{
private:
    std::vector<std::unique_ptr<CBlockIndex>> m_blocks;
    const uint32_t m_start_time;
    const uint32_t m_interval;
    const int32_t m_signal;
    const int32_t m_no_signal;

public:
    Blocks(uint32_t start_time, uint32_t interval, int32_t signal, int32_t no_signal)
        : m_start_time{start_time}, m_interval{interval}, m_signal{signal}, m_no_signal{no_signal} {}

    size_t size() const { return m_blocks.size(); }

    CBlockIndex* tip() const
    {
        return m_blocks.empty() ? nullptr : m_blocks.back().get();
    }

    CBlockIndex* mine_block(bool signal)
    {
        CBlockHeader header;
        header.nVersion = signal ? m_signal : m_no_signal;
        header.nTime = m_start_time + m_blocks.size() * m_interval;
        header.nBits = 0x1d00ffff;

        auto current_block = std::make_unique<CBlockIndex>(header);
        current_block->pprev = tip();
        current_block->nHeight = m_blocks.size();
        current_block->BuildSkip();

        return m_blocks.emplace_back(std::move(current_block)).get();
    }
};
} // namespace

void initialize()
{
    SelectParams(CBaseChainParams::MAIN);
}

constexpr uint32_t MAX_TIME = 4102444800; // 2100-01-01

void test_one_input(const std::vector<uint8_t>& buffer)
{
    const CChainParams& params = Params();

    const int64_t interval = params.GetConsensus().nPowTargetSpacing;
    assert(interval > 1); // need to be able to halve it
    assert(interval < std::numeric_limits<int32_t>::max());

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // making period/max_periods larger slows these tests down significantly
    const int period = 32;
    const size_t max_periods = 16;
    const size_t max_blocks = 2 * period * max_periods;

    const int threshold = fuzzed_data_provider.ConsumeIntegralInRange(1, period);
    assert(0 < threshold && threshold <= period); // must be able to both pass and fail threshold!

    // too many blocks at 10min each might cause uint32_t time to overflow if
    // block_start_time is at the end of the range above
    assert(std::numeric_limits<uint32_t>::max() - MAX_TIME > interval * max_blocks);

    const int64_t block_start_time = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(params.GenesisBlock().nTime, MAX_TIME);

    // what values for version will we use to signal / not signal?
    const int32_t ver_signal = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    const int32_t ver_nosignal = fuzzed_data_provider.ConsumeIntegral<int32_t>();

    // select deployment parameters: bit, start time, timeout
    const int bit = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, VERSIONBITS_NUM_BITS - 1);

    bool always_active_test = false;
    bool never_active_test = false;
    int64_t start_time;
    int64_t timeout;
    if (fuzzed_data_provider.ConsumeBool()) {
        // pick the timestamp to switch based on a block
        // note states will change *after* these blocks because mediantime lags
        int start_block = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, period * (max_periods - 3));
        int end_block = fuzzed_data_provider.ConsumeIntegralInRange<int>(start_block, period * (max_periods - 3));

        start_time = block_start_time + start_block * interval;
        timeout = block_start_time + end_block * interval;

        assert(start_time <= timeout);

        // allow for times to not exactly match a block
        if (fuzzed_data_provider.ConsumeBool()) start_time += interval / 2;
        if (fuzzed_data_provider.ConsumeBool()) timeout += interval / 2;

        // this may make timeout too early; if so, don't run the test
        if (start_time > timeout) return;
    } else {
        if (fuzzed_data_provider.ConsumeBool()) {
            start_time = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
            always_active_test = true;
        } else {
            start_time = Consensus::BIP9Deployment::NEVER_ACTIVE;
            never_active_test = true;
        }
        timeout = fuzzed_data_provider.ConsumeBool() ? Consensus::BIP9Deployment::NO_TIMEOUT : fuzzed_data_provider.ConsumeIntegral<int64_t>();
    }
    int min_activation = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, period * max_periods);

    TestConditionChecker checker(start_time, timeout, period, threshold, min_activation, bit);

    // Early exit if the versions don't signal sensibly for the deployment
    if (!checker.Condition(ver_signal)) return;
    if (checker.Condition(ver_nosignal)) return;
    if (ver_nosignal < 0) return;

    // TOP_BITS should ensure version will be positive
    assert(ver_signal > 0);

    // Now that we have chosen time and versions, setup to mine blocks
    Blocks blocks(block_start_time, interval, ver_signal, ver_nosignal);

    /* Strategy:
     *  * we will mine a final period worth of blocks, with
     *    randomised signalling according to a mask
     *  * but before we mine those blocks, we will mine some
     *    randomised number of prior periods; with either all
     *    or no blocks in the period signalling
     *
     * We establish the mask first, then consume "bools" until
     * we run out of fuzz data to work out how many prior periods
     * there are and which ones will signal.
     */

    // establish the mask
    const uint32_t signalling_mask = fuzzed_data_provider.ConsumeIntegral<uint32_t>();

    // mine prior periods
    while (fuzzed_data_provider.remaining_bytes() > 0) {
        // all blocks in these periods either do or don't signal
        bool signal = fuzzed_data_provider.ConsumeBool();
        for (int b = 0; b < period; ++b) {
            blocks.mine_block(signal);
        }

        // don't risk exceeding max_blocks or times may wrap around
        if (blocks.size() + period*2 > max_blocks) break;
    }
    // NOTE: fuzzed_data_provider may be fully consumed at this point and should not be used further

    // now we mine the final period and check that everything looks sane

    // count the number of signalling blocks
    int blocks_sig = 0;

    // get the info for the first block of the period
    CBlockIndex* prev = blocks.tip();
    const int exp_since = checker.GetStateSinceHeightFor(prev);
    const ThresholdState exp_state = checker.GetStateFor(prev);
    BIP9Stats last_stats = checker.GetStateStatisticsFor(prev);

    int prev_next_height = (prev == nullptr ? 0 : prev->nHeight + 1);
    assert(exp_since <= prev_next_height);

    // mine (period-1) blocks and check state
    for (int b = 1; b < period; ++b) {
        const bool signal = (signalling_mask >> (b % 32)) & 1;
        if (signal) ++blocks_sig;

        CBlockIndex* current_block = blocks.mine_block(signal);

        // verify that signalling attempt was interpreted correctly
        assert(checker.Condition(current_block) == signal);

        // state and since don't change within the period
        const ThresholdState state = checker.GetStateFor(current_block);
        const int since = checker.GetStateSinceHeightFor(current_block);
        assert(state == exp_state);
        assert(since == exp_since);

        // GetStateStatistics may crash when state is not STARTED
        if (state != ThresholdState::STARTED) continue;

        // check that after mining this block stats change as expected
        const BIP9Stats stats = checker.GetStateStatisticsFor(current_block);
        assert(stats.period == period);
        assert(stats.threshold == threshold);
        assert(stats.elapsed == b);
        assert(stats.count == last_stats.count + (signal ? 1 : 0));
        assert(stats.possible == (stats.count + period >= stats.elapsed + threshold));
        last_stats = stats;
    }

    if (exp_state == ThresholdState::STARTED) {
        // double check that stats.possible is sane
        if (blocks_sig >= threshold - 1) assert(last_stats.possible);
    }

    // mine the final block
    bool signal = (signalling_mask >> (period % 32)) & 1;
    if (signal) ++blocks_sig;
    CBlockIndex* current_block = blocks.mine_block(signal);
    assert(checker.Condition(current_block) == signal);

    // GetStateStatistics is safe on a period boundary
    // and has progressed to a new period
    const BIP9Stats stats = checker.GetStateStatisticsFor(current_block);
    assert(stats.period == period);
    assert(stats.threshold == threshold);
    assert(stats.elapsed == 0);
    assert(stats.count == 0);
    assert(stats.possible == true);

    // More interesting is whether the state changed.
    const ThresholdState state = checker.GetStateFor(current_block);
    const int since = checker.GetStateSinceHeightFor(current_block);

    // since is straightforward:
    assert(since % period == 0);
    assert(0 <= since && since <= current_block->nHeight + 1);
    if (state == exp_state) {
        assert(since == exp_since);
    } else {
        assert(since == current_block->nHeight + 1);
    }

    // state is where everything interesting is
    switch (state) {
    case ThresholdState::DEFINED:
        assert(since == 0);
        assert(exp_state == ThresholdState::DEFINED);
        assert(current_block->GetMedianTimePast() < checker.m_begin);
        assert(current_block->GetMedianTimePast() < checker.m_end);
        break;
    case ThresholdState::STARTED:
        assert(current_block->GetMedianTimePast() >= checker.m_begin);
        assert(current_block->GetMedianTimePast() < checker.m_end);
        if (exp_state == ThresholdState::STARTED) {
            assert(blocks_sig < threshold);
        } else {
            assert(exp_state == ThresholdState::DEFINED);
        }
        break;
    case ThresholdState::LOCKED_IN:
        if (exp_state == ThresholdState::LOCKED_IN) {
            assert(current_block->nHeight + 1 < min_activation);
        } else {
            assert(exp_state == ThresholdState::STARTED);
            assert(current_block->GetMedianTimePast() < checker.m_end);
            assert(blocks_sig >= threshold);
        }
        break;
    case ThresholdState::ACTIVE:
        assert(always_active_test || min_activation <= current_block->nHeight + 1);
        assert(exp_state == ThresholdState::ACTIVE || exp_state == ThresholdState::LOCKED_IN);
        break;
    case ThresholdState::FAILED:
        assert(never_active_test || current_block->GetMedianTimePast() >= checker.m_end);
        assert(exp_state != ThresholdState::LOCKED_IN && exp_state != ThresholdState::ACTIVE);
        break;
    default:
        assert(false);
    }

    if (blocks.size() >= max_periods * period) {
        // we chose the timeout (and block times) so that by the time we have this many blocks it's all over
        assert(state == ThresholdState::ACTIVE || state == ThresholdState::FAILED);
    }

    if (always_active_test) {
        // "always active" has additional restrictions
        assert(state == ThresholdState::ACTIVE);
        assert(exp_state == ThresholdState::ACTIVE);
        assert(since == 0);
    } else if (never_active_test) {
        // "never active" does too
        assert(state == ThresholdState::FAILED);
        assert(exp_state == ThresholdState::FAILED);
        assert(since == 0);
    } else {
        // for signalled deployments, the initial state is always DEFINED
        assert(since > 0 || state == ThresholdState::DEFINED);
        assert(exp_since > 0 || exp_state == ThresholdState::DEFINED);
    }
}
