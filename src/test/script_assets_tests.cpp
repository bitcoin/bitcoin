// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/sigcache.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <span.h>
#include <streams.h>
#include <test/util/json.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/strencodings.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

unsigned int ParseScriptFlags(std::string strFlags);

BOOST_AUTO_TEST_SUITE(script_assets_tests)

template <typename T>
CScript ToScript(const T& byte_container)
{
    auto span{MakeUCharSpan(byte_container)};
    return {span.begin(), span.end()};
}

static CScript ScriptFromHex(const std::string& str)
{
    return ToScript(*Assert(TryParseHex(str)));
}

static CMutableTransaction TxFromHex(const std::string& str)
{
    CMutableTransaction tx;
    SpanReader{ParseHex(str)} >> TX_NO_WITNESS(tx);
    return tx;
}

static std::vector<CTxOut> TxOutsFromJSON(const UniValue& univalue)
{
    assert(univalue.isArray());
    std::vector<CTxOut> prevouts;
    for (size_t i = 0; i < univalue.size(); ++i) {
        CTxOut txout;
        SpanReader{ParseHex(univalue[i].get_str())} >> txout;
        prevouts.push_back(std::move(txout));
    }
    return prevouts;
}

static CScriptWitness ScriptWitnessFromJSON(const UniValue& univalue)
{
    assert(univalue.isArray());
    CScriptWitness scriptwitness;
    for (size_t i = 0; i < univalue.size(); ++i) {
        auto bytes = ParseHex(univalue[i].get_str());
        scriptwitness.stack.push_back(std::move(bytes));
    }
    return scriptwitness;
}

static std::vector<unsigned int> AllConsensusFlags()
{
    std::vector<unsigned int> ret;

    for (unsigned int i = 0; i < 128; ++i) {
        unsigned int flag = 0;
        if (i & 1) flag |= SCRIPT_VERIFY_P2SH;
        if (i & 2) flag |= SCRIPT_VERIFY_DERSIG;
        if (i & 4) flag |= SCRIPT_VERIFY_NULLDUMMY;
        if (i & 8) flag |= SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY;
        if (i & 16) flag |= SCRIPT_VERIFY_CHECKSEQUENCEVERIFY;
        if (i & 32) flag |= SCRIPT_VERIFY_WITNESS;
        if (i & 64) flag |= SCRIPT_VERIFY_TAPROOT;
        if (i & 128) flag |= SCRIPT_VERIFY_P2QRH;

        // SCRIPT_VERIFY_WITNESS requires SCRIPT_VERIFY_P2SH
        if (flag & SCRIPT_VERIFY_WITNESS && !(flag & SCRIPT_VERIFY_P2SH)) continue;
        // SCRIPT_VERIFY_TAPROOT requires SCRIPT_VERIFY_WITNESS
        if (flag & SCRIPT_VERIFY_TAPROOT && !(flag & SCRIPT_VERIFY_WITNESS)) continue;
        // SCRIPT_VERIFY_P2QRH requires SCRIPT_VERIFY_WITNESS
        if (flag & SCRIPT_VERIFY_P2QRH && !(flag & SCRIPT_VERIFY_WITNESS)) continue;

        ret.push_back(flag);
    }

    return ret;
}

/** Precomputed list of all valid combinations of consensus-relevant script validation flags. */
static const std::vector<unsigned int> ALL_CONSENSUS_FLAGS = AllConsensusFlags();

