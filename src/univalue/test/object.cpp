// Copyright (c) 2014 BitPay Inc.
// Copyright (c) 2014-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#include <univalue.h>

#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#define BOOST_CHECK(expr) assert(expr)
#define BOOST_CHECK_EQUAL(v1, v2) assert((v1) == (v2))
#define BOOST_CHECK_THROW(stmt, excMatch) { \
        try { \
            (stmt); \
            assert(0 && "No exception caught"); \
        } catch (excMatch&) { \
        } catch (...) { \
            assert(0 && "Wrong exception caught"); \
        } \
    }
#define BOOST_CHECK_NO_THROW(stmt) { \
        try { \
            (stmt); \
        } catch (...) { \
            assert(0); \
        } \
    }

void univalue_constructor()
{
    UniValue v1;
    BOOST_CHECK(v1.isNull());

    UniValue v2(UniValue::VSTR);
    BOOST_CHECK(v2.isStr());

    UniValue v3(UniValue::VSTR, "foo");
    BOOST_CHECK(v3.isStr());
    BOOST_CHECK_EQUAL(v3.getValStr(), "foo");

    UniValue numTest;
    numTest.setNumStr("82");
    BOOST_CHECK(numTest.isNum());
    BOOST_CHECK_EQUAL(numTest.getValStr(), "82");

    uint64_t vu64 = 82;
    UniValue v4(vu64);
    BOOST_CHECK(v4.isNum());
    BOOST_CHECK_EQUAL(v4.getValStr(), "82");

    int64_t vi64 = -82;
    UniValue v5(vi64);
    BOOST_CHECK(v5.isNum());
    BOOST_CHECK_EQUAL(v5.getValStr(), "-82");

    int vi = -688;
    UniValue v6(vi);
    BOOST_CHECK(v6.isNum());
    BOOST_CHECK_EQUAL(v6.getValStr(), "-688");

    double vd = -7.21;
    UniValue v7(vd);
    BOOST_CHECK(v7.isNum());
    BOOST_CHECK_EQUAL(v7.getValStr(), "-7.21");

    std::string vs("yawn");
    UniValue v8(vs);
    BOOST_CHECK(v8.isStr());
    BOOST_CHECK_EQUAL(v8.getValStr(), "yawn");

    const char *vcs = "zappa";
    UniValue v9(vcs);
    BOOST_CHECK(v9.isStr());
    BOOST_CHECK_EQUAL(v9.getValStr(), "zappa");
}

void univalue_push_throw()
{
    UniValue j;
    BOOST_CHECK_THROW(j.push_back(1), std::runtime_error);
    BOOST_CHECK_THROW(j.push_backV({1}), std::runtime_error);
    BOOST_CHECK_THROW(j.pushKVEnd("k", 1), std::runtime_error);
    BOOST_CHECK_THROW(j.pushKV("k", 1), std::runtime_error);
    BOOST_CHECK_THROW(j.pushKVs({}), std::runtime_error);
}

