// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coinjoin/util.h>
#include <net.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <script/sign.h>
#include <validation.h>
#include <wallet/fees.h>
#include <wallet/wallet.h>
#include <util/translation.h>

#include <numeric>

inline unsigned int GetSizeOfCompactSizeDiff(uint64_t nSizePrev, uint64_t nSizeNew)
{
    assert(nSizePrev <= nSizeNew);
    return ::GetSizeOfCompactSize(nSizeNew) - ::GetSizeOfCompactSize(nSizePrev);
}

CKeyHolder::CKeyHolder(CWallet* pwallet) :
    reserveDestination(pwallet)
{
    reserveDestination.GetReservedDestination(dest, false);
}

void CKeyHolder::KeepKey()
{
    reserveDestination.KeepDestination();
}

void CKeyHolder::ReturnKey()
{
    reserveDestination.ReturnDestination();
}

CScript CKeyHolder::GetScriptForDestination() const
{
    return ::GetScriptForDestination(dest);
}


CScript CKeyHolderStorage::AddKey(CWallet* pwallet)
{
    auto keyHolderPtr = std::make_unique<CKeyHolder>(pwallet);
    auto script = keyHolderPtr->GetScriptForDestination();

    LOCK(cs_storage);
    storage.emplace_back(std::move(keyHolderPtr));
    LogPrint(BCLog::COINJOIN, "CKeyHolderStorage::%s -- storage size %lld\n", __func__, storage.size());
    return script;
}

void CKeyHolderStorage::KeepAll()
{
    std::vector<std::unique_ptr<CKeyHolder> > tmp;
    {
        // don't hold cs_storage while calling KeepKey(), which might lock cs_wallet
        LOCK(cs_storage);
        std::swap(storage, tmp);
    }

    if (!tmp.empty()) {
        for (const auto& key : tmp) {
            key->KeepKey();
        }
        LogPrint(BCLog::COINJOIN, "CKeyHolderStorage::%s -- %lld keys kept\n", __func__, tmp.size());
    }
}

void CKeyHolderStorage::ReturnAll()
{
    std::vector<std::unique_ptr<CKeyHolder> > tmp;
    {
        // don't hold cs_storage while calling ReturnKey(), which might lock cs_wallet
        LOCK(cs_storage);
        std::swap(storage, tmp);
    }

    if (!tmp.empty()) {
        for (const auto& key : tmp) {
            key->ReturnKey();
        }
        LogPrint(BCLog::COINJOIN, "CKeyHolderStorage::%s -- %lld keys returned\n", __func__, tmp.size());
    }
}

CTransactionBuilderOutput::CTransactionBuilderOutput(CTransactionBuilder* pTxBuilderIn, std::shared_ptr<CWallet> pwalletIn, CAmount nAmountIn) :
    pTxBuilder(pTxBuilderIn),
    dest(pwalletIn.get()),
    nAmount(nAmountIn)
{
    assert(pTxBuilder);
    CTxDestination txdest;
    LOCK(pwalletIn->cs_wallet);
    dest.GetReservedDestination(txdest, false);
    script = ::GetScriptForDestination(txdest);
}

bool CTransactionBuilderOutput::UpdateAmount(const CAmount nNewAmount)
{
    if (nNewAmount <= 0 || nNewAmount - nAmount > pTxBuilder->GetAmountLeft()) {
        return false;
    }
    nAmount = nNewAmount;
    return true;
}

