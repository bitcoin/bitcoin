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
    lastDseep = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = MIN_PEER_PROTO_VERSION;
    nLastDsq = 0;
    donationAddress = CScript();
    donationPercentage = 0;
    nVote = 0;
    lastVote = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;

    //mark last paid as current for new entries
    nLastPaid = GetAdjustedTime();
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
    lastDseep = other.lastDseep;
    lastTimeSeen = other.lastTimeSeen;
    cacheInputAge = other.cacheInputAge;
    cacheInputAgeBlock = other.cacheInputAgeBlock;
    unitTest = other.unitTest;
    allowFreeTx = other.allowFreeTx;
    protocolVersion = other.protocolVersion;
    nLastDsq = other.nLastDsq;
    donationAddress = other.donationAddress;
    donationPercentage = other.donationPercentage;
    nVote = other.nVote;
    lastVote = other.lastVote;
    nScanningErrorCount = other.nScanningErrorCount;
    nLastScanningErrorBlockHeight = other.nLastScanningErrorBlockHeight;
    nLastPaid = other.nLastPaid;
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
    lastDseep = 0;
    lastTimeSeen = 0;
    cacheInputAge = 0;
    cacheInputAgeBlock = 0;
    unitTest = false;
    allowFreeTx = true;
    protocolVersion = protocolVersionIn;
    nLastDsq = 0;
    donationAddress = newDonationAddress;
    donationPercentage = newDonationPercentage;
    nVote = 0;
    lastVote = 0;
    nScanningErrorCount = 0;
    nLastScanningErrorBlockHeight = 0;
    nLastPaid = GetAdjustedTime();
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