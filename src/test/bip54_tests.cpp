// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <addresstype.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key.h>
#include <node/miner.h>
#include <policy/policy.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <streams.h>
#include <test/data/bip54_coinbases.json.h>
#include <test/data/bip54_timestamps.json.h>
#include <test/util/json.h>
#include <test/util/setup_common.h>
#include <test/util/transaction_utils.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/fs.h>
#include <util/strencodings.h>
#include <validation.h>

#include <ranges>
#include <string>

using namespace util::hex_literals;

BOOST_AUTO_TEST_SUITE(bip54_tests)

// Uncomment to output updated JSON tests.
// #define UPDATE_JSON_TESTS

#ifdef UPDATE_JSON_TESTS
/** Write a list of test vectors to disk as JSON. */
template<typename T>
static void WriteJSONTestVectors(const std::vector<T>& test_vectors, std::string_view filename)
{
    UniValue json_vectors{UniValue::VARR};
    for (const auto& test_vector: test_vectors) {
        json_vectors.push_back(test_vector.GetJson());
    }
    auto json_str{json_vectors.write(4)};
    json_str += '\n';
    FILE* file = fsbridge::fopen(filename, "w");
    fputs(json_str.c_str(), file);
    fclose(file);
}
#endif

/** A test vector for the per-transaction sigop limit in BIP54. */
struct TestVectorSigops {
    //! The transaction being evaluated.
    const CTransaction tx;
    //! The outputs corresponding to the transaction's inputs.
    const std::vector<CTxOut> spent_outputs;
    //! Whether this transaction passes the BIP54 sigops check.
    const bool valid;
    //! Description of the test vector.
    const std::string comment;

    explicit TestVectorSigops(CTransaction spending_tx, std::vector<CTxOut> spent_txos, bool success, std::string com):
        tx{std::move(spending_tx)}, spent_outputs{std::move(spent_txos)}, valid{success}, comment{std::move(com)} {}

    UniValue GetJson() const
    {
        UniValue json{UniValue::VOBJ}, spent_txos{UniValue::VARR};
        for (const auto& txo: spent_outputs) {
            DataStream ssTxo;
            ssTxo << txo;
            spent_txos.push_back(HexStr(ssTxo));
        }
        json.pushKV("comment", comment);
        json.pushKV("spent_outputs", std::move(spent_txos));
        json.pushKV("tx", EncodeHexTx(tx));
        json.pushKV("valid", valid);
        return json;
    }
};

/** Get the (non-extended) child private key at the provided derivation index. */
static CKey GetKeyAt(const CExtKey& parent_xprv, unsigned idx)
{
    CExtKey child_xprv;
    Assert(parent_xprv.Derive(child_xprv, idx));
    return child_xprv.key;
}

/** Generate an ECDSA signature for a specified input. */
static std::vector<uint8_t> SignInput(const CKey& key, const CScript& spent_script, CMutableTransaction& tx, unsigned idx, unsigned type = SIGHASH_ALL)
{
    const CAmount dummy{0};
    std::vector<uint8_t> sig;
    const auto sighash{SignatureHash(spent_script, tx, idx, type, dummy, SigVersion::BASE)};
    Assert(key.Sign(sighash, sig));
    sig.push_back(static_cast<uint8_t>(type));
    return sig;
}

/** Verify a transaction input's script against consensus rules. */
static bool VerifyTxin(const CScript& spent_script, const CMutableTransaction& tx, unsigned idx, std::vector<CTxOut>&& spent_outputs, const CAmount& amount)
{
    Assert(idx < tx.vin.size());
    PrecomputedTransactionData txdata;
    txdata.Init(tx, std::forward<std::vector<CTxOut>>(spent_outputs), /*force=*/true);
    const MutableTransactionSignatureChecker checker{&tx, idx, amount, txdata, MissingDataBehavior::ASSERT_FAIL};
    return VerifyScript(tx.vin[idx].scriptSig, spent_script, &tx.vin[idx].scriptWitness, MANDATORY_SCRIPT_VERIFY_FLAGS, checker);
}

/** Verify a Segwit v0 input's script. */
static bool VerifyTxin(const CScript& spent_script, const CMutableTransaction& tx, unsigned idx, const CAmount& amount)
{
    return VerifyTxin(spent_script, tx, idx, {}, amount);
}

/** Verify a legacy input's script. */
static bool VerifyTxin(const CScript& spent_script, const CMutableTransaction& tx, unsigned idx)
{
    return VerifyTxin(spent_script, tx, idx, {}, 0);
}

/** Get a list of all coins spent by this transaction. All coins must be in cache. */
template<typename T>
static std::vector<CTxOut> RecordSpent(const CCoinsViewCache& coins, const T& tx)
{
    std::vector<CTxOut> spent_outputs(tx.vin.size());
    for (size_t i{0}; i < tx.vin.size(); ++i) {
        const auto coin{*Assert(coins.GetCoin(tx.vin[i].prevout))};
        spent_outputs[i] = std::move(coin.out);
    }
    return spent_outputs;
}

/** Check this transaction does not exceed the BIP54 sigops limit, and record it as a test vector. */
static void CheckWithinBIP54Limits(CTransaction&& tx, const CCoinsViewCache& coins, std::vector<TestVectorSigops>& test_vectors, std::string comment)
{
    BOOST_CHECK_MESSAGE(Consensus::CheckSigopsBIP54(tx, coins), comment);

    auto spent_outputs{RecordSpent(coins, tx)};
    test_vectors.emplace_back(std::move(tx), std::move(spent_outputs), /*valid=*/true, std::move(comment));
}

/** Check this transaction exceeds the BIP54 sigops limit, and record it as a test vector. */
static void CheckExceedsBIP54Limits(CTransaction&& tx, const CCoinsViewCache& coins, std::vector<TestVectorSigops>& test_vectors, std::string comment)
{
    BOOST_CHECK_MESSAGE(!Consensus::CheckSigopsBIP54(tx, coins), comment);

    auto spent_outputs{RecordSpent(coins, tx)};
    test_vectors.emplace_back(std::move(tx), std::move(spent_outputs), /*valid=*/false, std::move(comment));
}

/**
 * Test the BIP54 per-transaction limit on legacy signature operations in inputs. We perform
 * extensive tests of the new limit from a few different perspectives. These extensive tests will
 * also be used to generate test vectors to help validate the BIP's semantics and re-implementation
 * of this logic. There are broadly 3 categories to this test. First, we check the bounds of the
 * new limit under various semi-realistic conditions: valid transactions with different combinations
 * of inputs and outputs types. Then, we exercise the new limit under some historical block chain
 * transactions known to be BIP54-invalid. Finally, we exercise some specific details and edge
 * cases of the implementation.
 */
