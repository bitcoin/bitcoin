// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/ismine.h>

#include <key.h>
#include <key/extkey.h>
#include <key/stealth.h>
#include <keystore.h>
#include <script/script.h>
#include <script/sign.h>


//typedef std::vector<unsigned char> valtype;

bool HaveKeys(const std::vector<valtype>& pubkeys, const CKeyStore& keystore)
{
    for (const valtype& pubkey : pubkeys) {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (!keystore.HaveKey(keyID)) return false;
    }
    return true;
}

namespace {

/**
 * This is an enum that tracks the execution context of a script, similar to
 * SigVersion in script/interpreter. It is separate however because we want to
 * distinguish between top-level scriptPubKey execution and P2SH redeemScript
 * execution (a distinction that has no impact on consensus rules).
 */
enum class IsMineSigVersion
{
    TOP = 0,        //! scriptPubKey execution
    P2SH = 1,       //! P2SH redeemScript
    WITNESS_V0 = 2  //! P2WSH witness script execution
};

/**
 * This is an internal representation of isminetype + invalidity.
 * Its order is significant, as we return the max of all explored
 * possibilities.
 */
enum class IsMineResult
{
    NO = 0,          //! Not ours
    WATCH_ONLY = 1,  //! Included in watch-only balance
    SPENDABLE = 2,   //! Included in all balances
    INVALID = 3,     //! Not spendable by anyone (uncompressed pubkey in segwit, P2SH inside P2SH or witness, witness inside witness)
};

bool PermitsUncompressed(IsMineSigVersion sigversion)
{
    return sigversion == IsMineSigVersion::TOP || sigversion == IsMineSigVersion::P2SH;
}

isminetype IsMineInner(const CKeyStore& keystore, const CScript& scriptPubKey, bool& isInvalid, IsMineSigVersion sigversion)
{
    if (HasIsCoinstakeOp(scriptPubKey))
    {
        CScript scriptA, scriptB;
        if (!SplitConditionalCoinstakeScript(scriptPubKey, scriptA, scriptB))
            return ISMINE_NO;

        isminetype typeB = IsMineInner(keystore, scriptB, isInvalid, sigversion);
        if (typeB & ISMINE_SPENDABLE)
            return typeB;

        isminetype typeA = IsMineInner(keystore, scriptA, isInvalid, sigversion);
        if (typeA & ISMINE_SPENDABLE)
        {
            int ia = (int)typeA;
            ia &= ~ISMINE_SPENDABLE;
            ia |= ISMINE_WATCH_COLDSTAKE;
            typeA = (isminetype)ia;
        };

        return (isminetype)((int)typeA | (int)typeB);
    };

    isInvalid = false;

    std::vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(scriptPubKey, whichType, vSolutions)) {
        if (keystore.HaveWatchOnly(scriptPubKey))
            return ISMINE_WATCH_ONLY_;
        return ISMINE_NO;
    }

    CKeyID keyID;

