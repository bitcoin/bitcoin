// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <base58.h>
#include <chain.h>
#include <consensus/validation.h>
#include <consensus/tx_verify.h>
#include <core_io.h>
#include <init.h>
#include <httpserver.h>
#include <validation.h>
#include <net.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <rpc/server.h>
#include <rpc/mining.h>
#include <rpc/util.h>
#include <script/sign.h>
#include <timedata.h>
#include <util.h>
#include <txdb.h>
#include <blind.h>
#include <anon.h>
#include <utilmoneystr.h>
#include <wallet/hdwallet.h>
#include <wallet/hdwalletdb.h>
#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>
#include <chainparams.h>
#include <key/mnemonic.h>
#include <pos/miner.h>
#include <crypto/sha256.h>
#include <warnings.h>

#include <univalue.h>
#include <stdint.h>



static void EnsureWalletIsUnlocked(CHDWallet *pwallet)
{
    if (pwallet->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet locked, please enter the wallet passphrase with walletpassphrase first.");

    if (pwallet->fUnlockForStakingOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet is unlocked for staking only.");
};

static const std::string WALLET_ENDPOINT_BASE = "/wallet/";

static inline uint32_t reversePlace(const uint8_t *p)
{
    uint32_t rv = 0;
    for (int i = 0; i < 4; ++i)
        rv |= (uint32_t) *(p+i) << (8 * (3-i));
    return rv;
};

static int ExtractBip32InfoV(const std::vector<uint8_t> &vchKey, UniValue &keyInfo, std::string &sError)
{
    CExtKey58 ek58;
    CExtKeyPair vk;
    vk.DecodeV(&vchKey[4]);

    CChainParams::Base58Type typePk = CChainParams::EXT_PUBLIC_KEY;
    if (memcmp(&vchKey[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY)[0], 4) == 0)
    {
        keyInfo.pushKV("type", "Particl extended secret key");
    } else
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

static int ExtractBip32InfoP(const std::vector<uint8_t> &vchKey, UniValue &keyInfo, std::string &sError)
{
    CExtPubKey pk;

    if (memcmp(&vchKey[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY)[0], 4) == 0)
    {
        keyInfo.pushKV("type", "Particl extended public key");
    } else
    if (memcmp(&vchKey[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY_BTC)[0], 4) == 0)
    {
        keyInfo.pushKV("type", "Bitcoin extended public key");
    } else
    {
        keyInfo.pushKV("type", "Unknown extended public key");
    };

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

static int ExtKeyPathV(const std::string &sPath, const std::vector<uint8_t> &vchKey, UniValue &keyInfo, std::string &sError)
{
    if (sPath.compare("info") == 0)
        return ExtractBip32InfoV(vchKey, keyInfo, sError);

    CExtKey vk;
    vk.Decode(&vchKey[4]);
    CExtKey vkOut, vkWork = vk;

    std::vector<uint32_t> vPath;
    int rv;
    if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
        return errorN(1, sError, __func__, "ExtractExtKeyPath failed %s", ExtKeyGetString(rv));

    for (std::vector<uint32_t>::iterator it = vPath.begin(); it != vPath.end(); ++it)
    {
        if (!vkWork.Derive(vkOut, *it))
            return errorN(1, sError, __func__, "CExtKey Derive failed");
        vkWork = vkOut;
    };

    CBitcoinExtKey ekOut;
    ekOut.SetKey(vkOut);
    keyInfo.pushKV("result", ekOut.ToString());

    // Display path, the quotes can go missing through the debug console. eg: m/44'/1', m/44\'/1\' works
    std::string sPathOut;
    if (0 != PathToString(vPath, sPathOut))
        return errorN(1, sError, __func__, "PathToString failed");
    keyInfo.pushKV("path", sPathOut);

    return 0;
};

static int ExtKeyPathP(const std::string &sPath, const std::vector<uint8_t> &vchKey, UniValue &keyInfo, std::string &sError)
{
    if (sPath.compare("info") == 0)
        return ExtractBip32InfoP(vchKey, keyInfo, sError);

    CExtPubKey pk;
    pk.Decode(&vchKey[4]);

    CExtPubKey pkOut, pkWork = pk;

    std::vector<uint32_t> vPath;
    int rv;
    if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
        return errorN(1, sError, __func__, "ExtractExtKeyPath failed %s", ExtKeyGetString(rv));

    for (std::vector<uint32_t>::iterator it = vPath.begin(); it != vPath.end(); ++it)
    {
        if ((*it >> 31) == 1)
            return errorN(1, sError, __func__, "Can't derive hardened keys from public ext key");
        if (!pkWork.Derive(pkOut, *it))
            return errorN(1, sError, __func__, "CExtKey Derive failed");
        pkWork = pkOut;
    };

    CBitcoinExtPubKey ekOut;
    ekOut.SetKey(pkOut);
    keyInfo.pushKV("result", ekOut.ToString());

    // Display path, the quotes can go missing through the debug console. eg: m/44'/1', m/44\'/1\' works
    std::string sPathOut;
    if (0 != PathToString(vPath, sPathOut))
        return errorN(1, sError, __func__, "PathToString failed");
    keyInfo.pushKV("path", sPathOut);

    return 0;
};

static int AccountInfo(CHDWallet *pwallet, CExtKeyAccount *pa, int nShowKeys, bool fAllChains, UniValue &obj, std::string &sError)
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

    mvi = pa->mapValue.find(EKVT_HARDWARE_DEVICE);
    if (mvi != pa->mapValue.end())
    {
#if ENABLE_USBDEVICE

#endif
        if (mvi->second.size() >= 8)
        {
            int nVendorId = *((int*)mvi->second.data());
            int nProductId = *((int*)(mvi->second.data() + 4));
            obj.pushKV("hardware_device", strprintf("0x%04x 0x%04x", nVendorId, nProductId));
        };
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
                std::string sUseType;
                switch (it->second[0])
                {
                    case EKT_EXTERNAL:      sUseType = "external";      break;
                    case EKT_INTERNAL:      sUseType = "internal";      break;
                    case EKT_STEALTH:       sUseType = "stealth";       break;
                    case EKT_CONFIDENTIAL:  sUseType = "confidential";  break;
                    case EKT_STEALTH_SCAN:  sUseType = "stealth_scan";  break;
                    case EKT_STEALTH_SPEND: sUseType = "stealth_spend"; break;
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

static int AccountInfo(CHDWallet *pwallet, CKeyID &keyId, int nShowKeys, bool fAllChains, UniValue &obj, std::string &sError)
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

static int KeyInfo(CHDWallet *pwallet, CKeyID &idMaster, CKeyID &idKey, CStoredExtKey &sek, int nShowKeys, UniValue &obj, std::string &sError)
{
    CExtKey58 eKey58;

    bool fBip44Root = false;
    obj.pushKV("type", "Loose");
    obj.pushKV("active", sek.nFlags & EAF_ACTIVE ? "true" : "false");
    obj.pushKV("receive_on", sek.nFlags & EAF_RECEIVE_ON ? "true" : "false");
    obj.pushKV("encrypted", sek.nFlags & EAF_IS_CRYPTED ? "true" : "false");
    obj.pushKV("hardware_device", sek.nFlags & EAF_HARDWARE_DEVICE ? "true" : "false");
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

static int KeyInfo(CHDWallet *pwallet, CKeyID &idMaster, CKeyID &idKey, int nShowKeys, UniValue &obj, std::string &sError)
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

static int ManageExtKey(CStoredExtKey &sek, std::string &sOptName, std::string &sOptValue, UniValue &result, std::string &sError)
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
            if (!ParseUInt64(sOptValue, &nLookAhead))
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

static int ManageExtAccount(CExtKeyAccount &sea, std::string &sOptName, std::string &sOptValue, UniValue &result, std::string &sError)
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
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid key.");
    };

    return 0;
};

static UniValue extkey(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    static const char *help = ""
        "extkey \"mode\"\n"
        "\"mode\" can be: info|list|account|gen|import|importAccount|setMaster|setDefaultAccount|deriveAccount|options\n"
        "    Default: list, or info when called like: extkey \"key\"\n"
        "\n"
        "extkey info \"key\" ( \"path\" )\n"
        "    Return info for provided \"key\" or key at \"path\" from \"key\".\n"
        "extkey list ( show_secrets )\n"
        "    List loose and account ext keys.\n"
        "extkey account ( \"key/id\" show_secrets )\n"
        "    Display details of account.\n"
        "    Show default account when called without parameters.\n"
        "extkey key \"key/id\" ( show_secrets )\n"
        "    Display details of loose extkey in wallet.\n"
        "extkey gen \"passphrase\" ( numhashes \"seedstring\" )\n"
        "    DEPRECATED\n"
        "    If no passhrase is specified key will be generated from random data.\n"
        "    Warning: It is not recommended to use this feature.\n"
        "extkey import \"key\" ( \"label\" bip44 save_bip44_key )\n"
        "    Add loose key to wallet.\n"
        "    If bip44 is set import will add the key derived from <key> on the bip44 path.\n"
        "    If save_bip44_key is set import will save the bip44 key to the wallet.\n"
        "extkey importAccount \"key\" ( time_scan_from \"label\" ) \n"
        "    Add account key to wallet.\n"
        "        time_scan_from: N no check, Y-m-d date to start scanning the blockchain for owned txns.\n"
        "extkey setMaster \"key/id\"\n"
        "    Set a private ext key as current master key.\n"
        "    key can be a extkeyid or full key, but must be in the wallet.\n"
        "extkey setDefaultAccount \"id\"\n"
        "    Set an account as the default.\n"
        "extkey deriveAccount ( \"label\" \"path\" )\n"
        "    Make a new account from the current master key, save to wallet.\n"
        "extkey options \"key\" ( \"optionName\" \"newValue\" )\n"
        "    Manage keys and accounts.\n"
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
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "DecodeBase58 failed.");
        if (!VerifyChecksum(vchOut))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "VerifyChecksum failed.");

        size_t keyLen = vchOut.size();
        std::string sError;

        if (keyLen != BIP32_KEY_LEN)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Unknown ext key length '%d'", keyLen));

        if (memcmp(&vchOut[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY)[0], 4) == 0
            || memcmp(&vchOut[0], &Params().Base58Prefix(CChainParams::EXT_SECRET_KEY_BTC)[0], 4) == 0)
        {
            if (ExtKeyPathV(sMode, vchOut, keyInfo, sError) != 0)
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("ExtKeyPathV failed %s.", sError.c_str()));
        } else
        if (memcmp(&vchOut[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY)[0], 4) == 0
            || memcmp(&vchOut[0], &Params().Base58Prefix(CChainParams::EXT_PUBLIC_KEY_BTC)[0], 4) == 0)
        {
            if (ExtKeyPathP(sMode, vchOut, keyInfo, sError) != 0)
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("ExtKeyPathP failed %s.", sError.c_str()));
        } else
        {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Unknown prefix '%s'", sInKey.substr(0, 4)));
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
        };
        if (keyId.IsNull())
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Must specify ext key or id %s.", mode == "account" ? "or 'default'" : ""));

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
                throw JSONRPCError(RPC_MISC_ERROR, "AccountInfo failed: " + sError);
        } else
        {
            CKeyID idMaster;
            if (pwallet->pEKMaster)
                idMaster = pwallet->pEKMaster->GetID();
            else
                LogPrintf("%s: Warning: Master key isn't set!\n", __func__);
            if (0 != KeyInfo(pwallet, idMaster, keyId, nListFull, result, sError))
                throw JSONRPCError(RPC_MISC_ERROR, "KeyInfo failed: " + sError);
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
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid num hashes");

                if (nHashes < 1)
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Num hashes must be 1 or more.");
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
            fBip44 = GetBool(request.params[nParamOffset]);
            nParamOffset++;
        };

        bool fSaveBip44 = false;
        if (request.params.size() > nParamOffset)
        {
            fSaveBip44 = GetBool(request.params[nParamOffset]);
            nParamOffset++;
        };

        std::vector<uint8_t> v;
        sek.mapValue[EKVT_CREATED_AT] = SetCompressedInt64(v, GetTime());

        CExtKey58 eKey58;
        if (eKey58.Set58(sInKey.c_str()) != 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Import failed - Invalid key.");

        if (fBip44)
        {
            if (!eKey58.IsValid(CChainParams::EXT_SECRET_KEY_BTC))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Import failed - BIP44 key must begin with a bitcoin secret key prefix.");
        } else
        {
            if (!eKey58.IsValid(CChainParams::EXT_SECRET_KEY)
                && !eKey58.IsValid(CChainParams::EXT_PUBLIC_KEY_BTC))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Import failed - Key must begin with a particl prefix.");
        };

        sek.kp = eKey58.GetKey();

        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw JSONRPCError(RPC_MISC_ERROR, "TxnBegin failed.");

            int rv;
            CKeyID idDerived;
            if (0 != (rv = pwallet->ExtKeyImportLoose(&wdb, sek, idDerived, fBip44, fSaveBip44)))
            {
                wdb.TxnAbort();
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("ExtKeyImportLoose failed, %s", ExtKeyGetString(rv)));
            };

            if (!wdb.TxnCommit())
                throw JSONRPCError(RPC_MISC_ERROR, "TxnCommit failed.");

            CBitcoinAddress addr;
            addr.Set(fBip44 ? idDerived : sek.GetID(), CChainParams::EXT_KEY_HASH);
            result.pushKV("result", "Success.");
            result.pushKV("id", addr.ToString().c_str());
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
                if (sVar.length() && !ParseInt64(sVar, &nTimeStartScan))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Import Account failed - Parse time error.");
            } else
            {
                int year, month, day;

                if (sscanf(sVar.c_str(), "%d-%d-%d", &year, &month, &day) != 3)
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Import Account failed - Parse time error.");

                struct tm tmdate;
                tmdate.tm_year = year - 1900;
                tmdate.tm_mon = month - 1;
                tmdate.tm_mday = day;
                time_t t = mktime(&tmdate);

                nTimeStartScan = t;
            };
        };

        int64_t nCreatedAt = nTimeStartScan ? nTimeStartScan : GetTime();

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
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Import Account failed - Invalid key.");
        };

        {
            WalletRescanReserver reserver(pwallet);
            if (!reserver.reserve()) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
            }

            LOCK2(cs_main, pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw JSONRPCError(RPC_MISC_ERROR, "TxnBegin failed.");

            int rv = pwallet->ExtKeyImportAccount(&wdb, sek, nCreatedAt, sLabel);
            if (rv == 1)
            {
                wdb.TxnAbort();
                throw JSONRPCError(RPC_WALLET_ERROR, "Import failed - ExtKeyImportAccount failed.");
            } else
            if (rv == 2)
            {
                wdb.TxnAbort();
                throw JSONRPCError(RPC_WALLET_ERROR, "Import failed - account exists.");
            } else
            {
                if (!wdb.TxnCommit())
                    throw JSONRPCError(RPC_MISC_ERROR, "TxnCommit failed.");
                result.pushKV("result", "Success.");

                if (rv == 3)
                    result.pushKV("result", "secret added to existing account.");

                result.pushKV("account_label", sLabel);
                result.pushKV("scanned_from", nTimeStartScan);
                result.pushKV("note", "Please backup your wallet."); // TODO: check for child of existing key?
            };

            pwallet->RescanFromTime(nTimeStartScan, reserver, true /* update */);
            pwallet->MarkDirty();
            pwallet->ReacceptWalletTransactions();

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
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Must specify ext key or id.");
            };
        };

        CKeyID idNewMaster;
        ExtractExtKeyId(sInKey, idNewMaster, CChainParams::EXT_KEY_HASH);

        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw JSONRPCError(RPC_MISC_ERROR, "TxnBegin failed.");

            int rv;
            if (0 != (rv = pwallet->ExtKeySetMaster(&wdb, idNewMaster)))
            {
                wdb.TxnAbort();
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("ExtKeySetMaster failed, %s.", ExtKeyGetString(rv)));
            };
            if (!wdb.TxnCommit())
                throw JSONRPCError(RPC_MISC_ERROR, "TxnCommit failed.");
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
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Must specify ext key or id.");
            };
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
                throw JSONRPCError(RPC_MISC_ERROR, "TxnBegin failed.");
            };
            if (0 != (rv = pwallet->ExtKeySetDefaultAccount(&wdb, idNewDefault)))
            {
                delete sea;
                wdb.TxnAbort();
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("ExtKeySetDefaultAccount failed, %s.", ExtKeyGetString(rv)));
            };
            if (!wdb.TxnCommit())
            {
                delete sea;
                pwallet->idDefaultAccount = idOldDefault;
                throw JSONRPCError(RPC_MISC_ERROR, "TxnCommit failed.");
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

            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown parameter '%s'", strParam.c_str()));
        };

        CExtKeyAccount *sea = new CExtKeyAccount();

        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw JSONRPCError(RPC_MISC_ERROR, "TxnBegin failed.");

            int rv;
            if ((rv = pwallet->ExtKeyDeriveNewAccount(&wdb, sea, sLabel, sPath)) != 0)
            {
                wdb.TxnAbort();
                result.pushKV("result", "Failed.");
                result.pushKV("reason", ExtKeyGetString(rv));
            } else
            {
                if (!wdb.TxnCommit())
                    throw JSONRPCError(RPC_MISC_ERROR, "TxnCommit failed.");

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
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Must specify ext key or id.");
            };
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
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid key or account id.");

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
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid key or account id.");
        };

        CStoredExtKey sek;
        CExtKeyAccount sea;
        {
            LOCK(pwallet->cs_wallet);
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            if (!wdb.TxnBegin())
                throw JSONRPCError(RPC_MISC_ERROR, "TxnBegin failed.");

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
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Key not in wallet.");
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
                    throw JSONRPCError(RPC_MISC_ERROR, "WriteExtKey failed.");
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
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Account not in wallet.");
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
                    throw JSONRPCError(RPC_WALLET_ERROR,"WriteExtAccount failed.");
                };
            };

            if (sOptValue.length() == 0)
            {
                wdb.TxnAbort();
            } else
            {
                if (!wdb.TxnCommit())
                    throw JSONRPCError(RPC_MISC_ERROR, "TxnCommit failed.");
                result.pushKV("result", "Success.");
            };
        } // cs_wallet

    } else
    {
        throw std::runtime_error(help);
    };

    return result;
};

