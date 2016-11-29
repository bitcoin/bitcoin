#include "omnicore/script.h"

#include "base58.h"
#include "pubkey.h"
#include "script/script.h"
#include "test/test_bitcoin.h"
#include "utilstrencodings.h"

#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(omnicore_script_extraction_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(extract_pubkey_test)
{
    std::vector<unsigned char> vchPayload = ParseHex(
        "0347d08029b5cbc934f6079b650c50718eab5a56d51cf6b742ec9f865a41fcfca3");
    CPubKey pubKey(vchPayload.begin(), vchPayload.end());

    // Pay-to-pubkey script
    CScript script;
    script << ToByteVector(pubKey) << OP_CHECKSIG;

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_PUBKEY);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), 1);
    BOOST_CHECK_EQUAL(vstrSolutions[0], HexStr(vchPayload));
}

BOOST_AUTO_TEST_CASE(extract_pubkeyhash_test)
{
    std::vector<unsigned char> vchPayload = ParseHex(
        "0347d08029b5cbc934f6079b650c50718eab5a56d51cf6b742ec9f865a41fcfca3");

    CPubKey pubKey(vchPayload.begin(), vchPayload.end());
    CKeyID keyId = pubKey.GetID();

    // Pay-to-pubkey-hash script
    CScript script;
    script << OP_DUP << OP_HASH160 << ToByteVector(keyId) << OP_EQUALVERIFY << OP_CHECKSIG;

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_PUBKEYHASH);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), 1);
    BOOST_CHECK_EQUAL(vstrSolutions[0], HexStr(ToByteVector(keyId)));
}

BOOST_AUTO_TEST_CASE(extract_multisig_test)
{
    std::vector<unsigned char> vchPayload1 = ParseHex(
        "03a34b99f22c790c4e36b2b3c2c35a36db06226e41c692fc82b8b56ac1c540c5bd");
    std::vector<unsigned char> vchPayload2 = ParseHex(
        "0276f798620d7d0930711ab68688fc67ee2f5bbe0c1481506b08bd65e6053c16ca");
    std::vector<unsigned char> vchPayload3 = ParseHex(
        "02bf12b315172dc1b261d62dd146868ef9c9e2e108fa347f347f66bc048e9b15e4");

    CPubKey pubKey1(vchPayload1.begin(), vchPayload1.end());
    CPubKey pubKey2(vchPayload2.begin(), vchPayload2.end());
    CPubKey pubKey3(vchPayload3.begin(), vchPayload3.end());

    // 1-of-3 bare multisig script
    CScript script;
    script << CScript::EncodeOP_N(1);
    script << ToByteVector(pubKey1) << ToByteVector(pubKey2) << ToByteVector(pubKey3);
    script << CScript::EncodeOP_N(3);
    script << OP_CHECKMULTISIG;

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_MULTISIG);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), 3);
    BOOST_CHECK_EQUAL(vstrSolutions[0], HexStr(vchPayload1));
    BOOST_CHECK_EQUAL(vstrSolutions[1], HexStr(vchPayload2));
    BOOST_CHECK_EQUAL(vstrSolutions[2], HexStr(vchPayload3));
}

BOOST_AUTO_TEST_CASE(extract_multisig_with_skip_test)
{
    std::vector<unsigned char> vchPayload1 = ParseHex(
        "02619c30f643a4679ec2f690f3d6564df7df2ae23ae4a55393ae0bef22db9dbcaf");
    std::vector<unsigned char> vchPayload2 = ParseHex(
        "026766a63686d2cc5d82c929d339b7975010872aa6bf76f6fac69f28f8e293a914");
    std::vector<unsigned char> vchPayload3 = ParseHex(
        "02959b8e2f2e4fb67952cda291b467a1781641c94c37feaa0733a12782977da23b");
    std::vector<unsigned char> vchPayload4 = ParseHex(
        "0261a017029ec4688ec9bf33c44ad2e595f83aaf3ed4f3032d1955715f5ffaf6b8");
    std::vector<unsigned char> vchPayload5 = ParseHex(
        "02dc1a0afc933d703557d9f5e86423a5cec9fee4bfa850b3d02ceae72117178802");

    CPubKey pubKey1(vchPayload1.begin(), vchPayload1.end());
    CPubKey pubKey2(vchPayload2.begin(), vchPayload2.end());
    CPubKey pubKey3(vchPayload3.begin(), vchPayload3.end());
    CPubKey pubKey4(vchPayload4.begin(), vchPayload4.end());
    CPubKey pubKey5(vchPayload5.begin(), vchPayload5.end());

    // 1-of-5 bare multisig script
    CScript script;
    script << CScript::EncodeOP_N(1);
    script << ToByteVector(pubKey1);
    script << ToByteVector(pubKey2) << ToByteVector(pubKey3);
    script << ToByteVector(pubKey4) << ToByteVector(pubKey5);
    script << CScript::EncodeOP_N(5);
    script << OP_CHECKMULTISIG;

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_MULTISIG);

    // Confirm extracted data, and skip first public key
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions, true));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), 4);
    BOOST_CHECK_EQUAL(vstrSolutions[0], HexStr(vchPayload2));
    BOOST_CHECK_EQUAL(vstrSolutions[1], HexStr(vchPayload3));
    BOOST_CHECK_EQUAL(vstrSolutions[2], HexStr(vchPayload4));
    BOOST_CHECK_EQUAL(vstrSolutions[3], HexStr(vchPayload5));
}

