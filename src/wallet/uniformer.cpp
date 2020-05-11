// Copyright (c) 2017-2020 The BitcoinHD Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/uniformer.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <key_io.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/wallet.h>
#include <txmempool.h>
#include <consensus/tx_verify.h>
#include <util/moneystr.h>
#include <util/rbf.h>
#include <util/system.h>
#include <util/translation.h>
#include <util/validation.h>
#include <net.h>

//! Check whether transaction has descendant in wallet, or has been
//! mined, or conflicts with a mined transaction. Return a uniformer::Result.
static uniformer::Result PreconditionChecks(interfaces::Chain::Lock& locked_chain, const CWallet* wallet, const CWalletTx& wtx, std::vector<std::string>& errors) EXCLUSIVE_LOCKS_REQUIRED(wallet->cs_wallet)
{
    if (wallet->HasWalletSpend(wtx.GetHash())) {
        errors.push_back("Transaction has descendants in the wallet");
        return uniformer::Result::INVALID_PARAMETER;
    }

    if (wtx.GetDepthInMainChain(locked_chain) <= 0) {
        errors.push_back("Transaction not mined, or is conflicted with a mined transaction");
        return uniformer::Result::WALLET_ERROR;
    }

    // check that original tx consists entirely of our inputs
    if (!wallet->IsAllFromMe(*wtx.tx, ISMINE_SPENDABLE)) {
        errors.push_back("Transaction contains inputs that don't belong to this wallet");
        return uniformer::Result::WALLET_ERROR;
    }

    // freeze tx
    auto txAction = wtx.GetTxAction();
    if (txAction != CWalletTx::TX_BINDPLOTTER && txAction != CWalletTx::TX_POINT) {
        errors.push_back("Transaction can't unfreeze");
        return uniformer::Result::INVALID_PARAMETER;
    }

    return uniformer::Result::OK;
}