static UniValue extkeyimportinternal(const JSONRPCRequest &request, bool fGenesisChain)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    EnsureWalletIsUnlocked(pwallet);

    if (request.params.size() < 1)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify a private extkey or mnemonic phrase.");

    std::string sMnemonic = request.params[0].get_str();
    bool fSaveBip44Root = false;
    std::string sLblMaster = "Master Key";
    std::string sLblAccount = "Default Account";
    std::string sPassphrase = "";
    std::string sError;
    int64_t nScanFrom = 1;

    if (request.params.size() > 1)
        sPassphrase = request.params[1].get_str();

    if (request.params.size() > 2)
    {
        std::string s = request.params[2].get_str();

        if (!part::GetStringBool(s, fSaveBip44Root))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown argument for save_bip44_root: %s.", s.c_str()));
    };

    if (request.params.size() > 3)
        sLblMaster = request.params[3].get_str();
    if (request.params.size() > 4)
        sLblAccount = request.params[4].get_str();

    if (request.params[5].isStr())
    {
        std::string s = request.params[5].get_str();
        if (s.length() && !ParseInt64(s, &nScanFrom))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown argument for scan_chain_from: %s.", s.c_str()));
    } else
    if (request.params[5].isNum())
    {
        nScanFrom = request.params[5].get_int64();
    };
    if (request.params.size() > 6)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unknown parameter '%s'", request.params[6].get_str()));

    LogPrintf("Importing master key and account with labels '%s', '%s'.\n", sLblMaster.c_str(), sLblAccount.c_str());

    WalletRescanReserver reserver(pwallet);
    if (!reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    CExtKey58 eKey58;
    CExtKeyPair ekp;
    if (eKey58.Set58(sMnemonic.c_str()) == 0)
    {
        if (!eKey58.IsValid(CChainParams::EXT_SECRET_KEY)
            && !eKey58.IsValid(CChainParams::EXT_SECRET_KEY_BTC))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify a private extkey or mnemonic phrase.");

        // Key was provided directly
        ekp = eKey58.GetKey();
    } else
    {
        std::vector<uint8_t> vSeed, vEntropy;

        // First check the mnemonic is valid
        int nLanguage = -1;
        if (0 != MnemonicDecode(nLanguage, sMnemonic, vEntropy, sError))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("MnemonicDecode failed: %s", sError.c_str()));

        if (0 != MnemonicToSeed(sMnemonic, sPassphrase, vSeed))
            throw JSONRPCError(RPC_MISC_ERROR, "MnemonicToSeed failed.");

        ekp.SetSeed(&vSeed[0], vSeed.size());
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
            throw JSONRPCError(RPC_MISC_ERROR, "TxnBegin failed.");

        if (0 != (rv = pwallet->ExtKeyImportLoose(&wdb, sek, idDerived, fBip44, fSaveBip44Root)))
        {
            wdb.TxnAbort();
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("ExtKeyImportLoose failed, %s", ExtKeyGetString(rv)));
        };

        if (0 != (rv = pwallet->ExtKeySetMaster(&wdb, idDerived)))
        {
            wdb.TxnAbort();
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("ExtKeySetMaster failed, %s.", ExtKeyGetString(rv)));
        };

        sea = new CExtKeyAccount();
        if (0 != (rv = pwallet->ExtKeyDeriveNewAccount(&wdb, sea, sLblAccount)))
        {
            pwallet->ExtKeyRemoveAccountFromMapsAndFree(sea);
            wdb.TxnAbort();
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("ExtKeyDeriveNewAccount failed, %s.", ExtKeyGetString(rv)));
        };

        CKeyID idNewDefaultAccount = sea->GetID();
        CKeyID idOldDefault = pwallet->idDefaultAccount;

        if (0 != (rv = pwallet->ExtKeySetDefaultAccount(&wdb, idNewDefaultAccount)))
        {
            pwallet->ExtKeyRemoveAccountFromMapsAndFree(sea);
            wdb.TxnAbort();
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("ExtKeySetDefaultAccount failed, %s.", ExtKeyGetString(rv)));
        };

        if (fGenesisChain)
        {
            std::string genesisChainLabel = "Genesis Import";
            CStoredExtKey *sekGenesisChain = new CStoredExtKey();

            if (0 != (rv = pwallet->NewExtKeyFromAccount(&wdb, idNewDefaultAccount,
                genesisChainLabel, sekGenesisChain, nullptr, &CHAIN_NO_GENESIS)))
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
            throw JSONRPCError(RPC_MISC_ERROR, "TxnCommit failed.");
        };
    } // cs_wallet

    if (nScanFrom >= 0)
    {
        pwallet->RescanFromTime(nScanFrom, reserver, true);
        pwallet->MarkDirty();
        pwallet->ReacceptWalletTransactions();
    };

    UniValue warnings(UniValue::VARR);
    // Check for coldstaking outputs without coldstakingaddress set
    if (pwallet->CountColdstakeOutputs() > 0)
    {
        UniValue jsonSettings;
        if (!pwallet->GetSetting("changeaddress", jsonSettings)
            || !jsonSettings["coldstakingaddress"].isStr())
        {
            warnings.push_back("Wallet has coldstaking outputs. Please remember to set a coldstakingaddress.");
        };
    };

    CBitcoinAddress addr;
    addr.Set(idDerived, CChainParams::EXT_KEY_HASH);
    result.pushKV("result", "Success.");
    result.pushKV("master_id", addr.ToString());
    result.pushKV("master_label", sek.sLabel);

    result.pushKV("account_id", sea->GetIDString58());
    result.pushKV("account_label", sea->sLabel);

    result.pushKV("note", "Please backup your wallet.");

    if (warnings.size() > 0)
        result.pushKV("warnings", warnings);

    return result;
}

static UniValue extkeyimportmaster(const JSONRPCRequest &request)
{
    // Doesn't generate key, require users to run mnemonic new, more likely they'll save the phrase
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 6)
        throw std::runtime_error(
        "extkeyimportmaster \"mnemonic/key\" ( \"passphrase\" save_bip44_root master_label account_label scan_chain_from )\n"
        "Import master key from bip44 mnemonic root key and derive default account.\n"
        + HelpRequiringPassphrase(pwallet) +
        "\nArguments:\n"
        "1. \"mnemonic/key\"          (string, required) The mnemonic or root extended key.\n"
        "       Use '-stdin' to be prompted to enter a passphrase.\n"
        "       if mnemonic is blank, defaults to '-stdin'.\n"
        "2. \"passphrase\":           (string, optional) passphrase when importing mnemonic - default blank.\n"
        "       Use '-stdin' to be prompted to enter a passphrase.\n"
        "3. save_bip44_root:        (bool, optional) Save bip44 root key to wallet - default false.\n"
        "4. \"master_label\":         (string, optional) Label for master key - default 'Master Key'.\n"
        "5. \"account_label\":        (string, optional) Label for account - default 'Default Account'.\n"
        "6. scan_chain_from:        (int, optional) Scan for transactions in blocks after timestamp, negative number to skip (default=0).\n"
        "\nExamples:\n"
        + HelpExampleCli("extkeyimportmaster", "-stdin -stdin false \"label_master\" \"label_account\"")
        + HelpExampleCli("extkeyimportmaster", "\"word1 ... word24\" \"passphrase\" false \"label_master\" \"label_account\"")
        + HelpExampleRpc("extkeyimportmaster", "\"word1 ... word24\", \"passphrase\", false, \"label_master\", \"label_account\""));

    return extkeyimportinternal(request, false);
};

static UniValue extkeygenesisimport(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 6)
        throw std::runtime_error(
        "extkeygenesisimport \"mnemonic/key\" ( \"passphrase\" save_bip44_root master_label account_label scan_chain_from )\n"
        "Import master key from bip44 mnemonic root key and derive default account.\n"
        "Derives an extra chain from path 444444 to receive imported coin.\n"
        + HelpRequiringPassphrase(pwallet) +
        "\nArguments:\n"
        "1. \"mnemonic/key\"          (string, required) The mnemonic or root extended key.\n"
        "       Use '-stdin' to be prompted to enter a passphrase.\n"
        "       if mnemonic is blank, defaults to '-stdin'.\n"
        "2. \"passphrase\":           (string, optional) passphrase when importing mnemonic - default blank.\n"
        "       Use '-stdin' to be prompted to enter a passphrase.\n"
        "3. save_bip44_root:        (bool, optional) Save bip44 root key to wallet - default false.\n"
        "4. \"master_label\":         (string, optional) Label for master key - default 'Master Key'.\n"
        "5. \"account_label\":        (string, optional) Label for account - default 'Default Account'.\n"
        "6. scan_chain_from:        (int, optional) Scan for transactions in blocks after timestamp, negative number to skip (default=0).\n"
        "\nExamples:\n"
        + HelpExampleCli("extkeygenesisimport", "-stdin -stdin false \"label_master\" \"label_account\"")
        + HelpExampleCli("extkeygenesisimport", "\"word1 ... word24\" \"passphrase\" false \"label_master\" \"label_account\"")
        + HelpExampleRpc("extkeygenesisimport", "\"word1 ... word24\", \"passphrase\", false, \"label_master\", \"label_account\""));

    return extkeyimportinternal(request, true);
}

static UniValue extkeyaltversion(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "extkeyaltversion \"ext_key\"\n"
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


static UniValue getnewextaddress(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 4)
        throw std::runtime_error(
            "getnewextaddress ( \"label\" childNo bech32 hardened )\n"
            "Returns a new Particl ext address for receiving payments.\n"
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. \"label\"             (string, optional) If specified the key is added to the address book.\n"
            "2. \"childNo\"           (string, optional), If specified the account derive counter is not updated.\n"
            "3. bech32              (bool, optional, default=false) Use Bech32 encoding.\n"
            "4. hardened            (bool, optional, default=false) Derive a hardened key.\n"
            "\nResult:\n"
            "\"address\"              (string) The new particl extended address\n"
            "\nExamples:\n"
            + HelpExampleCli("getnewextaddress", "")
            + HelpExampleRpc("getnewextaddress", ""));

    EnsureWalletIsUnlocked(pwallet);

    uint32_t nChild = 0;
    uint32_t *pChild = nullptr;
    std::string strLabel;
    const char *pLabel = nullptr;
    if (request.params[0].isStr())
    {
        strLabel = request.params[0].get_str();
        if (strLabel.size() > 0)
            pLabel = strLabel.c_str();
    };

    if (request.params[1].isStr())
    {
        std::string s = request.params[1].get_str();
        if (!s.empty())
        {
            // TODO, make full path work
            std::vector<uint32_t> vPath;
            if (0 != ExtractExtKeyPath(s, vPath) || vPath.size() != 1)
                throw JSONRPCError(RPC_INVALID_PARAMETER, _("bad childNo."));
            nChild = vPath[0];
            pChild = &nChild;
        };
    };

    bool fBech32 = !request.params[2].isNull() ? request.params[2].get_bool() : false;
    bool fHardened = !request.params[3].isNull() ? request.params[3].get_bool() : false;

    CStoredExtKey *sek = new CStoredExtKey();
    if (0 != pwallet->NewExtKeyFromAccount(strLabel, sek, pLabel, pChild, fHardened, fBech32))
    {
        delete sek;
        throw JSONRPCError(RPC_WALLET_ERROR, _("NewExtKeyFromAccount failed."));
    };

    // CBitcoinAddress displays public key only
    return CBitcoinAddress(sek->kp, fBech32).ToString();
}

static UniValue getnewstealthaddress(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 5)
        throw std::runtime_error(
            "getnewstealthaddress ( \"label\" num_prefix_bits prefix_num bech32 makeV2 )\n"
            "Returns a new Particl stealth address for receiving payments."
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. \"label\"             (string, optional) If specified the key is added to the address book.\n"
            "2. num_prefix_bits     (int, optional) If specified and > 0, the stealth address is created with a prefix.\n"
            "3. prefix_num          (int, optional) If prefix_num is not specified the prefix will be selected deterministically.\n"
            "           prefix_num can be specified in base2, 10 or 16, for base 2 prefix_num must begin with 0b, 0x for base16.\n"
            "           A 32bit integer will be created from prefix_num and the least significant num_prefix_bits will become the prefix.\n"
            "           A stealth address created without a prefix will scan all incoming stealth transactions, irrespective of transaction prefixes.\n"
            "           Stealth addresses with prefixes will scan only incoming stealth transactions with a matching prefix.\n"
            "4. bech32              (bool, optional, default=false) Use Bech32 encoding.\n"
            "5. makeV2              (bool, optional, default=false) Generate an address from the same method used for hardware wallets.\n"
            "\nResult:\n"
            "\"address\"              (string) The new particl stealth address\n"
            "\nExamples:\n"
            + HelpExampleCli("getnewstealthaddress", "\"lblTestSxAddrPrefix\" 3 \"0b101\"")
            + HelpExampleRpc("getnewstealthaddress", "\"lblTestSxAddrPrefix\", 3, \"0b101\""));

    EnsureWalletIsUnlocked(pwallet);

    std::string sLabel;
    if (request.params.size() > 0)
        sLabel = request.params[0].get_str();

    uint32_t num_prefix_bits = 0;
    if (request.params.size() > 1)
    {
        std::string s = request.params[1].get_str();
        if (s.length() && !ParseUInt32(s, &num_prefix_bits))
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("num_prefix_bits invalid number."));
    };

    if (num_prefix_bits > 32)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("num_prefix_bits must be <= 32."));

    std::string sPrefix_num;
    if (request.params.size() > 2)
        sPrefix_num = request.params[2].get_str();

    bool fBech32 = request.params.size() > 3 ? request.params[3].get_bool() : false;
    bool fMakeV2 = request.params.size() > 4 ? request.params[4].get_bool() : false;

    if (fMakeV2 && !fBech32)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("bech32 must be true when using makeV2."));

    CEKAStealthKey akStealth;
    std::string sError;
    if (fMakeV2)
    {
        if (0 != pwallet->NewStealthKeyV2FromAccount(sLabel, akStealth, num_prefix_bits, sPrefix_num.empty() ? nullptr : sPrefix_num.c_str(), fBech32))
            throw JSONRPCError(RPC_WALLET_ERROR, _("NewStealthKeyV2FromAccount failed."));
    } else
    {
        if (0 != pwallet->NewStealthKeyFromAccount(sLabel, akStealth, num_prefix_bits, sPrefix_num.empty() ? nullptr : sPrefix_num.c_str(), fBech32))
            throw JSONRPCError(RPC_WALLET_ERROR, _("NewStealthKeyFromAccount failed."));
    };

    CStealthAddress sxAddr;
    akStealth.SetSxAddr(sxAddr);

    return sxAddr.ToString(fBech32);
}