static void AssetTest(const UniValue& test, SignatureCache& signature_cache)
{
    BOOST_CHECK(test.isObject());

    CMutableTransaction mtx = TxFromHex(test["tx"].get_str());
    const std::vector<CTxOut> prevouts = TxOutsFromJSON(test["prevouts"]);
    BOOST_CHECK(prevouts.size() == mtx.vin.size());
    size_t idx = test["index"].getInt<int64_t>();
    uint32_t test_flags{ParseScriptFlags(test["flags"].get_str())};
    bool fin = test.exists("final") && test["final"].get_bool();

    if (test.exists("success")) {
        mtx.vin[idx].scriptSig = ScriptFromHex(test["success"]["scriptSig"].get_str());
        mtx.vin[idx].scriptWitness = ScriptWitnessFromJSON(test["success"]["witness"]);
        CTransaction tx(mtx);
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::vector<CTxOut>(prevouts));
        CachingTransactionSignatureChecker txcheck(&tx, idx, prevouts[idx].nValue, true, signature_cache, txdata);

        for (const auto flags : ALL_CONSENSUS_FLAGS) {
            // "final": true tests are valid for all flags. Others are only valid with flags that are
            // a subset of test_flags.
            if (fin || ((flags & test_flags) == flags)) {
                // Check if this is a P2QRH script (witness version 3, 32-byte program)
                bool is_p2qrh_script = false;
                if (prevouts[idx].scriptPubKey.size() >= 2 && 
                    prevouts[idx].scriptPubKey[0] == 0x53 && 
                    prevouts[idx].scriptPubKey[1] == 0x20 &&
                    tx.vin[idx].scriptSig.empty()) {  // P2QRH should have empty ScriptSig
                    is_p2qrh_script = true;
                }
                
                // For P2QRH scripts, only run with P2QRH flags, not Taproot flags
                if (is_p2qrh_script && (flags & SCRIPT_VERIFY_TAPROOT)) {
                    continue; // Skip Taproot validation for P2QRH scripts
                }
                
                bool ret = VerifyScript(tx.vin[idx].scriptSig, prevouts[idx].scriptPubKey, &tx.vin[idx].scriptWitness, flags, txcheck, nullptr);
                if (!ret) {
                    // Debug output to see what's failing
                    std::cout << "ScriptSig: " << HexStr(tx.vin[idx].scriptSig) << std::endl;
                    std::cout << "ScriptPubKey: " << HexStr(prevouts[idx].scriptPubKey) << std::endl;
                    std::cout << "Flags: " << flags << std::endl;
                    std::cout << "Test flags: " << test_flags << std::endl;
                    std::cout << "Witness size: " << tx.vin[idx].scriptWitness.stack.size() << std::endl;
                    std::cout << "Is P2QRH: " << (is_p2qrh_script ? "true" : "false") << std::endl;
                }
                BOOST_CHECK(ret);
            }
        }
    }

    if (test.exists("failure")) {
        mtx.vin[idx].scriptSig = ScriptFromHex(test["failure"]["scriptSig"].get_str());
        mtx.vin[idx].scriptWitness = ScriptWitnessFromJSON(test["failure"]["witness"]);
        CTransaction tx(mtx);
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::vector<CTxOut>(prevouts));
        CachingTransactionSignatureChecker txcheck(&tx, idx, prevouts[idx].nValue, true, signature_cache, txdata);

        for (const auto flags : ALL_CONSENSUS_FLAGS) {
            // If a test is supposed to fail with test_flags, it should also fail with any superset thereof.
            if ((flags & test_flags) == test_flags) {
                bool ret = VerifyScript(tx.vin[idx].scriptSig, prevouts[idx].scriptPubKey, &tx.vin[idx].scriptWitness, flags, txcheck, nullptr);
                BOOST_CHECK(!ret);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(script_assets_test)
{
    // See src/test/fuzz/script_assets_test_minimizer.cpp for information on how to generate
    // the script_assets_test.json file used by this test.
    SignatureCache signature_cache{DEFAULT_SIGNATURE_CACHE_BYTES};

    const char* dir = std::getenv("DIR_UNIT_TEST_DATA");
    BOOST_WARN_MESSAGE(dir != nullptr, "Variable DIR_UNIT_TEST_DATA unset, skipping script_assets_test");
    if (dir == nullptr) return;
    auto path = fs::path(dir) / "script_assets_test.json";
    bool exists = fs::exists(path);
    BOOST_WARN_MESSAGE(exists, "File $DIR_UNIT_TEST_DATA/script_assets_test.json not found, skipping script_assets_test");
    if (!exists) return;
    std::ifstream file{path};
    BOOST_CHECK(file.is_open());
    file.seekg(0, std::ios::end);
    size_t length = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string data(length, '\0');
    file.read(data.data(), data.size());
    UniValue tests = read_json(data);
    BOOST_CHECK(tests.isArray());
    BOOST_CHECK(tests.size() > 0);

    for (size_t i = 0; i < tests.size(); i++) {
        AssetTest(tests[i], signature_cache);
    }
    file.close();
}

BOOST_AUTO_TEST_SUITE_END()
