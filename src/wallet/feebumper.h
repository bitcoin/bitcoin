// Copyright (c) 2017 The Bitcoin Core developers
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

enum class BumpFeeResult
{
    OK,
    INVALID_ADDRESS_OR_KEY,
    INVALID_REQUEST,
    INVALID_PARAMETER,
    WALLET_ERROR,
    MISC_ERROR,
};

class FeeBumper
{
public:
    FeeBumper(const CWallet *wallet, const uint256 txid_in, const CCoinControl& coin_control, CAmount total_fee);
    BumpFeeResult getResult() const { return current_result; }
    const std::vector<std::string>& getErrors() const { return errors; }
    CAmount getOldFee() const { return old_fee; }
    CAmount getNewFee() const { return new_fee; }
    uint256 getBumpedTxId() const { return bumped_txid; }

    /* signs the new transaction,
     * returns false if the tx couldn't be found or if it was
     * impossible to create the signature(s)
     */
    bool signTransaction(CWallet *wallet);

    /* commits the fee bump,
     * returns true, in case of CWallet::CommitTransaction was successful
     * but, eventually sets errors if the tx could not be added to the mempool (will try later)
     * or if the old transaction could not be marked as replaced
     */
    bool commit(CWallet *wallet);

private:
    bool preconditionChecks(const CWallet *wallet, const CWalletTx& wtx);

    const uint256 txid;
    uint256 bumped_txid;
    CMutableTransaction mtx;
    std::vector<std::string> errors;
    BumpFeeResult current_result;
    CAmount old_fee;
    CAmount new_fee;
};

#endif // BITCOIN_WALLET_FEEBUMPER_H
