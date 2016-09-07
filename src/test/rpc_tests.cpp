// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "rpcclient.h"

#include "base58.h"
#include "netbase.h"

#include "test/test_bitcoin.h"

#include <boost/atomic.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

#include <univalue.h>

#include "chain.h"
#include "chainparams.h"

using namespace std;

extern CChain chainActive;
extern boost::atomic<uint32_t> sizeForkTime;

// Helper function which returns a fresh chain of nVersion=4 blocks and ascending
// nTime at nice nPowTargetSpacing intervals, to be used as tip on chainActive.
// Individual tests can then modify that chain as they need before calling RPC
std::vector<CBlockIndex> *freshchain(const int chain_length)
{
    // vector of dummy block hashes, need to initialize with nonzero hashes
    std::vector<uint256> vHashMain(chain_length);
    // the block vector to be dynamically allocated and returned
    std::vector<CBlockIndex> *blocks;
    blocks = new std::vector<CBlockIndex>(chain_length);
    
    SelectParams(CBaseChainParams::TESTNET);
    const Consensus::Params& params = Params().GetConsensus();
    
    for (int i = 0; i < chain_length; i++)
    {
        (*blocks)[i].pprev = i ? &((*blocks)[i - 1]) : NULL;
        (*blocks)[i].nHeight = i;
        (*blocks)[i].nTime = 1461000039 + i * params.nPowTargetSpacing;
        (*blocks)[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        (*blocks)[i].nVersion = 4; /* old version, non-BIP109 */
        (*blocks)[i].nChainWork = i ? (*blocks)[i - 1].nChainWork + GetBlockProof((*blocks)[i - 1]) : arith_uint256(0);
        vHashMain[i] = ArithToUint256(i);
        (*blocks)[i].phashBlock = &vHashMain[i];
    }    
    return blocks;
}

UniValue
createArgs(int nRequired, const char* address1=NULL, const char* address2=NULL)
{
    UniValue result(UniValue::VARR);
    result.push_back(nRequired);
    UniValue addresses(UniValue::VARR);
    if (address1) addresses.push_back(address1);
    if (address2) addresses.push_back(address2);
    result.push_back(addresses);
    return result;
}

UniValue CallRPC(string args)
{
    vector<string> vArgs;
    boost::split(vArgs, args, boost::is_any_of(" \t"));
    string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    UniValue params = RPCConvertValues(strMethod, vArgs);

    rpcfn_type method = tableRPC[strMethod]->actor;
    try {
        UniValue result = (*method)(params, false);
        return result;
    }
    catch (const UniValue& objError) {
        throw runtime_error(find_value(objError, "message").get_str());
    }
}


BOOST_FIXTURE_TEST_SUITE(rpc_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(rpc_rawparams)
{
    // Test raw transaction API argument handling
    UniValue r;

    BOOST_CHECK_THROW(CallRPC("getrawtransaction"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("getrawtransaction not_hex"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("getrawtransaction a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed not_int"), runtime_error);

    BOOST_CHECK_THROW(CallRPC("createrawtransaction"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("createrawtransaction null null"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("createrawtransaction not_array"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("createrawtransaction [] []"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("createrawtransaction {} {}"), runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC("createrawtransaction [] {}"));
    BOOST_CHECK_THROW(CallRPC("createrawtransaction [] {} extra"), runtime_error);

    BOOST_CHECK_THROW(CallRPC("decoderawtransaction"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("decoderawtransaction null"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("decoderawtransaction DEADBEEF"), runtime_error);
    string rawtx = "0100000001a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff0100e1f505000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac00000000";
    BOOST_CHECK_NO_THROW(r = CallRPC(string("decoderawtransaction ")+rawtx));
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "size").get_int(), 193);
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "version").get_int(), 1);
    BOOST_CHECK_EQUAL(find_value(r.get_obj(), "locktime").get_int(), 0);
    BOOST_CHECK_THROW(r = CallRPC(string("decoderawtransaction ")+rawtx+" extra"), runtime_error);

    BOOST_CHECK_THROW(CallRPC("signrawtransaction"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("signrawtransaction null"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("signrawtransaction ff00"), runtime_error);
    BOOST_CHECK_NO_THROW(CallRPC(string("signrawtransaction ")+rawtx));
    BOOST_CHECK_NO_THROW(CallRPC(string("signrawtransaction ")+rawtx+" null null NONE|ANYONECANPAY"));
    BOOST_CHECK_NO_THROW(CallRPC(string("signrawtransaction ")+rawtx+" [] [] NONE|ANYONECANPAY"));
    BOOST_CHECK_THROW(CallRPC(string("signrawtransaction ")+rawtx+" null null badenum"), runtime_error);

    // Only check failure cases for sendrawtransaction, there's no network to send to...
    BOOST_CHECK_THROW(CallRPC("sendrawtransaction"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("sendrawtransaction null"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("sendrawtransaction DEADBEEF"), runtime_error);
    BOOST_CHECK_THROW(CallRPC(string("sendrawtransaction ")+rawtx+" extra"), runtime_error);
}

BOOST_AUTO_TEST_CASE(rpc_rawsign)
{
    UniValue r;
    // input is a 1-of-2 multisig (so is output):
    string prevout =
      "[{\"txid\":\"b4cc287e58f87cdae59417329f710f3ecd75a4ee1d2872b7248f50977c8493f3\","
      "\"vout\":1,\"scriptPubKey\":\"a914b10c9df5f7edf436c697f02f1efdba4cf399615187\","
      "\"redeemScript\":\"512103debedc17b3df2badbcdd86d5feb4562b86fe182e5998abd8bcd4f122c6155b1b21027e940bb73ab8732bfdf7f9216ecefca5b94d6df834e77e108f68e66f126044c052ae\"}]";
    r = CallRPC(string("createrawtransaction ")+prevout+" "+
      "{\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\":11}");
    string notsigned = r.get_str();
    string privkey1 = "\"KzsXybp9jX64P5ekX1KUxRQ79Jht9uzW7LorgwE65i5rWACL6LQe\"";
    string privkey2 = "\"Kyhdf5LuKTRx4ge69ybABsiUAWjVRK4XGxAKk2FQLp2HjGMy87Z4\"";
    r = CallRPC(string("signrawtransaction ")+notsigned+" "+prevout+" "+"[]");
    BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);
    r = CallRPC(string("signrawtransaction ")+notsigned+" "+prevout+" "+"["+privkey1+","+privkey2+"]");
    BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);

    // again for a v4 transaction.
    r = CallRPC(string("createrawtransaction ")+prevout+" {\"3HqAe9LtNBjnsfM4CyYaWTnvCaUYT7v4oZ\":11}	0	4");
    notsigned = r.get_str();
    BOOST_CHECK(notsigned.size() > 8);
    BOOST_CHECK_EQUAL(notsigned.substr(0, 8), "04000000"); // make sure we actually got a v4 tx
    r = CallRPC(string("signrawtransaction ")+notsigned+" "+prevout+" "+"[]");
    BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == false);
    r = CallRPC(string("signrawtransaction ")+notsigned+" "+prevout+" "+"["+privkey1+","+privkey2+"]");
    BOOST_CHECK(find_value(r.get_obj(), "complete").get_bool() == true);
}

BOOST_AUTO_TEST_CASE(rpc_createraw_op_return)
{
    BOOST_CHECK_NO_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"68656c6c6f776f726c64\"}"));

    // Allow more than one data transaction output
    BOOST_CHECK_NO_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"68656c6c6f776f726c64\",\"data\":\"68656c6c6f776f726c64\"}"));

    // Key not "data" (bad address)
    BOOST_CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"somedata\":\"68656c6c6f776f726c64\"}"), runtime_error);

    // Bad hex encoding of data output
    BOOST_CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"12345\"}"), runtime_error);
    BOOST_CHECK_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"12345g\"}"), runtime_error);

    // Data 81 bytes long
    BOOST_CHECK_NO_THROW(CallRPC("createrawtransaction [{\"txid\":\"a3b807410df0b60fcb9736768df5823938b2f838694939ba45f3c0a1bff150ed\",\"vout\":0}] {\"data\":\"010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081\"}"));
}

