#include <wallet/createtransaction.h>
#include <consensus/validation.h>
#include <logging.h>
#include <mweb/mweb_transact.h>
#include <policy/policy.h>
#include <policy/fees.h>
#include <policy/rbf.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/fees.h>
#include <util/rbf.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/fees.h>
#include <wallet/reserve.h>

#include <numeric>
#include <boost/optional.hpp>

static OutputType TransactionChangeType(const CWallet& wallet, const Optional<OutputType>& change_type, const std::vector<CRecipient>& vecSend)
{
    // If -changetype is specified, always use that change type.
    if (change_type) {
        return *change_type;
    }

    // if m_default_address_type is legacy, use legacy address as change (even
    // if some of the outputs are P2WPKH or P2WSH).
    if (wallet.m_default_address_type == OutputType::LEGACY) {
        return OutputType::LEGACY;
    }

    // if any destination is P2WPKH or P2WSH, use P2WPKH for the change
    // output.
    for (const auto& recipient : vecSend) {
        // Check if any destination contains a witness program:
        if (!recipient.IsMWEB()) {
            int witnessversion = 0;
            std::vector<unsigned char> witnessprogram;
            if (recipient.GetScript().IsWitnessProgram(witnessversion, witnessprogram)) {
                return OutputType::BECH32;
            }
        }
    }

    // else use m_default_address_type for change
    return wallet.m_default_address_type;
}

static bool IsCurrentForAntiFeeSniping(interfaces::Chain& chain, const uint256& block_hash)
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
/**
 * Return a height-based locktime for new transactions (uses the height of the
 * current chain tip unless we are not synced with the current chain
 */
