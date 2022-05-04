#include <wallet/txassembler.h>

#include <consensus/tx_check.h>
#include <consensus/validation.h>
#include <policy/policy.h>
#include <util/check.h>
#include <util/fees.h>
#include <util/rbf.h>
#include <util/translation.h>
#include <wallet/fees.h>
#include <wallet/reserve.h>

Optional<AssembledTx> TxAssembler::AssembleTx(
    const std::vector<CRecipient>& recipients,
    const CCoinControl& coin_control,
    const int nChangePosRequest,
    const bool sign,
    bilingual_str& errorOut)
{
    try {
        return CreateTransaction(recipients, coin_control, nChangePosRequest, sign);
    } catch (const CreateTxError& e) {
        m_wallet.WalletLogPrintf("%s\n", e.what());
        errorOut = e.GetError();
    }

    return nullopt;
}

AssembledTx TxAssembler::CreateTransaction(
    const std::vector<CRecipient>& recipients,
    const CCoinControl& coin_control,
    const int nChangePosRequest,
    const bool sign)
{
    VerifyRecipients(recipients);

    if (boost::get<StealthAddress>(&coin_control.destChange)) {
        throw CreateTxError(_("Custom MWEB change addresses not yet supported"));
    }

    InProcessTx new_tx;
    new_tx.recipients = recipients;
    new_tx.recipient_amount = std::accumulate(
        new_tx.recipients.cbegin(), new_tx.recipients.cend(), CAmount(0),
        [](CAmount amt, const CRecipient& recipient) { return amt + recipient.nAmount; }
    );
    new_tx.subtract_fee_from_amount = std::count_if(recipients.cbegin(), recipients.cend(),
        [](const CRecipient& recipient) { return recipient.fSubtractFeeFromAmount; }
    );
    new_tx.coin_control = coin_control;

    {
        LOCK(m_wallet.cs_wallet);
        CreateTransaction_Locked(new_tx, nChangePosRequest, sign);
    }

    // Return the constructed transaction data.
    CTransactionRef tx = MakeTransactionRef(std::move(new_tx.tx));

    // Limit size
    if (GetTransactionWeight(*tx) > MAX_STANDARD_TX_WEIGHT) {
        throw CreateTxError(_("Transaction too large"));
    }

    TxValidationState validation_state;
    if (!CheckTransaction(*tx, validation_state)) {
        throw CreateTxError(_("Transaction is invalid"));
    }

    if (new_tx.total_fee > m_wallet.m_default_max_tx_fee) {
        throw CreateTxError(TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED));
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        if (!m_wallet.chain().checkChainLimits(tx)) {
            throw CreateTxError(_("Transaction has too long of a mempool chain"));
        }
    }

    // Before we return success, we assume any change key will be used to prevent
    // accidental re-use.
    if (new_tx.reserve_dest != nullptr) {
        new_tx.reserve_dest->KeepDestination();
    }

    const FeeCalculation& feeCalc = new_tx.fee_calc;
    m_wallet.WalletLogPrintf("Fee Calculation: Fee:%d Bytes:%u Needed:%d Tgt:%d (requested %d) Reason:\"%s\" Decay %.5f: Estimation: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
        new_tx.total_fee, new_tx.bytes, new_tx.fee_needed, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
        feeCalc.est.pass.start, feeCalc.est.pass.end,
        (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) > 0.0 ? 100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) : 0.0,
        feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
        feeCalc.est.fail.start, feeCalc.est.fail.end,
        (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) > 0.0 ? 100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) : 0.0,
        feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);

    return AssembledTx{tx, new_tx.total_fee, new_tx.fee_calc, new_tx.change_position};
}