BOOST_AUTO_TEST_CASE(rpc_format_monetary_values)
{
    BOOST_CHECK(ValueFromAmount(0LL).write() == "0.00000000");
    BOOST_CHECK(ValueFromAmount(1LL).write() == "0.00000001");
    BOOST_CHECK(ValueFromAmount(17622195LL).write() == "0.17622195");
    BOOST_CHECK(ValueFromAmount(50000000LL).write() == "0.50000000");
    BOOST_CHECK(ValueFromAmount(89898989LL).write() == "0.89898989");
    BOOST_CHECK(ValueFromAmount(100000000LL).write() == "1.00000000");
    BOOST_CHECK(ValueFromAmount(2099999999999990LL).write() == "20999999.99999990");
    BOOST_CHECK(ValueFromAmount(2099999999999999LL).write() == "20999999.99999999");

    BOOST_CHECK_EQUAL(ValueFromAmount(0).write(), "0.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount((COIN/10000)*123456789).write(), "12345.67890000");
    BOOST_CHECK_EQUAL(ValueFromAmount(-COIN).write(), "-1.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(-COIN/10).write(), "-0.10000000");

    BOOST_CHECK_EQUAL(ValueFromAmount(COIN*100000000).write(), "100000000.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN*10000000).write(), "10000000.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN*1000000).write(), "1000000.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN*100000).write(), "100000.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN*10000).write(), "10000.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN*1000).write(), "1000.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN*100).write(), "100.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN*10).write(), "10.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN).write(), "1.00000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN/10).write(), "0.10000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN/100).write(), "0.01000000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN/1000).write(), "0.00100000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN/10000).write(), "0.00010000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN/100000).write(), "0.00001000");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN/1000000).write(), "0.00000100");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN/10000000).write(), "0.00000010");
    BOOST_CHECK_EQUAL(ValueFromAmount(COIN/100000000).write(), "0.00000001");
}