static uint32_t GetLocktimeForNewTransaction(interfaces::Chain& chain, const uint256& block_hash, int block_height)
{
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
    if (IsCurrentForAntiFeeSniping(chain, block_hash)) {
        locktime = block_height;

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

bool VerifyRecipients(const std::vector<CRecipient>& vecSend, bilingual_str& error)
{
    if (vecSend.empty()) {
        error = _("Transaction must have at least one recipient");
        return false;
    }

    CAmount nValue = 0;
    for (const auto& recipient : vecSend) {
        if (recipient.IsMWEB() && vecSend.size() > 1) {
            error = _("Only one MWEB recipient supported at this time");
            return false;
        }

        if (nValue < 0 || recipient.nAmount < 0) {
            error = _("Transaction amounts must not be negative");
            return false;
        }

        nValue += recipient.nAmount;
    }

    return true;
}

bool AddRecipientOutputs(
    CMutableTransaction& txNew,
    const std::vector<CRecipient>& vecSend,
    CoinSelectionParams& coin_selection_params,
    CAmount nFeeRet,
    unsigned int nSubtractFeeFromAmount,
    interfaces::Chain& chain,
    bilingual_str& error)
{
    // vouts to the payees
    if (!coin_selection_params.m_subtract_fee_outputs) {
        coin_selection_params.tx_noinputs_size = 11; // Static vsize overhead + outputs vsize. 4 nVersion, 4 nLocktime, 1 input count, 1 output count, 1 witness overhead (dummy, flag, stack size)
    }

    bool fFirst = true;
    for (const auto& recipient : vecSend) {
        if (recipient.IsMWEB()) {
            continue;
        }

        CTxOut txout(recipient.nAmount, recipient.GetScript());

        if (recipient.fSubtractFeeFromAmount) {
            assert(nSubtractFeeFromAmount != 0);
            txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

            if (fFirst) // first receiver pays the remainder not divisible by output count
            {
                fFirst = false;
                txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
            }
        }
        // Include the fee cost for outputs. Note this is only used for BnB right now
        coin_selection_params.tx_noinputs_size += ::GetSerializeSize(txout, PROTOCOL_VERSION);

        if (IsDust(txout, chain.relayDustFee())) {
            if (recipient.fSubtractFeeFromAmount && nFeeRet > 0) {
                if (txout.nValue < 0)
                    error = _("The transaction amount is too small to pay the fee");
                else
                    error = _("The transaction amount is too small to send after the fee has been deducted");
            } else
                error = _("Transaction amount too small");
            return false;
        }
        txNew.vout.push_back(txout);
    }

    return true;
}

static bool IsChangeOnMWEB(const CWallet& wallet, const MWEB::TxType& mweb_type, const std::vector<CRecipient>& vecSend, const CTxDestination& dest_change)
{
    if (mweb_type == MWEB::TxType::MWEB_TO_MWEB || mweb_type == MWEB::TxType::PEGOUT) {
        return true;
    }

    if (mweb_type == MWEB::TxType::PEGIN) {
        // If you try pegging-in to only MWEB addresses belonging to others,
        // you risk revealing the kernel blinding factor, allowing the receiver to malleate the kernel.
        // To avoid this risk, include a change output so that kernel blind is not leaked.
        bool pegin_change_required = true;
        for (const CRecipient& recipient : vecSend) {
            if (recipient.IsMWEB() && wallet.IsMine(recipient.receiver)) {
                pegin_change_required = false;
                break;
            }
        }

        return pegin_change_required
            || dest_change.type() == typeid(CNoDestination)
            || dest_change.type() == typeid(StealthAddress);
    }

    return false;
}

static bool ContainsPegIn(const MWEB::TxType& mweb_type, const std::set<CInputCoin>& setCoins)
{
    if (mweb_type == MWEB::TxType::PEGIN) {
        return true;
    }
    
    if (mweb_type == MWEB::TxType::PEGOUT) {
        return std::any_of(
            setCoins.cbegin(), setCoins.cend(),
            [](const CInputCoin& coin) { return !coin.IsMWEB(); });
    }

    return false;
}

static bool SelectCoinsEx(
    CWallet& wallet,
    const std::vector<COutputCoin>& vAvailableCoins,
    const CAmount& nTargetValue,
    std::set<CInputCoin>& setCoinsRet,
    CAmount& nValueRet,
    const CCoinControl& coin_control,
    CoinSelectionParams& coin_selection_params,
    bool& bnb_used)
{
    nValueRet = 0;
    if (wallet.SelectCoins(vAvailableCoins, nTargetValue, setCoinsRet, nValueRet, coin_control, coin_selection_params, bnb_used)) {
        return true;
    }

    nValueRet = 0;
    if (bnb_used) {
        bnb_used = false;
        coin_selection_params.use_bnb = false;
        return wallet.SelectCoins(vAvailableCoins, nTargetValue, setCoinsRet, nValueRet, coin_control, coin_selection_params, bnb_used);
    }

    return false;
}

static bool AttemptCoinSelection(
    CWallet& wallet,
    const std::vector<COutputCoin>& vAvailableCoins,
    const CAmount& nTargetValue,
    std::set<CInputCoin>& setCoinsRet,
    CAmount& nValueRet,
    const CCoinControl& coin_control,
    const std::vector<CRecipient>& recipients,
    CoinSelectionParams& coin_selection_params,
    bool& bnb_used)
{
    static auto is_ltc = [](const CInputCoin& input) { return !input.IsMWEB(); };
    static auto is_mweb = [](const CInputCoin& input) { return input.IsMWEB(); };

    if (recipients.front().IsMWEB()) {
        // First try to construct an MWEB-to-MWEB transaction
        CoinSelectionParams mweb_to_mweb = coin_selection_params;
        mweb_to_mweb.input_preference = InputPreference::MWEB_ONLY;
        mweb_to_mweb.mweb_change_output_weight = Weight::STANDARD_OUTPUT_WEIGHT;
        mweb_to_mweb.mweb_nochange_weight = Weight::KERNEL_WITH_STEALTH_WEIGHT + (recipients.size() * Weight::STANDARD_OUTPUT_WEIGHT);
        mweb_to_mweb.change_output_size = 0;
        mweb_to_mweb.change_spend_size = 0;
        mweb_to_mweb.tx_noinputs_size = 0;
        bnb_used = false;

        if (SelectCoinsEx(wallet, vAvailableCoins, nTargetValue, setCoinsRet, nValueRet, coin_control, mweb_to_mweb, bnb_used)) {
            return true;
        }

        // If MWEB-to-MWEB fails, create a peg-in transaction
        const bool change_on_mweb = IsChangeOnMWEB(wallet, MWEB::TxType::PEGIN, recipients, coin_control.destChange);
        CoinSelectionParams params_pegin = coin_selection_params;
        params_pegin.input_preference = InputPreference::ANY;
        params_pegin.mweb_change_output_weight = change_on_mweb ? Weight::STANDARD_OUTPUT_WEIGHT : 0;
        params_pegin.mweb_nochange_weight = Weight::KERNEL_WITH_STEALTH_WEIGHT + (recipients.size() * Weight::STANDARD_OUTPUT_WEIGHT);
        params_pegin.change_output_size = change_on_mweb ? 0 : coin_selection_params.change_output_size;
        params_pegin.change_spend_size = change_on_mweb ? 0 : coin_selection_params.change_spend_size;
        bnb_used = false;

        if (SelectCoinsEx(wallet, vAvailableCoins, nTargetValue, setCoinsRet, nValueRet, coin_control, params_pegin, bnb_used)) {
            return std::any_of(setCoinsRet.cbegin(), setCoinsRet.cend(), is_ltc);
        }
    } else {
        // First try to construct a LTC-to-LTC transaction
        CoinSelectionParams mweb_to_mweb = coin_selection_params;
        mweb_to_mweb.input_preference = InputPreference::LTC_ONLY;
        mweb_to_mweb.mweb_change_output_weight = 0;
        mweb_to_mweb.mweb_nochange_weight = 0;
        bnb_used = false;

        if (SelectCoinsEx(wallet, vAvailableCoins, nTargetValue, setCoinsRet, nValueRet, coin_control, mweb_to_mweb, bnb_used)) {
            return true;
        }

        // Only supports pegging-out to one address
        if (recipients.size() > 1) {
            return false;
        }

        // If LTC-to-LTC fails, create a peg-out transaction
        CoinSelectionParams params_pegout = coin_selection_params;
        params_pegout.input_preference = InputPreference::ANY;
        params_pegout.mweb_change_output_weight = Weight::STANDARD_OUTPUT_WEIGHT;
        params_pegout.mweb_nochange_weight = Weight::CalcKernelWeight(true, recipients.front().GetScript());
        params_pegout.change_output_size = 0;
        params_pegout.change_spend_size = 0;
        bnb_used = false;

        if (SelectCoinsEx(wallet, vAvailableCoins, nTargetValue, setCoinsRet, nValueRet, coin_control, params_pegout, bnb_used)) {
            return std::any_of(setCoinsRet.cbegin(), setCoinsRet.cend(), is_mweb);
        }
    }

    return false;
}

bool CreateTransactionEx(
    CWallet& wallet,
    const std::vector<CRecipient>& vecSend,
    CTransactionRef& tx,
    CAmount& nFeeRet,
    int& nChangePosInOut,
    bilingual_str& error,
    const CCoinControl& coin_control,
    FeeCalculation& fee_calc_out,
    bool sign)
{
    if (!VerifyRecipients(vecSend, error)) {
        return false;
    }

    CAmount nValue = std::accumulate(
        vecSend.cbegin(), vecSend.cend(), CAmount(0),
        [](CAmount amt, const CRecipient& recipient) { return amt + recipient.nAmount; }
    );

    const OutputType change_type = TransactionChangeType(wallet, coin_control.m_change_type ? *coin_control.m_change_type : wallet.m_default_change_type, vecSend);
    ReserveDestination reservedest(&wallet, change_type);

    unsigned int nSubtractFeeFromAmount = std::count_if(vecSend.cbegin(), vecSend.cend(),
        [](const CRecipient& recipient) { return recipient.fSubtractFeeFromAmount; }
    );

    int nChangePosRequest = nChangePosInOut;

    CMutableTransaction txNew;

    FeeCalculation feeCalc;
    CAmount nFeeNeeded;

    uint64_t mweb_weight = 0;
    MWEB::TxType mweb_type = MWEB::TxType::LTC_TO_LTC;
    bool change_on_mweb = false;
    CAmount mweb_fee = 0;

    int nBytes;
    {
        std::set<CInputCoin> setCoins;
        LOCK(wallet.cs_wallet);
        txNew.nLockTime = GetLocktimeForNewTransaction(wallet.chain(), wallet.GetLastBlockHash(), wallet.GetLastBlockHeight());
        {
            std::vector<COutputCoin> vAvailableCoins;
            wallet.AvailableCoins(vAvailableCoins, true, &coin_control, 1, MAX_MONEY, MAX_MONEY, 0);

            // Create change script that will be used if we need change
            // TODO: pass in scriptChange instead of reservekey so
            // change transaction isn't always pay-to-bitcoin-address
            DestinationAddr change_addr;

            // coin control: send change to custom address
            if (!boost::get<CNoDestination>(&coin_control.destChange)) {
                change_addr = DestinationAddr(coin_control.destChange);
            } else { // no coin control: send change to newly generated address
                // Note: We use a new key here to keep it from being obvious which side is the change.
                //  The drawback is that by not reusing a previous key, the change may be lost if a
                //  backup is restored, if the backup doesn't have the new private key for the change.
                //  If we reused the old key, it would be possible to add code to look for and
                //  rediscover unknown transactions that were written with keys of ours to recover
                //  post-backup change.

                
                // Reserve a new key pair from key pool. If it fails, provide a dummy
                // destination in case we don't need change.
                CTxDestination dest;
                if (!reservedest.GetReservedDestination(dest, true)) {
                    error = _("Transaction needs a change address, but we can't generate it. Please call keypoolrefill first.");
                }
                change_addr = GetScriptForDestination(dest);
                // A valid destination implies a change script (and
                // vice-versa). An empty change script will abort later, if the
                // change keypool ran out, but change is required.
                CHECK_NONFATAL(IsValidDestination(dest) != change_addr.IsEmpty());
            }

            CTxOut change_prototype_txout(0, change_addr.IsMWEB() ? CScript() : change_addr.GetScript());

            CoinSelectionParams coin_selection_params; // Parameters for coin selection, init with dummy
            coin_selection_params.change_output_size = GetSerializeSize(change_prototype_txout);
            coin_selection_params.mweb_change_output_weight = Weight::STANDARD_OUTPUT_WEIGHT;

            // Set discard feerate
            coin_selection_params.m_discard_feerate = GetDiscardRate(wallet);

            // Get the fee rate to use effective values in coin selection
            coin_selection_params.m_effective_feerate = GetMinimumFeeRate(wallet, coin_control, &feeCalc);
            // Do not, ever, assume that it's fine to change the fee rate if the user has explicitly
            // provided one
            if (coin_control.m_feerate && coin_selection_params.m_effective_feerate > *coin_control.m_feerate) {
                error = strprintf(_("Fee rate (%s) is lower than the minimum fee rate setting (%s)"), coin_control.m_feerate->ToString(FeeEstimateMode::SAT_VB), coin_selection_params.m_effective_feerate.ToString(FeeEstimateMode::SAT_VB));
                return false;
            }
            if (feeCalc.reason == FeeReason::FALLBACK && !wallet.m_allow_fallback_fee) {
                // eventually allow a fallback fee
                error = _("Fee estimation failed. Fallbackfee is disabled. Wait a few blocks or enable -fallbackfee.");
                return false;
            }

            // Get long term estimate
            CCoinControl cc_temp;
            cc_temp.m_confirm_target = wallet.chain().estimateMaxBlocks();
            coin_selection_params.m_long_term_feerate = GetMinimumFeeRate(wallet, cc_temp, nullptr);

            // BnB selector is the only selector used when this is true.
            // That should only happen on the first pass through the loop.
            coin_selection_params.use_bnb = true;
            coin_selection_params.m_subtract_fee_outputs = nSubtractFeeFromAmount != 0; // If we are doing subtract fee from recipient, don't use effective values

            nFeeRet = 0;
            bool pick_new_inputs = true;
            CAmount nValueIn = 0;

            // Start with no fee and loop until there is enough fee
            while (true) {
                nChangePosInOut = nChangePosRequest;
                txNew.vin.clear();
                txNew.vout.clear();
                mweb_weight = 0;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nValueToSelect += nFeeRet;

                if (!AddRecipientOutputs(txNew, vecSend, coin_selection_params, nFeeRet, nSubtractFeeFromAmount, wallet.chain(), error)) {
                    return false;
                }

                // Choose coins to use
                bool bnb_used = false;
                if (pick_new_inputs) {
                    nValueIn = 0;
                    setCoins.clear();
                    int change_spend_size = CalculateMaximumSignedInputSize(change_prototype_txout, &wallet);
                    // If the wallet doesn't know how to sign change output, assume p2sh-p2wpkh
                    // as lower-bound to allow BnB to do it's thing
                    if (change_spend_size == -1) {
                        coin_selection_params.change_spend_size = DUMMY_NESTED_P2WPKH_INPUT_SIZE;
                    } else {
                        coin_selection_params.change_spend_size = (size_t)change_spend_size;
                    }
                    if (!AttemptCoinSelection(wallet, vAvailableCoins, nValueToSelect, setCoins, nValueIn, coin_control, vecSend, coin_selection_params, bnb_used)) {
                        error = _("Insufficient funds");
                        return false;
                    }
                }

                mweb_type = MWEB::GetTxType(vecSend, std::vector<CInputCoin>(setCoins.begin(), setCoins.end()));
                change_on_mweb = IsChangeOnMWEB(wallet, mweb_type, vecSend, coin_control.destChange);

                if (mweb_type != MWEB::TxType::LTC_TO_LTC) {
                    if (mweb_type == MWEB::TxType::PEGIN || mweb_type == MWEB::TxType::MWEB_TO_MWEB) {
                        mweb_weight += Weight::STANDARD_OUTPUT_WEIGHT; // MW: FUTURE - Look at actual recipients list, but for now we only support 1 MWEB recipient.
                    }
                    
                    if (change_on_mweb) {
                        mweb_weight += Weight::STANDARD_OUTPUT_WEIGHT;
                    }

                    CScript pegout_script = (mweb_type == MWEB::TxType::PEGOUT) ? vecSend.front().GetScript() : CScript();
                    mweb_weight += Weight::CalcKernelWeight(true, pegout_script);

                    // We don't support multiple recipients for MWEB txs yet,
                    // so the only possible LTC outputs are pegins & change.
                    // Both of those are added after this, so clear outputs for now. 
                    txNew.vout.clear();
                    nChangePosInOut = -1;
                }

                const CAmount nChange = nValueIn - nValueToSelect;
                if (nChange > 0 && !change_on_mweb) {
                    // Fill a vout to ourself
                    CTxOut newTxOut(nChange, change_addr.IsMWEB() ? CScript() : change_addr.GetScript());

                    // Never create dust outputs; if we would, just
                    // add the dust to the fee.
                    // The nChange when BnB is used is always going to go to fees.
                    if (IsDust(newTxOut, coin_selection_params.m_discard_feerate) || bnb_used) {
                        nChangePosInOut = -1;
                        nFeeRet += nChange;
                    } else {
                        if (nChangePosInOut == -1) {
                            // Insert change txn at random position:
                            nChangePosInOut = GetRandInt(txNew.vout.size() + 1);
                        } else if ((unsigned int)nChangePosInOut > txNew.vout.size()) {
                            error = _("Change index out of range");
                            return false;
                        }

                        std::vector<CTxOut>::iterator position = txNew.vout.begin() + nChangePosInOut;
                        txNew.vout.insert(position, newTxOut);
                    }
                } else {
                    nChangePosInOut = -1;
                }

                // Add Dummy peg-in script
                if (ContainsPegIn(mweb_type, setCoins)) {
                    txNew.vout.push_back(CTxOut(MAX_MONEY, GetScriptForPegin(mw::Hash())));
                }

                // Dummy fill vin for maximum size estimation
                //
                for (const auto& coin : setCoins) {
                    if (!coin.IsMWEB()) {
                        txNew.vin.push_back(CTxIn(boost::get<COutPoint>(coin.GetIndex()), CScript()));
                    }
                }

                if (mweb_type != MWEB::TxType::MWEB_TO_MWEB) {
                    nBytes = CalculateMaximumSignedTxSize(CTransaction(txNew), &wallet, coin_control.fAllowWatchOnly);
                } else {
                    nBytes = 0;
                }
                if (nBytes < 0) {
                    LogPrintf("Transaction failed to sign: %s", CTransaction(txNew).ToString().c_str());
                    error = _("Signing transaction failed");
                    return false;
                }

                mweb_fee = GetMinimumFee(wallet, 0, mweb_weight, coin_control, &feeCalc);
                if (mweb_type == MWEB::TxType::PEGOUT) {
                    CTxOut pegout_output(vecSend.front().nAmount, vecSend.front().receiver.GetScript());
                    size_t pegout_weight = ::GetSerializeSize(pegout_output, PROTOCOL_VERSION) * WITNESS_SCALE_FACTOR;
                    size_t pegout_bytes = GetVirtualTransactionSize(pegout_weight, 0, 0);

                    mweb_fee = GetMinimumFee(wallet, pegout_bytes, mweb_weight, coin_control, &feeCalc);
                    nBytes += pegout_bytes;
                }

                nFeeNeeded = coin_selection_params.m_effective_feerate.GetTotalFee(nBytes, mweb_weight);

                if (nFeeRet >= nFeeNeeded) {
                    // Reduce fee to only the needed amount if possible. This
                    // prevents potential overpayment in fees if the coins
                    // selected to meet nFeeNeeded result in a transaction that
                    // requires less fee than the prior iteration.

                    // If we have no change and a big enough excess fee, then
                    // try to construct transaction again only without picking
                    // new inputs. We now know we only need the smaller fee
                    // (because of reduced tx size) and so we should add a
                    // change output. Only try this once.
                    if (nChangePosInOut == -1 && nSubtractFeeFromAmount == 0 && pick_new_inputs && (mweb_type == MWEB::TxType::LTC_TO_LTC || mweb_type == MWEB::TxType::PEGIN)) {
                        unsigned int tx_size_with_change = change_on_mweb ? nBytes : nBytes + coin_selection_params.change_output_size + 2; // Add 2 as a buffer in case increasing # of outputs changes compact size
                        CAmount fee_needed_with_change = coin_selection_params.m_effective_feerate.GetTotalFee(tx_size_with_change, mweb_weight);
                        CAmount minimum_value_for_change = GetDustThreshold(change_prototype_txout, coin_selection_params.m_discard_feerate);
                        if (nFeeRet >= fee_needed_with_change + minimum_value_for_change) {
                            pick_new_inputs = false;
                            nFeeRet = fee_needed_with_change;
                            continue;
                        }
                    }

                    // If we have change output already, just increase it
                    if (nFeeRet > nFeeNeeded && nSubtractFeeFromAmount == 0) {
                        CAmount extraFeePaid = nFeeRet - nFeeNeeded;

                        if (nChangePosInOut != -1) {
                            std::vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
                            change_position->nValue += extraFeePaid;
                            nFeeRet -= extraFeePaid;
                        } else if (change_on_mweb) {
                            nFeeRet -= extraFeePaid;
                        }
                    }

                    if (nFeeRet)
                    break; // Done, enough fee included.
                } else if (!pick_new_inputs) {
                    // This shouldn't happen, we should have had enough excess
                    // fee to pay for the new output and still meet nFeeNeeded
                    // Or we should have just subtracted fee from recipients and
                    // nFeeNeeded should not have changed
                    error = _("Transaction fee and change calculation failed");
                    return false;
                }

                // Try to reduce change to include necessary fee
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                    std::vector<CTxOut>::iterator change_position = txNew.vout.begin() + nChangePosInOut;
                    // Only reduce change if remaining amount is still a large enough output.
                    if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
                        change_position->nValue -= additionalFeeNeeded;
                        nFeeRet += additionalFeeNeeded;
                        break; // Done, able to increase fee from change
                    }
                }

                // If subtracting fee from recipients, we now know what fee we
                // need to subtract, we have no reason to reselect inputs
                if (nSubtractFeeFromAmount > 0) {
                    pick_new_inputs = false;
                }

                // Include more fee and try again.
                nFeeRet = nFeeNeeded;
                coin_selection_params.use_bnb = false;
                continue;
            }

            // Give up if change keypool ran out and change is required
            if (change_addr.IsEmpty() && nChangePosInOut != -1) {
                return false;
            }
        }

        // Shuffle selected coins and fill in final vin
        txNew.vin.clear();
        std::vector<CInputCoin> selected_coins(setCoins.begin(), setCoins.end());
        Shuffle(selected_coins.begin(), selected_coins.end(), FastRandomContext());

        // Note how the sequence number is set to non-maxint so that
        // the nLockTime set above actually works.
        //
        // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
        // we use the highest possible value in that range (maxint-2)
        // to avoid conflicting with other possible uses of nSequence,
        // and in the spirit of "smallest possible change from prior
        // behavior."
        const uint32_t nSequence = coin_control.m_signal_bip125_rbf.get_value_or(wallet.m_signal_rbf) ? MAX_BIP125_RBF_SEQUENCE : (CTxIn::SEQUENCE_FINAL - 1);
        for (const auto& coin : selected_coins) {
            if (!coin.IsMWEB()) {
                txNew.vin.push_back(CTxIn(boost::get<COutPoint>(coin.GetIndex()), CScript(), nSequence));
            }
        }

        if (!MWEB::Transact::CreateTx(wallet.GetMWWallet(), txNew, selected_coins, vecSend, nFeeRet, mweb_fee, change_on_mweb)) {
            error = _("Failed to create MWEB transaction");
            return false;
        }

        if (sign && !wallet.SignTransaction(txNew)) {
            error = _("Signing transaction failed");
            return false;
        }

        // Return the constructed transaction data.
        tx = MakeTransactionRef(std::move(txNew));

        // Limit size
        if (GetTransactionWeight(*tx) > MAX_STANDARD_TX_WEIGHT) {
            error = _("Transaction too large");
            return false;
        }
    }

    if (nFeeRet > wallet.m_default_max_tx_fee) {
        error = TransactionErrorString(TransactionError::MAX_FEE_EXCEEDED);
        return false;
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        if (!wallet.chain().checkChainLimits(tx)) {
            error = _("Transaction has too long of a mempool chain");
            return false;
        }
    }

    // Before we return success, we assume any change key will be used to prevent
    // accidental re-use.
    reservedest.KeepDestination();
    fee_calc_out = feeCalc;

    wallet.WalletLogPrintf("Fee Calculation: Fee:%d Bytes:%u Needed:%d Tgt:%d (requested %d) Reason:\"%s\" Decay %.5f: Estimation: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
        nFeeRet, nBytes, nFeeNeeded, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
        feeCalc.est.pass.start, feeCalc.est.pass.end,
        (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) > 0.0 ? 100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool) : 0.0,
        feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
        feeCalc.est.fail.start, feeCalc.est.fail.end,
        (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) > 0.0 ? 100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool) : 0.0,
        feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);
    return true;
}