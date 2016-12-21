// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet_ismine.h"

#include "core_io.h" // freeze debug only

#include "key.h"
#include "keystore.h"
#include "script/script.h"
#include "script/standard.h"
#include "script/sign.h"


#include <boost/foreach.hpp>

using namespace std;

typedef vector<unsigned char> valtype;

unsigned int HaveKeys(const vector<valtype>& pubkeys, const CKeyStore& keystore)
{
    unsigned int nResult = 0;
    BOOST_FOREACH(const valtype& pubkey, pubkeys)
    {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (keystore.HaveKey(keyID))
            ++nResult;
    }
    return nResult;
}

isminetype IsMine(const CKeyStore &keystore, const CTxDestination& dest, CBlockIndex* bestBlock)
{
    CScript script = GetScriptForDestination(dest);
    return IsMine(keystore, script, bestBlock);
}

isminetype IsMine(const CKeyStore &keystore, const CScript& scriptPubKey, CBlockIndex* bestBlock)
{
    vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(scriptPubKey, whichType, vSolutions)) {
        if (keystore.HaveWatchOnly(scriptPubKey))
            return ISMINE_WATCH_UNSOLVABLE;
        return ISMINE_NO;
    }

    CKeyID keyID;
    switch (whichType)
    {
		case TX_NONSTANDARD:
		case TX_NULL_DATA:
			break;
		case TX_PUBKEY:
			keyID = CPubKey(vSolutions[0]).GetID();
			if (keystore.HaveKey(keyID))
				return ISMINE_SPENDABLE;
			break;
		case TX_PUBKEYHASH:
			keyID = CKeyID(uint160(vSolutions[0]));
			if (keystore.HaveKey(keyID))
				return ISMINE_SPENDABLE;
			break;
		case TX_SCRIPTHASH:
		{

			CScriptID scriptID = CScriptID(uint160(vSolutions[0]));
			CScript subscript;
			if (keystore.GetCScript(scriptID, subscript)) {
				isminetype ret = IsMine(keystore, subscript, bestBlock);
				LogPrintf("Freeze SUBSCRIPT = %s! **** MINE=%d  *****  \n", ::ScriptToAsmStr(subscript), ret);
				//if (ret == ISMINE_SPENDABLE) TODO Don't understand why this line was required. Had to comment it so all minetypes in subscripts (eg CLTV) are recognizable
					return ret;
			}
			break;

		}
		case TX_MULTISIG:
		{
			// Only consider transactions "mine" if we own ALL the
			// keys involved. Multi-signature transactions that are
			// partially owned (somebody else has a key that can spend
			// them) enable spend-out-from-under-you attacks, especially
			// in shared-wallet situations.
			vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
			if (HaveKeys(keys, keystore) == keys.size())
				return ISMINE_SPENDABLE;
			break;
		}
		case TX_CLTV:
		{
			keyID = CPubKey(vSolutions[1]).GetID();
			if (keystore.HaveKey(keyID))
			{
				int64_t nFreezeLockTime = CScriptNum(vSolutions[0], false).getint64();

				LogPrintf("Found Freeze Have Key. nFreezeLockTime=%d \n", nFreezeLockTime);
				if (nFreezeLockTime < LOCKTIME_THRESHOLD)
				{
					// locktime is a block
					if (nFreezeLockTime > bestBlock->nHeight)
						return ISMINE_WATCH_SOLVABLE;
					else
						return ISMINE_SPENDABLE;
				}
				else
				{
					// locktime is a time
					if (nFreezeLockTime > bestBlock->GetMedianTimePast())
						return ISMINE_WATCH_SOLVABLE;
					else
						return ISMINE_SPENDABLE;
				}

			}
			else
			{
				LogPrintf("Found Freeze DONT HAVE KEY!! \n");
				return ISMINE_NO;
			}
		}
    } // switch

    if (keystore.HaveWatchOnly(scriptPubKey)) {
        // TODO: This could be optimized some by doing some work after the above solver
        CScript scriptSig;
        return ProduceSignature(DummySignatureCreator(&keystore), scriptPubKey, scriptSig) ? ISMINE_WATCH_SOLVABLE : ISMINE_WATCH_UNSOLVABLE;
    }
    return ISMINE_NO;
}