void TxAssembler::CreateTransaction_Locked(
    InProcessTx& new_tx,
    const int nChangePosRequest,
    const bool sign)
{
    AssertLockHeld(m_wallet.cs_wallet);

    new_tx.tx.nLockTime = GetLocktimeForNewTransaction();

    m_wallet.AvailableCoins(new_tx.available_coins, true, &new_tx.coin_control, 1, MAX_MONEY, MAX_MONEY, 0);
    UpdateChangeAddress(new_tx);

    InitCoinSelectionParams(new_tx);

    bool pick_new_inputs = true;

    // Starts with no fee and loops until there is enough fee
    while (true) {
        new_tx.change_position = nChangePosRequest;
        new_tx.tx.vin.clear();
        new_tx.tx.vout.clear();

        // Start by adding outputs for the recipients.
        AddRecipientOutputs(new_tx);

        // Determine input amount needed.
        CAmount amount_needed = new_tx.recipient_amount;
        if (new_tx.subtract_fee_from_amount == 0) {
            amount_needed += new_tx.total_fee;
        }

        // Choose coins to use
        if (pick_new_inputs) {
            if (!AttemptCoinSelection(new_tx, amount_needed)) {
                throw CreateTxError(_("Insufficient funds"));
            }

            // Only use bnb on the first attempt.
            new_tx.coin_selection_params.use_bnb = false;
        }

        new_tx.mweb_type = MWEB::GetTxType(new_tx.recipients, new_tx.selected_coins);

        // We already created an output for each non-MWEB recipient, but for pegout transactions,
        // the recipients are funded through the pegout kernel instead of traditional LTC outputs.
        if (new_tx.mweb_type == MWEB::TxType::PEGOUT) {
            new_tx.tx.vout.clear();
        }

        new_tx.change_on_mweb = MWEB::IsChangeOnMWEB(m_wallet, new_tx.mweb_type, new_tx.recipients, new_tx.coin_control.destChange);
        new_tx.mweb_weight = MWEB::CalcMWEBWeight(new_tx.mweb_type, new_tx.change_on_mweb, new_tx.recipients);

        // Add peg-in script for 'value_selected', which is the most it could be with the chosen inputs.
        // This ensures there will be enough fee included.
        if (MWEB::ContainsPegIn(new_tx.mweb_type, new_tx.selected_coins)) {
            std::vector<CTxOut>::iterator position = new_tx.tx.vout.begin() + GetRandInt(new_tx.tx.vout.size() + 1);
            new_tx.tx.vout.insert(position, CTxOut(new_tx.value_selected, GetScriptForPegin(mw::Hash())));
        }

        // Adds in a change output if there's enough leftover to create one
        AddChangeOutput(new_tx, new_tx.value_selected - amount_needed);

        // Calculate transaction size and the total necessary fee amount (includes LTC and MWEB fees)
        new_tx.bytes = CalculateMaximumTxSize(new_tx);
        new_tx.fee_needed = new_tx.coin_selection_params.m_effective_feerate.GetTotalFee(new_tx.bytes, new_tx.mweb_weight);

        // Calculate the portion of the fee that should be paid on the MWEB side (using kernel fees)
        size_t pegout_bytes = MWEB::CalcPegOutBytes(new_tx.mweb_type, new_tx.recipients);
        new_tx.mweb_fee = new_tx.coin_selection_params.m_effective_feerate.GetTotalFee(pegout_bytes, new_tx.mweb_weight);

        if (new_tx.total_fee < new_tx.fee_needed && !pick_new_inputs) {
            // This shouldn't happen, we should have had enough excess
            // fee to pay for the new output and still meet nFeeNeeded
            // Or we should have just subtracted fee from recipients and
            // nFeeNeeded should not have changed
            throw CreateTxError(_("Transaction fee and change calculation failed"));
        }

        if (new_tx.total_fee >= new_tx.fee_needed) {
            // Reduce fee to only the needed amount if possible. This
            // prevents potential overpayment in fees if the coins
            // selected to meet nFeeNeeded result in a transaction that
            // requires less fee than the prior iteration.
            ReduceFee(new_tx);
            break; // Done, enough fee included.
        }

        // Try to reduce change to include necessary fee
        if (new_tx.change_position != -1 && new_tx.subtract_fee_from_amount == 0) {
            CAmount additional_fee_needed = new_tx.fee_needed - new_tx.total_fee;
            std::vector<CTxOut>::iterator change_position = new_tx.tx.vout.begin() + new_tx.change_position;

            // Only reduce change if remaining amount is still a large enough output.
            if (change_position->nValue >= MIN_FINAL_CHANGE + additional_fee_needed) {
                change_position->nValue -= additional_fee_needed;
                new_tx.total_fee += additional_fee_needed;
                break; // Done, able to increase fee from change
            }
        }

        // If subtracting fee from recipients, we now know what fee we
        // need to subtract, we have no reason to reselect inputs.
        if (new_tx.subtract_fee_from_amount > 0) {
            pick_new_inputs = false;
        }

        // Include more fee and try again.
        new_tx.total_fee = new_tx.fee_needed;
    }

    // Give up if change keypool ran out and change is required
    if (new_tx.change_addr.IsEmpty() && new_tx.change_position != -1) {
        throw CreateTxError(_("Transaction needs a change address, but we can't generate it. Please call keypoolrefill first."));
    }

    AddTxInputs(new_tx);

    // Now build the MWEB side of the transaction
    if (new_tx.mweb_type != MWEB::TxType::LTC_TO_LTC) {
        MWEB::Transact(m_wallet).AddMWEBTx(new_tx);
    }

    if (sign && !m_wallet.SignTransaction(new_tx.tx)) {
        throw CreateTxError(_("Signing transaction failed"));
    }
}

