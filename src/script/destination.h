// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_SCRIPT_DESTINATION
#define H_BITCOIN_SCRIPT_DESTINATION
#include "key.h"
#include <boost/variant.hpp>

class CScript;

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
};

class ScriptDestinationVisitor : public boost::static_visitor<bool>
{
private:
    CScript& m_script;
public:
    ScriptDestinationVisitor(CScript& script) : m_script(script) {}
    bool operator()(const CNoDestination &dest) const;
    bool operator()(const CKeyID &keyID) const;
    bool operator()(const CScriptID &scriptID) const;
};

typedef boost::variant<CNoDestination, CKeyID, CScriptID> CTxDestination;


bool SetScriptDestination(CScript& script, const CTxDestination& dest);
void SetScriptDestination(CScript& script, const CKeyID& keyID);
void SetScriptDestination(CScript& script, const CScriptID& scriptID);
void SetScriptDestination(CScript& script, const CNoDestination& dest);
#endif
