// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <versionbits.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

bool operator==(const SignalInfo& a, const SignalInfo& b)
{
    return a.height == b.height && a.revision == b.revision && a.activate == b.activate;
}

namespace {
class TestConditionChecker : public AbstractThresholdConditionChecker
{
private:
    mutable ThresholdConditionCache m_cache;

public:
    const int64_t m_begin;
    const int64_t m_end;
    const int m_period;
    const int32_t m_activate;
    const int32_t m_abandon;

    TestConditionChecker(int64_t begin, int64_t end, int period, int32_t activate, int32_t abandon)
        : m_begin{begin}, m_end{end}, m_period{period}, m_activate{activate}, m_abandon{abandon}
    {
        assert(m_period > 0);
    }

    int64_t BeginTime() const override { return m_begin; }
    int64_t EndTime() const override { return m_end; }
    int Period() const override { return m_period; }
    int32_t ActivateVersion() const override { return m_activate; }
    int32_t AbandonVersion() const override { return m_abandon; }

    ThresholdState GetStateFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateFor(pindexPrev, m_cache); }
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateSinceHeightFor(pindexPrev, m_cache); }
};

/** Track blocks mined for test */
class Blocks
{
private:
    std::vector<std::unique_ptr<CBlockIndex>> m_blocks;
    const uint32_t m_start_time;
    const uint32_t m_interval;

public:
    Blocks(uint32_t start_time, uint32_t interval)
        : m_start_time{start_time}, m_interval{interval}
    {
    }

    size_t size() const { return m_blocks.size(); }

    CBlockIndex* tip() const
    {
        return m_blocks.empty() ? nullptr : m_blocks.back().get();
    }

    CBlockIndex* mine_block(int32_t version)
    {
        CBlockHeader header;
        header.nVersion = version;
        header.nTime = m_start_time + m_blocks.size() * m_interval;
        header.nBits = 0x1d00ffff;

        auto current_block = std::make_unique<CBlockIndex>(header);
        current_block->pprev = tip();
        current_block->nHeight = m_blocks.size();
        current_block->BuildSkip();

        return m_blocks.emplace_back(std::move(current_block)).get();
    }
};

std::unique_ptr<const CChainParams> g_params;

void initialize()
{
    // this is actually comparatively slow, so only do it once
    g_params = CreateChainParams(ArgsManager{}, ChainType::MAIN);
    assert(g_params != nullptr);
}

constexpr uint32_t MAX_START_TIME = 4102444800; // 2100-01-01