BOOST_AUTO_TEST_CASE(bip54_legacy_sigops)
{
    ECC_Context ecc_ctx;
    // All keys in this test are derived from this seed using BIP32.
    CExtKey xprv;
    constexpr auto seed{std::to_array({std::byte{'B'}, std::byte{'I'}, std::byte{'P'}, std::byte{'5'}, std::byte{'4'}})};
    xprv.SetSeed(seed);

    // For all following test cases we will use a transaction with outputs of various types.
    CMutableTransaction tx;
    tx.vout.emplace_back(0, GetScriptForDestination(PubKeyDestination(GetKeyAt(xprv, 1).GetPubKey())));
    tx.vout.emplace_back(0, GetScriptForDestination(PKHash(GetKeyAt(xprv, 2).GetPubKey())));
    const auto ms_script{CScript{} << OP_2 << ToByteVector(GetKeyAt(xprv, 2).GetPubKey()) << ToByteVector(GetKeyAt(xprv, 3).GetPubKey())
                         << ToByteVector(GetKeyAt(xprv, 4).GetPubKey()) << OP_3 << OP_CHECKMULTISIG};
    tx.vout.emplace_back(0, ms_script);
    tx.vout.emplace_back(0, GetScriptForDestination(ScriptHash(ms_script)));
    tx.vout.emplace_back(0, GetScriptForDestination(WitnessV0KeyHash(GetKeyAt(xprv, 5).GetPubKey())));
    tx.vout.emplace_back(0, GetScriptForDestination(WitnessV0ScriptHash(ms_script)));
    tx.vout.emplace_back(0, GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey{GetKeyAt(xprv, 6).GetPubKey()})));
    tx.vout.emplace_back(0, GetScriptForDestination(PayToAnchor()));
    tx.vout.emplace_back(0, GetScriptForDestination(WitnessUnknown(8, {42, 42, 42})));
    Assert(tx.vout.size() == std::variant_size_v<CTxDestination>);

    // Record the test vectors.
    std::vector<TestVectorSigops> test_vectors;

    // Reach the 2'500 limit using only CHECKSIG's in a bare Script.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Use the first derivation as the private key to use in spent coins.
        const CKey privkey{GetKeyAt(xprv, 0)};
        const auto pubkey{privkey.GetPubKey()};

        // Create a spent Script that accounts for exactly a hundred sigops.
        auto spent_script{CScript() << ToByteVector(pubkey)};
        for (int i{0}; i < 99; ++i) {
            spent_script << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        spent_script << OP_CHECKSIG;

        // Reach 2500 sigops: one more sigop and we'll exceed the limit.
        for (int i{0}; i < 25; ++i) {
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(i, spent_script);
            AddCoins(coins, CTransaction(tx_create), 0, false);
            tx_copy.vin.emplace_back(tx_create.GetHash(), 0);
        }

        // Sign each input. Make sure all transaction inputs are valid.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << SignInput(privkey, spent_script, tx_copy, i);
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 2500 CHECKSIGs");

        // Add one more input with a single CHECKSIG.
        auto spent_script2{CScript() << ToByteVector(pubkey) << OP_CHECKSIG};
        CMutableTransaction tx_create_last;
        const auto idx{tx_copy.vin.size()};
        const auto value{static_cast<CAmount>(idx)};
        tx_create_last.vout.emplace_back(value, spent_script2);
        AddCoins(coins, CTransaction(tx_create_last), 0, false);
        tx_copy.vin.emplace_back(tx_create_last.GetHash(), 0);

        // Sign it and make sure it's valid.
        tx_copy.vin.back().scriptSig = CScript{} << SignInput(privkey, spent_script2, tx_copy, idx);
        BOOST_REQUIRE(VerifyTxin(spent_script2, tx_copy, idx));

        // Resign the previous inputs as we just invalidated the signature.
        for (size_t i{0}; i < tx_copy.vin.size() - 1; ++i) {
            tx_copy.vin[i].scriptSig = CScript{} << SignInput(privkey, spent_script, tx_copy, i);
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // Now we bump into the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 2501 CHECKSIGs");

        // Now malleate a bunch of unrelated fields to demonstrate how changing those does not affect
        // the BIP54 sigops calculation.
        tx_copy.version = 42;
        tx_copy.nLockTime = 21;
        tx_copy.vout = {tx_copy.vout.begin() + 2, tx_copy.vout.end()};
        tx_copy.vout[0].nValue = 50;
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].nSequence = (84 * i) | CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG;
        }

        // Resign inputs as we just invalidated the signatures.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            const bool last_txin{i == tx_copy.vin.size() - 1};
            const CScript& script{last_txin ? spent_script2 : spent_script};
            tx_copy.vin[i].scriptSig = CScript{} << SignInput(privkey, script, tx_copy, i);
            BOOST_REQUIRE(VerifyTxin(script, tx_copy, i));
        }

        // The number of accounted sigops hasn't changed. We still exceed the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 2501 CHECKSIGs and unrelated transaction fields malleated");

        // Drop the last input with the single CHECKSIG, and resign everything (since we use SIGHASH_ALL).
        tx_copy.vin.pop_back();
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig = CScript{} << SignInput(privkey, spent_script, tx_copy, i);
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // Now we don't exceed the limit anymore.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 2500 CHECKSIGs and unrelated transaction fields malleated");
    }

    // Reach the 2'500 limit using only CHECKSIG's in a P2SH redeemScript.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Use the second derivation as the private key to use in spent coins.
        const CKey privkey{GetKeyAt(xprv, 1)};
        const auto pubkey{privkey.GetPubKey()};

        // Create a redeem Script that accounts for exactly a hundred sigops.
        auto redeem_script{CScript() << ToByteVector(pubkey)};
        for (int i{0}; i < 99; ++i) {
            redeem_script = redeem_script << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        redeem_script << OP_CHECKSIG;
        const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

        // Reach 2500 sigops: one more sigop and we'll exceed the limit. Contrary
        // to the bare Script version, here we'll use a single creation tx.
        CMutableTransaction tx_create;
        for (int i{0}; i < 25; ++i) {
            tx_create.vout.emplace_back(i, spk);
        }
        const auto prev_txid{tx_create.GetHash()};
        for (int i{0}; i < 25; ++i) {
            tx_copy.vin.emplace_back(prev_txid, i);
        }
        AddCoins(coins, CTransaction(tx_create), 0, false);

        // Sign each input. Make sure all transaction inputs are valid.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << SignInput(privkey, redeem_script, tx_copy, i) << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "P2SH inputs totalling 2500 CHECKSIGs");

        // Add one more input with a single CHECKSIG (a bare P2PK, to mix input types).
        auto spent_script{CScript() << ToByteVector(pubkey) << OP_CHECKSIG};
        CMutableTransaction tx_create_last;
        const auto idx{tx_copy.vin.size()};
        const auto value{static_cast<CAmount>(idx)};
        tx_create_last.vout.emplace_back(value, spent_script);
        AddCoins(coins, CTransaction(tx_create_last), 0, false);
        tx_copy.vin.emplace_back(tx_create_last.GetHash(), 0);

        // Sign it and make sure it's valid.
        tx_copy.vin.back().scriptSig << SignInput(privkey, spent_script, tx_copy, idx);
        BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, idx));

        // Now we bump into the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "P2SH inputs totalling 2500 CHECKSIGs + 1 P2PK input");
    }

    // Create a transaction spending 250 7-of-10 bare multisigs with 10 different public keys.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Get 10 private keys to create the multisig.
        std::vector<CKey> privkeys;
        for (int i{0}; i < 10; ++i) {
            privkeys.push_back(GetKeyAt(xprv, 10 + i));
        }

        // A 7-of-10 multisig.
        auto spent_script{CScript() << OP_7};
        for (const auto& pk: privkeys) {
            spent_script << ToByteVector(pk.GetPubKey());
        }
        spent_script << OP_10 << OP_CHECKMULTISIG;

        // Spend 250 of those in a single transaction, reaching the 2500 sigops limit.
        for (int i{0}; i < 250; ++i) {
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(i, spent_script);
            AddCoins(coins, CTransaction(tx_create), 0, false);
            tx_copy.vin.emplace_back(tx_create.GetHash(), 0);
        }

        // Sign each input. Make sure all transaction inputs are valid. Sign using ACP so we
        // don't invalidate the signatures when adding an input below.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << OP_0;
            for (const auto& pk: privkeys | std::views::take(7)) {
                tx_copy.vin[i].scriptSig << SignInput(pk, spent_script, tx_copy, i, SIGHASH_ALL | SIGHASH_ANYONECANPAY);
            }
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 250 7-of-10 CHECKMULTISIGs");

        // Add a 1-of-1 CHECKMULTISIG input.
        auto single_spent_script{CScript{} << OP_1 << ToByteVector(privkeys.front().GetPubKey()) << OP_1 << OP_CHECKMULTISIG};
        CMutableTransaction tx_create_single;
        const auto idx{tx_copy.vin.size()};
        const auto value{static_cast<CAmount>(idx)};
        tx_create_single.vout.emplace_back(value, single_spent_script);
        AddCoins(coins, CTransaction(tx_create_single), 0, false);
        tx_copy.vin.emplace_back(tx_create_single.GetHash(), 0);

        // Sign the 1-of-1 CMS.
        tx_copy.vin.back().scriptSig << OP_0 << SignInput(privkeys.front(), single_spent_script, tx_copy, idx);
        BOOST_REQUIRE(VerifyTxin(single_spent_script, tx_copy, idx));

        // Now we do exceed the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 250 7-of-10 CHECKMULTISIGs + 1 1-of-1 CHECKMULTISIG");
    }

    // Create a transaction spending 125 16-of-17 bare multisigs. This demonstrates how
    // a pubkey count >16 will be accounted as 20 keys. This also repeats the same public
    // key and does not use ACP.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Get a single private key to repeat in the multisig.
        const auto privkey{GetKeyAt(xprv, 20)};
        const auto pubkey{privkey.GetPubKey()};

        // A 16-of-17 multisig with the same key repeated 17 times.
        auto spent_script{CScript() << OP_16};
        for (int i{0}; i < 17; ++i) {
            spent_script << ToByteVector(pubkey);
        }
        spent_script << 17 << OP_CHECKMULTISIG;

        // Spend 125 of those in a single transaction, reaching the 2500 sigops limit.
        for (int i{0}; i < 125; ++i) {
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(i, spent_script);
            AddCoins(coins, CTransaction(tx_create), 0, false);
            tx_copy.vin.emplace_back(tx_create.GetHash(), 0);
        }

        // Sign each input. Make sure all transaction inputs are valid.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << OP_0;
            const auto sig{SignInput(privkey, spent_script, tx_copy, i)};
            for (int j{0}; j < 16; ++j) {
                tx_copy.vin[i].scriptSig << sig;
            }
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "P2SH inputs totalling 125 16-of-17 CHECKMULTISIGs");

        // Add one more input with a single sigop (a P2PKH to mix input types).
        auto spk{GetScriptForDestination(PKHash(pubkey))};
        CMutableTransaction tx_create_last;
        const auto idx{tx_copy.vin.size()};
        const auto value{static_cast<CAmount>(idx)};
        tx_create_last.vout.emplace_back(value, spk);
        AddCoins(coins, CTransaction(tx_create_last), 0, false);
        tx_copy.vin.emplace_back(tx_create_last.GetHash(), 0);

        // Sign it and make sure it's valid.
        tx_copy.vin.back().scriptSig << SignInput(privkey, spk, tx_copy, idx) << ToByteVector(pubkey);
        BOOST_REQUIRE(VerifyTxin(spk, tx_copy, idx));

        // Resign all previous inputs which were invalidated by adding a new input.
        for (size_t i{0}; i < idx; ++i) {
            tx_copy.vin[i].scriptSig = CScript{} << OP_0;
            const auto sig{SignInput(privkey, spent_script, tx_copy, i)};
            for (int j{0}; j < 16; ++j) {
                tx_copy.vin[i].scriptSig << sig;
            }
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // Now we bump into the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "P2SH inputs totalling 125 16-of-17 CHECKMULTISIGs + 1 P2PKH input");
    }

    // Exceed the 2'500 limit using 18-of-18's CHECKMULTISIGs in an intentionally contrived P2SH redeemScript.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Use a single private key, we'll copy the key to create the multisigs.
        const CKey privkey{GetKeyAt(xprv, 30)};
        const auto pubkey{privkey.GetPubKey()};

        // Create a redeem script performing two 18-of-18 CHECKMULTISIG's.
        auto redeem_script{CScript() << ToByteVector(pubkey)};
        // From a stack `<sig> <pk>`, create `<sig> <pk> <> {<sig>}*18 <pk>`
        redeem_script << OP_DUP << OP_TOALTSTACK << OP_OVER << OP_0 << OP_SWAP << OP_DUP << OP_2DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_FROMALTSTACK;
        // From a stack `<sig> <pk> <> {<sig>}*18 <pk>`, create `<sig> <pk> <> {<sig>}*18 <18> {<pk>}*18 <18> CMSVERIFY`
        redeem_script << 18 << OP_SWAP << OP_DUP << OP_2DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << 18 << OP_CHECKMULTISIGVERIFY;
        // From a stack `<sig> <pk>`, create `<> {<sig>}*18 <pk>`
        redeem_script << OP_0 << OP_ROT << OP_ROT << OP_TOALTSTACK << OP_DUP << OP_2DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_FROMALTSTACK;
        // From a stack `<> {<sig>}*18 <pk>`, create `<> {<sig>}*18 <18> {<pk>}*18 <18> CHECKMULTISIG`
        redeem_script << 18 << OP_SWAP << OP_DUP << OP_2DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << 18 << OP_CHECKMULTISIG;
        const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

        // Reach 2400 sigops with 62 inputs.
        CMutableTransaction tx_create;
        for (int i{0}; i < 62; ++i) {
            tx_create.vout.emplace_back(i, spk);
        }
        const auto prev_txid{tx_create.GetHash()};
        for (int i{0}; i < 62; ++i) {
            tx_copy.vin.emplace_back(prev_txid, i);
        }
        AddCoins(coins, CTransaction(tx_create), 0, false);

        // Sign each input. Make sure all transaction inputs are valid.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << SignInput(privkey, redeem_script, tx_copy, i) << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "62 inputs with a contrived P2SH redeemScript executing 2 18-of-18 CHECKMULTISIGs");

        // Add one more input with the same spent script.
        const auto idx{tx_copy.vin.size()};
        const auto value{static_cast<CAmount>(idx)};
        tx_create.vout.emplace_back(value, spk);
        AddCoins(coins, CTransaction(tx_create), 0, false);
        tx_copy.vin.emplace_back(tx_create.GetHash(), idx);

        // Re-sign each input.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig = CScript{} << SignInput(privkey, redeem_script, tx_copy, i) << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy, i));
        }

        // We now reached 2600 sigops. We exceed the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "62 inputs with a contrived P2SH redeemScript executing 2 18-of-18 CHECKMULTISIGs");
    }

    // Now reach exactly 2500 sigops with a transaction mixing CMS-only input, CHECKSIG-only input, an input with
    // a mix of both, and 3 more of the same but under P2SH. Then we'll check we do exceed the limit by adding one
    // more legacy sigop, but not by adding non-legacy sigops for various existing non-legacy input types. This
    // test also demonstrates how sigops in unexecuted branches still account toward the limit.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // A script with 100 CHECKSIG's which takes a single sig as input.
        const auto cs_only_privkey{GetKeyAt(xprv, 40)};
        const auto cs_only_pubkey{cs_only_privkey.GetPubKey()};
        auto cs_only_script{CScript{} << ToByteVector(cs_only_pubkey)};
        for (int i{0}; i < 99; ++i) {
            cs_only_script << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        cs_only_script << OP_CHECKSIG;

        // A script with 9 1-of-20 CHECKMULTISIG's which takes 9 sigs as input.
        CScript cms_only_barescript;
        for (int i{0}; i < 9; ++i) {
            const auto privkey{GetKeyAt(xprv, 41 + i)};
            const auto pubkey{privkey.GetPubKey()};
            cms_only_barescript << OP_0 << OP_SWAP << OP_1;
            for (int j{0}; j < 20; ++j) cms_only_barescript << ToByteVector(pubkey);
            if (i < 9 - 1) {
                cms_only_barescript << 20 << OP_CHECKMULTISIGVERIFY;
            } else {
                cms_only_barescript << 20 << OP_CHECKMULTISIG;
            }
        }

        // A script with 50 CHECKSIG's and 5 19-of-19 CHECKMULTISIG's which takes as input
        // one signature for all CHECKSIG's and then the inputs to the 5 CMS.
        const auto mixed_bare_privkey{GetKeyAt(xprv, 50)};
        const auto mixed_bare_pubkey{mixed_bare_privkey.GetPubKey()};
        auto mixed_barescript{CScript{} << ToByteVector(mixed_bare_pubkey)};
        for (int i{0}; i < 49; ++i) {
            mixed_barescript << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        mixed_barescript << OP_CHECKSIGVERIFY;
        for (int i{0}; i < 5; ++i) {
            mixed_barescript << 19;
            for (int j{0}; j < 19; ++j) mixed_barescript << ToByteVector(mixed_bare_pubkey);
            mixed_barescript << 19;
            if (i < 5 - 1) {
                mixed_barescript << OP_CHECKMULTISIGVERIFY;
            } else {
                mixed_barescript << OP_CHECKMULTISIG;
            }
        }

        // A P2SH with 100 CHECKSIG's.
        const auto cs_only_p2sh{GetScriptForDestination(ScriptHash(cs_only_script))};

        // A P2SH with 8 16-of-16 CHECKMULTISIG's. Takes the inputs to all 8 CMS.
        CScript cms_only_redeem_script;
        for (int i{0}; i < 8; ++i) {
            const auto privkey{GetKeyAt(xprv, 51 + i)};
            const auto pubkey{privkey.GetPubKey()};
            cms_only_redeem_script << OP_16 << ToByteVector(pubkey) << OP_DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_16;
            if (i < 8 - 1) {
                cms_only_redeem_script << OP_CHECKMULTISIGVERIFY;
            } else {
                cms_only_redeem_script << OP_CHECKMULTISIG;
            }
        }
        const auto cms_only_p2sh{GetScriptForDestination(ScriptHash(cms_only_redeem_script))};

        // A P2SH with 9 13-of-13 CMS followed by 9 CHECKSIGs. The 9 CHECKMULTISIG's are in
        // a non-executed Script branch. Takes as input a single signature for the CHECKSIG's.
        const auto mixed_p2sh_privkey{GetKeyAt(xprv, 50)};
        const auto mixed_p2sh_pubkey{mixed_p2sh_privkey.GetPubKey()};
        auto mixed_redeem_script{CScript{} << OP_0 << OP_IF};
        for (int i{0}; i < 9; ++i) {
            mixed_redeem_script << OP_13 << ToByteVector(mixed_p2sh_pubkey) << OP_DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_13 << OP_CHECKMULTISIGVERIFY;
        }
        mixed_redeem_script << OP_ENDIF << ToByteVector(mixed_p2sh_pubkey);
        for (int i{0}; i < 4; ++i) {
            mixed_redeem_script << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        mixed_redeem_script << OP_CHECKSIG;
        const auto mixed_p2sh{GetScriptForDestination(ScriptHash(mixed_redeem_script))};

        // Now create the spending transaction. To start it will spend 2 inputs with a
        // bare script filled with CHECKSIG's. That's 200 sigops accounted for.
        for (int i{0}; i < 2; ++i) {
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(i, cs_only_script);
            AddCoins(coins, CTransaction(tx_create), 0, false);
            tx_copy.vin.emplace_back(tx_create.GetHash(), 0);
        }

        // Sign those two first inputs. (With ACP so we can add more inputs.)
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << SignInput(cs_only_privkey, cs_only_script, tx_copy, i, SIGHASH_ALL | SIGHASH_ANYONECANPAY);
            BOOST_REQUIRE(VerifyTxin(cs_only_script, tx_copy, i));
        }

        // Then it will spend 10 inputs with bare multisigs, bringing it to 2000 sigops
        // accounted for in total.
        {
            CMutableTransaction tx_create;
            for (int i{0}; i < 10; ++i) {
                tx_create.vout.emplace_back(i, cms_only_barescript);
            }
            const auto prev_txid{tx_create.GetHash()};
            for (int i{0}; i < 10; ++i) {
                tx_copy.vin.emplace_back(prev_txid, i);
            }
            AddCoins(coins, CTransaction(tx_create), 0, false);
        }

        // Sign those ten additional inputs. (With ACP so we can add more inputs.)
        for (size_t i{2}; i < tx_copy.vin.size(); ++i) {
            for (size_t j{0}; j < 9; ++j) {
                // We used indexes from 41 through 49 for the 9 CMS.
                const auto privkey{GetKeyAt(xprv, 49 - j)};
                const auto sig{SignInput(privkey, cms_only_barescript, tx_copy, i, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
                tx_copy.vin[i].scriptSig << sig;
            }
            BOOST_REQUIRE(VerifyTxin(cms_only_barescript, tx_copy, i));
        }

        // And it will spend one input for a bare Script with mixed CMS and CHECKSIGs. That's
        // now 2150 sigops accounted in total.
        {
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(422421, mixed_barescript);
            const auto prev_txid{tx_create.GetHash()};
            tx_copy.vin.emplace_back(prev_txid, 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);
        }

        // Sign this input (still ACP). Use ops in the scriptSig for a change.
        {
            const auto idx{tx_copy.vin.size() - 1};
            const auto sig{SignInput(mixed_bare_privkey, mixed_barescript, tx_copy, idx, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
            for (size_t i{0}; i < 5; ++i) {
                tx_copy.vin[idx].scriptSig << OP_0 << sig << OP_DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP;
            }
            tx_copy.vin[idx].scriptSig << sig; // For the CHECKSIG's
            BOOST_REQUIRE(VerifyTxin(mixed_barescript, tx_copy, idx));
        }

        // It will spend a single p2sh input with only CHECKSIGs, getting to 2250 sigops.
        {
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(181827, cs_only_p2sh);
            const auto prev_txid{tx_create.GetHash()};
            tx_copy.vin.emplace_back(prev_txid, 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);
        }

        // Sign this input (still ACP).
        {
            const auto idx{tx_copy.vin.size() - 1};
            const auto sig{SignInput(cs_only_privkey, cs_only_script, tx_copy, idx, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
            tx_copy.vin[idx].scriptSig << sig << ToByteVector(cs_only_script);
            BOOST_REQUIRE(VerifyTxin(cs_only_p2sh, tx_copy, idx));
        }

        // It will spend a single p2sh input only CMS's, getting to 2378 sigops.
        {
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(999, cms_only_p2sh);
            const auto prev_txid{tx_create.GetHash()};
            tx_copy.vin.emplace_back(prev_txid, 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);
        }

        // Sign the P2SH multisigs, still with ACP.
        {
            const auto idx{tx_copy.vin.size() - 1};
            for (int i{0}; i < 8; ++i) {
                // Traverse the derivation indexes used to create them (51 through 58) in reverse order.
                const auto privkey{GetKeyAt(xprv, 58 - i)};
                const auto sig{SignInput(privkey, cms_only_redeem_script, tx_copy, idx, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
                tx_copy.vin[idx].scriptSig << OP_0;
                for (int j{0}; j < 16; ++j) {
                    tx_copy.vin[idx].scriptSig << sig;
                }
            }
            tx_copy.vin[idx].scriptSig << ToByteVector(cms_only_redeem_script);
            BOOST_REQUIRE(VerifyTxin(cms_only_p2sh, tx_copy, idx));
        }

        // It will finally spend a single p2sh input with mixed CS - CMS, getting
        // to 2500 sigops.
        {
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(776, mixed_p2sh);
            const auto prev_txid{tx_create.GetHash()};
            tx_copy.vin.emplace_back(prev_txid, 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);
        }

        // Sign this final input. It only needs one signature for the CHECKSIG's because
        // the CMS are not executed.
        {
            const auto idx{tx_copy.vin.size() - 1};
            const auto sig{SignInput(mixed_p2sh_privkey, mixed_redeem_script, tx_copy, idx, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
            tx_copy.vin[idx].scriptSig << sig << ToByteVector(mixed_redeem_script);
            BOOST_REQUIRE(VerifyTxin(mixed_p2sh, tx_copy, idx));
        }

        // We reached exactly 2500 sigops, we don't exceed the limit.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops");

        // Now we are going to add an input and make sure we exceed the limit or not as
        // expected. This is the index of this input.
        const auto idx{tx_copy.vin.size()};

        // Adding a P2PK input will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 60)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spent_script{CScript{} << ToByteVector(pubkey) << OP_CHECKSIG};

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10101, spent_script);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            const auto sig{SignInput(privkey, spent_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig;
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2PK input");
        }

        // Adding a P2PKH input will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 60)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spk{GetScriptForDestination(PKHash(pubkey))};

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10101, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            const auto sig{SignInput(privkey, spk, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig << ToByteVector(pubkey);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2PKH input");
        }

        // Adding a 1-of-1 bare multisig input will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 61)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spent_script{CScript{} << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG};

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10102, spent_script);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            const auto sig{SignInput(privkey, spent_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << OP_0 << sig;
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a bare 1-of-1 multisig input");
        }

        // Adding an input spending an empty Script but having a sigop in the scriptSig
        // will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 62)};
            const auto pubkey{privkey.GetPubKey()};
            const CScript spent_script;

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10103, spent_script);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            const auto signed_script{CScript{} << ToByteVector(pubkey) << OP_CHECKSIG};
            const auto sig{SignInput(privkey, signed_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig << ToByteVector(pubkey) << OP_CHECKSIG;
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + an input with a CHECKSIG in the scriptSig");
        }

        // Adding an input spending a single CHECKSIG in a p2sh will make us exceed the
        // limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 63)};
            const auto pubkey{privkey.GetPubKey()};
            const auto redeem_script{CScript{} << ToByteVector(pubkey) << OP_CHECKSIG};
            const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10104, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            const auto sig{SignInput(privkey, redeem_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2SH input with a single CHECKSIG");
        }

        // Adding an input spending a 1of1 multisig in a p2sh will make us exceed the
        // limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 64)};
            const auto pubkey{privkey.GetPubKey()};
            const auto redeem_script{CScript{} << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG};
            const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10105, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            const auto sig{SignInput(privkey, redeem_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << OP_0 << sig << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2SH input with 1-of-1 CHECKMULTISIG");
        }

        // Adding an input spending an invalid Script but containing a CHECKSIG will
        // make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            const auto spent_script{CScript{} << OP_0 << OP_CHECKSIG};

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10106, spent_script);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + an input spending an invalid bare Script containing a CHECKSIG");
        }

        // Adding an input spending an invalid p2sh but containing a CHECKMULTISIG
        // will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto redeem_script{CScript{} << OP_RETURN << OP_CHECKMULTISIG};
            const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10107, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);
            tx_copy2.vin[idx].scriptSig << ToByteVector(redeem_script);

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + an input spending an invalid p2sh containing a CHECKMULTISIG");
        }

        // Adding an input spending a P2WPKH will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FillableSigningProvider keystore;
            SignatureData dummy_sigdata;

            const auto privkey{GetKeyAt(xprv, 65)};
            const auto pubkey{privkey.GetPubKey()};
            const auto keyhash{WitnessV0KeyHash(pubkey)};
            const auto spk{GetScriptForDestination(keyhash)};
            Assert(keystore.AddKeyPubKey(privkey, pubkey));

            const CAmount value{10108};
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(value, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, dummy_sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2WPKH input");
        }

        // Adding an input spending a P2WSH will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FillableSigningProvider keystore;
            SignatureData dummy_sigdata;

            const auto privkey{GetKeyAt(xprv, 66)};
            const auto pubkey{privkey.GetPubKey()};
            const auto witscript{CScript{} << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG};
            const auto witprogram{WitnessV0ScriptHash(witscript)};
            const auto spk{GetScriptForDestination(witprogram)};
            Assert(keystore.AddKeyPubKey(privkey, pubkey));
            Assert(keystore.AddCScript(witscript));

            const CAmount value{10109};
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(value, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, dummy_sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2WSH input");
        }

        // Adding an input spending a P2SH-P2WPKH will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FillableSigningProvider keystore;
            SignatureData dummy_sigdata;

            const auto privkey{GetKeyAt(xprv, 67)};
            const auto pubkey{privkey.GetPubKey()};
            const auto keyhash{WitnessV0KeyHash(pubkey)};
            const auto witprogram{GetScriptForDestination(keyhash)};
            const auto spk{GetScriptForDestination(ScriptHash(witprogram))};
            Assert(keystore.AddKeyPubKey(privkey, pubkey));
            Assert(keystore.AddCScript(witprogram));

            const CAmount value{10110};
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(value, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, dummy_sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2SH-P2WPKH input");
        }

        // Adding an input spending a P2SH-P2WSH will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FillableSigningProvider keystore;
            SignatureData dummy_sigdata;

            const auto privkey{GetKeyAt(xprv, 68)};
            const auto pubkey{privkey.GetPubKey()};
            const auto witscript{CScript{} << ToByteVector(pubkey) << OP_CHECKSIG};
            const auto witprogram{GetScriptForDestination(WitnessV0ScriptHash(witscript))};
            const auto spk{GetScriptForDestination(ScriptHash(witprogram))};
            Assert(keystore.AddKeyPubKey(privkey, pubkey));
            Assert(keystore.AddCScript(witscript));
            Assert(keystore.AddCScript(witprogram));

            const CAmount value{10111};
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(value, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, dummy_sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2SH-P2WSH input");
        }

        // Adding an input spending a Taproot through the key path will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FlatSigningProvider keystore;
            SignatureData sigdata;

            const auto privkey{GetKeyAt(xprv, 69)};
            const auto pubkey{privkey.GetPubKey()};
            TaprootBuilder builder;
            builder.Finalize(XOnlyPubKey{pubkey});
            const auto spk{GetScriptForDestination(builder.GetOutput())};
            keystore.keys[pubkey.GetID()] = privkey;

            const CAmount value{10112};
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(value, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);

            sigdata.tr_spenddata = builder.GetSpendData();
            auto spent_outputs{RecordSpent(coins, tx_copy2)};
            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, std::vector<CTxOut>(spent_outputs), SIGHASH_ALL, sigdata));
            const bool res{VerifyTxin(spk, tx_copy2, idx, std::move(spent_outputs), value)};
            BOOST_REQUIRE(res);

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a Taproot key path spend input");
        }

        // Adding an input spending a Taproot through the script path will not exceed the limit. This is
        // true also for future tapleaf versions.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FlatSigningProvider keystore;
            SignatureData sigdata;

            // Create a Taproot with one Tapscript leaf and one future-version leaf.
            TaprootBuilder builder;
            const auto privkey{GetKeyAt(xprv, 70)}, privkey_future{GetKeyAt(xprv, 71)};
            const auto pubkey{privkey.GetPubKey()}, pubkey_future{privkey_future.GetPubKey()};
            const auto leaf_script{CScript{} << ToByteVector(XOnlyPubKey{pubkey}) << OP_CHECKSIG};
            builder.Add(1, ToByteVector(leaf_script), TAPROOT_LEAF_TAPSCRIPT);
            const auto leaf_script_future{CScript{} << ToByteVector(XOnlyPubKey{pubkey_future}) << OP_CHECKSIG};
            const auto future_leaf_version{0xc2};
            builder.Add(1, ToByteVector(leaf_script_future), future_leaf_version);
            builder.Finalize(XOnlyPubKey::NUMS_H);
            const auto spk{GetScriptForDestination(builder.GetOutput())};

            // Record the coin paying to this Taproot.
            const CAmount value{10113};
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(value, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            AddCoins(coins, CTransaction(tx_create), 0, false);
            auto spent_outputs{RecordSpent(coins, tx_copy2)};

            // Spend through the Tapscript leaf.
            keystore.keys[pubkey.GetID()] = privkey;
            sigdata.tr_spenddata = builder.GetSpendData();
            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, std::vector<CTxOut>(spent_outputs), SIGHASH_ALL, sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, std::vector<CTxOut>(spent_outputs), value));
            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a Taproot script path spend input");

            // Spend through the future-version leaf.
            keystore.keys.clear();
            keystore.keys[pubkey_future.GetID()] = privkey_future;
            sigdata.tr_spenddata = builder.GetSpendData();
            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, std::vector<CTxOut>(spent_outputs), SIGHASH_ALL, sigdata));
            const bool res{VerifyTxin(spk, tx_copy2, idx, std::move(spent_outputs), value)};
            BOOST_REQUIRE(res);
            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a Taproot script path spend input of a future leaf version");
        }

        // Adding an input spending a future witness program does not somehow make us exceed
        // the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 72)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spk{GetScriptForDestination(WitnessUnknown(4, ToByteVector(pubkey)))};

            const CAmount value{10114};
            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(value, spk);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            tx_copy2.vin.back().scriptWitness.stack.push_back({0x42, 0x42});
            auto sig{"5da6d1157e4f2c45fa8441152f01e3e96898fd0c39326a47327d3bd024c4f6a2083e954ac803f8cb539aba8e31cdd9554a92309f47d489aa6a431ab262efa15a"_hex_v_u8};
            tx_copy2.vin.back().scriptWitness.stack.push_back(std::move(sig));
            tx_copy2.vin.back().scriptWitness.stack.push_back(ToByteVector(pubkey));
            AddCoins(coins, CTransaction(tx_create), 0, false);

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a future Segwit program input");
        }

        // Adding an input spending a bare Script with no sigop but with a sigop in the
        // scriptSig will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 73)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spent_script{CScript{} << OP_2 << OP_2 << OP_ADD << OP_4 << OP_EQUAL};

            CMutableTransaction tx_create;
            tx_create.vout.emplace_back(10101, spent_script);
            tx_copy2.vin.emplace_back(tx_create.GetHash(), 0);
            tx_copy2.vin.back().scriptSig << OP_0 << ToByteVector(pubkey) << OP_CHECKSIG << OP_DROP;
            AddCoins(coins, CTransaction(tx_create), 0, false);

            const auto sig{SignInput(privkey, spent_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig;
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + an input with one CHECKSIG in the scriptSig");
        }
    }

    // Demonstrate that CHECKMULTISIGs are also "accurately" counted in scriptSigs, using a
    // contrived example.
    // We spend an empty scriptPubKey with a scriptSig containing 166 unexecuted 15-pubkeys
    // CHECKMULTISIGVERIFY's, plus 10 CHECKSIGs. This reaches the BIP 54 limit but only exceeds
    // it once we add one more CHECKSIG, demonstrating that the 166 CMSV accounted for 15
    // sigops each and not 20.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // We spend a coin with an empty scriptPubkey.
        CScript spent_script{};
        CMutableTransaction mtx_create;
        mtx_create.vout.emplace_back(0, spent_script);
        AddCoins(coins, CTransaction(mtx_create), 0, false);
        tx_copy.vin.emplace_back(mtx_create.GetHash(), 0);

        // 166 * 15 = 1490 CMS sigops + 10 CHECKSIG sigops
        tx_copy.vin[0].scriptSig << OP_1 << OP_0 << OP_IF;
        for (int i{0}; i < 166; ++i) {
            tx_copy.vin[0].scriptSig << 15 << OP_CHECKMULTISIGVERIFY;
        }
        for (int i{0}; i < 10; ++i) {
            tx_copy.vin[0].scriptSig << OP_CHECKSIGVERIFY;
        }
        tx_copy.vin[0].scriptSig << OP_ENDIF;
        BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, 0));

        // We reach but don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "CHECKMULTISIG's in scriptSig with 16 pubkeys or less are not rounded up to 20 sigops");

        // Add a single CHECKSIG.
        tx_copy.vin[0].scriptSig << OP_0 << OP_1 << OP_CHECKSIG << OP_DROP;
        BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, 0));

        // Now we do.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Enough sigops in scriptSig to exceed the BIP 54 limit (even if unexecuted)");
    }

    // Some historical transactions which would have exceeded the BIP54 sigops limit. For
    // a full list of such transactions, see this mailing list post:
    // https://gnusha.org/pi/bitcoindev/49dyqqkf5NqGlGdinp6SELIoxzE_ONh3UIj6-EB8S804Id5yROq-b1uGK8DUru66eIlWuhb5R3nhRRutwuYjemiuOOBS2FQ4KWDnEh0wLuA=@protonmail.com/
    struct HistoricalTx {
        const std::vector<CTxOut> spent_outputs;
        const CTransaction tx;
    };
    std::vector<HistoricalTx> historical_txs;

    // This is bea1c2b87fee95a203c5b5d9f3e5d0f472385c34cb5af02d0560aab973169683.
    {
        DataStream stream{"0100000001cce43bcf04bec97d9389e2e92705723a6f5869770dde262312fbcccdaba35c9702000000fd2703510000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004cc9afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafffffffff01905f0100000000001976a9140af76822b5d13fd23b7b9380184c66e9a543368388ac00000000"_hex};
        std::vector<CTxOut> spent_outputs = {
            CTxOut{100000, CScript{} << OP_HASH160 << "923fdf3ff05b994004e374c69c8a2196c9e79344"_hex << OP_EQUAL},
        };
        historical_txs.push_back({
            .spent_outputs = std::move(spent_outputs),
            .tx = CTransaction{deserialize, TX_WITH_WITNESS, stream},
        });
    }

    // This is 62fc8d091a7c597783981f00b889d72d24ad5e3e224dbe1c2a317aabef89217e.
    {
        DataStream stream{"0100000002321438a932139c6642dfd5c14f231aed8988e3519e116ed96600db86a3511c3001000000cd51004cc963afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafaf68ffffffff321438a932139c6642dfd5c14f231aed8988e3519e116ed96600db86a3511c3002000000cd51004cc963afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafaf68ffffffff01905f0100000000001976a9140af770d29305bc0cfee379cac4d3f932dfb343de88ac00000000"_hex};
        std::vector<CTxOut> spent_outputs = {
            CTxOut{50000, CScript{} << OP_HASH160 << "a9395da5c22d266bb90ea2cc695a92ffd68c5597"_hex << OP_EQUAL},
            CTxOut{50000, CScript{} << OP_HASH160 << "a9395da5c22d266bb90ea2cc695a92ffd68c5597"_hex << OP_EQUAL},
        };
        historical_txs.push_back({
            .spent_outputs = std::move(spent_outputs),
            .tx = CTransaction{deserialize, TX_WITH_WITNESS, stream},
        });
    }

    for (const auto& tx: historical_txs) {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};

        for (size_t i{0}; i < tx.tx.vin.size(); ++i) {
            const auto& spent_txo{tx.spent_outputs[i]};
            CMutableTransaction mtx{tx.tx};
            BOOST_REQUIRE(VerifyTxin(spent_txo.scriptPubKey, mtx, i));
            coins.AddCoin(tx.tx.vin[i].prevout, Coin(spent_txo, 0, false), false);
        }

        std::string comment{"Historical Bitcoin transaction "};
        CheckExceedsBIP54Limits(CTransaction(tx.tx), coins, test_vectors, std::move(comment) + tx.tx.GetHash().ToString());
    }

    // Now we move on to test some pathological transactions to demonstrates edge cases of the
    // sigop accounting functions. Those transactions would already be invalid anyways, but
    // this is useful to generate explicit test vectors.

    // CheckSigopsBIP54() does not validate Script. It will count also for (some) invalid Scripts.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        // Create an unspendable bare Script which exceeds 2500 sigops.
        CScript spk;
        for (unsigned i{0}; i < MAX_TX_BIP54_SIGOPS + 1; ++i) {
            spk << OP_CHECKSIGVERIFY;
        }

        CMutableTransaction tx_create;
        tx_create.vout.emplace_back(0, spk);
        AddCoins(coins, CTransaction(tx_create), 0, false);
        tx2.vin.emplace_back(tx_create.GetHash(), 0);

        // CheckSigopsBIP54 will return false despite the Script being invalid.
        CheckExceedsBIP54Limits(CTransaction(tx2), coins, test_vectors, "Invalid bare script with 2501 CHECKSIGs");
    }

    // CheckSigopsBIP54 uses GetSigOpCount, which will only count the number of sigops in
    // CHECKMULTISIG operations accurately if the CMS / CMSVERIFY opcode is directly preceded
    // by OP_1-OP_16 and will always count it for 20 sigops otherwise. The previous tests have
    // exercised OP_0 and various counts >16. This exercises having another opcode as the count.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        // The first CMS will have OP_INVALIDOPCODE as its "last op", and count for 20. All the
        // following 125 CMS will have the previous CMS as their "last op", and will similarly
        // count for 20.
        CScript spk;
        for (unsigned i{0}; i < MAX_TX_BIP54_SIGOPS / 20 + 1; ++i) {
            spk << OP_CHECKMULTISIG;
        }

        CMutableTransaction tx_create;
        tx_create.vout.emplace_back(0, spk);
        AddCoins(coins, CTransaction(tx_create), 0, false);
        tx2.vin.emplace_back(tx_create.GetHash(), 0);

        // CheckSigopsBIP54 will return false because there is 126 CMS that account for 20 each.
        CheckExceedsBIP54Limits(CTransaction(tx2), coins, test_vectors, "Invalid bare script with 126 CHECKMULTISIGs each accounted for 20 BIP54-sigops");
    }

    // Note this is also a limitation for legitimate Scripts, for instance if the arguments to
    // CMS are decided dynamically.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        const auto privkey{GetKeyAt(xprv, 100)};
        const auto pubkey{privkey.GetPubKey()};

        // Start with a dummy multisig which can be either a 1of1 or a 1of2. Then pad the Script
        // with 2481 dummy CHECKSIG's.
        auto spk{CScript{} << OP_IF << OP_1 << ToByteVector(pubkey) << OP_1 << OP_ELSE};
        spk << OP_1 << ToByteVector(pubkey) << ToByteVector(pubkey) << OP_2 << OP_ENDIF << OP_CHECKMULTISIG;
        for (unsigned i{0}; i < MAX_TX_BIP54_SIGOPS - 20 + 1; ++i) {
            spk << OP_CHECKSIG;
        }

        CMutableTransaction tx_create;
        tx_create.vout.emplace_back(0, spk);
        AddCoins(coins, CTransaction(tx_create), 0, false);
        tx2.vin.emplace_back(tx_create.GetHash(), 0);

        // CheckSigopsBIP54 will return false because the first CHECKMULTISIG counts for 20 sigops.
        CheckExceedsBIP54Limits(CTransaction(tx2), coins, test_vectors, "Invalid bare script with 1 CHECKMULTISIG accounted for 20 sigops + 2481 CHECKSIGs");
    }

    // In case of parsing error, CheckSigopsBIP54 will count sigops up to the point with incorrect encoding.
    // Here we have 2501 sigops and an invalid PUSHDATA1. This will fail the check.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        CScript spk;
        for (unsigned i{0}; i < MAX_TX_BIP54_SIGOPS + 1; ++i) {
            spk << OP_CHECKSIG;
        }
        spk.push_back(static_cast<uint8_t>(OP_PUSHDATA1));
        spk.push_back(0x01);

        CMutableTransaction tx_create;
        tx_create.vout.emplace_back(0, spk);
        AddCoins(coins, CTransaction(tx_create), 0, false);
        tx2.vin.emplace_back(tx_create.GetHash(), 0);

        // CheckSigopsBIP54 will return false because 2501 sigops were counted before encountering the error.
        CheckExceedsBIP54Limits(CTransaction(tx2), coins, test_vectors, "Bare Script with malformed PUSHDATA1 after counting 2501 BIP54-sigops");
    }

    // Now we have 2500 CHECKSIGs before the PUSHDATA2 parsing error, and one after. This will pass the check.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        CScript spk;
        for (unsigned i{0}; i < MAX_TX_BIP54_SIGOPS; ++i) {
            spk << OP_CHECKSIG;
        }
        spk.push_back(static_cast<uint8_t>(OP_PUSHDATA2));
        spk.push_back(0x42);
        spk.push_back(0x42);
        spk << OP_CHECKSIG;

        CMutableTransaction tx_create;
        tx_create.vout.emplace_back(0, spk);
        AddCoins(coins, CTransaction(tx_create), 0, false);
        tx2.vin.emplace_back(tx_create.GetHash(), 0);

        // CheckSigopsBIP54 will return true because 2500 sigops were counted before encountering the error.
        CheckWithinBIP54Limits(CTransaction(tx2), coins, test_vectors, "Bare Script with malformed PUSHDATA2 after counting 2500 BIP54-sigops");
    }

    // Optionally dump test vectors as JSON. Uncomment UPDATE_JSON_TESTS at the top of this file to use.