void univalue_typecheck()
{
    UniValue v1;
    v1.setNumStr("1");
    BOOST_CHECK(v1.isNum());
    BOOST_CHECK_THROW(v1.get_bool(), std::runtime_error);

    {
        UniValue v_negative;
        v_negative.setNumStr("-1");
        BOOST_CHECK_THROW(v_negative.getInt<uint8_t>(), std::runtime_error);
        BOOST_CHECK_EQUAL(v_negative.getInt<int8_t>(), -1);
    }

    UniValue v2;
    v2.setBool(true);
    BOOST_CHECK_EQUAL(v2.get_bool(), true);
    BOOST_CHECK_THROW(v2.getInt<int>(), std::runtime_error);

    UniValue v3;
    v3.setNumStr("32482348723847471234");
    BOOST_CHECK_THROW(v3.getInt<int64_t>(), std::runtime_error);
    v3.setNumStr("1000");
    BOOST_CHECK_EQUAL(v3.getInt<int64_t>(), 1000);

    UniValue v4;
    v4.setNumStr("2147483648");
    BOOST_CHECK_EQUAL(v4.getInt<int64_t>(), 2147483648);
    BOOST_CHECK_THROW(v4.getInt<int>(), std::runtime_error);
    v4.setNumStr("1000");
    BOOST_CHECK_EQUAL(v4.getInt<int>(), 1000);
    BOOST_CHECK_THROW(v4.get_str(), std::runtime_error);
    BOOST_CHECK_EQUAL(v4.get_real(), 1000);
    BOOST_CHECK_THROW(v4.get_array(), std::runtime_error);
    BOOST_CHECK_THROW(v4.getKeys(), std::runtime_error);
    BOOST_CHECK_THROW(v4.getValues(), std::runtime_error);
    BOOST_CHECK_THROW(v4.get_obj(), std::runtime_error);

    UniValue v5;
    BOOST_CHECK(v5.read("[true, 10]"));
    BOOST_CHECK_NO_THROW(v5.get_array());
    std::vector<UniValue> vals = v5.getValues();
    BOOST_CHECK_THROW(vals[0].getInt<int>(), std::runtime_error);
    BOOST_CHECK_EQUAL(vals[0].get_bool(), true);

    BOOST_CHECK_EQUAL(vals[1].getInt<int>(), 10);
    BOOST_CHECK_THROW(vals[1].get_bool(), std::runtime_error);
}

void univalue_set()
{
    UniValue v(UniValue::VSTR, "foo");
    v.clear();
    BOOST_CHECK(v.isNull());
    BOOST_CHECK_EQUAL(v.getValStr(), "");

    v.setObject();
    BOOST_CHECK(v.isObject());
    BOOST_CHECK_EQUAL(v.size(), 0);
    BOOST_CHECK_EQUAL(v.getType(), UniValue::VOBJ);
    BOOST_CHECK(v.empty());

    v.setArray();
    BOOST_CHECK(v.isArray());
    BOOST_CHECK_EQUAL(v.size(), 0);

    v.setStr("zum");
    BOOST_CHECK(v.isStr());
    BOOST_CHECK_EQUAL(v.getValStr(), "zum");

    {
        std::string_view sv{"ab\0c", 4};
        UniValue j{sv};
        BOOST_CHECK(j.isStr());
        BOOST_CHECK_EQUAL(j.getValStr(), sv);
        BOOST_CHECK_EQUAL(j.write(), "\"ab\\u0000c\"");
    }

    v.setFloat(-1.01);
    BOOST_CHECK(v.isNum());
    BOOST_CHECK_EQUAL(v.getValStr(), "-1.01");

    v.setInt(int{1023});
    BOOST_CHECK(v.isNum());
    BOOST_CHECK_EQUAL(v.getValStr(), "1023");

    v.setInt(int64_t{-1023LL});
    BOOST_CHECK(v.isNum());
    BOOST_CHECK_EQUAL(v.getValStr(), "-1023");

    v.setInt(uint64_t{1023ULL});
    BOOST_CHECK(v.isNum());
    BOOST_CHECK_EQUAL(v.getValStr(), "1023");

    v.setNumStr("-688");
    BOOST_CHECK(v.isNum());
    BOOST_CHECK_EQUAL(v.getValStr(), "-688");

    v.setBool(false);
    BOOST_CHECK_EQUAL(v.isBool(), true);
    BOOST_CHECK_EQUAL(v.isTrue(), false);
    BOOST_CHECK_EQUAL(v.isFalse(), true);
    BOOST_CHECK_EQUAL(v.get_bool(), false);

    v.setBool(true);
    BOOST_CHECK_EQUAL(v.isBool(), true);
    BOOST_CHECK_EQUAL(v.isTrue(), true);
    BOOST_CHECK_EQUAL(v.isFalse(), false);
    BOOST_CHECK_EQUAL(v.get_bool(), true);

    BOOST_CHECK_THROW(v.setNumStr("zombocom"), std::runtime_error);

    v.setNull();
    BOOST_CHECK(v.isNull());
}

