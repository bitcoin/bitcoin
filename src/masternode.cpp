// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode.h"
#include "masternodeman.h"
#include "darksend.h"
#include "util.h"
#include "sync.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

// keep track of the scanning errors I've seen
map<uint256, int> mapSeenMasternodeScanningErrors;
// cache block hashes as we calculate them
std::map<int64_t, uint256> mapCacheBlockHashes;

struct CompareValueOnly
{
    bool operator()(const pair<int64_t, CTxIn>& t1,
                    const pair<int64_t, CTxIn>& t2) const
    {
        return t1.first < t2.first;
    }
};

//Get the last hash that matches the modulus given. Processed in reverse order
bool GetBlockHash(uint256& hash, int nBlockHeight)
{
    if (chainActive.Tip() == NULL) return false;

    if(nBlockHeight == 0)
        nBlockHeight = chainActive.Tip()->nHeight;

    if(mapCacheBlockHashes.count(nBlockHeight)){
        hash = mapCacheBlockHashes[nBlockHeight];
        return true;
    }

    const CBlockIndex *BlockLastSolved = chainActive.Tip();
    const CBlockIndex *BlockReading = chainActive.Tip();

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || chainActive.Tip()->nHeight+1 < nBlockHeight) return false;

    int nBlocksAgo = 0;
    if(nBlockHeight > 0) nBlocksAgo = (chainActive.Tip()->nHeight+1)-nBlockHeight;
    assert(nBlocksAgo >= 0);

    int n = 0;
    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if(n >= nBlocksAgo){
            hash = BlockReading->GetBlockHash();
            mapCacheBlockHashes[nBlockHeight] = hash;
            return true;
        }
        n++;

        if (BlockReading->pprev == NULL) { assert(BlockReading); break; }
        BlockReading = BlockReading->pprev;
    }

    return false;
}

CMasternode::CMasternode()
{
    LOCK(cs);
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastMnping = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = MIN_PEER_PROTO_VERSION;
    nLastDsq = 0;
    donationAddress = CScript();
    donationPercentage = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    nVotedTimes = 0;
    nLastPaid = 0;
}

CMasternode::CMasternode(const CMasternode& other)
{
    LOCK(cs);
    vin = other.vin;
    addr = other.addr;
    pubkey = other.pubkey;
    pubkey2 = other.pubkey2;
    sig = other.sig;
    activeState = other.activeState;
    sigTime = other.sigTime;
    lastMnping = other.lastMnping;
    lastTimeSeen = other.lastTimeSeen;
    cacheInputAge = other.cacheInputAge;
    cacheInputAgeBlock = other.cacheInputAgeBlock;
    unitTest = other.unitTest;
    allowFreeTx = other.allowFreeTx;
    protocolVersion = other.protocolVersion;
    nLastDsq = other.nLastDsq;
    donationAddress = other.donationAddress;
    donationPercentage = other.donationPercentage;
    nScanningErrorCount = other.nScanningErrorCount;
    nLastScanningErrorBlockHeight = other.nLastScanningErrorBlockHeight;
    nLastPaid = other.nLastPaid;
    nVotedTimes = other.nVotedTimes;
}

CMasternode::CMasternode(CService newAddr, CTxIn newVin, CPubKey newPubkey, std::vector<unsigned char> newSig, int64_t newSigTime, CPubKey newPubkey2, int protocolVersionIn, CScript newDonationAddress, int newDonationPercentage)
{
    LOCK(cs);
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = newSig;
    activeState = MASTERNODE_ENABLED;
    sigTime = newSigTime;
    lastMnping = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = protocolVersionIn;
    nLastDsq = 0;
    donationAddress = newDonationAddress;
    donationPercentage = newDonationPercentage;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    nLastPaid = 0;    
    nVotedTimes = 0;
}

CMasternode::CMasternode(const CMasternodeBroadcast& mnb)
{
    LOCK(cs);
    vin = mnb.vin;
    addr = mnb.addr;
    pubkey = mnb.pubkey;
    pubkey2 = mnb.pubkey2;
    sig = mnb.sig;
    activeState = MASTERNODE_ENABLED;
    sigTime = mnb.sigTime;
    lastMnping = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = mnb.protocolVersion;
    nLastDsq = 0;
    donationAddress = mnb.donationAddress;
    donationPercentage = mnb.donationPercentage;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    nLastPaid = mnb.nLastPaid;
    nVotedTimes = 0;
}

//
// When a new masternode broadcast is sent, update our information
//
void CMasternode::UpdateFromNewBroadcast(CMasternodeBroadcast& mnb)
{
    pubkey2 = mnb.pubkey2;
    sigTime = mnb.sigTime;
    sig = mnb.sig;
    protocolVersion = mnb.protocolVersion;
    addr = mnb.addr;
    donationAddress = mnb.donationAddress;
    donationPercentage = mnb.donationPercentage;
    nLastPaid = mnb.nLastPaid;
}

