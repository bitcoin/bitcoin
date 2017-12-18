// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"
#include "base58.h"
#include "chain.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "init.h"
#include "httpserver.h"
#include "validation.h"
#include "net.h"
#include "policy/policy.h"
#include "policy/rbf.h"
#include "rpc/server.h"
#include "rpc/mining.h"
#include "script/sign.h"
#include "timedata.h"
#include "util.h"
#include "txdb.h"
#include "anon.h"
#include "utilmoneystr.h"
#include "wallet/hdwallet.h"
#include "wallet/hdwalletdb.h"
#include "wallet/coincontrol.h"
#include "wallet/rpcwallet.h"
#include "chainparams.h"
#include "key/mnemonic.h"
#include "pos/miner.h"
#include "crypto/sha256.h"
#include "warnings.h"

#include <univalue.h>
#include <stdint.h>



void EnsureWalletIsUnlocked(CHDWallet *pwallet)
{
    if (pwallet->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet locked, please enter the wallet passphrase with walletpassphrase first.");

    if (pwallet->fUnlockForStakingOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet is unlocked for staking only.");
};

static const std::string WALLET_ENDPOINT_BASE = "/wallet/";

CHDWallet *GetHDWalletForJSONRPCRequest(const JSONRPCRequest &request)
{
    if (request.URI.substr(0, WALLET_ENDPOINT_BASE.size()) == WALLET_ENDPOINT_BASE) {
        // wallet endpoint was used
        std::string requestedWallet = urlDecode(request.URI.substr(WALLET_ENDPOINT_BASE.size()));
        for (auto pwallet : ::vpwallets) {
            if (pwallet->GetName() == requestedWallet) {
                return GetHDWallet(pwallet);
            }
        }
        throw JSONRPCError(RPC_WALLET_NOT_FOUND, "Requested wallet does not exist or is not loaded");
    }
    return ::vpwallets.size() == 1 || (request.fHelp && ::vpwallets.size() > 0) ? GetHDWallet(::vpwallets[0]) : nullptr;
}

inline uint32_t reversePlace(uint8_t *p)
{
    uint32_t rv = 0;
    for (int i = 0; i < 4; ++i)
        rv |= (uint32_t) *(p+i) << (8 * (3-i));
    return rv;
};

int ExtractBip32InfoV(std::vector<uint8_t> &vchKey, UniValue &keyInfo, std::string &sError)
{
    CExtKey58 ek58;
    CExtKeyPair vk;
    vk.DecodeV(&vchKey[4]);

    CChainParams::Base58Type typePk = CChainParams::EXT_PUBLIC_KEY;
    if (memcmp(&vchKey[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY)[0], 4) == 0)
        keyInfo.pushKV("type", "Particl extended secret key");
    else
    if (memcmp(&vchKey[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY_BTC)[0], 4) == 0)
    {
        keyInfo.pushKV("type", "Bitcoin extended secret key");
        typePk = CChainParams::EXT_PUBLIC_KEY_BTC;
    } else
    {
        keyInfo.pushKV("type", "Unknown extended secret key");
    };

    keyInfo.pushKV("version", strprintf("%02X", reversePlace(&vchKey[0])));
    keyInfo.pushKV("depth", strprintf("%u", vchKey[4]));
    keyInfo.pushKV("parent_fingerprint", strprintf("%08X", reversePlace(&vchKey[5])));
    keyInfo.pushKV("child_index", strprintf("%u", reversePlace(&vchKey[9])));
    keyInfo.pushKV("chain_code", strprintf("%s", HexStr(&vchKey[13], &vchKey[13+32])));
    keyInfo.pushKV("key", strprintf("%s", HexStr(&vchKey[46], &vchKey[46+32])));

    // don't display raw secret ??
    // TODO: add option

    CKey key;
    key.Set(&vchKey[46], true);
    keyInfo.pushKV("privkey", strprintf("%s", CBitcoinSecret(key).ToString()));
    CKeyID id = key.GetPubKey().GetID();
    CBitcoinAddress addr;
    addr.Set(id, CChainParams::EXT_KEY_HASH);

    keyInfo.pushKV("id", addr.ToString().c_str());
    addr.Set(id);
    keyInfo.pushKV("address", addr.ToString().c_str());
    keyInfo.pushKV("checksum", strprintf("%02X", reversePlace(&vchKey[78])));

    ek58.SetKey(vk, typePk);
    keyInfo.pushKV("ext_public_key", ek58.ToString());

    return 0;
};

int ExtractBip32InfoP(std::vector<uint8_t> &vchKey, UniValue &keyInfo, std::string &sError)
{
    CExtPubKey pk;

    if (memcmp(&vchKey[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY)[0], 4) == 0)
        keyInfo.pushKV("type", "Particl extended public key");
    else
    if (memcmp(&vchKey[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY_BTC)[0], 4) == 0)
        keyInfo.pushKV("type", "Bitcoin extended public key");
    else
        keyInfo.pushKV("type", "Unknown extended public key");

    keyInfo.pushKV("version", strprintf("%02X", reversePlace(&vchKey[0])));
    keyInfo.pushKV("depth", strprintf("%u", vchKey[4]));
    keyInfo.pushKV("parent_fingerprint", strprintf("%08X", reversePlace(&vchKey[5])));
    keyInfo.pushKV("child_index", strprintf("%u", reversePlace(&vchKey[9])));
    keyInfo.pushKV("chain_code", strprintf("%s", HexStr(&vchKey[13], &vchKey[13+32])));
    keyInfo.pushKV("key", strprintf("%s", HexStr(&vchKey[45], &vchKey[45+33])));

    CPubKey key;
    key.Set(&vchKey[45], &vchKey[78]);
    CKeyID id = key.GetID();
    CBitcoinAddress addr;
    addr.Set(id, CChainParams::EXT_KEY_HASH);

    keyInfo.pushKV("id", addr.ToString().c_str());
    addr.Set(id);
    keyInfo.pushKV("address", addr.ToString().c_str());
    keyInfo.pushKV("checksum", strprintf("%02X", reversePlace(&vchKey[78])));

    return 0;
};

int ExtKeyPathV(std::string &sPath, std::vector<uint8_t> &vchKey, UniValue &keyInfo, std::string &sError)
{
    if (sPath.compare("info") == 0)
        return ExtractBip32InfoV(vchKey, keyInfo, sError);

    CExtKey vk;
    vk.Decode(&vchKey[4]);

    CExtKey vkOut;
    CExtKey vkWork = vk;

    std::vector<uint32_t> vPath;
    int rv;
    if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
    {
        sError = ExtKeyGetString(rv);
        return 1;
    };

    for (std::vector<uint32_t>::iterator it = vPath.begin(); it != vPath.end(); ++it)
    {
        if (*it == 0)
        {
            vkOut = vkWork;
        } else
        if (!vkWork.Derive(vkOut, *it))
        {
            sError = "CExtKey Derive failed.";
            return 1;
        };
        vkWork = vkOut;
    };

    CBitcoinExtKey ekOut;
    ekOut.SetKey(vkOut);
    keyInfo.pushKV("result", ekOut.ToString());

    return 0;
};

int ExtKeyPathP(std::string &sPath, std::vector<uint8_t> &vchKey, UniValue &keyInfo, std::string &sError)
{
    if (sPath.compare("info") == 0)
        return ExtractBip32InfoP(vchKey, keyInfo, sError);

    CExtPubKey pk;
    pk.Decode(&vchKey[4]);

    CExtPubKey pkOut;
    CExtPubKey pkWork = pk;

    std::vector<uint32_t> vPath;
    int rv;
    if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
    {
        sError = ExtKeyGetString(rv);
        return 1;
    };

    for (std::vector<uint32_t>::iterator it = vPath.begin(); it != vPath.end(); ++it)
    {
        if (*it == 0)
        {
            pkOut = pkWork;
        } else
        if ((*it >> 31) == 1)
        {
            sError = "Can't derive hardened keys from public ext key.";
            return 1;
        } else
        if (!pkWork.Derive(pkOut, *it))
        {
            sError = "CExtKey Derive failed.";
            return 1;
        };
        pkWork = pkOut;
    };

    CBitcoinExtPubKey ekOut;
    ekOut.SetKey(pkOut);
    keyInfo.pushKV("result", ekOut.ToString());

    return 0;
};

int AccountInfo(CHDWallet *pwallet, CExtKeyAccount *pa, int nShowKeys, bool fAllChains, UniValue &obj, std::string &sError)
{
    CExtKey58 eKey58;

    obj.pushKV("type", "Account");
    obj.pushKV("active", pa->nFlags & EAF_ACTIVE ? "true" : "false");
    obj.pushKV("label", pa->sLabel);

    if (pwallet->idDefaultAccount == pa->GetID())
        obj.pushKV("default_account", "true");

    mapEKValue_t::iterator mvi = pa->mapValue.find(EKVT_CREATED_AT);
    if (mvi != pa->mapValue.end())
    {
        int64_t nCreatedAt;
        GetCompressedInt64(mvi->second, (uint64_t&)nCreatedAt);
        obj.pushKV("created_at", nCreatedAt);
    };

    obj.pushKV("id", pa->GetIDString58());
    obj.pushKV("has_secret", pa->nFlags & EAF_HAVE_SECRET ? "true" : "false");

    CStoredExtKey *sekAccount = pa->ChainAccount();
    if (!sekAccount)
    {
        obj.pushKV("error", "chain account not set.");
        return 0;
    };

    CBitcoinAddress addr;
    addr.Set(pa->idMaster, CChainParams::EXT_KEY_HASH);
    obj.pushKV("root_key_id", addr.ToString());

    mvi = sekAccount->mapValue.find(EKVT_PATH);
    if (mvi != sekAccount->mapValue.end())
    {
        std::string sPath;
        if (0 == PathToString(mvi->second, sPath, 'h'))
            obj.pushKV("path", sPath);
    };
    // TODO: separate passwords for accounts
    if (pa->nFlags & EAF_HAVE_SECRET
        && nShowKeys > 1
        && pwallet->ExtKeyUnlock(sekAccount) == 0)
    {
        eKey58.SetKeyV(sekAccount->kp);
        obj.pushKV("evkey", eKey58.ToString());
    };

    if (nShowKeys > 0)
    {
        eKey58.SetKeyP(sekAccount->kp);
        obj.pushKV("epkey", eKey58.ToString());
    };

    if (nShowKeys > 2) // dumpwallet
    {
        obj.pushKV("stealth_address_pack", (int)pa->nPackStealth);
        obj.pushKV("stealth_keys_received_pack", (int)pa->nPackStealthKeys);
    };


    if (fAllChains)
    {
        UniValue arChains(UniValue::VARR);
        for (size_t i = 1; i < pa->vExtKeys.size(); ++i) // vExtKeys[0] stores the account key
        {
            UniValue objC(UniValue::VOBJ);
            CStoredExtKey *sek = pa->vExtKeys[i];
            eKey58.SetKeyP(sek->kp);

            if (pa->nActiveExternal == i)
                objC.pushKV("function", "active_external");
            if (pa->nActiveInternal == i)
                objC.pushKV("function", "active_internal");
            if (pa->nActiveStealth == i)
                objC.pushKV("function", "active_stealth");

            objC.pushKV("id", sek->GetIDString58());
            objC.pushKV("chain", eKey58.ToString());
            objC.pushKV("label", sek->sLabel);
            objC.pushKV("active", sek->nFlags & EAF_ACTIVE ? "true" : "false");
            objC.pushKV("receive_on", sek->nFlags & EAF_RECEIVE_ON ? "true" : "false");

            mapEKValue_t::iterator it = sek->mapValue.find(EKVT_KEY_TYPE);
            if (it != sek->mapValue.end() && it->second.size() > 0)
            {
                std::string(sUseType);
                switch(it->second[0])
                {
                    case EKT_EXTERNAL:      sUseType = "external";      break;
                    case EKT_INTERNAL:      sUseType = "internal";      break;
                    case EKT_STEALTH:       sUseType = "stealth";       break;
                    case EKT_CONFIDENTIAL:  sUseType = "confidential";  break;
                    default:                sUseType = "unknown";       break;
                };
                objC.pushKV("use_type", sUseType);
            };

            objC.pushKV("num_derives", strprintf("%u", sek->nGenerated));
            objC.pushKV("num_derives_h", strprintf("%u", sek->nHGenerated));

            if (nShowKeys > 2 // dumpwallet
                && pa->nFlags & EAF_HAVE_SECRET)
            {
                eKey58.SetKeyV(sek->kp);
                objC.pushKV("evkey", eKey58.ToString());

                mvi = sek->mapValue.find(EKVT_CREATED_AT);
                if (mvi != sek->mapValue.end())
                {
                    int64_t nCreatedAt;
                    GetCompressedInt64(mvi->second, (uint64_t&)nCreatedAt);
                    objC.pushKV("created_at", nCreatedAt);
                };
            };

            mvi = sek->mapValue.find(EKVT_PATH);
            if (mvi != sek->mapValue.end())
            {
                std::string sPath;
                if (0 == PathToString(mvi->second, sPath, 'h'))
                    objC.pushKV("path", sPath);
            };

            arChains.push_back(objC);
        };
        obj.pushKV("chains", arChains);
    } else
    {
        if (pa->nActiveExternal < pa->vExtKeys.size())
        {
            CStoredExtKey *sekE = pa->vExtKeys[pa->nActiveExternal];
            if (nShowKeys > 0)
            {
                eKey58.SetKeyP(sekE->kp);
                obj.pushKV("external_chain", eKey58.ToString());
            };
            obj.pushKV("num_derives_external", strprintf("%u", sekE->nGenerated));
            obj.pushKV("num_derives_external_h", strprintf("%u", sekE->nHGenerated));
        };

        if (pa->nActiveInternal < pa->vExtKeys.size())
        {
            CStoredExtKey *sekI = pa->vExtKeys[pa->nActiveInternal];
            if (nShowKeys > 0)
            {
                eKey58.SetKeyP(sekI->kp);
                obj.pushKV("internal_chain", eKey58.ToString());
            };
            obj.pushKV("num_derives_internal", strprintf("%u", sekI->nGenerated));
            obj.pushKV("num_derives_internal_h", strprintf("%u", sekI->nHGenerated));
        };

        if (pa->nActiveStealth < pa->vExtKeys.size())
        {
            CStoredExtKey *sekS = pa->vExtKeys[pa->nActiveStealth];
            obj.pushKV("num_derives_stealth", strprintf("%u", sekS->nGenerated));
            obj.pushKV("num_derives_stealth_h", strprintf("%u", sekS->nHGenerated));
        };
    };

    return 0;
};

int AccountInfo(CHDWallet *pwallet, CKeyID &keyId, int nShowKeys, bool fAllChains, UniValue &obj, std::string &sError)
{
    // TODO: inactive keys can be in db and not in memory - search db for keyId
    ExtKeyAccountMap::iterator mi = pwallet->mapExtAccounts.find(keyId);
    if (mi == pwallet->mapExtAccounts.end())
    {
        sError = "Unknown account.";
        return 1;
    };

    CExtKeyAccount *pa = mi->second;

    return AccountInfo(pwallet, pa, nShowKeys, fAllChains, obj, sError);
};

int KeyInfo(CHDWallet *pwallet, CKeyID &idMaster, CKeyID &idKey, CStoredExtKey &sek, int nShowKeys, UniValue &obj, std::string &sError)
{
    CExtKey58 eKey58;

    bool fBip44Root = false;
    obj.pushKV("type", "Loose");
    obj.pushKV("active", sek.nFlags & EAF_ACTIVE ? "true" : "false");
    obj.pushKV("receive_on", sek.nFlags & EAF_RECEIVE_ON ? "true" : "false");
    obj.pushKV("encrypted", sek.nFlags & EAF_IS_CRYPTED ? "true" : "false");
    obj.pushKV("label", sek.sLabel);

    if (reversePlace(&sek.kp.vchFingerprint[0]) == 0)
    {
        obj.pushKV("path", "Root");
    } else
    {
        mapEKValue_t::iterator mvi = sek.mapValue.find(EKVT_PATH);
        if (mvi != sek.mapValue.end())
        {
            std::string sPath;
            if (0 == PathToString(mvi->second, sPath, 'h'))
                obj.pushKV("path", sPath);
        };
    };

    mapEKValue_t::iterator mvi = sek.mapValue.find(EKVT_KEY_TYPE);
    if (mvi != sek.mapValue.end())
    {
        uint8_t type = EKT_MAX_TYPES;
        if (mvi->second.size() == 1)
            type = mvi->second[0];

        std::string sType;
        switch (type)
        {
            case EKT_MASTER      : sType = "Master"; break;
            case EKT_BIP44_MASTER:
                sType = "BIP44 Root Key";
                fBip44Root = true;
                break;
            default              : sType = "Unknown"; break;
        };
        obj.pushKV("key_type", sType);
    };

    if (idMaster == idKey)
        obj.pushKV("current_master", "true");

    CBitcoinAddress addr;
    mvi = sek.mapValue.find(EKVT_ROOT_ID);
    if (mvi != sek.mapValue.end())
    {
        CKeyID idRoot;

        if (GetCKeyID(mvi->second, idRoot))
        {
            addr.Set(idRoot, CChainParams::EXT_KEY_HASH);
            obj.pushKV("root_key_id", addr.ToString());
        } else
        {
            obj.pushKV("root_key_id", "malformed");
        };
    };

    mvi = sek.mapValue.find(EKVT_CREATED_AT);
    if (mvi != sek.mapValue.end())
    {
        int64_t nCreatedAt;
        GetCompressedInt64(mvi->second, (uint64_t&)nCreatedAt);
        obj.pushKV("created_at", nCreatedAt);
    };

    addr.Set(idKey, CChainParams::EXT_KEY_HASH);
    obj.pushKV("id", addr.ToString());


    if (nShowKeys > 1
        && pwallet->ExtKeyUnlock(&sek) == 0)
    {
        std::string sKey;
        if (sek.kp.IsValidV())
        {
            if (fBip44Root)
                eKey58.SetKey(sek.kp, CChainParams::EXT_SECRET_KEY_BTC);
            else
                eKey58.SetKeyV(sek.kp);
            sKey = eKey58.ToString();
        } else
        {
            sKey = "Unknown";
        };

        obj.pushKV("evkey", sKey);
    };

    if (nShowKeys > 0)
    {
        if (fBip44Root)
            eKey58.SetKey(sek.kp, CChainParams::EXT_PUBLIC_KEY_BTC);
        else
            eKey58.SetKeyP(sek.kp);

        obj.pushKV("epkey", eKey58.ToString());
    };

    obj.pushKV("num_derives", strprintf("%u", sek.nGenerated));
    obj.pushKV("num_derives_hardened", strprintf("%u", sek.nHGenerated));

    return 0;
};

int KeyInfo(CHDWallet *pwallet, CKeyID &idMaster, CKeyID &idKey, int nShowKeys, UniValue &obj, std::string &sError)
{
    CStoredExtKey sek;
    {
        LOCK(pwallet->cs_wallet);
        CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");

        if (!wdb.ReadExtKey(idKey, sek))
        {
            sError = "Key not found in wallet.";
            return 1;
        };
    }

    return KeyInfo(pwallet, idMaster, idKey, sek, nShowKeys, obj, sError);
};

class ListExtCallback : public LoopExtKeyCallback
{
public:
    ListExtCallback(CHDWallet *pwalletIn, UniValue *arr, int _nShowKeys)
    {
        pwallet = pwalletIn;
        nItems = 0;
        rvArray = arr;
        nShowKeys = _nShowKeys;

        if (pwallet && pwallet->pEKMaster)
            idMaster = pwallet->pEKMaster->GetID();
    };

    int ProcessKey(CKeyID &id, CStoredExtKey &sek)
    {
        nItems++;
        UniValue obj(UniValue::VOBJ);
        if (0 != KeyInfo(pwallet, idMaster, id, sek, nShowKeys, obj, sError))
        {
            obj.pushKV("id", sek.GetIDString58());
            obj.pushKV("error", sError);
        };

        rvArray->push_back(obj);
        return 0;
    };

    int ProcessAccount(CKeyID &id, CExtKeyAccount &sea)
    {
        nItems++;
        UniValue obj(UniValue::VOBJ);

        bool fAllChains = nShowKeys > 2 ? true : false;
        if (0 != AccountInfo(pwallet, &sea, nShowKeys, fAllChains, obj, sError))
        {
            obj.pushKV("id", sea.GetIDString58());
            obj.pushKV("error", sError);
        };

        rvArray->push_back(obj);
        return 0;
    };

    std::string sError;
    int nItems;
    int nShowKeys;
    CKeyID idMaster;
    UniValue *rvArray;
};

int ListLooseExtKeys(CHDWallet *pwallet, int nShowKeys, UniValue &ret, size_t &nKeys)
{
    ListExtCallback cbc(pwallet, &ret, nShowKeys);

    if (0 != LoopExtKeysInDB(pwallet, true, false, cbc))
        return errorN(1, "LoopExtKeys failed.");

    nKeys = cbc.nItems;

    return 0;
};

int ListAccountExtKeys(CHDWallet *pwallet, int nShowKeys, UniValue &ret, size_t &nKeys)
{
    ListExtCallback cbc(pwallet, &ret, nShowKeys);

    if (0 != LoopExtAccountsInDB(pwallet, true, cbc))
        return errorN(1, "LoopExtKeys failed.");

    nKeys = cbc.nItems;

    return 0;
};

int ManageExtKey(CStoredExtKey &sek, std::string &sOptName, std::string &sOptValue, UniValue &result, std::string &sError)
{
    if (sOptName == "label")
    {
        if (sOptValue.length() == 0)
            sek.sLabel = sOptValue;

        result.pushKV("set_label", sek.sLabel);
    } else
    if (sOptName == "active")
    {
        if (sOptValue.length() > 0)
        {
            if (part::IsStringBoolPositive(sOptValue))
                sek.nFlags |= EAF_ACTIVE;
            else
                sek.nFlags &= ~EAF_ACTIVE;
        };

        result.pushKV("set_active", sek.nFlags & EAF_ACTIVE ? "true" : "false");
    } else
    if (sOptName == "receive_on")
    {
        if (sOptValue.length() > 0)
        {
            if (part::IsStringBoolPositive(sOptValue))
                sek.nFlags |= EAF_RECEIVE_ON;
            else
                sek.nFlags &= ~EAF_RECEIVE_ON;
        };

        result.pushKV("receive_on", sek.nFlags & EAF_RECEIVE_ON ? "true" : "false");
    } else
    if (sOptName == "look_ahead")
    {
        uint64_t nLookAhead = gArgs.GetArg("-defaultlookaheadsize", N_DEFAULT_LOOKAHEAD);

        if (sOptValue.length() > 0)
        {
            char *pend;
            errno = 0;
            nLookAhead = strtoul(sOptValue.c_str(), &pend, 10);
            if (errno != 0 || !pend || *pend != '\0')
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed: look_ahead invalid number.");

            if (nLookAhead < 1 || nLookAhead > 1000)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed: look_ahead number out of range.");

            std::vector<uint8_t> v;
            sek.mapValue[EKVT_N_LOOKAHEAD] = SetCompressedInt64(v, nLookAhead);
            result.pushKV("note", "Wallet must be restarted to reload lookahead pool.");
        };

        mapEKValue_t::iterator itV = sek.mapValue.find(EKVT_N_LOOKAHEAD);
        if (itV != sek.mapValue.end())
        {
            nLookAhead = GetCompressedInt64(itV->second, nLookAhead);
            result.pushKV("look_ahead", (int)nLookAhead);
        } else
        {
            result.pushKV("look_ahead", "default");
        };
    } else
    {
        // List all possible
        result.pushKV("label", sek.sLabel);
        result.pushKV("active", sek.nFlags & EAF_ACTIVE ? "true" : "false");
        result.pushKV("receive_on", sek.nFlags & EAF_RECEIVE_ON ? "true" : "false");


        mapEKValue_t::iterator itV = sek.mapValue.find(EKVT_N_LOOKAHEAD);
        if (itV != sek.mapValue.end())
        {
            uint64_t nLookAhead = GetCompressedInt64(itV->second, nLookAhead);
            result.pushKV("look_ahead", (int)nLookAhead);
        } else
        {
            result.pushKV("look_ahead", "default");
        };
    };

    return 0;
};

int ManageExtAccount(CExtKeyAccount &sea, std::string &sOptName, std::string &sOptValue, UniValue &result, std::string &sError)
{
    if (sOptName == "label")
    {
        if (sOptValue.length() > 0)
            sea.sLabel = sOptValue;

        result.pushKV("set_label", sea.sLabel);
    } else
    if (sOptName == "active")
    {
        if (sOptValue.length() > 0)
        {
            if (part::IsStringBoolPositive(sOptValue))
                sea.nFlags |= EAF_ACTIVE;
            else
                sea.nFlags &= ~EAF_ACTIVE;
        };

        result.pushKV("set_active", sea.nFlags & EAF_ACTIVE ? "true" : "false");
    } else
    {
        // List all possible
        result.pushKV("label", sea.sLabel);
        result.pushKV("active", sea.nFlags & EAF_ACTIVE ? "true" : "false");
    };

    return 0;
};

static int ExtractExtKeyId(const std::string &sInKey, CKeyID &keyId, CChainParams::Base58Type prefix)
{
    CExtKey58 eKey58;
    CExtKeyPair ekp;
    CBitcoinAddress addr;

    if (addr.SetString(sInKey)
        && addr.IsValid(prefix)
        && addr.GetKeyID(keyId, prefix))
    {
        // keyId is set
    } else
    if (eKey58.Set58(sInKey.c_str()) == 0)
    {
        ekp = eKey58.GetKey();
        keyId = ekp.GetID();
    } else
    {
        throw std::runtime_error("Invalid key.");
    };
    return 0;
};

UniValue extkey(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    static const char *help = ""
        "extkey [info|list|account|gen|import|importAccount|setMaster|setDefaultAccount|deriveAccount|options]\n"
        "extkey [\"info\"] [key] [path]\n"
        "extkey list [show_secrets] - default\n"
        "    List loose and account ext keys.\n"
        "extkey account <key/id> [show_secrets]\n"
        "    Display details of account.\n"
        "extkey key <key/id> [show_secrets]\n"
        "    Display details of loose key.\n"
        "extkey gen [passphrase] [num hashes] [seed string]\n"
        "    If no passhrase is specified key will be generated from random data.\n"
        "    Warning: It is recommended to not use the passphrase\n"
        "extkey import <key> [label] [bip44] [save_bip44_key]\n"
        "    Add loose key to wallet.\n"
        "    If bip44 is set import will add the key derived from <key> on the bip44 path.\n"
        "    If save_bip44_key is set import will save the bip44 key to the wallet.\n"
        "extkey importAccount <key> [time_scan_from] [label] \n"
        "    Add account key to wallet.\n"
        "        time_scan_from: N no check, Y-m-d date to start scanning the blockchain for owned txns.\n"
        "extkey setMaster <key/id>\n"
        "    Set a private ext key as current master key.\n"
        "    key can be a extkeyid or full key, but must be in the wallet.\n"
        "extkey setDefaultAccount <id>\n"
        "    Set an account as the default.\n"
        "extkey deriveAccount [label] [path]\n"
        "    Make a new account from the current master key, saves to wallet.\n"
        "extkey options <key> [optionName] [newValue]\n"
        "    Manage keys and accounts\n"
        "\n";


    // default mode is list unless 1st parameter is a key - then mode is set to info

    // path:
    // master keys are hashed with an integer (child_index) to form child keys
    // each child key can spawn more keys
    // payments etc are not send to keys derived from the master keys
    //  m - master key
    //  m/0 - key0 (1st) key derived from m
    //  m/1/2 key2 (3rd) key derived from key1 derived from m

    // hardened keys are keys with (child_index) > 2^31
    // it's not possible to compute the next extended public key in the sequence from a hardened public key (still possible with a hardened private key)

    // this maintains privacy, you can give hardened public keys to customers
    // and they will not be able to compute/guess the key you give out to other customers
    // but will still be able to send payments to you on the 2^32 keys derived from the public key you provided


    // accounts to receive must be non-hardened
    //   - locked wallets must be able to derive new keys as they receive

    if (request.fHelp || request.params.size() > 5) // defaults to info, will always take at least 1 parameter
        throw std::runtime_error(help);

    EnsureWalletIsUnlocked(pwallet);

    std::string mode = "list";
    std::string sInKey = "";

    uint32_t nParamOffset = 0;
    if (request.params.size() > 0)
    {
        std::string s = request.params[0].get_str();
        std::string st = " " + s + " "; // Note the spaces
        std::transform(st.begin(), st.end(), st.begin(), ::tolower);
        static const char *pmodes = " info list gen account key import importaccount setmaster setdefaultaccount deriveaccount options ";
        if (strstr(pmodes, st.c_str()) != nullptr)
        {
            st.erase(std::remove(st.begin(), st.end(), ' '), st.end());
            mode = st;

            nParamOffset = 1;
        } else
        {
            sInKey = s;
            mode = "info";
            nParamOffset = 1;
        };
    };

    CBitcoinExtKey bvk;
    CBitcoinExtPubKey bpk;

    std::vector<uint8_t> vchVersionIn;
    vchVersionIn.resize(4);

    UniValue result(UniValue::VOBJ);

    if (mode == "info")
    {
        std::string sMode = "info"; // info lists details of bip32 key, m displays internal key

        if (sInKey.length() == 0)
        {
            if (request.params.size() > nParamOffset)
            {
                sInKey = request.params[nParamOffset].get_str();
                nParamOffset++;
            };
        };

        if (request.params.size() > nParamOffset)
            sMode = request.params[nParamOffset].get_str();

        UniValue keyInfo(UniValue::VOBJ);
        std::vector<uint8_t> vchOut;

        if (!DecodeBase58(sInKey.c_str(), vchOut))
            throw std::runtime_error("DecodeBase58 failed.");
        if (!VerifyChecksum(vchOut))
            throw std::runtime_error("VerifyChecksum failed.");

        size_t keyLen = vchOut.size();
        std::string sError;

        if (keyLen != BIP32_KEY_LEN)
            throw std::runtime_error(strprintf("Unknown ext key length '%d'", keyLen));

        if (memcmp(&vchOut[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY)[0], 4) == 0
            || memcmp(&vchOut[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY_BTC)[0], 4) == 0)
        {
            if (ExtKeyPathV(sMode, vchOut, keyInfo, sError) != 0)
                throw std::runtime_error(strprintf("ExtKeyPathV failed %s.", sError.c_str()));
        } else
        if (memcmp(&vchOut[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY)[0], 4) == 0
            || memcmp(&vchOut[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY_BTC)[0], 4) == 0)
        {
            if (ExtKeyPathP(sMode, vchOut, keyInfo, sError) != 0)
                throw std::runtime_error(strprintf("ExtKeyPathP failed %s.", sError.c_str()));
        } else
        {
            throw std::runtime_error(strprintf("Unknown prefix '%s'", sInKey.substr(0, 4)));
        };


        result.pushKV("key_info", keyInfo);
    } else
    if (mode == "list")
    {
        UniValue ret(UniValue::VARR);

        int nListFull = 0; // 0 id only, 1 id+pubkey, 2 id+pubkey+secret
        if (request.params.size() > nParamOffset)
        {
            std::string st = request.params[nParamOffset].get_str();
            if (part::IsStringBoolPositive(st))
                nListFull = 2;

            nParamOffset++;
        };

        size_t nKeys = 0, nAcc = 0;

        {
            LOCK(pwallet->cs_wallet);
            ListLooseExtKeys(pwallet, nListFull, ret, nKeys);
            ListAccountExtKeys(pwallet, nListFull, ret, nAcc);
        } // cs_wallet

        if (nKeys + nAcc > 0)
            return ret;

        result.pushKV("result", "No keys to list.");
    } else
    if (mode == "account"
        || mode == "key")
    {
        CKeyID keyId;
        if (request.params.size() > nParamOffset)
        {
            sInKey = request.params[nParamOffset].get_str();
            nParamOffset++;

            if (mode == "account" && sInKey == "default")
                keyId = pwallet->idDefaultAccount;
            else
                ExtractExtKeyId(sInKey, keyId, mode == "account" ? CChainParams::EXT_ACC_HASH : CChainParams::EXT_KEY_HASH);
        } else
        {
            if (mode == "account")
            {
                // Display default account
                keyId = pwallet->idDefaultAccount;
            };
        }
        if (keyId.IsNull())
            throw std::runtime_error(strprintf("Must specify ext key or id %s.", mode == "account" ? "or 'default'" : ""));

        int nListFull = 0; // 0 id only, 1 id+pubkey, 2 id+pubkey+secret
        if (request.params.size() > nParamOffset)
        {
            std::string st = request.params[nParamOffset].get_str();
            if (part::IsStringBoolPositive(st))
                nListFull = 2;

            nParamOffset++;
        };

        std::string sError;
        if (mode == "account")
        {
            if (0 != AccountInfo(pwallet, keyId, nListFull, true, result, sError))
                throw std::runtime_error("AccountInfo failed: " + sError);
        } else
        {
            CKeyID idMaster;
            if (pwallet->pEKMaster)
                idMaster = pwallet->pEKMaster->GetID();
            else
                LogPrintf("%s: Warning: Master key isn't set!\n", __func__);
            if (0 != KeyInfo(pwallet, idMaster, keyId, nListFull, result, sError))
                throw std::runtime_error("KeyInfo failed: " + sError);
        };
    } else
    if (mode == "gen")
    {
        // Make a new master key
        // from random or passphrase + int + seed string

        CExtKey newKey;
        CBitcoinExtKey b58Key;

        if (request.params.size() > 1)
        {
            std::string sPassphrase = request.params[1].get_str();
            int32_t nHashes = 100;
            std::string sSeed = "Bitcoin seed";

            // Generate from passphrase
            //   allow generator string and nhashes to be specified
            //   To allow importing of bip32 strings from other systems
            //   Match bip32.org: bip32 gen "pass" 50000 "Bitcoin seed"

            if (request.params.size() > 2)
            {
                std::stringstream sstr(request.params[2].get_str());

                sstr >> nHashes;
                if (!sstr)
                    throw std::runtime_error("Invalid num hashes");

                if (nHashes < 1)
                    throw std::runtime_error("Num hashes must be 1 or more.");
            };

            if (request.params.size() > 3)
            {
                sSeed = request.params[3].get_str();
            };

            if (request.params.size() > 4)
                throw std::runtime_error(help);

            pwallet->ExtKeyNew32(newKey, sPassphrase.c_str(), nHashes, sSeed.c_str());

            result.pushKV("warning",
                "If the same passphrase is used by another your privacy and coins will be compromised.\n"
                "It is not recommended to use this feature - if you must, pick very unique values for passphrase, num hashes and generator parameters.");
        } else
        {
             pwallet->ExtKeyNew32(newKey);
        };

        b58Key.SetKey(newKey);

        result.pushKV("result", b58Key.ToString());
    } else
    if (mode == "import")
    {
        if (sInKey.length() == 0)
        {
            if (request.params.size() > nParamOffset)
            {
                sInKey = request.params[nParamOffset].get_str();
                nParamOffset++;
            };
        };

        CStoredExtKey sek;
        if (request.params.size() > nParamOffset)
        {
            sek.sLabel = request.params[nParamOffset].get_str();
            nParamOffset++;
        };

        bool fBip44 = false;
        if (request.params.size() > nParamOffset)
        {
            std::string s = request.params[nParamOffset].get_str();
            if (part::IsStringBoolPositive(s))
                fBip44 = true;
            nParamOffset++;
        };

        bool fSaveBip44 = false;
        if (request.params.size() > nParamOffset)
        {
            std::string s = request.params[nParamOffset].get_str();
            if (part::IsStringBoolPositive(s))
                fSaveBip44 = true;
            nParamOffset++;
        };

        std::vector<uint8_t> v;
        sek.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());

        CExtKey58 eKey58;
        if (eKey58.Set58(sInKey.c_str()) != 0)
            throw std::runtime_error("Import failed - Invalid key.");

        if (fBip44)
        {
            if (!eKey58.IsValid(CChainParams::EXT_SECRET_KEY_BTC))
                throw std::runtime_error("Import failed - BIP44 key must begin with a bitcoin secret key prefix.");
        } else
        {
            if (!eKey58.IsValid(CChainParams::EXT_SECRET_KEY)
                && !eKey58.IsValid(CChainParams::EXT_PUBLIC_KEY_BTC))
                throw std::runtime_error("Import failed - Key must begin with a particl prefix.");
        };

        sek.kp = eKey58.GetKey();

        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw std::runtime_error("TxnBegin failed.");

            int rv;
            CKeyID idDerived;
            if (0 != (rv = pwallet->ExtKeyImportLoose(&wdb, sek, idDerived, fBip44, fSaveBip44)))
            {
                wdb.TxnAbort();
                throw std::runtime_error(strprintf("ExtKeyImportLoose failed, %s", ExtKeyGetString(rv)));
            };

            if (!wdb.TxnCommit())
                throw std::runtime_error("TxnCommit failed.");
            result.pushKV("result", "Success.");
            result.pushKV("key_label", sek.sLabel);
            result.pushKV("note", "Please backup your wallet."); // TODO: check for child of existing key?
        } // cs_wallet
    } else
    if (mode == "importaccount")
    {
        if (sInKey.length() == 0)
        {
            if (request.params.size() > nParamOffset)
            {
                sInKey = request.params[nParamOffset].get_str();
                nParamOffset++;
            };
        };

        int64_t nTimeStartScan = 1; // scan from start, 0 means no scan
        if (request.params.size() > nParamOffset)
        {
            std::string sVar = request.params[nParamOffset].get_str();
            nParamOffset++;

            if (sVar == "N")
            {
                nTimeStartScan = 0;
            } else
            if (part::IsStrOnlyDigits(sVar))
            {
                // Setting timestamp directly
                errno = 0;
                nTimeStartScan = strtoimax(sVar.c_str(), nullptr, 10);
                if (errno != 0)
                    throw std::runtime_error("Import Account failed - Parse time error.");
            } else
            {
                int year, month, day;

                if (sscanf(sVar.c_str(), "%d-%d-%d", &year, &month, &day) != 3)
                    throw std::runtime_error("Import Account failed - Parse time error.");

                struct tm tmdate;
                tmdate.tm_year = year - 1900;
                tmdate.tm_mon = month - 1;
                tmdate.tm_mday = day;
                time_t t = mktime(&tmdate);

                nTimeStartScan = t;
            };
        };

        std::string sLabel;
        if (request.params.size() > nParamOffset)
        {
            sLabel = request.params[nParamOffset].get_str();
            nParamOffset++;
        };

        CStoredExtKey sek;
        CExtKey58 eKey58;
        if (eKey58.Set58(sInKey.c_str()) == 0)
        {
            sek.kp = eKey58.GetKey();
        } else
        {
            throw std::runtime_error("Import Account failed - Invalid key.");
        };

        {
            //LOCK(pwallet->cs_wallet);
            LOCK2(cs_main, pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw std::runtime_error("TxnBegin failed.");

            int rv = pwallet->ExtKeyImportAccount(&wdb, sek, nTimeStartScan, sLabel);
            if (rv == 1)
            {
                wdb.TxnAbort();
                throw std::runtime_error("Import failed - ExtKeyImportAccount failed.");
            } else
            if (rv == 2)
            {
                wdb.TxnAbort();
                throw std::runtime_error("Import failed - account exists.");
            } else
            {
                if (!wdb.TxnCommit())
                    throw std::runtime_error("TxnCommit failed.");
                result.pushKV("result", "Success.");

                if (rv == 3)
                    result.pushKV("result", "secret added to existing account.");

                result.pushKV("account_label", sLabel);
                result.pushKV("scanned_from", nTimeStartScan);
                result.pushKV("note", "Please backup your wallet."); // TODO: check for child of existing key?
            };

        } // cs_wallet
    } else
    if (mode == "setmaster")
    {
        if (sInKey.length() == 0)
        {
            if (request.params.size() > nParamOffset)
            {
                sInKey = request.params[nParamOffset].get_str();
                nParamOffset++;
            } else
                throw std::runtime_error("Must specify ext key or id.");
        };

        CKeyID idNewMaster;
        ExtractExtKeyId(sInKey, idNewMaster, CChainParams::EXT_KEY_HASH);

        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw std::runtime_error("TxnBegin failed.");

            int rv;
            if (0 != (rv = pwallet->ExtKeySetMaster(&wdb, idNewMaster)))
            {
                wdb.TxnAbort();
                throw std::runtime_error(strprintf("ExtKeySetMaster failed, %s.", ExtKeyGetString(rv)));
            };
            if (!wdb.TxnCommit())
                throw std::runtime_error("TxnCommit failed.");
            result.pushKV("result", "Success.");
        } // cs_wallet

    } else
    if (mode == "setdefaultaccount")
    {
        if (sInKey.length() == 0)
        {
            if (request.params.size() > nParamOffset)
            {
                sInKey = request.params[nParamOffset].get_str();
                nParamOffset++;
            } else
                throw std::runtime_error("Must specify ext key or id.");
        };

        CKeyID idNewDefault;
        CKeyID idOldDefault = pwallet->idDefaultAccount;
        CBitcoinAddress addr;

        CExtKeyAccount *sea = new CExtKeyAccount();

        if (addr.SetString(sInKey)
            && addr.IsValid(CChainParams::EXT_ACC_HASH)
            && addr.GetKeyID(idNewDefault, CChainParams::EXT_ACC_HASH))
        {
            // idNewDefault is set
        };

        int rv;
        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");

            if (!wdb.TxnBegin())
            {
                delete sea;
                throw std::runtime_error("TxnBegin failed.");
            };
            if (0 != (rv = pwallet->ExtKeySetDefaultAccount(&wdb, idNewDefault)))
            {
                delete sea;
                wdb.TxnAbort();
                throw std::runtime_error(strprintf("ExtKeySetDefaultAccount failed, %s.", ExtKeyGetString(rv)));
            };
            if (!wdb.TxnCommit())
            {
                delete sea;
                pwallet->idDefaultAccount = idOldDefault;
                throw std::runtime_error("TxnCommit failed.");
            };

            result.pushKV("result", "Success.");
        } // cs_wallet

    } else
    if (mode == "deriveaccount")
    {
        std::string sLabel, sPath;
        if (request.params.size() > nParamOffset)
        {
            sLabel = request.params[nParamOffset].get_str();
            nParamOffset++;
        };

        if (request.params.size() > nParamOffset)
        {
            sPath = request.params[nParamOffset].get_str();
            nParamOffset++;
        };

        for (; nParamOffset < request.params.size(); nParamOffset++)
        {
            std::string strParam = request.params[nParamOffset].get_str();
            std::transform(strParam.begin(), strParam.end(), strParam.begin(), ::tolower);

            throw std::runtime_error(strprintf("Unknown parameter '%s'", strParam.c_str()));
        };

        CExtKeyAccount *sea = new CExtKeyAccount();

        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw std::runtime_error("TxnBegin failed.");

            int rv;
            if ((rv = pwallet->ExtKeyDeriveNewAccount(&wdb, sea, sLabel, sPath)) != 0)
            {
                wdb.TxnAbort();
                result.pushKV("result", "Failed.");
                result.pushKV("reason", ExtKeyGetString(rv));
            } else
            {
                if (!wdb.TxnCommit())
                    throw std::runtime_error("TxnCommit failed.");

                result.pushKV("result", "Success.");
                result.pushKV("account", sea->GetIDString58());
                CStoredExtKey *sekAccount = sea->ChainAccount();
                if (sekAccount)
                {
                    CExtKey58 eKey58;
                    eKey58.SetKeyP(sekAccount->kp);
                    result.pushKV("public key", eKey58.ToString());
                };

                if (sLabel != "")
                    result.pushKV("label", sLabel);
            };
        } // cs_wallet
    } else
    if (mode == "options")
    {
        std::string sOptName, sOptValue, sError;
        if (sInKey.length() == 0)
        {
            if (request.params.size() > nParamOffset)
            {
                sInKey = request.params[nParamOffset].get_str();
                nParamOffset++;
            } else
                throw std::runtime_error("Must specify ext key or id.");
        };
        if (request.params.size() > nParamOffset)
        {
            sOptName = request.params[nParamOffset].get_str();
            nParamOffset++;
        };
        if (request.params.size() > nParamOffset)
        {
            sOptValue = request.params[nParamOffset].get_str();
            nParamOffset++;
        };

        CBitcoinAddress addr;

        CKeyID id;
        if (!addr.SetString(sInKey))
            throw std::runtime_error("Invalid key or account id.");

        bool fAccount = false;
        bool fKey = false;
        if (addr.IsValid(CChainParams::EXT_KEY_HASH)
            && addr.GetKeyID(id, CChainParams::EXT_KEY_HASH))
        {
            // id is set
            fKey = true;
        } else
        if (addr.IsValid(CChainParams::EXT_ACC_HASH)
            && addr.GetKeyID(id, CChainParams::EXT_ACC_HASH))
        {
            // id is set
            fAccount = true;
        } else
        if (addr.IsValid(CChainParams::EXT_PUBLIC_KEY))
        {
            CExtKeyPair ek = boost::get<CExtKeyPair>(addr.Get());

            id = ek.GetID();

            ExtKeyAccountMap::iterator it = pwallet->mapExtAccounts.find(id);
            if (it != pwallet->mapExtAccounts.end())
                fAccount = true;
            else
                fKey = true;
        } else
        {
            throw std::runtime_error("Invalid key or account id.");
        };

        CStoredExtKey sek;
        CExtKeyAccount sea;
        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw std::runtime_error("TxnBegin failed.");

            if (fKey)
            {
                // Try key in memory first
                CStoredExtKey *pSek;
                ExtKeyMap::iterator it = pwallet->mapExtKeys.find(id);

                if (it != pwallet->mapExtKeys.end())
                {
                    pSek = it->second;
                } else
                if (wdb.ReadExtKey(id, sek))
                {
                    pSek = &sek;
                } else
                {
                    wdb.TxnAbort();
                    throw std::runtime_error("Key not in wallet.");
                };

                if (0 != ManageExtKey(*pSek, sOptName, sOptValue, result, sError))
                {
                    wdb.TxnAbort();
                    throw std::runtime_error("Error: " + sError);
                };

                if (sOptValue.length() > 0
                    && !wdb.WriteExtKey(id, *pSek))
                {
                    wdb.TxnAbort();
                    throw std::runtime_error("WriteExtKey failed.");
                };
            };

            if (fAccount)
            {
                CExtKeyAccount *pSea;
                ExtKeyAccountMap::iterator it = pwallet->mapExtAccounts.find(id);
                if (it != pwallet->mapExtAccounts.end())
                {
                    pSea = it->second;
                } else
                if (wdb.ReadExtAccount(id, sea))
                {
                    pSea = &sea;
                } else
                {
                    wdb.TxnAbort();
                    throw std::runtime_error("Account not in wallet.");
                };

                if (0 != ManageExtAccount(*pSea, sOptName, sOptValue, result, sError))
                {
                    wdb.TxnAbort();
                    throw std::runtime_error("Error: " + sError);
                };

                if (sOptValue.length() > 0
                    && !wdb.WriteExtAccount(id, *pSea))
                {
                    wdb.TxnAbort();
                    throw std::runtime_error("Write failed.");
                };
            };

            if (sOptValue.length() == 0)
            {
                wdb.TxnAbort();
            } else
            {
                if (!wdb.TxnCommit())
                    throw std::runtime_error("TxnCommit failed.");
                result.pushKV("result", "Success.");
            };
        } // cs_wallet

    } else
    {
        throw std::runtime_error(help);
    };

    return result;
};

