// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TXFACTORY_H
#define TXFACTORY_H

#include <blsct/arith/elements.h>
#include <blsct/range_proof/bulletproofs/range_proof_logic.h>
#include <blsct/wallet/keyman.h>
#include <policy/fees.h>
#include <util/rbf.h>
#include <wallet/coincontrol.h>
#include <wallet/spend.h>

using T = Mcl;
using Point = T::Point;
using Points = Elements<Point>;
using Scalar = T::Scalar;
using Scalars = Elements<Scalar>;

#define BLSCT_DEFAULT_FEE 200000

namespace blsct {
struct UnsignedOutput {
    CTxOut out;
    Scalar blindingKey;
    Scalar value;
    Scalar gamma;

    void GenerateKeys(Scalar blindingKey, DoublePublicKey destKeys);

    Signature GetSignature() const;
};

struct UnsignedInput {
    CTxIn in;
    Scalar value;
    Scalar gamma;
    PrivateKey sk;
};

struct Amounts {
    CAmount nFromInputs;
    CAmount nFromOutputs;
};

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

    static UnsignedOutput CreateOutput(const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId = TokenId());
    void AddOutput(const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId = TokenId());
    bool AddInput(const CCoinsViewCache& cache, const COutPoint& outpoint, const bool& rbf = false);
    std::optional<CMutableTransaction> BuildTx();
    static std::optional<CMutableTransaction> CreateTransaction(std::shared_ptr<wallet::CWallet> wallet, const CCoinsViewCache& cache, const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId = TokenId());
    static CTransactionRef AggregateTransactions(const std::vector<CTransactionRef>& txs);
};
} // namespace blsct

#endif // TXFACTORY_H