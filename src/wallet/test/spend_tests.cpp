// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <key.h>
#include <key_io.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <util/strencodings.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(spend_tests, WalletTestingSetup)

BOOST_AUTO_TEST_CASE(max_signed_input_size_uses_external_outpoint)
{
    const CKey key{GenerateRandomKey()};
    FillableSigningProvider provider;
    BOOST_REQUIRE(provider.AddKey(key));

    const CTxOut txout{COIN, GetScriptForDestination(PKHash{key.GetPubKey()})};
    const COutPoint outpoint{Txid{}, 0};
    CCoinControl coin_control;
    coin_control.Select(outpoint).SetTxOut(txout);

    const int low_r{CalculateMaximumSignedInputSize(txout, COutPoint{}, &provider, /*can_grind_r=*/true, &coin_control)};
    const int high_r{CalculateMaximumSignedInputSize(txout, outpoint, &provider, /*can_grind_r=*/true, &coin_control)};
    BOOST_CHECK_EQUAL(high_r, low_r + 1);
}

BOOST_FIXTURE_TEST_CASE(SubtractFee, TestChain100Setup)
{
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    auto wallet = CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey);

    // Check that a subtract-from-recipient transaction slightly less than the
    // coinbase input amount does not create a change output (because it would
    // be uneconomical to add and spend the output), and make sure it pays the
    // leftover input amount which would have been change to the recipient
    // instead of the miner.
    auto check_tx = [&wallet](CAmount leftover_input_amount) {
        CRecipient recipient{PaymentDestination{PubKeyDestination({})}, 50 * COIN - leftover_input_amount, /*subtract_fee=*/true};
        CCoinControl coin_control;
        coin_control.m_feerate.emplace(10000);
        coin_control.fOverrideFeeRate = true;
        // We need to use a change type with high cost of change so that the leftover amount will be dropped to fee instead of added as a change output
        coin_control.m_change_type = OutputType::LEGACY;
        auto res = CreateTransaction(*wallet, {recipient}, /*change_pos=*/std::nullopt, coin_control);
        BOOST_CHECK(res);
        const auto& txr = *res;
        BOOST_CHECK_EQUAL(txr.tx->vout.size(), 1);
        BOOST_CHECK_EQUAL(txr.tx->vout[0].nValue, recipient.nAmount + leftover_input_amount - txr.fee);
        BOOST_CHECK_GT(txr.fee, 0);
        return txr.fee;
    };

    // Send full input amount to recipient, check that only nonzero fee is
    // subtracted (to_reduce == fee).
    const CAmount fee{check_tx(0)};

    // Send slightly less than full input amount to recipient, check leftover
    // input amount is paid to recipient not the miner (to_reduce == fee - 123)
    BOOST_CHECK_EQUAL(fee, check_tx(123));

    // Send full input minus fee amount to recipient, check leftover input
    // amount is paid to recipient not the miner (to_reduce == 0)
    BOOST_CHECK_EQUAL(fee, check_tx(fee));

    // Send full input minus more than the fee amount to recipient, check
    // leftover input amount is paid to recipient not the miner (to_reduce ==
    // -123). This overpays the recipient instead of overpaying the miner more
    // than double the necessary fee.
    BOOST_CHECK_EQUAL(fee, check_tx(fee + 123));
}

