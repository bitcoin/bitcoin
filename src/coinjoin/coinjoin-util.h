// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_COINJOIN_UTIL_H
#define BITCOIN_COINJOIN_COINJOIN_UTIL_H

#include <wallet/wallet.h>

class CTransactionBuilder;

class CKeyHolder
{
private:
    CReserveKey reserveKey;
    CPubKey pubKey;

public:
    explicit CKeyHolder(CWallet* pwalletIn);
    CKeyHolder(CKeyHolder&&) = delete;
    CKeyHolder& operator=(CKeyHolder&&) = delete;
    void KeepKey();
    void ReturnKey();

    CScript GetScriptForDestination() const;
};

class CKeyHolderStorage
{
private:
    std::vector<std::unique_ptr<CKeyHolder> > storage;
    mutable CCriticalSection cs_storage;

public:
    CScript AddKey(CWallet* pwalletIn);
    void KeepAll();
    void ReturnAll();
};

/**
 * @brief Used by CTransactionBuilder to represent its transaction outputs.
 * It creates a CReserveKey for the given CWallet as destination.
 */
class CTransactionBuilderOutput
{
    /// Used for amount updates
    CTransactionBuilder* pTxBuilder{nullptr};
    /// Reserve key where the amount of this output will end up
    CReserveKey key;
    /// Amount this output will receive
    CAmount nAmount{0};
    /// ScriptPubKey of this output
    CScript script;

public:
    CTransactionBuilderOutput(CTransactionBuilder* pTxBuilderIn, std::shared_ptr<CWallet> pwalletIn, CAmount nAmountIn);
    CTransactionBuilderOutput(CTransactionBuilderOutput&&) = delete;
    CTransactionBuilderOutput& operator=(CTransactionBuilderOutput&&) = delete;
    /// Get the scriptPubKey of this output
    CScript GetScript() const { return script; }
    /// Get the amount of this output
    CAmount GetAmount() const { return nAmount; }
    /// Try update the amount of this output. Returns true if it was successful and false if not (e.g. insufficient amount left).
    bool UpdateAmount(CAmount nAmount);
    /// Tell the wallet to remove the key used by this output from the keypool
    void KeepKey() { key.KeepKey(); }
    /// Tell the wallet to return the key used by this output to the keypool
    void ReturnKey() { key.ReturnKey(); }
};

/**
 * @brief Enables simple transaction generation for a given CWallet object. The resulting
 * transaction's inputs are defined by the given CompactTallyItem. The outputs are
 * defined by CTransactionBuilderOutput.
 */
class CTransactionBuilder
{
    /// Wallet the transaction will be build for
    std::shared_ptr<CWallet> pwallet;
    /// See CTransactionBuilder() for initialization
    CCoinControl coinControl;
    /// Dummy since we anyway use tallyItem's destination as change destination in coincontrol.
    /// Its a member just to make sure ReturnKey can be called in destructor just in case it gets generated/kept
    /// somewhere in CWallet code.
    CReserveKey dummyReserveKey;
    /// Contains all utxos available to generate this transactions. They are all from the same address.
    CompactTallyItem tallyItem;
    /// Contains the number of bytes required for a transaction with only the inputs of tallyItems, no outputs
    int nBytesBase{0};
    /// Contains the number of bytes required to add one output
    int nBytesOutput{0};
    /// Call KeepKey for all keys in destructor if fKeepKeys is true, call ReturnKey for all key if its false.
    bool fKeepKeys{false};
    /// Protect vecOutputs
    mutable CCriticalSection cs_outputs;
    /// Contains all outputs already added to the transaction
    std::vector<std::unique_ptr<CTransactionBuilderOutput>> vecOutputs;
    /// Needed by CTransactionBuilderOutput::UpdateAmount to lock cs_outputs
    friend class CTransactionBuilderOutput;

public:
    CTransactionBuilder(std::shared_ptr<CWallet> pwalletIn, const CompactTallyItem& tallyItemIn);
    ~CTransactionBuilder();
    /// Check it would be possible to add a single output with the amount nAmount. Returns true if its possible and false if not.
    bool CouldAddOutput(CAmount nAmountOutput) const;
    /// Check if its possible to add multiple outputs as vector of amounts. Returns true if its possible to add all of them and false if not.
    bool CouldAddOutputs(const std::vector<CAmount>& vecOutputAmounts) const;
    /// Add an output with the amount nAmount. Returns a pointer to the output if it could be added and nullptr if not due to insufficient amount left.
    CTransactionBuilderOutput* AddOutput(CAmount nAmountOutput = 0);
    /// Get amount we had available when we started
    CAmount GetAmountInitial() const { return tallyItem.nAmount; }
    /// Get the amount currently left to add more outputs. Does respect fees.
    CAmount GetAmountLeft() const { return GetAmountInitial() - GetAmountUsed() - GetFee(GetBytesTotal()); }
    /// Check if an amounts should be considered as dust
    bool IsDust(CAmount nAmount) const;
    /// Get the total number of added outputs
    int CountOutputs() const { return vecOutputs.size(); }
    /// Create and Commit the transaction to the wallet
    bool Commit(std::string& strResult);
    /// Convert to a string
    std::string ToString() const;

private:
    /// Clear the output vector and keep/return the included keys depending on the value of fKeepKeys
    void Clear();
    /// Get the total number of bytes used already by this transaction
    unsigned int GetBytesTotal() const;
    /// Helper to calculate static amount left by simply subtracting an used amount and a fee from a provided initial amount.
    static CAmount GetAmountLeft(CAmount nAmountInitial, CAmount nAmountUsed, CAmount nFee);
    /// Get the amount currently used by added outputs. Does not include fees.
    CAmount GetAmountUsed() const;
    /// Get fees based on the number of bytes and the feerate set in CoinControl.
    /// NOTE: To get the total transaction fee this should only be called once with the total number of bytes for the transaction to avoid
    /// calling CFeeRate::GetFee multiple times with subtotals as this may add rounding errors with each further call.
    CAmount GetFee(unsigned int nBytes) const;
    /// Helper to get GetSizeOfCompactSizeDiff(vecOutputs.size(), vecOutputs.size() + nAdd)
    int GetSizeOfCompactSizeDiff(size_t nAdd) const;
};

#endif // BITCOIN_COINJOIN_COINJOIN_UTIL_H
