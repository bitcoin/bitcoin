// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <coins.h>
#include <common/args.h>
#include <common/signmessage.h>
#include <key.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/solver.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <test/data/bip322_vectors.json.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace wallet {
namespace {
using Witness = std::vector<std::vector<unsigned char>>;

UniValue Vectors()
{
    UniValue vectors;
    BOOST_REQUIRE(vectors.read(json_tests::bip322_vectors));
    return vectors;
}

CTxDestination Import(CWallet& wallet, const std::string& descriptor)
{
    auto* spkm = CreateDescriptor(wallet, descriptor, /*success=*/true);
    BOOST_REQUIRE(spkm);
    const auto scripts = spkm->GetScriptPubKeys();
    BOOST_REQUIRE_EQUAL(scripts.size(), 1);
    CTxDestination destination;
    BOOST_REQUIRE(ExtractDestination(*scripts.begin(), destination));
    return destination;
}

Witness SignProof(CWallet& wallet, const CTxDestination& destination, const std::string& message)
{
    LOCK(wallet.cs_wallet);
    const auto tx_count = wallet.mapWallet.size();
    const auto order_count = wallet.wtxOrdered.size();
    std::string proof;
    BOOST_REQUIRE(wallet.SignBIP322Message(message, destination, proof) == SigningResult::OK);
    BOOST_REQUIRE(proof.starts_with("smp"));
    BOOST_CHECK(BIP322Verify(destination, proof, message) == BIP322VerificationResult::OK);
    BOOST_CHECK(BIP322Verify(destination, proof.substr(3), message) == BIP322VerificationResult::OK);
    BOOST_CHECK(BIP322Verify(destination, proof, message + "!") != BIP322VerificationResult::OK);
    const auto decoded = DecodeBase64(proof.substr(3));
    BOOST_REQUIRE(decoded);
    SpanReader reader{*decoded};
    Witness witness;
    reader >> witness;
    BOOST_CHECK(reader.empty());
    DataStream encoded;
    encoded << witness;
    BOOST_CHECK_EQUAL("smp" + EncodeBase64(encoded), proof);
    BOOST_CHECK_EQUAL(wallet.mapWallet.size(), tx_count);
    BOOST_CHECK_EQUAL(wallet.wtxOrdered.size(), order_count);
    return witness;
}

void CheckFailure(CWallet& wallet, const CTxDestination& destination, SigningResult expected)
{
    LOCK(wallet.cs_wallet);
    const auto tx_count = wallet.mapWallet.size();
    const auto order_count = wallet.wtxOrdered.size();
    for (const std::string initial : {"", "previous signature"}) {
        std::string output{initial};
        for (int attempt{0}; attempt < 2; ++attempt) {
            BOOST_CHECK(wallet.SignBIP322Message("failure test", destination, output) == expected);
            BOOST_CHECK_EQUAL(output, initial);
        }
    }
    BOOST_CHECK_EQUAL(wallet.mapWallet.size(), tx_count);
    BOOST_CHECK_EQUAL(wallet.wtxOrdered.size(), order_count);
}

// Preserve the public-key order of the published witness script.
std::string MultisigDescriptor(const UniValue& row)
{
    const auto bytes = ParseHex(row["witness_script"].get_str());
    const CScript script{bytes.begin(), bytes.end()};
    std::vector<std::vector<unsigned char>> solutions;
    BOOST_REQUIRE(Solver(script, solutions) == TxoutType::MULTISIG);
    std::string descriptor = "wsh(multi(" + util::ToString(static_cast<unsigned int>(solutions.front()[0]));
    for (size_t i{1}; i + 1 < solutions.size(); ++i) {
        const auto& pubkey = solutions[i];
        bool found{false};
        for (const auto& secret : row["private_keys"].getValues()) {
            const auto key = DecodeSecret(secret.get_str());
            BOOST_REQUIRE(key.IsValid());
            if (ToByteVector(key.GetPubKey()) == pubkey) {
                descriptor += "," + secret.get_str();
                found = true;
                break;
            }
        }
        BOOST_REQUIRE(found);
    }
    return descriptor + "))";
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(bip322_wallet_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(official_vectors)
{
    const auto vectors = Vectors();
    size_t rows{0}, external_proofs{0};
    for (const std::string source : {"basic", "generated"}) {
        for (const auto& row : vectors[source]["simple"].getValues()) {
            BOOST_TEST_CONTEXT(source << " simple row " << rows) {
                CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
                const auto type = row["type"].get_str();
                const auto secret = row["private_keys"][0].get_str();
                const auto descriptor = type == "p2wpkh" ? "wpkh(" + secret + ")" :
                    type == "p2tr" ? "tr(" + secret + ")" : MultisigDescriptor(row);
                const auto destination = Import(wallet, descriptor);
                BOOST_CHECK(destination == DecodeDestination(row["address"].get_str()));
                const auto message = row["message"].get_str();
                for (const auto& proof : row["bip322_signatures"].getValues()) {
                    BOOST_CHECK(BIP322Verify(destination, proof.get_str(), message) == BIP322VerificationResult::OK);
                    ++external_proofs;
                }
                const auto witness = SignProof(wallet, destination, message);
                if (type == "p2wpkh") {
                    BOOST_REQUIRE_EQUAL(witness.size(), 2);
                    BOOST_CHECK_EQUAL(witness[0].back(), SIGHASH_ALL);
                    BOOST_CHECK(witness[1] == ToByteVector(DecodeSecret(secret).GetPubKey()));
                } else if (type == "p2tr") {
                    BOOST_REQUIRE_EQUAL(witness.size(), 1);
                    BOOST_REQUIRE_EQUAL(witness[0].size(), 65);
                    BOOST_CHECK_EQUAL(witness[0].back(), SIGHASH_ALL);
                } else {
                    BOOST_REQUIRE_EQUAL(witness.size(), row["private_keys"].size() + 2);
                    BOOST_CHECK(witness.front().empty());
                    BOOST_CHECK_EQUAL(HexStr(witness.back()), row["witness_script"].get_str());
                    for (size_t i{1}; i + 1 < witness.size(); ++i) BOOST_CHECK_EQUAL(witness[i].back(), SIGHASH_ALL);
                }
                ++rows;
            }
        }
    }
    BOOST_CHECK_EQUAL(rows, 8);
    BOOST_CHECK_EQUAL(external_proofs, 10);
}

BOOST_AUTO_TEST_CASE(p2wsh_threshold)
{
    const auto vectors = Vectors();
    const auto& secrets = vectors["basic"]["simple"][2]["private_keys"];
    CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
    // The third private key is deliberately absent: exactly two of three suffice.
    const auto third_pubkey = DecodeSecret(secrets[2].get_str()).GetPubKey();
    const auto destination = Import(wallet, "wsh(multi(2," + secrets[0].get_str() + "," + secrets[1].get_str() + "," + HexStr(third_pubkey) + "))");
    const auto witness = SignProof(wallet, destination, "2-of-3 BIP322");
    BOOST_REQUIRE_EQUAL(witness.size(), 4);
    BOOST_CHECK(witness[0].empty());
    BOOST_CHECK_EQUAL(witness[1].back(), SIGHASH_ALL);
    BOOST_CHECK_EQUAL(witness[2].back(), SIGHASH_ALL);
    const auto expected_script = CScript{} << OP_2 << ToByteVector(DecodeSecret(secrets[0].get_str()).GetPubKey()) <<
        ToByteVector(DecodeSecret(secrets[1].get_str()).GetPubKey()) << ToByteVector(third_pubkey) << OP_3 << OP_CHECKMULTISIG;
    BOOST_CHECK(witness.back() == std::vector<unsigned char>(expected_script.begin(), expected_script.end()));
}

BOOST_AUTO_TEST_CASE(p2wsh_miniscript)
{
    const auto vectors = Vectors();
    const auto& secrets = vectors["generated"]["simple"][2]["private_keys"];
    CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
    const auto destination = Import(wallet, "wsh(and_v(v:pk(" + secrets[0].get_str() + "),pk(" + secrets[1].get_str() + ")))");
    const auto witness = SignProof(wallet, destination, "compound Miniscript BIP322");
    BOOST_REQUIRE_EQUAL(witness.size(), 3);
    BOOST_CHECK_EQUAL(witness[0].back(), SIGHASH_ALL);
    BOOST_CHECK_EQUAL(witness[1].back(), SIGHASH_ALL);
    const auto expected_script = CScript{} << ToByteVector(DecodeSecret(secrets[0].get_str()).GetPubKey()) << OP_CHECKSIGVERIFY <<
        ToByteVector(DecodeSecret(secrets[1].get_str()).GetPubKey()) << OP_CHECKSIG;
    BOOST_CHECK(witness.back() == std::vector<unsigned char>(expected_script.begin(), expected_script.end()));
}

BOOST_AUTO_TEST_CASE(taproot_paths)
{
    const auto vectors = Vectors();
    const auto internal_secret = vectors["basic"]["simple"][3]["private_keys"][0].get_str();
    const auto leaf_secret = vectors["generated"]["simple"][1]["private_keys"][0].get_str();
    const XOnlyPubKey leaf_pubkey{DecodeSecret(leaf_secret).GetPubKey()};
    {
        CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
        const auto destination = Import(wallet, "tr(" + internal_secret + ",pk(" + HexStr(leaf_pubkey) + "))");
        BOOST_CHECK(destination != DecodeDestination(vectors["basic"]["simple"][3]["address"].get_str()));
        const auto witness = SignProof(wallet, destination, "Taproot key path with tree");
        BOOST_REQUIRE_EQUAL(witness.size(), 1);
        BOOST_REQUIRE_EQUAL(witness[0].size(), 65);
        BOOST_CHECK_EQUAL(witness[0].back(), SIGHASH_ALL);
    }
    {
        CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
        const auto destination = Import(wallet, "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",pk(" + leaf_secret + "))");
        const auto witness = SignProof(wallet, destination, "Taproot forced script path");
        BOOST_REQUIRE_EQUAL(witness.size(), 3);
        BOOST_REQUIRE_EQUAL(witness[0].size(), 65);
        BOOST_CHECK_EQUAL(witness[0].back(), SIGHASH_ALL);
        const auto expected_script = CScript{} << ToByteVector(leaf_pubkey) << OP_CHECKSIG;
        BOOST_CHECK(witness[1] == std::vector<unsigned char>(expected_script.begin(), expected_script.end()));
        BOOST_REQUIRE_EQUAL(witness[2].size(), TAPROOT_CONTROL_BASE_SIZE);
        BOOST_CHECK(std::equal(witness[2].begin() + 1, witness[2].end(), XOnlyPubKey::NUMS_H.begin()));
    }
}

BOOST_AUTO_TEST_CASE(unknown_and_unsupported)
{
    const auto vectors = Vectors();
    CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
    for (const auto& row : vectors["basic"]["simple"].getValues()) {
        CheckFailure(wallet, DecodeDestination(row["address"].get_str()), SigningResult::PRIVATE_KEY_NOT_AVAILABLE);
    }
    const auto secret = vectors["basic"]["simple"][0]["private_keys"][0].get_str();
    for (const auto& destination : {Import(wallet, "pkh(" + secret + ")"),
                                    Import(wallet, "sh(wpkh(" + secret + "))"),
                                    CTxDestination{WitnessUnknown{2, std::vector<unsigned char>(32, 1)}},
                                    CTxDestination{CNoDestination{}}}) {
        CheckFailure(wallet, destination, SigningResult::SIGNING_FAILED);
    }
}

BOOST_AUTO_TEST_CASE(watch_only_and_missing_data)
{
    const auto vectors = Vectors();
    const auto& secrets = vectors["basic"]["simple"][2]["private_keys"];
    const auto pubkey = HexStr(DecodeSecret(secrets[0].get_str()).GetPubKey());
    {
        CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
        wallet.SetWalletFlag(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
        for (const auto& descriptor : {"wpkh(" + pubkey + ")", "wsh(pk(" + pubkey + "))", "tr(" + pubkey + ")"}) {
            const auto destination = Import(wallet, descriptor);
            BOOST_CHECK(WITH_LOCK(wallet.cs_wallet, return wallet.IsMine(destination)));
            CheckFailure(wallet, destination, SigningResult::PRIVATE_KEY_NOT_AVAILABLE);
        }
    }
    {
        CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
        const auto destination = Import(wallet, "wsh(multi(2," + secrets[0].get_str() + "," +
            HexStr(DecodeSecret(secrets[1].get_str()).GetPubKey()) + "," + HexStr(DecodeSecret(secrets[2].get_str()).GetPubKey()) + "))");
        auto txs = CreateBIP322Transactions(GetScriptForDestination(destination), "failure test");
        const std::map<COutPoint, Coin> coins{{txs.to_sign.vin[0].prevout, Coin{txs.to_spend.vout[0], 0, false}}};
        std::map<int, bilingual_str> errors;
        BOOST_CHECK(!WITH_LOCK(wallet.cs_wallet, return wallet.SignTransaction(txs.to_sign, coins, SIGHASH_ALL, errors)));
        BOOST_CHECK(!txs.to_sign.vin[0].scriptWitness.IsNull());
        CheckFailure(wallet, destination, SigningResult::PRIVATE_KEY_NOT_AVAILABLE);
    }
    for (const size_t index : {size_t{2}, size_t{3}}) {
        CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
        const auto expected = DecodeDestination(vectors["basic"]["simple"][index]["address"].get_str());
        // A raw output commitment supplies neither the witness script nor Taproot spend data.
        const auto destination = Import(wallet, "raw(" + HexStr(GetScriptForDestination(expected)) + ")");
        BOOST_CHECK(destination == expected);
        BOOST_CHECK(WITH_LOCK(wallet.cs_wallet, return wallet.IsMine(destination)));
        CheckFailure(wallet, destination, SigningResult::PRIVATE_KEY_NOT_AVAILABLE);
    }
}

BOOST_FIXTURE_TEST_CASE(encrypted_unlock, WalletTestingSetup)
{
    FakeNodeClock clock;
    m_args.ForceSetArg("-keypool", "1");
    const auto vectors = Vectors();
    const auto& secrets = vectors["basic"]["simple"][2]["private_keys"];
    const std::vector<CTxDestination> destinations{
        Import(m_wallet, "wpkh(" + secrets[0].get_str() + ")"),
        Import(m_wallet, MultisigDescriptor(vectors["basic"]["simple"][2])),
        Import(m_wallet, "tr(" + secrets[0].get_str() + ")"),
        Import(m_wallet, "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",pk(" + secrets[1].get_str() + "))")};
    BOOST_REQUIRE(m_wallet.EncryptWallet("passphrase"));
    BOOST_REQUIRE(m_wallet.IsLocked());
    for (const auto& destination : destinations) CheckFailure(m_wallet, destination, SigningResult::PRIVATE_KEY_NOT_AVAILABLE);
    BOOST_REQUIRE(m_wallet.Unlock("passphrase"));
    for (const auto& destination : destinations) SignProof(m_wallet, destination, "after unlock");
    m_wallet.Lock();
    for (const auto& destination : destinations) CheckFailure(m_wallet, destination, SigningResult::PRIVATE_KEY_NOT_AVAILABLE);
}

BOOST_AUTO_TEST_CASE(nonzero_timelocks)
{
    const auto vectors = Vectors();
    const auto secret = vectors["basic"]["simple"][0]["private_keys"][0].get_str();
    for (const std::string timelock : {"after(1)", "older(1)"}) {
        for (const bool taproot : {false, true}) {
            CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
            const std::string policy = "and_v(v:pk(" + secret + ")," + timelock + ")";
            const auto destination = Import(wallet, taproot ? "tr(" + HexStr(XOnlyPubKey::NUMS_H) + "," + policy + ")" : "wsh(" + policy + ")");
            CheckFailure(wallet, destination, SigningResult::PRIVATE_KEY_NOT_AVAILABLE);
            auto txs = CreateBIP322Transactions(GetScriptForDestination(destination), "failure test");
            // The same descriptor is satisfiable when the transaction has the required fields.
            txs.to_sign.version = 2;
            txs.to_sign.nLockTime = 1;
            txs.to_sign.vin[0].nSequence = 1;
            const std::map<COutPoint, Coin> coins{{txs.to_sign.vin[0].prevout, Coin{txs.to_spend.vout[0], 0, false}}};
            std::map<int, bilingual_str> errors;
            BOOST_REQUIRE(WITH_LOCK(wallet.cs_wallet, return wallet.SignTransaction(txs.to_sign, coins, SIGHASH_ALL, errors)));
            DataStream bytes;
            bytes << txs.to_sign.vin[0].scriptWitness.stack;
            BOOST_CHECK(BIP322Verify(destination, "smp" + EncodeBase64(bytes), "failure test") == BIP322VerificationResult::INVALID);
        }
    }
}

BOOST_AUTO_TEST_CASE(forbidden_script_commitments)
{
    for (const auto& script : {CScript{} << OP_CODESEPARATOR << OP_TRUE,
                              CScript{} << OP_0 << OP_IF << OP_CODESEPARATOR << OP_ENDIF << OP_TRUE,
                              CScript{} << OP_NOP4 << OP_TRUE}) {
        const CTxDestination destination{WitnessV0ScriptHash{script}};
        auto txs = CreateBIP322Transactions(GetScriptForDestination(destination), "failure test");
        txs.to_sign.vin[0].scriptWitness.stack = {{script.begin(), script.end()}};
        const CTransaction tx{txs.to_sign};
        PrecomputedTransactionData data;
        data.Init(tx, {txs.to_spend.vout[0]}, true);
        const TransactionSignatureChecker checker{&tx, 0, 0, data, MissingDataBehavior::FAIL};
        BOOST_REQUIRE(VerifyScript({}, txs.to_spend.vout[0].scriptPubKey, &tx.vin[0].scriptWitness,
            SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS, checker));
        DataStream bytes;
        bytes << tx.vin[0].scriptWitness.stack;
        const auto expected = script.front() == OP_NOP4 ? BIP322VerificationResult::INCONCLUSIVE : BIP322VerificationResult::INVALID;
        BOOST_CHECK(BIP322Verify(destination, "smp" + EncodeBase64(bytes), "failure test") == expected);

        // Raw commitments cannot provide these scripts to the descriptor satisfier.
        CWallet wallet{m_node.chain.get(), "", CreateMockableWalletDatabase()};
        BOOST_CHECK(Import(wallet, "raw(" + HexStr(GetScriptForDestination(destination)) + ")") == destination);
        CheckFailure(wallet, destination, SigningResult::PRIVATE_KEY_NOT_AVAILABLE);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
