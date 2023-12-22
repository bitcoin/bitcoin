// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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

UnsignedOutput CreateOutput(const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId, const Scalar& blindingKey)
{
    auto ret = UnsignedOutput();

    ret.out.nValue = 0;
    ret.out.tokenId = tokenId;
    ret.out.scriptPubKey = CScript(OP_TRUE);

    Scalars vs;
    vs.Add(nAmount);

    auto destKeys = destination.GetKeys();

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

CTransactionRef AggregateTransactions(const std::vector<CTransactionRef>& txs)
{
    auto ret = CMutableTransaction();
    std::vector<Signature> vSigs;
    CAmount nFee = 0;
    bool fCommitmentAdded = false;
    bool fCoinbaseAdded = false;

    for (auto& tx : txs) {
        vSigs.push_back(tx->txSig);
        for (auto& in : tx->vin) {
            if (!fCoinbaseAdded && in.prevout.IsNull()) {
                fCoinbaseAdded = true;
                ret.vin.insert(ret.vin.begin(), in);
            } else {
                ret.vin.push_back(in);
            }
        }
        for (auto& out : tx->vout) {
            if (out.IsBLSCT())
                ret.vout.push_back(out);
            else if (!fCommitmentAdded &&
                     out.scriptPubKey.size() >= 38 &&
                     out.scriptPubKey[0] == OP_RETURN &&
                     out.scriptPubKey[1] == 0x24 &&
                     out.scriptPubKey[2] == 0xaa &&
                     out.scriptPubKey[3] == 0x21 &&
                     out.scriptPubKey[4] == 0xa9 &&
                     out.scriptPubKey[5] == 0xed) {
                ret.vout.push_back(out);
                fCommitmentAdded = true;
            } else if (out.scriptPubKey.IsFee())
                nFee += out.nValue;
        }
    }

    ret.vout.push_back(CTxOut{nFee, CScript{OP_RETURN}});

    ret.txSig = blsct::Signature::Aggregate(vSigs);
    ret.nVersion = CTransaction::BLSCT_MARKER;

    return MakeTransactionRef(ret);
}
} // namespace blsct