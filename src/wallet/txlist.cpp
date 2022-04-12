#include <wallet/txlist.h>
#include <wallet/wallet.h>
#include <key_io.h>

std::vector<WalletTxRecord> TxList::ListAll(const isminefilter& filter_ismine)
{
    std::vector<WalletTxRecord> tx_records;
    for (const auto& entry : m_wallet.mapWallet) {
        List(tx_records, entry.second, filter_ismine);
    }

    return tx_records;
}

std::vector<WalletTxRecord> TxList::List(const CWalletTx& wtx, const isminefilter& filter_ismine, const boost::optional<int>& nMinDepth, const boost::optional<std::string>& filter_label)
{
    std::vector<WalletTxRecord> tx_records;
    List(tx_records, wtx, filter_ismine);

    auto iter = tx_records.begin();
    while (iter != tx_records.end()) {
        if (iter->credit > 0) {
            // Filter received transactions
            if (nMinDepth && wtx.GetDepthInMainChain() < *nMinDepth) {
                iter = tx_records.erase(iter);
                continue;
            }

            if (filter_label) {
                std::string label;

                CTxDestination destination = DecodeDestination(iter->address);
                if (IsValidDestination(destination)) {
                    const auto* address_book_entry = m_wallet.FindAddressBookEntry(destination);
                    if (address_book_entry) {
                        label = address_book_entry->GetLabel();
                    }
                }

                if (label != *filter_label) {
                    iter = tx_records.erase(iter);
                    continue;
                }
            }
        } else {
            // Filter sent transactions
            if (filter_label) {
                iter = tx_records.erase(iter);
                continue;
            }
        }

        iter++;
    }

    return tx_records;
}

void TxList::List(std::vector<WalletTxRecord>& tx_records, const CWalletTx& wtx, const isminefilter& filter_ismine)
{
    CAmount nCredit = wtx.GetCredit(filter_ismine);
    CAmount nDebit = wtx.GetDebit(filter_ismine);
    CAmount nNet = nCredit - nDebit;

    if (!IsMine(wtx)) {
        return;
    }

    if (nNet > 0 || wtx.IsCoinBase() || wtx.IsHogEx()) {
        // Credit
        List_Credit(tx_records, wtx, filter_ismine);
    } else if (IsAllFromMe(wtx)) {
        if (IsAllToMe(wtx)) {
            // Payment to Self
            List_SelfSend(tx_records, wtx, filter_ismine);
        } else {
            // Debit
            List_Debit(tx_records, wtx, filter_ismine);
        }
    } else {
        // Mixed debit transaction, can't break down payees
        WalletTxRecord tx_record(&m_wallet, &wtx);
        tx_record.type = WalletTxRecord::Type::Other;
        tx_record.debit = nNet;
        tx_records.push_back(std::move(tx_record));
    }
}

