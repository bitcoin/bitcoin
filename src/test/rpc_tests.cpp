// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "rpcclient.h"

#include "base58.h"
#include "netbase.h"

#include <boost/algorithm/string.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace json_spirit;

Array
createArgs(int nRequired, const char* address1=NULL, const char* address2=NULL)
{
    Array result;
    result.push_back(nRequired);
    Array addresses;
    if (address1) addresses.push_back(address1);
    if (address2) addresses.push_back(address2);
    result.push_back(addresses);
    return result;
}

Value CallRPC(string args)
{
    vector<string> vArgs;
    boost::split(vArgs, args, boost::is_any_of(" \t"));
    string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    Array params = RPCConvertValues(strMethod, vArgs);

    rpcfn_type method = tableRPC[strMethod]->actor;
    try {
        Value result = (*method)(params, false);
        return result;
    }
    catch (Object& objError)
    {
        throw runtime_error(find_value(objError, "message").get_str());
    }
}


BOOST_AUTO_TEST_SUITE(rpc_tests)

BOOST_AUTO_TEST_CASE(rpc_rawparams)
{
    // Test raw transaction API argument handling
    Value r;

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
    Value r;
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
}

BOOST_AUTO_TEST_CASE(rpc_format_monetary_values)
{
    BOOST_CHECK_EQUAL(write_string(ValueFromAmount(0LL), false), "0.00000000");
    BOOST_CHECK_EQUAL(write_string(ValueFromAmount(1LL), false), "0.00000001");
    BOOST_CHECK_EQUAL(write_string(ValueFromAmount(17622195LL), false), "0.17622195");
    BOOST_CHECK_EQUAL(write_string(ValueFromAmount(50000000LL), false), "0.50000000");
    BOOST_CHECK_EQUAL(write_string(ValueFromAmount(89898989LL), false), "0.89898989");
    BOOST_CHECK_EQUAL(write_string(ValueFromAmount(100000000LL), false), "1.00000000");
    BOOST_CHECK_EQUAL(write_string(ValueFromAmount(2099999999999990LL), false), "20999999.99999990");
    BOOST_CHECK_EQUAL(write_string(ValueFromAmount(2099999999999999LL), false), "20999999.99999999");
}

static Value ValueFromString(const std::string &str)
{
    Value value;
    BOOST_CHECK(read_string(str, value));
    return value;
}

BOOST_AUTO_TEST_CASE(rpc_parse_monetary_values)
{
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.00000001")), 1LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.17622195")), 17622195LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.5")), 50000000LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.50000000")), 50000000LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("0.89898989")), 89898989LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("1.00000000")), 100000000LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("20999999.9999999")), 2099999999999990LL);
    BOOST_CHECK_EQUAL(AmountFromValue(ValueFromString("20999999.99999999")), 2099999999999999LL);
}

BOOST_AUTO_TEST_CASE(rpc_boostasiotocnetaddr)
{
    // Check IPv4 addresses
    BOOST_CHECK_EQUAL(BoostAsioToCNetAddr(boost::asio::ip::address::from_string("1.2.3.4")).ToString(), "1.2.3.4");
    BOOST_CHECK_EQUAL(BoostAsioToCNetAddr(boost::asio::ip::address::from_string("127.0.0.1")).ToString(), "127.0.0.1");
    // Check IPv6 addresses
    BOOST_CHECK_EQUAL(BoostAsioToCNetAddr(boost::asio::ip::address::from_string("::1")).ToString(), "::1");
    BOOST_CHECK_EQUAL(BoostAsioToCNetAddr(boost::asio::ip::address::from_string("123:4567:89ab:cdef:123:4567:89ab:cdef")).ToString(),
                                         "123:4567:89ab:cdef:123:4567:89ab:cdef");
    // v4 compatible must be interpreted as IPv4
    BOOST_CHECK_EQUAL(BoostAsioToCNetAddr(boost::asio::ip::address::from_string("::0:127.0.0.1")).ToString(), "127.0.0.1");
    // v4 mapped must be interpreted as IPv4
    BOOST_CHECK_EQUAL(BoostAsioToCNetAddr(boost::asio::ip::address::from_string("::ffff:127.0.0.1")).ToString(), "127.0.0.1");
}

BOOST_AUTO_TEST_CASE(rpc_decoderawtransaction_scriptPubKey)
{
    Value result;
    Value scriptPubKeyAsm;
    string tx;
    Value txid;
    // this test case uses a main net transaction that has both P2PKH and P2SH outputs. it's from james d'angelo's awesome video about multisig -- https://www.youtube.com/watch?v=zIbUSaZBJgU.
    tx = std::string("010000000214e74c8aa195fd544715fb4401008a7e139b23804e8c07b6bb538989270ebbdf000000008b48304502205192c9c285f47be0f0dc34cdfb6e2093b0b09d58cb361e7a996ce6d9d5bda1c2022100e82562a42ffb27e9da98eb086f5376efc3eb534b2bbee410dd1436406be2613501410460f8395ff5b0db8cac5a471d11066fb1598411153b04d76984b7c36bc01dc0699b83d995333dc513d082f423c20a4b2d1a6753daccecbf9ee01cb15af97c9fc2ffffffffcfcd79a7e8c838a08587ab9347dce1a0420a43ca4cae73b2e1b7d797b46e2a07010000008b483045022001ba3879426b190c6bf82adc91af4f58eecd27c61c6603604de5f4c1c0052f59022100e3af7e854e437fa57cf2081ab67911063d9fbd78a9050687df4da9dafeb538ba014104983fd4a1ffa71bfe7dfd623b39ba27a735b6ceb70931a25acdad529cf06d7ab454cc47bb81e1da0a0d00cb00d4085b75a1f46ef84ffe566cdc6624709c2c56dfffffffff02a45e02000000000017a9142a5edea39971049a540474c6a99edf0aa4074c58874b920000000000001976a914b9c670c7aa955c9808af7eeb9643b9fd6285cbf588ac00000000");
    BOOST_CHECK_NO_THROW(result = CallRPC(string("decoderawtransaction ") + tx));
    txid = find_value(result.get_obj(), "txid");
    BOOST_CHECK_EQUAL(txid.get_str(), "73926d12845ecf0dbb51abe0d64a1e11deb385dc58df2d7e2c6ed0ab35561f8d");
    scriptPubKeyAsm = find_value(find_value(find_value(result.get_obj(), "vout").get_array().at(0).get_obj(), "scriptPubKey").get_obj(), "asm");
    BOOST_CHECK_EQUAL(scriptPubKeyAsm.get_str(), "HASH160 2a5edea39971049a540474c6a99edf0aa4074c58 EQUAL");
    scriptPubKeyAsm = find_value(find_value(find_value(result.get_obj(), "vout").get_array().at(1).get_obj(), "scriptPubKey").get_obj(), "asm");
    BOOST_CHECK_EQUAL(scriptPubKeyAsm.get_str(), "DUP HASH160 b9c670c7aa955c9808af7eeb9643b9fd6285cbf5 EQUALVERIFY CHECKSIG");
}

BOOST_AUTO_TEST_SUITE_END()