void univalue_array()
{
    UniValue arr(UniValue::VARR);

    UniValue v((int64_t)1023LL);
    arr.push_back(v);

    std::string vStr("zippy");
    arr.push_back(vStr);

    const char *s = "pippy";
    arr.push_back(s);

    std::vector<UniValue> vec;
    v.setStr("boing");
    vec.push_back(v);

    v.setStr("going");
    vec.push_back(v);

    arr.push_backV(vec);

    arr.push_back(uint64_t{400ULL});
    arr.push_back(int64_t{-400LL});
    arr.push_back(int{-401});
    arr.push_back(-40.1);
    arr.push_back(true);

    BOOST_CHECK_EQUAL(arr.empty(), false);
    BOOST_CHECK_EQUAL(arr.size(), 10);

    BOOST_CHECK_EQUAL(arr[0].getValStr(), "1023");
    BOOST_CHECK_EQUAL(arr[0].getType(), UniValue::VNUM);
    BOOST_CHECK_EQUAL(arr[1].getValStr(), "zippy");
    BOOST_CHECK_EQUAL(arr[1].getType(), UniValue::VSTR);
    BOOST_CHECK_EQUAL(arr[2].getValStr(), "pippy");
    BOOST_CHECK_EQUAL(arr[2].getType(), UniValue::VSTR);
    BOOST_CHECK_EQUAL(arr[3].getValStr(), "boing");
    BOOST_CHECK_EQUAL(arr[3].getType(), UniValue::VSTR);
    BOOST_CHECK_EQUAL(arr[4].getValStr(), "going");
    BOOST_CHECK_EQUAL(arr[4].getType(), UniValue::VSTR);
    BOOST_CHECK_EQUAL(arr[5].getValStr(), "400");
    BOOST_CHECK_EQUAL(arr[5].getType(), UniValue::VNUM);
    BOOST_CHECK_EQUAL(arr[6].getValStr(), "-400");
    BOOST_CHECK_EQUAL(arr[6].getType(), UniValue::VNUM);
    BOOST_CHECK_EQUAL(arr[7].getValStr(), "-401");
    BOOST_CHECK_EQUAL(arr[7].getType(), UniValue::VNUM);
    BOOST_CHECK_EQUAL(arr[8].getValStr(), "-40.1");
    BOOST_CHECK_EQUAL(arr[8].getType(), UniValue::VNUM);
    BOOST_CHECK_EQUAL(arr[9].getValStr(), "1");
    BOOST_CHECK_EQUAL(arr[9].getType(), UniValue::VBOOL);

    BOOST_CHECK_EQUAL(arr[999].getValStr(), "");

    arr.clear();
    BOOST_CHECK(arr.empty());
    BOOST_CHECK_EQUAL(arr.size(), 0);
}

