// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/data/script_tests.json.h>

#include <core_io.h>
#include <fs.h>
#include <key.h>
#include <rpc/util.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sigcache.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <test/util/transaction_utils.h>
#include <util/strencodings.h>
#include <util/system.h>

#if defined(HAVE_CONSENSUS_LIB)
#include <script/bitcoinconsensus.h>
#endif

#include <stdint.h>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <univalue.h>

// Uncomment if you want to output updated JSON tests.
// #define UPDATE_JSON_TESTS

static const unsigned int gFlags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC;

unsigned int ParseScriptFlags(std::string strFlags);
std::string FormatScriptFlags(unsigned int flags);

UniValue
read_json(const std::string& jsondata)
{
    UniValue v;

    if (!v.read(jsondata) || !v.isArray())
    {
        BOOST_ERROR("Parse error.");
        return UniValue(UniValue::VARR);
    }
    return v.get_array();
}

struct ScriptErrorDesc
{
    ScriptError_t err;
    const char *name;
};

static ScriptErrorDesc script_errors[]={
    {SCRIPT_ERR_OK, "OK"},
    {SCRIPT_ERR_UNKNOWN_ERROR, "UNKNOWN_ERROR"},
    {SCRIPT_ERR_EVAL_FALSE, "EVAL_FALSE"},
    {SCRIPT_ERR_OP_RETURN, "OP_RETURN"},
    {SCRIPT_ERR_SCRIPT_SIZE, "SCRIPT_SIZE"},
    {SCRIPT_ERR_PUSH_SIZE, "PUSH_SIZE"},
    {SCRIPT_ERR_OP_COUNT, "OP_COUNT"},
    {SCRIPT_ERR_STACK_SIZE, "STACK_SIZE"},
    {SCRIPT_ERR_SIG_COUNT, "SIG_COUNT"},
    {SCRIPT_ERR_PUBKEY_COUNT, "PUBKEY_COUNT"},
    {SCRIPT_ERR_VERIFY, "VERIFY"},
    {SCRIPT_ERR_EQUALVERIFY, "EQUALVERIFY"},
    {SCRIPT_ERR_CHECKMULTISIGVERIFY, "CHECKMULTISIGVERIFY"},
    {SCRIPT_ERR_CHECKSIGVERIFY, "CHECKSIGVERIFY"},
    {SCRIPT_ERR_NUMEQUALVERIFY, "NUMEQUALVERIFY"},
    {SCRIPT_ERR_BAD_OPCODE, "BAD_OPCODE"},
    {SCRIPT_ERR_DISABLED_OPCODE, "DISABLED_OPCODE"},
    {SCRIPT_ERR_INVALID_STACK_OPERATION, "INVALID_STACK_OPERATION"},
    {SCRIPT_ERR_INVALID_ALTSTACK_OPERATION, "INVALID_ALTSTACK_OPERATION"},
    {SCRIPT_ERR_UNBALANCED_CONDITIONAL, "UNBALANCED_CONDITIONAL"},
    {SCRIPT_ERR_NEGATIVE_LOCKTIME, "NEGATIVE_LOCKTIME"},
    {SCRIPT_ERR_UNSATISFIED_LOCKTIME, "UNSATISFIED_LOCKTIME"},
    {SCRIPT_ERR_SIG_HASHTYPE, "SIG_HASHTYPE"},
    {SCRIPT_ERR_SIG_DER, "SIG_DER"},
    {SCRIPT_ERR_MINIMALDATA, "MINIMALDATA"},
    {SCRIPT_ERR_SIG_PUSHONLY, "SIG_PUSHONLY"},
    {SCRIPT_ERR_SIG_HIGH_S, "SIG_HIGH_S"},
    {SCRIPT_ERR_SIG_NULLDUMMY, "SIG_NULLDUMMY"},
    {SCRIPT_ERR_PUBKEYTYPE, "PUBKEYTYPE"},
    {SCRIPT_ERR_CLEANSTACK, "CLEANSTACK"},
    {SCRIPT_ERR_MINIMALIF, "MINIMALIF"},
    {SCRIPT_ERR_SIG_NULLFAIL, "NULLFAIL"},
    {SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS, "DISCOURAGE_UPGRADABLE_NOPS"},
    {SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM, "DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM"},
    {SCRIPT_ERR_WITNESS_PROGRAM_WRONG_LENGTH, "WITNESS_PROGRAM_WRONG_LENGTH"},
    {SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY, "WITNESS_PROGRAM_WITNESS_EMPTY"},
    {SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH, "WITNESS_PROGRAM_MISMATCH"},
    {SCRIPT_ERR_WITNESS_MALLEATED, "WITNESS_MALLEATED"},
    {SCRIPT_ERR_WITNESS_MALLEATED_P2SH, "WITNESS_MALLEATED_P2SH"},
    {SCRIPT_ERR_WITNESS_UNEXPECTED, "WITNESS_UNEXPECTED"},
    {SCRIPT_ERR_WITNESS_PUBKEYTYPE, "WITNESS_PUBKEYTYPE"},
    {SCRIPT_ERR_OP_CODESEPARATOR, "OP_CODESEPARATOR"},
    {SCRIPT_ERR_SIG_FINDANDDELETE, "SIG_FINDANDDELETE"},
};

static std::string FormatScriptError(ScriptError_t err)
{
    for (const auto& se : script_errors)
        if (se.err == err)
            return se.name;
    BOOST_ERROR("Unknown scripterror enumeration value, update script_errors in script_tests.cpp.");
    return "";
}

static ScriptError_t ParseScriptError(const std::string& name)
{
    for (const auto& se : script_errors)
        if (se.name == name)
            return se.err;
    BOOST_ERROR("Unknown scripterror \"" << name << "\" in test description");
    return SCRIPT_ERR_UNKNOWN_ERROR;
}

BOOST_FIXTURE_TEST_SUITE(script_tests, BasicTestingSetup)

void DoTest(const CScript& scriptPubKey, const CScript& scriptSig, const CScriptWitness& scriptWitness, int flags, const std::string& message, int scriptError, CAmount nValue = 0)
{
    bool expect = (scriptError == SCRIPT_ERR_OK);
    if (flags & SCRIPT_VERIFY_CLEANSTACK) {
        flags |= SCRIPT_VERIFY_P2SH;
        flags |= SCRIPT_VERIFY_WITNESS;
    }
    ScriptError err;
    const CTransaction txCredit{BuildCreditingTransaction(scriptPubKey, nValue)};
    CMutableTransaction tx = BuildSpendingTransaction(scriptSig, scriptWitness, txCredit);
    CMutableTransaction tx2 = tx;
    BOOST_CHECK_MESSAGE(VerifyScript(scriptSig, scriptPubKey, &scriptWitness, flags, MutableTransactionSignatureChecker(&tx, 0, txCredit.vout[0].nValue), &err) == expect, message);
    BOOST_CHECK_MESSAGE(err == scriptError, FormatScriptError(err) + " where " + FormatScriptError((ScriptError_t)scriptError) + " expected: " + message);

    // Verify that removing flags from a passing test or adding flags to a failing test does not change the result.
    for (int i = 0; i < 16; ++i) {
        int extra_flags = InsecureRandBits(16);
        int combined_flags = expect ? (flags & ~extra_flags) : (flags | extra_flags);
        // Weed out some invalid flag combinations.
        if (combined_flags & SCRIPT_VERIFY_CLEANSTACK && ~combined_flags & (SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)) continue;
        if (combined_flags & SCRIPT_VERIFY_WITNESS && ~combined_flags & SCRIPT_VERIFY_P2SH) continue;
        BOOST_CHECK_MESSAGE(VerifyScript(scriptSig, scriptPubKey, &scriptWitness, combined_flags, MutableTransactionSignatureChecker(&tx, 0, txCredit.vout[0].nValue), &err) == expect, message + strprintf(" (with flags %x)", combined_flags));
    }

#if defined(HAVE_CONSENSUS_LIB)
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << tx2;
    int libconsensus_flags = flags & bitcoinconsensus_SCRIPT_FLAGS_VERIFY_ALL;
    if (libconsensus_flags == flags) {
        int expectedSuccessCode = expect ? 1 : 0;
        if (flags & bitcoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS) {
            BOOST_CHECK_MESSAGE(bitcoinconsensus_verify_script_with_amount(scriptPubKey.data(), scriptPubKey.size(), txCredit.vout[0].nValue, (const unsigned char*)&stream[0], stream.size(), 0, libconsensus_flags, nullptr) == expectedSuccessCode, message);
        } else {
            BOOST_CHECK_MESSAGE(bitcoinconsensus_verify_script_with_amount(scriptPubKey.data(), scriptPubKey.size(), 0, (const unsigned char*)&stream[0], stream.size(), 0, libconsensus_flags, nullptr) == expectedSuccessCode, message);
            BOOST_CHECK_MESSAGE(bitcoinconsensus_verify_script(scriptPubKey.data(), scriptPubKey.size(), (const unsigned char*)&stream[0], stream.size(), 0, libconsensus_flags, nullptr) == expectedSuccessCode, message);
        }
    }
#endif
}

void static NegateSignatureS(std::vector<unsigned char>& vchSig) {
    // Parse the signature.
    std::vector<unsigned char> r, s;
    r = std::vector<unsigned char>(vchSig.begin() + 4, vchSig.begin() + 4 + vchSig[3]);
    s = std::vector<unsigned char>(vchSig.begin() + 6 + vchSig[3], vchSig.begin() + 6 + vchSig[3] + vchSig[5 + vchSig[3]]);

    // Really ugly to implement mod-n negation here, but it would be feature creep to expose such functionality from libsecp256k1.
    static const unsigned char order[33] = {
        0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
        0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
        0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41
    };
    while (s.size() < 33) {
        s.insert(s.begin(), 0x00);
    }
    int carry = 0;
    for (int p = 32; p >= 1; p--) {
        int n = (int)order[p] - s[p] - carry;
        s[p] = (n + 256) & 0xFF;
        carry = (n < 0);
    }
    assert(carry == 0);
    if (s.size() > 1 && s[0] == 0 && s[1] < 0x80) {
        s.erase(s.begin());
    }

    // Reconstruct the signature.
    vchSig.clear();
    vchSig.push_back(0x30);
    vchSig.push_back(4 + r.size() + s.size());
    vchSig.push_back(0x02);
    vchSig.push_back(r.size());
    vchSig.insert(vchSig.end(), r.begin(), r.end());
    vchSig.push_back(0x02);
    vchSig.push_back(s.size());
    vchSig.insert(vchSig.end(), s.begin(), s.end());
}

