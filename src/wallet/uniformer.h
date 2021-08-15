// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_UNIFORMER_H
#define BITCOIN_WALLET_UNIFORMER_H

#include <primitives/transaction.h>
#include <script/standard.h>

#include <map>
#include <string>

class CCoinControl;
class COutPoint;
class CWallet;
class CWalletTx;

namespace uniformer {

enum class Result
{
    OK,
    INVALID_ADDRESS_OR_KEY,
    INVALID_REQUEST,
    INVALID_PARAMETER,
    BIND_HIGHFEE_ERROR,
    UNBIND_LIMIT_ERROR,
    WALLET_ERROR,
    MISC_ERROR,
};

//! Return whether coin can be unfreeze.
bool CoinCanBeUnfreeze(const CWallet* wallet, const COutPoint& outpoint);

//! Create bind plotter tnrasaction.
Result CreateBindPlotterTransaction(CWallet* wallet,
                                    const CTxDestination &dest,
                                    const CScript &bindScriptData,
                                    bool fAllowHighFee,
                                    const CCoinControl& coin_control,
                                    std::vector<std::string>& errors,
                                    CAmount& txfee,
                                    CMutableTransaction& mtx);

//! Create point transaction.
Result CreatePointTransaction(CWallet* wallet,
                              const CTxDestination &senderDest,
                              const CTxDestination &receiverDest,
                              CAmount nAmount,
                              int nLockBlocks,
                              bool fSubtractFeeFromAmount,
                              const CCoinControl& coin_control,
                              std::vector<std::string>& errors,
                              CAmount& txfee,
                              CMutableTransaction& mtx);

//! Create staking transaction.
Result CreateStakingTransaction(CWallet* wallet,
                                const CTxDestination &senderDest,
                                const CTxDestination &receiverDest,
                                CAmount nAmount,
                                int nLockBlocks,
                                bool fSubtractFeeFromAmount,
                                const CCoinControl& coin_control,
                                std::vector<std::string>& errors,
                                CAmount& txfee,
                                CMutableTransaction& mtx);

//! Create unfreeze transaction.
Result CreateUnfreezeTransaction(CWallet* wallet,
                                 const COutPoint& outpoint,
                                 const CCoinControl& coin_control,
                                 std::vector<std::string>& errors,
                                 CAmount& txfee,
                                 CMutableTransaction& mtx);

//! Sign the new transaction,
//! @return false if the tx couldn't be found or if it was
//! impossible to create the signature(s)
bool SignTransaction(CWallet* wallet, CMutableTransaction& mtx);

//! Commit the bumpfee transaction.
//! @return success in case of CWallet::CommitTransaction was successful,
//! but sets errors if the tx could not be added to the mempool (will try later)
Result CommitTransaction(CWallet* wallet,
                         CMutableTransaction&& mtx,
                         std::map<std::string, std::string>&& mapValue,
                         std::vector<std::string>& errors);

} // namespace uniformer

#endif // BITCOIN_WALLET_UNIFORMER_H