void TxAssembler::VerifyRecipients(const std::vector<CRecipient>& recipients)
{
    if (recipients.empty()) {
        throw CreateTxError(_("Transaction must have at least one recipient"));
    }

    CAmount nValue = 0;
    for (const auto& recipient : recipients) {
        if (recipient.IsMWEB() && recipients.size() > 1) {
            throw CreateTxError(_("Only one MWEB recipient supported at this time"));
        }

        
        nValue += recipient.nAmount;
        if (nValue < 0 || recipient.nAmount < 0) {
            throw CreateTxError(_("Transaction amounts must not be negative"));
        }
    }
}

// Adds an output for each non-MWEB recipient.
// If the recipient is marked as 'fSubtractFeeFromAmount', the fee will be deducted from the amount.
//
// Throws a CreateTxError when a recipient amount is considered dust.
void TxAssembler::AddRecipientOutputs(InProcessTx& new_tx)
{
    // vouts to the payees
    if (!new_tx.coin_selection_params.m_subtract_fee_outputs) {
        new_tx.coin_selection_params.tx_noinputs_size = 11; // Static vsize overhead + outputs vsize. 4 nVersion, 4 nLocktime, 1 input count, 1 output count, 1 witness overhead (dummy, flag, stack size)
    }

    bool fFirst = true;
    for (const auto& recipient : new_tx.recipients) {
        if (recipient.IsMWEB()) {
            continue;
        }

        CTxOut txout(recipient.nAmount, recipient.GetScript());

        if (recipient.fSubtractFeeFromAmount) {
            assert(new_tx.subtract_fee_from_amount != 0);
            txout.nValue -= new_tx.total_fee / new_tx.subtract_fee_from_amount; // Subtract fee equally from each selected recipient

            if (fFirst) // first receiver pays the remainder not divisible by output count
            {
                fFirst = false;
                txout.nValue -= new_tx.total_fee % new_tx.subtract_fee_from_amount;
            }
        }
        // Include the fee cost for outputs. Note this is only used for BnB right now
        new_tx.coin_selection_params.tx_noinputs_size += ::GetSerializeSize(txout, PROTOCOL_VERSION);

        if (IsDust(txout, m_wallet.chain().relayDustFee())) {
            if (recipient.fSubtractFeeFromAmount && new_tx.total_fee > 0) {
                if (txout.nValue < 0) {
                    throw CreateTxError(_("The transaction amount is too small to pay the fee"));
                } else {
                    throw CreateTxError(_("The transaction amount is too small to send after the fee has been deducted"));
                }
            }

            throw CreateTxError(_("Transaction amount too small"));
        }
        new_tx.tx.vout.push_back(txout);
    }
}