//
// Deterministically calculate a given "score" for a Masternode depending on how close it's hash is to
// the proof of work for that block. The further away they are the better, the furthest will win the election
// and get paid this block
//
uint256 CMasternode::CalculateScore(int mod, int64_t nBlockHeight)
{
    if(chainActive.Tip() == NULL) return 0;

    uint256 hash = 0;
    uint256 aux = vin.prevout.hash + vin.prevout.n;

    if(!GetBlockHash(hash, nBlockHeight)) return 0;

    uint256 hash2 = Hash(BEGIN(hash), END(hash));
    uint256 hash3 = Hash(BEGIN(hash), END(hash), BEGIN(aux), END(aux));

    uint256 r = (hash3 > hash2 ? hash3 - hash2 : hash2 - hash3);

    return r;
}

void CMasternode::Check()
{
    //TODO: Random segfault with this line removed
    TRY_LOCK(cs_main, lockRecv);
    if(!lockRecv) return;

    if(nScanningErrorCount >= MASTERNODE_SCANNING_ERROR_THESHOLD)
    {
        activeState = MASTERNODE_POS_ERROR;
        return;
    }

    //once spent, stop doing the checks
    if(activeState == MASTERNODE_VIN_SPENT) return;


    if(!UpdatedWithin(MASTERNODE_REMOVAL_SECONDS)){
        activeState = MASTERNODE_REMOVE;
        return;
    }

    if(!UpdatedWithin(MASTERNODE_EXPIRATION_SECONDS)){
        activeState = MASTERNODE_EXPIRED;
        return;
    }

    if(!unitTest){
        CValidationState state;
        CMutableTransaction tx = CMutableTransaction();
        CTxOut vout = CTxOut(999.99*COIN, darkSendPool.collateralPubKey);
        tx.vin.push_back(vin);
        tx.vout.push_back(vout);

        if(!AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)){
            activeState = MASTERNODE_VIN_SPENT;
            return;
        }
    }

    activeState = MASTERNODE_ENABLED; // OK
}


CMasternodeBroadcast::CMasternodeBroadcast()
{
    vin = CTxIn();
    addr = CService();
    pubkey = CPubKey();
    pubkey2 = CPubKey();
    sig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastMnping = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = MIN_PEER_PROTO_VERSION;
    nLastDsq = 0;
    donationAddress = CScript();
    donationPercentage = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    nLastPaid = 0;
}

CMasternodeBroadcast::CMasternodeBroadcast(CService newAddr, CTxIn newVin, CPubKey newPubkey, CPubKey newPubkey2, int protocolVersionIn, CScript newDonationAddress, int newDonationPercentage)
{
    vin = newVin;
    addr = newAddr;
    pubkey = newPubkey;
    pubkey2 = newPubkey2;
    sig = std::vector<unsigned char>();
    activeState = MASTERNODE_ENABLED;
    sigTime = GetAdjustedTime();
    lastMnping = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = protocolVersionIn;
    nLastDsq = 0;
    donationAddress = newDonationAddress;
    donationPercentage = newDonationPercentage;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    nLastPaid = 0;  
}

CMasternodeBroadcast::CMasternodeBroadcast(const CMasternode& other)
{
    vin = other.vin;
    addr = other.addr;
    pubkey = other.pubkey;
    pubkey2 = other.pubkey2;
    sig = other.sig;
    activeState = other.activeState;
    sigTime = other.sigTime;
    lastMnping = other.lastMnping;
    lastTimeSeen = other.lastTimeSeen;
    cacheInputAge = other.cacheInputAge;
    cacheInputAgeBlock = other.cacheInputAgeBlock;
    unitTest = other.unitTest;
    allowFreeTx = other.allowFreeTx;
    protocolVersion = other.protocolVersion;
    nLastDsq = other.nLastDsq;
    donationAddress = other.donationAddress;
    donationPercentage = other.donationPercentage;
    nScanningErrorCount = other.nScanningErrorCount;
    nLastScanningErrorBlockHeight = other.nLastScanningErrorBlockHeight;
    nLastPaid = other.nLastPaid;
}

