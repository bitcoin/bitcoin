// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/txfactory.h>
#include <wallet/fees.cpp>

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

        if (nFee == (long long)(BLSCT_DEFAULT_FEE * (tx.vin.size() + tx.vout.size() + 1))) {
            CTxOut fee_out{nFee, CScript(OP_RETURN)};
            auto blindingKey = PrivateKey(Scalar::Rand());
            fee_out.blsctData.ephemeralKey = blindingKey.GetPublicKey().GetG1Point();
            txSigs.push_back(blindingKey.Sign(fee_out.GetHash()));
            tx.vout.push_back(fee_out);
            txSigs.push_back(PrivateKey(gammaAcc).SignBalance());
            tx.txSig = Signature::Aggregate(txSigs);
            return tx;
        }

        nFee = BLSCT_DEFAULT_FEE * (tx.vin.size() + tx.vout.size() + 1);
    }

    return std::nullopt;
}

std::optional<CMutableTransaction> TxFactory::CreateTransaction(std::shared_ptr<wallet::CWallet> wallet, const CCoinsViewCache& cache, const SubAddress& destination, const CAmount& nAmount, std::string sMemo, const TokenId& tokenId)
{
    wallet::CCoinControl coin_control;
    wallet::CoinFilterParams coins_params;
    coins_params.min_amount = 0;
    coins_params.only_blsct = true;
    coins_params.token_id = tokenId;

    bool rbf = false;

    FeeCalculation fee_calc_out;
    CFeeRate fee_rate{wallet::GetMinimumFeeRate(*wallet, coin_control, &fee_calc_out)};

    auto blsct_km = wallet->GetOrCreateBLSCTKeyMan();
    auto tx = blsct::TxFactory(blsct_km);

    for (const wallet::COutput& output : AvailableCoins(*wallet, &coin_control, fee_rate, coins_params).All()) {
        CHECK_NONFATAL(output.input_bytes > 0);
        tx.AddInput(cache, COutPoint(output.outpoint.hash, output.outpoint.n));

        if (tx.nAmounts[tokenId].nFromInputs > nAmount + (long long)(BLSCT_DEFAULT_FEE * (tx.vInputs.size() + 3))) break;
    }

    tx.AddOutput(destination, nAmount, sMemo, tokenId);

    return tx.BuildTx();
}

CTransactionRef TxFactory::AggregateTransactions(const std::vector<CTransactionRef>& txs)
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
            if (out.IsBLSCT())
                ret.vout.push_back(out);
            else if (out.scriptPubKey.IsUnspendable())
                nFee = out.nValue;
        }
    }

    ret.vout.push_back(CTxOut{nFee, CScript{OP_RETURN}});

    ret.txSig = blsct::Signature::Aggregate(vSigs);

    return MakeTransactionRef(ret);
}

} // namespace blsct