void TxAssembler::AddChangeOutput(InProcessTx& new_tx, const CAmount& nChange) const
{
    if (nChange > 0 && !new_tx.change_on_mweb && !new_tx.change_addr.IsMWEB()) {
        // Fill a vout to ourself
        CTxOut newTxOut(nChange, new_tx.change_addr.GetScript());

        // Never create dust outputs; if we would, just
        // add the dust to the fee.
        // The nChange when BnB is used is always going to go to fees.
        if (IsDust(newTxOut, new_tx.coin_selection_params.m_discard_feerate) || new_tx.bnb_used) {
            new_tx.change_position = -1;
            new_tx.total_fee += nChange;
        } else {
            if (new_tx.change_position == -1) {
                // Insert change txn at random position:
                new_tx.change_position = GetRandInt(new_tx.tx.vout.size() + 1);
            } else if ((unsigned int)new_tx.change_position > new_tx.tx.vout.size()) {
                throw CreateTxError(_("Change index out of range"));
            }

            std::vector<CTxOut>::iterator position = new_tx.tx.vout.begin() + new_tx.change_position;
            new_tx.tx.vout.insert(position, newTxOut);
        }
    } else {
        new_tx.change_position = -1;
    }
}

void TxAssembler::AddTxInputs(InProcessTx& new_tx) const
{
    // Shuffle selected coins and fill in final vin
    new_tx.tx.vin.clear();
    std::vector<CInputCoin> shuffled_coins(new_tx.selected_coins.begin(), new_tx.selected_coins.end());
    Shuffle(shuffled_coins.begin(), shuffled_coins.end(), FastRandomContext());

    // Note how the sequence number is set to non-maxint so that
    // the nLockTime set above actually works.
    //
    // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
    // we use the highest possible value in that range (maxint-2)
    // to avoid conflicting with other possible uses of nSequence,
    // and in the spirit of "smallest possible change from prior
    // behavior."
    const uint32_t nSequence = new_tx.coin_control.m_signal_bip125_rbf.get_value_or(m_wallet.m_signal_rbf) ? MAX_BIP125_RBF_SEQUENCE : (CTxIn::SEQUENCE_FINAL - 1);
    for (const auto& coin : shuffled_coins) {
        if (!coin.IsMWEB()) {
            new_tx.tx.vin.push_back(CTxIn(boost::get<COutPoint>(coin.GetIndex()), CScript(), nSequence));
        }
    }
}

void TxAssembler::InitCoinSelectionParams(InProcessTx& new_tx) const
{
    new_tx.coin_selection_params.change_output_size = new_tx.change_addr.IsMWEB() ? 0 : GetSerializeSize(CTxOut(0, new_tx.change_addr.GetScript()));

    // Set discard feerate
    new_tx.coin_selection_params.m_discard_feerate = GetDiscardRate(m_wallet);

    // Get the fee rate to use effective values in coin selection
    new_tx.coin_selection_params.m_effective_feerate = GetMinimumFeeRate(m_wallet, new_tx.coin_control, &new_tx.fee_calc);

    // Do not, ever, assume that it's fine to change the fee rate if the user has explicitly
    // provided one
    if (new_tx.coin_control.m_feerate && new_tx.coin_selection_params.m_effective_feerate > *new_tx.coin_control.m_feerate) {
        throw CreateTxError(strprintf(
            _("Fee rate (%s) is lower than the minimum fee rate setting (%s)"),
            new_tx.coin_control.m_feerate->ToString(FeeEstimateMode::SAT_VB),
            new_tx.coin_selection_params.m_effective_feerate.ToString(FeeEstimateMode::SAT_VB)
        ));
    }

    if (new_tx.fee_calc.reason == FeeReason::FALLBACK && !m_wallet.m_allow_fallback_fee) {
        // eventually allow a fallback fee
        throw CreateTxError(_("Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -fallbackfee."));
    }

    // Get long term estimate
    CCoinControl cc_temp;
    cc_temp.m_confirm_target = m_wallet.chain().estimateMaxBlocks();
    new_tx.coin_selection_params.m_long_term_feerate = GetMinimumFeeRate(m_wallet, cc_temp, nullptr);

    // BnB selector is the only selector used when this is true.
    // That should only happen on the first pass through the loop.
    new_tx.coin_selection_params.use_bnb = true;
    new_tx.coin_selection_params.m_subtract_fee_outputs = new_tx.subtract_fee_from_amount != 0; // If we are doing subtract fee from recipient, don't use effective values
}