BOOST_AUTO_TEST_CASE(extract_scripthash_test)
{
    std::vector<unsigned char> vchInnerScript = ParseHex(
        "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000");

    // A transaction puzzle (not relevant)
    CScript scriptInner;
    scriptInner << OP_HASH256 << vchInnerScript << OP_EQUAL;

    // The actual hash
    CScriptID innerId(scriptInner);

    // Pay-to-script-hash script
    CScript script;
    script << OP_HASH160 << ToByteVector(innerId) << OP_EQUAL;

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_SCRIPTHASH);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), 1);
    BOOST_CHECK_EQUAL(vstrSolutions[0], HexStr(ToByteVector(innerId)));
}

BOOST_AUTO_TEST_CASE(extract_no_nulldata_test)
{
    // Null data script
    CScript script;
    script << OP_RETURN;

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_NULL_DATA);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), 0);
}

BOOST_AUTO_TEST_CASE(extract_empty_nulldata_test)
{
    // Null data script
    CScript script;
    script << OP_RETURN << ParseHex("");

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_NULL_DATA);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), 1);
    BOOST_CHECK_EQUAL(vstrSolutions[0].size(), 0);
}

BOOST_AUTO_TEST_CASE(extract_nulldata_test)
{
    std::vector<unsigned char> vchPayload = ParseHex(
        "657874726163745f6e756c6c646174615f74657374");

    // Null data script
    CScript script;
    script << OP_RETURN << vchPayload;

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_NULL_DATA);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), 1);
    BOOST_CHECK_EQUAL(vstrSolutions[0], HexStr(vchPayload));
}

BOOST_AUTO_TEST_CASE(extract_nulldata_multipush_test)
{
    std::vector<std::string> vstrPayloads;
    vstrPayloads.push_back("6f6d");
    vstrPayloads.push_back("00000000000000010000000006dac2c0");
    vstrPayloads.push_back("01d84bcec5b65aa1a03d6abfd975824c75856a2961");
    vstrPayloads.push_back("00000000000000030000000000000d48");
    vstrPayloads.push_back("05627138bb55251bfb289a1ec390eafd3755b1a698");
    vstrPayloads.push_back("00000032010001000000005465737473004f6d6e6920436f726500546573"
            "7420546f6b656e7300687474703a2f2f6275696c6465722e62697477617463682e636f2f005"
            "573656420746f2074657374207468652065787472616374696f6e206f66206d756c7469706c"
            "652070757368657320696e20616e204f505f52455455524e207363726970742e00000000000"
            "00f4240");

    // Null data script
    CScript script;
    script << OP_RETURN;
    for (unsigned n = 0; n < vstrPayloads.size(); ++n) {
        script << ParseHex(vstrPayloads[n]);
    }

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_NULL_DATA);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), vstrPayloads.size());
    for (unsigned n = 0; n < vstrSolutions.size(); ++n) {
        BOOST_CHECK_EQUAL(vstrSolutions[n], vstrPayloads[n]);
    }
}

BOOST_AUTO_TEST_CASE(extract_anypush_test)
{
    std::vector<std::vector<unsigned char> > vvchPayloads;
    vvchPayloads.push_back(ParseHex("111111"));
    vvchPayloads.push_back(ParseHex("222222"));
    vvchPayloads.push_back(ParseHex("333333"));
    vvchPayloads.push_back(ParseHex("444444"));
    vvchPayloads.push_back(ParseHex("555555"));

    // Non-standard script
    CScript script;
    script << vvchPayloads[0] << OP_DROP;
    script << vvchPayloads[1] << OP_DROP;
    script << vvchPayloads[2] << OP_DROP;
    script << vvchPayloads[3] << OP_DROP;
    script << vvchPayloads[4];

    // Check script type
    txnouttype outtype;
    BOOST_CHECK(!GetOutputType(script, outtype));
    BOOST_CHECK_EQUAL(outtype, TX_NONSTANDARD);

    // Confirm extracted data
    std::vector<std::string> vstrSolutions;
    BOOST_CHECK(GetScriptPushes(script, vstrSolutions));
    BOOST_CHECK_EQUAL(vstrSolutions.size(), vvchPayloads.size());
    for (size_t n = 0; n < vstrSolutions.size(); ++n) {
        BOOST_CHECK_EQUAL(vstrSolutions[n], HexStr(vvchPayloads[n]));
    }
}


BOOST_AUTO_TEST_SUITE_END()