CTransactionBuilder::CTransactionBuilder(std::shared_ptr<CWallet> pwalletIn, const CompactTallyItem& tallyItemIn, const CBlockPolicyEstimator& fee_estimator) :
    pwallet(pwalletIn),
    dummyReserveDestination(pwalletIn.get()),
    tallyItem(tallyItemIn)
{
    // Generate a feerate which will be used to consider if the remainder is dust and will go into fees or not
    coinControl.m_discard_feerate = ::GetDiscardRate(*pwallet);
    // Generate a feerate which will be used by calculations of this class and also by CWallet::CreateTransaction
    coinControl.m_feerate = std::max(fee_estimator.estimateSmartFee(int(pwallet->m_confirm_target), nullptr, true), pwallet->m_pay_tx_fee);
    // Change always goes back to origin
    coinControl.destChange = tallyItemIn.txdest;
    // Only allow tallyItems inputs for tx creation
    coinControl.fAllowOtherInputs = false;
    // Create dummy tx to calculate the exact required fees upfront for accurate amount and fee calculations
    CMutableTransaction dummyTx;
    // Select all tallyItem outputs in the coinControl so that CreateTransaction knows what to use
    for (const auto& coin : tallyItem.vecInputCoins) {
        coinControl.Select(coin.outpoint);
        dummyTx.vin.emplace_back(coin.outpoint, CScript());
    }
    // Get a comparable dummy scriptPubKey, avoid writing/flushing to the actual wallet db
    CScript dummyScript;
    {
        LOCK(pwallet->cs_wallet);
        WalletBatch dummyBatch(pwallet->GetDatabase(), false);
        dummyBatch.TxnBegin();
        CKey secret;
        secret.MakeNewKey(pwallet->CanSupportFeature(FEATURE_COMPRPUBKEY));
        CPubKey dummyPubkey = secret.GetPubKey();
        dummyBatch.TxnAbort();
        dummyScript = ::GetScriptForDestination(PKHash(dummyPubkey));
        // Calculate required bytes for the dummy signed tx with tallyItem's inputs only
        nBytesBase = CalculateMaximumSignedTxSize(CTransaction(dummyTx), pwallet.get(), false);
    }
    // Calculate the output size
    nBytesOutput = ::GetSerializeSize(CTxOut(0, dummyScript), PROTOCOL_VERSION);
    // Just to make sure..
    Clear();
}

CTransactionBuilder::~CTransactionBuilder()
{
    Clear();
}

void CTransactionBuilder::Clear()
{
    std::vector<std::unique_ptr<CTransactionBuilderOutput>> vecOutputsTmp;
    {
        // Don't hold cs_outputs while clearing the outputs which might indirectly call lock cs_wallet
        LOCK(cs_outputs);
        std::swap(vecOutputs, vecOutputsTmp);
        vecOutputs.clear();
    }

    for (auto& key : vecOutputsTmp) {
        if (fKeepKeys) {
            key->KeepKey();
        } else {
            key->ReturnKey();
        }
    }
    // Always return this key just to make sure..
    dummyReserveDestination.ReturnDestination();
}

bool CTransactionBuilder::CouldAddOutput(CAmount nAmountOutput) const
{
    if (nAmountOutput < 0) {
        return false;
    }
    // Adding another output can change the serialized size of the vout size hence + GetSizeOfCompactSizeDiff()
    unsigned int nBytes = GetBytesTotal() + nBytesOutput + GetSizeOfCompactSizeDiff(1);
    return GetAmountLeft(GetAmountInitial(), GetAmountUsed() + nAmountOutput, GetFee(nBytes)) >= 0;
}

bool CTransactionBuilder::CouldAddOutputs(const std::vector<CAmount>& vecOutputAmounts) const
{
    CAmount nAmountAdditional{0};
    assert(vecOutputAmounts.size() < std::numeric_limits<int>::max());
    int nBytesAdditional = nBytesOutput * int(vecOutputAmounts.size());
    for (const auto nAmountOutput : vecOutputAmounts) {
        if (nAmountOutput < 0) {
            return false;
        }
        nAmountAdditional += nAmountOutput;
    }
    // Adding other outputs can change the serialized size of the vout size hence + GetSizeOfCompactSizeDiff()
    unsigned int nBytes = GetBytesTotal() + nBytesAdditional + GetSizeOfCompactSizeDiff(vecOutputAmounts.size());
    return GetAmountLeft(GetAmountInitial(), GetAmountUsed() + nAmountAdditional, GetFee(nBytes)) >= 0;
}

CTransactionBuilderOutput* CTransactionBuilder::AddOutput(CAmount nAmountOutput)
{
    if (CouldAddOutput(nAmountOutput)) {
        LOCK(cs_outputs);
        vecOutputs.push_back(std::make_unique<CTransactionBuilderOutput>(this, pwallet, nAmountOutput));
        return vecOutputs.back().get();
    }
    return nullptr;
}

unsigned int CTransactionBuilder::GetBytesTotal() const
{
    LOCK(cs_outputs);
    // Adding other outputs can change the serialized size of the vout size hence + GetSizeOfCompactSizeDiff()
    return nBytesBase + vecOutputs.size() * nBytesOutput + ::GetSizeOfCompactSizeDiff(0, vecOutputs.size());
}

CAmount CTransactionBuilder::GetAmountLeft(const CAmount nAmountInitial, const CAmount nAmountUsed, const CAmount nFee)
{
    return nAmountInitial - nAmountUsed - nFee;
}

CAmount CTransactionBuilder::GetAmountUsed() const
{
    LOCK(cs_outputs);
    return std::accumulate(vecOutputs.begin(), vecOutputs.end(), CAmount{0}, [](const CAmount& a, const auto& b){
        return a + b->GetAmount();
    });
}