static UniValue importstealthaddress(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(
            "importstealthaddress \"scan_secret\" \"spend_secret\" ( \"label\" num_prefix_bits prefix_num bech32 )\n"
            "Import an owned stealth addresses.\n"
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. \"scan_secret\"       (string, required) The hex or wif encoded scan secret.\n"
            "2. \"spend_secret\"      (string, required) The hex or wif encoded spend secret.\n"
            "3. \"label\"             (string, optional) If specified the key is added to the address book.\n"
            "4. num_prefix_bits     (int, optional) If specified and > 0, the stealth address is created with a prefix.\n"
            "5. prefix_num          (int, optional) If prefix_num is not specified the prefix will be selected deterministically.\n"
            "           prefix_num can be specified in base2, 10 or 16, for base 2 prefix_num must begin with 0b, 0x for base16.\n"
            "           A 32bit integer will be created from prefix_num and the least significant num_prefix_bits will become the prefix.\n"
            "           A stealth address created without a prefix will scan all incoming stealth transactions, irrespective of transaction prefixes.\n"
            "           Stealth addresses with prefixes will scan only incoming stealth transactions with a matching prefix.\n"
            "6. bech32              (bool, optional) Use Bech32 encoding.\n"
            "\nResult:\n"
            "\"address\"              (string) The new particl stealth address\n"
            "\nExamples:\n"
            + HelpExampleCli("importstealthaddress", "scan_secret spend_secret \"label\" 3 \"0b101\"")
            + HelpExampleRpc("importstealthaddress", "scan_secret, spend_secret, \"label\", 3, \"0b101\""));

    EnsureWalletIsUnlocked(pwallet);

    std::string sScanSecret  = request.params[0].get_str();
    std::string sSpendSecret = request.params[1].get_str();
    std::string sLabel;

    if (request.params.size() > 2)
        sLabel = request.params[2].get_str();

    uint32_t num_prefix_bits = 0;
    if (request.params.size() > 3)
    {
        std::string s = request.params[3].get_str();
        if (s.length() && !ParseUInt32(s, &num_prefix_bits))
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

    bool fBech32 = request.params.size() > 5 ? request.params[5].get_bool() : false;

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

    if (skSpend == skScan)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Spend secret must be different to scan secret."));

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
                LOCK(pwallet->cs_wallet);
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

        pwallet->SetAddressBook(sxAddr, sLabel, "", fBech32);
    }

    if (fFound)
    {
        result.pushKV("result", "Success, updated " + sxAddr.Encoded(fBech32));
    } else
    {
        if (!pwallet->ImportStealthAddress(sxAddr, skSpend))
            throw JSONRPCError(RPC_WALLET_ERROR, "Could not save to wallet.");
        result.pushKV("result", "Success");
        result.pushKV("stealth_address", sxAddr.Encoded(fBech32));
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

static UniValue liststealthaddresses(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "liststealthaddresses ( show_secrets )\n"
            "List owned stealth addresses.\n"
            "\nArguments:\n"
            "1. \"show_secrets\"      (bool, optional, default = false) Print secret keys to stealth addresses, wallet must be unlocked.\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"Account\": \"str\",          (string) Account name.\n"
            "    \"Stealth Addresses\": [\n"
            "      {\n"
            "        \"Label\": \"str\",          (string) Stealth address label.\n"
            "        \"Address\": \"str\",        (string) Stealth address.\n"
            "        \"Scan Secret\": \"str\",    (string) Scan secret, if show_secrets=1.\n"
            "        \"Spend Secret\": \"str\",   (string) Spend secret, if show_secrets=1.\n"
            "      }\n"
            "    ]\n"
            "  }...\n"
            "]\n"
            "\"address\"              (string) The new particl stealth address\n"
            "\nExamples:\n"
            + HelpExampleCli("liststealthaddresses", "")
            + HelpExampleRpc("liststealthaddresses", ""));

    bool fShowSecrets = request.params.size() > 0 ? GetBool(request.params[0]) : false;

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


static UniValue scanchain(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "scanchain [from_height]\n"
            "\nDEPRECATED, will be removed in 0.17. Replaced by rescanblockchain.\n"
            "Scan blockchain for owned transactions.");

    if (!IsDeprecatedRPCEnabled("scanchain")) {
        throw JSONRPCError(RPC_METHOD_DEPRECATED, "scanchain is deprecated and will be fully removed in v0.18. "
            "To use scanchain in v0.17, restart particld with -deprecatedrpc=scanchain.\n"
            "Projects should transition to using rescanblockchain before upgrading to v0.18");
    }

    //EnsureWalletIsUnlocked(pwallet);

    UniValue result(UniValue::VOBJ);
    int32_t nFromHeight = 0;

    if (request.params.size() > 0)
        nFromHeight = request.params[0].get_int();

    pwallet->ScanChainFromHeight(nFromHeight);

    result.pushKV("result", "Scan complete.");

    return result;
}

static UniValue reservebalance(const JSONRPCRequest &request)
{
    // Reserve balance from being staked for network protection

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "reservebalance reserve ( amount )\n"
            "reserve is true or false to turn balance reserve on or off.\n"
            "amount is a real and rounded to cent.\n"
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

static UniValue deriverangekeys(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 1 ||request.params.size() > 7)
        throw std::runtime_error(
            "deriverangekeys start ( end \"key/id\" hardened save add_to_addressbook 256bithash )\n"
            "Derive keys from the specified chain.\n"
            "Wallet must be unlocked if save or hardened options are set.\n"
            "\nArguments:\n"
            "1. start               (int, required) Start from key.\n"
            "2. end                 (int, optional) Stop deriving after key, default set to derive one key.\n"
            "3. \"key/id\"            (string, optional)  Account to derive from, default external chain of current account.\n"
            "4. hardened            (bool, optional, default=false) Derive hardened keys.\n"
            "5. save                (bool, optional, default=false) Save derived keys to the wallet.\n"
            "6. add_to_addressbook  (bool, optional, default=false) Add derived keys to address book, only applies when saving keys.\n"
            "7. 256bithash          (bool, optional, default=false) Display addresses from sha256 hash of public keys.\n"
            "\nResult:\n"
            "\"addresses\"            (json) Array of derived addresses\n"
            "\nExamples:\n"
            + HelpExampleCli("deriverangekeys", "0 1")
            + HelpExampleRpc("deriverangekeys", "0, 1"));

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
                if (HK_YES != sea->HaveSavedKey(idk))
                {
                    CEKAKey ak(nChain, nChildOut);
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

static UniValue clearwallettransactions(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "clearwallettransactions ( remove_all )\n"
            "Delete transactions from the wallet.\n"
            "By default removes only failed stakes.\n"
            "Warning: Backup your wallet before using!\n"
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. remove_all           (bool, optional, default=false) Remove all transactions.\n"
            "\nExamples:\n"
            + HelpExampleCli("clearwallettransactions", "")
            + HelpExampleRpc("clearwallettransactions", "true"));

    EnsureWalletIsUnlocked(pwallet);

    bool fRemoveAll = request.params.size() > 0 ? GetBool(request.params[0]) : false;

    int rv;
    size_t nRemoved = 0;
    size_t nRecordsRemoved = 0;

    {
        LOCK2(cs_main, pwallet->cs_wallet);

        CHDWalletDB wdb(pwallet->GetDBHandle());
        if (!wdb.TxnBegin())
            throw JSONRPCError(RPC_MISC_ERROR, "TxnBegin failed.");

        Dbc *pcursor = wdb.GetTxnCursor();
        if (!pcursor)
            throw JSONRPCError(RPC_MISC_ERROR, "GetTxnCursor failed.");

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
                throw JSONRPCError(RPC_MISC_ERROR, "pcursor->del failed.");

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
                    throw JSONRPCError(RPC_MISC_ERROR, "pcursor->del failed.");

                // TODO: Remove CStoredTransaction

                nRecordsRemoved++;
            };
        };

        pcursor->close();
        if (!wdb.TxnCommit())
        {
            throw JSONRPCError(RPC_MISC_ERROR, "TxnCommit failed.");
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
        output.pushKV("narration", mvi->second);
    }
    if (addr.Set(o.destination)) {
        output.pushKV("address", addr.ToString());
        addresses.push_back(addr.ToString());
    }
    if (o.ismine & ISMINE_WATCH_ONLY) {
        if (watchonly & ISMINE_WATCH_ONLY) {
            output.pushKV("involvesWatchonly", true);
        } else {
            return false;
        }
    }
    if (o.destStake.type() != typeid(CNoDestination)) {
        output.pushKV("coldstake_address", CBitcoinAddress(o.destStake).ToString());
    }
    if (pwallet->mapAddressBook.count(o.destination)) {
        output.pushKV("label", pwallet->mapAddressBook[o.destination].name);
    }
    output.pushKV("vout", o.vout);
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
    bool                 fBech32,
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
        ISMINE_ALL,
        true);

    if (wtx.IsFromMe(ISMINE_WATCH_ONLY) && !(watchonly & ISMINE_WATCH_ONLY)) {
        return ;
    }

    std::vector<std::string> addresses;
    std::vector<std::string> amounts;

    UniValue outputs(UniValue::VARR);
    // common to every type of transaction
    if (strSentAccount != "") {
        entry.pushKV("account", strSentAccount);
    }
    WalletTxToJSON(wtx, entry, true);

    if (!listStaked.empty() || !listSent.empty())
        entry.pushKV("abandoned", wtx.isAbandoned());

    // staked
    if (!listStaked.empty()) {
        if (wtx.GetDepthInMainChain() < 1) {
            entry.pushKV("category", "orphaned_stake");
        } else {
            entry.pushKV("category", "stake");
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
            output.pushKV("amount", ValueFromAmount(s.amount));
            outputs.push_back(output);
        }

        amount += -nFee;
    } else {
        // sent
        if (!listSent.empty()) {
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
                output.pushKV("amount", ValueFromAmount(-s.amount));
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
                        output.pushKV("stealth_address", sx.Encoded(fBech32));
                    }
                }
                output.pushKV("amount", ValueFromAmount(r.amount));
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
                entry.pushKV("category", "orphan");
            } else if (wtx.GetBlocksToMaturity() > 0) {
                entry.pushKV("category", "immature");
            } else {
                entry.pushKV("category", "coinbase");
            }
        } else if (!nFee) {
            entry.pushKV("category", "receive");
        } else if (amount == 0) {
            if (listSent.empty())
                entry.pushKV("fee", ValueFromAmount(-nFee));
            entry.pushKV("category", "internal_transfer");
        } else {
            entry.pushKV("category", "send");

            // Handle txns partially funded by wallet
            if (nFee < 0)
                amount = wtx.GetCredit(ISMINE_ALL) - wtx.GetDebit(ISMINE_ALL);
            else
                entry.pushKV("fee", ValueFromAmount(-nFee));
        }
    };

    entry.pushKV("outputs", outputs);
    entry.pushKV("amount", ValueFromAmount(amount));

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
        entry.pushKV("reward", ValueFromAmount(nOutput - nInput));
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
                    LogPrintf("%s: Warning, malformed vPath.\n", __func__);
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
    } else if (nOwned && !nFrom) {
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

    if (nOwned && nFrom && nOwned != outputs.size())
    {
        // Must check against the owned input value
        CAmount nInput = 0;
        for (const auto &vin : rtx.vin)
        {
            if (vin.IsAnonInput())
                continue;
            nInput += pwallet->GetOwnedOutputValue(vin, watchonly_filter);
        };

        CAmount nOutput = 0;
        for (auto &record : rtx.vout)
        {
            if ((record.nFlags & ORF_OWNED && watchonly_filter & ISMINE_SPENDABLE)
                || (record.nFlags & ORF_OWN_WATCH && watchonly_filter & ISMINE_WATCH_ONLY))
                nOutput += record.nValue;
        };

        push(entry, "amount", ValueFromAmount(nOutput-nInput));
    } else
    {
        push(entry, "amount", ValueFromAmount(totalAmount));
    };
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

static UniValue filtertransactions(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "filtertransactions ( options )\n"
            "List transactions.\n"
            "1. options (json, optional) : A configuration object for the query\n"
            "\n"
            "  All keys are optional. Default values are:\n"
            "  {\n"
            "    \"count\":             10,\n"
            "    \"skip\":              0,\n"
            "    \"include_watchonly\": false,\n"
            "    \"search\":            ''\n"
            "    \"category\":          'all',\n"
            "    \"type\":              'all',\n"
            "    \"sort\":              'time'\n"
            "    \"from\":              '0'\n"
            "    \"to\":                '9999'\n"
            "    \"collate\":           false\n"
            "    \"with_reward\":       false\n"
            "    \"use_bech32\":        false\n"
            "  }\n"
            "\n"
            "  Expected values are:\n"
            "    count:             number of transactions to be displayed\n"
            "                       (integer >= 0, use 0 for unlimited)\n"
            "    skip:              number of transactions to skip\n"
            "                       (integer >= 0)\n"
            "    include_watchonly: whether to include watchOnly transactions\n"
            "                       (bool string)\n"
            "    search:            a query to search addresses and amounts\n"
            "                       character DOT '.' is not searched for:\n"
            "                       search \"123\" will find 1.23 and 12.3\n"
            "                       (query string)\n"
            "    category:          select only one category of transactions to return\n"
            "                       (string from list)\n"
            "                       all, send, orphan, immature, coinbase, receive,\n"
            "                       orphaned_stake, stake, internal_transfer\n"
            "    type:              select only one type of transactions to return\n"
            "                       (string from list)\n"
            "                       all, standard, anon, blind\n"
            "    sort:              sort transactions by criteria\n"
            "                       (string from list)\n"
            "                       time          most recent first\n"
            "                       address       alphabetical\n"
            "                       category      alphabetical\n"
            "                       amount        biggest first\n"
            "                       confirmations most confirmations first\n"
            "                       txid          alphabetical\n"
            "    from:              unix timestamp or string \"yyyy-mm-ddThh:mm:ss\"\n"
            "    to:                unix timestamp or string \"yyyy-mm-ddThh:mm:ss\"\n"
            "    collate:           display number of records and sum of amount fields\n"
            "    with_reward        calculate reward explicitly from txindex if necessary\n"
            "    use_bech32         display addresses in bech32 encoding\n"
            "\n"
            "\nExamples:\n"
            "\nList only when category is 'stake'\n"
            + HelpExampleCli("filtertransactions", "\"{\\\"category\\\":\\\"stake\\\"}\"") +
            "\nMultiple arguments\n"
            + HelpExampleCli("filtertransactions", "\"{\\\"sort\\\":\\\"amount\\\", \\\"category\\\":\\\"receive\\\"}\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("filtertransactions", "{\\\"category\\\":\\\"stake\\\"}")
        );

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

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
    bool fBech32 = false;

    if (!request.params[0].isNull()) {
        const UniValue & options = request.params[0].get_obj();
        RPCTypeCheckObj(options,
            {
                {"count",             UniValueType(UniValue::VNUM)},
                {"skip",              UniValueType(UniValue::VNUM)},
                {"include_watchonly", UniValueType(UniValue::VBOOL)},
                {"search",            UniValueType(UniValue::VSTR)},
                {"category",          UniValueType(UniValue::VSTR)},
                {"type",              UniValueType(UniValue::VSTR)},
                {"sort",              UniValueType(UniValue::VSTR)},
                {"collate",           UniValueType(UniValue::VBOOL)},
                {"with_reward",       UniValueType(UniValue::VBOOL)},
                {"use_bech32",        UniValueType(UniValue::VBOOL)},
            },
            true, // allow null
            false // strict
        );
        if (options.exists("count")) {
            int _count = options["count"].get_int();
            if (_count < 0) {
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
        if (options["use_bech32"].isBool())
            fBech32 = options["use_bech32"].get_bool();
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
                fBech32,
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
        std::string a_address = getAddress(a);
        std::string b_address = getAddress(b);
        double a_amount =   a["category"].get_str() == "send"
                        ? -(a["amount"  ].get_real())
                        :   a["amount"  ].get_real();
        double b_amount =   b["category"].get_str() == "send"
                        ? -(b["amount"  ].get_real())
                        :   b["amount"  ].get_real();
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

    // filter, skip, count and sum
    CAmount nTotalAmount = 0, nTotalReward = 0;
    UniValue result(UniValue::VARR);
    if (count == 0) {
        count = values.size();
    }
    // for every value while count is positive
    for (unsigned int i = 0; i < values.size() && count != 0; i++) {
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

static UniValue filteraddresses(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 6)
        throw std::runtime_error(
            "filteraddresses ( offset count sort_code \"match_str\" match_owned show_path )\n"
            "List addresses."
            "filteraddresses offset count will list 'count' addresses starting from 'offset'\n"
            "filteraddresses -1 will count addresses\n"
            "sort_code 0 sort by label ascending, 1 sort by label descending, default 0\n"
            "\"match_str]\" filter by label\n"
            "match_owned 0 off, 1 owned, 2 non-owned, default 0\n"
            );

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

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
                it->second.nOwned = pwallet->HaveAddress(it->first) ? 1 : 2;

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
        if (s != "" && !ParseInt32(s, &nMatchOwned))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown nMatchOwned.");
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
                it->second.nOwned = pwallet->HaveAddress(it->first) ? 1 : 2;

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

static UniValue manageaddressbook(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 4)
        throw std::runtime_error(
            "manageaddressbook \"action\" \"address\" ( \"label\" \"purpose\" )\n"
            "Manage the address book."
            "\nArguments:\n"
            "1. \"action\"      (string, required) 'add/edit/del/info/newsend' The action to take.\n"
            "2. \"address\"     (string, required) The address to affect.\n"
            "3. \"label\"       (string, optional) Optional label.\n"
            "4. \"purpose\"     (string, optional) Optional purpose label.\n");

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

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
            mabi->second.nOwned = pwallet->HaveAddress(mabi->first) ? 1 : 2;

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
static UniValue getstakinginfo(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "getstakinginfo\n"
            "Returns an object containing staking-related information."
            "\nResult:\n"
            "{\n"
            "  \"enabled\": true|false,         (boolean) if staking is enabled or not on this wallet\n"
            "  \"staking\": true|false,         (boolean) if this wallet is staking or not\n"
            "  \"errors\": \"...\"              (string) any error messages\n"
            "  \"percentyearreward\": xxxxxxx,  (numeric) current stake reward percentage\n"
            "  \"moneysupply\": xxxxxxx,        (numeric) the total amount of particl in the network\n"
            "  \"reserve\": xxxxxxx,            (numeric) the total amount of particl in the network\n"
            "  \"walletfoundationdonationpercent\": xxxxxxx,\n    (numeric) user set percentage of the block reward ceded to the foundation\n"
            "  \"foundationdonationpercent\": xxxxxxx,\n    (numeric) network enforced percentage of the block reward ceded to the foundation\n"
            "  \"foundationdonationpercent\": xxxxxxx,\n    (numeric) network enforced percentage of the block reward ceded to the foundation\n"
            "  \"currentblocksize\": nnn,       (numeric) the last block size in bytes\n"
            "  \"currentblockweight\": nnn,     (numeric) the last block weight\n"
            "  \"currentblocktx\": nnn,         (numeric) the number of transactions in the last block\n"
            "  \"pooledtx\": n                  (numeric) the number of transactions in the mempool\n"
            "  \"difficulty\": xxx.xxxxx        (numeric) the current difficulty\n"
            "  \"lastsearchtime\": xxxxxxx      (numeric) the last time this wallet searched for a coinstake\n"
            "  \"weight\": xxxxxxx              (numeric) the current stake weight of this wallet\n"
            "  \"netstakeweight\": xxxxxxx      (numeric) the current stake weight of the network\n"
            "  \"expectedtime\": xxxxxxx        (numeric) estimated time for next stake\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getstakinginfo", "")
            + HelpExampleRpc("getstakinginfo", ""));

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

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

    obj.pushKV("enabled", gArgs.GetBoolArg("-staking", true)); // enabled on node, vs enabled on wallet
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
        case CHDWallet::NOT_STAKING_DISABLED:
            obj.pushKV("cause", "disabled");
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

    obj.pushKV("difficulty", GetDifficulty(chainActive.Tip()));
    obj.pushKV("lastsearchtime", (uint64_t)pwallet->nLastCoinStakeSearchTime);

    obj.pushKV("weight", (uint64_t)nWeight);
    obj.pushKV("netstakeweight", (uint64_t)nNetworkWeight);

    obj.pushKV("expectedtime", nExpectedTime);

    return obj;
};

static UniValue getcoldstakinginfo(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "getcoldstakinginfo\n"
            "Returns an object containing coldstaking related information."
            "\nResult:\n"
            "{\n"
            "  \"enabled\": true|false,             (boolean) If a valid coldstakingaddress is loaded or not on this wallet.\n"
            "  \"coldstaking_extkey_id\"            (string) The id of the current coldstakingaddress.\n"
            "  \"coin_in_stakeable_script\"         (numeric) Current amount of coin in scripts stakeable by this wallet.\n"
            "  \"coin_in_coldstakeable_script\"     (numeric) Current amount of coin in scripts stakeable by the wallet with the coldstakingaddress.\n"
            "  \"percent_in_coldstakeable_script\"  (numeric) Percentage of coin in coldstakeable scripts.\n"
            "  \"currently_staking\"                (numeric) Amount of coin estimated to be currently staking by this wallet.\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getcoldstakinginfo", "")
            + HelpExampleRpc("getcoldstakinginfo", ""));

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    UniValue obj(UniValue::VOBJ);

    std::vector<COutput> vecOutputs;

    bool include_unsafe = false;
    bool fIncludeImmature = true;
    CAmount nMinimumAmount = 0;
    CAmount nMaximumAmount = MAX_MONEY;
    CAmount nMinimumSumAmount = MAX_MONEY;
    uint64_t nMaximumCount = 0;
    int nMinDepth = 0;
    int nMaxDepth = 0x7FFFFFFF;
    int nHeight, nRequiredDepth;

    {
        LOCK2(cs_main, pwallet->cs_wallet);
        nHeight = chainActive.Tip()->nHeight;
        nRequiredDepth = std::min((int)(Params().GetStakeMinConfirmations()-1), (int)(nHeight / 2));
        pwallet->AvailableCoins(vecOutputs, !include_unsafe, nullptr, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount, nMinDepth, nMaxDepth, fIncludeImmature);
    }

    LOCK(pwallet->cs_wallet);

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


    bool fEnabled = false;
    UniValue jsonSettings;
    CBitcoinAddress addrColdStaking;
    if (pwallet->GetSetting("changeaddress", jsonSettings)
        && jsonSettings["coldstakingaddress"].isStr())
    {
        std::string sAddress;
        try { sAddress = jsonSettings["coldstakingaddress"].get_str();
        } catch (std::exception &e) {
            return error("%s: Get coldstakingaddress failed %s.", __func__, e.what());
        };

        addrColdStaking = sAddress;
        if (addrColdStaking.IsValid())
            fEnabled = true;
    };

    obj.pushKV("enabled", fEnabled);
    if (addrColdStaking.IsValid(CChainParams::EXT_PUBLIC_KEY))
    {
        CTxDestination dest = addrColdStaking.Get();
        CExtKeyPair kp = boost::get<CExtKeyPair>(dest);
        CKeyID idk = kp.GetID();
        CBitcoinAddress addr;
        addr.Set(idk, CChainParams::EXT_KEY_HASH);
        obj.pushKV("coldstaking_extkey_id", addr.ToString());
    };
    obj.pushKV("coin_in_stakeable_script", ValueFromAmount(nStakeable));
    obj.pushKV("coin_in_coldstakeable_script", ValueFromAmount(nColdStakeable));
    CAmount nTotal = nColdStakeable + nStakeable;
    obj.pushKV("percent_in_coldstakeable_script",
        UniValue(UniValue::VNUM, strprintf("%.2f", nTotal == 0 ? 0.0 : (nColdStakeable * 10000 / nTotal) / 100.0)));
    obj.pushKV("currently_staking", ValueFromAmount(nWalletStaking));

    return obj;
};


static UniValue listunspentanon(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
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
            "    \"label\" : \"label\",        (string) The associated label, or \"\" for the default label\n"
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
            + HelpExampleRpc("listunspentanon", "6, 9999999, \"[\\\"PfqK97PXYfqRFtdYcZw82x3dzPrZbEAcYa\\\",\\\"Pka9M2Bva8WetQhQ4ngC255HAbMJf5P5Dc\\\"]\"")
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

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    UniValue results(UniValue::VARR);
    std::vector<COutputR> vecOutputs;
    assert(pwallet != nullptr);

    {
        LOCK2(cs_main, pwallet->cs_wallet);
        // TODO: filter on stealth address
        pwallet->AvailableAnonCoins(vecOutputs, !include_unsafe, nullptr, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount, nMinDepth, nMaxDepth, fIncludeImmature);
    }

    LOCK(pwallet->cs_wallet);

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
                LogPrintf("%s: Warning, malformed vPath.\n", __func__);
            } else
            {
                uint32_t sidx;
                memcpy(&sidx, &pout->vPath[1], 4);
                CStealthAddress sx;
                if (pwallet->GetStealthByIndex(sidx, sx)) {
                    entry.pushKV("address", sx.Encoded());

                    auto i = pwallet->mapAddressBook.find(sx);
                    if (i != pwallet->mapAddressBook.end()) {
                        entry.pushKV("label", i->second.name);
                    }
                }
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
        entry.pushKV("safe", out.fSafe);
        if (fIncludeImmature)
            entry.pushKV("mature", out.fMature);

        results.push_back(entry);
    }

    return results;
};

static UniValue listunspentblind(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
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
            "3. \"addresses\"      (string) A json array of particl addresses to filter\n"
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
            "    \"label\" : \"label\",        (string) The associated label, or \"\" for the default label\n"
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
            + HelpExampleRpc("listunspentblind", "6, 9999999, \"[\\\"PfqK97PXYfqRFtdYcZw82x3dzPrZbEAcYa\\\",\\\"Pka9M2Bva8WetQhQ4ngC255HAbMJf5P5Dc\\\"]\"")
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

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    UniValue results(UniValue::VARR);
    std::vector<COutputR> vecOutputs;
    assert(pwallet != nullptr);

    {
        LOCK2(cs_main, pwallet->cs_wallet);
        pwallet->AvailableBlindedCoins(vecOutputs, !include_unsafe, nullptr, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount, nMinDepth, nMaxDepth);
    }

    LOCK(pwallet->cs_wallet);

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

            auto i = pwallet->mapAddressBook.find(address);
            if (i != pwallet->mapAddressBook.end()) {
                entry.pushKV("label", i->second.name);
            }

            if (address.type() == typeid(CKeyID))
            {
                CStealthAddress sx;
                CKeyID idk = boost::get<CKeyID>(address);
                if (pwallet->GetStealthLinked(idk, sx)) {
                    entry.pushKV("stealth_address", sx.Encoded());
                    if (!entry.exists("label")) {
                        auto i = pwallet->mapAddressBook.find(sx);
                        if (i != pwallet->mapAddressBook.end()) {
                            entry.pushKV("label", i->second.name);
                        }
                    }
                }
            };

            if (scriptPubKey->IsPayToScriptHash()) {
                const CScriptID& hash = boost::get<CScriptID>(address);
                CScript redeemScript;
                if (pwallet->GetCScript(hash, redeemScript))
                    entry.pushKV("redeemScript", HexStr(redeemScript.begin(), redeemScript.end()));
            } else
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
        //entry.pushKV("spendable", out.fSpendable);
        //entry.pushKV("solvable", out.fSolvable);
        entry.pushKV("safe", out.fSafe);
        results.push_back(entry);
    }

    return results;
};


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
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    if (!request.fSkipBlock)
        pwallet->BlockUntilSyncedToCurrentChain();

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
                    JSONRPCError(RPC_INVALID_PARAMETER, "Address parameter must be 'script' to set script explicitly.");

                std::string sScript = obj["script"].get_str();
                std::vector<uint8_t> scriptData = ParseHex(sScript);
                r.scriptPubKey = CScript(scriptData.begin(), scriptData.end());
                r.fScriptSet = true;

                if (typeOut != OUTPUT_STANDARD)
                    throw std::runtime_error("TODO: Currently setting a script only works for standard outputs.");
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
    CTransactionRef tx_new;
    CWalletTx wtx(pwallet, tx_new);
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
                if (IsStandard(script, whichType))
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
            coincontrol.m_signal_bip125_rbf = uvCoinControl["replaceable"].get_bool();
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
        result.pushKV("need_hwdevice", UniValue(coincontrol.fNeedHardwareKey ? true : false));

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
        std::string sFromAccount = "";
        if (!pwallet->CommitTransaction(wtx.tx, wtx.mapValue, wtx.vOrderForm, sFromAccount, reservekey, g_connman.get(), state))
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Transaction commit failed: %s", FormatStateMessage(state)));
    } else
    {
        if (!pwallet->CommitTransaction(wtx, rtx, reservekey, g_connman.get(), state))
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Transaction commit failed: %s", FormatStateMessage(state)));
    };

    /*
    UniValue vErrors(UniValue::VARR);
    if (!state.IsValid()) // Should be caught in CommitTransaction
    {
        // This can happen if the mempool rejected the transaction.  Report
        // what happened in the "errors" response.
        vErrors.push_back(strprintf("Error: The transaction was rejected: %s", FormatStateMessage(state)));

        UniValue result(UniValue::VOBJ);
        result.pushKV("txid", wtx.GetHash().GetHex());
        result.pushKV("errors", vErrors);
        return result;
    };
    */

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

