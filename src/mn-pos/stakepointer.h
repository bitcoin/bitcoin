#ifndef CROWNCOIN_PROOFPOINTER_H
#define CROWNCOIN_PROOFPOINTER_H

#include "uint256.h"

/*
 *  Crown's masternode staking consensus layer use a "staking node proof pointer" that will point to the most recent
 *  masternode reward that node has received. If the reward is recent enough, and the coins used as collateral for the
 *  masternode have not yet been spent, it can reasonably be assumed that this node is a valid masternode or at least
 *  has been a valid masternode within the recent past.
 *
 * @param hashBlock The blockhash that recently paid this masternode
 * @param txid The transaction ID within the block that paid the masternode todo: this may not be necessary
 * @param nPos The position within the transaction output vector that paid the masternode
 * @param hashPubKey The CKeyID (or pubkeyhash) of the masternode's payment
 */

struct StakePointer
{
    uint256 hashBlock;
    uint256 txid;
    unsigned int nPos;
    uint160 hashPubKey;

    void SetNull()
    {
        hashBlock = uint256();
        txid = uint256();
        hashPubKey = uint160();
        nPos = 0;
    }
};

#endif //CROWNCOIN_PROOFPOINTER_H
