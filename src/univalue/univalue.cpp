// Copyright 2014 BitPay Inc.
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <ctype.h>
#include <sstream>
#include "univalue.h"

using namespace std;

static const UniValue nullValue;

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

static bool validNumStr(const string& s)
{
    string tokenVal;
    unsigned int consumed;
    enum jtokentype tt = getJsonToken(tokenVal, consumed, s.c_str());
    return (tt == JTOK_NUMBER);
}

bool UniValue::setNumStr(const string& val_)
{
    if (!validNumStr(val_))
        return false;

    clear();
    typ = VNUM;
    val = val_;
    return true;
}

bool UniValue::setInt(uint64_t val)
{
    string s;
    ostringstream oss;

    oss << val;

    return setNumStr(oss.str());
}

bool UniValue::setInt(int64_t val)
{
    string s;
    ostringstream oss;

    oss << val;

    return setNumStr(oss.str());
}

bool UniValue::setFloat(double val)
{
    string s;
    ostringstream oss;

    oss << val;

    return setNumStr(oss.str());
}

bool UniValue::setStr(const string& val_)
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

bool UniValue::push_back(const UniValue& val)
{
    if (typ != VARR)
        return false;

    values.push_back(val);
    return true;
}

bool UniValue::push_backV(const std::vector<UniValue>& vec)
{
    if (typ != VARR)
        return false;

    values.insert(values.end(), vec.begin(), vec.end());

    return true;
}

bool UniValue::pushKV(const std::string& key, const UniValue& val)
{
    if (typ != VOBJ)
        return false;

    keys.push_back(key);
    values.push_back(val);
    return true;
}

bool UniValue::pushKVs(const UniValue& obj)
{
    if (typ != VOBJ || obj.typ != VOBJ)
        return false;

    for (unsigned int i = 0; i < obj.keys.size(); i++) {
        keys.push_back(obj.keys[i]);
        values.push_back(obj.values[i]);
    }

    return true;
}

int UniValue::findKey(const std::string& key) const
{
    for (unsigned int i = 0; i < keys.size(); i++) {
        if (keys[i] == key)
            return (int) i;
    }

    return -1;
}

bool UniValue::checkObject(const std::map<std::string,UniValue::VType>& t)
{
    for (std::map<std::string,UniValue::VType>::const_iterator it = t.begin();
         it != t.end(); it++) {
        int idx = findKey(it->first);
        if (idx < 0)
            return false;

        if (values[idx].getType() != it->second)
            return false;
    }

    return true;
}

const UniValue& UniValue::operator[](const std::string& key) const
{
    if (typ != VOBJ)
        return nullValue;

    int index = findKey(key);
    if (index < 0)
        return nullValue;

    return values[index];
}

const UniValue& UniValue::operator[](unsigned int index) const
{
    if (typ != VOBJ && typ != VARR)
        return nullValue;
    if (index >= values.size())
        return nullValue;

    return values[index];
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

