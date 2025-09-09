// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/coinjoin.h>

#include <key_io.h>
#include <wallet/receive.h>
#include <wallet/spend.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>

#include <coinjoin/common.h>
#include <coinjoin/options.h>

namespace wallet {
void CWallet::InitCJSaltFromDb()
{
    assert(nCoinJoinSalt.IsNull());

    WalletBatch batch(GetDatabase());
    if (!batch.ReadCoinJoinSalt(nCoinJoinSalt) && batch.ReadCoinJoinSalt(nCoinJoinSalt, true)) {
        // Migrate salt stored with legacy key
        batch.WriteCoinJoinSalt(nCoinJoinSalt);
    }
}

const uint256& CWallet::GetCoinJoinSalt()
{
    if (nCoinJoinSalt.IsNull()) {
        InitCJSaltFromDb();
    }
    return nCoinJoinSalt;
}

bool CWallet::SetCoinJoinSalt(const uint256& cj_salt)
{
    WalletBatch batch(GetDatabase());
    // Only store new salt in CWallet if database write is successful
    if (batch.WriteCoinJoinSalt(cj_salt)) {
        nCoinJoinSalt = cj_salt;
        return true;
    }
    return false;
}

bool CWallet::SelectTxDSInsByDenomination(int nDenom, CAmount nValueMax, std::vector<CTxDSIn>& vecTxDSInRet)
{
    LOCK(cs_wallet);

    vecTxDSInRet.clear();
    if (!CoinJoin::IsValidDenomination(nDenom)) {
        return false;
    }

    CAmount nDenomAmount{CoinJoin::DenominationToAmount(nDenom)};
    CAmount nValueTotal{0};
    CCoinControl coin_control(CoinType::ONLY_READY_TO_MIX);
    std::set<uint256> setRecentTxIds;
    std::vector<COutput> vCoins{AvailableCoinsListUnspent(*this, &coin_control).all()};

    WalletCJLogPrint(this, "CWallet::%s -- vCoins.size(): %d\n", __func__, vCoins.size());

    Shuffle(vCoins.rbegin(), vCoins.rend(), FastRandomContext());

    for (const auto& out : vCoins) {
        uint256 txHash = out.outpoint.hash;
        CAmount nValue = out.txout.nValue;
        if (setRecentTxIds.find(txHash) != setRecentTxIds.end()) continue; // no duplicate txids
        if (nValueTotal + nValue > nValueMax) continue;
        if (nValue != nDenomAmount) continue;

        CTxIn txin = CTxIn(txHash, out.outpoint.n);
        CScript scriptPubKey = out.txout.scriptPubKey;
        int nRounds = GetRealOutpointCoinJoinRounds(txin.prevout);

        nValueTotal += nValue;
        vecTxDSInRet.emplace_back(CTxDSIn(txin, scriptPubKey, nRounds));
        setRecentTxIds.emplace(txHash);
        WalletCJLogPrint(this, "CWallet::%s -- hash: %s, nValue: %d.%08d\n", __func__, txHash.ToString(), nValue / COIN,
                         nValue % COIN);
    }

    WalletCJLogPrint(this, "CWallet::%s -- setRecentTxIds.size(): %d\n", __func__, setRecentTxIds.size());

    return nValueTotal > 0;
}

struct CompareByPriority {
    bool operator()(const COutput& t1, const COutput& t2) const
    {
        return CoinJoin::CalculateAmountPriority(t1.GetEffectiveValue()) >
               CoinJoin::CalculateAmountPriority(t2.GetEffectiveValue());
    }
};

bool CWallet::SelectDenominatedAmounts(CAmount nValueMax, std::set<CAmount>& setAmountsRet) const
{
    LOCK(cs_wallet);

    setAmountsRet.clear();

    CAmount nValueTotal{0};
    CCoinControl coin_control(CoinType::ONLY_READY_TO_MIX);
    // CompareByPriority() cares about effective value, which is only calculable when supplied a feerate
    std::vector<COutput> vCoins{AvailableCoins(*this, &coin_control, /*feerate=*/CFeeRate(0)).all()};

    // larger denoms first
    std::sort(vCoins.rbegin(), vCoins.rend(), CompareByPriority());

    for (const auto& out : vCoins) {
        CAmount nValue = out.txout.nValue;
        if (nValueTotal + nValue <= nValueMax) {
            nValueTotal += nValue;
            setAmountsRet.emplace(nValue);
        }
    }

    return nValueTotal >= CoinJoin::GetSmallestDenomination();
}

std::vector<CompactTallyItem> CWallet::SelectCoinsGroupedByAddresses(bool fSkipDenominated, bool fAnonymizable,
                                                                     bool fSkipUnconfirmed, int nMaxOupointsPerAddress) const
{
    LOCK(cs_wallet);

    isminefilter filter = ISMINE_SPENDABLE;

    // Try using the cache for already confirmed mixable inputs.
    // This should only be used if nMaxOupointsPerAddress was NOT specified.
    if (nMaxOupointsPerAddress == -1 && fAnonymizable && fSkipUnconfirmed) {
        if (fSkipDenominated && fAnonymizableTallyCachedNonDenom) {
            LogPrint(BCLog::SELECTCOINS, "SelectCoinsGroupedByAddresses - using cache for non-denom inputs %d\n",
                     vecAnonymizableTallyCachedNonDenom.size());
            return vecAnonymizableTallyCachedNonDenom;
        }
        if (!fSkipDenominated && fAnonymizableTallyCached) {
            LogPrint(BCLog::SELECTCOINS, "SelectCoinsGroupedByAddresses - using cache for all inputs %d\n",
                     vecAnonymizableTallyCached.size());
            return vecAnonymizableTallyCached;
        }
    }

    CAmount nSmallestDenom = CoinJoin::GetSmallestDenomination();

    // Tally
    std::map<CTxDestination, CompactTallyItem> mapTally;
    std::set<uint256> setWalletTxesCounted;
    for (const auto& outpoint : setWalletUTXO) {
        if (!setWalletTxesCounted.emplace(outpoint.hash).second) continue;

        const auto it{mapWallet.find(outpoint.hash)};
        if (it == mapWallet.end()) continue;

        const CWalletTx& wtx{(*it).second};

        if (wtx.IsCoinBase() && GetTxBlocksToMaturity(wtx) > 0) continue;
        if (fSkipUnconfirmed && !CachedTxIsTrusted(*this, wtx)) continue;
        if (GetTxDepthInMainChain(wtx) < 0) continue;

        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            CTxDestination txdest;
            if (!ExtractDestination(wtx.tx->vout[i].scriptPubKey, txdest)) continue;

            isminefilter mine = IsMine(txdest);
            if (!(mine & filter)) continue;

            auto itTallyItem = mapTally.find(txdest);
            if (nMaxOupointsPerAddress != -1 && itTallyItem != mapTally.end() &&
                int64_t(itTallyItem->second.outpoints.size()) >= nMaxOupointsPerAddress) {
                continue;
            }

            COutPoint target_outpoint(outpoint.hash, i);
            if (IsSpent(target_outpoint) || IsLockedCoin(target_outpoint)) continue;

            if (fSkipDenominated && CoinJoin::IsDenominatedAmount(wtx.tx->vout[i].nValue)) continue;

            if (fAnonymizable) {
                // ignore collaterals
                if (CoinJoin::IsCollateralAmount(wtx.tx->vout[i].nValue)) continue;
                // ignore outputs that are 10 times smaller then the smallest denomination
                // otherwise they will just lead to higher fee / lower priority
                if (wtx.tx->vout[i].nValue <= nSmallestDenom / 10) continue;
                // ignore mixed
                if (IsFullyMixed(target_outpoint)) continue;
            }

            if (itTallyItem == mapTally.end()) {
                itTallyItem = mapTally.emplace(txdest, CompactTallyItem()).first;
                itTallyItem->second.txdest = txdest;
            }
            itTallyItem->second.nAmount += wtx.tx->vout[i].nValue;
            itTallyItem->second.outpoints.emplace_back(COutPoint{outpoint.hash, i});
        }
    }

