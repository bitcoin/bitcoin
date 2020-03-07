// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "legacysigner.h"
#include "main.h"
#include "init.h"
#include "util.h"
#include "masternodeman.h"
#include "script/sign.h"
#include "instantx.h"
#include "ui_interface.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>
#include <boost/assign/list_of.hpp>
#include <openssl/rand.h>

using namespace std;
using namespace boost;

// A helper object for signing messages from Masternodes
CLegacySigner legacySigner;

// Keep track of the active Masternode
CActiveMasternode activeMasternode;

bool CLegacySigner::SetCollateralAddress(std::string strAddress){
    CBitcoinAddress address;
    if (!address.SetString(strAddress))
    {
        LogPrintf("CLegacySigner::SetCollateralAddress - Invalid collateral address\n");
        return false;
    }
    collateralPubKey = GetScriptForDestination(address.Get());
    return true;
}

bool CLegacySigner::IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey, int value){
    CScript payee2;
    payee2 = GetScriptForDestination(pubkey.GetID());

    CTransaction txVin;
    uint256 hash;
    if(GetTransaction(vin.prevout.hash, txVin, hash, true)){
        BOOST_FOREACH(CTxOut out, txVin.vout){
            if(out.nValue == value*COIN){
                if(out.scriptPubKey == payee2) return true;
            }
        }
    }

    return false;
}

bool CLegacySigner::SetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey){
    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood) {
        errorMessage = _("Invalid private key.");
        return false;
    }

    key = vchSecret.GetKey();
    pubkey = key.GetPubKey();

    return true;
}

bool CLegacySigner::SignMessage(std::string strMessage, std::string& errorMessage, vector<unsigned char>& vchSig, CKey key)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    if (!key.SignCompact(ss.GetHash(), vchSig)) {
        errorMessage = _("Signing failed.");
        return false;
    }

    return true;
}

bool CLegacySigner::VerifyMessage(CPubKey pubkey, const vector<unsigned char>& vchSig, std::string strMessage, std::string& errorMessage)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey2;
    if (!pubkey2.RecoverCompact(ss.GetHash(), vchSig)) {
        errorMessage = _("Error recovering public key.");
        return false;
    }

    if (pubkey2.GetID() != pubkey.GetID()) {
        errorMessage = strprintf("keys don't match - input: %s, recovered: %s, message: %s, sig: %s\n",
                    pubkey.GetID().ToString(), pubkey2.GetID().ToString(), strMessage,
                    EncodeBase64(&vchSig[0], vchSig.size()));
        return false;
    }

    return true;
}

//TODO: Rename/move to core
void ThreadCheckLegacySigner()
{
    if(fLiteMode) return; //disable all Masternode related functionality

    // Make this thread recognisable as the wallet flushing thread
    RenameThread("crown-legacysigner");

    unsigned int c1 = 0;
    unsigned int c2 = 0;

    while (true)
    {
        MilliSleep(1000);
        //LogPrintf("ThreadCheckLegacySigner::check timeout\n");

        // try to sync from all available nodes, one step at a time
        masternodeSync.Process();
        systemnodeSync.Process();

        if(masternodeSync.IsBlockchainSynced()) {

            c1++;

            // check if we should activate or ping every few minutes,
            // start right after sync is considered to be done
            if(c1 % MASTERNODE_PING_SECONDS == 15) activeMasternode.ManageStatus();

            if(c1 % 60 == 0)
            {
                mnodeman.CheckAndRemove();
                mnodeman.ProcessMasternodeConnections();
                masternodePayments.CheckAndRemove();
                GetInstantSend().CheckAndRemove();
            }

            //if(c1 % MASTERNODES_DUMP_SECONDS == 0) DumpMasternodes();
        }

        if(systemnodeSync.IsBlockchainSynced()) {

            c2++;

            // check if we should activate or ping every few minutes,
            // start right after sync is considered to be done
            if(c2 % SYSTEMNODE_PING_SECONDS == 15) activeSystemnode.ManageStatus();

            if(c2 % 60 == 0)
            {
                snodeman.CheckAndRemove();
                snodeman.ProcessSystemnodeConnections();
                systemnodePayments.CheckAndRemove();
                GetInstantSend().CheckAndRemove();
            }

            //if(c2 % SYSTEMNODES_DUMP_SECONDS == 0) DumpSystemnodes();
        }

    }
}

bool CHashSigner::SignHash(const uint256& hash, const CKey& key, std::vector<unsigned char>& vchSigRet)
{
    return key.SignCompact(hash, vchSigRet);
}

bool CHashSigner::VerifyHash(const uint256& hash, const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, std::string& strErrorRet)
{
    return VerifyHash(hash, pubkey.GetID(), vchSig, strErrorRet);
}

bool CHashSigner::VerifyHash(const uint256& hash, const CKeyID& keyID, const std::vector<unsigned char>& vchSig, std::string& strErrorRet)
{
    CPubKey pubkeyFromSig;
    if(!pubkeyFromSig.RecoverCompact(hash, vchSig)) {
        strErrorRet = "Error recovering public key.";
        return false;
    }

    if(pubkeyFromSig.GetID() != keyID) {
        strErrorRet = strprintf("Keys don't match: pubkey=%s, pubkeyFromSig=%s, hash=%s, vchSig=%s",
                                keyID.ToString(), pubkeyFromSig.GetID().ToString(), hash.ToString(),
                                EncodeBase64(&vchSig[0], vchSig.size()));
        return false;
    }

    return true;
}