#ifdef UPDATE_JSON_TESTS
    WriteJSONTestVectors(test_vectors, "bip54_sigops.json.gen");
#endif
}

/** A test case for the BIP 54 timestamp rules. */
struct TimestampTestCase {
    std::vector<CBlockHeader> header_chain;
    bool valid;
    std::string comment;
};

/**
 * The BIP 54 timestamp test vectors are arranged as a tree. This recursively build a list of
 * header-chain test cases from the root of the JSON test vectors.
 */
// NOLINTNEXTLINE(misc-no-recursion)
static std::vector<TimestampTestCase> VisitNode(std::vector<CBlockHeader> header_chain, const UniValue& test_node)
{
    std::vector<TimestampTestCase> test_cases;

    // Nodes of the tree are represented as JSON objects. They always have a "block_headers" field
    // that contains a chain of hex-encoded block headers..
    for (const auto& header_str: test_node["block_headers"].getValues()) {
        header_chain.emplace_back();
        BOOST_REQUIRE(DecodeHexBlockHeader(header_chain.back(), header_str.get_str()));
    }

    // ..And either contain an "extensions" field, which is an array of more nodes, in which case
    // the header chain in "block_headers" is a common ancestor to all those nodes..
    const auto& branches{test_node["extensions"]};
    if (!branches.isNull()) {
        for (const auto& branch: branches.getValues()) {
            auto cases{VisitNode(header_chain, branch)};
            test_cases.insert(test_cases.end(), std::make_move_iterator(cases.begin()), std::make_move_iterator(cases.end()));
        }
    } else {
        // ..Or contain "valid" and "comment" fields, which means this is a leaf of the tree that
        // represents a test case with the "block_headers" field comporting the final headers of the
        // test's header chain descending from the tree root.
        test_cases.emplace_back(TimestampTestCase {
            .header_chain = std::move(header_chain),
            .valid = test_node["valid"].get_bool(),
            .comment = test_node["comment"].get_str(),
        });
    }

    return test_cases;
}