    // construct resulting vector
    // NOTE: vecTallyRet is "sorted" by txdest (i.e. address), just like mapTally
    std::vector<CompactTallyItem> vecTallyRet;
    for (const auto& item : mapTally) {
        if (fAnonymizable && item.second.nAmount < nSmallestDenom) continue;
        vecTallyRet.push_back(item.second);
    }

    // Cache already confirmed mixable entries for later use.
    // This should only be used if nMaxOupointsPerAddress was NOT specified.
    if (nMaxOupointsPerAddress == -1 && fAnonymizable && fSkipUnconfirmed) {
        if (fSkipDenominated) {
            vecAnonymizableTallyCachedNonDenom = vecTallyRet;
            fAnonymizableTallyCachedNonDenom = true;
        } else {
            vecAnonymizableTallyCached = vecTallyRet;
            fAnonymizableTallyCached = true;
        }
    }

    // debug
    if (LogAcceptDebug(BCLog::SELECTCOINS)) {
        std::string strMessage = "SelectCoinsGroupedByAddresses - vecTallyRet:\n";
        for (const auto& item : vecTallyRet) {
            strMessage += strprintf("  %s %f\n", EncodeDestination(item.txdest), float(item.nAmount) / COIN);
        }
        LogPrint(BCLog::SELECTCOINS, "%s", strMessage); /* Continued */
    }