FUZZ_TARGET(versionbits, .init = initialize)
{
    const CChainParams& params = *g_params;
    const int64_t interval = params.GetConsensus().nPowTargetSpacing;
    assert(interval > 1); // need to be able to halve it
    assert(interval < std::numeric_limits<int32_t>::max());

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // making period/max_periods larger slows these tests down significantly
    const int period = 32;
    const size_t max_periods = 16;
    const size_t max_blocks = 2 * period * max_periods;

    // too many blocks at 10min each might cause uint32_t time to overflow if
    // block_start_time is at the end of the range above
    assert(std::numeric_limits<uint32_t>::max() - MAX_START_TIME > interval * max_blocks);

    const int64_t block_start_time = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(params.GenesisBlock().nTime, MAX_START_TIME);

    // select deployment parameters
    bool always_active_test = false;
    bool never_active_test = false;
    int64_t start_time;
    int64_t timeout;
    if (fuzzed_data_provider.ConsumeBool()) {
        // pick the timestamp to switch based on a block
        // note states will change *after* these blocks because mediantime lags
        int start_block = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, period * (max_periods - 3));
        int end_block = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, period * (max_periods - 3));

        start_time = block_start_time + start_block * interval;
        timeout = block_start_time + end_block * interval;

        // allow for times to not exactly match a block
        if (fuzzed_data_provider.ConsumeBool()) start_time += interval / 2;
        if (fuzzed_data_provider.ConsumeBool()) timeout += interval / 2;
    } else {
        if (fuzzed_data_provider.ConsumeBool()) {
            start_time = Consensus::HereticalDeployment::ALWAYS_ACTIVE;
            always_active_test = true;
        } else {
            start_time = Consensus::HereticalDeployment::NEVER_ACTIVE;
            never_active_test = true;
        }
        timeout = fuzzed_data_provider.ConsumeBool() ? Consensus::HereticalDeployment::NO_TIMEOUT : fuzzed_data_provider.ConsumeIntegral<int64_t>();
    }

    // what values for version will we use to signal / not signal?
    const int32_t ver_activate = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    const int32_t ver_abandon = fuzzed_data_provider.ConsumeIntegral<int32_t>();

    TestConditionChecker checker(start_time, timeout, period, ver_activate, ver_abandon);

    assert(checker.ActivateVersion() == ver_activate);
    assert(checker.AbandonVersion() == ver_abandon);

    if (ver_activate < 0) return;
    if (ver_abandon < 0) return;
    if (ver_activate == ver_abandon) return; // not checking this case

    // Pick a non-signalling version
    const int32_t ver_nosignal = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    if (ver_nosignal < 0) return; // only positive versions are interesting
    if (ver_nosignal == ver_activate) return;
    if (ver_nosignal == ver_abandon) return;

    // Now that we have chosen time and versions, setup to mine blocks
    Blocks blocks(block_start_time, interval);

    const auto siginfo_nosignal = [&]() -> std::optional<SignalInfo> {
        int year, number, revision;
        if (checker.BINANA(year, number, revision)) {
            if ((ver_nosignal & 0xFFFFFF00l) == (ver_activate & 0xFFFFFF00l)) {
                return SignalInfo{.height = 0, .revision = static_cast<uint8_t>(ver_nosignal & 0xFF), .activate = true};
            } else if ((ver_nosignal & 0xFFFFFF00l) == (ver_abandon & 0xFFFFFF00l)) {
                return SignalInfo{.height = 0, .revision = static_cast<uint8_t>(ver_nosignal & 0xFF), .activate = false};
            }
        }
        return std::nullopt;
    }();

    /* Strategy:
     *  * we mine n*period blocks, with zero/one of
     *    those blocks signalling activation, and zero/one of
     *    them signalling abandonment
     *  * we then mine a final period worth of blocks, with
     *    randomised signalling
     */

    // mine prior periods
    const int prior_periods = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, max_periods);
    assert(prior_periods * period + period <= (int64_t)max_blocks); // fuzzer bug if this triggers

    const size_t activate_block = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, prior_periods * period);
    const size_t abandon_block = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, prior_periods * period);

    for (int i = 0; i < prior_periods; ++i) {
        for (int b = 0; b < period; ++b) {
            if (blocks.size() == abandon_block) {
                blocks.mine_block(ver_abandon);
            } else if (blocks.size() == activate_block) {
                blocks.mine_block(ver_activate);
            } else {
                blocks.mine_block(ver_nosignal);
            }
        }
    }

    // now we mine the final period and check that everything looks sane

    // get the info for the first block of the period
    CBlockIndex* prev = blocks.tip();
    const int exp_since = checker.GetStateSinceHeightFor(prev);
    const ThresholdState exp_state = checker.GetStateFor(prev);

    int prev_next_height = (prev == nullptr ? 0 : prev->nHeight + 1);
    assert(exp_since <= prev_next_height);

    // mine period blocks and check state doesn't change prior to mining the final block
    bool sig_active = false;
    bool sig_abandon = false;
    int next_active = 0;
    int next_abandon = 0;

    std::vector<SignalInfo> exp_siginfo = checker.GetSignalInfo(nullptr); // dummy
    assert(exp_siginfo.empty());

    for (int b = 1; b <= period; ++b) {
        CBlockIndex* current_block = blocks.tip();
        const int next_height = (current_block != nullptr ? current_block->nHeight + 1 : 0);

        if (current_block != nullptr) {
            // state and since don't change within the period
            const ThresholdState state = checker.GetStateFor(current_block);
            const int since = checker.GetStateSinceHeightFor(current_block);
            assert(state == exp_state);
            assert(since == exp_since);
        }

        while (b > next_abandon) next_abandon += 1 + fuzzed_data_provider.ConsumeIntegral<uint8_t>();
        while (b > next_active) next_active += 1 + fuzzed_data_provider.ConsumeIntegral<uint8_t>();
        if (b == next_abandon) {
            exp_siginfo.push_back({.height = next_height, .revision = -1, .activate = false});
            blocks.mine_block(ver_abandon);
            sig_abandon = true;
        } else if (b == next_active) {
            exp_siginfo.push_back({.height = next_height, .revision = -1, .activate = true});
            blocks.mine_block(ver_activate);
            sig_active = true;
        } else {
            if (siginfo_nosignal) {
                exp_siginfo.push_back(*siginfo_nosignal);
                exp_siginfo.back().height = next_height;
            }
            blocks.mine_block(ver_nosignal);
        }

        const std::vector<SignalInfo> siginfo = checker.GetSignalInfo(blocks.tip());
        assert(siginfo.size() == exp_siginfo.size());
        assert(std::equal(siginfo.begin(), siginfo.end(), exp_siginfo.rbegin(), exp_siginfo.rend()));
    }

    // grab the final block and the state it's left us in
    CBlockIndex* current_block = blocks.tip();
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
            assert(!sig_active && !sig_abandon);
        } else {
            assert(exp_state == ThresholdState::DEFINED);
        }
        break;
    case ThresholdState::LOCKED_IN:
        assert(current_block->GetMedianTimePast() >= checker.m_begin);
        assert(current_block->GetMedianTimePast() < checker.m_end);
        assert(exp_state == ThresholdState::STARTED);
        assert(sig_active && !sig_abandon);
        break;
    case ThresholdState::ACTIVE:
        if (!always_active_test) {
            assert(current_block->GetMedianTimePast() >= checker.m_begin);
            assert(current_block->GetMedianTimePast() < checker.m_end);
            assert(exp_state == ThresholdState::ACTIVE || exp_state == ThresholdState::LOCKED_IN);
            assert(!sig_abandon);
        }
        break;
    case ThresholdState::DEACTIVATING:
        assert(current_block->GetMedianTimePast() >= checker.m_begin);
        assert(exp_state == ThresholdState::LOCKED_IN || exp_state == ThresholdState::ACTIVE);
        assert(sig_abandon || current_block->GetMedianTimePast() >= checker.m_end);
        break;
    case ThresholdState::ABANDONED:
        if (exp_state == ThresholdState::DEFINED || exp_state == ThresholdState::STARTED) {
            assert(sig_abandon || current_block->GetMedianTimePast() >= checker.m_end);
        } else {
            assert(exp_state == ThresholdState::DEACTIVATING || exp_state == ThresholdState::ABANDONED);
        }
        break;
    default:
        assert(false);
    }

    if (always_active_test) {
        // "always active" has additional restrictions
        assert(state == ThresholdState::ACTIVE);
        assert(exp_state == ThresholdState::ACTIVE);
        assert(since == 0);
    } else if (never_active_test) {
        // "never active" does too
        assert(state == ThresholdState::ABANDONED);
        assert(exp_state == ThresholdState::ABANDONED);
        assert(since == 0);
    } else {
        // for signalled deployments, the initial state is always DEFINED
        assert(since > 0 || state == ThresholdState::DEFINED);
        assert(exp_since > 0 || exp_state == ThresholdState::DEFINED);

        if (blocks.size() >= period * max_periods) {
            // we chose the timeout (and block times) so that by the time we have this many blocks it's all over
            assert(state == ThresholdState::ABANDONED);
        }
    }
}
} // namespace
