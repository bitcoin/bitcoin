// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "netaddress.h"
#include "tinyformat.h"
#include "utilstrencodings.h"


CSubNet::CSubNet() : valid(false) { memset(netmask, 0, sizeof(netmask)); }
CSubNet::CSubNet(const std::string &strSubnet)
{
    size_t slash = strSubnet.find_last_of('/');
    std::vector<CNetAddr> vIP;

    valid = true;
    // Default to /32 (IPv4) or /128 (IPv6), i.e. match single address
    memset(netmask, 255, sizeof(netmask));

    std::string strAddress = strSubnet.substr(0, slash);
    if (LookupHost(strAddress.c_str(), vIP, 1, false))
    {
        network = vIP[0];
        if (slash != strSubnet.npos)
        {
            std::string strNetmask = strSubnet.substr(slash + 1);
            int32_t n;
            // IPv4 addresses start at offset 12, and first 12 bytes must match, so just offset n
            const int astartofs = network.IsIPv4() ? 12 : 0;
            if (ParseInt32(strNetmask, &n)) // If valid number, assume /24 symtex
            {
                if (n >= 0 && n <= (128 - astartofs * 8)) // Only valid if in range of bits of address
                {
                    n += astartofs * 8;
                    // Clear bits [n..127]
                    for (; n < 128; ++n)
                        netmask[n >> 3] &= ~(1 << (7 - (n & 7)));
                }
                else
                {
                    valid = false;
                }
            }
            else // If not a valid number, try full netmask syntax
            {
                if (LookupHost(strNetmask.c_str(), vIP, 1, false)) // Never allow lookup for netmask
                {
                    // Copy only the *last* four bytes in case of IPv4, the rest of the mask should stay 1's as
                    // we don't want pchIPv4 to be part of the mask.
                    for (int x = astartofs; x < 16; ++x)
                        netmask[x] = vIP[0].ip[x];
                }
                else
                {
                    valid = false;
                }
            }
        }
    }
    else
    {
        valid = false;
    }

    // Normalize network according to netmask
    for (int x = 0; x < 16; ++x)
        network.ip[x] &= netmask[x];
}

CSubNet::CSubNet(const CNetAddr &addr) : valid(addr.IsValid())
{
    memset(netmask, 255, sizeof(netmask));
    network = addr;
}

bool CSubNet::Match(const CNetAddr &addr) const
{
    if (!valid || !addr.IsValid())
        return false;
    for (int x = 0; x < 16; ++x)
        if ((addr.ip[x] & netmask[x]) != network.ip[x])
            return false;
    return true;
}

static inline int NetmaskBits(uint8_t x)
{
    switch (x)
    {
    case 0x00:
        return 0;
        break;
    case 0x80:
        return 1;
        break;
    case 0xc0:
        return 2;
        break;
    case 0xe0:
        return 3;
        break;
    case 0xf0:
        return 4;
        break;
    case 0xf8:
        return 5;
        break;
    case 0xfc:
        return 6;
        break;
    case 0xfe:
        return 7;
        break;
    case 0xff:
        return 8;
        break;
    default:
        return -1;
        break;
    }
}

std::string CSubNet::ToString() const
{
    /* Parse binary 1{n}0{N-n} to see if mask can be represented as /n */
    int cidr = 0;
    bool valid_cidr = true;
    int n = network.IsIPv4() ? 12 : 0;
    for (; n < 16 && netmask[n] == 0xff; ++n)
        cidr += 8;
    if (n < 16)
    {
        int bits = NetmaskBits(netmask[n]);
        if (bits < 0)
            valid_cidr = false;
        else
            cidr += bits;
        ++n;
    }
    for (; n < 16 && valid_cidr; ++n)
        if (netmask[n] != 0x00)
            valid_cidr = false;

    /* Format output */
    std::string strNetmask;
    if (valid_cidr)
    {
        strNetmask = strprintf("%u", cidr);
    }
    else
    {
        if (network.IsIPv4())
            strNetmask = strprintf("%u.%u.%u.%u", netmask[12], netmask[13], netmask[14], netmask[15]);
        else
            strNetmask =
                strprintf("%x:%x:%x:%x:%x:%x:%x:%x", netmask[0] << 8 | netmask[1], netmask[2] << 8 | netmask[3],
                    netmask[4] << 8 | netmask[5], netmask[6] << 8 | netmask[7], netmask[8] << 8 | netmask[9],
                    netmask[10] << 8 | netmask[11], netmask[12] << 8 | netmask[13], netmask[14] << 8 | netmask[15]);
    }

    return network.ToString() + "/" + strNetmask;
}

bool CSubNet::IsValid() const { return valid; }
bool operator==(const CSubNet &a, const CSubNet &b)
{
    return a.valid == b.valid && a.network == b.network && !memcmp(a.netmask, b.netmask, 16);
}

bool operator!=(const CSubNet &a, const CSubNet &b) { return !(a == b); }
bool operator<(const CSubNet &a, const CSubNet &b)
{
    return (a.network < b.network || (a.network == b.network && memcmp(a.netmask, b.netmask, 16) < 0));
}
