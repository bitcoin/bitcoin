// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "script/destination.h"

#include "script/script.h"
#include "key.h"

bool ScriptDestinationVisitor::operator()(const CNoDestination &dest) const
{
    SetScriptDestination(m_script, dest);
    return false;
}

bool ScriptDestinationVisitor::operator()(const CKeyID &keyID) const
{
    SetScriptDestination(m_script, keyID);
    return true;
}

bool ScriptDestinationVisitor::operator()(const CScriptID &scriptID) const
{
    SetScriptDestination(m_script, scriptID);
    return true;
}

void SetScriptDestination(CScript& script, const CNoDestination& dest)
{
    script.SetDestinationNone();
}

void SetScriptDestination(CScript& script, const CKeyID& keyID)
{
    script.SetDestinationKeyID(keyID);
}

void SetScriptDestination(CScript& script, const CScriptID& scriptID)
{
    script.SetDestinationScriptID(scriptID);
}

bool SetScriptDestination(CScript& script, const CTxDestination& dest)
{
    return boost::apply_visitor(ScriptDestinationVisitor(script), dest);
}
