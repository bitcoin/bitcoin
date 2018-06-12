// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/client.h>

#include <base58.h>
#include <validation.h>
#include <wallet/hdwallet.h>

#include <policy/policy.h>

#include <wallet/test/hdwallet_test_fixture.h>

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

#include <univalue.h>


using namespace std;

extern UniValue createArgs(int nRequired, const char* address1 = NULL, const char* address2 = nullptr);
extern UniValue CallRPC(std::string args, std::string wallet="");

void RewindHdSxChain(CHDWallet *pwallet)
{
    // Rewind the chain to get the same key next request
    ExtKeyAccountMap::iterator mi = pwallet->mapExtAccounts.find(pwallet->idDefaultAccount);
    BOOST_REQUIRE(mi != pwallet->mapExtAccounts.end());
    CExtKeyAccount *sea = mi->second;
    BOOST_REQUIRE(sea->nActiveStealth < sea->vExtKeys.size());
    CStoredExtKey *sek = sea->vExtKeys[sea->nActiveStealth];
    sek->nHGenerated -= 2;
};

BOOST_FIXTURE_TEST_SUITE(rpc_hdwallet_tests, HDWalletTestingSetup)

BOOST_AUTO_TEST_CASE(rpc_hdwallet)
{
    UniValue rv;

    BOOST_CHECK_NO_THROW(rv = CallRPC("extkeyimportmaster xprv9s21ZrQH143K3VrEYG4rhyPddr2o53qqqpCufLP6Rb3XSta2FZsqCanRJVfpTi4UX28pRaAfVGfiGpYDczv8tzTM6Qm5TRvUA9HDStbNUbQ"));

    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress"));
    BOOST_CHECK(StripQuotes(rv.write()) == "SPGxiYZ1Q5dhAJxJNMk56ZbxcsUBYqTCsdEPPHsJJ96Vcns889gHTqSrTZoyrCd5E9NSe9XxLivK6izETniNp1Gu1DtrhVwv3VuZ3e");

    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress onebit 1"));
    BOOST_CHECK(StripQuotes(rv.write()) == "2w3KaKNNRkWvgxNVymgTwxVd95hDTKRwa98eh5fUpyZfQ17XCRDsxQ3tTARJYz2pNnCekEFni7ukDwvgdbVDgbTy449DNcJYrevkyzPL");

    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress onebit 1 0b1"));
    std::string sResult = StripQuotes(rv.write());
    BOOST_CHECK(sResult == "2w3KJzSkeDxiZDFQ5cQdMGKyEuM2zhKHX7TWCTVFobXXcWxWS5zs3aaoF3LPWfcwKd3m65CHx7j8F9CbESmi53GqmHJmnwKggRiXQoac");

    CStealthAddress s1;
    s1.SetEncoded(sResult);
    BOOST_CHECK(s1.Encoded() == sResult);
    BOOST_CHECK(s1.prefix.number_bits == 1);
    BOOST_CHECK(s1.prefix.bitfield == 0b1);


    RewindHdSxChain(pwalletMain.get());

    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress onebit 32"));
    sResult = StripQuotes(rv.write());
    BOOST_CHECK(sResult == "3s73gdiUKMVi4tHMTdker9YzHAS2r6F2CJvC12GfimDdTTn9CLEnEeWW8vdXXkeZouWLgxFGqzbPsnSShNRMsW3j3yL6ssEtjc3gwNSkbBfy");

    CStealthAddress s2;
    s2.SetEncoded(sResult);
    BOOST_CHECK(s2.prefix.number_bits == 32);
    BOOST_CHECK(s2.prefix.bitfield == 4215576597);

    // Check the key is the same
    BOOST_CHECK(s2.scan_pubkey == s1.scan_pubkey);
    BOOST_CHECK(s2.spend_pubkey == s1.spend_pubkey);

    RewindHdSxChain(pwalletMain.get());

    // Check the same prefix is generated
    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress onebit 32"));
    BOOST_CHECK(StripQuotes(rv.write()) == "3s73gdiUKMVi4tHMTdker9YzHAS2r6F2CJvC12GfimDdTTn9CLEnEeWW8vdXXkeZouWLgxFGqzbPsnSShNRMsW3j3yL6ssEtjc3gwNSkbBfy");


    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress t1bin 10 0b1010101111"));
    sResult = StripQuotes(rv.write());
    BOOST_CHECK(sResult == "9XXDiTExjRZsi1ZrvptyJr8AVpMpS5hPsi9uQ3EHgkhicC4EP5MzTg7BkLkSjbgeE69V3wRyuvuoR8WdRPCK6aTcNFKcRYJopwy7BinU3");

    RewindHdSxChain(pwalletMain.get());
    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress t2hex 10 0x2AF"));
    BOOST_CHECK(sResult == StripQuotes(rv.write()));

    RewindHdSxChain(pwalletMain.get());
    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewstealthaddress t3dec 10 687"));
    BOOST_CHECK(sResult == StripQuotes(rv.write()));


    BOOST_CHECK_THROW(rv = CallRPC("getnewstealthaddress mustfail 33"), runtime_error);
    BOOST_CHECK_THROW(rv = CallRPC("getnewstealthaddress mustfail 5 wrong"), runtime_error);




    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewextaddress"));
    sResult = StripQuotes(rv.write());
}

