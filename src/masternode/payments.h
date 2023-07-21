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
class CTransaction;
struct CMutableTransaction;
class CSporkManager;
class CTxOut;
class CMasternodeSync;

//
// Masternode Payments Class
// Keeps track of who should get paid for which blocks
//

namespace CMasternodePayments
{
bool IsBlockValueValid(const CSporkManager& sporkManager, CGovernanceManager& governanceManager, const CMasternodeSync& mn_sync,
                       const CBlock& block, const int nBlockHeight, const CAmount blockReward, std::string& strErrorRet);
bool IsBlockPayeeValid(const CSporkManager& sporkManager, CGovernanceManager& governanceManager,
                       const CTransaction& txNew, const int nBlockHeight, const CAmount blockReward);
void FillBlockPayments(const CSporkManager& sporkManager, CGovernanceManager& governanceManager,
                       CMutableTransaction& txNew, const int nBlockHeight, const CAmount blockReward,
                       std::vector<CTxOut>& voutMasternodePaymentsRet, std::vector<CTxOut>& voutSuperblockPaymentsRet);
} // namespace CMasternodePayments

#endif // BITCOIN_MASTERNODE_PAYMENTS_H