static UniValue sendparttoblind(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_STANDARD, OUTPUT_CT));

    return SendToInner(request, OUTPUT_STANDARD, OUTPUT_CT);
};

static UniValue sendparttoanon(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_STANDARD, OUTPUT_RINGCT));

    return SendToInner(request, OUTPUT_STANDARD, OUTPUT_RINGCT);
};


static UniValue sendblindtopart(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_CT, OUTPUT_STANDARD));

    return SendToInner(request, OUTPUT_CT, OUTPUT_STANDARD);
};

static UniValue sendblindtoblind(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_CT, OUTPUT_CT));

    return SendToInner(request, OUTPUT_CT, OUTPUT_CT);
};

static UniValue sendblindtoanon(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 6)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_CT, OUTPUT_RINGCT));

    return SendToInner(request, OUTPUT_CT, OUTPUT_RINGCT);
};


static UniValue sendanontopart(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 8)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_RINGCT, OUTPUT_STANDARD));

    return SendToInner(request, OUTPUT_RINGCT, OUTPUT_STANDARD);
};

static UniValue sendanontoblind(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 8)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_RINGCT, OUTPUT_CT));

    return SendToInner(request, OUTPUT_RINGCT, OUTPUT_CT);
};

static UniValue sendanontoanon(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 8)
        throw std::runtime_error(SendHelp(pwallet, OUTPUT_RINGCT, OUTPUT_RINGCT));

    return SendToInner(request, OUTPUT_RINGCT, OUTPUT_RINGCT);
};

UniValue sendtypeto(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
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

static UniValue createsignaturewithwallet(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 3 || request.params.size() > 4)
        throw std::runtime_error(
            "createsignaturewithwallet \"hexstring\" \"prevtx\" \"address\" \"sighashtype\"\n"
            "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
            + HelpRequiringPassphrase(pwallet) + "\n"

            "\nArguments:\n"
            "1. \"hexstring\"                      (string, required) The transaction hex string\n"
            "2. \"prevtx\"                         (json, required) The prevtx signing for\n"
            "    {\n"
            "     \"txid\":\"id\",                   (string, required) The transaction id\n"
            "     \"vout\":n,                      (numeric, required) The output number\n"
            "     \"scriptPubKey\": \"hex\",         (string, required) script key\n"
            "     \"redeemScript\": \"hex\",         (string, required for P2SH or P2WSH) redeem script\n"
            "     \"amount\": value                (numeric or string, required) The amount spent\n"
            "     \"amount_commitment\": \"hex\",    (string, required) The amount commitment spent\n"
            "   }\n"
            "3. \"address\"                        (string, required) The address of the private key to sign with\n"
            "4. \"sighashtype\"                    (string, optional, default=ALL) The signature hash type. Must be one of\n"
            "       \"ALL\"\n"
            "       \"NONE\"\n"
            "       \"SINGLE\"\n"
            "       \"ALL|ANYONECANPAY\"\n"
            "       \"NONE|ANYONECANPAY\"\n"
            "       \"SINGLE|ANYONECANPAY\"\n"
            "\nResult:\n"
            "The hex encoded signature.\n"
            "\nExamples:\n"
            + HelpExampleCli("createsignaturewithwallet", "\"myhex\" 0 \"myaddress\"")
            + HelpExampleRpc("createsignaturewithwallet", "\"myhex\", 0, \"myaddress\"")
        );

    EnsureWalletIsUnlocked(pwallet);

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VOBJ, UniValue::VSTR, UniValue::VSTR}, true);

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str(), true)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    UniValue prevOut = request.params[1].get_obj();

    RPCTypeCheckObj(prevOut,
        {
            {"txid", UniValueType(UniValue::VSTR)},
            {"vout", UniValueType(UniValue::VNUM)},
            {"scriptPubKey", UniValueType(UniValue::VSTR)},
        });

    uint256 txid = ParseHashO(prevOut, "txid");

    int nOut = find_value(prevOut, "vout").get_int();
    if (nOut < 0) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");
    }

    COutPoint out(txid, nOut);
    std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
    CScript scriptRedeem, scriptPubKey(pkData.begin(), pkData.end());

    std::vector<uint8_t> vchAmount;
    if (prevOut.exists("amount"))
    {
        if (prevOut.exists("amount_commitment"))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Both \"amount\" and \"amount_commitment\" found.");
        CAmount nValue = AmountFromValue(prevOut["amount"]);
        vchAmount.resize(8);
        memcpy(vchAmount.data(), &nValue, 8);
    } else
    if (prevOut.exists("amount_commitment"))
    {
        std::string s = prevOut["amount_commitment"].get_str();
        if (!IsHex(s) || !(s.size() == 66))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "\"amount_commitment\" must be 33 bytes and hex encoded.");
        vchAmount = ParseHex(s);
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "\"amount\" or \"amount_commitment\" is required");
    };

    if (prevOut.exists("redeemScript"))
    {
        std::vector<unsigned char> redeemData(ParseHexO(prevOut, "redeemScript"));
        scriptRedeem = CScript(redeemData.begin(), redeemData.end());
    };

    CKeyID idSign;
    CTxDestination dest = DecodeDestination(request.params[2].get_str());
    if (!IsValidDestination(dest))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");

    if (dest.type() == typeid(CKeyID))
    {
        idSign = boost::get<CKeyID>(dest);
    } else
    {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unsupported destination type.");
    };

    const UniValue &hashType = request.params[3];
    int nHashType = SIGHASH_ALL;
    if (!hashType.isNull()) {
        static std::map<std::string, int> mapSigHashValues = {
            {std::string("ALL"), int(SIGHASH_ALL)},
            {std::string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY)},
            {std::string("NONE"), int(SIGHASH_NONE)},
            {std::string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY)},
            {std::string("SINGLE"), int(SIGHASH_SINGLE)},
            {std::string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY)},
        };
        std::string strHashType = hashType.get_str();
        if (mapSigHashValues.count(strHashType)) {
            nHashType = mapSigHashValues[strHashType];
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid sighash param");
        }
    }


    // Sign the transaction
    LOCK2(cs_main, pwallet->cs_wallet);

    std::vector<uint8_t> vchSig;
    unsigned int i;
    for (i = 0; i < mtx.vin.size(); i++) {
        CTxIn& txin = mtx.vin[i];

        if (txin.prevout == out)
        {
            MutableTransactionSignatureCreator creator(&mtx, i, vchAmount, nHashType);
            CScript &scriptSig = scriptPubKey.IsPayToScriptHashAny(mtx.IsCoinStake()) ? scriptRedeem : scriptPubKey;

            if (!creator.CreateSig(*pwallet, vchSig, idSign, scriptSig, SigVersion::BASE))
                throw JSONRPCError(RPC_MISC_ERROR, "CreateSig failed.");

            break;
        };
    };

    if (i >= mtx.vin.size())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "No matching input found.");

    return HexStr(vchSig);
}

