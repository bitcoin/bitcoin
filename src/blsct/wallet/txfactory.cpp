// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/txfactory.h>

using T = Mcl;
using Point = T::Point;
using Points = Elements<Point>;
using Scalar = T::Scalar;
using Scalars = Elements<Scalar>;

namespace blsct {

void UnsignedOutput::GenerateKeys(Scalar blindingKey, DoublePublicKey destKeys)
{
    out.blsctData.ephemeralKey = PrivateKey(blindingKey).GetPoint();

    Point vk, sk;

    if (!destKeys.GetViewKey(vk)) {
        throw std::runtime_error(strprintf("%s: could not get view key from destination address\n", __func__));
    }

    if (!destKeys.GetSpendKey(sk)) {
        throw std::runtime_error(strprintf("%s: could not get spend key from destination address\n", __func__));
    }

    out.blsctData.blindingKey = sk * blindingKey;

    auto rV = vk * blindingKey;

    out.blsctData.spendingKey = sk + (PrivateKey(Scalar(rV.GetHashWithSalt(0))).GetPoint());
}

UnsignedOutput TxFactory::CreateOutput(const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId)
{
    auto ret = UnsignedOutput();

    ret.out.nValue = 0;
    ret.out.tokenId = tokenId;
    ret.out.scriptPubKey = CScript(OP_TRUE);

    Scalars vs;
    vs.Add(nAmount);

    auto destKeys = destination.GetKeys();
    auto blindingKey = Scalar::Rand();

    ret.blindingKey = blindingKey;

    Points nonces;
    Point vk;

    if (!destKeys.GetViewKey(vk)) {
        throw std::runtime_error(strprintf("%s: could not get view key from destination address\n", __func__));
    }

    auto nonce = vk * blindingKey;
    nonces.Add(nonce);

    ret.value = nAmount;
    ret.gamma = nonce.GetHashWithSalt(100);

    std::vector<unsigned char> memo{sMemo.begin(), sMemo.end()};

    bulletproofs::RangeProofLogic<T> rp;
    auto p = rp.Prove(vs, nonce, memo, tokenId);

    ret.out.blsctData.rangeProof = p;

    CHashWriter hash(SER_GETHASH, PROTOCOL_VERSION);
    hash << nonce;

    ret.GenerateKeys(blindingKey, destKeys);
    ret.out.blsctData.viewTag = (hash.GetHash().GetUint64(0) & 0xFFFF);

    return ret;
}

void TxFactory::AddOutput(const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId)
{
    UnsignedOutput out;
    out = TxFactory::CreateOutput(destination, nAmount, sMemo, tokenId);

    if (nAmounts.count(tokenId) <= 0)
        nAmounts[tokenId] = {0, 0};

    nAmounts[tokenId].nFromOutputs += nAmount;

    if (vOutputs.count(tokenId) <= 0)
        vOutputs[tokenId] = std::vector<UnsignedOutput>();

    vOutputs[tokenId].push_back(out);
}

bool TxFactory::AddInput(const CCoinsViewCache& cache, const COutPoint& outpoint)
{
    Coin coin;

    if (!cache.GetCoin(outpoint, coin))
        return false;

    auto recoveredInfo = km->RecoverOutputs(std::vector<CTxOut>{coin.out});

    if (!recoveredInfo.is_completed)
        return false;

    if (vInputs.count(coin.out.tokenId) <= 0)
        vInputs[coin.out.tokenId] = std::vector<UnsignedInput>();

    vInputs[coin.out.tokenId].push_back({CTxIn(outpoint), recoveredInfo.amounts[0].amount, recoveredInfo.amounts[0].gamma, km->GetSpendingKeyForOutput(coin.out)});

    if (nAmounts.count(coin.out.tokenId) <= 0)
        nAmounts[coin.out.tokenId] = {0, 0};

    nAmounts[coin.out.tokenId].nFromInputs += recoveredInfo.amounts[0].amount;

    return true;
}

std::optional<CMutableTransaction> TxFactory::BuildTx()
{
    CAmount nFee = 200000 * (vInputs.size() + vOutputs.size() + 1);

    while (true) {
        CMutableTransaction tx;
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

        if (nFee == (long long)(200000 * (tx.vin.size() + tx.vout.size() + 1))) {
            CTxOut fee_out{nFee, CScript(OP_RETURN)};
            auto blindingKey = PrivateKey(Scalar::Rand());
            fee_out.blsctData.ephemeralKey = blindingKey.GetPublicKey().GetG1Point();
            txSigs.push_back(blindingKey.Sign(fee_out.GetHash()));
            tx.vout.push_back(fee_out);
            txSigs.push_back(PrivateKey(gammaAcc).SignBalance());
            tx.txSig = Signature::Aggregate(txSigs);
            return tx;
        }

        nFee = 200000 * (tx.vin.size() + tx.vout.size() + 1);
    }

    return std::nullopt;
}

} // namespace blsct