// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/pos/pos.h>
#include <blsct/public_keys.h>
#include <blsct/range_proof/bulletproofs/range_proof.h>
#include <blsct/range_proof/bulletproofs/range_proof_logic.h>
#include <blsct/range_proof/generators.h>
#include <blsct/wallet/verification.h>
#include <util/strencodings.h>

namespace blsct {
bool VerifyTx(const CTransaction& tx, const CCoinsViewCache& view, TxValidationState& state, const CAmount& blockReward, const CAmount& minStake)
{
    if (!view.HaveInputs(tx)) {
        return state.Invalid(TxValidationResult::TX_MISSING_INPUTS, "bad-inputs-unknown");
    }

    range_proof::GeneratorsFactory<Mcl> gf;
    bulletproofs::RangeProofLogic<Mcl> rp;
    std::vector<bulletproofs::RangeProofWithSeed<Mcl>> vProofs;
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
                return state.Invalid(TxValidationResult::TX_MISSING_INPUTS, "bad-input-unknown");
            }

            vPubKeys.emplace_back(coin.out.blsctData.spendingKey);
            auto in_hash = in.GetHash();
            vMessages.emplace_back(in_hash.begin(), in_hash.end());
            balanceKey = balanceKey + coin.out.blsctData.rangeProof.Vs[0];
        }
    }

    CAmount nFee = 0;
    bulletproofs::RangeProofWithSeed<Mcl> stakedCommitmentRangeProof;

    for (auto& out : tx.vout) {
        if (out.IsBLSCT()) {
            vPubKeys.emplace_back(out.blsctData.ephemeralKey);
            auto out_hash = out.GetHash();
            vMessages.emplace_back(out_hash.begin(), out_hash.end());
            vProofs.emplace_back(bulletproofs::RangeProofWithSeed<Mcl>{out.blsctData.rangeProof, out.tokenId});
            balanceKey = balanceKey - out.blsctData.rangeProof.Vs[0];

            if (out.GetStakedCommitmentRangeProof(stakedCommitmentRangeProof)) {
                stakedCommitmentRangeProof.Vs.Clear();
                stakedCommitmentRangeProof.Vs.Add(out.blsctData.rangeProof.Vs[0]);
                vProofs.push_back(bulletproofs::RangeProofWithSeed<Mcl>{stakedCommitmentRangeProof, TokenId(), minStake});
            }
        } else {
            if (!out.scriptPubKey.IsUnspendable() && out.nValue > 0) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "spendable-output-with-public-value");
            }
            if (nFee > 0 || !MoneyRange(out.nValue)) {
                return state.Invalid(TxValidationResult::TX_CONSENSUS, "more-than-one-fee-output");
            }
            if (out.nValue == 0) continue;
            nFee = out.nValue;
            range_proof::Generators<Mcl> gen = gf.GetInstance(out.tokenId);
            balanceKey = balanceKey - (gen.G * MclScalar(out.nValue));
        }
    }

    vMessages.emplace_back(blsct::Common::BLSCTBALANCE);
    vPubKeys.emplace_back(balanceKey);

    auto sigCheck = PublicKeys{vPubKeys}.VerifyBatch(vMessages, tx.txSig, true);

    if (!sigCheck)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-signature-check");

    auto rpCheck = rp.Verify(vProofs);

    if (!rpCheck)
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "failed-rangeproof-check");

    return true;
}
} // namespace blsct