    isminetype mine = ISMINE_NO;
    switch (whichType)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_WITNESS_UNKNOWN:
        break;
    case TX_PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        if (!PermitsUncompressed(sigversion) && vSolutions[0].size() != 33) {
            isInvalid = true;
            return ISMINE_NO;
        }
        if ((mine = keystore.IsMine(keyID)))
            return mine;
        //if (keystore.HaveKey(keyID))
        //    return ISMINE_SPENDABLE;
        break;
    case TX_WITNESS_V0_KEYHASH:
    {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            // P2WPKH inside P2WSH is invalid.
            isInvalid = true;
            return ISMINE_NO;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            // We do not support bare witness outputs unless the P2SH version of it would be
            // acceptable as well. This protects against matching before segwit activates.
            // This also applies to the P2WSH case.
            break;
        }
        isminetype ret = IsMineInner(keystore, GetScriptForDestination(CKeyID(uint160(vSolutions[0]))), isInvalid, IsMineSigVersion::WITNESS_V0);
        if (ret == ISMINE_SPENDABLE || ret == ISMINE_WATCH_ONLY_ || (ret == ISMINE_NO && isInvalid))
            return ret;
        break;
    }
    case TX_PUBKEYHASH:
    case TX_TIMELOCKED_PUBKEYHASH:
    case TX_PUBKEYHASH256:
        if (vSolutions[0].size() == 20)
            keyID = CKeyID(uint160(vSolutions[0]));
        else
        if (vSolutions[0].size() == 32)
            keyID = CKeyID(uint256(vSolutions[0]));
        else
            return ISMINE_NO;
        if (!PermitsUncompressed(sigversion)) {
            CPubKey pubkey;
            if (keystore.GetPubKey(keyID, pubkey) && !pubkey.IsCompressed()) {
                isInvalid = true;
                return ISMINE_NO;
            }
        }
        if ((mine = keystore.IsMine(keyID)))
            return mine;
        //if (keystore.HaveKey(keyID))
        //    return ISMINE_SPENDABLE;
        break;
    case TX_SCRIPTHASH:
    case TX_TIMELOCKED_SCRIPTHASH:
    case TX_SCRIPTHASH256:
    {
        if (sigversion != IsMineSigVersion::TOP) {
            // P2SH inside P2WSH or P2SH is invalid.
            isInvalid = true;
            return ISMINE_NO;
        }
        CScriptID scriptID;
        if (vSolutions[0].size() == 20)
            scriptID = CScriptID(uint160(vSolutions[0]));
        else
        if (vSolutions[0].size() == 32)
            scriptID.Set(uint256(vSolutions[0]));
        else
            return ISMINE_NO;
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            isminetype ret = IsMineInner(keystore, subscript, isInvalid, IsMineSigVersion::P2SH);
            if (ret == ISMINE_SPENDABLE || ret == ISMINE_WATCH_ONLY_ || (ret == ISMINE_NO && isInvalid))
                return ret;
        }
        break;
    }
    case TX_WITNESS_V0_SCRIPTHASH:
    {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            // P2WSH inside P2WSH is invalid.
            isInvalid = true;
            return ISMINE_NO;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            break;
        }
        uint160 hash;
        CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(hash.begin());
        CScriptID scriptID = CScriptID(hash);
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            isminetype ret = IsMineInner(keystore, subscript, isInvalid, IsMineSigVersion::WITNESS_V0);
            if (ret == ISMINE_SPENDABLE || ret == ISMINE_WATCH_ONLY_ || (ret == ISMINE_NO && isInvalid))
                return ret;
        }
        break;
    }

    case TX_MULTISIG:
    case TX_TIMELOCKED_MULTISIG:
    {
        // Never treat bare multisig outputs as ours (they can still be made watchonly-though)
        if (sigversion == IsMineSigVersion::TOP) break;

        // Only consider transactions "mine" if we own ALL the
        // keys involved. Multi-signature transactions that are
        // partially owned (somebody else has a key that can spend
        // them) enable spend-out-from-under-you attacks, especially
        // in shared-wallet situations.
        std::vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
        if (!PermitsUncompressed(sigversion)) {
            for (size_t i = 0; i < keys.size(); i++) {
                if (keys[i].size() != 33) {
                    isInvalid = true;
                    return ISMINE_NO;
                }
            }
        }
        if (HaveKeys(keys, keystore))
            return ISMINE_SPENDABLE;
        break;
    }
    default:
        break;
    }

    if (keystore.HaveWatchOnly(scriptPubKey)) {
        return ISMINE_WATCH_ONLY_;
    }
    return ISMINE_NO;
}

} // namespace

isminetype IsMine(const CKeyStore& keystore, const CScript& scriptPubKey, bool& isInvalid)
{
    isminetype rv = IsMineInner(keystore, scriptPubKey, isInvalid, IsMineSigVersion::TOP);
    return isInvalid ? ISMINE_NO : rv;
}

isminetype IsMine(const CKeyStore& keystore, const CScript& scriptPubKey)
{
    bool isInvalid = false;
    isminetype rv = IsMineInner(keystore, scriptPubKey, isInvalid, IsMineSigVersion::TOP);
    return isInvalid ? ISMINE_NO : rv;
}

isminetype IsMine(const CKeyStore& keystore, const CTxDestination& dest)
{
    if (dest.type() == typeid(CStealthAddress))
    {
        const CStealthAddress &sxAddr = boost::get<CStealthAddress>(dest);
        return sxAddr.scan_secret.size() == EC_SECRET_SIZE ? ISMINE_SPENDABLE : ISMINE_NO; // TODO: watch only?
    };

    CScript script = GetScriptForDestination(dest);
    return IsMine(keystore, script);
}

isminetype IsMineP2SH(const CKeyStore& keystore, const CScript& scriptPubKey)
{
    bool isInvalid = false;
    return IsMineInner(keystore, scriptPubKey, isInvalid, IsMineSigVersion::P2SH);
}