UniValue extkeyimportinternal(const JSONRPCRequest &request, bool fGenesisChain)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    EnsureWalletIsUnlocked(pwallet);

    if (request.params.size() < 1)
        throw std::runtime_error("Please specify a private extkey or mnemonic phrase.");

    std::string sMnemonic = request.params[0].get_str();
    bool fSaveBip44Root = false;
    std::string sLblMaster = "Master Key";
    std::string sLblAccount = "Default Account";
    std::string sPassphrase = "";
    std::string sError;

    if (request.params.size() > 1)
        sPassphrase = request.params[1].get_str();


    if (request.params.size() > 2)
    {
        std::string s = request.params[2].get_str();

        if (!part::GetStringBool(s, fSaveBip44Root))
            throw std::runtime_error(strprintf("Unknown argument for save_bip44_root: %s.", s.c_str()));
    };

    if (request.params.size() > 3)
        sLblMaster = request.params[3].get_str();
    if (request.params.size() > 4)
        sLblAccount = request.params[4].get_str();
    if (request.params.size() > 5)
        throw std::runtime_error(strprintf("Unknown parameter '%s'", request.params[5].get_str()));

    LogPrintf("Importing master key and account with labels '%s', '%s'.\n", sLblMaster.c_str(), sLblAccount.c_str());

    CExtKey58 eKey58;
    CExtKeyPair ekp;
    if (eKey58.Set58(sMnemonic.c_str()) == 0)
    {
        if (!eKey58.IsValid(CChainParams::EXT_SECRET_KEY)
            && !eKey58.IsValid(CChainParams::EXT_SECRET_KEY_BTC))
            throw std::runtime_error("Please specify a private extkey or mnemonic phrase.");

        // Key was provided directly
        ekp = eKey58.GetKey();
    } else
    {
        std::vector<uint8_t> vSeed, vEntropy;

        // First check the mnemonic is valid
        if (0 != MnemonicDecode(-1, sMnemonic, vEntropy, sError))
            throw std::runtime_error(strprintf("MnemonicDecode failed: %s", sError.c_str()));

        if (0 != MnemonicToSeed(sMnemonic, sPassphrase, vSeed))
            throw std::runtime_error("MnemonicToSeed failed.");

        ekp.SetMaster(&vSeed[0], vSeed.size());
    };

    CStoredExtKey sek;
    sek.sLabel = sLblMaster;

    std::vector<uint8_t> v;
    sek.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());
    sek.kp = ekp;

    UniValue result(UniValue::VOBJ);

    int rv;
    bool fBip44 = true;
    CKeyID idDerived;
    CExtKeyAccount *sea;

    {
        LOCK(pwallet->cs_wallet);
        CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
        if (!wdb.TxnBegin())
            throw std::runtime_error("TxnBegin failed.");

        if (0 != (rv = pwallet->ExtKeyImportLoose(&wdb, sek, idDerived, fBip44, fSaveBip44Root)))
        {
            wdb.TxnAbort();
            throw std::runtime_error(strprintf("ExtKeyImportLoose failed, %s", ExtKeyGetString(rv)));
        };

        if (0 != (rv = pwallet->ExtKeySetMaster(&wdb, idDerived)))
        {
            wdb.TxnAbort();
            throw std::runtime_error(strprintf("ExtKeySetMaster failed, %s.", ExtKeyGetString(rv)));
        };

        sea = new CExtKeyAccount();
        if (0 != (rv = pwallet->ExtKeyDeriveNewAccount(&wdb, sea, sLblAccount)))
        {
            pwallet->ExtKeyRemoveAccountFromMapsAndFree(sea);
            wdb.TxnAbort();
            throw std::runtime_error(strprintf("ExtKeyDeriveNewAccount failed, %s.", ExtKeyGetString(rv)));
        };

        CKeyID idNewDefaultAccount = sea->GetID();
        CKeyID idOldDefault = pwallet->idDefaultAccount;

        if (0 != (rv = pwallet->ExtKeySetDefaultAccount(&wdb, idNewDefaultAccount)))
        {
            pwallet->ExtKeyRemoveAccountFromMapsAndFree(sea);
            wdb.TxnAbort();
            throw std::runtime_error(strprintf("ExtKeySetDefaultAccount failed, %s.", ExtKeyGetString(rv)));
        };

        if (fGenesisChain)
        {
            std::string genesisChainLabel = "Genesis Import";
            uint32_t genesisChainNo = 444444;
            CStoredExtKey *sekGenesisChain = new CStoredExtKey();

            if (0 != (rv = pwallet->NewExtKeyFromAccount(&wdb, idNewDefaultAccount,
                genesisChainLabel, sekGenesisChain, nullptr, &genesisChainNo)))
            {
                delete sekGenesisChain;
                pwallet->ExtKeyRemoveAccountFromMapsAndFree(sea);
                wdb.TxnAbort();
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf(_("NewExtKeyFromAccount failed, %s."), ExtKeyGetString(rv)));
            };
        };

        if (!wdb.TxnCommit())
        {
            pwallet->idDefaultAccount = idOldDefault;
            pwallet->ExtKeyRemoveAccountFromMapsAndFree(sea);
            throw std::runtime_error("TxnCommit failed.");
        };
    } // cs_wallet

    if (0 != pwallet->ScanChainFromTime(1))
        throw std::runtime_error("ScanChainFromTime failed.");

    CBitcoinAddress addr;
    addr.Set(idDerived, CChainParams::EXT_KEY_HASH);
    result.pushKV("result", "Success.");
    result.pushKV("master_id", addr.ToString());
    result.pushKV("master_label", sek.sLabel);

    result.pushKV("account_id", sea->GetIDString58());
    result.pushKV("account_label", sea->sLabel);

    result.pushKV("note", "Please backup your wallet.");

    return result;
}

