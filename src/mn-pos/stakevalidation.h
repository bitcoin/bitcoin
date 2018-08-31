#ifndef CROWNCOIN_STAKEVALIDATION_H
#define CROWNCOIN_STAKEVALIDATION_H

class CBlock;
class CPubKey;

bool CheckBlockSignature(const CBlock& block, const CPubKey& pubkeyMasternode);

#endif //CROWNCOIN_STAKEVALIDATION_H
