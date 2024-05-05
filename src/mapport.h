// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MAPPORT_H
#define BITCOIN_MAPPORT_H

static constexpr bool DEFAULT_UPNP = false;

static constexpr bool DEFAULT_NATPMP = false;

enum MapPortProtoFlag : unsigned int {
    NONE = 0x00,
    UPNP = 0x01,
    PCP = 0x02,   // PCP with NAT-PMP fallback.
};

void StartMapPort(bool use_upnp, bool use_pcp);
void InterruptMapPort();
void StopMapPort();

#endif // BITCOIN_MAPPORT_H