UniValue extkeyimportmaster(const JSONRPCRequest &request)
{
    static const char *help = ""
        "extkeyimportmaster <mnemonic/key> [passphrase] [save_bip44_root] [master_label] [account_label]\n"
        "Import master key from bip44 mnemonic root key and derive default account.\n"
        "       Use '-stdin' to be prompted to enter a passphrase.\n"
        "       if mnemonic is blank, defaults to '-stdin'.\n"
        "   passphrase:         passphrase when importing mnemonic - default blank.\n"
        "       Use '-stdin' to be prompted to enter a passphrase.\n"
        "   save_bip44_root:    Save bip44 root key to wallet - default false.\n"
        "   master_label:       Label for master key - default 'Master Key'.\n"
        "   account_label:      Label for account - default 'Default Account'.\n"
        "Examples:\n"
        "   extkeyimportmaster -stdin -stdin false label_master label_account\n"
        "\n";

    // Doesn't generate key, require users to run mnemonic new, more likely they'll save the phrase

    if (request.fHelp)
        throw std::runtime_error(help);

    return extkeyimportinternal(request, false);
};

UniValue extkeygenesisimport(const JSONRPCRequest &request)
{
    static const char *help = ""
        "extkeygenesisimport <mnemonic/key> [passphrase] [save_bip44_root] [master_label] [account_label]\n"
        "Import master key from bip44 mnemonic root key and derive default account.\n"
        "Derives an extra chain from path 444444 to receive imported coin.\n"
        "       Use '-stdin' to be prompted to enter a passphrase.\n"
        "       if mnemonic is blank, defaults to '-stdin'.\n"
        "   passphrase:         passphrase when importing mnemonic - default blank.\n"
        "       Use '-stdin' to be prompted to enter a passphrase.\n"
        "   save_bip44_root:    Save bip44 root key to wallet - default false.\n"
        "   master_label:       Label for master key - default 'Master Key'.\n"
        "   account_label:      Label for account - default 'Default Account'.\n"
        "Examples:\n"
        "   extkeygenesisimport -stdin -stdin false label_master label_account\n"
        "\n";

    if (request.fHelp)
        throw std::runtime_error(help);

    return extkeyimportinternal(request, true);
}


UniValue keyinfo(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    static const char *help = ""
        "keyinfo <key> [show_secret]\n"
        "Return public key.\n"
        "\n";

    if (request.fHelp) // defaults to info, will always take at least 1 parameter
        throw std::runtime_error(help);

    if (request.params.size() < 1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Please specify a key."));

    // TODO: show public keys with unlocked wallet?
    EnsureWalletIsUnlocked(pwallet);


    std::string sKey = request.params[0].get_str();

    UniValue result(UniValue::VOBJ);


    CExtKey58 eKey58;
    CExtKeyPair ekp;
    if (eKey58.Set58(sKey.c_str()) == 0)
    {
        // Key was provided directly
        ekp = eKey58.GetKey();
        result.pushKV("key_type", "extaddress");
        result.pushKV("mode", ekp.IsValidV() ? "private" : "public");

        CKeyID id = ekp.GetID();

        result.pushKV("owned", pwallet->HaveExtKey(id) ? "true" : "false");

        std::string sError;

        std::vector<uint8_t> vchOut;

        if (!DecodeBase58(sKey.c_str(), vchOut))
            throw std::runtime_error("DecodeBase58 failed.");
        if (!VerifyChecksum(vchOut))
            throw std::runtime_error("VerifyChecksum failed.");

        if (ekp.IsValidV())
        {
            if (0 != ExtractBip32InfoV(vchOut, result, sError))
                throw std::runtime_error(strprintf("ExtractBip32InfoV failed %s.", sError.c_str()));
        } else
        {
            if (0 != ExtractBip32InfoP(vchOut, result, sError))
                throw std::runtime_error(strprintf("ExtractBip32InfoP failed %s.", sError.c_str()));
        };

        return result;
    }

    CBitcoinAddress addr;
    if (addr.SetString(sKey))
    {
        result.pushKV("key_type", "address");

        CKeyID id;
        CPubKey pk;
        if (!addr.GetKeyID(id))
            throw JSONRPCError(RPC_INTERNAL_ERROR, _("GetKeyID failed."));

        if (!pwallet->GetPubKey(id, pk))
        {
            result.pushKV("result", "Address not in wallet.");
            return result;
        };

        result.pushKV("public_key", HexStr(pk.begin(), pk.end()));

        result.pushKV("result", "Success.");
        return result;
    }

    throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown keytype."));
};

UniValue extkeyaltversion(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "extkeyaltversion <ext_key>\n"
            "Returns the provided ext_key encoded with alternate version bytes.\n"
            "If the provided ext_key has a Bitcoin prefix the output will be encoded with a Particl prefix.\n"
            "If the provided ext_key has a Particl prefix the output will be encoded with a Bitcoin prefix.");

    std::string sKeyIn = request.params[0].get_str();
    std::string sKeyOut;

    CExtKey58 eKey58;
    CExtKeyPair ekp;
    if (eKey58.Set58(sKeyIn.c_str()) != 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Invalid input key."));

    // TODO: handle testnet keys on main etc
    if (eKey58.IsValid(CChainParams::EXT_SECRET_KEY_BTC))
        return eKey58.ToStringVersion(CChainParams::EXT_SECRET_KEY);
    if (eKey58.IsValid(CChainParams::EXT_SECRET_KEY))
        return eKey58.ToStringVersion(CChainParams::EXT_SECRET_KEY_BTC);

    if (eKey58.IsValid(CChainParams::EXT_PUBLIC_KEY_BTC))
        return eKey58.ToStringVersion(CChainParams::EXT_PUBLIC_KEY);
    if (eKey58.IsValid(CChainParams::EXT_PUBLIC_KEY))
        return eKey58.ToStringVersion(CChainParams::EXT_PUBLIC_KEY_BTC);

    throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown input key version."));
}


UniValue getnewextaddress(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "getnewextaddress [label] [childNo]\n"
            "Returns a new Particl ext address for receiving payments.\n"
            "label   (string, optional), if specified the key is added to the address book.\n"
            "childNo (int, optional), if specified, the account derive counter is not updated.");

    EnsureWalletIsUnlocked(pwallet);

    uint32_t nChild = 0;
    uint32_t *pChild = nullptr;
    std::string strLabel;
    const char *pLabel = nullptr;
    if (request.params.size() > 0)
    {
        strLabel = request.params[0].get_str();
        if (strLabel.size() > 0)
            pLabel = strLabel.c_str();
    };

    if (request.params.size() > 1)
    {
        nChild = request.params[1].get_int();
        pChild = &nChild;
    };

    CStoredExtKey *sek = new CStoredExtKey();
    if (0 != pwallet->NewExtKeyFromAccount(strLabel, sek, pLabel, pChild))
    {
        delete sek;
        throw JSONRPCError(RPC_WALLET_ERROR, _("NewExtKeyFromAccount failed."));
    };

    // CBitcoinAddress displays public key only
    return CBitcoinAddress(sek->kp).ToString();
}

UniValue getnewstealthaddress(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 4)
        throw std::runtime_error(
            "getnewstealthaddress [label] [num_prefix_bits] [prefix_num] [bech32]\n"
            "Returns a new Particl stealth address for receiving payments."
            "If num_prefix_bits is specified and > 0, the stealth address is created with a prefix.\n"
            "If prefix_num is not specified the prefix will be selected deterministically.\n"
            "prefix_num can be specified in base2, 10 or 16, for base 2 prefix_str must begin with 0b, 0x for base16.\n"
            "A 32bit integer will be created from prefix_num and the least significant num_prefix_bits will become the prefix.\n"
            "A stealth address created without a prefix will scan all incoming stealth transactions, irrespective of transaction prefixes.\n"
            "Stealth addresses with prefixes will scan only incoming stealth transactions with a matching prefix.\n"
            "Examples:\n"
            "   getnewstealthaddress \"lblTestSxAddrPrefix\" 3 \"0b101\" \n"
            + HelpRequiringPassphrase(pwallet));

    EnsureWalletIsUnlocked(pwallet);

    std::string sLabel;
    if (request.params.size() > 0)
        sLabel = request.params[0].get_str();

    uint32_t num_prefix_bits = 0;
    if (request.params.size() > 1)
    {
        std::string sTemp = request.params[1].get_str();
        char *pend;
        errno = 0;
        num_prefix_bits = strtoul(sTemp.c_str(), &pend, 10);
        if (errno != 0 || !pend || *pend != '\0')
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("num_prefix_bits invalid number."));
    };

    if (num_prefix_bits > 32)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("num_prefix_bits must be <= 32."));

    std::string sPrefix_num;
    if (request.params.size() > 2)
        sPrefix_num = request.params[2].get_str();

    bool fbech32 = request.params.size() > 3 ? request.params[3].get_bool() : false;

    CEKAStealthKey akStealth;
    std::string sError;
    if (0 != pwallet->NewStealthKeyFromAccount(sLabel, akStealth, num_prefix_bits, sPrefix_num.empty() ? nullptr : sPrefix_num.c_str(), fbech32))
        throw JSONRPCError(RPC_WALLET_ERROR, _("NewStealthKeyFromAccount failed."));

    CStealthAddress sxAddr;
    akStealth.SetSxAddr(sxAddr);

    return sxAddr.ToString(fbech32);
}

UniValue importstealthaddress(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 5)
        throw std::runtime_error(
            "importstealthaddress <scan_secret> <spend_secret> [label] [num_prefix_bits] [prefix_num]\n"
            "Import an owned stealth addresses.\n"
            "If num_prefix_bits is specified and > 0, the stealth address is created with a prefix.\n"
            "If prefix_num is not specified the prefix will be selected deterministically.\n"
            "prefix_num can be specified in base2, 10 or 16, for base 2 prefix_str must begin with 0b, 0x for base16.\n"
            "A 32bit integer will be created from prefix_num and the least significant num_prefix_bits will become the prefix.\n"
            "A stealth address created without a prefix will scan all incoming stealth transactions, irrespective of transaction prefixes.\n"
            "Stealth addresses with prefixes will scan only incoming stealth transactions with a matching prefix.\n"
            "Examples:\n"
            "   getnewstealthaddress \"lblTestSxAddrPrefix\" 3 \"0b101\" \n"
            + HelpRequiringPassphrase(pwallet));

    EnsureWalletIsUnlocked(pwallet);

    std::string sScanSecret  = request.params[0].get_str();
    std::string sSpendSecret = request.params[1].get_str();
    std::string sLabel;

    if (request.params.size() > 2)
        sLabel = request.params[2].get_str();

    uint32_t num_prefix_bits = 0;
    if (request.params.size() > 3)
    {
        std::string sTemp = request.params[3].get_str();
        char *pend;
        errno = 0;
        num_prefix_bits = strtoul(sTemp.c_str(), &pend, 10);
        if (errno != 0 || !pend || *pend != '\0')
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("num_prefix_bits invalid number."));
    };

    if (num_prefix_bits > 32)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("num_prefix_bits must be <= 32."));

    uint32_t nPrefix = 0;
    std::string sPrefix_num;
    if (request.params.size() > 4)
    {
        sPrefix_num = request.params[4].get_str();
        if (!ExtractStealthPrefix(sPrefix_num.c_str(), nPrefix))
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Could not convert prefix to number."));
    };

    std::vector<uint8_t> vchScanSecret;
    std::vector<uint8_t> vchSpendSecret;
    CBitcoinSecret wifScanSecret, wifSpendSecret;
    CKey skScan, skSpend;
    if (IsHex(sScanSecret))
    {
        vchScanSecret = ParseHex(sScanSecret);
    } else
    if (wifScanSecret.SetString(sScanSecret))
    {
        skScan = wifScanSecret.GetKey();
    } else
    {
        if (!DecodeBase58(sScanSecret, vchScanSecret))
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Could not decode scan secret as wif, hex or base58."));
    };
    if (vchScanSecret.size() > 0)
    {
        if (vchScanSecret.size() != 32)
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Scan secret is not 32 bytes."));
        skScan.Set(&vchScanSecret[0], true);
    };

    if (IsHex(sSpendSecret))
    {
        vchSpendSecret = ParseHex(sSpendSecret);
    } else
    if (wifSpendSecret.SetString(sSpendSecret))
    {
        skSpend = wifSpendSecret.GetKey();
    } else
    {
        if (!DecodeBase58(sSpendSecret, vchSpendSecret))
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Could not decode spend secret as hex or base58."));
    };
    if (vchSpendSecret.size() > 0)
    {
        if (vchSpendSecret.size() != 32)
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Spend secret is not 32 bytes."));
        skSpend.Set(&vchSpendSecret[0], true);
    };

    CStealthAddress sxAddr;
    sxAddr.label = sLabel;
    sxAddr.scan_secret = skScan;
    sxAddr.spend_secret_id = skSpend.GetPubKey().GetID();

    sxAddr.prefix.number_bits = num_prefix_bits;
    if (sxAddr.prefix.number_bits > 0)
    {
        if (sPrefix_num.empty())
        {
            // if pPrefix is null, set nPrefix from the hash of kSpend
            uint8_t tmp32[32];
            CSHA256().Write(skSpend.begin(), 32).Finalize(tmp32);
            memcpy(&nPrefix, tmp32, 4);
        };

        uint32_t nMask = SetStealthMask(num_prefix_bits);
        nPrefix = nPrefix & nMask;
        sxAddr.prefix.bitfield = nPrefix;
    };

    if (0 != SecretToPublicKey(sxAddr.scan_secret, sxAddr.scan_pubkey))
        throw JSONRPCError(RPC_INTERNAL_ERROR, _("Could not get scan public key."));
    if (0 != SecretToPublicKey(skSpend, sxAddr.spend_pubkey))
        throw JSONRPCError(RPC_INTERNAL_ERROR, _("Could not get spend public key."));

    UniValue result(UniValue::VOBJ);
    bool fFound = false;
    // Find if address already exists, can update
    std::set<CStealthAddress>::iterator it;
    for (it = pwallet->stealthAddresses.begin(); it != pwallet->stealthAddresses.end(); ++it)
    {
        CStealthAddress &sxAddrIt = const_cast<CStealthAddress&>(*it);
        if (sxAddrIt.scan_pubkey == sxAddr.scan_pubkey
            && sxAddrIt.spend_pubkey == sxAddr.spend_pubkey)
        {
            CKeyID sid = sxAddrIt.GetSpendKeyID();

            if (!pwallet->HaveKey(sid))
            {
                CPubKey pk = skSpend.GetPubKey();
                if (!pwallet->AddKeyPubKey(skSpend, pk))
                    throw JSONRPCError(RPC_WALLET_ERROR, _("Import failed - AddKeyPubKey failed."));
                fFound = true; // update stealth address with secret
                break;
            };

            throw JSONRPCError(RPC_WALLET_ERROR, _("Import failed - stealth address exists."));
        };
    };

    {
        LOCK(pwallet->cs_wallet);
        if (pwallet->HaveStealthAddress(sxAddr)) // check for extkeys, no update possible
            throw JSONRPCError(RPC_WALLET_ERROR, _("Import failed - stealth address exists."));

        pwallet->SetAddressBook(sxAddr, sLabel, "");
    }

    if (fFound)
    {
        result.pushKV("result", "Success, updated " + sxAddr.Encoded());
    } else
    {
        if (!pwallet->ImportStealthAddress(sxAddr, skSpend))
            throw std::runtime_error("Could not save to wallet.");
        result.pushKV("result", "Success");
        result.pushKV("stealth_address", sxAddr.Encoded());
    };

    return result;
}

int ListLooseStealthAddresses(UniValue &arr, CHDWallet *pwallet, bool fShowSecrets, bool fAddressBookInfo)
{
    std::set<CStealthAddress>::iterator it;
    for (it = pwallet->stealthAddresses.begin(); it != pwallet->stealthAddresses.end(); ++it)
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("Label", it->label);
        obj.pushKV("Address", it->Encoded());

        if (fShowSecrets)
        {
            obj.pushKV("Scan Secret", CBitcoinSecret(it->scan_secret).ToString());

            CKeyID sid = it->GetSpendKeyID();
            CKey skSpend;
            if (pwallet->GetKey(sid, skSpend))
                obj.pushKV("Spend Secret", CBitcoinSecret(skSpend).ToString());
        };

        if (fAddressBookInfo)
        {
            std::map<CTxDestination, CAddressBookData>::const_iterator mi = pwallet->mapAddressBook.find(*it);
            if (mi != pwallet->mapAddressBook.end())
            {
                // TODO: confirm vPath?

                if (mi->second.name != it->label)
                    obj.pushKV("addr_book_label", mi->second.name);
                if (!mi->second.purpose.empty())
                    obj.pushKV("purpose", mi->second.purpose);

                UniValue objDestData(UniValue::VOBJ);
                for (const auto &pair : mi->second.destdata)
                    obj.pushKV(pair.first, pair.second);
                if (objDestData.size() > 0)
                    obj.pushKV("destdata", objDestData);
            };
        };

        arr.push_back(obj);
    };

    return 0;
};

UniValue liststealthaddresses(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "liststealthaddresses [show_secrets=0]\n"
            "List owned stealth addresses.");

    bool fShowSecrets = false;

    if (request.params.size() > 0)
    {
        std::string str = request.params[0].get_str();

        if (part::IsStringBoolNegative(str))
            fShowSecrets = false;
        else
            fShowSecrets = true;
    };

    if (fShowSecrets)
        EnsureWalletIsUnlocked(pwallet);

    UniValue result(UniValue::VARR);

    ExtKeyAccountMap::const_iterator mi;
    for (mi = pwallet->mapExtAccounts.begin(); mi != pwallet->mapExtAccounts.end(); ++mi)
    {
        CExtKeyAccount *ea = mi->second;

        if (ea->mapStealthKeys.size() < 1)
            continue;

        UniValue rAcc(UniValue::VOBJ);
        UniValue arrayKeys(UniValue::VARR);

        rAcc.pushKV("Account", ea->sLabel);

        AccStealthKeyMap::iterator it;
        for (it = ea->mapStealthKeys.begin(); it != ea->mapStealthKeys.end(); ++it)
        {
            const CEKAStealthKey &aks = it->second;

            UniValue objA(UniValue::VOBJ);
            objA.pushKV("Label", aks.sLabel);
            objA.pushKV("Address", aks.ToStealthAddress());

            if (fShowSecrets)
            {
                objA.pushKV("Scan Secret", HexStr(aks.skScan.begin(), aks.skScan.end()));
                std::string sSpend;
                CStoredExtKey *sekAccount = ea->ChainAccount();
                if (sekAccount && !sekAccount->fLocked)
                {
                    CKey skSpend;
                    if (ea->GetKey(aks.akSpend, skSpend))
                        sSpend = HexStr(skSpend.begin(), skSpend.end());
                    else
                        sSpend = "Extract failed.";
                } else
                {
                    sSpend = "Account Locked.";
                };
                objA.pushKV("Spend Secret", sSpend);
            };

            arrayKeys.push_back(objA);
        };

        if (arrayKeys.size() > 0)
        {
            rAcc.pushKV("Stealth Addresses", arrayKeys);
            result.push_back(rAcc);
        };
    };


    if (pwallet->stealthAddresses.size() > 0)
    {
        UniValue rAcc(UniValue::VOBJ);
        UniValue arrayKeys(UniValue::VARR);

        rAcc.pushKV("Account", "Loose Keys");

        ListLooseStealthAddresses(arrayKeys, pwallet, fShowSecrets, false);

        if (arrayKeys.size() > 0)
        {
            rAcc.pushKV("Stealth Addresses", arrayKeys);
            result.push_back(rAcc);
        };
    };

    return result;
}