/** Process chains of headers from the BIP54 test vectors for timewarp and Murch-Zawy fixes. */
BOOST_AUTO_TEST_CASE(bip54_timestamps)
{
    UniValue tests;
    Assert(tests.read(json_tests::bip54_timestamps));
    const auto test_cases{VisitNode({}, tests)};

    for (const auto& test: test_cases) {
        // All invalid test vectors fail on rules newly introduced by BIP54, which is a soft fork. Therefore
        // all cases will pass without the rules active.
        {
            auto test_setup{TestingSetup{ChainType::MAIN}};
            BlockValidationState state;
            BOOST_CHECK(test_setup.m_node.chainman->ProcessNewBlockHeaders(test.header_chain, /*min_pow_checked=*/true, state, /*ppindex=*/nullptr));
            BOOST_CHECK(state.IsValid());
        }

        // Now assert the validity status of each case under the new rules.
        {
            auto test_setup{TestingSetup{ChainType::MAIN, {.extra_args = {"-vbparams=consensuscleanup:-1:-1"}}}};
            BlockValidationState state;
            const bool res{test_setup.m_node.chainman->ProcessNewBlockHeaders(test.header_chain, /*min_pow_checked=*/true, state, /*ppindex=*/nullptr)};
            BOOST_CHECK_MESSAGE(res == test.valid, test.comment);
            if (!test.valid) {
                const auto reason{state.GetRejectReason()};
                BOOST_CHECK_MESSAGE(reason == "time-timewarp-attack" || reason == "time-negative-interval", test.comment);
            }
        }
    }
}

