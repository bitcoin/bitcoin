// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_MASTERNODE_PAYMENTS_H
#define BITCOIN_MASTERNODE_MASTERNODE_PAYMENTS_H

#include <util.h>
#include <core_io.h>
#include <key.h>
#include <net_processing.h>
#include <utilstrencodings.h>

#include <evo/deterministicmns.h>

class CMasternodePayments;

/// TODO: all 4 functions do not belong here really, they should be refactored/moved somewhere (main.cpp ?)
bool IsBlockValueValid(const CBlock& block, int nBlockHeight, CAmount blockReward, std::string& strErrorRet);
bool IsBlockPayeeValid(const CTransaction& txNew, int nBlockHeight, CAmount blockReward);
void FillBlockPayments(CMutableTransaction& txNew, int nBlockHeight, CAmount blockReward, std::vector<CTxOut>& voutMasternodePaymentsRet, std::vector<CTxOut>& voutSuperblockPaymentsRet);
std::map<int, std::string> GetRequiredPaymentsStrings(int nStartHeight, int nEndHeight);

extern CMasternodePayments mnpayments;

//
// Masternode Payments Class
// Keeps track of who should get paid for which blocks
//

class CMasternodePayments
{
public:
    bool GetBlockTxOuts(int nBlockHeight, CAmount blockReward, std::vector<CTxOut>& voutMasternodePaymentsRet) const;
    bool IsTransactionValid(const CTransaction& txNew, int nBlockHeight, CAmount blockReward) const;

    bool GetMasternodeTxOuts(int nBlockHeight, CAmount blockReward, std::vector<CTxOut>& voutMasternodePaymentsRet) const;
};

#endif // BITCOIN_MASTERNODE_MASTERNODE_PAYMENTS_H
