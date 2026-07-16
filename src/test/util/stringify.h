/** Copyright (c) 2026-present The Bitcoin Core developers
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php. */
#ifndef BITCOIN_TEST_UTIL_STRINGIFY_H
#define BITCOIN_TEST_UTIL_STRINGIFY_H

#include <addresstype.h>
#include <crypto/hex_base.h>
#include <netaddress.h>
#include <protocol.h>
#include <test/util/framework.h>
#include <util/feefrac.h>
#include <util/string.h>

#include <concepts>
#include <string>

inline std::string stringify(const CService& v)
{
    return v.ToStringAddrPort();
}

inline std::string stringify(const CAddress& v)
{
    return v.ToStringAddrPort();
}

inline std::string stringify(const PubKeyDestination& d)
{
    return "PubKeyDestination{" + HexStr(d.GetPubKey()) + "}";
}

inline std::string stringify(const WitnessUnknown& w)
{
    return "WitnessUnknown{version=" + util::ToString(w.GetWitnessVersion()) +
           ", program=" + HexStr(w.GetWitnessProgram()) + "}";
}

inline std::string stringify(const PayToAnchor& a)
{
    return "PayToAnchor{version=" + util::ToString(a.GetWitnessVersion()) +
           ", program=" + HexStr(a.GetWitnessProgram()) + "}";
}

template <std::derived_from<FeeFrac> T>
std::string stringify(const T& f)
{
    return "FeeFrac{fee=" + util::ToString(f.fee) + ", size=" + util::ToString(f.size) + "}";
}

#endif // BITCOIN_TEST_UTIL_STRINGIFY_H