static UniValue ValueFromString(const std::string &str)
{
    UniValue value;
    BOOST_CHECK(value.setNumStr(str));
    return value;
}

BOOST_AUTO_TEST_CASE(rpc_parse_monetary_values)
{
    BOOST_CHECK_THROW(AmountFromValue(ValueFromString("-0.00000001")), UniValue);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0")), 0LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.00000000")), 0LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.00000001")), 1LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.17622195")), 17622195LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.5")), 50000000LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.50000000")), 50000000LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.89898989")), 89898989LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("1.00000000")), 100000000LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("20999999.9999999")), 2099999999999990LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("20999999.99999999")), 2099999999999999LL);

    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("1e-8")), COIN/100000000);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.1e-7")), COIN/100000000);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.01e-6")), COIN/100000000);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.0000000000000000000000000000000000000000000000000000000000000000000000000001e+68")), COIN/100000000);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("10000000000000000000000000000000000000000000000000000000000000000e-64")), COIN);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000e64")), COIN);

    BOOST_CHECK_THROW(AmountFromValue(ValueFromString("1e-9")), UniValue); //should fail
    BOOST_CHECK_THROW(AmountFromValue(ValueFromString("0.000000019")), UniValue); //should fail
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.00000001000000")), 1LL); //should pass, cut trailing 0
    BOOST_CHECK_THROW(AmountFromValue(ValueFromString("19e-9")), UniValue); //should fail
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.19e-6")), 19); //should pass, leading 0 is present

    BOOST_CHECK_THROW(AmountFromValue(ValueFromString("92233720368.54775808")), UniValue); //overflow error
    BOOST_CHECK_THROW(AmountFromValue(ValueFromString("1e+11")), UniValue); //overflow error
    BOOST_CHECK_THROW(AmountFromValue(ValueFromString("1e11")), UniValue); //overflow error signless
    BOOST_CHECK_THROW(AmountFromValue(ValueFromString("93e+9")), UniValue); //overflow error
}