CAmount CTransactionBuilder::GetFee(unsigned int nBytes) const
{
    CAmount nFeeCalc = coinControl.m_feerate->GetFee(nBytes);
    CAmount nRequiredFee = GetRequiredFee(*pwallet, nBytes);
    if (nRequiredFee > nFeeCalc) {
        nFeeCalc = nRequiredFee;
    }
    if (nFeeCalc > pwallet->m_default_max_tx_fee) {
        nFeeCalc = pwallet->m_default_max_tx_fee;
    }
    return nFeeCalc;
}

int CTransactionBuilder::GetSizeOfCompactSizeDiff(size_t nAdd) const
{
    size_t nSize = WITH_LOCK(cs_outputs, return vecOutputs.size());
    unsigned int ret = ::GetSizeOfCompactSizeDiff(nSize, nSize + nAdd);
    assert(ret <= std::numeric_limits<int>::max());
    return int(ret);
}

bool CTransactionBuilder::IsDust(CAmount nAmount) const
{
    return ::IsDust(CTxOut(nAmount, ::GetScriptForDestination(tallyItem.txdest)), coinControl.m_discard_feerate.value());
}

bool CTransactionBuilder::Commit(bilingual_str& strResult)
{
    CAmount nFeeRet = 0;
    int nChangePosRet = -1;

    // Transform the outputs to the format CWallet::CreateTransaction requires
    std::vector<CRecipient> vecSend;
    {
        LOCK(cs_outputs);
        vecSend.reserve(vecOutputs.size());
        for (const auto& out : vecOutputs) {
            vecSend.push_back((CRecipient){out->GetScript(), out->GetAmount(), false});
        }
    }

    CTransactionRef tx;
    {
        LOCK2(pwallet->cs_wallet, cs_main);
        if (!pwallet->CreateTransaction(vecSend, tx, nFeeRet, nChangePosRet, strResult, coinControl)) {
            return false;
        }
    }

    CAmount nAmountLeft = GetAmountLeft();
    bool fDust = IsDust(nAmountLeft);
    // If there is a either remainder which is considered to be dust (will be added to fee in this case) or no amount left there should be no change output, return if there is a change output.
    if (nChangePosRet != -1 && fDust) {
        strResult = Untranslated(strprintf("Unexpected change output %s at position %d", tx->vout[nChangePosRet].ToString(), nChangePosRet));
        return false;
    }

    // If there is a remainder which is not considered to be dust it should end up in a change output, return if not.
    if (nChangePosRet == -1 && !fDust) {
        strResult = Untranslated(strprintf("Change output missing: %d", nAmountLeft));
        return false;
    }

    CAmount nFeeAdditional{0};
    unsigned int nBytesAdditional{0};

    if (fDust) {
        nFeeAdditional = nAmountLeft;
    } else {
        // Add a change output and GetSizeOfCompactSizeDiff(1) as another output can changes the serialized size of the vout size in CTransaction
        nBytesAdditional = nBytesOutput + GetSizeOfCompactSizeDiff(1);
    }

    // If the calculated fee does not match the fee returned by CreateTransaction aka if this check fails something is wrong!
    CAmount nFeeCalc = GetFee(GetBytesTotal() + nBytesAdditional) + nFeeAdditional;
    if (nFeeRet != nFeeCalc) {
        strResult = Untranslated(strprintf("Fee validation failed -> nFeeRet: %d, nFeeCalc: %d, nFeeAdditional: %d, nBytesAdditional: %d, %s", nFeeRet, nFeeCalc, nFeeAdditional, nBytesAdditional, ToString()));
        return false;
    }

    {
        LOCK2(pwallet->cs_wallet, cs_main);
        pwallet->CommitTransaction(tx, {}, {});
    }

    fKeepKeys = true;

    strResult = Untranslated(tx->GetHash().ToString());

    return true;
}

std::string CTransactionBuilder::ToString() const
{
    return strprintf("CTransactionBuilder(Amount initial: %d, Amount left: %d, Bytes base: %d, Bytes output: %d, Bytes total: %d, Amount used: %d, Outputs: %d, Fee rate: %d, Discard fee rate: %d, Fee: %d)",
        GetAmountInitial(),
        GetAmountLeft(),
        nBytesBase,
        nBytesOutput,
        GetBytesTotal(),
        GetAmountUsed(),
        CountOutputs(),
        coinControl.m_feerate->GetFeePerK(),
        coinControl.m_discard_feerate->GetFeePerK(),
        GetFee(GetBytesTotal()));
}