static UniValue createsignaturewithkey(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() < 3 || request.params.size() > 4)
        throw std::runtime_error(
            "createsignaturewithkey \"hexstring\" \"prevtx\" \"address\" \"sighashtype\"\n"
            "\nSign inputs for raw transaction (serialized, hex-encoded).\n"

            "\nArguments:\n"
            "1. \"hexstring\"                      (string, required) The transaction hex string\n"
            "2. \"prevtx\"                         (json, required) The prevtx signing for\n"
            "    {\n"
            "     \"txid\":\"id\",                   (string, required) The transaction id\n"
            "     \"vout\":n,                      (numeric, required) The output number\n"
            "     \"scriptPubKey\": \"hex\",         (string, required) script key\n"
            "     \"redeemScript\": \"hex\",         (string, required for P2SH or P2WSH) redeem script\n"
            "     \"amount\": value                (numeric or string, required) The amount spent\n"
            "     \"amount_commitment\": \"hex\",    (string, required) The amount commitment spent\n"
            "   }\n"
            "3. \"privkey\"                        (string, required) A base58-encoded private key for signing\n"
            "4. \"sighashtype\"                    (string, optional, default=ALL) The signature hash type. Must be one of\n"
            "       \"ALL\"\n"
            "       \"NONE\"\n"
            "       \"SINGLE\"\n"
            "       \"ALL|ANYONECANPAY\"\n"
            "       \"NONE|ANYONECANPAY\"\n"
            "       \"SINGLE|ANYONECANPAY\"\n"
            "\nResult:\n"
            "The hex encoded signature.\n"
            "\nExamples:\n"
            + HelpExampleCli("createsignaturewithkey", "\"myhex\" 0 \"myprivkey\"")
            + HelpExampleRpc("createsignaturewithkey", "\"myhex\", 0, \"myprivkey\"")
        );

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VOBJ, UniValue::VSTR, UniValue::VSTR}, true);

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str(), true)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    UniValue prevOut = request.params[1].get_obj();

    RPCTypeCheckObj(prevOut,
        {
            {"txid", UniValueType(UniValue::VSTR)},
            {"vout", UniValueType(UniValue::VNUM)},
            {"scriptPubKey", UniValueType(UniValue::VSTR)},
        });

    uint256 txid = ParseHashO(prevOut, "txid");

    int nOut = find_value(prevOut, "vout").get_int();
    if (nOut < 0) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");
    }

    COutPoint out(txid, nOut);
    std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
    CScript scriptRedeem, scriptPubKey(pkData.begin(), pkData.end());


    std::vector<uint8_t> vchAmount(8);
    if (prevOut.exists("amount"))
    {
        if (prevOut.exists("amount_commitment"))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Both \"amount\" and \"amount_commitment\" found.");
        CAmount nValue = AmountFromValue(prevOut["amount"]);
        vchAmount.resize(8);
        memcpy(vchAmount.data(), &nValue, 8);
    } else
    if (prevOut.exists("amount_commitment"))
    {
        std::string s = prevOut["amount_commitment"].get_str();
        if (!IsHex(s) || !(s.size() == 66))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "\"amount_commitment\" must be 33 bytes and hex encoded.");
        vchAmount = ParseHex(s);
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "\"amount\" or \"amount_commitment\" is required");
    };

    if (prevOut.exists("redeemScript"))
    {
        std::vector<unsigned char> redeemData(ParseHexO(prevOut, "redeemScript"));
        scriptRedeem = CScript(redeemData.begin(), redeemData.end());
    };

    std::string strPrivkey = request.params[2].get_str();
    CKey key = DecodeSecret(strPrivkey);
    if (!key.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    }

    CBasicKeyStore keystore;
    keystore.AddKey(key);
    CKeyID idSign = key.GetPubKey().GetID();

    const UniValue &hashType = request.params[3];
    int nHashType = SIGHASH_ALL;
    if (!hashType.isNull()) {
        static std::map<std::string, int> mapSigHashValues = {
            {std::string("ALL"), int(SIGHASH_ALL)},
            {std::string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY)},
            {std::string("NONE"), int(SIGHASH_NONE)},
            {std::string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY)},
            {std::string("SINGLE"), int(SIGHASH_SINGLE)},
            {std::string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY)},
        };
        std::string strHashType = hashType.get_str();
        if (mapSigHashValues.count(strHashType)) {
            nHashType = mapSigHashValues[strHashType];
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid sighash param");
        }
    }

    std::vector<uint8_t> vchSig;
    unsigned int i;
    for (i = 0; i < mtx.vin.size(); i++) {
        CTxIn& txin = mtx.vin[i];

        if (txin.prevout == out)
        {
            MutableTransactionSignatureCreator creator(&mtx, i, vchAmount, nHashType);
            CScript &scriptSig = scriptPubKey.IsPayToScriptHashAny(mtx.IsCoinStake()) ? scriptRedeem : scriptPubKey;

            if (!creator.CreateSig(keystore, vchSig, idSign, scriptSig, SigVersion::BASE))
                throw JSONRPCError(RPC_MISC_ERROR, "CreateSig failed.");

            break;
        };
    };

    if (i >= mtx.vin.size())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "No matching input found.");

    return HexStr(vchSig);
}

static UniValue debugwallet(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "debugwallet ( attempt_repair )\n"
            "Detect problems in wallet.\n"
            + HelpRequiringPassphrase(pwallet));

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    bool fAttemptRepair = request.params.size() > 0 ? GetBool(request.params[0]) : false;

    EnsureWalletIsUnlocked(pwallet);

    UniValue result(UniValue::VOBJ);
    UniValue errors(UniValue::VARR);
    UniValue warnings(UniValue::VARR);
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

        int64_t rv = 0;
        if (pwallet->CountRecords("sxkm", rv))
            result.pushKV("locked_stealth_outputs", (int)rv);
        else
            result.pushKV("locked_stealth_outputs", "error");

        if (pwallet->CountRecords("lao", rv))
            result.pushKV("locked_blinded_outputs", (int)rv);
        else
            result.pushKV("locked_blinded_outputs", "error");

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
                    errors.push_back(tmp);
                };

                // TODO: Check hardened keys, must detect stealth key chain
            };
        };

        {
            CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");
            for (const auto &ri : pwallet->mapRecords)
            {
                const uint256 &txhash = ri.first;
                const CTransactionRecord &rtx = ri.second;

                if (!pwallet->IsTrusted(txhash, rtx.blockHash, rtx.nIndex))
                    continue;

                for (const auto &r : rtx.vout)
                {
                    if ((r.nType == OUTPUT_CT || r.nType == OUTPUT_RINGCT)
                        && (r.nFlags & ORF_OWNED || r.nFlags & ORF_STAKEONLY)
                        && !pwallet->IsSpent(txhash, r.n))
                    {
                        CStoredTransaction stx;
                        if (!wdb.ReadStoredTx(txhash, stx))
                        {
                            UniValue tmp(UniValue::VOBJ);
                            tmp.pushKV("type", "Missing stored txn.");
                            tmp.pushKV("txid", txhash.ToString());
                            tmp.pushKV("n", r.n);
                            errors.push_back(tmp);
                            continue;
                        };

                        uint256 tmp;
                        if (!stx.GetBlind(r.n, tmp.begin()))
                        {
                            UniValue tmp(UniValue::VOBJ);
                            tmp.pushKV("type", "Missing blinding factor.");
                            tmp.pushKV("txid", txhash.ToString());
                            tmp.pushKV("n", r.n);
                            errors.push_back(tmp);
                        };
                    };
                };
            };
        }
        if (pwallet->CountColdstakeOutputs() > 0)
        {
            UniValue jsonSettings;
            if (!pwallet->GetSetting("changeaddress", jsonSettings)
                || !jsonSettings["coldstakingaddress"].isStr())
            {
                UniValue tmp(UniValue::VOBJ);
                tmp.pushKV("type", "Wallet has coldstaking outputs with coldstakingaddress unset.");
                warnings.push_back(tmp);
            };
        };
    }

    result.pushKV("errors", errors);
    result.pushKV("warnings", warnings);

    return result;
};

static UniValue rewindchain(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "rewindchain ( height )\n"
            + HelpRequiringPassphrase(pwallet) +
            "height default - tip_height-1.\n");

    EnsureWalletIsUnlocked(pwallet);

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();


    LOCK2(cs_main, pwallet->cs_wallet);

    UniValue result(UniValue::VOBJ);

    CCoinsViewCache view(pcoinsTip.get());
    view.fForceDisconnect = true;
    CBlockIndex* pindexState = chainActive.Tip();
    CValidationState state;

    int nBlocks = 0;

    int nToHeight = request.params[0].isNum() ? request.params[0].get_int() : pindexState->nHeight - 1;
    result.pushKV("to_height", nToHeight);


    std::string sError;
    if (!RewindToCheckpoint(nToHeight, nBlocks, sError))
        result.pushKV("error", sError);

    result.pushKV("nBlocks", nBlocks);

    return result;
};

static UniValue walletsettings(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "walletsettings \"setting\" {...}\n"
            "\nManage wallet settings.\n"
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. \"setting\"                    (string, required) Settings group to modify.\n"
            "2. \"value\"                      (json, optional) Settings.\n"
            "\"changeaddress\" {\n"
            "  \"address_standard\"          (string, optional, default=none) Change address for standard inputs.\n"
            "  \"coldstakingaddress\"        (string, optional, default=none) Cold staking address for standard inputs.\n"
            "}\n"
            "\"stakingoptions\" {\n"
            "  \"enabled\"                   (bool, optional, default=true) Toggle staking enabled on this wallet.\n"
            "  \"stakecombinethreshold\"     (amount, optional, default=1000) Join outputs below this value.\n"
            "  \"stakesplitthreshold\"       (amount, optional, default=2000) Split outputs above this value.\n"
            "  \"foundationdonationpercent\" (int, optional, default=0) Set the percentage of each block reward to donate to the foundation.\n"
            "  \"rewardaddress\"             (string, optional, default=none) An address which the user portion of the block reward gets sent to.\n"
            "}\n"
            "\"stakelimit\" {\n"
            "  \"height\"                    (int, optional, default=0) Prevent staking above chain height, used in functional testing.\n"
            "}\n"
            "Omit the json object to print the settings group.\n"
            "Pass an empty json object to clear the settings group.\n"
            "\nExamples\n"
            "Set coldstaking changeaddress extended public key:\n"
            + HelpExampleCli("walletsettings", "changeaddress \"{\\\"coldstakingaddress\\\":\\\"extpubkey\\\"}\"") + "\n"
            "Clear changeaddress settings\n"
            + HelpExampleCli("walletsettings", "changeaddress \"{}\"") + "\n"
        );

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

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
                    if (pwallet->HaveAddress(addr.Get()))
                        throw JSONRPCError(RPC_INVALID_PARAMETER, sAddress + _(" is spendable from this wallet."));
                    if (pwallet->idDefaultAccount.IsNull())
                        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Wallet must have a default account set."));

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
                if (sKey == "enabled")
                {
                } else
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

static UniValue transactionblinds(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "transactionblinds \"txnid\"\n"
            "\nShow known blinding factors for transaction.\n"
            + HelpRequiringPassphrase(pwallet) + "\n"
            "\nArguments:\n"
            "1. \"txnid\"                          (string, required) The transaction id\n"
            "\nResult:\n"
            "   {\n"
            "     \"n\":\"hex\",                   (string) The blinding factor for output n, hex encoded\n"
            "   }\n"
            "\nExamples:\n"
            + HelpExampleCli("transactionblinds", "\"txnid\"")
            + HelpExampleRpc("transactionblinds", "\"txnid\"")
        );

    EnsureWalletIsUnlocked(pwallet);

    uint256 hash;
    hash.SetHex(request.params[0].get_str());

    MapRecords_t::const_iterator mri = pwallet->mapRecords.find(hash);

    if (mri == pwallet->mapRecords.end())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid or non-wallet transaction id");
    //const CTransactionRecord &rtx = mri->second;

    UniValue result(UniValue::VOBJ);
    CStoredTransaction stx;
    if (!CHDWalletDB(pwallet->GetDBHandle()).ReadStoredTx(hash, stx))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No stored data found for txn");

    for (size_t i = 0; i < stx.tx->vpout.size(); ++i)
    {
        uint256 tmp;
        if (stx.GetBlind(i, tmp.begin()))
            result.pushKV(strprintf("%d", i), tmp.ToString());
    };

    return result;
};

static UniValue derivefromstealthaddress(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "derivefromstealthaddress \"stealthaddress\" (\"ephempubkey\")\n"
            "\nDerive a pubkey from a stealth address and random value.\n"
            "\nArguments:\n"
            "1. \"stealthaddress\"                 (string, required) The stealth address\n"
            "2. \"ephemeralvalue\"                 (string, optional) The ephemeral value, interpreted as private key if 32 bytes or public key if 33.\n"
            "   If an ephemeral public key is provided the spending private key will be derived, wallet must be unlocked\n"
            "\nResult:\n"
            "   {\n"
            "     \"address\":\"base58\",            (string) The derived address\n"
            "     \"pubkey\":\"hex\",                (string) The derived public key\n"
            "     \"ephemeral\":\"hex\",             (string) The ephemeral public key\n"
            "     \"privatekey\":\"wif\",            (string) The derived privatekey, if \"ephempubkey\" is provided\n"
            "   }\n"
            "\nExamples:\n"
            + HelpExampleCli("derivefromstealthaddress", "\"stealthaddress\"")
            + HelpExampleRpc("derivefromstealthaddress", "\"stealthaddress\"")
        );

    CBitcoinAddress addr(request.params[0].get_str());
    if (!addr.IsValidStealthAddress())
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Must input a stealthaddress."));

    CStealthAddress sx = boost::get<CStealthAddress>(addr.Get());


    UniValue result(UniValue::VOBJ);

    CKey sSpendR, sShared, sEphem;
    CPubKey pkEphem, pkDest;
    CTxDestination dest;

    if (request.params[1].isStr())
    {
        EnsureWalletIsUnlocked(pwallet);

        std::string s = request.params[1].get_str();
        if (!IsHex(s))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "ephemeralvalue must be hex encoded.");

        if (s.size() == 64)
        {
            std::vector<uint8_t> v = ParseHex(s);
            sEphem.Set(v.data(), true);
            if (!sEphem.IsValid())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid ephemeral private key.");
        } else
        if (s.size() == 66)
        {
            std::vector<uint8_t> v = ParseHex(s);
            pkEphem = CPubKey(v.begin(), v.end());

            if (!pkEphem.IsValid())
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid ephemeral public key.");

            CKey sSpend;
            if (!pwallet->GetStealthAddressScanKey(sx))
                throw JSONRPCError(RPC_WALLET_ERROR, "Scan key not found for stealth address.");
            if (!pwallet->GetStealthAddressSpendKey(sx, sSpend))
                throw JSONRPCError(RPC_WALLET_ERROR, "Spend key not found for stealth address.");

            ec_point pEphem;;
            pEphem.resize(EC_COMPRESSED_SIZE);
            memcpy(&pEphem[0], pkEphem.begin(), pkEphem.size());

            if (StealthSecretSpend(sx.scan_secret, pEphem, sSpend, sSpendR) != 0)
                throw JSONRPCError(RPC_WALLET_ERROR, "StealthSecretSpend failed.");

            pkDest = sSpendR.GetPubKey();
        } else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "ephemeralvalue must be 33 byte public key or 32 byte private key.");
        }
    } else
    {
        sEphem.MakeNewKey(true);
    };

    if (sEphem.IsValid())
    {
        ec_point pkSendTo;
        if (0 != StealthSecret(sEphem, sx.scan_pubkey, sx.spend_pubkey, sShared, pkSendTo))
            throw JSONRPCError(RPC_INTERNAL_ERROR, _("StealthSecret failed, try again."));

        pkEphem = sEphem.GetPubKey();
        pkDest = CPubKey(pkSendTo);
    };

    dest = GetDestinationForKey(pkDest, OutputType::LEGACY);

    result.pushKV("address", EncodeDestination(dest));
    result.pushKV("pubkey", HexStr(pkDest));
    result.pushKV("ephemeral_pubkey", HexStr(pkEphem));
    if (sEphem.IsValid())
        result.pushKV("ephemeral_privatekey", HexStr(sEphem.begin(), sEphem.end()));

    if (sSpendR.IsValid())
        result.pushKV("privatekey", CBitcoinSecret(sSpendR).ToString());

    return result;
};


