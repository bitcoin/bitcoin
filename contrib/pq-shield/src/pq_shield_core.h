// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef PQ_SHIELD_CORE_H
#define PQ_SHIELD_CORE_H

#include <cstdint>
#include <string>
#include <vector>

namespace pq_shield {

// Buffer sizes are derived from liboqs metadata so they track the enabled algorithm.
constexpr const char* ALG = "ML-DSA-65";

bool Keygen(std::vector<uint8_t>& public_key, std::vector<uint8_t>& secret_key);

bool Sign(std::vector<uint8_t>& signature, const std::vector<uint8_t>& message,
          const std::vector<uint8_t>& secret_key);

bool Verify(const std::vector<uint8_t>& message,
            const std::vector<uint8_t>& signature,
            const std::vector<uint8_t>& public_key);

size_t PublicKeyLen();
size_t SecretKeyLen();
size_t SignatureMaxLen();

// Only secret keys are chmod'ed; public artifacts keep default umask permissions.
bool WriteFile(const std::string& path, const std::vector<uint8_t>& data, bool secret);
bool ReadFile(const std::string& path, std::vector<uint8_t>& data);

// Secret keys additionally fail the audit when their mode exceeds 0600.
bool AuditFile(const std::string& path, bool expect_secret, std::string& error);

} // namespace pq_shield

#endif // PQ_SHIELD_CORE_H