UniValue scanchain(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "scanchain [fromHeight]\n"
            "Scan blockchain for owned transactions.");

    //EnsureWalletIsUnlocked(pwallet);

    UniValue result(UniValue::VOBJ);
    int32_t nFromHeight = 0;

    if (request.params.size() > 0)
        nFromHeight = request.params[0].get_int();


    pwallet->ScanChainFromHeight(nFromHeight);

    result.pushKV("result", "Scan complete.");

    return result;
}

UniValue reservebalance(const JSONRPCRequest &request)
{
    // Reserve balance from being staked for network protection

    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "reservebalance <reserve> [amount]\n"
            "<reserve> is true or false to turn balance reserve on or off.\n"
            "[amount] is a real and rounded to cent.\n"
            "Set reserve amount not participating in network protection.\n"
            "If no parameters provided current setting is printed.\n"
            "Wallet must be unlocked to modify.\n");

    if (request.params.size() > 0)
    {
        EnsureWalletIsUnlocked(pwallet);

        bool fReserve = request.params[0].get_bool();
        if (fReserve)
        {
            if (request.params.size() == 1)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "must provide amount to reserve balance.");
            int64_t nAmount = AmountFromValue(request.params[1]);
            nAmount = (nAmount / CENT) * CENT;  // round to cent
            if (nAmount < 0)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "amount cannot be negative.");
            pwallet->SetReserveBalance(nAmount);
        } else
        {
            if (request.params.size() > 1)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "cannot specify amount to turn off reserve.");
            pwallet->SetReserveBalance(0);
        };
        WakeThreadStakeMiner(pwallet);
    };

    UniValue result(UniValue::VOBJ);
    result.pushKV("reserve", (pwallet->nReserveBalance > 0));
    result.pushKV("amount", ValueFromAmount(pwallet->nReserveBalance));
    return result;
}

UniValue deriverangekeys(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 1 ||request.params.size() > 7)
        throw std::runtime_error(
            "deriverangekeys <start> [end] [key/id] [hardened] [save] [add_to_addressbook] [256bithash]\n"
            "<start> start from key.\n"
            "[end] stop deriving after key, default set to derive one key.\n"
            "[key/id] account to derive from, default external chain of current account.\n"
            "[hardened] derive hardened keys, default false.\n"
            "[save] save derived keys to the wallet, default false.\n"
            "[add_to_addressbook] add derived keys to address book, only applies when saving keys, default false.\n"
            "[256bithash] Display addresses from sha256 hash of public keys.\n"
            "Derive keys from the specified chain.\n"
            "Wallet must be unlocked if save or hardened options are set.\n");

    // TODO: manage nGenerated, nHGenerated properly

    int nStart = request.params[0].get_int();
    int nEnd = nStart;

    if (request.params.size() > 1)
        nEnd = request.params[1].get_int();

    if (nEnd < nStart)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "end can not be before start.");

    if (nStart < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "start can not be negative.");

    if (nEnd < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "end can not be positive.");

    std::string sInKey;
    if (request.params.size() > 2)
        sInKey = request.params[2].get_str();

    bool fHardened = false;
    if (request.params.size() > 3)
    {
        std::string s = request.params[3].get_str();
        if (!part::GetStringBool(s, fHardened))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown argument for hardened: %s.", s.c_str()));
    };

    bool fSave = false;
    if (request.params.size() > 4)
    {
        std::string s = request.params[4].get_str();
        if (!part::GetStringBool(s, fSave))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown argument for save: %s.", s.c_str()));
    };

    bool fAddToAddressBook = false;
    if (request.params.size() > 5)
    {
        std::string s = request.params[5].get_str();
        if (!part::GetStringBool(s, fAddToAddressBook))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf(_("Unknown argument for add_to_addressbook: %s."), s.c_str()));
    };

    bool f256bit = false;
    if (request.params.size() > 6)
    {
        std::string s = request.params[6].get_str();
        if (!part::GetStringBool(s, f256bit))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf(_("Unknown argument for 256bithash: %s."), s.c_str()));
    };

    if (!fSave && fAddToAddressBook)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("add_to_addressbook can't be set without save"));

    if (fSave || fHardened)
        EnsureWalletIsUnlocked(pwallet);

    UniValue result(UniValue::VARR);

    {
        LOCK2(cs_main, pwallet->cs_wallet);

        CStoredExtKey *sek = nullptr;
        CExtKeyAccount *sea = nullptr;
        uint32_t nChain = 0;
        if (sInKey.length() == 0)
        {
            if (pwallet->idDefaultAccount.IsNull())
                throw JSONRPCError(RPC_WALLET_ERROR, _("No default account set."));

            ExtKeyAccountMap::iterator mi = pwallet->mapExtAccounts.find(pwallet->idDefaultAccount);
            if (mi == pwallet->mapExtAccounts.end())
                throw JSONRPCError(RPC_WALLET_ERROR, _("Unknown account."));

            sea = mi->second;
            nChain = sea->nActiveExternal;
            if (nChain < sea->vExtKeys.size())
                sek = sea->vExtKeys[nChain];
        } else
        {
            CKeyID keyId;
            ExtractExtKeyId(sInKey, keyId, CChainParams::EXT_KEY_HASH);

            ExtKeyAccountMap::iterator mi = pwallet->mapExtAccounts.begin();
            for (; mi != pwallet->mapExtAccounts.end(); ++mi)
            {
                sea = mi->second;
                for (uint32_t i = 0; i < sea->vExtKeyIDs.size(); ++i)
                {
                    if (sea->vExtKeyIDs[i] != keyId)
                        continue;
                    nChain = i;
                    sek = sea->vExtKeys[i];
                };
                if (sek)
                    break;
            };
        };

        CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
        CStoredExtKey sekLoose, sekDB;
        if (!sek)
        {
            CExtKey58 eKey58;
            CBitcoinAddress addr;
            CKeyID idk;

            if (addr.SetString(sInKey)
                && addr.IsValid(CChainParams::EXT_KEY_HASH)
                && addr.GetKeyID(idk, CChainParams::EXT_KEY_HASH))
            {
                // idk is set
            } else
            if (eKey58.Set58(sInKey.c_str()) == 0)
            {
                sek = &sekLoose;
                sek->kp = eKey58.GetKey();
                idk = sek->kp.GetID();
            } else
            {
                throw JSONRPCError(RPC_WALLET_ERROR, _("Invalid key."));
            };

            if (!idk.IsNull())
            {
                if (wdb.ReadExtKey(idk, sekDB))
                {
                    sek = &sekDB;
                    if (fHardened && (sek->nFlags & EAF_IS_CRYPTED))
                        throw std::runtime_error("TODO: decrypt key.");
                };
            };
        };

        if (!sek)
            throw JSONRPCError(RPC_WALLET_ERROR, _("Unknown chain."));

        if (fHardened && !sek->kp.IsValidV())
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("extkey must have private key to derive hardened keys."));

        if (fSave && !sea)
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Must have account to save keys."));


        uint32_t idIndex;
        if (fAddToAddressBook)
        {
            if (0 != pwallet->ExtKeyGetIndex(sea, idIndex))
                throw JSONRPCError(RPC_WALLET_ERROR, _("ExtKeyGetIndex failed."));
        };

        uint32_t nChildIn = (uint32_t)nStart;
        CPubKey newKey;
        for (int i = nStart; i <= nEnd; ++i)
        {
            nChildIn = (uint32_t)i;
            uint32_t nChildOut = 0;
            if (0 != sek->DeriveKey(newKey, nChildIn, nChildOut, fHardened))
                throw JSONRPCError(RPC_WALLET_ERROR, "DeriveKey failed.");

            if (nChildIn != nChildOut)
                LogPrintf("Warning: %s - DeriveKey skipped key %d.\n", __func__, nChildIn);

            if (fHardened)
                SetHardenedBit(nChildOut);


            CKeyID idk = newKey.GetID();
            CKeyID256 idk256;
            if (f256bit)
            {
                idk256 = newKey.GetID256();
                result.push_back(CBitcoinAddress(idk256).ToString());
            } else
            {
                result.push_back(CBitcoinAddress(idk).ToString());
            };

            if (fSave)
            {
                CEKAKey ak(nChain, nChildOut);
                if (1 != sea->HaveKey(idk, false, ak))
                {
                    if (0 != pwallet->ExtKeySaveKey(sea, idk, ak))
                        throw JSONRPCError(RPC_WALLET_ERROR, "ExtKeySaveKey failed.");
                };

                if (fAddToAddressBook)
                {
                    std::vector<uint32_t> vPath;
                    vPath.push_back(idIndex); // first entry is the index to the account / master key

                    if (0 == AppendChainPath(sek, vPath))
                        vPath.push_back(nChildOut);
                    else
                        vPath.clear();

                    std::string strAccount = "";
                    if (f256bit)
                        pwallet->SetAddressBook(&wdb, idk256, strAccount, "receive", vPath, false);
                    else
                        pwallet->SetAddressBook(&wdb, idk, strAccount, "receive", vPath, false);
                };
            };
        };
    }

    return result;
}

UniValue clearwallettransactions(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "clearwallettransactions [remove_all]\n"
            "[remove_all] remove all transactions.\n"
            "Delete transactions from the wallet.\n"
            "By default removes only failed stakes.\n"
            "Wallet must be unlocked.\n"
            "Warning: Backup your wallet first!");

    EnsureWalletIsUnlocked(pwallet);

    bool fRemoveAll = false;
    if (request.params.size() > 0)
    {
        std::string s = request.params[0].get_str();

        if (!part::GetStringBool(s, fRemoveAll))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown argument for remove_all: %s.", s.c_str()));
    };

    int rv;
    size_t nRemoved = 0;
    size_t nRecordsRemoved = 0;

    {
        LOCK2(cs_main, pwallet->cs_wallet);

        CHDWalletDB wdb(pwallet->GetDBHandle());
        if (!wdb.TxnBegin())
            throw std::runtime_error("TxnBegin failed.");

        Dbc *pcursor = wdb.GetTxnCursor();
        if (!pcursor)
            throw std::runtime_error("GetTxnCursor failed.");

        CDataStream ssKey(SER_DISK, CLIENT_VERSION);

        std::map<uint256, CWalletTx>::iterator itw;
        std::string strType;
        uint256 hash;
        uint32_t fFlags = DB_SET_RANGE;
        ssKey << std::string("tx");
        while (wdb.ReadKeyAtCursor(pcursor, ssKey, fFlags) == 0)
        {
            fFlags = DB_NEXT;

            ssKey >> strType;
            if (strType != "tx")
                break;
            ssKey >> hash;

            if (!fRemoveAll)
            {
                if ((itw = pwallet->mapWallet.find(hash)) == pwallet->mapWallet.end())
                {
                    LogPrintf("Warning: %s - tx not found in mapwallet! %s.\n", __func__, hash.ToString());
                    continue; // err on the side of caution
                };

                CWalletTx *pcoin = &itw->second;
                if (!pcoin->IsCoinStake() || !pcoin->isAbandoned())
                    continue;
            };

            //if (0 != pwallet->UnloadTransaction(hash))
            //    throw std::runtime_error("UnloadTransaction failed.");
            pwallet->UnloadTransaction(hash); // ignore failure

            if ((rv = pcursor->del(0)) != 0)
                throw std::runtime_error("pcursor->del failed.");

            nRemoved++;
        };

        if (fRemoveAll)
        {
            fFlags = DB_SET_RANGE;
            ssKey.clear();
            ssKey << std::string("rtx");
            while (wdb.ReadKeyAtCursor(pcursor, ssKey, fFlags) == 0)
            {
                fFlags = DB_NEXT;

                ssKey >> strType;
                if (strType != "rtx")
                    break;
                ssKey >> hash;

                pwallet->UnloadTransaction(hash); // ignore failure

                if ((rv = pcursor->del(0)) != 0)
                    throw std::runtime_error("pcursor->del failed.");

                // TODO: Remove CStoredTransaction

                nRecordsRemoved++;
            };
        };

        pcursor->close();
        if (!wdb.TxnCommit())
        {
            throw std::runtime_error("TxnCommit failed.");
        };
    }

    UniValue result(UniValue::VOBJ);

    result.pushKV("transactions_removed", (int)nRemoved);
    result.pushKV("records_removed", (int)nRecordsRemoved);

    return result;
}

static bool ParseOutput(
    UniValue &                 output,
    const COutputEntry &       o,
    CHDWallet * const          pwallet,
    CWalletTx &                wtx,
    const isminefilter &       watchonly,
    std::vector<std::string> & addresses,
    std::vector<std::string> & amounts
) {
    CBitcoinAddress addr;

    std::string sKey = strprintf("n%d", o.vout);
    mapValue_t::iterator mvi = wtx.mapValue.find(sKey);
    if (mvi != wtx.mapValue.end()) {
        output.push_back(Pair("narration", mvi->second));
    }
    if (addr.Set(o.destination)) {
        output.push_back(Pair("address", addr.ToString()));
        addresses.push_back(addr.ToString());
    }
    if (o.ismine & ISMINE_WATCH_ONLY) {
        if (watchonly & ISMINE_WATCH_ONLY) {
            output.push_back(Pair("involvesWatchonly", true));
        } else {
            return false;
        }
    }
    if (o.destStake.type() != typeid(CNoDestination)) {
        output.push_back(Pair("coldstake_address", CBitcoinAddress(o.destStake).ToString()));
    }
    if (pwallet->mapAddressBook.count(o.destination)) {
        output.push_back(Pair("label", pwallet->mapAddressBook[o.destination].name));
    }
    output.push_back(Pair("vout", o.vout));
    amounts.push_back(std::to_string(o.amount));
    return true;
}

static void ParseOutputs(
    UniValue &           entries,
    CWalletTx &          wtx,
    CHDWallet * const    pwallet,
    const isminefilter & watchonly,
    std::string          search,
    bool                 fWithReward,
    std::vector<CScript> &vDevFundScripts
) {
    UniValue entry(UniValue::VOBJ);

    // GetAmounts variables
    std::list<COutputEntry> listReceived;
    std::list<COutputEntry> listSent;
    std::list<COutputEntry> listStaked;
    CAmount nFee;
    CAmount amount = 0;
    std::string strSentAccount;

    wtx.GetAmounts(
        listReceived,
        listSent,
        listStaked,
        nFee,
        strSentAccount,
        ISMINE_ALL);

    if (wtx.IsFromMe(ISMINE_WATCH_ONLY) && !(watchonly & ISMINE_WATCH_ONLY)) {
        return ;
    }

    std::vector<std::string> addresses;
    std::vector<std::string> amounts;

    UniValue outputs(UniValue::VARR);
    // common to every type of transaction
    if (strSentAccount != "") {
        entry.push_back(Pair("account", strSentAccount));
    }
    WalletTxToJSON(wtx, entry, true);

    if (!listStaked.empty() || !listSent.empty())
        entry.push_back(Pair("abandoned", wtx.isAbandoned()));

    // staked
    if (!listStaked.empty()) {
        if (wtx.GetDepthInMainChain() < 1) {
            entry.push_back(Pair("category", "orphaned_stake"));
        } else {
            entry.push_back(Pair("category", "stake"));
        }
        for (const auto &s : listStaked) {
            UniValue output(UniValue::VOBJ);
            if (!ParseOutput(
                output,
                s,
                pwallet,
                wtx,
                watchonly,
                addresses,
                amounts
            )) {
                return ;
            }
            output.push_back(Pair("amount", ValueFromAmount(s.amount)));
            outputs.push_back(output);
        }

        amount += -nFee;
    } else {
        // sent
        if (!listSent.empty()) {
            entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
            for (const auto &s : listSent) {
                UniValue output(UniValue::VOBJ);
                if (!ParseOutput(output,
                    s,
                    pwallet,
                    wtx,
                    watchonly,
                    addresses,
                    amounts
                )) {
                    return ;
                }
                output.push_back(Pair("amount", ValueFromAmount(-s.amount)));
                amount -= s.amount;
                outputs.push_back(output);
            }
        }

        // received
        if (!listReceived.empty()) {
            for (const auto &r : listReceived) {
                UniValue output(UniValue::VOBJ);
                if (!ParseOutput(
                    output,
                    r,
                    pwallet,
                    wtx,
                    watchonly,
                    addresses,
                    amounts
                )) {
                    return ;
                }
                if (r.destination.type() == typeid(CKeyID)) {
                    CStealthAddress sx;
                    CKeyID idK = boost::get<CKeyID>(r.destination);
                    if (pwallet->GetStealthLinked(idK, sx)) {
                        output.push_back(Pair("stealth_address", sx.Encoded()));
                    }
                }
                output.push_back(Pair("amount", ValueFromAmount(r.amount)));
                amount += r.amount;

                bool fExists = false;
                for (size_t i = 0; i < outputs.size(); ++i) {
                    auto &o = outputs.get(i);
                    if (o["vout"].get_int() == r.vout) {
                        o.get("amount").setStr(part::AmountToString(r.amount));
                        fExists = true;
                    }
                }
                if (!fExists)
                    outputs.push_back(output);
            }
        }

        if (wtx.IsCoinBase()) {
            if (wtx.GetDepthInMainChain() < 1) {
                entry.push_back(Pair("category", "orphan"));
            } else if (wtx.GetBlocksToMaturity() > 0) {
                entry.push_back(Pair("category", "immature"));
            } else {
                entry.push_back(Pair("category", "coinbase"));
            }
        } else if (!nFee) {
            entry.push_back(Pair("category", "receive"));
        } else if (amount == 0) {
            entry.push_back(Pair("category", "internal_transfer"));
        } else {
            entry.push_back(Pair("category", "send"));
        }
    };

    entry.push_back(Pair("outputs", outputs));
    entry.push_back(Pair("amount", ValueFromAmount(amount)));

    if (fWithReward && !listStaked.empty())
    {
        CAmount nOutput = wtx.tx->GetValueOut();
        CAmount nInput = 0;

        // Remove dev fund outputs
        if (wtx.tx->vpout.size() > 2 && wtx.tx->vpout[1]->IsStandardOutput())
        {
            for (const auto &s : vDevFundScripts)
            {
                if (s == *wtx.tx->vpout[1]->GetPScriptPubKey())
                {
                    nOutput -= wtx.tx->vpout[1]->GetValue();
                    break;
                }
            }
        };

        for (const auto &vin : wtx.tx->vin)
        {
            if (vin.IsAnonInput())
                continue;
            nInput += pwallet->GetOutputValue(vin.prevout, true);
        };
        entry.push_back(Pair("reward", ValueFromAmount(nOutput - nInput)));
    };

    if (search != "") {
        // search in addresses
        if (std::any_of(addresses.begin(), addresses.end(), [search](std::string addr) {
            return addr.find(search) != std::string::npos;
        })) {
            entries.push_back(entry);
            return ;
        }
        // search in amounts
        // character DOT '.' is not searched for: search "123" will find 1.23 and 12.3
        if (std::any_of(amounts.begin(), amounts.end(), [search](std::string amount) {
            return amount.find(search) != std::string::npos;
        })) {
            entries.push_back(entry);
            return ;
        }
    } else {
        entries.push_back(entry);
    }
}

static void push(UniValue & entry, std::string key, UniValue const & value)
{
    if (entry[key].getType() == 0) {
        entry.push_back(Pair(key, value));
    }
}

static void ParseRecords(
    UniValue &                 entries,
    const uint256 &            hash,
    const CTransactionRecord & rtx,
    CHDWallet * const          pwallet,
    const isminefilter &       watchonly_filter,
    std::string                search
) {
    std::vector<std::string> addresses;
    std::vector<std::string> amounts;
    UniValue   entry(UniValue::VOBJ);
    UniValue outputs(UniValue::VARR);
    size_t  nOwned      = 0;
    size_t  nFrom       = 0;
    size_t  nWatchOnly  = 0;
    CAmount totalAmount = 0;

    int confirmations = pwallet->GetDepthInMainChain(rtx.blockHash);
    push(entry, "confirmations", confirmations);
    if (confirmations > 0) {
        push(entry, "blockhash", rtx.blockHash.GetHex());
        push(entry, "blockindex", rtx.nIndex);
        push(entry, "blocktime", mapBlockIndex[rtx.blockHash]->GetBlockTime());
    } else {
        push(entry, "trusted", pwallet->IsTrusted(hash, rtx.blockHash));
    };

    push(entry, "txid", hash.ToString());
    UniValue conflicts(UniValue::VARR);
    std::set<uint256> setconflicts = pwallet->GetConflicts(hash);
    setconflicts.erase(hash);
    for (const auto &conflict : setconflicts) {
        conflicts.push_back(conflict.GetHex());
    }
    if (conflicts.size() > 0) {
        push(entry, "walletconflicts", conflicts);
    }
    PushTime(entry, "time", rtx.nTimeReceived);

    size_t nLockedOutputs = 0;
    for (auto &record : rtx.vout) {

        UniValue output(UniValue::VOBJ);

        if (record.nFlags & ORF_CHANGE) {
            continue ;
        }
        if (record.nFlags & ORF_OWN_ANY) {
            nOwned++;
        }
        if (record.nFlags & ORF_FROM) {
            nFrom++;
        }
        if (record.nFlags & ORF_OWN_WATCH) {
            nWatchOnly++;
        }
        if (record.nFlags & ORF_LOCKED) {
            nLockedOutputs++;
        }

        CBitcoinAddress addr;
        CTxDestination  dest;
        bool extracted = ExtractDestination(record.scriptPubKey, dest);

        // get account name
        if (extracted && !record.scriptPubKey.IsUnspendable()) {
            addr.Set(dest);
            std::map<CTxDestination, CAddressBookData>::iterator mai;
            mai = pwallet->mapAddressBook.find(dest);
            if (mai != pwallet->mapAddressBook.end() && !mai->second.name.empty()) {
                push(output, "account", mai->second.name);
            }
        };

        // stealth addresses
        CStealthAddress sx;
        if (record.vPath.size() > 0) {
            if (record.vPath[0] == ORA_STEALTH) {
                if (record.vPath.size() < 5) {
                    LogPrintf("%s: Warning, malformed vPath.", __func__);
                } else {
                    uint32_t sidx;
                    memcpy(&sidx, &record.vPath[1], 4);
                    if (pwallet->GetStealthByIndex(sidx, sx)) {
                        push(output, "stealth_address", sx.Encoded());
                        addresses.push_back(sx.Encoded());
                    }
                }
            }
        } else {
            if (extracted && dest.type() == typeid(CKeyID)) {
                CKeyID idK = boost::get<CKeyID>(dest);
                if (pwallet->GetStealthLinked(idK, sx)) {
                    push(output, "stealth_address", sx.Encoded());
                    addresses.push_back(sx.Encoded());
                }
            }
        }

        if (extracted && dest.type() == typeid(CNoDestination)) {
            push(output, "address", "none");
        } else if (extracted) {
            push(output, "address", addr.ToString());
            addresses.push_back(addr.ToString());
        }

        push(output, "type",
              record.nType == OUTPUT_STANDARD ? "standard"
            : record.nType == OUTPUT_CT       ? "blind"
            : record.nType == OUTPUT_RINGCT   ? "anon"
            : "unknown");

        if (!record.sNarration.empty()) {
            push(output, "narration", record.sNarration);
        }

        CAmount amount = record.nValue;
        if (!(record.nFlags & ORF_OWN_ANY)) {
            amount *= -1;
        }
        totalAmount += amount;
        amounts.push_back(std::to_string(ValueFromAmount(amount).get_real()));
        push(output, "amount", ValueFromAmount(amount));
        push(output, "vout", record.n);
        outputs.push_back(output);
    }

    if (nFrom > 0) {
        push(entry, "abandoned", rtx.IsAbandoned());
        push(entry, "fee", ValueFromAmount(-rtx.nFee));
    }

    if (nOwned && nFrom) {
        push(entry, "category", "internal_transfer");
    } else if (nOwned) {
        push(entry, "category", "receive");
    } else if (nFrom) {
        push(entry, "category", "send");
    } else {
        push(entry, "category", "unknown");
    }

    if (nLockedOutputs) {
        push(entry, "requires_unlock", "true");
    }
    if (nWatchOnly) {
        push(entry, "involvesWatchonly", "true");
    }

    push(entry, "outputs", outputs);

    push(entry, "amount", ValueFromAmount(totalAmount));
    amounts.push_back(std::to_string(ValueFromAmount(totalAmount).get_real()));

    if (search != "") {
        // search in addresses
        if (std::any_of(addresses.begin(), addresses.end(), [search](std::string addr) {
            return addr.find(search) != std::string::npos;
        })) {
            entries.push_back(entry);
            return ;
        }
        // search in amounts
        // character DOT '.' is not searched for: search "123" will find 1.23 and 12.3
        if (std::any_of(amounts.begin(), amounts.end(), [search](std::string amount) {
            return amount.find(search) != std::string::npos;
        })) {
            entries.push_back(entry);
            return ;
        }
    } else {
        entries.push_back(entry);
    }
}