    return vecTallyRet;
}

bool CWallet::HasCollateralInputs(bool fOnlyConfirmed) const
{
    LOCK(cs_wallet);

    CCoinControl coin_control(CoinType::ONLY_COINJOIN_COLLATERAL);
    coin_control.m_include_unsafe_inputs = !fOnlyConfirmed;

    return AvailableCoinsListUnspent(*this, &coin_control).size() > 0;
}

int CWallet::CountInputsWithAmount(CAmount nInputAmount) const
{
    CAmount nTotal = 0;

    LOCK(cs_wallet);

    for (const auto& outpoint : setWalletUTXO) {
        const auto it{mapWallet.find(outpoint.hash)};
        if (it == mapWallet.end()) continue;
        if (it->second.tx->vout[outpoint.n].nValue != nInputAmount) continue;
        if (GetTxDepthInMainChain(it->second) < 0) continue;

        nTotal++;
    }

    return nTotal;
}

// Recursively determine the rounds of a given input (How deep is the CoinJoin chain for a given input)
int CWallet::GetRealOutpointCoinJoinRounds(const COutPoint& outpoint, int nRounds) const
{
    LOCK(cs_wallet);

    const int nRoundsMax{MAX_COINJOIN_ROUNDS + CCoinJoinClientOptions::GetRandomRounds()};

    if (nRounds >= nRoundsMax) {
        // there can only be nRoundsMax rounds max
        return nRoundsMax - 1;
    }

    auto pair = mapOutpointRoundsCache.emplace(outpoint, -10);
    auto nRoundsRef = &pair.first->second;
    if (!pair.second) {
        // we already processed it, just return what we have
        return *nRoundsRef;
    }

    // TODO wtx should refer to a CWalletTx object, not a pointer, based on surrounding code
    const CWalletTx* wtx{GetWalletTx(outpoint.hash)};

    if (wtx == nullptr || wtx->tx == nullptr) {
        // no such tx in this wallet
        *nRoundsRef = -1;
        WalletCJLogPrint(this, "%s FAILED    %-70s %3d\n", __func__, outpoint.ToStringShort(), -1);
        return *nRoundsRef;
    }

    // bounds check
    if (outpoint.n >= wtx->tx->vout.size()) {
        // should never actually hit this
        *nRoundsRef = -4;
        WalletCJLogPrint(this, "%s FAILED    %-70s %3d\n", __func__, outpoint.ToStringShort(), -4);
        return *nRoundsRef;
    }

    auto txOutRef = &wtx->tx->vout[outpoint.n];

    if (CoinJoin::IsCollateralAmount(txOutRef->nValue)) {
        *nRoundsRef = -3;
        WalletCJLogPrint(this, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
        return *nRoundsRef;
    }

    // make sure the final output is non-denominate
    if (!CoinJoin::IsDenominatedAmount(txOutRef->nValue)) { // NOT DENOM
        *nRoundsRef = -2;
        WalletCJLogPrint(this, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
        return *nRoundsRef;
    }

    for (const auto& out : wtx->tx->vout) {
        if (!CoinJoin::IsDenominatedAmount(out.nValue)) {
            // this one is denominated but there is another non-denominated output found in the same tx
            *nRoundsRef = 0;
            WalletCJLogPrint(this, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
            return *nRoundsRef;
        }
    }

    // make sure we spent all of it with 0 fee, reset to 0 rounds otherwise
    if (CachedTxGetDebit(*this, *wtx, ISMINE_SPENDABLE) != CachedTxGetCredit(*this, *wtx, ISMINE_SPENDABLE)) {
        *nRoundsRef = 0;
        WalletCJLogPrint(this, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
        return *nRoundsRef;
    }

    int nShortest = -10; // an initial value, should be no way to get this by calculations
    bool fDenomFound = false;
    // only denoms here so let's look up
    for (const auto& txinNext : wtx->tx->vin) {
        if (InputIsMine(*this, txinNext)) {
            int n = GetRealOutpointCoinJoinRounds(txinNext.prevout, nRounds + 1);
            // denom found, find the shortest chain or initially assign nShortest with the first found value
            if (n >= 0 && (n < nShortest || nShortest == -10)) {
                nShortest = n;
                fDenomFound = true;
            }
        }
    }
    *nRoundsRef = [&]() {
        if (fDenomFound) {
            // good, we a +1 to the shortest one but only nRoundsMax rounds max allowed
            return nShortest >= nRoundsMax - 1 ? nRoundsMax : nShortest + 1;
        }
        // too bad, we are the first one in that chain
        return 0;
    }();
    WalletCJLogPrint(this, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
    return *nRoundsRef;
}

// respect current settings
int CWallet::GetCappedOutpointCoinJoinRounds(const COutPoint& outpoint) const
{
    LOCK(cs_wallet);
    int realCoinJoinRounds = GetRealOutpointCoinJoinRounds(outpoint);
    return realCoinJoinRounds > CCoinJoinClientOptions::GetRounds() ? CCoinJoinClientOptions::GetRounds()
                                                                    : realCoinJoinRounds;
}

void CWallet::ClearCoinJoinRoundsCache()
{
    LOCK(cs_wallet);
    mapOutpointRoundsCache.clear();
    MarkDirty();
    // Notify UI
    NotifyTransactionChanged(uint256::ONE, CT_UPDATED);
}

bool CWallet::IsDenominated(const COutPoint& outpoint) const
{
    LOCK(cs_wallet);

    const auto it{mapWallet.find(outpoint.hash)};
    if (it == mapWallet.end()) {
        return false;
    }

    if (outpoint.n >= it->second.tx->vout.size()) {
        return false;
    }

    return CoinJoin::IsDenominatedAmount(it->second.tx->vout[outpoint.n].nValue);
}

bool CWallet::IsFullyMixed(const COutPoint& outpoint) const
{
    int nRounds = GetRealOutpointCoinJoinRounds(outpoint);
    // Mix again if we don't have N rounds yet
    if (nRounds < CCoinJoinClientOptions::GetRounds()) return false;

    // Try to mix a "random" number of rounds more than minimum.
    // If we have already mixed N + MaxOffset rounds, don't mix again.
    // Otherwise, we should mix again 50% of the time, this results in an exponential decay
    // N rounds 50% N+1 25% N+2 12.5%... until we reach N + GetRandomRounds() rounds where we stop.
    if (nRounds < CCoinJoinClientOptions::GetRounds() + CCoinJoinClientOptions::GetRandomRounds()) {
        CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << outpoint << nCoinJoinSalt;
        uint256 nHash;
        CSHA256().Write(reinterpret_cast<const uint8_t*>(ss.data()), ss.size()).Finalize(nHash.begin());
        if (ReadLE64(nHash.begin()) % 2 == 0) {
            return false;
        }
    }

    return true;
}

void CWallet::RecalculateMixedCredit(const uint256 hash)
{
    AssertLockHeld(cs_wallet);
    if (auto it = mapWallet.find(hash); it != mapWallet.end()) {
        // Recalculate all credits for this tx
        it->second.MarkDirty();
    }
    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

CAmount GetBalanceAnonymized(const CWallet& wallet, const CCoinControl& coinControl)
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    CAmount anonymized_amount{0};
    LOCK(wallet.cs_wallet);
    for (auto pcoin : wallet.GetSpendableTXs()) {
        anonymized_amount += CachedTxGetAnonymizedCredit(wallet, *pcoin, coinControl);
    }
    return anonymized_amount;
}

CAmount CWallet::GetAnonymizableBalance(bool fSkipDenominated, bool fSkipUnconfirmed) const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    std::vector<CompactTallyItem> vecTally = SelectCoinsGroupedByAddresses(fSkipDenominated, true, fSkipUnconfirmed);
    if (vecTally.empty()) return 0;

    CAmount nTotal = 0;

    const CAmount nSmallestDenom{CoinJoin::GetSmallestDenomination()};
    const CAmount nMixingCollateral{CoinJoin::GetCollateralAmount()};
    for (const auto& item : vecTally) {
        bool fIsDenominated = CoinJoin::IsDenominatedAmount(item.nAmount);
        if (fSkipDenominated && fIsDenominated) continue;
        // assume that the fee to create denoms should be mixing collateral at max
        if (item.nAmount >= nSmallestDenom + (fIsDenominated ? 0 : nMixingCollateral)) nTotal += item.nAmount;
    }

    return nTotal;
}

// Note: calculated including unconfirmed,
// that's ok as long as we use it for informational purposes only
float CWallet::GetAverageAnonymizedRounds() const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    int nTotal = 0;
    int nCount = 0;

    LOCK(cs_wallet);
    for (const auto& outpoint : setWalletUTXO) {
        if (!IsDenominated(outpoint)) continue;

        nTotal += GetCappedOutpointCoinJoinRounds(outpoint);
        nCount++;
    }

    if (nCount == 0) return 0;

    return (float)nTotal / nCount;
}

// Note: calculated including unconfirmed,
// that's ok as long as we use it for informational purposes only
CAmount CWallet::GetNormalizedAnonymizedBalance() const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    CAmount nTotal = 0;

    LOCK(cs_wallet);
    for (const auto& outpoint : setWalletUTXO) {
        const auto it{mapWallet.find(outpoint.hash)};
        if (it == mapWallet.end()) continue;

        CAmount nValue = it->second.tx->vout[outpoint.n].nValue;
        if (!CoinJoin::IsDenominatedAmount(nValue)) continue;
        if (GetTxDepthInMainChain(it->second) < 0) continue;

        int nRounds = GetCappedOutpointCoinJoinRounds(outpoint);
        nTotal += nValue * nRounds / CCoinJoinClientOptions::GetRounds();
    }

    return nTotal;
}

CAmount CachedTxGetAnonymizedCredit(const CWallet& wallet, const CWalletTx& wtx, const CCoinControl& coinControl)
{
    AssertLockHeld(wallet.cs_wallet);

    // Exclude coinbase and conflicted txes
    if (wtx.IsCoinBase() || wallet.GetTxDepthInMainChain(wtx) < 0) return 0;

    CAmount nCredit = 0;
    uint256 hashTx = wtx.GetHash();
    for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
        const CTxOut& txout{wtx.tx->vout[i]};
        const COutPoint outpoint(hashTx, i);

        if (coinControl.HasSelected() && !coinControl.IsSelected(outpoint)) {
            continue;
        }

        if (wallet.IsSpent(outpoint) || !CoinJoin::IsDenominatedAmount(txout.nValue)) {
            continue;
        }

        if (!wallet.IsFullyMixed(outpoint)) {
            continue;
        }

        nCredit += OutputGetCredit(wallet, txout, ISMINE_SPENDABLE);
        if (!MoneyRange(nCredit)) {
            throw std::runtime_error(std::string(__func__) + ": value out of range");
        }
    }

    return nCredit;
}

CoinJoinCredits CachedTxGetAvailableCoinJoinCredits(const CWallet& wallet, const CWalletTx& wtx)
{
    CoinJoinCredits ret;

    AssertLockHeld(wallet.cs_wallet);

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (wtx.IsCoinBase() && wallet.GetTxBlocksToMaturity(wtx) > 0) return ret;

    int nDepth = wallet.GetTxDepthInMainChain(wtx);
    if (nDepth < 0) return ret;

    ret.is_unconfirmed = CachedTxIsTrusted(wallet, wtx) && nDepth == 0;

    if (wtx.m_amounts[CWalletTx::ANON_CREDIT].m_cached[ISMINE_SPENDABLE]) {
        if (ret.is_unconfirmed && wtx.m_amounts[CWalletTx::DENOM_UCREDIT].m_cached[ISMINE_SPENDABLE]) {
            return {wtx.m_amounts[CWalletTx::ANON_CREDIT].m_value[ISMINE_SPENDABLE],
                    wtx.m_amounts[CWalletTx::DENOM_UCREDIT].m_value[ISMINE_SPENDABLE], ret.is_unconfirmed};
        } else if (!ret.is_unconfirmed && wtx.m_amounts[CWalletTx::DENOM_CREDIT].m_cached[ISMINE_SPENDABLE]) {
            return {wtx.m_amounts[CWalletTx::ANON_CREDIT].m_value[ISMINE_SPENDABLE],
                    wtx.m_amounts[CWalletTx::DENOM_CREDIT].m_value[ISMINE_SPENDABLE], ret.is_unconfirmed};
        }
    }

    uint256 hashTx = wtx.GetHash();
    for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
        const CTxOut& txout{wtx.tx->vout[i]};
        const COutPoint outpoint(hashTx, i);

        if (wallet.IsSpent(outpoint) || !CoinJoin::IsDenominatedAmount(txout.nValue)) {
            continue;
        }

        const CAmount credit{OutputGetCredit(wallet, txout, ISMINE_SPENDABLE)};
        if (wallet.IsFullyMixed(outpoint)) {
            ret.m_anonymized += credit;
            if (!MoneyRange(ret.m_anonymized)) {
                throw std::runtime_error(std::string(__func__) + ": value out of range");
            }
        }

        ret.m_denominated += credit;
        if (!MoneyRange(ret.m_denominated)) {
            throw std::runtime_error(std::string(__func__) + ": value out of range");
        }
    }

    wtx.m_amounts[CWalletTx::ANON_CREDIT].Set(ISMINE_SPENDABLE, ret.m_anonymized);
    if (ret.is_unconfirmed) {
        wtx.m_amounts[CWalletTx::DENOM_UCREDIT].Set(ISMINE_SPENDABLE, ret.m_denominated);
    } else {
        wtx.m_amounts[CWalletTx::DENOM_CREDIT].Set(ISMINE_SPENDABLE, ret.m_denominated);
    }
    return ret;
}
} // namespace wallet
