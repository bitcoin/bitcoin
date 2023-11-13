// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_PAYMENTS_H
#define BITCOIN_MASTERNODE_PAYMENTS_H

#include <amount.h>

#include <string>
#include <vector>

class CGovernanceManager;
class CBlock;
class CBlockIndex;
class CTransaction;
struct CMutableTransaction;
class CSporkManager;
class CTxOut;
class CMasternodeSync;

/**
 * Masternode Payments Namespace
 * Helpers to kees track of who should get paid for which blocks
 */

namespace MasternodePayments
{
bool IsBlockValueValid(const CSporkManager& sporkManager, CGovernanceManager& governanceManager, const CMasternodeSync& mn_sync,
                       const CBlock& block, const int nBlockHeight, const CAmount blockReward, std::string& strErrorRet);
bool IsBlockPayeeValid(const CSporkManager& sporkManager, CGovernanceManager& governanceManager, const CMasternodeSync& mn_sync,
                       const CTransaction& txNew, const CBlockIndex* const pindexPrev, const CAmount blockSubsidy, const CAmount feeReward);
void FillBlockPayments(const CSporkManager& sporkManager, CGovernanceManager& governanceManager,
                       CMutableTransaction& txNew, const CBlockIndex* const pindexPrev, const CAmount blockSubsidy, const CAmount feeReward,
                       std::vector<CTxOut>& voutMasternodePaymentsRet, std::vector<CTxOut>& voutSuperblockPaymentsRet);

/**
 * this helper returns amount that should be reallocated to platform
 * it is calculated based on total amount of Masternode rewards (not block reward)
 */
CAmount PlatformShare(const CAmount masternodeReward);

} // namespace MasternodePayments

#endif // BITCOIN_MASTERNODE_PAYMENTS_H