static UniValue setvote(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 4)
        throw std::runtime_error(
            "setvote proposal option height_start height_end\n"
            "Set voting token.\n"
            "Wallet will include this token in staked blocks from height_start to height_end.\n"
            "Set proposal and/or option to 0 to stop voting.\n"
            "The last added option valid for the current chain height will be applied.\n"
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. proposal            (int, required) The proposal to vote on.\n"
            "2. option              (int, required) The option to vote for.\n"
            "3. height_start        (int, required) Start voting at this block height.\n"
            "3. height_end          (int, required) Stop voting at this block height.\n"
            "\nExamples:\n"
            + HelpExampleCli("setvote", "1 1 1000 2000")
            + HelpExampleRpc("setvote", "1, 1, 1000, 2000"));

    EnsureWalletIsUnlocked(pwallet);

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

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

static UniValue votehistory(const JSONRPCRequest &request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            "votehistory ( current_only )\n"
            "Display voting history.\n"
            "\nArguments:\n"
            "1. current_only        (bool, optional, default=false) Show only the currently active vote.\n"
            "\nResult\n"
            "[                   (array of json object)\n"
            "  {\n"
            "    \"proposal\" : n,      (numeric) the proposal id \n"
            "    \"option\" : n,        (numeric) the option marked\n"
            "    \"from_height\" : n,   (numeric) the starting chain height\n"
            "    \"to_height\" : n,     (numeric) the ending chain height\n"
            "  }\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("votehistory", "true")
            + HelpExampleRpc("votehistory", "true"));

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    UniValue result(UniValue::VARR);

    if (request.params.size() > 0)
    {
        if (GetBool(request.params[0]))
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

static UniValue tallyvotes(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            "tallyvotes proposal height_start height_end\n"
            "Count votes.\n"
            "\nArguments:\n"
            "1. proposal         (int, required) the proposal id.\n"
            "2. height_start     (int, required) the chain starting height.\n"
            "3. height_end       (int, required) the chain ending height.\n"
            "\nResult\n"
            " {\n"
            "   \"proposal\" : n,      (numeric) the proposal id \n"
            "   \"option\" : n,        (numeric) the option marked\n"
            "   \"height_start\" : n,  (numeric) the starting chain height\n"
            "   \"height_end\" : n,    (numeric) the ending chain height\n"
            "   \"blocks_counted\" : n,(numeric) the ending chain height\n"
            "   \"Option x\": total, %,(string) the number of votes cast for option x.\n"
            " }\n"
            "\nExamples:\n"
            + HelpExampleCli("tallyvotes", "1 2000 30000")
            + HelpExampleRpc("tallyvotes", "1, 2000, 30000"));

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

static UniValue buildscript(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "buildscript json\n"
            "\nArguments:\n"
            "1. Recipe (json, required)\n"
            " {\"recipe\":\"ifcoinstake\", \"addrstake\":\"addrA\", \"addrspend\":\"addrB\"}\n"
            " {\"recipe\":\"abslocktime\", \"time\":timestamp, \"addr\":\"addr\"}\n"
            " {\"recipe\":\"rellocktime\", \"time\":timestamp, \"addr\":\"addr\"}\n"
            "\nResult\n"
            " {\n"
            "   \"hex\" : n,        (string) Script as hex\n"
            "   \"asm\" : n,        (string) Script as asm\n"
            " }\n"
            "\nExamples:\n"
            + HelpExampleCli("buildscript", "\"{\\\"recipe\\\":\\\"ifcoinstake\\\", \\\"addrstake\\\":\\\"addrA\\\", \\\"addrspend\\\":\\\"addrB\\\"}\"")
            + HelpExampleRpc("buildscript", "\"{\\\"recipe\\\":\\\"ifcoinstake\\\", \\\"addrstake\\\":\\\"addrA\\\", \\\"addrspend\\\":\\\"addrB\\\"}\""));

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

static UniValue createrawparttransaction(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() < 2 || request.params.size() > 4)
        throw std::runtime_error(
            "createrawparttransaction [{\"txid\":\"id\",\"vout\":n},...] [{\"address\":amount,\"data\":\"hex\",...}] ( locktime replaceable \"fundfrombalance\" )\n"
            "\nCreate a transaction spending the given inputs and creating new confidential outputs.\n"
            "Outputs can be addresses or data.\n"
            "Returns hex-encoded raw transaction.\n"
            "Note that the transaction's inputs are not signed, and\n"
            "it is not stored in the wallet or transmitted to the network.\n"

            "\nArguments:\n"
            "1. \"inputs\"                (array, required) A json array of json objects\n"
            "     [\n"
            "       {\n"
            "         \"txid\":\"id\",              (string, required) The transaction id\n"
            "         \"vout\":n,                   (numeric, required) The output number\n"
            "         \"sequence\":n                (numeric, optional) The sequence number\n"
            "         \"blindingfactor\":\"hex\",   (string, optional) The blinding factor, required if blinded input is unknown to wallet\n"
            "       } \n"
            "       ,...\n"
            "     ]\n"
            "2. \"outputs\"               (array, required) A json array of json objects\n"
            "     [\n"
            "       {\n"
            "         \"address\": \"str\"          (string, required) The particl address\n"
            "         \"amount\": x.xxx           (numeric or string, required) The numeric value (can be string) in " + CURRENCY_UNIT + " of the output\n"
            "         \"data\": \"hex\",            (string, required) The key is \"data\", the value is hex encoded data\n"
            "         \"script\": \"str\",          (string, optional) Specify script directly.\n"
            "         \"type\": \"str\",            (string, optional, default=\"plain\") The type of output to create, plain, blind or anon.\n"
            "         \"pubkey\": \"hex\",          (string, optional) The key is \"pubkey\", the value is hex encoded public key for encrypting the metadata\n"
//            "         \"subfee\":bool      (boolean, optional, default=false) The fee will be deducted from the amount being sent.\n"
            "         \"narration\" \"str\",        (string, optional) Up to 24 character narration sent with the transaction.\n"
            "         \"blindingfactor\" \"hex\",   (string, optional) Blinding factor to use. Blinding factor is randomly generated if not specified.\n"
            "         \"ephemeral_key\" \"hex\",    (string, optional) Ephemeral secret key for blinded outputs.\n"
            "         \"nonce\":\"hex\"\n           (string, optional) Nonce for blinded outputs.\n"
            "       }\n"
            "       ,...\n"
            "     ]\n"
            "3. locktime                  (numeric, optional, default=0) Raw locktime. Non-0 value also locktime-activates inputs\n"
            "4. replaceable               (boolean, optional, default=false) Marks this transaction as BIP125 replaceable.\n"
            "                             Allows this transaction to be replaced by a transaction with higher fees. If provided, it is an error if explicit sequence numbers are incompatible.\n"
            //"5. \"fundfrombalance\"       (string, optional, default=none) Fund transaction from standard, blinded or anon balance.\n"
            "\nResult:\n"
            "{\n"
            "  \"transaction\"      (string) hex string of the transaction\n"
            "  \"amounts\"          (json) Coin values of outputs with blinding factors of blinded outputs.\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("createrawparttransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"address\\\":0.01}\"")
            + HelpExampleCli("createrawparttransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\" \"{\\\"data\\\":\\\"00010203\\\"}\"")
            + HelpExampleRpc("createrawparttransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"address\\\":0.01}\"")
            + HelpExampleRpc("createrawparttransaction", "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0}]\", \"{\\\"data\\\":\\\"00010203\\\"}\"")
        );

    EnsureWalletIsUnlocked(pwallet);

    RPCTypeCheck(request.params, {UniValue::VARR, UniValue::VARR, UniValue::VNUM, UniValue::VBOOL, UniValue::VSTR}, true);
    if (request.params[0].isNull() || request.params[1].isNull())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, arguments 1 and 2 must be non-null");

    UniValue inputs = request.params[0].get_array();
    UniValue outputs = request.params[1].get_array();

    CMutableTransaction rawTx;
    rawTx.nVersion = PARTICL_TXN_VERSION;


    if (!request.params[2].isNull()) {
        int64_t nLockTime = request.params[2].get_int64();
        if (nLockTime < 0 || nLockTime > std::numeric_limits<uint32_t>::max())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, locktime out of range");
        rawTx.nLockTime = nLockTime;
    }

    bool rbfOptIn = request.params[3].isTrue();

    std::map<int, uint256> mInputBlinds;
    for (unsigned int idx = 0; idx < inputs.size(); idx++) {
        const UniValue& input = inputs[idx];
        const UniValue& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const UniValue& vout_v = find_value(o, "vout");
        if (!vout_v.isNum())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        uint32_t nSequence;
        if (rbfOptIn) {
            nSequence = MAX_BIP125_RBF_SEQUENCE;
        } else if (rawTx.nLockTime) {
            nSequence = std::numeric_limits<uint32_t>::max() - 1;
        } else {
            nSequence = std::numeric_limits<uint32_t>::max();
        }

        // set the sequence number if passed in the parameters object
        const UniValue& sequenceObj = find_value(o, "sequence");
        if (sequenceObj.isNum()) {
            int64_t seqNr64 = sequenceObj.get_int64();
            if (seqNr64 < 0 || seqNr64 > std::numeric_limits<uint32_t>::max()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, sequence number is out of range");
            } else {
                nSequence = (uint32_t)seqNr64;
            }
        }

        const UniValue &blindObj = find_value(o, "blindingfactor");
        if (blindObj.isStr()) {
            std::string s = blindObj.get_str();
            if (!IsHex(s) || !(s.size() == 64))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Blinding factor must be 32 bytes and hex encoded.");

            uint256 blind;
            blind.SetHex(s);
            mInputBlinds[rawTx.vin.size()] = blind;
        }

        CTxIn in(COutPoint(txid, nOutput), CScript(), nSequence);

        rawTx.vin.push_back(in);
    }

    std::vector<CTempRecipient> vecSend;
    for (size_t idx = 0; idx < outputs.size(); idx++) {
        const UniValue &o = outputs[idx].get_obj();
        CTempRecipient r;

        uint8_t nType = OUTPUT_STANDARD;
        const UniValue &typeObj = find_value(o, "type");
        if (typeObj.isStr()) {
            std::string s = typeObj.get_str();
            if (s == "standard")
            {
                nType = OUTPUT_STANDARD;
            } else
            if (s == "blind")
            {
                nType = OUTPUT_CT;
            } else
            if (s == "anon")
            {
                nType = OUTPUT_RINGCT;
            } else
            if (s == "data")
            {
                nType = OUTPUT_DATA;
            } else
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown output type.");
            };
        };

        CAmount nAmount = AmountFromValue(o["amount"]);

        bool fSubtractFeeFromAmount = false;
        //if (o.exists("subfee"))
        //    fSubtractFeeFromAmount = obj["subfee"].get_bool();

        if (o["pubkey"].isStr())
        {
            std::string s = o["pubkey"].get_str();
            if (!IsHex(s) || !(s.size() == 66))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Public key must be 33 bytes and hex encoded.");
            std::vector<uint8_t> v = ParseHex(s);
            r.pkTo = CPubKey(v.begin(), v.end());
        };
        if (o["ephemeral_key"].isStr())
        {
            std::string s = o["ephemeral_key"].get_str();
            if (!IsHex(s) || !(s.size() == 64))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "\"ephemeral_key\" must be 32 bytes and hex encoded.");
            std::vector<uint8_t> v = ParseHex(s);
            r.sEphem.Set(v.data(), true);
        };
        if (o["nonce"].isStr())
        {
            std::string s = o["nonce"].get_str();
            if (!IsHex(s) || !(s.size() == 64))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "\"nonce\" must be 32 bytes and hex encoded.");
            std::vector<uint8_t> v = ParseHex(s);
            r.nonce.SetHex(s);
            r.fNonceSet = true;
        };

        if (o["data"].isStr())
        {
            std::string s = o["data"].get_str();
            if (!IsHex(s))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "\"data\" must be hex encoded.");
            r.vData = ParseHex(s);
        };

        if (o["address"].isStr() && o["script"].isStr())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Can't specify both \"address\" and \"script\".");

        if (o["address"].isStr())
        {
            CTxDestination dest = DecodeDestination(o["address"].get_str());
            if (!IsValidDestination(dest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            r.address = dest;
        };

        if (o["script"].isStr())
        {
            r.scriptPubKey = ParseScript(o["script"].get_str());
            r.fScriptSet = true;
        };


        std::string sNarr;
        if (o["narration"].isStr())
            sNarr = o["narration"].get_str();

        r.nType = nType;
        r.SetAmount(nAmount);
        r.fSubtractFeeFromAmount = fSubtractFeeFromAmount;
        //r.address = address;
        r.sNarration = sNarr;

        // Need to know the fee before calulating the blind sum
        if (r.nType == OUTPUT_CT || r.nType == OUTPUT_RINGCT)
        {
            r.vBlind.resize(32);
            if (o["blindingfactor"].isStr())
            {
                std::string s = o["blindingfactor"].get_str();
                if (!IsHex(s) || !(s.size() == 64))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Blinding factor must be 32 bytes and hex encoded.");

                uint256 blind;
                blind.SetHex(s);
                memcpy(r.vBlind.data(), blind.begin(), 32);
            } else
            {
                // Generate a random blinding factor if not provided
                GetStrongRandBytes(r.vBlind.data(), 32);
            };
        };

        vecSend.push_back(r);
    };

    std::string sError;
    // Note: wallet is only necessary when sending to  an extkey address
    if (0 != pwallet->ExpandTempRecipients(vecSend, nullptr, sError))
        throw JSONRPCError(RPC_WALLET_ERROR, strprintf("ExpandTempRecipients failed: %s.", sError));

    UniValue amounts(UniValue::VOBJ);

    CAmount nFeeRet = 0;
    //bool fFirst = true;
    for (size_t i = 0; i < vecSend.size(); ++i)
    {
        auto &r = vecSend[i];

        //r.ApplySubFee(nFeeRet, nSubtractFeeFromAmount, fFirst);

        OUTPUT_PTR<CTxOutBase> txbout;
        if (0 != CreateOutput(txbout, r, sError))
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("CreateOutput failed: %s.", sError));

        if (!CheckOutputValue(r, &*txbout, nFeeRet, sError))
            throw JSONRPCError(RPC_WALLET_ERROR, strprintf("CheckOutputValue failed: %s.", sError));
        /*
        if (r.nType == OUTPUT_STANDARD)
            nValueOutPlain += r.nAmount;

        if (r.fChange && r.nType == OUTPUT_CT)
            nChangePosInOut = i;
        */
        r.n = rawTx.vpout.size();
        rawTx.vpout.push_back(txbout);

        UniValue amount(UniValue::VOBJ);
        amount.pushKV("value", ValueFromAmount(r.nAmount));

        if (r.nType == OUTPUT_CT || r.nType == OUTPUT_RINGCT)
        {
            uint256 blind(r.vBlind.data(), 32);
            amount.pushKV("blind", blind.ToString());

            if (0 != pwallet->AddCTData(txbout.get(), r, sError))
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("AddCTData failed: %s.", sError));
            amount.pushKV("nonce", r.nonce.ToString());
        };

        if (r.nType != OUTPUT_DATA)
            amounts.pushKV(strprintf("%d", r.n), amount);
    };

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(rawTx));
    result.pushKV("amounts", amounts);

    return result;
};


