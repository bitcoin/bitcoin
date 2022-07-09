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
        m_address_book[dest].SetLabel(label);
        if (!purpose.empty()) /* update purpose only if requested */
            m_address_book[dest].purpose = purpose;
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

CAddressBookData* AddressBookMan::GetEntry(const CTxDestination& dest)
{
    auto address_book_it = m_address_book.find(dest);
    if (address_book_it == m_address_book.end()) return nullptr;
    return &address_book_it->second;
}