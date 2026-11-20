// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <governance/superblock.h>
#include <key.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#include <uint256.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <vector>

struct SuperblockRegtestSetup : public BasicTestingSetup {
    SuperblockRegtestSetup() : BasicTestingSetup(CBaseChainParams::REGTEST) {}
};

BOOST_FIXTURE_TEST_SUITE(governance_superblock_tests, SuperblockRegtestSetup)

// Regression test for: CSuperblock::IsValid was matching expected payments
// against coinbase outputs using a forward scan that re-started at the
// previously matched index, which allowed two adjacent expected payments
// with identical scriptPubKey and amount to both match the same coinbase
// output. Each expected payment must consume a distinct output.
BOOST_AUTO_TEST_CASE(isvalid_duplicate_payments_require_distinct_outputs)
{
    const auto& consensus = Params().GetConsensus();
    const int nBlockHeight = consensus.nSuperblockStartBlock + consensus.nSuperblockCycle;
    BOOST_REQUIRE(CSuperblock::IsValidBlockHeight(nBlockHeight));

    CKey key;
    key.MakeNewKey(/*fCompressed=*/true);
    const CTxDestination dest{PKHash(key.GetPubKey())};
    const CScript scriptPayee = GetScriptForDestination(dest);
    const CAmount nPayAmount = 1 * COIN;

    // Two identical expected payments (same script, same amount).
    std::vector<CGovernancePayment> payments;
    payments.emplace_back(dest, nPayAmount, /*proposalHash=*/uint256());
    payments.emplace_back(dest, nPayAmount, /*proposalHash=*/uint256::ONE);
    BOOST_REQUIRE(payments[0].IsValid());
    BOOST_REQUIRE(payments[1].IsValid());

    CSuperblock sb(nBlockHeight, payments);
    BOOST_REQUIRE_EQUAL(sb.CountPayments(), 2);

    const CScript scriptMinerOrMN = CScript() << OP_RETURN;
    const CAmount blockReward = 500 * COIN;
    CChain dummy_chain;

    // Case 1 (regression, V24): coinbase carries only ONE output matching the
    // duplicate expected payment. With the buggy forward scan that restarted
    // at the previously matched index, both expected payments would match the
    // single matching vout and IsValid would (incorrectly) return true.
    // From V24 on, the second expected payment must find a distinct output
    // and validation must fail.
    {
        CMutableTransaction txNew;
        txNew.vout.emplace_back(blockReward - nPayAmount, scriptMinerOrMN);
        txNew.vout.emplace_back(nPayAmount, scriptPayee); // single matching output
        BOOST_CHECK(!sb.IsValid(dummy_chain, CTransaction(txNew), nBlockHeight, blockReward, /*is_v24=*/true));
    }

    // Case 2 (V24): coinbase carries TWO outputs matching the duplicate expected
    // payments. The fix must still accept this legitimate case.
    {
        CMutableTransaction txNew;
        txNew.vout.emplace_back(blockReward - 2 * nPayAmount, scriptMinerOrMN);
        txNew.vout.emplace_back(nPayAmount, scriptPayee);
        txNew.vout.emplace_back(nPayAmount, scriptPayee);
        BOOST_CHECK(sb.IsValid(dummy_chain, CTransaction(txNew), nBlockHeight, blockReward, /*is_v24=*/true));
    }

    // Case 3 (pre-V24): the stricter distinct-output rule is gated behind V24.
    // Before activation the legacy scan is preserved, so the single-output
    // coinbase from Case 1 must still be accepted to avoid changing consensus
    // for already-validated history.
    {
        CMutableTransaction txNew;
        txNew.vout.emplace_back(blockReward - nPayAmount, scriptMinerOrMN);
        txNew.vout.emplace_back(nPayAmount, scriptPayee); // single matching output
        BOOST_CHECK(sb.IsValid(dummy_chain, CTransaction(txNew), nBlockHeight, blockReward, /*is_v24=*/false));
    }
}

// The masternode building a trigger estimates the superblock's time from its own height and clock, within the last
// maturity window before the superblock, and accepts a proposal whose window is at most two hours off that estimate.
BOOST_AUTO_TEST_CASE(proposal_window_holds_exactly_the_chosen_superblocks)
{
    const int cycle{16616};
    const int maturity_window{1662};
    const int next_superblock{155 * cycle};
    const int64_t now{1'800'000'000};
    const int64_t fudge{2 * 60 * 60};

    for (const double block_seconds : {2.62 * 60 * 0.99, 2.62 * 60, 2.62 * 60 * 1.01}) {
        for (const int blocks_to_next : {maturity_window + 1, cycle / 2 - 1, cycle / 2 + 1, cycle - 1}) {
            const int tip{next_superblock - blocks_to_next};
            const auto estimated_by_trigger_builder = [&](int superblock, int trigger_height) {
                const auto trigger_time{static_cast<int64_t>(now + (trigger_height - tip) * block_seconds)};
                return governance::EstimateBlockTime(superblock, trigger_height, trigger_time);
            };
            for (int skipped{0}; skipped < 12; ++skipped) {
                for (int payments{1}; payments <= 12; ++payments) {
                    const int first_superblock{next_superblock + skipped * cycle};
                    const auto window{governance::EstimateProposalWindow(first_superblock, payments, cycle, tip, now)};

                    for (int n{-1}; n <= payments; ++n) {
                        const int superblock{first_superblock + n * cycle};
                        if (superblock <= tip) continue;
                        const bool chosen{n >= 0 && n < payments};
                        const int64_t slack{chosen ? 0 : fudge};
                        for (const int trigger_height : {superblock - maturity_window, superblock - 1}) {
                            const int64_t time{estimated_by_trigger_builder(superblock, trigger_height)};
                            const bool inside{window.start_epoch - slack <= time && time <= window.end_epoch + slack};
                            BOOST_CHECK_MESSAGE(inside == chosen,
                                                "blocks_to_next=" << blocks_to_next << " skipped=" << skipped
                                                                  << " payments=" << payments << " n=" << n
                                                                  << " trigger_height=" << trigger_height);
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
