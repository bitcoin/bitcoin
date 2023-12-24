// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/verification.h>
#include <util/strencodings.h>

namespace blsct {
bool VerifyTx(const CTransaction& tx, const CCoinsViewCache& view, const CAmount& blockReward)
{
    if (!view.HaveInputs(tx)) {
        return false;
    }

    range_proof::GeneratorsFactory<Mcl> gf;
    bulletproofs::RangeProofLogic<Mcl> rp;
    std::vector<bulletproofs::RangeProof<Mcl>> vProofs;
    std::vector<Message> vMessages;
    std::vector<PublicKey> vPubKeys;
    MclG1Point balanceKey;

    if (blockReward > 0) {
        range_proof::Generators<Mcl> gen = gf.GetInstance(TokenId());
        balanceKey = (gen.G * MclScalar(blockReward));
    }

    if (!tx.IsCoinBase()) {
        for (auto& in : tx.vin) {
            Coin coin;

            if (!view.GetCoin(in.prevout, coin)) {
                return false;
            }

            vPubKeys.push_back(coin.out.blsctData.spendingKey);
            auto in_hash = in.GetHash();
            vMessages.push_back(Message(in_hash.begin(), in_hash.end()));
            balanceKey = balanceKey + coin.out.blsctData.rangeProof.Vs[0];
        }
    }

    CAmount nFee = 0;

    for (auto& out : tx.vout) {
        if (out.IsBLSCT()) {
            vPubKeys.push_back(out.blsctData.ephemeralKey);
            auto out_hash = out.GetHash();
            vMessages.push_back(Message(out_hash.begin(), out_hash.end()));

            vProofs.push_back(out.blsctData.rangeProof);
            balanceKey = balanceKey - out.blsctData.rangeProof.Vs[0];
        } else {
            if (!out.scriptPubKey.IsUnspendable())
                return false;
            if (nFee > 0)
                return false;
            nFee = out.nValue;
            range_proof::Generators<Mcl> gen = gf.GetInstance(out.tokenId);
            balanceKey = balanceKey - (gen.G * MclScalar(out.nValue));
        }
    }

    vMessages.push_back(blsct::Common::BLSCTBALANCE);
    vPubKeys.push_back(balanceKey);

    return PublicKeys{vPubKeys}.VerifyBatch(vMessages, tx.txSig, true) &&
           rp.Verify(vProofs);
}
} // namespace blsct