static UniValue fundrawtransactionfrom(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CHDWallet *const pwallet = GetParticlWallet(wallet.get());
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() < 4 || request.params.size() > 5)
        throw std::runtime_error(
            "fundrawtransactionfrom \"input_type\" \"hexstring\" input_amounts output_amounts ( options iswitness )\n"
            "\nAdd inputs to a transaction until it has enough in value to meet its out value.\n"
            "This will not modify existing inputs, and will add at most one change output to the outputs.\n"
            "No existing outputs will be modified unless \"subtractFeeFromOutputs\" is specified.\n"
            "Note that inputs which were signed may need to be resigned after completion since in/outputs have been added.\n"
            "The inputs added will not be signed, use signrawtransaction for that.\n"
            "Note that all existing inputs must have their previous output transaction be in the wallet or have their amount and blinding factor specified in input_amounts.\n"
            /*"Note that all inputs selected must be of standard form and P2SH scripts must be\n"
            "in the wallet using importaddress or addmultisigaddress (to calculate fees).\n"
            "You can see whether this is the case by checking the \"solvable\" field in the listunspent output.\n"
            "Only pay-to-pubkey, multisig, and P2SH versions thereof are currently supported for watch-only\n"*/
            "\nArguments:\n"
            "1. \"input_type\"          (string, required) The type of inputs to use standard/anon/blind.\n"
            "2. \"hexstring\"           (string, required) The hex string of the raw transaction\n"
            "3. input_amounts         (object, required)\n"
            "   {\n"
            "       \"n\":\n"
            "         {\n"
            "           \"value\":amount\n"
            "           \"blind\":\"hex\"\n"
            "           \"witnessstack\":[\"hex\",]\n" // needed to estimate fee
            //"           \"script\":\"hex\"\n"
            //"           \"redeemscript\":\"hex\"\n"
            "         }\n"
            "   }\n"
            "4. output_amounts        (object, required)\n"
            "   {\n"
            "       \"n\":\n"
            "         {\n"
            "           \"value\":amount\n"
            //"           \"blind\":\"hex\"\n"
            "           \"nonce\":\"hex\"\n"
            "         }\n"
            "   }\n"
            "5. \"options\"             (object, optional)\n"
            "   {\n"
            "     \"changeAddress\"          (string, optional, default pool address) The particl address to receive the change\n"
            "     \"changePosition\"         (numeric, optional, default random) The index of the change output\n"
            "     \"change_type\"            (string, optional) The output type to use. Only valid if changeAddress is not specified. Options are \"legacy\", \"p2sh-segwit\", and \"bech32\". Default is set by -changetype.\n"
            "     \"includeWatching\"        (boolean, optional, default false) Also select inputs which are watch only\n"
            "     \"lockUnspents\"           (boolean, optional, default false) Lock selected unspent outputs\n"
            "     \"feeRate\"                (numeric, optional, default not set: makes wallet determine the fee) Set a specific fee rate in " + CURRENCY_UNIT + "/kB\n"
            "     \"subtractFeeFromOutputs\" (array, optional) A json array of integers.\n"
            "                              The fee will be equally deducted from the amount of each specified output.\n"
            "                              The outputs are specified by their zero-based index, before any change output is added.\n"
            "                              Those recipients will receive less coins than you enter in their corresponding amount field.\n"
            "                              If no outputs are specified here, the sender pays the fee.\n"
            "                                  [vout_index,...]\n"
            "     \"replaceable\"            (boolean, optional) Marks this transaction as BIP125 replaceable.\n"
            "     \"allow_other_inputs\"     (boolean, optional, default=true) Allow inputs to be added if any inputs already exist.\n"
            "     \"allow_change_output\"    (boolean, optional, default=true) Allow change output to be added if needed (only for 'blind' input_type).\n"
            "                              Allows this transaction to be replaced by a transaction with higher fees\n"
            "     \"conf_target\"            (numeric, optional) Confirmation target (in blocks)\n"
            "     \"estimate_mode\"          (string, optional, default=UNSET) The fee estimate mode, must be one of:\n"
            "         \"UNSET\"\n"
            "         \"ECONOMICAL\"\n"
            "         \"CONSERVATIVE\"\n"
            "   }\n"
            "\nResult:\n"
            "{\n"
            "  \"hex\":       \"value\", (string)  The resulting raw transaction (hex-encoded string)\n"
            "  \"fee\":       n,       (numeric) Fee in " + CURRENCY_UNIT + " the resulting transaction pays\n"
            "  \"changepos\": n        (numeric) The position of the added change output, or -1\n"
            "  \"output_amounts\": obj (json) Output values and blinding factors\n"
            "}\n"
            "\nExamples:\n"
            "\nCreate a transaction with no inputs\n"
            + HelpExampleCli("createrawctransaction", "\"[]\" \"{\\\"myaddress\\\":0.01}\"") +
            "\nAdd sufficient unsigned inputs to meet the output value\n"
            + HelpExampleCli("fundrawtransactionfrom", "\"blind\" \"rawtransactionhex\"") +
            "\nSign the transaction\n"
            + HelpExampleCli("signrawtransactionwithwallet", "\"fundedtransactionhex\"") +
            "\nSend the transaction\n"
            + HelpExampleCli("sendrawtransaction", "\"signedtransactionhex\"")
        );

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR, UniValue::VOBJ, UniValue::VOBJ, UniValue::VOBJ}, true);

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    std::string sInputType = request.params[0].get_str();

    if (sInputType != "standard" && sInputType != "anon" && sInputType != "blind")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown input type.");

    CCoinControl coinControl;
    int changePosition = -1;
    bool lockUnspents = false;
    UniValue subtractFeeFromOutputs;
    std::set<int> setSubtractFeeFromOutputs;

    coinControl.fAllowOtherInputs = true;

    if (request.params[4].isObject()) {
        UniValue options = request.params[4];

        RPCTypeCheckObj(options,
            {
                {"changeAddress", UniValueType(UniValue::VSTR)},
                {"changePosition", UniValueType(UniValue::VNUM)},
                {"change_type", UniValueType(UniValue::VSTR)},
                {"includeWatching", UniValueType(UniValue::VBOOL)},
                {"lockUnspents", UniValueType(UniValue::VBOOL)},
                {"feeRate", UniValueType()}, // will be checked below
                {"subtractFeeFromOutputs", UniValueType(UniValue::VARR)},
                {"replaceable", UniValueType(UniValue::VBOOL)},
                {"allow_other_inputs", UniValueType(UniValue::VBOOL)},
                {"allow_change_output", UniValueType(UniValue::VBOOL)},
                {"conf_target", UniValueType(UniValue::VNUM)},
                {"estimate_mode", UniValueType(UniValue::VSTR)},
            },
            true, true);

        if (options.exists("changeAddress")) {
            CTxDestination dest = DecodeDestination(options["changeAddress"].get_str());

            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "changeAddress must be a valid particl address");
            }

            coinControl.destChange = dest;
        }

        if (options.exists("changePosition"))
            changePosition = options["changePosition"].get_int();

        if (options.exists("change_type")) {
            if (options.exists("changeAddress")) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both changeAddress and address_type options");
            }
            coinControl.m_change_type = pwallet->m_default_change_type;
            if (!ParseOutputType(options["change_type"].get_str(), *coinControl.m_change_type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Unknown change type '%s'", options["change_type"].get_str()));
            }
        }

        if (options.exists("includeWatching"))
            coinControl.fAllowWatchOnly = options["includeWatching"].get_bool();

        if (options.exists("lockUnspents"))
            lockUnspents = options["lockUnspents"].get_bool();

        if (options.exists("feeRate"))
        {
            coinControl.m_feerate = CFeeRate(AmountFromValue(options["feeRate"]));
            coinControl.fOverrideFeeRate = true;
        }

        if (options.exists("subtractFeeFromOutputs"))
            subtractFeeFromOutputs = options["subtractFeeFromOutputs"].get_array();

        if (options.exists("replaceable")) {
            coinControl.m_signal_bip125_rbf = options["replaceable"].get_bool();
        }
        if (options.exists("allow_other_inputs")) {
            coinControl.fAllowOtherInputs = options["allow_other_inputs"].get_bool();
        }
        if (options.exists("allow_change_output")) {
            coinControl.m_addChangeOutput = options["allow_change_output"].get_bool();
        }
        if (options.exists("conf_target")) {
            if (options.exists("feeRate")) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both conf_target and feeRate");
            }
            coinControl.m_confirm_target = ParseConfirmTarget(options["conf_target"]);
        }
        if (options.exists("estimate_mode")) {
            if (options.exists("feeRate")) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot specify both estimate_mode and feeRate");
            }
            if (!FeeModeFromString(options["estimate_mode"].get_str(), coinControl.m_fee_mode)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid estimate_mode parameter");
            }
        }
    }

    // parse hex string from parameter
    CMutableTransaction tx;
    tx.nVersion = PARTICL_TXN_VERSION;
    if (!DecodeHexTx(tx, request.params[1].get_str(), true)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    size_t nOutputs = tx.GetNumVOuts();
    if (nOutputs == 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "TX must have at least one output");

    if (changePosition != -1 && (changePosition < 0 || (unsigned int)changePosition > nOutputs))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "changePosition out of bounds");
    coinControl.nChangePos = changePosition;

    for (unsigned int idx = 0; idx < subtractFeeFromOutputs.size(); idx++) {
        int pos = subtractFeeFromOutputs[idx].get_int();
        if (setSubtractFeeFromOutputs.count(pos))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, duplicated position: %d", pos));
        if (pos < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, negative position: %d", pos));
        if (pos >= int(nOutputs))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid parameter, position too large: %d", pos));
        setSubtractFeeFromOutputs.insert(pos);
    }

    UniValue inputAmounts = request.params[2];
    UniValue outputAmounts = request.params[3];
    std::map<int, uint256> mInputBlinds, mOutputBlinds;
    std::map<int, CAmount> mOutputAmounts;

    std::vector<CTempRecipient> vecSend(nOutputs);

    const std::vector<std::string> &vInputKeys = inputAmounts.getKeys();
    pwallet->mapTempRecords.clear();
    for (const std::string &sKey : vInputKeys)
    {
        int64_t n;
        if (!ParseInt64(sKey, &n) || n >= (int64_t)tx.vin.size() || n < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Bad index for input blinding factor.");

        CInputData im;
        COutputRecord r;
        r.nType = OUTPUT_STANDARD;

        if (tx.vin[n].prevout.n >= OR_PLACEHOLDER_N)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Input offset too large for output record.");
        r.n = tx.vin[n].prevout.n;

        uint256 blind;
        if (inputAmounts[sKey]["blind"].isStr())
        {
            std::string s = inputAmounts[sKey]["blind"].get_str();
            if (!IsHex(s) || !(s.size() == 64))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Blinding factor must be 32 bytes and hex encoded.");

            blind.SetHex(s);
            mInputBlinds[n] = blind;
            r.nType = OUTPUT_CT;
        };

        if (inputAmounts[sKey]["value"].isNull())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing 'value' for input.");

        r.nValue = AmountFromValue(inputAmounts[sKey]["value"]);

        if (inputAmounts[sKey]["witnessstack"].isArray())
        {
            const UniValue &stack = inputAmounts[sKey]["witnessstack"].get_array();

            for (size_t k = 0; k < stack.size(); ++k)
            {
                if (!stack[k].isStr())
                    continue;
                std::string s = stack.get_str();
                if (!IsHex(s))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Input witness must be hex encoded.");
                std::vector<uint8_t> v = ParseHex(s);
                im.scriptWitness.stack.push_back(v);
            };
        };

        //r.scriptPubKey = ; // TODO
        std::pair<MapRecords_t::iterator, bool> ret = pwallet->mapTempRecords.insert(std::make_pair(tx.vin[n].prevout.hash, CTransactionRecord()));
        ret.first->second.InsertOutput(r);

        im.nValue = r.nValue;
        im.blind = blind;

        coinControl.m_inputData[tx.vin[n].prevout] = im;
    };

    const std::vector<std::string> &vOutputKeys = outputAmounts.getKeys();
    for (const std::string &sKey : vOutputKeys)
    {
        int64_t n;
        if (!ParseInt64(sKey, &n) || n >= (int64_t)tx.GetNumVOuts() || n < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Bad index for output blinding factor.");

        const auto &txout = tx.vpout[n];

        if (!outputAmounts[sKey]["value"].isNull())
            mOutputAmounts[n] = AmountFromValue(outputAmounts[sKey]["value"]);

        if (outputAmounts[sKey]["nonce"].isStr()
            && txout->GetPRangeproof())
        {
            CTempRecipient &r = vecSend[n];
            std::string s = outputAmounts[sKey]["nonce"].get_str();
            if (!IsHex(s) || !(s.size() == 64))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Nonce must be 32 bytes and hex encoded.");

            r.fNonceSet = true;
            r.nonce.SetHex(s);

            uint64_t min_value, max_value;
            uint8_t blindOut[32];
            unsigned char msg[256]; // Currently narration is capped at 32 bytes
            size_t mlen = sizeof(msg);
            memset(msg, 0, mlen);
            uint64_t amountOut;
            if (1 != secp256k1_rangeproof_rewind(secp256k1_ctx_blind,
                blindOut, &amountOut, msg, &mlen, r.nonce.begin(),
                &min_value, &max_value,
                txout->GetPCommitment(), txout->GetPRangeproof()->data(), txout->GetPRangeproof()->size(),
                nullptr, 0,
                secp256k1_generator_h))
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("secp256k1_rangeproof_rewind failed, output %d.", n));
            uint256 blind;
            memcpy(blind.begin(), blindOut, 32);

            mOutputBlinds[n] = blind;
            mOutputAmounts[n] = amountOut;

            msg[mlen-1] = '\0';
            size_t nNarr = strlen((const char*)msg);
            if (nNarr > 0)
                r.sNarration.assign((const char*)msg, nNarr);
        } else
        {
            if (txout->GetPRangeproof())
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Missing nonce for output %d.", n));
        };
        /*
        if (outputAmounts[sKey]["blind"].isStr())
        {
            std::string s = outputAmounts[sKey]["blind"].get_str();
            if (!IsHex(s) || !(s.size() == 64))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Blinding factor must be 32 bytes and hex encoded.");

            uint256 blind;
            blind.SetHex(s);
            mOutputBlinds[n] = blind;
        };
        */
        vecSend[n].SetAmount(mOutputAmounts[n]);
    };

    CAmount nTotalOutput = 0;

    for (size_t i = 0; i < tx.vpout.size(); ++i)
    {
        const auto &txout = tx.vpout[i];
        CTempRecipient &r = vecSend[i];

        if (txout->IsType(OUTPUT_CT) || txout->IsType(OUTPUT_RINGCT))
        {
            // Check commitment matches
            std::map<int, CAmount>::iterator ita = mOutputAmounts.find(i);
            std::map<int, uint256>::iterator itb = mOutputBlinds.find(i);

            if (ita == mOutputAmounts.end())
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Missing amount for blinded output %d.", i));
            if (itb == mOutputBlinds.end())
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Missing blinding factor for blinded output %d.", i));

            secp256k1_pedersen_commitment commitment;
            if (!secp256k1_pedersen_commit(secp256k1_ctx_blind,
                &commitment, (const uint8_t*)(itb->second.begin()),
                ita->second, secp256k1_generator_h))
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("secp256k1_pedersen_commit failed, output %d.", i));

            if (memcmp(txout->GetPCommitment()->data, commitment.data, 33) != 0)
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("Bad blinding factor, output %d.", i));
            nTotalOutput += mOutputAmounts[i];

            r.vBlind.resize(32);
            memcpy(r.vBlind.data(), itb->second.begin(), 32);
        } else
        if (txout->IsType(OUTPUT_STANDARD))
        {
            mOutputAmounts[i] = txout->GetValue();
            nTotalOutput += mOutputAmounts[i];
        };


        r.nType = txout->GetType();
        if (txout->IsType(OUTPUT_DATA))
        {
            r.vData = ((CTxOutData*)txout.get())->vData;
        } else
        {
            r.SetAmount(mOutputAmounts[i]);
            r.fSubtractFeeFromAmount = setSubtractFeeFromOutputs.count(i);

            if (txout->IsType(OUTPUT_CT))
                r.vData = ((CTxOutCT*)txout.get())->vData;
            else
            if (txout->IsType(OUTPUT_RINGCT))
                r.vData = ((CTxOutRingCT*)txout.get())->vData;

            if (txout->GetPScriptPubKey())
            {
                r.fScriptSet = true;
                r.scriptPubKey = *txout->GetPScriptPubKey();
            };
        };
    };

    for (const CTxIn& txin : tx.vin) {
        coinControl.Select(txin.prevout);
    };


    CTransactionRef tx_new;
    CWalletTx wtx(pwallet, tx_new);
    CTransactionRecord rtx;
    CAmount nFee;
    std::string sError;
    {
        LOCK2(cs_main, pwallet->cs_wallet);
        if (sInputType == "standard")
        {
            if (0 != pwallet->AddStandardInputs(wtx, rtx, vecSend, false, nFee, &coinControl, sError))
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("AddStandardInputs failed: %s.", sError));
        } else
        if (sInputType == "anon")
        {
            sError = "TODO";
            //if (0 != pwallet->AddAnonInputs(wtx, rtx, vecSend, false, nFee, &coinControl, sError))
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("AddAnonInputs failed: %s.", sError));
        } else
        if (sInputType == "blind")
        {
            if (0 != pwallet->AddBlindedInputs(wtx, rtx, vecSend, false, nFee, &coinControl, sError))
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("AddBlindedInputs failed: %s.", sError));
        } else
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown input type.");
        };
    }

    tx.vpout = wtx.tx->vpout;
    // keep existing sequences
    for (const auto &txin : wtx.tx->vin)
    {
        if (!coinControl.IsSelected(txin.prevout))
        {
            tx.vin.push_back(txin);
        };
        if (lockUnspents)
        {
            LOCK2(cs_main, pwallet->cs_wallet);
            pwallet->LockCoin(txin.prevout);
        };
    };


    UniValue outputValues(UniValue::VOBJ);
    for (size_t i = 0; i < vecSend.size(); ++i)
    {
        auto &r = vecSend[i];

        UniValue outputValue(UniValue::VOBJ);
        if (r.vBlind.size() == 32)
        {
            uint256 blind(r.vBlind.data(), 32);
            outputValue.pushKV("blind", blind.ToString());
        };
        if (r.nType != OUTPUT_DATA)
        {
            outputValue.pushKV("value", ValueFromAmount(r.nAmount));
            outputValues.pushKV(strprintf("%d", r.n), outputValue);
        };
    };

    pwallet->mapTempRecords.clear();

    UniValue result(UniValue::VOBJ);
    result.pushKV("hex", EncodeHexTx(tx));
    result.pushKV("fee", ValueFromAmount(nFee));
    result.pushKV("changepos", coinControl.nChangePos);
    result.pushKV("output_amounts", outputValues);

    return result;
};

