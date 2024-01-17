// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <node/context.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <random.h>
#include <test/util/setup_common.h>
#include <util/translation.h>
#include <wallet/coincontrol.h>
#include <wallet/coinselection.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <random>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(coinselector_tests, WalletTestingSetup)

typedef std::set<std::shared_ptr<COutput>> CoinSet;

static const CoinEligibilityFilter filter_standard(1, 6, 0);
static int nextLockTime = 0;

static void add_coin(CoinsResult& available_coins, CWallet& wallet, const CAmount& nValue, CFeeRate feerate = CFeeRate(0), int nAge = 6*24, bool fIsFromMe = false, int nInput =0, bool spendable = false, int custom_size = 0)
{
    CMutableTransaction tx;
    tx.nLockTime = nextLockTime++;        // so all transactions get different hashes
    tx.vout.resize(nInput + 1);
    tx.vout[nInput].nValue = nValue;
    if (spendable) {
        tx.vout[nInput].scriptPubKey = GetScriptForDestination(*Assert(wallet.GetNewDestination(OutputType::BECH32, "")));
    }
    uint256 txid = tx.GetHash();

    LOCK(wallet.cs_wallet);
    auto ret = wallet.mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(txid), std::forward_as_tuple(MakeTransactionRef(std::move(tx)), TxStateInactive{}));
    assert(ret.second);
    CWalletTx& wtx = (*ret.first).second;
    const auto& txout = wtx.tx->vout.at(nInput);
    available_coins.Add(OutputType::BECH32, {COutPoint(wtx.GetHash(), nInput), txout, nAge, custom_size == 0 ? CalculateMaximumSignedInputSize(txout, &wallet, /*coin_control=*/nullptr) : custom_size, /*spendable=*/ true, /*solvable=*/ true, /*safe=*/ true, wtx.GetTxTime(), fIsFromMe, feerate});
}

// Helpers

static std::unique_ptr<CWallet> NewWallet(const node::NodeContext& m_node, const std::string& wallet_name = "")
{
    std::unique_ptr<CWallet> wallet = std::make_unique<CWallet>(m_node.chain.get(), wallet_name, CreateMockableWalletDatabase());
    BOOST_CHECK(wallet->LoadWallet() == DBErrors::LOAD_OK);
    LOCK(wallet->cs_wallet);
    wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    wallet->SetupDescriptorScriptPubKeyMans();
    return wallet;
}


static util::Result<SelectionResult> select_coins(const CAmount& target, const CoinSelectionParams& cs_params, const CCoinControl& cc, std::function<CoinsResult(CWallet&)> coin_setup, const node::NodeContext& m_node)
{
    std::unique_ptr<CWallet> wallet = NewWallet(m_node);
    auto available_coins = coin_setup(*wallet);

    LOCK(wallet->cs_wallet);
    auto result = SelectCoins(*wallet, available_coins, /*pre_set_inputs=*/ {}, target, cc, cs_params);
    if (result) {
        const auto signedTxSize = 10 + 34 + 68 * result->GetInputSet().size(); // static header size + output size + inputs size (P2WPKH)
        BOOST_CHECK_LE(signedTxSize * WITNESS_SCALE_FACTOR, MAX_STANDARD_TX_WEIGHT);

        BOOST_CHECK_GE(result->GetSelectedValue(), target);
    }
    return result;
}

static bool has_coin(const CoinSet& set, CAmount amount)
{
    return std::any_of(set.begin(), set.end(), [&](const auto& coin) { return coin->GetEffectiveValue() == amount; });
}

BOOST_AUTO_TEST_CASE(check_max_weight)
{
    const CAmount target = 49.5L * COIN;
    CCoinControl cc;

    FastRandomContext rand;
    CoinSelectionParams cs_params{
        rand,
        /*change_output_size=*/34,
        /*change_spend_size=*/68,
        /*min_change_target=*/CENT,
        /*effective_feerate=*/CFeeRate(0),
        /*long_term_feerate=*/CFeeRate(0),
        /*discard_feerate=*/CFeeRate(0),
        /*tx_noinputs_size=*/10 + 34, // static header size + output size
        /*avoid_partial=*/false,
    };

    {
        // Scenario 1:
        // The actor starts with 1x 50.0 BTC and 1515x 0.033 BTC (~100.0 BTC total) unspent outputs
        // Then tries to spend 49.5 BTC
        // The 50.0 BTC output should be selected, because the transaction would otherwise be too large

        // Perform selection

        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 1515; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.033 * COIN), CFeeRate(0), 144, false, 0, true);
                }

                add_coin(available_coins, wallet, CAmount(50 * COIN), CFeeRate(0), 144, false, 0, true);
                return available_coins;
            },
            m_node);

        BOOST_CHECK(result);
        // Verify that only the 50 BTC UTXO was selected
        const auto& selection_res = result->GetInputSet();
        BOOST_CHECK(selection_res.size() == 1);
        BOOST_CHECK((*selection_res.begin())->GetEffectiveValue() == 50 * COIN);
    }

    {
        // Scenario 2:

        // The actor starts with 400x 0.0625 BTC and 2000x 0.025 BTC (75.0 BTC total) unspent outputs
        // Then tries to spend 49.5 BTC
        // A combination of coins should be selected, such that the created transaction is not too large

        // Perform selection
        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 400; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.0625 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                for (int j = 0; j < 2000; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.025 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                return available_coins;
            },
            m_node);

        BOOST_CHECK(has_coin(result->GetInputSet(), CAmount(0.0625 * COIN)));
        BOOST_CHECK(has_coin(result->GetInputSet(), CAmount(0.025 * COIN)));
    }

    {
        // Scenario 3:

        // The actor starts with 1515x 0.033 BTC (49.995 BTC total) unspent outputs
        // No results should be returned, because the transaction would be too large

        // Perform selection
        const auto result = select_coins(
            target, cs_params, cc, [&](CWallet& wallet) {
                CoinsResult available_coins;
                for (int j = 0; j < 1515; ++j) {
                    add_coin(available_coins, wallet, CAmount(0.033 * COIN), CFeeRate(0), 144, false, 0, true);
                }
                return available_coins;
            },
            m_node);

        // No results
        // 1515 inputs * 68 bytes = 103,020 bytes
        // 103,020 bytes * 4 = 412,080 weight, which is above the MAX_STANDARD_TX_WEIGHT of 400,000
        BOOST_CHECK(!result);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