void univalue_object()
{
    UniValue obj(UniValue::VOBJ);
    std::string strKey, strVal;
    UniValue v;

    strKey = "age";
    v.setInt(100);
    obj.pushKV(strKey, v);

    strKey = "first";
    strVal = "John";
    obj.pushKV(strKey, strVal);

    strKey = "last";
    const char* cVal = "Smith";
    obj.pushKV(strKey, cVal);

    strKey = "distance";
    obj.pushKV(strKey, int64_t{25});

    strKey = "time";
    obj.pushKV(strKey, uint64_t{3600});

    strKey = "calories";
    obj.pushKV(strKey, int{12});

    strKey = "temperature";
    obj.pushKV(strKey, double{90.012});

    strKey = "moon";
    obj.pushKV(strKey, true);

    strKey = "spoon";
    obj.pushKV(strKey, false);

    UniValue obj2(UniValue::VOBJ);
    obj2.pushKV("cat1", 9000);
    obj2.pushKV("cat2", 12345);

    obj.pushKVs(obj2);

    BOOST_CHECK_EQUAL(obj.empty(), false);
    BOOST_CHECK_EQUAL(obj.size(), 11);

    BOOST_CHECK_EQUAL(obj["age"].getValStr(), "100");
    BOOST_CHECK_EQUAL(obj["first"].getValStr(), "John");
    BOOST_CHECK_EQUAL(obj["last"].getValStr(), "Smith");
    BOOST_CHECK_EQUAL(obj["distance"].getValStr(), "25");
    BOOST_CHECK_EQUAL(obj["time"].getValStr(), "3600");
    BOOST_CHECK_EQUAL(obj["calories"].getValStr(), "12");
    BOOST_CHECK_EQUAL(obj["temperature"].getValStr(), "90.012");
    BOOST_CHECK_EQUAL(obj["moon"].getValStr(), "1");
    BOOST_CHECK_EQUAL(obj["spoon"].getValStr(), "");
    BOOST_CHECK_EQUAL(obj["cat1"].getValStr(), "9000");
    BOOST_CHECK_EQUAL(obj["cat2"].getValStr(), "12345");

    BOOST_CHECK_EQUAL(obj["nyuknyuknyuk"].getValStr(), "");

    BOOST_CHECK(obj.exists("age"));
    BOOST_CHECK(obj.exists("first"));
    BOOST_CHECK(obj.exists("last"));
    BOOST_CHECK(obj.exists("distance"));
    BOOST_CHECK(obj.exists("time"));
    BOOST_CHECK(obj.exists("calories"));
    BOOST_CHECK(obj.exists("temperature"));
    BOOST_CHECK(obj.exists("moon"));
    BOOST_CHECK(obj.exists("spoon"));
    BOOST_CHECK(obj.exists("cat1"));
    BOOST_CHECK(obj.exists("cat2"));

    BOOST_CHECK(!obj.exists("nyuknyuknyuk"));

    std::map<std::string, UniValue::VType> objTypes;
    objTypes["age"] = UniValue::VNUM;
    objTypes["first"] = UniValue::VSTR;
    objTypes["last"] = UniValue::VSTR;
    objTypes["distance"] = UniValue::VNUM;
    objTypes["time"] = UniValue::VNUM;
    objTypes["calories"] = UniValue::VNUM;
    objTypes["temperature"] = UniValue::VNUM;
    objTypes["moon"] = UniValue::VBOOL;
    objTypes["spoon"] = UniValue::VBOOL;
    objTypes["cat1"] = UniValue::VNUM;
    objTypes["cat2"] = UniValue::VNUM;
    BOOST_CHECK(obj.checkObject(objTypes));

    objTypes["cat2"] = UniValue::VSTR;
    BOOST_CHECK(!obj.checkObject(objTypes));

    obj.clear();
    BOOST_CHECK(obj.empty());
    BOOST_CHECK_EQUAL(obj.size(), 0);
    BOOST_CHECK_EQUAL(obj.getType(), UniValue::VNULL);

    obj.setObject();
    UniValue uv;
    uv.setInt(42);
    obj.pushKVEnd("age", uv);
    BOOST_CHECK_EQUAL(obj.size(), 1);
    BOOST_CHECK_EQUAL(obj["age"].getValStr(), "42");

    uv.setInt(43);
    obj.pushKV("age", uv);
    BOOST_CHECK_EQUAL(obj.size(), 1);
    BOOST_CHECK_EQUAL(obj["age"].getValStr(), "43");

    obj.pushKV("name", "foo bar");

    std::map<std::string,UniValue> kv;
    obj.getObjMap(kv);
    BOOST_CHECK_EQUAL(kv["age"].getValStr(), "43");
    BOOST_CHECK_EQUAL(kv["name"].getValStr(), "foo bar");

}

