// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINSELECTION_H
#define BITCOIN_WALLET_COINSELECTION_H

#include <amount.h>
#include <primitives/transaction.h>
#include <random.h>

//! target minimum change amount
static const CAmount MIN_CHANGE = CENT;
//! final minimum change amount after paying for fees
static const CAmount MIN_FINAL_CHANGE = MIN_CHANGE/2;

class CInputCoin {
public:
    CInputCoin() = delete;
    CInputCoin(const CTransactionRef& tx, unsigned int i)
    {
        if (!tx)
            throw std::invalid_argument("tx should not be null");
        if (i >= tx->vout.size())
            throw std::out_of_range("The output index is out of range");

        outpoint = COutPoint(tx->GetHash(), i);
        txout = tx->vout[i];
    }

    COutPoint outpoint;
    CTxOut txout;

    bool operator<(const CInputCoin& rhs) const {
        return outpoint < rhs.outpoint;
    }

    bool operator!=(const CInputCoin& rhs) const {
        return outpoint != rhs.outpoint;
    }

    bool operator==(const CInputCoin& rhs) const {
        return outpoint == rhs.outpoint;
    }
};

class InputCoinWithFee
{
public:
    InputCoinWithFee() = delete;
    InputCoinWithFee(const CInputCoin&& coin_in, const CAmount& fee_in, const CAmount& long_term_fee)
        : coin(coin_in), fee(fee_in), effective_value(coin.txout.nValue - fee), waste(fee - long_term_fee)
    {
    }

    CInputCoin coin;
    CAmount fee;
    CAmount effective_value;
    CAmount waste;
};

bool SelectCoinsBnB(std::vector<InputCoinWithFee>& utxo_pool, const CAmount& target_value, const CAmount& cost_of_change, std::set<CInputCoin>& out_set, CAmount& value_ret, CAmount not_input_fees);

// Original coin selection algorithm as a fallback
bool KnapsackSolver(const CAmount& nTargetValue, std::vector<CInputCoin>& vCoins, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet);
#endif // BITCOIN_WALLET_COINSELECTION_H
