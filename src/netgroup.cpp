// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netgroup.h>

std::vector<unsigned char> NetGroupManager::GetGroup(const CNetAddr& address) const
{
    return address.GetGroup(m_asmap);
}

uint32_t NetGroupManager::GetMappedAS(const CNetAddr& address) const
{
    return address.GetMappedAS(m_asmap);
}
