// Copyright (c) 2021-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/policy.h>
#include <script/interpreter.h>

#include <test/lcg.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <array>

typedef std::vector<uint8_t> valtype;
typedef std::vector<valtype> stacktype;

BOOST_FIXTURE_TEST_SUITE(checkdatasig_tests, BasicTestingSetup)

std::array<uint32_t, 2> flagset{{0, STANDARD_SCRIPT_VERIFY_FLAGS}};

const uint8_t vchPrivkey[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

struct KeyData {
    CKey privkey, privkeyC;
    CPubKey pubkey, pubkeyC, pubkeyH;

    KeyData() {
        privkey.Set(vchPrivkey, vchPrivkey + 32, false);
        privkeyC.Set(vchPrivkey, vchPrivkey + 32, true);
        pubkey = privkey.GetPubKey();
        pubkeyH = privkey.GetPubKey();
        pubkeyC = privkeyC.GetPubKey();
        *const_cast<uint8_t *>(&pubkeyH[0]) = 0x06 | (pubkeyH[64] & 1);
    }
};

static void CheckError(uint32_t flags, const stacktype& original_stack,
                       const CScript& script, ScriptError expected)
{
    BaseSignatureChecker sigchecker;
    ScriptError err = ScriptError::SCRIPT_ERR_OK;
    stacktype stack{original_stack};
    bool r = EvalScript(stack, script, flags | SCRIPT_ENABLE_DIP0020_OPCODES, sigchecker, SigVersion::BASE, &err);
    BOOST_CHECK(!r);
    BOOST_CHECK(err == expected);
}

static void CheckPass(uint32_t flags, const stacktype& original_stack,
                      const CScript& script, const stacktype& expected)
{
    BaseSignatureChecker sigchecker;
    ScriptError err = ScriptError::SCRIPT_ERR_OK;
    stacktype stack{original_stack};
    bool r = EvalScript(stack, script, flags | SCRIPT_ENABLE_DIP0020_OPCODES, sigchecker, SigVersion::BASE, &err);
    BOOST_CHECK(r);
    BOOST_CHECK(err == ScriptError::SCRIPT_ERR_OK);
    BOOST_CHECK(stack == expected);
}

/**
 * General utility functions to check for script passing/failing.
 */
static void CheckTestResultForAllFlags(const stacktype& original_stack,
                                       const CScript& script,
                                       const stacktype& expected)
{
    for (uint32_t flags : flagset) {
        CheckPass(flags, original_stack, script, expected);
    }
}

static void CheckErrorForAllFlags(const stacktype& original_stack,
                                  const CScript& script, ScriptError expected)
{
    for (uint32_t flags : flagset) {
        CheckError(flags, original_stack, script, expected);
    }
}

BOOST_AUTO_TEST_CASE(checkdatasig_test)
{
    // Empty stack.
    CheckErrorForAllFlags({}, CScript() << OP_CHECKDATASIG,
                          ScriptError::SCRIPT_ERR_INVALID_STACK_OPERATION);
    CheckErrorForAllFlags({{0x00}}, CScript() << OP_CHECKDATASIG,
                          ScriptError::SCRIPT_ERR_INVALID_STACK_OPERATION);
    CheckErrorForAllFlags({{0x00}, {0x00}}, CScript() << OP_CHECKDATASIG,
                          ScriptError::SCRIPT_ERR_INVALID_STACK_OPERATION);
    CheckErrorForAllFlags({}, CScript() << OP_CHECKDATASIGVERIFY,
                          ScriptError::SCRIPT_ERR_INVALID_STACK_OPERATION);
    CheckErrorForAllFlags({{0x00}}, CScript() << OP_CHECKDATASIGVERIFY,
                          ScriptError::SCRIPT_ERR_INVALID_STACK_OPERATION);
    CheckErrorForAllFlags({{0x00}, {0x00}}, CScript() << OP_CHECKDATASIGVERIFY,
                          ScriptError::SCRIPT_ERR_INVALID_STACK_OPERATION);

    // Check various pubkey encoding.
    const valtype message{};
    valtype vchHash(32);
    CSHA256().Write(message.data(), message.size()).Finalize(vchHash.data());
    uint256 messageHash(vchHash);

    KeyData kd;
    valtype pubkey = ToByteVector(kd.pubkey);
    valtype pubkeyC = ToByteVector(kd.pubkeyC);
    valtype pubkeyH = ToByteVector(kd.pubkeyH);

    CheckTestResultForAllFlags({{}, message, pubkey},
                               CScript() << OP_CHECKDATASIG, {{}});
    CheckTestResultForAllFlags({{}, message, pubkeyC},
                               CScript() << OP_CHECKDATASIG, {{}});
    CheckErrorForAllFlags({{}, message, pubkey},
                          CScript() << OP_CHECKDATASIGVERIFY,
                          ScriptError::SCRIPT_ERR_CHECKDATASIGVERIFY);
    CheckErrorForAllFlags({{}, message, pubkeyC},
                          CScript() << OP_CHECKDATASIGVERIFY,
                          ScriptError::SCRIPT_ERR_CHECKDATASIGVERIFY);

    // Flags dependent checks.
    const CScript script = CScript() << OP_CHECKDATASIG << OP_NOT << OP_VERIFY;
    const CScript scriptverify = CScript() << OP_CHECKDATASIGVERIFY;

    // Check valid signatures (as in the signature format is valid).
    valtype validsig;
    kd.privkey.Sign(messageHash, validsig);
    validsig.push_back(static_cast<unsigned char>(1));

    CheckTestResultForAllFlags({validsig, message, pubkey},
                               CScript() << OP_CHECKDATASIG, {{0x01}});
    CheckTestResultForAllFlags({validsig, message, pubkey},
                               CScript() << OP_CHECKDATASIGVERIFY, {});

    const valtype minimalsig{0x30, 0x06, 0x02, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01};
    const valtype nondersig{0x30, 0x80, 0x06, 0x02, 0x01,
                            0x01, 0x02, 0x01, 0x01, 0x01};
    const valtype highSSig{
        0x30, 0x45, 0x02, 0x20, 0x3e, 0x45, 0x16, 0xda, 0x72, 0x53, 0xcf, 0x06,
        0x8e, 0xff, 0xec, 0x6b, 0x95, 0xc4, 0x12, 0x21, 0xc0, 0xcf, 0x3a, 0x8e,
        0x6c, 0xcb, 0x8c, 0xbf, 0x17, 0x25, 0xb5, 0x62, 0xe9, 0xaf, 0xde, 0x2c,
        0x02, 0x21, 0x00, 0xab, 0x1e, 0x3d, 0xa7, 0x3d, 0x67, 0xe3, 0x20, 0x45,
        0xa2, 0x0e, 0x0b, 0x99, 0x9e, 0x04, 0x99, 0x78, 0xea, 0x8d, 0x6e, 0xe5,
        0x48, 0x0d, 0x48, 0x5f, 0xcf, 0x2c, 0xe0, 0xd0, 0x3b, 0x2e, 0xf0, 0x01};

    MMIXLinearCongruentialGenerator lcg;
    for (int i = 0; i < 4096; i++) {
        uint32_t flags = lcg.next();

        if (flags & SCRIPT_VERIFY_STRICTENC) {
            // When strict encoding is enforced, hybrid keys are invalid.
            CheckError(flags, {{}, message, pubkeyH}, script,
                       ScriptError::SCRIPT_ERR_PUBKEYTYPE);
            CheckError(flags, {{}, message, pubkeyH}, scriptverify,
                       ScriptError::SCRIPT_ERR_PUBKEYTYPE);
        } else {
            // Otherwise, hybrid keys are valid.
            CheckPass(flags, {{}, message, pubkeyH}, script, {});
            CheckError(flags, {{}, message, pubkeyH}, scriptverify,
                       ScriptError::SCRIPT_ERR_CHECKDATASIGVERIFY);
        }

        // Uncompressed keys are valid.
        CheckPass(flags, {{}, message, pubkey}, script, {});
        CheckError(flags, {{}, message, pubkey}, scriptverify,
                   ScriptError::SCRIPT_ERR_CHECKDATASIGVERIFY);

        if (flags & SCRIPT_VERIFY_NULLFAIL) {
            // Invalid signature causes checkdatasig to fail.
            CheckError(flags, {minimalsig, message, pubkeyC}, script,
                       ScriptError::SCRIPT_ERR_SIG_NULLFAIL);
            CheckError(flags, {minimalsig, message, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_SIG_NULLFAIL);

            // Invalid message causes checkdatasig to fail.
            CheckError(flags, {validsig, {0x01}, pubkeyC}, script,
                       ScriptError::SCRIPT_ERR_SIG_NULLFAIL);
            CheckError(flags, {validsig, {0x01}, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_SIG_NULLFAIL);
        } else {
            // When nullfail is not enforced, invalid signature are just false.
            CheckPass(flags, {minimalsig, message, pubkeyC}, script, {});
            CheckError(flags, {minimalsig, message, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_CHECKDATASIGVERIFY);

            // Invalid message cause checkdatasig to fail.
            CheckPass(flags, {validsig, {0x01}, pubkeyC}, script, {});
            CheckError(flags, {validsig, {0x01}, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_CHECKDATASIGVERIFY);
        }

        if (flags & SCRIPT_VERIFY_LOW_S) {
            // If we do enforce low S, then high S sigs are rejected.
            CheckError(flags, {highSSig, message, pubkeyC}, script,
                       ScriptError::SCRIPT_ERR_SIG_HIGH_S);
            CheckError(flags, {highSSig, message, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_SIG_HIGH_S);
        } else if (flags & SCRIPT_VERIFY_NULLFAIL) {
            // If we do enforce nullfail, these invalid sigs hit this.
            CheckError(flags, {highSSig, message, pubkeyC}, script,
                       ScriptError::SCRIPT_ERR_SIG_NULLFAIL);
            CheckError(flags, {highSSig, message, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_SIG_NULLFAIL);
        } else {
            // If we do not enforce low S, then high S sigs are accepted.
            CheckPass(flags, {highSSig, message, pubkeyC}, script, {});
            CheckError(flags, {highSSig, message, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_CHECKDATASIGVERIFY);
        }

        if (flags & (SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_LOW_S |
                     SCRIPT_VERIFY_STRICTENC)) {
            // If we get any of the dersig flags, the non canonical dersig
            // signature fails.
            CheckError(flags, {nondersig, message, pubkeyC}, script,
                       ScriptError::SCRIPT_ERR_SIG_DER);
            CheckError(flags, {nondersig, message, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_SIG_DER);
        } else if (flags & SCRIPT_VERIFY_NULLFAIL) {
            // If we do enforce nullfail, these invalid sigs hit this.
            CheckError(flags, {nondersig, message, pubkeyC}, script,
                       ScriptError::SCRIPT_ERR_SIG_NULLFAIL);
            CheckError(flags, {nondersig, message, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_SIG_NULLFAIL);
        } else {
            // If we do not check, then it is accepted.
            CheckPass(flags, {nondersig, message, pubkeyC}, script, {});
            CheckError(flags, {nondersig, message, pubkeyC}, scriptverify,
                       ScriptError::SCRIPT_ERR_CHECKDATASIGVERIFY);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