static const char *json1 =
"[1.10000000,{\"key1\":\"str\\u0000\",\"key2\":800,\"key3\":{\"name\":\"martian http://test.com\"}}]";

void univalue_readwrite()
{
    UniValue v;
    BOOST_CHECK(v.read(json1));

    std::string strJson1(json1);
    BOOST_CHECK(v.read(strJson1));

    BOOST_CHECK(v.isArray());
    BOOST_CHECK_EQUAL(v.size(), 2);

    BOOST_CHECK_EQUAL(v[0].getValStr(), "1.10000000");

    UniValue obj = v[1];
    BOOST_CHECK(obj.isObject());
    BOOST_CHECK_EQUAL(obj.size(), 3);

    BOOST_CHECK(obj["key1"].isStr());
    std::string correctValue("str");
    correctValue.push_back('\0');
    BOOST_CHECK_EQUAL(obj["key1"].getValStr(), correctValue);
    BOOST_CHECK(obj["key2"].isNum());
    BOOST_CHECK_EQUAL(obj["key2"].getValStr(), "800");
    BOOST_CHECK(obj["key3"].isObject());

    BOOST_CHECK_EQUAL(strJson1, v.write());

    // Valid
    BOOST_CHECK(v.read("1.0") && (v.get_real() == 1.0));
    BOOST_CHECK(v.read("true") && v.get_bool());
    BOOST_CHECK(v.read("[false]") && !v[0].get_bool());
    BOOST_CHECK(v.read("{\"a\": true}") && v["a"].get_bool());
    BOOST_CHECK(v.read("{\"1\": \"true\"}") && (v["1"].get_str() == "true"));
    // Valid, with leading or trailing whitespace
    BOOST_CHECK(v.read(" 1.0") && (v.get_real() == 1.0));
    BOOST_CHECK(v.read("1.0 ") && (v.get_real() == 1.0));
    BOOST_CHECK(v.read("0.00000000000000000000000000000000000001e+30 "));

    BOOST_CHECK(!v.read(".19e-6")); //should fail, missing leading 0, therefore invalid JSON
    // Invalid, initial garbage
    BOOST_CHECK(!v.read("[1.0"));
    BOOST_CHECK(!v.read("a1.0"));
    // Invalid, trailing garbage
    BOOST_CHECK(!v.read("1.0sds"));
    BOOST_CHECK(!v.read("1.0]"));
    // Invalid, keys have to be names
    BOOST_CHECK(!v.read("{1: \"true\"}"));
    BOOST_CHECK(!v.read("{true: 1}"));
    BOOST_CHECK(!v.read("{[1]: 1}"));
    BOOST_CHECK(!v.read("{{\"a\": \"a\"}: 1}"));
    // BTC addresses should fail parsing
    BOOST_CHECK(!v.read("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    BOOST_CHECK(!v.read("3J98t1WpEZ73CNmQviecrnyiWrnqRhWNL"));

    /* Check for (correctly reporting) a parsing error if the initial
       JSON construct is followed by more stuff.  Note that whitespace
       is, of course, exempt.  */

    BOOST_CHECK(v.read("  {}\n  "));
    BOOST_CHECK(v.isObject());
    BOOST_CHECK(v.read("  []\n  "));
    BOOST_CHECK(v.isArray());

    BOOST_CHECK(!v.read("@{}"));
    BOOST_CHECK(!v.read("{} garbage"));
    BOOST_CHECK(!v.read("[]{}"));
    BOOST_CHECK(!v.read("{}[]"));
    BOOST_CHECK(!v.read("{} 42"));
}

int main(int argc, char* argv[])
{
    univalue_constructor();
    univalue_push_throw();
    univalue_typecheck();
    univalue_set();
    univalue_array();
    univalue_object();
    univalue_readwrite();
    return 0;
}
