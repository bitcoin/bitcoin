#include <mweb/mweb_transact.h>
#include <key_io.h>
#include <policy/policy.h>

#include <mw/models/tx/PegOutCoin.h>
#include <mw/wallet/TxBuilder.h>
#include <util/translation.h>

using namespace MWEB;

bool MWEB::ContainsPegIn(const TxType& mweb_type, const std::set<CInputCoin>& input_coins)
{
    if (mweb_type == MWEB::TxType::PEGIN) {
        return true;
    }

    if (mweb_type == MWEB::TxType::PEGOUT) {
        return std::any_of(
            input_coins.cbegin(), input_coins.cend(),
            [](const CInputCoin& coin) { return !coin.IsMWEB(); });
    }

    return false;
}

bool MWEB::IsChangeOnMWEB(const CWallet& wallet, const MWEB::TxType& mweb_type, const std::vector<CRecipient>& recipients, const CTxDestination& dest_change)
{
    if (mweb_type == MWEB::TxType::MWEB_TO_MWEB || mweb_type == MWEB::TxType::PEGOUT) {
        return true;
    }

    if (mweb_type == MWEB::TxType::PEGIN) {
        return dest_change.type() == typeid(CNoDestination) || dest_change.type() == typeid(StealthAddress);
    }

    return false;
}

uint64_t MWEB::CalcMWEBWeight(const MWEB::TxType& mweb_type, const bool change_on_mweb, const std::vector<CRecipient>& recipients)
{
    uint64_t mweb_weight = 0;

    if (mweb_type == MWEB::TxType::PEGIN || mweb_type == MWEB::TxType::MWEB_TO_MWEB) {
        mweb_weight += Weight::STANDARD_OUTPUT_WEIGHT; // MW: FUTURE - Look at actual recipients list, but for now we only support 1 MWEB recipient.
    }

    if (change_on_mweb) {
        mweb_weight += Weight::STANDARD_OUTPUT_WEIGHT;
    }

    if (mweb_type != MWEB::TxType::LTC_TO_LTC) {
        CScript pegout_script = (mweb_type == MWEB::TxType::PEGOUT) ? recipients.front().GetScript() : CScript();
        mweb_weight += Weight::CalcKernelWeight(true, pegout_script);
    }

    return mweb_weight;
}

int64_t MWEB::CalcPegOutBytes(const TxType& mweb_type, const std::vector<CRecipient>& recipients)
{
    if (mweb_type == MWEB::TxType::PEGOUT) {
        CTxOut pegout_output(recipients.front().nAmount, recipients.front().receiver.GetScript());
        int64_t pegout_weight = (int64_t)::GetSerializeSize(pegout_output, PROTOCOL_VERSION) * WITNESS_SCALE_FACTOR;
        return GetVirtualTransactionSize(pegout_weight, 0, 0);
    }

    return 0;
}

