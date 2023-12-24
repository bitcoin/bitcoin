// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TXFACTORY_H
#define TXFACTORY_H

#include <blsct/arith/elements.h>
#include <blsct/wallet/keyman.h>
#include <blsct/wallet/txfactory_global.h>
#include <policy/fees.h>
#include <util/rbf.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>

namespace blsct {
class TxFactory
{
private:
    KeyMan* km;
    CMutableTransaction tx;
    std::map<TokenId, std::vector<UnsignedOutput>> vOutputs;
    std::map<TokenId, std::vector<UnsignedInput>> vInputs;
    std::map<TokenId, Amounts> nAmounts;

public:
    TxFactory(KeyMan* km) : km(km){};

    void AddOutput(const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId = TokenId());
    bool AddInput(wallet::CWallet* wallet, const COutPoint& outpoint, const bool& rbf = false) EXCLUSIVE_LOCKS_REQUIRED(wallet->cs_wallet);
    bool AddInput(const CCoinsViewCache& cache, const COutPoint& outpoint, const bool& rbf = false);
    std::optional<CMutableTransaction> BuildTx();
    static std::optional<CMutableTransaction> CreateTransaction(wallet::CWallet* wallet, blsct::KeyMan* blsct_km, const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId = TokenId());
};
} // namespace blsct

#endif // TXFACTORY_H