namespace
{
const unsigned char vchKey0[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
const unsigned char vchKey1[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0};
const unsigned char vchKey2[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0};

struct KeyData
{
    CKey key0, key0C, key1, key1C, key2, key2C;
    CPubKey pubkey0, pubkey0C, pubkey0H;
    CPubKey pubkey1, pubkey1C;
    CPubKey pubkey2, pubkey2C;

    KeyData()
    {
        key0.Set(vchKey0, vchKey0 + 32, false);
        key0C.Set(vchKey0, vchKey0 + 32, true);
        pubkey0 = key0.GetPubKey();
        pubkey0H = key0.GetPubKey();
        pubkey0C = key0C.GetPubKey();
        *const_cast<unsigned char*>(&pubkey0H[0]) = 0x06 | (pubkey0H[64] & 1);

        key1.Set(vchKey1, vchKey1 + 32, false);
        key1C.Set(vchKey1, vchKey1 + 32, true);
        pubkey1 = key1.GetPubKey();
        pubkey1C = key1C.GetPubKey();

        key2.Set(vchKey2, vchKey2 + 32, false);
        key2C.Set(vchKey2, vchKey2 + 32, true);
        pubkey2 = key2.GetPubKey();
        pubkey2C = key2C.GetPubKey();
    }
};

enum class WitnessMode {
    NONE,
    PKH,
    SH
};

class TestBuilder
{
private:
    //! Actually executed script
    CScript script;
    //! The P2SH redeemscript
    CScript redeemscript;
    //! The Witness embedded script
    CScript witscript;
    CScriptWitness scriptWitness;
    CTransactionRef creditTx;
    CMutableTransaction spendTx;
    bool havePush;
    std::vector<unsigned char> push;
    std::string comment;
    int flags;
    int scriptError;
    CAmount nValue;

    void DoPush()
    {
        if (havePush) {
            spendTx.vin[0].scriptSig << push;
            havePush = false;
        }
    }

    void DoPush(const std::vector<unsigned char>& data)
    {
        DoPush();
        push = data;
        havePush = true;
    }

public:
    TestBuilder(const CScript& script_, const std::string& comment_, int flags_, bool P2SH = false, WitnessMode wm = WitnessMode::NONE, int witnessversion = 0, CAmount nValue_ = 0) : script(script_), havePush(false), comment(comment_), flags(flags_), scriptError(SCRIPT_ERR_OK), nValue(nValue_)
    {
        CScript scriptPubKey = script;
        if (wm == WitnessMode::PKH) {
            uint160 hash;
            CHash160().Write(MakeSpan(script).subspan(1)).Finalize(hash);
            script = CScript() << OP_DUP << OP_HASH160 << ToByteVector(hash) << OP_EQUALVERIFY << OP_CHECKSIG;
            scriptPubKey = CScript() << witnessversion << ToByteVector(hash);
        } else if (wm == WitnessMode::SH) {
            witscript = scriptPubKey;
            uint256 hash;
            CSHA256().Write(&witscript[0], witscript.size()).Finalize(hash.begin());
            scriptPubKey = CScript() << witnessversion << ToByteVector(hash);
        }
        if (P2SH) {
            redeemscript = scriptPubKey;
            scriptPubKey = CScript() << OP_HASH160 << ToByteVector(CScriptID(redeemscript)) << OP_EQUAL;
        }
        creditTx = MakeTransactionRef(BuildCreditingTransaction(scriptPubKey, nValue));
        spendTx = BuildSpendingTransaction(CScript(), CScriptWitness(), *creditTx);
    }

    TestBuilder& ScriptError(ScriptError_t err)
    {
        scriptError = err;
        return *this;
    }

    TestBuilder& Opcode(const opcodetype& _op)
    {
        DoPush();
        spendTx.vin[0].scriptSig << _op;
        return *this;
    }

    TestBuilder& Num(int num)
    {
        DoPush();
        spendTx.vin[0].scriptSig << num;
        return *this;
    }

    TestBuilder& Push(const std::string& hex)
    {
        DoPush(ParseHex(hex));
        return *this;
    }

    TestBuilder& Push(const CScript& _script)
    {
        DoPush(std::vector<unsigned char>(_script.begin(), _script.end()));
        return *this;
    }

    TestBuilder& PushSig(const CKey& key, int nHashType = SIGHASH_ALL, unsigned int lenR = 32, unsigned int lenS = 32, SigVersion sigversion = SigVersion::BASE, CAmount amount = 0)
    {
        uint256 hash = SignatureHash(script, spendTx, 0, nHashType, amount, sigversion);
        std::vector<unsigned char> vchSig, r, s;
        uint32_t iter = 0;
        do {
            key.Sign(hash, vchSig, false, iter++);
            if ((lenS == 33) != (vchSig[5 + vchSig[3]] == 33)) {
                NegateSignatureS(vchSig);
            }
            r = std::vector<unsigned char>(vchSig.begin() + 4, vchSig.begin() + 4 + vchSig[3]);
            s = std::vector<unsigned char>(vchSig.begin() + 6 + vchSig[3], vchSig.begin() + 6 + vchSig[3] + vchSig[5 + vchSig[3]]);
        } while (lenR != r.size() || lenS != s.size());
        vchSig.push_back(static_cast<unsigned char>(nHashType));
        DoPush(vchSig);
        return *this;
    }

    TestBuilder& PushWitSig(const CKey& key, CAmount amount = -1, int nHashType = SIGHASH_ALL, unsigned int lenR = 32, unsigned int lenS = 32, SigVersion sigversion = SigVersion::WITNESS_V0)
    {
        if (amount == -1)
            amount = nValue;
        return PushSig(key, nHashType, lenR, lenS, sigversion, amount).AsWit();
    }

    TestBuilder& Push(const CPubKey& pubkey)
    {
        DoPush(std::vector<unsigned char>(pubkey.begin(), pubkey.end()));
        return *this;
    }

    TestBuilder& PushRedeem()
    {
        DoPush(std::vector<unsigned char>(redeemscript.begin(), redeemscript.end()));
        return *this;
    }

    TestBuilder& PushWitRedeem()
    {
        DoPush(std::vector<unsigned char>(witscript.begin(), witscript.end()));
        return AsWit();
    }

    TestBuilder& EditPush(unsigned int pos, const std::string& hexin, const std::string& hexout)
    {
        assert(havePush);
        std::vector<unsigned char> datain = ParseHex(hexin);
        std::vector<unsigned char> dataout = ParseHex(hexout);
        assert(pos + datain.size() <= push.size());
        BOOST_CHECK_MESSAGE(std::vector<unsigned char>(push.begin() + pos, push.begin() + pos + datain.size()) == datain, comment);
        push.erase(push.begin() + pos, push.begin() + pos + datain.size());
        push.insert(push.begin() + pos, dataout.begin(), dataout.end());
        return *this;
    }

    TestBuilder& DamagePush(unsigned int pos)
    {
        assert(havePush);
        assert(pos < push.size());
        push[pos] ^= 1;
        return *this;
    }

    TestBuilder& Test()
    {
        TestBuilder copy = *this; // Make a copy so we can rollback the push.
        DoPush();
        DoTest(creditTx->vout[0].scriptPubKey, spendTx.vin[0].scriptSig, scriptWitness, flags, comment, scriptError, nValue);
        *this = copy;
        return *this;
    }

    TestBuilder& AsWit()
    {
        assert(havePush);
        scriptWitness.stack.push_back(push);
        havePush = false;
        return *this;
    }

    UniValue GetJSON()
    {
        DoPush();
        UniValue array(UniValue::VARR);
        if (!scriptWitness.stack.empty()) {
            UniValue wit(UniValue::VARR);
            for (unsigned i = 0; i < scriptWitness.stack.size(); i++) {
                wit.push_back(HexStr(scriptWitness.stack[i]));
            }
            wit.push_back(ValueFromAmount(nValue));
            array.push_back(wit);
        }
        array.push_back(FormatScript(spendTx.vin[0].scriptSig));
        array.push_back(FormatScript(creditTx->vout[0].scriptPubKey));
        array.push_back(FormatScriptFlags(flags));
        array.push_back(FormatScriptError((ScriptError_t)scriptError));
        array.push_back(comment);
        return array;
    }

    std::string GetComment() const
    {
        return comment;
    }
};

std::string JSONPrettyPrint(const UniValue& univalue)
{
    std::string ret = univalue.write(4);
    // Workaround for libunivalue pretty printer, which puts a space between commas and newlines
    size_t pos = 0;
    while ((pos = ret.find(" \n", pos)) != std::string::npos) {
        ret.replace(pos, 2, "\n");
        pos++;
    }
    return ret;
}
} // namespace

BOOST_AUTO_TEST_CASE(script_build)
{
    const KeyData keys;

    std::vector<TestBuilder> tests;

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2PK", 0
                               ).PushSig(keys.key0));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2PK, bad sig", 0
                               ).PushSig(keys.key0).DamagePush(10).ScriptError(SCRIPT_ERR_EVAL_FALSE));

    tests.push_back(TestBuilder(CScript() << OP_DUP << OP_HASH160 << ToByteVector(keys.pubkey1C.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG,
                                "P2PKH", 0
                               ).PushSig(keys.key1).Push(keys.pubkey1C));
    tests.push_back(TestBuilder(CScript() << OP_DUP << OP_HASH160 << ToByteVector(keys.pubkey2C.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG,
                                "P2PKH, bad pubkey", 0
                               ).PushSig(keys.key2).Push(keys.pubkey2C).DamagePush(5).ScriptError(SCRIPT_ERR_EQUALVERIFY));

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG,
                                "P2PK anyonecanpay", 0
                               ).PushSig(keys.key1, SIGHASH_ALL | SIGHASH_ANYONECANPAY));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG,
                                "P2PK anyonecanpay marked with normal hashtype", 0
                               ).PushSig(keys.key1, SIGHASH_ALL | SIGHASH_ANYONECANPAY).EditPush(70, "81", "01").ScriptError(SCRIPT_ERR_EVAL_FALSE));

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0C) << OP_CHECKSIG,
                                "P2SH(P2PK)", SCRIPT_VERIFY_P2SH, true
                               ).PushSig(keys.key0).PushRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0C) << OP_CHECKSIG,
                                "P2SH(P2PK), bad redeemscript", SCRIPT_VERIFY_P2SH, true
                               ).PushSig(keys.key0).PushRedeem().DamagePush(10).ScriptError(SCRIPT_ERR_EVAL_FALSE));

    tests.push_back(TestBuilder(CScript() << OP_DUP << OP_HASH160 << ToByteVector(keys.pubkey0.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG,
                                "P2SH(P2PKH)", SCRIPT_VERIFY_P2SH, true
                               ).PushSig(keys.key0).Push(keys.pubkey0).PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_DUP << OP_HASH160 << ToByteVector(keys.pubkey1.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG,
                                "P2SH(P2PKH), bad sig but no VERIFY_P2SH", 0, true
                               ).PushSig(keys.key0).DamagePush(10).PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_DUP << OP_HASH160 << ToByteVector(keys.pubkey1.GetID()) << OP_EQUALVERIFY << OP_CHECKSIG,
                                "P2SH(P2PKH), bad sig", SCRIPT_VERIFY_P2SH, true
                               ).PushSig(keys.key0).DamagePush(10).PushRedeem().ScriptError(SCRIPT_ERR_EQUALVERIFY));

    tests.push_back(TestBuilder(CScript() << OP_3 << ToByteVector(keys.pubkey0C) << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_3 << OP_CHECKMULTISIG,
                                "3-of-3", 0
                               ).Num(0).PushSig(keys.key0).PushSig(keys.key1).PushSig(keys.key2));
    tests.push_back(TestBuilder(CScript() << OP_3 << ToByteVector(keys.pubkey0C) << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_3 << OP_CHECKMULTISIG,
                                "3-of-3, 2 sigs", 0
                               ).Num(0).PushSig(keys.key0).PushSig(keys.key1).Num(0).ScriptError(SCRIPT_ERR_EVAL_FALSE));

    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey0C) << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_3 << OP_CHECKMULTISIG,
                                "P2SH(2-of-3)", SCRIPT_VERIFY_P2SH, true
                               ).Num(0).PushSig(keys.key1).PushSig(keys.key2).PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey0C) << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_3 << OP_CHECKMULTISIG,
                                "P2SH(2-of-3), 1 sig", SCRIPT_VERIFY_P2SH, true
                               ).Num(0).PushSig(keys.key1).Num(0).PushRedeem().ScriptError(SCRIPT_ERR_EVAL_FALSE));

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "P2PK with too much R padding but no DERSIG", 0
                               ).PushSig(keys.key1, SIGHASH_ALL, 31, 32).EditPush(1, "43021F", "44022000"));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "P2PK with too much R padding", SCRIPT_VERIFY_DERSIG
                               ).PushSig(keys.key1, SIGHASH_ALL, 31, 32).EditPush(1, "43021F", "44022000").ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "P2PK with too much S padding but no DERSIG", 0
                               ).PushSig(keys.key1, SIGHASH_ALL).EditPush(1, "44", "45").EditPush(37, "20", "2100"));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "P2PK with too much S padding", SCRIPT_VERIFY_DERSIG
                               ).PushSig(keys.key1, SIGHASH_ALL).EditPush(1, "44", "45").EditPush(37, "20", "2100").ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "P2PK with too little R padding but no DERSIG", 0
                               ).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220"));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "P2PK with too little R padding", SCRIPT_VERIFY_DERSIG
                               ).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with bad sig with too much R padding but no DERSIG", 0
                               ).PushSig(keys.key2, SIGHASH_ALL, 31, 32).EditPush(1, "43021F", "44022000").DamagePush(10));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with bad sig with too much R padding", SCRIPT_VERIFY_DERSIG
                               ).PushSig(keys.key2, SIGHASH_ALL, 31, 32).EditPush(1, "43021F", "44022000").DamagePush(10).ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with too much R padding but no DERSIG", 0
                               ).PushSig(keys.key2, SIGHASH_ALL, 31, 32).EditPush(1, "43021F", "44022000").ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with too much R padding", SCRIPT_VERIFY_DERSIG
                               ).PushSig(keys.key2, SIGHASH_ALL, 31, 32).EditPush(1, "43021F", "44022000").ScriptError(SCRIPT_ERR_SIG_DER));

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "BIP66 example 1, without DERSIG", 0
                               ).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220"));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "BIP66 example 1, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG << OP_NOT,
                                "BIP66 example 2, without DERSIG", 0
                               ).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG << OP_NOT,
                                "BIP66 example 2, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "BIP66 example 3, without DERSIG", 0
                               ).Num(0).ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "BIP66 example 3, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(0).ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG << OP_NOT,
                                "BIP66 example 4, without DERSIG", 0
                               ).Num(0));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG << OP_NOT,
                                "BIP66 example 4, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(0));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "BIP66 example 5, without DERSIG", 0
                               ).Num(1).ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG,
                                "BIP66 example 5, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(1).ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG << OP_NOT,
                                "BIP66 example 6, without DERSIG", 0
                               ).Num(1));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1C) << OP_CHECKSIG << OP_NOT,
                                "BIP66 example 6, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(1).ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG,
                                "BIP66 example 7, without DERSIG", 0
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").PushSig(keys.key2));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG,
                                "BIP66 example 7, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").PushSig(keys.key2).ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG << OP_NOT,
                                "BIP66 example 8, without DERSIG", 0
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").PushSig(keys.key2).ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG << OP_NOT,
                                "BIP66 example 8, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").PushSig(keys.key2).ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG,
                                "BIP66 example 9, without DERSIG", 0
                               ).Num(0).Num(0).PushSig(keys.key2, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG,
                                "BIP66 example 9, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(0).Num(0).PushSig(keys.key2, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG << OP_NOT,
                                "BIP66 example 10, without DERSIG", 0
                               ).Num(0).Num(0).PushSig(keys.key2, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220"));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG << OP_NOT,
                                "BIP66 example 10, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(0).Num(0).PushSig(keys.key2, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").ScriptError(SCRIPT_ERR_SIG_DER));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG,
                                "BIP66 example 11, without DERSIG", 0
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").Num(0).ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG,
                                "BIP66 example 11, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").Num(0).ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG << OP_NOT,
                                "BIP66 example 12, without DERSIG", 0
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").Num(0));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_2 << OP_CHECKMULTISIG << OP_NOT,
                                "BIP66 example 12, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL, 33, 32).EditPush(1, "45022100", "440220").Num(0));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG,
                                "P2PK with multi-byte hashtype, without DERSIG", 0
                               ).PushSig(keys.key2, SIGHASH_ALL).EditPush(70, "01", "0101"));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG,
                                "P2PK with multi-byte hashtype, with DERSIG", SCRIPT_VERIFY_DERSIG
                               ).PushSig(keys.key2, SIGHASH_ALL).EditPush(70, "01", "0101").ScriptError(SCRIPT_ERR_SIG_DER));

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG,
                                "P2PK with high S but no LOW_S", 0
                               ).PushSig(keys.key2, SIGHASH_ALL, 32, 33));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG,
                                "P2PK with high S", SCRIPT_VERIFY_LOW_S
                               ).PushSig(keys.key2, SIGHASH_ALL, 32, 33).ScriptError(SCRIPT_ERR_SIG_HIGH_S));

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0H) << OP_CHECKSIG,
                                "P2PK with hybrid pubkey but no STRICTENC", 0
                               ).PushSig(keys.key0, SIGHASH_ALL));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0H) << OP_CHECKSIG,
                                "P2PK with hybrid pubkey", SCRIPT_VERIFY_STRICTENC
                               ).PushSig(keys.key0, SIGHASH_ALL).ScriptError(SCRIPT_ERR_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0H) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with hybrid pubkey but no STRICTENC", 0
                               ).PushSig(keys.key0, SIGHASH_ALL).ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0H) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with hybrid pubkey", SCRIPT_VERIFY_STRICTENC
                               ).PushSig(keys.key0, SIGHASH_ALL).ScriptError(SCRIPT_ERR_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0H) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with invalid hybrid pubkey but no STRICTENC", 0
                               ).PushSig(keys.key0, SIGHASH_ALL).DamagePush(10));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0H) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with invalid hybrid pubkey", SCRIPT_VERIFY_STRICTENC
                               ).PushSig(keys.key0, SIGHASH_ALL).DamagePush(10).ScriptError(SCRIPT_ERR_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey0H) << ToByteVector(keys.pubkey1C) << OP_2 << OP_CHECKMULTISIG,
                                "1-of-2 with the second 1 hybrid pubkey and no STRICTENC", 0
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL));
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey0H) << ToByteVector(keys.pubkey1C) << OP_2 << OP_CHECKMULTISIG,
                                "1-of-2 with the second 1 hybrid pubkey", SCRIPT_VERIFY_STRICTENC
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL));
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0H) << OP_2 << OP_CHECKMULTISIG,
                                "1-of-2 with the first 1 hybrid pubkey", SCRIPT_VERIFY_STRICTENC
                               ).Num(0).PushSig(keys.key1, SIGHASH_ALL).ScriptError(SCRIPT_ERR_PUBKEYTYPE));

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG,
                                "P2PK with undefined hashtype but no STRICTENC", 0
                               ).PushSig(keys.key1, 5));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG,
                                "P2PK with undefined hashtype", SCRIPT_VERIFY_STRICTENC
                               ).PushSig(keys.key1, 5).ScriptError(SCRIPT_ERR_SIG_HASHTYPE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with invalid sig and undefined hashtype but no STRICTENC", 0
                               ).PushSig(keys.key1, 5).DamagePush(10));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG << OP_NOT,
                                "P2PK NOT with invalid sig and undefined hashtype", SCRIPT_VERIFY_STRICTENC
                               ).PushSig(keys.key1, 5).DamagePush(10).ScriptError(SCRIPT_ERR_SIG_HASHTYPE));

    tests.push_back(TestBuilder(CScript() << OP_3 << ToByteVector(keys.pubkey0C) << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_3 << OP_CHECKMULTISIG,
                                "3-of-3 with nonzero dummy but no NULLDUMMY", 0
                               ).Num(1).PushSig(keys.key0).PushSig(keys.key1).PushSig(keys.key2));
    tests.push_back(TestBuilder(CScript() << OP_3 << ToByteVector(keys.pubkey0C) << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_3 << OP_CHECKMULTISIG,
                                "3-of-3 with nonzero dummy", SCRIPT_VERIFY_NULLDUMMY
                               ).Num(1).PushSig(keys.key0).PushSig(keys.key1).PushSig(keys.key2).ScriptError(SCRIPT_ERR_SIG_NULLDUMMY));
    tests.push_back(TestBuilder(CScript() << OP_3 << ToByteVector(keys.pubkey0C) << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_3 << OP_CHECKMULTISIG << OP_NOT,
                                "3-of-3 NOT with invalid sig and nonzero dummy but no NULLDUMMY", 0
                               ).Num(1).PushSig(keys.key0).PushSig(keys.key1).PushSig(keys.key2).DamagePush(10));
    tests.push_back(TestBuilder(CScript() << OP_3 << ToByteVector(keys.pubkey0C) << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey2C) << OP_3 << OP_CHECKMULTISIG << OP_NOT,
                                "3-of-3 NOT with invalid sig with nonzero dummy", SCRIPT_VERIFY_NULLDUMMY
                               ).Num(1).PushSig(keys.key0).PushSig(keys.key1).PushSig(keys.key2).DamagePush(10).ScriptError(SCRIPT_ERR_SIG_NULLDUMMY));

    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey1C) << OP_2 << OP_CHECKMULTISIG,
                                "2-of-2 with two identical keys and sigs pushed using OP_DUP but no SIGPUSHONLY", 0
                               ).Num(0).PushSig(keys.key1).Opcode(OP_DUP));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey1C) << OP_2 << OP_CHECKMULTISIG,
                                "2-of-2 with two identical keys and sigs pushed using OP_DUP", SCRIPT_VERIFY_SIGPUSHONLY
                               ).Num(0).PushSig(keys.key1).Opcode(OP_DUP).ScriptError(SCRIPT_ERR_SIG_PUSHONLY));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG,
                                "P2SH(P2PK) with non-push scriptSig but no P2SH or SIGPUSHONLY", 0, true
                               ).PushSig(keys.key2).Opcode(OP_NOP8).PushRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG,
                                "P2PK with non-push scriptSig but with P2SH validation", 0
                               ).PushSig(keys.key2).Opcode(OP_NOP8));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG,
                                "P2SH(P2PK) with non-push scriptSig but no SIGPUSHONLY", SCRIPT_VERIFY_P2SH, true
                               ).PushSig(keys.key2).Opcode(OP_NOP8).PushRedeem().ScriptError(SCRIPT_ERR_SIG_PUSHONLY));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey2C) << OP_CHECKSIG,
                                "P2SH(P2PK) with non-push scriptSig but not P2SH", SCRIPT_VERIFY_SIGPUSHONLY, true
                               ).PushSig(keys.key2).Opcode(OP_NOP8).PushRedeem().ScriptError(SCRIPT_ERR_SIG_PUSHONLY));
    tests.push_back(TestBuilder(CScript() << OP_2 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey1C) << OP_2 << OP_CHECKMULTISIG,
                                "2-of-2 with two identical keys and sigs pushed", SCRIPT_VERIFY_SIGPUSHONLY
                               ).Num(0).PushSig(keys.key1).PushSig(keys.key1));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2PK with unnecessary input but no CLEANSTACK", SCRIPT_VERIFY_P2SH
                               ).Num(11).PushSig(keys.key0));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2PK with unnecessary input", SCRIPT_VERIFY_CLEANSTACK | SCRIPT_VERIFY_P2SH
                               ).Num(11).PushSig(keys.key0).ScriptError(SCRIPT_ERR_CLEANSTACK));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2SH with unnecessary input but no CLEANSTACK", SCRIPT_VERIFY_P2SH, true
                               ).Num(11).PushSig(keys.key0).PushRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2SH with unnecessary input", SCRIPT_VERIFY_CLEANSTACK | SCRIPT_VERIFY_P2SH, true
                               ).Num(11).PushSig(keys.key0).PushRedeem().ScriptError(SCRIPT_ERR_CLEANSTACK));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2SH with CLEANSTACK", SCRIPT_VERIFY_CLEANSTACK | SCRIPT_VERIFY_P2SH, true
                               ).PushSig(keys.key0).PushRedeem());

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "Basic P2WSH", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH,
                                0, 1).PushWitSig(keys.key0).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "Basic P2WPKH", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::PKH,
                                0, 1).PushWitSig(keys.key0).Push(keys.pubkey0).AsWit());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "Basic P2SH(P2WSH)", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::SH,
                                0, 1).PushWitSig(keys.key0).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "Basic P2SH(P2WPKH)", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::PKH,
                                0, 1).PushWitSig(keys.key0).Push(keys.pubkey0).AsWit().PushRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG,
                                "Basic P2WSH with the wrong key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH
                               ).PushWitSig(keys.key0).PushWitRedeem().ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1),
                                "Basic P2WPKH with the wrong key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::PKH
                               ).PushWitSig(keys.key0).Push(keys.pubkey1).AsWit().ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG,
                                "Basic P2SH(P2WSH) with the wrong key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::SH
                               ).PushWitSig(keys.key0).PushWitRedeem().PushRedeem().ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1),
                                "Basic P2SH(P2WPKH) with the wrong key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::PKH
                               ).PushWitSig(keys.key0).Push(keys.pubkey1).AsWit().PushRedeem().ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG,
                                "Basic P2WSH with the wrong key but no WITNESS", SCRIPT_VERIFY_P2SH, false, WitnessMode::SH
                               ).PushWitSig(keys.key0).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1),
                                "Basic P2WPKH with the wrong key but no WITNESS", SCRIPT_VERIFY_P2SH, false, WitnessMode::PKH
                               ).PushWitSig(keys.key0).Push(keys.pubkey1).AsWit());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1) << OP_CHECKSIG,
                                "Basic P2SH(P2WSH) with the wrong key but no WITNESS", SCRIPT_VERIFY_P2SH, true, WitnessMode::SH
                               ).PushWitSig(keys.key0).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1),
                                "Basic P2SH(P2WPKH) with the wrong key but no WITNESS", SCRIPT_VERIFY_P2SH, true, WitnessMode::PKH
                               ).PushWitSig(keys.key0).Push(keys.pubkey1).AsWit().PushRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "Basic P2WSH with wrong value", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH,
                                0, 0).PushWitSig(keys.key0, 1).PushWitRedeem().ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "Basic P2WPKH with wrong value", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::PKH,
                                0, 0).PushWitSig(keys.key0, 1).Push(keys.pubkey0).AsWit().ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "Basic P2SH(P2WSH) with wrong value", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::SH,
                                0, 0).PushWitSig(keys.key0, 1).PushWitRedeem().PushRedeem().ScriptError(SCRIPT_ERR_EVAL_FALSE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "Basic P2SH(P2WPKH) with wrong value", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::PKH,
                                0, 0).PushWitSig(keys.key0, 1).Push(keys.pubkey0).AsWit().PushRedeem().ScriptError(SCRIPT_ERR_EVAL_FALSE));

    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "P2WPKH with future witness version", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH |
                                SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM, false, WitnessMode::PKH, 1
                               ).PushWitSig(keys.key0).Push(keys.pubkey0).AsWit().ScriptError(SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM));
    {
        CScript witscript = CScript() << ToByteVector(keys.pubkey0);
        uint256 hash;
        CSHA256().Write(&witscript[0], witscript.size()).Finalize(hash.begin());
        std::vector<unsigned char> hashBytes = ToByteVector(hash);
        hashBytes.pop_back();
        tests.push_back(TestBuilder(CScript() << OP_0 << hashBytes,
                                    "P2WPKH with wrong witness program length", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false
                                   ).PushWitSig(keys.key0).Push(keys.pubkey0).AsWit().ScriptError(SCRIPT_ERR_WITNESS_PROGRAM_WRONG_LENGTH));
    }
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2WSH with empty witness", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH
                               ).ScriptError(SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY));
    {
        CScript witscript = CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG;
        tests.push_back(TestBuilder(witscript,
                                    "P2WSH with witness program mismatch", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH
                                   ).PushWitSig(keys.key0).Push(witscript).DamagePush(0).AsWit().ScriptError(SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH));
    }
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "P2WPKH with witness program mismatch", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::PKH
                               ).PushWitSig(keys.key0).Push(keys.pubkey0).AsWit().Push("0").AsWit().ScriptError(SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "P2WPKH with non-empty scriptSig", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::PKH
                               ).PushWitSig(keys.key0).Push(keys.pubkey0).AsWit().Num(11).ScriptError(SCRIPT_ERR_WITNESS_MALLEATED));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey1),
                                "P2SH(P2WPKH) with superfluous push in scriptSig", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::PKH
                               ).PushWitSig(keys.key0).Push(keys.pubkey1).AsWit().Num(11).PushRedeem().ScriptError(SCRIPT_ERR_WITNESS_MALLEATED_P2SH));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "P2PK with witness", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH
                               ).PushSig(keys.key0).Push("0").AsWit().ScriptError(SCRIPT_ERR_WITNESS_UNEXPECTED));

    // Compressed keys should pass SCRIPT_VERIFY_WITNESS_PUBKEYTYPE
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0C) << OP_CHECKSIG,
                                "Basic P2WSH with compressed key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::SH,
                                0, 1).PushWitSig(keys.key0C).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0C),
                                "Basic P2WPKH with compressed key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::PKH,
                                0, 1).PushWitSig(keys.key0C).Push(keys.pubkey0C).AsWit());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0C) << OP_CHECKSIG,
                                "Basic P2SH(P2WSH) with compressed key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::SH,
                                0, 1).PushWitSig(keys.key0C).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0C),
                                "Basic P2SH(P2WPKH) with compressed key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::PKH,
                                0, 1).PushWitSig(keys.key0C).Push(keys.pubkey0C).AsWit().PushRedeem());

    // Testing uncompressed key in witness with SCRIPT_VERIFY_WITNESS_PUBKEYTYPE
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "Basic P2WSH", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::SH,
                                0, 1).PushWitSig(keys.key0).PushWitRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "Basic P2WPKH", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::PKH,
                                0, 1).PushWitSig(keys.key0).Push(keys.pubkey0).AsWit().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0) << OP_CHECKSIG,
                                "Basic P2SH(P2WSH)", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::SH,
                                0, 1).PushWitSig(keys.key0).PushWitRedeem().PushRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << ToByteVector(keys.pubkey0),
                                "Basic P2SH(P2WPKH)", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::PKH,
                                0, 1).PushWitSig(keys.key0).Push(keys.pubkey0).AsWit().PushRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));

    // P2WSH 1-of-2 multisig with compressed keys
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with compressed keys", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0C).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG with compressed keys", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0C).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with compressed keys", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1C).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG with compressed keys", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1C).PushWitRedeem().PushRedeem());

    // P2WSH 1-of-2 multisig with first key uncompressed
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with first key uncompressed and signing with the first key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG first key uncompressed and signing with the first key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with first key uncompressed and signing with the first key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0).PushWitRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG with first key uncompressed and signing with the first key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0).PushWitRedeem().PushRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with first key uncompressed and signing with the second key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1C).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG with first key uncompressed and signing with the second key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1C).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with first key uncompressed and signing with the second key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1C).PushWitRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1C) << ToByteVector(keys.pubkey0) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG with first key uncompressed and signing with the second key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1C).PushWitRedeem().PushRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));
    // P2WSH 1-of-2 multisig with second key uncompressed
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with second key uncompressed and signing with the first key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0C).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG second key uncompressed and signing with the first key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0C).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with second key uncompressed and signing with the first key should pass as the uncompressed key is not used", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0C).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG with second key uncompressed and signing with the first key should pass as the uncompressed key is not used", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key0C).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with second key uncompressed and signing with the second key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1).PushWitRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG with second key uncompressed and signing with the second key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1).PushWitRedeem().PushRedeem());
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2WSH CHECKMULTISIG with second key uncompressed and signing with the second key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, false, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1).PushWitRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));
    tests.push_back(TestBuilder(CScript() << OP_1 << ToByteVector(keys.pubkey1) << ToByteVector(keys.pubkey0C) << OP_2 << OP_CHECKMULTISIG,
                                "P2SH(P2WSH) CHECKMULTISIG with second key uncompressed and signing with the second key", SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS_PUBKEYTYPE, true, WitnessMode::SH,
                                0, 1).Push(CScript()).AsWit().PushWitSig(keys.key1).PushWitRedeem().PushRedeem().ScriptError(SCRIPT_ERR_WITNESS_PUBKEYTYPE));

    std::set<std::string> tests_set;

    {
        UniValue json_tests = read_json(std::string(json_tests::script_tests, json_tests::script_tests + sizeof(json_tests::script_tests)));

        for (unsigned int idx = 0; idx < json_tests.size(); idx++) {
            const UniValue& tv = json_tests[idx];
            tests_set.insert(JSONPrettyPrint(tv.get_array()));
        }
    }