BOOST_AUTO_TEST_CASE(rpc_hdwallet_timelocks)
{
    UniValue rv;
    std::string sResult, sTxn, sCmd;
    std::vector<std::string> vAddresses;

    BOOST_CHECK_NO_THROW(rv = CallRPC("extkeyimportmaster xprv9s21ZrQH143K3VrEYG4rhyPddr2o53qqqpCufLP6Rb3XSta2FZsqCanRJVfpTi4UX28pRaAfVGfiGpYDczv8tzTM6Qm5TRvUA9HDStbNUbQ"));

    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewaddress"));
    sResult = StripQuotes(rv.write());
    BOOST_CHECK(sResult == "PZdYWHgyhuG7NHVCzEkkx3dcLKurTpvmo6");


    CKeyID id;
    BOOST_CHECK(CBitcoinAddress(sResult).GetKeyID(id));

    CScript script = CScript() << 1487406900 << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_DUP << OP_HASH160 << ToByteVector(id) << OP_EQUALVERIFY << OP_CHECKSIG;

    CMutableTransaction txn;
    txn.nVersion = PARTICL_TXN_VERSION;
    txn.SetType(TXN_COINBASE);
    txn.nLockTime = 0;
    OUTPUT_PTR<CTxOutStandard> out0 = MAKE_OUTPUT<CTxOutStandard>();
    out0->nValue = 100;
    out0->scriptPubKey = script;
    txn.vpout.push_back(out0);



    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewaddress"));
    sResult = StripQuotes(rv.write());
    vAddresses.push_back(sResult);
    BOOST_CHECK(sResult == "PdsEywwkgVLJ8bF8b8Wp9gCj63KrXX3zww");
    BOOST_CHECK(CBitcoinAddress(sResult).GetKeyID(id));


    sTxn = "[{\"txid\":\""
        + txn.GetHash().ToString() + "\","
        + "\"vout\":0,"
        + "\"scriptPubKey\":\""+HexStr(script.begin(), script.end())+"\","
        + "\"amount\":100}]";
    sCmd = "createrawtransaction " + sTxn + " {\""+CBitcoinAddress(id).ToString()+"\":99.99}";


    BOOST_REQUIRE_NO_THROW(rv = CallRPC(sCmd));
    sResult = StripQuotes(rv.write());



    sCmd = "signrawtransactionwithwallet " + sResult + " " + sTxn;

    BOOST_REQUIRE_NO_THROW(rv = CallRPC(sCmd));
    BOOST_CHECK(rv["errors"][0]["error"].getValStr() == "Locktime requirement not satisfied");

    sTxn = "[{\"txid\":\""
        + txn.GetHash().ToString() + "\","
        + "\"vout\":0,"
        + "\"scriptPubKey\":\""+HexStr(script.begin(), script.end())+"\","
        + "\"amount\":100}]";
    sCmd = "createrawtransaction " + sTxn + " {\""+CBitcoinAddress(id).ToString()+"\":99.99}" + " 1487500000";

    BOOST_REQUIRE_NO_THROW(rv = CallRPC(sCmd));
    sResult = StripQuotes(rv.write());

    sCmd = "signrawtransactionwithwallet " + sResult + " " + sTxn;
    BOOST_REQUIRE_NO_THROW(rv = CallRPC(sCmd));
    BOOST_CHECK(rv["complete"].getBool() == true);
    sResult = rv["hex"].getValStr();



    uint32_t nSequence = 0;

    int32_t nTime = 2880 / (2 << CTxIn::SEQUENCE_LOCKTIME_GRANULARITY); // 48 hours
    nSequence |= CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG;
    nSequence |= nTime;

    script = CScript() << nSequence << OP_CHECKSEQUENCEVERIFY << OP_DROP << OP_DUP << OP_HASH160 << ToByteVector(id) << OP_EQUALVERIFY << OP_CHECKSIG;
    CScript *ps = &((CTxOutStandard*)txn.vpout[0].get())->scriptPubKey;
    BOOST_REQUIRE(ps);

    *ps = script;



    sTxn = "[{\"txid\":\""
        + txn.GetHash().ToString() + "\","
        + "\"vout\":0,"
        + "\"scriptPubKey\":\""+HexStr(script.begin(), script.end())+"\","
        + "\"amount\":100}]";
    sCmd = "createrawtransaction " + sTxn + " {\""+CBitcoinAddress(vAddresses[0]).ToString()+"\":99.99}";


    BOOST_REQUIRE_NO_THROW(rv = CallRPC(sCmd));
    sResult = StripQuotes(rv.write());


    sCmd = "signrawtransactionwithwallet " + sResult + " " + sTxn;
    BOOST_REQUIRE_NO_THROW(rv = CallRPC(sCmd));
    BOOST_CHECK(rv["errors"][0]["error"].getValStr() == "Locktime requirement not satisfied");

    sTxn = "[{\"txid\":\""
        + txn.GetHash().ToString() + "\","
        + "\"vout\":0,"
        + "\"scriptPubKey\":\""+HexStr(script.begin(), script.end())+"\","
        + "\"amount\":100,"
        +"\"sequence\":"+strprintf("%d", nSequence)+"}]";
    sCmd = "createrawtransaction " + sTxn + " {\""+CBitcoinAddress(vAddresses[0]).ToString()+"\":99.99}";



    BOOST_REQUIRE_NO_THROW(rv = CallRPC(sCmd));
    sResult = StripQuotes(rv.write());


    sCmd = "signrawtransactionwithwallet " + sResult + " " + sTxn;
    BOOST_REQUIRE_NO_THROW(rv = CallRPC(sCmd));
    BOOST_CHECK(rv["complete"].getBool() == true);
    sResult = rv["hex"].getValStr();




    BOOST_CHECK_NO_THROW(rv = CallRPC("getnewaddress"));
    std::string sAddr = StripQuotes(rv.write());
    BOOST_CHECK(sAddr == "PYWn26pQyqRE84XSWGmUBMQs67AzCRtvdG");

    // 2147483648 is > 32bit signed
    BOOST_CHECK_NO_THROW(rv = CallRPC("buildscript {\"recipe\":\"abslocktime\",\"time\":2147483648,\"addr\":\""+sAddr+"\"}"));
    BOOST_REQUIRE(rv["hex"].isStr());

    std::vector<uint8_t> vScript = ParseHex(rv["hex"].get_str());
    script = CScript(vScript.begin(), vScript.end());

    txnouttype whichType;
    BOOST_CHECK(IsStandard(script, whichType));

    opcodetype opcode;
    valtype vchPushValue;
    CScript::const_iterator pc = script.begin();
    BOOST_REQUIRE(script.GetOp(pc, opcode, vchPushValue));
    CScriptNum nTest(vchPushValue, false, 5);
    BOOST_CHECK(nTest == 2147483648);



    BOOST_CHECK_NO_THROW(rv = CallRPC("buildscript {\"recipe\":\"rellocktime\",\"time\":1447483648,\"addr\":\""+sAddr+"\"}"));
    BOOST_REQUIRE(rv["hex"].isStr());

    vScript = ParseHex(rv["hex"].get_str());
    script = CScript(vScript.begin(), vScript.end());

    BOOST_CHECK(IsStandard(script, whichType));

    pc = script.begin();
    BOOST_REQUIRE(script.GetOp(pc, opcode, vchPushValue));
    nTest = CScriptNum(vchPushValue, false, 5);
    BOOST_CHECK(nTest == 1447483648);

}

BOOST_AUTO_TEST_SUITE_END()
