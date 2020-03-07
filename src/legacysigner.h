// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LEGACYSIGNER_H
#define LEGACYSIGNER_H

#include "main.h"
#include "sync.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "activesystemnode.h"
#include "systemnodeman.h"
#include "systemnode-payments.h"
#include "systemnode-sync.h"

class CTxIn;
class CLegacySigner;
class CMasterNodeVote;
class CBitcoinAddress;
class CActiveMasternode;

// status update message constants
#define MASTERNODE_ACCEPTED                    1
#define MASTERNODE_REJECTED                    0
#define MASTERNODE_RESET                       -1

extern CLegacySigner legacySigner;
extern std::string strMasterNodePrivKey;
extern CActiveMasternode activeMasternode;

/** Helper object for signing and checking signatures
 */
class CLegacySigner
{
public:
    void InitCollateralAddress(){
        SetCollateralAddress(Params().LegacySignerDummyAddress());
    }
    bool SetCollateralAddress(std::string strAddress);
    /// Is the inputs associated with this public key? (and there is enough CRW (10000 by default for checking if valid masternode)
    bool IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey, int value=MASTERNODE_COLLATERAL);
    /// Set the private/public key values, returns true if successful
    bool SetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey);
    /// Sign the message, returns true if successful
    bool SignMessage(std::string strMessage, std::string& errorMessage, std::vector<unsigned char>& vchSig, CKey key);
    /// Verify the message, returns true if succcessful
    bool VerifyMessage(CPubKey pubkey, const std::vector<unsigned char>& vchSig, std::string strMessage, std::string& errorMessage);
    // where collateral should be made out to
    CScript collateralPubKey;
    CMasternode* pSubmittedToMasternode;
    CSystemnode* pSubmittedToSystemnode;
};

/** Helper class for signing hashes and checking their signatures
 */
class CHashSigner
{
public:
    /// Sign the hash, returns true if successful
    static bool SignHash(const uint256& hash, const CKey& key, std::vector<unsigned char>& vchSigRet);
    /// Verify the hash signature, returns true if succcessful
    static bool VerifyHash(const uint256& hash, const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, std::string& strErrorRet);
    /// Verify the hash signature, returns true if succcessful
    static bool VerifyHash(const uint256& hash, const CKeyID& keyID, const std::vector<unsigned char>& vchSig, std::string& strErrorRet);
};

void ThreadCheckLegacySigner();

#endif