/** Run AcceptBlock() on this chain of blocks in order. */
static bool AcceptBlocks(ChainstateManager& chainman, std::vector<CBlock> blocks, BlockValidationState& state)
{
    LOCK(chainman.GetMutex());
    for (auto& block: blocks) {
        const auto shared_blk{std::make_shared<const CBlock>(std::move(block))};
        if (!chainman.AcceptBlock(shared_blk, state, /*ppindex=*/nullptr, /*fRequested*/true, /*dbp=*/nullptr, /*fNewBlock=*/nullptr, /*min_pow_checked=*/true)) {
            return false;
        }
    }
    return true;
}

/** Run the BIP54 test vectors for the new restrictions on coinbase transactions. */
BOOST_AUTO_TEST_CASE(bip54_coinbase)
{
    const auto tests{read_json(json_tests::bip54_coinbases)};

    for (const auto& test: tests.getValues()) {
        // Get the data for this test case.
        const auto is_valid{test["valid"].get_bool()};
        const auto& comment{test["comment"].get_str()};
        const auto blocks_hex{test["block_chain"].getValues()};
        std::vector<CBlock> blocks;
        for (const auto& blk: blocks_hex) {
            blocks.emplace_back();
            BOOST_REQUIRE(DecodeHexBlk(blocks.back(), blk.get_str()));
        }

        // Check the test case by activating the Consensus Cleanup and accepting those blocks. Note how some of the
        // test vectors demonstrate finality is enforced on coinbase transactions too, which is a rule pre-existing BIP54.
        {
            auto test_setup{TestingSetup{ChainType::MAIN, {.extra_args = {"-vbparams=consensuscleanup:-1:-1"}}}};
            BlockValidationState state;
            const bool res{AcceptBlocks(*test_setup.m_node.chainman, std::move(blocks), state) == is_valid};
            BOOST_CHECK_MESSAGE(res, comment);
            if (!is_valid) {
                const auto reason{state.GetRejectReason()};
                BOOST_CHECK_MESSAGE(reason == "bad-cb-locktime" || reason == "bad-cb-sequence" || reason == "bad-txns-nonfinal", comment);
            }
        }
    }
}