bool TxAssembler::AttemptCoinSelection(InProcessTx& new_tx, const CAmount& nTargetValue) const
{
    new_tx.value_selected = 0;
    new_tx.selected_coins.clear();

    CTxOut change_prototype_txout(0, new_tx.change_addr.IsMWEB() ? CScript() : new_tx.change_addr.GetScript());
    const int change_spend_size = CalculateMaximumSignedInputSize(change_prototype_txout, &m_wallet);

    // If the wallet doesn't know how to sign change output, assume p2sh-p2wpkh
    // as lower-bound to allow BnB to do it's thing
    if (change_spend_size == -1) {
        new_tx.coin_selection_params.change_spend_size = DUMMY_NESTED_P2WPKH_INPUT_SIZE;
    } else {
        new_tx.coin_selection_params.change_spend_size = (size_t)change_spend_size;
    }

    static auto is_ltc = [](const CInputCoin& input) { return !input.IsMWEB(); };
    static auto is_mweb = [](const CInputCoin& input) { return input.IsMWEB(); };

    if (new_tx.recipients.front().IsMWEB()) {
        // First try to construct an MWEB-to-MWEB transaction
        CoinSelectionParams mweb_to_mweb = new_tx.coin_selection_params;
        mweb_to_mweb.input_preference = InputPreference::MWEB_ONLY;
        mweb_to_mweb.mweb_change_output_weight = mw::STANDARD_OUTPUT_WEIGHT;
        mweb_to_mweb.mweb_nochange_weight = mw::KERNEL_WITH_STEALTH_WEIGHT + (new_tx.recipients.size() * mw::STANDARD_OUTPUT_WEIGHT);
        mweb_to_mweb.change_output_size = 0;
        mweb_to_mweb.change_spend_size = 0;
        mweb_to_mweb.tx_noinputs_size = 0;

        if (SelectCoins(new_tx, nTargetValue, mweb_to_mweb)) {
            return true;
        }

        // If MWEB-to-MWEB fails, create a peg-in transaction
        const bool change_on_mweb = MWEB::IsChangeOnMWEB(m_wallet, MWEB::TxType::PEGIN, new_tx.recipients, new_tx.coin_control.destChange);
        CoinSelectionParams params_pegin = new_tx.coin_selection_params;
        params_pegin.input_preference = InputPreference::ANY;
        params_pegin.mweb_change_output_weight = change_on_mweb ? mw::STANDARD_OUTPUT_WEIGHT : 0;
        params_pegin.mweb_nochange_weight = mw::KERNEL_WITH_STEALTH_WEIGHT + (new_tx.recipients.size() * mw::STANDARD_OUTPUT_WEIGHT);
        params_pegin.change_output_size = change_on_mweb ? 0 : new_tx.coin_selection_params.change_output_size;
        params_pegin.change_spend_size = change_on_mweb ? 0 : new_tx.coin_selection_params.change_spend_size;

        if (SelectCoins(new_tx, nTargetValue, params_pegin)) {
            return std::any_of(new_tx.selected_coins.cbegin(), new_tx.selected_coins.cend(), is_ltc);
        }
    } else {
        // First try to construct a LTC-to-LTC transaction
        CoinSelectionParams mweb_to_mweb = new_tx.coin_selection_params;
        mweb_to_mweb.input_preference = InputPreference::LTC_ONLY;
        mweb_to_mweb.mweb_change_output_weight = 0;
        mweb_to_mweb.mweb_nochange_weight = 0;

        if (SelectCoins(new_tx, nTargetValue, mweb_to_mweb)) {
            return true;
        }

        // Only supports pegging-out to one address
        if (new_tx.recipients.size() > 1) {
            return false;
        }

        // If LTC-to-LTC fails, create a peg-out transaction
        CoinSelectionParams params_pegout = new_tx.coin_selection_params;
        params_pegout.input_preference = InputPreference::ANY;
        params_pegout.mweb_change_output_weight = mw::STANDARD_OUTPUT_WEIGHT;
        params_pegout.mweb_nochange_weight = Weight::CalcKernelWeight(true, new_tx.recipients.front().GetScript());
        params_pegout.change_output_size = 0;
        params_pegout.change_spend_size = 0;
        new_tx.tx.vout.clear();

        if (SelectCoins(new_tx, nTargetValue, params_pegout)) {
            return std::any_of(new_tx.selected_coins.cbegin(), new_tx.selected_coins.cend(), is_mweb);
        }
    }

    return false;
}