static std::string getAddress(UniValue const & transaction)
{
    if (transaction["stealth_address"].getType() != 0) {
        return transaction["stealth_address"].get_str();
    }
    if (transaction["address"].getType() != 0) {
        return transaction["address"].get_str();
    }
    if (transaction["outputs"][0]["stealth_address"].getType() != 0) {
        return transaction["outputs"][0]["stealth_address"].get_str();
    }
    if (transaction["outputs"][0]["address"].getType() != 0) {
        return transaction["outputs"][0]["address"].get_str();
    }
    return std::string();
}

UniValue filtertransactions(const JSONRPCRequest &request)
{
    CHDWallet * const pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "filtertransactions [options]\n"
            "List transactions.\n"
            "1. options (json, optional) : A configuration object for the query\n"
            "\n"
            "        All keys are optional. Default values are:\n"
            "        {\n"
            "                \"count\":             10,\n"
            "                \"skip\":              0,\n"
            "                \"include_watchonly\": false,\n"
            "                \"search\":            ''\n"
            "                \"category\":          'all',\n"
            "                \"type\":              'all',\n"
            "                \"sort\":              'time'\n"
            "                \"from\":              '0'\n"
            "                \"to\":                '9999'\n"
            "                \"collate\":           false\n"
            "                \"with_reward\":       false\n"
            "        }\n"
            "\n"
            "        Expected values are as follows:\n"
            "                count:             number of transactions to be displayed\n"
            "                                   (integer > 0)\n"
            "                skip:              number of transactions to skip\n"
            "                                   (integer >= 0)\n"
            "                include_watchonly: whether to include watchOnly transactions\n"
            "                                   (bool string)\n"
            "                search:            a query to search addresses and amounts\n"
            "                                   character DOT '.' is not searched for:\n"
            "                                   search \"123\" will find 1.23 and 12.3\n"
            "                                   (query string)\n"
            "                category:          select only one category of transactions to return\n"
            "                                   (string from list)\n"
            "                                   all, send, orphan, immature, coinbase, receive,\n"
            "                                   orphaned_stake, stake, internal_transfer\n"
            "                type:              select only one type of transactions to return\n"
            "                                   (string from list)\n"
            "                                   all, standard, anon, blind\n"
            "                sort:              sort transactions by criteria\n"
            "                                   (string from list)\n"
            "                                   time          most recent first\n"
            "                                   address       alphabetical\n"
            "                                   category      alphabetical\n"
            "                                   amount        biggest first\n"
            "                                   confirmations most confirmations first\n"
            "                                   txid          alphabetical\n"
            "                from:              unix timestamp or string \"yyyy-mm-ddThh:mm:ss\"\n"
            "                to:                unix timestamp or string \"yyyy-mm-ddThh:mm:ss\"\n"
            "                collate:           display number of records and sum of amount fields\n"
            "                with_reward        calculate reward explicitly from txindex if necessary\n"
            "\n"
            "        Examples:\n"
            "            List only when category is 'stake'\n"
            "                " + HelpExampleCli("filtertransactions", "\"{\\\"category\\\":\\\"stake\\\"}\"") + "\n"
            "            Multiple arguments\n"
            "                " + HelpExampleCli("filtertransactions", "\"{\\\"sort\\\":\\\"amount\\\", \\\"category\\\":\\\"receive\\\"}\"") + "\n"
            "            As a JSON-RPC call\n"
            "                " + HelpExampleRpc("filtertransactions", "{\\\"category\\\":\\\"stake\\\"}") + "\n"
        );

    LOCK2(cs_main, pwallet->cs_wallet);

    unsigned int count     = 10;
    int          skip      = 0;
    isminefilter watchonly = ISMINE_SPENDABLE;
    std::string  search    = "";
    std::string  category  = "all";
    std::string  type      = "all";
    std::string  sort      = "time";

    int64_t timeFrom = 0;
    int64_t timeTo = 0x3AFE130E00; // 9999
    bool fCollate = false;
    bool fWithReward = false;

    if (!request.params[0].isNull()) {
        const UniValue & options = request.params[0].get_obj();
        RPCTypeCheckObj(options,
            {
                {"count",               UniValueType(UniValue::VNUM)},
                {"skip",                UniValueType(UniValue::VNUM)},
                {"include_watchonly",   UniValueType(UniValue::VBOOL)},
                {"search",              UniValueType(UniValue::VSTR)},
                {"category",            UniValueType(UniValue::VSTR)},
                {"type",                UniValueType(UniValue::VSTR)},
                {"sort",                UniValueType(UniValue::VSTR)}
            },
            true,             // allow null
            false             // strict
        );
        if (options.exists("count")) {
            int _count = options["count"].get_int();
            if (_count < 1) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                    strprintf("Invalid count: %i.", _count));
            }
            count = _count;
        }
        if (options.exists("skip")) {
            skip = options["skip"].get_int();
            if (skip < 0) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                    strprintf("Invalid skip number: %i.", skip));
            }
        }
        if (options.exists("include_watchonly")) {
            if (options["include_watchonly"].get_bool()) {
                watchonly = watchonly | ISMINE_WATCH_ONLY;
            }
        }
        if (options.exists("search")) {
            search = options["search"].get_str();
        }
        if (options.exists("category")) {
            category = options["category"].get_str();
            std::vector<std::string> categories = {
                "all",
                "send",
                "orphan",
                "immature",
                "coinbase",
                "receive",
                "orphaned_stake",
                "stake",
                "internal_transfer"
            };
            auto it = std::find(categories.begin(), categories.end(), category);
            if (it == categories.end()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                    strprintf("Invalid category: %s.", category));
            }
        }
        if (options.exists("type")) {
            type = options["type"].get_str();
            std::vector<std::string> types = {
                "all",
                "standard",
                "anon",
                "blind"
            };
            auto it = std::find(types.begin(), types.end(), type);
            if (it == types.end()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                    strprintf("Invalid type: %s.", type));
            }
        }
        if (options.exists("sort")) {
            sort = options["sort"].get_str();
            std::vector<std::string> sorts = {
                "time",
                "address",
                "category",
                "amount",
                "confirmations",
                "txid"
            };
            auto it = std::find(sorts.begin(), sorts.end(), sort);
            if (it == sorts.end()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                    strprintf("Invalid sort: %s.", sort));
            }
        }

        if (options["from"].isStr())
            timeFrom = part::strToEpoch(options["from"].get_str().c_str());
        else
        if (options["from"].isNum())
            timeFrom = options["from"].get_int64();
        if (options["to"].isStr())
            timeTo = part::strToEpoch(options["to"].get_str().c_str(), true);
        else
        if (options["to"].isNum())
            timeTo = options["to"].get_int64();
        if (options["collate"].isBool())
            fCollate = options["collate"].get_bool();
        if (options["with_reward"].isBool())
            fWithReward = options["with_reward"].get_bool();
    }


    std::vector<CScript> vDevFundScripts;
    if (fWithReward)
    {
        const auto v = Params().GetDevFundSettings();
        for (const auto &s : v)
        {
            CTxDestination dfDest = CBitcoinAddress(s.second.sDevFundAddresses).Get();
            if (dfDest.type() == typeid(CNoDestination))
                continue;
            CScript script = GetScriptForDestination(dfDest);
            vDevFundScripts.push_back(script);
        };
    };


    // for transactions and records
    UniValue transactions(UniValue::VARR);

    // transaction processing
    const CHDWallet::TxItems &txOrdered = pwallet->wtxOrdered;
    CWallet::TxItems::const_reverse_iterator tit = txOrdered.rbegin();
    while (tit != txOrdered.rend()) {
        CWalletTx* const pwtx = tit->second.first;
        int64_t txTime = pwtx->GetTxTime();
        if (txTime < timeFrom) break;
        if (txTime <= timeTo)
        ParseOutputs(
            transactions,
            *pwtx,
            pwallet,
            watchonly,
            search,
            fWithReward,
            vDevFundScripts
        );
        tit++;
    }

    // records processing
    const RtxOrdered_t &rtxOrdered = pwallet->rtxOrdered;
    RtxOrdered_t::const_reverse_iterator rit = rtxOrdered.rbegin();
    while (rit != rtxOrdered.rend()) {
        const uint256 &hash = rit->second->first;
        const CTransactionRecord &rtx = rit->second->second;
        int64_t txTime = rtx.GetTxTime();
        if (txTime < timeFrom) break;
        if (txTime <= timeTo)
        ParseRecords(
            transactions,
            hash,
            rtx,
            pwallet,
            watchonly,
            search
        );
        rit++;
    }

    // sort
    std::vector<UniValue> values = transactions.getValues();
    std::sort(values.begin(), values.end(), [sort] (UniValue a, UniValue b) -> bool {
        double a_amount = a["category"].get_str() == "send"
            ? -(a["amount"].get_real())
            :   a["amount"].get_real();
        double b_amount = b["category"].get_str() == "send"
            ? -(b["amount"].get_real())
            :   b["amount"].get_real();
        std::string a_address = getAddress(a);
        std::string b_address = getAddress(b);
        return (
              sort == "address"
                ? a_address < b_address
            : sort == "category" || sort == "txid"
                ? a[sort].get_str() < b[sort].get_str()
            : sort == "time" || sort == "confirmations"
                ? a[sort].get_real() > b[sort].get_real()
            : sort == "amount"
                ? a_amount > b_amount
            : false
        );
    });

    CAmount nTotalAmount = 0, nTotalReward = 0;
    // filter, skip and count
    UniValue result(UniValue::VARR);
    // for every value while count is positive
    for (unsigned int i = 0; i < values.size() && count > 0; i++) {
        // if value's category is relevant
        if (values[i]["category"].get_str() == category || category == "all") {
            // if value's type is not relevant
            if (values[i]["type"].getType() == 0) {
                // value's type is undefined
                if (!(type == "all" || type == "standard")) {
                    // type is not 'all' or 'standard'
                    continue ;
                }
            } else if (!(values[i]["type"].get_str() == type || type == "all")) {
                // value's type is defined
                // value's type is not type or 'all'
                continue ;
            }
            // if we've skipped enough valid values
            if (skip-- <= 0) {
                result.push_back(values[i]);
                count--;

                if (fCollate) {
                    if (!values[i]["amount"].isNull())
                        nTotalAmount += AmountFromValue(values[i]["amount"]);
                    if (!values[i]["reward"].isNull())
                        nTotalReward += AmountFromValue(values[i]["reward"]);
                };
            }
        }
    }

    if (fCollate) {
        UniValue retObj(UniValue::VOBJ);
        UniValue stats(UniValue::VOBJ);
        stats.pushKV("records", (int)result.size());
        stats.pushKV("total_amount", ValueFromAmount(nTotalAmount));
        if (fWithReward)
            stats.pushKV("total_reward", ValueFromAmount(nTotalReward));
        retObj.pushKV("tx", result);
        retObj.pushKV("collated", stats);
        return retObj;
    };

    return result;
}

enum SortCodes
{
    SRT_LABEL_ASC,
    SRT_LABEL_DESC,
};

class AddressComp {
public:
    int nSortCode;
    AddressComp(int nSortCode_) : nSortCode(nSortCode_) {}
    bool operator() (
        const std::map<CTxDestination, CAddressBookData>::iterator a,
        const std::map<CTxDestination, CAddressBookData>::iterator b) const
    {
        switch (nSortCode)
        {
            case SRT_LABEL_DESC:
                return b->second.name.compare(a->second.name) < 0;
            default:
                break;
        };
        //default: case SRT_LABEL_ASC:
        return a->second.name.compare(b->second.name) < 0;
    }
};

UniValue filteraddresses(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 6)
        throw std::runtime_error(
            "filteraddresses [offset] [count] [sort_code] [match_str] [match_owned] [show_path]\n"
            "filteraddresses [offset] [count] will list 'count' addresses starting from 'offset'\n"
            "filteraddresses -1 will count addresses\n"
            "[sort_code] 0 sort by label ascending, 1 sort by label descending, default 0\n"
            "[match_str] filter by label\n"
            "[match_owned] 0 off, 1 owned, 2 non-owned, default 0\n"
            "List addresses.");

    int nOffset = 0, nCount = 0x7FFFFFFF;
    if (request.params.size() > 0)
        nOffset = request.params[0].get_int();

    std::map<CTxDestination, CAddressBookData>::iterator it;
    if (request.params.size() == 1 && nOffset == -1)
    {
        LOCK(pwallet->cs_wallet);
        // Count addresses
        UniValue result(UniValue::VOBJ);

        result.pushKV("total", (int)pwallet->mapAddressBook.size());

        int nReceive = 0, nSend = 0;
        for (it = pwallet->mapAddressBook.begin(); it != pwallet->mapAddressBook.end(); ++it)
        {
            if (it->second.nOwned == 0)
            {
                CBitcoinAddress address(it->first);
                it->second.nOwned = pwallet->HaveAddress(address) ? 1 : 2;
            };

            if (it->second.nOwned == 1)
                nReceive++;
            else
            if (it->second.nOwned == 2)
                nSend++;
        };

        result.pushKV("num_receive", nReceive);
        result.pushKV("num_send", nSend);

        return result;
    };

    if (request.params.size() > 1)
        nCount = request.params[1].get_int();

    if (nOffset < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "offset must be 0 or greater.");
    if (nCount < 1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "count must be 1 or greater.");


    // TODO: Make better
    int nSortCode = SRT_LABEL_ASC;
    if (request.params.size() > 2)
    {
        std::string sCode = request.params[2].get_str();
        if (sCode == "0")
            nSortCode = SRT_LABEL_ASC;
        else
        if (sCode == "1")
            nSortCode = SRT_LABEL_DESC;
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown sort_code.");
    };

    int nMatchOwned = 0; // 0 off/all, 1 owned, 2 non-owned
    int nMatchMode = 0; // 1 contains
    int nShowPath = 1;

    std::string sMatch;
    if (request.params.size() > 3)
        sMatch = request.params[3].get_str();

    if (sMatch != "")
        nMatchMode = 1;

    if (request.params.size() > 4)
    {
        std::string s = request.params[4].get_str();
        if (s != "")
            nMatchOwned = std::stoi(s);
    };

    if (request.params.size() > 5)
    {
        std::string s = request.params[5].get_str();
        bool fTemp;
        if (!part::GetStringBool(s, fTemp))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown argument for show_path: %s.", s.c_str()));
        nShowPath = !fTemp ? 0 : nShowPath;
    };

    UniValue result(UniValue::VARR);
    {
        LOCK(pwallet->cs_wallet);

        CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");

        if (nOffset >= (int)pwallet->mapAddressBook.size())
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("offset is beyond last address (%d).", nOffset));
        std::vector<std::map<CTxDestination, CAddressBookData>::iterator> vitMapAddressBook;
        vitMapAddressBook.reserve(pwallet->mapAddressBook.size());

        for (it = pwallet->mapAddressBook.begin(); it != pwallet->mapAddressBook.end(); ++it)
        {
            if (it->second.nOwned == 0)
            {
                CBitcoinAddress address(it->first);
                it->second.nOwned = pwallet->HaveAddress(address) ? 1 : 2;
            };

            if (nMatchOwned && it->second.nOwned != nMatchOwned)
                continue;

            if (nMatchMode)
            {
                if (!part::stringsMatchI(it->second.name, sMatch, nMatchMode-1))
                    continue;
            };

            vitMapAddressBook.push_back(it);
        };

        std::sort(vitMapAddressBook.begin(), vitMapAddressBook.end(), AddressComp(nSortCode));

        std::map<uint32_t, std::string> mapKeyIndexCache;
        std::vector<std::map<CTxDestination, CAddressBookData>::iterator>::iterator vit;
        int nEntries = 0;
        for (vit = vitMapAddressBook.begin()+nOffset;
            vit != vitMapAddressBook.end() && nEntries < nCount; ++vit)
        {
            auto &item = *vit;
            UniValue entry(UniValue::VOBJ);

            CBitcoinAddress address(item->first, item->second.fBech32);
            entry.pushKV("address", address.ToString());
            entry.pushKV("label", item->second.name);
            entry.pushKV("owned", item->second.nOwned == 1 ? "true" : "false");

            if (nShowPath > 0)
            {
                if (item->second.vPath.size() > 0)
                {
                    uint32_t index = item->second.vPath[0];
                    std::map<uint32_t, std::string>::iterator mi = mapKeyIndexCache.find(index);

                    if (mi != mapKeyIndexCache.end())
                    {
                        entry.pushKV("root", mi->second);
                    } else
                    {
                        CKeyID accId;
                        if (!wdb.ReadExtKeyIndex(index, accId))
                        {
                            entry.pushKV("root", "error");
                        } else
                        {
                            CBitcoinAddress addr;
                            addr.Set(accId, CChainParams::EXT_ACC_HASH);
                            std::string sTmp = addr.ToString();
                            entry.pushKV("root", sTmp);
                            mapKeyIndexCache[index] = sTmp;
                        };
                    };
                };

                if (item->second.vPath.size() > 1)
                {
                    std::string sPath;
                    if (0 == PathToString(item->second.vPath, sPath, '\'', 1))
                        entry.pushKV("path", sPath);
                };
            };

            result.push_back(entry);
            nEntries++;
        };
    } // cs_wallet

    return result;
}

UniValue manageaddressbook(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 4)
        throw std::runtime_error(
            "manageaddressbook <action> <address> [label] [purpose]\n"
            "Manage the address book."
            "\nArguments:\n"
            "1. \"action\"      (string, required) 'add/edit/del/info/newsend' The action to take.\n"
            "2. \"address\"     (string, required) The address to affect.\n"
            "3. \"label\"       (string, optional) Optional label.\n"
            "4. \"purpose\"     (string, optional) Optional purpose label.\n");

    std::string sAction = request.params[0].get_str();
    std::string sAddress = request.params[1].get_str();
    std::string sLabel, sPurpose;

    if (sAction != "info")
        EnsureWalletIsUnlocked(pwallet);

    bool fHavePurpose = false;
    if (request.params.size() > 2)
        sLabel = request.params[2].get_str();
    if (request.params.size() > 3)
    {
        sPurpose = request.params[3].get_str();
        fHavePurpose = true;
    };

    CBitcoinAddress address(sAddress);

    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Invalid Particl address."));

    CTxDestination dest = address.Get();

    std::map<CTxDestination, CAddressBookData>::iterator mabi;
    mabi = pwallet->mapAddressBook.find(dest);

    std::vector<uint32_t> vPath;

    UniValue objDestData(UniValue::VOBJ);

    if (sAction == "add")
    {
        if (mabi != pwallet->mapAddressBook.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf(_("Address '%s' is recorded in the address book."), sAddress));

        if (!pwallet->SetAddressBook(nullptr, dest, sLabel, sPurpose, vPath, true))
            throw JSONRPCError(RPC_WALLET_ERROR, "SetAddressBook failed.");
    } else
    if (sAction == "edit")
    {
        if (request.params.size() < 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Need a parameter to change."));
        if (mabi == pwallet->mapAddressBook.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf(_("Address '%s' is not in the address book."), sAddress));

        if (!pwallet->SetAddressBook(nullptr, dest, sLabel,
            fHavePurpose ? sPurpose : mabi->second.purpose, mabi->second.vPath, true))
            throw JSONRPCError(RPC_WALLET_ERROR, "SetAddressBook failed.");

        sLabel = mabi->second.name;
        sPurpose = mabi->second.purpose;

        for (const auto &pair : mabi->second.destdata)
            objDestData.pushKV(pair.first, pair.second);

    } else
    if (sAction == "del")
    {
        if (mabi == pwallet->mapAddressBook.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf(_("Address '%s' is not in the address book."), sAddress));
        sLabel = mabi->second.name;
        sPurpose = mabi->second.purpose;

        if (!pwallet->DelAddressBook(dest))
            throw JSONRPCError(RPC_WALLET_ERROR, "DelAddressBook failed.");
    } else
    if (sAction == "info")
    {
        if (mabi == pwallet->mapAddressBook.end())
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf(_("Address '%s' is not in the address book."), sAddress));

        UniValue result(UniValue::VOBJ);

        result.pushKV("action", sAction);
        result.pushKV("address", sAddress);

        result.pushKV("label", mabi->second.name);
        result.pushKV("purpose", mabi->second.purpose);

        if (mabi->second.nOwned == 0)
            mabi->second.nOwned = pwallet->HaveAddress(address) ? 1 : 2;

        result.pushKV("owned", mabi->second.nOwned == 1 ? "true" : "false");

        if (mabi->second.vPath.size() > 1)
        {
            std::string sPath;
            if (0 == PathToString(mabi->second.vPath, sPath, '\'', 1))
                result.pushKV("path", sPath);
        };

        for (const auto &pair : mabi->second.destdata)
            objDestData.pushKV(pair.first, pair.second);
        if (objDestData.size() > 0)
            result.pushKV("destdata", objDestData);

        result.pushKV("result", "success");

        return result;
    } else
    if (sAction == "newsend")
    {
        // Only update the purpose field if address does not yet exist
        if (mabi != pwallet->mapAddressBook.end())
            sPurpose = "";// "" means don't change purpose

        if (!pwallet->SetAddressBook(dest, sLabel, sPurpose))
            throw JSONRPCError(RPC_WALLET_ERROR, "SetAddressBook failed.");

        if (mabi != pwallet->mapAddressBook.end())
            sPurpose = mabi->second.purpose;
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown action, must be one of 'add/edit/del'."));
    };

    UniValue result(UniValue::VOBJ);

    result.pushKV("action", sAction);
    result.pushKV("address", sAddress);

    if (sLabel.size() > 0)
        result.pushKV("label", sLabel);
    if (sPurpose.size() > 0)
        result.pushKV("purpose", sPurpose);
    if (objDestData.size() > 0)
        result.pushKV("destdata", objDestData);

    result.pushKV("result", "success");

    return result;
}

