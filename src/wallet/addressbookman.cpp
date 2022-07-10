// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <wallet/addressbookman.h>

#include <key_io.h>
#include <logging.h>
#include <wallet/walletdb.h>

std::optional<CAddressBookData> AddressBookMan::Find(const CTxDestination& dest) const
{
    LOCK(cs_addrbook);
    const auto& address_book_it = m_address_book.find(dest);
    if (address_book_it == m_address_book.end()) return std::nullopt;
    return address_book_it->second;
}

bool AddressBookMan::Has(const CTxDestination& dest, bool allow_change) const
{
    const auto& op_entry = Find(dest);
    if (!op_entry) return false;
    return op_entry->IsChange() && allow_change;
}

bool AddressBookMan::Put(wallet::WalletBatch& batch, const CTxDestination& dest, const std::string& label, const std::string& purpose)
{
    const std::string& dest_str = EncodeDestination(dest);
    {
        LOCK(cs_addrbook);
        if (!purpose.empty() && !batch.WritePurpose(dest_str, purpose)) {
            LogPrintf("%s error writing purpose\n", __func__);
            return false;
        }
        if (!batch.WriteName(dest_str, label)) {
            LogPrintf("%s error writing name\n", __func__);
            return false;
        }

        // If db write went well, update memory
        LoadEntryLabel(dest, label);
        if (!purpose.empty()) { /* update purpose only if requested */
            LoadEntryPurpose(dest, purpose);
        }
    }

    // All good
    return true;
}

bool AddressBookMan::Delete(wallet::WalletBatch& batch, const CTxDestination& address)
{
    LOCK(cs_addrbook);
    CAddressBookData* entry = GetEntry(address);
    if (!entry) return false;

    const std::string& dest = EncodeDestination(address);
    // Delete destdata tuples associated with address
    for (const std::pair<const std::string, std::string> &item : entry->destdata) {
        if (!batch.EraseDestData(dest, item.first)) {
            LogPrintf("%s error erasing dest data\n", __func__);
            return false;
        }
    }

    if (!batch.ErasePurpose(dest) || !batch.EraseName(dest)) {
        LogPrintf("%s error erasing purpose/name\n", __func__);
        return false;
    }

    // finally, remove it from the map
    m_address_book.erase(address);
    return true;
}

void AddressBookMan::ForEachAddrBookEntry(const ListAddrBookFunc& func) const
{
    LOCK(cs_addrbook);
    for (const std::pair<const CTxDestination, CAddressBookData>& item : m_address_book) {
        const auto& entry = item.second;
        func(item.first, entry.GetLabel(), entry.purpose, entry.IsChange());
    }
}

CAddressBookData* AddressBookMan::GetEntry(const CTxDestination& dest)
{
    auto address_book_it = m_address_book.find(dest);
    if (address_book_it == m_address_book.end()) return nullptr;
    return &address_book_it->second;
}

void AddressBookMan::LoadDestData(const CTxDestination& dest, const std::string& key, const std::string& value)
{
    LOCK(cs_addrbook);
    m_address_book[dest].destdata.insert(std::make_pair(key, value));
}

void AddressBookMan::LoadEntryLabel(const CTxDestination& dest, const std::string& label)
{
    LOCK(cs_addrbook);
    m_address_book[dest].SetLabel(label);
}

void AddressBookMan::LoadEntryPurpose(const CTxDestination& dest, const std::string& purpose)
{
    LOCK(cs_addrbook);
    m_address_book[dest].purpose = purpose;
}

int AddressBookMan::GetSize() const
{
    LOCK(cs_addrbook);
    return m_address_book.size();
}

bool AddressBookMan::SetDestUsed(wallet::WalletBatch& batch, const CTxDestination& dest, bool used)
{
    const std::string key{"used"};
    if (std::get_if<CNoDestination>(&dest))
        return false;

    LOCK(cs_addrbook);
    if (!used) {
        if (auto* data = GetEntry(dest)) data->destdata.erase(key);
        return batch.EraseDestData(EncodeDestination(dest), key);
    }

    const std::string value{"1"};
    LoadDestData(dest, key, value);
    return batch.WriteDestData(EncodeDestination(dest), key, value);
}

bool AddressBookMan::IsDestUsed(const CTxDestination& dest) const
{
    const std::string key{"used"};
    if (const auto& op_entry = Find(dest)) {
        if (op_entry->destdata.find(key) != op_entry->destdata.end()) {
            return true;
        }
    }
    return false;
}