#ifdef UPDATE_JSON_TESTS
    std::string strGen;
#endif
    for (TestBuilder& test : tests) {
        test.Test();
        std::string str = JSONPrettyPrint(test.GetJSON());
#ifdef UPDATE_JSON_TESTS
        strGen += str + ",\n";
#else
        if (tests_set.count(str) == 0) {
            BOOST_CHECK_MESSAGE(false, "Missing auto script_valid test: " + test.GetComment());
        }
#endif
    }

#ifdef UPDATE_JSON_TESTS
    FILE* file = fopen("script_tests.json.gen", "w");
    fputs(strGen.c_str(), file);
    fclose(file);
#endif
}

BOOST_AUTO_TEST_CASE(script_json_test)
{
    // Read tests from test/data/script_tests.json
    // Format is an array of arrays
    // Inner arrays are [ ["wit"..., nValue]?, "scriptSig", "scriptPubKey", "flags", "expected_scripterror" ]
    // ... where scriptSig and scriptPubKey are stringified
    // scripts.
    // If a witness is given, then the last value in the array should be the
    // amount (nValue) to use in the crediting tx
    UniValue tests = read_json(std::string(json_tests::script_tests, json_tests::script_tests + sizeof(json_tests::script_tests)));

    for (unsigned int idx = 0; idx < tests.size(); idx++) {
        UniValue test = tests[idx];
        std::string strTest = test.write();
        CScriptWitness witness;
        CAmount nValue = 0;
        unsigned int pos = 0;
        if (test.size() > 0 && test[pos].isArray()) {
            unsigned int i=0;
            for (i = 0; i < test[pos].size()-1; i++) {
                witness.stack.push_back(ParseHex(test[pos][i].get_str()));
            }
            nValue = AmountFromValue(test[pos][i]);
            pos++;
        }
        if (test.size() < 4 + pos) // Allow size > 3; extra stuff ignored (useful for comments)
        {
            if (test.size() != 1) {
                BOOST_ERROR("Bad test: " << strTest);
            }
            continue;
        }
        std::string scriptSigString = test[pos++].get_str();
        CScript scriptSig = ParseScript(scriptSigString);
        std::string scriptPubKeyString = test[pos++].get_str();
        CScript scriptPubKey = ParseScript(scriptPubKeyString);
        unsigned int scriptflags = ParseScriptFlags(test[pos++].get_str());
        int scriptError = ParseScriptError(test[pos++].get_str());

        DoTest(scriptPubKey, scriptSig, witness, scriptflags, strTest, scriptError, nValue);
    }
}

