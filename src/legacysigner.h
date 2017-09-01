// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LEGACYSIGNER_H
#define LEGACYSIGNER_H

#include "main.h"
#include "sync.h"
#include "activethrone.h"
#include "throneman.h"
#include "throne-payments.h"
#include "throne-sync.h"

class CTxIn;
class CLegacySigner;
class CThroNeVote;
class CBitcoinAddress;
class CActiveThrone;

// status update message constants
#define MASTERNODE_ACCEPTED                    1
#define MASTERNODE_REJECTED                    0
#define MASTERNODE_RESET                       -1

extern CLegacySigner legacySigner;
extern std::string strThroNePrivKey;
extern CActiveThrone activeThrone;

/** Helper object for signing and checking signatures
 */
class CLegacySigner
{
public:
    void InitCollateralAddress(){
        SetCollateralAddress(Params().LegacySignerDummyAddress());
    }
    bool SetCollateralAddress(std::string strAddress);
    /// Is the inputs associated with this public key? (and there is 10000 CRW - checking if valid throne)
    bool IsVinAssociatedWithPubkey(CTxIn& vin, CPubKey& pubkey);
    /// Set the private/public key values, returns true if successful
    bool SetKey(std::string strSecret, std::string& errorMessage, CKey& key, CPubKey& pubkey);
    /// Sign the message, returns true if successful
    bool SignMessage(std::string strMessage, std::string& errorMessage, std::vector<unsigned char>& vchSig, CKey key);
    /// Verify the message, returns true if succcessful
    bool VerifyMessage(CPubKey pubkey, std::vector<unsigned char>& vchSig, std::string strMessage, std::string& errorMessage);
    // where collateral should be made out to
    CScript collateralPubKey;
    CThrone* pSubmittedToThrone;
};

void ThreadCheckLegacySigner();

#endif
