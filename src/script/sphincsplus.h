// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_SPHINCSPLUS_H
#define BITCOIN_SCRIPT_SPHINCSPLUS_H

#include <uint256.h>
#include <vector>

// Forward declaration
class uint256;

class CSPHINCSKey {
public:
    std::vector<unsigned char> privkey;
    std::vector<unsigned char> pubkey;

    /**
     * @brief Generates a new SPHINCS+ key pair.
     * @return True if key generation was successful, false otherwise.
     * @note This is a stub and will require a SPHINCS+ library.
     */
    bool GenerateNewKey();

    /**
     * @brief Signs a hash with the SPHINCS+ private key.
     * @param[in] hash The hash to sign.
     * @param[out] sig The resulting signature.
     * @return True if signing was successful, false otherwise.
     * @note This is a stub and will require a SPHINCS+ library.
     */
    bool Sign(const uint256& hash, std::vector<unsigned char>& sig) const;

    /**
     * @brief Verifies a signature with the SPHINCS+ public key.
     * @param[in] hash The hash that was signed.
     * @param[in] sig The signature to verify.
     * @return True if the signature is valid, false otherwise.
     * @note This is a stub and will require a SPHINCS+ library.
     */
    bool Verify(const uint256& hash, const std::vector<unsigned char>& sig) const;
};

#endif // BITCOIN_SCRIPT_SPHINCSPLUS_H