void TxList::List_Credit(std::vector<WalletTxRecord>& tx_records, const CWalletTx& wtx, const isminefilter& filter_ismine)
{
    std::vector<CTxOutput> outputs = wtx.GetOutputs();
    for (size_t i = 0; i < outputs.size(); i++) {
        const CTxOutput& output = outputs[i];
        if (!output.IsMWEB() && output.GetScriptPubKey().IsMWEBPegin()) {
            continue;
        }

        isminetype ismine = m_wallet.IsMine(output);
        if (ismine & filter_ismine) {
            // Skip displaying hog-ex outputs when we have the MWEB transaction that contains the pegout.
            // The original MWEB transaction will be displayed instead.
            if (wtx.IsHogEx() && wtx.pegout_indices.size() > i) {
                mw::Hash kernel_id = wtx.pegout_indices[i].first;
                if (m_wallet.FindWalletTxByKernelId(kernel_id) != nullptr) {
                    continue;
                }
            }

            WalletTxRecord sub(&m_wallet, &wtx, output.GetIndex());
            sub.credit = m_wallet.GetValue(output);
            sub.involvesWatchAddress = ismine & ISMINE_WATCH_ONLY;

            if (IsAddressMine(output)) {
                // Received by Litecoin Address
                sub.type = WalletTxRecord::Type::RecvWithAddress;
                sub.address = GetAddress(output).Encode();
            } else {
                // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                sub.type = WalletTxRecord::Type::RecvFromOther;
                sub.address = wtx.mapValue.count("from") > 0 ? wtx.mapValue.at("from") : "";
            }

            if (wtx.IsCoinBase()) {
                sub.type = WalletTxRecord::Type::Generated;
            }

            tx_records.push_back(sub);
        }
    }

    // Include pegouts to addresses belonging to the wallet.
    if (wtx.tx->HasMWEBTx()) {
        for (const Kernel& kernel : wtx.tx->mweb_tx.m_transaction->GetKernels()) {
            for (size_t i = 0; i < kernel.GetPegOuts().size(); i++) {
                const PegOutCoin& pegout = kernel.GetPegOuts()[i];
                if (!(m_wallet.IsMine(DestinationAddr{pegout.GetScriptPubKey()}) & filter_ismine)) {
                    continue;
                }

                WalletTxRecord tx_record(&m_wallet, &wtx, PegoutIndex{kernel.GetKernelID(), i});
                tx_record.type = WalletTxRecord::Type::RecvWithAddress;
                tx_record.credit = pegout.GetAmount();
                tx_record.address = DestinationAddr(pegout.GetScriptPubKey()).Encode();
                tx_records.push_back(std::move(tx_record));
            }
        }
    }
}