static void MineRegtestBlock(CBlock& block, const Consensus::Params& params)
{
    while (!CheckProofOfWork(block.GetHash(), block.nBits, params)) {
        Assert(++block.nNonce);
    }
}

/** A test case for the BIP54 rule on 64-byte transactions. */
struct TestVectorTxSize {
    //! The transaction to check against BIP54.
    const CTransaction tx;
    //! Whether this transaction is valid according to BIP54.
    bool valid;
    //! Description of this test case.
    std::string comment;

    explicit TestVectorTxSize(CTransaction t, bool val, std::string com)
        : tx{std::move(t)}, valid{val}, comment{std::move(com)} {}

    UniValue GetJson() const
    {
        UniValue json{UniValue::VOBJ};
        json.pushKV("comment", comment);
        json.pushKV("tx", EncodeHexTx(tx));
        json.pushKV("valid", valid);
        return json;
    }
};

static void RecordTestCase(std::vector<TestVectorTxSize>& test_vectors, CTransaction t, bool valid, std::string com)
{
    test_vectors.emplace_back(std::move(t), valid, std::move(com));
}

/**
 * Unit tests for the BIP54 rule on 64-byte transactions. Used to generate the BIP's test vectors. This checks
 * the bounds of the check with different transaction types (legacy / Segwit) as well as historical violations.
 */