BOOST_AUTO_TEST_CASE(json_parse_errors)
{
    // Valid
    BOOST_CHECK_EQUAL(ParseNonRFCJSONValue("1.0").get_real(), 1.0);
    // Valid, with leading or trailing whitespace
    BOOST_CHECK_EQUAL(ParseNonRFCJSONValue(" 1.0").get_real(), 1.0);
    BOOST_CHECK_EQUAL(ParseNonRFCJSONValue("1.0 ").get_real(), 1.0);

    BOOST_CHECK_THROW(AmountFromValue(ParseNonRFCJSONValue(".19e-6")), std::runtime_error); //should fail, missing leading 0, therefore invalid JSON
    BOOST_CHECK_EQUAL(AmountFromValue(ParseNonRFCJSONValue("0.00000000000000000000000000000000000001e+30 ")), 1);
    // Invalid, initial garbage
    BOOST_CHECK_THROW(ParseNonRFCJSONValue("[1.0"), std::runtime_error);
    BOOST_CHECK_THROW(ParseNonRFCJSONValue("a1.0"), std::runtime_error);
    // Invalid, trailing garbage
    BOOST_CHECK_THROW(ParseNonRFCJSONValue("1.0sds"), std::runtime_error);
    BOOST_CHECK_THROW(ParseNonRFCJSONValue("1.0]"), std::runtime_error);
    // BTC addresses should fail parsing
    BOOST_CHECK_THROW(ParseNonRFCJSONValue("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"), std::runtime_error);
    BOOST_CHECK_THROW(ParseNonRFCJSONValue("3J98t1WpEZ73CNmQviecrnyiWrnqRhWNL"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(rpc_ban)
{
    BOOST_CHECK_NO_THROW(CallRPC(string("clearbanned")));
    
    UniValue r;
    BOOST_CHECK_NO_THROW(r = CallRPC(string("setban 127.0.0.0 add")));
    BOOST_CHECK_THROW(r = CallRPC(string("setban 127.0.0.0:8334")), runtime_error); //portnumber for setban not allowed
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    UniValue ar = r.get_array();
    UniValue o1 = ar[0].get_obj();
    UniValue adr = find_value(o1, "address");
    BOOST_CHECK_EQUAL(adr.get_str(), "127.0.0.0/32");
    BOOST_CHECK_NO_THROW(CallRPC(string("setban 127.0.0.0 remove")));;
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    ar = r.get_array();
    BOOST_CHECK_EQUAL(ar.size(), 0);

    BOOST_CHECK_NO_THROW(r = CallRPC(string("setban 127.0.0.0/24 add 1607731200 true")));
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = find_value(o1, "address");
    UniValue banned_until = find_value(o1, "banned_until");
    BOOST_CHECK_EQUAL(adr.get_str(), "127.0.0.0/24");
    BOOST_CHECK_EQUAL(banned_until.get_int64(), 1607731200); // absolute time check

    BOOST_CHECK_NO_THROW(CallRPC(string("clearbanned")));

    BOOST_CHECK_NO_THROW(r = CallRPC(string("setban 127.0.0.0/24 add 200")));
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = find_value(o1, "address");
    banned_until = find_value(o1, "banned_until");
    BOOST_CHECK_EQUAL(adr.get_str(), "127.0.0.0/24");
    int64_t now = GetTime();    
    BOOST_CHECK(banned_until.get_int64() > now);
    BOOST_CHECK(banned_until.get_int64()-now <= 200);

    // must throw an exception because 127.0.0.1 is in already banned suubnet range
    BOOST_CHECK_THROW(r = CallRPC(string("setban 127.0.0.1 add")), runtime_error);

    BOOST_CHECK_NO_THROW(CallRPC(string("setban 127.0.0.0/24 remove")));;
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    ar = r.get_array();
    BOOST_CHECK_EQUAL(ar.size(), 0);

    BOOST_CHECK_NO_THROW(r = CallRPC(string("setban 127.0.0.0/255.255.0.0 add")));
    BOOST_CHECK_THROW(r = CallRPC(string("setban 127.0.1.1 add")), runtime_error);

    BOOST_CHECK_NO_THROW(CallRPC(string("clearbanned")));
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    ar = r.get_array();
    BOOST_CHECK_EQUAL(ar.size(), 0);


    BOOST_CHECK_THROW(r = CallRPC(string("setban test add")), runtime_error); //invalid IP

    //IPv6 tests
    BOOST_CHECK_NO_THROW(r = CallRPC(string("setban FE80:0000:0000:0000:0202:B3FF:FE1E:8329 add")));
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = find_value(o1, "address");
    BOOST_CHECK_EQUAL(adr.get_str(), "fe80::202:b3ff:fe1e:8329/128");

    BOOST_CHECK_NO_THROW(CallRPC(string("clearbanned")));
    BOOST_CHECK_NO_THROW(r = CallRPC(string("setban 2001:db8::/ffff:fffc:0:0:0:0:0:0 add")));
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = find_value(o1, "address");
    BOOST_CHECK_EQUAL(adr.get_str(), "2001:db8::/30");

    BOOST_CHECK_NO_THROW(CallRPC(string("clearbanned")));
    BOOST_CHECK_NO_THROW(r = CallRPC(string("setban 2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/128 add")));
    BOOST_CHECK_NO_THROW(r = CallRPC(string("listbanned")));
    ar = r.get_array();
    o1 = ar[0].get_obj();
    adr = find_value(o1, "address");
    BOOST_CHECK_EQUAL(adr.get_str(), "2001:4d48:ac57:400:cacf:e9ff:fe1d:9c63/128");
}


BOOST_AUTO_TEST_CASE(rpc_getblockchaininfo)
{
    // test bip109 counting

    // use testnet params for faster testing (smaller window size)
    SelectParams(CBaseChainParams::TESTNET);
    const Consensus::Params& params = Params().GetConsensus();
    // we need 1 more than the size of the majority window because
    // we want to test the lower boundary (just outside window)
    const int chain_len = params.nMajorityWindow + 1;
    // use reduced length chain for some tests where window is not
    // needed
    const int min_chain_len = 3;
    
    // ensure that the hard fork is not yet seen as active
    // otherwise the global pblocktree must be set up right,
    // or else an assert will trip in HardForkMajorityDesc
    sizeForkTime.store(std::numeric_limits<uint32_t>::max());
    
    ///////////////////////////////////////////////////////////////////////////
    // case 1: a block history with only version 4 blocks (zero bip109 found)
    std::vector<CBlockIndex> *testblocks;
    testblocks = freshchain(min_chain_len);
    chainActive.SetTip(&(*testblocks)[min_chain_len-1]);
    UniValue r;
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    UniValue o1 = r.get_obj();
    UniValue hf = find_value(o1, "hardforks");
    UniValue hfa = hf.get_array();
    BOOST_CHECK_EQUAL(hfa.size(), 1);
    UniValue bip109_obj = hfa[0].get_obj();
    UniValue bip109_status_obj = find_value(bip109_obj, "status");
    UniValue bip109_found = find_value(bip109_status_obj, "found");
    BOOST_CHECK_EQUAL(bip109_found.get_int(), 0);
    delete testblocks;

    ///////////////////////////////////////////////////////////////////////////
    // case 2: a block history with only version 4 blocks in the window, but 
    // one bip109 block before that (at genesis, outside window, zero bip109)
    testblocks = freshchain(chain_len);
    (*testblocks)[0].nVersion = BASE_VERSION + FORK_BIT_2MB;
    chainActive.SetTip(&(*testblocks)[chain_len-1]);
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    o1 = r.get_obj();
    hf = find_value(o1, "hardforks");
    hfa = hf.get_array();
    BOOST_CHECK_EQUAL(hfa.size(), 1);
    bip109_obj = hfa[0].get_obj();
    bip109_status_obj = find_value(bip109_obj, "status");
    bip109_found = find_value(bip109_status_obj, "found");
    BOOST_CHECK_EQUAL(bip109_found.get_int(), 0);
    delete testblocks;
    
    ///////////////////////////////////////////////////////////////////////////
    // case 3: a full-window bip109-block history with last block beyond 
    // expiry date (results in no 'hardforks' entry in the RPC output)
    testblocks = freshchain(chain_len);
    // add BIP109 blocks, one short of the required majority
    for (int i = 1; i <= chain_len-1; i++) {
        (*testblocks)[i].nVersion = BASE_VERSION + FORK_BIT_2MB;
    }
    // timestamp the last block as expired w.r.t. BIP109
    (*testblocks)[chain_len-1].nTime = params.nSizeForkExpiration + 1;
    chainActive.SetTip(&(*testblocks)[chain_len-1]);
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    o1 = r.get_obj();
    hf = find_value(o1, "hardforks");
    UniValue bip109 = find_value(hf, "bip109");
    BOOST_CHECK(bip109.getType() == UniValue::VNULL);
    delete testblocks;

    ///////////////////////////////////////////////////////////////////////////
    // case 4: a block history with (majority-1) BIP9 (0x200000) blocks followed
    //         by the remainder BIP109 blocks, all in window
    testblocks = freshchain(chain_len);
    // add BASE_VERSION (BIP9) blocks, one short of the required majority
    for (int i = 1; i <= params.nActivateSizeForkMajority-1; i++) {
        (*testblocks)[i].nVersion = BASE_VERSION;
    }
    for (int i = params.nActivateSizeForkMajority; i <= chain_len-1; i++) {
        (*testblocks)[i].nVersion = BASE_VERSION + FORK_BIT_2MB;
    }
    chainActive.SetTip(&(*testblocks)[chain_len-1]);
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    o1 = r.get_obj();
    hf = find_value(o1, "hardforks");
    hfa = hf.get_array();
    BOOST_CHECK_EQUAL(hfa.size(), 1);
    bip109_obj = hfa[0].get_obj();
    bip109_status_obj = find_value(bip109_obj, "status");
    bip109_found = find_value(bip109_status_obj, "found");
    BOOST_CHECK_EQUAL(bip109_found.get_int(), chain_len-params.nActivateSizeForkMajority);
    delete testblocks;
    
    ///////////////////////////////////////////////////////////////////////////
    // case 5: a v4-block history with one bip109 block at lower end (height 1)
    testblocks = freshchain(chain_len);
    (*testblocks)[1].nVersion = BASE_VERSION + FORK_BIT_2MB;
    chainActive.SetTip(&(*testblocks)[chain_len-1]);
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    o1 = r.get_obj();
    hf = find_value(o1, "hardforks");
    hfa = hf.get_array();
    BOOST_CHECK_EQUAL(hfa.size(), 1);
    bip109_obj = hfa[0].get_obj();
    bip109_status_obj = find_value(bip109_obj, "status");
    bip109_found = find_value(bip109_status_obj, "found");
    BOOST_CHECK_EQUAL(bip109_found.get_int(), 1);
    delete testblocks;

    ///////////////////////////////////////////////////////////////////////////
    // case 6: a v4-block history with one bip109 block at tip
    testblocks = freshchain(chain_len);
    (*testblocks)[testblocks->size()-1].nVersion = BASE_VERSION + FORK_BIT_2MB;
    chainActive.SetTip(&(*testblocks)[chain_len-1]);
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    o1 = r.get_obj();
    hf = find_value(o1, "hardforks");
    BOOST_CHECK_EQUAL(1, 1);
    hfa = hf.get_array();
    BOOST_CHECK_EQUAL(hfa.size(), 1);
    bip109_obj = hfa[0].get_obj();
    bip109_status_obj = find_value(bip109_obj, "status");
    bip109_found = find_value(bip109_status_obj, "found");
    BOOST_CHECK_EQUAL(bip109_found.get_int(), 1);
    delete testblocks;

    ///////////////////////////////////////////////////////////////////////////
    // case 7: full majority of bip109 blocks starting at lower end of window
    testblocks = freshchain(chain_len);
    for (int i = 1; i <= params.nActivateSizeForkMajority; i++) {
        (*testblocks)[i].nVersion = BASE_VERSION + FORK_BIT_2MB;
    }
    chainActive.SetTip(&(*testblocks)[chain_len-1]);
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    o1 = r.get_obj();
    hf = find_value(o1, "hardforks");
    BOOST_CHECK_EQUAL(1, 1);
    hfa = hf.get_array();
    BOOST_CHECK_EQUAL(hfa.size(), 1);
    bip109_obj = hfa[0].get_obj();
    bip109_status_obj = find_value(bip109_obj, "status");
    bip109_found = find_value(bip109_status_obj, "found");
    BOOST_CHECK_EQUAL(bip109_found.get_int(), params.nActivateSizeForkMajority);
    delete testblocks;
    
    ///////////////////////////////////////////////////////////////////////////
    // case 8: full window of bip109 blocks (exceeding majority)
    testblocks = freshchain(chain_len);
    for (int i = 1; i <= params.nMajorityWindow; i++) {
        (*testblocks)[i].nVersion = BASE_VERSION + FORK_BIT_2MB;
    }
    chainActive.SetTip(&(*testblocks)[chain_len-1]);
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    o1 = r.get_obj();
    hf = find_value(o1, "hardforks");
    BOOST_CHECK_EQUAL(1, 1);
    hfa = hf.get_array();
    BOOST_CHECK_EQUAL(hfa.size(), 1);
    bip109_obj = hfa[0].get_obj();
    bip109_status_obj = find_value(bip109_obj, "status");
    bip109_found = find_value(bip109_status_obj, "found");
    BOOST_CHECK_EQUAL(bip109_found.get_int(), params.nMajorityWindow);
    delete testblocks;
    
    ///////////////////////////////////////////////////////////////////////////
    // case 9: full window of blocks with version > 0x30000000 - all of them should count
    testblocks = freshchain(chain_len);
    for (int i = 1; i <= params.nMajorityWindow; i++) {
        (*testblocks)[i].nVersion = BASE_VERSION + FORK_BIT_2MB + i % 10;
    }
    chainActive.SetTip(&(*testblocks)[chain_len-1]);
    BOOST_CHECK_NO_THROW(r = CallRPC(string("getblockchaininfo")));
    o1 = r.get_obj();
    hf = find_value(o1, "hardforks");
    BOOST_CHECK_EQUAL(1, 1);
    hfa = hf.get_array();
    BOOST_CHECK_EQUAL(hfa.size(), 1);
    bip109_obj = hfa[0].get_obj();
    bip109_status_obj = find_value(bip109_obj, "status");
    bip109_found = find_value(bip109_status_obj, "found");
    BOOST_CHECK_EQUAL(bip109_found.get_int(), params.nMajorityWindow);
    delete testblocks;
}

BOOST_AUTO_TEST_SUITE_END()
