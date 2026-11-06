// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <test/util/common.h>

#include <addresstype.h>
#include <addrman.h>
#include <core_io.h>
#include <crypto/hex_base.h>
#include <key.h>
#include <netaddress.h>
#include <netbase.h>
#include <protocol.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <script/script.h>
#include <streams.h>
#include <tinyformat.h>
#include <util/bip32.h>
#include <util/string.h>

#include <array>
#include <ostream>
#include <variant>

std::ostream& operator<<(std::ostream& os, const AddressPosition& pos)
{
    return os << strprintf("AddressPosition(tried=%d, multiplicity=%d, bucket=%d, position=%d)", pos.tried, pos.multiplicity, pos.bucket, pos.position);
}

std::ostream& operator<<(std::ostream& os, const CScript& script)
{
    return os << strprintf("CScript(asm=%s, hex=%s)", ScriptToAsmStr(script), HexStr(script));
}

std::ostream& operator<<(std::ostream& os, const CService& service)
{
    return os << strprintf("CService(addr=%s, net=%s)", service.ToStringAddrPort(), GetNetworkName(service.GetNetwork()));
}

std::ostream& operator<<(std::ostream& os, const CAddress& addr)
{
    return os << strprintf("CAddress(addr=%s, nTime=%s, nServices=[%s])",
                           addr.ToStringAddrPort(),
                           addr.nTime,
                           util::Join(serviceFlagsToStr(addr.nServices), ", "));
}

std::ostream& operator<<(std::ostream& os, const CExtKey& k)
{
    if (!k.key.IsValid()) return os << "CExtKey(<invalid>)";
    std::array<std::byte, BIP32_EXTKEY_SIZE> code;
    SpanWriter{code} << k;
    return os << strprintf("CExtKey(encoded=%s)", HexStr(code));
}

std::ostream& operator<<(std::ostream& os, const CExtPubKey& k)
{
    if (k.pubkey.size() != CPubKey::COMPRESSED_SIZE) return os << "CExtPubKey(<invalid>)";
    std::array<std::byte, BIP32_EXTKEY_SIZE> code;
    SpanWriter{code} << k;
    return os << strprintf("CExtPubKey(encoded=%s)", HexStr(code));
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
    return os << strprintf("CNoDestination(script=%s)", dest.GetScript());
}

std::ostream& operator<<(std::ostream& os, const PubKeyDestination& dest)
{
    return os << strprintf("PubKeyDestination(pubkey=%s)", dest.GetPubKey());
}

std::ostream& operator<<(std::ostream& os, const PKHash& h)
{
    return os << strprintf("PKHash(hash=%s)", h.ToString());
}

std::ostream& operator<<(std::ostream& os, const ScriptHash& h)
{
    return os << strprintf("ScriptHash(hash=%s)", h.ToString());
}

std::ostream& operator<<(std::ostream& os, const WitnessV0KeyHash& h)
{
    return os << strprintf("WitnessV0KeyHash(hash=%s)", h.ToString());
}

std::ostream& operator<<(std::ostream& os, const WitnessV0ScriptHash& h)
{
    return os << strprintf("WitnessV0ScriptHash(hash=%s)", h.ToString());
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
    return os << strprintf("WitnessV1Taproot(program=%s)", HexStr(t));
}

std::ostream& operator<<(std::ostream& os, const CTxDestination& dest)
{
    std::visit([&os](const auto& alt) { os << alt; }, dest);
    return os;
}

std::ostream& operator<<(std::ostream& os, const CPubKey& pk)
{
    return os << strprintf("CPubKey(pubkey=%s)", HexStr(pk));
}

std::ostream& operator<<(std::ostream& os, const XOnlyPubKey& pk)
{
    return os << strprintf("XOnlyPubKey(pubkey=%s)", HexStr(pk));
}

std::ostream& operator<<(std::ostream& os, const KeyOriginInfo& info)
{
    return os << strprintf("KeyOriginInfo(fingerprint=%s, path=%s)", HexStr(info.fingerprint), FormatHDKeypath(info.path));
}

void SimulateFileSystemError(const fs::path& test_root_dir, const fs::path& path, const std::function<void()>& fn)
{
#ifdef WIN32
    // On Windows, any open file (such as the db) prevents directory renaming,
    // so we can't simulate a filesystem error in this platform. Skip it.
    return;
#else
    // This check relies on filesystem permission manipulation to simulate I/O
    // failures. Running as root bypasses permission checks, so the OS will
    // allow directory creation and file writes even when perms are set to
    // none, making it impossible to simulate the expected failures.
    if (getuid() == 0) return;
#endif

    const fs::path root = fs::weakly_canonical(test_root_dir);
    const fs::path target = fs::weakly_canonical(path);

    // Simple sanity check: ensure target is inside the test directory.
    if (!fs::PathToString(target).starts_with(fs::PathToString(root))) {
        throw std::runtime_error("Path escapes test root directory");
    }

    if (!fs::exists(target)) {
        throw std::runtime_error("Path does not exist");
    }

    const fs::path parent = target.parent_path();
    const fs::path backup = parent / fs::PathFromString(target.filename().string() + "_bak");
    const auto old_perms = fs::status(parent).permissions();

    // Make the target disappear (file or directory)
    fs::rename(target, backup);

    // Helper to restore state (permissions + original path)
    auto restore = [&]() {
        fs::permissions(parent, old_perms, fs::perm_options::replace);
        fs::rename(backup, target);
    };

    // Block any access and dir recreation under the parent directory
    fs::permissions(parent, fs::perms::none, fs::perm_options::replace);

    try {
        fn(); // run test under simulated failure
    } catch (...) {
        restore();
        throw;
    }

    restore();
}