extern double GetDifficulty(const CBlockIndex* blockindex = nullptr);
UniValue getstakinginfo(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "getstakinginfo\n"
            "Returns an object containing staking-related information.");

    UniValue obj(UniValue::VOBJ);

    int64_t nTipTime;
    float rCoinYearReward;
    CAmount nMoneySupply;
    {
        LOCK(cs_main);
        nTipTime = chainActive.Tip()->nTime;
        rCoinYearReward = Params().GetCoinYearReward(nTipTime) / CENT;
        nMoneySupply = chainActive.Tip()->nMoneySupply;
    }

    uint64_t nWeight = pwallet->GetStakeWeight();

    uint64_t nNetworkWeight = GetPoSKernelPS();

    bool fStaking = nWeight && fIsStaking;
    uint64_t nExpectedTime = fStaking ? (Params().GetTargetSpacing() * nNetworkWeight / nWeight) : 0;

    obj.pushKV("enabled", gArgs.GetBoolArg("-staking", true));
    obj.pushKV("staking", fStaking && pwallet->nIsStaking == CHDWallet::IS_STAKING);
    switch (pwallet->nIsStaking)
    {
        case CHDWallet::NOT_STAKING_BALANCE:
            obj.pushKV("cause", "low_balance");
            break;
        case CHDWallet::NOT_STAKING_DEPTH:
            obj.pushKV("cause", "low_depth");
            break;
        case CHDWallet::NOT_STAKING_LOCKED:
            obj.pushKV("cause", "locked");
            break;
        case CHDWallet::NOT_STAKING_LIMITED:
            obj.pushKV("cause", "limited");
            break;
        default:
            break;
    };

    obj.pushKV("errors", GetWarnings("statusbar"));

    obj.pushKV("percentyearreward", rCoinYearReward);
    obj.pushKV("moneysupply", ValueFromAmount(nMoneySupply));

    if (pwallet->nReserveBalance > 0)
        obj.pushKV("reserve", ValueFromAmount(pwallet->nReserveBalance));

    if (pwallet->nWalletDevFundCedePercent > 0)
        obj.pushKV("walletfoundationdonationpercent", pwallet->nWalletDevFundCedePercent);

    const DevFundSettings *pDevFundSettings = Params().GetDevFundSettings(nTipTime);
    if (pDevFundSettings && pDevFundSettings->nMinDevStakePercent > 0)
        obj.pushKV("foundationdonationpercent", pDevFundSettings->nMinDevStakePercent);

    obj.pushKV("currentblocksize", (uint64_t)nLastBlockSize);
    obj.pushKV("currentblocktx", (uint64_t)nLastBlockTx);
    obj.pushKV("pooledtx", (uint64_t)mempool.size());

    obj.pushKV("difficulty", GetDifficulty());
    obj.pushKV("last-search-time", (uint64_t)pwallet->nLastCoinStakeSearchTime);

    obj.pushKV("weight", (uint64_t)nWeight);
    obj.pushKV("netstakeweight", (uint64_t)nNetworkWeight);

    obj.pushKV("expectedtime", nExpectedTime);

    return obj;
};

UniValue getcoldstakinginfo(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "getcoldstakinginfo\n"
            "Returns an object containing coldstaking-related information.");

    UniValue obj(UniValue::VOBJ);

    LOCK2(cs_main, pwallet->cs_wallet);
    std::vector<COutput> vecOutputs;

    bool include_unsafe = false;
    bool fIncludeImmature = true;
    CAmount nMinimumAmount = 0;
    CAmount nMaximumAmount = MAX_MONEY;
    CAmount nMinimumSumAmount = MAX_MONEY;
    uint64_t nMaximumCount = 0;
    int nMinDepth = 0;
    int nMaxDepth = 0x7FFFFFFF;

    int nHeight = chainActive.Tip()->nHeight;
    int nRequiredDepth = std::min((int)(Params().GetStakeMinConfirmations()-1), (int)(nHeight / 2));

    pwallet->AvailableCoins(vecOutputs, !include_unsafe, nullptr, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount, nMinDepth, nMaxDepth, fIncludeImmature);

    CAmount nStakeable = 0;
    CAmount nColdStakeable = 0;
    CAmount nWalletStaking = 0;

    CKeyID keyID;
    CScript coinstakePath;
    for (const auto &out : vecOutputs)
    {
        const CScript *scriptPubKey = out.tx->tx->vpout[out.i]->GetPScriptPubKey();
        CAmount nValue = out.tx->tx->vpout[out.i]->GetValue();

        if (scriptPubKey->IsPayToPublicKeyHash() || scriptPubKey->IsPayToPublicKeyHash256())
        {
            if (!out.fSpendable)
                continue;
            nStakeable += nValue;
        } else
        if (scriptPubKey->IsPayToPublicKeyHash256_CS() || scriptPubKey->IsPayToScriptHash256_CS() || scriptPubKey->IsPayToScriptHash_CS())
        {
            // Show output on both the spending and staking wallets
            if (!out.fSpendable)
            {
                if (!ExtractStakingKeyID(*scriptPubKey, keyID)
                    || !pwallet->HaveKey(keyID))
                    continue;
            };
            nColdStakeable += nValue;
        } else
        {
            continue;
        };

        if (out.nDepth < nRequiredDepth)
            continue;

        if (!ExtractStakingKeyID(*scriptPubKey, keyID))
            continue;
        if (pwallet->HaveKey(keyID))
            nWalletStaking += nValue;
    };

    obj.pushKV("coin_in_stakeable_script", ValueFromAmount(nStakeable));
    obj.pushKV("coin_in_coldstakeable_script", ValueFromAmount(nColdStakeable));
    CAmount nTotal = nColdStakeable + nStakeable;
    obj.pushKV("percent_in_coldstakeable_script",
        UniValue(UniValue::VNUM, strprintf("%.2f", nTotal == 0 ? 0.0 : (nColdStakeable * 10000 / nTotal) / 100.0)));
    obj.pushKV("currently_staking", ValueFromAmount(nWalletStaking));

    return obj;
};


UniValue listunspentanon(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 5)
        throw std::runtime_error(
            "listunspentanon ( minconf maxconf  [\"addresses\",...] [include_unsafe] )\n"
            "\nReturns array of unspent transaction anon outputs\n"
            "with between minconf and maxconf (inclusive) confirmations.\n"
            "Optionally filter to only include txouts paid to specified addresses.\n"
            "\nArguments:\n"
            "1. minconf          (numeric, optional, default=1) The minimum confirmations to filter\n"
            "2. maxconf          (numeric, optional, default=9999999) The maximum confirmations to filter\n"
            "3. \"addresses\"    (string) A json array of particl addresses to filter\n"
            "    [\n"
            "      \"address\"   (string) particl address\n"
            "      ,...\n"
            "    ]\n"
            "4. include_unsafe (bool, optional, default=true) Include outputs that are not safe to spend\n"
            "                  because they come from unconfirmed untrusted transactions or unconfirmed\n"
            "                  replacement transactions (cases where we are less sure that a conflicting\n"
            "                  transaction won't be mined).\n"
            "5. query_options    (json, optional) JSON with query options\n"
            "    {\n"
            "      \"minimumAmount\"    (numeric or string, default=0) Minimum value of each UTXO in " + CURRENCY_UNIT + "\n"
            "      \"maximumAmount\"    (numeric or string, default=unlimited) Maximum value of each UTXO in " + CURRENCY_UNIT + "\n"
            "      \"maximumCount\"     (numeric or string, default=unlimited) Maximum number of UTXOs\n"
            "      \"minimumSumAmount\" (numeric or string, default=unlimited) Minimum sum value of all UTXOs in " + CURRENCY_UNIT + "\n"
            "      \"cc_format\"        (bool, default=false) Format for coincontrol\n"
            "      \"include_immature\" (bool, default=false) Include immature outputs\n"
            "    }\n"
            "\nResult\n"
            "[                   (array of json object)\n"
            "  {\n"
            "    \"txid\" : \"txid\",          (string) the transaction id \n"
            "    \"vout\" : n,               (numeric) the vout value\n"
            "    \"address\" : \"address\",    (string) the particl address\n"
            "    \"account\" : \"account\",    (string) DEPRECATED. The associated account, or \"\" for the default account\n"
            //"    \"scriptPubKey\" : \"key\",   (string) the script key\n"
            "    \"amount\" : x.xxx,         (numeric) the transaction output amount in " + CURRENCY_UNIT + "\n"
            "    \"confirmations\" : n,      (numeric) The number of confirmations\n"
            //"    \"redeemScript\" : n        (string) The redeemScript if scriptPubKey is P2SH\n"
            //"    \"spendable\" : xxx,        (bool) Whether we have the private keys to spend this output\n"
            //"    \"solvable\" : xxx          (bool) Whether we know how to spend this output, ignoring the lack of keys\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples\n"
            + HelpExampleCli("listunspentanon", "")
            + HelpExampleCli("listunspentanon", "6 9999999 \"[\\\"PfqK97PXYfqRFtdYcZw82x3dzPrZbEAcYa\\\",\\\"Pka9M2Bva8WetQhQ4ngC255HAbMJf5P5Dc\\\"]\"")
            + HelpExampleRpc("listunspentanon", "6, 9999999 \"[\\\"PfqK97PXYfqRFtdYcZw82x3dzPrZbEAcYa\\\",\\\"Pka9M2Bva8WetQhQ4ngC255HAbMJf5P5Dc\\\"]\"")
        );

    int nMinDepth = 1;
    if (request.params.size() > 0 && !request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VNUM);
        nMinDepth = request.params[0].get_int();
    }

    int nMaxDepth = 0x7FFFFFFF;
    if (request.params.size() > 1 && !request.params[1].isNull()) {
        RPCTypeCheckArgument(request.params[1], UniValue::VNUM);
        nMaxDepth = request.params[1].get_int();
    }

    std::set<CBitcoinAddress> setAddress;
    if (request.params.size() > 2 && !request.params[2].isNull()) {
        RPCTypeCheckArgument(request.params[2], UniValue::VARR);
        UniValue inputs = request.params[2].get_array();
        for (unsigned int idx = 0; idx < inputs.size(); idx++) {
            const UniValue& input = inputs[idx];
            CBitcoinAddress address(input.get_str());
            if (!address.IsValidStealthAddress())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Particl stealth address: ")+input.get_str());
            if (setAddress.count(address))
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ")+input.get_str());
           setAddress.insert(address);
        }
    }

    bool include_unsafe = true;
    if (request.params.size() > 3 && !request.params[3].isNull()) {
        RPCTypeCheckArgument(request.params[3], UniValue::VBOOL);
        include_unsafe = request.params[3].get_bool();
    }

    bool fCCFormat = false;
    bool fIncludeImmature = false;
    CAmount nMinimumAmount = 0;
    CAmount nMaximumAmount = MAX_MONEY;
    CAmount nMinimumSumAmount = MAX_MONEY;
    uint64_t nMaximumCount = 0;

    if (!request.params[4].isNull()) {
        const UniValue& options = request.params[4].get_obj();

        RPCTypeCheckObj(options,
            {
                {"maximumCount",            UniValueType(UniValue::VNUM)},
                {"cc_format",               UniValueType(UniValue::VBOOL)},
            }, true, false);

        if (options.exists("minimumAmount"))
            nMinimumAmount = AmountFromValue(options["minimumAmount"]);

        if (options.exists("maximumAmount"))
            nMaximumAmount = AmountFromValue(options["maximumAmount"]);

        if (options.exists("minimumSumAmount"))
            nMinimumSumAmount = AmountFromValue(options["minimumSumAmount"]);

        if (options.exists("maximumCount"))
            nMaximumCount = options["maximumCount"].get_int64();

        if (options.exists("cc_format"))
            fCCFormat = options["cc_format"].get_bool();

        if (options.exists("include_immature"))
            fIncludeImmature = options["include_immature"].get_bool();
    }

    UniValue results(UniValue::VARR);
    std::vector<COutputR> vecOutputs;
    assert(pwallet != nullptr);
    LOCK2(cs_main, pwallet->cs_wallet);

    // TODO: filter on stealth address
    pwallet->AvailableAnonCoins(vecOutputs, !include_unsafe, nullptr, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount, nMinDepth, nMaxDepth, fIncludeImmature);

    for (const auto &out : vecOutputs)
    {
        if (out.nDepth < nMinDepth || out.nDepth > nMaxDepth)
            continue;

        const COutputRecord *pout = out.rtx->second.GetOutput(out.i);

        if (!pout)
        {
            LogPrintf("%s: ERROR - Missing output %s %d\n", __func__, out.txhash.ToString(), out.i);
            continue;
        };

        CAmount nValue = pout->nValue;


        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", out.txhash.GetHex());
        entry.pushKV("vout", out.i);

        if (pout->vPath.size() > 0 && pout->vPath[0] == ORA_STEALTH)
        {
            if (pout->vPath.size() < 5)
            {
                LogPrintf("%s: Warning, malformed vPath.", __func__);
            } else
            {
                uint32_t sidx;
                memcpy(&sidx, &pout->vPath[1], 4);
                CStealthAddress sx;
                if (pwallet->GetStealthByIndex(sidx, sx))
                    entry.pushKV("address", sx.Encoded());
            };
        };

        if (!entry.exists("address"))
            entry.pushKV("address", "unknown");
        if (fCCFormat)
        {
            entry.pushKV("time", out.rtx->second.GetTxTime());
            entry.pushKV("amount", nValue);
        } else
        {
            entry.pushKV("amount", ValueFromAmount(nValue));
        };
        entry.pushKV("confirmations", out.nDepth);
        //entry.pushKV("spendable", out.fSpendable);
        //entry.pushKV("solvable", out.fSolvable);
        entry.push_back(Pair("safe", out.fSafe));
        if (fIncludeImmature)
            entry.push_back(Pair("mature", out.fMature));

        results.push_back(entry);
    }

    return results;
};

UniValue listunspentblind(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 5)
        throw std::runtime_error(
            "listunspentblind ( minconf maxconf  [\"addresses\",...] [include_unsafe] )\n"
            "\nReturns array of unspent transaction blind outputs\n"
            "with between minconf and maxconf (inclusive) confirmations.\n"
            "Optionally filter to only include txouts paid to specified addresses.\n"
            "\nArguments:\n"
            "1. minconf          (numeric, optional, default=1) The minimum confirmations to filter\n"
            "2. maxconf          (numeric, optional, default=9999999) The maximum confirmations to filter\n"
            "3. \"addresses\"    (string) A json array of particl addresses to filter\n"
            "    [\n"
            "      \"address\"   (string) particl address\n"
            "      ,...\n"
            "    ]\n"
            "4. include_unsafe (bool, optional, default=true) Include outputs that are not safe to spend\n"
            "                  because they come from unconfirmed untrusted transactions or unconfirmed\n"
            "                  replacement transactions (cases where we are less sure that a conflicting\n"
            "                  transaction won't be mined).\n"
            "5. query_options    (json, optional) JSON with query options\n"
            "    {\n"
            "      \"minimumAmount\"    (numeric or string, default=0) Minimum value of each UTXO in " + CURRENCY_UNIT + "\n"
            "      \"maximumAmount\"    (numeric or string, default=unlimited) Maximum value of each UTXO in " + CURRENCY_UNIT + "\n"
            "      \"maximumCount\"     (numeric or string, default=unlimited) Maximum number of UTXOs\n"
            "      \"minimumSumAmount\" (numeric or string, default=unlimited) Minimum sum value of all UTXOs in " + CURRENCY_UNIT + "\n"
            "      \"cc_format\"        (bool, default=false) Format for coincontrol\n"
            "    }\n"
            "\nResult\n"
            "[                   (array of json object)\n"
            "  {\n"
            "    \"txid\" : \"txid\",          (string) the transaction id \n"
            "    \"vout\" : n,               (numeric) the vout value\n"
            "    \"address\" : \"address\",    (string) the particl address\n"
            "    \"account\" : \"account\",    (string) DEPRECATED. The associated account, or \"\" for the default account\n"
            "    \"scriptPubKey\" : \"key\",   (string) the script key\n"
            "    \"amount\" : x.xxx,         (numeric) the transaction output amount in " + CURRENCY_UNIT + "\n"
            "    \"confirmations\" : n,      (numeric) The number of confirmations\n"
            "    \"redeemScript\" : n        (string) The redeemScript if scriptPubKey is P2SH\n"
            //"    \"spendable\" : xxx,        (bool) Whether we have the private keys to spend this output\n"
            //"    \"solvable\" : xxx          (bool) Whether we know how to spend this output, ignoring the lack of keys\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples\n"
            + HelpExampleCli("listunspentblind", "")
            + HelpExampleCli("listunspentblind", "6 9999999 \"[\\\"PfqK97PXYfqRFtdYcZw82x3dzPrZbEAcYa\\\",\\\"Pka9M2Bva8WetQhQ4ngC255HAbMJf5P5Dc\\\"]\"")
            + HelpExampleRpc("listunspentblind", "6, 9999999 \"[\\\"PfqK97PXYfqRFtdYcZw82x3dzPrZbEAcYa\\\",\\\"Pka9M2Bva8WetQhQ4ngC255HAbMJf5P5Dc\\\"]\"")
        );

    int nMinDepth = 1;
    if (request.params.size() > 0 && !request.params[0].isNull()) {
        RPCTypeCheckArgument(request.params[0], UniValue::VNUM);
        nMinDepth = request.params[0].get_int();
    }

    int nMaxDepth = 0x7FFFFFFF;
    if (request.params.size() > 1 && !request.params[1].isNull()) {
        RPCTypeCheckArgument(request.params[1], UniValue::VNUM);
        nMaxDepth = request.params[1].get_int();
    }

    bool fCCFormat = false;
    CAmount nMinimumAmount = 0;
    CAmount nMaximumAmount = MAX_MONEY;
    CAmount nMinimumSumAmount = MAX_MONEY;
    uint64_t nMaximumCount = 0;

    if (!request.params[4].isNull()) {
        const UniValue& options = request.params[4].get_obj();

        RPCTypeCheckObj(options,
            {
                {"maximumCount",            UniValueType(UniValue::VNUM)},
                {"cc_format",               UniValueType(UniValue::VBOOL)},
            }, true, false);

        if (options.exists("minimumAmount"))
            nMinimumAmount = AmountFromValue(options["minimumAmount"]);

        if (options.exists("maximumAmount"))
            nMaximumAmount = AmountFromValue(options["maximumAmount"]);

        if (options.exists("minimumSumAmount"))
            nMinimumSumAmount = AmountFromValue(options["minimumSumAmount"]);

        if (options.exists("maximumCount"))
            nMaximumCount = options["maximumCount"].get_int64();

        if (options.exists("cc_format"))
            fCCFormat = options["cc_format"].get_bool();
    }

    std::set<CBitcoinAddress> setAddress;
    if (request.params.size() > 2 && !request.params[2].isNull()) {
        RPCTypeCheckArgument(request.params[2], UniValue::VARR);
        UniValue inputs = request.params[2].get_array();
        for (unsigned int idx = 0; idx < inputs.size(); idx++) {
            const UniValue& input = inputs[idx];
            CBitcoinAddress address(input.get_str());
            if (!address.IsValidStealthAddress())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Particl stealth address: ")+input.get_str());
            if (setAddress.count(address))
                throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ")+input.get_str());
           setAddress.insert(address);
        }
    }

    bool include_unsafe = true;
    if (request.params.size() > 3 && !request.params[3].isNull()) {
        RPCTypeCheckArgument(request.params[3], UniValue::VBOOL);
        include_unsafe = request.params[3].get_bool();
    }

    UniValue results(UniValue::VARR);
    std::vector<COutputR> vecOutputs;
    assert(pwallet != nullptr);
    LOCK2(cs_main, pwallet->cs_wallet);

    pwallet->AvailableBlindedCoins(vecOutputs, !include_unsafe, nullptr, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount, nMinDepth, nMaxDepth);

    for (const auto &out : vecOutputs)
    {
        if (out.nDepth < nMinDepth || out.nDepth > nMaxDepth)
            continue;

        const COutputRecord *pout = out.rtx->second.GetOutput(out.i);

        if (!pout)
        {
            LogPrintf("%s: ERROR - Missing output %s %d\n", __func__, out.txhash.ToString(), out.i);
            continue;
        };

        CAmount nValue = pout->nValue;

        CTxDestination address;
        const CScript *scriptPubKey = &pout->scriptPubKey;
        bool fValidAddress;

        fValidAddress = ExtractDestination(*scriptPubKey, address);
        if (setAddress.size() && (!fValidAddress || !setAddress.count(address)))
            continue;

        UniValue entry(UniValue::VOBJ);
        entry.pushKV("txid", out.txhash.GetHex());
        entry.pushKV("vout", out.i);

        if (fValidAddress) {
            entry.pushKV("address", CBitcoinAddress(address).ToString());

            if (pwallet->mapAddressBook.count(address))
                entry.pushKV("account", pwallet->mapAddressBook[address].name);

            if (scriptPubKey->IsPayToScriptHash()) {
                const CScriptID& hash = boost::get<CScriptID>(address);
                CScript redeemScript;
                if (pwallet->GetCScript(hash, redeemScript))
                    entry.pushKV("redeemScript", HexStr(redeemScript.begin(), redeemScript.end()));
            }
            if (scriptPubKey->IsPayToScriptHash256()) {
                const CScriptID256& hash = boost::get<CScriptID256>(address);
                CScriptID scriptID;
                scriptID.Set(hash);
                CScript redeemScript;
                if (pwallet->GetCScript(scriptID, redeemScript))
                    entry.pushKV("redeemScript", HexStr(redeemScript.begin(), redeemScript.end()));
            }
        }

        entry.pushKV("scriptPubKey", HexStr(scriptPubKey->begin(), scriptPubKey->end()));

        if (fCCFormat)
        {
            entry.pushKV("time", out.rtx->second.GetTxTime());
            entry.pushKV("amount", nValue);
        } else
        {
            entry.pushKV("amount", ValueFromAmount(nValue));
        };
        entry.pushKV("confirmations", out.nDepth);
        //entry.push_back(Pair("spendable", out.fSpendable));
        //entry.push_back(Pair("solvable", out.fSolvable));
        entry.push_back(Pair("safe", out.fSafe));
        results.push_back(entry);
    }

    return results;
};

/*
UniValue gettransactionsummary(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "gettransactionsummary <txhash>\n"
            "Returns a summary of a transaction in the wallet.");

    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);

    UniValue obj(UniValue::VOBJ);

    uint256 hash;
    hash.SetHex(request.params[0].get_str());

    {
        LOCK(pwallet->cs_wallet);

        MapRecords_t::const_iterator mri;
        MapWallet_t::const_iterator mwi;

        if ((mwi = pwallet->mapWallet.find(hash)) != pwallet->mapWallet.end())
        {
            const CWalletTx &wtx = mwi->second;

            obj.push_back(Pair("time", (int64_t)wtx.nTimeSmart));

        } else
        if ((mri = pwallet->mapRecords.find(hash)) != pwallet->mapRecords.end())
        {
            const CTransactionRecord &rtx = mri->second;

            obj.push_back(Pair("time", std::min((int64_t)rtx.nTimeReceived, (int64_t)rtx.nBlockTime)));
        } else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown transaction."));
        };
    }

    obj.push_back(Pair("part_balance", ""));
    obj.push_back(Pair("blind_balance", ""));
    obj.push_back(Pair("anon_balance", ""));


    return obj;
};
*/


static int AddOutput(uint8_t nType, std::vector<CTempRecipient> &vecSend, const CTxDestination &address, CAmount nValue,
    bool fSubtractFeeFromAmount, std::string &sNarr, std::string &sError)
{
    CTempRecipient r;
    r.nType = nType;
    r.SetAmount(nValue);
    r.fSubtractFeeFromAmount = fSubtractFeeFromAmount;
    r.address = address;
    r.sNarration = sNarr;

    vecSend.push_back(r);
    return 0;
};

