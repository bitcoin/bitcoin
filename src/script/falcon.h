#ifndef BITCOIN_SCRIPT_FALCON_H
#define BITCOIN_SCRIPT_FALCON_H

#include <vector>
#include <cstdint>
#include <oqs/oqs.h>

#define FALCON1024_SIG_SIZE 1462        // Max signature size
#define FALCON1024_PUBKEY_SIZE 1793     // Public key size
#define FALCON1024_PRIVKEY_SIZE 2305    // Private key size

/** Real Falcon-1024 via liboqs — NIST FIPS 204 Level 5 */
bool Falcon1024Keygen(std::vector<unsigned char>& pk, std::vector<unsigned char>& sk);
bool Falcon1024Sign(const std::vector<unsigned char>& msg, 
                     const std::vector<unsigned char>& sk,
                     std::vector<unsigned char>& sig);
bool Falcon1024Verify(const std::vector<unsigned char>& sig,
                       const std::vector<unsigned char>& msg,
                       const std::vector<unsigned char>& pk);

#endif