bool Transact::CreateTx(
    const std::shared_ptr<MWEB::Wallet>& mweb_wallet,
    CMutableTransaction& transaction,
    const std::vector<CInputCoin>& selected_coins,
    const std::vector<CRecipient>& recipients,
    const CAmount& total_fee,
    const CAmount& mweb_fee,
    const CAmount& ltc_change,
    const bool include_mweb_change,
    bilingual_str& error)
{
    // Add recipients
    std::vector<mw::Recipient> receivers;
    std::vector<PegOutCoin> pegouts;
    for (const CRecipient& recipient : recipients) {
        CAmount recipient_amount = recipient.nAmount;
        if (recipient.fSubtractFeeFromAmount) {
            recipient_amount -= total_fee;
        }

        if (recipient.IsMWEB()) {
            receivers.push_back(mw::Recipient{recipient_amount, recipient.GetMWEBAddress()});
        } else {
            PegOutCoin pegout_recipient(
                recipient_amount,
                recipient.receiver.GetScript()
            );
            pegouts.push_back(std::move(pegout_recipient));
        }
    }

    // Calculate pegin_amount
    boost::optional<CAmount> pegin_amount = boost::none;
    CAmount ltc_input_amount = std::accumulate(
        selected_coins.cbegin(), selected_coins.cend(), CAmount(0),
        [](CAmount amt, const CInputCoin& input) { return amt + (input.IsMWEB() ? 0 : input.GetAmount()); }
    );
    if (ltc_input_amount > 0) {
        assert(total_fee < ltc_input_amount);
        const CAmount ltc_fee = total_fee - mweb_fee;
        pegin_amount = (ltc_input_amount - (ltc_fee + ltc_change));
    }

    // Add Change
    if (include_mweb_change) {
        CAmount recipient_amount = std::accumulate(
            recipients.cbegin(), recipients.cend(), CAmount(0),
            [total_fee](CAmount amount, const CRecipient& recipient) {
                return amount + (recipient.nAmount - (recipient.fSubtractFeeFromAmount ? total_fee : 0));
            }
        );

        CAmount mweb_input_amount = std::accumulate(
            selected_coins.cbegin(), selected_coins.cend(), CAmount(0),
            [](CAmount amt, const CInputCoin& input) { return amt + (input.IsMWEB() ? input.GetAmount() : 0); }
        );

        CAmount change_amount = (pegin_amount.value_or(0) + mweb_input_amount) - (recipient_amount + mweb_fee + ltc_change);
        if (change_amount < 0) {
            error = _("MWEB change calculation failed");
            return false;
        }

        StealthAddress change_address;
        if (!mweb_wallet->GetStealthAddress(mw::CHANGE_INDEX, change_address)) {
            error = _("Failed to retrieve change stealth address");
            return false;
        }

        receivers.push_back(mw::Recipient{change_amount, change_address});
    }

    std::vector<mw::Coin> input_coins;
    for (const auto& coin : selected_coins) {
        if (coin.IsMWEB()) {
            input_coins.push_back(coin.GetMWEBCoin());
        }
    }

    std::vector<mw::Coin> output_coins;

    try {
        // Create the MWEB transaction
        transaction.mweb_tx = TxBuilder::BuildTx(
            input_coins,
            receivers,
            pegouts,
            pegin_amount,
            mweb_fee,
            output_coins
        );
    } catch (std::exception& e) {
        error = Untranslated(e.what());
        return false;
    }

    if (!output_coins.empty()) {
        mweb_wallet->SaveToWallet(output_coins);
    }

    // Update pegin output
    auto pegins = transaction.mweb_tx.GetPegIns();
    if (!pegins.empty()) {
        for (size_t i = 0; i < transaction.vout.size(); i++) {
            if (IsPegInOutput(CTransaction(transaction).GetOutput(i))) {
                transaction.vout[i].nValue = pegins.front().GetAmount();
                transaction.vout[i].scriptPubKey = GetScriptForPegin(pegins.front().GetKernelID());
                break;
            }
        }
    }

    return true;
}

mw::Recipient Transact::BuildChangeRecipient()
{
    CAmount recipient_amount = std::accumulate(
        recipients.cbegin(), recipients.cend(), CAmount(0),
        [total_fee](CAmount amount, const CRecipient& recipient) {
            return amount + (recipient.nAmount - (recipient.fSubtractFeeFromAmount ? total_fee : 0));
        }
    );

    CAmount mweb_input_amount = std::accumulate(
        selected_coins.cbegin(), selected_coins.cend(), CAmount(0),
        [](CAmount amt, const CInputCoin& input) { return amt + (input.IsMWEB() ? input.GetAmount() : 0); }
    );

    CAmount change_amount = (pegin_amount.value_or(0) + mweb_input_amount) - (recipient_amount + mweb_fee + ltc_change);
    if (change_amount < 0) {
        error = _("MWEB change calculation failed");
        return false;
    }

    StealthAddress change_address;
    if (!mweb_wallet->GetStealthAddress(mw::CHANGE_INDEX, change_address)) {
        error = _("Failed to retrieve change stealth address");
        return false;
    }

    return mw::Recipient{change_amount, change_address};
}