static UniValue SendToInner(const JSONRPCRequest &request, OutputTypes typeIn, OutputTypes typeOut)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    EnsureWalletIsUnlocked(pwallet);

    if (pwallet->GetBroadcastTransactions() && !g_connman)
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

    if (typeOut == OUTPUT_RINGCT && Params().NetworkID() == "main")
        throw std::runtime_error("Disabled on mainnet.");

    CAmount nTotal = 0;

    std::vector<CTempRecipient> vecSend;
    std::string sError;

    size_t nCommentOfs = 2;
    size_t nRingSizeOfs = 6;
    size_t nTestFeeOfs = 99;
    size_t nCoinControlOfs = 99;

    if (request.params[0].isArray())
    {
        const UniValue &outputs = request.params[0].get_array();

        for (size_t k = 0; k < outputs.size(); ++k)
        {
            if (!outputs[k].isObject())
                throw JSONRPCError(RPC_TYPE_ERROR, "Not an object");
            const UniValue &obj = outputs[k].get_obj();

            std::string sAddress;
            CAmount nAmount;

            if (obj.exists("address"))
                sAddress = obj["address"].get_str();
            else
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Must provide an address.");

            CBitcoinAddress address(sAddress);

            if (typeOut == OUTPUT_RINGCT
                && !address.IsValidStealthAddress())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Particl stealth address");

            if (!obj.exists("script") && !address.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Particl address");

            if (obj.exists("amount"))
                nAmount = AmountFromValue(obj["amount"]);
            else
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Must provide an amount.");

            if (nAmount <= 0)
                throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
            nTotal += nAmount;

            bool fSubtractFeeFromAmount = false;
            if (obj.exists("subfee"))
                fSubtractFeeFromAmount = obj["subfee"].get_bool();

            std::string sNarr;
            if (obj.exists("narr"))
                sNarr = obj["narr"].get_str();

            if (0 != AddOutput(typeOut, vecSend, address.Get(), nAmount, fSubtractFeeFromAmount, sNarr, sError))
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("AddOutput failed: %s.", sError));

            if (obj.exists("script"))
            {
                CTempRecipient &r = vecSend.back();

                if (sAddress != "script")
                    JSONRPCError(RPC_INVALID_PARAMETER, "address parameter must be 'script' to set script explicitly.");

                std::string sScript = obj["script"].get_str();
                std::vector<uint8_t> scriptData = ParseHex(sScript);
                r.scriptPubKey = CScript(scriptData.begin(), scriptData.end());
                r.fScriptSet = true;

                if (typeOut != OUTPUT_STANDARD)
                    throw std::runtime_error("In progress, setting script only works for standard outputs.");
            };
        };
        nCommentOfs = 1;
        nRingSizeOfs = 3;
        nTestFeeOfs = 5;
        nCoinControlOfs = 6;
    } else
    {
        std::string sAddress = request.params[0].get_str();
        CBitcoinAddress address(sAddress);

        if (typeOut == OUTPUT_RINGCT
            && !address.IsValidStealthAddress())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Particl stealth address");

        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Particl address");

        CAmount nAmount = AmountFromValue(request.params[1]);
        if (nAmount <= 0)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
        nTotal += nAmount;

        bool fSubtractFeeFromAmount = false;
        if (request.params.size() > 4)
            fSubtractFeeFromAmount = request.params[4].get_bool();

        std::string sNarr;
        if (request.params.size() > 5)
        {
            sNarr = request.params[5].get_str();
            if (sNarr.length() > 24)
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Narration can range from 1 to 24 characters.");
        };

        if (0 != AddOutput(typeOut, vecSend, address.Get(), nAmount, fSubtractFeeFromAmount, sNarr, sError))
            throw JSONRPCError(RPC_MISC_ERROR, strprintf("AddOutput failed: %s.", sError));
    };

    switch (typeIn)
    {
        case OUTPUT_STANDARD:
            if (nTotal > pwallet->GetBalance())
                throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
            break;
        case OUTPUT_CT:
            if (nTotal > pwallet->GetBlindBalance())
                throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient blinded funds");
            break;
        case OUTPUT_RINGCT:
            if (nTotal > pwallet->GetAnonBalance())
                throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient anon funds");
            break;
        default:
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Unknown input type: %d.", typeIn));
    };

    // Wallet comments
    CWalletTx wtx;
    CTransactionRecord rtx;

    size_t nv = nCommentOfs;
    if (request.params.size() > nv && !request.params[nv].isNull())
    {
        std::string s = request.params[nv].get_str();
        part::TrimQuotes(s);
        if (!s.empty())
        {
            std::vector<uint8_t> v(s.begin(), s.end());
            wtx.mapValue["comment"] = s;
            rtx.mapValue[RTXVT_COMMENT] = v;
        };
    };
    nv++;
    if (request.params.size() > nv && !request.params[nv].isNull())
    {
        std::string s = request.params[nv].get_str();
        part::TrimQuotes(s);
        if (!s.empty())
        {
            std::vector<uint8_t> v(s.begin(), s.end());
            wtx.mapValue["to"] = s;
            rtx.mapValue[RTXVT_TO] = v;
        };
    };

    nv = nRingSizeOfs;
    size_t nRingSize = 4; // TODO: default size?
    if (request.params.size() > nv)
        nRingSize = request.params[nv].get_int();
    nv++;
    size_t nInputsPerSig = 64;
    if (request.params.size() > nv)
        nInputsPerSig = request.params[nv].get_int();

    bool fShowHex = false;
    bool fCheckFeeOnly = false;
    nv = nTestFeeOfs;
    if (request.params.size() > nv)
        fCheckFeeOnly = request.params[nv].get_bool();


    CCoinControl coincontrol;

    nv = nCoinControlOfs;
    if (request.params.size() > nv
        && request.params[nv].isObject())
    {
        const UniValue &uvCoinControl = request.params[nv].get_obj();

        if (uvCoinControl.exists("changeaddress"))
        {
            std::string sChangeAddress = uvCoinControl["changeaddress"].get_str();

            // Check for script
            bool fHaveScript = false;
            if (IsHex(sChangeAddress))
            {
                std::vector<uint8_t> vScript = ParseHex(sChangeAddress);
                CScript script(vScript.begin(), vScript.end());

                txnouttype whichType;
                if (IsStandard(script, whichType, true))
                {
                    coincontrol.scriptChange = script;
                    fHaveScript = true;
                };
            };

            if (!fHaveScript)
            {
                CBitcoinAddress addrChange(sChangeAddress);
                coincontrol.destChange = addrChange.Get();
            };
        };

        const UniValue &uvInputs = uvCoinControl["inputs"];
        if (uvInputs.isArray())
        {
            for (size_t i = 0; i < uvInputs.size(); ++i)
            {
                const UniValue &uvi = uvInputs[i];
                RPCTypeCheckObj(uvi,
                {
                    {"tx", UniValueType(UniValue::VSTR)},
                    {"n", UniValueType(UniValue::VNUM)},
                });

                COutPoint op(uint256S(uvi["tx"].get_str()), uvi["n"].get_int());
                coincontrol.setSelected.insert(op);
            };
        };

        if (uvCoinControl.exists("feeRate") && uvCoinControl.exists("estimate_mode"))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both estimate_mode and feeRate");
        if (uvCoinControl.exists("feeRate") && uvCoinControl.exists("conf_target"))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both conf_target and feeRate");

        if (uvCoinControl.exists("replaceable"))
        {
            if (!uvCoinControl["replaceable"].isBool())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Replaceable parameter must be boolean.");

            coincontrol.signalRbf = uvCoinControl["replaceable"].get_bool();
        };

        if (uvCoinControl.exists("conf_target"))
        {
            if (!uvCoinControl["conf_target"].isNum())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "conf_target parameter must be numeric.");

            coincontrol.m_confirm_target = ParseConfirmTarget(uvCoinControl["conf_target"]);
        };

        if (uvCoinControl.exists("estimate_mode"))
        {
            if (!uvCoinControl["estimate_mode"].isStr())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "estimate_mode parameter must be a string.");

            if (!FeeModeFromString(uvCoinControl["estimate_mode"].get_str(), coincontrol.m_fee_mode))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid estimate_mode parameter");
        };

        if (uvCoinControl.exists("feeRate"))
        {
            coincontrol.m_feerate = CFeeRate(AmountFromValue(uvCoinControl["feeRate"]));
            coincontrol.fOverrideFeeRate = true;
        };

        if (uvCoinControl["debug"].isBool() && uvCoinControl["debug"].get_bool() == true)
            fShowHex = true;
    };


    CAmount nFeeRet = 0;
    switch (typeIn)
    {
        case OUTPUT_STANDARD:
            if (0 != pwallet->AddStandardInputs(wtx, rtx, vecSend, !fCheckFeeOnly, nFeeRet, &coincontrol, sError))
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("AddStandardInputs failed: %s.", sError));
            break;
        case OUTPUT_CT:
            if (0 != pwallet->AddBlindedInputs(wtx, rtx, vecSend, !fCheckFeeOnly, nFeeRet, &coincontrol, sError))
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("AddBlindedInputs failed: %s.", sError));
            break;
        case OUTPUT_RINGCT:
            if (0 != pwallet->AddAnonInputs(wtx, rtx, vecSend, !fCheckFeeOnly, nRingSize, nInputsPerSig, nFeeRet, &coincontrol, sError))
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("AddAnonInputs failed: %s.", sError));
            break;
        default:
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Unknown input type: %d.", typeIn));
    };

    if (fCheckFeeOnly)
    {
        UniValue result(UniValue::VOBJ);
        result.pushKV("fee", ValueFromAmount(nFeeRet));
        result.pushKV("bytes", (int)GetVirtualTransactionSize(*(wtx.tx)));

        if (fShowHex)
        {
            std::string strHex = EncodeHexTx(*(wtx.tx), RPCSerializationFlags());
            result.pushKV("hex", strHex);
        };

        UniValue objChangedOutputs(UniValue::VOBJ);
        std::map<std::string, CAmount> mapChanged; // Blinded outputs are split, join the values for display
        for (const auto &r : vecSend)
        {
            if (!r.fChange
                && r.nAmount != r.nAmountSelected)
            {
                std::string sAddr = CBitcoinAddress(r.address).ToString();

                if (mapChanged.count(sAddr))
                    mapChanged[sAddr] += r.nAmount;
                else
                    mapChanged[sAddr] = r.nAmount;
            };
        };

        for (const auto &v : mapChanged)
            objChangedOutputs.pushKV(v.first, v.second);

        result.pushKV("outputs_fee", objChangedOutputs);

        return result;
    };

    // Store sent narrations
    for (const auto &r : vecSend)
    {
        if (r.nType != OUTPUT_STANDARD
            || r.sNarration.size() < 1)
            continue;
        std::string sKey = strprintf("n%d", r.n);
        wtx.mapValue[sKey] = r.sNarration;
    };

    CValidationState state;
    CReserveKey reservekey(pwallet);
    if (typeIn == OUTPUT_STANDARD && typeOut == OUTPUT_STANDARD)
    {
        if (!pwallet->CommitTransaction(wtx, reservekey, g_connman.get(), state))
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Transaction commit failed: %s", state.GetRejectReason()));
    } else
    {
        if (!pwallet->CommitTransaction(wtx, rtx, reservekey, g_connman.get(), state))
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Transaction commit failed: %s", state.GetRejectReason()));
    };

    UniValue vErrors(UniValue::VARR);
    if (!state.IsValid())
    {
        // This can happen if the mempool rejected the transaction.  Report
        // what happened in the "errors" response.
        vErrors.push_back(strprintf("Error: The transaction was rejected: %s", FormatStateMessage(state)));

        UniValue result(UniValue::VOBJ);
        result.pushKV("txid", wtx.GetHash().GetHex());
        result.pushKV("errors", vErrors);
        return result;
    };

    pwallet->PostProcessTempRecipients(vecSend);

    return wtx.GetHash().GetHex();
}

static const char *TypeToWord(OutputTypes type)
{
    switch (type)
    {
        case OUTPUT_STANDARD:
            return "part";
        case OUTPUT_CT:
            return "blind";
        case OUTPUT_RINGCT:
            return "anon";
        default:
            break;
    };
    return "unknown";
};

static OutputTypes WordToType(std::string &s)
{
    if (s == "part")
        return OUTPUT_STANDARD;
    if (s == "blind")
        return OUTPUT_CT;
    if (s == "anon")
        return OUTPUT_RINGCT;
    return OUTPUT_NULL;
};

static std::string SendHelp(CHDWallet *pwallet, OutputTypes typeIn, OutputTypes typeOut)
{
    std::string rv;

    std::string cmd = std::string("send") + TypeToWord(typeIn) + "to" + TypeToWord(typeOut);

    rv = cmd + " \"address\" amount ( \"comment\" \"comment-to\" subtractfeefromamount \"narration\"";
    if (typeIn == OUTPUT_RINGCT)
        rv += " ringsize inputs_per_sig";
    rv += ")\n";

    rv += "\nSend an amount of ";
    rv += typeIn == OUTPUT_RINGCT ? "anon" : typeIn == OUTPUT_CT ? "blinded" : "";
    rv += std::string(" part in a") + (typeOut == OUTPUT_RINGCT || typeOut == OUTPUT_CT ? " blinded" : "") + " payment to a given address"
        + (typeOut == OUTPUT_CT ? " in anon part": "") + ".\n";

    rv += HelpRequiringPassphrase(pwallet);

    rv +=   "\nArguments:\n"
            "1. \"address\"     (string, required) The particl address to send to.\n"
            "2. \"amount\"      (numeric or string, required) The amount in " + CURRENCY_UNIT + " to send. eg 0.1\n"
            "3. \"comment\"     (string, optional) A comment used to store what the transaction is for. \n"
            "                            This is not part of the transaction, just kept in your wallet.\n"
            "4. \"comment_to\"  (string, optional) A comment to store the name of the person or organization \n"
            "                            to which you're sending the transaction. This is not part of the \n"
            "                            transaction, just kept in your wallet.\n"
            "5. subtractfeefromamount  (boolean, optional, default=false) The fee will be deducted from the amount being sent.\n"
            "                            The recipient will receive less " + CURRENCY_UNIT + " than you enter in the amount field.\n"
            "6. \"narration\"   (string, optional) Up to 24 characters sent with the transaction.\n"
            "                            The narration is stored in the blockchain and is sent encrypted when destination is a stealth address and uncrypted otherwise.\n";
    if (typeIn == OUTPUT_RINGCT)
        rv +=
            "7. ringsize        (int, optional, default=4).\n"
            "8. inputs_per_sig  (int, optional, default=64).\n";

    rv +=
            "\nResult:\n"
            "\"txid\"           (string) The transaction id.\n";

    rv +=   "\nExamples:\n"
            + HelpExampleCli(cmd, "\"SPGyji8uZFip6H15GUfj6bsutRVLsCyBFL3P7k7T7MUDRaYU8GfwUHpfxonLFAvAwr2RkigyGfTgWMfzLAAP8KMRHq7RE8cwpEEekH\" 0.1");

    return rv;
};

UniValue sendparttoblind(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_STANDARD, OUTPUT_CT));

    return SendToInner(request, OUTPUT_STANDARD, OUTPUT_CT);
};

UniValue sendparttoanon(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_STANDARD, OUTPUT_RINGCT));

    return SendToInner(request, OUTPUT_STANDARD, OUTPUT_RINGCT);
};


UniValue sendblindtopart(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_CT, OUTPUT_STANDARD));

    return SendToInner(request, OUTPUT_CT, OUTPUT_STANDARD);
};

UniValue sendblindtoblind(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_CT, OUTPUT_CT));

    return SendToInner(request, OUTPUT_CT, OUTPUT_CT);
};

UniValue sendblindtoanon(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_CT, OUTPUT_RINGCT));

    return SendToInner(request, OUTPUT_CT, OUTPUT_RINGCT);
};


UniValue sendanontopart(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 8)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_RINGCT, OUTPUT_STANDARD));

    return SendToInner(request, OUTPUT_RINGCT, OUTPUT_STANDARD);
};

UniValue sendanontoblind(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 8)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_RINGCT, OUTPUT_CT));

    return SendToInner(request, OUTPUT_RINGCT, OUTPUT_CT);
};

UniValue sendanontoanon(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 8)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_RINGCT, OUTPUT_RINGCT));

    return SendToInner(request, OUTPUT_RINGCT, OUTPUT_RINGCT);
};

UniValue sendtypeto(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 3 || request.params.size() > 9)
        throw std::runtime_error(
            "sendtypeto \"typein\" \"typeout\" [{address: , amount: , narr: , subfee:},...] (\"comment\" \"comment-to\" ringsize inputs_per_sig test_fee coin_control)\n"
            "\nSend part to multiple outputs.\n"
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. \"typein\"          (string, required) part/blind/anon\n"
            "2. \"typeout\"         (string, required) part/blind/anon\n"
            "3. \"outputs\"         (json, required) Array of output objects\n"
            "    3.1 \"address\"    (string, required) The particl address to send to.\n"
            "    3.2 \"amount\"     (numeric or string, required) The amount in " + CURRENCY_UNIT + " to send. eg 0.1\n"
            "    3.x \"narr\"       (string, optional) Up to 24 character narration sent with the transaction.\n"
            "    3.x \"subfee\"     (boolean, optional, default=false) The fee will be deducted from the amount being sent.\n"
            "    3.x \"script\"     (string, optional) Hex encoded script, will override the address.\n"
            "4. \"comment\"         (string, optional) A comment used to store what the transaction is for. \n"
            "                            This is not part of the transaction, just kept in your wallet.\n"
            "5. \"comment_to\"      (string, optional) A comment to store the name of the person or organization \n"
            "                            to which you're sending the transaction. This is not part of the \n"
            "                            transaction, just kept in your wallet.\n"
            "6. ringsize         (int, optional, default=4) Only applies when typein is anon.\n"
            "7. inputs_per_sig   (int, optional, default=64) Only applies when typein is anon.\n"
            "8. test_fee         (bool, optional, default=false) Only return the fee it would cost to send, txn is discarded.\n"
            "9. coin_control     (json, optional) Coincontrol object.\n"
            "   {\"changeaddress\": ,\n"
            "    \"inputs\": [{\"tx\":, \"n\":},...],\n"
            "    \"replaceable\": boolean,\n"
            "       Allow this transaction to be replaced by a transaction with higher fees via BIP 125\n"
            "    \"conf_target\": numeric,\n"
            "       Confirmation target (in blocks)\n"
            "    \"estimate_mode\": string,\n"
            "       The fee estimate mode, must be one of:\n"
            "           \"UNSET\"\n"
            "           \"ECONOMICAL\"\n"
            "           \"CONSERVATIVE\"\n"
            "     \"feeRate\"                (numeric, optional, default not set: makes wallet determine the fee) Set a specific feerate (" + CURRENCY_UNIT + " per KB)\n"
            "   }\n"
            "\nResult:\n"
            "\"txid\"              (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("sendtypeto", "anon part \"[{\\\"address\\\":\\\"PbpVcjgYatnkKgveaeqhkeQBFwjqR7jKBR\\\",\\\"amount\\\":0.1}]\""));

    std::string sTypeIn = request.params[0].get_str();
    std::string sTypeOut = request.params[1].get_str();

    OutputTypes typeIn = WordToType(sTypeIn);
    OutputTypes typeOut = WordToType(sTypeOut);

    if (typeIn == OUTPUT_NULL)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown input type.");
    if (typeOut == OUTPUT_NULL)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown output type.");

    JSONRPCRequest req = request;
    req.params.erase(0, 2);

    return SendToInner(req, typeIn, typeOut);
};

UniValue buildscript(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "buildscript json\n"
            "\nArguments:\n"
            "{recipe: , ...}\n"
            "\nRecipes:\n"
            "{\"recipe\":\"ifcoinstake\", \"addrstake\":\"addrA\", \"addrspend\":\"addrB\"}"
            "{\"recipe\":\"abslocktime\", \"time\":timestamp, \"addr\":\"addr\"}"
            "{\"recipe\":\"rellocktime\", \"time\":timestamp, \"addr\":\"addr\"}"
            );

    if (!request.params[0].isObject())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Input must be a json object.");

    const UniValue &params = request.params[0].get_obj();

    const UniValue &recipe = params["recipe"];
    if (!recipe.isStr())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing recipe.");

    std::string sRecipe = recipe.get_str();

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("recipe", sRecipe);

    CScript scriptOut;

    if (sRecipe == "ifcoinstake")
    {
        RPCTypeCheckObj(params,
        {
            {"addrstake", UniValueType(UniValue::VSTR)},
            {"addrspend", UniValueType(UniValue::VSTR)},
        });

        CBitcoinAddress addrTrue(params["addrstake"].get_str());
        CBitcoinAddress addrFalse(params["addrspend"].get_str());

        if (!addrTrue.IsValid())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid addrstake.");

        if (!addrFalse.IsValid())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid addrspend.");
        if (addrFalse.IsValid(CChainParams::PUBKEY_ADDRESS))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid addrspend, can't be p2pkh.");

        CScript scriptTrue = GetScriptForDestination(addrTrue.Get());
        CScript scriptFalse = GetScriptForDestination(addrFalse.Get());
        // TODO: More checks

        scriptOut = CScript() << OP_ISCOINSTAKE << OP_IF;
        scriptOut += scriptTrue;
        scriptOut << OP_ELSE;
        scriptOut += scriptFalse;
        scriptOut << OP_ENDIF;
    } else
    if (sRecipe == "abslocktime")
    {
        RPCTypeCheckObj(params,
        {
            {"time", UniValueType(UniValue::VNUM)},
            {"addr", UniValueType(UniValue::VSTR)},
        });

        CBitcoinAddress addr(params["addr"].get_str());
        if (!addr.IsValid())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid addr.");

        CScript scriptAddr = GetScriptForDestination(addr.Get());

        scriptOut = CScript() << params["time"].get_int64() << OP_CHECKLOCKTIMEVERIFY << OP_DROP;
        scriptOut += scriptAddr;
    } else
    if (sRecipe == "rellocktime")
    {
        RPCTypeCheckObj(params,
        {
            {"time", UniValueType(UniValue::VNUM)},
            {"addr", UniValueType(UniValue::VSTR)},
        });

        CBitcoinAddress addr(params["addr"].get_str());
        if (!addr.IsValid())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid addr.");

        CScript scriptAddr = GetScriptForDestination(addr.Get());

        scriptOut = CScript() << params["time"].get_int64() << OP_CHECKSEQUENCEVERIFY << OP_DROP;
        scriptOut += scriptAddr;
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown recipe.");
    };

    obj.pushKV("hex", HexStr(scriptOut.begin(), scriptOut.end()));
    obj.pushKV("asm", ScriptToAsmStr(scriptOut));

    return obj;
};