BOOST_FIXTURE_TEST_CASE(wallet_duplicated_preset_inputs_test, TestChain100Setup)
{
    // Verify that the wallet's Coin Selection process does not include pre-selected inputs twice in a transaction.

    // Add 4 spendable UTXO, 50 BTC each, to the wallet (total balance 200 BTC)
    for (int i = 0; i < 4; i++) CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    auto wallet = CreateSyncedWallet(*m_node.chain, WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return m_node.chainman->ActiveChain()), coinbaseKey);

    LOCK(wallet->cs_wallet);
    auto available_coins = AvailableCoins(*wallet);
    std::vector<COutput> coins = available_coins.All();
    // Preselect the first 3 UTXO (150 BTC total)
    std::set<COutPoint> preset_inputs = {coins[0].outpoint, coins[1].outpoint, coins[2].outpoint};

    // Try to create a tx that spends more than what preset inputs + wallet selected inputs are covering for.
    // The wallet can cover up to 200 BTC, and the tx target is 299 BTC.
    std::vector<CRecipient> recipients{{PaymentDestination{*Assert(wallet->GetNewDestination(OutputType::BECH32, "dummy"))},
                                           /*nAmount=*/299 * COIN, /*fSubtractFeeFromAmount=*/true}};
    CCoinControl coin_control;
    coin_control.m_allow_other_inputs = true;
    for (const auto& outpoint : preset_inputs) {
        coin_control.Select(outpoint);
    }

    // Attempt to send 299 BTC from a wallet that only has 200 BTC. The wallet should exclude
    // the preset inputs from the pool of available coins, realize that there is not enough
    // money to fund the 299 BTC payment, and fail with "Insufficient funds".
    //
    // Even with SFFO, the wallet can only afford to send 200 BTC.
    // If the wallet does not properly exclude preset inputs from the pool of available coins
    // prior to coin selection, it may create a transaction that does not fund the full payment
    // amount or, through SFFO, incorrectly reduce the recipient's amount by the difference
    // between the original target and the wrongly counted inputs (in this case 99 BTC)
    // so that the recipient's amount is no longer equal to the user's selected target of 299 BTC.

    // First case, use 'subtract_fee_from_outputs=true'
    BOOST_CHECK(!CreateTransaction(*wallet, recipients, /*change_pos=*/std::nullopt, coin_control));

    // Second case, don't use 'subtract_fee_from_outputs'.
    recipients[0].fSubtractFeeFromAmount = false;
    BOOST_CHECK(!CreateTransaction(*wallet, recipients, /*change_pos=*/std::nullopt, coin_control));
}

BOOST_AUTO_TEST_CASE(check_silent_payments_input)
{
    const COutPoint outpoint{Txid::FromUint256(uint256::ONE), 0};
    // Checks the coin of a wallet holding the descriptor
    const auto check{[&](const std::string& desc) {
        CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
        const auto spks{CreateDescriptor(wallet, desc, /*success=*/true)->GetScriptPubKeys()};
        BOOST_REQUIRE_EQUAL(spks.size(), 1U);
        return bool{CheckSilentPaymentsInput(wallet, outpoint, CTxOut{COIN, *spks.begin()})};
    }};
    const auto check_external{[&](const CScript& spk) {
        return bool{CheckSilentPaymentsInput(m_wallet, outpoint, CTxOut{COIN, spk})};
    }};
    const CKey key{GenerateRandomKey()};
    const std::string priv{EncodeSecret(key)};
    const std::string pub{HexStr(GenerateRandomKey().GetPubKey())};
    const std::string xonly_pub{HexStr(XOnlyPubKey{GenerateRandomKey().GetPubKey()})};

    // Eligible coins require their private key, which the wallet does not have for external ones
    BOOST_CHECK(check("wpkh(" + priv + ")"));
    BOOST_CHECK(!check("wpkh(" + pub + ")"));
    BOOST_CHECK(check("pkh(" + priv + ")"));
    BOOST_CHECK(!check("pkh(" + pub + ")"));
    BOOST_CHECK(check("sh(wpkh(" + priv + "))"));
    BOOST_CHECK(!check("sh(wpkh(" + pub + "))"));
    BOOST_CHECK(!check_external(GetScriptForDestination(WitnessV0KeyHash{key.GetPubKey()})));

    // Coins that are not eligible are allowed without a private key, also external ones
    const CScript p2wsh{GetScriptForDestination(WitnessV0ScriptHash{CScript{} << OP_TRUE})};
    BOOST_CHECK(check("wsh(pk(" + pub + "))"));
    BOOST_CHECK(check_external(p2wsh));

    // Unknown witness versions are not allowed
    const CScript witness_v2{CScript{} << OP_2 << std::vector<unsigned char>(32, 0x01)};
    BOOST_CHECK(!check("raw(" + HexStr(witness_v2) + ")"));
    BOOST_CHECK(!check_external(witness_v2));
    // Without the redeemScript, the wrapped witness version is unknown
    BOOST_CHECK(check("raw(" + HexStr(GetScriptForDestination(ScriptHash{witness_v2})) + ")"));

    // Taproot coins require the private key of the output key, which is derived from the internal key
    BOOST_CHECK(check("tr(" + priv + ")"));
    BOOST_CHECK(!check("tr(" + xonly_pub + ")"));
    BOOST_CHECK(check("tr(" + priv + ",pk(" + xonly_pub + "))"));
    BOOST_CHECK(!check("tr(" + xonly_pub + ",pk(" + priv + "))"));
    // Script path only coins, with the NUMS point as internal key, are not eligible
    BOOST_CHECK(check("tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",pk(" + xonly_pub + "))"));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
