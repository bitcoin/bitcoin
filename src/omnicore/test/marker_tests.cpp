#include "omnicore/omnicore.h"
#include "omnicore/script.h"

#include "base58.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "utilstrencodings.h"

#include <stdint.h>
#include <limits>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

// Forward declarations
static CTxOut PayToPubKeyHash_Exodus();
static CTxOut PayToPubKeyHash_ExodusCrowdsale(int nHeight);
static CTxOut PayToPubKeyHash_Unrelated();
static CTxOut PayToScriptHash_Unrelated();
static CTxOut PayToPubKey_Unrelated();
static CTxOut PayToBareMultisig_1of3();
static CTxOut PayToBareMultisig_3of5();
static CTxOut OpReturn_Empty();
static CTxOut OpReturn_UnrelatedShort();
static CTxOut OpReturn_Unrelated();
static CTxOut OpReturn_Tagged_A();
static CTxOut OpReturn_Tagged_B();
static CTxOut OpReturn_Tagged_C();
static CTxOut NonStandardOutput();

BOOST_AUTO_TEST_SUITE(omnicore_marker_tests)

BOOST_AUTO_TEST_CASE(class_no_marker)
{
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        CTransaction tx(mutableTx);

        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_C());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_A());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_C());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_A());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(NonStandardOutput());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), NO_MARKER);
    }
}

BOOST_AUTO_TEST_CASE(class_class_a)
{
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_A);
    }
    {
        int nBlock = P2SH_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_A);
    }
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_C());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_A());
        mutableTx.vout.push_back(OpReturn_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_A);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_A);
    }
}

BOOST_AUTO_TEST_CASE(class_class_b)
{
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_B);
    }
    {
        int nBlock = OP_RETURN_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_B);
    }
    {
        int nBlock = 0;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Tagged_A());
        mutableTx.vout.push_back(OpReturn_Tagged_C());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_B);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_B);
    }
}

BOOST_AUTO_TEST_CASE(class_class_c)
{
    {
        int nBlock = OP_RETURN_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Tagged_A());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_A());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = OP_RETURN_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Tagged_B());
        mutableTx.vout.push_back(PayToBareMultisig_3of5());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = std::numeric_limits<int>().max();

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(OpReturn_UnrelatedShort());
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(OpReturn_Unrelated());
        mutableTx.vout.push_back(OpReturn_Empty());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));
        mutableTx.vout.push_back(OpReturn_Tagged_C());

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
    {
        int nBlock = OP_RETURN_BLOCK;

        CMutableTransaction mutableTx;
        mutableTx.vout.push_back(PayToPubKey_Unrelated());
        mutableTx.vout.push_back(PayToBareMultisig_1of3());
        mutableTx.vout.push_back(PayToPubKeyHash_Unrelated());
        mutableTx.vout.push_back(PayToScriptHash_Unrelated());
        mutableTx.vout.push_back(NonStandardOutput());
        mutableTx.vout.push_back(OpReturn_Tagged_A());
        mutableTx.vout.push_back(PayToPubKeyHash_Exodus());
        mutableTx.vout.push_back(PayToPubKeyHash_ExodusCrowdsale(nBlock));

        CTransaction tx(mutableTx);
        BOOST_CHECK_EQUAL(GetEncodingClass(tx, nBlock), OMNI_CLASS_C);
    }
}

BOOST_AUTO_TEST_SUITE_END()

static CTxOut PayToPubKeyHash_Exodus()
{
    CBitcoinAddress address = ExodusAddress();
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut PayToPubKeyHash_ExodusCrowdsale(int nHeight)
{
    CBitcoinAddress address = ExodusCrowdsaleAddress(nHeight);
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut PayToPubKeyHash_Unrelated()
{
    CBitcoinAddress address("1f2dj45pxYb8BCW5sSbCgJ5YvXBfSapeX");
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut PayToScriptHash_Unrelated()
{
    CBitcoinAddress address("3MbYQMMmSkC3AgWkj9FMo5LsPTW1zBTwXL");
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut PayToPubKey_Unrelated()
{
    std::vector<unsigned char> vchPubKey = ParseHex(
        "04ad90e5b6bc86b3ec7fac2c5fbda7423fc8ef0d58df594c773fa05e2c281b2bfe"
        "877677c668bd13603944e34f4818ee03cadd81a88542b8b4d5431264180e2c28");

    CPubKey pubKey(vchPubKey.begin(), vchPubKey.end());

    CScript scriptPubKey;
    scriptPubKey << ToByteVector(pubKey) << OP_CHECKSIG;

    int64_t amount = GetDustThreshold(scriptPubKey);

    txnouttype outType;
    BOOST_CHECK(GetOutputType(scriptPubKey, outType));
    BOOST_CHECK(outType == TX_PUBKEY);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut PayToBareMultisig_1of3()
{
    std::vector<unsigned char> vchPayload1 = ParseHex(
        "04ad90e5b6bc86b3ec7fac2c5fbda7423fc8ef0d58df594c773fa05e2c281b2bfe"
        "877677c668bd13603944e34f4818ee03cadd81a88542b8b4d5431264180e2c28");
    std::vector<unsigned char> vchPayload2 = ParseHex(
        "026766a63686d2cc5d82c929d339b7975010872aa6bf76f6fac69f28f8e293a914");
    std::vector<unsigned char> vchPayload3 = ParseHex(
        "02959b8e2f2e4fb67952cda291b467a1781641c94c37feaa0733a12782977da23b");

    CPubKey pubKey1(vchPayload1.begin(), vchPayload1.end());
    CPubKey pubKey2(vchPayload2.begin(), vchPayload2.end());
    CPubKey pubKey3(vchPayload3.begin(), vchPayload3.end());

    // 1-of-3 bare multisig script
    CScript scriptPubKey;
    scriptPubKey << CScript::EncodeOP_N(1);
    scriptPubKey << ToByteVector(pubKey1);
    scriptPubKey << ToByteVector(pubKey2);
    scriptPubKey << ToByteVector(pubKey3);
    scriptPubKey << CScript::EncodeOP_N(3);
    scriptPubKey << OP_CHECKMULTISIG;

    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut PayToBareMultisig_3of5()
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

    // 3-of-5 bare multisig script
    CScript scriptPubKey;
    scriptPubKey << CScript::EncodeOP_N(3);
    scriptPubKey << ToByteVector(pubKey1);
    scriptPubKey << ToByteVector(pubKey2) << ToByteVector(pubKey3);
    scriptPubKey << ToByteVector(pubKey4) << ToByteVector(pubKey5);
    scriptPubKey << CScript::EncodeOP_N(5);
    scriptPubKey << OP_CHECKMULTISIG;

    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

static CTxOut OpReturn_Empty()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN;

    return CTxOut(0, scriptPubKey);
}

static CTxOut OpReturn_UnrelatedShort()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("b9");

    return CTxOut(0, scriptPubKey);
}

static CTxOut OpReturn_Unrelated()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("7468697320697320646578782028323031352d30362d313129");

    return CTxOut(0, scriptPubKey);
}

static CTxOut OpReturn_Tagged_A()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("6f6d0000408");

    return CTxOut(0, scriptPubKey);
}

static CTxOut OpReturn_Tagged_B()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("6f6d00011516");

    return CTxOut(0, scriptPubKey);
}

static CTxOut OpReturn_Tagged_C()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("6f6dffff2342");

    return CTxOut(0, scriptPubKey);
}

static CTxOut NonStandardOutput()
{
    CScript scriptPubKey;
    scriptPubKey << ParseHex("decafbad") << OP_DROP << OP_TRUE;
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

