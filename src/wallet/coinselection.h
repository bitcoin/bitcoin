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
    CInputCoin(const CTransactionRef& tx, unsigned int i)
    {
        if (!tx)
            throw std::invalid_argument("tx should not be null");
        if (i >= tx->vout.size())
            throw std::out_of_range("The output index is out of range");

        outpoint = COutPoint(tx->GetHash(), i);
        txout = tx->vout[i];
        effective_value = txout.nValue;
    }

    COutPoint outpoint;
    CTxOut txout;
    CAmount effective_value;
    CAmount fee = 0;
    CAmount long_term_fee = 0;

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

struct CoinEligibilityFilter
{
    const int conf_mine;
    const int conf_theirs;
    const uint64_t max_ancestors;
    const uint64_t max_descendants;

    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_ancestors) {}
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants) {}
};

bool SelectCoinsBnB(std::vector<CInputCoin>& utxo_pool, const CAmount& target_value, const CAmount& cost_of_change, std::set<CInputCoin>& out_set, CAmount& value_ret, CAmount not_input_fees);

// Original coin selection algorithm as a fallback
bool KnapsackSolver(const CAmount& nTargetValue, std::vector<CInputCoin>& vCoins, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet);
#endif // BITCOIN_WALLET_COINSELECTION_H
