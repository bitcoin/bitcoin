#include <mweb/mweb_transact.h>
#include <mw/models/tx/PegOutCoin.h>
#include <mw/wallet/TxBuilder.h>
#include <key_io.h>
#include <policy/policy.h>
#include <util/translation.h>
#include <wallet/txassembler.h>

using namespace MWEB;

TxType MWEB::GetTxType(const std::vector<CRecipient>& recipients, const std::set<CInputCoin>& input_coins)
{
    assert(!recipients.empty());

    static auto is_ltc = [](const CInputCoin& input) { return !input.IsMWEB(); };
    static auto is_mweb = [](const CInputCoin& input) { return input.IsMWEB(); };

    if (recipients.front().IsMWEB()) {
        // If any inputs are non-MWEB inputs, this is a peg-in transaction.
        // Otherwise, it's a simple MWEB-to-MWEB transaction.
        if (std::any_of(input_coins.cbegin(), input_coins.cend(), is_ltc)) {
            return TxType::PEGIN;
        } else {
            return TxType::MWEB_TO_MWEB;
        }
    } else {
        // If any inputs are MWEB inputs, this is a peg-out transaction.
        // NOTE: This does not exclude the possibility that it's also pegging-in in addition to the pegout.
        // Otherwise, if there are no MWEB inputs, it's a simple LTC-to-LTC transaction.
        if (std::any_of(input_coins.cbegin(), input_coins.cend(), is_mweb)) {
            return TxType::PEGOUT;
        } else {
            return TxType::LTC_TO_LTC;
        }
    }
}

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
        mweb_weight += mw::STANDARD_OUTPUT_WEIGHT; // MW: FUTURE - Look at actual recipients list, but for now we only support 1 MWEB recipient.
    }

    if (change_on_mweb) {
        mweb_weight += mw::STANDARD_OUTPUT_WEIGHT;
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

void Transact::AddMWEBTx(InProcessTx& new_tx)
{
    // Add recipients
    std::vector<mw::Recipient> receivers;
    std::vector<PegOutCoin> pegouts;
    for (const CRecipient& recipient : new_tx.recipients) {
        CAmount recipient_amount = recipient.nAmount;
        if (recipient.fSubtractFeeFromAmount) {
            recipient_amount -= new_tx.total_fee;
        }

        if (recipient_amount < 0) {
            throw CreateTxError(_("The transaction amount is too small to pay the fee"));
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

    // Lookup the change paid on the LTC side
    CAmount ltc_change = 0;
    if (new_tx.change_position != -1) {
        assert(new_tx.tx.vout.size() > (size_t)new_tx.change_position);
        ltc_change = new_tx.tx.vout[new_tx.change_position].nValue;
    }

    // Calculate pegin_amount
    boost::optional<CAmount> pegin_amount = boost::none;
    CAmount ltc_input_amount = std::accumulate(
        new_tx.selected_coins.cbegin(), new_tx.selected_coins.cend(), CAmount(0),
        [](CAmount amt, const CInputCoin& input) { return amt + (input.IsMWEB() ? 0 : input.GetAmount()); }
    );
    if (ltc_input_amount > 0) {
        const CAmount ltc_fee = new_tx.total_fee - new_tx.mweb_fee;
        assert(ltc_fee <= ltc_input_amount);
        pegin_amount = (ltc_input_amount - (ltc_fee + ltc_change));
    }

    // Add Change
    if (new_tx.change_on_mweb) {
        receivers.push_back(BuildChangeRecipient(new_tx, pegin_amount, ltc_change));
    }

    std::vector<mw::Coin> input_coins;
    for (const auto& coin : new_tx.selected_coins) {
        if (coin.IsMWEB()) {
            input_coins.push_back(coin.GetMWEBCoin());
        }
    }

    std::vector<mw::Coin> output_coins;

    try {
        // Create the MWEB transaction
        new_tx.tx.mweb_tx = TxBuilder::BuildTx(
            input_coins,
            receivers,
            pegouts,
            pegin_amount,
            new_tx.mweb_fee,
            output_coins
        );
    } catch (std::exception& e) {
        throw CreateTxError(Untranslated(e.what()));
    }

    if (!output_coins.empty()) {
        m_wallet.GetMWWallet()->SaveToWallet(output_coins);
    }

    // TxBuilder::BuildTx only builds partial coins.
    // We still need to rewind them to populate any remaining fields, like address index.
    m_wallet.GetMWWallet()->RewindOutputs(CTransaction(new_tx.tx));

    // Update pegin output
    auto pegins = new_tx.tx.mweb_tx.GetPegIns();
    if (!pegins.empty()) {
        for (size_t i = 0; i < new_tx.tx.vout.size(); i++) {
            if (IsPegInOutput(CTransaction(new_tx.tx).GetOutput(i))) {
                new_tx.tx.vout[i].nValue = pegins.front().GetAmount();
                new_tx.tx.vout[i].scriptPubKey = GetScriptForPegin(pegins.front().GetKernelID());
                break;
            }
        }
    }
}

mw::Recipient Transact::BuildChangeRecipient(const InProcessTx& new_tx, const boost::optional<CAmount>& pegin_amount, const CAmount& ltc_change)
{
    CAmount recipient_amount = std::accumulate(
        new_tx.recipients.cbegin(), new_tx.recipients.cend(), CAmount(0),
        [&new_tx](CAmount amount, const CRecipient& recipient) {
            return amount + (recipient.nAmount - (recipient.fSubtractFeeFromAmount ? new_tx.total_fee : 0));
        }
    );

    CAmount mweb_input_amount = std::accumulate(
        new_tx.selected_coins.cbegin(), new_tx.selected_coins.cend(), CAmount(0),
        [](CAmount amt, const CInputCoin& input) { return amt + (input.IsMWEB() ? input.GetAmount() : 0); }
    );

    CAmount change_amount = (pegin_amount.value_or(0) + mweb_input_amount) - (recipient_amount + new_tx.mweb_fee + ltc_change);
    if (change_amount < 0) {
        throw CreateTxError(_("MWEB change calculation failed"));
    }

    StealthAddress change_address;
    if (!m_wallet.GetMWWallet()->GetStealthAddress(mw::CHANGE_INDEX, change_address)) {
        throw CreateTxError(_("Failed to retrieve change stealth address"));
    }

    return mw::Recipient{change_amount, change_address};
}