static UniValue verifycommitment(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            "verifycommitment \"commitment\" \"blind\" amount\n"
            "\nVerify value commitment.\n"

            "\nArguments:\n"
            "1. \"commitment\"                     (string, required) The 33byte commitment hex string\n"
            "2. \"blind\"                          (string, required) The 32byte blinding factor hex string\n"
            "3. amount                           (numeric or string, required) The amount committed to\n"
            "\nResult:\n"
            "{\n"
            "  \"result\": true,                   (boolean) If valid commitment, else throw error.\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("verifycommitment", "\"commitment\" \"blind\" 1.1")
            + HelpExampleRpc("verifycommitment", "\"commitment\", \"blind\", 1.1")
        );

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR});

    std::vector<uint8_t> vchCommitment;
    uint256 blind;

    {
        std::string s = request.params[0].get_str();
        if (!IsHex(s) || !(s.size() == 66))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "commitment must be 33 bytes and hex encoded.");
        vchCommitment = ParseHex(s);
    }

    {
        std::string s = request.params[1].get_str();
        if (!IsHex(s) || !(s.size() == 64))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Blinding factor must be 32 bytes and hex encoded.");
        blind.SetHex(s);
    };

    CAmount nValue = AmountFromValue(request.params[2]);

    secp256k1_pedersen_commitment commitment;
    if (!secp256k1_pedersen_commit(secp256k1_ctx_blind,
        &commitment, blind.begin(),
        nValue, secp256k1_generator_h))
        throw JSONRPCError(RPC_MISC_ERROR, strprintf("secp256k1_pedersen_commit failed."));

    if (memcmp(vchCommitment.data(), commitment.data, 33) != 0)
        throw JSONRPCError(RPC_MISC_ERROR, strprintf("Mismatched commitment."));


    UniValue result(UniValue::VOBJ);
    bool rv = true;
    result.pushKV("result", rv);
    return result;
};

static UniValue verifyrawtransaction(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
        throw std::runtime_error(
            "verifyrawtransaction \"hexstring\" ( [{\"txid\":\"id\",\"vout\":n,\"scriptPubKey\":\"hex\",\"redeemScript\":\"hex\"},...] returndecoded )\n"
            "\nVerify inputs for raw transaction (serialized, hex-encoded).\n"
            "The second optional argument (may be null) is an array of previous transaction outputs that\n"
            "this transaction depends on but may not yet be in the block chain.\n"
            "\nArguments:\n"
            "1. \"hexstring\"                      (string, required) The transaction hex string\n"
            "2. \"prevtxs\"                        (string, optional) An json array of previous dependent transaction outputs\n"
            "     [                              (json array of json objects, or 'null' if none provided)\n"
            "       {\n"
            "         \"txid\":\"id\",               (string, required) The transaction id\n"
            "         \"vout\":n,                  (numeric, required) The output number\n"
            "         \"scriptPubKey\": \"hex\",     (string, required) script key\n"
            //"         \"redeemScript\": \"hex\",     (string, required for P2SH or P2WSH) redeem script\n"
            "         \"amount\": value            (numeric, required) The amount spent\n"
            "         \"amount_commitment\": \"hex\",(string, required) The amount commitment spent\n"
            "       }\n"
            "       ,...\n"
            "    ]\n"
            "3. returndecoded                     (bool, optional) Return the decoded txn as a json object\n"
            "\nResult:\n"
            "{\n"
            "  \"complete\" : true|false,          (boolean) If the transaction has a complete set of signatures\n"
            "  \"errors\" : [                      (json array of objects) Script verification errors (if there are any)\n"
            "    {\n"
            "      \"txid\" : \"hash\",              (string) The hash of the referenced, previous transaction\n"
            "      \"vout\" : n,                   (numeric) The index of the output to spent and used as input\n"
            "      \"scriptSig\" : \"hex\",          (string) The hex-encoded signature script\n"
            "      \"sequence\" : n,               (numeric) Script sequence number\n"
            "      \"error\" : \"text\"              (string) Verification or signing error related to the input\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("verifyrawtransaction", "\"myhex\"")
            + HelpExampleRpc("verifyrawtransaction", "\"myhex\"")
        );

    // TODO: verify amounts / commitment sum

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VBOOL}, true);

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str(), true)) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK2(cs_main, mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : mtx.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    // Add previous txouts given in the RPC call:
    if (!request.params[1].isNull()) {
        UniValue prevTxs = request.params[1].get_array();
        for (unsigned int idx = 0; idx < prevTxs.size(); ++idx) {
            const UniValue& p = prevTxs[idx];
            if (!p.isObject()) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");
            }

            UniValue prevOut = p.get_obj();

            RPCTypeCheckObj(prevOut,
                {
                    {"txid", UniValueType(UniValue::VSTR)},
                    {"vout", UniValueType(UniValue::VNUM)},
                    {"scriptPubKey", UniValueType(UniValue::VSTR)},
                });

            uint256 txid = ParseHashO(prevOut, "txid");

            int nOut = find_value(prevOut, "vout").get_int();
            if (nOut < 0) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");
            }

            COutPoint out(txid, nOut);
            std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            {
            const Coin& coin = view.AccessCoin(out);

            if (coin.nType != OUTPUT_STANDARD && coin.nType != OUTPUT_CT)
                throw JSONRPCError(RPC_MISC_ERROR, strprintf("Bad input type: %d", coin.nType));

            if (!coin.IsSpent() && coin.out.scriptPubKey != scriptPubKey) {
                std::string err("Previous output scriptPubKey mismatch:\n");
                err = err + ScriptToAsmStr(coin.out.scriptPubKey) + "\nvs:\n"+
                    ScriptToAsmStr(scriptPubKey);
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, err);
            }
            Coin newcoin;
            newcoin.out.scriptPubKey = scriptPubKey;
            newcoin.out.nValue = 0;
            if (prevOut.exists("amount")) {
                if (prevOut.exists("amount_commitment"))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Both \"amount\" and \"amount_commitment\" found.");
                newcoin.nType = OUTPUT_STANDARD;
                newcoin.out.nValue = AmountFromValue(find_value(prevOut, "amount"));
            } else
            if (prevOut.exists("amount_commitment")) {
                std::string s = prevOut["amount_commitment"].get_str();
                if (!IsHex(s) || !(s.size() == 66))
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "\"amount_commitment\" must be 33 bytes and hex encoded.");
                std::vector<uint8_t> vchCommitment = ParseHex(s);
                assert(vchCommitment.size() == 33);
                memcpy(newcoin.commitment.data, vchCommitment.data(), 33);
                newcoin.nType = OUTPUT_CT;
            } else
            {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "\"amount\" or \"amount_commitment\" is required");
            };

            newcoin.nHeight = 1;
            view.AddCoin(out, std::move(newcoin), true);
            }
        }
    }


    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mtx);

    // Script verification errors
    UniValue vErrors(UniValue::VARR);


    int nSpendHeight = 0; // TODO: make option
    {
        LOCK(cs_main);
        nSpendHeight = chainActive.Tip()->nHeight;
    }

    {
        CValidationState state;
        CAmount nFee = 0;
        if (!Consensus::CheckTxInputs(txConst, state, view, nSpendHeight, nFee))
            vErrors.push_back("CheckTxInputs: \"" + state.GetRejectReason() + "\"");
    }

    // Sign what we can:
    for (unsigned int i = 0; i < mtx.vin.size(); i++) {
        CTxIn& txin = mtx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            TxInErrorToJSON(txin, vErrors, "Input not found or already spent");
            continue;
        }

        CScript prevPubKey = coin.out.scriptPubKey;

        std::vector<uint8_t> vchAmount;
        if (coin.nType == OUTPUT_STANDARD)
        {
            vchAmount.resize(8);
            memcpy(vchAmount.data(), &coin.out.nValue, 8);
        } else
        if (coin.nType == OUTPUT_CT)
        {
            vchAmount.resize(33);
            memcpy(vchAmount.data(), coin.commitment.data, 33);
        } else
        {
            throw JSONRPCError(RPC_MISC_ERROR, strprintf("Bad input type: %d", coin.nType));
        };

        ScriptError serror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, i, vchAmount), &serror)) {
            TxInErrorToJSON(txin, vErrors, ScriptErrorString(serror));
        }
    }
    bool fComplete = vErrors.empty();

    UniValue result(UniValue::VOBJ);

    if (!request.params[2].isNull() && request.params[2].get_bool())
    {
        UniValue txn(UniValue::VOBJ);
        TxToUniv(CTransaction(std::move(mtx)), uint256(), txn, false);
        result.pushKV("txn", txn);
    };

    //result.pushKV("hex", EncodeHexTx(mtx));
    result.pushKV("complete", fComplete);
    if (!vErrors.empty()) {
        result.pushKV("errors", vErrors);
    }

    return result;
};

static const CRPCCommand commands[] =
{ //  category              name                                actor (function)                argNames
  //  --------------------- ------------------------            -----------------------         ----------
    { "wallet",             "extkey",                           &extkey,                        {} },
    { "wallet",             "extkeyimportmaster",               &extkeyimportmaster,            {"source","passphrase","save_bip44_root","master_label","account_label","scan_chain_from"} }, // import, set as master, derive account, set default account, force users to run mnemonic new first make them copy the key
    { "wallet",             "extkeygenesisimport",              &extkeygenesisimport,           {"source","passphrase","save_bip44_root","master_label","account_label","scan_chain_from"} },
    { "wallet",             "extkeyaltversion",                 &extkeyaltversion,              {"ext_key"} },
    { "wallet",             "getnewextaddress",                 &getnewextaddress,              {"label","childNo","bech32","hardened"} },
    { "wallet",             "getnewstealthaddress",             &getnewstealthaddress,          {"label","num_prefix_bits","prefix_num","bech32","makeV2"} },
    { "wallet",             "importstealthaddress",             &importstealthaddress,          {"scan_secret","spend_secret","label","num_prefix_bits","prefix_num","bech32"} },
    { "wallet",             "liststealthaddresses",             &liststealthaddresses,          {"show_secrets"} },

    { "wallet",             "scanchain",                        &scanchain,                     {"from_height"} },
    { "wallet",             "reservebalance",                   &reservebalance,                {"enabled","amount"} },
    { "wallet",             "deriverangekeys",                  &deriverangekeys,               {"start","end","key/id","hardened","save","add_to_addressbook","256bithash"} },
    { "wallet",             "clearwallettransactions",          &clearwallettransactions,       {"remove_all"} },

    { "wallet",             "filtertransactions",               &filtertransactions,            {"options"} },
    { "wallet",             "filteraddresses",                  &filteraddresses,               {"offset","count","sort_code"} },
    { "wallet",             "manageaddressbook",                &manageaddressbook,             {"action","address","label","purpose"} },

    { "wallet",             "getstakinginfo",                   &getstakinginfo,                {} },
    { "wallet",             "getcoldstakinginfo",               &getcoldstakinginfo,            {} },

    { "wallet",             "listunspentanon",                  &listunspentanon,               {"minconf","maxconf","addresses","include_unsafe","query_options"} },
    { "wallet",             "listunspentblind",                 &listunspentblind,              {"minconf","maxconf","addresses","include_unsafe","query_options"} },


    //sendparttopart // normal txn
    { "wallet",             "sendparttoblind",                  &sendparttoblind,               {"address","amount","comment","comment_to","subtractfeefromamount","narration"} },
    { "wallet",             "sendparttoanon",                   &sendparttoanon,                {"address","amount","comment","comment_to","subtractfeefromamount","narration"} },

    { "wallet",             "sendblindtopart",                  &sendblindtopart,               {"address","amount","comment","comment_to","subtractfeefromamount","narration"} },
    { "wallet",             "sendblindtoblind",                 &sendblindtoblind,              {"address","amount","comment","comment_to","subtractfeefromamount","narration"} },
    { "wallet",             "sendblindtoanon",                  &sendblindtoanon,               {"address","amount","comment","comment_to","subtractfeefromamount","narration"} },

    { "wallet",             "sendanontopart",                   &sendanontopart,                {"address","amount","comment","comment_to","subtractfeefromamount","narration","ringsize","inputs_per_sig"} },
    { "wallet",             "sendanontoblind",                  &sendanontoblind,               {"address","amount","comment","comment_to","subtractfeefromamount","narration","ringsize","inputs_per_sig"} },
    { "wallet",             "sendanontoanon",                   &sendanontoanon,                {"address","amount","comment","comment_to","subtractfeefromamount","narration","ringsize","inputs_per_sig"} },

    { "wallet",             "sendtypeto",                       &sendtypeto,                    {"typein","typeout","outputs","comment","comment_to","ringsize","inputs_per_sig","test_fee","coincontrol"} },



    { "wallet",             "createsignaturewithwallet",        &createsignaturewithwallet,     {"hexstring","prevtx","address","sighashtype"} },
    { "rawtransactions",    "createsignaturewithkey",           &createsignaturewithkey,        {"hexstring","prevtx","privkey","sighashtype"} },

    { "wallet",             "debugwallet",                      &debugwallet,                   {"attempt_repair"} },
    { "wallet",             "rewindchain",                      &rewindchain,                   {"height"} },

    { "wallet",             "walletsettings",                   &walletsettings,                {"setting","json"} },

    { "wallet",             "transactionblinds",                &transactionblinds,             {"txnid"} },
    { "wallet",             "derivefromstealthaddress",         &derivefromstealthaddress,      {"stealthaddress","ephempubkey"} },


    { "governance",         "setvote",                          &setvote,                       {"proposal","option","height_start","height_end"} },
    { "governance",         "votehistory",                      &votehistory,                   {"current_only"} },
    { "governance",         "tallyvotes",                       &tallyvotes,                    {"proposal","height_start","height_end"} },

    { "rawtransactions",    "buildscript",                      &buildscript,                   {"json"} },
    { "rawtransactions",    "createrawparttransaction",         &createrawparttransaction,      {"inputs","outputs","locktime","replaceable"} },
    { "rawtransactions",    "fundrawtransactionfrom",           &fundrawtransactionfrom,        {"input_type","hexstring","input_amounts","output_amounts","options"} },
    { "rawtransactions",    "verifycommitment",                 &verifycommitment,              {"commitment","blind","amount"} },
    { "rawtransactions",    "verifyrawtransaction",             &verifyrawtransaction,          {"hexstring","prevtxs","returndecoded"} },
};

void RegisterHDWalletRPCCommands(CRPCTable &t)
{
    if (gArgs.GetBoolArg("-disablewallet", false))
        return;

    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
