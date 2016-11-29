#include "omnicore/script.h"

#include "amount.h"
#include "main.h"
#include "script/script.h"
#include "test/test_bitcoin.h"
#include "utilstrencodings.h"
#include "primitives/transaction.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <string>
#include <vector>

// Is resetted to a norm value in each test
extern CFeeRate minRelayTxFee;
static CFeeRate minRelayTxFeeOriginal = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);

BOOST_FIXTURE_TEST_SUITE(omnicore_script_dust_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(dust_threshold_pubkey_hash)
{
    minRelayTxFee = CFeeRate(1000);
    const int64_t nExpected = 546; // satoshi
    const size_t nScriptSize = 25; // byte

    // Pay-to-pubkey-hash
    std::vector<unsigned char> vchScript = ParseHex(
        "76a914946cb2e08075bcbaf157e47bcb67eb2b2339d24288ac");

    CScript script(vchScript.begin(), vchScript.end());
    BOOST_CHECK_EQUAL(script.size(), nScriptSize);
    BOOST_CHECK_EQUAL(GetDustThreshold(script), nExpected);

    // Confirm an output with that value and script is no dust
    BOOST_CHECK_EQUAL(CTxOut(nExpected, script).IsDust(minRelayTxFee), false);
    // ... but an output, spending one satoshi less, is indeed
    BOOST_CHECK_EQUAL(CTxOut(nExpected - 1, script).IsDust(minRelayTxFee), true);

    minRelayTxFee = minRelayTxFeeOriginal;
}

BOOST_AUTO_TEST_CASE(dust_threshold_script_hash)
{
    minRelayTxFee = CFeeRate(1000);
    const int64_t nExpected = 540; // satoshi
    const size_t nScriptSize = 23; // byte

    // Pay-to-script-hash
    std::vector<unsigned char> vchScript = ParseHex(
        "a914da59767e81f4b019fe9f5984dbaa4f61bf19796787");

    CScript script(vchScript.begin(), vchScript.end());
    BOOST_CHECK_EQUAL(script.size(), nScriptSize);
    BOOST_CHECK_EQUAL(GetDustThreshold(script), nExpected);
    BOOST_CHECK_EQUAL(CTxOut(nExpected, script).IsDust(minRelayTxFee), false);
    BOOST_CHECK_EQUAL(CTxOut(nExpected - 1, script).IsDust(minRelayTxFee), true);

    minRelayTxFee = minRelayTxFeeOriginal;
}

BOOST_AUTO_TEST_CASE(dust_threshold_multisig_compressed_compressed)
{
    minRelayTxFee = CFeeRate(1000);
    const int64_t nExpected = 684; // satoshi
    const size_t nScriptSize = 71; // byte

    // Bare multisig, two compressed public keys
    std::vector<unsigned char> vchScript = ParseHex(
        "512102f3e471222bb57a7d416c82bf81c627bfcd2bdc47f36e763ae69935bba4601ece"
        "21021580b888e354feb27e17f08802ebea87058c23697d6a462d43fc13b565fda29052"
        "ae");

    CScript script(vchScript.begin(), vchScript.end());
    BOOST_CHECK_EQUAL(script.size(), nScriptSize);
    BOOST_CHECK_EQUAL(GetDustThreshold(script), nExpected);
    BOOST_CHECK_EQUAL(CTxOut(nExpected, script).IsDust(minRelayTxFee), false);
    BOOST_CHECK_EQUAL(CTxOut(nExpected - 1, script).IsDust(minRelayTxFee), true);

    minRelayTxFee = minRelayTxFeeOriginal;
}

BOOST_AUTO_TEST_CASE(dust_threshold_multisig_uncompressed_compressed)
{
    minRelayTxFee = CFeeRate(1000);
    const int64_t nExpected  = 780; // satoshi
    const size_t nScriptSize = 103; // byte

    // Bare multisig, one uncompressed, one compressed public key
    std::vector<unsigned char> vchScript = ParseHex(
        "514104fcf07bb1222f7925f2b7cc15183a40443c578e62ea17100aa3b44ba66905c95d"
        "4980aec4cd2f6eb426d1b1ec45d76724f26901099416b9265b76ba67c8b0b73d210202"
        "be80a0ca69c0e000b97d507f45b988d2f58fec6650b64ff70e6ffccc3e6df152ae");

    CScript script(vchScript.begin(), vchScript.end());
    BOOST_CHECK_EQUAL(script.size(), nScriptSize);
    BOOST_CHECK_EQUAL(GetDustThreshold(script), nExpected);
    BOOST_CHECK_EQUAL(CTxOut(nExpected, script).IsDust(minRelayTxFee), false);
    BOOST_CHECK_EQUAL(CTxOut(nExpected - 1, script).IsDust(minRelayTxFee), true);

    minRelayTxFee = minRelayTxFeeOriginal;
}

BOOST_AUTO_TEST_CASE(dust_threshold_multisig_compressed_compressed_compressed)
{
    minRelayTxFee = CFeeRate(1000);
    const int64_t nExpected  = 786; // satoshi
    const size_t nScriptSize = 105; // byte

    // Bare multisig, three uncompressed public keys
    std::vector<unsigned char> vchScript = ParseHex(
        "512102619c30f643a4679ec2f690f3d6564df7df2ae23ae4a55393ae0bef22db9dbcaf"
        "21026766a63686d2cc5d82c929d339b7975010872aa6bf76f6fac69f28f8e293a91421"
        "02959b8e2f2e4fb67952cda291b467a1781641c94c37feaa0733a12782977da23b53ae");

    CScript script(vchScript.begin(), vchScript.end());
    BOOST_CHECK_EQUAL(script.size(), nScriptSize);
    BOOST_CHECK_EQUAL(GetDustThreshold(script), nExpected);
    BOOST_CHECK_EQUAL(CTxOut(nExpected, script).IsDust(minRelayTxFee), false);
    BOOST_CHECK_EQUAL(CTxOut(nExpected - 1, script).IsDust(minRelayTxFee), true);

    minRelayTxFee = minRelayTxFeeOriginal;
}

BOOST_AUTO_TEST_CASE(dust_threshold_multisig_uncompressed_compressed_compressed)
{
    minRelayTxFee = CFeeRate(1000);
    const int64_t nExpected  = 882; // satoshi
    const size_t nScriptSize = 137; // byte

    // Bare multisig, one uncompressed, two compressed public keys
    std::vector<unsigned char> vchScript = ParseHex(
        "5141044ab250ef237ef1e96300f530fc9fded23e0aa93221f69f7f767e07872cf35e60"
        "77f0a000910705555f0c7b43597b0435b67b93e6888484aac85d3a630715699f2102de"
        "1e51c492851b0ffbaf8f3a66244977ddebf03a8962a45665a60daeea962c682102145f"
        "d8229247bb399b369de809f95139b65bd93a00b767286ac3101ac4cb49bc53ae");

    CScript script(vchScript.begin(), vchScript.end());
    BOOST_CHECK_EQUAL(script.size(), nScriptSize);
    BOOST_CHECK_EQUAL(GetDustThreshold(script), nExpected);
    BOOST_CHECK_EQUAL(CTxOut(nExpected, script).IsDust(minRelayTxFee), false);
    BOOST_CHECK_EQUAL(CTxOut(nExpected - 1, script).IsDust(minRelayTxFee), true);

    minRelayTxFee = minRelayTxFeeOriginal;
}


BOOST_AUTO_TEST_SUITE_END()