namespace uniformer {

bool CoinCanBeUnfreeze(const CWallet* wallet, const COutPoint& outpoint)
{
    if (outpoint.n != 0)
        return false;

    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    const CWalletTx* wtx = wallet->GetWalletTx(outpoint.hash);
    if (wtx == nullptr) return false;

    std::vector<std::string> errors_dummy;
    Result res = PreconditionChecks(*locked_chain, wallet, *wtx, errors_dummy);
    return res == Result::OK;
}

Result CreateBindPlotterTransaction(CWallet* wallet, const CTxDestination &dest, const CScript &bindScriptData, bool fAllowHighFee,
                                    const CCoinControl& coin_control, std::vector<std::string>& errors, CAmount& txfee, CMutableTransaction& mtx)
{
    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    errors.clear();
    
    CAccountID accountID = ExtractAccountID(dest);
    if (accountID.IsNull()) {
        errors.push_back("Invalid bind destination");
        return Result::INVALID_ADDRESS_OR_KEY;
    }
    
    uint64_t plotterId = GetBindPlotterIdFromScript(bindScriptData);
    if (plotterId == 0) {
        errors.push_back("Invalid bind data");
        return Result::INVALID_PARAMETER;
    }

    const Consensus::Params& params = Params().GetConsensus();
    int nSpendHeight = locked_chain->getHeight().get_value_or(0) + 1;
    if (nSpendHeight < params.BHDIP006Height) { // Check active status
        errors.push_back(strprintf("The bind plotter inactive (Will active on %d)", params.BHDIP006Height));
        return Result::INVALID_REQUEST;
    }

    // Check already actived bind
    if (wallet->chain().haveActiveBindPlotter(accountID, plotterId)) {
        errors.push_back(strprintf("The plotter %ull already binded to %s and actived.", plotterId, EncodeDestination(dest)));
        return Result::INVALID_REQUEST;
    }

    // Create special coin control for bind plotter
    CCoinControl realCoinControl = coin_control;
    realCoinControl.m_coin_pick_policy = CoinPickPolicy::IncludeIfSet;
    realCoinControl.m_pick_dest = dest;
    realCoinControl.destChange = dest;
    if (nSpendHeight >= params.BHDIP006CheckRelayHeight) { // Limit bind plotter minimal fee
        realCoinControl.m_min_txfee = PROTOCOL_BINDPLOTTER_MINFEE;
    }
    // Calculate bind transaction fee
    if (CAmount punishmentReward = wallet->chain().getBindPlotterPunishment(nSpendHeight, plotterId).first) { // Calculate bind transaction fee
        realCoinControl.m_min_txfee = std::max(realCoinControl.m_min_txfee, punishmentReward + PROTOCOL_BINDPLOTTER_MINFEE);
        if (!fAllowHighFee) {
            errors.push_back(strprintf("This binding operation triggers a pledge anti-cheating mechanism and therefore requires a large bind plotter fee %s BHD", FormatMoney(realCoinControl.m_min_txfee)));
            return Result::BIND_HIGHFEE_ERROR;
        }
    }

    // Create bind plotter transaction
    std::string strError;
    std::vector<CRecipient> vecSend = { {GetScriptForDestination(dest), PROTOCOL_BINDPLOTTER_LOCKAMOUNT, false}, {bindScriptData, 0, false} };
    int nChangePosRet = 1;
    CTransactionRef tx;
    if (!wallet->CreateTransaction(*locked_chain, vecSend, tx, txfee, nChangePosRet, strError, realCoinControl, false, CTransaction::UNIFORM_VERSION)) {
        errors.push_back(strError);
        return Result::WALLET_ERROR;
    }

    // Check
    bool fReject = false;
    int lastActiveHeight = 0;
    CDatacarrierPayloadRef payload = ExtractTransactionDatacarrier(*tx, nSpendHeight, DatacarrierTypes{DATACARRIER_TYPE_BINDPLOTTER}, fReject, lastActiveHeight);
    if (!payload) {
        if (fReject)
            errors.push_back("Not for current address");
        else if (lastActiveHeight != 0 && lastActiveHeight < nSpendHeight)
            errors.push_back(strprintf("Invalid active height. Last active height is %d", lastActiveHeight));
        else
            errors.push_back("Invalid bind hex data");
        return Result::INVALID_PARAMETER;
    }
    assert(payload->type == DATACARRIER_TYPE_BINDPLOTTER);

    // return
    mtx = CMutableTransaction(*tx);
    return Result::OK;
}

//! Create point transaction.
Result CreatePointTransaction(CWallet* wallet, const CTxDestination &senderDest, const CTxDestination &receiverDest, CAmount nAmount, bool fSubtractFeeFromAmount,
                              const CCoinControl& coin_control, std::vector<std::string>& errors, CAmount& txfee, CMutableTransaction& mtx)
{
    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    errors.clear();

    if (nAmount <= 0) {
        errors.push_back("Invalid amount");
        return Result::INVALID_PARAMETER;
    } if (nAmount < PROTOCOL_POINT_AMOUNT_MIN) {
        errors.push_back(strprintf("Point amount too minimal, require more than %s BHD", FormatMoney(PROTOCOL_POINT_AMOUNT_MIN)));
        return Result::INVALID_PARAMETER;
    }

    // Create special coin control for point
    CCoinControl realCoinControl = coin_control;
    realCoinControl.m_coin_pick_policy = CoinPickPolicy::IncludeIfSet;
    realCoinControl.m_pick_dest = senderDest;
    realCoinControl.destChange = senderDest;

    // Create point transaction
    std::string strError;
    std::vector<CRecipient> vecSend = { {GetScriptForDestination(senderDest), nAmount, fSubtractFeeFromAmount}, {GetPointScriptForDestination(receiverDest), 0, false} };
    int nChangePosRet = 1;
    CTransactionRef tx;
    if (!wallet->CreateTransaction(*locked_chain, vecSend, tx, txfee, nChangePosRet, strError, realCoinControl, false, CTransaction::UNIFORM_VERSION)) {
        errors.push_back(strError);
        return Result::WALLET_ERROR;
    }

    // Check
    CDatacarrierPayloadRef payload = ExtractTransactionDatacarrier(*tx, locked_chain->getHeight().get_value_or(0) + 1, DatacarrierTypes{DATACARRIER_TYPE_POINT});
    if (!payload) {
        errors.push_back("Error on create point transaction");
        return Result::WALLET_ERROR;
    }
    assert(payload->type == DATACARRIER_TYPE_POINT);

    // Return
    mtx = CMutableTransaction(*tx);
    return Result::OK;
}

Result CreateUnfreezeTransaction(CWallet* wallet, const COutPoint& outpoint,
                                 const CCoinControl& coin_control, std::vector<std::string>& errors, CAmount& txfee, CMutableTransaction& mtx)
{
    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    const CWalletTx* wtx = wallet->GetWalletTx(outpoint.hash);
    if (wtx == nullptr || outpoint.n != 0 || wtx->IsUnfrozen(*locked_chain)) {
        errors.push_back("Can't unfreeze");
        return Result::INVALID_REQUEST;
    }

    Result res = PreconditionChecks(*locked_chain, wallet, *wtx, errors);
    if (res != Result::OK) {
        return res;
    }

    // Check UTXO
    const Coin &coin = wallet->chain().accessCoin(outpoint);
    if (coin.IsSpent() || coin.GetExtraDataType() == DATACARRIER_TYPE_UNKNOWN) {
        errors.push_back("Can't unfreeze");
        return Result::INVALID_REQUEST;
    }

    // Check unbind limit
    if (coin.IsBindPlotter()) {
        int nSpendHeight = locked_chain->getHeight().get_value_or(0) + 1;
        int nActiveHeight = wallet->chain().getUnbindPlotterLimitHeight(CBindPlotterInfo(outpoint, coin));
        if (nSpendHeight < nActiveHeight) {
            errors.push_back(strprintf("Unbind plotter active on %d block height (%d blocks after, about %d minute)",
                    nActiveHeight,
                    nActiveHeight - nSpendHeight,
                    (nActiveHeight - nSpendHeight) * Consensus::GetTargetSpacing(nSpendHeight, Params().GetConsensus()) / 60));
            return Result::WALLET_ERROR;
        }
    }

    // Create transaction
    CMutableTransaction txNew;
    txNew.nLockTime = locked_chain->getHeight().get_value_or(0);
    txNew.nVersion = CTransaction::UNIFORM_VERSION;
    txNew.vin = { CTxIn(outpoint, CScript(), coin_control.m_signal_bip125_rbf.get_value_or(wallet->m_signal_rbf) ? MAX_BIP125_RBF_SEQUENCE : (CTxIn::SEQUENCE_FINAL - 1)) };
    txNew.vout = { CTxOut(coin.out.nValue, coin.out.scriptPubKey) };
    int64_t nBytes = CalculateMaximumSignedTxSize(CTransaction(txNew), wallet, coin_control.fAllowWatchOnly);
    if (nBytes < 0) {
        errors.push_back(_("Signing transaction failed").translated);
        return Result::WALLET_ERROR;
    }
    txfee = GetMinimumFee(*wallet, nBytes, coin_control, nullptr);
    txNew.vout[0].nValue -= txfee;

    // Check
    if (txNew.vin.size() != 1 || txNew.vin[0].prevout != outpoint || txNew.vout.size() != 1 || txNew.vout[0].scriptPubKey != coin.out.scriptPubKey) {
        errors.push_back("Error on create unfreeze transaction");
        return Result::WALLET_ERROR;
    }

    // return
    mtx = std::move(txNew);
    return Result::OK;
}

bool SignTransaction(CWallet* wallet, CMutableTransaction& mtx) {
    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    return wallet->SignTransaction(mtx);
}

Result CommitTransaction(CWallet* wallet, CMutableTransaction&& mtx, std::map<std::string, std::string>&& mapValue, std::vector<std::string>& errors)
{
    auto locked_chain = wallet->chain().lock();
    LOCK(wallet->cs_wallet);
    if (!errors.empty()) {
        return Result::MISC_ERROR;
    }

    // commit/broadcast the tx
    CTransactionRef tx = MakeTransactionRef(std::move(mtx));
    CValidationState state;
    if (!wallet->CommitTransaction(tx, std::move(mapValue), {} /* orderForm */, state)) {
        // NOTE: CommitTransaction never returns false, so this should never happen.
        errors.push_back(strprintf("The transaction was rejected: %s", FormatStateMessage(state)));
        return Result::WALLET_ERROR;
    }

    if (state.IsInvalid()) {
        // This can happen if the mempool rejected the transaction.  Report
        // what happened in the "errors" response.
        errors.push_back(strprintf("Error: The transaction was rejected: %s", FormatStateMessage(state)));
    }

    return Result::OK;
}

} // namespace feebumper