BOOST_AUTO_TEST_CASE(script_PushData)
{
    // Check that PUSHDATA1, PUSHDATA2, and PUSHDATA4 create the same value on
    // the stack as the 1-75 opcodes do.
    static const unsigned char direct[] = { 1, 0x5a };
    static const unsigned char pushdata1[] = { OP_PUSHDATA1, 1, 0x5a };
    static const unsigned char pushdata2[] = { OP_PUSHDATA2, 1, 0, 0x5a };
    static const unsigned char pushdata4[] = { OP_PUSHDATA4, 1, 0, 0, 0, 0x5a };

    ScriptError err;
    std::vector<std::vector<unsigned char> > directStack;
    BOOST_CHECK(EvalScript(directStack, CScript(direct, direct + sizeof(direct)), SCRIPT_VERIFY_P2SH, BaseSignatureChecker(), SigVersion::BASE, &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    std::vector<std::vector<unsigned char> > pushdata1Stack;
    BOOST_CHECK(EvalScript(pushdata1Stack, CScript(pushdata1, pushdata1 + sizeof(pushdata1)), SCRIPT_VERIFY_P2SH, BaseSignatureChecker(), SigVersion::BASE, &err));
    BOOST_CHECK(pushdata1Stack == directStack);
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    std::vector<std::vector<unsigned char> > pushdata2Stack;
    BOOST_CHECK(EvalScript(pushdata2Stack, CScript(pushdata2, pushdata2 + sizeof(pushdata2)), SCRIPT_VERIFY_P2SH, BaseSignatureChecker(), SigVersion::BASE, &err));
    BOOST_CHECK(pushdata2Stack == directStack);
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    std::vector<std::vector<unsigned char> > pushdata4Stack;
    BOOST_CHECK(EvalScript(pushdata4Stack, CScript(pushdata4, pushdata4 + sizeof(pushdata4)), SCRIPT_VERIFY_P2SH, BaseSignatureChecker(), SigVersion::BASE, &err));
    BOOST_CHECK(pushdata4Stack == directStack);
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    const std::vector<unsigned char> pushdata1_trunc{OP_PUSHDATA1, 1};
    const std::vector<unsigned char> pushdata2_trunc{OP_PUSHDATA2, 1, 0};
    const std::vector<unsigned char> pushdata4_trunc{OP_PUSHDATA4, 1, 0, 0, 0};

    std::vector<std::vector<unsigned char>> stack_ignore;
    BOOST_CHECK(!EvalScript(stack_ignore, CScript(pushdata1_trunc.begin(), pushdata1_trunc.end()), SCRIPT_VERIFY_P2SH, BaseSignatureChecker(), SigVersion::BASE, &err));
    BOOST_CHECK_EQUAL(err, SCRIPT_ERR_BAD_OPCODE);
    BOOST_CHECK(!EvalScript(stack_ignore, CScript(pushdata2_trunc.begin(), pushdata2_trunc.end()), SCRIPT_VERIFY_P2SH, BaseSignatureChecker(), SigVersion::BASE, &err));
    BOOST_CHECK_EQUAL(err, SCRIPT_ERR_BAD_OPCODE);
    BOOST_CHECK(!EvalScript(stack_ignore, CScript(pushdata4_trunc.begin(), pushdata4_trunc.end()), SCRIPT_VERIFY_P2SH, BaseSignatureChecker(), SigVersion::BASE, &err));
    BOOST_CHECK_EQUAL(err, SCRIPT_ERR_BAD_OPCODE);
}

BOOST_AUTO_TEST_CASE(script_cltv_truncated)
{
    const auto script_cltv_trunc = CScript() << OP_CHECKLOCKTIMEVERIFY;

    std::vector<std::vector<unsigned char>> stack_ignore;
    ScriptError err;
    BOOST_CHECK(!EvalScript(stack_ignore, script_cltv_trunc, SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY, BaseSignatureChecker(), SigVersion::BASE, &err));
    BOOST_CHECK_EQUAL(err, SCRIPT_ERR_INVALID_STACK_OPERATION);
}

static CScript
sign_multisig(const CScript& scriptPubKey, const std::vector<CKey>& keys, const CTransaction& transaction)
{
    uint256 hash = SignatureHash(scriptPubKey, transaction, 0, SIGHASH_ALL, 0, SigVersion::BASE);

    CScript result;
    //
    // NOTE: CHECKMULTISIG has an unfortunate bug; it requires
    // one extra item on the stack, before the signatures.
    // Putting OP_0 on the stack is the workaround;
    // fixing the bug would mean splitting the block chain (old
    // clients would not accept new CHECKMULTISIG transactions,
    // and vice-versa)
    //
    result << OP_0;
    for (const CKey &key : keys)
    {
        std::vector<unsigned char> vchSig;
        BOOST_CHECK(key.Sign(hash, vchSig));
        vchSig.push_back((unsigned char)SIGHASH_ALL);
        result << vchSig;
    }
    return result;
}
static CScript
sign_multisig(const CScript& scriptPubKey, const CKey& key, const CTransaction& transaction)
{
    std::vector<CKey> keys;
    keys.push_back(key);
    return sign_multisig(scriptPubKey, keys, transaction);
}

BOOST_AUTO_TEST_CASE(script_CHECKMULTISIG12)
{
    ScriptError err;
    CKey key1, key2, key3;
    key1.MakeNewKey(true);
    key2.MakeNewKey(false);
    key3.MakeNewKey(true);

    CScript scriptPubKey12;
    scriptPubKey12 << OP_1 << ToByteVector(key1.GetPubKey()) << ToByteVector(key2.GetPubKey()) << OP_2 << OP_CHECKMULTISIG;

    const CTransaction txFrom12{BuildCreditingTransaction(scriptPubKey12)};
    CMutableTransaction txTo12 = BuildSpendingTransaction(CScript(), CScriptWitness(), txFrom12);

    CScript goodsig1 = sign_multisig(scriptPubKey12, key1, CTransaction(txTo12));
    BOOST_CHECK(VerifyScript(goodsig1, scriptPubKey12, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo12, 0, txFrom12.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
    txTo12.vout[0].nValue = 2;
    BOOST_CHECK(!VerifyScript(goodsig1, scriptPubKey12, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo12, 0, txFrom12.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));

    CScript goodsig2 = sign_multisig(scriptPubKey12, key2, CTransaction(txTo12));
    BOOST_CHECK(VerifyScript(goodsig2, scriptPubKey12, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo12, 0, txFrom12.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    CScript badsig1 = sign_multisig(scriptPubKey12, key3, CTransaction(txTo12));
    BOOST_CHECK(!VerifyScript(badsig1, scriptPubKey12, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo12, 0, txFrom12.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));
}

BOOST_AUTO_TEST_CASE(script_CHECKMULTISIG23)
{
    ScriptError err;
    CKey key1, key2, key3, key4;
    key1.MakeNewKey(true);
    key2.MakeNewKey(false);
    key3.MakeNewKey(true);
    key4.MakeNewKey(false);

    CScript scriptPubKey23;
    scriptPubKey23 << OP_2 << ToByteVector(key1.GetPubKey()) << ToByteVector(key2.GetPubKey()) << ToByteVector(key3.GetPubKey()) << OP_3 << OP_CHECKMULTISIG;

    const CTransaction txFrom23{BuildCreditingTransaction(scriptPubKey23)};
    CMutableTransaction txTo23 = BuildSpendingTransaction(CScript(), CScriptWitness(), txFrom23);

    std::vector<CKey> keys;
    keys.push_back(key1); keys.push_back(key2);
    CScript goodsig1 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(VerifyScript(goodsig1, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    keys.clear();
    keys.push_back(key1); keys.push_back(key3);
    CScript goodsig2 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(VerifyScript(goodsig2, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    keys.clear();
    keys.push_back(key2); keys.push_back(key3);
    CScript goodsig3 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(VerifyScript(goodsig3, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));

    keys.clear();
    keys.push_back(key2); keys.push_back(key2); // Can't re-use sig
    CScript badsig1 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(!VerifyScript(badsig1, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));

    keys.clear();
    keys.push_back(key2); keys.push_back(key1); // sigs must be in correct order
    CScript badsig2 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(!VerifyScript(badsig2, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));

    keys.clear();
    keys.push_back(key3); keys.push_back(key2); // sigs must be in correct order
    CScript badsig3 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(!VerifyScript(badsig3, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));

    keys.clear();
    keys.push_back(key4); keys.push_back(key2); // sigs must match pubkeys
    CScript badsig4 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(!VerifyScript(badsig4, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));

    keys.clear();
    keys.push_back(key1); keys.push_back(key4); // sigs must match pubkeys
    CScript badsig5 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(!VerifyScript(badsig5, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_EVAL_FALSE, ScriptErrorString(err));

    keys.clear(); // Must have signatures
    CScript badsig6 = sign_multisig(scriptPubKey23, keys, CTransaction(txTo23));
    BOOST_CHECK(!VerifyScript(badsig6, scriptPubKey23, nullptr, gFlags, MutableTransactionSignatureChecker(&txTo23, 0, txFrom23.vout[0].nValue), &err));
    BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_INVALID_STACK_OPERATION, ScriptErrorString(err));
}

/* Wrapper around ProduceSignature to combine two scriptsigs */
SignatureData CombineSignatures(const CTxOut& txout, const CMutableTransaction& tx, const SignatureData& scriptSig1, const SignatureData& scriptSig2)
{
    SignatureData data;
    data.MergeSignatureData(scriptSig1);
    data.MergeSignatureData(scriptSig2);
    ProduceSignature(DUMMY_SIGNING_PROVIDER, MutableTransactionSignatureCreator(&tx, 0, txout.nValue), txout.scriptPubKey, data);
    return data;
}

BOOST_AUTO_TEST_CASE(script_combineSigs)
{
    // Test the ProduceSignature's ability to combine signatures function
    FillableSigningProvider keystore;
    std::vector<CKey> keys;
    std::vector<CPubKey> pubkeys;
    for (int i = 0; i < 3; i++)
    {
        CKey key;
        key.MakeNewKey(i%2 == 1);
        keys.push_back(key);
        pubkeys.push_back(key.GetPubKey());
        BOOST_CHECK(keystore.AddKey(key));
    }

    CMutableTransaction txFrom = BuildCreditingTransaction(GetScriptForDestination(PKHash(keys[0].GetPubKey())));
    CMutableTransaction txTo = BuildSpendingTransaction(CScript(), CScriptWitness(), CTransaction(txFrom));
    CScript& scriptPubKey = txFrom.vout[0].scriptPubKey;
    SignatureData scriptSig;

    SignatureData empty;
    SignatureData combined = CombineSignatures(txFrom.vout[0], txTo, empty, empty);
    BOOST_CHECK(combined.scriptSig.empty());

    // Single signature case:
    BOOST_CHECK(SignSignature(keystore, CTransaction(txFrom), txTo, 0, SIGHASH_ALL)); // changes scriptSig
    scriptSig = DataFromTransaction(txTo, 0, txFrom.vout[0]);
    combined = CombineSignatures(txFrom.vout[0], txTo, scriptSig, empty);
    BOOST_CHECK(combined.scriptSig == scriptSig.scriptSig);
    combined = CombineSignatures(txFrom.vout[0], txTo, empty, scriptSig);
    BOOST_CHECK(combined.scriptSig == scriptSig.scriptSig);
    SignatureData scriptSigCopy = scriptSig;
    // Signing again will give a different, valid signature:
    BOOST_CHECK(SignSignature(keystore, CTransaction(txFrom), txTo, 0, SIGHASH_ALL));
    scriptSig = DataFromTransaction(txTo, 0, txFrom.vout[0]);
    combined = CombineSignatures(txFrom.vout[0], txTo, scriptSigCopy, scriptSig);
    BOOST_CHECK(combined.scriptSig == scriptSigCopy.scriptSig || combined.scriptSig == scriptSig.scriptSig);

    // P2SH, single-signature case:
    CScript pkSingle; pkSingle << ToByteVector(keys[0].GetPubKey()) << OP_CHECKSIG;
    BOOST_CHECK(keystore.AddCScript(pkSingle));
    scriptPubKey = GetScriptForDestination(ScriptHash(pkSingle));
    BOOST_CHECK(SignSignature(keystore, CTransaction(txFrom), txTo, 0, SIGHASH_ALL));
    scriptSig = DataFromTransaction(txTo, 0, txFrom.vout[0]);
    combined = CombineSignatures(txFrom.vout[0], txTo, scriptSig, empty);
    BOOST_CHECK(combined.scriptSig == scriptSig.scriptSig);
    combined = CombineSignatures(txFrom.vout[0], txTo, empty, scriptSig);
    BOOST_CHECK(combined.scriptSig == scriptSig.scriptSig);
    scriptSigCopy = scriptSig;
    BOOST_CHECK(SignSignature(keystore, CTransaction(txFrom), txTo, 0, SIGHASH_ALL));
    scriptSig = DataFromTransaction(txTo, 0, txFrom.vout[0]);
    combined = CombineSignatures(txFrom.vout[0], txTo, scriptSigCopy, scriptSig);
    BOOST_CHECK(combined.scriptSig == scriptSigCopy.scriptSig || combined.scriptSig == scriptSig.scriptSig);

    // Hardest case:  Multisig 2-of-3
    scriptPubKey = GetScriptForMultisig(2, pubkeys);
    BOOST_CHECK(keystore.AddCScript(scriptPubKey));
    BOOST_CHECK(SignSignature(keystore, CTransaction(txFrom), txTo, 0, SIGHASH_ALL));
    scriptSig = DataFromTransaction(txTo, 0, txFrom.vout[0]);
    combined = CombineSignatures(txFrom.vout[0], txTo, scriptSig, empty);
    BOOST_CHECK(combined.scriptSig == scriptSig.scriptSig);
    combined = CombineSignatures(txFrom.vout[0], txTo, empty, scriptSig);
    BOOST_CHECK(combined.scriptSig == scriptSig.scriptSig);

    // A couple of partially-signed versions:
    std::vector<unsigned char> sig1;
    uint256 hash1 = SignatureHash(scriptPubKey, txTo, 0, SIGHASH_ALL, 0, SigVersion::BASE);
    BOOST_CHECK(keys[0].Sign(hash1, sig1));
    sig1.push_back(SIGHASH_ALL);
    std::vector<unsigned char> sig2;
    uint256 hash2 = SignatureHash(scriptPubKey, txTo, 0, SIGHASH_NONE, 0, SigVersion::BASE);
    BOOST_CHECK(keys[1].Sign(hash2, sig2));
    sig2.push_back(SIGHASH_NONE);
    std::vector<unsigned char> sig3;
    uint256 hash3 = SignatureHash(scriptPubKey, txTo, 0, SIGHASH_SINGLE, 0, SigVersion::BASE);
    BOOST_CHECK(keys[2].Sign(hash3, sig3));
    sig3.push_back(SIGHASH_SINGLE);

    // Not fussy about order (or even existence) of placeholders or signatures:
    CScript partial1a = CScript() << OP_0 << sig1 << OP_0;
    CScript partial1b = CScript() << OP_0 << OP_0 << sig1;
    CScript partial2a = CScript() << OP_0 << sig2;
    CScript partial2b = CScript() << sig2 << OP_0;
    CScript partial3a = CScript() << sig3;
    CScript partial3b = CScript() << OP_0 << OP_0 << sig3;
    CScript partial3c = CScript() << OP_0 << sig3 << OP_0;
    CScript complete12 = CScript() << OP_0 << sig1 << sig2;
    CScript complete13 = CScript() << OP_0 << sig1 << sig3;
    CScript complete23 = CScript() << OP_0 << sig2 << sig3;
    SignatureData partial1_sigs;
    partial1_sigs.signatures.emplace(keys[0].GetPubKey().GetID(), SigPair(keys[0].GetPubKey(), sig1));
    SignatureData partial2_sigs;
    partial2_sigs.signatures.emplace(keys[1].GetPubKey().GetID(), SigPair(keys[1].GetPubKey(), sig2));
    SignatureData partial3_sigs;
    partial3_sigs.signatures.emplace(keys[2].GetPubKey().GetID(), SigPair(keys[2].GetPubKey(), sig3));

    combined = CombineSignatures(txFrom.vout[0], txTo, partial1_sigs, partial1_sigs);
    BOOST_CHECK(combined.scriptSig == partial1a);
    combined = CombineSignatures(txFrom.vout[0], txTo, partial1_sigs, partial2_sigs);
    BOOST_CHECK(combined.scriptSig == complete12);
    combined = CombineSignatures(txFrom.vout[0], txTo, partial2_sigs, partial1_sigs);
    BOOST_CHECK(combined.scriptSig == complete12);
    combined = CombineSignatures(txFrom.vout[0], txTo, partial1_sigs, partial2_sigs);
    BOOST_CHECK(combined.scriptSig == complete12);
    combined = CombineSignatures(txFrom.vout[0], txTo, partial3_sigs, partial1_sigs);
    BOOST_CHECK(combined.scriptSig == complete13);
    combined = CombineSignatures(txFrom.vout[0], txTo, partial2_sigs, partial3_sigs);
    BOOST_CHECK(combined.scriptSig == complete23);
    combined = CombineSignatures(txFrom.vout[0], txTo, partial3_sigs, partial2_sigs);
    BOOST_CHECK(combined.scriptSig == complete23);
    combined = CombineSignatures(txFrom.vout[0], txTo, partial3_sigs, partial3_sigs);
    BOOST_CHECK(combined.scriptSig == partial3c);
}

BOOST_AUTO_TEST_CASE(script_standard_push)
{
    ScriptError err;
    for (int i=0; i<67000; i++) {
        CScript script;
        script << i;
        BOOST_CHECK_MESSAGE(script.IsPushOnly(), "Number " << i << " is not pure push.");
        BOOST_CHECK_MESSAGE(VerifyScript(script, CScript() << OP_1, nullptr, SCRIPT_VERIFY_MINIMALDATA, BaseSignatureChecker(), &err), "Number " << i << " push is not minimal data.");
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
    }

    for (unsigned int i=0; i<=MAX_SCRIPT_ELEMENT_SIZE; i++) {
        std::vector<unsigned char> data(i, '\111');
        CScript script;
        script << data;
        BOOST_CHECK_MESSAGE(script.IsPushOnly(), "Length " << i << " is not pure push.");
        BOOST_CHECK_MESSAGE(VerifyScript(script, CScript() << OP_1, nullptr, SCRIPT_VERIFY_MINIMALDATA, BaseSignatureChecker(), &err), "Length " << i << " push is not minimal data.");
        BOOST_CHECK_MESSAGE(err == SCRIPT_ERR_OK, ScriptErrorString(err));
    }
}

BOOST_AUTO_TEST_CASE(script_IsPushOnly_on_invalid_scripts)
{
    // IsPushOnly returns false when given a script containing only pushes that
    // are invalid due to truncation. IsPushOnly() is consensus critical
    // because P2SH evaluation uses it, although this specific behavior should
    // not be consensus critical as the P2SH evaluation would fail first due to
    // the invalid push. Still, it doesn't hurt to test it explicitly.
    static const unsigned char direct[] = { 1 };
    BOOST_CHECK(!CScript(direct, direct+sizeof(direct)).IsPushOnly());
}

BOOST_AUTO_TEST_CASE(script_GetScriptAsm)
{
    BOOST_CHECK_EQUAL("OP_CHECKLOCKTIMEVERIFY", ScriptToAsmStr(CScript() << OP_NOP2, true));
    BOOST_CHECK_EQUAL("OP_CHECKLOCKTIMEVERIFY", ScriptToAsmStr(CScript() << OP_CHECKLOCKTIMEVERIFY, true));
    BOOST_CHECK_EQUAL("OP_CHECKLOCKTIMEVERIFY", ScriptToAsmStr(CScript() << OP_NOP2));
    BOOST_CHECK_EQUAL("OP_CHECKLOCKTIMEVERIFY", ScriptToAsmStr(CScript() << OP_CHECKLOCKTIMEVERIFY));

    std::string derSig("304502207fa7a6d1e0ee81132a269ad84e68d695483745cde8b541e3bf630749894e342a022100c1f7ab20e13e22fb95281a870f3dcf38d782e53023ee313d741ad0cfbc0c5090");
    std::string pubKey("03b0da749730dc9b4b1f4a14d6902877a92541f5368778853d9c4a0cb7802dcfb2");
    std::vector<unsigned char> vchPubKey = ToByteVector(ParseHex(pubKey));

    BOOST_CHECK_EQUAL(derSig + "00 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "00")) << vchPubKey, true));
    BOOST_CHECK_EQUAL(derSig + "80 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "80")) << vchPubKey, true));
    BOOST_CHECK_EQUAL(derSig + "[ALL] " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "01")) << vchPubKey, true));
    BOOST_CHECK_EQUAL(derSig + "[NONE] " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "02")) << vchPubKey, true));
    BOOST_CHECK_EQUAL(derSig + "[SINGLE] " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "03")) << vchPubKey, true));
    BOOST_CHECK_EQUAL(derSig + "[ALL|ANYONECANPAY] " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "81")) << vchPubKey, true));
    BOOST_CHECK_EQUAL(derSig + "[NONE|ANYONECANPAY] " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "82")) << vchPubKey, true));
    BOOST_CHECK_EQUAL(derSig + "[SINGLE|ANYONECANPAY] " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "83")) << vchPubKey, true));

    BOOST_CHECK_EQUAL(derSig + "00 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "00")) << vchPubKey));
    BOOST_CHECK_EQUAL(derSig + "80 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "80")) << vchPubKey));
    BOOST_CHECK_EQUAL(derSig + "01 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "01")) << vchPubKey));
    BOOST_CHECK_EQUAL(derSig + "02 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "02")) << vchPubKey));
    BOOST_CHECK_EQUAL(derSig + "03 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "03")) << vchPubKey));
    BOOST_CHECK_EQUAL(derSig + "81 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "81")) << vchPubKey));
    BOOST_CHECK_EQUAL(derSig + "82 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "82")) << vchPubKey));
    BOOST_CHECK_EQUAL(derSig + "83 " + pubKey, ScriptToAsmStr(CScript() << ToByteVector(ParseHex(derSig + "83")) << vchPubKey));
}

static CScript ScriptFromHex(const std::string& str)
{
    std::vector<unsigned char> data = ParseHex(str);
    return CScript(data.begin(), data.end());
}

BOOST_AUTO_TEST_CASE(script_FindAndDelete)
{
    // Exercise the FindAndDelete functionality
    CScript s;
    CScript d;
    CScript expect;

    s = CScript() << OP_1 << OP_2;
    d = CScript(); // delete nothing should be a no-op
    expect = s;
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 0);
    BOOST_CHECK(s == expect);

    s = CScript() << OP_1 << OP_2 << OP_3;
    d = CScript() << OP_2;
    expect = CScript() << OP_1 << OP_3;
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 1);
    BOOST_CHECK(s == expect);

    s = CScript() << OP_3 << OP_1 << OP_3 << OP_3 << OP_4 << OP_3;
    d = CScript() << OP_3;
    expect = CScript() << OP_1 << OP_4;
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 4);
    BOOST_CHECK(s == expect);

    s = ScriptFromHex("0302ff03"); // PUSH 0x02ff03 onto stack
    d = ScriptFromHex("0302ff03");
    expect = CScript();
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 1);
    BOOST_CHECK(s == expect);

    s = ScriptFromHex("0302ff030302ff03"); // PUSH 0x2ff03 PUSH 0x2ff03
    d = ScriptFromHex("0302ff03");
    expect = CScript();
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 2);
    BOOST_CHECK(s == expect);

    s = ScriptFromHex("0302ff030302ff03");
    d = ScriptFromHex("02");
    expect = s; // FindAndDelete matches entire opcodes
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 0);
    BOOST_CHECK(s == expect);

    s = ScriptFromHex("0302ff030302ff03");
    d = ScriptFromHex("ff");
    expect = s;
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 0);
    BOOST_CHECK(s == expect);

    // This is an odd edge case: strip of the push-three-bytes
    // prefix, leaving 02ff03 which is push-two-bytes:
    s = ScriptFromHex("0302ff030302ff03");
    d = ScriptFromHex("03");
    expect = CScript() << ParseHex("ff03") << ParseHex("ff03");
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 2);
    BOOST_CHECK(s == expect);

    // Byte sequence that spans multiple opcodes:
    s = ScriptFromHex("02feed5169"); // PUSH(0xfeed) OP_1 OP_VERIFY
    d = ScriptFromHex("feed51");
    expect = s;
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 0); // doesn't match 'inside' opcodes
    BOOST_CHECK(s == expect);

    s = ScriptFromHex("02feed5169"); // PUSH(0xfeed) OP_1 OP_VERIFY
    d = ScriptFromHex("02feed51");
    expect = ScriptFromHex("69");
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 1);
    BOOST_CHECK(s == expect);

    s = ScriptFromHex("516902feed5169");
    d = ScriptFromHex("feed51");
    expect = s;
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 0);
    BOOST_CHECK(s == expect);

    s = ScriptFromHex("516902feed5169");
    d = ScriptFromHex("02feed51");
    expect = ScriptFromHex("516969");
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 1);
    BOOST_CHECK(s == expect);

    s = CScript() << OP_0 << OP_0 << OP_1 << OP_1;
    d = CScript() << OP_0 << OP_1;
    expect = CScript() << OP_0 << OP_1; // FindAndDelete is single-pass
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 1);
    BOOST_CHECK(s == expect);

    s = CScript() << OP_0 << OP_0 << OP_1 << OP_0 << OP_1 << OP_1;
    d = CScript() << OP_0 << OP_1;
    expect = CScript() << OP_0 << OP_1; // FindAndDelete is single-pass
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 2);
    BOOST_CHECK(s == expect);

    // Another weird edge case:
    // End with invalid push (not enough data)...
    s = ScriptFromHex("0003feed");
    d = ScriptFromHex("03feed"); // ... can remove the invalid push
    expect = ScriptFromHex("00");
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 1);
    BOOST_CHECK(s == expect);

    s = ScriptFromHex("0003feed");
    d = ScriptFromHex("00");
    expect = ScriptFromHex("03feed");
    BOOST_CHECK_EQUAL(FindAndDelete(s, d), 1);
    BOOST_CHECK(s == expect);
}

