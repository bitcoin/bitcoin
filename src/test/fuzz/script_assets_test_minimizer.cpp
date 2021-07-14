// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <primitives/transaction.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util/strencodings.h>

#include <boost/algorithm/string.hpp>
#include <cstdint>
#include <string>
#include <vector>

// This fuzz "test" can be used to minimize test cases for script_assets_test in
// src/test/script_tests.cpp. While it written as a fuzz test, and can be used as such,
// fuzzing the inputs is unlikely to construct useful test cases.
//
// Instead, it is primarily intended to be run on a test set that was generated
// externally, for example using test/functional/feature_taproot.py's --dumptests mode.
// The minimized set can then be concatenated together, surrounded by '[' and ']',
// and used as the script_assets_test.json input to the script_assets_test unit test:
//
// (normal build)
// $ mkdir dump
// $ for N in $(seq 1 10); do TEST_DUMP_DIR=dump test/functional/feature_taproot.py --dumptests; done
// $ ...
//
// (libFuzzer build)
// $ mkdir dump-min
// $ FUZZ=script_assets_test_minimizer ./src/test/fuzz/fuzz -merge=1 -use_value_profile=1 dump-min/ dump/
// $ (echo -en '[\n'; cat dump-min/* | head -c -2; echo -en '\n]') >script_assets_test.json

namespace {

std::vector<unsigned char> CheckedParseHex(const std::string& str)
{
    if (str.size() && !IsHex(str)) throw std::runtime_error("Non-hex input '" + str + "'");
    return ParseHex(str);
}

CScript ScriptFromHex(const std::string& str)
{
    std::vector<unsigned char> data = CheckedParseHex(str);
    return CScript(data.begin(), data.end());
}

CMutableTransaction TxFromHex(const std::string& str)
{
    CMutableTransaction tx;
    try {
        VectorReader(SER_DISK, SERIALIZE_TRANSACTION_NO_WITNESS, CheckedParseHex(str), 0) >> tx;
    } catch (const std::ios_base::failure&) {
        throw std::runtime_error("Tx deserialization failure");
    }
    return tx;
}

std::vector<CTxOut> TxOutsFromJSON(const UniValue& univalue)
{
    if (!univalue.isArray()) throw std::runtime_error("Prevouts must be array");
    std::vector<CTxOut> prevouts;
    for (size_t i = 0; i < univalue.size(); ++i) {
        CTxOut txout;
        try {
            VectorReader(SER_DISK, 0, CheckedParseHex(univalue[i].get_str()), 0) >> txout;
        } catch (const std::ios_base::failure&) {
            throw std::runtime_error("Prevout invalid format");
        }
        prevouts.push_back(std::move(txout));
    }
    return prevouts;
}

CScriptWitness ScriptWitnessFromJSON(const UniValue& univalue)
{
    if (!univalue.isArray()) throw std::runtime_error("Script witness is not array");
    CScriptWitness scriptwitness;
    for (size_t i = 0; i < univalue.size(); ++i) {
        auto bytes = CheckedParseHex(univalue[i].get_str());
        scriptwitness.stack.push_back(std::move(bytes));
    }
    return scriptwitness;
}

const std::map<std::string, unsigned int> FLAG_NAMES = {
    {std::string("P2SH"), (unsigned int)SCRIPT_VERIFY_P2SH},
    {std::string("DERSIG"), (unsigned int)SCRIPT_VERIFY_DERSIG},
    {std::string("NULLDUMMY"), (unsigned int)SCRIPT_VERIFY_NULLDUMMY},
    {std::string("CHECKLOCKTIMEVERIFY"), (unsigned int)SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY},
    {std::string("CHECKSEQUENCEVERIFY"), (unsigned int)SCRIPT_VERIFY_CHECKSEQUENCEVERIFY},
    {std::string("WITNESS"), (unsigned int)SCRIPT_VERIFY_WITNESS},
    {std::string("TAPROOT"), (unsigned int)SCRIPT_VERIFY_TAPROOT},
};

std::vector<unsigned int> AllFlags()
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

