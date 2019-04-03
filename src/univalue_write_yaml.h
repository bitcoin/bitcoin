// Copyright 2014 BitPay Inc.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iomanip>
#include <sstream>
#include <stdio.h>
#include "univalue.h"
#include "univalue/lib/univalue_escapes.h"

static std::string univalue_yaml(const UniValue& u, unsigned int prettyIndent = 0,
                                 unsigned int indentLevel = 0, unsigned int preIndent = 0);

static std::string yaml_escape(const std::string& inS)
{
    std::string outS;
    outS.reserve(inS.size() * 2);

    for (unsigned int i = 0; i < inS.size(); i++) {
        unsigned char ch = inS[i];
        const char *escStr = escapes[ch];

        if (escStr)
            outS += escStr;
        else
            outS += ch;
    }

    return outS;
}

static void indentStr(unsigned int prettyIndent, unsigned int indentLevel, std::string& s, unsigned int preIndent = 0)
{
    assert(prettyIndent < 2000 && indentLevel < 2000 && preIndent <= prettyIndent * indentLevel);
    s.append(prettyIndent * indentLevel - preIndent, ' ');
}

std::string yaml_key(std::string k)
{
    if (k == "" || k[0] == '_' || k.find_first_of("0123456789_") == 0 || k.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_0123456789") != std::string::npos) {
        return "\"" + yaml_escape(k) + "\":";
    } else {
        return k + ":";
    }
}

static std::string num_to_yaml(const std::string &num)
{
    std::string res;
    size_t p = num.find(".");
    if (p == std::string::npos) p = num.size();
    if (p > 0) {
        size_t i = (p + 2) % 3 + 1;
        res = num.substr(0, i);
        for (; i < p; i+=3) {
            res += "_" + num.substr(i,3);
        }
    } else {
        res = "0";
    }
    if (p < num.size()) {
        res += ".";
        p += 1;
        while (p + 3 < num.size()) {
            res += num.substr(p,3) + "_";
            p += 3;
        }
        res += num.substr(p);
    }
    return res;
}

static unsigned int numalign(const std::string& num) {
    const std::string v = num_to_yaml(num);
    auto i = std::min(v.size(), v.find("."));
    assert(i < 1000);
    assert(i >= 0);
    return i;
}

void writeArray(const UniValue& u, unsigned int prettyIndent, unsigned int indentLevel, unsigned int preIndent, std::string& s)
{
    assert(preIndent == 0 || preIndent + 2 <= prettyIndent * indentLevel);

    const auto& values = u.getValues();

    if (values.size() == 0) {
        s += "[]";
        return;
    }

    std::string indentSpaces = "";
    if (!prettyIndent) s += "[";
    const std::string indentDash = "- ";
    const unsigned int indentBump = (prettyIndent == 1 ? 2 : 1);
    const unsigned int indentWithDash = prettyIndent * indentLevel + 2;

    for (unsigned int i = 0; i < values.size(); i++) {
        if (prettyIndent) {
            indentStr(prettyIndent, indentLevel, s, preIndent);
            s += indentDash;
            preIndent = 0;
        }
        s += univalue_yaml(values[i], prettyIndent, indentLevel + indentBump, indentWithDash);

        if (i != (values.size() - 1)) {
            if (!prettyIndent) s += ",";
            if (prettyIndent) s += "\n";
        }
    }
    if (!prettyIndent) s += "]";
}

void writeObject(const UniValue& u, unsigned int prettyIndent, unsigned int indentLevel, unsigned int preIndent, std::string& s)
{
    assert(preIndent <= prettyIndent * indentLevel);

    const auto& keys = u.getKeys();
    const auto& values = u.getValues();

    unsigned long alignNums = 0;

    if (keys.size() == 0) {
        s += "{}";
        return;
    }

    if (!prettyIndent) s += "{";
    if (prettyIndent && !s.empty() && s.back() != '\n') s += '\n';
    for (unsigned int i = 0; i < keys.size(); ++i) {
        if (prettyIndent) {
            indentStr(prettyIndent, indentLevel, s, preIndent);
            preIndent = 0;
        }
        const std::string key = yaml_key(keys[i]);
        s += key;
        switch (values[i].getType()) {
        case UniValue::VOBJ:
        case UniValue::VARR:
            if (prettyIndent && values[i].getValues().size() > 0) {
                s+="\n";
                break;
            }
        case UniValue::VNULL:
        case UniValue::VSTR:
        case UniValue::VNUM:
        case UniValue::VBOOL:
            if (prettyIndent) s += " ";
        }
        if (prettyIndent && !alignNums) {
            unsigned int j = i;
            while (j < values.size() && values[j].getType() == UniValue::VNUM) {
                alignNums = std::max(alignNums, yaml_key(keys[j]).size() + numalign(values[j].getValStr()));
                ++j;
            }
        }
        if (prettyIndent && alignNums) {
            if (values[i].getType() != UniValue::VNUM) {
                alignNums = 0;
            } else {
                auto amt = key.size() + numalign(values[i].getValStr());
                assert(amt <= alignNums);
                s.append(alignNums - amt, ' ');
            }
        }
        s += univalue_yaml(values[i], prettyIndent, indentLevel + 1);
        if (i < keys.size() - 1) {
            if (!prettyIndent) s += ",";
            if (prettyIndent) s += "\n";
        }
    }
    if (!prettyIndent) s += "}";
    if (prettyIndent && keys.size() > 10 && s[s.length()-1] != '\n') s += "\n";
}

static std::string univalue_yaml(const UniValue& u, unsigned int prettyIndent,
                            unsigned int indentLevel, unsigned int preIndent)
{
    std::string s;
    const std::string& val = u.getValStr();
    s.reserve(1024);

    unsigned int modIndent = indentLevel;

    switch (u.getType()) {
    case UniValue::VNULL:
        s += "null";
        break;
    case UniValue::VOBJ:
        writeObject(u, prettyIndent, modIndent, preIndent, s);
        break;
    case UniValue::VARR:
        writeArray(u, prettyIndent, modIndent, preIndent, s);
        break;
    case UniValue::VSTR:
        s += "\"" + yaml_escape(val) + "\"";
        break;
    case UniValue::VNUM:
        s += num_to_yaml(val);
        break;
    case UniValue::VBOOL:
        s += (val == "1" ? "true" : "false");
        break;
    }

    return s;
}

