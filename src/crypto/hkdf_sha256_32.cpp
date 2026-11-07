// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hkdf_sha256_32.h>

#include <crypto/hmac_sha256.h>
#include <span.h>
#include <support/cleanse.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>

static CHMAC_SHA256 PrepareHMAC(std::span<const std::byte> ikm, const std::string& salt)
{
    std::array<uint8_t, CHMAC_SHA256::OUTPUT_SIZE> prk;
    CHMAC_SHA256{UCharCast(salt.data()), salt.size()}.Write(UCharCast(ikm.data()), ikm.size()).Finalize(prk.data());
    const CHMAC_SHA256 hmac{prk.data(), prk.size()};
    memory_cleanse(prk.data(), prk.size());
    return hmac;
}

CHKDF_HMAC_SHA256_L32::CHKDF_HMAC_SHA256_L32(std::span<const std::byte> ikm, const std::string& salt)
    : m_hmac{PrepareHMAC(ikm, salt)}
{
}

void CHKDF_HMAC_SHA256_L32::Expand32(const std::string& info, std::span<std::byte, OUTPUT_SIZE> hash) const
{
    // The fixed output length needs only one HKDF expansion block
    assert(info.size() <= 128);
    static constexpr uint8_t one[]{1};
    CHMAC_SHA256{m_hmac}.Write(UCharCast(info.data()), info.size()).Write(one, 1).Finalize(UCharCast(hash.data()));
}
