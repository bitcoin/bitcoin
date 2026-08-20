// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <headerssync.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/time.h>
#include <validation.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <vector>

static void initialize_headers_sync_state_fuzz()
{
    static const auto testing_setup = MakeNoLogFileContext<>(
        /*chain_type=*/ChainType::MAIN);
}

void MakeHeadersContinuous(
    const CBlockHeader& genesis_header,
    const std::vector<CBlockHeader>& all_headers,
    std::vector<CBlockHeader>& new_headers)
{
    Assume(!new_headers.empty());

    const CBlockHeader* prev_header{
        all_headers.empty() ? &genesis_header : &all_headers.back()};

    for (auto& header : new_headers) {
        header.hashPrevBlock = prev_header->GetHash();

        prev_header = &header;
    }
}

class FuzzedHeadersSyncState : public HeadersSyncState
{
public:
    FuzzedHeadersSyncState(const HeadersSyncParams& sync_params, const size_t commit_offset,
                           const CBlockIndex& chain_start, const arith_uint256& minimum_required_work)
        : HeadersSyncState(/*id=*/0, Params().GetConsensus(), sync_params, chain_start, minimum_required_work)
    {
        const_cast<size_t&>(m_commit_offset) = commit_offset;
    }
};

FUZZ_TARGET(headers_sync_state, .init = initialize_headers_sync_state_fuzz)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    CBlockHeader genesis_header{Params().GenesisBlock()};
    CBlockIndex start_index(genesis_header);

    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider, /*min=*/start_index.GetMedianTimePast())};

    const uint256 genesis_hash = genesis_header.GetHash();
    start_index.phashBlock = &genesis_hash;

    const HeadersSyncParams params{
        .commitment_period = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 2000),
        .redownload_buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 40000),
    };
    arith_uint256 min_work{UintToArith256(ConsumeUInt256(fuzzed_data_provider))};
    FuzzedHeadersSyncState headers_sync(
        params,
        /*commit_offset=*/fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, params.commitment_period - 1),
        /*chain_start=*/start_index,
        /*minimum_required_work=*/min_work);

    // Store headers for potential redownload phase.
    std::vector<CBlockHeader> all_headers;
    std::vector<CBlockHeader>::const_iterator redownloaded_it;
    bool presync{true};
    bool requested_more{true};

    while (requested_more) {
        std::vector<CBlockHeader> headers;

        // Consume headers from fuzzer or maybe replay headers if we got to the
        // redownload phase.
        if (presync || fuzzed_data_provider.ConsumeBool()) {
            auto deser_headers = ConsumeDeserializable<std::vector<CBlockHeader>>(fuzzed_data_provider);
            if (!deser_headers || deser_headers->empty()) return;

            if (fuzzed_data_provider.ConsumeBool()) {
                MakeHeadersContinuous(genesis_header, all_headers, *deser_headers);
            }

            headers.swap(*deser_headers);
        } else if (auto num_headers_left{std::distance(redownloaded_it, all_headers.cend())}; num_headers_left > 0) {
            // Consume some headers from the redownload buffer (At least one
            // header is consumed).
            auto begin_it{redownloaded_it};
            std::advance(redownloaded_it, fuzzed_data_provider.ConsumeIntegralInRange<int>(1, num_headers_left));
            headers.insert(headers.cend(), begin_it, redownloaded_it);
        }

        if (headers.empty()) return;
        auto result = headers_sync.ProcessNextHeaders(headers, fuzzed_data_provider.ConsumeBool());
        requested_more = result.request_more;

        if (result.request_more) {
            if (presync) {
                all_headers.insert(all_headers.cend(), headers.cbegin(), headers.cend());

                if (headers_sync.GetState() == HeadersSyncState::State::REDOWNLOAD) {
                    presync = false;
                    redownloaded_it = all_headers.cbegin();

                    // If we get to redownloading, the presynced headers need
                    // to have the min amount of work on them.
                    assert(CalculateClaimedHeadersWork(all_headers) >= min_work);
                }
            }

            (void)headers_sync.NextHeadersRequestLocator();
        }
    }
}

/** Maximum timespan that the headers sync parameter computation can be asked about.
 *  std::chrono::seconds is only guaranteed to be a signed integer type of at least 35 bits. */
constexpr int64_t MAX_TIMESPAN = (int64_t{1} << 34) - 1;

/** The largest max_headers value ComputeHeadersSyncParams can pass to the optimizer. */
constexpr int64_t MAX_MAX_HEADERS = 6 * MAX_TIMESPAN;

FUZZ_TARGET(headers_sync_params_inner)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    // The number of headers in the minimum-chainwork chain is a block height, so any legal value
    // fits in an int (see Consensus::Params::minchainwork_height).
    auto minchainwork_headers = provider.ConsumeIntegralInRange<int64_t>(1, std::numeric_limits<int>::max());
    // The optimizer requires max_headers > 2 * minchainwork_headers.
    auto max_headers = provider.ConsumeIntegralInRange<int64_t>(2 * minchainwork_headers + 1, MAX_MAX_HEADERS);
    // The approximate number of bits of security. Beyond ~900 bits, the internal computations underflow.
    auto bits = provider.ConsumeIntegralInRange<int>(1, 900000) / 1000.0;

    // Reason backwards, approximately, from security bits to attack_headers input. 0.7 is (a guess
    // for) the internal kappa value.
    auto memory_factor = std::sqrt(double(max_headers - minchainwork_headers) / (sizeof(CompressedHeader) * 8));
    auto attack_headers{0.7 * memory_factor * std::exp2(-bits)};

    auto [period, bufsize] = ComputeHeadersSyncParamsInner(max_headers, minchainwork_headers, attack_headers);

    // The commitment period is positive, and never exceeds the length of the minimum-chainwork
    // chain (a larger period could fail to commit to that chain at all).
    assert(period >= 1);
    assert(period <= uint64_t(minchainwork_headers));
    // The redownload buffer holds at least one header.
    assert(bufsize >= 1);
    // A header released from the redownload buffer with no verified commitment on top of it
    // (possible when bufsize < period) gets accepted with probability >= 0.5 by an
    // optimally-placed attack, so any acceptable rate below that requires bufsize >= period.
    if (attack_headers < 0.5) assert(bufsize >= period);
}

FUZZ_TARGET(headers_sync_params)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    std::chrono::seconds timespan{provider.ConsumeIntegralInRange<int64_t>(-MAX_TIMESPAN, MAX_TIMESPAN)};
    auto minchainwork_headers = provider.ConsumeIntegralInRange<int64_t>(1, std::numeric_limits<int>::max());

    auto [period, bufsize] = ComputeHeadersSyncParams(timespan, minchainwork_headers);

    assert(period >= 1);
    assert(period <= uint64_t(minchainwork_headers));
    // The acceptable attack rate used internally is always far below 0.5 headers per attack, so
    // every released header has at least one verified commitment on top of it (see the
    // headers_sync_params_inner target).
    assert(bufsize >= period);
}
