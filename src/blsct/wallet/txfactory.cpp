// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/txfactory.h>
#include <wallet/fees.h>

using T = Mcl;
using Point = T::Point;
using Points = Elements<Point>;
using Scalar = T::Scalar;
using Scalars = Elements<Scalar>;

namespace blsct {

void TxFactory::AddOutput(const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId)
{
    UnsignedOutput out;
    out = CreateOutput(destination, nAmount, sMemo, tokenId);

    if (nAmounts.count(tokenId) <= 0)
        nAmounts[tokenId] = {0, 0};

    nAmounts[tokenId].nFromOutputs += nAmount;

    if (vOutputs.count(tokenId) <= 0)
        vOutputs[tokenId] = std::vector<UnsignedOutput>();

    vOutputs[tokenId].push_back(out);
}

bool TxFactory::AddInput(const CCoinsViewCache& cache, const COutPoint& outpoint, const bool& rbf)
{
    Coin coin;

    if (!cache.GetCoin(outpoint, coin))
        return false;

    auto recoveredInfo = km->RecoverOutputs(std::vector<CTxOut>{coin.out});

    if (!recoveredInfo.is_completed)
        return false;

    if (vInputs.count(coin.out.tokenId) <= 0)
        vInputs[coin.out.tokenId] = std::vector<UnsignedInput>();

    vInputs[coin.out.tokenId].push_back({CTxIn(outpoint, CScript(), rbf ? MAX_BIP125_RBF_SEQUENCE : CTxIn::SEQUENCE_FINAL), recoveredInfo.amounts[0].amount, recoveredInfo.amounts[0].gamma, km->GetSpendingKeyForOutput(coin.out)});

    if (nAmounts.count(coin.out.tokenId) <= 0)
        nAmounts[coin.out.tokenId] = {0, 0};

    nAmounts[coin.out.tokenId].nFromInputs += recoveredInfo.amounts[0].amount;

    return true;
}
bool TxFactory::AddInput(wallet::CWallet* wallet, const COutPoint& outpoint, const bool& rbf)
{
    auto tx = wallet->GetWalletTx(outpoint.hash);

    if (tx == nullptr)
        return false;

    auto out = tx->tx->vout[outpoint.n];

    if (vInputs.count(out.tokenId) <= 0)
        vInputs[out.tokenId] = std::vector<UnsignedInput>();

    auto recoveredInfo = tx->GetBLSCTRecoveryData(outpoint.n);

    vInputs[out.tokenId].push_back({CTxIn(outpoint, CScript(), rbf ? MAX_BIP125_RBF_SEQUENCE : CTxIn::SEQUENCE_FINAL), recoveredInfo.amount, recoveredInfo.gamma, km->GetSpendingKeyForOutput(out)});

    if (nAmounts.count(out.tokenId) <= 0)
        nAmounts[out.tokenId] = {0, 0};

    nAmounts[out.tokenId].nFromInputs += recoveredInfo.amount;

    return true;
}

std::optional<CMutableTransaction> TxFactory::BuildTx()
{
    CAmount nFee = BLSCT_DEFAULT_FEE * (vInputs.size() + vOutputs.size() + 1);

    while (true) {
        CMutableTransaction tx;
        tx.nVersion |= CTransaction::BLSCT_MARKER;

        Scalar gammaAcc;
        std::map<TokenId, CAmount> mapChange;
        std::vector<Signature> txSigs;

        for (auto& amounts : nAmounts) {
            if (amounts.second.nFromInputs < amounts.second.nFromOutputs)
                return std::nullopt;
            mapChange[amounts.first] = amounts.second.nFromInputs - amounts.second.nFromOutputs - nFee;
        }

        for (auto& in_ : vInputs) {
            for (auto& in : in_.second) {
                tx.vin.push_back(in.in);
                gammaAcc = gammaAcc + in.gamma;
                txSigs.push_back(in.sk.Sign(in.in.GetHash()));
            }
        }

        for (auto& out_ : vOutputs) {
            for (auto& out : out_.second) {
                tx.vout.push_back(out.out);
                gammaAcc = gammaAcc - out.gamma;
                txSigs.push_back(PrivateKey(out.blindingKey).Sign(out.out.GetHash()));
            }
        }

        for (auto& change : mapChange) {
            auto changeOutput = CreateOutput(blsct::SubAddress(std::get<blsct::DoublePublicKey>(km->GetNewDestination(0).value())), change.second, "Change", change.first);
            tx.vout.push_back(changeOutput.out);
            gammaAcc = gammaAcc - changeOutput.gamma;
            txSigs.push_back(PrivateKey(changeOutput.blindingKey).Sign(changeOutput.out.GetHash()));
        }

        if (nFee == (long long)(BLSCT_DEFAULT_FEE * (tx.vin.size() + tx.vout.size()))) {
            CTxOut fee_out{nFee, CScript(OP_RETURN)};
            tx.vout.push_back(fee_out);
            txSigs.push_back(PrivateKey(gammaAcc).SignBalance());
            tx.txSig = Signature::Aggregate(txSigs);
            return tx;
        }

        nFee = BLSCT_DEFAULT_FEE * (tx.vin.size() + tx.vout.size());
    }

    return std::nullopt;
}

std::optional<CMutableTransaction> TxFactory::CreateTransaction(wallet::CWallet* wallet, blsct::KeyMan* blsct_km, const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId)
{
    wallet::CoinFilterParams coins_params;
    coins_params.min_amount = 0;
    coins_params.only_blsct = true;
    coins_params.token_id = tokenId;

    auto tx = blsct::TxFactory(blsct_km);

    for (const wallet::COutput& output : WITH_LOCK(wallet->cs_wallet,
                                                   return AvailableCoins(*wallet, nullptr, std::nullopt, coins_params))
                                             .All()) {
        tx.AddInput(wallet, COutPoint(output.outpoint.hash, output.outpoint.n));

        if (tx.nAmounts[tokenId].nFromInputs > nAmount + (long long)(BLSCT_DEFAULT_FEE * (tx.vInputs.size() + 3))) break;
    }

    tx.AddOutput(destination, nAmount, sMemo, tokenId);

    return tx.BuildTx();
}

} // namespace blsct