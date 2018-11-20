#ifndef CROWNCORE_STAKEVALIDATION_H
#define CROWNCORE_STAKEVALIDATION_H

class CBlock;
class CPubKey;
class CBlockIndex;
class COutPoint;
class CTransaction;

bool CheckBlockSignature(const CBlock& block, const CPubKey& pubkeyMasternode);

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CBlock& block, const CBlockIndex* prevBlock, const COutPoint& outpoint, const CTransaction& txPayment);

#endif //CROWNCORE_STAKEVALIDATION_H