BOOST_AUTO_TEST_CASE(script_HasValidOps)
{
    // Exercise the HasValidOps functionality
    CScript script;
    script = ScriptFromHex("76a9141234567890abcdefa1a2a3a4a5a6a7a8a9a0aaab88ac"); // Normal script
    BOOST_CHECK(script.HasValidOps());
    script = ScriptFromHex("76a914ff34567890abcdefa1a2a3a4a5a6a7a8a9a0aaab88ac");
    BOOST_CHECK(script.HasValidOps());
    script = ScriptFromHex("ff88ac"); // Script with OP_INVALIDOPCODE explicit
    BOOST_CHECK(!script.HasValidOps());
    script = ScriptFromHex("88acc0"); // Script with undefined opcode
    BOOST_CHECK(!script.HasValidOps());
}

static CMutableTransaction TxFromHex(const std::string& str)
{
    CMutableTransaction tx;
    VectorReader(SER_DISK, SERIALIZE_TRANSACTION_NO_WITNESS, ParseHex(str), 0) >> tx;
    return tx;
}

static std::vector<CTxOut> TxOutsFromJSON(const UniValue& univalue)
{
    assert(univalue.isArray());
    std::vector<CTxOut> prevouts;
    for (size_t i = 0; i < univalue.size(); ++i) {
        CTxOut txout;
        VectorReader(SER_DISK, 0, ParseHex(univalue[i].get_str()), 0) >> txout;
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

#if defined(HAVE_CONSENSUS_LIB)

/* Test simple (successful) usage of bitcoinconsensus_verify_script */
BOOST_AUTO_TEST_CASE(bitcoinconsensus_verify_script_returns_true)
{
    unsigned int libconsensus_flags = 0;
    int nIn = 0;

    CScript scriptPubKey;
    CScript scriptSig;
    CScriptWitness wit;

    scriptPubKey << OP_1;
    CTransaction creditTx = BuildCreditingTransaction(scriptPubKey, 1);
    CTransaction spendTx = BuildSpendingTransaction(scriptSig, wit, creditTx);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << spendTx;

    bitcoinconsensus_error err;
    int result = bitcoinconsensus_verify_script(scriptPubKey.data(), scriptPubKey.size(), (const unsigned char*)&stream[0], stream.size(), nIn, libconsensus_flags, &err);
    BOOST_CHECK_EQUAL(result, 1);
    BOOST_CHECK_EQUAL(err, bitcoinconsensus_ERR_OK);
}

/* Test bitcoinconsensus_verify_script returns invalid tx index err*/
BOOST_AUTO_TEST_CASE(bitcoinconsensus_verify_script_tx_index_err)
{
    unsigned int libconsensus_flags = 0;
    int nIn = 3;

    CScript scriptPubKey;
    CScript scriptSig;
    CScriptWitness wit;

    scriptPubKey << OP_EQUAL;
    CTransaction creditTx = BuildCreditingTransaction(scriptPubKey, 1);
    CTransaction spendTx = BuildSpendingTransaction(scriptSig, wit, creditTx);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << spendTx;

    bitcoinconsensus_error err;
    int result = bitcoinconsensus_verify_script(scriptPubKey.data(), scriptPubKey.size(), (const unsigned char*)&stream[0], stream.size(), nIn, libconsensus_flags, &err);
    BOOST_CHECK_EQUAL(result, 0);
    BOOST_CHECK_EQUAL(err, bitcoinconsensus_ERR_TX_INDEX);
}

/* Test bitcoinconsensus_verify_script returns tx size mismatch err*/
BOOST_AUTO_TEST_CASE(bitcoinconsensus_verify_script_tx_size)
{
    unsigned int libconsensus_flags = 0;
    int nIn = 0;

    CScript scriptPubKey;
    CScript scriptSig;
    CScriptWitness wit;

    scriptPubKey << OP_EQUAL;
    CTransaction creditTx = BuildCreditingTransaction(scriptPubKey, 1);
    CTransaction spendTx = BuildSpendingTransaction(scriptSig, wit, creditTx);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << spendTx;

    bitcoinconsensus_error err;
    int result = bitcoinconsensus_verify_script(scriptPubKey.data(), scriptPubKey.size(), (const unsigned char*)&stream[0], stream.size() * 2, nIn, libconsensus_flags, &err);
    BOOST_CHECK_EQUAL(result, 0);
    BOOST_CHECK_EQUAL(err, bitcoinconsensus_ERR_TX_SIZE_MISMATCH);
}

/* Test bitcoinconsensus_verify_script returns invalid tx serialization error */
BOOST_AUTO_TEST_CASE(bitcoinconsensus_verify_script_tx_serialization)
{
    unsigned int libconsensus_flags = 0;
    int nIn = 0;

    CScript scriptPubKey;
    CScript scriptSig;
    CScriptWitness wit;

    scriptPubKey << OP_EQUAL;
    CTransaction creditTx = BuildCreditingTransaction(scriptPubKey, 1);
    CTransaction spendTx = BuildSpendingTransaction(scriptSig, wit, creditTx);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << 0xffffffff;

    bitcoinconsensus_error err;
    int result = bitcoinconsensus_verify_script(scriptPubKey.data(), scriptPubKey.size(), (const unsigned char*)&stream[0], stream.size(), nIn, libconsensus_flags, &err);
    BOOST_CHECK_EQUAL(result, 0);
    BOOST_CHECK_EQUAL(err, bitcoinconsensus_ERR_TX_DESERIALIZE);
}

/* Test bitcoinconsensus_verify_script returns amount required error */
BOOST_AUTO_TEST_CASE(bitcoinconsensus_verify_script_amount_required_err)
{
    unsigned int libconsensus_flags = bitcoinconsensus_SCRIPT_FLAGS_VERIFY_WITNESS;
    int nIn = 0;

    CScript scriptPubKey;
    CScript scriptSig;
    CScriptWitness wit;

    scriptPubKey << OP_EQUAL;
    CTransaction creditTx = BuildCreditingTransaction(scriptPubKey, 1);
    CTransaction spendTx = BuildSpendingTransaction(scriptSig, wit, creditTx);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << spendTx;

    bitcoinconsensus_error err;
    int result = bitcoinconsensus_verify_script(scriptPubKey.data(), scriptPubKey.size(), (const unsigned char*)&stream[0], stream.size(), nIn, libconsensus_flags, &err);
    BOOST_CHECK_EQUAL(result, 0);
    BOOST_CHECK_EQUAL(err, bitcoinconsensus_ERR_AMOUNT_REQUIRED);
}

/* Test bitcoinconsensus_verify_script returns invalid flags err */
BOOST_AUTO_TEST_CASE(bitcoinconsensus_verify_script_invalid_flags)
{
    unsigned int libconsensus_flags = 1 << 3;
    int nIn = 0;

    CScript scriptPubKey;
    CScript scriptSig;
    CScriptWitness wit;

    scriptPubKey << OP_EQUAL;
    CTransaction creditTx = BuildCreditingTransaction(scriptPubKey, 1);
    CTransaction spendTx = BuildSpendingTransaction(scriptSig, wit, creditTx);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << spendTx;

    bitcoinconsensus_error err;
    int result = bitcoinconsensus_verify_script(scriptPubKey.data(), scriptPubKey.size(), (const unsigned char*)&stream[0], stream.size(), nIn, libconsensus_flags, &err);
    BOOST_CHECK_EQUAL(result, 0);
    BOOST_CHECK_EQUAL(err, bitcoinconsensus_ERR_INVALID_FLAGS);
}

#endif // defined(HAVE_CONSENSUS_LIB)

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

        // SCRIPT_VERIFY_WITNESS requires SCRIPT_VERIFY_P2SH
        if (flag & SCRIPT_VERIFY_WITNESS && !(flag & SCRIPT_VERIFY_P2SH)) continue;
        // SCRIPT_VERIFY_TAPROOT requires SCRIPT_VERIFY_WITNESS
        if (flag & SCRIPT_VERIFY_TAPROOT && !(flag & SCRIPT_VERIFY_WITNESS)) continue;

        ret.push_back(flag);
    }

    return ret;
}