UniValue debugwallet(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "debugwallet [attempt_repair]\n"
            "Detect problems in wallet.\n");

    bool fAttemptRepair = false;
    if (request.params.size() > 0)
    {
        std::string s = request.params[0].get_str();
        if (part::IsStringBoolPositive(s))
            fAttemptRepair = true;
    };

    EnsureWalletIsUnlocked(pwallet);

    UniValue result(UniValue::VOBJ);
    result.pushKV("wallet_name", pwallet->GetName());


    size_t nUnabandonedOrphans = 0;
    size_t nCoinStakes = 0;
    size_t nAbandonedOrphans = 0;
    size_t nMapWallet = 0;

    {
        LOCK2(cs_main, pwallet->cs_wallet);

        std::map<uint256, CWalletTx>::const_iterator it;
        for (it = pwallet->mapWallet.begin(); it != pwallet->mapWallet.end(); ++it)
        {
            const uint256 &wtxid = it->first;
            const CWalletTx &wtx = it->second;

            nMapWallet++;
            if (wtx.IsCoinStake())
            {
                nCoinStakes++;
                if (wtx.GetDepthInMainChain() < 1)
                {
                    if (wtx.isAbandoned())
                    {
                        nAbandonedOrphans++;
                    } else
                    {
                        nUnabandonedOrphans++;
                        LogPrintf("Unabandoned orphaned stake: %s\n", wtxid.ToString());

                        if (fAttemptRepair)
                        {
                            if (!pwallet->AbandonTransaction(wtxid))
                                LogPrintf("ERROR: %s - Orphaning stake, AbandonTransaction failed for %s\n", __func__, wtxid.ToString());
                        };
                    };
                };
            };
        };

        LogPrintf("nUnabandonedOrphans %d\n", nUnabandonedOrphans);
        LogPrintf("nCoinStakes %d\n", nCoinStakes);
        LogPrintf("nAbandonedOrphans %d\n", nAbandonedOrphans);
        LogPrintf("nMapWallet %d\n", nMapWallet);
        result.pushKV("unabandoned_orphans", (int)nUnabandonedOrphans);

        // Check for gaps in the hd key chains
        ExtKeyAccountMap::const_iterator itam = pwallet->mapExtAccounts.begin();
        for ( ; itam != pwallet->mapExtAccounts.end(); ++itam)
        {
            CExtKeyAccount *sea = itam->second;
            LogPrintf("Checking account %s\n", sea->GetIDString58());
            for (CStoredExtKey *sek : sea->vExtKeys)
            {
                if (!(sek->nFlags & EAF_ACTIVE)
                    || !(sek->nFlags & EAF_RECEIVE_ON))
                    continue;

                UniValue rva(UniValue::VARR);
                LogPrintf("Checking chain %s\n", sek->GetIDString58());
                uint32_t nGenerated = sek->GetCounter(false);
                LogPrintf("Generated %d\n", nGenerated);

                bool fHardened = false;
                CPubKey newKey;

                for (uint32_t i = 0; i < nGenerated; ++i)
                {
                    uint32_t nChildOut;
                    if (0 != sek->DeriveKey(newKey, i, nChildOut, fHardened))
                        throw JSONRPCError(RPC_WALLET_ERROR, "DeriveKey failed.");

                    if (i != nChildOut)
                        LogPrintf("Warning: %s - DeriveKey skipped key %d, %d.\n", __func__, i, nChildOut);

                    CEKAKey ak;
                    CKeyID idk = newKey.GetID();
                    CPubKey pk;
                    if (!sea->GetPubKey(idk, pk))
                    {
                        UniValue tmp(UniValue::VOBJ);
                        tmp.pushKV("position", (int)i);
                        tmp.pushKV("address", CBitcoinAddress(idk).ToString());

                        if (fAttemptRepair)
                        {
                            uint32_t nChain;
                            if (!sea->GetChainNum(sek, nChain))
                                throw JSONRPCError(RPC_WALLET_ERROR, "GetChainNum failed.");

                            CEKAKey ak(nChain, nChildOut);
                            if (0 != pwallet->ExtKeySaveKey(sea, idk, ak))
                                throw JSONRPCError(RPC_WALLET_ERROR, "ExtKeySaveKey failed.");

                            UniValue b;
                            b.setBool(true);
                            tmp.pushKV("attempt_fix", b);
                        };

                        rva.push_back(tmp);
                    };
                };

                if (rva.size() > 0)
                {
                    UniValue tmp(UniValue::VOBJ);
                    tmp.pushKV("account", sea->GetIDString58());
                    tmp.pushKV("chain", sek->GetIDString58());
                    tmp.pushKV("missing_keys", rva);
                    result.pushKV("error", tmp);
                };

                // TODO: Check hardened keys, must detect stealth key chain
            };
        };
    }

    return result;
};

UniValue rewindchain(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "rewindchain [height]\n"
            "height default - last known rct index .\n");

    EnsureWalletIsUnlocked(pwallet);

    LOCK2(cs_main, pwallet->cs_wallet);

    const CChainParams& chainparams = Params();

    UniValue result(UniValue::VOBJ);

    CCoinsViewCache view(pcoinsTip);
    CBlockIndex* pindexState = chainActive.Tip();
    CValidationState state;

    int nBlocks = 0;
    result.pushKV("start_height", pindexState->nHeight);

    int nLastRCTCheckpointHeight = ((pindexState->nHeight-1) / 250) * 250;

    int64_t nLastRCTOutput = 0;

    pblocktree->ReadRCTOutputCheckpoint(nLastRCTCheckpointHeight, nLastRCTOutput);

    result.pushKV("rct_checkpoint_height", nLastRCTCheckpointHeight);
    result.pushKV("last_rct_output", (int)nLastRCTOutput);

    std::set<CCmpPubKey> setKi; // unused
    if (!RollBackRCTIndex(nLastRCTOutput, setKi))
        throw JSONRPCError(RPC_MISC_ERROR, "RollBackRCTIndex failed.");

    for (CBlockIndex *pindex = chainActive.Tip(); pindex && pindex->pprev; pindex = pindex->pprev)
    {
        if (pindex->nHeight <= nLastRCTCheckpointHeight)
            break;

        nBlocks++;

        CBlock block;
        if (!ReadBlockFromDisk(block, pindex, chainparams.GetConsensus()))
        {
            result.pushKV("ReadBlockFromDisk failed", pindex->GetBlockHash().ToString());
            break;
        };
        if (DISCONNECT_OK != DisconnectBlock(block, pindex, view))
        {
            result.pushKV("DisconnectBlock failed", pindex->GetBlockHash().ToString());
            break;
        };
        if (!FlushView(&view, state, true))
        {
            result.pushKV("FlushView failed", pindex->GetBlockHash().ToString());
            break;
        };

        if (!FlushStateToDisk(Params(), state, FLUSH_STATE_IF_NEEDED))
            return false;

        // Update chainActive and related variables.
        UpdateTip(pindex->pprev, chainparams);
    };

    result.pushKV("nBlocks", nBlocks);


    return result;
};

UniValue walletsettings(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "walletsettings \"setting\" json\n"
            "\nManage wallet settings.\n"
            "\nchangeaddress {\"address_standard\":,\"coldstakingaddress\":}.\n"
            "   - \"address_standard\": Change address for standard inputs.\n"
            "   - \"coldstakingaddress\": Cold staking address for standard inputs.\n"
            "\nstakingoptions {\"stakecombinethreshold\":str,\"stakesplitthreshold\":str,\"foundationdonationpercent\":int,\"rewardaddress\":str}.\n"
            "   - \"stakecombinethreshold\": Join outputs below this value.\n"
            "   - \"stakesplitthreshold\": Split outputs above this value.\n"
            "   - \"foundationdonationpercent\": .\n"
            "\nstakelimit {\"height\":int}.\n"
            "   Don't stake above height, used in functional testing.\n"
            "\nUse an empty json object to clear the setting."
            "\nstakelimit {}.\n"

            "\nExamples\n"
            "Set coldstaking changeaddress extended public key:\n"
            + HelpExampleCli("walletsettings", "changeaddress \"{\\\"coldstakingaddress\\\":\\\"extpubkey\\\"}\"") + "\n"
            "Clear changeaddress settings\n"
            + HelpExampleCli("walletsettings", "changeaddress \"{}\"") + "\n"
        );

    EnsureWalletIsUnlocked(pwallet);

    UniValue result(UniValue::VOBJ);

    std::string sSetting = request.params[0].get_str();

    if (sSetting == "changeaddress")
    {
        UniValue json;
        UniValue warnings(UniValue::VARR);

        if (request.params.size() == 1)
        {
            if (!pwallet->GetSetting("changeaddress", json))
            {
                result.pushKV(sSetting, "default");
            } else
            {
                result.pushKV(sSetting, json);
            };
            return result;
        };

        if (request.params[1].isObject())
        {
            json = request.params[1].get_obj();

            const std::vector<std::string> &vKeys = json.getKeys();
            if (vKeys.size() < 1)
            {
                if (!pwallet->EraseSetting(sSetting))
                    throw JSONRPCError(RPC_WALLET_ERROR, _("EraseSetting failed."));
                result.pushKV(sSetting, "cleared");
                return result;
            };

            for (const auto &sKey : vKeys)
            {
                if (sKey == "address_standard")
                {
                    if (!json["address_standard"].isStr())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("address_standard must be a string."));

                    std::string sAddress = json["address_standard"].get_str();
                    CBitcoinAddress addr(sAddress);
                    if (!addr.IsValid())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid address_standard.");
                } else
                if (sKey == "coldstakingaddress")
                {
                    if (!json["coldstakingaddress"].isStr())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("coldstakingaddress must be a string."));

                    std::string sAddress = json["coldstakingaddress"].get_str();
                    CBitcoinAddress addr(sAddress);
                    if (!addr.IsValid())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Invalid coldstakingaddress."));
                    if (addr.IsValidStealthAddress())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("coldstakingaddress can't be a stealthaddress."));

                    // TODO: override option?
                    if (pwallet->HaveAddress(addr))
                        throw JSONRPCError(RPC_INVALID_PARAMETER, sAddress + _(" is spendable from this wallet."));

                    const Consensus::Params& consensusParams = Params().GetConsensus();
                    if (GetAdjustedTime() < consensusParams.OpIsCoinstakeTime)
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("OpIsCoinstake is not active yet."));
                } else
                {
                    warnings.push_back("Unknown key " + sKey);
                };
            };

            json.pushKV("time", GetTime());
            if (!pwallet->SetSetting(sSetting, json))
                throw JSONRPCError(RPC_WALLET_ERROR, _("SetSetting failed."));

            if (warnings.size() > 0)
                result.pushKV("warnings", warnings);
        } else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Must be json object."));
        };
        result.pushKV(sSetting, json);
    } else
    if (sSetting == "stakingoptions")
    {
        UniValue json;
        UniValue warnings(UniValue::VARR);

        if (request.params.size() == 1)
        {
            if (!pwallet->GetSetting("stakingoptions", json))
                result.pushKV(sSetting, "default");
            else
                result.pushKV(sSetting, json);
            return result;
        };

        if (request.params[1].isObject())
        {
            json = request.params[1].get_obj();

            const std::vector<std::string> &vKeys = json.getKeys();
            if (vKeys.size() < 1)
            {
                if (!pwallet->EraseSetting(sSetting))
                    throw JSONRPCError(RPC_WALLET_ERROR, _("EraseSetting failed."));
                result.pushKV(sSetting, "cleared");
                return result;
            };

            UniValue jsonOld;
            bool fHaveOldSetting = pwallet->GetSetting(sSetting, jsonOld);

            for (const auto &sKey : vKeys)
            {
                if (sKey == "stakecombinethreshold")
                {
                    CAmount test = AmountFromValue(json["stakecombinethreshold"]);
                    if (test < 0)
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("stakecombinethreshold can't be negative."));
                } else
                if (sKey == "stakesplitthreshold")
                {
                    CAmount test = AmountFromValue(json["stakesplitthreshold"]);
                    if (test < 0)
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("stakesplitthreshold can't be negative."));
                } else
                if (sKey == "foundationdonationpercent")
                {
                    if (!json["foundationdonationpercent"].isNum())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("foundationdonationpercent must be a number."));
                } else
                if (sKey == "rewardaddress")
                {
                    if (!json["rewardaddress"].isStr())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("rewardaddress must be a string."));

                    CBitcoinAddress addr(json["rewardaddress"].get_str());
                    if (!addr.IsValid())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Invalid rewardaddress."));
                    if (addr.IsValidStealthAddress())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("rewardaddress can't be a stealthaddress."));
                } else
                {
                    warnings.push_back("Unknown key " + sKey);
                };
            };

            json.pushKV("time", GetTime());
            if (!pwallet->SetSetting(sSetting, json))
                throw JSONRPCError(RPC_WALLET_ERROR, _("SetSetting failed."));

            std::string sError;
            pwallet->ProcessStakingSettings(sError);
            if (!sError.empty())
            {
                result.pushKV("error", sError);
                if (fHaveOldSetting)
                    pwallet->SetSetting(sSetting, jsonOld);
            };

            if (warnings.size() > 0)
                result.pushKV("warnings", warnings);
        } else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Must be json object."));
        };
        result.pushKV(sSetting, json);
    } else
    if (sSetting == "stakelimit")
    {
        UniValue json;
        UniValue warnings(UniValue::VARR);

        if (request.params.size() == 1)
        {
            result.pushKV(sSetting, pwallet->nStakeLimitHeight);
            return result;
        };

        if (request.params[1].isObject())
        {
            json = request.params[1].get_obj();

            const std::vector<std::string> &vKeys = json.getKeys();
            if (vKeys.size() < 1)
            {
                pwallet->nStakeLimitHeight = 0;
                result.pushKV(sSetting, "cleared");
                return result;
            };

            for (const auto &sKey : vKeys)
            {
                if (sKey == "height")
                {
                    if (!json["height"].isNum())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("height must be a number."));

                    pwallet->nStakeLimitHeight = json["height"].get_int();
                    result.pushKV(sSetting, pwallet->nStakeLimitHeight);
                } else
                {
                    warnings.push_back("Unknown key " + sKey);
                };
            };

            if (warnings.size() > 0)
                result.pushKV("warnings", warnings);
        } else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("Must be json object."));
        };

        WakeThreadStakeMiner(pwallet);
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown setting"));
    };

    return result;
};


UniValue setvote(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 4)
        throw std::runtime_error(
            "setvote <proposal> <option> <height_start> <height_end>\n"
            "Set voting token.\n"
            "Proposal is the proposal to vote on.\n"
            "Option is the option to vote for.\n"
            "The last added option valid for a range will be applied.\n"
            "Wallet will include this token in staked blocks from height_start to height_end.\n"
            "Set proposal and/or option to 0 to stop voting.\n");

    EnsureWalletIsUnlocked(pwallet);

    uint32_t issue = request.params[0].get_int();
    uint32_t option = request.params[1].get_int();

    if (issue > 0xFFFF)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Proposal out of range."));
    if (option > 0xFFFF)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Option out of range."));

    int nStartHeight = request.params[2].get_int();
    int nEndHeight = request.params[3].get_int();

    if (nEndHeight < nStartHeight)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("height_end must be after height_start."));

    uint32_t voteToken = issue | (option << 16);

    {
        LOCK(pwallet->cs_wallet);

        CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");

        std::vector<CVoteToken> vVoteTokens;

        wdb.ReadVoteTokens(vVoteTokens);

        CVoteToken v(voteToken, nStartHeight, nEndHeight, GetTime());
        vVoteTokens.push_back(v);

        if (!wdb.WriteVoteTokens(vVoteTokens))
            throw JSONRPCError(RPC_WALLET_ERROR, "WriteVoteTokens failed.");

        pwallet->LoadVoteTokens(&wdb);
    }

    UniValue result(UniValue::VOBJ);

    if (issue < 1)
        result.pushKV("result", _("Cleared vote token."));
    else
        result.pushKV("result", strprintf(_("Voting for option %u on proposal %u"), option, issue));

    result.pushKV("from_height", nStartHeight);
    result.pushKV("to_height", nEndHeight);

    return result;
}

UniValue votehistory(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "votehistory [current_only]\n"
            "Display voting history.\n");

    UniValue result(UniValue::VARR);

    if (request.params.size() > 0)
    {
        std::string s = request.params[0].get_str();
        if (part::IsStringBoolPositive(s))
        {
            UniValue vote(UniValue::VOBJ);

            int nNextHeight = chainActive.Height() + 1;

            for (auto i = pwallet->vVoteTokens.size(); i-- > 0; )
            {
                auto &v = pwallet->vVoteTokens[i];
                if (v.nEnd < nNextHeight
                    || v.nStart > nNextHeight)
                    continue;

                if ((v.nToken >> 16) < 1
                    || (v.nToken & 0xFFFF) < 1)
                    continue;
                UniValue vote(UniValue::VOBJ);
                vote.pushKV("proposal", (int)(v.nToken & 0xFFFF));
                vote.pushKV("option", (int)(v.nToken >> 16));
                vote.pushKV("from_height", v.nStart);
                vote.pushKV("to_height", v.nEnd);
                result.push_back(vote);
            };
            return result;
        };
    };

    std::vector<CVoteToken> vVoteTokens;
    {
        LOCK(pwallet->cs_wallet);

        CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
        wdb.ReadVoteTokens(vVoteTokens);
    }

    for (auto i = vVoteTokens.size(); i-- > 0; )
    {
        auto &v = vVoteTokens[i];
        UniValue vote(UniValue::VOBJ);
        vote.pushKV("proposal", (int)(v.nToken & 0xFFFF));
        vote.pushKV("option", (int)(v.nToken >> 16));
        vote.pushKV("from_height", v.nStart);
        vote.pushKV("to_height", v.nEnd);
        vote.pushKV("added", v.nTimeAdded);
        result.push_back(vote);
    };

    return result;
}

UniValue tallyvotes(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            "tallyvotes <proposal> <height_start> <height_end>\n"
            "count votes.\n");

    int issue = request.params[0].get_int();
    if (issue < 1 || issue >= (1 << 16))
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Proposal out of range."));

    int nStartHeight = request.params[1].get_int();
    int nEndHeight = request.params[2].get_int();

    CBlock block;
    const Consensus::Params& consensusParams = Params().GetConsensus();

    std::map<int, int> mapVotes;
    std::pair<std::map<int, int>::iterator, bool> ri;

    int nBlocks = 0;
    CBlockIndex *pindex = chainActive.Tip();
    if (pindex)
    do
    {
        if (pindex->nHeight < nStartHeight)
            break;
        if (pindex->nHeight <= nEndHeight)
        {
            if (!ReadBlockFromDisk(block, pindex, consensusParams))
                continue;

            if (block.vtx.size() < 1
                || !block.vtx[0]->IsCoinStake())
                continue;

            std::vector<uint8_t> &vData = ((CTxOutData*)block.vtx[0]->vpout[0].get())->vData;
            if (vData.size() < 9 || vData[4] != DO_VOTE)
            {
                ri = mapVotes.insert(std::pair<int, int>(0, 1));
                if (!ri.second) ri.first->second++;
            } else
            {
                uint32_t voteToken;
                memcpy(&voteToken, &vData[5], 4);
                int option = 0; // default to abstain

                // count only if related to current issue:
                if ((int) (voteToken & 0xFFFF) == issue)
                    option = (voteToken >> 16) & 0xFFFF;

                ri = mapVotes.insert(std::pair<int, int>(option, 1));
                if (!ri.second) ri.first->second++;
            };

            nBlocks++;
        };
    } while ((pindex = pindex->pprev));

    UniValue result(UniValue::VOBJ);
    result.pushKV("proposal", issue);
    result.pushKV("height_start", nStartHeight);
    result.pushKV("height_end", nEndHeight);
    result.pushKV("blocks_counted", nBlocks);

    float fnBlocks = (float) nBlocks;
    for (auto &i : mapVotes)
    {
        std::string sKey = i.first == 0 ? "Abstain" : strprintf("Option %d", i.first);
        result.pushKV(sKey, strprintf("%d, %.02f%%", i.second, ((float) i.second / fnBlocks) * 100.0));
    };

    return result;
};


static const CRPCCommand commands[] =
{ //  category              name                        actor (function)           okSafeMode
  //  --------------------- ------------------------    -----------------------    ----------
    { "wallet",             "extkey",                   &extkey,                   false,  {} },
    { "wallet",             "extkeyimportmaster",       &extkeyimportmaster,       false,  {"source","passphrase","save_bip44_root","master_label","account_label"} }, // import, set as master, derive account, set default account, force users to run mnemonic new first make them copy the key
    { "wallet",             "extkeygenesisimport",      &extkeygenesisimport,      false,  {"source","passphrase","save_bip44_root","master_label","account_label"} },
    { "wallet",             "keyinfo",                  &keyinfo,                  false,  {"key","show_secret"} },
    { "wallet",             "extkeyaltversion",         &extkeyaltversion,         false,  {"ext_key"} },
    { "wallet",             "getnewextaddress",         &getnewextaddress,         false,  {"label","childNo"} },
    { "wallet",             "getnewstealthaddress",     &getnewstealthaddress,     false,  {"label","num_prefix_bits","prefix_num","bech32"} },
    { "wallet",             "importstealthaddress",     &importstealthaddress,     false,  {"scan_secret","spend_secret","label","num_prefix_bits","prefix_num"} },
    { "wallet",             "liststealthaddresses",     &liststealthaddresses,     false,  {"show_secrets"} },

    { "wallet",             "scanchain",                &scanchain,                false,  {"fromHeight"} },
    { "wallet",             "reservebalance",           &reservebalance,           false,  {"enabled","amount"} },
    { "wallet",             "deriverangekeys",          &deriverangekeys,          false,  {"start", "end", "key/id", "hardened", "save", "add_to_addressbook", "256bithash"} },
    { "wallet",             "clearwallettransactions",  &clearwallettransactions,  false,  {"remove_all"} },

    { "wallet",             "filtertransactions",       &filtertransactions,       false,  {"options"} },
    { "wallet",             "filteraddresses",          &filteraddresses,          false,  {"offset","count","sort_code"} },
    { "wallet",             "manageaddressbook",        &manageaddressbook,        true,   {"action","address","label","purpose"} },

    { "wallet",             "getstakinginfo",           &getstakinginfo,           true,   {} },
    { "wallet",             "getcoldstakinginfo",       &getcoldstakinginfo,       true,   {} },

    //{ "wallet",             "gettransactionsummary",    &gettransactionsummary,    true,  {} },

    { "wallet",             "listunspentanon",          &listunspentanon,          true,   {"minconf","maxconf","addresses","include_unsafe","query_options"} },
    { "wallet",             "listunspentblind",         &listunspentblind,         true,   {"minconf","maxconf","addresses","include_unsafe","query_options"} },


    //sendparttopart // normal txn
    { "wallet",             "sendparttoblind",          &sendparttoblind,          false,  {"address","amount","comment","comment_to","subtractfeefromamount", "narration"} },
    { "wallet",             "sendparttoanon",           &sendparttoanon,           false,  {"address","amount","comment","comment_to","subtractfeefromamount", "narration"} },

    { "wallet",             "sendblindtopart",          &sendblindtopart,          false,  {"address","amount","comment","comment_to","subtractfeefromamount", "narration"} },
    { "wallet",             "sendblindtoblind",         &sendblindtoblind,         false,  {"address","amount","comment","comment_to","subtractfeefromamount", "narration"} },
    { "wallet",             "sendblindtoanon",          &sendblindtoanon,          false,  {"address","amount","comment","comment_to","subtractfeefromamount", "narration"} },

    { "wallet",             "sendanontopart",           &sendanontopart,           false,  {"address","amount","comment","comment_to","subtractfeefromamount", "narration", "ring_size", "inputs_per_sig"} },
    { "wallet",             "sendanontoblind",          &sendanontoblind,          false,  {"address","amount","comment","comment_to","subtractfeefromamount", "narration", "ring_size", "inputs_per_sig"} },
    { "wallet",             "sendanontoanon",           &sendanontoanon,           false,  {"address","amount","comment","comment_to","subtractfeefromamount", "narration", "ring_size", "inputs_per_sig"} },

    { "wallet",             "sendtypeto",               &sendtypeto,               false,  {"typein", "typeout", "outputs","comment","comment_to", "ring_size", "inputs_per_sig", "test_fee"} },

    { "wallet",             "buildscript",              &buildscript,              false,  {"json"} },

    { "wallet",             "debugwallet",              &debugwallet,              false,  {"attempt_repair"} },
    { "wallet",             "rewindchain",              &rewindchain,              false,  {"height"} },

    { "wallet",             "walletsettings",           &walletsettings,           true,   {"setting","json"} },


    { "governance",         "setvote",                  &setvote,                  false,  {"proposal","option","height_start","height_end"} },
    { "governance",         "votehistory",              &votehistory,              false,  {"current_only"} },
    { "governance",         "tallyvotes",               &tallyvotes,               false,  {"proposal","height_start","height_end"} },
};

void RegisterHDWalletRPCCommands(CRPCTable &t)
{
    if (gArgs.GetBoolArg("-disablewallet", false))
        return;

    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
