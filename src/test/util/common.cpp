// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <test/util/common.h>

#include <addresstype.h>
#include <addrman.h>
#include <crypto/hex_base.h>
#include <index/txindex_key.h>
#include <key.h>
#include <netaddress.h>
#include <netbase.h>
#include <protocol.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <tinyformat.h>
#include <util/bip32.h>
#include <util/string.h>

#include <ostream>
#include <variant>

std::ostream& operator<<(std::ostream& os, const AddressPosition& pos)
{
    return os << strprintf("AddressPosition(tried=%d, multiplicity=%d, bucket=%d, position=%d)", pos.tried, pos.multiplicity, pos.bucket, pos.position);
}

std::ostream& operator<<(std::ostream& os, const CService& service)
{
    return os << strprintf("CService(%s, net=%s)", service.ToStringAddrPort(), GetNetworkName(service.GetNetwork()));
}

std::ostream& operator<<(std::ostream& os, const CAddress& addr)
{
    return os << strprintf("CAddress(%s, nTime=%s, nServices=[%s])",
                           addr.ToStringAddrPort(),
                           addr.nTime,
                           util::Join(serviceFlagsToStr(addr.nServices), ", "));
}

std::ostream& operator<<(std::ostream& os, const CExtKey& k)
{
    if (!k.key.IsValid()) return os << "CExtKey(<invalid>)";
    unsigned char code[BIP32_EXTKEY_SIZE];
    k.Encode(code);
    return os << strprintf("CExtKey(%s)", HexStr(code));
}

std::ostream& operator<<(std::ostream& os, const CExtPubKey& k)
{
    if (k.pubkey.size() != CPubKey::COMPRESSED_SIZE) return os << "CExtPubKey(<invalid>)";
    unsigned char code[BIP32_EXTKEY_SIZE];
    k.Encode(code);
    return os << strprintf("CExtPubKey(%s)", HexStr(code));
}

std::ostream& operator<<(std::ostream& os, const FeeFrac& ff)
{
    return os << strprintf("FeeFrac(fee=%d, size=%d)", ff.fee, ff.size);
}

std::ostream& operator<<(std::ostream& os, const ByRatioNegSize<FeeFrac>& b)
{
    return os << "ByRatioNegSize(" << static_cast<const FeeFrac&>(b) << ")";
}

std::ostream& operator<<(std::ostream& os, const CNoDestination& dest)
{
    return os << strprintf("CNoDestination(%s)", HexStr(dest.GetScript()));
}

std::ostream& operator<<(std::ostream& os, const PubKeyDestination& dest)
{
    return os << strprintf("PubKeyDestination(%s)", HexStr(dest.GetPubKey()));
}

std::ostream& operator<<(std::ostream& os, const PKHash& h)
{
    return os << strprintf("PKHash(%s)", h.ToString());
}

std::ostream& operator<<(std::ostream& os, const ScriptHash& h)
{
    return os << strprintf("ScriptHash(%s)", h.ToString());
}

std::ostream& operator<<(std::ostream& os, const WitnessV0KeyHash& h)
{
    return os << strprintf("WitnessV0KeyHash(%s)", h.ToString());
}

std::ostream& operator<<(std::ostream& os, const WitnessV0ScriptHash& h)
{
    return os << strprintf("WitnessV0ScriptHash(%s)", h.ToString());
}

std::ostream& operator<<(std::ostream& os, const WitnessUnknown& w)
{
    return os << strprintf("WitnessUnknown(version=%u, program=%s)", w.GetWitnessVersion(), HexStr(w.GetWitnessProgram()));
}

std::ostream& operator<<(std::ostream& os, const PayToAnchor& p)
{
    return os << strprintf("PayToAnchor(version=%u, program=%s)", p.GetWitnessVersion(), HexStr(p.GetWitnessProgram()));
}

std::ostream& operator<<(std::ostream& os, const WitnessV1Taproot& t)
{
    return os << strprintf("WitnessV1Taproot(%s)", HexStr(t));
}

std::ostream& operator<<(std::ostream& os, const CTxDestination& dest)
{
    std::visit([&os](const auto& alt) { os << alt; }, dest);
    return os;
}

std::ostream& operator<<(std::ostream& os, const CPubKey& pk)
{
    return os << strprintf("CPubKey(%s)", HexStr(pk));
}

std::ostream& operator<<(std::ostream& os, const KeyOriginInfo& info)
{
    return os << strprintf("KeyOriginInfo(fingerprint=%s, path=%s)", HexStr(info.fingerprint), FormatHDKeypath(info.path));
}

namespace txindex {
std::ostream& operator<<(std::ostream& os, const BlockTxPosition& pos)
{
    return os << strprintf("BlockTxPosition(block_seq=%u, tx_offset_in_block=%u)", pos.block_seq, pos.tx_offset_in_block);
}
} // namespace txindex