/** Precomputed list of all valid combinations of consensus-relevant script validation flags. */
static const std::vector<unsigned int> ALL_CONSENSUS_FLAGS = AllConsensusFlags();

static void AssetTest(const UniValue& test)
{
    BOOST_CHECK(test.isObject());

    CMutableTransaction mtx = TxFromHex(test["tx"].get_str());
    const std::vector<CTxOut> prevouts = TxOutsFromJSON(test["prevouts"]);
    BOOST_CHECK(prevouts.size() == mtx.vin.size());
    size_t idx = test["index"].get_int64();
    unsigned int test_flags = ParseScriptFlags(test["flags"].get_str());
    bool fin = test.exists("final") && test["final"].get_bool();

    if (test.exists("success")) {
        mtx.vin[idx].scriptSig = ScriptFromHex(test["success"]["scriptSig"].get_str());
        mtx.vin[idx].scriptWitness = ScriptWitnessFromJSON(test["success"]["witness"]);
        CTransaction tx(mtx);
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::vector<CTxOut>(prevouts));
        CachingTransactionSignatureChecker txcheck(&tx, idx, prevouts[idx].nValue, true, txdata);
        for (const auto flags : ALL_CONSENSUS_FLAGS) {
            // "final": true tests are valid for all flags. Others are only valid with flags that are
            // a subset of test_flags.
            if (fin || ((flags & test_flags) == flags)) {
                bool ret = VerifyScript(tx.vin[idx].scriptSig, prevouts[idx].scriptPubKey, &tx.vin[idx].scriptWitness, flags, txcheck, nullptr);
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
        CachingTransactionSignatureChecker txcheck(&tx, idx, prevouts[idx].nValue, true, txdata);
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

    const char* dir = std::getenv("DIR_UNIT_TEST_DATA");
    BOOST_WARN_MESSAGE(dir != nullptr, "Variable DIR_UNIT_TEST_DATA unset, skipping script_assets_test");
    if (dir == nullptr) return;
    auto path = fs::path(dir) / "script_assets_test.json";
    bool exists = fs::exists(path);
    BOOST_WARN_MESSAGE(exists, "File $DIR_UNIT_TEST_DATA/script_assets_test.json not found, skipping script_assets_test");
    if (!exists) return;
    fs::ifstream file(path);
    BOOST_CHECK(file.is_open());
    file.seekg(0, std::ios::end);
    size_t length = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string data(length, '\0');
    file.read(&data[0], data.size());
    UniValue tests = read_json(data);
    BOOST_CHECK(tests.isArray());
    BOOST_CHECK(tests.size() > 0);

    for (size_t i = 0; i < tests.size(); i++) {
        AssetTest(tests[i]);
    }
    file.close();
}

BOOST_AUTO_TEST_SUITE_END()
