// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/range_proof/bulletproofs/range_proof.h>
#include <blsct/wallet/txfactory_global.h>

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

Signature UnsignedOutput::GetSignature() const
{
    std::vector<Signature> txSigs;

    txSigs.push_back(blsct::PrivateKey(blindingKey).Sign(out.GetHash()));
    txSigs.push_back(blsct::PrivateKey(gamma.Negate()).SignBalance());

    return Signature::Aggregate(txSigs);
}

UnsignedOutput CreateOutput(const blsct::DoublePublicKey& destKeys, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId, const Scalar& blindingKey, const CreateOutputType& type, const CAmount& minStake)
{
    bulletproofs::RangeProofLogic<T> rp;
    auto ret = UnsignedOutput();

    ret.out.nValue = 0;
    ret.out.tokenId = tokenId;

    Scalars vs;
    vs.Add(nAmount);

    ret.blindingKey = blindingKey.IsZero() ? MclScalar::Rand() : blindingKey;

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

    if (type == NORMAL) {
        ret.out.scriptPubKey = CScript(OP_TRUE);
    }

    auto p = rp.Prove(vs, nonce, memo, tokenId);
    ret.out.blsctData.rangeProof = p;

    HashWriter hash{};
    hash << nonce;

    ret.GenerateKeys(blindingKey, destKeys);
    ret.out.blsctData.viewTag = (hash.GetHash().GetUint64(0) & 0xFFFF);

    return ret;
}

CTransactionRef AggregateTransactions(const std::vector<CTransactionRef>& txs)
{
    auto ret = CMutableTransaction();
    std::vector<Signature> vSigs;
    CAmount nFee = 0;

    for (auto& tx : txs) {
        vSigs.push_back(tx->txSig);
        for (auto& in : tx->vin) {
            ret.vin.push_back(in);
        }
        for (auto& out : tx->vout) {
            if (out.scriptPubKey.IsFee()) {
                nFee += out.nValue;
                continue;
            }
            ret.vout.push_back(out);
        }
    }

    ret.vout.emplace_back(nFee, CScript{OP_RETURN});

    ret.txSig = blsct::Signature::Aggregate(vSigs);
    ret.nVersion = CTransaction::BLSCT_MARKER;

    return MakeTransactionRef(ret);
}
} // namespace blsct
