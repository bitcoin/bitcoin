// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addressindex.h"

#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "util.h"


bool ExtractIndexInfo(const CScript *pScript, int &scriptType, std::vector<uint8_t> &hashBytes)
{
    scriptType = 0;
    if (pScript->IsPayToScriptHash())
    {
        hashBytes.assign(pScript->begin()+2, pScript->begin()+22);
        scriptType = 2;
    } else
    if (pScript->IsPayToPublicKeyHash())
    {
        hashBytes.assign(pScript->begin()+3, pScript->begin()+23);
        scriptType = 1;
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

    nValue = out->IsType(OUTPUT_STANDARD) ? out->GetValue() : -1;

    CScript tmpScript;
    if (HasIsCoinstakeOp(*pScript)
        && GetNonCoinstakeScriptPath(*pScript, tmpScript))
        pScript = &tmpScript;

    ExtractIndexInfo(pScript, scriptType, hashBytes);

    // Reset if HasIsCoinstakeOp
    pScript = out->GetPScriptPubKey();

    return true;
};

