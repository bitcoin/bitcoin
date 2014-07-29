// Copyright 2014 BitPay Inc.
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ctype.h>
#include <stdio.h>
#include "univalue.h"

// TODO: Using UTF8

using namespace std;

static bool initEscapes;
static const char *escapes[256];

static void initJsonEscape()
{
    escapes['"'] = "\\\"";
    escapes['\\'] = "\\\\";
    escapes['/'] = "\\/";
    escapes['\b'] = "\\b";
    escapes['\f'] = "\\f";
    escapes['\n'] = "\\n";
    escapes['\r'] = "\\r";
    escapes['\t'] = "\\t";

    initEscapes = true;
}

static string json_escape(const string& inS)
{
    if (!initEscapes)
        initJsonEscape();

    string outS;
    outS.reserve(inS.size() * 2);

    for (unsigned int i = 0; i < inS.size(); i++) {
        unsigned char ch = inS[i];
        const char *escStr = escapes[ch];

        if (escStr)
            outS += escStr;

        else if (isprint(ch))
            outS += ch;

        else {
            char tmpesc[16];
            sprintf(tmpesc, "\\u%04x", ch);
            outS += tmpesc;
        }
    }

    return outS;
}

string UniValue::write(unsigned int prettyIndent,
                       unsigned int indentLevel) const
{
    string s;
    s.reserve(1024);

    unsigned int modIndent = indentLevel;
    if (modIndent == 0)
        modIndent = 1;

    switch (typ) {
    case VNULL:
        s += "null";
        break;
    case VOBJ:
        writeObject(prettyIndent, modIndent, s);
        break;
    case VARR:
        writeArray(prettyIndent, modIndent, s);
        break;
    case VSTR:
        s += "\"" + json_escape(val) + "\"";
        break;
    case VNUM:
        s += val;
        break;
    case VBOOL:
        s += (val == "1" ? "true" : "false");
        break;
    }

    return s;
}

static string spaceStr;

static string indentStr(unsigned int prettyIndent, unsigned int indentLevel)
{
    unsigned int spaces = prettyIndent * indentLevel;
    while (spaceStr.size() < spaces)
        spaceStr += "                ";

    return spaceStr.substr(0, spaces);
}

void UniValue::writeArray(unsigned int prettyIndent, unsigned int indentLevel, string& s) const
{
    s += "[";
    if (prettyIndent)
        s += "\n";

    for (unsigned int i = 0; i < values.size(); i++) {
        if (prettyIndent)
            s += indentStr(prettyIndent, indentLevel);
        s += values[i].write(prettyIndent, indentLevel + 1);
        if (i != (values.size() - 1))
            s += ", ";
        if (prettyIndent)
            s += "\n";
    }

    if (prettyIndent)
        s += indentStr(prettyIndent, indentLevel - 1);
    s += "]";
}

void UniValue::writeObject(unsigned int prettyIndent, unsigned int indentLevel, string& s) const
{
    s += "{";
    if (prettyIndent)
        s += "\n";

    for (unsigned int i = 0; i < keys.size(); i++) {
        if (prettyIndent)
            s += indentStr(prettyIndent, indentLevel);
        s += "\"" + json_escape(keys[i]) + "\": ";
        s += values[i].write(prettyIndent, indentLevel + 1);
        if (i != (values.size() - 1))
            s += ",";
        if (prettyIndent)
            s += "\n";
    }

    if (prettyIndent)
        s += indentStr(prettyIndent, indentLevel - 1);
    s += "}";
}

