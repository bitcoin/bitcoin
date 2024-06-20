// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net/net_message.h>

#include <memusage.h>

size_t CSerializedNetMsg::GetMemoryUsage() const noexcept
{
    // Don't count the dynamic memory used for the m_type string, by assuming it fits in the
    // "small string" optimization area (which stores data inside the object itself, up to some
    // size; 15 bytes in modern libstdc++).
    return sizeof(*this) + memusage::DynamicUsage(data);
}
