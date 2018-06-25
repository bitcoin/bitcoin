// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <insight/addressindex.h>

#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <util.h>

bool ExtractIndexInfo(const CScript *pScript, int &scriptType, std::vector<uint8_t> &hashBytes)
{
    CScript tmpScript;
    if (HasIsCoinstakeOp(*pScript)
        && GetNonCoinstakeScriptPath(*pScript, tmpScript))
        return ExtractIndexInfo(&tmpScript, scriptType, hashBytes);

    scriptType = ADDR_INDT_UNKNOWN;
    if (pScript->IsPayToPublicKeyHash())
    {
        hashBytes.assign(pScript->begin()+3, pScript->begin()+23);
        scriptType = ADDR_INDT_PUBKEY_ADDRESS;
    } else
    if (pScript->IsPayToScriptHash())
    {
        hashBytes.assign(pScript->begin()+2, pScript->begin()+22);
        scriptType = ADDR_INDT_SCRIPT_ADDRESS;
    } else
    if (pScript->IsPayToPublicKeyHash256())
    {
        hashBytes.assign(pScript->begin()+3, pScript->begin()+35);
        scriptType = ADDR_INDT_PUBKEY_ADDRESS_256;
    } else
    if (pScript->IsPayToScriptHash256())
    {
        hashBytes.assign(pScript->begin()+2, pScript->begin()+34);
        scriptType = ADDR_INDT_SCRIPT_ADDRESS_256;
    };

    return true;
};

bool ExtractIndexInfo(const CTxOutBase *out, int &scriptType, std::vector<uint8_t> &hashBytes, CAmount &nValue, const CScript *&pScript)
{
    if (!(pScript = out->GetPScriptPubKey()))
    {
        LogPrintf("ERROR: %s - expected script pointer.\n", __func__);
        return false;
    };

    nValue = out->IsType(OUTPUT_STANDARD) ? out->GetValue() : 0;

    ExtractIndexInfo(pScript, scriptType, hashBytes);

    return true;
};