bool CMasternodeBroadcast::CheckAndUpdate(int& nDos, bool fRequested)
{
    // make sure signature isn't in the future (past is OK)
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("dsee - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
        return false;
    }

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());
    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion)  + donationAddress.ToString() + boost::lexical_cast<std::string>(donationPercentage);

    if(donationPercentage < 0 || donationPercentage > 100){
        LogPrintf("dsee - donation percentage out of range %d\n", donationPercentage);
        return false;
    }

    if(protocolVersion < nMasternodeMinProtocol) {
        LogPrintf("dsee - ignoring outdated Masternode %s protocol version %d\n", vin.ToString().c_str(), protocolVersion);
        return false;
    }

    CScript pubkeyScript;
    pubkeyScript = GetScriptForDestination(pubkey.GetID());

    if(pubkeyScript.size() != 25) {
        LogPrintf("dsee - pubkey the wrong size\n");
        nDos = 100;
        return false;
    }

    CScript pubkeyScript2;
    pubkeyScript2 = GetScriptForDestination(pubkey2.GetID());

    if(pubkeyScript2.size() != 25) {
        LogPrintf("dsee - pubkey2 the wrong size\n");
        nDos = 100;
        return false;
    }

    if(!vin.scriptSig.empty()) {
        LogPrintf("dsee - Ignore Not Empty ScriptSig %s\n",vin.ToString().c_str());
        return false;
    }

    std::string errorMessage = "";
    if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)){
        LogPrintf("dsee - Got bad Masternode address signature\n");
        nDos = 100;
        return false;
    }

    if(Params().NetworkID() == CBaseChainParams::MAIN) {
        if(addr.GetPort() != 9999) return false;
    } else if(addr.GetPort() == 9999) return false;

    //search existing Masternode list, this is where we update existing Masternodes with new dsee broadcasts
    CMasternode* pmn = mnodeman.Find(vin);
    // if we are masternode but with undefined vin and this dsee is ours (matches our Masternode privkey) then just skip this part
    if(pmn != NULL && !(fMasterNode && activeMasternode.vin == CTxIn() && pubkey2 == activeMasternode.pubKeyMasternode))
    {
        // mn.pubkey = pubkey, IsVinAssociatedWithPubkey is validated once below,
        //   after that they just need to match
        if(!fRequested && pmn->pubkey == pubkey && !pmn->UpdatedWithin(MASTERNODE_MIN_DSEE_SECONDS)){
            pmn->UpdateLastSeen();

            if(pmn->sigTime < sigTime){ //take the newest entry
                LogPrintf("dsee - Got updated entry for %s\n", addr.ToString().c_str());
                
                pmn->UpdateFromNewBroadcast((*this));
                
                pmn->Check();
                if(pmn->IsEnabled()) {
                    Relay(fRequested);
                }
            }
        }
        return false;
    }

    return true;
}

bool CMasternodeBroadcast::CheckInputsAndAdd(int& nDoS, bool fRequested)
{
    CValidationState state;
    CMutableTransaction tx = CMutableTransaction();
    CTxOut vout = CTxOut(999.99*COIN, darkSendPool.collateralPubKey);
    tx.vin.push_back(vin);
    tx.vout.push_back(vout);
    if(AcceptableInputs(mempool, state, CTransaction(tx), false, NULL)){
        if(fDebug) LogPrintf("dsee - Accepted Masternode entry\n");

        if(GetInputAge(vin) < MASTERNODE_MIN_CONFIRMATIONS){
            LogPrintf("dsee - Input must have least %d confirmations\n", MASTERNODE_MIN_CONFIRMATIONS);
            return false;
        }

        // verify that sig time is legit in past
        // should be at least not earlier than block when 1000 DASH tx got MASTERNODE_MIN_CONFIRMATIONS
        uint256 hashBlock = 0;
        CTransaction tx2;
        GetTransaction(vin.prevout.hash, tx2, hashBlock, true);
        BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end() && (*mi).second)
        {
            CBlockIndex* pMNIndex = (*mi).second; // block for 1000 DASH tx -> 1 confirmation
            CBlockIndex* pConfIndex = chainActive[pMNIndex->nHeight + MASTERNODE_MIN_CONFIRMATIONS - 1]; // block where tx got MASTERNODE_MIN_CONFIRMATIONS
            if(pConfIndex->GetBlockTime() > sigTime)
            {
                LogPrintf("dsee - Bad sigTime %d for Masternode %20s %105s (%i conf block is at %d)\n",
                          sigTime, addr.ToString(), vin.ToString(), MASTERNODE_MIN_CONFIRMATIONS, pConfIndex->GetBlockTime());
                return false;
            }
        }

        //doesn't support multisig addresses
        if(donationAddress.IsPayToScriptHash()){
            donationAddress = CScript();
            donationPercentage = 0;
        }

        // add our Masternode
        CMasternode mn((*this));
        mn.UpdateLastSeen(lastTimeSeen);
        mnodeman.Add(mn);

        // if it matches our Masternode privkey, then we've been remotely activated
        if(pubkey2 == activeMasternode.pubKeyMasternode && protocolVersion == PROTOCOL_VERSION){
            activeMasternode.EnableHotColdMasterNode(vin, addr);
        }

        bool isLocal = addr.IsRFC1918() || addr.IsLocal();
        if(Params().NetworkID() == CBaseChainParams::REGTEST) isLocal = false;

        if(!fRequested && !isLocal) Relay(fRequested);

        return true;
    } else {
        //set nDos
        state.IsInvalid(nDoS);
    }

    return false;
}