bool TxAssembler::SelectCoins(InProcessTx& new_tx, const CAmount& nTargetValue, CoinSelectionParams& coin_selection_params) const
{
    new_tx.value_selected = 0;
    if (m_wallet.SelectCoins(new_tx.available_coins, nTargetValue, new_tx.selected_coins, new_tx.value_selected, new_tx.coin_control, coin_selection_params, new_tx.bnb_used)) {
        return true;
    }

    new_tx.value_selected = 0;
    if (new_tx.bnb_used) {
        coin_selection_params.use_bnb = false;
        return m_wallet.SelectCoins(new_tx.available_coins, nTargetValue, new_tx.selected_coins, new_tx.value_selected, new_tx.coin_control, coin_selection_params, new_tx.bnb_used);
    }

    return false;
}

// Return a height-based locktime for new transactions
// (uses the height of the current chain tip unless we are not synced with the current chain)
uint32_t TxAssembler::GetLocktimeForNewTransaction() const
{
    AssertLockHeld(m_wallet.cs_wallet);

    uint32_t locktime;

    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.
    if (IsCurrentForAntiFeeSniping(m_wallet.chain(), m_wallet.GetLastBlockHash())) {
        locktime = m_wallet.GetLastBlockHeight();

        // Secondly occasionally randomly pick a nLockTime even further back, so
        // that transactions that are delayed after signing for whatever reason,
        // e.g. high-latency mix networks and some CoinJoin implementations, have
        // better privacy.
        if (GetRandInt(10) == 0)
            locktime = std::max(0, (int)locktime - GetRandInt(100));
    } else {
        // If our chain is lagging behind, we can't discourage fee sniping nor help
        // the privacy of high-latency transactions. To avoid leaking a potentially
        // unique "nLockTime fingerprint", set nLockTime to a constant.
        locktime = 0;
    }
    assert(locktime < LOCKTIME_THRESHOLD);
    return locktime;
}

bool TxAssembler::IsCurrentForAntiFeeSniping(interfaces::Chain& chain, const uint256& block_hash) const
{
    if (chain.isInitialBlockDownload()) {
        return false;
    }

    constexpr int64_t MAX_ANTI_FEE_SNIPING_TIP_AGE = 8 * 60 * 60; // in seconds
    int64_t block_time;
    CHECK_NONFATAL(chain.findBlock(block_hash, interfaces::FoundBlock().time(block_time)));

    if (block_time < (GetTime() - MAX_ANTI_FEE_SNIPING_TIP_AGE)) {
        return false;
    }

    return true;
}

void TxAssembler::ReduceFee(InProcessTx& new_tx) const
{
    // If we have no change and a big enough excess fee, then try to construct transaction again, only without picking new inputs.
    // We now know we only need the smaller fee (because of reduced tx size) and so we should add a change output.
    if (new_tx.change_position == -1 && new_tx.subtract_fee_from_amount == 0 && !new_tx.change_on_mweb && !new_tx.change_addr.IsMWEB()) {
        unsigned int output_buffer = 2; // Add 2 as a buffer in case increasing # of outputs changes compact size
        unsigned int tx_size_with_change = new_tx.change_on_mweb ? new_tx.bytes : new_tx.bytes + new_tx.coin_selection_params.change_output_size + output_buffer;
        CAmount fee_needed_with_change = new_tx.coin_selection_params.m_effective_feerate.GetTotalFee(tx_size_with_change, new_tx.mweb_weight);

        CTxOut change_prototype_txout(0, new_tx.change_addr.GetScript());
        CAmount minimum_value_for_change = GetDustThreshold(change_prototype_txout, new_tx.coin_selection_params.m_discard_feerate);

        if (new_tx.total_fee >= fee_needed_with_change + minimum_value_for_change) {
            const CAmount nChange = new_tx.total_fee - fee_needed_with_change;
            new_tx.fee_needed = fee_needed_with_change;
            new_tx.total_fee = fee_needed_with_change;
            AddChangeOutput(new_tx, nChange);
            new_tx.bytes = CalculateMaximumTxSize(new_tx);
        }
    }

    // If we have a change output already, just increase it
    if (new_tx.total_fee > new_tx.fee_needed && new_tx.subtract_fee_from_amount == 0) {
        CAmount extra_fee_paid = new_tx.total_fee - new_tx.fee_needed;
        if (new_tx.change_position != -1) {
            std::vector<CTxOut>::iterator change_position = new_tx.tx.vout.begin() + new_tx.change_position;
            change_position->nValue += extra_fee_paid;
            new_tx.total_fee -= extra_fee_paid;
        } else if (new_tx.change_on_mweb) {
            new_tx.total_fee -= extra_fee_paid;
        }
    }
}

