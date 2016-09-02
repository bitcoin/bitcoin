// Copyright 2014 BitPay, Inc.
// Copyright (c) 2014-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <univalue.h>
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_FIXTURE_TEST_SUITE(univalue_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(univalue_constructor)
{
    UniValue v1;
    FAST_CHECK(v1.isNull());

    UniValue v2(UniValue::VSTR);
    FAST_CHECK(v2.isStr());

    UniValue v3(UniValue::VSTR, "foo");
    FAST_CHECK(v3.isStr());
    FAST_CHECK_EQUAL(v3.getValStr(), "foo");

    UniValue numTest;
    FAST_CHECK(numTest.setNumStr("82"));
    FAST_CHECK(numTest.isNum());
    FAST_CHECK_EQUAL(numTest.getValStr(), "82");

    uint64_t vu64 = 82;
    UniValue v4(vu64);
    FAST_CHECK(v4.isNum());
    FAST_CHECK_EQUAL(v4.getValStr(), "82");

    int64_t vi64 = -82;
    UniValue v5(vi64);
    FAST_CHECK(v5.isNum());
    FAST_CHECK_EQUAL(v5.getValStr(), "-82");

    int vi = -688;
    UniValue v6(vi);
    FAST_CHECK(v6.isNum());
    FAST_CHECK_EQUAL(v6.getValStr(), "-688");

    double vd = -7.21;
    UniValue v7(vd);
    FAST_CHECK(v7.isNum());
    FAST_CHECK_EQUAL(v7.getValStr(), "-7.21");

    string vs("yawn");
    UniValue v8(vs);
    FAST_CHECK(v8.isStr());
    FAST_CHECK_EQUAL(v8.getValStr(), "yawn");

    const char *vcs = "zappa";
    UniValue v9(vcs);
    FAST_CHECK(v9.isStr());
    FAST_CHECK_EQUAL(v9.getValStr(), "zappa");
}

BOOST_AUTO_TEST_CASE(univalue_typecheck)
{
    UniValue v1;
    FAST_CHECK(v1.setNumStr("1"));
    FAST_CHECK(v1.isNum());
    FAST_CHECK_THROW(v1.get_bool(), runtime_error);

    UniValue v2;
    FAST_CHECK(v2.setBool(true));
    FAST_CHECK_EQUAL(v2.get_bool(), true);
    FAST_CHECK_THROW(v2.get_int(), runtime_error);

    UniValue v3;
    FAST_CHECK(v3.setNumStr("32482348723847471234"));
    FAST_CHECK_THROW(v3.get_int64(), runtime_error);
    FAST_CHECK(v3.setNumStr("1000"));
    FAST_CHECK_EQUAL(v3.get_int64(), 1000);

    UniValue v4;
    FAST_CHECK(v4.setNumStr("2147483648"));
    FAST_CHECK_EQUAL(v4.get_int64(), 2147483648);
    FAST_CHECK_THROW(v4.get_int(), runtime_error);
    FAST_CHECK(v4.setNumStr("1000"));
    FAST_CHECK_EQUAL(v4.get_int(), 1000);
    FAST_CHECK_THROW(v4.get_str(), runtime_error);
    FAST_CHECK_EQUAL(v4.get_real(), 1000);
    FAST_CHECK_THROW(v4.get_array(), runtime_error);
    FAST_CHECK_THROW(v4.getKeys(), runtime_error);
    FAST_CHECK_THROW(v4.getValues(), runtime_error);
    FAST_CHECK_THROW(v4.get_obj(), runtime_error);

    UniValue v5;
    FAST_CHECK(v5.read("[true, 10]"));
    FAST_CHECK_NO_THROW(v5.get_array());
    std::vector<UniValue> vals = v5.getValues();
    FAST_CHECK_THROW(vals[0].get_int(), runtime_error);
    FAST_CHECK_EQUAL(vals[0].get_bool(), true);

    FAST_CHECK_EQUAL(vals[1].get_int(), 10);
    FAST_CHECK_THROW(vals[1].get_bool(), runtime_error);
}

BOOST_AUTO_TEST_CASE(univalue_set)
{
    UniValue v(UniValue::VSTR, "foo");
    v.clear();
    FAST_CHECK(v.isNull());
    FAST_CHECK_EQUAL(v.getValStr(), "");

    FAST_CHECK(v.setObject());
    FAST_CHECK(v.isObject());
    FAST_CHECK_EQUAL(v.size(), 0);
    FAST_CHECK_EQUAL(v.getType(), UniValue::VOBJ);
    FAST_CHECK(v.empty());

    FAST_CHECK(v.setArray());
    FAST_CHECK(v.isArray());
    FAST_CHECK_EQUAL(v.size(), 0);

    FAST_CHECK(v.setStr("zum"));
    FAST_CHECK(v.isStr());
    FAST_CHECK_EQUAL(v.getValStr(), "zum");

    FAST_CHECK(v.setFloat(-1.01));
    FAST_CHECK(v.isNum());
    FAST_CHECK_EQUAL(v.getValStr(), "-1.01");

    FAST_CHECK(v.setInt((int)1023));
    FAST_CHECK(v.isNum());
    FAST_CHECK_EQUAL(v.getValStr(), "1023");

    FAST_CHECK(v.setInt((int64_t)-1023LL));
    FAST_CHECK(v.isNum());
    FAST_CHECK_EQUAL(v.getValStr(), "-1023");

    FAST_CHECK(v.setInt((uint64_t)1023ULL));
    FAST_CHECK(v.isNum());
    FAST_CHECK_EQUAL(v.getValStr(), "1023");

    FAST_CHECK(v.setNumStr("-688"));
    FAST_CHECK(v.isNum());
    FAST_CHECK_EQUAL(v.getValStr(), "-688");

    FAST_CHECK(v.setBool(false));
    FAST_CHECK_EQUAL(v.isBool(), true);
    FAST_CHECK_EQUAL(v.isTrue(), false);
    FAST_CHECK_EQUAL(v.isFalse(), true);
    FAST_CHECK_EQUAL(v.getBool(), false);

    FAST_CHECK(v.setBool(true));
    FAST_CHECK_EQUAL(v.isBool(), true);
    FAST_CHECK_EQUAL(v.isTrue(), true);
    FAST_CHECK_EQUAL(v.isFalse(), false);
    FAST_CHECK_EQUAL(v.getBool(), true);

    FAST_CHECK(!v.setNumStr("zombocom"));

    FAST_CHECK(v.setNull());
    FAST_CHECK(v.isNull());
}

BOOST_AUTO_TEST_CASE(univalue_array)
{
    UniValue arr(UniValue::VARR);

    UniValue v((int64_t)1023LL);
    FAST_CHECK(arr.push_back(v));

    string vStr("zippy");
    FAST_CHECK(arr.push_back(vStr));

    const char *s = "pippy";
    FAST_CHECK(arr.push_back(s));

    vector<UniValue> vec;
    v.setStr("boing");
    vec.push_back(v);

    v.setStr("going");
    vec.push_back(v);

    FAST_CHECK(arr.push_backV(vec));

    FAST_CHECK_EQUAL(arr.empty(), false);
    FAST_CHECK_EQUAL(arr.size(), 5);

    FAST_CHECK_EQUAL(arr[0].getValStr(), "1023");
    FAST_CHECK_EQUAL(arr[1].getValStr(), "zippy");
    FAST_CHECK_EQUAL(arr[2].getValStr(), "pippy");
    FAST_CHECK_EQUAL(arr[3].getValStr(), "boing");
    FAST_CHECK_EQUAL(arr[4].getValStr(), "going");

    FAST_CHECK_EQUAL(arr[999].getValStr(), "");

    arr.clear();
    FAST_CHECK(arr.empty());
    FAST_CHECK_EQUAL(arr.size(), 0);
}

BOOST_AUTO_TEST_CASE(univalue_object)
{
    UniValue obj(UniValue::VOBJ);
    string strKey, strVal;
    UniValue v;

    strKey = "age";
    v.setInt(100);
    FAST_CHECK(obj.pushKV(strKey, v));

    strKey = "first";
    strVal = "John";
    FAST_CHECK(obj.pushKV(strKey, strVal));

    strKey = "last";
    const char *cVal = "Smith";
    FAST_CHECK(obj.pushKV(strKey, cVal));

    strKey = "distance";
    FAST_CHECK(obj.pushKV(strKey, (int64_t) 25));

    strKey = "time";
    FAST_CHECK(obj.pushKV(strKey, (uint64_t) 3600));

    strKey = "calories";
    FAST_CHECK(obj.pushKV(strKey, (int) 12));

    strKey = "temperature";
    FAST_CHECK(obj.pushKV(strKey, (double) 90.012));

    UniValue obj2(UniValue::VOBJ);
    FAST_CHECK(obj2.pushKV("cat1", 9000));
    FAST_CHECK(obj2.pushKV("cat2", 12345));

    FAST_CHECK(obj.pushKVs(obj2));

    FAST_CHECK_EQUAL(obj.empty(), false);
    FAST_CHECK_EQUAL(obj.size(), 9);

    FAST_CHECK_EQUAL(obj["age"].getValStr(), "100");
    FAST_CHECK_EQUAL(obj["first"].getValStr(), "John");
    FAST_CHECK_EQUAL(obj["last"].getValStr(), "Smith");
    FAST_CHECK_EQUAL(obj["distance"].getValStr(), "25");
    FAST_CHECK_EQUAL(obj["time"].getValStr(), "3600");
    FAST_CHECK_EQUAL(obj["calories"].getValStr(), "12");
    FAST_CHECK_EQUAL(obj["temperature"].getValStr(), "90.012");
    FAST_CHECK_EQUAL(obj["cat1"].getValStr(), "9000");
    FAST_CHECK_EQUAL(obj["cat2"].getValStr(), "12345");

    FAST_CHECK_EQUAL(obj["nyuknyuknyuk"].getValStr(), "");

    FAST_CHECK(obj.exists("age"));
    FAST_CHECK(obj.exists("first"));
    FAST_CHECK(obj.exists("last"));
    FAST_CHECK(obj.exists("distance"));
    FAST_CHECK(obj.exists("time"));
    FAST_CHECK(obj.exists("calories"));
    FAST_CHECK(obj.exists("temperature"));
    FAST_CHECK(obj.exists("cat1"));
    FAST_CHECK(obj.exists("cat2"));

    FAST_CHECK(!obj.exists("nyuknyuknyuk"));

    map<string, UniValue::VType> objTypes;
    objTypes["age"] = UniValue::VNUM;
    objTypes["first"] = UniValue::VSTR;
    objTypes["last"] = UniValue::VSTR;
    objTypes["distance"] = UniValue::VNUM;
    objTypes["time"] = UniValue::VNUM;
    objTypes["calories"] = UniValue::VNUM;
    objTypes["temperature"] = UniValue::VNUM;
    objTypes["cat1"] = UniValue::VNUM;
    objTypes["cat2"] = UniValue::VNUM;
    FAST_CHECK(obj.checkObject(objTypes));

    objTypes["cat2"] = UniValue::VSTR;
    FAST_CHECK(!obj.checkObject(objTypes));

    obj.clear();
    FAST_CHECK(obj.empty());
    FAST_CHECK_EQUAL(obj.size(), 0);
}

static const char *json1 =
"[1.10000000,{\"key1\":\"str\\u0000\",\"key2\":800,\"key3\":{\"name\":\"martian http://test.com\"}}]";

BOOST_AUTO_TEST_CASE(univalue_readwrite)
{
    UniValue v;
    FAST_CHECK(v.read(json1));

    string strJson1(json1);
    FAST_CHECK(v.read(strJson1));

    FAST_CHECK(v.isArray());
    FAST_CHECK_EQUAL(v.size(), 2);

    FAST_CHECK_EQUAL(v[0].getValStr(), "1.10000000");

    UniValue obj = v[1];
    FAST_CHECK(obj.isObject());
    FAST_CHECK_EQUAL(obj.size(), 3);

    FAST_CHECK(obj["key1"].isStr());
    std::string correctValue("str");
    correctValue.push_back('\0');
    FAST_CHECK_EQUAL(obj["key1"].getValStr(), correctValue);
    FAST_CHECK(obj["key2"].isNum());
    FAST_CHECK_EQUAL(obj["key2"].getValStr(), "800");
    FAST_CHECK(obj["key3"].isObject());

    FAST_CHECK_EQUAL(strJson1, v.write());

    /* Check for (correctly reporting) a parsing error if the initial
       JSON construct is followed by more stuff.  Note that whitespace
       is, of course, exempt.  */

    FAST_CHECK(v.read("  {}\n  "));
    FAST_CHECK(v.isObject());
    FAST_CHECK(v.read("  []\n  "));
    FAST_CHECK(v.isArray());

    FAST_CHECK(!v.read("@{}"));
    FAST_CHECK(!v.read("{} garbage"));
    FAST_CHECK(!v.read("[]{}"));
    FAST_CHECK(!v.read("{}[]"));
    FAST_CHECK(!v.read("{} 42"));
}

BOOST_AUTO_TEST_SUITE_END()