void CMasternodeBroadcast::Relay(bool fRequested)
{
    CInv inv(MSG_MASTERNODE_ANNOUNCE, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}

bool CMasternodeBroadcast::Sign(CKey& keyCollateralAddress)
{   
    std::string errorMessage;

    std::string vchPubKey(pubkey.begin(), pubkey.end());
    std::string vchPubKey2(pubkey2.begin(), pubkey2.end());

    sigTime = GetAdjustedTime();

    std::string strMessage = addr.ToString() + boost::lexical_cast<std::string>(sigTime) + vchPubKey + vchPubKey2 + boost::lexical_cast<std::string>(protocolVersion) + donationAddress.ToString() + boost::lexical_cast<std::string>(donationPercentage);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, sig, keyCollateralAddress)) {
        LogPrintf("CMasternodeBroadcast::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubkey, sig, strMessage, errorMessage)) {
        LogPrintf("CMasternodeBroadcast::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return true;
}

CMasternodePing::CMasternodePing()
{
    vin = CTxIn();
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
}

CMasternodePing::CMasternodePing(CTxIn& newVin)
{
    vin = newVin;
    blockHash = chainActive[chainActive.Height() - 12]->GetBlockHash();
}


bool CMasternodePing::Sign(CKey& keyMasternode, CPubKey& pubKeyMasternode)
{
    std::string errorMessage;
    std::string strMasterNodeSignMessage;

    sigTime = GetAdjustedTime();
    std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

    if(!darkSendSigner.SignMessage(strMessage, errorMessage, vchSig, keyMasternode)) {
        LogPrintf("CMasternodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    if(!darkSendSigner.VerifyMessage(pubKeyMasternode, vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePing::Sign() - Error: %s\n", errorMessage.c_str());
        return false;
    }

    return true;
}

bool CMasternodePing::CheckAndUpdate(int& nDos)
{
    if (sigTime > GetAdjustedTime() + 60 * 60) {
        LogPrintf("mnping - Signature rejected, too far into the future %s\n", vin.ToString().c_str());
        return false;
    }

    if (sigTime <= GetAdjustedTime() - 60 * 60) {
        LogPrintf("mnping - Signature rejected, too far into the past %s - %d %d \n", vin.ToString().c_str(), sigTime, GetAdjustedTime());
        return false;
    }

    // see if we have this Masternode
    CMasternode* pmn = mnodeman.Find(vin);
    if(pmn != NULL && pmn->protocolVersion >= nMasternodeMinProtocol)
    {
        // LogPrintf("mnping - Found corresponding mn for vin: %s\n", vin.ToString().c_str());
        // take this only if it's newer and was last updated more then MASTERNODE_MIN_MNP_SECONDS ago
        if(pmn->lastMnping < sigTime && !pmn->UpdatedWithin(MASTERNODE_MIN_MNP_SECONDS))
        {
            std::string strMessage = vin.ToString() + blockHash.ToString() + boost::lexical_cast<std::string>(sigTime);

            std::string errorMessage = "";
            if(!darkSendSigner.VerifyMessage(pmn->pubkey2, vchSig, strMessage, errorMessage))
            {
                LogPrintf("mnping - Got bad Masternode address signature %s\n", vin.ToString());
                nDos = 33;
                return false;
            }

            pmn->lastMnping = sigTime;

            BlockMap::iterator mi = mapBlockIndex.find(blockHash);
            if (mi != mapBlockIndex.end() && (*mi).second)
            {
                if((*mi).second->nHeight < chainActive.Height() - 24)
                {
                    LogPrintf("mnping - Masternode %s block hash %s is too old\n", vin.ToString(), blockHash.ToString());
                    nDos = 33;
                    return false;
                }
            } else {
                if (fDebug) LogPrintf("mnping - Masternode %s block hash %s is unknown\n", vin.ToString(), blockHash.ToString());
                // maybe we stuck so we shouldn't ban this node, just fail to accept it
                // TODO: or should we also request this block?
                return false;
            }

            pmn->UpdateLastSeen();
            pmn->Check();
            if(!pmn->IsEnabled()) return false;

            Relay();
            return true;
        }
    }

    return false;
}

void CMasternodePing::Relay()
{
    CInv inv(MSG_MASTERNODE_PING, GetHash());

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        pnode->PushMessage("inv", vInv);
    }
}