int64_t TxAssembler::CalculateMaximumTxSize(const InProcessTx& new_tx) const
{
    CMutableTransaction tmp_tx(new_tx.tx);

    // Dummy fill vin for maximum size estimation
    for (const auto& coin : new_tx.selected_coins) {
        if (!coin.IsMWEB()) {
            tmp_tx.vin.push_back(CTxIn(boost::get<COutPoint>(coin.GetIndex()), CScript()));
        }
    }

    int64_t bytes = 0;
    if (new_tx.mweb_type != MWEB::TxType::MWEB_TO_MWEB && (new_tx.mweb_type != MWEB::TxType::PEGOUT || !tmp_tx.vin.empty())) {
        bytes = CalculateMaximumSignedTxSize(CTransaction(tmp_tx), &m_wallet, new_tx.coin_control.fAllowWatchOnly);
        if (bytes < 0) {
            throw CreateTxError(_("Signing transaction failed"));
        }
    }

    // Include HogEx input bytes for any pegins
    if (MWEB::ContainsPegIn(new_tx.mweb_type, new_tx.selected_coins)) {
        bytes += ::GetSerializeSize(CTxIn(), PROTOCOL_VERSION);
    }

    // Include pegout bytes
    bytes += MWEB::CalcPegOutBytes(new_tx.mweb_type, new_tx.recipients);

    return bytes;
}

void TxAssembler::UpdateChangeAddress(InProcessTx& new_tx) const
{
    // coin control: send change to custom address
    if (!boost::get<CNoDestination>(&new_tx.coin_control.destChange)) {
        new_tx.change_addr = DestinationAddr(new_tx.coin_control.destChange);
    } else { // no coin control: send change to newly generated address
        // Note: We use a new key here to keep it from being obvious which side is the change.
        //  The drawback is that by not reusing a previous key, the change may be lost if a
        //  backup is restored, if the backup doesn't have the new private key for the change.
        //  If we reused the old key, it would be possible to add code to look for and
        //  rediscover unknown transactions that were written with keys of ours to recover
        //  post-backup change.

        new_tx.reserve_dest = std::make_unique<ReserveDestination>(&m_wallet, GetChangeType(new_tx));

        // Reserve a new key pair from key pool. If it fails, provide a dummy
        // destination in case we don't need change.
        CTxDestination dest;
        new_tx.reserve_dest->GetReservedDestination(dest, true);
        new_tx.change_addr = GetScriptForDestination(dest);
        // A valid destination implies a change script (and
        // vice-versa). An empty change script will abort later, if the
        // change keypool ran out, but change is required.
        CHECK_NONFATAL(IsValidDestination(dest) != new_tx.change_addr.IsEmpty());
    }
}

OutputType TxAssembler::GetChangeType(const InProcessTx& new_tx) const
{
    Optional<OutputType> change_type = new_tx.coin_control.m_change_type ? *new_tx.coin_control.m_change_type : m_wallet.m_default_change_type;

    // If -changetype is specified, always use that change type.
    if (change_type) {
        return *change_type;
    }

    // if m_default_address_type is legacy, use legacy address as change
    // (even if some of the outputs are P2WPKH or P2WSH).
    if (m_wallet.m_default_address_type == OutputType::LEGACY) {
        return OutputType::LEGACY;
    }

    // If any destination is P2WPKH or P2WSH, use P2WPKH for the change output.
    for (const auto& recipient : new_tx.recipients) {
        if (!recipient.IsMWEB()) {
            int witnessversion = 0;
            std::vector<unsigned char> witnessprogram;
            if (recipient.GetScript().IsWitnessProgram(witnessversion, witnessprogram)) {
                return OutputType::BECH32;
            }
        }
    }

    // else use m_default_address_type for change
    return m_wallet.m_default_address_type;
}