BOOST_AUTO_TEST_CASE(bip54_txsize)
{
    std::vector<TestVectorTxSize> test_vectors;

    // Use this dummy transaction to craft a number of test cases exercising the bounds of the 64-byte check.
    CMutableTransaction tx;
    tx.vin.emplace_back(COutPoint{*Txid::FromHex("83c8e0289fecf93b5a284705396f5a652d9886cbd26236b0d647655ad8a37d82"), 21});
    tx.vout.emplace_back(0, CScript{});

    // A 63-byte transaction is valid.
    {
        CMutableTransaction tx_copy{tx};
        tx_copy.vout.back().scriptPubKey << OP_0 << OP_1 << OP_2;
        Assert(GetSerializeSize(TX_NO_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE - 1);
        RecordTestCase(test_vectors, CTransaction{tx_copy}, /*valid=*/true, "A 63-byte legacy transaction.");
    }

    // A less-than-64-byte transaction is valid even with a witness.
    {
        CMutableTransaction tx_copy{tx};
        tx_copy.vin.back().scriptWitness.stack.resize(1);
        Assert(GetSerializeSize(TX_NO_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE - 4);
        Assert(GetSerializeSize(TX_WITH_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE);
        RecordTestCase(test_vectors, CTransaction{tx_copy}, /*valid=*/true, "A 61-byte legacy transaction with a witness.");
    }

    // A 64-byte transaction is invalid.
    {
        CMutableTransaction tx_copy{tx};
        tx_copy.vout.back().scriptPubKey << OP_0 << OP_1 << OP_2 << OP_4;
        Assert(GetSerializeSize(TX_NO_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE);
        RecordTestCase(test_vectors, CTransaction{tx_copy}, /*valid=*/false, "A 64-byte legacy transaction (4 bytes in spk).");
    }

    // A 64-byte transaction is invalid even if constructed differently.
    {
        CMutableTransaction tx_copy{tx};
        tx_copy.vout.back().nValue = MAX_MONEY;
        tx_copy.vin.back().scriptSig << std::vector<uint8_t>{0x42, 0x42, 0x42};
        Assert(GetSerializeSize(TX_NO_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE);
        RecordTestCase(test_vectors, CTransaction{tx_copy}, /*valid=*/false, "A 64-byte legacy transaction (4 bytes in scriptsig).");
    }

    // A 65-byte transaction is valid.
    {
        CMutableTransaction tx_copy{tx};
        tx_copy.vout.back().scriptPubKey << OP_0 << OP_1 << OP_2 << OP_4 << OP_8;
        Assert(GetSerializeSize(TX_NO_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE + 1);
        RecordTestCase(test_vectors, CTransaction{tx_copy}, /*valid=*/true, "A 65-byte legacy transaction.");
    }

    // A 64-byte transaction is invalid even if it has a witness.
    {
        CMutableTransaction tx_copy{tx};
        tx_copy.vout.back().scriptPubKey << OP_0 << OP_1 << OP_2 << OP_4;
        tx_copy.vin.back().scriptWitness.stack.push_back({0x21, 0x32, 0x45, 0x57, 0x62, 0x81, 0x94, 0x12});
        Assert(GetSerializeSize(TX_WITH_WITNESS(tx_copy)) > INVALID_TX_NONWITNESS_SIZE);
        Assert(GetSerializeSize(TX_NO_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE);
        RecordTestCase(test_vectors, CTransaction{tx_copy}, /*valid=*/false, "A 64-byte Segwit transaction.");
    }

    // A semi-realistic 64-byte transaction: 1 native Segwit input, 1 anchor output.
    {
        CMutableTransaction tx_copy{tx};
        tx_copy.vout.back().scriptPubKey = GetScriptForDestination(PayToAnchor());
        auto sig{"5a78b5a14a2527feb02c08b8124e74c3b9bcc1bd3dba1fbfa87f1c930f28a46fea2bf375105dfd835e212c9127aad4976c46ef86be02edbb681e6f38f9a9e06f01"_hex_v_u8};
        tx_copy.vin.back().scriptWitness.stack.emplace_back(std::move(sig));
        Assert(GetSerializeSize(TX_NO_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE);
        RecordTestCase(test_vectors, CTransaction{tx_copy}, /*valid=*/false, "A 64-byte Segwit transaction (1 p2tr input, 1 p2a output).");
    }

    // A semi-realistic 64-byte transaction: 1 Taproot input with an annex, 1 OP_RETURN output.
    {
        CMutableTransaction tx_copy{tx};
        tx_copy.vout.back().scriptPubKey << OP_RETURN << "ab01"_hex_v;
        auto sig{"5a78b5a14a2527feb02c08b8124e74c3b9bcc1bd3dba1fbfa87f1c930f28a46fea2bf375105dfd835e212c9127aad4976c46ef86be02edbb681e6f38f9a9e06f01"_hex_v_u8};
        tx_copy.vin.back().scriptWitness.stack.emplace_back(std::move(sig));
        auto annex{"4242ffab2121"_hex_v_u8};
        tx_copy.vin.back().scriptWitness.stack.emplace_back(std::move(annex));
        Assert(GetSerializeSize(TX_NO_WITNESS(tx_copy)) == INVALID_TX_NONWITNESS_SIZE);
        RecordTestCase(test_vectors, CTransaction{tx_copy}, /*valid=*/false, "A 64-byte Segwit transaction (1 p2tr input with annex, 1 OP_RETURN output).");
    }

    // Historical 64-byte transactions. Taken from Chris Stewart's BIP53.
    constexpr std::string_view historical_txs_hex[]{
        "0200000001deb98691723fa71260ffca6ea0a7bc0a63b0a8a366e1b585caad47fb269a2ce401000000030251b201000000010000000000000000016a00000000",
        "01000000010d0afe3d74062ee60c0ec55579d691d8c8af5c04eb97b777157a21a8c5fb143d00000000035101b100000000010000000000000000016a01000000",
        "02000000011658a33df410379bb512206659910c9fbd0e50bfb732f7be9936558ff036919401000000035101b201000000010000000000000000016a00000000",
        "02000000011a7a4cf262fb7e53e2e6e0b2ef8b763f6ee97d8681ca968d1938418d56e6c38700000000035101b201000000010000000000000000016a00000000",
        "01000000019222bbb054bb9f94571dfe769af5866835f2a97e883959fa757de4064bed8bca01000000035101b100000000010000000000000000016a01000000",
    };
    for (const auto tx_hex: historical_txs_hex) {
        CMutableTransaction hist_tx;
        Assert(DecodeHexTx(hist_tx, std::string{tx_hex}));
        std::string comment{"Historical 64-byte transaction "};
        RecordTestCase(test_vectors, CTransaction(hist_tx), /*valid=*/false, comment + hist_tx.GetHash().ToString());
    }

    // Go through every recorded transaction and make sure ContextualCheckBlock() return value is
    // as expected by calling AcceptBlock().
    RegTestingSetup test_setup{};
    auto& chainman{*test_setup.m_node.chainman};
    const auto& params{chainman.GetConsensus()};
    LOCK(chainman.GetMutex());
    for (const auto& test_case: test_vectors) {
        // Craft a block containing this transaction.
        auto block{node::BlockAssembler{chainman.ActiveChainstate(), /*mempool=*/nullptr, {}}.CreateNewBlock()->block};
        block.vtx.push_back(MakeTransactionRef(test_case.tx));
        node::RegenerateCommitments(block, chainman);
        MineRegtestBlock(block, params);
        const auto pblock{std::make_shared<CBlock>(std::move(block))};
        // Preliminary checks performed on the block before storing it will detect any 64-byte
        // transaction.
        BlockValidationState state;
        const bool res{chainman.AcceptBlock(pblock, state, /*ppindex=*/nullptr, /*fRequested=*/true, /*dbp=*/nullptr, /*fNewBlock=*/nullptr, /*min_pow_checked=*/true)};
        BOOST_CHECK_MESSAGE(res == test_case.valid, test_case.comment);
        if (!test_case.valid) {
            BOOST_CHECK_MESSAGE(state.GetRejectReason() == "bad-txns-size", test_case.comment);
        }
    }

#ifdef UPDATE_JSON_TESTS
    WriteJSONTestVectors(test_vectors, "bip54_txsize.json.gen");
#endif
}

BOOST_AUTO_TEST_SUITE_END()