void TxList::List_Debit(std::vector<WalletTxRecord>& tx_records, const CWalletTx& wtx, const isminefilter& filter_ismine)
{
    CAmount nTxFee = wtx.GetFee(filter_ismine);

    for (const CTxOutput& output : wtx.GetOutputs()) {
        // If the output is a peg-in script, and we have the MWEB pegin tx, then ignore the peg-in script output.
        // If it's a peg-in script, and we don't have the MWEB pegin tx, treat the output as a spend.
        mw::Hash kernel_id;
        if (!output.IsMWEB() && output.GetScriptPubKey().IsMWEBPegin(&kernel_id)) {
            if (wtx.tx->HasMWEBTx() && wtx.tx->mweb_tx.GetKernelIDs().count(kernel_id) > 0) {
                continue;
            }
        }

        if (m_wallet.IsMine(output)) {
            // Ignore parts sent to self, as this is usually the change
            // from a transaction sent back to our own address.
            continue;
        }

        WalletTxRecord tx_record(&m_wallet, &wtx, output.GetIndex());
        tx_record.debit = -m_wallet.GetValue(output);

        DestinationAddr address = GetAddress(output);
        if (!address.IsEmpty()) {
            // Sent to Litecoin Address
            tx_record.type = WalletTxRecord::Type::SendToAddress;
            tx_record.address = address.Encode();
        } else {
            // Sent to IP, or other non-address transaction like OP_EVAL
            tx_record.type = WalletTxRecord::Type::SendToOther;
            tx_record.address = wtx.mapValue.count("to") > 0 ? wtx.mapValue.at("to") : "";
        }

        /* Add fee to first output */
        if (nTxFee > 0) {
            tx_record.fee = -nTxFee;
            nTxFee = 0;
        }

        tx_records.push_back(tx_record);
    }

    if (wtx.tx->HasMWEBTx()) {
        for (const Kernel& kernel : wtx.tx->mweb_tx.m_transaction->GetKernels()) {
            for (size_t i = 0; i < kernel.GetPegOuts().size(); i++) {
                const PegOutCoin& pegout = kernel.GetPegOuts()[i];
                if (m_wallet.IsMine(DestinationAddr{pegout.GetScriptPubKey()})) {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                WalletTxRecord tx_record(&m_wallet, &wtx, PegoutIndex{kernel.GetKernelID(), i});
                tx_record.debit = -pegout.GetAmount();
                tx_record.type = WalletTxRecord::Type::SendToAddress;
                tx_record.address = DestinationAddr(pegout.GetScriptPubKey()).Encode();

                /* Add fee to first output */
                if (nTxFee > 0) {
                    tx_record.fee = -nTxFee;
                    nTxFee = 0;
                }

                tx_records.push_back(std::move(tx_record));
            }
        }
    }
}

void TxList::List_SelfSend(std::vector<WalletTxRecord>& tx_records, const CWalletTx& wtx, const isminefilter& filter_ismine)
{
    std::string address;
    for (const CTxOutput& output : wtx.GetOutputs()) {
        if (!output.IsMWEB() && output.GetScriptPubKey().IsMWEBPegin()) {
            continue;
        }

        if (!address.empty()) address += ", ";
        address += GetAddress(output).Encode();
    }

    for (const PegOutCoin& pegout : wtx.tx->mweb_tx.GetPegOuts()) {
        if (!address.empty()) address += ", ";
        address += DestinationAddr(pegout.GetScriptPubKey()).Encode();
    }

    CAmount nCredit = wtx.GetCredit(filter_ismine);
    CAmount nDebit = wtx.GetDebit(filter_ismine);
    CAmount nChange = wtx.GetChange();

    WalletTxRecord tx_record(&m_wallet, &wtx);
    tx_record.type = WalletTxRecord::SendToSelf;
    tx_record.address = address;
    tx_record.debit = -(nDebit - nChange);
    tx_record.credit = nCredit - nChange;
    tx_records.push_back(std::move(tx_record));
}

isminetype TxList::IsAddressMine(const CTxOutput& txout)
{
    CTxDestination dest;
    return m_wallet.ExtractOutputDestination(txout, dest) ? m_wallet.IsMine(dest) : ISMINE_NO;
}

DestinationAddr TxList::GetAddress(const CTxOutput& output)
{
    if (!output.IsMWEB()) {
        return output.GetTxOut().scriptPubKey;
    }

    mw::Coin coin;
    if (m_wallet.GetCoin(output.ToMWEB(), coin)) {
        StealthAddress addr;
        if (m_wallet.GetMWWallet()->GetStealthAddress(coin, addr)) {
            return addr;
        }
    }

    return DestinationAddr{};
}

bool TxList::IsAllFromMe(const CWalletTx& wtx)
{
    for (const CTxInput& input : wtx.GetInputs()) {
        if (!m_wallet.IsMine(input)) {
            return false;
        }
    }

    return true;
}

bool TxList::IsAllToMe(const CWalletTx& wtx)
{
    for (const CTxOutput& output : wtx.GetOutputs()) {
        // If we don't have the MWEB peg-in tx, then we treat it as an output not belonging to ourselves.
        mw::Hash kernel_id;
        if (!output.IsMWEB() && output.GetScriptPubKey().IsMWEBPegin(&kernel_id)) {
            if (wtx.tx->HasMWEBTx() && wtx.tx->mweb_tx.GetKernelIDs().count(kernel_id) > 0) {
                continue;
            }
        }

        if (!m_wallet.IsMine(output)) {
            return false;
        }
    }

    // Also check pegouts
    for (const PegOutCoin& pegout : wtx.tx->mweb_tx.GetPegOuts()) {
        if (!m_wallet.IsMine(DestinationAddr{pegout.GetScriptPubKey()})) {
            return false;
        }
    }

    return true;
}

// A few release candidates of v0.21.2 added some transactions to the wallet that didn't actually belong to it.
// This is a temporary band-aid to filter out these transactions from the list.
// We can consider removing it after testing, since only a limited number of testnet wallets should've been impacted.
bool TxList::IsMine(const CWalletTx& wtx)
{
    for (const CTxInput& input : wtx.GetInputs()) {
        if (m_wallet.IsMine(input)) {
            return true;
        }
    }

    for (const CTxOutput& output : wtx.GetOutputs()) {
        if (m_wallet.IsMine(output)) {
            return true;
        }
    }

    for (const PegOutCoin& pegout : wtx.tx->mweb_tx.GetPegOuts()) {
        if (m_wallet.IsMine(DestinationAddr{pegout.GetScriptPubKey()})) {
            return true;
        }
    }

    return false;
}