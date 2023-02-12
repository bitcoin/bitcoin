// Copyright 2014 BitPay Inc.
// Copyright 2015 Bitcoin Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#include <univalue.h>

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

const UniValue NullUniValue;

void UniValue::clear()
{
    typ = VNULL;
    val.clear();
    keys.clear();
    values.clear();
}

void UniValue::setNull()
{
    clear();
}

void UniValue::setBool(bool val_)
{
    clear();
    typ = VBOOL;
    if (val_)
        val = "1";
}

static bool validNumStr(const std::string& s)
{
    std::string tokenVal;
    unsigned int consumed;
    enum jtokentype tt = getJsonToken(tokenVal, consumed, s.data(), s.data() + s.size());
    return (tt == JTOK_NUMBER);
}

void UniValue::setNumStr(std::string str)
{
    if (!validNumStr(str)) {
        throw std::runtime_error{"The string '" + str + "' is not a valid JSON number"};
    }

    clear();
    typ = VNUM;
    val = std::move(str);
}

void UniValue::setInt(uint64_t val_)
{
    std::ostringstream oss;

    oss << val_;

    return setNumStr(oss.str());
}

void UniValue::setInt(int64_t val_)
{
    std::ostringstream oss;

    oss << val_;

    return setNumStr(oss.str());
}

void UniValue::setFloat(double val_)
{
    std::ostringstream oss;

    oss << std::setprecision(16) << val_;

    return setNumStr(oss.str());
}

void UniValue::setStr(std::string str)
{
    clear();
    typ = VSTR;
    val = std::move(str);
}

void UniValue::setArray()
{
    clear();
    typ = VARR;
}

void UniValue::setObject()
{
    clear();
    typ = VOBJ;
}

void UniValue::push_back(UniValue val)
{
    checkType(VARR);

    values.push_back(std::move(val));
}

void UniValue::push_backV(const std::vector<UniValue>& vec)
{
    checkType(VARR);

    values.insert(values.end(), vec.begin(), vec.end());
}

void UniValue::__pushKV(std::string key, UniValue val)
{
    checkType(VOBJ);

    keys.push_back(std::move(key));
    values.push_back(std::move(val));
}

void UniValue::pushKV(std::string key, UniValue val)
{
    checkType(VOBJ);

    size_t idx;
    if (findKey(key, idx))
        values[idx] = std::move(val);
    else
        __pushKV(std::move(key), std::move(val));
}

void UniValue::pushKVs(UniValue obj)
{
    checkType(VOBJ);
    obj.checkType(VOBJ);

    for (size_t i = 0; i < obj.keys.size(); i++)
        __pushKV(std::move(obj.keys.at(i)), std::move(obj.values.at(i)));
}

void UniValue::getObjMap(std::map<std::string,UniValue>& kv) const
{
    if (typ != VOBJ)
        return;

    kv.clear();
    for (size_t i = 0; i < keys.size(); i++)
        kv[keys[i]] = values[i];
}

bool UniValue::findKey(const std::string& key, size_t& retIdx) const
{
    for (size_t i = 0; i < keys.size(); i++) {
        if (keys[i] == key) {
            retIdx = i;
            return true;
        }
    }

    return false;
}

bool UniValue::checkObject(const std::map<std::string,UniValue::VType>& t) const
{
    if (typ != VOBJ) {
        return false;
    }

    for (const auto& object: t) {
        size_t idx = 0;
        if (!findKey(object.first, idx)) {
            return false;
        }

        if (values.at(idx).getType() != object.second) {
            return false;
        }
    }

    return true;
}

const UniValue& UniValue::operator[](const std::string& key) const
{
    if (typ != VOBJ)
        return NullUniValue;

    size_t index = 0;
    if (!findKey(key, index))
        return NullUniValue;

    return values.at(index);
}

const UniValue& UniValue::operator[](size_t index) const
{
    if (typ != VOBJ && typ != VARR)
        return NullUniValue;
    if (index >= values.size())
        return NullUniValue;

    return values.at(index);
}

void UniValue::checkType(const VType& expected) const
{
    if (typ != expected) {
        throw type_error{"JSON value of type " + std::string{uvTypeName(typ)} + " is not of expected type " +
                                 std::string{uvTypeName(expected)}};
    }
}

const char *uvTypeName(UniValue::VType t)
{
    switch (t) {
    case UniValue::VNULL: return "null";
    case UniValue::VBOOL: return "bool";
    case UniValue::VOBJ: return "object";
    case UniValue::VARR: return "array";
    case UniValue::VSTR: return "string";
    case UniValue::VNUM: return "number";
    }

    // not reached
    return nullptr;
}

const UniValue& find_value(const UniValue& obj, const std::string& name)
{
    for (unsigned int i = 0; i < obj.keys.size(); i++)
        if (obj.keys[i] == name)
            return obj.values.at(i);

    return NullUniValue;
}

