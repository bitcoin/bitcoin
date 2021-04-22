// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_FEEBUMPER_H
#define BITCOIN_WALLET_FEEBUMPER_H

#include <primitives/transaction.h>

class CWallet;
class CWalletTx;
class uint256;
class CCoinControl;
enum class FeeEstimateMode;
struct bilingual_str;

namespace feebumper {

enum class Result
{
    OK,
    INVALID_ADDRESS_OR_KEY,
    INVALID_REQUEST,
    INVALID_PARAMETER,
    WALLET_ERROR,
    MISC_ERROR,
};

//! Return whether transaction can be bumped.
bool TransactionCanBeBumped(const CWallet& wallet, const uint256& txid);

//! Create bumpfee transaction based on feerate estimates.
Result CreateRateBumpTransaction(CWallet& wallet,
    const uint256& txid,
    const CCoinControl& coin_control,
    std::vector<bilingual_str>& errors,
    CAmount& old_fee,
    CAmount& new_fee,
    CMutableTransaction& mtx,
    std::vector<unsigned char> vecContract);

//! Sign the new transaction,
//! @return false if the tx couldn't be found or if it was
//! impossible to create the signature(s)
bool SignTransaction(CWallet& wallet, CMutableTransaction& mtx);

//! Commit the bumpfee transaction.
//! @return success in case of CWallet::CommitTransaction was successful,
//! but sets errors if the tx could not be added to the mempool (will try later)
//! or if the old transaction could not be marked as replaced.
Result CommitTransaction(CWallet& wallet,
    const uint256& txid,
    CMutableTransaction&& mtx,
    std::vector<bilingual_str>& errors,
    uint256& bumped_txid);

} // namespace feebumper

#endif // BITCOIN_WALLET_FEEBUMPER_H
