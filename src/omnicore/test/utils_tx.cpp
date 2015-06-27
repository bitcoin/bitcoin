#include "omnicore/test/utils_tx.h"

#include "omnicore/omnicore.h"
#include "omnicore/script.h"

#include "base58.h"
#include "primitives/transaction.h"
#include "pubkey.h"
#include "script/script.h"
#include "script/standard.h"
#include "utilstrencodings.h"

#include <stdint.h>

CTxOut PayToPubKeyHash_Exodus()
{
    CBitcoinAddress address = ExodusAddress();
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

CTxOut PayToPubKeyHash_ExodusCrowdsale(int nHeight)
{
    CBitcoinAddress address = ExodusCrowdsaleAddress(nHeight);
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

CTxOut PayToPubKeyHash_Unrelated()
{
    CBitcoinAddress address("1f2dj45pxYb8BCW5sSbCgJ5YvXBfSapeX");
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

CTxOut PayToScriptHash_Unrelated()
{
    CBitcoinAddress address("3MbYQMMmSkC3AgWkj9FMo5LsPTW1zBTwXL");
    CScript scriptPubKey = GetScriptForDestination(address.Get());
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

CTxOut PayToPubKey_Unrelated()
{
    std::vector<unsigned char> vchPubKey = ParseHex(
        "04ad90e5b6bc86b3ec7fac2c5fbda7423fc8ef0d58df594c773fa05e2c281b2bfe"
        "877677c668bd13603944e34f4818ee03cadd81a88542b8b4d5431264180e2c28");

    CPubKey pubKey(vchPubKey.begin(), vchPubKey.end());

    CScript scriptPubKey;
    scriptPubKey << ToByteVector(pubKey) << OP_CHECKSIG;

    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

CTxOut PayToBareMultisig_1of2()
{
    std::vector<unsigned char> vchPayload1 = ParseHex(
        "04ad90e5b6bc86b3ec7fac2c5fbda7423fc8ef0d58df594c773fa05e2c281b2bfe"
        "877677c668bd13603944e34f4818ee03cadd81a88542b8b4d5431264180e2c28");
    std::vector<unsigned char> vchPayload2 = ParseHex(
        "026766a63686d2cc5d82c929d339b7975010872aa6bf76f6fac69f28f8e293a914");

    CPubKey pubKey1(vchPayload1.begin(), vchPayload1.end());
    CPubKey pubKey2(vchPayload2.begin(), vchPayload2.end());

    // 1-of-2 bare multisig script
    CScript scriptPubKey;
    scriptPubKey << CScript::EncodeOP_N(1);
    scriptPubKey << ToByteVector(pubKey1);
    scriptPubKey << ToByteVector(pubKey2);
    scriptPubKey << CScript::EncodeOP_N(2);
    scriptPubKey << OP_CHECKMULTISIG;

    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

CTxOut PayToBareMultisig_1of3()
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

CTxOut PayToBareMultisig_3of5()
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

CTxOut OpReturn_Empty()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN;

    return CTxOut(0, scriptPubKey);
}

CTxOut OpReturn_UnrelatedShort()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("b9");

    return CTxOut(0, scriptPubKey);
}

CTxOut OpReturn_Unrelated()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("7468697320697320646578782028323031352d30362d313129");

    return CTxOut(0, scriptPubKey);
}

CTxOut OpReturn_PlainMarker()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("6f6d6e69");

    return CTxOut(0, scriptPubKey);
}

CTxOut OpReturn_SimpleSend()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN << ParseHex("6f6d6e6900000000000000070000000006dac2c0");

    return CTxOut(0, scriptPubKey);
}


CTxOut OpReturn_MultiSimpleSend()
{
    CScript scriptPubKey;
    scriptPubKey << OP_RETURN;
    scriptPubKey << ParseHex("6f6d6e69");
    scriptPubKey << ParseHex("00000000000000070000000000002329");
    scriptPubKey << ParseHex("0062e907b15cbf27d5425399ebf6f0fb50ebb88f18");
    scriptPubKey << ParseHex("000000000000001f0000000001406f40");
    scriptPubKey << ParseHex("05da59767e81f4b019fe9f5984dbaa4f61bf197967");

    return CTxOut(0, scriptPubKey);
}

CTxOut NonStandardOutput()
{
    CScript scriptPubKey;
    scriptPubKey << ParseHex("decafbad") << OP_DROP << OP_TRUE;
    int64_t amount = GetDustThreshold(scriptPubKey);

    return CTxOut(amount, scriptPubKey);
}

