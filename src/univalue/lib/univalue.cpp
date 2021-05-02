// Copyright 2014 BitPay Inc.
// Copyright 2015 XBit Core Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <iomanip>
#include <sstream>
#include <stdlib.h>

#include "univalue.h"

const UniValue NullUniValue;

void UniValue::clear()
{
    typ = VNULL;
    val.clear();
    keys.clear();
    values.clear();
}

bool UniValue::setNull()
{
    clear();
    return true;
}

bool UniValue::setBool(bool val_)
{
    clear();
    typ = VBOOL;
    if (val_)
        val = "1";
    return true;
}

static bool validNumStr(const std::string& s)
{
    std::string tokenVal;
    unsigned int consumed;
    enum jtokentype tt = getJsonToken(tokenVal, consumed, s.data(), s.data() + s.size());
    return (tt == JTOK_NUMBER);
}

bool UniValue::setNumStr(const std::string& val_)
{
    if (!validNumStr(val_))
        return false;

    clear();
    typ = VNUM;
    val = val_;
    return true;
}

bool UniValue::setInt(uint64_t val_)
{
    std::ostringstream oss;

    oss << val_;

    return setNumStr(oss.str());
}

bool UniValue::setInt(int64_t val_)
{
    std::ostringstream oss;

    oss << val_;

    return setNumStr(oss.str());
}

bool UniValue::setFloat(double val_)
{
    std::ostringstream oss;

    oss << std::setprecision(16) << val_;

    bool ret = setNumStr(oss.str());
    typ = VNUM;
    return ret;
}

bool UniValue::setStr(const std::string& val_)
{
    clear();
    typ = VSTR;
    val = val_;
    return true;
}

bool UniValue::setArray()
{
    clear();
    typ = VARR;
    return true;
}

bool UniValue::setObject()
{
    clear();
    typ = VOBJ;
    return true;
}

bool UniValue::push_back(const UniValue& val_)
{
    if (typ != VARR)
        return false;

    values.push_back(val_);
    return true;
}

bool UniValue::push_backV(const std::vector<UniValue>& vec)
{
    if (typ != VARR)
        return false;

    values.insert(values.end(), vec.begin(), vec.end());

    return true;
}

void UniValue::__pushKV(const std::string& key, const UniValue& val_)
{
    keys.push_back(key);
    values.push_back(val_);
}

bool UniValue::pushKV(const std::string& key, const UniValue& val_)
{
    if (typ != VOBJ)
        return false;

    size_t idx;
    if (findKey(key, idx))
        values[idx] = val_;
    else
        __pushKV(key, val_);
    return true;
}

bool UniValue::pushKVs(const UniValue& obj)
{
    if (typ != VOBJ || obj.typ != VOBJ)
        return false;

    for (size_t i = 0; i < obj.keys.size(); i++)
        __pushKV(obj.keys[i], obj.values.at(i));

    return true;
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
    if (typ != VOBJ)
        return false;

    for (std::map<std::string,UniValue::VType>::const_iterator it = t.begin();
         it != t.end(); ++it) {
        size_t idx = 0;
        if (!findKey(it->first, idx))
            return false;

        if (values.at(idx).getType() != it->second)
            return false;
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
    return NULL;
}

const UniValue& find_value(const UniValue& obj, const std::string& name)
{
    for (unsigned int i = 0; i < obj.keys.size(); i++)
        if (obj.keys[i] == name)
            return obj.values.at(i);

    return NullUniValue;
}