        // SCRIPT_VERIFY_WITNESS requires SCRIPT_VERIFY_P2SH
        if (flag & SCRIPT_VERIFY_WITNESS && !(flag & SCRIPT_VERIFY_P2SH)) continue;
        // SCRIPT_VERIFY_TAPROOT requires SCRIPT_VERIFY_WITNESS
        if (flag & SCRIPT_VERIFY_TAPROOT && !(flag & SCRIPT_VERIFY_WITNESS)) continue;

        ret.push_back(flag);
    }

    return ret;
}

const std::vector<unsigned int> ALL_FLAGS = AllFlags();

unsigned int ParseScriptFlags(const std::string& str)
{
    if (str.empty()) return 0;

    unsigned int flags = 0;
    std::vector<std::string> words;
    boost::algorithm::split(words, str, boost::algorithm::is_any_of(","));

    for (const std::string& word : words) {
        auto it = FLAG_NAMES.find(word);
        if (it == FLAG_NAMES.end()) throw std::runtime_error("Unknown verification flag " + word);
        flags |= it->second;
    }

    return flags;
}

void Test(const std::string& str)
{
    UniValue test;
    if (!test.read(str) || !test.isObject()) throw std::runtime_error("Non-object test input");

    CMutableTransaction tx = TxFromHex(test["tx"].get_str());
    const std::vector<CTxOut> prevouts = TxOutsFromJSON(test["prevouts"]);
    if (prevouts.size() != tx.vin.size()) throw std::runtime_error("Incorrect number of prevouts");
    size_t idx = test["index"].get_int64();
    if (idx >= tx.vin.size()) throw std::runtime_error("Invalid index");
    unsigned int test_flags = ParseScriptFlags(test["flags"].get_str());
    bool final = test.exists("final") && test["final"].get_bool();

    if (test.exists("success")) {
        tx.vin[idx].scriptSig = ScriptFromHex(test["success"]["scriptSig"].get_str());
        tx.vin[idx].scriptWitness = ScriptWitnessFromJSON(test["success"]["witness"]);
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::vector<CTxOut>(prevouts));
        MutableTransactionSignatureChecker txcheck(tx, idx, prevouts[idx].nValue, txdata, MissingDataBehavior::ASSERT_FAIL);
        for (const auto flags : ALL_FLAGS) {
            // "final": true tests are valid for all flags. Others are only valid with flags that are
            // a subset of test_flags.
            if (final || ((flags & test_flags) == flags)) {
                (void)VerifyScript(tx.vin[idx].scriptSig, prevouts[idx].scriptPubKey, &tx.vin[idx].scriptWitness, flags, txcheck, nullptr);
            }
        }
    }

    if (test.exists("failure")) {
        tx.vin[idx].scriptSig = ScriptFromHex(test["failure"]["scriptSig"].get_str());
        tx.vin[idx].scriptWitness = ScriptWitnessFromJSON(test["failure"]["witness"]);
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::vector<CTxOut>(prevouts));
        MutableTransactionSignatureChecker txcheck(tx, idx, prevouts[idx].nValue, txdata, MissingDataBehavior::ASSERT_FAIL);
        for (const auto flags : ALL_FLAGS) {
            // If a test is supposed to fail with test_flags, it should also fail with any superset thereof.
            if ((flags & test_flags) == test_flags) {
                (void)VerifyScript(tx.vin[idx].scriptSig, prevouts[idx].scriptPubKey, &tx.vin[idx].scriptWitness, flags, txcheck, nullptr);
            }
        }
    }
}

void test_init()
{
    static ECCVerifyHandle handle;
}

FUZZ_TARGET_INIT_HIDDEN(script_assets_test_minimizer, test_init, /* hidden */ true)
{
    if (buffer.size() < 2 || buffer.back() != '\n' || buffer[buffer.size() - 2] != ',') return;
    const std::string str((const char*)buffer.data(), buffer.size() - 2);
    try {
        Test(str);
    } catch (const std::runtime_error&) {
    }
}

} // namespace
