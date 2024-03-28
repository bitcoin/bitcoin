// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/transport.h>
#include <memusage.h>

std::string TransportTypeAsString(TransportProtocolType transport_type)
{
    switch (transport_type) {
    case TransportProtocolType::DETECTING:
        return "detecting";
    case TransportProtocolType::V1:
        return "v1";
    case TransportProtocolType::V2:
        return "v2";
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}

size_t CSerializedNetMsg::GetMemoryUsage() const noexcept
{
    // Don't count the dynamic memory used for the m_type string, by assuming it fits in the
    // "small string" optimization area (which stores data inside the object itself, up to some
    // size; 15 bytes in modern libstdc++).
    return sizeof(*this) + memusage::DynamicUsage(data);
}
